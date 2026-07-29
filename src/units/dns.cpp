/**
 * @file: dns.cpp
 * @date: 2026-02-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля DNS-резолвера — асинхронное выполнение запросов по записям A, AAAA, MX, TXT и другим,
 *        сборка и разбор DNS-пакетов, раунд-робин по пулу серверов, кеширование результатов и контроль таймаутов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <vector>
#include <random>
#include <cerrno>
#include <cstdint>
#include <string_view>
#include <shared_mutex>
#include <unordered_set>

/**
 * Системные заголовочные файлы
 */
#include <arpa/inet.h>
#include <sys/types.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/ascii.hpp>
#include <units/dns.hpp>

/**
 * Если используется модуль IDN и операционная система не MS Windows
 */
#if AWH_IDN && !_WIN32 && !_WIN64
	/**
	 * Заголовочный файл для работы с IDN
	 */
	#include <idn2.h>
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * Если стандартные DNS-серверы IPv4 не установлены
 */
#ifndef AWH_IPV4_NS
	/**
	 * Устанавливаем стандартные DNS-серверы IPv4
	 */
	#define AWH_IPV4_NS \
		"8.8.8.8", \
		"8.8.4.4", \
		"1.1.1.1", \
		"1.0.0.1", \
		"77.88.8.8", \
		"77.88.8.1"
#endif

/**
 * Если стандартные DNS-серверы IPv6 не установлены
 */
#ifndef AWH_IPV6_NS
	/**
	 * Устанавливаем стандартные DNS-серверы IPv6
	 */
	#define AWH_IPV6_NS \
		"2001:4860:4860::8888", \
		"2001:4860:4860::8844", \
		"2606:4700:4700::1111", \
		"2606:4700:4700::1001", \
		"2A02:6B8::FEED:0FF", \
		"2A02:6B8:0:1::FEED:0FF"
#endif

/**
 * @brief Пространство имён со списком DNS-серверов
 *
 */
namespace ns {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Общий список DNS-серверов
	 *
	 */
	vector <unique_ptr <net::addr_t>> general;
};

/**
 * @brief Внутренние структуры и служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Структура записи из файла хостов
	 *
	 */
	struct HostsEntry {
		// IP-адрес из файла хостов
		string_view ip;
		// Список доменных имён
		vector <string_view> domains;
	};

	/**
	 * @brief Структура записи DNS-кэша
	 *
	 */
	struct Entry {
		// Размер IP-адреса
		uint8_t size;
		// Метка времени истечения записи (в миллисекундах)
		uint64_t life;
		// Двоичное представление IP-адреса
		uint8_t ip[0x10];
		// Доменное имя записи
		uint8_t domain[0xFF];
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Entry() noexcept :
		 size(0), life(0),
		 ip{0}, domain{0} {}
	};

	/**
	 * @brief Структура IP-адреса, связанного с доменом
	 *
	 */
	struct EntryIP {
		// Флаг локального IP-адреса
		bool local;
		// Метка времени истечения записи (в миллисекундах)
		uint64_t life;
		// Двоичное представление IP-адреса
		unique_ptr <net::addr_t> ip;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit EntryIP() noexcept : local(false), life(0), ip(nullptr) {}
	};

	/**
	 * @brief Структура записи доменного имени
	 *
	 */
	struct EntryDomain {
		// Флаг локального доменного имени
		bool local;
		// Метка времени истечения записи (в миллисекундах)
		uint64_t life;
		// Доменное имя записи
		string domain;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit EntryDomain() noexcept : local(false), life(0), domain{""} {}
	};

	/**
	 * @brief Функция сравнения двух IP-адресов
	 *
	 * @param a первый IP-адрес
	 * @param b второй IP-адрес
	 * @return  результат сравнения
	 *
	 */
	inline bool sameAddress(const net::addr_t * a, const net::addr_t * b) noexcept {
		// Если один из адресов не передан
		if((a == nullptr) || (b == nullptr))
			// Адреса не совпадают
			return false;
		// Если размеры адресов не совпадают
		if(a->size != b->size)
			// Адреса не совпадают
			return false;
		/**
		 * Определяем тип адреса
		 */
		switch(a->size){
			// Если адрес является IPv4
			case 4:
				// Сравниваем IPv4-адреса
				return (awh_cast <const net::addr_net_ipv4_t *> (a)->address == awh_cast <const net::addr_net_ipv4_t *> (b)->address);
			// Если адрес является IPv6
			case 16:
				// Сравниваем IPv6-адреса
				return !::memcmp(&awh_cast <const net::addr_net_ipv6_t *> (a)->address[0], &awh_cast <const net::addr_net_ipv6_t *> (b)->address[0], 16);
		}
		// Адреса не совпадают
		return false;
	}

	/**
	 * @brief Функция добавления или обновления IP-адреса в списке записей домена
	 *
	 * @param entries список записей домена
	 * @param record  запись для добавления или обновления
	 *
	 */
	inline void upsertEntryIP(vector <EntryIP> & entries, EntryIP && record) noexcept {
		/**
		 * Выполняем перебор всех записей домена
		 */
		for(auto & entry : entries){
			// Если IP-адрес уже присутствует в списке
			if(sameAddress(entry.ip.get(), record.ip.get())){
				// Обновляем время жизни записи
				entry.life = record.life;
				// Обновляем флаг локальной записи
				entry.local = record.local;
				// Выходим из функции
				return;
			}
		}
		// Добавляем новую запись в список
		entries.push_back(::move(record));
	}

	/**
	 * @brief Функция добавления или обновления доменного имени в списке записей IP-адреса
	 *
	 * @param entries список записей IP-адреса
	 * @param record  запись для добавления или обновления
	 *
	 */
	inline void upsertEntryDomain(vector <EntryDomain> & entries, EntryDomain && record) noexcept {
		/**
		 * Выполняем перебор всех записей IP-адреса
		 */
		for(auto & entry : entries){
			// Если доменное имя уже присутствует в списке
			if(entry.domain == record.domain){
				// Обновляем время жизни записи
				entry.life = record.life;
				// Обновляем флаг локальной записи
				entry.local = record.local;
				// Выходим из функции
				return;
			}
		}
		// Добавляем новую запись в список
		entries.push_back(::move(record));
	}

	/**
	 * @brief Структура хэш-функции для IPv6 ключа
	 *
	 * @note Использует FNV-1a алгоритм — быстрый и с хорошим распределением
	 *
	 */
	struct IpV6Hash {
		/**
		 * @brief Оператор генерации числового хэша ключа
		 *
		 * @param key ключ для которого необходима генерация
		 * @return    сгенерированный хэш ключа
		 *
		 */
		uint64_t operator()(const array <uint8_t, 16> & key) const noexcept {
			// FNV-1a 64-bit constants
			uint64_t result = 14695981039346656037ULL; // FNV offset basis
			/**
			 * Выполняем перебор всех байт ключа
			 */
			for(uint8_t byte : key){
				// Смешиваем текущий байт с накопленным значением
				result ^= static_cast <uint64_t> (byte);
				// Умножаем на константу FNV prime
				result *= 1099511628211ULL; // FNV prime
			}
			// Возвращаем результат
			return result;
		}
	};

	/**
	 * @brief Структура хэш-функции для ключа доменного имени
	 *
	 * @note Использует FNV-1a алгоритм — быстрый и с хорошим распределением
	 *
	 */
	struct DomainHash {
		/**
		 * @brief Оператор генерации числового хэша доменного имени
		 *
		 * @param domain доменное имя, для которого вычисляется хэш
		 * @return       сгенерированный хэш доменного имени
		 *
		 */
		uint64_t operator()(string_view domain) const noexcept {
			// FNV-1a 64-bit constants
			uint64_t result = 14695981039346656037ULL; // FNV offset basis
			/**
			 * Выполняем перебор всех байт доменного имени
			 */
			for(char c : domain){
				// Приводим к lowercase на лету (если ещё не нормализовали)
				char lower = awh::ascii::toLower(c);
				// Смешиваем текущий символ с накопленным значением
				result ^= static_cast <uint64_t> (lower);
				// Умножаем на константу FNV prime
				result *= 1099511628211ULL; // FNV prime
			}
			// Возвращаем результат
			return result;
		}
	};

	/**
	 * @brief Объект работы с чёрным списком
	 *
	 */
	struct Blacklist {
		// Чёрный список для блокировки IPv4-адресов
		unordered_set <uint32_t> ipv4;
		// Чёрный список для блокировки IPv6-адресов
		unordered_set <array <uint8_t, 16>, IpV6Hash> ipv6;
	} __awh_blacklist__;

	/**
	 * @brief Объект работы с кэшированием
	 *
	 */
	struct Cache {
		// Адрес файла для сохранения дампа кэша
		string filename;
		// Идентификатор таймера DNS-резолвера для сохранения кэша
		event::id_t tid;
		// Идентификатор события для загрузки локальных хостов
		event::id_t fid;
		// Интервал сохранения дампа кэша в миллисекундах
		uint32_t interval;
		// Список IPv4-адресов с доменными именами
		unordered_map <uint32_t, vector <EntryDomain>> ipv4;
		// Список доменных имён с IP-адресами
		unordered_map <string, vector <EntryIP>, DomainHash> domains;
		// Список IPv6-адресов с доменными именами
		unordered_map <array <uint8_t, 16>, vector <EntryDomain>, IpV6Hash> ipv6;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Cache() noexcept : filename{""}, tid(0), fid(0), interval(0) {}
	} __awh_cache__;
	
	/**
	 * @brief Функция вычисления абсолютного времени жизни записи кэша
	 *
	 * @param now текущая метка времени (в миллисекундах)
	 * @param ttl время жизни записи (в секундах, 0 — не кэшировать)
	 * @return    абсолютное время истечения записи (в миллисекундах)
	 *
	 */
	inline uint64_t cacheLifeFromTtl(const uint64_t now, const uint32_t ttl) noexcept {
		// Если время жизни не установлено
		if(ttl == 0)
			// Возвращаем нулевое значение (запись без срока истечения)
			return 0;
		// Возвращаем абсолютное время истечения записи
		return (now + (static_cast <uint64_t> (ttl) * 1000));
	}
};

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для рандомизации DNS-серверов
	 *
	 */
	random_device __awh_randev__;

	/**
	 * @brief Флаг одноразовой инициализации DNS-серверов
	 *
	 */
	once_flag __awh_dns_init_once__;

	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;

	/**
	 * Блокировка доступа к глобальному кэшу DNS
	 */
	static lock_state_t <std::shared_mutex> __awh_dns_cache_mutex__;

	/**
	 * Блокировка доступа к глобальному чёрному списку DNS
	 */
	static lock_state_t <std::shared_mutex> __awh_dns_blacklist_mutex__;
};

/**
 * @brief Структуры протокола DNS
 *
 */
namespace dns {
	/**
	 * @brief Бинарный буфер запроса DNS
	 *
	 */
	thread_local uint8_t buffer[0x1000];

	/**
	 * @brief Структура A-записи DNS
	 *
	 */
	typedef struct ARecord {
		// Доменное имя записи
		string name;
		// IPv4-адрес в виде 32-битного целого числа
		uint32_t ip;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit ARecord() noexcept :
		 name{""}, ip(0), ttl(0) {}
	} a_record_t;

	/**
	 * @brief Структура AAAA-записи DNS
	 *
	 */
	typedef struct AAAARecord {
		// Доменное имя, связанное с этим AAAA-записью
		string name;
		// IPv6-адрес в виде массива из 16 байт
		array <uint8_t, 16> ip;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit AAAARecord() noexcept :
		 name{""}, ip{0}, ttl(0) {}
	} aaaa_record_t;

	/**
	 * @brief Структура NS-записи DNS
	 *
	 */
	typedef struct NSRecord {
		// Доменное имя записи
		string name;
		// Доменное имя сервера имён, который авторитетен для данного домена
		string server;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit NSRecord() noexcept :
		 name{""}, server{""}, ttl(0) {}
	} ns_record_t;

	/**
	 * @brief Структура CNAME-записи DNS
	 *
	 */
	typedef struct CNAMERecord {
		// Доменное имя записи
		string name;
		// Каноническое имя записи
		string canonical;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit CNAMERecord() noexcept :
		 name{""}, canonical{""}, ttl(0) {}
	} cname_record_t;

	/**
	 * @brief Структура MX-записи DNS
	 *
	 */
	typedef struct MXRecord {
		// Доменное имя записи
		string name;
		// Доменное имя почтового сервера, который обрабатывает почту для данного домена
		string server;
		// Приоритет почтового сервера (меньшее значение означает более высокий приоритет)
		uint16_t preference;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit MXRecord() noexcept :
		 name{""}, server{""}, preference(0), ttl(0) {}
	} mx_record_t;

	/**
	 * @brief Структура TXT-записи DNS
	 *
	 */
	typedef struct TXTRecord {
		// Доменное имя записи
		string name;
		// Текстовые значения записи; может содержать несколько строк
		vector <string> texts;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit TXTRecord() noexcept :
		 name{""}, texts{}, ttl(0) {}
	} txt_record_t;

	/**
	 * @brief Структура SOA-записи DNS
	 *
	 */
	typedef struct SOARecord {
		// Доменное имя записи
		string name;
		// Доменное имя главного сервера имён для данного домена
		string mname;
		// Доменное имя ответственного лица за зону (обычно email-адрес, где "@" заменён на ".")
		string rname;
		// Время в секундах, через которое вторичные серверы должны повторить попытку получения данных зоны после неудачной попытки
		uint32_t retry;
		// Время в секундах, после которого данные зоны на вторичных серверах считаются недействительными, если не удалось обновить их с первичного сервера
		uint32_t expire;
		// Серийный номер зоны, который должен увеличиваться при каждом изменении данных зоны (обычно в формате YYYYMMDDnn, где nn — порядковый номер изменений в течение дня)
		uint32_t serial;
		// Время в секундах, через которое вторичные серверы должны обновить данные зоны, даже если серийный номер не изменился (для обеспечения актуальности данных на вторичных серверах)
		uint32_t refresh;
		// Время в секундах, которое вторичные серверы должны использовать в качестве минимального TTL для всех записей в зоне, если не указано другое значение (для обеспечения кэширования данных на вторичных серверах)
		uint32_t minimum;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit SOARecord() noexcept :
		 name{""}, mname{""}, rname{""},
		 retry(0), expire(0), serial(0),
		 refresh(0), minimum(0), ttl(0) {}
	} soa_record_t;

	/**
	 * @brief Структура PTR-записи DNS
	 *
	 */
	typedef struct PTRRecord {
		// Доменное имя записи
		string name;
		// Доменное имя, на которое указывает эта PTR-запись
		string domain;
		// Время жизни в секундах
		uint32_t ttl;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit PTRRecord() noexcept :
		 name{""}, domain{""}, ttl(0) {}
	} ptr_record_t;

	/**
	 * @brief Структура для хранения результатов DNS-запросов
	 *
	 */
	typedef struct DNSResult {
		// Список A-записей
		vector <a_record_t> a;
		// Список AAAA-записей
		vector <aaaa_record_t> aaaa;
		// Список NS-записей
		vector <ns_record_t> ns;
		// Список MX-записей
		vector <mx_record_t> mx;
		// Список TXT-записей
		vector <txt_record_t> txt;
		// Список SOA-записей
		vector <soa_record_t> soa;
		// Список PTR-записей
		vector <ptr_record_t> ptr;
		// Список CNAME-записей
		vector <cname_record_t> cname;
		/**
		 * @brief Метод очистки перед повторным использованием
		 *
		 */
		void clear() noexcept {
			// Очищаем все списки записей
			a.clear(); aaaa.clear(); ns.clear(); cname.clear();
			mx.clear(); txt.clear(); soa.clear(); ptr.clear();
		}
	} dns_result_t;

	/**
	 * @brief Структура заголовка DNS
	 *
	 */
	typedef struct Header {
		uint16_t id;        // Идентификатор операции
		uint8_t rd : 1;     // Флаг рекурсивного разрешения (Recursion Desired)
		uint8_t tc : 1;     // Флаг усечения сообщения (Truncated)
		uint8_t aa : 1;     // Флаг авторитетного ответа (Authoritative Answer)
		uint8_t opcode : 4; // Код операции (Opcode)
		uint8_t qr : 1;     // Признак запроса (0) или ответа (1)
		uint8_t rcode : 4;  // Код ответа (Response Code)
		uint8_t z : 3;      // Зарезервировано (должно быть 0)
		uint8_t ra : 1;     // Флаг поддержки рекурсии на сервере (Recursion Available)
		uint16_t qdcount;   // Число записей в секции Question
		uint16_t ancount;   // Число записей в секции Answer
		uint16_t nscount;   // Число записей в секции Authority
		uint16_t arcount;   // Число записей в секции Additional
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Header() noexcept :
		 qdcount(0), ancount(0),
		 nscount(0), arcount(0) {}
	} head_t;

	/**
	 * @brief Структура полей Question (QTYPE и QCLASS)
	 *
	 */
	typedef struct Q_Flags {
		uint16_t type; // Тип запрашиваемой записи (QTYPE)
		uint16_t cls;  // Класс запроса (QCLASS, обычно IN = 1)
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Q_Flags() noexcept : type(0), cls(0) {}
	} q_flags_t;
};

/**
 * @brief Вспомогательные функции протокола DNS
 *
 */
namespace dns {
	/**
	 * @brief Бинарный буфер парсинга доменного имени в формате DNS
	 *
	 */
	thread_local char domain[0xFF];

	/**
	 * @brief Функция генерации уникального идентификатора
	 *
	 * @return уникальный идентификатор
	 *
	 */
	static unit::dns_t::id_t identifier() noexcept {
		// Генератор случайных идентификаторов DNS-запросов
		static mt19937 generator(::__awh_randev__());
		// Диапазон допустимых идентификаторов (0 зарезервирован как недопустимый)
		static uniform_int_distribution <unit::dns_t::id_t> dist(1, 65535);
		// Возвращаем случайный идентификатор
		return dist(generator);
	}

	/**
	 * @brief Функция чтения 16-битного целого числа из буфера данных в сетевом порядке (big-endian)
	 *
	 * @param p буфер данных, из которого необходимо прочитать 16-битное число
	 * @return  16-битное число, прочитанное из буфера данных
	 *
	 */
	inline uint16_t readU16(const uint8_t * p) noexcept {
		// Читаем 16-битное число из буфера данных в сетевом порядке (big-endian)
		return ((static_cast <uint16_t> (p[0]) << 8) | p[1]);
	}

	/**
	 * @brief Функция чтения 32-битного целого числа из буфера данных в сетевом порядке (big-endian)
	 *
	 * @param p буфер данных, из которого необходимо прочитать 32-битное число
	 * @return  32-битное число, прочитанное из буфера данных
	 *
	 */
	inline uint32_t readU32(const uint8_t * p) noexcept {
		// Читаем 32-битное число из буфера данных в сетевом порядке (big-endian)
		return (
			(static_cast <uint32_t> (p[0]) << 24) |
			(static_cast <uint32_t> (p[1]) << 16) |
			(static_cast <uint32_t> (p[2]) << 8) | p[3]
		);
	}

	/**
	 * @brief Функция кодирования доменного имени в формат DNS (length-prefixed labels)
	 *
	 * @param domain доменное имя (например, "ns1.yandex.ru")
	 * @return       бинарный буфер в DNS-формате
	 *
	 */
	static vector <uint8_t> encodeDomainName(string_view domain) noexcept {
		// Переменная результата
		vector <uint8_t> result;
		// Резервируем память (примерно длина домена + запас на лейблы + null)
		result.reserve(domain.size() + 10);
		// Длина текущего лейбла
		uint8_t length = 0;
		// Позиции начала и конца текущего лейбла в доменном имени
		size_t start = 0, end = 0;
		/**
		 * Перебираем все лейблы в доменном имени
		 */
		while(start < domain.size()){
			// Ищем конец текущего лейбла (точка или конец строки)
			end = domain.find('.', start);
			// Если точка не найдена, значит это последний лейбл
			if(end == std::string_view::npos)
				// Устанавливаем конец лейбла на конец строки
				end = domain.size();
			// Получаем длину лейбла
			length = static_cast <uint8_t> (end - start);
			// Проверка: лейбл не может быть больше 63 байт (RFC 1035)
			if(length > 63)
				// Ошибка: слишком длинный лейбл
				return {};
			// Если пустой лейбл (две точки подряд) — невалидно
			if(length == 0){
				// Пропускаем или ошибка
				start = (end + 1);
				// Пропускаем пустой лейбл
				continue;
			}
			// Добавляем байт длины
			result.push_back(length);
			// Добавляем сам лейбл в результирующий буфер данных
			result.insert(
				result.end(),
				reinterpret_cast <const uint8_t *> (&domain[0] + start),
				reinterpret_cast <const uint8_t *> (&domain[0] + end)
			);
			// Переходим к следующему лейблу
			start = (end + 1);
		}
		// Возвращаем результат
		return result;
	}

	/**
	 * @brief Функция декодирования DNS-формата в строку
	 *
	 * @param buffer бинарный буфер данных в формате DNS
	 * @param size   размер буфера данных
	 * @param offset количество прочитанных байт (output)
	 * @return       доменное имя или пустая строка при ошибке
	 *
	 */
	static string decodeDomainName(const uint8_t * buffer, const size_t size, size_t & offset) noexcept {
		// Переменная результата
		string result = "";
		// Текущая позиция в буфере данных
		size_t pos = offset;
		// Длина текущего лейбла
		uint8_t length = 0;
		/**
		 * Перебираем лейблы в буфере данных
		 */
		while(pos < size){
			// Получаем длину текущего лейбла
			length = buffer[pos];
			// Нулевой байт — конец имени
			if(length == 0){
				// Выполняем смещение на 1 байт для учёта нулевого байта
				++pos;
				// Выходим из цикла, так как достигнут конец доменного имени
				break;
			}
			// Проверка на сжатие (pointer) — первые 2 бита = 11
			if((length & 0xC0) == 0xC0){
				// Указатель сжатия имени — в этой функции не поддерживается
				break;
			}
			// Проверка: лейбл не больше 63 байт
			if(length > 63)
				// Ошибка: слишком длинный лейбл
				return "";
			// Проверка: хватает ли данных
			if((pos + 1 + static_cast <size_t> (length)) > size)
				// Ошибка: недостаточно данных для чтения лейбла
				return "";
			// Добавляем точку между лейблами
			if(!result.empty())
				// Добавляем точку между лейблами
				result.push_back('.');
			// Добавляем лейбл
			result.append(reinterpret_cast <const char *> (buffer + pos + 1), length);
			// Переходим к следующему лейблу
			pos += (static_cast <size_t> (length) + 1);
		}
		// Устанавливаем количество прочитанных байт
		offset = pos;
		// Возвращаем результат
		return result;
	}

