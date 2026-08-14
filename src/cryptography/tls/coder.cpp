/**
 * @file coder.cpp
 * @date 2025-12-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация модуля транспортного уровня безопасности — создание и настройка контекстов TLS и DTLS,
 *        загрузка сертификатов и ключей, выбор наборов шифров и ALPN,
 *        верификация пиров и выполнение защищённого рукопожатия поверх BoringSSL
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <map>
#include <ctime>
#include <atomic>
#include <memory>
#include <csignal>
#include <shared_mutex>
#include <net/event.hpp>
#include <unordered_set>
#include <unordered_map>

/**
 * Заголовочные файлы OpenSSL
 */
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

/**
 * Если BoringSSL используется в качестве криптографической библиотеки
 */
#include <openssl/hpke.h>

/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки arpa/inet.h и netinet/in.h принадлежат POSIX и у MS Windows
 *       отсутствуют. Соответствующие им объявления, включая порядок байтов сети,
 *       приходят там из winsock2.h
 *
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>

	/**
	 * Системный заголовочный файл хранилища сертификатов
	 *
	 * @note Подключается отдельно: выключатель WIN32_LEAN_AND_MEAN, каким пользуется
	 *       sys/win32.hpp, исключает подсистему шифрования из состава windows.h,
	 *       и без него не находятся ни HCERTSTORE, ни CertOpenSystemStore
	 *
	 */
	#include <wincrypt.h>

	/**
	 * Снимаем макросы, чьи имена заняты типами OpenSSL
	 *
	 * @details Заголовок wincrypt.h заводит эти имена макросами-постоянными, например
	 *          "#define X509_NAME ((LPCSTR) 7)", а у OpenSSL это типы. Препроцессор
	 *          заменяет имя прежде разбора, и объявление вида
	 *          "X509_NAME * subject = X509_get_subject_name(x509)" превращается в
	 *          "((LPCSTR) 7) * subject = ...", отчего сборка отвечает отказом
	 *          "'subject' was not declared in this scope"
	 *
	 *          Снятие здесь постоянное и возврата не требует: файл этот работает с
	 *          OpenSSL, а не с подсистемой шифрования MS Windows, и постоянные эти
	 *          ему не нужны. Хранилище сертификатов системы, ради какого wincrypt.h
	 *          и подключён, обращается к именам иным - HCERTSTORE, CertOpenSystemStore,
	 *          CertEnumCertificatesInStore
	 *
	 */
	#undef X509_NAME
	#undef X509_EXTENSIONS
	#undef X509_CERT_PAIR
	#undef OCSP_REQUEST
	#undef OCSP_RESPONSE
	#undef PKCS7_SIGNER_INFO
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/os.hpp>
#include <sys/locker.hpp>
#include <net/net.hpp>
#include <cryptography/tls/coder.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Если максимальный размер SSL буфера не определён
 */
#ifndef AWH_MAX_SSL_BUFFER_SIZE
	/**
	 * Устанавливаем максимальный размер SSL буфера в 16 КБ
	 */
	#define AWH_MAX_SSL_BUFFER_SIZE 0x4000
#endif

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Прототип класса участника уровня защищённых сокетов
	 *
	 */
	class Member;
	/**
	 * @brief Тип контейнера участников уровня защищённых сокетов
	 *
	 */
	using members_t = unordered_set <unique_ptr <Member>>;

	/**
	 * @brief Глобальный контейнер уровней защищённых сокетов
	 *
	 */
	members_t __awh_ssl_members__;

	/**
	 * @brief Счётчик инициализации библиотеки OpenSSL
	 *
	 */
	uint16_t __awh_ssl_init_count__ = 0;

	/**
	 * @brief Флаг инициализации библиотеки OpenSSL
	 *
	 */
	bool __awh_ssl_initialized__ = false;

	/**
	 * @brief Глобальный набор идентификаторов контекстов TLS
	 *
	 */
	unordered_set <::tls::coder_t::id_t> __awh_ssl_ids__;

	/**
	 * @brief Глобальная карта сплайса контекстов TLS
	 *
	 */
	map <pair <event::protocol_t, string>, ::tls::coder_t::id_t> __awh_ssl_splice_map__;

	/**
	 * @brief Индексы для хранения состояний проверки куков
	 *
	 */
	int32_t __awh_ssl_index__[7] = {-1, -1, -1, -1, -1, -1, -1};
};

/**
 * @brief Инкапсулируем статические параметры защиты потоков в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Флаг одноразовой инициализации TLS-модуля
	 *
	 */
	once_flag __awh_ssl_init_once__;

	/**
	 * @brief Мьютекс блокировки глобального состояния TLS-модуля
	 *
	 */
	lock_state_t <std::shared_mutex> __awh_ssl_mutex__;

	/**
	 * @brief Режим безопасности работы потоков TLS-модуля
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;
};

/**
 * @brief Пространство имён доступа к глобальному реестру TLS
 *
 * @note Lock удерживается только внутри методов registry.
 * @note pin() закрепляет участника (refs++) без lock на время дальнейшей работы.
 *
 */
namespace ssl {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Пространство имён глобального реестра TLS
	 *
	 */
	namespace registry {
		/**
		 * @brief Тип блокировки глобального реестра TLS
		 *
		 */
		using global_lock_t = locker_t <std::shared_mutex>;

		/**
		 * @brief Метод получения разделяемой блокировки глобального реестра TLS
		 *
		 * @return объект блокировки
		 *
		 */
		inline global_lock_t globalShared() noexcept {
			// Возвращаем объект блокировки глобального реестра TLS
			return global_lock_t(::__awh_ssl_mutex__, global_lock_t::mode_t::SHARED);
		}
		/**
		 * @brief Метод получения эксклюзивной блокировки глобального реестра TLS
		 *
		 * @return объект блокировки
		 *
		 */
		inline global_lock_t globalExclusive() noexcept {
			// Возвращаем объект блокировки глобального реестра TLS
			return global_lock_t(::__awh_ssl_mutex__, global_lock_t::mode_t::EXCLUSIVE);
		}
		/**
		 * @brief Метод проверки регистрации идентификатора TLS
		 *
		 * @param id идентификатор контекста TLS
		 * @return   результат проверки
		 *
		 */
		inline bool contains(const ::tls::coder_t::id_t id) noexcept {
			// Выполняем разделяемую блокировку глобального реестра TLS
			const global_lock_t lock = globalShared();
			// Возвращаем результат проверки регистрации идентификатора TLS
			return ::__awh_ssl_ids__.find(id) != ::__awh_ssl_ids__.end();
		}
		/**
		 * @brief Метод регистрации идентификатора TLS
		 *
		 * @param id идентификатор контекста TLS
		 *
		 */
		inline void add(const ::tls::coder_t::id_t id) noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Регистрируем идентификатор контекста TLS
			::__awh_ssl_ids__.emplace(id);
		}
		/**
		 * @brief Метод удаления участника из глобального реестра TLS
		 *
		 * @param id       идентификатор контекста TLS
		 * @param members  контейнер участников обмена защищёнными данными
		 * @param iterator итератор удаляемого участника
		 *
		 */
		inline void drop(const ::tls::coder_t::id_t id, members_t & members, const members_t::iterator & iterator) noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end())
				// Удаляем идентификатор контекста TLS из глобального набора идентификаторов контекстов TLS
				::__awh_ssl_ids__.erase(i);
			// Удаляем уровень защищённых сокетов из контейнера
			members.erase(iterator);
		}
		/**
		 * @brief Метод добавления участника в глобальный контейнер TLS
		 *
		 * @param item объект участника обмена защищёнными данными
		 * @return   итератор добавленного участника
		 *
		 */
		inline members_t::iterator emplace(members_t::value_type item) noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Добавляем участника в глобальный контейнер и возвращаем итератор
			return ::__awh_ssl_members__.emplace(std::move(item)).first;
		}
		/**
		 * @brief Метод удаления записи из карты сплайса TLS
		 *
		 * @param proto тип протокола события
		 * @param host  имя хоста
		 *
		 */
		inline void spliceErase(const event::protocol_t proto, const std::string & host) noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Выполняем поиск записи в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
			auto i = ::__awh_ssl_splice_map__.find(std::make_pair(proto, host));
			// Если запись найдена
			if(i != ::__awh_ssl_splice_map__.end())
				// Удаляем запись из глобальной карты сопоставления имён хостов и идентификаторов узлов TLS
				::__awh_ssl_splice_map__.erase(i);
		}
		/**
		 * @brief Метод добавления записи в карту сплайса TLS
		 *
		 * @param proto тип протокола события
		 * @param host  имя хоста
		 * @param id    идентификатор контекста TLS
		 *
		 */
		inline void spliceEmplace(const event::protocol_t proto, const std::string & host, const ::tls::coder_t::id_t id) noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Добавляем запись в глобальную карту сопоставления имён хостов и идентификаторов узлов TLS
			::__awh_ssl_splice_map__.emplace(std::make_pair(proto, host), id);
		}
		/**
		 * @brief Метод поиска идентификатора контекста TLS в карте сплайса
		 *
		 * @param proto тип протокола события
		 * @param host  имя хоста
		 * @return      идентификатор контекста TLS или 0
		 *
		 */
		inline ::tls::coder_t::id_t spliceResolve(const event::protocol_t proto, const std::string & host) noexcept {
			// Выполняем разделяемую блокировку глобального реестра TLS
			const global_lock_t lock = globalShared();
			// Выполняем поиск записи в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
			auto i = ::__awh_ssl_splice_map__.find(std::make_pair(proto, host));
			// Если запись не найдена
			if(i == ::__awh_ssl_splice_map__.end())
				// Возвращаем нулевой идентификатор
				return 0;
			// Если идентификатор контекста TLS не зарегистрирован
			if(::__awh_ssl_ids__.find(i->second) == ::__awh_ssl_ids__.end())
				// Возвращаем нулевой идентификатор
				return 0;
			// Возвращаем идентификатор контекста TLS
			return i->second;
		}
		/**
		 * @brief Метод увеличения счётчика инициализации OpenSSL
		 *
		 * @return флаг необходимости инициализации OpenSSL
		 *
		 */
		inline bool acquireInit() noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Увеличиваем счётчик инициализации библиотеки OpenSSL
			::__awh_ssl_init_count__++;
			// Если библиотека OpenSSL ещё не инициализирована
			if(!::__awh_ssl_initialized__){
				// Устанавливаем флаг инициализации библиотеки OpenSSL
				::__awh_ssl_initialized__ = true;
				// Сообщаем о необходимости инициализации OpenSSL
				return true;
			}
			// Сообщаем об отсутствии необходимости инициализации OpenSSL
			return false;
		}
		/**
		 * @brief Метод уменьшения счётчика инициализации OpenSSL
		 *
		 * @return флаг необходимости деинициализации OpenSSL
		 *
		 */
		inline bool releaseInit() noexcept {
			// Выполняем эксклюзивную блокировку глобального реестра TLS
			const global_lock_t lock = globalExclusive();
			// Уменьшаем счётчик инициализации библиотеки OpenSSL
			::__awh_ssl_init_count__--;
			// Если счётчик инициализации библиотеки OpenSSL равен нулю
			if(::__awh_ssl_init_count__ == 0){
				// Сбрасываем флаг инициализации библиотеки OpenSSL
				::__awh_ssl_initialized__ = false;
				// Сообщаем о необходимости деинициализации OpenSSL
				return true;
			}
			// Сообщаем об отсутствии необходимости деинициализации OpenSSL
			return false;
		}
	}
	/**
	 * @brief Метод одноразовой инициализации OpenSSL для TLS-модуля
	 *
	 * @param log объект для работы с логами
	 *
	 */
	inline void initOpenSSL(const log_t * log) noexcept {
		/**
		 * Для операционной системы не являющейся MS Windows
		 *
		 * @note Сигнала SIGPIPE у MS Windows не существует: запись в закрытое соединение
		 *       отвечает там кодом ошибки, а не сигналом, и гасить нечего
		 *
		 */
		#if !_WIN32 && !_WIN64
		// Выполняем игнорирование сигналов SIGPIPE
		if(::signal(SIGPIPE, SIG_IGN) == SIG_ERR){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("Failed to ignore signal SIGPIPE", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("Failed to ignore signal SIGPIPE", log_t::flag_t::CRITICAL);
			#endif
		}
		#endif
		// Выполняем инициализацию OpenSSL
		::OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, nullptr);
		// Выполняем загрузки описаний ошибок SSL
		::ERR_load_SSL_strings();
		// Выполняем загрузки описаний ошибок шифрования
		::ERR_load_crypto_strings();
		// Выполняем загрузки описаний ошибок OpenSSL
		::SSL_load_error_strings();
		// Добавляем все алгоритмы шифрования
		::OpenSSL_add_all_algorithms();
		// Активируем рандомный генератор
		if(::RAND_poll() != 1){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("Rand poll is not allowed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("Rand poll is not allowed", log_t::flag_t::CRITICAL);
			#endif
			// Продолжаем инициализацию; без энтропии TLS может быть небезопасен
		}
		// Регистрируем новый индекс для хранения пользовательских данных в структуре SSL
		::__awh_ssl_index__[0] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта фреймворка AWH в структуре SSL
		::__awh_ssl_index__[1] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта логирования AWH в структуре SSL
		::__awh_ssl_index__[2] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта компрессора AWH в структуре SSL
		::__awh_ssl_index__[3] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения пользовательских данных в структуре SSL_CTX
		::__awh_ssl_index__[4] = ::SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта фреймворка AWH в структуре SSL_CTX
		::__awh_ssl_index__[5] = ::SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта логирования AWH в структуре SSL_CTX
		::__awh_ssl_index__[6] = ::SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
	}
};

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Уровни транспортной безопасности
	 *
	 */
	enum class layer_t : uint8_t {
		NONE = 0x00, // Уровень не инициализирован
		CTS  = 0x01, // Уровень шаблонного контекста безопасности (Context Template Security)
		CTL  = 0x02  // Уровень транспортной передачи данных (Context Transfer Layer)
	};

	/**
	 * @brief Структура буферов BIO
	 *
	 */
	typedef struct BufferIO {
		BIO * read;  // Объект буфера BIO для чтения
		BIO * write; // Объект буфера BIO для записи
		/**
		 * @brief Конструктор
		 *
		 */
		explicit BufferIO() noexcept :
		 read(nullptr), write(nullptr) {}
	} bio_t;

	/**
	 * @brief Структура cookie SSL
	 *
	 */
	typedef struct Cookie {
		// Буфер секретного слова cookie
		uint8_t buffer[16];
		// Флаг инициализации cookie SSL
		atomic_bool initialized;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Cookie() noexcept :
		 buffer{0}, initialized(false) {}
	} cookie_t;

	/**
	 * @brief Структура хоста
	 *
	 */
	typedef struct Host {
		// Имя хоста
		string name;
		// Параметры однорангового узла
		unique_ptr <net::attr_net_t> peer;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Host() noexcept :
		 name{""}, peer(nullptr) {}
	} host_t;

	/**
	 * @brief Структура ALPN-протоколов
	 *
	 */
	typedef struct ALPN {
		// Идентификатор выбранного ALPN-протокола
		uint8_t id;
		// Список идентификаторов поддерживаемых ALPN-протоколов
		vector <uint8_t> ids;
		// Буфер ALPN-протоколов
		vector <uint8_t> buffer;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit ALPN() noexcept : id(0) {}
	} alpn_t;

	/**
	 * @brief Структура обратных вызовов
	 *
	 */
	typedef struct Callback {
		// Функция обратного вызова получения ошибок
		::tls::coder_t::error_callback_t error;
		// Функция обратного вызова при изменении состояния
		::tls::coder_t::state_callback_t state;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Callback() noexcept :
		 error(nullptr), state(nullptr) {}
		/**
		 * @brief Деструктор
		 *
		 */
		virtual ~Callback() noexcept = default;
	} callback_t;

	/**
	 * @brief Структура обратных вызовов контроля передачи данных
	 *
	 */
	typedef struct Callback_Transfer : public callback_t {
		// Функция обратного вызова чтения данных
		::tls::coder_t::read_callback_t read;
		// Функция обратного вызова записи данных
		::tls::coder_t::write_callback_t write;
		// Функция обратного вызова получения снимка браузера приславшего ClientHello
		::tls::coder_t::fingerprint_callback_t fingerprint;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Callback_Transfer() noexcept :
		 read(nullptr), write(nullptr), fingerprint(nullptr) {}
	} callback_transfer_t;

	/**
	 * @brief Класс участника обмена защищёнными данными
	 *
	 */
	typedef class Member {
		public:
			// Уровень транспортной безопасности
			layer_t layer;
			// Объект состояния
			uint8_t state;
			// Идентификатор цифрового отпечатка браузера
			tls::fgp_t::id_t fid;
			// Счётчик ссылок на событие
			atomic_uint16_t refs;
			// Тип узла события
			event::node_t node;
			// Тип протокола события
			event::protocol_t proto;
			// Итератор шаблона контекста безопасности
			members_t::iterator iterator;
		public:
			/**
			 * @brief Метод удаления участника обмена защищёнными данными
			 *
			 * @param members контейнер участников обмена защищёнными данными
			 *
			 */
			void erase(members_t & members) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Member(const layer_t layer) noexcept :
			 layer(layer), state(0), fid(0), refs(0),
			 node(event::node_t::NONE),
			 proto(event::protocol_t::NONE) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Member() noexcept = default;
	} member_t;
	/**
	 * @brief Метод удаления участника обмена защищёнными данными
	 *
	 * @param members контейнер участников обмена защищёнными данными
	 *
	 */
	void Member::erase(members_t & members) noexcept {
		// Удаляем участника из глобального реестра TLS
		::ssl::registry::drop(
			static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> ((* this->iterator).get())),
			members,
			this->iterator
		);
	}

	/**
	 * @brief Структура шаблона контекста безопасности
	 *
	 */
	typedef struct Contex_Template_Security : public member_t {
		SSL_CTX * ctx;                           // Объект SSL контекста
		X509_CRL * crl;                          // Объект CRL-файла сертификата
		string host;                             // Объект хоста сервера
		alpn_t alpn;                             // Объект ALPN-протоколов
		callback_t callback;                     // Функции обратных вызовов
		vector <uint8_t> ech;                    // ECHConfigList для клиентов / байты приватного HPKE ключа для серверов
		unordered_map <string, string> sessions; // Кэш билетов возобновления сессии по ключу сервера (SNI/адрес)
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Contex_Template_Security() noexcept :
		 member_t(layer_t::CTS),
		 ctx(nullptr), crl(nullptr), host{""} {}
		/**
		 * @brief Деструктор
		 *
		 */
		~Contex_Template_Security() noexcept {
			// Если CRL-файл сертификата уже создан
			if(this->crl != nullptr)
				// Выполняем освобождение памяти
				::X509_CRL_free(this->crl);
			// Если объект SSL контекста существует
			if(this->ctx != nullptr)
				// Удаляем объект SSL контекста
				::SSL_CTX_free(this->ctx);
		}
	} cts_t;

	/**
	 * @brief Структура транспортного уровня передачи
	 *
	 */
	typedef struct Content_Transfer_Layer : public member_t {
		SSL * ssl;                    // Объект SSL
		SSL_CTX * ctx;                // Объект шаблона контекста SSL
		X509_CRL ** crl;              // Объект CRL-файла сертификата
		bio_t bio;                    // Объект буферов BIO
		host_t host;                  // Объект хоста сервера
		alpn_t alpn;                  // Объект ALPN-протоколов
		cookie_t cookie;              // Объект cookie SSL
		callback_transfer_t callback; // Объект обратных вызовов
		vector <uint8_t> ech;         // ECHConfigList (копия из CTS при создании CTL)
		vector <uint8_t> hello;       // Буфер сборки TLS/DTLS record для apply()
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Content_Transfer_Layer() noexcept :
		 member_t(layer_t::CTL),
		 ssl(nullptr), ctx(nullptr), crl(nullptr) {}
		/**
		 * @brief Деструктор
		 *
		 */
		~Content_Transfer_Layer() noexcept {
			// Если объект SSL существует
			if(this->ssl != nullptr)
				// Удаляем объект SSL
				::SSL_free(this->ssl);
		}
	} ctl_t;
};

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace state {
	/**
	 * @brief Флаг проверки на мусорные данные подлежащие удалению
	 *
	 */
	static constexpr uint8_t GARBAGE_MODE = 0x01;
	/**
	 * @brief Флаг выполненного рукопожатия TLS
	 *
	 */
	static constexpr uint8_t HANDSHAKE_MODE = 0x02;
	/**
	 * @brief Флаг проверки режима безсостояния TLS
	 *
	 */
	static constexpr uint8_t STATELESS_MODE = 0x04;
	/**
	 * @brief Флаг работы в мультисертификатном режиме
	 *
	 */
	static constexpr uint8_t MULTICERT_MODE = 0x08;
	/**
	 * @brief Флаг проверки имени хоста сервера
	 *
	 */
	static constexpr uint8_t CERTIFICATE_VERIFY = 0x10;
};

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Структура охранника участника обмена защищёнными данными
	 *
	 */
	class Guard_Transport_Layer_Security {
		private:
			// Объект участника обмена защищёнными данными
			::member_t * _member;
		public:
			/**
			 * @brief Метод проверки статуса участника обмена как мусорного
			 *
			 * @return результат проверки
			 *
			 */
			bool garbage() const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param member объект участника обмена защищёнными данными
			 *
			 */
			explicit Guard_Transport_Layer_Security(::member_t * member) noexcept;
		public:
			/**
			 * @brief Запрещаем копирование объекта
			 *
			 */
			Guard_Transport_Layer_Security(const Guard_Transport_Layer_Security &) = delete;
			/**
			 * @brief Запрещаем присваивание объекта
			 *
			 */
			Guard_Transport_Layer_Security & operator = (const Guard_Transport_Layer_Security &) = delete;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Guard_Transport_Layer_Security() noexcept;
	};

	/**
	 * @brief Метод проверки статуса участника обмена как мусорного
	 *
	 * @return результат проверки
	 *
	 */
	bool Guard_Transport_Layer_Security::garbage() const noexcept {
		// Проверяем статус участника обмена
		return (
			(this->_member->state & ::state::GARBAGE_MODE) &&
			(this->_member->refs.load(std::memory_order_acquire) == 1)
		);
	}
	/**
	 * @brief Конструктор
	 *
	 * @param member объект участника обмена защищёнными данными
	 *
	 */
	Guard_Transport_Layer_Security::Guard_Transport_Layer_Security(::member_t * member) noexcept : _member(member) {
		/**
		 * Участник обмена отсутствует, если объект TLS создан не кодером: шаблон
		 * контекста безопасности допускает создание объектов TLS сторонним модулем,
		 * а функции обратного вызова устанавливаются на контекст и вызываются
		 * для любого созданного из него объекта
		 */
		if(this->_member == nullptr)
			// Выходим из конструктора - закреплять нечего
			return;
		// Увеличиваем счётчик ссылок участника обмена
		this->_member->refs.fetch_add(1, std::memory_order_relaxed);
	}
	/**
	 * @brief Деструктор
	 *
	 */
	Guard_Transport_Layer_Security::~Guard_Transport_Layer_Security() noexcept {
		// Если участник обмена отсутствует
		if(this->_member == nullptr)
			// Выходим из деструктора - закрепление не выполнялось
			return;
		// Уменьшаем счётчик ссылок участника обмена
		this->_member->refs.fetch_sub(1, std::memory_order_release);
		// Если счётчик ссылок участника обмена равен нулю и статус участника обмена установлен как мусорный
		if((this->_member->state & ::state::GARBAGE_MODE) &&
		   (this->_member->refs.load(std::memory_order_acquire) == 0))
			// Удаляем контекст TLS из контейнера уровней защищённых сокетов
			this->_member->erase(::__awh_ssl_members__);
	}
};

/**
 * @brief Инкапсулируем статические объекты в пространство имён временных переменных
 *
 */
namespace local {
	/**
	 * @brief Создаём новый тип данных принадлежащий локальному защитнику
	 *
	 */
	using guard_t = Guard_Transport_Layer_Security;

	/**
	 * @brief Буфер для компрессии/декомпрессии сертификатов TLS
	 *
	 */
	thread_local vector <uint8_t> certBuffer;

	/**
	 * @brief Буфер для обмена данными SSL
	 *
	 * @note Буфер thread_local: указатель передаётся в read_callback и
	 *       действителен только до возврата из callback. Callback обязан
	 *       синхронно скопировать данные; при реентрантном вызове буфер
	 *       может быть перезаписан.
	 *
	 */
	thread_local uint8_t buffer[AWH_MAX_SSL_BUFFER_SIZE];

	/**
	 * @brief Вычисляет полный размер TLS/DTLS record layer
	 *
	 * @param buffer буфер с данными record layer
	 * @param size   размер буфера в байтах
	 * @return       полный размер record layer или 0, если данных недостаточно
	 *
	 */
	static inline size_t tlsRecordLayerSize(const uint8_t * buffer, const size_t size) noexcept {
		// Если буфер пустой или слишком короткий
		if((buffer == nullptr) || (size < 3u))
			// Возвращаем 0 — размер определить нельзя
			return 0;
		// Получаем версию протокола из заголовка record layer
		const uint16_t version = static_cast <uint16_t> ((static_cast <uint16_t> (buffer[1]) << 8) | static_cast <uint16_t> (buffer[2]));
		// Если это DTLS record layer
		if((version == 0xFEFFu) || (version == 0xFEFDu)){
			// Если данных недостаточно для DTLS record header
			if(size < 13u)
				// Возвращаем 0 — размер определить нельзя
				return 0;
			// Получаем длину payload DTLS record layer
			const size_t payload = static_cast <size_t> ((static_cast <uint16_t> (buffer[11]) << 8) | static_cast <uint16_t> (buffer[12]));
			// Возвращаем полный размер DTLS record layer
			return (13u + payload);
		}
		// Если данных недостаточно для TLS record header
		if(size < 5u)
			// Возвращаем 0 — размер определить нельзя
			return 0;
		// Получаем длину payload TLS record layer
		const size_t payload = static_cast <size_t> ((static_cast <uint16_t> (buffer[3]) << 8) | static_cast <uint16_t> (buffer[4]));
		// Возвращаем полный размер TLS record layer
		return (5u + payload);
	}
};

/**
 * @brief Инкапсулируем функции защитника в пространство имён
 *
 */
namespace ssl {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Закрепление участника в глобальном реестре TLS
	 *
	 * @note Lock удерживается только внутри pin(). Дальнейшая работа/callbacks — без lock.
	 *
	 */
	namespace registry {
		/**
		 * @brief Закрепление участника в глобальном реестре TLS
		 *
		 */
		class pin_t {
			private:
				// Охранник участника обмена защищёнными данными
				::local::guard_t _guard;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param member объект участника обмена защищёнными данными
				 *
				 */
				explicit pin_t(::member_t * member) noexcept : _guard(member) {}
				/**
				 * @brief Конструктор копирования
				 *
				 */
				pin_t(const pin_t &) = delete;
				/**
				 * @brief Конструктор перемещения
				 *
				 */
				pin_t(pin_t &&) noexcept = delete;
		};

		/**
		 * @brief Метод закрепления участника в глобальном реестре TLS
		 *
		 * @param id идентификатор контекста TLS
		 * @return   объект закрепления или nullptr
		 *
		 */
		static unique_ptr <pin_t> pin(const ::tls::coder_t::id_t id) noexcept {
			// Эксклюзивная блокировка: find в ids и refs++ должны быть атомарны относительно drop()
			const global_lock_t lock = globalExclusive();
			// Если идентификатор контекста TLS не зарегистрирован
			if(::__awh_ssl_ids__.find(id) == ::__awh_ssl_ids__.end())
				// Возвращаем пустой результат
				return nullptr;
			// Получаем объект участника обмена защищёнными данными
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Если участник помечен на удаление
			if(member->state & ::state::GARBAGE_MODE)
				// Возвращаем пустой результат
				return nullptr;
			// Возвращаем объект закрепления участника
			return make_unique <pin_t> (member);
		}
	};
};

/**
 * @brief Инкапсулируем функции TLS в пространство имён
 *
 */
