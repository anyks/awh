/**
 * @file: dns.cpp
 * @date: 2026-02-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <array>
#include <vector>
#include <random>
#include <cerrno>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Системные модули
 */
#include <arpa/inet.h>
#include <sys/types.h>

/**
 * Подключаем заголовочный файл модуля
 */
#include <units/dns.hpp>

/**
 * Если используется модуль IDN и операционная система не MS Windows
 */
#if AWH_IDN && !_WIN32 && !_WIN64
	#include <idn2.h>
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
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
 * Инкапсулируем параметры DNS-серверов в пространство имён
 */
namespace ns {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Общий список DNS-серверов
	 *
	 */
	vector <unique_ptr <net::addr_t>> general;
};

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
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
		// Время завершения жизни
		uint64_t life;
		// IP-адрес доменного имени
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
	 * @brief Структура записи IP-адреса принадлежащего домену
	 *
	 */
	struct EntryIP {
		// Флаг локального IP-адреса
		bool local;
		// Время завершения жизни
		uint64_t life;
		// IP-адрес доменного имени
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
		// Время завершения жизни
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
	 * @brief Структура хэш-функции для IPv6 ключа
	 *
	 * Использует FNV-1a алгоритм — быстрый и с хорошим распределением
	 */
	struct IpV6Hash {
		/**
		 * @brief Оператор генерации числового хэша ключа
		 *
		 * @param key ключ для которого необходима генерация
		 * @return    сгенерированный хэш ключа
		 */
		uint64_t operator()(const array <uint8_t, 16> & key) const noexcept {
			// FNV-1a 64-bit constants
			uint64_t result = 14695981039346656037ULL; // FNV offset basis
			// Выполняем перебор всех байт ключа
			for(uint8_t byte : key){
				// Выполняем инвертирование байт ключа
				result ^= static_cast <uint64_t> (byte);
				// Выполняем компенсацию
				result *= 1099511628211ULL; // FNV prime
			}
			// Выводим результат
			return result;
		}
	};

	/**
	 * @brief Структура хэш-функции для ключа доменного имени
	 *
	 * Использует FNV-1a алгоритм — быстрый и с хорошим распределением
	 */
	struct DomainHash {
		/**
		 * @brief Оператор генерации числового хэша доменного имени
		 *
		 * @param domain доменное имя для которого необходима генерация
		 * @return       сгенерированный хэш доменного имени
		 */
		uint64_t operator()(string_view domain) const noexcept {
			// FNV-1a 64-bit constants
			uint64_t result = 14695981039346656037ULL; // FNV offset basis
			// Выполняем перебор всех байт доменного имени
			for(char c : domain){
				// Приводим к lowercase на лету (если ещё не нормализовали)
				char lower = static_cast <char> (::tolower(static_cast<uint8_t>(c)));
				// Выполняем инвертирование байт доменного имени
				result ^= static_cast <uint64_t> (lower);
				// Выполняем компенсацию
				result *= 1099511628211ULL; // FNV prime
			}
			// Выводим результат
			return result;
		}
	};

	/**
	 * @brief Объект работы с чёрным списком
	 *
	 */
	struct Blacklist {
		// Мютекс для блокировки потока
		lock_state_t <std::shared_mutex> mtx;
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
		// Мютекс для блокировки потока
		lock_state_t <std::shared_mutex> mtx;
		// Список IPv4-адресов с доменными именами
		unordered_map <uint32_t, vector <EntryDomain>> ipv4;
		// Список IPv6-адресов с доменными именами
		unordered_map <array <uint8_t, 16>, vector <EntryDomain>, IpV6Hash> ipv6;
		// Список доменных имён с IP-адресами
		unordered_map <string, vector <EntryIP>, DomainHash> domains;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Cache() noexcept : filename{""}, tid(0), fid(0), interval(0) {}
	} __awh_cache__;
};

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для рандомизации DNS-серверов
	 *
	 */
	random_device __awh_randev__;
	/**
	 * @brief Мютекс для блокировки потока
	 *
	 */
	lock_state_t <std::mutex> __awh_mtx__;
	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;
};

/**
 * Инкапсулируем структуры протокола DNS в собственное пространство имён
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
		// Доменное имя, связанное с этим A-записью
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
		// Доменное имя, связанное с этой NS-записью
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
		// Доменное имя, связанное с этой CNAME-записью
		string name;
		// Каноническое имя, связанное с этой CNAME-записью
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
		// Доменное имя, связанное с этой MX-записью
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
		// Доменное имя, связанное с этой TXT-записью
		string name;
		// Список текстовых строк, связанных с этой TXT-записью (может содержать несколько строк)
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
		// Доменное имя, связанное с этой SOA-записью
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
		// Доменное имя, связанное с этой PTR-записью
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
		uint8_t rd : 1;     // Флаг выполнения желаемой рекурсии
		uint8_t tc : 1;     // Флаг усечения сообщения если оно слишком большое
		uint8_t aa : 1;     // Флаг авторитетного ответа сервера
		uint8_t opcode : 4; // Опкод операции
		uint8_t qr : 1;     // Тип запроса или ответа
		uint8_t rcode : 4;  // Код выполнения операции
		uint8_t z : 3;      // Зарезервировано для использования в будущем
		uint8_t ra : 1;     // Флаг активации рекурсивных запросов на сервере
		uint16_t qdcount;   // Количество записей в разделе запроса
		uint16_t ancount;   // Количество записей в разделе ответа
		uint16_t nscount;   // Количество записей ресурсов в разделе Authority (серверы имён)
		uint16_t arcount;   // Количество записей ресурсов в разделе дополнительных записей
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Header() noexcept :
		 qdcount(0), ancount(0),
		 nscount(0), arcount(0) {}
	} head_t;

	/**
	 * @brief Структура флагов DNS запросов
	 *
	 */
	typedef struct Q_Flags {
		uint16_t type; // Тип записи
		uint16_t cls;  // Класс записи
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Q_Flags() noexcept : type(0), cls(0) {}
	} q_flags_t;
};