	/**
	 * @brief Функция декодирования DNS-формата в строку с поддержкой сжатия (pointer)
	 *
	 * @param packet буфер данных всего DNS-пакета для поддержки сжатия
	 * @param length размер буфера данных всего DNS-пакета
	 * @param offset количество прочитанных байт (output)
	 * @param buffer буфер для записи декодированного доменного имени
	 * @param size   размер буфера для записи декодированного доменного имени
	 * @return       результат декодирования (true при успешном декодировании, false при ошибке)
	 *
	 */
	static bool decodeDomainName(const uint8_t * packet, const size_t length, size_t & offset, char * buffer, const size_t size) noexcept {
		// Текущая позиция в буфере данных
		size_t pos = 0;
		// Смещение для обработки сжатия (pointer)
		uint16_t ptr = 0;
		// Максимальное количество прыжков по указателям, чтобы избежать бесконечных циклов при повреждённых данных
		size_t maxJumps = 10;
		// Длина текущего лейбла
		uint8_t labelSize = 0;
		// Сохраняем начальное смещение для корректного обновления offset после декодирования
		size_t originalOffset = offset;
		// Флаг, указывающий, был ли уже выполнен прыжок по указателю (для обработки сжатия)
		bool jumped = false;
		/**
		 * Перебираем лейблы в буфере данных
		 */
		while((offset < length) && (pos < size - 1)){
			// Получаем длину текущего лейбла
			labelSize = packet[offset];
			// Нулевой байт — конец имени
			if(labelSize == 0){
				// Выполняем смещение на 1 байт для учёта нулевого байта
				if(!jumped)
					// Если мы не прыгали по указателю, то обновляем offset на позицию после нулевого байта
					offset++;
				// Выходим из цикла, так как достигнут конец доменного имени
				break;
			}
			// Проверяем на сжатие (pointer) — первые 2 бита = 11
			if((labelSize & 0xC0) == 0xC0){
				// Если это pointer (сжатие), то извлекаем смещение из двух байт
				if((offset + 1) >= length)
					// Ошибка: недостаточно данных для чтения указателя
					return false;
				// Извлекаем смещение из двух байт (убирая флаги сжатия)
				ptr = (((labelSize & 0x3F) << 8) | packet[offset + 1]);
				// Если превышено допустимое число переходов по указателям
				if(maxJumps-- == 0)
					// Ошибка: слишком много прыжков по указателям, возможно повреждённые данные
					return false;
				// Если мы ещё не прыгали по указателю, сохраняем текущее смещение для корректного обновления offset после декодирования
				if(!jumped){
					// Устанавливаем флаг, что мы уже прыгали по указателю, чтобы не обновлять offset несколько раз
					jumped = true;
					// Сохраняем начальное смещение для корректного обновления offset после декодирования
					originalOffset = (offset + 2);
				}
				// Устанавливаем смещение на позицию, указанную в pointer для продолжения декодирования
				offset = ptr;
				// Продолжаем обработку с нового смещения
				continue;
			}
			// Проверка: лейбл не больше 63 байт
			if(labelSize > 63)
				// Ошибка: слишком длинный лейбл
				return false;
			// Проверка: хватает ли данных для чтения лейбла
			if((offset + 1 + static_cast <size_t> (labelSize)) > length)
				// Ошибка: недостаточно данных для чтения лейбла
				return false;
			// Если это не первый лейбл, добавляем точку между лейблами
			if((pos > 0) && (pos < (size - 1)))
				// Добавляем точку между лейблами
				buffer[pos++] = '.';
			// Проверка: хватает ли места в результирующем буфере
			if((pos + labelSize) >= size)
				// Ошибка: недостаточно места в буфере для записи лейбла
				return false;
			// Добавляем лейбл в результирующий буфер данных
			::memcpy(buffer + pos, packet + offset + 1, labelSize);
			// Переходим к следующему лейблу
			pos += static_cast <size_t> (labelSize);
			// Выполняем смещение на следующий лейбл
			offset += (static_cast <size_t> (labelSize) + 1);
		}
		// Завершаем строку нулевым байтом
		buffer[pos] = '\0';
		// Если мы прыгали по указателю
		if(jumped)
			// Устанавливаем смещение на позицию после указателя, если мы прыгали по указателю
			offset = originalOffset;
		// Возвращаем результат
		return true;
	}

	/**
	 * @brief Функция парсинга DNS-ответа из бинарного буфера данных
	 *
	 * @param buffer бинарный буфер данных, содержащий DNS-ответ
	 * @param size   размер буфера данных
	 * @param result структура для хранения результатов парсинга DNS-ответа
	 * @return       true при успешном парсинге, false при ошибке
	 *
	 */
	static bool parse(const uint8_t * buffer, const size_t size, dns_result_t & result) noexcept {
		// Очищаем результат перед заполнением новыми данными
		result.clear();
		// Проверяем, что буфер данных достаточно велик для чтения заголовка DNS (12 байт)
		if(size < 12)
			// Ошибка: недостаточно данных для чтения заголовка DNS
			return false;
		/**
		 * Читаем заголовок DNS из буфера данных
		 */
		const uint16_t qdcount = readU16(buffer + 4);
		const uint16_t ancount = readU16(buffer + 6);
		const uint16_t nscount = readU16(buffer + 8);
		const uint16_t arcount = readU16(buffer + 10);
		// Начальное смещение для чтения секций DNS после заголовка
		size_t offset = 12;
		/**
		 * Пропускаем Question section
		 */
		for(uint16_t i = 0; i < qdcount; ++i){
			// Читаем доменное имя в секции Question
			if(!decodeDomainName(buffer, size, offset, domain, sizeof(domain)))
				// Ошибка: не удалось декодировать доменное имя в секции Question
				return false;
			// Устанавливаем смещение на следующий Question (QTYPE + QCLASS)
			offset += 4;
		}
		// Время жизни для записей DNS (TTL)
		uint32_t ttl = 0;
		// Смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
		size_t rdataOffset = 0;
		// Временные переменные для чтения полей RR
		uint16_t type = 0, rdlength = 0;
		// Общее количество записей в секциях Answer, Authority и Additional
		const uint32_t total = (static_cast <uint32_t> (ancount) + static_cast <uint32_t> (nscount) + static_cast <uint32_t> (arcount));
		/**
		 * Перебираем все записи в секциях Answer, Authority и Additional
		 * И распределяем их по типам (A, AAAA, NS, CNAME, MX, TXT, SOA, PTR)
		 * Сохраняем результаты в результирующем объекте
		 */
		for(uint32_t i = 0; i < total; ++i){
			// Читаем имя записи
			if(!decodeDomainName(buffer, size, offset, domain, sizeof(domain)))
				// Ошибка: не удалось декодировать доменное имя в записи DNS
				return false;
			// Проверяем, что достаточно данных для чтения TYPE, CLASS, TTL и RDLENGTH (10 байт)
			if((offset + 10) > size)
				// Ошибка: недостаточно данных для чтения полей записи DNS
				return false;
			// Читаем заголовок RR
			type = readU16(buffer + offset);
			// Устанавливаем смещение на следующие поля после TYPE (CLASS, TTL, RDLENGTH)
			offset += 2;
			// Устанавливаем смещение на следующие поля после CLASS (TTL, RDLENGTH)
			offset += 2;
			// Читаем TTL из буфера данных
			ttl = readU32(buffer + offset);
			// Устанавливаем смещение на следующие поля после TTL (RDLENGTH)
			offset += 4;
			// Читаем RDLENGTH из буфера данных
			rdlength = readU16(buffer + offset);
			// Устанавливаем смещение на RDATA после RDLENGTH
			offset += 2;
			// Проверяем, что достаточно данных для чтения RDATA в соответствии с RDLENGTH
			if((offset + static_cast <size_t> (rdlength)) > size)
				// Ошибка: недостаточно данных для чтения RDATA в записи DNS
				return false;
			// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
			rdataOffset = offset;
			/**
			 * В зависимости от типа записи (TYPE) выполняем соответствующий парсинг RDATA и сохраняем результат в результирующем объекте
			 * Поддерживаем следующие типы записей: A (1), AAAA (28), NS (2), CNAME (5), MX (15), TXT (16), SOA (6), PTR (12)
			 * Для каждого типа выполняем проверку на корректность RDLENGTH и парсим RDATA в соответствии с форматом данного типа записи
			 * Сохраняем результаты в соответствующих списках в результирующем объекте (a, aaaa, ns, cname, mx, txt, soa, ptr)
			 * Если тип записи не поддерживается, пропускаем её без сохранения
			 */
			switch(type){
				// A-запись (IPv4)
				case 1: {
					// Проверяем, что RDLENGTH для A-записи соответствует 4 байтам (размер IPv4-адреса)
					if(rdlength == 4){
						// Объект для хранения A-записи
						a_record_t record;
						// Устанавливаем TTL для A-записи
						record.ttl = ttl;
						// Копируем 4 байта IPv4-адреса из RDATA в структуру A-записи
						::memcpy(&record.ip, buffer + rdataOffset, 4);
						// Устанавливаем доменное имя для A-записи
						record.name = domain;
						// Добавляем A-запись в список A-записей в результирующем объекте
						result.a.push_back(record);
					}
				} break;
				// AAAA-запись (IPv6)
				case 28: {
					// Проверяем, что RDLENGTH для AAAA-записи соответствует 16 байтам (размер IPv6-адреса)
					if(rdlength == 16){
						// Объект для хранения данных AAAA-записи
						aaaa_record_t record;
						// Устанавливаем TTL для AAAA-записи
						record.ttl = ttl;
						// Копируем 16 байт IPv6-адреса из RDATA в структуру AAAA-записи
						::memcpy(&record.ip[0], buffer + rdataOffset, 16);
						// Устанавливаем доменное имя для AAAA-записи
						record.name = domain;
						// Добавляем AAAA-запись в список AAAA-записей в результирующем объекте
						result.aaaa.push_back(record);
					}
				} break;
				// NS-запись (Name Server)
				case 2: {
					// Декодируем доменное имя сервера из RDATA для NS-записи и сохраняем его в результирующем объекте
					char name[0xFF];
					// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
					size_t offset = rdataOffset;
					// Декодируем доменное имя сервера из RDATA для NS-записи и сохраняем его в результирующем объекте
					if(decodeDomainName(buffer, size, offset, name, sizeof(name))){
						// Объект для хранения данных NS-записи
						ns_record_t record;
						// Устанавливаем TTL для NS-записи
						record.ttl = ttl;
						// Устанавливаем сервер для NS-записи
						record.server = name;
						// Устанавливаем доменное имя для NS-записи
						record.name = domain;
						// Добавляем NS-запись в список NS-записей в результирующем объекте
						result.ns.push_back(record);
					}
				} break;
				// CNAME-запись (Canonical Name)
				case 5: {
					// Декодируем каноническое имя из RDATA для CNAME-записи и сохраняем его в результирующем объекте
					char cname[0xFF];
					// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
					size_t offset = rdataOffset;
					// Декодируем каноническое имя из RDATA для CNAME-записи и сохраняем его в результирующем объекте
					if(decodeDomainName(buffer, size, offset, cname, sizeof(cname))){
						// Объект для хранения данных CNAME-записи
						cname_record_t record;
						// Устанавливаем TTL для CNAME-записи
						record.ttl = ttl;
						// Устанавливаем доменное имя для CNAME-записи
						record.name = domain;
						// Устанавливаем каноническое имя для CNAME-записи
						record.canonical = cname;
						// Добавляем CNAME-запись в список CNAME-записей в результирующем объекте
						result.cname.push_back(record);
					}
				} break;
				// MX-запись (Mail Exchange)
				case 15: {
					// Проверяем, что RDLENGTH для MX-записи соответствует минимум 3 байтам (2 байта для приоритета + минимум 1 байт для имени сервера)
					if(rdlength >= 3){
						// Декодируем имя почтового сервера из RDATA для MX-записи и сохраняем его в результирующем объекте
						char name[0xFF];
						// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
						size_t offset = (rdataOffset + 2);
						// Читаем приоритет почтового сервера из первых 2 байт RDATA для MX-записи
						const uint16_t pref = readU16(buffer + rdataOffset);
						// Декодируем имя почтового сервера из RDATA для MX-записи и сохраняем его в результирующем объекте
						if(decodeDomainName(buffer, size, offset, name, sizeof(name))){
							// Объект для хранения данных MX-записи
							mx_record_t record;
							// Устанавливаем TTL для MX-записи
							record.ttl = ttl;
							// Устанавливаем доменное имя для MX-записи
							record.name = domain;
							// Устанавливаем сервер для MX-записи
							record.server = name;
							// Устанавливаем приоритет для MX-записи
							record.preference = pref;
							// Добавляем MX-запись в список MX-записей в результирующем объекте
							result.mx.push_back(record);
						}
					}
				} break;
				// TXT-запись (Text)
				case 16: {
					// Объект для хранения данных TXT-записи
					txt_record_t record;
					// Устанавливаем TTL для TXT-записи
					record.ttl = ttl;
					// Устанавливаем доменное имя для TXT-записи
					record.name = domain;
					// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
					size_t offset = rdataOffset;
					// Устанавливаем конец RDATA для TXT-записи, чтобы не выходить за пределы данных при чтении текстовых строк
					size_t end = (rdataOffset + rdlength);
					/**
					 * В TXT-записи RDATA может содержать одну или несколько текстовых строк, каждая из которых начинается с байта длины, за которым следует текстовая строка.
					 * Читаем все текстовые строки, пока не достигнем конца RDATA для TXT-записи
					 */
					while(offset < end){
						// Получаем длину текущей текстовой строки из первого байта
						uint8_t length = buffer[offset++];
						// Проверяем, что длина текстовой строки не превышает оставшийся размер RDATA для TXT-записи
						if((offset + static_cast <size_t> (length)) > end)
							// Ошибка: недостаточно данных для чтения текстовой строки в RDATA для TXT-записи
							break;
						// Добавляем текстовую строку в список текстов для TXT-записи в результирующем объекте
						record.texts.emplace_back(reinterpret_cast <const char *> (buffer + offset), length);
						// Выполняем смещение на следующую текстовую строку в RDATA для TXT-записи
						offset += length;
					}
					// Если удалось прочитать хотя бы одну текстовую строку
					if(!record.texts.empty())
						// Добавляем TXT-запись в список TXT-записей в результирующем объекте
						result.txt.push_back(record);
				} break;
				// SOA-запись (Start of Authority)
				case 6: {
					// Декодируем имя главного сервера (MNAME) и имя администратора (RNAME) из RDATA для SOA-записи и сохраняем их в результирующем объекте
					char mname[256], rname[256];
					// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
					size_t offset = rdataOffset;
					// Проверяем, что RDLENGTH для SOA-записи соответствует минимум 20 байтам (минимальный размер RDATA для SOA-записи с пустыми именами)
					if(decodeDomainName(buffer, size, offset, mname, sizeof(mname)) &&
					   decodeDomainName(buffer, size, offset, rname, sizeof(rname)) && ((offset + 20) <= (rdataOffset + rdlength))){
						// Объект для хранения данных SOA-записи
						soa_record_t record;
						// Устанавливаем TTL для SOA-записи
						record.ttl = ttl;
						// Устанавливаем имя главного сервера (MNAME) для SOA-записи
						record.mname = mname;
						// Устанавливаем имя администратора (RNAME) для SOA-записи
						record.rname = rname;
						// Читаем поле SERIAL для SOA-записи из RDATA и сохраняем его в результирующем объекте
						record.serial = readU32(buffer + offset);
						// Устанавливаем смещение на следующие поля после SERIAL (REFRESH, RETRY, EXPIRE, MINIMUM)
						offset += 4;
						// Читаем поле REFRESH для SOA-записи из RDATA и сохраняем его в результирующем объекте
						record.refresh = readU32(buffer + offset);
						// Устанавливаем смещение на следующие поля после REFRESH (RETRY, EXPIRE, MINIMUM)
						offset += 4;
						// Читаем поле RETRY для SOA-записи из RDATA и сохраняем его в результирующем объекте
						record.retry = readU32(buffer + offset);
						// Устанавливаем смещение на следующие поля после RETRY (EXPIRE, MINIMUM)
						offset += 4;
						// Читаем поле EXPIRE для SOA-записи из RDATA и сохраняем его в результирующем объекте
						record.expire  = readU32(buffer + offset);
						// Устанавливаем смещение на следующие поля после EXPIRE (MINIMUM)
						offset += 4;
						// Читаем поле MINIMUM для SOA-записи из RDATA и сохраняем его в результирующем объекте
						record.minimum = readU32(buffer + offset);
						// Устанавливаем доменное имя для SOA-записи
						record.name = domain;
						// Добавляем SOA-запись в список SOA-записей в результирующем объекте
						result.soa.push_back(record);
					}
				} break;
				// PTR-запись (Pointer)
				case 12: {
					// Декодируем доменное имя из RDATA для PTR-записи и сохраняем его в результирующем объекте
					char name[256];
					// Устанавливаем смещение для чтения RDATA в каждой записи DNS после чтения заголовка RR
					size_t offset = rdataOffset;
					// Декодируем доменное имя из RDATA для PTR-записи и сохраняем его в результирующем объекте
					if(decodeDomainName(buffer, size, offset, name, sizeof(name))){
						// Объект для хранения данных PTR-записи
						ptr_record_t record;
						// Устанавливаем TTL для PTR-записи
						record.ttl = ttl;
						// Устанавливаем доменное имя для PTR-записи
						record.name = domain;
						// Устанавливаем доменное имя для PTR-записи
						record.domain = name;
						// Добавляем PTR-запись в список PTR-записей в результирующем объекте
						result.ptr.push_back(record);
					}
				} break;
			}
			// Переходим к следующей записи
			offset += rdlength;
		}
		// Возвращаем результат
		return true;
	}

	/**
	 * @brief Функция формирования DNS-запроса для указанной записи
	 *
	 * @param id     идентификатор DNS-запроса
	 * @param record тип DNS-записи для запроса (A, AAAA, NS, CNAME, MX, TXT, SOA, PTR)
	 * @param domain доменное имя
	 * @param log    объект для работы с логами
	 * @return       размер сформированного DNS-запроса или 0 при ошибке
	 *
	 */
	static size_t request(const unit::dns_t::id_t id, const unit::dns_t::record_t record, string_view domain, const log_t * log) noexcept {
		// Переменная результата
		size_t result = 0;
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если доменное имя передано
			if((id > 0) && !domain.empty()){
				// Зануляем буфер полезной нагрузки
				::memset(::dns::buffer, 0, sizeof(::dns::buffer));
				// Получаем указатель на заголовок DNS
				::dns::head_t * header = reinterpret_cast <::dns::head_t *> (::dns::buffer);
				// Устанавливаем идентификатор заголовка
				header->id = htons(id);
				/**
				 * Заполняем оставшиеся поля заголовка DNS
				 */
				header->z = 0;
				header->qr = 0;
				header->aa = 0;
				header->tc = 0;
				header->rd = 1;
				header->ra = 0;
				header->rcode = 0;
				header->opcode = 0;
				header->ancount = 0x0000;
				header->nscount = 0x0000;
				header->arcount = 0x0000;
				header->qdcount = htons(static_cast <uint16_t> (1));
				// Получаем размер запроса
				result = sizeof(::dns::head_t);
				// Получаем доменное имя в нужном формате
				const auto & fqdn = ::dns::encodeDomainName(domain);
				// Если доменное имя не удалось закодировать
				if(fqdn.empty())
					// Возвращаем нулевой размер запроса
					return 0;
				// Выполняем копирование домена
				::memcpy(&::dns::buffer[result], &fqdn[0], fqdn.size());
				// Увеличиваем размер запроса
				result += (fqdn.size() + 1);
				// Создаём части флагов вопроса пакета запроса
				::dns::q_flags_t * qflags = reinterpret_cast <::dns::q_flags_t *> (&::dns::buffer[result]);
				// Устанавливаем класс флага запроса
				qflags->cls = htons(0x0001);
				// Устанавливаем тип запроса в флагах вопроса
				qflags->type = htons(static_cast <uint16_t> (record));
				// Увеличиваем размер запроса
				result += sizeof(::dns::q_flags_t);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (record), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Возвращаем результат
		return result;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::unit::DNS::Payload::Payload() noexcept : size(0), buffer(nullptr) {}

/**
 * @brief Оператор перемещающего присваивания параметров пакета
 *
 * @param packet объект параметров пакета
 * @return       текущие параметры пакета
 *
 */
awh::unit::DNS::Packet & awh::unit::DNS::Packet::operator = (Packet && packet) noexcept {
	// Копируем время жизни из объекта параметров пакета
	this->alive = packet.alive;
	// Копируем количество попыток из объекта параметров пакета
	this->attempt = packet.attempt;
	// Копируем размер полезной нагрузки из объекта параметров пакета
	this->payload.size = packet.payload.size;
	// Перемещаем буфер полезной нагрузки из объекта параметров пакета
	this->payload.buffer = ::move(packet.payload.buffer);
	// Зануляем количество попыток в объекте параметров пакета
	packet.attempt = 0;
	// Зануляем время жизни в объекте параметров пакета
	packet.alive = 0;
	// Зануляем размер полезной нагрузки в объекте параметров пакета
	packet.payload.size = 0;
	// Зануляем буфер полезной нагрузки в объекте параметров пакета, чтобы избежать двойного освобождения памяти
	packet.payload.buffer = nullptr;
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Оператор копирующего присваивания параметров пакета
 *
 * @param packet объект параметров пакета
 * @return        текущие параметры пакета
 *
 */
awh::unit::DNS::Packet & awh::unit::DNS::Packet::operator = (const Packet & packet) noexcept {
	// Копируем время жизни из объекта параметров пакета
	this->alive = packet.alive;
	// Копируем количество попыток из объекта параметров пакета
	this->attempt = packet.attempt;
	// Копируем размер полезной нагрузки из объекта параметров пакета
	this->payload.size = packet.payload.size;
	// Выделяем новый буфер для полезной нагрузки и копируем данные из объекта параметров пакета
	this->payload.buffer = make_unique <uint8_t []> (packet.payload.size);
	// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
	::memcpy(this->payload.buffer.get(), packet.payload.buffer.get(), packet.payload.size);
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 * @param packet объект параметров пакета
 *
 */
awh::unit::DNS::Packet::Packet(Packet && packet) noexcept {
	// Копируем время жизни из объекта параметров пакета
	this->alive = packet.alive;
	// Копируем количество попыток из объекта параметров пакета
	this->attempt = packet.attempt;
	// Копируем размер полезной нагрузки из объекта параметров пакета
	this->payload.size = packet.payload.size;
	// Перемещаем буфер полезной нагрузки из объекта параметров пакета
	this->payload.buffer = ::move(packet.payload.buffer);
	// Зануляем время жизни в объекте параметров пакета
	packet.alive = 0;
	// Зануляем количество попыток в объекте параметров пакета
	packet.attempt = 0;
	// Зануляем размер полезной нагрузки в объекте параметров пакета
	packet.payload.size = 0;
	// Зануляем буфер полезной нагрузки в объекте параметров пакета, чтобы избежать двойного освобождения памяти
	packet.payload.buffer = nullptr;
}
/**
 * @brief Конструктор копирования
 *
 * @param packet объект параметров пакета
 *
 */
awh::unit::DNS::Packet::Packet(const Packet & packet) noexcept {
	// Копируем время жизни из объекта параметров пакета
	this->alive = packet.alive;
	// Копируем количество попыток из объекта параметров пакета
	this->attempt = packet.attempt;
	// Копируем размер полезной нагрузки из объекта параметров пакета
	this->payload.size = packet.payload.size;
	// Выделяем новый буфер для полезной нагрузки и копируем данные из объекта параметров пакета
	this->payload.buffer = make_unique <uint8_t []> (packet.payload.size);
	// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
	::memcpy(this->payload.buffer.get(), packet.payload.buffer.get(), packet.payload.size);
}
/**
 * @brief Конструктор
 *
 */
awh::unit::DNS::Packet::Packet() noexcept : alive(0), attempt(0) {}

/**
 * @brief Метод очистки очереди идентификаторов событий
 *
 */
void awh::unit::DNS::SimpleQueue::clear() noexcept {
	// Очищаем очередь идентификаторов событий, используя swap с пустой очередью для эффективной очистки
	std::queue <event::id_t> ().swap(this->_ids);
}
/**
 * @brief Метод получения размера очереди идентификаторов событий
 *
 * @return размер очереди идентификаторов событий
 *
 */
size_t awh::unit::DNS::SimpleQueue::size() const noexcept {
	// Возвращаем размер очереди идентификаторов событий
	return this->_ids.size();
}
/**
 * @brief Метод добавления идентификатора события в очередь
 *
 * @param eid идентификатор события для добавления в очередь
 *
 */
void awh::unit::DNS::SimpleQueue::push(event::id_t eid) noexcept {
	// Добавляем идентификатор события в очередь
    this->_ids.push(eid);
}
/**
 * @brief Метод извлечения идентификатора события из очереди
 *
 * @param eid идентификатор события для извлечения из очереди
 * @return    результат извлечения идентификатора
 *
 */
bool awh::unit::DNS::SimpleQueue::pop(event::id_t & eid) noexcept {
	// Проверяем, что очередь идентификаторов событий пуста
	if(this->_ids.empty())
		// Ошибка: очередь идентификаторов событий пуста, нет идентификатора для извлечения
		return false;
	// Извлекаем идентификатор события из очереди
	eid = this->_ids.front();
	// Удаляем извлечённый идентификатор события из очереди
	this->_ids.pop();
	// Возвращаем результат
	return true;
}
/**
 * @brief Метод удаления идентификатора события из очереди
 *
 * @param eid идентификатор события для удаления из очереди
 *
 */
void awh::unit::DNS::SimpleQueue::remove(const event::id_t eid) noexcept {
	// Временная очередь идентификаторов событий
	std::queue <event::id_t> tmp;
	/**
	 * Переносим идентификаторы, пропуская удаляемый
	 */
	while(!this->_ids.empty()){
		// Получаем идентификатор события из очереди
		const event::id_t front = this->_ids.front();
		// Удаляем идентификатор события из очереди
		this->_ids.pop();
		// Если идентификатор события не совпадает с удаляемым
		if(front != eid)
			// Добавляем идентификатор события во временную очередь
			tmp.push(front);
	}
	// Заменяем текущую очередь временной
	this->_ids.swap(tmp);
}
/**
 * @brief Конструктор
 *
 */
awh::unit::DNS::SimpleQueue::SimpleQueue() noexcept {}

/**
 * @brief Метод инициализации списка DNS-серверов из переменных окружения или стандартных значений
 *
 */
void awh::unit::DNS::Servers::init() noexcept {
	/**
	 * Выполняем перебор всех общих серверов
	 */
	for(const auto & server : ::ns::general){
		/**
		 * Определяем тип адреса
		 */
		switch(server->size){
			// Если адрес является IPv4
			case 4: {
				// Активируем флаг инициализации списка DNS-серверов IPv4
				this->_initializedIPv4 = true;
				// Создаём объект IP-адреса для хранения IPv4-адреса
				unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
				// Копируем IP-адрес из DNS-сервера в объект IP-адреса
				awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (server.get())->address;
				// Добавляем сервер в список DNS-серверов для выполнения запросов
				this->_ipv4.push_back(::move(ip));
			} break;
			// Если адрес является IPv6
			case 16: {
				// Активируем флаг инициализации списка DNS-серверов IPv6
				this->_initializedIPv6 = true;
				// Создаём объект IP-адреса для хранения IPv6-адреса
				unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
				// Копируем IP-адрес из DNS-сервера в объект IP-адреса
				::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (server.get())->address[0], 16);
				// Добавляем сервер в список DNS-серверов для выполнения запросов
				this->_ipv6.push_back(::move(ip));
			} break;
		}
	}
}
/**
 * @brief Метод сброса списка DNS-серверов
 *
 * @param family семейство IP-адресов IPv4/IPv6
 *
 */
void awh::unit::DNS::Servers::reset(const event::family_t family) noexcept {
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Сбрасываем индекс текущего DNS-сервера
			this->_indexIPv4 = 0;
			// Выполняем очистку списка DNS-серверов
			this->_ipv4.clear();
			/**
			 * Выполняем перебор всех общих серверов
			 */
			for(const auto & server : ::ns::general){
				// Если адрес является IPv4
				if(server->size == 4){
					// Активируем флаг инициализации списка DNS-серверов IPv4
					this->_initializedIPv4 = true;
					// Создаём объект IP-адреса для хранения IPv4-адреса
					unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
					// Копируем IP-адрес из DNS-сервера в объект IP-адреса
					awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (server.get())->address;
					// Добавляем сервер в список DNS-серверов для выполнения запросов
					this->_ipv4.push_back(::move(ip));
				}
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Сбрасываем индекс текущего DNS-сервера
			this->_indexIPv6 = 0;
			// Выполняем очистку списка DNS-серверов
			this->_ipv6.clear();
			/**
			 * Выполняем перебор всех общих серверов
			 */
			for(const auto & server : ::ns::general){
				// Если адрес является IPv6
				if(server->size == 16){
					// Активируем флаг инициализации списка DNS-серверов IPv6
					this->_initializedIPv6 = true;
					// Создаём объект IP-адреса для хранения IPv6-адреса
					unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
					// Копируем IP-адрес из DNS-сервера в объект IP-адреса
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (server.get())->address[0], 16);
					// Добавляем сервер в список DNS-серверов для выполнения запросов
					this->_ipv6.push_back(::move(ip));
				}
			}
		} break;
	}
}
/**
 * @brief Метод получения текущего DNS-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @return       объект DNS-сервера для выполнения запроса
 *
 */
const awh::net::addr_t * awh::unit::DNS::Servers::get(const event::family_t family) noexcept {
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Если список DNS-серверов не пустой
			if(!this->_ipv4.empty()){
				// Получаем текущий DNS-сервер из списка по индексу
				const net::addr_t * server = this->_ipv4[this->_indexIPv4].get();
				// Увеличиваем индекс для следующего запроса, циклически возвращаясь к началу списка при достижении конца
				this->_indexIPv4 = ((this->_indexIPv4 + 1) % this->_ipv4.size());
				// Возвращаем текущий DNS-сервер для выполнения запроса
				return server;
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Если список DNS-серверов не пустой
			if(!this->_ipv6.empty()){
				// Получаем текущий DNS-сервер из списка по индексу
				const net::addr_t * server = this->_ipv6[this->_indexIPv6].get();
				// Увеличиваем индекс для следующего запроса, циклически возвращаясь к началу списка при достижении конца
				this->_indexIPv6 = ((this->_indexIPv6 + 1) % this->_ipv6.size());
				// Возвращаем текущий DNS-сервер для выполнения запроса
				return server;
			}
		} break;
	}
	// Возвращаем пустой результат
	return nullptr;
}
/**
 * @brief Метод добавления DNS-сервера в список
 *
 * @param server объект DNS-сервера для добавления в список
 *
 */
void awh::unit::DNS::Servers::push(const net::addr_t * server) noexcept {
	/**
	 * Определяем тип адреса
	 */
	switch(server->size){
		// Если адрес является IPv4
		case 4: {
			// Если список DNS-серверов IPv4 уже инициализирован
			if(this->_initializedIPv4){
				// Снимаем флаг инициализации списка DNS-серверов IPv4, так как мы будем его переинициализировать
				this->_initializedIPv4 = false;
				// Сбрасываем индекс текущего DNS-сервера
				this->_indexIPv4 = 0;
				// Выполняем очистку списка DNS-серверов
				this->_ipv4.clear();
			}
			// Создаём объект IP-адреса для хранения IPv4-адреса
			unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
			// Копируем IP-адрес из DNS-сервера в объект IP-адреса
			awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (server)->address;
			// Добавляем сервер в список DNS-серверов для выполнения запросов
			this->_ipv4.push_back(::move(ip));
		} break;
		// Если адрес является IPv6
		case 16: {
			// Если список DNS-серверов IPv6 уже инициализирован
			if(this->_initializedIPv6){
				// Снимаем флаг инициализации списка DNS-серверов IPv6, так как мы будем его переинициализировать
				this->_initializedIPv6 = false;
				// Сбрасываем индекс текущего DNS-сервера
				this->_indexIPv6 = 0;
				// Выполняем очистку списка DNS-серверов
				this->_ipv6.clear();
			}
			// Создаём объект IP-адреса для хранения IPv6-адреса
			unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
			// Копируем IP-адрес из DNS-сервера в объект IP-адреса
			::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (server)->address[0], 16);
			// Добавляем сервер в список DNS-серверов для выполнения запросов
			this->_ipv6.push_back(::move(ip));
		} break;
	}
}
/**
 * @brief Конструктор
 *
 */