namespace ssl {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция формирования сообщения об ошибке
	 *
	 * @param id      идентификатор события
	 * @param message дополнительное сообщение
	 * @return        сформированное сообщение об ошибке
	 *
	 */
	static string error(const ::tls::coder_t::id_t id, string_view message = "") noexcept {
		// Переменная результата
		string result = "";
		/**
		 * Определяем уровень транспортной безопасности
		 */
		switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
			// Если уровень является шаблонным контекстом безопасности
			case static_cast <uint8_t> (layer_t::CTS):
				// Записываем в лог сообщение об ошибка как оно передано
				return string{message};
			// Если уровень является транспортной передачей данных
			case static_cast <uint8_t> (layer_t::CTL): {
				// Если в очереди OpenSSL нет ошибок — возвращаем только переданное сообщение
				if((::ERR_peek_error() == 0) && !message.empty())
					// Записываем в лог сообщение об ошибка как оно передано
					return string{message};
				// Выполняем извлечение объекта транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				/**
				 * Выполняем перехват ошибок
				 */
				try {
					// Получаем данные описание ошибки
					uint64_t error = ::ERR_get_error();
					// Получаем объект фреймворка
					awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[1]));
					// Если ошибка получена
					if(error != 0){
						// Буфер данных для получения сообщения об ошибке
						char buffer[0xFF];
						// Получаем текст общего сообщения
						const string state = ::SSL_state_string(member->ssl);
						/**
						 * Выполняем извлечение остальных ошибок
						 */
						do {
							// Зануляем буфер данных
							::memset(buffer, 0, sizeof(buffer));
							// Получаем сообщение об ошибке
							::ERR_error_string_n(error, buffer, sizeof(buffer));
							// Если результат уже сформирован
							if(!result.empty())
								// Добавляем разделитель
								result.append("\n\n");
							// Если получено состояние SSL
							if(!state.empty())
								// Добавляем информацию об ошибке в результат
								result.append(fmk->format("%s: %s", state.c_str(), buffer));
							// Если получено дополнительное сообщение
							else if(!message.empty())
								// Добавляем информацию об ошибке в результат
								result.append(fmk->format("%s: %s", message.data(), buffer));
							// Если не получено ни состояние SSL, ни дополнительное сообщение
							else result.append(buffer);
						/**
						 * Если ещё есть ошибки
						 */
						} while((error = ::ERR_get_error()));
					// Если получено дополнительное сообщение
					} else if(!message.empty())
						// Записываем в лог сообщение об ошибка как оно передано
						return string{message};
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					// Получаем объект фреймворка
					awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[1]));
					// Если получено дополнительное сообщение
					if(!message.empty())
						// Добавляем информацию об ошибке в результат
						result.append(fmk->format("%s: %s", message.data(), error.what()));
					// Если не получено ни состояние SSL, ни дополнительное сообщение
					else result.append(error.what());
				}
			} break;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция отправки данных из BIO буфера записи через callback
	 *
	 * @param member объект транспортного уровня передачи
	 * @param id     идентификатор события
	 * @return       результат выполнения отправки
	 *
	 */
	static bool emitWriteBio(::ctl_t * member, const ::tls::coder_t::id_t id) noexcept {
		// Количество прочитанных данных
		int32_t bytes = 0;
		// Количество ожидающих данных для чтения
		size_t pending = 0;
		/**
		 * Читаем все ожидающие данные из BIO буфера записи
		 */
		while((pending = ::BIO_ctrl_pending(member->bio.write)) > 0){
			// Читаем данные из BIO буфера записи
			bytes = ::BIO_read(member->bio.write, ::local::buffer, static_cast <size_t> (::min(pending, static_cast <size_t> (AWH_MAX_SSL_BUFFER_SIZE))));
			// Если данные не прочитаны (SSL_get_error здесь неприменим — это BIO, не SSL)
			if(bytes <= 0)
				// Выходим из цикла
				return false;
			// Если функция обратного вызова чтения данных установлена
			if(member->callback.read != nullptr)
				// Вызываем функцию обратного вызова чтения данных
				member->callback.read(id, ::tls::coder_t::event_t::ENCRYPTION, ::local::buffer, static_cast <size_t> (bytes));
		}
		// Возвращаем результат
		return true;
	}
	/**
	 * @brief Функция обратного вызова сообщений SSL
	 *
	 * @param write  флаг записи сообщения
	 * @param version версия протокола
	 * @param type    тип сообщения
	 * @param buffer  буфер сообщения
	 * @param size    размер буфера сообщения
	 * @param ssl     объект SSL
	 * @param ctx     передаваемый контекст
	 *
	 */
	static void message(int32_t write, [[maybe_unused]] int32_t version, int32_t type, const void * buffer, size_t size, SSL * ssl, [[maybe_unused]] void * ctx) noexcept {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Если тип сообщения является рукопожатием SSL
			if(type == SSL3_RT_HANDSHAKE){
				// Получаем объект контекста модуля
				auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
				/**
				 * Объект транспортного уровня отсутствует, если объект TLS-соединения
				 * создан не кодером, а внешним потребителем шаблона контекста
				 * безопасности: функция обратного вызова устанавливается на контекст
				 * и вызывается для любого созданного из него соединения
				 */
				if(member == nullptr)
					// Выходим из функции - отладочный вывод для такого соединения не ведётся
					return;
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				/**
				 * Обрабатываем тип сообщения рукопожатия SSL
				 */
				switch(reinterpret_cast <const uint8_t *> (buffer)[0]){
					// Если сообщение является ClientHello
					case SSL3_MT_CLIENT_HELLO: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "ClientHello", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "ClientHello", size);
							break;
						}
					} break;
					// Если сообщение является ServerHello
					case SSL3_MT_SERVER_HELLO: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "ServerHello", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "ServerHello", size);
							break;
						}
					} break;
					// Если сообщение является Certificate
					case SSL3_MT_CERTIFICATE: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "Certificate", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "Certificate", size);
							break;
						}
					} break;
					// Если сообщение является HelloRequest
					case SSL3_MT_HELLO_REQUEST: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "HelloRequest", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "HelloRequest", size);
							break;
						}
					} break;
					// Если сообщение является NewSessionTicket
					case SSL3_MT_NEWSESSION_TICKET: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "NewSessionTicket", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "NewSessionTicket", size);
							break;
						}
					} break;
					// Если сообщение является EndOfEarlyData
					case SSL3_MT_END_OF_EARLY_DATA: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "EndOfEarlyData", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "EndOfEarlyData", size);
							break;
						}
					} break;
					// Если сообщение является EncryptedExtensions
					case SSL3_MT_ENCRYPTED_EXTENSIONS: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "EncryptedExtensions", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "EncryptedExtensions", size);
							break;
						}
					} break;
					// Если сообщение является ServerKeyExchange
					case SSL3_MT_SERVER_KEY_EXCHANGE: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "ServerKeyExchange", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "ServerKeyExchange", size);
							break;
						}
					} break;
					// Если сообщение является CertificateRequest
					case SSL3_MT_CERTIFICATE_REQUEST: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "CertificateRequest", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "CertificateRequest", size);
							break;
						}
					} break;
					// Если сообщение является ServerHelloDone
					case SSL3_MT_SERVER_DONE: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "ServerHelloDone", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "ServerHelloDone", size);
							break;
						}
					} break;
					// Если сообщение является CertificateVerify
					case SSL3_MT_CERTIFICATE_VERIFY: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "CertificateVerify", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "CertificateVerify", size);
							break;
						}
					} break;
					// Если сообщение является ClientKeyExchange
					case SSL3_MT_CLIENT_KEY_EXCHANGE: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "ClientKeyExchange", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "ClientKeyExchange", size);
							break;
						}
					} break;
					// Если сообщение является CertificateStatus
					case SSL3_MT_CERTIFICATE_STATUS: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "CertificateStatus", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "CertificateStatus", size);
							break;
						}
					} break;
					// Если сообщение является SupplementalData
					case SSL3_MT_SUPPLEMENTAL_DATA: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "SupplementalData", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "SupplementalData", size);
							break;
						}
					} break;
					// Если сообщение является KeyUpdate
					case SSL3_MT_KEY_UPDATE: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "KeyUpdate", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "KeyUpdate", size);
							break;
						}
					} break;
					// Если сообщение является CompressedCertificate
					case SSL3_MT_COMPRESSED_CERTIFICATE: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "CompressedCertificate", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "CompressedCertificate", size);
							break;
						}
					} break;
					// Если сообщение является HelloVerifyRequest
					case DTLS1_MT_HELLO_VERIFY_REQUEST: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "HelloVerifyRequest", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "HelloVerifyRequest", size);
							break;
						}
					} break;
					// Если сообщение является MessageHash
					case SSL3_MT_MESSAGE_HASH: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "MessageHash", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "MessageHash", size);
							break;
						}
					} break;
					// Если сообщение является Finished
					case SSL3_MT_FINISHED: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "Finished", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "Finished", size);
							break;
						}
					} break;
					/**
					 * Если сообщение является NextProto, то это сообщение используется для обмена информацией о поддерживаемых протоколах между клиентом и сервером в процессе рукопожатия SSL/TLS.
					 * Оно позволяет сторонам согласовать, какой протокол будет использоваться для дальнейшей коммуникации после завершения рукопожатия.
					 */
					case SSL3_MT_NEXT_PROTO: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "NextProto", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "NextProto", size);
							break;
						}
					} break;
					// Если сообщение является иным типом рукопожатия SSL
					default: {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? ">>>" : "<<<"), "Handshake", size);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Записываем ошибку в лог
								log->print("%s %s (%zu bytes)", log_t::flag_t::INFO, (write ? "<<<" : ">>>"), "Handshake", size);
							break;
						}
					}
				}
			}
		#endif
	}
	/**
	 * @brief Функция выполнения выбора протокола
	 *
	 * @param out     буфер назначения
	 * @param outSize размер буфера назначения
	 * @param in      буфер входящих данных
	 * @param inSize  размер буфера входящих данных
	 * @param key     ключ копирования
	 * @param keySize размер ключа для копирования
	 * @return        результат переключения протокола
	 *
	 */
	static bool selectProto(uint8_t ** out, uint8_t * outSize, const uint8_t * in, const uint8_t inSize, const uint8_t * key, const uint8_t keySize) noexcept {
		// Переменная результата
		bool result = false;
		/**
		 * Выполняем перебор всех данных в входящем буфере
		 */
		for(uint8_t i = 0; (i + keySize) <= inSize; i += static_cast <uint8_t> (in[i] + 1)){
			// Если данные ключа скопированны удачно
			if((result = (::memcmp(&in[i], key, keySize) == 0))){
				// Выполняем установку размеров исходящего буфера
				(* outSize) = in[i];
				// Выполняем установку полученных данных в исходящий буфер
				(* out) = const_cast <uint8_t *> (&in[i + 1]);
				// Выходим из функции
				break;
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция обратного вызова сервера для переключения на следующий протокол
	 *
	 * @param ssl  объект SSL
	 * @param data данные буфера данных протокола
	 * @param len  размер буфера данных протокола
	 * @param ctx  передаваемый контекст
	 * @return     результат переключения протокола
	 *
	 */
	static int32_t nextProto(SSL * ssl, const uint8_t ** data, uint32_t * len, [[maybe_unused]] void * ctx) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (ctx != nullptr)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			/**
			 * Объект транспортного уровня отсутствует, если объект TLS создан
			 * не кодером: шаблон контекста безопасности допускает создание
			 * объектов TLS сторонним модулем, а функции обратного вызова
			 * устанавливаются на контекст и вызываются для любого созданного
			 * из него объекта. Список протоколов хранится в состоянии
			 * транспортного уровня, поэтому согласование не выполняется
			 */
			if(member == nullptr)
				// Выводим результат отказа от обработки расширения
				return SSL_TLSEXT_ERR_NOACK;
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Выполняем установку буфера данных
			(* data) = &member->alpn.buffer[0];
			// Выполняем установку размер буфера данных протокола
			(* len) = static_cast <uint32_t> (member->alpn.buffer.size());
			// Возвращаем результат
			return SSL_TLSEXT_ERR_OK;
		}
		// Возвращаем результат
		return SSL_TLSEXT_ERR_NOACK;
	}
	/**
	 * @brief Функция обратного вызова клиента для расширения ALPN TLS
	 *
	 * @param ssl     объект SSL
	 * @param out     буфер исходящего протокола
	 * @param outSize размер буфера исходящего протокола
	 * @param in      буфер входящего протокола
	 * @param inSize  размер буфера входящего протокола
	 * @param ctx     передаваемый контекст
	 * @return        результат выбора протокола
	 *
	 */
	static int32_t clientNextProtoSelect(SSL * ssl, uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, [[maybe_unused]] void * ctx) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (ctx != nullptr)){
			// Размер и индекс протокола
			uint8_t size = 0, index = 0;
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			/**
			 * Объект транспортного уровня отсутствует, если объект TLS создан
			 * не кодером: шаблон контекста безопасности допускает создание
			 * объектов TLS сторонним модулем, а функции обратного вызова
			 * устанавливаются на контекст и вызываются для любого созданного
			 * из него объекта. Список протоколов хранится в состоянии
			 * транспортного уровня, поэтому согласование не выполняется
			 */
			if(member == nullptr)
				// Выводим результат отказа от обработки расширения
				return SSL_TLSEXT_ERR_NOACK;
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			/**
			 * Выполняем перебор всех поддерживаемых протоколов
			 */
			for(uint8_t i = 0; i < member->alpn.buffer.size(); i++){
				// Получаем размер протокола
				size = member->alpn.buffer[i];
				// Выполняем выбор протокола из входящего буфера
				if(::ssl::selectProto(out, outSize, in, static_cast <uint8_t> (inSize), &member->alpn.buffer[i], size + 1)){
					// Выполняем переключение на выбранный протокол
					member->alpn.id = member->alpn.ids[index];
					// Возвращаем результат
					return SSL_TLSEXT_ERR_OK;
				}
				// Переходим к следующему протоколу
				i += size;
				// Увеличиваем индекс протокола
				index++;
			}
		}
		// Возвращаем результат
		return SSL_TLSEXT_ERR_NOACK;
	}
	/**
	 * @brief Функция обратного вызова сервера для расширения ALPN TLS
	 *
	 * @param ssl     объект SSL
	 * @param out     буфер исходящего протокола
	 * @param outSize размер буфера исходящего протокола
	 * @param in      буфер входящего протокола
	 * @param inSize  размер буфера входящего протокола
	 * @param ctx     передаваемый контекст
	 * @return        результат выбора протокола
	 *
	 */
	static int32_t serverNextProtoSelect(SSL * ssl, const uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, [[maybe_unused]] void * ctx) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (ctx != nullptr)){
			// Размер и индекс протокола
			uint8_t size = 0, index = 0;
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Получаем объект шаблона контекста безопасности
			auto context = reinterpret_cast <::cts_t *> (ctx);
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			/**
			 * Объект транспортного уровня отсутствует, если объект TLS создан
			 * не кодером: шаблон контекста безопасности допускает создание
			 * объектов TLS сторонним модулем, а функция обратного вызова
			 * устанавливается на контекст и вызывается для любого созданного
			 * из него объекта. Список протоколов в этом случае берётся из
			 * самого шаблона контекста, на котором функция и установлена
			 */
			auto & alpn = ((member != nullptr) ? member->alpn : context->alpn);
			/**
			 * Выполняем перебор всех поддерживаемых протоколов
			 */
			for(uint8_t i = 0; i < alpn.buffer.size(); i++){
				// Получаем размер протокола
				size = alpn.buffer[i];
				// Выполняем выбор протокола из входящего буфера
				if(::ssl::selectProto(const_cast <uint8_t **> (out), outSize, in, static_cast <uint8_t> (inSize), &alpn.buffer[i], size + 1)){
					/**
					 * Запоминаем выбранный протокол только в состоянии транспортного
					 * уровня: шаблон контекста общий для всех соединений, и запись
					 * результата согласования в него исказила бы соседние
					 */
					if(member != nullptr)
						// Выполняем переключение на выбранный протокол
						member->alpn.id = member->alpn.ids[index];
					// Возвращаем результат
					return SSL_TLSEXT_ERR_OK;
				}
				// Переходим к следующему протоколу
				i += size;
				// Увеличиваем индекс протокола
				index++;
			}
		}
		// Возвращаем результат
		return SSL_TLSEXT_ERR_NOACK;
	}
	/**
	 * @brief Функция установки сертификата и цепочки сертификатов из PEM-файла для BoringSSL
	 *
	 * @param ssl      объект SSL
	 * @param filename путь к PEM-файлу
	 * @return         результат выполнения функции (1 - успех, 0 - ошибка)
	 *
	 */
	static int32_t useCertificateChainFile(SSL * ssl, const char * filename) noexcept {
		// Переменная результата
		int32_t result = 0;
		// Открываем PEM-файл через BIO
		BIO * bio = ::BIO_new_file(filename, "r");
		// Если файл открыт успешно
		if(bio != nullptr){
			// Читаем листовой (leaf) сертификат
			X509 * cert = ::PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
			// Если листовой сертификат получен
			if(cert != nullptr){
				// Устанавливаем листовой сертификат в объект SSL*
				result = ::SSL_use_certificate(ssl, cert);
				// Освобождаем ресурсы листового сертификата
				::X509_free(cert);
				// Если листовой сертификат установлен успешно
				if(result == 1){
					// Статус добавления промежуточного сертификата в цепочку SSL*
					int32_t status = 0;
					/**
					 * Читаем промежуточные сертификаты цепочки из PEM-файла и добавляем их в объект SSL*.
					 * PEM-файл должен быть структурирован так, что листовой сертификат идёт первым,
					 * а затем следуют промежуточные сертификаты в порядке их расположения в цепочке.
					 */
					while((cert = ::PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) != nullptr){
						// Добавляем промежуточный сертификат в цепочку SSL*
						status = ::SSL_add1_chain_cert(ssl, cert);
						// Освобождаем ресурсы промежуточного сертификата
						::X509_free(cert);
						// Если добавление промежуточного сертификата не удалось
						if(status != 1){
							// Фиксируем ошибку и выходим из цикла
							result = 0;
							// Выходим из цикла
							break;
						}
					}
					// PEM_read_bio_X509 в конце файла ставит ошибку "нет данных" — сбрасываем её
					::ERR_clear_error();
				}
			}
			// Закрываем BIO
			::BIO_free(bio);
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * @brief Функция проверки параметров сертификата
		 *
		 * @param store магазин с сертификатами для работы
		 * @param name  название параметра сертификата
		 * @param log   объект для работы с логами
		 * @return      результат проверки
		 *
		 */
		static bool addCertToStore(X509_STORE * store, string_view name, const awh::log_t * log) noexcept {
			// Переменная результата
			bool result = false;
			// Если объекты переданы верно
			if((store != nullptr) && !name.empty()){
				// Получаем данные системного стора
				HCERTSTORE sys = ::CertOpenSystemStore(0, name.data());
				// Если системный стор не получен
				if(!(result = (sys != nullptr))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, "Failed to open system certificate store");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", log_t::flag_t::CRITICAL, "Failed to open system certificate store");
					#endif
					// Возвращаем результат
					return result;
				}
				// Контекст сертификата
				PCCERT_CONTEXT ctx = nullptr;
				/**
				 * Перебираем все сертификаты в системном сторе
				 */
				while((ctx = ::CertEnumCertificatesInStore(sys, ctx))){
					// Выполняем создание сертификата
					X509 * x509 = X509_new();
					// Если сертификат создан удачно
					if((result = (x509 != nullptr))){
						// Получаем объект закодированного сертификата
						const BYTE * encoded = ctx->pbCertEncoded;
						// Добавляем сертификат в стор
						::X509_STORE_add_cert(store, ::d2i_X509(&x509, reinterpret_cast <const uint8_t **> (&encoded), ctx->cbCertEncoded));
						// Очищаем выделенную память
						::X509_free(x509);
					// Если сертификат не создан
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, "X509 creation failed");
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("%s", log_t::flag_t::CRITICAL, "X509 creation failed");
						#endif
						// Выходим из цикла
						break;
					}
				}
				// Закрываем системный стор
				::CertCloseStore(sys, 0);
			}
			// Возвращаем результат
			return result;
		}
	#endif
};

/**
 * @brief Инкапсулируем функции компрессии/декомпрессии в пространство имён compressor
 *
 */
namespace compressor {
	/**
	 * @brief Функция обратного вызова для компрессии данных методом Zlib
	 *
	 * @param ssl  объект SSL
	 * @param out  буфер для компрессированных данных
	 * @param in   входные данные для компрессии
	 * @param size размер входных данных
	 * @return     результат выполнения функции
	 *
	 */
	static int32_t compressionZlib(SSL * ssl, CBB * out, const uint8_t * in, const size_t size) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (out != nullptr) && (in != nullptr) && (size > 0)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Получаем объект компрессора из контекста SSL
			awh::compressor::block_t * compressor = reinterpret_cast <awh::compressor::block_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[3]));
			// Буфер для хранения данных после компрессии
			::local::certBuffer.clear();
			// Выполняем компрессию данных
			compressor->compress(in, size, awh::compressor::method_t::ZLIB, ::local::certBuffer);
			// Если буфер не пустой
			if(!::local::certBuffer.empty()){
				// Выполняем копирование данных в выходной буфер
				if(::CBB_add_bytes(out, &::local::certBuffer[0], ::local::certBuffer.size()))
					// Возвращаем положительный результат
					return 1;
			}
		}
		// Возвращаем отрицательный результат
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для компрессии данных методом Brotli
	 *
	 * @param ssl  объект SSL
	 * @param out  буфер для компрессированных данных
	 * @param in   входные данные для компрессии
	 * @param size размер входных данных
	 * @return     результат выполнения функции
	 *
	 */
	static int32_t compressionBrotli(SSL * ssl, CBB * out, const uint8_t * in, const size_t size) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (out != nullptr) && (in != nullptr) && (size > 0)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Получаем объект компрессора из контекста SSL
			awh::compressor::block_t * compressor = reinterpret_cast <awh::compressor::block_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[3]));
			// Буфер для хранения данных после компрессии
			::local::certBuffer.clear();
			// Выполняем компрессию данных
			compressor->compress(in, size, awh::compressor::method_t::BROTLI, ::local::certBuffer);
			// Если буфер не пустой
			if(!::local::certBuffer.empty()){
				// Выполняем копирование данных в выходной буфер
				if(::CBB_add_bytes(out, &::local::certBuffer[0], ::local::certBuffer.size()))
					// Возвращаем положительный результат
					return 1;
			}
		}
		// Возвращаем отрицательный результат
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для компрессии данных методом ZSTD (Zstandard)
	 *
	 * @param ssl  объект SSL
	 * @param out  буфер для компрессированных данных
	 * @param in   входные данные для компрессии
	 * @param size размер входных данных
	 * @return     результат выполнения функции
	 *
	 */
	static int32_t compressionZstandard(SSL * ssl, CBB * out, const uint8_t * in, const size_t size) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (out != nullptr) && (in != nullptr) && (size > 0)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Получаем объект компрессора из контекста SSL
			awh::compressor::block_t * compressor = reinterpret_cast <awh::compressor::block_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[3]));
			// Буфер для хранения данных после компрессии
			::local::certBuffer.clear();
			// Выполняем компрессию данных
			compressor->compress(in, size, awh::compressor::method_t::ZSTD, ::local::certBuffer);
			// Если буфер не пустой
			if(!::local::certBuffer.empty()){
				// Выполняем копирование данных в выходной буфер
				if(::CBB_add_bytes(out, &::local::certBuffer[0], ::local::certBuffer.size()))
					// Возвращаем положительный результат
					return 1;
			}
		}
		// Возвращаем отрицательный результат
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для декомпрессии данных методом Zlib
	 *
	 * @param ssl    объект SSL
	 * @param out    буфер для декомпрессированных данных
	 * @param length размер буфера декомпрессированных данных
	 * @param in     входные данные для декомпрессии
	 * @param size   размер входных данных
	 * @return       результат выполнения функции
	 *
	 */
	static int32_t decompressionZlib(SSL * ssl, CRYPTO_BUFFER ** out, const size_t length, const uint8_t * in, const size_t size) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (out != nullptr) && (in != nullptr) && (size > 0)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Получаем объект компрессора из контекста SSL
			awh::compressor::block_t * compressor = reinterpret_cast <awh::compressor::block_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[3]));
			// Буфер для хранения данных после декомпрессии
			::local::certBuffer.clear();
			// Выполняем декомпрессию данных
			compressor->decompress(in, size, awh::compressor::method_t::ZLIB, ::local::certBuffer);
			// Если размер декомпрессированных данных не соответствует ожидаемому
			if(::local::certBuffer.size() != length)
				// Возвращаем отрицательный результат
				return 0;
			// Выполняем создание объекта CRYPTO_BUFFER из входящих данных
			(* out) = ::CRYPTO_BUFFER_new(&::local::certBuffer[0], ::local::certBuffer.size(), nullptr);
			// Если объект CRYPTO_BUFFER создан успешно
			if((* out) != nullptr)
				// Возвращаем положительный результат
				return 1;
		}
		// Возвращаем отрицательный результат
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для декомпрессии данных методом Brotli
	 *
	 * @param ssl    объект SSL
	 * @param out    буфер для декомпрессированных данных
	 * @param length размер буфера декомпрессированных данных
	 * @param in     входные данные для декомпрессии
	 * @param size   размер входных данных
	 * @return       результат выполнения функции
	 *
	 */
	static int32_t decompressionBrotli(SSL * ssl, CRYPTO_BUFFER ** out, const size_t length, const uint8_t * in, const size_t size) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (out != nullptr) && (in != nullptr) && (size > 0)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Получаем объект компрессора из контекста SSL
			awh::compressor::block_t * compressor = reinterpret_cast <awh::compressor::block_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[3]));
			// Буфер для хранения данных после декомпрессии
			::local::certBuffer.clear();
			// Выполняем декомпрессию данных
			compressor->decompress(in, size, awh::compressor::method_t::BROTLI, ::local::certBuffer);
			// Если размер декомпрессированных данных не соответствует ожидаемому
			if(::local::certBuffer.size() != length)
				// Возвращаем отрицательный результат
				return 0;
			// Выполняем создание объекта CRYPTO_BUFFER из входящих данных
			(* out) = ::CRYPTO_BUFFER_new(&::local::certBuffer[0], ::local::certBuffer.size(), nullptr);
			// Если объект CRYPTO_BUFFER создан успешно
			if((* out) != nullptr)
				// Возвращаем положительный результат
				return 1;
		}
		// Возвращаем отрицательный результат
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для декомпрессии данных методом ZSTD (Zstandard)
	 *
	 * @param ssl    объект SSL
	 * @param out    буфер для декомпрессированных данных
	 * @param length размер буфера декомпрессированных данных
	 * @param in     входные данные для декомпрессии
	 * @param size   размер входных данных
	 * @return       результат выполнения функции
	 *
	 */
	static int32_t decompressionZstandard(SSL * ssl, CRYPTO_BUFFER ** out, const size_t length, const uint8_t * in, const size_t size) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (out != nullptr) && (in != nullptr) && (size > 0)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Получаем объект компрессора из контекста SSL
			awh::compressor::block_t * compressor = reinterpret_cast <awh::compressor::block_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[3]));
			// Буфер для хранения данных после декомпрессии
			::local::certBuffer.clear();
			// Выполняем декомпрессию данных
			compressor->decompress(in, size, awh::compressor::method_t::ZSTD, ::local::certBuffer);
			// Если размер декомпрессированных данных не соответствует ожидаемому
			if(::local::certBuffer.size() != length)
				// Возвращаем отрицательный результат
				return 0;
			// Выполняем создание объекта CRYPTO_BUFFER из входящих данных
			(* out) = ::CRYPTO_BUFFER_new(&::local::certBuffer[0], ::local::certBuffer.size(), nullptr);
			// Если объект CRYPTO_BUFFER создан успешно
			if((* out) != nullptr)
				// Возвращаем положительный результат
				return 1;
		}
		// Возвращаем отрицательный результат
		return 0;
	}
};

/**
 * @brief Инкапсулируем функции работы с cookie в пространство имён
 *
 */
namespace cookie {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция проверки наличия адреса однорангового узла для DTLS cookie
	 *
	 * @param ssl    объект SSL
	 * @param member объект транспортного уровня передачи
	 * @return       результат проверки
	 *
	 */
	static int32_t requirePeer(SSL * ssl, ::ctl_t * member) noexcept {
		// Если адрес однорангового узла установлен
		if((member->host.peer != nullptr) && (member->host.peer->ip != nullptr))
			// Выходим из функции с удачей
			return 1;
		// Выполняем получение идентификатора контекста TLS
		const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
		// Если функция обратного вызова состояния установлена
		if(member->callback.state != nullptr)
			// Вызываем функцию обратного вызова состояния
			member->callback.state(id, ::tls::coder_t::state_t::FAILED);
		// Получаем текст ошибки
		const string error = ::ssl::error(id, "Peer address is not set for DTLS cookie");
		// Если функция обратного вызова ошибки установлена
		if(member->callback.error != nullptr)
			// Вызываем функцию обратного вызова ошибки
			member->callback.error(id, ::tls::coder_t::error_t::COOKIE_FAILED, error);
		// Если функция обратного вызова ошибки не установлена
		else {
			// Получаем объект логирования
			log_t * log = reinterpret_cast <log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
			#endif
		}
		// Выходим из функции с неудачей
		return 0;
	}

	/**
	 * @brief Функция обратного вызова для генерации куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 *
	 */
	static int32_t generate(SSL * ssl, uint8_t * cookie, uint32_t * size) noexcept {
		// Получаем объект уровня защищённых сокетов
		auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
		// Создаём охранника участника обмена защищёнными данными
		::local::guard_t guard(member);
		// Если cookie еще не проинициализированы
		if(!member->cookie.initialized){
			// Выполняем произвольно генерацию байт в буфере cookie
			if(!(member->cookie.initialized = ::RAND_bytes(member->cookie.buffer, sizeof(member->cookie.buffer)))){
				// Выполняем получение идентификатора контекста TLS
				const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
				// Если функция обратного вызова состояния установлена
				if(member->callback.state != nullptr)
					// Вызываем функцию обратного вызова состояния
					member->callback.state(id, ::tls::coder_t::state_t::FAILED);
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "Setting random cookie secret");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, ::tls::coder_t::error_t::COOKIE_FAILED, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					// Получаем объект логирования
					log_t * log = reinterpret_cast <log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
				}
				// Выходим и сообщаем, что генерация куков не удалась
				return 0;
			}
		}
		// Если адрес однорангового узла не установлен
		if(::cookie::requirePeer(ssl, member) != 1)
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		// Получаем объект хоста IPv4-адреса
		net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
		// Размер буфера и длина сгенерированных cookie
		uint32_t bytes = (address->ip->size + 2), length = 0;
		// Выполняем выделение память для буфера данных
		uint8_t * buffer = reinterpret_cast <uint8_t *> (::OPENSSL_malloc(bytes));
		// Если память для буфера данных не выделена
		if(buffer == nullptr){
			// Выполняем получение идентификатора контекста TLS
			const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
			// Если функция обратного вызова состояния установлена
			if(member->callback.state != nullptr)
				// Вызываем функцию обратного вызова состояния
				member->callback.state(id, ::tls::coder_t::state_t::FAILED);
			// Получаем текст ошибки
			const string error = ::ssl::error(id, "Out of memory cookie");
			// Если функция обратного вызова ошибки установлена
			if(member->callback.error != nullptr)
				// Вызываем функцию обратного вызова ошибки
				member->callback.error(id, ::tls::coder_t::error_t::COOKIE_FAILED, error);
			// Если функция обратного вызова ошибки не установлена
			else {
				// Получаем объект логирования
				log_t * log = reinterpret_cast <log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в log
					log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
				#endif
			}
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		}
		// Выполняем чтение в буфер данных данные порта
		::memcpy(buffer, &address->port, 2);
		/**
		 * Определяем тип адреса
		 */
		switch(address->ip->size){
			// Если адрес является IPv4
			case 4:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv4_t *> (address->ip.get())->address, 4);
			break;
			// Если адрес является IPv6
			case 16:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address[0], 16);
			break;
			// Если производится работа с другими протоколами, выходим
			default:
				// Очищаем ранее выделенную память
				::OPENSSL_free(buffer);
				// Выходим и сообщаем, что генерация куков не удалась
				return 0;
		}
		// Буфер под генерацию cookie
		uint8_t result[EVP_MAX_MD_SIZE];
		// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
		::HMAC(::EVP_sha1(), reinterpret_cast <void *> (member->cookie.buffer), sizeof(member->cookie.buffer), buffer, bytes, result, &length);
		// Очищаем ранее выделенную память
		::OPENSSL_free(buffer);
		// Выполняем копирование полученного результата в буфер cookie
		::memcpy(cookie, result, length);
		// Устанавливаем размер буфера cookie
		(* size) = length;
		// Возвращаем положительный результат
		return 1;
	}
	/**
	 * @brief Функция обратного вызова для проверки куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 *
	 */
	static int32_t verify(SSL * ssl, const uint8_t * cookie, uint32_t size) noexcept {
		// Получаем объект уровня защищённых сокетов
		auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
		// Создаём охранника участника обмена защищёнными данными
		::local::guard_t guard(member);
		// Если cookie не проинициализированы, значит cookie не валидные
		if(!member->cookie.initialized)
			// Выходим из функции
			return 0;
		// Если адрес однорангового узла не установлен
		if(::cookie::requirePeer(ssl, member) != 1)
			// Выходим из функции с неудачей
			return 0;
		// Получаем объект хоста IPv4-адреса
		net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
		// Размер буфера и длина сгенерированных cookie
		uint32_t bytes = (address->ip->size + 2), length = 0;
		// Выполняем выделение память для буфера данных
		uint8_t * buffer = reinterpret_cast <uint8_t *> (::OPENSSL_malloc(bytes));
		// Если память для буфера данных не выделена
		if(buffer == nullptr){
			// Выполняем получение идентификатора контекста TLS
			const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
			// Если функция обратного вызова состояния установлена
			if(member->callback.state != nullptr)
				// Вызываем функцию обратного вызова состояния
				member->callback.state(id, ::tls::coder_t::state_t::FAILED);
			// Получаем текст ошибки
			const string error = ::ssl::error(id, "Out of memory cookie");
			// Если функция обратного вызова ошибки установлена
			if(member->callback.error != nullptr)
				// Вызываем функцию обратного вызова ошибки
				member->callback.error(id, ::tls::coder_t::error_t::COOKIE_FAILED, error);
			// Если функция обратного вызова ошибки не установлена
			else {
				// Получаем объект логирования
				log_t * log = reinterpret_cast <log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
				#endif
			}
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		}
		// Выполняем чтение в буфер данных данные порта
		::memcpy(buffer, &address->port, 2);
		/**
		 * Определяем тип адреса
		 */
		switch(address->ip->size){
			// Если адрес является IPv4
			case 4:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv4_t *> (address->ip.get())->address, 4);
			break;
			// Если адрес является IPv6
			case 16:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address[0], 16);
			break;
			// Если производится работа с другими протоколами, выходим
			default:
				// Очищаем ранее выделенную память
				::OPENSSL_free(buffer);
				// Выходим из функции с неудачей
				return 0;
		}
		// Буфер под генерацию cookie
		uint8_t result[EVP_MAX_MD_SIZE];
		// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
		::HMAC(::EVP_sha1(), reinterpret_cast <void *> (member->cookie.buffer), sizeof(member->cookie.buffer), buffer, bytes, result, &length);
		// Очищаем ранее выделенную память
		::OPENSSL_free(buffer);
		// Выполняем проверку cookie, если cookie совпадают, значит всё хорошо
		if((size == length) && (::memcmp(result, cookie, length) == 0))
			// Выходим из функции с удачей
			return 1;
		// Выходим из функции с неудачей
		return 0;
	}
};

/**
 * @brief Инкапсулируем функции проверки сертификата в пространство имён
 *
 */
namespace verify {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Статусы проверки сертификата
	 *
	 */
	enum class status_t : uint8_t {
		NONE                 = 0x00, // Не установлено
		Error                = 0x01, // Ошибка валидации
		MatchFound           = 0x02, // Валидация пройдена
		NoSANPresent         = 0x03, // Сеть не распознана
		MatchNotFound        = 0x04, // Валидация не пройдена
		MalformedCertificate = 0x05  // Неверный сертификат
	};