/**
 * Инкапсулируем вспомогательные функции протокола DNS в собственное пространство имён
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
	 */
	static unit::dns_t::id_t identifier() noexcept {
		// Результат работы функции
		unit::dns_t::id_t result = 0;
		// Начинаем с 1 (0 можно оставить как "invalid")
		static atomic_uint16_t id{1};
		// Выводим новое значение идентификатора
		result = id.fetch_add(1, memory_order_relaxed);
		// Если результат не получен
		if(result == 0)
			// Генерируем результат заново
			return identifier();
		// Выводим полученный результат
		return result;
	}

	/**
	 * @brief Функция чтения 16-битного целого числа из буфера данных в сетевом порядке (big-endian)
	 *
	 * @param p буфер данных, из которого необходимо прочитать 16-битное число
	 * @return  16-битное число, прочитанное из буфера данных
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
	 */
	static vector <uint8_t> encodeDomainName(string_view domain) noexcept {
		// Результат работы функции
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
		// Выводим результат
		return result;
	}

	/**
	 * @brief Функция декодирования DNS-формата в строку
	 *
	 * @param buffer бинарный буфер данных в формате DNS
	 * @param size   размер буфера данных
	 * @param offset количество прочитанных байт (output)
	 * @return       доменное имя или пустая строка при ошибке
	 */
	static string decodeDomainName(const uint8_t * buffer, const size_t size, size_t & offset) noexcept {
		// Результат работы функции
		string result = "";
		// Текущая позиция в буфере данных
		size_t pos = 0;
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
			if((length & 0xC0) == 0xC0)
				// Это pointer (сжатие), для простоты не обрабатываем
				// В полном парсере нужно следовать по указателю
				break;
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
		// Выводим результат
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
	 */
	static bool decodeDomainName(const uint8_t * packet, const size_t length, size_t & offset, char * buffer, const size_t size) noexcept {
		// Текущая позиция в буфере данных
		size_t pos = 0;
		// Смещение для обработки сжатия (pointer)
		uint16_t ptr = 0;
		// Длина текущего лейбла
		uint8_t labelSize = 0;
		// Максимальное количество прыжков по указателям, чтобы избежать бесконечных циклов при повреждённых данных
		size_t maxJumps = 10;
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
				// Если смещение выходит за пределы буфера данных, то это ошибка
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
		// Выводим результат
		return true;
	}
	/**
	 * @brief Функция парсинга DNS-ответа из бинарного буфера данных
	 *
	 * @param buffer бинарный буфер данных, содержащий DNS-ответ
	 * @param size   размер буфера данных
	 * @param result структура для хранения результатов парсинга DNS-ответа
	 * @return       true при успешном парсинге, false при ошибке
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
		const uint16_t total = (ancount + nscount + arcount);
		/**
		 * Перебираем все записи в секциях Answer, Authority и Additional
		 * И распределяем их по типам (A, AAAA, NS, CNAME, MX, TXT, SOA, PTR)
		 * Сохраняем результаты в результирующем объекте
		 */
		for(uint16_t i = 0; i < total; ++i){
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
		// Выводим результат
		return true;
	}

	/**
	 * @brief Функция выполнения резолвинга доменного имени
	 *
	 * @param id     идентификатор DNS-резолвера
	 * @param record тип DNS-записи для запроса (A, AAAA, NS, CNAME, MX, TXT, SOA, PTR)
	 * @param domain доменное имя сервера
	 * @param log    объект для работы с логами
	 * @return       размер буфера данных, отправленного на резолвинг доменного имени, или 0 при ошибке
	 */
	static size_t request(const unit::dns_t::id_t id, const unit::dns_t::record_t record, string_view domain, const log_t * log) noexcept {
		// Результат работы функции
		size_t result = 0;
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если доменное имя передано
			if((id > 0) && !domain.empty()){
				// Зануляем буфер полезной нагрузки
				::memset(::dns::buffer, 0, sizeof(::dns::buffer));
				// Получаем объект заголовка запроса
				::dns::head_t * header = reinterpret_cast <::dns::head_t *> (::dns::buffer);
				// Устанавливаем идентификатор заголовка
				header->id = htons(id);
				/**
				 * Заполняем оставшуюся структуру пакетов
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
				// Выводим сообщение об ошибке
				log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (record), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Выводим результат
		return result;
	}
};

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
 * @brief Метод создания события DNS-резолвера
 *
 * @param family семейство протоколов (например: IPv4 или IPv6)
 */
void awh::unit::DNS::create(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Результат работы функции
		bool result = false;
		{
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Добавляем новое событие клиента UDP
			this->_resolver.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(this->_resolver.eid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие чтения данных
			this->_io->on(this->_resolver.eid, static_cast <engine::callback::read_t> (std::bind(&dns_t::response, this, _1, _2, _3)));
			// Если опции события не установлены
			if(!(result = this->_io->setOptions(this->_resolver.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)))
				// Удаляем событие DNS-резолвера
				this->_io->destroy(this->_resolver.eid);
		}
		// Если событие DNS-резолвера успешно создано и опции установлены
		if(!result){
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to set options for DNS resolver event", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to set options for DNS resolver event", log_t::flag_t::CRITICAL);
				#endif
			}
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки событий дампинга DNS-кэша
 *
 * @param        идентификатор таймера DNS-резолвера
 * @param status статус события таймера DNS-резолвера
 */
void awh::unit::DNS::dumping([[maybe_unused]] const event::id_t, const event::status_t status) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если кэш DNS-резолвера не пустой
			if(!::__awh_cache__.domains.empty()){
				// Выполняем блокировку потока для работы с бинарным контейнером
				const locker_t <> lock(::__awh_mtx__);
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				// Очищаем бинарный контейнер для хранения кэша доменных имён
				this->_binbox.clear();
				// Добавляем в контейнер метку времени сохранения кэша
				this->_binbox.add("TIMESTAMP", now);
				// Создаём объект записи для добавления в контейнер
				Entry record{};
				// Количество добавленных записей для статистики
				uint32_t count = 0;
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
							this->_binbox.add(this->_fmk->format("RECORD_%u", count++), &record, sizeof(record));
							// Продолжаем перебор кэша
							++i;
						// Продолжаем перебор кэша
						} else ++i;
					}
				}
				// Если есть записи для сохранения в кэше
				if(count > 0){
					// Добавляем в контейнер количество доменных имён с IP-адресами
					this->_binbox.add("COUNT", count);
					// Сохраняем кэш доменных имён в файл
					this->_binbox.save(::__awh_cache__.filename);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки событий коллектора DNS-кэша
 *
 * @param        идентификатор таймера DNS-резолвера
 * @param status статус события таймера DNS-резолвера
 */
void awh::unit::DNS::collector([[maybe_unused]] const event::id_t, const event::status_t status) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем текущую метку времени
			const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			// Если в кэше есть IPv4-адреса
			if(!::__awh_cache__.ipv4.empty()){
				// Выполняем блокировку потока для работы с кэшем
				const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				/**
				 * Выполняем перебор всех IP-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
					// Выполняем перебор всех записей IP-адреса
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
				// Выполняем блокировку потока для работы с кэшем
				const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				/**
				 * Выполняем перебор всех IP-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
					// Выполняем перебор всех записей IP-адреса
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
				// Выполняем блокировку потока для работы с кэшем
				const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				/**
				 * Выполняем перебор всех доменных имён в кэше
				 */
				for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
					// Выполняем перебор всех записей доменного имени
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
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
 */
void awh::unit::DNS::hosts(const event::id_t, const uint8_t * data, const size_t size) noexcept {
	// Если данные события загрузки локальных хостов не пустые
	if((data != nullptr) && (size > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Сначала удаляем все локальные записи из кэша, чтобы не было конфликтов с новыми данными
			 */
			{
				// Выполняем блокировку потока для работы с кэшем
				const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Если в кэше есть IPv4-адреса
				if(!::__awh_cache__.ipv4.empty()){
					/**
					 * Выполняем перебор всех IP-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
						// Выполняем перебор всех записей доменного имени
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
						// Выполняем перебор всех записей доменного имени
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
						// Выполняем перебор всех записей доменного имени
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
			}
			/**
			 * @brief Функция парсинга строки из файла хостов
			 *
			 * @param str строка из файла хостов для парсинга
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
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						/**
						 * Выполняем перебор всех доменных имён, связанных с IP-адресом
						 */
						for(auto & domain : entry.domains){
							// Выполняем парсинг IP-адреса
							if(this->_addr.parse(entry.ip)){
								// Выполняем блокировку потока для работы с кэшем
								const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
								// Выполняем поиск доменного имени в кэше
								auto i = ::__awh_cache__.domains.find(string{domain});
								// Если в кэше доменное имя найдено
								if(i != ::__awh_cache__.domains.end()){
									// Создаём объект записи
									EntryIP record;
									// Помечаем IP-адрес как локальный
									record.local = true;
									/**
									 * Определяем тип адреса
									 */
									switch(static_cast <uint8_t> (this->_addr.type())){
										// Если адрес является IPv4
										case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
											// Выполняем инициализацию объекта IP-адреса
											record.ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
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
											// Выполняем инициализацию объекта IP-адреса
											record.ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
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
									switch(static_cast <uint8_t> (this->_addr.type())){
										// Если адрес является IPv4
										case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
											// Выполняем инициализацию объекта IP-адреса
											entry.back().ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
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
											// Выполняем инициализацию объекта IP-адреса
											entry.back().ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
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
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(str), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(data, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки ошибок событий DNS-резолвера
 *
 * @param eid         идентификатор события DNS-резолвера
 * @param error       код ошибки события DNS-резолвера
 * @param description описание ошибки события DNS-резолвера
 */
void awh::unit::DNS::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если событие относится к DNS-резолверу
	if(eid == this->_resolver.eid)
		// Выполняем сброс DNS-резолвера
		this->reset();
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки ответов от DNS-сервера на запросы резолвинга доменных имён
 *
 * @param eid  идентификатор события чтения из DNS-резолвера
 * @param data данные события чтения из DNS-резолвера
 * @param size размер данных события чтения из DNS-резолвера
 */
void awh::unit::DNS::response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если данные события чтения из DNS-резолвера не пустые
		if((data != nullptr) && (size > 0)){
			// Получаем объект заголовка запроса
			const ::dns::head_t * header = reinterpret_cast <const ::dns::head_t *> (data);
			// Получаем идентификатор запроса из заголовка запроса
			const uint16_t id = ntohs(header->id);
			// Доменное имя для логирования
			string domain = "unknown";
			{
				// Выполняем блокировку потока для работы с контейнером активных пакетов
				const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Выполняем поиск пакета в контейнере активных пакетов
				auto i = this->_transfer.waiting.find(id);
				// Если пакет найден в контейнере активных пакетов
				if(i != this->_transfer.waiting.end()){
					// Получаем доменное имя из сохраненного запроса текущего DNS-резолвера для логирования
					domain = ::move(i->second.domain);
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Удаляем событие таймера для ожидания ответа от DNS-сервера
					this->_io->destroy(i->second.tid);
					// Удаляем пакет из контейнера активных пакетов
					this->_transfer.waiting.erase(i);
				}
			}
			/**
			 * Определяем код выполнения операции
			 */
			switch(header->rcode){
				// Если операция выполнена удачно
				case 0: {
					// Создаём объект для хранения результата парсинга ответа от DNS-сервера
					::dns::dns_result_t result;
					// Выполняем парсинг ответа от DNS-сервера
					if(::dns::parse(data, size, result)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим начальный разделитель
							cout << "------------------------------------------------------------" << endl << endl << flush;
							// Выводим заголовок
							cout << "DNS RESPONSE:" << endl << endl << flush;
							// Если мы получили A-записи в ответе
							if(!result.a.empty()){
								/**
								 * Перебираем все A-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.a){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выполняем блокировку потока для парсинга IP-адреса
									const locker_t <> lock(::__awh_mtx__);
									// Устанавливаем IPv4-адрес в объекте адреса
									this->_addr.v4(answer.ip);
									// Выводим IPv4-адрес
									::printf("IPv4: %s\n", static_cast <string> (this->_addr).c_str());
								}
							}
							// Если мы получили AAAA-записи в ответе
							if(!result.aaaa.empty()){
								/**
								 * Перебираем все AAAA-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.aaaa){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выполняем блокировку потока для парсинга IP-адреса
									const locker_t <> lock(::__awh_mtx__);
									// Устанавливаем IPv6-адрес в объекте адреса
									this->_addr.v6(answer.ip);
									// Выводим IPv6-адрес
									::printf("IPv6: %s\n", static_cast <string> (this->_addr).c_str());
								}
							}
							// Если мы получили NS-записи в ответе
							if(!result.ns.empty()){
								/**
								 * Перебираем все NS-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.ns){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выводим сервер имён
									::printf("NS: %s\n", answer.server.c_str());
								}
							}
							// Если мы получили CNAME-записи в ответе
							if(!result.cname.empty()){
								/**
								 * Перебираем все CNAME-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.cname){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выводим каноническое имя
									::printf("CNAME: %s\n", answer.canonical.c_str());
								}
							}
							// Если мы получили MX-записи в ответе
							if(!result.mx.empty()){
								/**
								 * Перебираем все MX-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.mx){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выводим сервер почты
									::printf("MX: %s\n", answer.server.c_str());
									// Выводим приоритет MX-записи
									::printf("PREFERENCE: %u\n", answer.preference);
								}
							}
							// Если мы получили TEXT-записи в ответе
							if(!result.txt.empty()){
								/**
								 * Перебираем все TEXT-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.txt){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									/**
									 * Вывод текстовых данных из TXT-записи
									 */
									for(auto & text : answer.texts)
										// Выводим текст записи
										::printf("TXT: %s\n", text.c_str());
								}
							}
							// Если мы получили PTR-записи в ответе
							if(!result.ptr.empty()){
								/**
								 * Перебираем все PTR-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.ptr){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выводим PTR-запись
									::printf("PTR: %s\n", answer.domain.c_str());
								}
							}
							// Если мы получили SOA-записи в ответе
							if(!result.soa.empty()){
								/**
								 * Перебираем все SOA-записи в ответе от DNS-сервера
								 */
								for(auto & answer : result.soa){
									// Выводим название записи
									::printf("\nNAME: %s\n", answer.name.c_str());
									// Выводим TTL записи
									::printf("TTL: %u\n", answer.ttl);
									// Выводим SOA-запись
									::printf("SOA: %s\n", answer.mname.c_str());
									// Выводим административный контакт
									::printf("RNAME: %s\n", answer.rname.c_str());
									// Выводим серийный номер зоны
									::printf("SERIAL: %u\n", answer.serial);
									// Выводим время обновления зоны
									::printf("REFRESH: %u\n", answer.refresh);
									// Выводим время повторной попытки обновления зоны
									::printf("RETRY: %u\n", answer.retry);
									// Выводим время истечения срока действия зоны
									::printf("EXPIRE: %u\n", answer.expire);
								}
							}
							// Выводим начальный разделитель
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
							// Если функция обратного вызова установлена для получения IP-адресов
							if(this->_callback.is("address")){
								// Выбираем стандарт рандомайзера
								mt19937 generator(::__awh_randev__());
								// Выполняем рандомную сортировку списка DNS-серверов
								::shuffle(result.a.begin(), result.a.end(), generator);
								// IP-адрес для вывода результата
								unique_ptr <net::addr_t> address = nullptr;
								{
									// Выполняем блокировку потока для парсинга IP-адреса
									const locker_t <> lock(::__awh_mtx__);
									// Устанавливаем IPv4-адрес в объекте адреса
									this->_addr.v4(result.a.front().ip);
									// Устанавливаем строковое представление IP-адреса для вывода результата
									address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
								}
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV4, result.a.front().name, address.get());
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
							// Если функция обратного вызова установлена для получения IP-адресов
							if(this->_callback.is("address")){
								// Выбираем стандарт рандомайзера
								mt19937 generator(::__awh_randev__());
								// Выполняем рандомную сортировку списка DNS-серверов
								::shuffle(result.aaaa.begin(), result.aaaa.end(), generator);
								// IP-адрес для вывода результата
								unique_ptr <net::addr_t> address = nullptr;
								{
									// Выполняем блокировку потока для парсинга IP-адреса
									const locker_t <> lock(::__awh_mtx__);
									// Устанавливаем IPv6-адрес в объекте адреса
									this->_addr.v6(result.aaaa.front().ip);
									// Устанавливаем строковое представление IP-адреса для вывода результата
									address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
								}
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV6, result.aaaa.front().name, address.get());
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
							// Если функция обратного вызова установлена для получения сервера имён
							if(!ns.empty() && this->_callback.is("ns"))
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
							// Если функция обратного вызова установлена для получения канонического имени
							if(!cname.empty() && this->_callback.is("cname"))
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
							// Если функция обратного вызова установлена для получения MX-записей
							if(!mx.empty() && this->_callback.is("mx"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const unordered_multimap <string, std::pair <string, uint16_t>> &)> ("mx", id, mx);
						}
						// Если мы получили TEXT-записи в ответе
						if(!result.txt.empty()){
							/**
							 * Создаём контейнер для хранения текстовых записей
							 */
							unordered_multimap <string, string> texts;
							/**
							 * Перебираем все TEXT-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.txt){
								/**
								 * Вывод текстовых данных из TXT-записи
								 */
								for(auto & text : answer.texts)
									// Добавляем запись в контейнер текстовых записей
									texts.emplace(answer.name, text);
							}
							// Если функция обратного вызова установлена для получения текстовых записей
							if(!texts.empty() && this->_callback.is("txt"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const unordered_multimap <string, string> &)> ("txt", id, texts);
						}
						// Если мы получили PTR-записи в ответе
						if(!result.ptr.empty()){
							/**
							 * Перебираем все PTR-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.ptr){
								// IP-адрес для кэширования
								unique_ptr <net::addr_t> address = nullptr;
								{
									// Выполняем блокировку потока для парсинга IP-адреса
									const locker_t <> lock(::__awh_mtx__);
									// Устанавливаем ARPA-адрес в объекте адреса
									this->_addr.arpa(answer.name);
									// Получаем IP-адрес
									address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
								}
								// Если IP-адрес получен
								if(address != nullptr)
									// Добавляем запись в кэш DNS-резолвера
									this->pushAddressToCache(answer.domain, address.get(), answer.ttl);
							}
							// Если функция обратного вызова установлена для получения IP-адресов
							if(this->_callback.is("address")){
								// Выбираем стандарт рандомайзера
								mt19937 generator(::__awh_randev__());
								// Выполняем рандомную сортировку списка DNS-серверов
								::shuffle(result.ptr.begin(), result.ptr.end(), generator);
								// IP-адрес для вывода результата
								unique_ptr <net::addr_t> address = nullptr;
								// Семейство адресов для вывода результата
								event::family_t family = event::family_t::NONE;
								{
									// Выполняем блокировку потока для парсинга IP-адреса
									const locker_t <> lock(::__awh_mtx__);
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
									// Устанавливаем строковое представление IP-адреса для вывода результата
									address = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
								}
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, family, result.ptr.front().domain, address.get());
							}
						}
						// Если мы получили SOA-записи в ответе
						if(!result.soa.empty()){
							/**
							 * Перебираем все SOA-записи в ответе от DNS-сервера
							 */
							for(auto & answer : result.soa){
								// Если функция обратного вызова установлена для получения SOA-записей
								if(this->_callback.is("soa"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const id_t, string_view, string_view)> ("soa", id, answer.name, answer.mname);
								// Если функция обратного вызова установлена для получения RNAME-записей
								if(this->_callback.is("rname"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const id_t, string_view, string_view)> ("rname", id, answer.name, answer.rname);
							}
						}
					}
				} break;
				// Если сервер DNS не смог интерпретировать запрос
				case 1: {
					// Формируем текст выводимой ошибки DNS-резолвера
					const string error = this->_fmk->format("DNS query format error to nameserver %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::UNKNOWN, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
				} break;
				// Если проблемы возникли на DNS-сервере
				case 2: {
					// Формируем текст выводимой ошибки DNS-резолвера
					const string error = this->_fmk->format("DNS server failure %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
				} break;
				// Если доменное имя, указанное в запросе, не существует
				case 3: {
					// Формируем текст выводимой ошибки DNS-резолвера
					const string error = this->_fmk->format("Domain name %s referenced in the query for nameserver %s does not exist", domain.c_str(), this->_io->getTarget(eid).c_str());
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::NOT_FOUND, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
				} break;
				// Если DNS-сервер не поддерживает подобный тип запросов
				case 4: {
					// Формируем текст выводимой ошибки DNS-резолвера
					const string error = this->_fmk->format("DNS server is not implemented at %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::INVALID_ADDRESS, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
				} break;
				// Если DNS-сервер отказался выполнять наш запрос (например, по политическим причинам)
				case 5: {
					// Формируем текст выводимой ошибки DNS-резолвера
					const string error = this->_fmk->format("DNS request is refused to nameserver %s for domain %s", this->_io->getTarget(eid).c_str(), domain.c_str());
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::ACCESS_DENIED, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки событий таймаута при ожидании ответа от DNS-сервера
 *
 * @param id     идентификатор DNS-резолвера
 * @param eid    идентификатор таймера DNS-резолвера
 * @param status статус события таймера DNS-резолвера
 * @param packet объект активного пакета DNS-запроса
 */
void awh::unit::DNS::timeout(const id_t id, const event::id_t eid, const event::status_t status, packet_t * packet) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		// Если попытки резолвинга не превышают максимально допустимое количество
		if(packet->attempt < this->_transfer.attempts){
			// Результат установки таймера для ожидания ответа от DNS-сервера
			bool result = false;
			{
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Увеличиваем количество попыток резолвинга
				packet->attempt++;
				// Добавляем новое событие таймаута для ожидания ответа от DNS-сервера
				packet->tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
				// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от DNS-сервера
				this->_io->setTimeout(packet->tid, event::action_t::NONE, (packet->delay > 0 ? packet->delay : 5000));
				// Устанавливаем функцию обратного вызова на событие получения ошибок
				this->_io->on(packet->tid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
				// Если не удалось установить таймер для ожидания ответа от DNS-сервера
				if(!(result = this->_io->commit(packet->tid)))
					// Удаляем событие таймера
					this->_io->destroy(packet->tid);
				// Если таймер для ожидания ответа от DNS-сервера успешно установлен
				else {
					// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от DNS-сервера
					this->_io->on(packet->tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::timeout, this, id, _1, _2, packet)));
					// Запускаем таймер для ожидания ответа от DNS-сервера
					this->_io->launch(packet->tid);
				}
			}
			// Если не удалось установить таймер для ожидания ответа от DNS-сервера
			if(!result){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Failed to commit DNS resolver timeout", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Failed to commit DNS resolver timeout", log_t::flag_t::CRITICAL);
					#endif
				}
				// Выходим из функции
				return;
			}
			// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
			const size_t size = ::dns::request(id, packet->record, packet->domain, this->_log);
			// Отправляем запрос на резолвинг доменного имени
			this->_io->send(eid, ::dns::buffer, size);
		// Если попытки резолвинга превышают максимально допустимое количество
		} else {
			// Если функция обратного вызова установлена
			if(this->_callback.is("attempts"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const id_t, const string &, const uint8_t)> ("attempts", id, packet->domain, packet->attempt);
			// Если функция обратного вызова не установлена
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(
						"DNS resolver timeout for domain '%s' (attempts: %u)",
						__PRETTY_FUNCTION__,
						std::make_tuple(static_cast <uint16_t> (status)),
						log_t::flag_t::WARNING,
						packet->domain, packet->attempt
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("DNS resolver timeout for domain '%s' (attempts: %u)", log_t::flag_t::WARNING, packet->domain, packet->attempt);
				#endif
			}
			// Выполняем блокировку потока для работы с контейнером активных пакетов
			const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск пакета в контейнере активных пакетов
			auto i = this->_transfer.waiting.find(id);
			// Если пакет найден в контейнере активных пакетов
			if(i != this->_transfer.waiting.end())
				// Удаляем пакет из контейнера активных пакетов
				this->_transfer.waiting.erase(i);
		}
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::unit::DNS::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для родительского юнита
	unit_t::threadSafety(mode);
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с IP-адресами
	::__awh_mtx__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с кэшем
	::__awh_cache__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с чёрным списком
	::__awh_blacklist__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с контейнером пакетов
	this->_transfer.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
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
	// Выполняем установку функции обратного вызова при получении IP-адресов
	this->_callback.set("address", callback);
	// Выполняем установку функции обратного вызова при получении события истечения количества попыток резолвинга доменного имени
	this->_callback.set("attempts", callback);
}
/**
 * @brief Метод установки количества попыток резолвинга доменного имени
 *
 * @param attempts количество попыток резолвинга доменного имени
 */
void awh::unit::DNS::setAttempts(const uint8_t attempts) noexcept {
	// Выполняем блокировку потока для работы с контейнером пакетов
	const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Устанавливаем количество попыток резолвинга доменного имени
	this->_transfer.attempts = attempts;
}
/**
 * @brief Метод кодирования интернационального доменного имени
 *
 * @param domain доменное имя для кодирования
 * @return       результат работы кодирования
 */
string awh::unit::DNS::encode(string_view domain) const noexcept {
	// Результат работы функции
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
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод декодирования интернационального доменного имени
 *
 * @param domain доменное имя для декодирования
 * @return       результат работы декодирования
 */
string awh::unit::DNS::decode(string_view domain) const noexcept {
	// Результат работы функции
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
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод пересортировки адресов в кэше для доменного имени
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 */
void awh::unit::DNS::shuffle(const event::family_t family, string_view domain) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с кэшем
		const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
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
		// Если чёрный список IPv4 адресов не пустой
		if(!::__awh_blacklist__.ipv4.empty()){
			// Выполняем блокировку потока для работы с чёрным списком
			const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Очищаем чёрный список
			::__awh_blacklist__.ipv4.clear();
		}
		// Если чёрный список IPv6 адресов не пустой
		if(!::__awh_blacklist__.ipv6.empty()){
			// Выполняем блокировку потока для работы с чёрным списком
			const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Очищаем чёрный список
			::__awh_blacklist__.ipv6.clear();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки чёрного списка
 *
 * @param family семейство IP-адресов IPv4/IPv6
 */
void awh::unit::DNS::clearBlacklist(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если чёрный список IPv4 адресов не пустой
				if(!::__awh_blacklist__.ipv4.empty()){
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем чёрный список
					::__awh_blacklist__.ipv4.clear();
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если чёрный список IPv6 адресов не пустой
				if(!::__awh_blacklist__.ipv6.empty()){
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем чёрный список
					::__awh_blacklist__.ipv6.clear();
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод удаления IP-адреса из чёрного списка
 *
 * @param ip адрес для удаления из чёрного списка
 */
void awh::unit::DNS::removeAddressInBlacklist(string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
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
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Выполняем поиск IP-адреса в чёрном списке
						auto i = ::__awh_blacklist__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
						// Если IP-адрес найден в чёрном списке
						if(i != ::__awh_blacklist__.ipv4.end())
							// Удаляем IP-адрес из чёрного списка
							::__awh_blacklist__.ipv4.erase(i);
					} break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод удаления IP-адреса из чёрного списка
 *
 * @param ip адрес для удаления из чёрного списка
 */
void awh::unit::DNS::removeAddressInBlacklist(const net::addr_t * ip) noexcept {
	// Если IP-адрес передан
	if(ip != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Выполняем поиск IP-адреса в чёрном списке
					auto i = ::__awh_blacklist__.ipv4.find(awh_cast <const net::addr_net_ipv4_t *> (ip)->address);
					// Если IP-адрес найден в чёрном списке
					if(i != ::__awh_blacklist__.ipv4.end())
						// Удаляем IP-адрес из чёрного списка
						::__awh_blacklist__.ipv4.erase(i);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
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
 */
void awh::unit::DNS::removeAddressInBlacklist(const event::family_t family, string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param ip адрес для добавления в чёрный список
 */
void awh::unit::DNS::pushAddressToBlacklist(string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(ip)){
				// Выполняем блокировку потока для работы с чёрным списком
				const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param ip адрес для добавления в чёрный список
 */
void awh::unit::DNS::pushAddressToBlacklist(const net::addr_t * ip) noexcept {
	// Если IP-адрес передан
	if(ip != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Выполняем добавление IP-адреса в чёрный список
					::__awh_blacklist__.ipv4.emplace(awh_cast <const net::addr_net_ipv4_t *> (ip)->address);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Выполняем добавление IP-адреса в чёрный список
					::__awh_blacklist__.ipv6.emplace(awh_cast <const net::addr_net_ipv6_t *> (ip)->address);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
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
 */
void awh::unit::DNS::pushAddressToBlacklist(const event::family_t family, string_view ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем добавление IP-адреса в чёрный список
						::__awh_blacklist__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(ip, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
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
 */
bool awh::unit::DNS::checkAddressInBlacklist(string_view ip) const noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(const_cast <dns_t *> (this)->_addr.parse(ip)){
				// Выполняем блокировку потока для работы с чёрным списком
				const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param ip адрес для проверки наличия в чёрном списке
 * @return   результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::unit::DNS::checkAddressInBlacklist(const net::addr_t * ip) const noexcept {
	// Если IP-адрес передан
	if(ip != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
					// Выполняем поиск IP-адреса в чёрном списке
					return (::__awh_blacklist__.ipv4.find(awh_cast <const net::addr_net_ipv4_t *> (ip)->address) != ::__awh_blacklist__.ipv4.end());
				}
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с чёрным списком
					const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
					// Выполняем поиск IP-адреса в чёрном списке
					return (::__awh_blacklist__.ipv6.find(awh_cast <const net::addr_net_ipv6_t *> (ip)->address) != ::__awh_blacklist__.ipv6.end());
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::unit::DNS::checkAddressInBlacklist(const event::family_t family, string_view ip) const noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(const_cast <dns_t *> (this)->_addr.parse(ip, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем поиск IP-адреса в чёрном списке
						return (::__awh_blacklist__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address) != ::__awh_blacklist__.ipv4.end());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(const_cast <dns_t *> (this)->_addr.parse(ip, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для работы с чёрным списком
						const locker_t <std::shared_mutex> lock(::__awh_blacklist__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выполняем поиск IP-адреса в чёрном списке
						return (::__awh_blacklist__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address) != ::__awh_blacklist__.ipv6.end());
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), ip), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод очистки кэша
 *
 */
void awh::unit::DNS::clearCache() noexcept {
	// Выполняем блокировку потока для работы с кэшем
	const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Если в кэше есть IPv4-адреса
	if(!::__awh_cache__.ipv4.empty()){
		/**
		 * Выполняем перебор всех IP-адресов в кэше
		 */
		for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
			// Выполняем перебор всех записей IP-адреса
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
			// Выполняем перебор всех записей IP-адреса
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
			// Выполняем перебор всех записей доменного имени
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
 */
void awh::unit::DNS::clearCache(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с кэшем
		const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Если в кэше есть доменные имена
		if(!::__awh_cache__.domains.empty()){
			/**
			 * Выполняем перебор всех доменных имён в кэше
			 */
			for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
				// Выполняем перебор всех записей доменного имени
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
						// Выполняем перебор всех записей IP-адреса
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
						// Выполняем перебор всех записей IP-адреса
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::unit::DNS::clearCache(string_view domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(string{domain});
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Выполняем перебор всех записей доменного имени
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
					// Выполняем перебор всех записей IPv4-адреса
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
					// Выполняем перебор всех записей IPv6-адреса
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::unit::DNS::clearCache(const event::family_t family, string_view domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
							// Выполняем перебор всех записей IPv4-адреса
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
							// Выполняем перебор всех записей IPv6-адреса
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
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
 */
string awh::unit::DNS::extractAddressFromCache(const event::family_t family, string_view domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
								// Выполняем блокировку потока для парсинга IP-адреса
								const locker_t <> lock(::__awh_mtx__);
								// Устанавливаем полученный IP-адрес
								this->_addr.source(j->ip.get(), net_addr_t::endian_t::LITTLE);
								// Выводим результат
								return static_cast <string> (this->_addr);
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if(j->ip->size == 16){
								// Выполняем блокировку потока для парсинга IP-адреса
								const locker_t <> lock(::__awh_mtx__);
								// Устанавливаем полученный IP-адрес
								this->_addr.source(j->ip.get(), net_addr_t::endian_t::LITTLE);
								// Выводим результат
								return static_cast <string> (this->_addr);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод получения IP-адреса из кэша
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param value  IP-адрес находящийся в кэше
 * @return       результат выполнения операции
 */
bool awh::unit::DNS::extractAddressFromCache(const event::family_t family, string_view domain, unique_ptr <net::addr_t> & value) noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
								// Выводим результат
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
								// Выводим результат
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 */
void awh::unit::DNS::pushAddressToCache(string_view domain, string_view ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока для парсинга IP-адреса
		const locker_t <> lock(::__awh_mtx__);
		// Выполняем парсинг IP-адреса
		if(this->_addr.parse(ip)){
			// Получаем IP-адрес в исходном виде
			auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
			// Выполняем добавление записи в кэш
			this->pushAddressToCache(domain, ip.get(), ttl);
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 */
void awh::unit::DNS::pushAddressToCache(string_view domain, const net::addr_t * ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && (ip != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
					record.life = (now + static_cast <uint64_t>  (ttl * 1000));
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
								record.life = (now + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
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
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (now + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
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
				// Выполняем добавление IP-адреса
				i->second.push_back(::move(record));
			// Если в кэше доменное имя не найдено
			} else {
				// Создаём список записей IP-адресов
				vector <EntryIP> entry(1);
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				// Устанавливаем время жизни
				entry.back().life = (now + static_cast <uint64_t>  (ttl * 1000));
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
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (now + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <EntryDomain> entry(1);
							// Устанавливаем доменное имя
							entry.back().domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								entry.back().life = (now + static_cast <uint64_t>  (ttl * 1000));
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
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (now + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <EntryDomain> entry(1);
							// Устанавливаем доменное имя
							entry.back().domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								entry.back().life = (now + static_cast <uint64_t>  (ttl * 1000));
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain, ttl), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
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
				// Выполняем блокировку потока для парсинга IP-адреса
				const locker_t <> lock(::__awh_mtx__);
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
				// Выполняем блокировку потока для парсинга IP-адреса
				const locker_t <> lock(::__awh_mtx__);
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
 * @brief Метод установки префикса переменной окружения
 *
 * @param prefix префикс переменной окружения для установки
 */
void awh::unit::DNS::setPrefixEnvironment(string_view prefix) noexcept {
	// Выполняем блокировку потока для работы с событием DNS-резолвера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Если префикс переменной окружения передан
	if(!prefix.empty())
		// Устанавливаем префикс переменной окружения
		this->_resolver.prefix = this->_fmk->transform(prefix, fmk_t::transform_t::UPPER_CASE);
	// Если префикс переменной окружения не передан, очищаем префикс переменной окружения
	else this->_resolver.prefix.clear();
}
/**
 * @brief Метод установки адреса файла локальных хостов
 *
 * @param filename адрес файла для установки
 */
void awh::unit::DNS::setHostsAddress(string_view filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес файла дампа кэша передан
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
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to set options for hosts file event", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
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
					// Выводим сообщение об ошибке
					this->_log->debug("[%s] host address cannot be established", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, filename);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса файлового дампа кэша
 *
 * @param filename адрес файла для установки
 * @param interval интервал сохранения дампа кэша в миллисекундах
 */
void awh::unit::DNS::setDumpAddress(string_view filename, const uint32_t interval) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес файла дампа кэша передан
		if(!filename.empty()){
			{
				// Выполняем блокировку потока для работы с бинарным контейнером
				const locker_t <> lock(::__awh_mtx__);
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
											this->pushAddressToCache(reinterpret_cast <char *> (record.domain), ip.get(), record.life);
										} break;
										// Если адрес является IPv6
										case 16:
											// Выполняем инициализацию объекта IP-адреса
											unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
											// Устанавливаем IP-адрес из записи кэша доменных имён
											::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address, record.ip, 16);
											// Устанавливаем запись в кэш доменных имён
											this->pushAddressToCache(reinterpret_cast <char *> (record.domain), ip.get(), record.life);
										break;
									}
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
					if(::__awh_cache__.interval != interval){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Удаляем старый интервал
						this->_io->destroy(::__awh_cache__.tid);
					// Если интервал времени сохранения дампа кэша совпадает с новым интервалом, то просто выходим из функции
					} else return;
				}
				// Результат установки интервала сохранения дампа кэша
				bool result = false;
				{
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Устанавливаем интервал сохранения дампа кэша
					::__awh_cache__.interval = interval;
					// Устанавливаем адрес файла дампа кэша
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
					if(!(result = (this->_io->commit(::__awh_cache__.tid) && this->_io->launch(::__awh_cache__.tid)))){
						// Удаляем событие интервала
						this->_io->destroy(::__awh_cache__.tid);
						// Сбрасываем идентификатор события интервала
						::__awh_cache__.tid = 0;
						// Сбрасываем интервал сохранения дампа кэша
						::__awh_cache__.interval = 0;
						// Сбрасываем адрес файла дампа кэша
						::__awh_cache__.filename = "";
					}
				}
				// Если не удалось установить интервал сохранения дампа кэша
				if(!result){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to start cache dump interval", __PRETTY_FUNCTION__, std::make_tuple(filename, interval), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to start cache dump interval", log_t::flag_t::CRITICAL);
						#endif
					}
				}
			}
		// Если адрес файла дампа кэша не передан, но интервал сохранения дампа кэша установлен, то удаляем старый интервал
		} else if(interval > 0) {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Удаляем событие интервала
			this->_io->destroy(::__awh_cache__.tid);
			// Сбрасываем идентификатор события интервала
			::__awh_cache__.tid = 0;
			// Сбрасываем интервал сохранения дампа кэша
			::__awh_cache__.interval = 0;
			// Сбрасываем адрес файла дампа кэша
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(filename, interval), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод сброса DNS-резолвера
 *
 * @return результат выполнения операции
 */
bool awh::unit::DNS::reset() noexcept {
	// Получаем семейство IP-адресов текущего события DNS-резолвера
	const event::family_t family = this->_io->family(this->_resolver.eid);
	{
		// Выполняем блокировку потока для работы с событием DNS-резолвера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Удаляем событие DNS-резолвера
		this->_io->destroy(this->_resolver.eid);
	}
	// Выполняем создание события DNS-резолвера для указанного семейство IP-адресов
	this->create(family);
	// Выполняем фиксацию параметров DNS-резолвера
	return this->commit();
}
/**
 * @brief Метод фиксации параметров DNS-резолвера
 *
 * @return результат выполнения операции
 */
bool awh::unit::DNS::commit() noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Устанавливаем порт события
			this->_io->setPort(this->_resolver.eid, this->_resolver.port);
			// Получаем семейство IP-адресов текущего события DNS-резолвера
			const event::family_t family = this->_io->family(this->_resolver.eid);
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Если префикс для переменных окружения установлен
					if(!this->_resolver.prefix.empty()){
						// Получаем значение переменной
						const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV4_SERVER", this->_resolver.prefix.c_str()).c_str());
						// Если IP-адрес из переменной окружения получен
						if(env != nullptr){
							// Устанавливаем адрес сервера назначения
							this->_io->setTarget(this->_resolver.eid, env);
							// Выходим из условия
							break;
						}
					}
					// Устанавливаем адрес сервера назначения
					this->_io->setTarget(this->_resolver.eid, this->_resolver.nameServers.get(family));
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Если префикс для переменных окружения установлен
					if(!this->_resolver.prefix.empty()){
						// Получаем значение переменной
						const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV6_SERVER", this->_resolver.prefix.c_str()).c_str());
						// Если IP-адрес из переменной окружения получен
						if(env != nullptr){
							// Устанавливаем адрес сервера назначения
							this->_io->setTarget(this->_resolver.eid, env);
							// Выходим из условия
							break;
						}
					}
					// Устанавливаем адрес сервера назначения
					this->_io->setTarget(this->_resolver.eid, this->_resolver.nameServers.get(family));
				} break;
			}
			// Если адрес сети для выполнения запроса установлен
			if(this->_resolver.source != nullptr){
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Устанавливаем IP-адрес события
						this->_io->setAddress(this->_resolver.eid, event::address_t::IPV4, this->_resolver.source.get());
					break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Устанавливаем IP-адрес события
						this->_io->setAddress(this->_resolver.eid, event::address_t::IPV6, this->_resolver.source.get());
					break;
				}
			}
			// Выполняем фиксацию параметров события и его запуск
			if(!(result = this->_io->commit(this->_resolver.eid) && this->_io->launch(this->_resolver.eid)))
				// Удаляем событие DNS-резолвера
				this->_io->destroy(this->_resolver.eid);
		}
		// Если не удалось запустить событие DNS-резолвера
		if(!result){
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to launch DNS-resolver", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to launch DNS-resolver", log_t::flag_t::CRITICAL);
				#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения порта сервера DNS-резолвера
 *
 * @return порт сервера DNS-резолвера
 */
uint16_t awh::unit::DNS::getPort() const noexcept {
	// Получаем порт события
	return this->_resolver.port;
}
/**
 * @brief Метод установки порта сервера DNS-резолвера
 *
 * @param port порт сервера DNS-резолвера
 */
void awh::unit::DNS::setPort(const uint16_t port) noexcept {
	// Если порт для установки передан
	if(port > 0){
		// Выполняем блокировку потока для работы с событием DNS-резолвера
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Устанавливаем порт события
		this->_resolver.port = port;
	}
}
/**
 * @brief Метод установки адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для установки
 */
void awh::unit::DNS::setServer(string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((this->_resolver.eid > 0) && !server.empty()){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(server)){
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Очищаем список IP-адресов события для семейство IPv4
						this->_resolver.nameServers.reset(event::family_t::IPV4);
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Очищаем список IP-адресов события для семейство IPv6
						this->_resolver.nameServers.reset(event::family_t::IPV6);
					break;
				}
				// Устанавливаем IP-адрес события
				this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
			}
		// Если адрес DNS-сервера не передан
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Очищаем список IP-адресов события для семейство IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейство IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для установки
 */
void awh::unit::DNS::setServer(const net::addr_t * server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((this->_resolver.eid > 0) && (server != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
					// Устанавливаем IP-адрес события
					this->_resolver.nameServers.push(server);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
					// Устанавливаем IP-адрес события
					this->_resolver.nameServers.push(server);
				} break;
			}
		// Если адрес DNS-сервера не передан
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Очищаем список IP-адресов события для семейство IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейство IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса DNS-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param server адрес DNS-сервера для установки
 */
void awh::unit::DNS::setServer(const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((this->_resolver.eid > 0) && !server.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Очищаем список IP-адресов события для семейство IPv4
						this->_resolver.nameServers.reset(event::family_t::IPV4);
						// Устанавливаем IP-адрес события
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Очищаем список IP-адресов события для семейство IPv6
						this->_resolver.nameServers.reset(event::family_t::IPV6);
						// Устанавливаем IP-адрес события
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
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
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для добавления
 */
void awh::unit::DNS::addServer(string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((this->_resolver.eid > 0) && !server.empty()){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(server)){
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Устанавливаем IP-адрес события
				this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса DNS-сервера
 *
 * @param server адрес DNS-сервера для добавления
 */
void awh::unit::DNS::addServer(const net::addr_t * server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((this->_resolver.eid > 0) && (server != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Устанавливаем IP-адрес события
					this->_resolver.nameServers.push(server);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Устанавливаем IP-адрес события
					this->_resolver.nameServers.push(server);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса DNS-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param server адрес DNS-сервера для добавления
 */
void awh::unit::DNS::addServer(const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((this->_resolver.eid > 0) && !server.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Устанавливаем IP-адрес события
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Устанавливаем IP-адрес события
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов DNS-серверов
 *
 * @param server адреса DNS-серверов для установки
 */
void awh::unit::DNS::setServers(const vector <string> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса DNS-серверов переданы
		if((this->_resolver.eid > 0) && !servers.empty()){
			// Результат выполнения парсинга IP-адреса
			bool result = false;
			// Флаг сброса списка IP-адресов события для семейства IPv4
			bool resetIPv4 = false;
			// Флаг сброса списка IP-адресов события для семейства IPv6
			bool resetIPv6 = false;
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
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
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv4
							resetIPv4 = true;
						break;
						// Если адрес является IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv6
							resetIPv6 = true;
						break;
					}
				// Выходим из цикла
				} else break;
			}
			// Если парсинг адресов выполнен
			if(result){
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Очищаем список IP-адресов события для семейство IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Очищаем список IP-адресов события для семейство IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
				/**
				 * Проходим по каждому адресу DNS-сервера для установки
				 */
				for(const auto & server : servers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(server))
						// Устанавливаем IP-адрес события
						this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
				}
			}
		// Если адреса DNS-серверов не переданы
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Очищаем список IP-адресов события для семейство IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейство IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов DNS-серверов
 *
 * @param server адреса DNS-серверов для установки
 */
void awh::unit::DNS::setServers(const vector <const net::addr_t *> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса DNS-серверов переданы
		if((this->_resolver.eid > 0) && !servers.empty()){
			// Флаг сброса списка IP-адресов события для семейства IPv4
			bool resetIPv4 = false;
			// Флаг сброса списка IP-адресов события для семейства IPv6
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
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv4
							resetIPv4 = true;
						break;
						// Если адрес является IPv6
						case 16:
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv6
							resetIPv6 = true;
						break;
					}
				}
			}
			// Если необходимо сбросить список IPv4 или IPv6
			if(resetIPv4 || resetIPv6){
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Очищаем список IP-адресов события для семейство IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Очищаем список IP-адресов события для семейство IPv6
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
								// Устанавливаем IP-адрес события
								this->_resolver.nameServers.push(server);
							break;
							// Если адрес является IPv6
							case 16:
								// Устанавливаем IP-адрес события
								this->_resolver.nameServers.push(server);
							break;
						}
					}
				}
			}
		// Если адрес DNS-сервера не передан
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Очищаем список IP-адресов события для семейство IPv4
			this->_resolver.nameServers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейство IPv6
			this->_resolver.nameServers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов DNS-серверов
 *
 * @param family  семейство IP-адресов IPv4/IPv6
 * @param servers адреса DNS-серверов для установки
 */
void awh::unit::DNS::setServers(const event::family_t family, const vector <string> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса DNS-серверов переданы
		if((this->_resolver.eid > 0) && !servers.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv4
					this->_resolver.nameServers.reset(event::family_t::IPV4);
					/**
					 * Проходим по каждому адресу DNS-сервера для установки
					 */
					for(const auto & server : servers){
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						// Выполняем парсинг IPv4-адреса
						if(this->_addr.parse(server, net_addr_t::type_t::IPV4))
							// Устанавливаем IP-адрес события
							this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Выходим из цикла
						else break;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv6
					this->_resolver.nameServers.reset(event::family_t::IPV6);
					/**
					 * Проходим по каждому адресу DNS-сервера для установки
					 */
					for(const auto & server : servers){
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						// Выполняем парсинг IPv6-адреса
						if(this->_addr.parse(server, net_addr_t::type_t::IPV6))
							// Устанавливаем IP-адрес события
							this->_resolver.nameServers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Выходим из цикла
						else break;
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
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv4
					this->_resolver.nameServers.reset(family);
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Очищаем список IP-адресов события для семейство IPv6
					this->_resolver.nameServers.reset(family);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, static_cast <uint16_t> (family), servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::DNS::setSource(string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((this->_resolver.eid > 0) && !source.empty()){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(source)){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Получаем IP-адрес в исходном виде
						this->_resolver.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					} break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Получаем IP-адрес в исходном виде
						this->_resolver.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					} break;
				}
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Сбрасываем IP-адрес события
			this->_resolver.source.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::DNS::setSource(const net::addr_t * source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((this->_resolver.eid > 0) && (source != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(source->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Выполняем инициализацию объекта IP-адреса
					this->_resolver.source = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_resolver.source.get())->address = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Выполняем инициализацию объекта IP-адреса
					this->_resolver.source = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_resolver.source.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], 16);
				} break;
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Сбрасываем IP-адрес события
			this->_resolver.source.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::DNS::setSource(const event::family_t family, string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((this->_resolver.eid > 0) && !source.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(source, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Получаем IP-адрес в исходном виде
						this->_resolver.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(source, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Получаем IP-адрес в исходном виде
						this->_resolver.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					}
				} break;
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для работы с событием DNS-резолвера
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Сбрасываем IP-адрес события
			this->_resolver.source.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_resolver.eid, static_cast <uint16_t> (family), source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения идентификатора DNS-резолвера для выполнения запроса к DNS-серверу
 *
 * @return идентификатор DNS-резолвера для выполнения запроса к DNS-серверу
 */
awh::unit::DNS::id_t awh::unit::DNS::issue() const noexcept {
	// Создаём идентификатор события DNS-резолвера
	return ::dns::identifier();
}
/**
 * @brief Метод получения типа события
 *
 * @return тип события
 */
awh::event::type_t awh::unit::DNS::type() const noexcept {
	// Получаем тип события DNS-резолвера
	return this->_io->type(this->_resolver.eid);
}
/**
 * @brief Метод получения типа узла события
 *
 * @return тип узла события
 */
awh::event::node_t awh::unit::DNS::node() const noexcept {
	// Получаем тип узла события DNS-резолвера
	return this->_io->node(this->_resolver.eid);
}
/**
 * @brief Метод получения семейства события
 *
 * @return семейство адресов
 */
awh::event::family_t awh::unit::DNS::family() const noexcept {
	// Получаем семейство адресов события DNS-резолвера
	return this->_io->family(this->_resolver.eid);
}
/**
 * @brief Метод получения статуса события
 *
 * @return статус события
 */
awh::event::status_t awh::unit::DNS::status() const noexcept {
	// Получаем статус события DNS-резолвера
	return this->_io->status(this->_resolver.eid);
}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
 * @param ip      адрес для поиска доменного имени
 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::DNS::search(const id_t id, string_view ip, const uint32_t timeout) noexcept {
	// Выполняем поиск доменного имени соответствующему IP-адресу
	return this->search(id, this->_io->family(this->_resolver.eid), ip, timeout);
}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
 * @param ip      адрес для поиска доменного имени
 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::DNS::search(const id_t id, const net::addr_t * ip, const uint32_t timeout) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((ip != nullptr) && this->_callback.is("address")){
			// Доменное имя в формате ARPA для обратного поиска доменного имени по IP-адресу
			string domain = "";
			{
				// Выполняем блокировку потока для парсинга IP-адреса
				const locker_t <> lock(::__awh_mtx__);
				// Устанавливаем полученный IP-адрес
				this->_addr.source(ip);
				// Извлекаем доменное имя в формате ARPA
				domain = ::move(this->_addr.arpa());
			}
			/**
			 * Определяем тип адреса
			 */
			switch(static_cast <uint8_t> (ip->size)){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для работы с кэшем
					const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV4, j->domain, ip);
							// Выводим положительный результат
							return true;
						}
					}
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для работы с кэшем
					const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV6, j->domain, ip);
							// Выводим положительный результат
							return true;
						}
					}
				} break;
			}
			// Если доменное имя получено
			if(!domain.empty()){
				// Выполняем блокировку потока для работы с контейнером пакетов
				const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Добавляем пакет в контейнер пакетов для отслеживания его выполнения
				auto ret = this->_transfer.waiting.emplace(id, packet_t());
				// Если пакет уже существует для данного идентификатора DNS-резолвера
				if(!ret.second){
					// Формируем текст выводимой ошибки DNS-резолвера
					const string error = this->_fmk->format("DNS request for ID=%d is still in progress, please wait for the result", id);
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_resolver.eid, event::error_t::INVALID, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, timeout), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
					// Выводим результат по умолчанию
					return false;
				// Если таймаут успешно добавлен для данного идентификатора DNS-резолвера
				} else {
					// Флаг успешного добавления таймаута для данного идентификатора DNS-резолвера
					bool result = false;
					{
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Добавляем новое событие таймаута для ожидания ответа от DNS-сервера
						const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
						// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от DNS-сервера
						this->_io->setTimeout(tid, event::action_t::NONE, (timeout > 0 ? timeout : 5000));
						// Устанавливаем функцию обратного вызова на событие получения ошибок
						this->_io->on(tid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
						// Если не удалось установить таймер для ожидания ответа от DNS-сервера
						if(!(result = this->_io->commit(tid)))
							// Удаляем событие таймера
							this->_io->destroy(tid);
						// Если таймер для ожидания ответа от DNS-сервера успешно установлен
						else {
							// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
							ret.first->second.tid = tid;
							// Устанавливаем доменное имя для отслеживания его выполнения
							ret.first->second.domain = domain;
							// Устанавливаем время жизни пакета для отслеживания его выполнения
							ret.first->second.delay = timeout;
							// Устанавливаем тип записи для отслеживания передачи пакета
							ret.first->second.record = record_t::PTR;
							// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от DNS-сервера
							this->_io->on(tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::timeout, this, id, _1, _2, &ret.first->second)));
							// Запускаем таймер для ожидания ответа от DNS-сервера
							this->_io->launch(tid);
						}
					}
					// Если не удалось установить таймер для ожидания ответа от DNS-сервера
					if(!result){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to commit DNS resolver timeout", __PRETTY_FUNCTION__, std::make_tuple(id, timeout), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to commit DNS resolver timeout", log_t::flag_t::CRITICAL);
							#endif
						}
						// Выводим результат по умолчанию
						return false;
					}
				}
			}
			// Выполняем резолвинг доменного имени
			if(!domain.empty()){
				// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
				const size_t size = ::dns::request(id, record_t::PTR, domain, this->_log);
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Отправляем запрос на резолвинг доменного имени
				return (this->_io->send(this->_resolver.eid, ::dns::buffer, size) > 0);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
 * @param family  тип интернет-протокола IPv4/IPv6
 * @param ip      адрес для поиска доменного имени
 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::DNS::search(const id_t id, const event::family_t family, string_view ip, const uint32_t timeout) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!ip.empty() && this->_callback.is("address")){
			// Доменное имя в формате ARPA для обратного поиска доменного имени по IP-адресу
			string domain = "";
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// IP-адрес в исходном виде для поиска доменного имени
					unique_ptr <net::addr_t> addr = nullptr;
					{
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						// Выполняем парсинг IPv4-адреса
						if(this->_addr.parse(ip, net_addr_t::type_t::IPV4)){
							// Получаем IP-адрес в исходном виде
							addr = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							// Извлекаем доменное имя в формате ARPA
							domain = ::move(this->_addr.arpa());
						}
					}
					// Если IP-адрес в исходном виде получен
					if(addr != nullptr){
						// Выполняем блокировку потока для работы с кэшем
						const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Выполняем поиск IP-адреса в кэше
						auto i = ::__awh_cache__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (addr.get())->address);
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
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV4, j->domain, addr.get());
								// Выводим положительный результат
								return true;
							}
						}
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// IP-адрес в исходном виде для поиска доменного имени
					unique_ptr <net::addr_t> addr = nullptr;
					{
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						// Выполняем парсинг IPv6-адреса
						if(this->_addr.parse(ip, net_addr_t::type_t::IPV6)){
							// Получаем IP-адрес в исходном виде
							addr = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
							// Извлекаем доменное имя в формате ARPA
							domain = ::move(this->_addr.arpa());
						}
					}
					// Если IP-адрес в исходном виде получен
					if(addr != nullptr){
						// Выполняем блокировку потока для работы с кэшем
						const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Выполняем поиск IP-адреса в кэше
						auto i = ::__awh_cache__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (addr.get())->address);
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
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV6, j->domain, addr.get());
								// Выводим положительный результат
								return true;
							}
						}
					}
				} break;
			}
			// Если доменное имя получено
			if(!domain.empty()){
				// Текст ошибки для вывода при невозможности выполнить запрос к DNS-серверу
				string error = "";
				{
					// Выполняем блокировку потока для работы с контейнером пакетов
					const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Добавляем пакет в контейнер пакетов для отслеживания его выполнения
					auto ret = this->_transfer.waiting.emplace(id, packet_t());
					// Если пакет уже существует для данного идентификатора DNS-резолвера
					if(!ret.second)
						// Формируем текст выводимой ошибки DNS-резолвера
						error = this->_fmk->format("DNS request for ID=%d is still in progress, please wait for the result", id);
					// Если таймаут успешно добавлен для данного идентификатора DNS-резолвера
					else {
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Добавляем новое событие таймаута для ожидания ответа от DNS-сервера
						const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
						// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от DNS-сервера
						this->_io->setTimeout(tid, event::action_t::NONE, (timeout > 0 ? timeout : 5000));
						// Устанавливаем функцию обратного вызова на событие получения ошибок
						this->_io->on(tid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
						// Если не удалось установить таймер для ожидания ответа от DNS-сервера
						if(!this->_io->commit(tid)){
							// Удаляем событие таймера
							this->_io->destroy(tid);
							// Удаляем пакет из контейнера ожидания выполнения запроса
							this->_transfer.waiting.erase(id);
							// Формируем текст выводимой ошибки DNS-резолвера
							error = "Failed to commit DNS resolver timeout";
						// Если таймер для ожидания ответа от DNS-сервера успешно установлен
						} else {
							// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
							ret.first->second.tid = tid;
							// Устанавливаем доменное имя для отслеживания его выполнения
							ret.first->second.domain = domain;
							// Устанавливаем время жизни пакета для отслеживания его выполнения
							ret.first->second.delay = timeout;
							// Устанавливаем тип записи для отслеживания передачи пакета
							ret.first->second.record = record_t::PTR;
							// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от DNS-сервера
							this->_io->on(tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::timeout, this, id, _1, _2, &ret.first->second)));
							// Запускаем таймер для ожидания ответа от DNS-сервера
							this->_io->launch(tid);
						}
					}
				}
				// Если возникла ошибка при добавлении таймаута для данного идентификатора DNS-резолвера
				if(!error.empty()){
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_resolver.eid, event::error_t::INVALID, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (family), ip, timeout), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
					// Выводим результат по умолчанию
					return false;
				}
			}
			// Выполняем резолвинг доменного имени
			if(!domain.empty()){
				// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
				const size_t size = ::dns::request(id, record_t::PTR, domain, this->_log);
				// Выполняем блокировку потока для работы с событием DNS-резолвера
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Отправляем запрос на резолвинг доменного имени
				return (this->_io->send(this->_resolver.eid, ::dns::buffer, size) > 0);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (family), ip, timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод выполнения произвольного запроса
 *
 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
 * @param record  тип DNS-записи которую необходимо получить
 * @param domain  доменное имя сервера
 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::DNS::request(const id_t id, const record_t record, string_view domain, const uint32_t timeout) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано и функция обратного вызова установлена для получения IP-адресов
		if(!domain.empty()){
			// Текст ошибки для вывода при невозможности выполнить запрос к DNS-серверу
			string error = "";
			{
				// Выполняем блокировку потока для работы с контейнером пакетов
				const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Добавляем пакет в контейнер пакетов для отслеживания его выполнения
				auto ret = this->_transfer.waiting.emplace(id, packet_t());
				// Если пакет уже существует для данного идентификатора DNS-резолвера
				if(!ret.second)
					// Формируем текст выводимой ошибки DNS-резолвера
					error = this->_fmk->format("DNS request for ID=%d is still in progress, please wait for the result", id);
				// Если таймаут успешно добавлен для данного идентификатора DNS-резолвера
				else {
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Добавляем новое событие таймаута для ожидания ответа от DNS-сервера
					const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
					// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от DNS-сервера
					this->_io->setTimeout(tid, event::action_t::NONE, (timeout > 0 ? timeout : 5000));
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(tid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
					// Если не удалось установить таймер для ожидания ответа от DNS-сервера
					if(!this->_io->commit(tid)){
						// Удаляем событие таймера
						this->_io->destroy(tid);
						// Удаляем пакет из контейнера ожидания выполнения запроса
						this->_transfer.waiting.erase(id);
						// Формируем текст выводимой ошибки DNS-резолвера
						error = "Failed to commit DNS resolver timeout";
					// Если таймер для ожидания ответа от DNS-сервера успешно установлен
					} else {
						// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
						ret.first->second.tid = tid;
						// Устанавливаем доменное имя для отслеживания его выполнения
						ret.first->second.domain = domain;
						// Устанавливаем время жизни пакета для отслеживания его выполнения
						ret.first->second.delay = timeout;
						// Устанавливаем тип записи для отслеживания передачи пакета
						ret.first->second.record = record;
						// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от DNS-сервера
						this->_io->on(tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::timeout, this, id, _1, _2, &ret.first->second)));
						// Запускаем таймер для ожидания ответа от DNS-сервера
						this->_io->launch(tid);
						// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
						const size_t size = ::dns::request(id, record, domain, this->_log);
						// Отправляем запрос на резолвинг доменного имени
						return (this->_io->send(this->_resolver.eid, ::dns::buffer, size) > 0);
					}
				}
			}
			// Если возникла ошибка при добавлении таймаута для данного идентификатора DNS-резолвера
			if(!error.empty()){
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_resolver.eid, event::error_t::INVALID, error);
				// Если функция вывода ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (record), domain, timeout), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
				// Выводим результат по умолчанию
				return false;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (record), domain, timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод резолвинга доменного имени
 *
 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
 * @param domain  доменное имя сервера
 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::DNS::resolve(const id_t id, string_view domain, const uint32_t timeout) noexcept {
	// Выполняем резолвинг доменного имени
	return this->resolve(id, this->_io->family(this->_resolver.eid), domain, timeout);
}
/**
 * @brief Метод резолвинга доменного имени
 *
 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
 * @param family  тип интернет-протокола IPv4/IPv6
 * @param domain  доменное имя сервера
 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::DNS::resolve(const id_t id, const event::family_t family, string_view domain, const uint32_t timeout) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано и функция обратного вызова установлена для получения IP-адресов
		if(!domain.empty() && this->_callback.is("address")){
			{
				// Выполняем блокировку потока для работы с кэшем
				const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV4, string{domain}, j->ip.get());
									// Выводим положительный результат
									return true;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								// Если IP-адрес доменного имени является IPv6
								if(j->ip->size == 16){
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const id_t, const event::family_t, const string &, const net::addr_t *)> ("address", id, event::family_t::IPV6, string{domain}, j->ip.get());
									// Выводим положительный результат
									return true;
								}
							} break;
						}
					}
				}
			}{
				// Текст ошибки для вывода при невозможности выполнить запрос к DNS-серверу
				string error = "";
				{
					// Выполняем блокировку потока для работы с контейнером пакетов
					const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Добавляем пакет в контейнер пакетов для отслеживания его выполнения
					auto ret = this->_transfer.waiting.emplace(id, packet_t());
					// Если пакет уже существует для данного идентификатора DNS-резолвера
					if(!ret.second)
						// Формируем текст выводимой ошибки DNS-резолвера
						error = this->_fmk->format("DNS request for ID=%d is still in progress, please wait for the result", id);
					// Если таймаут успешно добавлен для данного идентификатора DNS-резолвера
					else {
						// Выполняем блокировку потока для работы с событием DNS-резолвера
						const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Добавляем новое событие таймаута для ожидания ответа от DNS-сервера
						const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
						// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от DNS-сервера
						this->_io->setTimeout(tid, event::action_t::NONE, (timeout > 0 ? timeout : 5000));
						// Устанавливаем функцию обратного вызова на событие получения ошибок
						this->_io->on(tid, static_cast <engine::callback::error_t> (std::bind(&dns_t::error, this, _1, _2, _3)));
						// Если не удалось установить таймер для ожидания ответа от DNS-сервера
						if(!this->_io->commit(tid)){
							// Удаляем событие таймера
							this->_io->destroy(tid);
							// Удаляем пакет из контейнера ожидания выполнения запроса
							this->_transfer.waiting.erase(id);
							// Формируем текст выводимой ошибки DNS-резолвера
							error = "Failed to commit DNS resolver timeout";
						// Если таймер для ожидания ответа от DNS-сервера успешно установлен
						} else {
							// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
							ret.first->second.tid = tid;
							// Устанавливаем доменное имя для отслеживания его выполнения
							ret.first->second.domain = domain;
							// Устанавливаем время жизни пакета для отслеживания его выполнения
							ret.first->second.delay = timeout;
							/**
							 * Определяем семейство события
							 */
							switch(static_cast <uint8_t> (family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
									// Устанавливаем тип записи для отслеживания передачи пакета
									ret.first->second.record = record_t::A;
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
									// Устанавливаем тип записи для отслеживания передачи пакета
									ret.first->second.record = record_t::AAAA;
								break;
							}
							// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от DNS-сервера
							this->_io->on(tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::timeout, this, id, _1, _2, &ret.first->second)));
							// Запускаем таймер для ожидания ответа от DNS-сервера
							this->_io->launch(tid);
						}
					}
				}
				// Если возникла ошибка при добавлении таймаута для данного идентификатора DNS-резолвера
				if(!error.empty()){
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_resolver.eid, event::error_t::INVALID, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (family), domain, timeout), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
					// Выводим результат по умолчанию
					return false;
				}
			}
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
					const size_t size = ::dns::request(id, record_t::A, domain, this->_log);
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Отправляем запрос на резолвинг доменного имени
					return (this->_io->send(this->_resolver.eid, ::dns::buffer, size) > 0);
				}
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем генерацию запроса к DNS-серверу для получения доменного имени по IP-адресу
					const size_t size = ::dns::request(id, record_t::AAAA, domain, this->_log);
					// Выполняем блокировку потока для работы с событием DNS-резолвера
					const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Отправляем запрос на резолвинг доменного имени
					return (this->_io->send(this->_resolver.eid, ::dns::buffer, size) > 0);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (family), domain, timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::unit::DNS::DNS(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _addr(fmk, log), _binbox(fmk, log) {
	// Активируем работу мьютекса блокировки потока при работе с пакетами
	this->_transfer.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Если общие DNS-резолверы ещё не добавлены в глобальный список
	if(::ns::general.empty()){
		// Активируем работу мьютекса блокировки потока при работе с IP-адресами
		::__awh_mtx__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Активируем работу мьютекса блокировки потока при работе с кэшем
		::__awh_cache__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Активируем работу мьютекса блокировки потока при работе с чёрным списком
		::__awh_blacklist__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		{
			// Создаём массив стандартных DNS-серверов IPv4
			array <string_view, 6> resolvers = {AWH_IPV4_NS};
			// Выбираем стандарт рандомайзера
			mt19937 generator(::__awh_randev__());
			// Выполняем рандомную сортировку списка DNS-серверов
			::shuffle(resolvers.begin(), resolvers.end(), generator);
			// Выполняем перебор всех DNS-серверов из массива
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
			// Выполняем перебор всех DNS-серверов из массива
			for(const auto & item : resolvers){
				// Выполняем парсинг IP-адреса
				if(this->_addr.parse(item, net_addr_t::type_t::IPV6))
					// Добавляем DNS-сервер в глобальный список для использования при выполнении запросов к DNS-серверам
					::ns::general.push_back(::move(this->_addr.source(net_addr_t::endian_t::LITTLE)));
			}
		}
		// Добавляем новое событие таймаута для периодической очистки кэша от устаревших записей
		const event::id_t tid = this->_io->event(event::node_t::INTERVAL, event::family_t::TIMER);
		// Устанавливаем интервал таймера по умолчанию на 1 минуту для периодической очистки кэша
		this->_io->setTimeout(tid, event::action_t::NONE, 60000);
		// Устанавливаем обработчик события таймера для периодической очистки кэша
		this->_io->on(tid, static_cast <engine::callback::status_t> (std::bind(&dns_t::collector, this, _1, _2)));
		// Если не удалось установить таймер для периодической очистки кэша
		if(!this->_io->commit(tid) || !this->_io->launch(tid)){
			// Удаляем событие таймера
			this->_io->destroy(tid);
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Failed to create collector timeout", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Failed to create collector timeout", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
	/**
	 * Инициализация DNS-сервера
	 */
	this->_resolver.nameServers.init();
	/**
	 * Выполняем создание события DNS-резолвера для указанного семейство IP-адресов
	 */
	this->create(family);
}
/**
 * @brief Деструктор
 *
 */
awh::unit::DNS::~DNS() noexcept {
	// Выполняем блокировку потока для работы с событием DNS-резолвера
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Если событие DNS-резолвера активно
	if(this->_resolver.eid > 0)
		// Удаляем событие DNS-резолвера
		this->_io->destroy(this->_resolver.eid);
	// Если событие таймера для периодической очистки кэша активно
	if(::__awh_cache__.tid > 0)
		// Удаляем событие таймера для периодической очистки кэша
		this->_io->destroy(::__awh_cache__.tid);
	// Если событие таймера для периодической очистки кэша активно
	if(::__awh_cache__.fid > 0)
		// Удаляем событие таймера для периодической очистки кэша
		this->_io->destroy(::__awh_cache__.fid);
	// Если контейнер пакетов для отслеживания выполнения запросов не пустой
	if(!this->_transfer.waiting.empty()){
		// Выполняем перебор всех пакетов в контейнере пакетов
		for(auto i = this->_transfer.waiting.begin(); i != this->_transfer.waiting.end(); ++i){
			// Если событие таймаута для ожидания ответа от DNS-сервера активно
			if(i->second.tid > 0)
				// Удаляем событие таймаута для ожидания ответа от DNS-сервера
				this->_io->destroy(i->second.tid);
		}
	}
}