awh::unit::DNS::Servers::Servers() noexcept :
 _indexIPv4(0), _indexIPv6(0),
 _initializedIPv4(false), _initializedIPv6(false) {}

/**
 * @brief Конструктор
 *
 */
awh::unit::DNS::Resolver::Resolver() noexcept :
 prefix{AWH_SHORT_NAME},
 port(53), delay(5000),
 sourceIPv4(nullptr), sourceIPv6(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::unit::DNS::Transfer::Transfer() noexcept : attempts(3), maxPackets(200) {}

/**
 * @brief Метод сохранения дампа DNS-кэша в файл
 *
 * @param tid    идентификатор таймера DNS-резолвера
 * @param status статус события таймера DNS-резолвера
 *
 */
void awh::unit::DNS::dumping([[maybe_unused]] const event::id_t, const event::status_t status) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Количество добавленных записей для статистики
			uint32_t count = 0;
			// Имя файла для сохранения дампа кэша
			string filename = "";
			// Бинарный контейнер для сериализации дампа вне блокировки кэша
			binbox_t dumpBox(this->_fmk, this->_log);
			{
				// Блокируем доступ к глобальному кэшу DNS
				const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Если кэш DNS-резолвера не пустой
				if(!::__awh_cache__.domains.empty()){
					// Получаем текущую метку времени
					const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					// Сохраняем имя файла дампа кэша
					filename = ::__awh_cache__.filename;
					// Очищаем бинарный контейнер для хранения кэша доменных имён
					dumpBox.clear();
					// Добавляем в контейнер метку времени сохранения кэша
					dumpBox.add("TIMESTAMP", now);
					// Создаём объект записи для добавления в контейнер
					Entry record{};
					/**
					 * Выполняем перебор всего списка доменных имён с IP-адресами
					 */
					for(const auto & [domain, ips] : __awh_cache__.domains){
						// Размер доменного имени
						size_t size = 0;
						/**
						 * Выполняем перебор всех IP-адресов доменного имени
						 */
						for(auto i = ips.begin(); i != ips.end();){
							// Проверяем устарела ли запись в кэше
							if((i->life > 0) && (i->life <= now))
								// Если запись в кэше устарела, удаляем её
								i = const_cast <vector <EntryIP> &> (ips).erase(i);
							// Если запись в кэше не является локальной
							else if(!i->local) {
								// Определяем размер доменного имени
								size = ::min(domain.size(), sizeof(record.domain) - 1);
								// Зануляем буфер доменного имени
								::memset(record.domain, 0, sizeof(record.domain));
								// Копируем доменное имя в запись
								::strncpy(reinterpret_cast <char *> (record.domain), domain.data(), size);
								// Устанавливаем завершающий нулевой байт в доменном имени
								record.domain[size] = '\0';
								// Устанавливаем время жизни записи
								record.life = i->life;
								// Устанавливаем размер IP-адреса в записи
								record.size = static_cast <uint8_t> (i->ip->size);
								/**
								 * Определяем тип адреса
								 */
								switch(i->ip->size){
									// Если адрес является IPv4
									case 4:
										// Копируем IP-адрес в запись
										::memcpy(record.ip, &awh_cast <net::addr_net_ipv4_t *> (i->ip.get())->address, record.size);
									break;
									// Если адрес является IPv6
									case 16:
										// Копируем IP-адрес в запись
										::memcpy(record.ip, &awh_cast <net::addr_net_ipv6_t *> (i->ip.get())->address, record.size);
									break;
								}
								// Добавляем запись в контейнер
								dumpBox.add(this->_fmk->format("RECORD_%u", count++), &record, sizeof(record));
								// Продолжаем перебор кэша
								++i;
							// Продолжаем перебор кэша
							} else ++i;
						}
					}
				}
			}
			// Если есть записи для сохранения в кэше
			if((count > 0) && !filename.empty()){
				// Добавляем в контейнер количество доменных имён с IP-адресами
				dumpBox.add("COUNT", count);
				// Сохраняем кэш доменных имён в файл
				dumpBox.save(filename);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод очистки устаревших записей DNS-кэша
 *
 * @param tid    идентификатор таймера DNS-резолвера
 * @param status статус события таймера DNS-резолвера
 *
 */
void awh::unit::DNS::collector([[maybe_unused]] const event::id_t, const event::status_t status) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Получаем текущую метку времени
			const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			// Если в кэше есть IPv4-адреса
			if(!::__awh_cache__.ipv4.empty()){
				/**
				 * Выполняем перебор всех IP-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
					/**
					 * Выполняем перебор всех записей IP-адреса
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если время жизни записи IP-адреса истекло и запись не является локальной
						if(!j->local && (j->life > 0) && (j->life <= now))
							// Удаляем запись IP-адреса из кэша
							j = i->second.erase(j);
						// Если IP-адрес является локальным
						else ++j;
					}
					// Если после удаления всех записей IP-адреса в кэше не осталось
					if(i->second.empty())
						// Удаляем IP-адрес из кэша
						i = ::__awh_cache__.ipv4.erase(i);
					// Если у IP-адреса остались записи
					else ++i;
				}
			}
			// Если в кэше есть IPv6-адреса
			if(!::__awh_cache__.ipv6.empty()){
				/**
				 * Выполняем перебор всех IP-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
					/**
					 * Выполняем перебор всех записей IP-адреса
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если время жизни записи IP-адреса истекло и запись не является локальной
						if(!j->local && (j->life > 0) && (j->life <= now))
							// Удаляем запись IP-адреса из кэша
							j = i->second.erase(j);
						// Если IP-адрес является локальным
						else ++j;
					}
					// Если после удаления всех записей IP-адреса в кэше не осталось
					if(i->second.empty())
						// Удаляем IP-адрес из кэша
						i = ::__awh_cache__.ipv6.erase(i);
					// Если у IP-адреса остались записи
					else ++i;
				}
			}
			// Если в кэше есть доменные имена
			if(!::__awh_cache__.domains.empty()){
				/**
				 * Выполняем перебор всех доменных имён в кэше
				 */
				for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
					/**
					 * Выполняем перебор всех записей доменного имени
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если время жизни записи доменного имени истекло и запись не является локальной
						if(!j->local && (j->life > 0) && (j->life <= now))
							// Удаляем запись доменного имени из кэша
							j = i->second.erase(j);
						// Если доменное имя является локальным
						else ++j;
					}
					// Если после удаления всех записей доменного имени в кэше не осталось
					if(i->second.empty())
						// Удаляем доменное имя из кэша
						i = ::__awh_cache__.domains.erase(i);
					// Если у доменного имени остались записи
					else ++i;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки событий загрузки локальных хостов
 *
 * @param      идентификатор события загрузки локальных хостов
 * @param data данные события загрузки локальных хостов
 * @param size размер данных события загрузки локальных хостов
 *
 */
void awh::unit::DNS::hosts(const event::id_t, const uint8_t * data, const size_t size) noexcept {
	// Если данные события загрузки локальных хостов не пустые
	if((data != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);

			/**
			 * Сначала удаляем все локальные записи из кэша, чтобы не было конфликтов с новыми данными
			 */

			// Если в кэше есть IPv4-адреса
			if(!::__awh_cache__.ipv4.empty()){
				/**
				 * Выполняем перебор всех IP-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
					/**
					 * Выполняем перебор всех записей доменного имени
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя является локальным
						if(j->local)
							// Удаляем запись доменного имени из кэша
							j = i->second.erase(j);
						// Если доменное имя не является локальным
						else ++j;
					}
					// Если после удаления всех записей доменного имени в кэше не осталось
					if(i->second.empty())
						// Удаляем доменное имя из кэша
						i = ::__awh_cache__.ipv4.erase(i);
					// Если у доменного имени остались записи
					else ++i;
				}
			}
			// Если в кэше есть IPv6-адреса
			if(!::__awh_cache__.ipv6.empty()){
				/**
				 * Выполняем перебор всех IP-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
					/**
					 * Выполняем перебор всех записей доменного имени
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя является локальным
						if(j->local)
							// Удаляем запись доменного имени из кэша
							j = i->second.erase(j);
						// Если доменное имя не является локальным
						else ++j;
					}
					// Если после удаления всех записей доменного имени в кэше не осталось
					if(i->second.empty())
						// Удаляем доменное имя из кэша
						i = ::__awh_cache__.ipv6.erase(i);
					// Если у доменного имени остались записи
					else ++i;
				}
			}
			// Если в кэше есть доменные имена
			if(!::__awh_cache__.domains.empty()){
				/**
				 * Выполняем перебор всех доменных имён в кэше
				 */
				for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
					/**
					 * Выполняем перебор всех записей доменного имени
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя является локальным
						if(j->local)
							// Удаляем запись доменного имени из кэша
							j = i->second.erase(j);
						// Если доменное имя не является локальным
						else ++j;
					}
					// Если после удаления всех записей доменного имени в кэше не осталось
					if(i->second.empty())
						// Удаляем доменное имя из кэша
						i = ::__awh_cache__.domains.erase(i);
					// Если у доменного имени остались записи
					else ++i;
				}
			}
			/**
			 * @brief Функция парсинга строки из файла хостов
			 *
			 * @param str строка из файла хостов для парсинга
			 *
			 */
			auto parseStrHosts = [this](string_view str) noexcept -> void {
				/**
				 * Выполняем перехват ошибок
				 */
				try {
					// Создаём объект для хранения данных из строки файла хостов
					HostsEntry entry;
					// 1. Убираем комментарий (всё после '#')
					if(auto pos = str.find('#'); pos != string_view::npos)
						// Оставляем только часть строки до комментария
						str = str.substr(0, pos);
					// Начальная и конечная позиция для парсинга строки
					size_t start = 0, end = 0;
					/**
					 * 2. Проходим по строке и собираем токены
					 */
					while(start < str.size()){
						/**
						 * Пропускаем пробелы/табы
						 */
						while((start < str.size()) && ((str[start] == ' ') || (str[start] == '\t')))
							// Смещаем начальную позицию на следующий символ
							++start;
						// Если достигнут конец строки после пропуска пробелов/табов
						if(start >= str.size())
							// Прекращаем парсинг строки
							break;
						// Ищем конец токена
						end = start;
						/**
						 * Токен заканчивается пробелом, табом или концом строки
						 */
						while((end < str.size()) && (str[end] != ' ') && (str[end] != '\t'))
							// Смещаем конечную позицию на следующий символ
							++end;
						// Получаем токен
						auto token = str.substr(start, end - start);
						// Первый токен — IP, остальные — домены
						if(entry.ip.empty())
							// Устанавливаем IP-адрес в запись
							entry.ip = token;
						// Добавляем домен в запись
						else entry.domains.push_back(token);
						// Смещаем начальную позицию на конец текущего токена для поиска следующего
						start = end;
					}
					// Если IP-адрес и доменные имена успешно извлечены из строки файла хостов
					if(!entry.ip.empty() && !entry.domains.empty()){
						/**
						 * Выполняем перебор всех доменных имён, связанных с IP-адресом
						 */
						for(auto & domain : entry.domains){
							// Распарсенный адрес из файла hosts
							unique_ptr <net::addr_t> parsed = nullptr;
							// Тип адреса из результата парсинга
							net_addr_t::type_t type = net_addr_t::type_t::NONE;
							// Выполняем парсинг IP-адреса
							if(this->_addr.parse(entry.ip)){
								// Получаем тип адреса из результата парсинга
								type = this->_addr.type();
								// Получаем распарсенный IP-адрес
								parsed = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							}
							// Если парсинг IP-адреса не выполнен
							if(parsed == nullptr)
								// Пропускаем эту запись и переходим к следующей
								continue;
							// Выполняем поиск доменного имени в кэше
							auto i = ::__awh_cache__.domains.find(string{domain});
							// Если в кэше доменное имя найдено
							if(i != ::__awh_cache__.domains.end()){
								// Создаём объект записи
								EntryIP record{};
								// Помечаем IP-адрес как локальный
								record.local = true;
								/**
								 * Определяем тип адреса
								 */
								switch(static_cast <uint8_t> (type)){
									// Если адрес является IPv4
									case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
										// Создаём объект IP-адреса для хранения IPv4-адреса
										record.ip = make_unique <net::addr_net_ipv4_t> ();
										// Копируем IP-адрес из результата парсинга в запись
										awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (parsed.get())->address;
										// Выполняем поиск IP-адреса
										auto i = ::__awh_cache__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address);
										// Если IP-адрес найден в кэше
										if(i != ::__awh_cache__.ipv4.end()){
											// Создаём объект записи
											EntryDomain record{};
											// Помечаем IP-адрес как локальный
											record.local = true;
											// Устанавливаем доменное имя
											record.domain = domain;
											// Выполняем добавление IP-адреса
											i->second.push_back(::move(record));
										// Если IP-адрес не найден в кэше
										} else {
											// Создаём список записей IP-адресов
											vector <EntryDomain> entry(1);
											// Помечаем IP-адрес как локальный
											entry.back().local = true;
											// Устанавливаем доменное имя
											entry.back().domain = domain;
											// Добавляем новую запись в кэш IP-адресов
											::__awh_cache__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address, ::move(entry));
										}
									} break;
									// Если адрес является IPv6
									case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
										// Создаём объект IP-адреса для хранения IPv6-адреса
										record.ip = make_unique <net::addr_net_ipv6_t> ();
										// Копируем IP-адрес из результата парсинга в запись
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (parsed.get())->address[0], 16);
										// Выполняем поиск IP-адреса
										auto i = ::__awh_cache__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address);
										// Если IP-адрес найден в кэше
										if(i != ::__awh_cache__.ipv6.end()){
											// Создаём объект записи
											EntryDomain record{};
											// Помечаем IP-адрес как локальный
											record.local = true;
											// Устанавливаем доменное имя
											record.domain = domain;
											// Выполняем добавление IP-адреса
											i->second.push_back(::move(record));
										// Если IP-адрес не найден в кэше
										} else {
											// Создаём список записей IP-адресов
											vector <EntryDomain> entry(1);
											// Помечаем IP-адрес как локальный
											entry.back().local = true;
											// Устанавливаем доменное имя
											entry.back().domain = domain;
											// Добавляем новую запись в кэш IP-адресов
											::__awh_cache__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address, ::move(entry));
										}
									} break;
								}
								// Выполняем добавление IP-адреса
								i->second.push_back(::move(record));
							// Если в кэше доменное имя не найдено
							} else {
								// Создаём список записей IP-адресов
								vector <EntryIP> entry(1);
								// Помечаем IP-адрес как локальный
								entry.back().local = true;
								/**
								 * Определяем тип адреса
								 */
								switch(static_cast <uint8_t> (type)){
									// Если адрес является IPv4
									case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
										// Создаём объект IP-адреса для хранения IPv4-адреса
										entry.back().ip = make_unique <net::addr_net_ipv4_t> ();
										// Копируем IP-адрес из результата парсинга в запись
										awh_cast <net::addr_net_ipv4_t *> (entry.back().ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (parsed.get())->address;
										// Выполняем поиск IP-адреса
										auto i = ::__awh_cache__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (entry.back().ip.get())->address);
										// Если IP-адрес найден в кэше
										if(i != ::__awh_cache__.ipv4.end()){
											// Создаём объект записи
											EntryDomain record{};
											// Помечаем IP-адрес как локальный
											record.local = true;
											// Устанавливаем доменное имя
											record.domain = domain;
											// Выполняем добавление IP-адреса
											i->second.push_back(::move(record));
										// Если IP-адрес не найден в кэше
										} else {
											// Создаём список записей IP-адресов
											vector <EntryDomain> entryDomain(1);
											// Помечаем IP-адрес как локальный
											entryDomain.back().local = true;
											// Устанавливаем доменное имя
											entryDomain.back().domain = domain;
											// Добавляем новую запись в кэш IP-адресов
											::__awh_cache__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (entry.back().ip.get())->address, ::move(entryDomain));
										}
									} break;
									// Если адрес является IPv6
									case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
										// Создаём объект IP-адреса для хранения IPv6-адреса
										entry.back().ip = make_unique <net::addr_net_ipv6_t> ();
										// Копируем IP-адрес из результата парсинга в запись
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (entry.back().ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (parsed.get())->address[0], 16);
										// Выполняем поиск IP-адреса
										auto i = ::__awh_cache__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (entry.back().ip.get())->address);
										// Если IP-адрес найден в кэше
										if(i != ::__awh_cache__.ipv6.end()){
											// Создаём объект записи
											EntryDomain record{};
											// Помечаем IP-адрес как локальный
											record.local = true;
											// Устанавливаем доменное имя
											record.domain = domain;
											// Выполняем добавление IP-адреса
											i->second.push_back(::move(record));
										// Если IP-адрес не найден в кэше
										} else {
											// Создаём список записей IP-адресов
											vector <EntryDomain> entryDomain(1);
											// Помечаем IP-адрес как локальный
											entryDomain.back().local = true;
											// Устанавливаем доменное имя
											entryDomain.back().domain = domain;
											// Добавляем новую запись в кэш IP-адресов
											::__awh_cache__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (entry.back().ip.get())->address, ::move(entryDomain));
										}
									} break;
								}
								// Добавляем доменное имя в запись
								if(!domain.empty())
									// Добавляем новую запись в кэш доменных имён
									::__awh_cache__.domains.emplace(domain, ::move(entry));
							}
						}
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			};
			// Позиция в остатке
			size_t pos = 0;
			// Добавляем прочитанные данные к остатку
			string remainder(reinterpret_cast <const char *> (data), size);
			/**
			 * Выполняем обработку остатка на наличие полных строк
			 */
			while(pos < remainder.length()){
				// Извлекаем текущий символ
				char c = remainder[pos];
				// Если символ является символом новой строки
				if(c == '\n'){
					// Создаём представление строки
					string_view str(remainder.c_str(), pos);
					// Если строка заканчивается символом возврата каретки
					if(!str.empty() && (str.back() == '\r'))
						// Удаляем символ возврата каретки из строки
						str = str.substr(0, str.size() - 1);
					// Если строка не является комментарием
					if(!str.empty() && (str.front() != '#'))
						// Выполняем парсинг полученной строки
						parseStrHosts(str);
					// Обновляем остаток
					remainder = ::move(remainder.substr(pos + 1));
					// Сбрасываем позицию в остатке
					pos = 0;
				// Если символ является символом возврата каретки
				} else if((c == '\r') && ((pos + 1) < remainder.length()) && (remainder[pos + 1] == '\n')) {
					// Создаём представление строки
					string_view str(remainder.c_str(), pos);
					// Если строка не является комментарием
					if(!str.empty() && (str.front() != '#'))
						// Выполняем парсинг полученной строки
						parseStrHosts(str);
					// Обновляем остаток
					remainder = ::move(remainder.substr(pos + 2));
					// Сбрасываем позицию в остатке
					pos = 0;
				// Выполняем переход к следующему символу
				} else pos++;
			}
			// Обработка последней строки (если нет \n в конце)
			if(!remainder.empty()){
				// Создаём представление строки
				string_view str(remainder.c_str(), remainder.size());
				// Если строка заканчивается символом возврата каретки
				if(!str.empty() && (str.back() == '\r'))
					// Удаляем символ возврата каретки из строки
					str = str.substr(0, str.size() - 1);
				// Если строка не является комментарием
				if(!str.empty() && (str.front() != '#'))
					// Выполняем парсинг полученной строки
					parseStrHosts(str);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(data, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки ответов DNS-сервера
 *
 * @param eid  идентификатор события чтения из DNS-резолвера
 * @param data данные события чтения из DNS-резолвера
 * @param size размер данных события чтения из DNS-резолвера
 *
 */
void awh::unit::DNS::response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Идентификатор DNS-запроса
		id_t id = 0;
		// Признак принятого ответа
		bool accepted = false;
		// Копия полученного пакета для обработки вне блокировки передачи
		vector <uint8_t> packet;
		// Доменное имя для логирования
		string domain = "unknown";
		// Если данные события чтения из DNS-резолвера не пустые
		if((data != nullptr) && (size > 0)){
			// Блокируем доступ к состоянию передачи DNS-запросов
			const locker_t <> lock(this->_mtx);
			// Получаем указатель на заголовок DNS
			const ::dns::head_t * header = reinterpret_cast <const ::dns::head_t *> (data);
			// Извлекаем идентификатор DNS-запроса из заголовка
			id = ntohs(header->id);
			// Выполняем поиск привязки события к DNS-запросу
			auto attachedIt = this->_transfer.attached.find(eid);
			// Если привязка отсутствует или идентификатор не совпадает
			if((attachedIt == this->_transfer.attached.end()) || (attachedIt->second != id))
				// Игнорируем неподтверждённый ответ
				return;
			// Выполняем поиск пакета в контейнере активных пакетов
			auto waitingIt = this->_transfer.waiting.find(id);
			// Если пакет не найден в контейнере активных пакетов
			if(waitingIt == this->_transfer.waiting.end())
				// Игнорируем ответ без ожидающего запроса
				return;
			// Получаем размер запроса
			size_t offset = sizeof(::dns::head_t);
			// Выполняем декодирование доменного имени из бинарных данных запроса
			domain = ::move(::dns::decodeDomainName(waitingIt->second.payload.buffer.get(), waitingIt->second.payload.size, offset));
			// Удаляем пакет из контейнера активных пакетов
			this->_transfer.waiting.erase(waitingIt);
			// Копируем полученный пакет для обработки вне блокировки передачи
			packet.assign(data, data + size);
			// Устанавливаем признак принятого ответа
			accepted = true;
		}
		// Если ответ не принят
		if(!accepted)
			// Завершаем обработку
			return;
		// Получаем указатель на заголовок DNS
		const ::dns::head_t * header = reinterpret_cast <const ::dns::head_t *> (packet.data());
		// Размер полученного пакета
		const size_t packetSize = packet.size();
		/**
		 * Определяем код выполнения операции
		 */
		switch(header->rcode){
			// Если операция выполнена удачно
			case 0: {
				// Создаём объект для хранения результата парсинга ответа от DNS-сервера
				::dns::dns_result_t result;
				// Выполняем парсинг ответа от DNS-сервера
				if(::dns::parse(packet.data(), packetSize, result)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Печатаем разделитель в отладочный вывод
						cout << "------------------------------------------------------------" << endl << endl << flush;
						// Печатаем заголовок ответа
						cout << "DNS RESPONSE:" << endl << endl << flush;
						// Если мы получили A-записи в ответе
						if(!result.a.empty()){
							/**
							 * Перебираем все A-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.a){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Устанавливаем IPv4-адрес в объекте адреса
								this->_addr.v4(answer.ip);
								// Печатаем IPv4-адрес
								::printf("IPv4: %s\n", static_cast <string> (this->_addr).c_str());
							}
						}
						// Если мы получили AAAA-записи в ответе
						if(!result.aaaa.empty()){
							/**
							 * Перебираем все AAAA-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.aaaa){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Устанавливаем IPv6-адрес в объекте адреса
								this->_addr.v6(answer.ip);
								// Печатаем IPv6-адрес
								::printf("IPv6: %s\n", static_cast <string> (this->_addr).c_str());
							}
						}
						// Если мы получили NS-записи в ответе
						if(!result.ns.empty()){
							/**
							 * Перебираем все NS-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.ns){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Печатаем NS-сервер
								::printf("NS: %s\n", answer.server.c_str());
							}
						}
						// Если мы получили CNAME-записи в ответе
						if(!result.cname.empty()){
							/**
							 * Перебираем все CNAME-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.cname){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Печатаем каноническое имя
								::printf("CNAME: %s\n", answer.canonical.c_str());
							}
						}
						// Если мы получили MX-записи в ответе
						if(!result.mx.empty()){
							/**
							 * Перебираем все MX-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.mx){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Печатаем почтовый сервер
								::printf("MX: %s\n", answer.server.c_str());
								// Печатаем приоритет MX-записи
								::printf("PREFERENCE: %u\n", answer.preference);
							}
						}
						// Если мы получили TXT-записи в ответе
						if(!result.txt.empty()){
							/**
							 * Перебираем все TXT-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.txt){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								/**
								 * Вывод текстовых данных из TXT-записи
								 */
								for(auto & text : answer.texts)
									// Печатаем текст TXT-записи
									::printf("TXT: %s\n", text.c_str());
							}
						}
						// Если мы получили PTR-записи в ответе
						if(!result.ptr.empty()){
							/**
							 * Перебираем все PTR-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.ptr){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Печатаем PTR-запись
								::printf("PTR: %s\n", answer.domain.c_str());
							}
						}
						// Если мы получили SOA-записи в ответе
						if(!result.soa.empty()){
							/**
							 * Перебираем все SOA-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.soa){
								// Печатаем имя записи
								::printf("\nNAME: %s\n", answer.name.c_str());
								// Печатаем TTL записи
								::printf("TTL: %u\n", answer.ttl);
								// Печатаем MNAME SOA-записи
								::printf("SOA: %s\n", answer.mname.c_str());
								// Печатаем RNAME SOA-записи
								::printf("RNAME: %s\n", answer.rname.c_str());
								// Печатаем серийный номер зоны
								::printf("SERIAL: %u\n", answer.serial);
								// Печатаем интервал refresh
								::printf("REFRESH: %u\n", answer.refresh);
								// Печатаем интервал retry
								::printf("RETRY: %u\n", answer.retry);
								// Печатаем интервал expire
								::printf("EXPIRE: %u\n", answer.expire);
							}
						}
						// Печатаем разделитель в отладочный вывод
						cout << endl << "------------------------------------------------------------" << endl << endl << flush;
					#endif
					// Если мы получили A-записи в ответе
					if(!result.a.empty()){
						/**
						 * Перебираем все A-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.a){
							// Выполняем инициализацию объекта IP-адреса
							unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
							// Устанавливаем IP-адрес
							awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = answer.ip;
							// Добавляем запись в кэш DNS-резолвера
							this->pushAddressToCache(answer.name, ip.get(), answer.ttl);
						}
						// Выполняем получение идентификатора функции обратного вызова
						callback_t::id_t fid = this->_callback.id("addresses");
						// Если функция обратного вызова установлена для получения списка IP-адресов
						if(this->_callback.is(fid)){
							// Список IP-адресов для передачи в функцию обратного вызова
							vector <unique_ptr <net::addr_t>> addresses(result.a.size());
							/**
							 * Перебираем все A-записи в ответе от DNS-сервера
							 */
							for(size_t i = 0; i < result.a.size(); ++i){
								// Устанавливаем IPv4-адрес в объекте адреса
								this->_addr.v4(result.a[i].ip);
								// Устанавливаем представление IP-адреса для вывода результата
								addresses[i] = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const vector <unique_ptr <net::addr_t>> &)> (fid, id, event::family_t::IPV4, domain, addresses);
						}
						// Выполняем получение идентификатора функции обратного вызова
						fid = this->_callback.id("address");
						// Если функция обратного вызова установлена для получения IP-адресов
						if(this->_callback.is(fid)){
							// Выбираем стандарт рандомайзера
							mt19937 generator(::__awh_randev__());
							// Выполняем рандомную сортировку списка DNS-серверов
							::shuffle(result.a.begin(), result.a.end(), generator);
							// Устанавливаем IPv4-адрес в объекте адреса
							this->_addr.v4(result.a.front().ip);
							// Устанавливаем представление IP-адреса для вывода результата
							unique_ptr <net::addr_t> address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> (fid, id, event::family_t::IPV4, result.a.front().name, address.get());
						}
					}
					// Если мы получили AAAA-записи в ответе
					if(!result.aaaa.empty()){
						/**
						 * Перебираем все AAAA-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.aaaa){
							// Выполняем инициализацию объекта IP-адреса
							unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
							// Устанавливаем IP-адрес
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &answer.ip[0], 16);
							// Добавляем запись в кэш DNS-резолвера
							this->pushAddressToCache(answer.name, ip.get(), answer.ttl);
						}
						// Выполняем получение идентификатора функции обратного вызова
						callback_t::id_t fid = this->_callback.id("addresses");
						// Если функция обратного вызова установлена для получения списка IP-адресов
						if(this->_callback.is(fid)){
							// Список IP-адресов для передачи в функцию обратного вызова
							vector <unique_ptr <net::addr_t>> addresses(result.aaaa.size());
							/**
							 * Перебираем все AAAA-записи в ответе от DNS-сервера
							 */
							for(size_t i = 0; i < result.aaaa.size(); ++i){
								// Устанавливаем IPv6-адрес в объекте адреса
								this->_addr.v6(result.aaaa[i].ip);
								// Устанавливаем представление IP-адреса для вывода результата
								addresses[i] = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const vector <unique_ptr <net::addr_t>> &)> (fid, id, event::family_t::IPV6, domain, addresses);
						}
						// Выполняем получение идентификатора функции обратного вызова
						fid = this->_callback.id("address");
						// Если функция обратного вызова установлена для получения IP-адресов
						if(this->_callback.is(fid)){
							// Выбираем стандарт рандомайзера
							mt19937 generator(::__awh_randev__());
							// Выполняем рандомную сортировку списка DNS-серверов
							::shuffle(result.aaaa.begin(), result.aaaa.end(), generator);
							// Устанавливаем IPv6-адрес в объекте адреса
							this->_addr.v6(result.aaaa.front().ip);
							// Устанавливаем представление IP-адреса для вывода результата
							unique_ptr <net::addr_t> address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> (fid, id, event::family_t::IPV6, result.aaaa.front().name, address.get());
						}
					}
					// Если мы получили NS-записи в ответе
					if(!result.ns.empty()){
						/**
						 * Создаём контейнер для хранения серверов имён
						 */
						unordered_multimap <string, string> ns;
						/**
						 * Перебираем все NS-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.ns)
							// Добавляем запись в контейнер серверов имён
							ns.emplace(answer.name, answer.server);
						// Если сервера имён получены
						if(!ns.empty())
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const unordered_multimap <string, string> &)> ("ns", id, ns);
					}
					// Если мы получили CNAME-записи в ответе
					if(!result.cname.empty()){
						/**
						 * Создаём контейнер для хранения канонических имён
						 */
						unordered_multimap <string, string> cname;
						/**
						 * Перебираем все CNAME-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.cname)
							// Добавляем запись в контейнер канонических имён
							cname.emplace(answer.name, answer.canonical);
						// Если канонические имена получены
						if(!cname.empty())
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const unordered_multimap <string, string> &)> ("cname", id, cname);
					}
					// Если мы получили MX-записи в ответе
					if(!result.mx.empty()){
						/**
						 * Создаём контейнер для хранения MX-записей
						 */
						unordered_multimap <string, std::pair <string, uint16_t>> mx;
						/**
						 * Перебираем все MX-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.mx)
							// Добавляем запись в контейнер MX-записей
							mx.emplace(answer.name, ::make_pair(answer.server, answer.preference));
						// Если MX-записи получены
						if(!mx.empty())
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const unordered_multimap <string, std::pair <string, uint16_t>> &)> ("mx", id, mx);
					}
					// Если мы получили TXT-записи в ответе
					if(!result.txt.empty()){
						/**
						 * Создаём контейнер для хранения текстовых записей
						 */
						unordered_multimap <string, string> texts;
						/**
						 * Перебираем все TXT-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.txt){
							/**
							 * Вывод текстовых данных из TXT-записи
							 */
							for(auto & text : answer.texts)
								// Добавляем запись в контейнер текстовых записей
								texts.emplace(answer.name, text);
						}
						// Если TXT-записи получены
						if(!texts.empty())
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const unordered_multimap <string, string> &)> ("txt", id, texts);
					}
					// Если мы получили PTR-записи в ответе
					if(!result.ptr.empty()){
						/**
						 * Перебираем все PTR-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.ptr){
							// Устанавливаем ARPA-адрес в объекте адреса
							this->_addr.arpa(answer.name);
							// Получаем IP-адрес
							unique_ptr <net::addr_t> address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							// Если IP-адрес получен
							if(address != nullptr)
								// Добавляем запись в кэш DNS-резолвера
								this->pushAddressToCache(answer.domain, address.get(), answer.ttl);
						}
						// Выполняем получение идентификатора функции обратного вызова
						callback_t::id_t fid = this->_callback.id("addresses");
						// Если функция обратного вызова установлена для получения списка IP-адресов
						if(this->_callback.is(fid)){
							// Список IP-адресов для передачи в функцию обратного вызова
							vector <unique_ptr <net::addr_t>> addresses;
							/**
							 * Перебираем все A-записи в ответе от DNS-сервера
							 */
							for(auto & ptr : result.ptr){
								// Устанавливаем ARPA-адрес в объекте адреса
								this->_addr.arpa(ptr.name);
								/**
								 * Определяем тип адреса для установки семейство адресов для вывода результата
								 */
								switch(static_cast <uint8_t> (this->_addr.type())){
									// Если адрес является IPv4
									case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
									// Если адрес является IPv6
									case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
										// Устанавливаем представление IP-адреса для вывода результата
										addresses.push_back(::move(this->_addr.source(net_addr_t::endian_t::LITTLE)));
									break;
								}
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const vector <unique_ptr <net::addr_t>> &)> (fid, id, event::family_t::IPV4, domain, addresses);
						}
						// Выполняем получение идентификатора функции обратного вызова
						fid = this->_callback.id("address");
						// Если функция обратного вызова установлена для получения IP-адресов
						if(this->_callback.is(fid)){
							// Выбираем стандарт рандомайзера
							mt19937 generator(::__awh_randev__());
							// Выполняем рандомную сортировку списка DNS-серверов
							::shuffle(result.ptr.begin(), result.ptr.end(), generator);
							// Семейство адресов для вывода результата
							event::family_t family = event::family_t::NONE;
							// Устанавливаем ARPA-адрес в объекте адреса
							this->_addr.arpa(result.ptr.front().name);
							/**
							 * Определяем тип адреса для установки семейство адресов для вывода результата
							 */
							switch(static_cast <uint8_t> (this->_addr.type())){
								// Если адрес является IPv4
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
									// Устанавливаем семейство адресов для вывода результата как IPv4
									family = event::family_t::IPV4;
								break;
								// Если адрес является IPv6
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
									// Устанавливаем семейство адресов для вывода результата как IPv6
									family = event::family_t::IPV6;
								break;
							}
							// Устанавливаем представление IP-адреса для вывода результата
							unique_ptr <net::addr_t> address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> (fid, id, family, result.ptr.front().domain, address.get());
						}
					}
					// Если мы получили SOA-записи в ответе
					if(!result.soa.empty()){
						/**
						 * Перебираем все SOA-записи в ответе от DNS-сервера
						 */
						for(auto & answer : result.soa){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, string_view, string_view)> ("soa", id, answer.name, answer.mname);
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, string_view, string_view)> ("rname", id, answer.name, answer.rname);
						}
					}
				}
			} break;
			// Если сервер DNS не смог интерпретировать запрос
			case 1: {
				// Формируем текст сообщения об ошибке DNS-резолвера
				const string error = this->_fmk->format("DNS query format error to nameserver %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("error");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, eid, event::error_t::UNKNOWN, error);
				// Если callback ошибки не установлен
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
			} break;
			// Если проблемы возникли на DNS-сервере
			case 2: {
				// Формируем текст сообщения об ошибке DNS-резолвера
				const string error = this->_fmk->format("DNS server failure %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("error");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, eid, event::error_t::CONNECTION_FAIL, error);
				// Если callback ошибки не установлен
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
			} break;
			// Если доменное имя, указанное в запросе, не существует
			case 3: {
				// Формируем текст сообщения об ошибке DNS-резолвера
				const string error = this->_fmk->format("Domain name %s referenced in the query for nameserver %s does not exist", domain.c_str(), this->_io->getTarget(eid).c_str());
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("error");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, eid, event::error_t::NOT_FOUND, error);
				// Если callback ошибки не установлен
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
			} break;
			// Если DNS-сервер не поддерживает подобный тип запросов
			case 4: {
				// Формируем текст сообщения об ошибке DNS-резолвера
				const string error = this->_fmk->format("DNS server is not implemented at %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("error");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, eid, event::error_t::INVALID_ADDRESS, error);
				// Если callback ошибки не установлен
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
			} break;
			// Если DNS-сервер отказался выполнять наш запрос (например, по политическим причинам)
			case 5: {
				// Формируем текст сообщения об ошибке DNS-резолвера
				const string error = this->_fmk->format("DNS request is refused to nameserver %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("error");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, eid, event::error_t::ACCESS_DENIED, error);
				// Если callback ошибки не установлен
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
			} break;
		}
		// Блокируем доступ к состоянию передачи DNS-запросов
		const locker_t <> lock(this->_mtx);
		// Ищем идентификатор запроса, привязанный к событию DNS-резолвера
		auto j = this->_transfer.attached.find(eid);
		// Если привязка события к запросу найдена
		if(j != this->_transfer.attached.end()){
			// Если в очереди на отправку есть пакеты
			if(!this->_transfer.packets.empty()){
				// Если время жизни пакета ещё не истекло
				if(this->_transfer.packets.front().alive > this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS)){
					// Получаем указатель на заголовок DNS
					::dns::head_t * queuedHeader = reinterpret_cast <::dns::head_t *> (this->_transfer.packets.front().payload.buffer.get());
					// Добавляем пакет в контейнер активных пакетов
					auto ret = this->_transfer.waiting.emplace(ntohs(queuedHeader->id), ::move(this->_transfer.packets.front()));
					// Меняем идентификатор DNS-запроса
					j->second = ret.first->first;
					// Удаляем пакет из очереди на отправку
					this->_transfer.packets.pop();
					// Отправляем DNS-запрос
					this->_io->send(eid, ret.first->second.payload.buffer.get(), ret.first->second.payload.size);
					// Выходим из функции
					return;
				// Удаляем пакет из очереди на отправку
				} else this->_transfer.packets.pop();
			}
			// Возвращаем идентификатор события в очередь свободных резолверов
			this->_resolver.queue.push(eid);
			// Удаляем привязку события к DNS-запросу
			this->_transfer.attached.erase(j);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки истечения таймаута DNS-запроса
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param action тип действия для истекшего таймаута
 * @param delay  длительность таймаута в миллисекундах
 * @return       нужно ли завершить обработчик после истечения таймаута
 *
 */
bool awh::unit::DNS::timeout(const event::id_t eid, const event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если таймаут истёк для действия чтения
		if(action == event::action_t::READ){
			// Идентификатор DNS-запроса
			id_t id = 0;
			// Декодируем доменное имя
			string domain = "";
			// Количество попыток повторной отправки DNS-запроса
			uint8_t attempt = 0;
			// Признак необходимости уведомления об исчерпании попыток
			bool notifyTimeout = false;
			// Тип DNS-записи
			record_t record = record_t::NONE;
			{
				// Блокируем доступ к состоянию передачи DNS-запросов
				const locker_t <> lock(this->_mtx);
				// Ищем идентификатор запроса, привязанный к событию DNS-резолвера
				auto i = this->_transfer.attached.find(eid);
				// Если привязка события к запросу найдена
				if(i != this->_transfer.attached.end()){
					// Выполняем поиск пакета в контейнере активных пакетов
					auto j = this->_transfer.waiting.find(i->second);
					// Если пакет найден в контейнере активных пакетов
					if(j != this->_transfer.waiting.end()){
						// Если число повторных попыток не превышает допустимый предел
						if(j->second.attempt < this->_transfer.attempts){
							// Увеличиваем число повторных попыток DNS-запроса
							j->second.attempt++;
							// Семейство IP-адресов DNS-резолвера
							event::family_t family = event::family_t::NONE;
							/**
							 * Определяем семейство IP-адресов по идентификатору события
							 */
							for(const auto item : this->_resolver.idv4)
								// Если идентификатор события найден среди IPv4-резолверов
								if(item == eid)
									// Устанавливаем семейство IPv4
									family = event::family_t::IPV4;
							// Если семейство ещё не определено
							if(family == event::family_t::NONE){
								/**
								 * Выполняем перебор всех IPv6-резолверов
								 */
								for(const auto item : this->_resolver.idv6)
									// Если идентификатор события найден среди IPv6-резолверов
									if(item == eid)
										// Устанавливаем семейство IPv6
										family = event::family_t::IPV6;
							}
							// Если следующий DNS-сервер доступен
							if(const net::addr_t * server = this->_resolver.nameServers.get(family))
								// Переключаем DNS-резолвер на следующий сервер
								this->_io->setTarget(eid, server);
							// Повторно отправляем DNS-запрос
							this->_io->send(eid, j->second.payload.buffer.get(), j->second.payload.size);
							// Продолжаем ожидание ответа
							return false;
						}
						// Получаем идентификатор DNS-запроса
						id = i->second;
						// Сохраняем число выполненных повторных попыток
						attempt = j->second.attempt;
						// Получаем размер запроса
						size_t offset = sizeof(::dns::head_t);
						// Выполняем декодирование доменного имени из бинарных данных запроса
						domain = ::move(::dns::decodeDomainName(j->second.payload.buffer.get(), j->second.payload.size, offset));
						// Создаём части флагов вопроса пакета запроса
						::dns::q_flags_t * qflags = reinterpret_cast <::dns::q_flags_t *> (&j->second.payload.buffer.get()[offset]);
						// Извлекаем тип запроса из флагов запроса
						record = static_cast <record_t> (ntohs(qflags->type));
						// Удаляем пакет из контейнера активных пакетов
						this->_transfer.waiting.erase(j);
						// Если в очереди на отправку есть пакеты
						if(!this->_transfer.packets.empty()){
							// Если время жизни пакета ещё не истекло
							if(this->_transfer.packets.front().alive > this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS)){
								// Получаем указатель на заголовок DNS
								::dns::head_t * header = reinterpret_cast <::dns::head_t *> (this->_transfer.packets.front().payload.buffer.get());
								// Добавляем пакет в контейнер активных пакетов
								auto ret = this->_transfer.waiting.emplace(ntohs(header->id), ::move(this->_transfer.packets.front()));
								// Меняем идентификатор DNS-запроса
								i->second = ret.first->first;
								// Удаляем пакет из очереди на отправку
								this->_transfer.packets.pop();
								// Отправляем DNS-запрос
								this->_io->send(eid, ret.first->second.payload.buffer.get(), ret.first->second.payload.size);
								// Продолжаем ожидание ответа
								return false;
							// Удаляем пакет из очереди на отправку
							} else this->_transfer.packets.pop();
						}
						// Возвращаем идентификатор события в очередь свободных резолверов
						this->_resolver.queue.push(eid);
						// Удаляем привязку события к DNS-запросу
						this->_transfer.attached.erase(i);
						// Устанавливаем признак необходимости уведомления об исчерпании попыток
						notifyTimeout = true;
					}
				}
			}
			// Если декодирование доменного имени прошло успешно и попытки исчерпаны
			if(notifyTimeout && !domain.empty()){
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("attempts");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const id_t, const string &, const uint8_t)> (fid, id, domain, attempt);
				// Если функция обратного вызова не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"DNS resolver timeout for domain '%s' (attempts: %u)",
							__PRETTY_FUNCTION__,
							make_tuple(eid, static_cast <uint16_t> (action), delay),
							log_t::flag_t::WARNING,
							domain.c_str(), attempt
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("DNS resolver timeout for domain '%s' (attempts: %u)", log_t::flag_t::WARNING, domain.c_str(), attempt);
					#endif
				}
				// Выполняем функцию обратного вызова для неудачного резолвинга доменного имени
				this->_callback.call <void (const id_t, const record_t, const string &)> ("failure", id, record, domain);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), delay), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Продолжаем ожидание ответа
	return false;
}
/**
 * @brief Метод обработки ошибок событий DNS-резолвера
 *
 * @param eid         идентификатор события DNS-резолвера
 * @param error       код ошибки события DNS-резолвера
 * @param description описание ошибки события DNS-резолвера
 *
 */