	/**
	 * @brief Функция проверки на эквивалентность доменных имен
	 *
	 * @param first  первое доменное имя
	 * @param second второе доменное имя
	 * @return       результат проверки
	 *
	 */
	static bool equal(string_view first, string_view second) noexcept {
		// Переменная результата
		bool result = false;
		// Если данные переданы
		if(!first.empty() && !second.empty())
			// Проверяем совпадение строки
			result = (first.compare(second) == 0);
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция проверки на эквивалентность доменных имен с пропуском начальных символов
	 *
	 * @param first  первое доменное имя
	 * @param second второе доменное имя
	 * @param max    количество начальных символов для проверки
	 * @return       результат проверки
	 *
	 */
	static bool noqual(string_view first, string_view second, size_t max) noexcept {
		// Переменная результата
		bool result = false;
		// Если данные переданы
		if(!first.empty() && !second.empty())
			// Проверяем совпадение строки
			result = (first.substr(max, first.length() - max).compare(second.substr(max, second.length() - max)) == 0);
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция проверки эквивалентности доменного имени с учетом шаблона
	 *
	 * @param host доменное имя
	 * @param fqdn шаблон доменного имени
	 * @return     результат проверки
	 *
	 */
	static bool hostmatch(string_view host, string_view fqdn) noexcept {
		// Переменная результата
		bool result = true;
		// Если данные переданы
		if(!host.empty() && !fqdn.empty()){
			// Позиция звездочки в шаблоне
			const size_t pos1 = fqdn.find('*');
			// Ищем звездочку в шаблоне не найдена
			if(pos1 == string::npos)
				// Выполняем проверку эквивалентности доменных имён
				return ::verify::equal(fqdn, host);
			// Определяем конец шаблона
			const size_t pos2 = fqdn.find('.');
			// Если это конец тогда запрещаем активацию шаблона
			if((pos2 == string::npos) || (pos1 > pos2) || ::verify::noqual(fqdn, "xn--", 4))
				// Выполняем проверку эквивалентности доменных имён
				return ::verify::equal(fqdn, host);
			// Выполняем поиск точки в название хоста
			const size_t pos3 = host.find('.');
			// Если хост не найден
			if((pos2 != string::npos) && (pos3 != string::npos)){
				// Если шаблон начинается со звёздочки, то проверка пройдена
				if(fqdn.front() == '*')
					// Выполняем проверку эквивалентности доменных имён без учёта начальных сабдоменов
					return ((pos3 > 0) && ::verify::equal(fqdn.substr(2, pos2 - 2), host.substr(pos3 + 1, host.size() - pos3 - 1)));
				// Выполняем сравнение
				if(!::verify::equal(fqdn.substr(0, pos2), host.substr(0, pos3)))
					// Выходим из функции
					return false;
			// Выходим из функции
			} else return false;
			// Если диапазоны точки в шаблоне и хосте отличаются тогда выходим
			if(pos3 < pos2)
				// Выходим из функции
				return false;
			// Вычисляем длину обрезаемой строки
			const size_t length = (pos2 - (pos1 + 1));
			// Проверяем эквивалент результата
			return (
				::verify::noqual(fqdn, host, pos1) &&
				::verify::noqual(fqdn.substr(pos1 + 1, length), host.substr(pos3 - length, length), length)
			);
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция обработки SNI от клиента
	 *
	 * @param ssl объект SSL
	 * @param al  указатель на код ошибки
	 * @param ctx контекст модуля
	 * @return    результат обработки
	 *
	 */
	static int32_t matchSNI(SSL * ssl, int32_t * al, [[maybe_unused]] void * ctx) noexcept {
		// Переменная результата
		int32_t result = SSL_TLSEXT_ERR_NOACK;
		// Получаем название хоста (SNI) от клиента
		const char * sni = ::SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
		// Получаем объект уровня защищённых сокетов
		auto member = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
		/**
		 * Объект транспортного уровня отсутствует, если объект TLS создан не кодером:
		 * сопоставление имени хоста с сертификатами опирается на его состояние,
		 * поэтому для стороннего объекта TLS не выполняется
		 */
		if(member == nullptr)
			// Выводим результат отказа от обработки расширения
			return SSL_TLSEXT_ERR_NOACK;
		// Создаём охранника участника обмена защищёнными данными
		::local::guard_t guard(member);
		// Если SNI получен
		if(sni != nullptr){
			// Сохраняем полученное имя хоста
			member->host.name = sni;
			// Если установлен режим работы с несколькими сертификатами TLS
			if(member->state & state::MULTICERT_MODE){
				// Выполняем поиск идентификатора контекста TLS в карте сплайса
				const ::tls::coder_t::id_t spliceId = ::ssl::registry::spliceResolve(member->proto, string{sni});
				// Если идентификатор контекста TLS найден
				if(spliceId != 0){
					// Выполняем подмену сертификата на основной
					::SSL_set_SSL_CTX(ssl, reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (spliceId))->ctx);
					// Устанавливаем результат обработки
					result = SSL_TLSEXT_ERR_OK;
				}
			// Устанавливаем результат обработки
			} else result = SSL_TLSEXT_ERR_OK;
		// Если SNI не получен
		} else {
			// Выполняем получение идентификатора контекста TLS
			const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
			// Если функция обратного вызова состояния установлена
			if(member->callback.state != nullptr)
				// Вызываем функцию обратного вызова состояния
				member->callback.state(id, ::tls::coder_t::state_t::FAILED);
			// Получаем текст ошибки
			const string error = ::ssl::error(id, "SNI is not set by client");
			// Если функция обратного вызова ошибки установлена
			if(member->callback.error != nullptr)
				// Вызываем функцию обратного вызова ошибки
				member->callback.error(id, ::tls::coder_t::error_t::SNI_FAILED, error);
			// Если функция обратного вызова ошибки не установлена
			else {
				// Получаем объект логирования
				log_t * log = reinterpret_cast <log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.c_str());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::WARNING, error.c_str());
				#endif
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени по шаблону
	 *
	 * @param host доменное имя
	 * @param fqdn шаблон доменного имени
	 * @return     результат проверки
	 *
	 */
	static bool certHostcheck(string_view host, string_view fqdn) noexcept {
		// Переменная результата
		bool result = false;
		// Если данные переданы
		if(!host.empty() && !fqdn.empty())
			// Проверяем эквивалентны ли домен и шаблон
			result = (::verify::equal(host, fqdn) || ::verify::hostmatch(host, fqdn));
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени по списку доменных имен из сертификата
	 *
	 * @param host доменное имя
	 * @param x509 сертификат
	 * @return     результат проверки
	 *
	 */
	static status_t matchSubjectName(string_view host, const X509 * x509) noexcept {
		// Переменная результата
		status_t result = status_t::MatchNotFound;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Извлекаем SAN из сертификата
			STACK_OF(GENERAL_NAME) * san = reinterpret_cast <STACK_OF(GENERAL_NAME) *> (::X509_get_ext_d2i(const_cast <X509 *> (x509), NID_subject_alt_name, nullptr, nullptr));
			// Если SAN присутствует
			if(san != nullptr){
				// Полученное доменное имя
				string fqdn = "";
				/**
				 * Проверяем каждый элемент SAN
				 */
				for(int32_t i = 0; i < sk_GENERAL_NAME_num(san); i++){
					// Извлекаем элемент SAN
					const GENERAL_NAME * cn = sk_GENERAL_NAME_value(san, i);
					// Проверяем тип имени
					if(cn->type == GEN_DNS){
						// Формируем строковое представление доменного имени
						fqdn.assign(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cn->d.dNSName))), ::ASN1_STRING_length(cn->d.dNSName));
						// Если размер имени и dns имя совпадает
						if(::verify::certHostcheck(host, fqdn)){
							// Запоминаем результат что домен найден
							result = status_t::MatchFound;
							// Выходим из цикла
							break;
						}
					}
				}
				// Очищаем список имен
				sk_GENERAL_NAME_pop_free(san, GENERAL_NAME_free);
			// Если SAN отсутствует или имя не совпало
			} else {
				// Буфер данных для получения данных
				char buffer[0xFF];
				// Fallback на Common Name (устаревшее, но иногда нужно)
				X509_NAME * subject = ::X509_get_subject_name(x509);
				// Если удалось получить Common Name
				if(::X509_NAME_get_text_by_NID(subject, NID_commonName, buffer, sizeof(buffer)) > 0){
					// Если размер имени и dns имя совпадает
					if(::verify::certHostcheck(host, buffer))
						// Запоминаем результат что домен найден
						result = status_t::MatchFound;
				}
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени по данным из сертификата
	 *
	 * @param host доменное имя
	 * @param x509 сертификат
	 * @return     результат проверки
	 *
	 */
	static status_t matchesCommonName(string_view host, const X509 * x509) noexcept {
		// Переменная результата
		status_t result = status_t::MatchNotFound;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Получаем индекс имени по "NID"
			const int32_t cnl = ::X509_NAME_get_index_by_NID(X509_get_subject_name(const_cast <X509 *> (x509)), NID_commonName, -1);
			// Если индекс не получен тогда выходим
			if(cnl < 0)
				// Формируем текст ошибки
				return status_t::Error;
			// Извлекаем поле "CN"
			X509_NAME_ENTRY * cne = ::X509_NAME_get_entry(X509_get_subject_name(const_cast <X509 *> (x509)), cnl);
			// Если поле не получено тогда выходим
			if(cne == nullptr)
				// Формируем текст ошибки
				return status_t::Error;
			// Конвертируем "CN" поле в "C" строку
			ASN1_STRING * cna = ::X509_NAME_ENTRY_get_data(cne);
			// Если строка не сконвертирована тогда выходим
			if(cna == nullptr)
				// Формируем текст ошибки
				return status_t::Error;
			// Выполняем рукопожатие
			if(::verify::certHostcheck(host, string(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cna))), ::ASN1_STRING_length(cna))))
				// Формируем текст ошибки
				return status_t::MatchFound;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени
	 *
	 * @param host доменное имя
	 * @param x509 сертификат
	 * @return     результат проверки
	 *
	 */
	static status_t validateHostname(string_view host, const X509 * x509) noexcept {
		// Переменная результата
		status_t result = status_t::Error;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Выполняем проверку имени хоста по списку доменов у сертификата
			result = ::verify::matchSubjectName(host, x509);
			// Если у сертификата только один домен
			if(result == status_t::NoSANPresent)
				// Выполняем проверку имени хоста по общему имени у сертификата
				result = ::verify::matchesCommonName(host, x509);
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция обратного вызова для проверки валидности сертификата
	 *
	 * @param ok    результат получения сертификата
	 * @param store хранилище сертификатов
	 * @return      результат проверки
	 *
	 */
	static int32_t certificate(const int32_t ok, X509_STORE_CTX * store) noexcept {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Если хранилище сертификатов передано верное
			if(store != nullptr){
				// Выполняем извлечение сертификата
				X509 * x509 = ::X509_STORE_CTX_get_current_cert(store);
				// Если сертификат получен
				if(x509 != nullptr){
					// Буфер данных для получения данных
					char buffer[0xFF];
					// Печатаем разделитель в отладочный вывод
					printf("------------------------------------------------------------\n\n");
					// Печатаем заголовок в отладочный вывод
					printf("Current certificate verification:\n");
					// Получаем название сертификата
					::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
					// Возвращаем название сертификата
					printf("Subject: %s\n", buffer);
					// Получаем эмитента выпустившего сертификат
					::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
					// Возвращаем эмитента сертификата
					printf("Issuer: %s\n", buffer);
					// Записываем в лог информацию о ошибке
					printf("Error: %s\n", ::X509_verify_cert_error_string(::X509_STORE_CTX_get_error(store)));
					// Записываем в лог информацию об успешной проверке
					printf("Status: Certificate verified successfully at depth %d\n", ::X509_STORE_CTX_get_error_depth(store));
					// Печатаем конечный разделитель
					printf("\n------------------------------------------------------------\n\n");
				}
			}
		#endif
		// Возвращаем результат
		return ok;
	}
	/**
	 * @brief Функция обратного вызова для проверки валидности хоста
	 *
	 * @param store хранилище сертификатов
	 * @param ctx   передаваемый контекст
	 * @return      результат проверки
	 *
	 */
	static int32_t hostname(X509_STORE_CTX * store, void * ctx) noexcept {
		// Результат проверки домена
		int32_t result = 0;
		// Если объекты переданы верно
		if((store != nullptr) && (ctx != nullptr)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::cts_t *> (ctx);
			// Создаём охранника участника обмена защищёнными данными
			::local::guard_t guard(member);
			// Если проверка сертификата не требуется
			if(!(member->state & state::CERTIFICATE_VERIFY)){
				/**
				 * Определяем узел события к которому относится контекст TLS
				 */
				switch(static_cast <uint8_t> (member->node)){
					// Если узел является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT):
						// Печатаем сообщение об успешной проверке
						return ::verify::certificate(1, store);
					// Если узел является сервером
					case static_cast <uint8_t> (event::node_t::SERVER):
						// Печатаем сообщение об успешной проверке
						return 1;
				}
			}
			// Если проверка сертификата прошла удачно
			if((result = ::X509_verify_cert(store)) != 1){
				// Если произошла ошибка несоответствия имени хоста
				if(::X509_STORE_CTX_get_error(store) == X509_V_ERR_HOSTNAME_MISMATCH){
					// Запрашиваем данные сертификата
					X509 * x509 = ::X509_STORE_CTX_get_current_cert(store);
					// Если данные сертификата не получены
					if(x509 == nullptr){
						// Выполняем получение идентификатора контекста TLS
						const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, ::tls::coder_t::state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Certificate is not found in store");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, ::tls::coder_t::error_t::CERT_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							// Получаем объект логирования
							log_t * log = reinterpret_cast <log_t *> (::SSL_CTX_get_ex_data(member->ctx, ::__awh_ssl_index__[6]));
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					// Если данные сертификата получены
					} else {
						// Получаем имя эмитента выпустившего сертификат
						X509_NAME * name = ::X509_get_issuer_name(x509);
						// Если имя эмитента не получено
						if(name == nullptr){
							// Выполняем получение идентификатора контекста TLS
							const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, ::tls::coder_t::state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Certificate issuer name is not found");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, ::tls::coder_t::error_t::CERT_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								// Получаем объект логирования
								log_t * log = reinterpret_cast <log_t *> (::SSL_CTX_get_ex_data(member->ctx, ::__awh_ssl_index__[6]));
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("%s", log_t::flag_t::WARNING, error.c_str());
								#endif
							}
						// Если имя эмитента получено
						} else {
							// Буфер доменного имени
							char fqdn[0xFF];
							// Заполняем буфер нулями
							::memset(fqdn, 0, sizeof(fqdn));
							// Запрашиваем имя домена
							::X509_NAME_oneline(name, fqdn, sizeof(fqdn));
							// Выполняем проверку на соответствие хоста с данными хостов у сертификата
							const status_t status = ::verify::validateHostname(member->host, x509);
							// Если домен найден в записях сертификата (т.е. сертификат соответствует данному домену)
							if((result = static_cast <int32_t> (status == status_t::MatchFound))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Получаем объект логирования
									log_t * log = reinterpret_cast <log_t *> (::SSL_CTX_get_ex_data(member->ctx, ::__awh_ssl_index__[6]));
									// Записываем в лог сообщение
									log->print("HTTPS server [%s] has this certificate, which looks good to me: %s", log_t::flag_t::INFO, member->host.c_str(), fqdn);
								#endif
							// Если ресурс не найден тогда выводим сообщение об ошибке
							} else {
								// Буфер под результат
								char result[31];
								// Устанавливаем результат ошибки по умолчанию
								::snprintf(result, 31, "%s", "X509 Verify certificate failed");
								/**
								 * Определяем полученную ошибку
								 */
								switch(static_cast <uint8_t> (status)){
									// Если домен найден в записях сертификата
									case static_cast <uint8_t> (status_t::MatchFound):
										// Устанавливаем статус проверки
										::snprintf(result, 14, "%s", "Found a match");
									break;
									// Если домен не найден в записях сертификата
									case static_cast <uint8_t> (status_t::MatchNotFound):
										// Устанавливаем статус проверки
										::snprintf(result, 15, "%s", "No match found");
									break;
									// Если в сертификате отсутствует SAN
									case static_cast <uint8_t> (status_t::NoSANPresent):
										// Устанавливаем статус проверки
										::snprintf(result, 18, "%s", "Present is no SAN");
									break;
									// Если сертификат имеет неверный формат
									case static_cast <uint8_t> (status_t::MalformedCertificate):
										// Устанавливаем статус проверки
										::snprintf(result, 22, "%s", "Malformed certificate");
									break;
									// Если произошла ошибка при проверке
									case static_cast <uint8_t> (status_t::Error):
										// Устанавливаем статус проверки
										::snprintf(result, 6, "%s", "Error");
									break;
									// В иных случаях
									default: ::snprintf(result, 4, "%s", "WTF");
								}
								// Выполняем получение идентификатора контекста TLS
								const ::tls::coder_t::id_t id = static_cast <::tls::coder_t::id_t> (reinterpret_cast <uintptr_t> (member));
								// Получаем объект фреймворка
								fmk_t * fmk = reinterpret_cast <fmk_t *> (::SSL_CTX_get_ex_data(member->ctx, ::__awh_ssl_index__[5]));
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, ::tls::coder_t::state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, fmk->format("%s for hostname '%s' [%s]", result, member->host.c_str(), fqdn));
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, ::tls::coder_t::error_t::HOSTNAME_BAD, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									// Получаем объект логирования
									log_t * log = reinterpret_cast <log_t *> (::SSL_CTX_get_ex_data(member->ctx, ::__awh_ssl_index__[6]));
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log->print("%s", log_t::flag_t::WARNING, error.c_str());
									#endif
								}
							}
						}
					}
				}
			}
		}
		// Возвращаем результат
		return result;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::tls::Coder::CipherInfo::CipherInfo() noexcept :
 tls13(false),
 name{""}, origin{""},
 cipher(cipher_t::UNKNOWN) {}

/**
 * @brief Метод получения версии OpenSSL
 *
 * @return версия OpenSSL
 *
 */
string awh::tls::Coder::version() const noexcept {
	// Возвращаем версию OpenSSL
	return ::OpenSSL_version(OPENSSL_VERSION);
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 * @details Защищает только глобальный реестр TLS (ids, members, splice_map,
 *          init OpenSSL). Методы модуля после закрепления id выполняются без
 *          глобального lock; синхронизацию вызовов из разных потоков должен
 *          обеспечивать вызывающий код.
 *
 */
void awh::tls::Coder::threadSafety(const bool mode) noexcept {
	// Если объект для работы с отпечатками TLS установлен
	if(this->_fgp != nullptr)
		// Устанавливаем режим безопасности работы потоков для хранилища отпечатков
		const_cast <fgp_t *> (this->_fgp)->threadSafety(mode);
	// Активируем работу мьютекса блокировки глобального состояния TLS
	::__awh_ssl_mutex__.enabled = mode;
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
}
/**
 * @brief Метод подключения объекта для работы с отпечатками TLS
 *
 * @param fgp объект для работы с отпечатками TLS
 *
 */
void awh::tls::Coder::fingerprint(const fgp_t * fgp) noexcept {
	// Сохраняем объект для работы с отпечатками TLS
	this->_fgp = fgp;
	// Если объект для работы с отпечатками TLS установлен
	if(this->_fgp != nullptr)
		// Устанавливаем режим безопасности работы потоков для хранилища отпечатков
		const_cast <fgp_t *> (this->_fgp)->threadSafety(::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод получения общей информации о TLS соединении
 *
 * @param id идентификатор события
 * @return   общая информация о TLS соединении
 *
 */
string awh::tls::Coder::info(const id_t id) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for info operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::INVALID_LAYER, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Объект SSL сертификата
					X509 * x509 = nullptr;
					/**
					 * Определяем узел события к которому относится контекст TLS
					 */
					switch(static_cast <uint8_t> (member->node)){
						// Если узел является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
							// Выполняем получение сертификата сервера
							x509 = ::SSL_get_peer_certificate(member->ssl);
						break;
						// Если узел является сервером
						case static_cast <uint8_t> (event::node_t::SERVER):
							// Выполняем получение сертификата сервера
							x509 = ::SSL_get_certificate(member->ssl);
						break;
					}
					// Если сертификат сервера получен
					if(x509 != nullptr){
						// Буфер данных для получения данных
						char buffer[0xFF];
						// Если всё хорошо, формируем версию OpenSSL
						result.append(this->_fmk->format("Using %s\n\n", ::OpenSSL_version(OPENSSL_VERSION)));
						// Добавляем заголовок цепочки сертификатов
						result.append("---\nCertificate chain\n");
						// Получаем эмитента субъекта сертификата
						::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
						// Добавляем информацию о субъекте сертификата
						result.append(this->_fmk->format(" 0 s:%s\n", buffer));
						// Получаем эмитента выпустившего сертификат
						::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
						// Добавляем информацию об эмитенте сертификата
						result.append(this->_fmk->format("   i:%s\n", buffer));
						// Извлекаем объект публичного ключа сертификата
						EVP_PKEY * pubkey = ::X509_get0_pubkey(x509);
						// Получаем тип публичного ключа
						const int32_t pkeyType = ::EVP_PKEY_base_id(pubkey);
						// Строковое представление публичного ключа
						string pkey = "";
						/**
						 * Определяем тип публичного ключа
						 */
						switch(pkeyType){
							// Если тип ключа RSA
							case EVP_PKEY_RSA:
								// Устанавливаем строковое представление ключа
								pkey = "RSA";
							break;
							// Если тип ключа EC
							case EVP_PKEY_EC:
								// Устанавливаем строковое представление ключа
								pkey = "EC";
							break;
							// Если тип ключа DSA
							case EVP_PKEY_DSA:
								// Устанавливаем строковое представление ключа
								pkey = "DSA";
							break;
							// Если тип ключа DH
							case EVP_PKEY_DH:
								// Устанавливаем строковое представление ключа
								pkey = "DH";
							break;
							// Если тип ключа ED25519
							case EVP_PKEY_ED25519:
								// Устанавливаем строковое представление ключа
								pkey = "ED25519";
							break;
							// Если тип ключа ED448
							case EVP_PKEY_ED448:
								// Устанавливаем строковое представление ключа
								pkey = "ED448";
							break;
							// Если тип ключа X25519
							case EVP_PKEY_X25519:
								// Устанавливаем строковое представление ключа
								pkey = "X25519";
							break;
							// Если тип ключа X448
							case EVP_PKEY_X448:
								// Устанавливаем строковое представление ключа
								pkey = "X448";
							break;
							// В иных случаях
							default:
								// Устанавливаем строковое представление ключа
								pkey = "unknown";
						}
						// Добавляем информацию об алгоритме ключа и подписи
						result.append(this->_fmk->format("   a:PKEY: %s, %d (bit); sigalg: %s\n", pkey.c_str(), ::EVP_PKEY_bits(pubkey), ::OBJ_nid2ln(::X509_get_signature_nid(x509))));
						// Буферы для сроков действия
						char bufferBefore[64], bufferAfter[64];
						// Извлекаем объект срока окончания действия сертификата
						const ASN1_TIME * after = ::X509_get0_notAfter(x509);
						// Извлекаем объект срока начала действия сертификата
						const ASN1_TIME * before = ::X509_get0_notBefore(x509);
						// Создаём объект BIO для записи срока действия сертификата
						BIO * bio = ::BIO_new(::BIO_s_mem());
						// Извлекаем срока начала действия сертификата
						::ASN1_TIME_print(bio, before);
						// Извлекаем данные срока начала действия сертификата
						int32_t length = ::BIO_gets(bio, bufferBefore, sizeof(bufferBefore));
						// Выполняем сброс BIO
						BIO_reset(bio);
						// Извлекаем срока окончания действия сертификата
						::ASN1_TIME_print(bio, after);
						// Извлекаем данные срока окончания действия сертификата
						length = ::BIO_gets(bio, bufferAfter, sizeof(bufferAfter));
						// Выполняем сброс BIO
						BIO_reset(bio);
						// Добавляем информацию о сроках действия сертификата
						result.append(this->_fmk->format("   v:NotBefore: %s; NotAfter: %s\n", bufferBefore, bufferAfter));
						// Записываем сертификат в PEM формате в объект BIO
						::PEM_write_bio_X509(bio, x509);
						// Буффер для получения данных
						char * certificate = nullptr;
						// Извлекаем данные сертификата из BIO
						const long size = ::BIO_get_mem_data(bio, &certificate);
						// Если сертификат извлечён удачно
						if(size > 0)
							// Записываем результат
							result.append(certificate, static_cast <size_t> (size));
						// Освобождаем объект BIO
						::BIO_free(bio);
						// Добавляем заголовок сертификата сервера
						result.append("---\nServer certificate\n");
						// Получаем эмитента субъекта сертификата
						::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
						// Добавляем информацию о субъекте сертификата
						result.append(this->_fmk->format("subject=%s\n", buffer));
						// Получаем эмитента выпустившего сертификат
						::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
						// Добавляем информацию об эмитенте сертификата
						result.append(this->_fmk->format("issuer=%s\n", buffer));
						// Добавляем разделитель
						result.append("---\n");
						// Добавляем информацию о параметрах соединения
						result.append(this->_fmk->format("New, %s, Cipher is %s\n", ::SSL_get_version(member->ssl), ::SSL_CIPHER_get_name(::SSL_get_current_cipher(member->ssl))));
						// Добавляем информацию о протоколе
						result.append(this->_fmk->format("Protocol: %s\n", ::SSL_get_version(member->ssl)));
						// Добавляем информацию о публичном ключе сервера
						result.append(this->_fmk->format("Server public key is %d bit\n", ::EVP_PKEY_bits(pubkey)));
						// Извлекаем результат верификации
						const long verify = ::SSL_get_verify_result(member->ssl);
						// Добавляем информацию о результате верификации
						result.append(this->_fmk->format("Verify return code: %d (%s)\n", verify, ::X509_verify_cert_error_string(verify)));
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT)
							// Освобождаем объект сертификата
							::X509_free(x509);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения информации о одноразовом узле TLS
 *
 * @param id идентификатор события
 * @return   информация о одноразовом узле TLS
 *
 */
string awh::tls::Coder::peerInfo(const id_t id) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если версия OpenSSL не соответствует указанной при сборке
					if(::OpenSSL_version_num() != OPENSSL_VERSION_NUMBER){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr){
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(
								id, error_t::MISMATCH_VERSION,
								this->_fmk->format(
									"OpenSSL version mismatch!\n"
									"Compiled against %s\n"
									"Linked against   %s",
									OPENSSL_VERSION_TEXT,
									::OpenSSL_version(OPENSSL_VERSION)
								)
							);
							// Если мажорная и минорная версия OpenSSL не совпадают
							if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::MISMATCH_VERSION, "Major and minor version numbers must match, exiting");
						// Если функция обратного вызова ошибки не установлена
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"OpenSSL version mismatch!\n"
									"Compiled against %s\n"
									"Linked against   %s",
									__PRETTY_FUNCTION__,
									make_tuple(id),
									log_t::flag_t::WARNING,
									OPENSSL_VERSION_TEXT,
									::OpenSSL_version(OPENSSL_VERSION)
								);
								// Если мажорная и минорная версия OpenSSL не совпадают
								if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
									// Записываем в лог сообщение
									this->_log->debug("Major and minor version numbers must match, exiting", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем в лог сообщение
								this->_log->print(
									"OpenSSL version mismatch!\r\n"
									"Compiled against %s\r\n"
									"Linked against   %s",
									log_t::flag_t::WARNING,
									OPENSSL_VERSION_TEXT,
									::OpenSSL_version(OPENSSL_VERSION)
								);
								// Если мажорная и минорная версия OpenSSL не совпадают
								if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
									// Записываем в лог сообщение
									this->_log->print("Major and minor version numbers must match, exiting", log_t::flag_t::CRITICAL);
							#endif
						}
					// Если всё хорошо, формируем версию OpenSSL
					} else result.append(this->_fmk->format("Using %s\n\n", ::OpenSSL_version(OPENSSL_VERSION)));
					// Если версия OpenSSL ниже версии 1.1.1b
					if(OPENSSL_VERSION_NUMBER < 0x1010102FL){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(
								id, error_t::UNSUPPORTED_VERSION,
								this->_fmk->format("%s is unsupported, use OpenSSL Version 1.1.1a or higher", ::OpenSSL_version(OPENSSL_VERSION))
							);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем в лог сообщение
								this->_log->debug("%s is unsupported, use OpenSSL Version 1.1.1a or higher", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, ::OpenSSL_version(OPENSSL_VERSION));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем в лог сообщение
								this->_log->print("%s is unsupported, use OpenSSL Version 1.1.1a or higher", log_t::flag_t::CRITICAL, ::OpenSSL_version(OPENSSL_VERSION));
							#endif
						}
						// Выходим из приложения
						::_exit(EXIT_FAILURE);
					}
					// Если объект CRL-файла сертификата создан
					if(member->crl != nullptr){
						// Создаём memory BIO
						BIO * bio = ::BIO_new(::BIO_s_mem());
						// Если BIO не создан
						if(bio == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем результат
							return result;
						}
						// Печатаем CRL в BIO
						if(::X509_CRL_print(bio, member->crl) == 0){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выполняем очистку BIO
							::BIO_free(bio);
							// Возвращаем результат
							return result;
						}
						// Получаем размер данных
						char * data = nullptr;
						// Выполняем извлечение данных из BIO
						const size_t length = static_cast <size_t> (::BIO_get_mem_data(bio, &data));
						// Если информация получена
						if(length > 0)
							// Возвращаем параметры шифрования
							result = ::move(this->_fmk->format("%sCertificate Revocation List: %s\n", result.c_str(), string(data, length).c_str()));
						// Выполняем очистку BIO
						::BIO_free(bio);
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если версия OpenSSL не соответствует указанной при сборке
					if(::OpenSSL_version_num() != OPENSSL_VERSION_NUMBER){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr){
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(
								id, error_t::MISMATCH_VERSION,
								this->_fmk->format(
									"OpenSSL version mismatch!\n"
									"Compiled against %s\n"
									"Linked against   %s",
									OPENSSL_VERSION_TEXT,
									::OpenSSL_version(OPENSSL_VERSION)
								)
							);
							// Если мажорная и минорная версия OpenSSL не совпадают
							if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::MISMATCH_VERSION, "Major and minor version numbers must match, exiting");
						// Если функция обратного вызова ошибки не установлена
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"OpenSSL version mismatch!\n"
									"Compiled against %s\n"
									"Linked against   %s",
									__PRETTY_FUNCTION__,
									make_tuple(id),
									log_t::flag_t::WARNING,
									OPENSSL_VERSION_TEXT,
									::OpenSSL_version(OPENSSL_VERSION)
								);
								// Если мажорная и минорная версия OpenSSL не совпадают
								if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
									// Записываем в лог сообщение
									this->_log->debug("Major and minor version numbers must match, exiting", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем в лог сообщение
								this->_log->print(
									"OpenSSL version mismatch!\r\n"
									"Compiled against %s\r\n"
									"Linked against   %s",
									log_t::flag_t::WARNING,
									OPENSSL_VERSION_TEXT,
									::OpenSSL_version(OPENSSL_VERSION)
								);
								// Если мажорная и минорная версия OpenSSL не совпадают
								if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
									// Записываем в лог сообщение
									this->_log->print("Major and minor version numbers must match, exiting", log_t::flag_t::CRITICAL);
							#endif
						}
					// Если всё хорошо, формируем версию OpenSSL
					} else result.append(this->_fmk->format("Using %s\n\n", ::OpenSSL_version(OPENSSL_VERSION)));
					// Если версия OpenSSL ниже версии 1.1.1b
					if(OPENSSL_VERSION_NUMBER < 0x1010102fL){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(
								id, error_t::UNSUPPORTED_VERSION,
								this->_fmk->format("%s is unsupported, use OpenSSL Version 1.1.1a or higher", ::OpenSSL_version(OPENSSL_VERSION))
							);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем в лог сообщение
								this->_log->debug("%s is unsupported, use OpenSSL Version 1.1.1a or higher", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, ::OpenSSL_version(OPENSSL_VERSION));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем в лог сообщение
								this->_log->print("%s is unsupported, use OpenSSL Version 1.1.1a or higher", log_t::flag_t::CRITICAL, ::OpenSSL_version(OPENSSL_VERSION));
							#endif
						}
						// Выходим из приложения
						::_exit(EXIT_FAILURE);
					}
					// Если объект подключения создан и сертификат передан
					if(member->ssl != nullptr){
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Буфер данных для получения данных
								char buffer[0xFF];
								// Выполняем получение сертификата сервера
								X509 * x509 = ::SSL_get_certificate(member->ssl);
								// Если сертификат сервера получен
								if(x509 != nullptr){
									// Получаем название сертификата
									::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
									// Формируем результат
									result = ::move(this->_fmk->format("%sClient peer certificates:\nSubject: %s\n", result.c_str(), buffer));
									// Получаем эмитента выпустившего сертификат
									::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
									// Формируем результат
									result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
									// Возвращаем параметры шифрования
									result = ::move(this->_fmk->format("%sCipher: %s\n", result.c_str(), ::SSL_CIPHER_get_name(::SSL_get_current_cipher(member->ssl))));
								}
								// Выполняем получение сертификата сервера
								x509 = ::SSL_get_peer_certificate(member->ssl);
								// Если сертификат сервера получен
								if(x509 != nullptr){
									// Получаем название сертификата
									::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
									// Формируем результат
									result = ::move(this->_fmk->format("%s\nServer peer certificates:\nSubject: %s\n", result.c_str(), buffer));
									// Получаем эмитента выпустившего сертификат
									::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
									// Формируем результат
									result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
									// Возвращаем параметры шифрования
									result = ::move(this->_fmk->format("%sCipher: %s\n", result.c_str(), ::SSL_CIPHER_get_name(::SSL_get_current_cipher(member->ssl))));
									// Освобождаем объект сертификата
									::X509_free(x509);
								}
							} break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Выполняем получение сертификата сервера
								X509 * x509 = ::SSL_get_certificate(member->ssl);
								// Если сертификат сервера получен
								if(x509 != nullptr){
									// Буфер данных для получения данных
									char buffer[0xFF];
									// Получаем название сертификата
									::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
									// Формируем результат
									result = ::move(this->_fmk->format("%sPeer certificates:\nSubject: %s\n", result.c_str(), buffer));
									// Получаем эмитента выпустившего сертификат
									::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
									// Формируем результат
									result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
									// Возвращаем параметры шифрования
									result = ::move(this->_fmk->format("%sCipher: %s\n", result.c_str(), ::SSL_CIPHER_get_name(::SSL_get_current_cipher(member->ssl))));
								}
							} break;
						}
					}
					// Если объект CRL-файла сертификата создан
					if((member->crl != nullptr) && ((* member->crl) != nullptr)){
						// Создаём memory BIO
						BIO * bio = ::BIO_new(::BIO_s_mem());
						// Если BIO не создан
						if(bio == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем результат
							return result;
						}
						// Печатаем CRL в BIO
						if(::X509_CRL_print(bio, * member->crl) == 0){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выполняем очистку BIO
							::BIO_free(bio);
							// Возвращаем результат
							return result;
						}
						// Получаем размер данных
						char * data = nullptr;
						// Выполняем извлечение данных из BIO
						const size_t length = static_cast <size_t> (::BIO_get_mem_data(bio, &data));
						// Если информация получена
						if(length > 0)
							// Возвращаем параметры шифрования
							result = ::move(this->_fmk->format("%sCertificate Revocation List: %s\n", result.c_str(), string(data, length).c_str()));
						// Выполняем очистку BIO
						::BIO_free(bio);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения информации о шифре
 *
 * @param id идентификатор события
 * @return   информация о шифре
 *
 */
string awh::tls::Coder::cipherInfo(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for cipher info operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CIPHER_FAILED, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL):
					// Выполняем извлечение информации о шифре
					return ::SSL_CIPHER_get_name(::SSL_get_current_cipher(reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id))->ssl));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Метод получения информации о сертификате
 *
 * @param id идентификатор события
 * @return   информация о сертификате
 *
 */