void awh::unit::DNS::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::DNS::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении серверов имён
	this->_callback.set("ns", callback);
	// Выполняем установку функции обратного вызова при получении MX-записей
	this->_callback.set("mx", callback);
	// Выполняем установку функции обратного вызова при получении текстовых записей
	this->_callback.set("txt", callback);
	// Выполняем установку функции обратного вызова при получении SOA-записей
	this->_callback.set("soa", callback);
	// Выполняем установку функции обратного вызова при получении RNAME-записей
	this->_callback.set("rname", callback);
	// Выполняем установку функции обратного вызова при получении канонического имени
	this->_callback.set("cname", callback);
	// Выполняем установку функции обратного вызова при неудачном резолвинге доменного имени
	this->_callback.set("failure", callback);
	// Выполняем установку функции обратного вызова при получении IP-адресов
	this->_callback.set("address", callback);
	// Устанавливаем обработчик исчерпания числа попыток DNS-запроса
	this->_callback.set("attempts", callback);
	// Выполняем установку функции обратного вызова при получении списка IP-адресов
	this->_callback.set("addresses", callback);
}
/**
 * @brief Метод установки числа попыток DNS-запроса
 *
 * @param attempts количество попыток DNS-запроса
 *
 */
void awh::unit::DNS::setAttempts(const uint8_t attempts) noexcept {
	// Блокируем доступ к состоянию передачи DNS-запросов
	const locker_t <> lock(this->_mtx);
	// Устанавливаем число попыток DNS-запроса
	this->_transfer.attempts = attempts;
}
/**
 * @brief Метод установки максимального количества пакетов в очереди ожидания выполнения запроса к DNS-серверу
 *
 * @param count максимальное количество пакетов
 *
 */
void awh::unit::DNS::setMaxPackets(const uint16_t count) noexcept {
	// Блокируем доступ к состоянию передачи DNS-запросов
	const locker_t <> lock(this->_mtx);
	// Устанавливаем максимальное количество пакетов в очереди ожидания выполнения запроса к DNS-серверу
	this->_transfer.maxPackets = count;
}
/**
 * @brief Метод кодирования интернационального доменного имени
 *
 * @param domain доменное имя для кодирования
 * @return       результат работы кодирования
 *
 */
string awh::unit::DNS::encode(string_view domain) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано
		if(!domain.empty() && (domain.front() != '-') && (domain.back() != '-')){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Результирующий буфер данных
				wchar_t buffer[0xFF];
				// Выполняем кодирование доменного имени
				if(::IdnToAscii(0, this->_fmk->convert(domain).c_str(), -1, buffer, sizeof(buffer)) == 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				// Получаем результат кодирования
				} else result = this->_fmk->convert(wstring{buffer});
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#elif AWH_IDN
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем кодирования доменного имени
				const int32_t rc = ::idn2_to_ascii_8z(domain.data(), &buffer, IDN2_NONTRANSITIONAL);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					#endif
				// Получаем результат кодирования
				} else result = buffer;
				// Если память была выделена
				if(buffer != nullptr)
					// Очищаем буфер данных
					::free(buffer);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод декодирования интернационального доменного имени
 *
 * @param domain доменное имя для декодирования
 * @return       результат работы декодирования
 *
 */
string awh::unit::DNS::decode(string_view domain) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано
		if(!domain.empty() && (domain.front() != '-') && (domain.back() != '-')){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Результирующий буфер данных
				wchar_t buffer[0xFF];
				// Выполняем кодирования доменного имени
				if(::IdnToUnicode(0, this->_fmk->convert(domain).c_str(), -1, buffer, sizeof(buffer)) == 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xFF] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xFF, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				// Получаем результат кодирования
				} else result = this->_fmk->convert(wstring{buffer});
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#elif AWH_IDN
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем декодирование доменного имени
				const int32_t rc = ::idn2_to_unicode_8z8z(domain.data(), &buffer, 0);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					#endif
				// Получаем результат декодирования
				} else result = buffer;
				// Если память была выделена
				if(buffer != nullptr)
					// Очищаем буфер данных
					::free(buffer);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод пересортировки адресов в кэше для доменного имени
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 *
 */
void awh::unit::DNS::shuffle(const event::family_t family, string_view domain) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем доступ к глобальному кэшу DNS
		const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Если доменное имя передано
		if(!domain.empty()){
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Выбираем стандарт рандомайзера
				mt19937 generator(::__awh_randev__());
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(i->second.begin(), i->second.end(), generator);
			}
		}
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если список IPv4-адресов не пустой
				if(!::__awh_cache__.ipv4.empty()){
					// Выбираем стандарт рандомайзера
					mt19937 generator(::__awh_randev__());
					/**
					 * Выполняем перебор всех IPv4-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end(); ++i)
						// Выполняем рандомную сортировку списка доменных имён
						::shuffle(i->second.begin(), i->second.end(), generator);
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если список IPv6-адресов не пустой
				if(!::__awh_cache__.ipv6.empty()){
					// Выбираем стандарт рандомайзера
					mt19937 generator(::__awh_randev__());
					/**
					 * Выполняем перебор всех IPv6-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end(); ++i)
						// Выполняем рандомную сортировку списка доменных имён
						::shuffle(i->second.begin(), i->second.end(), generator);
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки чёрного списка
 *
 */
void awh::unit::DNS::clearBlacklist() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем доступ к глобальному чёрному списку DNS
		const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Если чёрный список IPv4 адресов не пустой
		if(!::__awh_blacklist__.ipv4.empty())
			// Очищаем чёрный список
			::__awh_blacklist__.ipv4.clear();
		// Если чёрный список IPv6 адресов не пустой
		if(!::__awh_blacklist__.ipv6.empty())
			// Очищаем чёрный список
			::__awh_blacklist__.ipv6.clear();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки чёрного списка
 *
 * @param family семейство IP-адресов IPv4/IPv6
 *
 */
void awh::unit::DNS::clearBlacklist(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем доступ к глобальному чёрному списку DNS
		const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если чёрный список IPv4 адресов не пустой
				if(!::__awh_blacklist__.ipv4.empty())
					// Очищаем чёрный список
					::__awh_blacklist__.ipv4.clear();
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если чёрный список IPv6 адресов не пустой
				if(!::__awh_blacklist__.ipv6.empty())
					// Очищаем чёрный список
					::__awh_blacklist__.ipv6.clear();
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод удаления IP-адреса из чёрного списка
 *
 * @param ip адрес для удаления из чёрного списка
 *
 */
void awh::unit::DNS::removeAddressInBlacklist(string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(ip)){
				// Получаем IP-адрес в исходном виде
				auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Выполняем поиск IP-адреса в чёрном списке
						auto i = ::__awh_blacklist__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
						// Если IP-адрес найден в чёрном списке
						if(i != ::__awh_blacklist__.ipv4.end())
							// Удаляем IP-адрес из чёрного списка
							::__awh_blacklist__.ipv4.erase(i);
					} break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Выполняем поиск IP-адреса в чёрном списке
						auto i = ::__awh_blacklist__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address);
						// Если IP-адрес найден в чёрном списке
						if(i != ::__awh_blacklist__.ipv6.end())
							// Удаляем IP-адрес из чёрного списка
							::__awh_blacklist__.ipv6.erase(i);
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод удаления IP-адреса из чёрного списка
 *
 * @param ip адрес для удаления из чёрного списка
 *
 */
void awh::unit::DNS::removeAddressInBlacklist(const net::addr_t * ip) noexcept {
	// Если IP-адрес передан
	if(ip != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем поиск IP-адреса в чёрном списке
					auto i = ::__awh_blacklist__.ipv4.find(awh_cast <const net::addr_net_ipv4_t *> (ip)->address);
					// Если IP-адрес найден в чёрном списке
					if(i != ::__awh_blacklist__.ipv4.end())
						// Удаляем IP-адрес из чёрного списка
						::__awh_blacklist__.ipv4.erase(i);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем поиск IP-адреса в чёрном списке
					auto i = ::__awh_blacklist__.ipv6.find(awh_cast <const net::addr_net_ipv6_t *> (ip)->address);
					// Если IP-адрес найден в чёрном списке
					if(i != ::__awh_blacklist__.ipv6.end())
						// Удаляем IP-адрес из чёрного списка
						::__awh_blacklist__.ipv6.erase(i);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод удаления IP-адреса из чёрного списка
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param ip     адрес для удаления из чёрного списка
 *
 */
void awh::unit::DNS::removeAddressInBlacklist(const event::family_t family, string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV4)){
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем поиск IP-адреса в чёрном списке
						auto i = ::__awh_blacklist__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
						// Если IP-адрес найден в чёрном списке
						if(i != ::__awh_blacklist__.ipv4.end())
							// Удаляем IP-адрес из чёрного списка
							::__awh_blacklist__.ipv4.erase(i);
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV6)){
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем поиск IP-адреса в чёрном списке
						auto i = ::__awh_blacklist__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address);
						// Если IP-адрес найден в чёрном списке
						if(i != ::__awh_blacklist__.ipv6.end())
							// Удаляем IP-адрес из чёрного списка
							::__awh_blacklist__.ipv6.erase(i);
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param ip адрес для добавления в чёрный список
 *
 */
void awh::unit::DNS::pushAddressToBlacklist(string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(ip)){
				// Получаем IP-адрес в исходном виде
				auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Выполняем добавление IP-адреса в чёрный список
						::__awh_blacklist__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Выполняем добавление IP-адреса в чёрный список
						::__awh_blacklist__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address);
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param ip адрес для добавления в чёрный список
 *
 */
void awh::unit::DNS::pushAddressToBlacklist(const net::addr_t * ip) noexcept {
	// Если IP-адрес передан
	if(ip != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4:
					// Выполняем добавление IP-адреса в чёрный список
					::__awh_blacklist__.ipv4.emplace(awh_cast <const net::addr_net_ipv4_t *> (ip)->address);
				break;
				// Если адрес является IPv6
				case 16:
					// Выполняем добавление IP-адреса в чёрный список
					::__awh_blacklist__.ipv6.emplace(awh_cast <const net::addr_net_ipv6_t *> (ip)->address);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param ip     адрес для добавления в чёрный список
 *
 */
void awh::unit::DNS::pushAddressToBlacklist(const event::family_t family, string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV4)){
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем добавление IP-адреса в чёрный список
						::__awh_blacklist__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV6)){
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем добавление IP-адреса в чёрный список
						::__awh_blacklist__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address);
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param ip адрес для проверки наличия в чёрном списке
 * @return   результат проверки наличия IP-адреса в чёрном списке
 *
 */
bool awh::unit::DNS::checkAddressInBlacklist(string_view ip) const noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем парсинг IP-адреса
			if(const_cast <dns_t *> (this)->_addr.parse(ip)){
				// Получаем IP-адрес в исходном виде
				auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Выполняем поиск IP-адреса в чёрном списке
						return (::__awh_blacklist__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address) != ::__awh_blacklist__.ipv4.end());
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Выполняем поиск IP-адреса в чёрном списке
						return (::__awh_blacklist__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address) != ::__awh_blacklist__.ipv6.end());
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param ip адрес для проверки наличия в чёрном списке
 * @return   результат проверки наличия IP-адреса в чёрном списке
 *
 */
bool awh::unit::DNS::checkAddressInBlacklist(const net::addr_t * ip) const noexcept {
	// Если IP-адрес передан
	if(ip != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному чёрному списку DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4:
					// Выполняем поиск IP-адреса в чёрном списке
					return (::__awh_blacklist__.ipv4.find(awh_cast <const net::addr_net_ipv4_t *> (ip)->address) != ::__awh_blacklist__.ipv4.end());
				// Если адрес является IPv6
				case 16:
					// Выполняем поиск IP-адреса в чёрном списке
					return (::__awh_blacklist__.ipv6.find(awh_cast <const net::addr_net_ipv6_t *> (ip)->address) != ::__awh_blacklist__.ipv6.end());
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 *
 */
bool awh::unit::DNS::checkAddressInBlacklist(const event::family_t family, string_view ip) const noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Распарсенный IP-адрес
			unique_ptr <net::addr_t> parsed = nullptr;
			// Локальный парсер сетевых адресов
			net_addr_t addr(this->_fmk, this->_log);
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if(addr.parse(ip, net_addr_t::type_t::IPV4))
						// Получаем IP-адрес в исходном виде
						parsed = ::move(addr.source(net_addr_t::endian_t::LITTLE));
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if(addr.parse(ip, net_addr_t::type_t::IPV6))
						// Получаем IP-адрес в исходном виде
						parsed = ::move(addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если IP-адрес успешно распознан
			if(parsed != nullptr){
				// Блокируем доступ к глобальному чёрному списку DNS
				const locker_t <std::shared_mutex> lock(::__awh_dns_blacklist_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
				/**
				 * Определяем тип адреса
				 */
				switch(parsed->size){
					// Если адрес является IPv4
					case 4:
						// Выполняем поиск IP-адреса в чёрном списке
						return (::__awh_blacklist__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (parsed.get())->address) != ::__awh_blacklist__.ipv4.end());
					// Если адрес является IPv6
					case 16:
						// Выполняем поиск IP-адреса в чёрном списке
						return (::__awh_blacklist__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (parsed.get())->address) != ::__awh_blacklist__.ipv6.end());
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод очистки кэша
 *
 */
void awh::unit::DNS::clearCache() noexcept {
	// Блокируем доступ к глобальному кэшу DNS
	const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Если в кэше есть IPv4-адреса
	if(!::__awh_cache__.ipv4.empty()){
		/**
		 * Выполняем перебор всех IP-адресов в кэше
		 */
		for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
			/**
			 * Выполняем перебор всех записей IP-адреса
			 */
			for(auto j = i->second.begin(); j != i->second.end();){
				// Если IP-адрес не является локальным
				if(!j->local)
					// Удаляем запись IP-адреса из кэша
					j = i->second.erase(j);
				// Если IP-адрес является локальным
				else ++j;
			}
			// Если после удаления всех записей IP-адреса в кэше не осталось
			if(i->second.empty())
				// Удаляем IP-адрес из кэша
				i = ::__awh_cache__.ipv4.erase(i);
			// Если у IP-адреса остались записи
			else ++i;
		}
	}
	// Если в кэше есть IPv6-адреса
	if(!::__awh_cache__.ipv6.empty()){
		/**
		 * Выполняем перебор всех IP-адресов в кэше
		 */
		for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
			/**
			 * Выполняем перебор всех записей IP-адреса
			 */
			for(auto j = i->second.begin(); j != i->second.end();){
				// Если IP-адрес не является локальным
				if(!j->local)
					// Удаляем запись IP-адреса из кэша
					j = i->second.erase(j);
				// Если IP-адрес является локальным
				else ++j;
			}
			// Если после удаления всех записей IP-адреса в кэше не осталось
			if(i->second.empty())
				// Удаляем IP-адрес из кэша
				i = ::__awh_cache__.ipv6.erase(i);
			// Если у IP-адреса остались записи
			else ++i;
		}
	}
	// Если в кэше есть доменные имена
	if(!::__awh_cache__.domains.empty()){
		/**
		 * Выполняем перебор всех доменных имён в кэше
		 */
		for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
			/**
			 * Выполняем перебор всех записей доменного имени
			 */
			for(auto j = i->second.begin(); j != i->second.end();){
				// Если доменное имя не является локальным
				if(!j->local)
					// Удаляем запись доменного имени из кэша
					j = i->second.erase(j);
				// Если доменное имя является локальным
				else ++j;
			}
			// Если после удаления всех записей доменного имени в кэше не осталось
			if(i->second.empty())
				// Удаляем доменное имя из кэша
				i = ::__awh_cache__.domains.erase(i);
			// Если у доменного имени остались записи
			else ++i;
		}
	}
}
/**
 * @brief Метод очистки кэша
 *
 * @param family семейство IP-адресов IPv4/IPv6
 *
 */
void awh::unit::DNS::clearCache(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем доступ к глобальному кэшу DNS
		const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Если в кэше есть доменные имена
		if(!::__awh_cache__.domains.empty()){
			/**
			 * Выполняем перебор всех доменных имён в кэше
			 */
			for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(auto j = i->second.begin(); j != i->second.end();){
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if(!j->local && (j->ip->size == 4))
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv6
							else ++j;
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if(!j->local && (j->ip->size == 16))
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv4
							else ++j;
						} break;
						// Если это какое-то другое семейство, то выходим из функции
						default: return;
					}
				}
				// Если после удаления всех записей доменного имени в кэше не осталось
				if(i->second.empty())
					// Удаляем доменное имя из кэша
					i = ::__awh_cache__.domains.erase(i);
				// Если у доменного имени остались записи
				else ++i;
			}
		}
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если в кэше есть IPv4-адреса
				if(!::__awh_cache__.ipv4.empty()){
					/**
					 * Выполняем перебор всех IP-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
						/**
						 * Выполняем перебор всех записей IP-адреса
						 */
						for(auto j = i->second.begin(); j != i->second.end();){
							// Если IP-адрес не является локальным
							if(!j->local)
								// Удаляем запись IP-адреса из кэша
								j = i->second.erase(j);
							// Если IP-адрес является локальным
							else ++j;
						}
						// Если после удаления всех записей IP-адреса в кэше не осталось
						if(i->second.empty())
							// Удаляем IP-адрес из кэша
							i = ::__awh_cache__.ipv4.erase(i);
						// Если у IP-адреса остались записи
						else ++i;
					}
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если в кэше есть IPv6-адреса
				if(!::__awh_cache__.ipv6.empty()){
					/**
					 * Выполняем перебор всех IP-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
						/**
						 * Выполняем перебор всех записей IP-адреса
						 */
						for(auto j = i->second.begin(); j != i->second.end();){
							// Если IP-адрес не является локальным
							if(!j->local)
								// Удаляем запись IP-адреса из кэша
								j = i->second.erase(j);
							// Если IP-адрес является локальным
							else ++j;
						}
						// Если после удаления всех записей IP-адреса в кэше не осталось
						if(i->second.empty())
							// Удаляем IP-адрес из кэша
							i = ::__awh_cache__.ipv6.erase(i);
						// Если у IP-адреса остались записи
						else ++i;
					}
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param domain доменное имя, для которого выполняется очистка кэша
 *
 */
void awh::unit::DNS::clearCache(string_view domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(auto j = i->second.begin(); j != i->second.end();){
					// Если доменное имя не является локальным
					if(!j->local)
						// Удаляем запись доменного имени из кэша
						j = i->second.erase(j);
					// Если доменное имя является локальным
					else ++j;
				}
				// Если после удаления всех записей доменного имени в кэше не осталось
				if(i->second.empty())
					// Удаляем доменное имя из кэша
					i = ::__awh_cache__.domains.erase(i);
			}
			// Если список IPv4-адресов не пустой
			if(!::__awh_cache__.ipv4.empty()){
				/**
				 * Выполняем перебор всех IPv4-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
					/**
					 * Выполняем перебор всех записей IPv4-адреса
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя соответствует удаляемому, то удаляем его из кэша
						if(!j->local && this->_fmk->compare(domain, j->domain))
							// Удаляем запись IPv4-адреса из кэша
							j = i->second.erase(j);
						// Если доменное имя не соответствует удаляемому, то пропускаем его
						else ++j;
					}
					// Если после удаления всех записей у IPv4-адреса не осталось записей, то удаляем его из кэша
					if(i->second.empty())
						// Удаляем IPv4-адрес из кэша
						i = ::__awh_cache__.ipv4.erase(i);
					// Если у IPv4-адреса остались записи, то пропускаем его
					else ++i;
				}
			}
			// Если список IPv6-адресов не пустой
			if(!::__awh_cache__.ipv6.empty()){
				/**
				 * Выполняем перебор всех IPv6-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
					/**
					 * Выполняем перебор всех записей IPv6-адреса
					 */
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя соответствует удаляемому, то удаляем его из кэша
						if(!j->local && this->_fmk->compare(domain, j->domain))
							// Удаляем запись IPv6-адреса из кэша
							j = i->second.erase(j);
						// Если доменное имя не соответствует удаляемому, то пропускаем его
						else ++j;
					}
					// Если после удаления всех записей у IPv6-адреса не осталось записей, то удаляем его из кэша
					if(i->second.empty())
						// Удаляем IPv6-адрес из кэша
						i = ::__awh_cache__.ipv6.erase(i);
					// Если у IPv6-адреса остались записи, то пропускаем его
					else ++i;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя, для которого выполняется очистка кэша
 *
 */