string awh::tls::Coder::certificateInfo(const id_t id) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for certificate info operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CERT_FAILED, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					/**
					 * Определяем узел события к которому относится контекст TLS
					 */
					switch(static_cast <uint8_t> (member->node)){
						// Если узел является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получить сертификат клиента (на сервере) или сервера (на клиенте)
							X509 * x509 = ::SSL_get_peer_certificate(member->ssl);
							// Если сертификат получен
							if(x509 != nullptr){
								// Буфер данных для получения данных
								char buffer[0xFF];
								// Получаем название сертификата
								::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
								// Формируем результат
								result = ::move(this->_fmk->format("Peer certificates:\nSubject: %s\n", buffer));
								// Получаем эмитента выпустившего сертификат
								::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
								// Формируем результат
								result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
								// Освобождаем объект сертификата
								::X509_free(x509);
							}
						} break;
						// Если узел является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Выполняем получение сертификата сервера
							X509 * x509 = ::SSL_get_certificate(member->ssl);
							// Если сертификат сервера получен
							if(x509 != nullptr){
								// Буфер данных для получения данных
								char buffer[0xFF];
								// Получаем название сертификата
								::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
								// Формируем результат
								result = ::move(this->_fmk->format("Peer certificates:\nSubject: %s\n", buffer));
								// Получаем эмитента выпустившего сертификат
								::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
								// Формируем результат
								result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
							}
						} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения информации о списке отзыва сертификатов
 *
 * @param id идентификатор события
 * @return   информация о списке отзыва сертификатов
 *
 */
string awh::tls::Coder::certificateRevocationListInfo(const id_t id) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если объект CRL-файла сертификата создан
					if(member->crl != nullptr){
						// Создаём memory BIO
						BIO * bio = ::BIO_new(::BIO_s_mem());
						// Если BIO не создан
						if(bio == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем результат
							return result;
						}
						// Печатаем CRL в BIO
						if(::X509_CRL_print(bio, member->crl) == 0){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выполняем очистку BIO
							::BIO_free(bio);
							// Возвращаем результат
							return result;
						}
						// Получаем размер данных
						char * data = nullptr;
						// Выполняем извлечение данных из BIO
						const size_t length = static_cast <size_t> (::BIO_get_mem_data(bio, &data));
						// Если информация получена
						if(length > 0)
							// Возвращаем параметры шифрования
							result.assign(data, length);
						// Выполняем очистку BIO
						::BIO_free(bio);
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если объект CRL-файла сертификата создан
					if((member->crl != nullptr) && ((* member->crl) != nullptr)){
						// Создаём memory BIO
						BIO * bio = ::BIO_new(::BIO_s_mem());
						// Если BIO не создан
						if(bio == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем результат
							return result;
						}
						// Печатаем CRL в BIO
						if(::X509_CRL_print(bio, * member->crl) == 0){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выполняем очистку BIO
							::BIO_free(bio);
							// Возвращаем результат
							return result;
						}
						// Получаем размер данных
						char * data = nullptr;
						// Выполняем извлечение данных из BIO
						const size_t length = static_cast <size_t> (::BIO_get_mem_data(bio, &data));
						// Если информация получена
						if(length > 0)
							// Возвращаем параметры шифрования
							result.assign(data, length);
						// Выполняем очистку BIO
						::BIO_free(bio);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения списка доступных шифров
 *
 * @param id идентификатор события
 * @return   список доступных шифров
 *
 */
vector <awh::tls::Coder::cipher_info_t> awh::tls::Coder::availableCiphers(const id_t id) const noexcept {
	// Переменная результата
	vector <cipher_info_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Объект для хранения информации о шифре
			cipher_info_t info;
			/**
			 * TLSv1.3 (фиксированы RFC 8446, всегда доступны в TLS_method())
			 * В BoringSSL нет публичного API для итерации cipher_list_tls13.
			 * Стандартные шифры жестко определены, их коды не меняются.
			 */
			static const struct { uint16_t code; const char * name; const char * origin; } ciphers[] = {
				{ 0x1301, "AES_128_GCM_SHA256", "TLS_AES_128_GCM_SHA256" },
				{ 0x1302, "AES_256_GCM_SHA384", "TLS_AES_256_GCM_SHA384" },
				{ 0x1303, "CHACHA20_POLY1305_SHA256", "TLS_CHACHA20_POLY1305_SHA256" }
			};
			/**
			 * Выполняем перебор всех стандартных шифров TLSv1.3 и добавляем их в результат, если они поддерживаются
			 */
			for(const auto & cipher : ciphers){
				// Определяем поддерживает ли шифр TLSv1.3
				info.tls13 = true;
				/**
				 * Определяем код шифра
				 */
				switch(cipher.code){
					// Если код шифра соответствует TLS_AES_128_GCM_SHA256
					case 0x1301:
						// Получаем код шифра
						info.cipher = cipher_t::TLS_AES_128_GCM_SHA256;
					break;
					// Если код шифра соответствует TLS_AES_256_GCM_SHA384
					case 0x1302:
						// Получаем код шифра
						info.cipher = cipher_t::TLS_AES_256_GCM_SHA384;
					break;
					// Если код шифра соответствует TLS_CHACHA20_POLY1305_SHA256
					case 0x1303:
						// Получаем код шифра
						info.cipher = cipher_t::TLS_CHACHA20_POLY1305_SHA256;
					break;
				}
				// Получаем название шифра
				info.name = cipher.name;
				// Получаем стандартное название шифра
				info.origin = cipher.origin;
				// Добавляем информацию о шифре в результат
				result.push_back(::move(info));
			}
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если объект контекста безопасности создан
					if(member->ctx != nullptr){
						// Получаем список доступных шифров
						const STACK_OF(SSL_CIPHER) * ciphers = ::SSL_CTX_get_ciphers(member->ctx);
						// Получаем количество шифров в списке
						const size_t count = static_cast <size_t> (::sk_SSL_CIPHER_num(ciphers));
						// Если в списке есть шифры
						if(count > 0){
							// Текущее значение кода шифра
							uint16_t code = 0;
							// Получаем текущий размер результата
							const size_t size = result.size();
							// Выделяем память для хранения информации о шифрах
							result.resize(size + count);
							/**
							 * Проходим по каждому шифру в списке
							 */
							for(size_t i = 0; i < count; ++i){
								// Получаем объект шифра
								const SSL_CIPHER * c = ::sk_SSL_CIPHER_value(ciphers, i);
								// Получаем код шифра
								code = static_cast <uint16_t> (::SSL_CIPHER_get_id(c) & 0xFFFF);
								/**
								 * Определяем код шифра
								 */
								switch(code){
									// Если код шифра соответствует AES128-SHA
									case 0x002F:
										// Получаем код шифра
										info.cipher = cipher_t::AES128_SHA;
									break;
									// Если код шифра соответствует AES256-SHA
									case 0x0035:
										// Получаем код шифра
										info.cipher = cipher_t::AES256_SHA;
									break;
									// Если код шифра соответствует AES128-GCM-SHA256
									case 0x009C:
										// Получаем код шифра
										info.cipher = cipher_t::AES128_GCM_SHA256;
									break;
									// Если код шифра соответствует AES256-GCM-SHA384
									case 0x009D:
										// Получаем код шифра
										info.cipher = cipher_t::AES256_GCM_SHA384;
									break;
									// Если код шифра соответствует PSK-AES128-CBC-SHA
									case 0x008C:
										// Получаем код шифра
										info.cipher = cipher_t::PSK_AES128_CBC_SHA;
									break;
									// Если код шифра соответствует PSK-AES256-CBC-SHA
									case 0x008D:
										// Получаем код шифра
										info.cipher = cipher_t::PSK_AES256_CBC_SHA;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES128-SHA
									case 0xC013:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES128_SHA;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES256-SHA
									case 0xC014:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES256_SHA;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA
									case 0xC009:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES128_SHA;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES256-SHA
									case 0xC00A:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES256_SHA;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES128-SHA256
									case 0xC027:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES128_SHA256;
									break;
									// Если код шифра соответствует ECDHE-PSK-AES128-CBC-SHA
									case 0xC035:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_PSK_AES128_CBC_SHA;
									break;
									// Если код шифра соответствует ECDHE-PSK-AES256-CBC-SHA
									case 0xC036:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_PSK_AES256_CBC_SHA;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA256
									case 0xC023:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES128_SHA256;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES128-GCM-SHA256
									case 0xC02F:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES128_GCM_SHA256;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES256-GCM-SHA384
									case 0xC030:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES256_GCM_SHA384;
									break;
									// Если код шифра соответствует ECDHE-RSA-CHACHA20-POLY1305
									case 0xCCA8:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_CHACHA20_POLY1305;
									break;
									// Если код шифра соответствует ECDHE-PSK-CHACHA20-POLY1305
									case 0xCCAC:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_PSK_CHACHA20_POLY1305;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES128-GCM-SHA256
									case 0xC02B:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES256-GCM-SHA384
									case 0xC02C:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-CHACHA20-POLY1305
									case 0xCCA9:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305;
									break;
									// Если код шифра не соответствует ни одному из известных
									default: info.cipher = cipher_t::UNKNOWN;
								}
								// Получаем название шифра
								info.name = ::SSL_CIPHER_get_name(c);
								// Получаем стандартное название шифра
								info.origin = ::SSL_CIPHER_standard_name(c);
								// Определяем поддерживает ли шифр TLSv1.3
								info.tls13 = ((code >= 0x1300) && (code <= 0x13FF));
								// Добавляем информацию о шифре в результат
								result[size + i] = ::move(info);
							}
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если объект подключения создан и сертификат передан
					if(member->ssl != nullptr){
						// Получаем список доступных шифров
						const STACK_OF(SSL_CIPHER) * ciphers = ::SSL_get_ciphers(member->ssl);
						// Получаем количество шифров в списке
						const size_t count = static_cast <size_t> (::sk_SSL_CIPHER_num(ciphers));
						// Если в списке есть шифры
						if(count > 0){
							// Текущее значение кода шифра
							uint16_t code = 0;
							// Получаем текущий размер результата
							const size_t size = result.size();
							// Выделяем память для хранения информации о шифрах
							result.resize(size + count);
							/**
							 * Проходим по каждому шифру в списке
							 */
							for(size_t i = 0; i < count; ++i){
								// Получаем объект шифра
								const SSL_CIPHER * c = ::sk_SSL_CIPHER_value(ciphers, i);
								// Получаем код шифра
								code = static_cast <uint16_t> (::SSL_CIPHER_get_id(c) & 0xFFFF);
								/**
								 * Определяем код шифра
								 */
								switch(code){
									// Если код шифра соответствует AES128-SHA
									case 0x002F:
										// Получаем код шифра
										info.cipher = cipher_t::AES128_SHA;
									break;
									// Если код шифра соответствует AES256-SHA
									case 0x0035:
										// Получаем код шифра
										info.cipher = cipher_t::AES256_SHA;
									break;
									// Если код шифра соответствует AES128-GCM-SHA256
									case 0x009C:
										// Получаем код шифра
										info.cipher = cipher_t::AES128_GCM_SHA256;
									break;
									// Если код шифра соответствует AES256-GCM-SHA384
									case 0x009D:
										// Получаем код шифра
										info.cipher = cipher_t::AES256_GCM_SHA384;
									break;
									// Если код шифра соответствует PSK-AES128-CBC-SHA
									case 0x008C:
										// Получаем код шифра
										info.cipher = cipher_t::PSK_AES128_CBC_SHA;
									break;
									// Если код шифра соответствует PSK-AES256-CBC-SHA
									case 0x008D:
										// Получаем код шифра
										info.cipher = cipher_t::PSK_AES256_CBC_SHA;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES128-SHA
									case 0xC013:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES128_SHA;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES256-SHA
									case 0xC014:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES256_SHA;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA
									case 0xC009:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES128_SHA;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES256-SHA
									case 0xC00A:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES256_SHA;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES128-SHA256
									case 0xC027:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES128_SHA256;
									break;
									// Если код шифра соответствует ECDHE-PSK-AES128-CBC-SHA
									case 0xC035:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_PSK_AES128_CBC_SHA;
									break;
									// Если код шифра соответствует ECDHE-PSK-AES256-CBC-SHA
									case 0xC036:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_PSK_AES256_CBC_SHA;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA256
									case 0xC023:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES128_SHA256;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES128-GCM-SHA256
									case 0xC02F:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES128_GCM_SHA256;
									break;
									// Если код шифра соответствует ECDHE-RSA-AES256-GCM-SHA384
									case 0xC030:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_AES256_GCM_SHA384;
									break;
									// Если код шифра соответствует ECDHE-RSA-CHACHA20-POLY1305
									case 0xCCA8:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_RSA_CHACHA20_POLY1305;
									break;
									// Если код шифра соответствует ECDHE-PSK-CHACHA20-POLY1305
									case 0xCCAC:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_PSK_CHACHA20_POLY1305;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES128-GCM-SHA256
									case 0xC02B:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-AES256-GCM-SHA384
									case 0xC02C:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384;
									break;
									// Если код шифра соответствует ECDHE-ECDSA-CHACHA20-POLY1305
									case 0xCCA9:
										// Получаем код шифра
										info.cipher = cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305;
									break;
									// Если код шифра не соответствует ни одному из известных
									default: info.cipher = cipher_t::UNKNOWN;
								}
								// Получаем название шифра
								info.name = ::SSL_CIPHER_get_name(c);
								// Получаем стандартное название шифра
								info.origin = ::SSL_CIPHER_standard_name(c);
								// Определяем поддерживает ли шифр TLSv1.3
								info.tls13 = ((code >= 0x1300) && (code <= 0x13FF));
								// Добавляем информацию о шифре в результат
								result[size + i] = ::move(info);
							}
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод извлечения сертификата TLS
 *
 * @param id идентификатор события
 * @return   активный протокол
 *
 */
string awh::tls::Coder::certificateExtract(const id_t id) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for certificate extract operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CERT_FAILED, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Объект SSL сертификата
					X509 * x509 = nullptr;
					/**
					 * Определяем узел события к которому относится контекст TLS
					 */
					switch(static_cast <uint8_t> (member->node)){
						// Если узел является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
							// Выполняем получение сертификата сервера
							x509 = ::SSL_get_peer_certificate(member->ssl);
						break;
						// Если узел является сервером
						case static_cast <uint8_t> (event::node_t::SERVER):
							// Выполняем получение сертификата сервера
							x509 = ::SSL_get_certificate(member->ssl);
						break;
					}
					// Если сертификат сервера получен
					if(x509 != nullptr){
						// Создаём объект BIO для записи сертификата
						BIO * bio = ::BIO_new(::BIO_s_mem());
						// Записываем сертификат в PEM формате в объект BIO
						::PEM_write_bio_X509(bio, x509);
						// Буффер для получения данных
						char * buffer = nullptr;
						// Извлекаем данные сертификата из BIO
						const long size = ::BIO_get_mem_data(bio, &buffer);
						// Если сертификат извлечён удачно
						if(size > 0)
							// Записываем результат
							result.assign(buffer, static_cast <size_t> (size));
						// Освобождаем объект BIO
						::BIO_free(bio);
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT)
							// Освобождаем объект сертификата
							::X509_free(x509);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки валидности сертификата
 *
 * @param id идентификатор события
 * @return   результат проверки валидности сертификата
 *
 * @note Только для CTL. На CLIENT проверяет сертификат пира (сервера)
 *       по member->host.name (ожидаемое имя/SNI). На SERVER peer-сертификат —
 *       сертификат клиента, host.name — SNI клиента; вызывающий код должен
 *       понимать эту семантику (mTLS и т.п.).
 *
 */
bool awh::tls::Coder::validateCertificate(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for validate certificate operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::INVALID_LAYER, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Шаг 1: Получить сертификат
					X509 * x509 = ::SSL_get_peer_certificate(member->ssl);
					// Если сертификат не получен
					if(x509 == nullptr)
						// Нет сертификата
						return false;
					// Получаем текущую дату и время
					time_t date = ::time(nullptr);
					// Если срок действия сертификата истёк
					if(!((::X509_cmp_time(::X509_get0_notBefore(x509), &date) <= 0) && (::X509_cmp_time(::X509_get0_notAfter(x509), &date) >= 0))){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Сertificate is not yet valid or has expired");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CERT_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
						// Освобождаем объект сертификата
						::X509_free(x509);
						// Печатаем предупреждение о недействительном сертификате
						return false;
					}
					// Получить хранилище CA из SSL_CTX
					X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
					// Создать контекст проверки
					X509_STORE_CTX * ctx = ::X509_STORE_CTX_new();
					// Если контекст не создан
					if(ctx == nullptr){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "X509 Store context init");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::STORE_X509_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
						// Освобождаем объект сертификата
						::X509_free(x509);
						// Возвращаем отрицательный результат
						return false;
					}
					// Инициализировать (x509 — сертификат пира, untrusted — промежуточные, если есть)
					if(::X509_STORE_CTX_init(ctx, store, x509, ::SSL_get_peer_cert_chain(member->ssl)) == 0){
						// Выполняем очистку контекста
						::X509_STORE_CTX_free(ctx);
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "X509 Store context init");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::STORE_X509_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
						// Освобождаем объект сертификата
						::X509_free(x509);
						// Возвращаем отрицательный результат
						return false;
					}
					// Запустить проверку
					const int32_t result = ::X509_verify_cert(ctx);
					// Проверить результат
					if(result <= 0){
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем код ошибки
						const int32_t error = ::X509_STORE_CTX_get_error(ctx);
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::STORE_X509_FAILED, ::X509_verify_cert_error_string(error));
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, ::X509_verify_cert_error_string(error));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::X509_verify_cert_error_string(error));
							#endif
						}
						// Выполняем очистку контекста
						::X509_STORE_CTX_free(ctx);
						// Освобождаем объект сертификата
						::X509_free(x509);
						// Возвращаем отрицательный результат
						return false;
					}
					// Выполняем очистку контекста
					::X509_STORE_CTX_free(ctx);
					// Проверка по Subject Alternative Name (SAN) или Common Name (CN)
					bool ok = false;
					// Извлекаем SAN из сертификата
					GENERAL_NAMES * san = reinterpret_cast <GENERAL_NAMES *> (::X509_get_ext_d2i(x509, NID_subject_alt_name, nullptr, nullptr));
					// Если SAN присутствует
					if(san != nullptr){
						// Полученное доменное имя
						string fqdn = "";
						/**
						 * Проверяем каждый элемент SAN
						 */
						for(int32_t i = 0; i < sk_GENERAL_NAME_num(san); i++){
							// Извлекаем элемент SAN
							const GENERAL_NAME * cn = sk_GENERAL_NAME_value(san, i);
							// Проверяем тип имени
							if(cn->type == GEN_DNS){
								// Формируем строковое представление доменного имени
								fqdn.assign(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cn->d.dNSName))), ::ASN1_STRING_length(cn->d.dNSName));
								// Если размер имени и dns имя совпадает
								if((ok = ::verify::certHostcheck(member->host.name, fqdn)))
									// Выходим из цикла
									break;
							}
						}
						// Выполняем очистку SAN
						::GENERAL_NAMES_free(san);
					// Если SAN отсутствует или имя не совпало
					} else {
						// Буфер данных для получения данных
						char buffer[0xFF];
						// Fallback на Common Name (устаревшее, но иногда нужно)
						X509_NAME * subject = ::X509_get_subject_name(x509);
						// Если удалось получить Common Name
						if(::X509_NAME_get_text_by_NID(subject, NID_commonName, buffer, sizeof(buffer)) > 0)
							// Если размер имени и dns имя совпадает
							ok = ::verify::certHostcheck(member->host.name, buffer);
					}
					// Освобождаем объект сертификата
					::X509_free(x509);
					// Возвращаем результат проверки
					return ok;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки проверки доменного имени сервера
 *
 * @param id   идентификатор события
 * @param mode режим проверки доменного имени сервера
 *
 */
void awh::tls::Coder::validateServerNameIndication(const id_t id, const bool mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если режим проверки хоста суревра установлен
					if(mode)
						// Устанавливаем режим проверки сертификата
						member->state |= state::CERTIFICATE_VERIFY;
					// Снимаем режим проверки сертификата
					else member->state &= ~state::CERTIFICATE_VERIFY;
					/**
					 * Определяем узел события к которому относится контекст TLS
					 */
					switch(static_cast <uint8_t> (member->node)){
						// Если узел является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Если нужно произвести проверку
							if(member->state & state::CERTIFICATE_VERIFY)
								// Активируем проверку сертификата сервера
								::SSL_CTX_set_verify(member->ctx, SSL_VERIFY_PEER, &::verify::certificate);
							// Деактивируем проверку сертификата сервера
							else ::SSL_CTX_set_verify(member->ctx, SSL_VERIFY_NONE, nullptr);
						} break;
						// Если узел является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Если нужно произвести проверку
							if(member->state & state::CERTIFICATE_VERIFY)
								// Выполняем проверку сертификата клиента
								::SSL_CTX_set_verify(member->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &::verify::certificate);
							// Запрещаем выполнять првоерку доменного имени
							else ::SSL_CTX_set_verify(member->ctx, SSL_VERIFY_NONE, nullptr);
						} break;
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если режим проверки хоста суревра установлен
					if(mode)
						// Устанавливаем режим проверки сертификата
						member->state |= state::CERTIFICATE_VERIFY;
					// Снимаем режим проверки сертификата
					else member->state &= ~state::CERTIFICATE_VERIFY;
					/**
					 * Определяем узел события к которому относится контекст TLS
					 */
					switch(static_cast <uint8_t> (member->node)){
						// Если узел является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Если нужно произвести проверку
							if(member->state & state::CERTIFICATE_VERIFY)
								// Активируем проверку сертификата сервера
								::SSL_set_verify(member->ssl, SSL_VERIFY_PEER, &::verify::certificate);
							// Деактивируем проверку сертификата сервера
							else ::SSL_set_verify(member->ssl, SSL_VERIFY_NONE, nullptr);
						} break;
						// Если узел является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Если нужно произвести проверку
							if(member->state & state::CERTIFICATE_VERIFY)
								// Выполняем проверку сертификата клиента
								::SSL_set_verify(member->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &::verify::certificate);
							// Запрещаем выполнять првоерку доменного имени
							else ::SSL_set_verify(member->ssl, SSL_VERIFY_NONE, nullptr);
						} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, mode), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения режима работы TLS
 *
 * @param id идентификатор события
 * @return   режим работы TLS
 *
 */
awh::tls::Coder::mode_t awh::tls::Coder::mode(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если установлен режим работы с несколькими сертификатами TLS
					if(member->state & state::MULTICERT_MODE)
						// Возвращаем режим работы с несколькими сертификатами TLS
						return mode_t::MULTICERT;
					// Возвращаем режим работы с одним сертификатом TLS
					else return mode_t::UNICERT;
				}
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если установлен режим работы с несколькими сертификатами TLS
					if(member->state & state::MULTICERT_MODE)
						// Возвращаем режим работы с несколькими сертификатами TLS
						return mode_t::MULTICERT;
					// Возвращаем режим работы с одним сертификатом TLS
					else return mode_t::UNICERT;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем режим работы TLS по умолчанию
	return mode_t::NONE;
}
/**
 * @brief Метод установки режима работы TLS
 *
 * @param id   идентификатор события
 * @param mode режим работы TLS
 *
 */
void awh::tls::Coder::mode(const id_t id, const mode_t mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					/**
					 * Определяем режим работы TLS
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если режим работы с одним сертификатом TLS
						case static_cast <uint8_t> (mode_t::UNICERT): {
							// Снимаем режим работы с несколькими сертификатами TLS
							member->state &= ~state::MULTICERT_MODE;
							// Если узел является сервером
							if(member->node == event::node_t::SERVER){
								// Если название хоста уже установлено
								if(!member->host.empty())
									// Выполняем поиск записи в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
									::ssl::registry::spliceErase(member->proto, member->host);
							}
						} break;
						// Если режим работы с несколькими сертификатами TLS
						case static_cast <uint8_t> (mode_t::MULTICERT):
							// Устанавливаем режим работы с несколькими сертификатами TLS
							member->state |= state::MULTICERT_MODE;
						break;
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					/**
					 * Определяем режим работы TLS
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если режим работы с одним сертификатом TLS
						case static_cast <uint8_t> (mode_t::UNICERT):
							// Снимаем режим работы с несколькими сертификатами TLS
							member->state &= ~state::MULTICERT_MODE;
						break;
						// Если режим работы с несколькими сертификатами TLS
						case static_cast <uint8_t> (mode_t::MULTICERT):
							// Устанавливаем режим работы с несколькими сертификатами TLS
							member->state |= state::MULTICERT_MODE;
						break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения доменного имени сервера
 *
 * @param id идентификатор события
 * @return   доменное имя сервера
 *
 */
string awh::tls::Coder::serverNameIndication(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS):
					// Выполняем извлечение имени хоста сервера
					return reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id))->host;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL):
					// Выполняем извлечение имени хоста сервера
					return reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id))->host.name;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Метод установки доменного имени сервера
 *
 * @param id  идентификатор события
 * @param sni доменное имя сервера
 *
 */
void awh::tls::Coder::serverNameIndication(const id_t id, string_view sni) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если имя хоста сервера не пустое
		if(!sni.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						// Если узел является сервером
						if(member->node == event::node_t::SERVER){
							// Если установлен режим работы с несколькими сертификатами TLS
							if(member->state & state::MULTICERT_MODE){
								// Если название хоста уже установлено
								if(!member->host.empty())
									// Выполняем поиск записи в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
									::ssl::registry::spliceErase(member->proto, member->host);
								// Добавляем новую запись в глобальную карту сопоставления имён хостов и идентификаторов узлов TLS
								::ssl::registry::spliceEmplace(member->proto, string{sni}, id);
							}
						}
						// Устанавливаем хост для уровня защищённых сокетов
						member->host = sni;
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Устанавливаем хост для уровня защищённых сокетов
						member->host.name = sni;
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT){
							// Устанавливаем имя хоста для SNI расширения
							::SSL_set_tlsext_host_name(member->ssl, member->host.name.c_str());
							/**
							 * Устанавливаем имя хоста для проверки: SSL_set1_host сам
							 * вызывает X509_VERIFY_PARAM_set1_host с корректным strlen(host).
							 * Прямой вызов с namelen=0 в BoringSSL трактуется как пустое
							 * имя, отклоняется и отравляет параметр (param->poison=1), что
							 * приводит к X509_V_ERR_INVALID_CALL при верификации, поэтому
							 * результат проверяется по возврату SSL_set1_host
							 */
							if(::SSL_set1_host(member->ssl, member->host.name.c_str()) != 1){
								// Очищаем установленное ранее название хоста
								member->host.name.clear();
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Host verification failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::HOSTNAME_VERIFY, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, sni), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, sni), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод извлечения кэшированного билета возобновления сессии (RFC 9001 §4.6)
 *
 * @note Билеты возобновления хранятся на шаблонном контексте безопасности
 *       по ключу сервера (SNI либо адрес эндпоинта), что делает
 *       возобновление сессии прозрачным для вызывающего кода
 *
 * @param id      идентификатор шаблонного контекста безопасности
 * @param key     ключ сервера (SNI либо адрес эндпоинта)
 * @param session объект для извлечения сериализованного билета возобновления
 * @return        результат извлечения (билет найден)
 *
 */
bool awh::tls::Coder::session(const id_t id, string_view key, string & session) const noexcept {
	// Сбрасываем результат извлечения билета возобновления
	session.clear();
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден и является шаблонным контекстом безопасности
		if((pin != nullptr) && (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTS)){
			// Выполняем извлечение объекта шаблона контекста безопасности
			auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
			// Выполняем поиск билета возобновления по ключу сервера
			auto i = member->sessions.find(string{key});
			// Если билет возобновления найден
			if(i != member->sessions.end())
				// Возвращаем сохранённый билет возобновления сессии
				session = i->second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, key, session), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат извлечения билета возобновления сессии
	return !session.empty();
}
/**
 * @brief Метод сохранения билета возобновления сессии в кэш (RFC 9001 §4.6)
 *
 * @param id      идентификатор шаблонного контекста безопасности
 * @param key     ключ сервера (SNI либо адрес эндпоинта)
 * @param session сериализованный билет возобновления для сохранения
 *
 */
void awh::tls::Coder::session(const id_t id, string_view key, string_view session) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден и является шаблонным контекстом безопасности
		if((pin != nullptr) && (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTS)){
			// Выполняем извлечение объекта шаблона контекста безопасности
			auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
			// Если билет возобновления пуст - удаляем запись из кэша по ключу сервера
			if(session.empty())
				// Удаляем билет возобновления из кэша по ключу сервера
				member->sessions.erase(string{key});
			// Сохраняем билет возобновления в кэш по ключу сервера
			else member->sessions[string{key}].assign(session.begin(), session.end());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, key, session), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки адреса и порта отдалённого узла
 *
 * @param id   идентификатор события
 * @param ip   IP-адрес отдалённого узла
 * @param port порт отдалённого узла
 * @return     результат выполнения установки
 *
 */
bool awh::tls::Coder::peer(const id_t id, string_view ip, const uint16_t port) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если IP-адрес сервера не пустой и порт сервера задан верно
		if((!ip.empty()) && (port > 0)){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = "Invalid layer for set peer operation";
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::INVALID_LAYER, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Выполняем парсинг I-адреса
						if(this->_addr.parse(ip)){
							// Выполняем инициализацию объекта хоста IPv4-адреса
							member->host.peer = make_unique <net::attr_net_t> ();
							// Получаем объект хоста IPv4-адреса
							net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
							/**
							 * Выполняем определение типа IP-адреса
							 */
							switch(static_cast <uint8_t> (this->_addr.type())){
								// Для IPv4-адреса
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Устанавливаем тип адреса однорангового узла
									address->type = net::type_t::IPV4;
									// Устанавливаем порт
									address->port = port;
									// Выполняем инициализацию объекта IP-адреса
									address->ip = this->_addr.source(net_addr_t::endian_t::LITTLE);
								} break;
								// Для IPv6-адреса
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Устанавливаем тип адреса однорангового узла
									address->type = net::type_t::IPV6;
									// Устанавливаем порт
									address->port = port;
									// Выполняем инициализацию объекта IP-адреса
									address->ip = this->_addr.source(net_addr_t::endian_t::LITTLE);
								} break;
								// Для других типов адресов
								default: {
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Unsupported IP-address type");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::UNSUPPORTED_IP, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
									// Возвращаем отрицательный результат
									return false;
								}
							}
						// Если парсинг IP-адреса не выполнен
						} else {
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Failed to parse IP-address");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::UNSUPPORTED_IP, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем отрицательный результат
							return false;
						}
						// Возвращаем положительный результат
						return true;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод удаления контекста TLS
 *
 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
 * @return   результат выполнения удаления
 *
 * @note После destroy() id помечается GARBAGE_MODE; дальнейшие вызовы методов
 *       с этим id недопустимы. Физическое удаление из реестра — при refs==0.
 *
 */
bool awh::tls::Coder::destroy(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if((result = (pin != nullptr))){
			// Выполняем извлечение участника обмена защищёнными данными
			::member_t * member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (member->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является сервером
					if(member->node == event::node_t::SERVER){
						// Если название хоста уже установлено
						if(!member->host.empty())
							// Выполняем поиск записи в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
							::ssl::registry::spliceErase(member->proto, member->host);
					}
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::DESTROYED);
					// Устанавливаем режим удаления участника обмена защищёнными данными
					member->state |= ::state::GARBAGE_MODE;
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::DESTROYED);
					// Устанавливаем режим удаления участника обмена защищёнными данными
					member->state |= ::state::GARBAGE_MODE;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод завершения TLS соединения
 *
 * @param id идентификатор события
 * @return   результат выполнения завершения
 *
 */
bool awh::tls::Coder::shutdown(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for shutdown operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::INVALID_LAYER, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Выполняем завершение TLS соединения
					const int32_t shutdown = ::SSL_shutdown(member->ssl);
					// Если завершение TLS соединения выполнено
					if(shutdown >= 0)
						// Устанавливаем положительный результат
						result = true;
					// Если завершение TLS соединения не выполнено
					else {
						// Получаем код ошибки
						const int32_t error = ::SSL_get_error(member->ssl, shutdown);
						// Если ошибка связана с необходимостью повторного чтения или записи
						result = ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE));
					}
					/**
					 * Дренируем bio.write после SSL_shutdown.
					 * close_notify попадает в memory BIO и должен быть отправлен пиру немедленно.
					 */
					if(result && !::ssl::emitWriteBio(member, id))
						// Устанавливаем отрицательный результат
						result = false;
					// Возвращаем результат
					return result;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод выполнения TLS рукопожатия
 *
 * @param id идентификатор события
 * @return   результат выполнения рукопожатия
 *
 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
 *
 */
bool awh::tls::Coder::handshake(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем уровень транспортной безопасности
		 */
		switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
			// Если уровень является шаблонным контекстом безопасности
			case static_cast <uint8_t> (layer_t::CTS): {
				// Выполняем извлечение объекта шаблона контекста безопасности
				auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				// Если функция обратного вызова состояния установлена
				if(member->callback.state != nullptr)
					// Вызываем функцию обратного вызова состояния
					member->callback.state(id, state_t::FAILED);
				// Получаем текст ошибки
				const string error = "Invalid layer for handshake operation";
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::INVALID_LAYER, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
				}
			} break;
			// Если уровень является транспортной передачей данных
			case static_cast <uint8_t> (layer_t::CTL): {
				// Выполняем извлечение объекта транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				// Если рукопожатие ещё не выполнено
				if(!(result = (member->state & state::HANDSHAKE_MODE))){
					// Если рукопожатие ещё не завершено
					if(!(result = (::SSL_is_init_finished(member->ssl) == 1))){
						// Выполняем TLS рукопожатие
						const int32_t handshake = ::SSL_do_handshake(member->ssl);
						// Если рукопожатие не выполнено
						if(!(result = (handshake == 1))){
							// Получаем код ошибки
							const int32_t error = ::SSL_get_error(member->ssl, handshake);
							// Если ошибка связана с необходимостью повторного чтения или записи
							if((result = ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE) || (error == SSL_ERROR_ZERO_RETURN)))){
								// Количество прочитанных данных
								int32_t bytes = 0;
								// Количество ожидающих данных для чтения
								size_t pending = 0;
								/**
								 * Читаем все ожидающие данные из BIO буфера записи
								 */
								while((pending = ::BIO_ctrl_pending(member->bio.write)) > 0){
									// Читаем данные из BIO буфера записи
									bytes = ::BIO_read(member->bio.write, ::local::buffer, static_cast <size_t> (::min(pending, static_cast <size_t> (AWH_MAX_SSL_BUFFER_SIZE))));
									// Если данные не прочитаны (SSL_get_error здесь неприменим — это BIO, не SSL)
									if(bytes <= 0){
										// Устанавливаем отрицательный результат
										result = false;
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Handshake failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::HANDSHAKE_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr){
											// Вызываем функцию обратного вызова на неудачу рукопожатия
											member->callback.state(id, state_t::HANDSHAKE_FAILED);
											// Вызываем функцию обратного вызова на уничтожение контекста TLS
											member->callback.state(id, state_t::DESTROYED);
										}
										// Устанавливаем режим удаления участника обмена защищёнными данными
										member->state |= ::state::GARBAGE_MODE;
										// Выходим из цикла
										break;
									// Если функция обратного вызова чтения данных установлена
									} else if(member->callback.read != nullptr) {
										// Если узел является клиентом и объект для работы с отпечатками TLS установлен
										if((member->node == event::node_t::CLIENT) && (this->_fgp != nullptr)){
											// Если рукопожатие ещё не выполнено
											if(!(member->state & state::HANDSHAKE_MODE)){
												// Получаем объект браузера из шаблона контекста безопасности
												const fgp_t::browser_t & browser = this->_fgp->get(member->fid);
												// Если объект браузера получен
												if(!browser.ciphers.empty() && !browser.extensions.empty()){
													// Добавляем прочитанный фрагмент в буфер сборки ClientHello
													member->hello.insert(member->hello.end(), ::local::buffer, ::local::buffer + bytes);
													// Получаем полный размер TLS/DTLS record layer
													const size_t recordLen = ::local::tlsRecordLayerSize(member->hello.data(), member->hello.size());
													// Если record layer ещё не собран полностью — ждём следующий фрагмент
													if((recordLen == 0) || (member->hello.size() < recordLen))
														// Переходим к следующему фрагменту из BIO
														continue;
													// Выполняем применение шаблона отпечатка браузера к полному ClientHello
													const auto applied = this->_fgp->apply(member->hello.data(), recordLen, browser);
													// Если отпечаток браузера TLS успешно наложен
													if(!applied.empty()){
														// Вызываем функцию обратного вызова чтения данных
														member->callback.read(id, event_t::ENCRYPTION, applied.data(), applied.size());
													// Если отпечаток браузера TLS не наложен
													} else {
														// Если функция обратного вызова состояния установлена
														if(member->callback.state != nullptr)
															// Вызываем функцию обратного вызова на получение ошибки
															member->callback.state(id, state_t::FAILED);
														// Получаем текст ошибки
														const string error = ::ssl::error(id, "Browser fingerprint is failed to apply");
														// Если функция обратного вызова ошибки установлена
														if(member->callback.error != nullptr)
															// Вызываем функцию обратного вызова ошибки
															member->callback.error(id, error_t::FINGERPRINT_FAILED, error);
														// Если функция обратного вызова ошибки не установлена
														else {
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Записываем ошибку в лог
																this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, recordLen), log_t::flag_t::WARNING, error.c_str());
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Записываем ошибку в лог
																this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
															#endif
														}
														// Отправляем исходный ClientHello без модификации
														member->callback.read(id, event_t::ENCRYPTION, member->hello.data(), recordLen);
													}
													// Удаляем обработанный record layer из буфера сборки
													member->hello.erase(member->hello.begin(), member->hello.begin() + recordLen);
													// Если в буфере остались данные — отправляем их без модификации
													if(!member->hello.empty())
														// Вызываем функцию обратного вызова чтения данных
														member->callback.read(id, event_t::ENCRYPTION, member->hello.data(), member->hello.size());
													// Очищаем буфер сборки ClientHello
													member->hello.clear();
													// Возвращаем результат
													return result;
												}
											}
										}
										// Вызываем функцию обратного вызова чтения данных
										member->callback.read(id, event_t::ENCRYPTION, ::local::buffer, static_cast <size_t> (bytes));
									}
								}
							// Если ошибка не связана с необходимостью повторного чтения или записи
							} else {
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова на получение ошибки рукопожатия
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Handshake failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::HANDSHAKE_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr){
									// Вызываем функцию обратного вызова на неудачу рукопожатия
									member->callback.state(id, state_t::HANDSHAKE_FAILED);
									// Вызываем функцию обратного вызова на уничтожение контекста TLS
									member->callback.state(id, state_t::DESTROYED);
								}
								// Устанавливаем режим удаления участника обмена защищёнными данными
								member->state |= ::state::GARBAGE_MODE;
							}
							// Возвращаем результат
							return result;
						}
					}
					/**
					 * Дренируем bio.write ПЕРЕД тем как выставить HANDSHAKE_MODE.
					 * Когда SSL_do_handshake() возвращает 1 (или SSL_is_init_finished уже 1),
					 * BoringSSL/OpenSSL может разместить в bio.write финальные рукопожатные данные
					 * (например: ServerCCS+ServerFinished для DTLS-сервера).
					 * Если не отправить их пиру немедленно — тот никогда не завершит своё
					 * рукопожатие (особенно критично для DTLS с memory BIO, без OS-сокетов).
					 */
					if(::BIO_ctrl_pending(member->bio.write) > 0)
						// Дренируем bio.write через общий helper
						::ssl::emitWriteBio(member, id);
					// Устанавливаем режим выполненного рукопожатия
					member->state |= state::HANDSHAKE_MODE;
					// Очищаем буфер сборки ClientHello
					member->hello.clear();
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						// Длина извлекаемого протокола
						uint32_t length = 0;
						// Название извлекаемого протокола
						const uint8_t * proto = nullptr;
						// Выполняем извлечение активного протокола
						::SSL_get0_alpn_selected(member->ssl, &proto, &length);
						// Размер и индекс протокола
						uint8_t size = 0, index = 0;
						/**
						 * Выполняем перебор всего буфера поддерживаемых ALPN-протоколов
						 */
						for(uint8_t i = 0; i < member->alpn.buffer.size(); i++){
							// Извлекаем размер протокола
							size = member->alpn.buffer[i];
							// Если размер протокола некорректен или выходит за пределы буфера
							if((size == 0) || ((static_cast <size_t> (i) + 1 + size) > member->alpn.buffer.size()))
								// Выходим из цикла
								break;
							// Если размер протокола совпадает с длиной извлекаемого протокола
							if((proto != nullptr) && (size == static_cast <uint8_t> (length))){
								// Если протокол совпадает с извлекаемым протоколом
								if(::memcmp(&member->alpn.buffer[i + 1], proto, length) == 0){
									// Если индекс протокола не выходит за пределы списка
									if(index < member->alpn.ids.size())
										// Устанавливаем активный протокол
										member->alpn.id = member->alpn.ids[index];
									// Выход из цикла
									break;
								}
							}
							// Смещаем индекс на размер протокола
							i += size;
							// Увеличиваем индекс протокола
							index++;
						}
					}
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::HANDSHAKED);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод повторной передачи данных
 *
 * @param id идентификатор события
 * @return   результат выполнения повторной передачи
 *
 */
bool awh::tls::Coder::retransmit(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for retransmit operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::INVALID_LAYER, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					/**
					 * Дослать данные, задержавшиеся в BIO буфере записи: доставка по UDP
					 * ненадёжна (DTLS/QUIC), и повторная передача снимает застрявшие записи.
					 * Пустой буфер - не ошибка, а успешный холостой вызов: слать нечего,
					 * всё уже отправлено. Ошибкой считается лишь сбой чтения из BIO, который
					 * emitWriteBio() и отражает, возвращая false
					 */
					result = ::ssl::emitWriteBio(member, id);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения нативного контекста криптографической библиотеки
 *
 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
 * @return   нативный контекст криптографической библиотеки либо nullptr
 *
 */
ssl_ctx_st * awh::tls::Coder::native(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS):
					// Выводим нативный контекст шаблона контекста безопасности
					return reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id))->ctx;
				// Если уровень является транспортным уровнем передачи
				case static_cast <uint8_t> (layer_t::CTL):
					// Выводим нативный контекст транспортного уровня передачи
					return reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id))->ctx;
			}
		}
	/**
	 * Выполняем перехват ошибки
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим пустой результат
	return nullptr;
}
/**
 * @brief Метод создания идентификатора транспортного уровня
 *
 * @param id идентификатор шаблона контекста безопасности
 * @return   идентификатор транспортного уровня
 *
 */
awh::tls::Coder::id_t awh::tls::Coder::transport(const id_t id) noexcept {
	// Переменная результата
	id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto cts = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Создаём новый транспортный уровень передачи и добавляем его в контейнер
					auto ret = ::ssl::registry::emplace(::make_unique <::ctl_t> ());
					// Извлекаем объект транспортного уровня передачи
					::ctl_t * member = awh_cast <::ctl_t *> ((* ret).get());
					{
						// Устанавливаем контекст TLS
						member->ctx = cts->ctx;
						// Устанавливаем список отзыва сертификатов
						member->crl = &cts->crl;
						// Устанавливаем идентификатор цифрового отпечатка браузера
						member->fid = cts->fid;
						// Устанавливаем ALPN-протоколов
						member->alpn = cts->alpn;
						// Устанавливаем тип узла события
						member->node = cts->node;
						// Устанавливаем тип протокола события
						member->proto = cts->proto;
						// Устанавливаем объект состояния
						member->state = cts->state;
						// Сохраняем итератор уровня защищённых сокетов
						member->iterator = ret;
						// Выполняем получение идентификатора контекста TLS
						result = static_cast <id_t> (reinterpret_cast <uintptr_t> ((* ret).get()));
						// Создаем SSL объект
						member->ssl = ::SSL_new(cts->ctx);
						// Если объект не создан
						if(member->ssl == nullptr){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(result, "Could not create TLS session object");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::TLS_SESSION_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Удаляем контекст TLS из контейнера уровней защищённых сокетов
							member->erase(::__awh_ssl_members__);
							// Выходим
							return 0;
						}
						// Привязываем текущий объект TLS к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[0], (* ret).get());
						// Привязываем текущий объект фреймворка к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[1], const_cast <fmk_t *> (this->_fmk));
						// Привязываем текущий объект лога к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[2], const_cast <log_t *> (this->_log));
						// Привязываем текущий объект компрессора к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[3], const_cast <compressor::block_t *> (&this->_compressor));
						// Создаём объект BIO для чтения
						member->bio.read = ::BIO_new(::BIO_s_mem());
						// Создаём объект BIO для записи
						member->bio.write = ::BIO_new(::BIO_s_mem());
						// Если один из объектов BIO не создан
						if((member->bio.read == nullptr) || (member->bio.write == nullptr)){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(result, "Create BIO is failed");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::BIO_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Если объект BIO для чтения создан
							if(member->bio.read != nullptr)
								// Освобождаем объект BIO для чтения
								::BIO_free(member->bio.read);
							// Если объект BIO для записи создан
							if(member->bio.write != nullptr)
								// Освобождаем объект BIO для записи
								::BIO_free(member->bio.write);
							// Удаляем контекст TLS из контейнера уровней защищённых сокетов
							member->erase(::__awh_ssl_members__);
							// Выходим
							return 0;
						}
						// Если протокол подключения UDP
						if(member->proto == event::protocol_t::UDP){
							// Устанавливаем MTU (обычно 1200–1400 для UDP)
							::BIO_ctrl(member->bio.read, BIO_CTRL_DGRAM_SET_MTU, 1200, nullptr);
							::BIO_ctrl(member->bio.write, BIO_CTRL_DGRAM_SET_MTU, 1200, nullptr);
						}
						// Привязываем объекты BIO к SSL объекту
						::SSL_set_bio(member->ssl, member->bio.read, member->bio.write);
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Устанавливаем режим клиента для SSL объекта
								::SSL_set_connect_state(member->ssl);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Устанавливаем режим сервера для SSL объекта
								::SSL_set_accept_state(member->ssl);
							break;
						}
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT){
							// Устанавливаем имя хоста для SNI расширения
							::SSL_set_tlsext_host_name(member->ssl, cts->host.c_str());
							// Устанавливаем имя хоста для проверки
							::SSL_set1_host(member->ssl, cts->host.c_str());
							/**
							 * SSL_set1_host выше уже вызвал X509_VERIFY_PARAM_set1_host
							 * с корректным strlen(host). Повторный вызов с namelen=0 в BoringSSL
							 * интерпретируется как пустое имя (не как strlen!) и устанавливает
							 * param->poison=1, что приводит к X509_V_ERR_INVALID_CALL при верификации,
							 * поэтому здесь он не повторяется
							 */
						}
						// Копируем список ECHConfig из шаблона контекста безопасности
						member->ech = cts->ech;
						// Если узел является клиентом и список ECHConfig не пустой — применяем для шифрования SNI
						if((member->node == event::node_t::CLIENT) && !member->ech.empty())
							// Устанавливаем список ECHConfig для зашифрованного ClientHello
							::SSL_set1_ech_config_list(member->ssl, &member->ech[0], member->ech.size());
						// Устанавливаем хост для уровня защищённых сокетов
						member->host.name = cts->host;
						// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
						::ssl::registry::add(result);
						// Если идентификатор цифрового отпечатка браузера установлен — применяем CTL-настройки отпечатка
						if((member->node == event::node_t::CLIENT) && (member->fid != 0) && (this->_fgp != nullptr))
							// Активируем поддержку наложения цифрового отпечатка браузера на TLS-соединение
							this->browser(result, member->fid);
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto cts = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Создаём новый транспортный уровень передачи и добавляем его в контейнер
					auto ret = ::ssl::registry::emplace(::make_unique <::ctl_t> ());
					// Извлекаем объект транспортного уровня передачи
					::ctl_t * member = awh_cast <::ctl_t *> ((* ret).get());
					{
						// Устанавливаем контекст TLS
						member->ctx = cts->ctx;
						// Устанавливаем список отзыва сертификатов
						member->crl = cts->crl;
						// Устанавливаем идентификатор цифрового отпечатка браузера
						member->fid = cts->fid;
						// Устанавливаем ALPN-протоколов
						member->alpn = cts->alpn;
						// Устанавливаем тип узла события
						member->node = cts->node;
						// Устанавливаем тип протокола события
						member->proto = cts->proto;
						// Устанавливаем объект состояния
						member->state = cts->state;
						// Сохраняем итератор уровня защищённых сокетов
						member->iterator = ret;
						// Выполняем получение идентификатора контекста TLS
						result = static_cast <id_t> (reinterpret_cast <uintptr_t> ((* ret).get()));
						// Создаем SSL объект
						member->ssl = ::SSL_new(cts->ctx);
						// Если объект не создан
						if(member->ssl == nullptr){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(result, "Could not create TLS session object");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::TLS_SESSION_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Удаляем контекст TLS из контейнера уровней защищённых сокетов
							member->erase(::__awh_ssl_members__);
							// Выходим
							return 0;
						}
						// Привязываем текущий объект TLS к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[0], (* ret).get());
						// Привязываем текущий объект фреймворка к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[1], const_cast <fmk_t *> (this->_fmk));
						// Привязываем текущий объект лога к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[2], const_cast <log_t *> (this->_log));
						// Привязываем текущий объект компрессора к SSL объекту
						::SSL_set_ex_data(member->ssl, ::__awh_ssl_index__[3], const_cast <compressor::block_t *> (&this->_compressor));
						// Создаём объект BIO для чтения
						member->bio.read = ::BIO_new(::BIO_s_mem());
						// Создаём объект BIO для записи
						member->bio.write = ::BIO_new(::BIO_s_mem());
						// Если один из объектов BIO не создан
						if((member->bio.read == nullptr) || (member->bio.write == nullptr)){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(result, "Create BIO is failed");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::BIO_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Если объект BIO для чтения создан
							if(member->bio.read != nullptr)
								// Освобождаем объект BIO для чтения
								::BIO_free(member->bio.read);
							// Если объект BIO для записи создан
							if(member->bio.write != nullptr)
								// Освобождаем объект BIO для записи
								::BIO_free(member->bio.write);
							// Удаляем контекст TLS из контейнера уровней защищённых сокетов
							member->erase(::__awh_ssl_members__);
							// Выходим
							return 0;
						}
						// Если протокол подключения UDP
						if(member->proto == event::protocol_t::UDP){
							// Устанавливаем MTU (обычно 1200–1400 для UDP)
							::BIO_ctrl(member->bio.read, BIO_CTRL_DGRAM_SET_MTU, 1200, nullptr);
							::BIO_ctrl(member->bio.write, BIO_CTRL_DGRAM_SET_MTU, 1200, nullptr);
						}
						// Привязываем объекты BIO к SSL объекту
						::SSL_set_bio(member->ssl, member->bio.read, member->bio.write);
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT):
								// Устанавливаем режим клиента для SSL объекта
								::SSL_set_connect_state(member->ssl);
							break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER):
								// Устанавливаем режим сервера для SSL объекта
								::SSL_set_accept_state(member->ssl);
							break;
						}
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT){
							// Устанавливаем имя хоста для SNI расширения
							::SSL_set_tlsext_host_name(member->ssl, cts->host.name.c_str());
							/**
							 * Устанавливаем имя хоста для проверки: SSL_set1_host сам
							 * вызывает X509_VERIFY_PARAM_set1_host с корректным strlen(host).
							 * Прямой вызов с namelen=0 в BoringSSL трактуется как пустое
							 * имя, отклоняется и отравляет параметр (param->poison=1), что
							 * приводит к X509_V_ERR_INVALID_CALL при верификации, поэтому
							 * результат проверяется по возврату SSL_set1_host
							 */
							if(::SSL_set1_host(member->ssl, cts->host.name.c_str()) != 1){
								// Если функция обратного вызова состояния установлена
								if(cts->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									cts->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Host verification failed");
								// Если функция обратного вызова ошибки установлена
								if(cts->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									cts->callback.error(id, error_t::HOSTNAME_VERIFY, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, cts->host.name), log_t::flag_t::WARNING, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
									#endif
								}
							}
						}
						// Копируем список ECHConfig из транспортного уровня
						member->ech = cts->ech;
						// Если узел является клиентом и список ECHConfig не пустой — применяем для шифрования SNI
						if((member->node == event::node_t::CLIENT) && !member->ech.empty())
							// Устанавливаем список ECHConfig для зашифрованного ClientHello
							::SSL_set1_ech_config_list(member->ssl, &member->ech[0], member->ech.size());
						// Устанавливаем хост для уровня защищённых сокетов
						member->host.name = cts->host.name;
						// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
						::ssl::registry::add(result);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод создания идентификатора шаблона контекста безопасности
 *
 * @param node  тип узла события
 * @param proto тип протокола события
 * @return      идентификатор шаблона контекста безопасности
 *
 */
awh::tls::Coder::id_t awh::tls::Coder::context(const event::node_t node, const event::protocol_t proto) noexcept {
	// Переменная результата
	id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Создаём новый уровень защищённых сокетов и добавляем его в контейнер
		auto ret = ::ssl::registry::emplace(::make_unique <::cts_t> ());
		// Извлекаем объект шаблона контекста безопасности
		::cts_t * member = awh_cast <::cts_t *> ((* ret).get());
		// Устанавливаем тип узла события
		member->node = node;
		// Устанавливаем тип протокола события
		member->proto = proto;
		// Сохраняем итератор уровня защищённых сокетов
		member->iterator = ret;
		// Устанавливаем режим проверки сертификата
		member->state |= state::CERTIFICATE_VERIFY;
		// Выполняем получение идентификатора контекста TLS
		result = static_cast <id_t> (reinterpret_cast <uintptr_t> ((* ret).get()));
		/**
		 * Определяем узел события к которому относится контекст TLS
		 */
		switch(static_cast <uint8_t> (node)){
			// Если узел является клиентом
			case static_cast <uint8_t> (event::node_t::CLIENT): {
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP):
						// Устанавливаем режим клиента для контекста DTLS
						member->ctx = ::SSL_CTX_new(::DTLS_client_method());
					break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
					/**
					 * Если протокол подключения QUIC: транспорт работает поверх UDP,
					 * но слой записей DTLS не задействует - криптографию хендшейка
					 * QUIC переносит собственными пакетами (RFC 9001 §4), поэтому
					 * контекст создаётся методом TLS
					 */
					case static_cast <uint8_t> (event::protocol_t::QUIC):
						// Устанавливаем режим клиента для контекста TLS
						member->ctx = ::SSL_CTX_new(::TLS_client_method());
					break;
				}
				// Если контекст не создан
				if(member->ctx == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Context is not initialization");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					member->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Устанавливаем опции запроса
				::SSL_CTX_set_options(
					/**
					 * Устанавливаем объект контекста TLS для которого устанавливаем опции запроса
					 */
					member->ctx,
					/**
					 * 1. Совместимость и безопасность по умолчанию
					 */
					SSL_OP_ALL |
					/**
					 * 2. Отключить устаревшие и небезопасные протоколы
					 */
					SSL_OP_NO_SSLv2 |
					SSL_OP_NO_SSLv3 |
					SSL_OP_NO_TLSv1 |
					SSL_OP_NO_TLSv1_1 |
					/**
					 * 3. Защита от атак
					 */
					SSL_OP_NO_COMPRESSION | // CRIME / BREACH
					/**
					 * 5. Дополнительные меры (опционально, но рекомендованы)
					 */
					SSL_OP_NO_RENEGOTIATION | // Отключить ренеготиацию вообще (OpenSSL 1.1.1+)
					SSL_OP_SINGLE_DH_USE |    // Свежие DH-ключи (для DHE)
					SSL_OP_SINGLE_ECDH_USE    // Свежие ECDH-ключи (для ECDHE)
				);
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Устанавливаем минимально-возможную версию DTLS
						::SSL_CTX_set_min_proto_version(member->ctx, DTLS1_VERSION);
						// Устанавливаем максимально-возможную версию DTLS
						::SSL_CTX_set_max_proto_version(member->ctx, DTLS1_2_VERSION);
					} break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP): {
						// Устанавливаем минимально-возможную версию TLS
						::SSL_CTX_set_min_proto_version(member->ctx, TLS1_2_VERSION);
						// Устанавливаем максимально-возможную версию TLS
						::SSL_CTX_set_max_proto_version(member->ctx, TLS1_3_VERSION);
					} break;
					/**
					 * Если протокол подключения QUIC: транспорт допускает
					 * исключительно TLS версии 1.3 (RFC 9001 §4.2), поэтому нижняя
					 * граница диапазона совпадает с верхней
					 */
					case static_cast <uint8_t> (event::protocol_t::QUIC): {
						// Устанавливаем минимально-возможную версию TLS
						::SSL_CTX_set_min_proto_version(member->ctx, TLS1_3_VERSION);
						// Устанавливаем максимально-возможную версию TLS
						::SSL_CTX_set_max_proto_version(member->ctx, TLS1_3_VERSION);
					} break;
				}
				/**
				 * Список групп обмена ключами не сужаем: в TLS 1.3 группа
				 * согласуется расширением supported_groups, и список библиотеки
				 * по умолчанию и шире, и современнее любого заданного здесь.
				 * Наследие OpenSSL 1.0 в виде SSL_CTX_set_tmp_ecdh здесь не
				 * годится: оно не "разрешает ECDH", а заменяет весь список
				 * единственной кривой, из-за чего клиенту, приславшему долю
				 * ключа другой группы, приходится отвечать HelloRetryRequest.
				 * Это лишний круговой обход на каждом соединении, а на стороне
				 * QUIC ещё и запрет ранних данных (RFC 8446 §4.2.10).
				 * Сузить список при необходимости позволяет метод groups()
				 */
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Устанавливаем функцию обратного вызова для обработки сообщений TLS
					::SSL_CTX_set_msg_callback(member->ctx, &::ssl::message);
					// Устанавливаем аргумент функции обратного вызова для обработки сообщений TLS
					::SSL_CTX_set_msg_callback_arg(member->ctx, (* ret).get());
				#endif
				// Получаем данные стора
				X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
				// Если стор не получен
				if(store == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Get x509 store is failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(
							static_cast <uint16_t> (node),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				// Если стор получен
				} else {
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Проверяем существует ли путь
						if(!::ssl::addCertToStore(store, "CA", this->_log) ||
						   !::ssl::addCertToStore(store, "ROOT", this->_log) ||
						   !::ssl::addCertToStore(store, "AuthRoot", this->_log))
							/**
							 * Возвращаем нулевой идентификатор - признак неудачи, принятый
							 * в этом методе прочими путями выхода. Прежде здесь стоял
							 * возврат без значения, и обнаружилось это лишь первой сборкой
							 * под MinGW64: ветка эта принадлежит MS Windows и компилятором
							 * не читалась ни разу
							 */
							return 0;
					#endif
					// Если стор не устанавливается, тогда выводим ошибку
					if(::X509_STORE_set_default_paths(store) == 0){
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set default paths for x509 store is not allowed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				}
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP):
						// Устанавливаем, что мы не можем читать больше байтов чем необходимо
						::SSL_CTX_set_read_ahead(member->ctx, 0);
					break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
						// Устанавливаем, что мы должны читать как можно больше входных байтов
						::SSL_CTX_set_read_ahead(member->ctx, 1);
					break;
				}
				// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
				::SSL_CTX_set_mode(member->ctx, SSL_MODE_RELEASE_BUFFERS);
				// Устанавливаем пути по умолчанию для проверки сертификатов
				::SSL_CTX_set_default_verify_paths(member->ctx);
				// Устанавливаем проверку сертификата сервера
				::SSL_CTX_set_verify(member->ctx, SSL_VERIFY_PEER, &::verify::certificate);
				// Выполняем проверку всех дочерних сертификатов
				::SSL_CTX_set_cert_verify_callback(member->ctx, &::verify::hostname, (* ret).get());
				// Привязываем текущий объект TLS к SSL_CTX объекту
				::SSL_CTX_set_ex_data(member->ctx, ::__awh_ssl_index__[4], (* ret).get());
				// Привязываем текущий объект фреймворка к SSL_CTX объекту
				::SSL_CTX_set_ex_data(member->ctx, ::__awh_ssl_index__[5], const_cast <fmk_t *> (this->_fmk));
				// Привязываем текущий объект лога к SSL_CTX объекту
				::SSL_CTX_set_ex_data(member->ctx, ::__awh_ssl_index__[6], const_cast <log_t *> (this->_log));
				// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
				::ssl::registry::add(result);
			} break;
			// Если узел является сервером
			case static_cast <uint8_t> (event::node_t::SERVER): {
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP):
						// Устанавливаем режим клиента для контекста TLS
						member->ctx = ::SSL_CTX_new(::DTLS_server_method());
					break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
					/**
					 * Если протокол подключения QUIC: транспорт работает поверх UDP,
					 * но слой записей DTLS не задействует - криптографию хендшейка
					 * QUIC переносит собственными пакетами (RFC 9001 §4), поэтому
					 * контекст создаётся методом TLS
					 */
					case static_cast <uint8_t> (event::protocol_t::QUIC):
						// Устанавливаем режим клиента для контекста TLS
						member->ctx = ::SSL_CTX_new(::TLS_server_method());
					break;
				}
				// Если контекст не создан
				if(member->ctx == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Context is not initialization");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					member->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Устанавливаем опции запроса
				::SSL_CTX_set_options(
					/**
					 * Устанавливаем объект контекста TLS для которого устанавливаем опции запроса
					 */
					member->ctx,
					/**
					 * 1. Совместимость и безопасность по умолчанию
					 */
					SSL_OP_ALL |
					/**
					 * 2. Отключить устаревшие и небезопасные протоколы
					 */
					SSL_OP_NO_SSLv2 |
					SSL_OP_NO_SSLv3 |
					SSL_OP_NO_TLSv1 |
					SSL_OP_NO_TLSv1_1 |
					/**
					 * 3. Защита от атак
					 */
					SSL_OP_NO_COMPRESSION |                         // CRIME / BREACH
					SSL_OP_CIPHER_SERVER_PREFERENCE |               // Сервер выбирает шифр
					SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION | // Безопасная ренеготиация
					/**
					 * 4. Дополнительные меры (опционально, но рекомендованы)
					 */
					SSL_OP_NO_RENEGOTIATION | // Отключить ренеготиацию вообще (OpenSSL 1.1.1+)
					SSL_OP_SINGLE_DH_USE |    // Свежие DH-ключи (для DHE)
					SSL_OP_SINGLE_ECDH_USE    // Свежие ECDH-ключи (для ECDHE)
				);
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Устанавливаем минимально-возможную версию DTLS
						::SSL_CTX_set_min_proto_version(member->ctx, DTLS1_VERSION);
						// Устанавливаем максимально-возможную версию DTLS
						::SSL_CTX_set_max_proto_version(member->ctx, DTLS1_2_VERSION);
					} break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP): {
						// Устанавливаем минимально-возможную версию TLS
						::SSL_CTX_set_min_proto_version(member->ctx, TLS1_2_VERSION);
						// Устанавливаем максимально-возможную версию TLS
						::SSL_CTX_set_max_proto_version(member->ctx, TLS1_3_VERSION);
					} break;
					/**
					 * Если протокол подключения QUIC: транспорт допускает
					 * исключительно TLS версии 1.3 (RFC 9001 §4.2), поэтому нижняя
					 * граница диапазона совпадает с верхней
					 */
					case static_cast <uint8_t> (event::protocol_t::QUIC): {
						// Устанавливаем минимально-возможную версию TLS
						::SSL_CTX_set_min_proto_version(member->ctx, TLS1_3_VERSION);
						// Устанавливаем максимально-возможную версию TLS
						::SSL_CTX_set_max_proto_version(member->ctx, TLS1_3_VERSION);
					} break;
				}
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Устанавливаем функцию обратного вызова для обработки сообщений TLS
					::SSL_CTX_set_msg_callback(member->ctx, &::ssl::message);
					// Устанавливаем аргумент функции обратного вызова для обработки сообщений TLS
					::SSL_CTX_set_msg_callback_arg(member->ctx, (* ret).get());
				#endif
				/**
				 * Список групп обмена ключами не сужаем: в TLS 1.3 группа
				 * согласуется расширением supported_groups, и список библиотеки
				 * по умолчанию и шире, и современнее любого заданного здесь.
				 * Наследие OpenSSL 1.0 в виде SSL_CTX_set_tmp_ecdh здесь не
				 * годится: оно не "разрешает ECDH", а заменяет весь список
				 * единственной кривой, из-за чего клиенту, приславшему долю
				 * ключа другой группы, приходится отвечать HelloRetryRequest.
				 * Это лишний круговой обход на каждом соединении, а на стороне
				 * QUIC ещё и запрет ранних данных (RFC 8446 §4.2.10).
				 * Сузить список при необходимости позволяет метод groups()
				 */
				// Выполняем установку идентификатора сессии
				if(::SSL_CTX_set_session_id_context(member->ctx, reinterpret_cast <const uint8_t *> (&result), sizeof(result)) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Failed to set session ID");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					member->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				/**
				 * Автосогласование групп обмена ключами включать не требуется:
				 * SSL_CTX_set_ecdh_auto - наследие OpenSSL 1.0.2, где оно
				 * разрешало эфемерный ECDH. С TLS 1.1.0 автовыбор всегда включён,
				 * и во всех поддерживаемых библиотеках вызов раскрывается в
				 * константу (в BoringSSL - в 1, в OpenSSL 3 - в проверку
				 * аргумента), то есть ничего не устанавливает. Список групп
				 * при необходимости задаётся методом groups()
				 */
				// Получаем данные стора
				X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
				// Если стор не получен
				if(store == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Get x509 store is failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(
							static_cast <uint16_t> (node),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				// Если стор получен
				} else {
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Проверяем существует ли путь
						if(!::ssl::addCertToStore(store, "CA", this->_log) ||
						   !::ssl::addCertToStore(store, "ROOT", this->_log) ||
						   !::ssl::addCertToStore(store, "AuthRoot", this->_log))
							/**
							 * Возвращаем нулевой идентификатор - признак неудачи, принятый
							 * в этом методе прочими путями выхода. Прежде здесь стоял
							 * возврат без значения, и обнаружилось это лишь первой сборкой
							 * под MinGW64: ветка эта принадлежит MS Windows и компилятором
							 * не читалась ни разу
							 */
							return 0;
					#endif
					// Если стор не устанавливается, тогда выводим ошибку
					if(::X509_STORE_set_default_paths(store) == 0){
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set default paths for x509 store is not allowed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				}
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP):
						// Устанавливаем, что мы не можем читать больше байтов чем необходимо
						::SSL_CTX_set_read_ahead(member->ctx, 0);
					break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
						// Устанавливаем, что мы должны читать как можно больше входных байтов
						::SSL_CTX_set_read_ahead(member->ctx, 1);
					break;
				}
				// Устанавливаем флаг для корректного завершения сеанса TLS
				::SSL_CTX_set_quiet_shutdown(member->ctx, 0);
				// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
				::SSL_CTX_set_mode(member->ctx, SSL_MODE_RELEASE_BUFFERS);
				// Выполняем отключение SSL-кеша
				::SSL_CTX_set_session_cache_mode(member->ctx, SSL_SESS_CACHE_OFF);
				// Запускаем кэширование сессий TLS на стороне сервера
				// ::SSL_CTX_set_session_cache_mode(member->ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
				// Устанавливаем пути по умолчанию для проверки сертификатов
				::SSL_CTX_set_default_verify_paths(member->ctx);
				// Устанавливаем проверку сертификата сервера
				::SSL_CTX_set_verify(member->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &::verify::certificate);
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						/**
						 * В BoringSSL HelloVerifyRequest на серверной стороне не поддерживается —
						 * генерация cookie и встраивание в них IP:port клиента невозможна.
						 * Защита от DoS реализуется на уровне приложения через SSL_CTX_set_dos_protection_cb:
						 * соединение разрешается только если пир явно зарегистрирован через tls.peer().
						 */
						::SSL_CTX_set_dos_protection_cb(member->ctx, [](const SSL_CLIENT_HELLO * hello) noexcept -> int32_t {
							// Получаем объект транспортного уровня из SSL* ex_data
							auto ctl = reinterpret_cast <::ctl_t *> (::SSL_get_ex_data(hello->ssl, ::__awh_ssl_index__[0]));
							// Разрешаем только явно зарегистрированные пиры (peer установлен через tls.peer())
							if((ctl == nullptr) || (ctl->host.peer == nullptr))
								// Выходим с ошибкой
								return 0;
							// Пир зарегистрирован — разрешаем соединение
							return 1;
						});
					} break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP): {
						/**
						 * Отдельной настройки cookie для TCP не требуется: stateless-cookie
						 * относятся к DTLS, а их API SSL_CTX_set_stateless_cookie_*_cb
						 * в BoringSSL отсутствует
						 */
					} break;
				}
				// Устанавливаем функцию обратного вызова для переключения протокола на другой
				::SSL_CTX_set_alpn_select_cb(member->ctx, &::ssl::serverNextProtoSelect, (* ret).get());
				// Устанавливаем функцию обратного вызова для обработки SNI
				::SSL_CTX_set_tlsext_servername_callback(member->ctx, &::verify::matchSNI);
				// Устанавливаем аргумент функции обратного вызова для обработки SNI
				::SSL_CTX_set_tlsext_servername_arg(member->ctx, (* ret).get());
				// Привязываем текущий объект TLS к SSL_CTX объекту
				::SSL_CTX_set_ex_data(member->ctx, ::__awh_ssl_index__[4], (* ret).get());
				// Привязываем текущий объект фреймворка к SSL_CTX объекту
				::SSL_CTX_set_ex_data(member->ctx, ::__awh_ssl_index__[5], const_cast <fmk_t *> (this->_fmk));
				// Привязываем текущий объект лога к SSL_CTX объекту
				::SSL_CTX_set_ex_data(member->ctx, ::__awh_ssl_index__[6], const_cast <log_t *> (this->_log));
				// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
				::ssl::registry::add(result);
			} break;
			// Во всех остальных случаях
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"Invalid event node type", __PRETTY_FUNCTION__,
						make_tuple(
							static_cast <uint16_t> (node),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Invalid event node type", log_t::flag_t::WARNING);
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
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					static_cast <uint16_t> (node),
					static_cast <uint16_t> (proto)
				), log_t::flag_t::CRITICAL, error.what()
			);
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
 * @brief Метод получения сериализованного ECHConfigList для публикации в DNS
 *
 * @param id идентификатор события
 * @return   байты ECHConfigList для DNS HTTPS-записи (для сервера)
 *           или байты ECHConfigList полученные из DNS (для клиента).
 *           Возвращает пустой вектор если ECH не был настроен.
 *
 * @details Метод возвращает ECHConfigList, сохранённый в памяти после вызова
 *          setKeysECH(). Для сервера содержит публичные ECHConfig, которые
 *          нужно опубликовать через DNS HTTPS-запись (поле eckparam), чтобы
 *          клиенты могли найти публичный ключ и зашифровать ClientHello.
 *
 *          Пример использования для сервера:
 *          @code
 *          // Шаг 1: настраиваем ECH на сервере
 *          coder.serverNameIndication(ctx, "example.com");
 *          coder.setKeysECH(ctx, {}); // {} = автогенерация ключа
 *
 *          // Шаг 2: получаем ECHConfigList для DNS
 *          vector <uint8_t> echDns = coder.getKeysECH(ctx);
 *          // Опубликовать echDns через DNS HTTPS-запись
 *          @endcode
 *
 *          Пример использования для клиента:
 *          @code
 *          // ECHConfigList из DNS HTTPS-записи был передан через setKeysECH().
 *          // getKeysECH() возвращает те же байты, которые были сохранены.
 *          vector <uint8_t> echDns = coder.getKeysECH(ctx);
 *          @endcode
 *
 */