void awh::unit::DNS::clearCache(const event::family_t family, string_view domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(auto j = i->second.begin(); j != i->second.end();){
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if(!j->local && (j->ip->size == 4))
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv6, то пропускаем его
							else ++j;
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if(!j->local && (j->ip->size == 16))
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv4, то пропускаем его
							else ++j;
						} break;
						// Если это какое-то другое семейство, то выходим из функции
						default: return;
					}
				}
				// Если после удаления всех записей доменного имени в кэше не осталось, то удаляем его из кэша
				if(i->second.empty())
					// Удаляем доменное имя из кэша
					::__awh_cache__.domains.erase(i);
			}
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Если список IPv4-адресов не пустой
					if(!::__awh_cache__.ipv4.empty()){
						/**
						 * Выполняем перебор всех IPv4-адресов в кэше
						 */
						for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
							/**
							 * Выполняем перебор всех записей IPv4-адреса
							 */
							for(auto j = i->second.begin(); j != i->second.end();){
								// Если доменное имя соответствует удаляемому, то удаляем его из кэша
								if(!j->local && this->_fmk->compare(domain, j->domain))
									// Удаляем запись IPv4-адреса из кэша
									j = i->second.erase(j);
								// Если доменное имя не соответствует удаляемому, то пропускаем его
								else ++j;
							}
							// Если после удаления всех записей у IPv4-адреса не осталось записей, то удаляем его из кэша
							if(i->second.empty())
								// Удаляем IPv4-адрес из кэша
								i = ::__awh_cache__.ipv4.erase(i);
							// Если у IPv4-адреса остались записи, то пропускаем его
							else ++i;
						}
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Если список IPv6-адресов не пустой
					if(!::__awh_cache__.ipv6.empty()){
						/**
						 * Выполняем перебор всех IPv6-адресов в кэше
						 */
						for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
							/**
							 * Выполняем перебор всех записей IPv6-адреса
							 */
							for(auto j = i->second.begin(); j != i->second.end();){
								// Если доменное имя соответствует удаляемому, то удаляем его из кэша
								if(!j->local && this->_fmk->compare(domain, j->domain))
									// Удаляем запись IPv6-адреса из кэша
									j = i->second.erase(j);
								// Если доменное имя не соответствует удаляемому, то пропускаем его
								else ++j;
							}
							// Если после удаления всех записей у IPv6-адреса не осталось записей, то удаляем его из кэша
							if(i->second.empty())
								// Удаляем IPv6-адрес из кэша
								i = ::__awh_cache__.ipv6.erase(i);
							// Если у IPv6-адреса остались записи, то пропускаем его
							else ++i;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения IP-адреса из кэша
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @return       IP-адрес находящийся в кэше
 *
 */
string awh::unit::DNS::extractAddressFromCache(const event::family_t family, string_view domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Локальный парсер сетевых адресов
			net_addr_t addr(this->_fmk, this->_log);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(auto j = i->second.begin(); j != i->second.end(); ++j){
					// Пропускаем устаревшие записи в кэше (удаление будет при записи)
					if((j->life > 0) && (j->life <= now))
						// Пропускаем записи доменного имени
						continue;
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if(j->ip->size == 4){
								// Устанавливаем полученный IP-адрес
								addr.source(j->ip.get(), net_addr_t::endian_t::LITTLE);
								// Возвращаем результат
								return static_cast <string> (addr);
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if(j->ip->size == 16){
								// Устанавливаем полученный IP-адрес
								addr.source(j->ip.get(), net_addr_t::endian_t::LITTLE);
								// Возвращаем результат
								return static_cast <string> (addr);
							}
						} break;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод получения IP-адреса из кэша
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param value  IP-адрес находящийся в кэше
 * @return       результат выполнения операции
 *
 */
bool awh::unit::DNS::extractAddressFromCache(const event::family_t family, string_view domain, unique_ptr <net::addr_t> & value) noexcept {
	// Переменная результата
	bool result = false;
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(auto j = i->second.begin(); j != i->second.end(); ++j){
					// Пропускаем устаревшие записи в кэше (удаление будет при записи)
					if((j->life > 0) && (j->life <= now))
						// Пропускаем записи доменного имени
						continue;
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if((result = (j->ip->size == 4))){
								// Если объект результата не инициализирован
								if(value == nullptr)
									// Инициализируем объект результата
									value = make_unique <net::addr_net_ipv4_t> ();
								// Устанавливаем IP-адрес
								awh_cast <net::addr_net_ipv4_t *> (value.get())->address = awh_cast <net::addr_net_ipv4_t *> (j->ip.get())->address;
								// Возвращаем результат
								return result;
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if((result = (j->ip->size == 16))){
								// Если объект результата не инициализирован
								if(value == nullptr)
									// Инициализируем объект результата
									value = make_unique <net::addr_net_ipv6_t> ();
								// Устанавливаем IP-адрес
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (value.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (j->ip.get())->address[0], 16);
								// Возвращаем результат
								return result;
							}
						} break;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 *
 */
void awh::unit::DNS::pushAddressToCache(string_view domain, string_view ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Локальный парсер сетевых адресов
		net_addr_t addr(this->_fmk, this->_log);
		// Выполняем парсинг IP-адреса
		if(addr.parse(ip)){
			// Получаем IP-адрес в исходном виде
			auto parsed = ::move(addr.source(net_addr_t::endian_t::LITTLE));
			// Выполняем добавление записи в кэш
			this->pushAddressToCache(domain, parsed.get(), ttl);
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 *
 */
void awh::unit::DNS::pushAddressToCache(string_view domain, const net::addr_t * ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && (ip != nullptr) && (ttl > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Блокируем доступ к глобальному кэшу DNS
			const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				// Создаём объект записи
				EntryIP record;
				// Если время жизни кэша установлено
				if(ttl > 0)
					// Устанавливаем время жизни
					record.life = ::cacheLifeFromTtl(now, ttl);
				/**
				 * Определяем тип адреса
				 */
				switch(ip->size){
					// Если адрес является IPv4
					case 4: {
						// Выполняем инициализацию объекта IP-адреса
						record.ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (ip)->address;
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv4.end()){
							// Создаём объект записи
							EntryDomain record{};
							// Устанавливаем доменное имя
							record.domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = ::cacheLifeFromTtl(now, ttl);
							// Выполняем добавление или обновление IP-адреса
							::upsertEntryDomain(i->second, ::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <EntryDomain> entry(1);
							// Устанавливаем доменное имя
							entry.back().domain = domain;
							// Устанавливаем время жизни
							entry.back().life = record.life;
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address, ::move(entry));
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Выполняем инициализацию объекта IP-адреса
						record.ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (ip)->address[0], 16);
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv6.end()){
							// Создаём объект записи
							EntryDomain record{};
							// Устанавливаем доменное имя
							record.domain = domain;
							// Устанавливаем время жизни
							record.life = ::cacheLifeFromTtl(now, ttl);
							// Выполняем добавление или обновление IP-адреса
							::upsertEntryDomain(i->second, ::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <EntryDomain> entry(1);
							// Устанавливаем доменное имя
							entry.back().domain = domain;
							// Устанавливаем время жизни
							entry.back().life = record.life;
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address, ::move(entry));
						}
					} break;
				}
				// Выполняем добавление или обновление IP-адреса
				::upsertEntryIP(i->second, ::move(record));
			// Если в кэше доменное имя не найдено
			} else {
				// Создаём список записей IP-адресов
				vector <EntryIP> entry(1);
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				// Устанавливаем время жизни
				entry.back().life = ::cacheLifeFromTtl(now, ttl);
				/**
				 * Определяем тип адреса
				 */
				switch(ip->size){
					// Если адрес является IPv4
					case 4: {
						// Выполняем инициализацию объекта IP-адреса
						entry.back().ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (entry.back().ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (ip)->address;
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv4.find(awh_cast <const net::addr_net_ipv4_t *> (ip)->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv4.end()){
							// Создаём объект записи
							EntryDomain record{};
							// Устанавливаем доменное имя
							record.domain = domain;
							// Устанавливаем время жизни
							record.life = ::cacheLifeFromTtl(now, ttl);
							// Выполняем добавление или обновление IP-адреса
							::upsertEntryDomain(i->second, ::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <EntryDomain> entry(1);
							// Устанавливаем доменное имя
							entry.back().domain = domain;
							// Устанавливаем время жизни
							entry.back().life = ::cacheLifeFromTtl(now, ttl);
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv4.emplace(awh_cast <const net::addr_net_ipv4_t *> (ip)->address, ::move(entry));
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Выполняем инициализацию объекта IP-адреса
						entry.back().ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (entry.back().ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (ip)->address[0], 16);
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv6.find(awh_cast <const net::addr_net_ipv6_t *> (ip)->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv6.end()){
							// Создаём объект записи
							EntryDomain record{};
							// Устанавливаем доменное имя
							record.domain = domain;
							// Устанавливаем время жизни
							record.life = ::cacheLifeFromTtl(now, ttl);
							// Выполняем добавление или обновление IP-адреса
							::upsertEntryDomain(i->second, ::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <EntryDomain> entry(1);
							// Устанавливаем доменное имя
							entry.back().domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								entry.back().life = ::cacheLifeFromTtl(now, ttl);
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv6.emplace(awh_cast <const net::addr_net_ipv6_t *> (ip)->address, ::move(entry));
						}
					} break;
				}
				// Добавляем новую запись в кэш доменных имён
				::__awh_cache__.domains.emplace(domain, ::move(entry));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(domain, ttl), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 *
 */
void awh::unit::DNS::pushAddressToCache(const event::family_t family, string_view domain, string_view ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Выполняем парсинг IPv4-адреса
				if(this->_addr.parse(ip, net_addr_t::type_t::IPV4)){
					// Получаем IP-адрес в исходном виде
					auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					// Выполняем добавление записи в кэш
					this->pushAddressToCache(domain, ip.get(), ttl);
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Выполняем парсинг IPv6-адреса
				if(this->_addr.parse(ip, net_addr_t::type_t::IPV6)){
					// Получаем IP-адрес в исходном виде
					auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					// Выполняем добавление записи в кэш
					this->pushAddressToCache(domain, ip.get(), ttl);
				}
			} break;
		}
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::unit::DNS::threadSafety(const bool mode) noexcept {
	// Активируем работу мьютекса блокировки доступа к состоянию передачи DNS-запросов
	this->_mtx.enabled = mode;
	// Устанавливаем режим безопасности работы потоков для функции обратного вызова
	this->_callback.threadSafety(mode);
	// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем DNS
	::__awh_dns_cache_mutex__.enabled = mode;
	// Активируем работу мьютекса блокировки потока при работе с глобальным черным списком DNS
	::__awh_dns_blacklist_mutex__.enabled = mode;
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
}
/**
 * @brief Метод установки префикса переменной окружения
 *
 * @param prefix префикс переменной окружения для установки
 *
 */
void awh::unit::DNS::setPrefixEnvironment(string_view prefix) noexcept {
	// Если префикс переменной окружения передан
	if(!prefix.empty())
		// Устанавливаем префикс переменной окружения
		this->_resolver.prefix = this->_fmk->transform(prefix, fmk_t::transform_t::UPPER_CASE);
	// Если префикс переменной окружения не передан, очищаем префикс переменной окружения
	else this->_resolver.prefix.clear();
}
/**
 * @brief Метод установки пути к файлу локальных хостов
 *
 * @param filename путь к файлу /etc/hosts или аналогу
 *
 */
void awh::unit::DNS::setHostsAddress(string_view filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу дампа кэша указан
		if(!filename.empty()){
			// Добавляем новое событие файла для мониторинга изменений в файле локальных хостов
			::__awh_cache__.fid = this->_io->event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(::__awh_cache__.fid, static_cast <engine::callback::read_t> (std::bind(&dns_t::hosts, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(::__awh_cache__.fid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
			// Устанавливаем путь к отслеживаемому файлу
			if(this->_io->setAddress(::__awh_cache__.fid, event::address_t::FS, filename)){
				// Выполняем фиксацию настроек события сервера
				if(this->_io->commit(::__awh_cache__.fid)){
					// Устанавливаем опции события
					if(!this->_io->setOptions(::__awh_cache__.fid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Failed to set options for hosts file event", __PRETTY_FUNCTION__, make_tuple(filename), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Failed to set options for hosts file event", log_t::flag_t::CRITICAL);
							#endif
						}
					// Если мы успешно установили опции события
					} else {
						// Если событие запущено успешно
						if(this->_io->launch(::__awh_cache__.fid))
							// Выходим из функции
							return;
					}
				}
			// Если адрес не добавлен в событие
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("[%s] host address cannot be established", __PRETTY_FUNCTION__, make_tuple(filename), log_t::flag_t::CRITICAL, filename);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("[%s] host address cannot be established", log_t::flag_t::CRITICAL, filename);
				#endif
			}
			// Удаляем событие
			this->_io->destroy(::__awh_cache__.fid);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки пути к файлу дампа кэша
 *
 * @param filename путь к файлу дампа кэша
 * @param interval интервал сохранения дампа кэша в миллисекундах
 *
 */
void awh::unit::DNS::setDumpAddress(string_view filename, const uint32_t interval) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу дампа кэша указан
		if(!filename.empty()){
			// Очищаем бинарный контейнер для хранения кэша доменных имён
			this->_binbox.clear();
			// Загружаем кэш доменных имён из файла дампа кэша
			this->_binbox.load(filename);
			// Если кэш из бинарного файла загружен в контейнер
			if(!this->_binbox.empty()){
				// Создаём объект записи для добавления в контейнер
				Entry record{};
				// Размер буфера для загрузки кэша доменных имён
				size_t size = 0;
				// Бинарный буфер для загрузки кэша доменных имён
				uint8_t * buffer = nullptr;
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				/**
				 * Выполняем обработку всех записей из контейнера для загрузки кэша доменных имён
				 */
				for(uint32_t i = 0; i < this->_binbox.get <uint32_t> ("COUNT"); i++){
					// Если запись загружена из контейнера
					if(this->_binbox.get(this->_fmk->format("RECORD_%u", i), &buffer, &size)){
						// Если размер загруженных данных записи совпадает с размером объекта записи
						if(size == sizeof(record)){
							// Выполняем копирование данных записи в объект записи
							::memcpy(&record, buffer, sizeof(record));
							// Если время жизни кэша доменного имени не истекло
							if((record.life == 0) || (now < record.life)){
								// Время жизни записи в секундах для добавления в кэш
								uint32_t ttl = 0;
								// Если время жизни записи абсолютное и ещё не истекло
								if((record.life > 0) && (record.life > now))
									// Вычисляем оставшееся время жизни в секундах
									ttl = static_cast <uint32_t> ((record.life - now) / 1000);
								/**
								 * Определяем тип адреса
								 */
								switch(record.size){
									// Если адрес является IPv4
									case 4: {
										// Выполняем инициализацию объекта IP-адреса
										unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
										// Устанавливаем IP-адрес из записи кэша доменных имён
										::memcpy(&awh_cast <net::addr_net_ipv4_t *> (ip.get())->address, record.ip, 4);
										// Устанавливаем запись в кэш доменных имён
										this->pushAddressToCache(string(reinterpret_cast <const char *> (record.domain), ::strnlen(reinterpret_cast <const char *> (record.domain), sizeof(record.domain))), ip.get(), ttl);
									} break;
									// Если адрес является IPv6
									case 16:
										// Выполняем инициализацию объекта IP-адреса
										unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
										// Устанавливаем IP-адрес из записи кэша доменных имён
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address, record.ip, 16);
										// Устанавливаем запись в кэш доменных имён
										this->pushAddressToCache(string(reinterpret_cast <const char *> (record.domain), ::strnlen(reinterpret_cast <const char *> (record.domain), sizeof(record.domain))), ip.get(), ttl);
									break;
								}
							}
						}
					}
				}
			}
			// Если интервал сохранения дампа кэша установлен
			if(interval > 0){
				// Если интервал сохранения дампа кэша уже установлен
				if(::__awh_cache__.tid > 0){
					// Если интервал времени сохранения дампа кэша не совпадает с новым интервалом, то удаляем старый интервал
					if(::__awh_cache__.interval != interval)
						// Удаляем старый интервал
						this->_io->destroy(::__awh_cache__.tid);
					// Если интервал времени сохранения дампа кэша совпадает с новым интервалом, то просто выходим из функции
					else return;
				}
				// Устанавливаем интервал сохранения дампа кэша
				::__awh_cache__.interval = interval;
				// Устанавливаем путь к файлу дампа кэша
				::__awh_cache__.filename = filename;
				// Добавляем новое событие интервала
				::__awh_cache__.tid = this->_io->event(event::node_t::INTERVAL, event::family_t::TIMER);
				// Устанавливаем таймаут таймера
				this->_io->setTimeout(::__awh_cache__.tid, event::action_t::NONE, ::__awh_cache__.interval);
				// Устанавливаем обработчик события таймера для сохранения дампа кэша
				this->_io->on(::__awh_cache__.tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::dumping, this, _1, _2)));
				// Устанавливаем функцию обратного вызова на событие получения ошибок
				this->_io->on(::__awh_cache__.tid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
				// Если не удалось установить интервал сохранения дампа кэша, то удаляем событие интервала
				if(!(this->_io->commit(::__awh_cache__.tid) && this->_io->launch(::__awh_cache__.tid))){
					// Удаляем событие интервала
					this->_io->destroy(::__awh_cache__.tid);
					// Сбрасываем идентификатор события интервала
					::__awh_cache__.tid = 0;
					// Сбрасываем интервал сохранения дампа кэша
					::__awh_cache__.interval = 0;
					// Сбрасываем путь к файлу дампа кэша
					::__awh_cache__.filename = "";
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Failed to start cache dump interval", __PRETTY_FUNCTION__, make_tuple(filename, interval), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Failed to start cache dump interval", log_t::flag_t::CRITICAL);
						#endif
					}
				}
			}
		// Если путь к файлу дампа не указан, но интервал сохранения задан, снимаем прежний таймер
		} else if(interval > 0) {
			// Удаляем событие интервала
			this->_io->destroy(::__awh_cache__.tid);
			// Сбрасываем идентификатор события интервала
			::__awh_cache__.tid = 0;
			// Сбрасываем интервал сохранения дампа кэша
			::__awh_cache__.interval = 0;
			// Сбрасываем путь к файлу дампа кэша
			::__awh_cache__.filename = "";
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(filename, interval), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки таймаута для ожидания ответа от DNS-сервера
 *
 * @param delay время ожидания ответа от DNS-сервера (в миллисекундах)
 *
 */
void awh::unit::DNS::setTimeout(const uint32_t delay) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Устанавливаем время ожидания ответа от DNS-сервера
		this->_resolver.delay = delay;
		// Если DNS-резолверы IPv4 уже инициализированы
		if(!this->_resolver.idv4.empty()){
			/**
			 * Выполняем перебор всех событий DNS-резолвера для семейства IPv4
			 */
			for(auto & eid : this->_resolver.idv4)
				// Устанавливаем время ожидания ответа от DNS-сервера
				this->_io->setTimeout(eid, event::action_t::READ, this->_resolver.delay);
		}
		// Если DNS-резолверы IPv6 уже инициализированы
		if(!this->_resolver.idv6.empty()){
			/**
			 * Выполняем перебор всех событий DNS-резолвера для семейства IPv6
			 */
			for(auto & eid : this->_resolver.idv6)
				// Устанавливаем время ожидания ответа от DNS-сервера
				this->_io->setTimeout(eid, event::action_t::READ, this->_resolver.delay);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(delay), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения количества DNS-резолверов для выполнения запросов к DNS-серверам
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @return       количество DNS-резолверов
 *
 */
uint16_t awh::unit::DNS::resolvers(const event::family_t family) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
				// Возвращаем количество DNS-резолверов для семейства IPv4
				return static_cast <uint16_t> (this->_resolver.idv4.size());
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
				// Возвращаем количество DNS-резолверов для семейства IPv6
				return static_cast <uint16_t> (this->_resolver.idv6.size());
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод инициализации DNS-резолверов
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param count  количество DNS-резолверов для инициализации
 * @return       результат выполнения операции
 *
 */
bool awh::unit::DNS::init(const event::family_t family, const uint16_t count) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем доступ к состоянию передачи DNS-запросов и очереди резолверов
		const locker_t <> lock(this->_mtx);
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если DNS-резолверы IPv4 уже инициализированы
				if(!this->_resolver.idv4.empty()){
					/**
					 * Удаляем все события DNS-резолвера
					 */
					for(auto i = this->_resolver.idv4.begin(); i != this->_resolver.idv4.end();){
						// Удаляем идентификатор события из очереди свободных резолверов
						this->_resolver.queue.remove(* i);
						// Удаляем событие DNS-резолвера
						this->_io->destroy(* i);
						// Удаляем идентификатор события DNS-резолвера из списка идентификаторов
						i = this->_resolver.idv4.erase(i);
					}
				}
				// Если количество DNS-резолверов установлено
				if(count > 0){
					// Ресайзим список идентификаторов событий DNS-резолвера для семейства IPv4
					this->_resolver.idv4.resize(count, 0);
					/**
					 * Выполняем перебор всех создаваемых DNS-резолверов для семейства IPv4
					 */
					for(uint16_t i = 0; i < count; i++){
						// Сбрасываем результат инициализации текущего резолвера
						result = false;
						// Добавляем новое событие клиента UDP
						this->_resolver.idv4[i] = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
						// Если опции события установлены
						if(this->_io->setOptions(this->_resolver.idv4[i], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
							// Устанавливаем порт события
							if(this->_io->setTargetPort(this->_resolver.idv4[i], this->_resolver.port)){
								// Устанавливаем время ожидания ответа от DNS-сервера
								this->_io->setTimeout(this->_resolver.idv4[i], event::action_t::READ, this->_resolver.delay);
								// Если префикс для переменных окружения установлен
								if(!this->_resolver.prefix.empty()){
									// Получаем значение переменной
									const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV4_SERVER", this->_resolver.prefix.c_str()).c_str());
									// Если IP-адрес из переменной окружения получен
									if(env != nullptr)
										// Устанавливаем адрес сервера назначения
										result = this->_io->setTarget(this->_resolver.idv4[i], env);
								}
								// Если адрес сервера назначения не установлен из переменной окружения
								if(!result)
									// Устанавливаем адрес сервера назначения
									result = this->_io->setTarget(this->_resolver.idv4[i], this->_resolver.nameServers.get(family));
								// Если адрес сервера назначения установлен
								if(result){
									// Если адрес сети для выполнения запроса установлен
									if((this->_resolver.sourceIPv4 != nullptr) && (this->_resolver.sourceIPv4->size == 4))
										// Добавляем DNS-сервер в список
										this->_io->setAddress(this->_resolver.idv4[i], event::address_t::IPV4, this->_resolver.sourceIPv4.get());
									// Устанавливаем функцию обратного вызова на событие получения ошибок
									this->_io->on(this->_resolver.idv4[i], static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
									// Устанавливаем функцию обратного вызова на событие чтения данных
									this->_io->on(this->_resolver.idv4[i], static_cast <engine::callback::read_t> (std::bind(&dns_t::response, this, _1, _2, _3)));
									// Устанавливаем функцию обратного вызова на событие таймаута
									this->_io->on(this->_resolver.idv4[i], static_cast <engine::callback::timeout_t> (std::bind(static_cast <bool (dns_t::*)(const event::id_t, const event::action_t, const uint32_t)> (&dns_t::timeout), this, _1, _2, _3)));
									// Выполняем фиксацию параметров события и его запуск
									result = (this->_io->commit(this->_resolver.idv4[i]) && this->_io->launch(this->_resolver.idv4[i]));
								}
							}
						}
						// Если не удалось запустить событие DNS-резолвера, то удаляем
						if(!result){
							// Удаляем событие DNS-резолвера
							this->_io->destroy(this->_resolver.idv4[i]);
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Failed to set options for DNS resolver event", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), count), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to set options for DNS resolver event", log_t::flag_t::CRITICAL);
								#endif
							}
							// Пропускаем неудачно инициализированный резолвер
							continue;
						// Добавляем резолвер в очередь доступных DNS-резолверов
						} else {
							this->_resolver.queue.push(this->_resolver.idv4[i]);
							// Устанавливаем признак успешной инициализации
							result = true;
						}
					}
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если DNS-резолверы IPv6 уже инициализированы
				if(!this->_resolver.idv6.empty()){
					/**
					 * Удаляем все события DNS-резолвера
					 */
					for(auto i = this->_resolver.idv6.begin(); i != this->_resolver.idv6.end();){
						// Удаляем идентификатор события из очереди свободных резолверов
						this->_resolver.queue.remove(* i);
						// Удаляем событие DNS-резолвера
						this->_io->destroy(* i);
						// Удаляем идентификатор события DNS-резолвера из списка идентификаторов
						i = this->_resolver.idv6.erase(i);
					}
				}
				// Если количество DNS-резолверов установлено
				if(count > 0){
					// Ресайзим список идентификаторов событий DNS-резолвера для семейства IPv6
					this->_resolver.idv6.resize(count, 0);
					/**
					 * Выполняем перебор всех создаваемых DNS-резолверов для семейства IPv6
					 */
					for(uint16_t i = 0; i < count; i++){
						// Сбрасываем результат инициализации текущего резолвера
						result = false;
						// Добавляем новое событие клиента UDP
						this->_resolver.idv6[i] = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
						// Если опции события установлены
						if(this->_io->setOptions(this->_resolver.idv6[i], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
							// Устанавливаем порт события
							if(this->_io->setTargetPort(this->_resolver.idv6[i], this->_resolver.port)){
								// Устанавливаем время ожидания ответа от DNS-сервера
								this->_io->setTimeout(this->_resolver.idv6[i], event::action_t::READ, this->_resolver.delay);
								// Если префикс для переменных окружения установлен
								if(!this->_resolver.prefix.empty()){
									// Получаем значение переменной
									const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV6_SERVER", this->_resolver.prefix.c_str()).c_str());
									// Если IP-адрес из переменной окружения получен
									if(env != nullptr)
										// Устанавливаем адрес сервера назначения
										result = this->_io->setTarget(this->_resolver.idv6[i], env);
								}
								// Если адрес сервера назначения не установлен из переменной окружения
								if(!result)
									// Устанавливаем адрес сервера назначения
									result = this->_io->setTarget(this->_resolver.idv6[i], this->_resolver.nameServers.get(family));
								// Если адрес сервера назначения установлен
								if(result){
									// Если адрес сети для выполнения запроса установлен
									if((this->_resolver.sourceIPv6 != nullptr) && (this->_resolver.sourceIPv6->size == 16))
										// Добавляем DNS-сервер в список
										this->_io->setAddress(this->_resolver.idv6[i], event::address_t::IPV6, this->_resolver.sourceIPv6.get());
									// Устанавливаем функцию обратного вызова на событие получения ошибок
									this->_io->on(this->_resolver.idv6[i], static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
									// Устанавливаем функцию обратного вызова на событие чтения данных
									this->_io->on(this->_resolver.idv6[i], static_cast <engine::callback::read_t> (std::bind(&dns_t::response, this, _1, _2, _3)));
									// Устанавливаем функцию обратного вызова на событие таймаута
									this->_io->on(this->_resolver.idv6[i], static_cast <engine::callback::timeout_t> (std::bind(static_cast <bool (dns_t::*)(const event::id_t, const event::action_t, const uint32_t)> (&dns_t::timeout), this, _1, _2, _3)));
									// Выполняем фиксацию параметров события и его запуск
									result = (this->_io->commit(this->_resolver.idv6[i]) && this->_io->launch(this->_resolver.idv6[i]));
								}
							}
						}
						// Если не удалось запустить событие DNS-резолвера, то удаляем
						if(!result){
							// Удаляем событие DNS-резолвера
							this->_io->destroy(this->_resolver.idv6[i]);
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Failed to set options for DNS resolver event", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), count), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to set options for DNS resolver event", log_t::flag_t::CRITICAL);
								#endif
							}
							// Пропускаем неудачно инициализированный резолвер
							continue;
						// Добавляем резолвер в очередь доступных DNS-резолверов
						} else {
							this->_resolver.queue.push(this->_resolver.idv6[i]);
							// Устанавливаем признак успешной инициализации
							result = true;
						}
					}
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), count), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения UDP-порта DNS-сервера
 *
 * @return UDP-порт DNS-сервера
 *
 */
uint16_t awh::unit::DNS::getTargetPort() const noexcept {
	// Получаем порт события
	return this->_resolver.port;
}
/**
 * @brief Метод установки UDP-порта DNS-сервера
 *
 * @param port UDP-порт DNS-сервера
 *
 */
void awh::unit::DNS::setTargetPort(const uint16_t port) noexcept {
	// Если порт для установки передан
	if(port > 0)
		// Устанавливаем порт события
		this->_resolver.port = port;
}
/**
 * @brief Метод установки адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для установки
 *
 */
void awh::unit::DNS::setServer(string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if(!server.empty()){
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(server)){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Сбрасываем список DNS-серверов для семейства IPv4
						this->_resolver.nameServers.reset(event::family_t::IPV4);
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Сбрасываем список DNS-серверов для семейства IPv6
						this->_resolver.nameServers.reset(event::family_t::IPV6);
					break;
				}
				// Добавляем DNS-сервер в список
				this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Переинициализируем DNS-резолвер для семейства IPv4
						this->init(event::family_t::IPV4, this->_resolver.idv4.size());
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Переинициализируем DNS-резолвер для семейства IPv6
						this->init(event::family_t::IPV6, this->_resolver.idv6.size());
					break;
				}
			}
		// Если адрес DNS-сервера не передан
		} else {
			// Сбрасываем список DNS-серверов для семейства IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Сбрасываем список DNS-серверов для семейства IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
			// Переинициализируем DNS-резолвер для семейства IPv4
			this->init(event::family_t::IPV4, this->_resolver.idv4.size());
			// Переинициализируем DNS-резолвер для семейства IPv6
			this->init(event::family_t::IPV6, this->_resolver.idv6.size());
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для установки
 *
 */
void awh::unit::DNS::setServer(const net::addr_t * server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if(server != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Сбрасываем список DNS-серверов для семейства IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
					// Добавляем DNS-сервер в список
					this->_resolver.nameServers.push(server);
					// Переинициализируем DNS-резолвер для семейства IPv4
					this->init(event::family_t::IPV4, this->_resolver.idv4.size());
				} break;
				// Если адрес является IPv6
				case 16: {
					// Сбрасываем список DNS-серверов для семейства IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
					// Добавляем DNS-сервер в список
					this->_resolver.nameServers.push(server);
					// Переинициализируем DNS-резолвер для семейства IPv6
					this->init(event::family_t::IPV6, this->_resolver.idv6.size());
				} break;
			}
		// Если адрес DNS-сервера не передан
		} else {
			// Сбрасываем список DNS-серверов для семейства IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Сбрасываем список DNS-серверов для семейства IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
			// Переинициализируем DNS-резолвер для семейства IPv4
			this->init(event::family_t::IPV4, this->_resolver.idv4.size());
			// Переинициализируем DNS-резолвер для семейства IPv6
			this->init(event::family_t::IPV6, this->_resolver.idv6.size());
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса DNS-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param server адрес DNS-сервера для установки
 *
 */
void awh::unit::DNS::setServer(const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if(!server.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV4)){
						// Сбрасываем список DNS-серверов для семейства IPv4
						this->_resolver.nameServers.reset(event::family_t::IPV4);
						// Добавляем DNS-сервер в список
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Переинициализируем DNS-резолвер для семейства IPv4
						this->init(event::family_t::IPV4, this->_resolver.idv4.size());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV6)){
						// Сбрасываем список DNS-серверов для семейства IPv6
						this->_resolver.nameServers.reset(event::family_t::IPV6);
						// Добавляем DNS-сервер в список
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Переинициализируем DNS-резолвер для семейства IPv6
						this->init(event::family_t::IPV6, this->_resolver.idv6.size());
					}
				} break;
			}
		// Если адрес DNS-сервера не передан
		} else {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Сбрасываем список DNS-серверов для семейства IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
					// Переинициализируем DNS-резолвер для семейства IPv4
					this->init(event::family_t::IPV4, this->_resolver.idv4.size());
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Сбрасываем список DNS-серверов для семейства IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
					// Переинициализируем DNS-резолвер для семейства IPv6
					this->init(event::family_t::IPV6, this->_resolver.idv6.size());
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для добавления
 *
 */
void awh::unit::DNS::addServer(string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if(!server.empty()){
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(server)){
				// Добавляем DNS-сервер в список
				this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Переинициализируем DNS-резолвер для семейства IPv4
						this->init(event::family_t::IPV4, this->_resolver.idv4.size());
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Переинициализируем DNS-резолвер для семейства IPv6
						this->init(event::family_t::IPV6, this->_resolver.idv6.size());
					break;
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для добавления
 *
 */
void awh::unit::DNS::addServer(const net::addr_t * server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if(server != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Добавляем DNS-сервер в список
					this->_resolver.nameServers.push(server);
					// Переинициализируем DNS-резолвер для семейства IPv4
					this->init(event::family_t::IPV4, this->_resolver.idv4.size());
				} break;
				// Если адрес является IPv6
				case 16: {
					// Добавляем DNS-сервер в список
					this->_resolver.nameServers.push(server);
					// Переинициализируем DNS-резолвер для семейства IPv6
					this->init(event::family_t::IPV6, this->_resolver.idv6.size());
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса DNS-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param server адрес DNS-сервера для добавления
 *
 */
void awh::unit::DNS::addServer(const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if(!server.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV4)){
						// Добавляем DNS-сервер в список
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Переинициализируем DNS-резолвер для семейства IPv4
						this->init(event::family_t::IPV4, this->_resolver.idv4.size());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV6)){
						// Добавляем DNS-сервер в список
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Переинициализируем DNS-резолвер для семейства IPv6
						this->init(event::family_t::IPV6, this->_resolver.idv6.size());
					}
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов DNS-серверов
 *
 * @param servers адреса DNS-серверов для установки
 *
 */
void awh::unit::DNS::setServers(const vector <string> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса DNS-серверов переданы
		if(!servers.empty()){
			// Результат выполнения парсинга IP-адреса
			bool result = false;
			// Флаг необходимости сброса списка DNS-серверов для семейства IPv4
			bool resetIPv4 = false;
			// Флаг необходимости сброса списка DNS-серверов для семейства IPv6
			bool resetIPv6 = false;
			/**
			 * Проходим по каждому адресу DNS-сервера для установки
			 */
			for(const auto & server : servers){
				// Выполняем парсинг IP-адреса
				if((result = this->_addr.parse(server))){
					/**
					 * Определяем тип IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_addr.type())){
						// Если адрес является IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
							// Отмечаем необходимость сброса списка DNS-серверов для семейства IPv4
							resetIPv4 = true;
						break;
						// Если адрес является IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
							// Отмечаем необходимость сброса списка DNS-серверов для семейства IPv6
							resetIPv6 = true;
						break;
					}
				// Выходим из цикла
				} else break;
			}
			// Если парсинг адресов выполнен
			if(result){
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Сбрасываем список DNS-серверов для семейства IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Сбрасываем список DNS-серверов для семейства IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
				/**
				 * Проходим по каждому адресу DNS-сервера для установки
				 */
				for(const auto & server : servers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(server))
						// Добавляем DNS-сервер в список
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
				}
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Переинициализируем DNS-резолвер для семейства IPv4
					this->init(event::family_t::IPV4, this->_resolver.idv4.size());
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Переинициализируем DNS-резолвер для семейства IPv6
					this->init(event::family_t::IPV6, this->_resolver.idv6.size());
			}
		// Если адреса DNS-серверов не переданы
		} else {
			// Сбрасываем список DNS-серверов для семейства IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Сбрасываем список DNS-серверов для семейства IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
			// Переинициализируем DNS-резолвер для семейства IPv4
			this->init(event::family_t::IPV4, this->_resolver.idv4.size());
			// Переинициализируем DNS-резолвер для семейства IPv6
			this->init(event::family_t::IPV6, this->_resolver.idv6.size());
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов DNS-серверов
 *
 * @param servers адреса DNS-серверов для установки
 *
 */
void awh::unit::DNS::setServers(const vector <const net::addr_t *> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса DNS-серверов переданы
		if(!servers.empty()){
			// Флаг необходимости сброса списка DNS-серверов для семейства IPv4
			bool resetIPv4 = false;
			// Флаг необходимости сброса списка DNS-серверов для семейства IPv6
			bool resetIPv6 = false;
			/**
			 * Проходим по каждому адресу DNS-сервера для установки
			 */
			for(const auto & server : servers){
				// Если адрес DNS-сервера передан
				if(server != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(server->size){
						// Если адрес является IPv4
						case 4:
							// Отмечаем необходимость сброса списка DNS-серверов для семейства IPv4
							resetIPv4 = true;
						break;
						// Если адрес является IPv6
						case 16:
							// Отмечаем необходимость сброса списка DNS-серверов для семейства IPv6
							resetIPv6 = true;
						break;
					}
				}
			}
			// Если необходимо сбросить список IPv4 или IPv6
			if(resetIPv4 || resetIPv6){
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Сбрасываем список DNS-серверов для семейства IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Сбрасываем список DNS-серверов для семейства IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
				/**
				 * Проходим по каждому адресу DNS-сервера для установки
				 */
				for(const auto & server : servers){
					// Если адрес DNS-сервера передан
					if(server != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(server->size){
							// Если адрес является IPv4
							case 4:
								// Добавляем DNS-сервер в список
								this->_resolver.nameServers.push(server);
							break;
							// Если адрес является IPv6
							case 16:
								// Добавляем DNS-сервер в список
								this->_resolver.nameServers.push(server);
							break;
						}
					}
				}
			}
			// Если необходимо сбросить список IPv4
			if(resetIPv4)
				// Переинициализируем DNS-резолвер для семейства IPv4
				this->init(event::family_t::IPV4, this->_resolver.idv4.size());
			// Если необходимо сбросить список IPv6
			if(resetIPv6)
				// Переинициализируем DNS-резолвер для семейства IPv6
				this->init(event::family_t::IPV6, this->_resolver.idv6.size());
		// Если адрес DNS-сервера не передан
		} else {
			// Сбрасываем список DNS-серверов для семейства IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Сбрасываем список DNS-серверов для семейства IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
			// Переинициализируем DNS-резолвер для семейства IPv4
			this->init(event::family_t::IPV4, this->_resolver.idv4.size());
			// Переинициализируем DNS-резолвер для семейства IPv6
			this->init(event::family_t::IPV6, this->_resolver.idv6.size());
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов DNS-серверов
 *
 * @param family  семейство IP-адресов IPv4/IPv6
 * @param servers адреса DNS-серверов для установки
 *
 */
void awh::unit::DNS::setServers(const event::family_t family, const vector <string> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса DNS-серверов переданы
		if(!servers.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Сбрасываем список DNS-серверов для семейства IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
					/**
					 * Проходим по каждому адресу DNS-сервера для установки
					 */
					for(const auto & server : servers){
						// Выполняем парсинг IPv4-адреса
						if(this->_addr.parse(server, net_addr_t::type_t::IPV4))
							// Добавляем DNS-сервер в список
							this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Выходим из цикла
						else break;
					}
					// Переинициализируем DNS-резолвер для семейства IPv4
					this->init(event::family_t::IPV4, this->_resolver.idv4.size());
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Сбрасываем список DNS-серверов для семейства IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
					/**
					 * Проходим по каждому адресу DNS-сервера для установки
					 */
					for(const auto & server : servers){
						// Выполняем парсинг IPv6-адреса
						if(this->_addr.parse(server, net_addr_t::type_t::IPV6))
							// Добавляем DNS-сервер в список
							this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Выходим из цикла
						else break;
					}
					// Переинициализируем DNS-резолвер для семейства IPv6
					this->init(event::family_t::IPV6, this->_resolver.idv6.size());
				} break;
			}
		// Если адрес DNS-сервера не передан
		} else {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Сбрасываем список DNS-серверов для семейства IPv4
					this->_resolver.nameServers.reset(family);
					// Переинициализируем DNS-резолвер для семейства IPv4
					this->init(event::family_t::IPV4, this->_resolver.idv4.size());
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Сбрасываем список DNS-серверов для семейства IPv6
					this->_resolver.nameServers.reset(family);
					// Переинициализируем DNS-резолвер для семейства IPv6
					this->init(event::family_t::IPV6, this->_resolver.idv6.size());
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 *
 */
void awh::unit::DNS::setSource(string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!source.empty()){
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(source)){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Получаем IP-адрес в исходном виде
						this->_resolver.sourceIPv4 = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Получаем IP-адрес в исходном виде
						this->_resolver.sourceIPv6 = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					break;
				}
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Сбрасываем IPv4-адрес события
			this->_resolver.sourceIPv4.reset(nullptr);
			// Сбрасываем IPv6-адрес события
			this->_resolver.sourceIPv6.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 *
 */
void awh::unit::DNS::setSource(const net::addr_t * source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(source != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(source->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем инициализацию объекта IP-адреса
					this->_resolver.sourceIPv4 = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_resolver.sourceIPv4.get())->address = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем инициализацию объекта IP-адреса
					this->_resolver.sourceIPv6 = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_resolver.sourceIPv6.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], 16);
				} break;
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Сбрасываем IPv4-адрес события
			this->_resolver.sourceIPv4.reset(nullptr);
			// Сбрасываем IPv6-адрес события
			this->_resolver.sourceIPv6.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param source адрес сети для выполнения запроса
 *
 */
void awh::unit::DNS::setSource(const event::family_t family, string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!source.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(source, net_addr_t::type_t::IPV4))
						// Получаем IP-адрес в исходном виде
						this->_resolver.sourceIPv4 = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(source, net_addr_t::type_t::IPV6))
						// Получаем IP-адрес в исходном виде
						this->_resolver.sourceIPv6 = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Сбрасываем IPv4-адрес события
			this->_resolver.sourceIPv4.reset(nullptr);
			// Сбрасываем IPv6-адрес события
			this->_resolver.sourceIPv6.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод генерации идентификатора DNS-запроса
 *
 * @return уникальный идентификатор DNS-запроса
 *
 */
awh::unit::DNS::id_t awh::unit::DNS::issue() const noexcept {
	// Создаём идентификатор DNS-запроса
	return ::dns::identifier();
}
/**
 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
 *
 * @param id    идентификатор DNS-запроса
 * @param ip    адрес для обратного DNS-запроса
 * @param alive срок ожидания ответа (в миллисекундах)
 * @return      результат постановки запроса в очередь
 *
 */
bool awh::unit::DNS::search(const id_t id, string_view ip, const uint32_t alive) noexcept {
	// Если список резолверов для семейства IPv6 не пустой
	if(!this->_resolver.idv6.empty())
		// Выполняем обратный DNS-запрос для IPv6
		return this->search(id, event::family_t::IPV6, ip, alive);
	// Если список резолверов для семейства IPv4 не пустой
	if(!this->_resolver.idv4.empty())
		// Выполняем обратный DNS-запрос для IPv4
		return this->search(id, event::family_t::IPV4, ip, alive);
	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
 *
 * @param id    идентификатор DNS-запроса
 * @param ip    адрес для обратного DNS-запроса
 * @param alive срок ожидания ответа (в миллисекундах)
 * @return      результат постановки запроса в очередь
 *
 */
bool awh::unit::DNS::search(const id_t id, const net::addr_t * ip, const uint32_t alive) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(ip != nullptr){
			// Признак найденной записи в кэше
			bool cacheHit = false;
			// Доменное имя найденной записи
			string cacheDomain = "";
			// Семейство найденной записи
			event::family_t cacheFamily = event::family_t::NONE;
			// Устанавливаем полученный IP-адрес
			this->_addr.source(ip);
			// Извлекаем доменное имя в формате ARPA
			const string domain = ::move(this->_addr.arpa());
			{
				// Блокируем доступ к глобальному кэшу DNS
				const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
				/**
				 * Определяем тип адреса
				 */
				switch(static_cast <uint8_t> (ip->size)){
					// Если адрес является IPv4
					case 4: {
						// Выполняем поиск IP-адреса в кэше
						auto i = ::__awh_cache__.ipv4.find(awh_cast <const net::addr_net_ipv4_t *> (ip)->address);
						// Если в кэше IP-адрес найден
						if(i != ::__awh_cache__.ipv4.end()){
							// Получаем текущую метку времени
							const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
							/**
							 * Выполняем перебор всех записей IP-адреса в кэше
							 */
							for(auto j = i->second.begin(); j != i->second.end(); ++j){
								// Пропускаем устаревшие записи в кэше (удаление будет при записи)
								if((j->life > 0) && (j->life <= now))
									// Пропускаем записи IP-адреса
									continue;
								// Устанавливаем признак найденной записи в кэше
								cacheHit = true;
								// Устанавливаем доменное имя найденной записи
								cacheDomain = j->domain;
								// Устанавливаем семейство найденной записи
								cacheFamily = event::family_t::IPV4;
								// Выходим из цикла
								break;
							}
							// Если запись найдена
							if(cacheHit)
								// Выходим из обхода
								break;
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Выполняем поиск IP-адреса в кэше
						auto i = ::__awh_cache__.ipv6.find(awh_cast <const net::addr_net_ipv6_t *> (ip)->address);
						// Если в кэше IP-адрес найден
						if(i != ::__awh_cache__.ipv6.end()){
							// Получаем текущую метку времени
							const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
							/**
							 * Выполняем перебор всех записей IP-адреса в кэше
							 */
							for(auto j = i->second.begin(); j != i->second.end(); ++j){
								// Пропускаем устаревшие записи в кэше (удаление будет при записи)
								if((j->life > 0) && (j->life <= now))
									// Пропускаем записи IP-адреса
									continue;
								// Устанавливаем признак найденной записи в кэше
								cacheHit = true;
								// Устанавливаем доменное имя найденной записи
								cacheDomain = j->domain;
								// Устанавливаем семейство найденной записи
								cacheFamily = event::family_t::IPV6;
								// Выходим из цикла
								break;
							}
							// Если запись найдена
							if(cacheHit)
								// Выходим из обхода
								break;
						}
					} break;
				}
			}
			// Вызываем callback только после выхода из блокировки кэша
			if(cacheHit){
				// Выполняем функцию обратного вызова для найденной записи в кэше
				this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, cacheFamily, cacheDomain, ip);
				// Возвращаем положительный результат
				return true;
			}
			// Блокируем доступ к состоянию передачи DNS-запросов
			const locker_t <> lock(this->_mtx);
			// Если доменное имя получено
			if(!domain.empty()){
				// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
				const size_t size = ::dns::request(id, record_t::PTR, domain, this->_log);
				// Если DNS-запрос не сформирован
				if(size == 0)
					// Возвращаем отрицательный результат
					return false;
				/**
				 * Устанавливаем метку начала формирования запроса к DNS-серверу
				 */
				Begin:
				// Если в очереди не осталось свободных резолверов для выполнения запроса
				if(this->_resolver.queue.size() == 0){
					// Если очередь ожидания выполнения запроса переполнена
					if(this->_transfer.packets.size() >= this->_transfer.maxPackets){
						// Формируем текст сообщения об ошибке DNS-резолвера
						const string error = this->_fmk->format("DNS resolver queue is full for domain %s", domain.c_str());
						// Если функция обратного вызова установлена
						if(this->_callback.is("error")){
							// Идентификатор события клиента DNS-резолвера
							event::id_t eid = 0;
							// Если список DNS-резолверов для семейства IPv4 не пустой
							if(!this->_resolver.idv4.empty())
								// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv4
								eid = this->_resolver.idv4.front();
							// Если список DNS-резолверов для семейства IPv6 не пустой
							else if(!this->_resolver.idv6.empty())
								// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv6
								eid = this->_resolver.idv6.front();
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, error);
						// Если callback ошибки не установлен
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, alive), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
						// Выполняем функцию обратного вызова для неудачного резолвинга доменного имени
						this->_callback.call <void (const id_t, const record_t, const string &)> ("failure", id, record_t::PTR, domain);
						// Возвращаем отрицательный результат
						return false;
					// Если очередь ещё может вместить в себя новый пакет
					} else {
						// Добавляем новый пакет в контейнер очереди ожидания выполнения запроса к DNS-серверу
						this->_transfer.packets.push(packet_t());
						// Устанавливаем размер полезной нагрузки
						this->_transfer.packets.back().payload.size = size;
						// Выделяем новый буфер для полезной нагрузки
						this->_transfer.packets.back().payload.buffer = make_unique <uint8_t []> (size);
						// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
						::memcpy(this->_transfer.packets.back().payload.buffer.get(), ::dns::buffer, size);
						// Устанавливаем время жизни пакета для отслеживания его выполнения
						this->_transfer.packets.back().alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
						// Выходим из функции, так как пакет успешно добавлен в очередь на отправку
						return true;
					}
				}
				// Добавляем пакет в контейнер активных пакетов
				auto ret = this->_transfer.waiting.emplace(id, packet_t());
				// Если пакет успешно добавлен в контейнер активных пакетов
				if(ret.second){
					// Устанавливаем размер полезной нагрузки
					ret.first->second.payload.size = size;
					// Выделяем новый буфер для полезной нагрузки
					ret.first->second.payload.buffer = make_unique <uint8_t []> (size);
					// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
					::memcpy(ret.first->second.payload.buffer.get(), ::dns::buffer, size);
					// Устанавливаем время жизни пакета для отслеживания его выполнения
					ret.first->second.alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
				// Если пакет не добавлен в контейнер активных пакетов
				} else {
					// Добавляем новый пакет в контейнер очереди ожидания выполнения запроса к DNS-серверу
					this->_transfer.packets.push(packet_t());
					// Устанавливаем размер полезной нагрузки
					this->_transfer.packets.back().payload.size = size;
					// Выделяем новый буфер для полезной нагрузки
					this->_transfer.packets.back().payload.buffer = make_unique <uint8_t []> (size);
					// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
					::memcpy(this->_transfer.packets.back().payload.buffer.get(), ::dns::buffer, size);
					// Устанавливаем время жизни пакета для отслеживания его выполнения
					this->_transfer.packets.back().alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
					// Выходим из функции, так как пакет успешно добавлен в очередь на отправку
					return true;
				}
				// Идентификатор события клиента DNS-резолвера
				event::id_t eid = 0;
				// Получаем идентификатор события клиента DNS-резолвера для отправки запроса к DNS-серверу
				this->_resolver.queue.pop(eid);
				// Если идентификатор события клиента DNS-резолвера не получен
				if(eid == 0)
					// Пытаемся повторить процедуру повторно
					goto Begin;
				// Добавляем идентификатор события клиента DNS-резолвера в контейнер соответствий с DNS-запросами
				else this->_transfer.attached.emplace(eid, id);
				// Отправляем DNS-запрос
				return (this->_io->send(eid, ::dns::buffer, size) > 0);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, alive), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство IP-адресов IPv4/IPv6
 * @param ip     адрес для обратного DNS-запроса
 * @param alive  срок ожидания ответа (в миллисекундах)
 * @return       результат постановки запроса в очередь
 *
 */
bool awh::unit::DNS::search(const id_t id, const event::family_t family, string_view ip, const uint32_t alive) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса не передан
		if(ip.empty())
			// Возвращаем отрицательный результат
			return false;
		// Тип IP-адреса для парсинга
		net_addr_t::type_t type = net_addr_t::type_t::NONE;
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
				// Устанавливаем тип IPv4-адреса
				type = net_addr_t::type_t::IPV4;
			break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
				// Устанавливаем тип IPv6-адреса
				type = net_addr_t::type_t::IPV6;
			break;
			// Если семейство события не определено
			default:
				// Возвращаем отрицательный результат
				return false;
		}
		// Локальный парсер сетевых адресов
		net_addr_t addr(this->_fmk, this->_log);
		// Если IP-адрес не распознан
		if(!addr.parse(ip, type))
			// Возвращаем отрицательный результат
			return false;
		// Получаем IP-адрес в исходном виде
		unique_ptr <net::addr_t> parsed = ::move(addr.source(net_addr_t::endian_t::LITTLE));
		// Выполняем обратный DNS-запрос через основной метод
		return this->search(id, parsed.get(), alive);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), ip, alive), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод выполнения произвольного DNS-запроса
 *
 * @param id     идентификатор DNS-запроса
 * @param record тип DNS-записи, которую необходимо получить
 * @param domain доменное имя
 * @param alive  срок ожидания ответа (в миллисекундах)
 * @return       результат постановки запроса в очередь
 *
 */
bool awh::unit::DNS::request(const id_t id, const record_t record, string_view domain, const uint32_t alive) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя получено
		if(!domain.empty()){
			// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
			const size_t size = ::dns::request(id, record, domain, this->_log);
			// Если DNS-запрос не сформирован
			if(size == 0)
				// Возвращаем отрицательный результат
				return false;
			// Блокируем доступ к состоянию передачи DNS-запросов
			const locker_t <> lock(this->_mtx);
			/**
			 * Устанавливаем метку начала формирования запроса к DNS-серверу
			 */
			Begin:
			// Если в очереди не осталось свободных резолверов для выполнения запроса
			if(this->_resolver.queue.size() == 0){
				// Если очередь ожидания выполнения запроса переполнена
				if(this->_transfer.packets.size() >= this->_transfer.maxPackets){
					// Формируем текст сообщения об ошибке DNS-резолвера
					const string error = this->_fmk->format("DNS resolver queue is full for domain %s", string(domain).c_str());
					// Если функция обратного вызова установлена
					if(this->_callback.is("error")){
						// Идентификатор события клиента DNS-резолвера
						event::id_t eid = 0;
						// Если список DNS-резолверов для семейства IPv4 не пустой
						if(!this->_resolver.idv4.empty())
							// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv4
							eid = this->_resolver.idv4.front();
						// Если список DNS-резолверов для семейства IPv6 не пустой
						else if(!this->_resolver.idv6.empty())
							// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv6
							eid = this->_resolver.idv6.front();
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, error);
					// Если callback ошибки не установлен
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (record), domain, alive), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
					// Выполняем функцию обратного вызова для неудачного резолвинга доменного имени
					this->_callback.call <void (const id_t, const record_t, const string &)> ("failure", id, record, string{domain});
					// Возвращаем отрицательный результат
					return false;
				// Если очередь ещё может вместить в себя новый пакет
				} else {
					// Добавляем новый пакет в контейнер очереди ожидания выполнения запроса к DNS-серверу
					this->_transfer.packets.push(packet_t());
					// Устанавливаем размер полезной нагрузки
					this->_transfer.packets.back().payload.size = size;
					// Выделяем новый буфер для полезной нагрузки
					this->_transfer.packets.back().payload.buffer = make_unique <uint8_t []> (size);
					// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
					::memcpy(this->_transfer.packets.back().payload.buffer.get(), ::dns::buffer, size);
					// Устанавливаем время жизни пакета для отслеживания его выполнения
					this->_transfer.packets.back().alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
					// Выходим из функции, так как пакет успешно добавлен в очередь на отправку
					return true;
				}
			}
			// Добавляем пакет в контейнер активных пакетов
			auto ret = this->_transfer.waiting.emplace(id, packet_t());
			// Если пакет успешно добавлен в контейнер активных пакетов
			if(ret.second){
				// Устанавливаем размер полезной нагрузки
				ret.first->second.payload.size = size;
				// Выделяем новый буфер для полезной нагрузки
				ret.first->second.payload.buffer = make_unique <uint8_t []> (size);
				// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
				::memcpy(ret.first->second.payload.buffer.get(), ::dns::buffer, size);
				// Устанавливаем время жизни пакета для отслеживания его выполнения
				ret.first->second.alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
			// Если пакет не добавлен в контейнер активных пакетов
			} else {
				// Добавляем новый пакет в контейнер очереди ожидания выполнения запроса к DNS-серверу
				this->_transfer.packets.push(packet_t());
				// Устанавливаем размер полезной нагрузки
				this->_transfer.packets.back().payload.size = size;
				// Выделяем новый буфер для полезной нагрузки
				this->_transfer.packets.back().payload.buffer = make_unique <uint8_t []> (size);
				// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
				::memcpy(this->_transfer.packets.back().payload.buffer.get(), ::dns::buffer, size);
				// Устанавливаем время жизни пакета для отслеживания его выполнения
				this->_transfer.packets.back().alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
				// Выходим из функции, так как пакет успешно добавлен в очередь на отправку
				return true;
			}
			// Идентификатор события клиента DNS-резолвера
			event::id_t eid = 0;
			// Получаем идентификатор события клиента DNS-резолвера для отправки запроса к DNS-серверу
			this->_resolver.queue.pop(eid);
			// Если идентификатор события клиента DNS-резолвера не получен
			if(eid == 0)
				// Пытаемся повторить процедуру повторно
				goto Begin;
			// Добавляем идентификатор события клиента DNS-резолвера в контейнер соответствий с DNS-запросами
			else this->_transfer.attached.emplace(eid, id);
			// Отправляем DNS-запрос
			return (this->_io->send(eid, ::dns::buffer, size) > 0);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (record), domain, alive), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод разрешения доменного имени
 *
 * @param id     идентификатор DNS-запроса
 * @param domain доменное имя
 * @param alive  срок ожидания ответа (в миллисекундах)
 * @return       результат постановки запроса в очередь
 *
 */
bool awh::unit::DNS::resolve(const id_t id, string_view domain, const uint32_t alive) noexcept {
	// Если список резолверов для семейства IPv6 не пустой
	if(!this->_resolver.idv6.empty())
		// Выполняем разрешение доменного имени
		return this->resolve(id, event::family_t::IPV6, domain, alive);
	// Если список резолверов для семейства IPv4 не пустой
	if(!this->_resolver.idv4.empty())
		// Выполняем разрешение доменного имени
		return this->resolve(id, event::family_t::IPV4, domain, alive);
	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод разрешения доменного имени
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя
 * @param alive  срок ожидания ответа (в миллисекундах)
 * @return       результат постановки запроса в очередь
 *
 */
bool awh::unit::DNS::resolve(const id_t id, const event::family_t family, string_view domain, const uint32_t alive) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано
		if(!domain.empty()){
			// Признак найденной записи в кэше
			bool cacheHit = false;
			// IP-адрес найденной записи (копия)
			unique_ptr <net::addr_t> cacheAddress = nullptr;
			// Семейство найденной записи
			event::family_t cacheFamily = event::family_t::NONE;
			{
				// Блокируем доступ к глобальному кэшу DNS
				const locker_t <std::shared_mutex> lock(::__awh_dns_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
				// Выполняем поиск доменного имени в кэше
				auto i = ::__awh_cache__.domains.find(string{domain});
				// Если в кэше доменное имя найдено
				if(i != ::__awh_cache__.domains.end()){
					// Получаем текущую метку времени
					const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					/**
					 * Выполняем перебор всех записей доменного имени
					 */
					for(auto j = i->second.begin(); j != i->second.end(); ++j){
						// Пропускаем устаревшие записи в кэше (удаление будет при записи)
						if((j->life > 0) && (j->life <= now))
							// Пропускаем записи доменного имени
							continue;
						/**
						 * Определяем семейство события
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								// Если IP-адрес доменного имени является IPv4
								if((cacheHit = (j->ip->size == 4))){
									// Устанавливаем семейство найденной записи
									cacheFamily = event::family_t::IPV4;
									// Выделяем новый буфер для IP-адреса найденной записи
									cacheAddress = make_unique <net::addr_net_ipv4_t> ();
									// Копируем данные IP-адреса найденной записи в новый буфер
									awh_cast <net::addr_net_ipv4_t *> (cacheAddress.get())->address = awh_cast <net::addr_net_ipv4_t *> (j->ip.get())->address;
									// Выходим из цикла
									break;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								// Если IP-адрес доменного имени является IPv6
								if((cacheHit = (j->ip->size == 16))){
									// Устанавливаем семейство найденной записи
									cacheFamily = event::family_t::IPV6;
									// Выделяем новый буфер для IP-адреса найденной записи
									cacheAddress = make_unique <net::addr_net_ipv6_t> ();
									// Копируем данные IP-адреса найденной записи в новый буфер
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (cacheAddress.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (j->ip.get())->address[0], 16);
									// Выходим из цикла
									break;
								}
							} break;
						}
						// Если запись найдена, завершаем перебор
						if(cacheHit)
							// Выходим из цикла
							break;
					}
				}
			}
			// Выполняем callback только после выхода из блокировки кэша
			if(cacheHit){
				// Выполняем функцию обратного вызова для найденной записи в кэше
				this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, cacheFamily, string{domain}, cacheAddress.get());
				// Возвращаем положительный результат
				return true;
			}
			// Блокируем доступ к состоянию передачи DNS-запросов
			const locker_t <> lock(this->_mtx);
			// Если доменное имя получено
			if(!domain.empty()){
				// Размер полезной нагрузки для запроса к DNS-серверу
				size_t size = 0;
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
						size = ::dns::request(id, record_t::A, domain, this->_log);
					break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
						size = ::dns::request(id, record_t::AAAA, domain, this->_log);
					break;
					// Если семейство события не определено
					default: {
						// Формируем текст сообщения об ошибке DNS-резолвера
						const string error = this->_fmk->format("DNS resolver family is undefined for domain %s", string(domain).c_str());
						// Если функция обратного вызова установлена
						if(this->_callback.is("error")){
							// Идентификатор события клиента DNS-резолвера
							event::id_t eid = 0;
							// Если список DNS-резолверов для семейства IPv4 не пустой
							if(!this->_resolver.idv4.empty())
								// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv4
								eid = this->_resolver.idv4.front();
							// Если список DNS-резолверов для семейства IPv6 не пустой
							else if(!this->_resolver.idv6.empty())
								// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv6
								eid = this->_resolver.idv6.front();
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, error);
						// Если callback ошибки не установлен
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain, alive), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
						// Выполняем функцию обратного вызова для неудачного резолвинга доменного имени
						this->_callback.call <void (const id_t, const record_t, const string &)> ("failure", id, record_t::NONE, string{domain});
						// Возвращаем отрицательный результат
						return false;
					}
				}
				// Если DNS-запрос не сформирован
				if(size == 0)
					// Возвращаем отрицательный результат
					return false;
				/**
				 * Устанавливаем метку начала формирования запроса к DNS-серверу
				 */
				Begin:
				// Если в очереди не осталось свободных резолверов для выполнения запроса
				if(this->_resolver.queue.size() == 0){
					// Если очередь ожидания выполнения запроса переполнена
					if(this->_transfer.packets.size() >= this->_transfer.maxPackets){
						// Формируем текст сообщения об ошибке DNS-резолвера
						const string error = this->_fmk->format("DNS resolver queue is full for domain %s", string(domain).c_str());
						// Если функция обратного вызова установлена
						if(this->_callback.is("error")){
							// Идентификатор события клиента DNS-резолвера
							event::id_t eid = 0;
							// Если список DNS-резолверов для семейства IPv4 не пустой
							if(!this->_resolver.idv4.empty())
								// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv4
								eid = this->_resolver.idv4.front();
							// Если список DNS-резолверов для семейства IPv6 не пустой
							else if(!this->_resolver.idv6.empty())
								// Извлекаем идентификатор события клиента DNS-резолвера для семейства IPv6
								eid = this->_resolver.idv6.front();
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::ACCESS_DENIED, error);
						// Если callback ошибки не установлен
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain, alive), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
						/**
						 * Определяем семейство события
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4):
								// Выполняем функцию обратного вызова для неудачного резолвинга доменного имени
								this->_callback.call <void (const id_t, const record_t, const string &)> ("failure", id, record_t::A, string{domain});
							break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6):
								// Выполняем функцию обратного вызова для неудачного резолвинга доменного имени
								this->_callback.call <void (const id_t, const record_t, const string &)> ("failure", id, record_t::AAAA, string{domain});
							break;
						}
						// Возвращаем отрицательный результат
						return false;
					// Если очередь ещё может вместить в себя новый пакет
					} else {
						// Добавляем новый пакет в контейнер очереди ожидания выполнения запроса к DNS-серверу
						this->_transfer.packets.push(packet_t());
						// Устанавливаем размер полезной нагрузки
						this->_transfer.packets.back().payload.size = size;
						// Выделяем новый буфер для полезной нагрузки
						this->_transfer.packets.back().payload.buffer = make_unique <uint8_t []> (size);
						// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
						::memcpy(this->_transfer.packets.back().payload.buffer.get(), ::dns::buffer, size);
						// Устанавливаем время жизни пакета для отслеживания его выполнения
						this->_transfer.packets.back().alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
						// Выходим из функции, так как пакет успешно добавлен в очередь на отправку
						return true;
					}
				}
				// Добавляем пакет в контейнер активных пакетов
				auto ret = this->_transfer.waiting.emplace(id, packet_t());
				// Если пакет успешно добавлен в контейнер активных пакетов
				if(ret.second){
					// Устанавливаем размер полезной нагрузки
					ret.first->second.payload.size = size;
					// Выделяем новый буфер для полезной нагрузки
					ret.first->second.payload.buffer = make_unique <uint8_t []> (size);
					// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
					::memcpy(ret.first->second.payload.buffer.get(), ::dns::buffer, size);
					// Устанавливаем время жизни пакета для отслеживания его выполнения
					ret.first->second.alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
				// Если пакет не добавлен в контейнер активных пакетов
				} else {
					// Добавляем новый пакет в контейнер очереди ожидания выполнения запроса к DNS-серверу
					this->_transfer.packets.push(packet_t());
					// Устанавливаем размер полезной нагрузки
					this->_transfer.packets.back().payload.size = size;
					// Выделяем новый буфер для полезной нагрузки
					this->_transfer.packets.back().payload.buffer = make_unique <uint8_t []> (size);
					// Копируем данные полезной нагрузки из объекта параметров пакета в новый буфер
					::memcpy(this->_transfer.packets.back().payload.buffer.get(), ::dns::buffer, size);
					// Устанавливаем время жизни пакета для отслеживания его выполнения
					this->_transfer.packets.back().alive = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + (alive > 0 ? alive : 15000));
					// Выходим из функции, так как пакет успешно добавлен в очередь на отправку
					return true;
				}
				// Идентификатор события клиента DNS-резолвера
				event::id_t eid = 0;
				// Получаем идентификатор события клиента DNS-резолвера для отправки запроса к DNS-серверу
				this->_resolver.queue.pop(eid);
				// Если идентификатор события клиента DNS-резолвера не получен
				if(eid == 0)
					// Пытаемся повторить процедуру повторно
					goto Begin;
				// Добавляем идентификатор события клиента DNS-резолвера в контейнер соответствий с DNS-запросами
				else this->_transfer.attached.emplace(eid, id);
				// Отправляем DNS-запрос
				return (this->_io->send(eid, ::dns::buffer, size) > 0);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain, alive), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::DNS::DNS(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _addr(fmk, log), _binbox(fmk, log) {
	// Активируем работу мьютекса блокировки доступа к состоянию передачи DNS-запросов
	this->_mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Устанавливаем режим безопасности работы потоков для функции обратного вызова
	this->_callback.threadSafety(::__awh_thread_safety__ == event::mode_t::ENABLED);
	/**
	 * Выполняем одноразовую инициализацию DNS-серверов для всех экземпляров класса DNS
	 */
	std::call_once(::__awh_dns_init_once__, [this]() noexcept {
		// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем DNS
		::__awh_dns_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Активируем работу мьютекса блокировки потока при работе с глобальным черным списком DNS
		::__awh_dns_blacklist_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Если общие DNS-резолверы ещё не добавлены в глобальный список
		if(::ns::general.empty()){
			{
				// Создаём массив стандартных DNS-серверов IPv4
				array <string_view, 6> resolvers = {AWH_IPV4_NS};
				// Выбираем стандарт рандомайзера
				mt19937 generator(::__awh_randev__());
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(resolvers.begin(), resolvers.end(), generator);
				/**
				 * Выполняем перебор всех DNS-серверов из массива
				 */
				for(const auto & item : resolvers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(item, net_addr_t::type_t::IPV4))
						// Добавляем DNS-сервер в глобальный список для использования при выполнении запросов к DNS-серверам
						::ns::general.push_back(::move(this->_addr.source(net_addr_t::endian_t::LITTLE)));
				}
			}{
				// Создаём массив стандартных DNS-серверов IPv6
				array <string_view, 6> resolvers = {AWH_IPV6_NS};
				// Выбираем стандарт рандомайзера
				mt19937 generator(::__awh_randev__());
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(resolvers.begin(), resolvers.end(), generator);
				/**
				 * Выполняем перебор всех DNS-серверов из массива
				 */
				for(const auto & item : resolvers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(item, net_addr_t::type_t::IPV6))
						// Добавляем DNS-сервер в глобальный список для использования при выполнении запросов к DNS-серверам
						::ns::general.push_back(::move(this->_addr.source(net_addr_t::endian_t::LITTLE)));
				}
			}
		}
	});
	/**
	 * Инициализация DNS-сервера
	 */
	this->_resolver.nameServers.init();
	// Выполняем инициализацию 5-и DNS-резолверов IPv4 по умолчанию
	this->init(event::family_t::IPV4, 5);
	// Выполняем инициализацию 5-и DNS-резолверов IPv6 по умолчанию
	this->init(event::family_t::IPV6, 5);
}
/**
 * @brief Конструктор
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::unit::DNS::DNS(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _addr(fmk, log), _binbox(fmk, log) {
	// Активируем работу мьютекса блокировки доступа к состоянию передачи DNS-запросов
	this->_mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Устанавливаем режим безопасности работы потоков для функции обратного вызова
	this->_callback.threadSafety(::__awh_thread_safety__ == event::mode_t::ENABLED);
	/**
	 * Выполняем одноразовую инициализацию DNS-серверов для всех экземпляров класса DNS
	 */
	std::call_once(::__awh_dns_init_once__, [this]() noexcept {
		// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем DNS
		::__awh_dns_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Активируем работу мьютекса блокировки потока при работе с глобальным черным списком DNS
		::__awh_dns_blacklist_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Если общие DNS-резолверы ещё не добавлены в глобальный список
		if(::ns::general.empty()){
			{
				// Создаём массив стандартных DNS-серверов IPv4
				array <string_view, 6> resolvers = {AWH_IPV4_NS};
				// Выбираем стандарт рандомайзера
				mt19937 generator(::__awh_randev__());
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(resolvers.begin(), resolvers.end(), generator);
				/**
				 * Выполняем перебор всех DNS-серверов из массива
				 */
				for(const auto & item : resolvers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(item, net_addr_t::type_t::IPV4))
						// Добавляем DNS-сервер в глобальный список для использования при выполнении запросов к DNS-серверам
						::ns::general.push_back(::move(this->_addr.source(net_addr_t::endian_t::LITTLE)));
				}
			}{
				// Создаём массив стандартных DNS-серверов IPv6
				array <string_view, 6> resolvers = {AWH_IPV6_NS};
				// Выбираем стандарт рандомайзера
				mt19937 generator(::__awh_randev__());
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(resolvers.begin(), resolvers.end(), generator);
				/**
				 * Выполняем перебор всех DNS-серверов из массива
				 */
				for(const auto & item : resolvers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(item, net_addr_t::type_t::IPV6))
						// Добавляем DNS-сервер в глобальный список для использования при выполнении запросов к DNS-серверам
						::ns::general.push_back(::move(this->_addr.source(net_addr_t::endian_t::LITTLE)));
				}
			}
		}
	});
	/**
	 * Инициализация DNS-сервера
	 */
	this->_resolver.nameServers.init();
	// Выполняем инициализацию 5-и DNS-резолверов
	this->init(family, 5);
}
/**
 * @brief Деструктор
 *
 */
awh::unit::DNS::~DNS() noexcept {
	// Если событие таймера для периодической очистки кэша активно
	if(::__awh_cache__.tid > 0)
		// Удаляем событие таймера для периодической очистки кэша
		this->_io->destroy(::__awh_cache__.tid);
	// Если событие таймера для периодической очистки кэша активно
	if(::__awh_cache__.fid > 0)
		// Удаляем событие таймера для периодической очистки кэша
		this->_io->destroy(::__awh_cache__.fid);
	// Если список DNS-резолверов IPv4 не пустой
	if(!this->_resolver.idv4.empty()){
		/**
		 * Выполняем перебор всех DNS-резолверов IPv4 из списка для их удаления
		 */
		for(auto i = this->_resolver.idv4.begin(); i != this->_resolver.idv4.end(); ++i){
			// Снимаем функцию обратного вызова на событие чтения данных
			this->_io->on(* i, static_cast <engine::callback::read_t> (nullptr));
			// Снимаем функцию обратного вызова на событие получения ошибок
			this->_io->on(* i, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие таймаута
			this->_io->on(* i, static_cast <engine::callback::timeout_t> (nullptr));
			// Удаляем событие DNS-резолвера
			this->_io->destroy(* i);
		}
	}
	// Если список DNS-резолверов IPv6 не пустой
	if(!this->_resolver.idv6.empty()){
		/**
		 * Выполняем перебор всех DNS-резолверов IPv6 из списка для их удаления
		 */
		for(auto i = this->_resolver.idv6.begin(); i != this->_resolver.idv6.end(); ++i){
			// Снимаем функцию обратного вызова на событие чтения данных
			this->_io->on(* i, static_cast <engine::callback::read_t> (nullptr));
			// Снимаем функцию обратного вызова на событие получения ошибок
			this->_io->on(* i, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие таймаута
			this->_io->on(* i, static_cast <engine::callback::timeout_t> (nullptr));
			// Удаляем событие DNS-резолвера
			this->_io->destroy(* i);
		}
	}
}