vector <uint8_t> awh::tls::Coder::getKeysECH(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS):
					// Возвращаем сохранённый ECHConfigList из шаблона контекста
					return reinterpret_cast <const ::cts_t *> (static_cast <uintptr_t> (id))->ech;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL):
					// Возвращаем сохранённый ECHConfigList из транспортного уровня
					return reinterpret_cast <const ::ctl_t *> (static_cast <uintptr_t> (id))->ech;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустой вектор
	return {};
}
/**
 * @brief Метод установки ключей EncryptedClientHello (ECH)
 *
 * @param id   идентификатор события
 * @param keys ключи EncryptedClientHello (ECH)
 * @return     результат выполнения установки
 *
 * @details Метод поддерживается только в BoringSSL. Поведение зависит от
 *          типа узла (CLIENT/SERVER) и уровня (CTS/CTL).
 *
 *          <b>Клиент (CTS или CTL):</b> @p keys — сериализованный ECHConfigList
 *          из DNS HTTPS-записи. Сохраняется и применяется к каждому новому
 *          SSL-соединению при вызове transport().
 *
 *          <b>Сервер (CTS):</b> @p keys — 32-байтовый приватный X25519 ключ.
 *          Если @p keys пустой — ключ генерируется автоматически.
 *          После успешной установки публичная часть ECHConfigList
 *          доступна через getKeysECH() для публикации в DNS.
 *          Для сервера CTL-уровень ECH не поддерживается; настройка
 *          должна выполняться через CTS до создания транспорта.
 *
 *          Пример для клиента:
 *          @code
 *          // ECHConfigList из DNS HTTPS-записи (поле eckparam)
 *          vector <uint8_t> echFromDns = fetchEchFromDns("example.com");
 *          coder.setKeysECH(ctx, echFromDns); // ctx = CTS клиента
 *          // Теперь каждый transport(ctx) будет шифровать ClientHello
 *          @endcode
 *
 *          Пример для сервера:
 *          @code
 *          // {} = автогенерация ключа; либо передать 32-байтовый X25519 приватный ключ
 *          coder.serverNameIndication(ctx, "example.com");
 *          coder.setKeysECH(ctx, {}); // ctx = CTS сервера
 *          // Получить ECHConfigList для DNS:
 *          vector <uint8_t> forDns = coder.getKeysECH(ctx);
 *          @endcode
 *
 */
bool awh::tls::Coder::setKeysECH(const id_t id, const vector <uint8_t> & keys) noexcept {
	// Если ключи не пустые
	if(!keys.empty())
		// Выполняем установку ключей EncryptedClientHello (ECH)
		return this->setKeysECH(id, &keys[0], keys.size());
	// Выполняем установку ключей EncryptedClientHello (ECH) с пустым вектором
	else return this->setKeysECH(id, nullptr, 0);
	// Возвращаем результат по умолчанию
	return false;
}
/**
 * @brief Метод установки ключей EncryptedClientHello (ECH)
 *
 * @param id   идентификатор события
 * @param keys ключи EncryptedClientHello (ECH)
 * @param size размер ключей EncryptedClientHello (ECH)
 * @return     результат выполнения установки
 *
 * @details Перегрузка setKeysECH() для сырого указателя вместо vector.
 *          Поведение идентично первой перегрузке; см. ее документацию.
 *
 *          Для сервера если @p keys равен nullptr или @p size равен 0 —
 *          ключ генерируется автоматически.
 *
 */
bool awh::tls::Coder::setKeysECH(const id_t id, const uint8_t * keys, const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					/**
					 * Определяем узел события к которому относится контекст TLS
					 */
					switch(static_cast <uint8_t> (member->node)){
						// Если узел является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							/**
							 * Для клиента: keys содержит ECHConfigList (сериализованный список из DNS HTTPS-записи).
							 * Сохраняем список — он будет применён к каждому SSL* при вызове transport().
							 */
							if((result = ((keys != nullptr) && (size > 0))))
								// Сохраняем список ECHConfig в шаблоне контекста
								member->ech.assign(keys, keys + size);
						} break;
						// Если узел является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							/**
							 * Для сервера: keys может содержать 32-байтовый приватный X25519 ключ.
							 * Если keys пустой — генерируется новая пара ключей автоматически.
							 * Создаём объект HPKE ключа.
							 */
							EVP_HPKE_KEY * key = ::EVP_HPKE_KEY_new();
							// Если объект HPKE ключа не создан
							if(key == nullptr){
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Failed to allocate HPKE key for ECH");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::SNI_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим
								break;
							}
							// Если приватный ключ передан в bytes — инициализируем из него
							if((keys != nullptr) && (size > 0))
								// Инициализируем HPKE ключ из переданных байтов приватного X25519 ключа
								result = (::EVP_HPKE_KEY_init(key, ::EVP_hpke_x25519_hkdf_sha256(), keys, size) == 1);
							// Иначе генерируем новую пару ключей X25519
							else result = (::EVP_HPKE_KEY_generate(key, ::EVP_hpke_x25519_hkdf_sha256()) == 1);
							// Если ключ успешно инициализирован
							if(result){
								// Генерируем случайный идентификатор конфигурации ECH
								uint8_t cid = 0;
								// Заполняем случайным значением
								::RAND_bytes(&cid, sizeof(cid));
								// Размер сериализованного ECHConfig
								size_t length = 0;
								// Указатель на сериализованный ECHConfig
								uint8_t * config = nullptr;
								/**
								 * Определяем публичное имя сервера для ECHConfig
								 * (открытое имя хоста, которое клиент увидит при неудаче ECH)
								 */
								const string name = (!member->host.empty() ? member->host : "localhost");
								// Формируем ECHConfig из HPKE ключа
								if(::SSL_marshal_ech_config(&config, &length, cid, key, name.c_str(), name.length()) == 1){
									// Создаём объект набора ECH ключей сервера
									SSL_ECH_KEYS * keys = ::SSL_ECH_KEYS_new();
									// Если объект набора ECH ключей создан
									if(keys != nullptr){
										// Добавляем конфигурацию ECH в набор (is_retry_config=1 для публикации в DNS)
										if(::SSL_ECH_KEYS_add(keys, 1, config, length, key) == 1){
											// Устанавливаем набор ECH ключей в контекст SSL
											if(!(result = (::SSL_CTX_set1_ech_keys(member->ctx, keys) == 1))){
												// Получаем текст ошибки
												const string error = ::ssl::error(id, "Failed to set ECH keys on SSL_CTX");
												// Если функция обратного вызова ошибки установлена
												if(member->callback.error != nullptr)
													// Вызываем функцию обратного вызова ошибки
													member->callback.error(id, error_t::SNI_FAILED, error);
												// Если функция обратного вызова ошибки не установлена
												else {
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Записываем ошибку в лог
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Записываем ошибку в лог
														this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
													#endif
												}
											// Если набор ECH ключей успешно установлен в контекст SSL
											} else {
												// Размер сериализованного ECHConfigList
												size_t length = 0;
												// Указатель на сериализованный ECHConfigList
												uint8_t * retry = nullptr;
												/**
												 * Успех: извлекаем retry-конфиги (ECHConfigList) для публикации в DNS.
												 * SSL_ECH_KEYS_marshal_retry_configs возвращает только те конфиги,
												 * которые были добавлены с is_retry_config=1 — именно они
												 * должны публиковаться в DNS HTTPS-записи eckparam.
												 */
												if(::SSL_ECH_KEYS_marshal_retry_configs(keys, &retry, &length) == 1){
													/**
													 * Сохраняем ECHConfigList в поле ech шаблона контекста
													 * (доступно через getKeysECH() для публикации в DNS).
													 */
													member->ech.assign(retry, retry + length);
													// Освобождаем временный буфер
													::OPENSSL_free(retry);
												}
											}
										}
										// Освобождаем набор ECH ключей
										::SSL_ECH_KEYS_free(keys);
									}
									// Освобождаем сериализованный ECHConfig
									::OPENSSL_free(config);
								// Если не удалось сериализовать ECHConfig
								} else {
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Failed to marshal ECH config");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::SNI_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
								}
							// Если ключ не удалось инициализировать
							} else {
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Failed to initialize HPKE key for ECH");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::SNI_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
							// Освобождаем HPKE ключ
							::EVP_HPKE_KEY_free(key);
						} break;
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						// Для клиента на CTL уровне: keys содержит ECHConfigList, применяем сразу к SSL*
						if((keys != nullptr) && (size > 0)){
							// Сохраняем список ECHConfig
							member->ech.assign(keys, keys + size);
							// Применяем список ECHConfig для зашифрованного ClientHello
							if(!(result = (::SSL_set1_ech_config_list(member->ssl, keys, size) == 1))){
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Failed to set ECH config list");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::SNI_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						}
					// Если узел является сервером
					} else {
						/**
						 * ECH серверные ключи должны быть настроены через CTS (уровень контекста),
						 * а не через CTL (уровень транспорта): SSL_CTX_set1_ech_keys требует SSL_CTX *.
						 */
						const string error = "ECH server keys must be configured at the context (CTS) level, not transport (CTL) level";
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::INVALID_LAYER, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод шифрования данных
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для шифрования
 * @param size   размер буфера данных для шифрования
 * @return       результат выполнения шифрования
 *
 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
 *
 */
bool awh::tls::Coder::encrypt(const id_t id, const void * buffer, const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если данные для шифрования переданы корректно
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for encryption operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::INVALID_LAYER, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Если рукопожатие выполнено успешно
					if(member->state & state::HANDSHAKE_MODE){
						// Смещение в буфере записанных данных
						size_t offset = 0;
						// Указатель на буфер данных для шифрования
						const auto * data = reinterpret_cast <const uint8_t *> (buffer);
						/**
						 * Записываем все данные в защищённый сокет и отправляем ciphertext из BIO
						 */
						while(offset < size){
							// Выполняем запись данных в защищённый сокет
							int32_t bytes = ::SSL_write(member->ssl, data + offset, static_cast <int32_t> (size - offset));
							// Если данные записаны
							if(bytes > 0){
								// Увеличиваем смещение в буфере записанных данных
								offset += static_cast <size_t> (bytes);
								// Если функция обратного вызова записи данных установлена
								if(member->callback.write != nullptr)
									// Вызываем функцию обратного вызова записи данных
									member->callback.write(id, event_t::ENCRYPTION, static_cast <size_t> (bytes));
							// Если данные не записаны
							} else {
								// Получаем код ошибки
								const int32_t error = ::SSL_get_error(member->ssl, bytes);
								// Если ошибка не связана с необходимостью повторного чтения или записи
								if((error != SSL_ERROR_WANT_READ) && (error != SSL_ERROR_WANT_WRITE)){
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Write is failed");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::WRITE_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова на уничтожение контекста TLS
										member->callback.state(id, state_t::DESTROYED);
									// Устанавливаем режим удаления участника обмена защищёнными данными
									member->state |= ::state::GARBAGE_MODE;
									// Выходим из цикла
									break;
								}
							}
							// Если данные из BIO буфера записи не отправлены
							if(!(result = ::ssl::emitWriteBio(member, id))){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Write is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::WRITE_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова на уничтожение контекста TLS
									member->callback.state(id, state_t::DESTROYED);
								// Устанавливаем режим удаления участника обмена защищёнными данными
								member->state |= ::state::GARBAGE_MODE;
								// Выходим из цикла
								break;
							}
							// Если все данные записаны
							if(offset >= size)
								// Выходим из цикла
								break;
							// Если запись приостановлена и в BIO нет данных для отправки
							if((bytes <= 0) && (::BIO_ctrl_pending(member->bio.write) == 0))
								// Выходим из цикла
								break;
						}
						// Если все данные записаны и отправлены
						result = (offset >= size);
					// Если рукопожатие не выполнено
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Handshake has not yet been completed");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::HANDSHAKE_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод расшифровки данных
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для расшифровки
 * @param size   размер буфера данных для расшифровки
 * @return       результат выполнения расшифровки
 *
 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
 *
 */
bool awh::tls::Coder::decrypt(const id_t id, const void * buffer, const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если данные для шифрования переданы корректно
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Если функция обратного вызова состояния установлена
					if(member->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = "Invalid layer for decryption operation";
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::INVALID_LAYER, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Выполняем запись данных в BIO буфер чтения
					int32_t bytes = ::BIO_write(member->bio.read, buffer, static_cast <int32_t> (size));
					// Если данные записаны успешно и размер записанных данных совпадает с размером входного буфера
					if((result = (bytes == static_cast <int32_t> (size)))){
						// Если функция обратного вызова записи данных установлена
						if(member->callback.write != nullptr)
							// Вызываем функцию обратного вызова записи данных
							member->callback.write(id, event_t::DECRYPTION, static_cast <size_t> (bytes));
						// Если рукопожатие ещё не выполнено
						if(!(member->state & state::HANDSHAKE_MODE)){
							// Если функция обратного вызова на вывод отпечатка браузера установлена
							if((this->_fgp != nullptr) && (member->callback.fingerprint != nullptr)){
								// Создаём объект отпечатка браузера
								fgp_t::browser_t browser{};
								// Выполняем парсинг отпечатка браузера
								if(this->_fgp->parse(reinterpret_cast <const uint8_t *> (buffer), size, browser))
									// Вызываем функцию обратного вызова на вывод отпечатка браузера
									member->callback.fingerprint(id, browser);
							}
							// Выполняем рукопожатие TLS
							result = this->handshake(id);
							// Если рукопожатие ещё не выполнено
							if(!result || !(member->state & state::HANDSHAKE_MODE))
								// Возвращаем результат
								return result;
						}
						// Если у нас есть подготовленные данные для чтения
						if((::BIO_ctrl_pending(member->bio.read) > 0) || (::SSL_has_pending(member->ssl) == 1)){
							/**
							 * Читаем все доступные данные из защищённого сокета
							 */
							do {
								// Читаем данные из защищённого сокета
								bytes = ::SSL_read(member->ssl, ::local::buffer, AWH_MAX_SSL_BUFFER_SIZE);
								// Если данные не прочитаны
								if(bytes <= 0){
									// Получаем код ошибки
									const int32_t error = ::SSL_get_error(member->ssl, bytes);
									// Если ошибка не связана с необходимостью повторного чтения или записи
									if(!(result = ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE) || (error == SSL_ERROR_ZERO_RETURN)))){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Read is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::READ_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова на уничтожение контекста TLS
											member->callback.state(id, state_t::DESTROYED);
										// Устанавливаем режим удаления участника обмена защищёнными данными
										member->state |= ::state::GARBAGE_MODE;
									}
									// Выходим из цикла
									break;
								// Если функция обратного вызова чтения данных установлена
								} else if(member->callback.read != nullptr)
									// Вызываем функцию обратного вызова чтения данных
									member->callback.read(id, event_t::DECRYPTION, ::local::buffer, static_cast <size_t> (bytes));
							/**
							 * Пока в BIO буфере чтения или в защищённом сокете есть ожидающие данные для чтения
							 */
							} while((::BIO_ctrl_pending(member->bio.read) > 0) || (::SSL_has_pending(member->ssl) == 1));
						}
					// Если данные не записаны полностью (SSL_get_error здесь неприменим — это BIO, не SSL)
					} else {
						// Устанавливаем отрицательный результат
						result = false;
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "BIO write failed");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::BIO_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова на уничтожение контекста TLS
							member->callback.state(id, state_t::DESTROYED);
						// Устанавливаем режим удаления участника обмена защищёнными данными
						member->state |= ::state::GARBAGE_MODE;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, buffer, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки поддерживаемых групп эллиптических кривых
 *
 * @param id     идентификатор события
 * @param groups список поддерживаемых групп эллиптических кривых
 *
 */
void awh::tls::Coder::groups(const id_t id, const vector <group_t> & groups) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список групп эллиптических кривых не пустой
		if(!groups.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Список поддерживаемых групп эллиптических кривых для установки
				vector <int32_t> support;
				/**
				 * Перебираем все группы эллиптических кривых для установки
				 */
				for(auto & item : groups){
					/**
					 * Определяем код группы эллиптической кривой для алгоритма обмена ключами
					 */
					switch(static_cast <uint8_t> (item)){
						// Если группа эллиптической кривой соответствует X25519
						case static_cast <uint8_t> (group_t::X25519):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x3B4);
						break;
						// Если группа эллиптической кривой соответствует X448
						case static_cast <uint8_t> (group_t::X448):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x3C1);
						break;
						// Если группа эллиптической кривой соответствует P-256
						case static_cast <uint8_t> (group_t::P_256):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x19F);
						break;
						// Если группа эллиптической кривой соответствует P-384
						case static_cast <uint8_t> (group_t::P_384):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x2CB);
						break;
						// Если группа эллиптической кривой соответствует P-521
						case static_cast <uint8_t> (group_t::P_521):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x2CC);
						break;
						// Если группа эллиптической кривой соответствует постквантовому алгоритму ML-KEM (Kyber)
						case static_cast <uint8_t> (group_t::MLKEM1024):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x3C6);
						break;
						// Если группа эллиптической кривой соответствует SECP256K1
						case static_cast <uint8_t> (group_t::SECP256K1):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x2CA);
						break;
						// Если группа эллиптической кривой соответствует X25519_MLKEM768
						case static_cast <uint8_t> (group_t::X25519_MLKEM768):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x3C5);
						break;
						// Если группа эллиптической кривой соответствует X25519_KYBER768_DRAFT00
						case static_cast <uint8_t> (group_t::X25519_KYBER768_DRAFT00):
							// Добавляем код группы эллиптической кривой в список поддерживаемых групп
							support.push_back(0x3C4);
						break;
					}
				}
				// Если список поддерживаемых групп эллиптических кривых не пустой
				if(!support.empty()){
					/**
					 * Определяем уровень транспортной безопасности
					 */
					switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
						// Если уровень является шаблонным контекстом безопасности
						case static_cast <uint8_t> (layer_t::CTS): {
							// Выполняем извлечение объекта шаблона контекста безопасности
							auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
							// Устанавливаем все группы эллиптических кривых для алгоритмов обмена ключами в контексте TLS
							if(::SSL_CTX_set1_groups(member->ctx, &support[0], support.size()) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Set groups is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CURVE_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, support.size()), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						} break;
						// Если уровень является транспортной передачей данных
						case static_cast <uint8_t> (layer_t::CTL): {
							// Выполняем извлечение объекта транспортного уровня передачи
							auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
							// Устанавливаем все группы эллиптических кривых для алгоритмов обмена ключами в контексте TLS
							if(::SSL_set1_groups(member->ssl, &support[0], support.size()) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Set groups is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CURVE_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, support.size()), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						} break;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, groups.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки алгоритмов шифрования
 *
 * @param id      идентификатор события
 * @param ciphers список алгоритмов шифрования для установки
 *
 */
void awh::tls::Coder::ciphers(const id_t id, const vector <cipher_t> & ciphers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список алгоритмов шифрования не пустой
		if(!ciphers.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Результирующая строка алгоритмов шифрования
				string result = "";
				/**
				 * Перебираем все алгоритмы шифрования для установки
				 */
				for(auto & item : ciphers){
					// Объект шифра поддерживаемого приложением
					const SSL_CIPHER * cipher = nullptr;
					/**
					 * Определяем код шифра для алгоритма шифрования
					 */
					switch(static_cast <uint8_t> (item)){
						// Если алгоритм шифрования соответствует AES128-SHA
						case static_cast <uint8_t> (cipher_t::AES128_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0x002F);
						break;
						// Если алгоритм шифрования соответствует AES256-SHA
						case static_cast <uint8_t> (cipher_t::AES256_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0x0035);
						break;
						// Если алгоритм шифрования соответствует AES128-GCM-SHA256
						case static_cast <uint8_t> (cipher_t::AES128_GCM_SHA256):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0x009C);
						break;
						// Если алгоритм шифрования соответствует AES256-GCM-SHA384
						case static_cast <uint8_t> (cipher_t::AES256_GCM_SHA384):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0x009D);
						break;
						// Если алгоритм шифрования соответствует PSK-AES128-CBC-SHA
						case static_cast <uint8_t> (cipher_t::PSK_AES128_CBC_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0x008C);
						break;
						// Если алгоритм шифрования соответствует PSK-AES256-CBC-SHA
						case static_cast <uint8_t> (cipher_t::PSK_AES256_CBC_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0x008D);
						break;
						// Если алгоритм шифрования соответствует ECDHE-RSA-AES128-SHA
						case static_cast <uint8_t> (cipher_t::ECDHE_RSA_AES128_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC013);
						break;
						// Если алгоритм шифрования соответствует ECDHE-RSA-AES256-SHA
						case static_cast <uint8_t> (cipher_t::ECDHE_RSA_AES256_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC014);
						break;
						// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES128-SHA
						case static_cast <uint8_t> (cipher_t::ECDHE_ECDSA_AES128_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC009);
						break;
						// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES256-SHA
						case static_cast <uint8_t> (cipher_t::ECDHE_ECDSA_AES256_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC00A);
						break;
						// Если алгоритм шифрования соответствует ECDHE-RSA-AES128-SHA256
						case static_cast <uint8_t> (cipher_t::ECDHE_RSA_AES128_SHA256):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC027);
						break;
						// Если алгоритм шифрования соответствует ECDHE-PSK-AES128-CBC-SHA
						case static_cast <uint8_t> (cipher_t::ECDHE_PSK_AES128_CBC_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC035);
						break;
						// Если алгоритм шифрования соответствует ECDHE-PSK-AES256-CBC-SHA
						case static_cast <uint8_t> (cipher_t::ECDHE_PSK_AES256_CBC_SHA):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC036);
						break;
						// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES128-SHA256
						case static_cast <uint8_t> (cipher_t::ECDHE_ECDSA_AES128_SHA256):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC023);
						break;
						// Если алгоритм шифрования соответствует ECDHE-RSA-AES128-GCM-SHA256
						case static_cast <uint8_t> (cipher_t::ECDHE_RSA_AES128_GCM_SHA256):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC02F);
						break;
						// Если алгоритм шифрования соответствует ECDHE-RSA-AES256-GCM-SHA384
						case static_cast <uint8_t> (cipher_t::ECDHE_RSA_AES256_GCM_SHA384):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC030);
						break;
						// Если алгоритм шифрования соответствует ECDHE-RSA-CHACHA20-POLY1305
						case static_cast <uint8_t> (cipher_t::ECDHE_RSA_CHACHA20_POLY1305):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xCCA8);
						break;
						// Если алгоритм шифрования соответствует ECDHE-PSK-CHACHA20-POLY1305
						case static_cast <uint8_t> (cipher_t::ECDHE_PSK_CHACHA20_POLY1305):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xCCAC);
						break;
						// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES128-GCM-SHA256
						case static_cast <uint8_t> (cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC02B);
						break;
						// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES256-GCM-SHA384
						case static_cast <uint8_t> (cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xC02C);
						break;
						// Если алгоритм шифрования соответствует ECDHE-ECDSA-CHACHA20-POLY1305
						case static_cast <uint8_t> (cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305):
							// Получаем объект шифра по его коду
							cipher = ::SSL_get_cipher_by_value(0xCCA9);
						break;
					}
					// Если объект шифра не найден
					if(cipher == nullptr)
						// Код не поддерживается BoringSSL — пропускаем
						continue;
					// Если строка алгоритмов шифрования не пустая
					if(!result.empty())
						// Добавляем разделитель алгоритмов шифрования
						result.append(1, ':');
					// Добавляем алгоритм шифрования в строку алгоритмов шифрования
					result.append(::SSL_CIPHER_standard_name(cipher));
				}
				// Если строка алгоритмов шифрования собрана
				if(!result.empty()){
					/**
					 * Определяем уровень транспортной безопасности
					 */
					switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
						// Если уровень является шаблонным контекстом безопасности
						case static_cast <uint8_t> (layer_t::CTS): {
							// Выполняем извлечение объекта шаблона контекста безопасности
							auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
							// Устанавливаем все основные алгоритмы шифрования
							if(::SSL_CTX_set_cipher_list(member->ctx, result.c_str()) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Set ciphers is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CIPHER_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ciphers.size()), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						} break;
						// Если уровень является транспортной передачей данных
						case static_cast <uint8_t> (layer_t::CTL): {
							// Выполняем извлечение объекта транспортного уровня передачи
							auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
							// Устанавливаем все основные алгоритмы шифрования
							if(::SSL_set_cipher_list(member->ssl, result.c_str()) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Set ciphers is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CIPHER_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ciphers.size()), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						} break;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, ciphers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод активации/деактивации GREASE-значений (мусорных кодов)
 *
 * @param id   идентификатор события
 * @param mode режим активации/деактивации
 *
 */
void awh::tls::Coder::grease(const id_t id, const event::mode_t mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						/**
						 * Определяем режим активации/деактивации GREASE-значений (мусорных кодов)
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если передан режим активации
							case static_cast <uint8_t> (event::mode_t::ENABLED):
								// Активируем GREASE-значения (мусорные коды) в контексте TLS
								::SSL_CTX_set_grease_enabled(member->ctx, 1);
							break;
							// Если передан режим деактивации
							case static_cast <uint8_t> (event::mode_t::DISABLED):
								// Деактивируем GREASE-значения (мусорные коды) в контексте TLS
								::SSL_CTX_set_grease_enabled(member->ctx, 0);
							break;
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "GREASE codes are only allowed to be added for the client");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CTS_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						/**
						 * Определяем режим активации/деактивации GREASE-значений (мусорных кодов)
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если передан режим активации
							case static_cast <uint8_t> (event::mode_t::ENABLED):
								// Активируем GREASE-значения (мусорные коды) в контексте TLS
								::SSL_CTX_set_grease_enabled(member->ctx, 1);
							break;
							// Если передан режим деактивации
							case static_cast <uint8_t> (event::mode_t::DISABLED):
								// Деактивируем GREASE-значения (мусорные коды) в контексте TLS
								::SSL_CTX_set_grease_enabled(member->ctx, 0);
							break;
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "GREASE codes are only allowed to be added for the client");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CTL_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод перемешивания поддерживаемых расширений TLS для имитации поведения различных браузеров
 *
 * @param id   идентификатор события
 * @param mode режим активации/деактивации перемешивания расширений
 *
 */
void awh::tls::Coder::permuteExtensions(const id_t id, const event::mode_t mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						/**
						 * Определяем режим активации/деактивации перемешивания поддерживаемых расширений TLS
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если передан режим активации
							case static_cast <uint8_t> (event::mode_t::ENABLED):
								// Активируем перемешивание поддерживаемых расширений TLS
								::SSL_CTX_set_permute_extensions(member->ctx, 1);
							break;
							// Если передан режим деактивации
							case static_cast <uint8_t> (event::mode_t::DISABLED):
								// Деактивируем перемешивание поддерживаемых расширений TLS
								::SSL_CTX_set_permute_extensions(member->ctx, 0);
							break;
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Permuting extensions is only allowed to be added for the client");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CTS_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						/**
						 * Определяем режим активации/деактивации перемешивания поддерживаемых расширений TLS
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если передан режим активации
							case static_cast <uint8_t> (event::mode_t::ENABLED):
								// Активируем перемешивание поддерживаемых расширений TLS
								::SSL_set_permute_extensions(member->ssl, 1);
							break;
							// Если передан режим деактивации
							case static_cast <uint8_t> (event::mode_t::DISABLED):
								// Деактивируем перемешивание поддерживаемых расширений TLS
								::SSL_set_permute_extensions(member->ssl, 0);
							break;
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Permuting extensions is only allowed to be added for the client");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CTL_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод активации поддержки SCT (Signed Certificate Timestamp)
 *
 * @param id идентификатор события
 *
 */
void awh::tls::Coder::signedCertificateTimestamp(const id_t id) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT)
						// Активируем поддержку SCT (Signed Certificate Timestamp) в контексте TLS
						::SSL_CTX_enable_signed_cert_timestamps(member->ctx);
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT)
						// Активируем поддержку SCT (Signed Certificate Timestamp) в транспортной передаче данных
						::SSL_enable_signed_cert_timestamps(member->ssl);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод активации поддержки Stapling (OCSP)
 *
 * @param id идентификатор события
 *
 */
void awh::tls::Coder::onlineCertificateStatusProtocol(const id_t id) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						// Активируем поддержку Stapling (OCSP) в контексте TLS
						::SSL_CTX_enable_ocsp_stapling(member->ctx);
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if(member->node == event::node_t::CLIENT){
						// Активируем поддержку Stapling (OCSP) в транспортной передаче данных
						::SSL_enable_ocsp_stapling(member->ssl);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод активации поддержки расширения Next Protocol Negotiation (NPN)
 *
 * @param id   идентификатор события
 * @param mode режим активации/деактивации поддержки расширения
 *
 */
void awh::tls::Coder::nextProtocolNegotiation(const id_t id, const event::mode_t mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если протокол подключения TCP
					if(member->proto == event::protocol_t::TCP){
						/**
						 * Определяем режим активации/деактивации поддержки расширения Next Protocol Negotiation (NPN)
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если передан режим активации
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								/**
								 * Определяем узел события к которому относится контекст TLS
								 */
								switch(static_cast <uint8_t> (member->node)){
									// Если узел является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT):
										// Устанавливаем функцию обратного вызова для переключения протокола
										::SSL_CTX_set_next_proto_select_cb(member->ctx, &::ssl::clientNextProtoSelect, member);
									break;
									// Если узел является сервером
									case static_cast <uint8_t> (event::node_t::SERVER):
										// Устанавливаем функцию обратного вызова при выборе следующего протокола
										::SSL_CTX_set_next_protos_advertised_cb(member->ctx, &::ssl::nextProto, member);
									break;
								}
							} break;
							// Если передан режим деактивации
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								/**
								 * Определяем узел события к которому относится контекст TLS
								 */
								switch(static_cast <uint8_t> (member->node)){
									// Если узел является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT):
										// Снимаем функцию обратного вызова для переключения протокола
										::SSL_CTX_set_next_proto_select_cb(member->ctx, nullptr, nullptr);
									break;
									// Если узел является сервером
									case static_cast <uint8_t> (event::node_t::SERVER):
										// Снимаем функцию обратного вызова при выборе следующего протокола
										::SSL_CTX_set_next_protos_advertised_cb(member->ctx, nullptr, nullptr);
									break;
								}
							} break;
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если протокол подключения TCP
					if(member->proto == event::protocol_t::TCP){
						/**
						 * Определяем режим активации/деактивации поддержки расширения Next Protocol Negotiation (NPN)
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если передан режим активации
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								/**
								 * Определяем узел события к которому относится контекст TLS
								 */
								switch(static_cast <uint8_t> (member->node)){
									// Если узел является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT):
										// Устанавливаем функцию обратного вызова для переключения протокола
										::SSL_CTX_set_next_proto_select_cb(member->ctx, &::ssl::clientNextProtoSelect, member);
									break;
									// Если узел является сервером
									case static_cast <uint8_t> (event::node_t::SERVER):
										// Устанавливаем функцию обратного вызова при выборе следующего протокола
										::SSL_CTX_set_next_protos_advertised_cb(member->ctx, &::ssl::nextProto, member);
									break;
								}
							} break;
							// Если передан режим деактивации
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								/**
								 * Определяем узел события к которому относится контекст TLS
								 */
								switch(static_cast <uint8_t> (member->node)){
									// Если узел является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT):
										// Снимаем функцию обратного вызова для переключения протокола
										::SSL_CTX_set_next_proto_select_cb(member->ctx, nullptr, nullptr);
									break;
									// Если узел является сервером
									case static_cast <uint8_t> (event::node_t::SERVER):
										// Снимаем функцию обратного вызова при выборе следующего протокола
										::SSL_CTX_set_next_protos_advertised_cb(member->ctx, nullptr, nullptr);
									break;
								}
							} break;
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод активации поддержки наложения цифрового отпечатка браузера на TLS-соединение
 *
 * @param id  идентификатор события
 * @param fid идентификатор цифрового отпечатка браузера
 *
 */
void awh::tls::Coder::browser(const id_t id, const fgp_t::id_t fid) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if((member->node == event::node_t::CLIENT) && (this->_fgp != nullptr)){
						// Выполняем установку идентификатора цифрового отпечатка браузера
						member->fid = fid;
						// Получаем объект браузера из шаблона контекста безопасности
						const fgp_t::browser_t & browser = this->_fgp->get(member->fid);
						// Если объект браузера получен
						if(!browser.ciphers.empty() && !browser.extensions.empty()){
							// Устанавливаем поддерживаемые шифры TLS
							this->ciphers(id, browser.ciphers);
							// Активируем/деактивируем поддержку GREASE-значений (мусорных кодов)
							this->grease(id, (browser.grease ? event::mode_t::ENABLED : event::mode_t::DISABLED));
							/**
							 * Выполняем перебор всего списка поддерживаемых расширений TLS
							 */
							for(auto & extension : browser.extensions){
								/**
								 * Определяем тип поддерживаемого расширения TLS
								 */
								switch(static_cast <uint8_t> (extension->type)){
									// Если тип расширения соответствует signed_certificate_timestamp
									case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP):
										// Активируем поддержку SCT (Signed Certificate Timestamp)
										this->signedCertificateTimestamp(id);
									break;
									// Если тип расширения соответствует status_request
									case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST):
										// Активируем поддержку Stapling (OCSP)
										this->onlineCertificateStatusProtocol(id);
									break;
									// Если тип расширения соответствует supported_groups
									case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS):
										// Устанавливаем поддерживаемые группы (кривые) для TLS-соединения
										this->groups(id, awh_cast <fgp_t::extension_supported_groups_t *> (extension.get())->supportedGroups);
									break;
									// Если тип расширения соответствует signature_algorithms
									case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS):
										// Устанавливаем поддерживаемые алгоритмы подписи для TLS-соединения
										this->signature(id, awh_cast <fgp_t::extension_signature_t *> (extension.get())->algorithms);
									break;
									// Если тип расширения соответствует compress_certificate
									case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
										// Список поддерживаемых методов компрессии сертификата TLS
										vector <compressor::method_t> methods;
										/**
										 * Перебираем весь список поддерживаемых компрессоров
										 */
										for(const compressor_t & compressor : awh_cast <fgp_t::extension_compress_certificate_t *> (extension.get())->algorithms){
											/**
											 * Определяем тип поддерживаемого компрессора
											 */
											switch(static_cast <uint8_t> (compressor)){
												// Если компрессор является Zlib
												case static_cast <uint8_t> (compressor_t::ZLIB):
													// Добавляем метод компрессии Zlib в список поддерживаемых методов компрессии
													methods.push_back(compressor::method_t::ZLIB);
												break;
												// Если компрессор является ZSTD (Zstandard)
												case static_cast <uint8_t> (compressor_t::ZSTD):
													// Добавляем метод компрессии ZSTD в список поддерживаемых методов компрессии
													methods.push_back(compressor::method_t::ZSTD);
												break;
												// Если компрессор является Brotli
												case static_cast <uint8_t> (compressor_t::BROTLI):
													// Добавляем метод компрессии Brotli в список поддерживаемых методов компрессии
													methods.push_back(compressor::method_t::BROTLI);
												break;
											}
										}
										// Если список поддерживаемых методов компрессии сертификата TLS не пустой
										if(!methods.empty())
											// Устанавливаем поддерживаемые методы компрессии сертификата TLS
											this->compressors(id, methods);
									} break;
									// Если тип расширения соответствует supported_versions
									case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
										/**
										 * Перебираем весь список поддерживаемых версий протокола TLS
										 */
										for(auto & version : awh_cast <fgp_t::extension_supported_versions_t *> (extension.get())->versions){
											/**
											 * Определяем тип поддерживаемой версии протокола TLS
											 */
											switch(static_cast <uint8_t> (version)){
												// Если версия протокола является TLS 1.2
												case static_cast <uint8_t> (version_t::TLS_1_2):
													// Устанавливаем минимально-возможную версию TLS 1.2
													::SSL_CTX_set_min_proto_version(member->ctx, TLS1_2_VERSION);
												break;
												// Если версия протокола является TLS 1.3
												case static_cast <uint8_t> (version_t::TLS_1_3):
													// Устанавливаем максимально-возможную версию TLS 1.3
													::SSL_CTX_set_max_proto_version(member->ctx, TLS1_3_VERSION);
												break;
												// Если версия протокола является DTLS 1.0
												case static_cast <uint8_t> (version_t::DTLS_1_0):
													// Устанавливаем минимально-возможную версию DTLS 1.0
													::SSL_CTX_set_min_proto_version(member->ctx, DTLS1_VERSION);
												break;
												// Если версия протокола является DTLS 1.2
												case static_cast <uint8_t> (version_t::DTLS_1_2):
													// Устанавливаем максимально-возможную версию DTLS 1.2
													::SSL_CTX_set_max_proto_version(member->ctx, DTLS1_2_VERSION);
												break;
											}
										}
									} break;
									// Если тип расширения соответствует next_protocol_negotiation
									case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG):
										// Активируем поддержку расширения Next Protocol Negotiation (NPN)
										this->nextProtocolNegotiation(id, event::mode_t::ENABLED);
									break;
								}
							}
						// Если шаблон отпечатка браузера пустой
						} else if(fid != 0) {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем предупреждение в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fid), log_t::flag_t::WARNING, "Browser fingerprint template is empty");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем предупреждение в лог
								this->_log->print("%s", log_t::flag_t::WARNING, "Browser fingerprint template is empty");
							#endif
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Activation browser fingerprint for TLS connections, supported for the client only");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::FINGERPRINT_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fid), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом
					if((member->node == event::node_t::CLIENT) && (this->_fgp != nullptr)){
						// Выполняем установку идентификатора цифрового отпечатка браузера
						member->fid = fid;
						// Получаем объект браузера из шаблона контекста безопасности
						const fgp_t::browser_t & browser = this->_fgp->get(member->fid);
						// Если объект браузера получен
						if(!browser.ciphers.empty() && !browser.extensions.empty()){
							// Переменная для хранения режима активации/деактивации EncryptedClientHello (ECH)
							event::mode_t ech = event::mode_t::DISABLED;
							// Устанавливаем поддерживаемые шифры TLS
							this->ciphers(id, browser.ciphers);
							// Активируем/деактивируем поддержку GREASE-значений (мусорных кодов)
							this->grease(id, (browser.grease ? event::mode_t::ENABLED : event::mode_t::DISABLED));
							/**
							 * Выполняем перебор всего списка поддерживаемых расширений TLS
							 */
							for(auto & extension : browser.extensions){
								/**
								 * Определяем тип поддерживаемого расширения TLS
								 */
								switch(static_cast <uint8_t> (extension->type)){
									// Если тип расширения соответствует signed_certificate_timestamp
									case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP):
										// Активируем поддержку SCT (Signed Certificate Timestamp)
										this->signedCertificateTimestamp(id);
									break;
									// Если тип расширения соответствует status_request
									case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST):
										// Активируем поддержку Stapling (OCSP)
										this->onlineCertificateStatusProtocol(id);
									break;
									// Если тип расширения соответствует supported_groups
									case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS):
										// Устанавливаем поддерживаемые группы (кривые) для TLS-соединения
										this->groups(id, awh_cast <fgp_t::extension_supported_groups_t *> (extension.get())->supportedGroups);
									break;
									// Если тип расширения соответствует signature_algorithms
									case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS):
										// Устанавливаем поддерживаемые алгоритмы подписи для TLS-соединения
										this->signature(id, awh_cast <fgp_t::extension_signature_t *> (extension.get())->algorithms);
									break;
									// Если тип расширения соответствует compress_certificate
									case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
										// Список поддерживаемых методов компрессии сертификата TLS
										vector <compressor::method_t> methods;
										/**
										 * Перебираем весь список поддерживаемых компрессоров
										 */
										for(const compressor_t & compressor : awh_cast <fgp_t::extension_compress_certificate_t *> (extension.get())->algorithms){
											/**
											 * Определяем тип поддерживаемого компрессора
											 */
											switch(static_cast <uint8_t> (compressor)){
												// Если компрессор является Zlib
												case static_cast <uint8_t> (compressor_t::ZLIB):
													// Добавляем метод компрессии Zlib в список поддерживаемых методов компрессии
													methods.push_back(compressor::method_t::ZLIB);
												break;
												// Если компрессор является ZSTD (Zstandard)
												case static_cast <uint8_t> (compressor_t::ZSTD):
													// Добавляем метод компрессии ZSTD в список поддерживаемых методов компрессии
													methods.push_back(compressor::method_t::ZSTD);
												break;
												// Если компрессор является Brotli
												case static_cast <uint8_t> (compressor_t::BROTLI):
													// Добавляем метод компрессии Brotli в список поддерживаемых методов компрессии
													methods.push_back(compressor::method_t::BROTLI);
												break;
											}
										}
										// Если список поддерживаемых методов компрессии сертификата TLS не пустой
										if(!methods.empty())
											// Устанавливаем поддерживаемые методы компрессии сертификата TLS
											this->compressors(id, methods);
									} break;
									// Если тип расширения соответствует supported_versions
									case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
										/**
										 * Перебираем весь список поддерживаемых версий протокола TLS
										 */
										for(auto & version : awh_cast <fgp_t::extension_supported_versions_t *> (extension.get())->versions){
											/**
											 * Определяем тип поддерживаемой версии протокола TLS
											 */
											switch(static_cast <uint8_t> (version)){
												// Если версия протокола является TLS 1.2
												case static_cast <uint8_t> (version_t::TLS_1_2):
													// Устанавливаем минимально-возможную версию TLS 1.2
													::SSL_set_min_proto_version(member->ssl, TLS1_2_VERSION);
												break;
												// Если версия протокола является TLS 1.3
												case static_cast <uint8_t> (version_t::TLS_1_3):
													// Устанавливаем максимально-возможную версию TLS 1.3
													::SSL_set_max_proto_version(member->ssl, TLS1_3_VERSION);
												break;
												// Если версия протокола является DTLS 1.0
												case static_cast <uint8_t> (version_t::DTLS_1_0):
													// Устанавливаем минимально-возможную версию DTLS 1.0
													::SSL_set_min_proto_version(member->ssl, DTLS1_VERSION);
												break;
												// Если версия протокола является DTLS 1.2
												case static_cast <uint8_t> (version_t::DTLS_1_2):
													// Устанавливаем максимально-возможную версию DTLS 1.2
													::SSL_set_max_proto_version(member->ssl, DTLS1_2_VERSION);
												break;
											}
										}
									} break;
									// Если тип расширения соответствует key_share
									case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE): {
										// Список поддерживаемых групп (кривых) для обмена ключами TLS
										vector <group_t> groups;
										/**
										 * Перебираем весь список поддерживаемых групп (кривых) для обмена ключами TLS
										 */
										for(auto & share : awh_cast <fgp_t::extension_key_share_t *> (extension.get())->shares)
											// Добавляем группу (кривую) для обмена ключами TLS в список поддерживаемых групп (кривых)
											groups.push_back(share.first);
										// Устанавливаем поддерживаемые группы (кривые) для обмена ключами TLS
										this->keyShare(id, groups, ech);
									} break;
									// Если тип расширения соответствует next_protocol_negotiation
									case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG):
										// Активируем поддержку расширения Next Protocol Negotiation (NPN)
										this->nextProtocolNegotiation(id, event::mode_t::ENABLED);
									break;
									// Если тип расширения соответствует application_settings_old (устаревшее)
									case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD):
										// Используем старый codepoint (application_settings_old = 0x4469 = 17513)
										::SSL_set_alps_use_new_codepoint(member->ssl, 0);
									break;
									// Если тип расширения соответствует application_settings
									case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS):
										// Используем новый codepoint (application_settings = 0x44CD = 17613)
										::SSL_set_alps_use_new_codepoint(member->ssl, 1);
									break;
									// Если тип расширения соответствует encrypted_client_hello
									case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO): {
										// Устанавливаем режим активации поддержки расширения EncryptedClientHello (ECH)
										ech = event::mode_t::ENABLED;
										// Активируем генерацию ложного ключа EncryptedClientHello (ECH)
										::SSL_set_enable_ech_grease(member->ssl, 1);
									} break;
								}
							}
						// Если шаблон отпечатка браузера пустой
						} else if(fid != 0) {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем предупреждение в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fid), log_t::flag_t::WARNING, "Browser fingerprint template is empty");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем предупреждение в лог
								this->_log->print("%s", log_t::flag_t::WARNING, "Browser fingerprint template is empty");
							#endif
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Activation browser fingerprint for TLS connections, supported for the client only");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::FINGERPRINT_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fid), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fid), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения списка поддерживаемых ALPN-протоколов
 *
 * @note Выдаёт настроенный список, а не согласованный протокол:
 *       согласованный отдаёт alpn() по идентификатору транспортного
 *       уровня. Предназначен для протоколов, которые ведут собственный
 *       обмен данными поверх настроенного контекста и применяют список
 *       к своему объекту TLS самостоятельно
 *
 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
 * @return   список поддерживаемых ALPN-протоколов
 *
 */
vector <awh::tls::Coder::alpn_t> awh::tls::Coder::protocols(const id_t id) const noexcept {
	// Результирующий список поддерживаемых ALPN-протоколов
	vector <alpn_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS не найден
		if(pin == nullptr)
			// Выводим пустой результат
			return result;
		// Список идентификаторов поддерживаемых протоколов
		const vector <uint8_t> * ids = nullptr;
		// Буфер поддерживаемых протоколов в проводном формате
		const vector <uint8_t> * buffer = nullptr;
		/**
		 * Определяем уровень транспортной безопасности
		 */
		switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
			// Если уровень является шаблонным контекстом безопасности
			case static_cast <uint8_t> (layer_t::CTS): {
				// Выполняем извлечение объекта шаблона контекста безопасности
				auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
				// Получаем список идентификаторов поддерживаемых протоколов
				ids = &member->alpn.ids;
				// Получаем буфер поддерживаемых протоколов
				buffer = &member->alpn.buffer;
			} break;
			// Если уровень является транспортным уровнем передачи
			case static_cast <uint8_t> (layer_t::CTL): {
				// Выполняем извлечение объекта транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Получаем список идентификаторов поддерживаемых протоколов
				ids = &member->alpn.ids;
				// Получаем буфер поддерживаемых протоколов
				buffer = &member->alpn.buffer;
			} break;
		}
		// Если буфер поддерживаемых протоколов не получен
		if((buffer == nullptr) || (ids == nullptr))
			// Выводим пустой результат
			return result;
		// Индекс протокола в списке идентификаторов
		size_t index = 0;
		/**
		 * Разбираем буфер протоколов проводного формата: каждая запись состоит
		 * из октета длины названия и самого названия
		 */
		for(size_t offset = 0; offset < buffer->size();){
			// Получаем длину названия протокола
			const size_t length = static_cast <size_t> ((* buffer)[offset]);
			// Если запись выходит за пределы буфера
			if((length == 0) || ((offset + 1 + length) > buffer->size()))
				// Прекращаем разбор буфера
				break;
			// Формируем запись поддерживаемого протокола
			alpn_t item;
			// Устанавливаем идентификатор протокола
			item.id = ((index < ids->size()) ? (* ids)[index] : 0);
			// Устанавливаем название протокола
			item.protocol.assign(reinterpret_cast <const char *> (buffer->data() + offset + 1), length);
			// Добавляем запись в результирующий список
			result.push_back(item);
			// Сдвигаем смещение разбора за записью
			offset += (1 + length);
			// Продвигаем индекс протокола
			index++;
		}
	/**
	 * Выполняем перехват ошибки
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения активного протокола
 *
 * @param id идентификатор события
 * @return   метод активного протокола
 *
 */
uint8_t awh::tls::Coder::alpn(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS):
					// Выполняем извлечение объекта транспортного уровня передачи
					return reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id))->alpn.id;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL):
					// Выполняем извлечение объекта транспортного уровня передачи
					return reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id))->alpn.id;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return -1;
}
/**
 * @brief Метод установки поддерживаемых ALPN-протоколов
 *
 * @param id   идентификатор события
 * @param alpn список поддерживаемых ALPN-протоколов
 *
 */
void awh::tls::Coder::alpn(const id_t id, const vector <alpn_t> & alpn) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список поддерживаемых ALPN-протоколов не пустой
		if(!alpn.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						// Выполняем сброс списка идентификаторов поддерживаемых ALPN-протоколов
						member->alpn.ids.clear();
						// Выполняем сброс буфера поддерживаемых ALPN-протоколов
						member->alpn.buffer.clear();
						/**
						 * Выполняем перебор всего списка поддерживаемых ALPN-протоколов
						 */
						for(const auto & item : alpn){
							// Длина имени ALPN-протокола в wire-формате — один байт (1..255)
							if(item.protocol.empty() || (item.protocol.size() > 255)){
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "ALPN protocol name length must be 1..255 bytes");
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, item.protocol), log_t::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
								#endif
								// Пропускаем некорректный протокол
								continue;
							}
							// Добавляем идентификатор протокола в список поддерживаемых протоколов
							member->alpn.ids.push_back(item.id);
							// Добавляем в буфер длину названия протокола
							member->alpn.buffer.push_back(static_cast <uint8_t> (item.protocol.size()));
							// Добавляем в буфер название протокола
							member->alpn.buffer.insert(member->alpn.buffer.end(), item.protocol.begin(), item.protocol.end());
						}
						// Если после фильтрации не осталось допустимых протоколов
						if(member->alpn.ids.empty())
							// Завершаем выполнение метода
							break;
						// Если идентификатор выбранного ALPN-протокола не передан
						member->alpn.id = member->alpn.ids.front();
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT)
							// Выполняем установку доступных протоколов передачи данных
							::SSL_CTX_set_alpn_protos(member->ctx, member->alpn.buffer.data(), static_cast <uint32_t> (member->alpn.buffer.size()));
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Выполняем сброс списка идентификаторов поддерживаемых ALPN-протоколов
						member->alpn.ids.clear();
						// Выполняем сброс буфера поддерживаемых ALPN-протоколов
						member->alpn.buffer.clear();
						/**
						 * Выполняем перебор всего списка поддерживаемых ALPN-протоколов
						 */
						for(const auto & item : alpn){
							// Длина имени ALPN-протокола в wire-формате — один байт (1..255)
							if(item.protocol.empty() || (item.protocol.size() > 255)){
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "ALPN protocol name length must be 1..255 bytes");
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, item.protocol), log_t::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
								#endif
								// Пропускаем некорректный протокол
								continue;
							}
							// Добавляем идентификатор протокола в список поддерживаемых протоколов
							member->alpn.ids.push_back(item.id);
							// Добавляем в буфер длину названия протокола
							member->alpn.buffer.push_back(static_cast <uint8_t> (item.protocol.size()));
							// Добавляем в буфер название протокола
							member->alpn.buffer.insert(member->alpn.buffer.end(), item.protocol.begin(), item.protocol.end());
						}
						// Если после фильтрации не осталось допустимых протоколов
						if(member->alpn.ids.empty())
							// Завершаем выполнение метода
							break;
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT)
							// Выполняем установку доступных протоколов передачи данных
							::SSL_set_alpn_protos(member->ssl, &member->alpn.buffer[0], static_cast <uint32_t> (member->alpn.buffer.size()));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, alpn.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки поддерживаемых ALPS-протоколов
 *
 * @param id   идентификатор события
 * @param alps список поддерживаемых ALPS-протоколов
 * @param std  флаг поддерживаемого стандарта
 *
 */
void awh::tls::Coder::alps(const id_t id, const vector <alpn_t> & alps, const standard_t std) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список поддерживаемых ALPS-протоколов не пустой
		if(!alps.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "ALPS protocols are only allowed to be added for the transport data layer");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::ALPS_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, alps.size()), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT){
							// Буфер ALPS-протоколов
							vector <uint8_t> buffer;
							/**
							 * Выполняем перебор всего списка поддерживаемых ALPS-протоколов
							 */
							for(const auto & item : alps){
								// Длина имени ALPN-протокола в wire-формате — один байт (1..255)
								if(item.protocol.empty() || (item.protocol.size() > 255)){
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "ALPS protocol name length must be 1..255 bytes");
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, item.protocol), log_t::flag_t::WARNING, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
									#endif
									// Пропускаем некорректный протокол
									continue;
								}
								// Добавляем в буфер длину названия протокола
								buffer.push_back(static_cast <uint8_t> (item.protocol.size()));
								// Добавляем в буфер название протокола
								buffer.insert(buffer.end(), item.protocol.begin(), item.protocol.end());
							}
							// Итерируем по буферу ALPN (формат: len + bytes)
							if(!buffer.empty()){
								// Смещение для итерации по буферу ALPN
								size_t offset = 0;
								// Длина названия протокола
								uint8_t length = 0;
								/**
								 * Итерируем по буферу ALPN, регистрируя ALPS для каждого протокола с пустыми настройками
								 */
								while(offset < buffer.size()){
									// Получаем длину названия протокола
									length = buffer[offset];
									// Проверяем, что смещение и длина названия протокола не выходят за пределы буфера ALPN
									if(static_cast <size_t> (offset + length + 1) > buffer.size())
										// Выходим из цикла, если данные некорректные
										break;
									// Регистрируем ALPS для протокола с пустыми настройками
									::SSL_add_application_settings(member->ssl, &buffer[0] + (offset + 1), length, nullptr, 0);
									// Увеличиваем смещение на длину названия протокола и байт длины
									offset += static_cast <size_t> (length + 1);
								}
							}
							/**
							 * В зависимости от стандарта, который поддерживает клиент, используем соответствующий codepoint для регистрации ALPS-протоколов
							 */
							switch(static_cast <uint8_t> (std)){
								// Если стандарт соответствует новому
								case static_cast <uint8_t> (standard_t::NEW):
									// Используем новый codepoint (application_settings = 0x44CD = 17613)
									::SSL_set_alps_use_new_codepoint(member->ssl, 1);
								break;
								// Если стандарт соответствует старому
								case static_cast <uint8_t> (standard_t::OLD):
									// Используем старый codepoint (application_settings_old = 0x4469 = 17513)
									::SSL_set_alps_use_new_codepoint(member->ssl, 0);
								break;
							}
						// Если узел является сервером
						} else {
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "ALPS protocols are only allowed to be added for the client");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::ALPS_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, alps.size()), log_t::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
								#endif
							}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, alps.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки поддерживаемых алгоритмов подписи
 *
 * @param id         идентификатор события
 * @param signatures список поддерживаемых алгоритмов подписи
 *
 */
void awh::tls::Coder::signature(const id_t id, const vector <signature_t> & signatures) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список поддерживаемых алгоритмов подписи не пустой
		if(!signatures.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Список поддерживаемых алгоритмов подписи для установки
				vector <uint16_t> sigalgs;
				/**
				 * Перебираем все группы алгоритмов подписи для установки
				 */
				for(auto & item : signatures){
					/**
					 * Определяем код алгоритма поддерживаемой подписи
					 */
					switch(static_cast <uint8_t> (item)){
						// Если группа алгоритма подписи соответствует ED25519
						case static_cast <uint8_t> (signature_t::ED25519):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_ED25519);
						break;
						// Если группа алгоритма подписи соответствует RSA_PKCS1_SHA1
						case static_cast <uint8_t> (signature_t::RSA_PKCS1_SHA1):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PKCS1_SHA1);
						break;
						// Если группа алгоритма подписи соответствует RSA_PKCS1_SHA256
						case static_cast <uint8_t> (signature_t::RSA_PKCS1_SHA256):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PKCS1_SHA256);
						break;
						// Если группа алгоритма подписи соответствует RSA_PKCS1_SHA384
						case static_cast <uint8_t> (signature_t::RSA_PKCS1_SHA384):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PKCS1_SHA384);
						break;
						// Если группа алгоритма подписи соответствует RSA_PKCS1_SHA512
						case static_cast <uint8_t> (signature_t::RSA_PKCS1_SHA512):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PKCS1_SHA512);
						break;
						// Если группа алгоритма подписи соответствует RSA_PSS_RSAE_SHA256
						case static_cast <uint8_t> (signature_t::RSA_PSS_RSAE_SHA256):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PSS_RSAE_SHA256);
						break;
						// Если группа алгоритма подписи соответствует RSA_PSS_RSAE_SHA384
						case static_cast <uint8_t> (signature_t::RSA_PSS_RSAE_SHA384):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PSS_RSAE_SHA384);
						break;
						// Если группа алгоритма подписи соответствует RSA_PSS_RSAE_SHA512
						case static_cast <uint8_t> (signature_t::RSA_PSS_RSAE_SHA512):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PSS_RSAE_SHA512);
						break;
						// Если группа алгоритма подписи соответствует ECDSA_SHA1
						case static_cast <uint8_t> (signature_t::ECDSA_SHA1):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_ECDSA_SHA1);
						break;
						// Если группа алгоритма подписи соответствует ECDSA_SECP256R1_SHA256
						case static_cast <uint8_t> (signature_t::ECDSA_SECP256R1_SHA256):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_ECDSA_SECP256R1_SHA256);
						break;
						// Если группа алгоритма подписи соответствует ECDSA_SECP384R1_SHA384
						case static_cast <uint8_t> (signature_t::ECDSA_SECP384R1_SHA384):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_ECDSA_SECP384R1_SHA384);
						break;
						// Если группа алгоритма подписи соответствует ECDSA_SECP521R1_SHA512
						case static_cast <uint8_t> (signature_t::ECDSA_SECP521R1_SHA512):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_ECDSA_SECP521R1_SHA512);
						break;
						// Если группа алгоритма подписи соответствует RSA_PKCS1_MD5_SHA1
						case static_cast <uint8_t> (signature_t::RSA_PKCS1_MD5_SHA1):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PKCS1_MD5_SHA1);
						break;
						// Если группа алгоритма подписи соответствует RSA_PKCS1_SHA256_LEGACY
						case static_cast <uint8_t> (signature_t::RSA_PKCS1_SHA256_LEGACY):
							// Добавляем код алгоритма подписи в список поддерживаемых подписей
							sigalgs.push_back(SSL_SIGN_RSA_PKCS1_SHA256_LEGACY);
						break;
					}
				}
				// Если список поддерживаемых алгоритмов подписи не пустой
				if(!sigalgs.empty()){
					/**
					 * Определяем уровень транспортной безопасности
					 */
					switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
						// Если уровень является шаблонным контекстом безопасности
						case static_cast <uint8_t> (layer_t::CTS): {
							// Выполняем извлечение объекта шаблона контекста безопасности
							auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
							// Устанавливаем все поддерживаемые алгоритмы подписи в контексте TLS
							if(
								(::SSL_CTX_set_signing_algorithm_prefs(member->ctx, &sigalgs[0], sigalgs.size()) != 1) ||
								(::SSL_CTX_set_verify_algorithm_prefs(member->ctx, &sigalgs[0], sigalgs.size()) != 1)
							){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Set signature algorithms is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::SIGNATURE_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, signatures.size()), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						} break;
						// Если уровень является транспортной передачей данных
						case static_cast <uint8_t> (layer_t::CTL): {
							// Выполняем извлечение объекта транспортного уровня передачи
							auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
							// Устанавливаем все поддерживаемые алгоритмы подписи в контексте TLS
							if(
								(::SSL_set_signing_algorithm_prefs(member->ssl, &sigalgs[0], sigalgs.size()) != 1) ||
								(::SSL_set_verify_algorithm_prefs(member->ssl, &sigalgs[0], sigalgs.size()) != 1)
							){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Set signature algorithms is failed");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::SIGNATURE_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, signatures.size()), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						} break;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, signatures.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки поддерживаемых алгоритмов компрессии сертификата
 *
 * @param id     идентификатор события
 * @param methods список поддерживаемых алгоритмов компрессии сертификата
 *
 */
void awh::tls::Coder::compressors(const id_t id, const vector <compressor::method_t> & methods) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список поддерживаемых алгоритмов компрессии сертификата не пустой
		if(!methods.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						/**
						 * Перебираем все поддерживаемые алгоритмы компрессии сертификата
						 */
						for(auto & method : methods){
							/**
							 * Определяем поддерживаемый алгоритм компрессии сертификата
							 */
							switch(static_cast <uint8_t> (method)){
								// Если алгоритм компрессии сертификата соответствует Zlib
								case static_cast <uint8_t> (compressor::method_t::ZLIB): {
									// Устанавливаем поддерживаемый алгоритм компрессии сертификата для TLS 1.3
									if(::SSL_CTX_add_cert_compression_alg(
										member->ctx,
										// Устанавливаем алгоритм компрессии сертификата как Zlib
										0x01,
										// Функция сжатия (для сервера)
										&::compressor::compressionZlib,
										// Функция распаковки (для клиента)
										&::compressor::decompressionZlib
									) != 1){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Set certificate compression method Zlib is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::COMPRESSION_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
									}
								} break;
								// Если алгоритм компрессии сертификата соответствует Brotli
								case static_cast <uint8_t> (compressor::method_t::BROTLI): {
									// Устанавливаем поддерживаемый алгоритм компрессии сертификата для TLS 1.3
									if(::SSL_CTX_add_cert_compression_alg(
										member->ctx,
										// Устанавливаем алгоритм компрессии сертификата как Brotli
										0x02,
										// Функция сжатия (для сервера)
										&::compressor::compressionBrotli,
										// Функция распаковки (для клиента)
										&::compressor::decompressionBrotli
									) != 1){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Set certificate compression method Brotli is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::COMPRESSION_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
									}
								} break;
								// Если алгоритм компрессии сертификата соответствует ZStandard (Zstd)
								case static_cast <uint8_t> (compressor::method_t::ZSTD): {
									// Устанавливаем поддерживаемый алгоритм компрессии сертификата для TLS 1.3
									if(::SSL_CTX_add_cert_compression_alg(
										member->ctx,
										// Устанавливаем алгоритм компрессии сертификата как ZStandard (Zstd)
										0x03,
										// Функция сжатия (для сервера)
										&::compressor::compressionZstandard,
										// Функция распаковки (для клиента)
										&::compressor::decompressionZstandard
									) != 1){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Set certificate compression method ZStandard (Zstd) is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::COMPRESSION_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
									}
								} break;
								// Если алгоритм компрессии сертификата не поддерживается
								default: {
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Unsupported certificate compression method");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::COMPRESSION_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
								}
							}
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						/**
						 * Перебираем все поддерживаемые алгоритмы компрессии сертификата
						 */
						for(auto & method : methods){
							/**
							 * Определяем поддерживаемый алгоритм компрессии сертификата
							 */
							switch(static_cast <uint8_t> (method)){
								// Если алгоритм компрессии сертификата соответствует Zlib
								case static_cast <uint8_t> (compressor::method_t::ZLIB): {
									// Устанавливаем поддерживаемый алгоритм компрессии сертификата для TLS 1.3
									if(::SSL_CTX_add_cert_compression_alg(
										member->ctx,
										// Устанавливаем алгоритм компрессии сертификата как Zlib
										0x01,
										// Функция сжатия (для сервера)
										&::compressor::compressionZlib,
										// Функция распаковки (для клиента)
										&::compressor::decompressionZlib
									) != 1){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Set certificate compression method Zlib is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::COMPRESSION_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
									}
								} break;
								// Если алгоритм компрессии сертификата соответствует Brotli
								case static_cast <uint8_t> (compressor::method_t::BROTLI): {
									// Устанавливаем поддерживаемый алгоритм компрессии сертификата для TLS 1.3
									if(::SSL_CTX_add_cert_compression_alg(
										member->ctx,
										// Устанавливаем алгоритм компрессии сертификата как Brotli
										0x02,
										// Функция сжатия (для сервера)
										&::compressor::compressionBrotli,
										// Функция распаковки (для клиента)
										&::compressor::decompressionBrotli
									) != 1){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Set certificate compression method Brotli is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::COMPRESSION_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
									}
								} break;
								// Если алгоритм компрессии сертификата соответствует ZStandard (Zstd)
								case static_cast <uint8_t> (compressor::method_t::ZSTD): {
									// Устанавливаем поддерживаемый алгоритм компрессии сертификата для TLS 1.3
									if(::SSL_CTX_add_cert_compression_alg(
										member->ctx,
										// Устанавливаем алгоритм компрессии сертификата как ZStandard (Zstd)
										0x03,
										// Функция сжатия (для сервера)
										&::compressor::compressionZstandard,
										// Функция распаковки (для клиента)
										&::compressor::decompressionZstandard
									) != 1){
										// Если функция обратного вызова состояния установлена
										if(member->callback.state != nullptr)
											// Вызываем функцию обратного вызова состояния
											member->callback.state(id, state_t::FAILED);
										// Получаем текст ошибки
										const string error = ::ssl::error(id, "Set certificate compression method ZStandard (Zstd) is failed");
										// Если функция обратного вызова ошибки установлена
										if(member->callback.error != nullptr)
											// Вызываем функцию обратного вызова ошибки
											member->callback.error(id, error_t::COMPRESSION_FAILED, error);
										// Если функция обратного вызова ошибки не установлена
										else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
											#endif
										}
									}
								} break;
								// Если алгоритм компрессии сертификата не поддерживается
								default: {
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Unsupported certificate compression method");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::COMPRESSION_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (method)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
								}
							}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, methods.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод генерации заранее клиентом эфемерного ключа и отправки серверу для поддерживаемых групп эллиптических кривых
 *
 * @param id     идентификатор события
 * @param groups список поддерживаемых групп эллиптических кривых для ключевого обмена
 * @param grease флаг активации/деактивации ложного ключа EncryptedClientHello (ECH)
 *
 */
void awh::tls::Coder::keyShare(const id_t id, const vector <group_t> & groups, const event::mode_t grease) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список поддерживаемых групп эллиптических кривых не пустой
		if(!groups.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Groups elliptic curves are only allowed to be added for the transport data layer");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CURVE_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, groups.size(), static_cast <uint16_t> (grease)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Если узел является клиентом
						if(member->node == event::node_t::CLIENT){
							// Список поддерживаемых групп эллиптических кривых для установки
							vector <uint16_t> support;
							/**
							 * Перебираем все группы эллиптических кривых для установки
							 */
							for(auto & item : groups){
								/**
								 * Определяем код группы эллиптической кривой для алгоритма обмена ключами
								 */
								switch(static_cast <uint8_t> (item)){
									// Если группа эллиптической кривой соответствует X25519
									case static_cast <uint8_t> (group_t::X25519):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_X25519);
									break;
									// Если группа эллиптической кривой соответствует P-256
									case static_cast <uint8_t> (group_t::P_256):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_SECP256R1);
									break;
									// Если группа эллиптической кривой соответствует P-384
									case static_cast <uint8_t> (group_t::P_384):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_SECP384R1);
									break;
									// Если группа эллиптической кривой соответствует P-521
									case static_cast <uint8_t> (group_t::P_521):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_SECP521R1);
									break;
									// Если группа эллиптической кривой соответствует постквантовому алгоритму ML-KEM (Kyber)
									case static_cast <uint8_t> (group_t::MLKEM1024):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_MLKEM1024);
									break;
									// Если группа эллиптической кривой соответствует X25519_MLKEM768
									case static_cast <uint8_t> (group_t::X25519_MLKEM768):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_X25519_MLKEM768);
									break;
									// Если группа эллиптической кривой соответствует X25519_KYBER768_DRAFT00
									case static_cast <uint8_t> (group_t::X25519_KYBER768_DRAFT00):
										// Добавляем код группы эллиптической кривой в список поддерживаемых групп
										support.push_back(SSL_GROUP_X25519_KYBER768_DRAFT00);
									break;
								}
							}
							// Если список поддерживаемых групп эллиптических кривых не пустой
							if(!support.empty()){
								/**
								 * Определяем режим активации/деактивации генерации ложного ключа EncryptedClientHello (ECH)
								 */
								switch(static_cast <uint8_t> (grease)){
									// Если передан режим активации
									case static_cast <uint8_t> (event::mode_t::ENABLED):
										// Активируем генерацию ложного ключа EncryptedClientHello (ECH)
										::SSL_set_enable_ech_grease(member->ssl, 1);
									break;
									// Если передан режим деактивации
									case static_cast <uint8_t> (event::mode_t::DISABLED):
										// Деактивируем генерацию ложного ключа EncryptedClientHello (ECH)
										::SSL_set_enable_ech_grease(member->ssl, 0);
									break;
								}
								// Устанавливаем все группы эллиптических кривых для алгоритмов обмена ключами в контексте TLS
								if(::SSL_set1_client_key_shares(member->ssl, &support[0], support.size()) != 1){
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Set groups is failed");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::CURVE_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, groups.size(), static_cast <uint16_t> (grease)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
								}
							}
						// Если узел является сервером
						} else {
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Groups elliptic curves are only allowed to be added for the client");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CURVE_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, groups.size(), static_cast <uint16_t> (grease)), log_t::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
								#endif
							}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, groups.size(), static_cast <uint16_t> (grease)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки сертификатов доверенных центров сертификации
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу сертификата доверенных центров сертификации
 *
 */
void awh::tls::Coder::ca(const id_t id, string_view filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если путь к файлу центра сертификации указан
					if(!filename.empty()){
						// Создаём новое хранилище
						X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
						// Если хранилище не создано
						if(store == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Get x509 store is not found");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::STORE_X509_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						// Загружаем местоположение центра сертификации
						if(::X509_STORE_load_locations(store, filename.data(), nullptr) != 1){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "CA-file is not loaded");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CA_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// handle error
							return;
						}
						// Если узел является сервером
						if(member->node == event::node_t::SERVER){
							// Загружаем список сертификатов центра сертификации
							STACK_OF(X509_NAME) * cert = ::SSL_load_client_CA_file(filename.data());
							// Если список сертификатов загружен успешно
							if(cert != nullptr)
								// Выполняем установку CRL-файла сертификата
								::SSL_CTX_set_client_CA_list(member->ctx, cert);
							// Если список сертификатов не загружен
							else {
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = "Load client CA file is failed";
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CA_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
							}
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если путь к файлу центра сертификации указан
					if(!filename.empty()){
						// Создаём новое хранилище
						X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
						// Если хранилище не создано
						if(store == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Get x509 store is not found");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::STORE_X509_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						// Загружаем местоположение центра сертификации
						if(::X509_STORE_load_locations(store, filename.data(), nullptr) != 1){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "CA-file is not loaded");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CA_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// handle error
							return;
						}
						// Если узел является сервером
						if(member->node == event::node_t::SERVER)
							// Выполняем установку CRL-файла сертификата
							::SSL_set_client_CA_list(member->ssl, ::SSL_load_client_CA_file(filename.data()));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки сертификатов доверенных центров сертификации
 *
 * @param id   идентификатор транспортного уровня или шаблона контекста безопасности
 * @param dir  адрес директории с сертификатами доверенных центров сертификации
 * @param file путь к файлу сертификата доверенного центра сертификации
 *
 */
void awh::tls::Coder::ca(const id_t id, string_view dir, string_view file) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если название файла центра сертификации не пустое
					if(!file.empty()){
						// Создаём новое хранилище
						X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
						// Если хранилище не создано
						if(store == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Get x509 store is not found");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::STORE_X509_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						// Если каталог сертификатов передан
						if(!dir.empty()){
							// Полный путь к файлу центра сертификации
							string filename = "";
							// Если последний символ каталога является разделителем
							if(dir.back() == AWH_FS_SEPARATOR[0])
								// Формируем полный путь к файлу центра сертификации
								filename = ::move(this->_fmk->format("%s%s", dir.data(), file.data()));
							// Формируем полный путь к файлу центра сертификации
							else filename = ::move(this->_fmk->format("%s%s%s", dir.data(), AWH_FS_SEPARATOR, file.data()));
							// Загружаем местоположение центра сертификации
							if(::X509_STORE_load_locations(store, filename.data(), nullptr) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "CA-file is not loaded");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CA_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
							// Если узел является сервером
							if(member->node == event::node_t::SERVER)
								// Выполняем установку CRL-файла сертификата
								::SSL_CTX_set_client_CA_list(member->ctx, ::SSL_load_client_CA_file(filename.data()));
						// Если каталог сертификатов не передан
						} else {
							// Загружаем местоположение центра сертификации
							if(::X509_STORE_load_locations(store, file.data(), nullptr) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "CA-file is not loaded");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CA_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
							// Если узел является сервером
							if(member->node == event::node_t::SERVER)
								// Выполняем установку CRL-файла сертификата
								::SSL_CTX_set_client_CA_list(member->ctx, ::SSL_load_client_CA_file(file.data()));
						}
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если название файла центра сертификации не пустое
					if(!file.empty()){
						// Создаём новое хранилище
						X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
						// Если хранилище не создано
						if(store == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Get x509 store is not found");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::STORE_X509_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						// Если каталог сертификатов передан
						if(!dir.empty()){
							// Полный путь к файлу центра сертификации
							string filename = "";
							// Если последний символ каталога является разделителем
							if(dir.back() == AWH_FS_SEPARATOR[0])
								// Формируем полный путь к файлу центра сертификации
								filename = ::move(this->_fmk->format("%s%s", dir.data(), file.data()));
							// Формируем полный путь к файлу центра сертификации
							else filename = ::move(this->_fmk->format("%s%s%s", dir.data(), AWH_FS_SEPARATOR, file.data()));
							// Загружаем местоположение центра сертификации
							if(::X509_STORE_load_locations(store, filename.data(), nullptr) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "CA-file is not loaded");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CA_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
							// Если узел является сервером
							if(member->node == event::node_t::SERVER)
								// Выполняем установку CRL-файла сертификата
								::SSL_set_client_CA_list(member->ssl, ::SSL_load_client_CA_file(filename.data()));
						// Если каталог сертификатов не передан
						} else {
							// Загружаем местоположение центра сертификации
							if(::X509_STORE_load_locations(store, file.data(), nullptr) != 1){
								// Если функция обратного вызова состояния установлена
								if(member->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									member->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "CA-file is not loaded");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::CA_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
							// Если узел является сервером
							if(member->node == event::node_t::SERVER)
								// Выполняем установку CRL-файла сертификата
								::SSL_set_client_CA_list(member->ssl, ::SSL_load_client_CA_file(file.data()));
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки списка отзыва сертификатов
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу списка отзыва сертификатов
 *
 */
void awh::tls::Coder::certificateRevocationList(const id_t id, string_view filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу сертификата указан
		if(!filename.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						// Если CRL-файл сертификата уже создан
						if(member->crl != nullptr)
							// Выполняем освобождение памяти
							::X509_CRL_free(member->crl);
						// Создаём объект BIO для загрузки файла
						BIO * bio = ::BIO_new(::BIO_s_file());
						// Если BIO не создан
						if(bio == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						// Выполняем чтение CRL-файла сертификата
						if(BIO_read_filename(bio, filename.data()) <= 0){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "CRL-file is corrupted or unreadable");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выполняем очистку памяти BIO
							::BIO_free(bio);
							// Выходим из функции
							return;
						}
						// Выполняем создание объекта CRL-файла сертификата
						member->crl = ::PEM_read_bio_X509_CRL(bio, nullptr, nullptr, nullptr);
						// Если CRL-файл сертификата не создан
						if(member->crl == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "CRL-file cannot be set");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
						}
						// Выполняем очистку памяти BIO
						::BIO_free(bio);
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Если CRL-файл сертификата уже создан
						if((member->crl != nullptr) && ((* member->crl) != nullptr))
							// Выполняем освобождение памяти
							::X509_CRL_free(* member->crl);
						// Создаём объект BIO для загрузки файла
						BIO * bio = ::BIO_new(::BIO_s_file());
						// Если BIO не создан
						if(bio == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Engine store CRL");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						// Выполняем чтение CRL-файла сертификата
						if(::BIO_read_filename(bio, filename.data()) <= 0){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "CRL-file is corrupted or unreadable");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выполняем очистку памяти BIO
							::BIO_free(bio);
							// Выходим из функции
							return;
						}
						// Выполняем создание объекта CRL-файла сертификата
						(* member->crl) = ::PEM_read_bio_X509_CRL(bio, nullptr, nullptr, nullptr);
						// Если CRL-файл сертификата не создан
						if((* member->crl) == nullptr){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "CRL-file cannot be set");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
						}
						// Выполняем очистку памяти BIO
						::BIO_free(bio);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки приватного ключа клиента
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу приватного ключа клиента
 * @param type     тип файла приватного ключа клиента
 *
 */
void awh::tls::Coder::privateKey(const id_t id, string_view filename, const type_t type) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу сертификата указан
		if(!filename.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						/**
						 * Определяем тип файла приватного ключа клиента
						 */
						switch(static_cast <uint8_t> (type)){
							// PEM-файл приватного ключа клиента
							case static_cast <uint8_t> (type_t::PEM): {
								// Если приватный ключ не может быть установлен
								if(::SSL_CTX_use_PrivateKey_file(member->ctx, filename.data(), SSL_FILETYPE_PEM) != 1){
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Private key cannot be set");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
									// Выходим
									return;
								}
							} break;
							// ASN1-файл приватного ключа клиента
							case static_cast <uint8_t> (type_t::ASN1): {
								// Если приватный ключ не может быть установлен
								if(::SSL_CTX_use_PrivateKey_file(member->ctx, filename.data(), SSL_FILETYPE_ASN1) != 1){
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Private key cannot be set");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
									// Выходим
									return;
								}
							} break;
						}
						// Если приватный ключ недействителен
						if(::SSL_CTX_check_private_key(member->ctx) != 1){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Private key is not valid");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						/**
						 * Определяем тип файла приватного ключа клиента
						 */
						switch(static_cast <uint8_t> (type)){
							// PEM-файл приватного ключа клиента
							case static_cast <uint8_t> (type_t::PEM): {
								// Если приватный ключ не может быть установлен
								if(::SSL_use_PrivateKey_file(member->ssl, filename.data(), SSL_FILETYPE_PEM) != 1){
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Private key cannot be set");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
									// Выходим
									return;
								}
							} break;
							// ASN1-файл приватного ключа клиента
							case static_cast <uint8_t> (type_t::ASN1): {
								// Если приватный ключ не может быть установлен
								if(::SSL_use_PrivateKey_file(member->ssl, filename.data(), SSL_FILETYPE_ASN1) != 1){
									// Если функция обратного вызова состояния установлена
									if(member->callback.state != nullptr)
										// Вызываем функцию обратного вызова состояния
										member->callback.state(id, state_t::FAILED);
									// Получаем текст ошибки
									const string error = ::ssl::error(id, "Private key cannot be set");
									// Если функция обратного вызова ошибки установлена
									if(member->callback.error != nullptr)
										// Вызываем функцию обратного вызова ошибки
										member->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
									// Если функция обратного вызова ошибки не установлена
									else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
										#endif
									}
									// Выходим
									return;
								}
							} break;
						}
						// Если приватный ключ недействителен
						if(::SSL_check_private_key(member->ssl) != 1){
							// Если функция обратного вызова состояния установлена
							if(member->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								member->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Private key is not valid");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки клиентского сертификата
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу клиентского сертификата
 * @param type     тип файла клиентского сертификата
 *
 */
void awh::tls::Coder::certificate(const id_t id, string_view filename, const type_t type) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу сертификата указан
		if(!filename.empty()){
			// Выполняем закрепление участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Выполняем извлечение объекта шаблона контекста безопасности
						auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								/**
								 * Определяем тип файла приватного ключа клиента
								 */
								switch(static_cast <uint8_t> (type)){
									// PEM-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::PEM): {
										// Если сертификат не устанавливается
										if(::SSL_CTX_use_certificate_file(member->ctx, filename.data(), SSL_FILETYPE_PEM) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
									// ASN1-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::ASN1): {
										// Если сертификат не устанавливается
										if(::SSL_CTX_use_certificate_file(member->ctx, filename.data(), SSL_FILETYPE_ASN1) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
								}
							} break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								/**
								 * Определяем тип файла приватного ключа клиента
								 */
								switch(static_cast <uint8_t> (type)){
									// PEM-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::PEM): {
										// Если сертификат не устанавливается
										if(::SSL_CTX_use_certificate_chain_file(member->ctx, filename.data()) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
									// ASN1-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::ASN1): {
										// Если сертификат не устанавливается
										if(::SSL_CTX_use_certificate_file(member->ctx, filename.data(), SSL_FILETYPE_ASN1) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
								}
							} break;
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Выполняем извлечение объекта транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						/**
						 * Определяем узел события к которому относится контекст TLS
						 */
						switch(static_cast <uint8_t> (member->node)){
							// Если узел является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								/**
								 * Определяем тип файла приватного ключа клиента
								 */
								switch(static_cast <uint8_t> (type)){
									// PEM-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::PEM): {
										// Если сертификат не устанавливается
										if(::SSL_use_certificate_file(member->ssl, filename.data(), SSL_FILETYPE_PEM) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
									// ASN1-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::ASN1): {
										// Если сертификат не устанавливается
										if(::SSL_use_certificate_file(member->ssl, filename.data(), SSL_FILETYPE_ASN1) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
								}
							} break;
							// Если узел является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								/**
								 * Определяем тип файла приватного ключа клиента
								 */
								switch(static_cast <uint8_t> (type)){
									// PEM-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::PEM): {
										// Если сертификат не устанавливается
										if(::ssl::useCertificateChainFile(member->ssl, filename.data()) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
									// ASN1-файл приватного ключа клиента
									case static_cast <uint8_t> (type_t::ASN1): {
										// Если сертификат не устанавливается
										if(::SSL_use_certificate_file(member->ssl, filename.data(), SSL_FILETYPE_ASN1) != 1){
											// Если функция обратного вызова состояния установлена
											if(member->callback.state != nullptr)
												// Вызываем функцию обратного вызова состояния
												member->callback.state(id, state_t::FAILED);
											// Получаем текст ошибки
											const string error = ::ssl::error(id, "Certificate cannot be set");
											// Если функция обратного вызова ошибки установлена
											if(member->callback.error != nullptr)
												// Вызываем функцию обратного вызова ошибки
												member->callback.error(id, error_t::CERT_FAILED, error);
											// Если функция обратного вызова ошибки не установлена
											else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.c_str());
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
												#endif
											}
										}
									} break;
								}
							} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, filename, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вызова получения данных
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::Coder::on(const id_t id, read_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if((result = (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL))){
				// Выполняем извлечение объекта транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Устанавливаем функцию обратного вызова получения данных
				member->callback.read = ::move(callback);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вызова передачи данных
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::Coder::on(const id_t id, write_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if((result = (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL))){
				// Выполняем извлечение объекта транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Устанавливаем функцию обратного вызова передачи данных
				member->callback.write = ::move(callback);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вызова изменения состояния
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::Coder::on(const id_t id, state_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if((result = (pin != nullptr))){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Устанавливаем функцию обратного вызова изменения состояния
					member->callback.state = ::move(callback);
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Устанавливаем функцию обратного вызова изменения состояния
					member->callback.state = ::move(callback);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вызова получения ошибок
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::Coder::on(const id_t id, error_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if((result = (pin != nullptr))){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Выполняем извлечение объекта шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Устанавливаем функцию обратного вызова получения ошибок
					member->callback.error = ::move(callback);
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Выполняем извлечение объекта транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Устанавливаем функцию обратного вызова получения ошибок
					member->callback.error = ::move(callback);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вызова получения снимка браузера приславшего ClientHello
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::Coder::on(const id_t id, fingerprint_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем закрепление участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if((result = (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL))){
				// Выполняем извлечение объекта транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Устанавливаем функцию обратного вызова получения снимка браузера приславшего ClientHello
				member->callback.fingerprint = ::move(callback);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::tls::Coder::Coder(const fmk_t * fmk, const log_t * log) noexcept :
 _addr(fmk, log), _compressor(log), _fgp(nullptr), _fmk(fmk), _log(log) {
	/**
	 * Выполняем одноразовую инициализацию TLS-модуля для всех экземпляров класса Coder
	 */
	std::call_once(::__awh_ssl_init_once__, [this]() noexcept {
		// Активируем работу мьютекса блокировки глобального состояния TLS
		::__awh_ssl_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Увеличиваем счётчик инициализации библиотеки OpenSSL
		const bool needInit = ::ssl::registry::acquireInit();
		// Если библиотека OpenSSL ещё не инициализирована
		if(needInit)
			// Выполняем одноразовую инициализацию OpenSSL
			::ssl::initOpenSSL(this->_log);
	});
}
/**
 * @brief Конструктор
 *
 * @param fgp объект для работы с отпечатками TLS
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::tls::Coder::Coder(const fgp_t * fgp, const fmk_t * fmk, const log_t * log) noexcept :
 _addr(fmk, log), _compressor(log), _fgp(fgp), _fmk(fmk), _log(log) {
	// Если объект для работы с отпечатками TLS установлен
	if(this->_fgp != nullptr)
		// Устанавливаем режим безопасности работы потоков для хранилища отпечатков
		const_cast <fgp_t *> (this->_fgp)->threadSafety(::__awh_thread_safety__ == event::mode_t::ENABLED);
	/**
	 * Выполняем одноразовую инициализацию TLS-модуля для всех экземпляров класса Coder
	 */
	std::call_once(::__awh_ssl_init_once__, [this]() noexcept {
		// Активируем работу мьютекса блокировки глобального состояния TLS
		::__awh_ssl_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Увеличиваем счётчик инициализации библиотеки OpenSSL
		const bool needInit = ::ssl::registry::acquireInit();
		// Если библиотека OpenSSL ещё не инициализирована
		if(needInit)
			// Выполняем одноразовую инициализацию OpenSSL
			::ssl::initOpenSSL(this->_log);
	});
}
/**
 * @brief Деструктор
 *
 */
awh::tls::Coder::~Coder() noexcept {
	// Уменьшаем счётчик инициализации библиотеки OpenSSL
	::ssl::registry::releaseInit();
}
