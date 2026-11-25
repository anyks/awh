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
 *        верификация пиров и выполнение защищённого рукопожатия поверх A-NTS
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <ctime>
#include <deque>
#include <atomic>
#include <memory>
#include <csignal>
#include <cstring>
#include <algorithm>
#include <shared_mutex>
#include <net/event.hpp>
#include <unordered_set>
#include <unordered_map>

/**
 * Заголовочные файлы библиотеки A-NTS
 */
#include <pem/pem.hpp>
#include <hpke/hpke.hpp>
#include <x509/x509.hpp>
#include <x509/crl.hpp>
#include <protect/protect.hpp>
#include <extension/extension.hpp>
#include <anyks/nts/client/client.hpp>
#include <anyks/nts/server/server.hpp>

/**
 * @brief Для операционной системы MS Windows
 *
 * @note Заголовки arpa/inet.h и netinet/in.h принадлежат POSIX и у MS Windows
 *       отсутствуют. Соответствующие им объявления, включая порядок байтов сети,
 *       приходят там из winsock2.h
 *
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/macro/win32.hpp>

	/**
	 * @brief Системный заголовочный файл хранилища сертификатов
	 *
	 * @note Подключается отдельно: выключатель WIN32_LEAN_AND_MEAN, каким пользуется
	 *       sys/macro/win32.hpp, исключает подсистему шифрования из состава windows.h,
	 *       и без него не находятся ни HCERTSTORE, ни CertOpenSystemStore
	 *
	 */
	#include <wincrypt.h>

	/**
	 * @brief Снимаем макросы, чьи имена заняты типами OpenSSL
	 *
	 * @details Заголовок wincrypt.h заводит эти имена макросами-постоянными, например
	 *          "#define X509_NAME ((LPCSTR) 7)", а у OpenSSL это типы. Препроцессор
	 *          заменяет имя прежде разбора, и объявление вида
	 *          "X509_NAME * subject = X509_get_subject_name(x509)" превращается в
	 *          "((LPCSTR) 7) * subject = ...", отчего сборка отвечает отказом
	 *          "'subject' was not declared in this scope".
	 *
	 *          Снятие здесь постоянное и возврата не требует: файл этот работает с
	 *          OpenSSL, а не с подсистемой шифрования MS Windows, и постоянные эти
	 *          ему не нужны. Хранилище сертификатов системы, ради какого wincrypt.h
	 *          и подключён, обращается к именам иным - HCERTSTORE, CertOpenSystemStore,
	 *          CertEnumCertificatesInStore.
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
#include <sys/fs.hpp>
#include <sys/os.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/locker.hpp>
#include <sys/macro/global.hpp>
#include <net/net.hpp>

/**
 * Подключаем заголовочный файл модуля
 */
#include <cryptography/tls/coder.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Если максимальный размер SSL буфера не определён
 */
#if !defined(AWH_MAX_SSL_BUFFER_SIZE)
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
	static members_t __awh_ssl_members__;
	/**
	 * @brief Глобальный набор идентификаторов контекстов TLS
	 *
	 */
	static unordered_set <::tls::coder::id_t> __awh_ssl_ids__;
	/**
	 * @brief Глобальная карта сплайса контекстов TLS
	 *
	 */
	static map <pair <event::protocol_t, string>, ::tls::coder::id_t> __awh_ssl_splice_map__;
};

/**
 * @brief Инкапсулируем статические локальные хранилища потоков в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Объект работы с сетевыми адресами
	 *
	 * @details Прежде принадлежал объекту кодера, а объект упразднён: контексты
	 *          TLS общие на процесс, а адрес нужен как временная запись -
	 *          разобрать строку при установке адреса собеседника. Оттого запись
	 *          эта не своя у объекта, а своя у каждого потока
	 *
	 */
	static thread_local net_addr_t __awh_addr__;
	/**
	 * @brief Объект работы с компрессией сертификатов
	 *
	 * @details Адрес объекта сохраняется в данных SSL на время рукопожатия,
	 *          поэтому запись эта живёт столько же, сколько поток, завёдший
	 *          контекст TLS
	 *
	 */
	static thread_local compressor::block_t __awh_compressor__;
	/**
	 * @brief Объект для работы с отпечатками TLS
	 *
	 * @details Задаётся функцией fingerprint() и принадлежит тому потоку,
	 *          который его задал: у каждого потока свой обмен и свои отпечатки
	 *
	 */
	static thread_local const nts::fingerprint::Store * __awh_fgp__ = nullptr;
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
	static once_flag __awh_ssl_init_once__;
	/**
	 * @brief Мьютекс блокировки глобального состояния TLS-модуля
	 *
	 */
	static lock_state_t <std::shared_mutex> __awh_ssl_mutex__;
	/**
	 * @brief Режим безопасности работы потоков TLS-модуля
	 *
	 */
	static event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;
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
		 * @brief Функция получения разделяемой блокировки глобального реестра TLS
		 *
		 * @return объект блокировки
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ global_lock_t globalShared() noexcept {
			// Возвращаем объект блокировки глобального реестра TLS
			return global_lock_t(::__awh_ssl_mutex__, global_lock_t::mode_t::SHARED);
		}
		/**
		 * @brief Функция получения эксклюзивной блокировки глобального реестра TLS
		 *
		 * @return объект блокировки
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ global_lock_t globalExclusive() noexcept {
			// Возвращаем объект блокировки глобального реестра TLS
			return global_lock_t(::__awh_ssl_mutex__, global_lock_t::mode_t::EXCLUSIVE);
		}
		/**
		 * @brief Функция проверки регистрации идентификатора TLS
		 *
		 * @param id идентификатор контекста TLS
		 * @return   результат проверки
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ bool contains(const ::tls::coder::id_t id) noexcept {
			// Блокируем разделяемо глобальный реестр TLS
			const global_lock_t lock = globalShared();
			// Возвращаем результат проверки регистрации идентификатора TLS
			return ::__awh_ssl_ids__.find(id) != ::__awh_ssl_ids__.end();
		}
		/**
		 * @brief Функция регистрации идентификатора TLS
		 *
		 * @param id идентификатор контекста TLS
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ void add(const ::tls::coder::id_t id) noexcept {
			// Блокируем эксклюзивно глобальный реестр TLS
			const global_lock_t lock = globalExclusive();
			// Регистрируем идентификатор контекста TLS
			::__awh_ssl_ids__.emplace(id);
		}
		/**
		 * @brief Функция удаления участника из глобального реестра TLS
		 *
		 * @param id       идентификатор контекста TLS
		 * @param members  контейнер участников обмена защищёнными данными
		 * @param iterator итератор удаляемого участника
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ void drop(const ::tls::coder::id_t id, members_t & members, const members_t::iterator & iterator) noexcept {
			// Блокируем эксклюзивно глобальный реестр TLS
			const global_lock_t lock = globalExclusive();
			// Ищем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto iter = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(iter != ::__awh_ssl_ids__.end())
				// Удаляем идентификатор контекста TLS из глобального набора идентификаторов контекстов TLS
				::__awh_ssl_ids__.erase(iter);
			// Удаляем уровень защищённых сокетов из контейнера
			members.erase(iterator);
		}
		/**
		 * @brief Функция добавления участника в глобальный контейнер TLS
		 *
		 * @param item объект участника обмена защищёнными данными
		 * @return   итератор добавленного участника
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ members_t::iterator emplace(members_t::value_type item) noexcept {
			// Блокируем эксклюзивно глобальный реестр TLS
			const global_lock_t lock = globalExclusive();
			// Добавляем участника в глобальный контейнер и возвращаем итератор
			return ::__awh_ssl_members__.emplace(std::move(item)).first;
		}
		/**
		 * @brief Функция удаления записи из карты сплайса TLS
		 *
		 * @param proto тип протокола события
		 * @param host  имя хоста
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ void spliceErase(const event::protocol_t proto, const std::string & host) noexcept {
			// Блокируем эксклюзивно глобальный реестр TLS
			const global_lock_t lock = globalExclusive();
			// Ищем запись в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
			auto iter = ::__awh_ssl_splice_map__.find(std::make_pair(proto, host));
			// Если запись найдена
			if(iter != ::__awh_ssl_splice_map__.end())
				// Удаляем запись из глобальной карты сопоставления имён хостов и идентификаторов узлов TLS
				::__awh_ssl_splice_map__.erase(iter);
		}
		/**
		 * @brief Функция добавления записи в карту сплайса TLS
		 *
		 * @param proto тип протокола события
		 * @param host  имя хоста
		 * @param id    идентификатор контекста TLS
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ void spliceEmplace(const event::protocol_t proto, const std::string & host, const ::tls::coder::id_t id) noexcept {
			// Блокируем эксклюзивно глобальный реестр TLS
			const global_lock_t lock = globalExclusive();
			// Добавляем запись в глобальную карту сопоставления имён хостов и идентификаторов узлов TLS
			::__awh_ssl_splice_map__.emplace(std::make_pair(proto, host), id);
		}
		/**
		 * @brief Функция поиска идентификатора контекста TLS в карте сплайса
		 *
		 * @param proto тип протокола события
		 * @param host  имя хоста
		 * @return      идентификатор контекста TLS или 0
		 *
		 */
		[[maybe_unused]] static __AWH_INLINE__ ::tls::coder::id_t spliceResolve(const event::protocol_t proto, const std::string & host) noexcept {
			// Блокируем разделяемо глобальный реестр TLS
			const global_lock_t lock = globalShared();
			// Ищем запись в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
			auto iter = ::__awh_ssl_splice_map__.find(std::make_pair(proto, host));
			// Если запись не найдена
			if(iter == ::__awh_ssl_splice_map__.end())
				// Возвращаем нулевой идентификатор
				return 0;
			// Если идентификатор контекста TLS не зарегистрирован
			if(::__awh_ssl_ids__.find(iter->second) == ::__awh_ssl_ids__.end())
				// Возвращаем нулевой идентификатор
				return 0;
			// Возвращаем идентификатор контекста TLS
			return iter->second;
		}
	}
	/**
	 * @brief Функция одноразовой инициализации TLS-модуля
	 *
	 * @details Прежде инициализацию выполнял конструктор объекта кодера,
	 *          а объект упразднён: функция зовётся при заведении первого
	 *          контекста TLS и выполняется единожды на процесс.
	 *
	 */
	[[maybe_unused]] static __AWH_INLINE__ void init() noexcept {
		// Инициализируем TLS-модуль
		std::call_once(::__awh_ssl_init_once__, []() noexcept {
			// Активируем работу мьютекса блокировки глобального состояния TLS
			::__awh_ssl_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
			/**
			 * @brief Для операционной системы не являющейся MS Windows
			 *
			 * @note Сигнала SIGPIPE у MS Windows не существует: запись в закрытое соединение
			 *       отвечает там кодом ошибки, а не сигналом, и гасить нечего
			 *
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Гасим сигналы SIGPIPE
				if(::signal(SIGPIPE, SIG_IGN) == SIG_ERR){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						log::debug("Failed to ignore signal SIGPIPE", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Failed to ignore signal SIGPIPE", log::flag_t::CRITICAL);
					#endif
				}
			#endif
		});
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
		::tls::coder::error_callback_t error;
		// Функция обратного вызова при изменении состояния
		::tls::coder::state_callback_t state;
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
		::tls::coder::read_callback_t read;
		// Функция обратного вызова записи данных
		::tls::coder::write_callback_t write;
		// Функция обратного вызова получения снимка браузера приславшего ClientHello
		::tls::coder::fingerprint_callback_t fingerprint;
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
			// Идентификатор цифрового отпечатка браузера
			uint8_t fid;
			// Объект состояния
			uint8_t state;
			// Уровень транспортной безопасности
			layer_t layer;
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
			 * @brief Деструктор
			 *
			 */
			virtual ~Member() noexcept = default;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Member(const layer_t layer) noexcept :
			 fid(0), state(0), layer(layer), refs(0),
			 node(event::node_t::NONE),
			 proto(event::protocol_t::NONE) {}
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
			static_cast <::tls::coder::id_t> (reinterpret_cast <uintptr_t> ((* this->iterator).get())),
			members,
			this->iterator
		);
	}

	/**
	 * @brief Структура шаблона контекста безопасности
	 *
	 */
	typedef struct Contex_Template_Security : public member_t {
		// Признак выработки следующего протокола NPN
		bool npn;
		// Признак запроса меток времени сертификата
		bool sct;
		// Признак запроса состояния сертификата
		bool ocsp;
		// Признак вставки значений GREASE
		bool grease;
		// Признак перемешивания расширений
		bool permute;
		// Наименьшее дозволенное издание протокола
		uint16_t lowest;
		// Наибольшее дозволенное издание протокола
		uint16_t highest;
		// Объект ALPN-протоколов
		alpn_t alpn;
		// Объект хоста сервера
		string host;
		// Функции обратных вызовов
		callback_t callback;
		// Запас доверенных центров и списков отзыва
		nts::trust::store_t store;
		// Точка кода расширения ALPS
		nts::extension::type_t alpsType;
		// Подлинность своей стороны связи
		nts::context::credential_t credential;
		// Выданные сервером билеты возобновления с контекстом ранних данных их связей
		deque <pair <string, string>> issued;
		// Кэш билетов возобновления сессии по ключу сервера
		unordered_map <string, string> sessions;
		// ECHConfigList для клиентов
		vector <uint8_t> ech;
		// Наборы шифрования от предпочтительного к запасному
		vector <nts::protect::suite_t> suites;
		// Алгоритмы сжатия цепочки сертификатов
		vector <nts::context::codec_t> codecs;
		// Кривые обмена ключами
		vector <nts::extension::group_t> groups;
		// Группы, по которым приветствием шлются доли ключа
		vector <nts::extension::group_t> shares;
		// Схемы подписи шифрования
		vector <nts::extension::scheme_t> schemes;
		// Настройки ALPS по прикладным протоколам
		vector <pair <string, vector <uint8_t>>> alps;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Contex_Template_Security() noexcept :
		 member_t(layer_t::CTS),
		 npn(false), sct(false), ocsp(false),
		 grease(false), permute(false),
		 lowest(0x0303), highest(0x0304), host{""},
		 alpsType(nts::extension::type_t::SETTINGS) {}
		/**
		 * @brief Деструктор
		 *
		 */
		~Contex_Template_Security() noexcept = default;
	} cts_t;
	/**
	 * @brief Структура транспортного уровня передачи
	 *
	 */
	typedef struct Content_Transfer_Layer : public member_t {
		// Объект хоста сервера
		host_t host;
		// Объект ALPN-протоколов
		alpn_t alpn;
		// Шаблон контекста безопасности, из которого заведена связь
		cts_t * cts;
		// ECHConfigList (копия из CTS при создании CTL)
		vector <uint8_t> ech;
		// Буфер сборки TLS/DTLS record для apply()
		vector <uint8_t> hello;
		// Объект обратных вызовов
		callback_transfer_t callback;
		// Связь клиента протокола TLS
		unique_ptr <nts::client::context_t> client;
		// Связь сервера протокола TLS
		unique_ptr <nts::server::context_t> server;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Content_Transfer_Layer() noexcept :
		 member_t(layer_t::CTL),
		 cts(nullptr), client(nullptr), server(nullptr) {}
		/**
		 * @brief Деструктор
		 *
		 */
		~Content_Transfer_Layer() noexcept = default;
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
	 * @brief Флаг отложенного заведения связи сервера
	 *
	 * @details Сервер дейтаграммного транспорта заводится лишь после установки адреса
	 *          собеседника: метка возврата состояния привязывается к нему, и без адреса
	 *          старт отвечает отказом настроек (раздел 5.1 RFC 9147). Адрес принятого
	 *          соединения известен позже заведения записи, и связь ждёт его в работе
	 *          установки собеседника.
	 *
	 */
	static constexpr uint8_t START_PENDING = 0x04;
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
	/**
	 * @brief Флаг снятия участника вызывающей стороной
	 *
	 * @details Признак мусорного участника ставится и самими работами обмена - всякая
	 *          неустранимая ошибка рукопожатия, шифрования и расшифровки метит участника
	 *          им. Освобождать по нему память нельзя: опознавателем владеет вызывающая
	 *          сторона, и работы обмена реестра не проверяют (горячий путь), оттого
	 *          запоздалый вызов читал бы освобождённое молча.
	 *
	 *          Признак этот ставится ТОЛЬКО снятием по требованию вызывающей стороны, и
	 *          лишь по нему охранник возвращает память: пока опознаватель не снят, тело
	 *          участника остаётся на месте, и запоздалый вызов приходит в живую память,
	 *          а не в отданную.
	 *
	 */
	static constexpr uint8_t RELEASED_MODE = 0x20;
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
	 * @brief Функция проверки статуса участника обмена как мусорного
	 *
	 * @details Покуда не зовётся: деструктор охранника сличает то же условие сам, и
	 *          порог счётчика ссылок у него иной - нуль после снятия своей ссылки
	 *          против единицы здесь. Пометка объявляет неупотребление намеренным.
	 *
	 * @return результат проверки
	 *
	 */
	[[maybe_unused]] bool Guard_Transport_Layer_Security::garbage() const noexcept {
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
			// Выходим из функции из конструктора - закреплять нечего
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
			// Выходим из функции из деструктора - закрепление не выполнялось
			return;
		// Уменьшаем счётчик ссылок участника обмена
		this->_member->refs.fetch_sub(1, std::memory_order_release);
		/**
		 * @brief Если счётчик ссылок участника обмена равен нулю и участник снят вызывающей стороной
		 *
		 * @details Память возвращается по признаку снятия вызывающей стороной, а не по признаку
		 *          мусорного: мусорным участника метят и сами работы обмена на пути ошибки, а
		 *          опознавателем в этот миг ещё владеет вызывающая сторона.
		 *
		 */
		if((this->_member->state & ::state::RELEASED_MODE) &&
		   (this->_member->refs.load(std::memory_order_acquire) == 0))
			// Удаляем контекст TLS из контейнера уровней защищённых сокетов
			this->_member->erase(::__awh_ssl_members__);
	}
};

/**
 * @brief Инкапсулируем статические локальные хранилища потоков в пространство имён локальных переменных
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
		 * @brief Структура закрепления участника в глобальном реестре TLS
		 *
		 */
		typedef class Pin {
			private:
				// Охранник участника обмена защищёнными данными
				::local::guard_t _guard;
			public:
				/**
				 * @brief Конструктор копирования
				 *
				 */
				Pin(const Pin &) = delete;
				/**
				 * @brief Конструктор перемещения
				 *
				 */
				Pin(Pin &&) noexcept = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param member объект участника обмена защищёнными данными
				 *
				 */
				explicit Pin(::member_t * member) noexcept : _guard(member) {}
		} pin_t;

		/**
		 * @brief Функция закрепления участника в глобальном реестре TLS
		 *
		 * @details Помеченный мусорным участник закреплению обычно не подлежит: работать
		 *          с ним более нечем. Исключение одно - снятие по требованию вызывающей
		 *          стороны: пометить участника мусорным могли и сами работы обмена на
		 *          пути ошибки, а память возвращается только снятием, и отказ в
		 *          закреплении оставил бы её удержанной навсегда.
		 *
		 * @param id      идентификатор контекста TLS
		 * @param garbage признак допуска участника, помеченного мусорным
		 * @return        объект закрепления или nullptr
		 *
		 */
		static __AWH_INLINE__ unique_ptr <pin_t> pin(const ::tls::coder::id_t id, const bool garbage = false) noexcept {
			// Эксклюзивная блокировка: find в ids и refs++ должны быть атомарны относительно drop()
			const global_lock_t lock = globalExclusive();
			// Если идентификатор контекста TLS не зарегистрирован
			if(::__awh_ssl_ids__.find(id) == ::__awh_ssl_ids__.end())
				// Возвращаем пустой результат
				return nullptr;
			// Получаем объект участника обмена защищёнными данными
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Если участник помечен на удаление, а допуск таких не запрошен
			if(!garbage && (member->state & ::state::GARBAGE_MODE))
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
	static __AWH_INLINE__ string error(const ::tls::coder::id_t id, string_view message = "") noexcept {
		// Переменная результата
		string result = "";
		/**
		 * Определяем уровень транспортной безопасности
		 */
		switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
			// Если уровень является шаблонным контекстом безопасности
			case static_cast <uint8_t> (layer_t::CTS):
				// Записываем в лог сообщение об ошибке как оно передано
				return string{message};
			// Если уровень является транспортной передачей данных
			case static_cast <uint8_t> (layer_t::CTL): {
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				// Если сообщение об ошибке передано
				if(!message.empty())
					// Добавляем сообщение об ошибке в результат
					result.append(message.data(), message.size());
				// Если узел является клиентом и связь заведена
				if((member->node == event::node_t::CLIENT) && (member->client != nullptr)){
					// Получаем отказ связи
					const nts::context::error_t code = member->client->error();
					// Если отказ получен
					if(code != nts::context::error_t::NONE){
						// Если результат уже сформирован
						if(!result.empty())
							// Добавляем разделитель
							result.append(": ");
						// Добавляем код отказа в результат
						result.append(awh::fmk::format("code %u", static_cast <uint8_t> (code)));
					}
				// Если узел является сервером и связь заведена
				} else if((member->node == event::node_t::SERVER) && (member->server != nullptr)) {
					// Получаем отказ связи
					const nts::context::error_t code = member->server->error();
					// Если отказ получен
					if(code != nts::context::error_t::NONE){
						// Если результат уже сформирован
						if(!result.empty())
							// Добавляем разделитель
							result.append(": ");
						// Добавляем код отказа в результат
						result.append(awh::fmk::format("code %u", static_cast <uint8_t> (code)));
					}
				}
			} break;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция выдачи накопленных байтов отправки через callback
	 *
	 * @param member объект транспортного уровня передачи
	 * @param id     идентификатор события
	 * @return       результат выполнения выдачи
	 *
	 */
	static __AWH_INLINE__ bool emitOutput(::ctl_t * member, const ::tls::coder::id_t id) noexcept {
		// Если узел является клиентом и связь заведена
		if((member->node == event::node_t::CLIENT) && (member->client != nullptr)){
			/**
			 * Выдаём накопленные байты из накопителя связи
			 */
			while(true){
				// Получаем накопленные байты отправки
				const nts::view_t view = member->client->output();
				// Если байтов для отправки нет
				if(view.size == 0)
					// Выходим из цикла
					break;
				// Если функция обратного вызова чтения данных установлена
				if(member->callback.read != nullptr)
					// Вызываем функцию обратного вызова чтения данных
					member->callback.read(id, ::tls::coder::event_t::ENCRYPTION, view.data, view.size);
				// Снимаем отправленные байты с накопителя
				member->client->consume(view.size);
			}
			// Возвращаем положительный результат
			return true;
		}
		// Если узел является сервером и связь заведена
		if((member->node == event::node_t::SERVER) && (member->server != nullptr)){
			/**
			 * Выдаём накопленные байты из накопителя связи
			 */
			while(true){
				// Получаем накопленные байты отправки
				const nts::view_t view = member->server->output();
				// Если байтов для отправки нет
				if(view.size == 0)
					// Выходим из цикла
					break;
				// Если функция обратного вызова чтения данных установлена
				if(member->callback.read != nullptr)
					// Вызываем функцию обратного вызова чтения данных
					member->callback.read(id, ::tls::coder::event_t::ENCRYPTION, view.data, view.size);
				// Снимаем отправленные байты с накопителя
				member->server->consume(view.size);
			}
			// Возвращаем положительный результат
			return true;
		}
		// Возвращаем отрицательный результат
		return false;
	}
	/**
	 * @brief Функция извлечения названий ALPN-протоколов из буфера провода
	 *
	 * @param alpn объект ALPN-протоколов
	 * @return     список названий ALPN-протоколов
	 *
	 */
	static __AWH_INLINE__ vector <string> alpnNames(const ::alpn_t & alpn) noexcept {
		// Переменная результата
		vector <string> result;
		// Смещение в буфере
		size_t offset = 0;
		/**
		 * Перебираем буфер ALPN-протоколов
		 */
		while(offset < alpn.buffer.size()){
			// Получаем длину протокола
			const size_t length = alpn.buffer[offset++];
			// Если длина некорректна или выходит за пределы буфера
			if((length == 0) || ((offset + length) > alpn.buffer.size()))
				// Выходим из цикла
				break;
			// Добавляем название протокола в результат
			result.emplace_back(reinterpret_cast <const char *> (&alpn.buffer[offset]), length);
			// Смещаем смещение на длину протокола
			offset += length;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Шаблон функции применения настроек шаблона к связи
	 *
	 * @tparam Client тип клиента
	 * @tparam T      тип контекста связи
	 */
	template <bool Client, typename T>
	/**
	 * @brief Функция применения настроек шаблона к связи
	 *
	 * @param ctx связь, к которой применяются настройки
	 * @param cts объект шаблона контекста безопасности
	 *
	 */
	static __AWH_INLINE__ void applySettings(T & ctx, ::cts_t * cts) noexcept {
		/**
		 * Определяем тип протокола подключения
		 */
		switch(static_cast <uint8_t> (cts->proto)){
			// Если протокол подключения UDP
			case static_cast <uint8_t> (event::protocol_t::UDP):
				// Устанавливаем перенос дейтаграммами
				ctx.transport(nts::context::transport_t::DATAGRAMS);
			break;
			// Если протокол подключения QUIC
			case static_cast <uint8_t> (event::protocol_t::QUIC):
				// Устанавливаем перенос сообщениями по уровням
				ctx.transport(nts::context::transport_t::LEVELS);
			break;
			// Во всех остальных случаях
			default:
				// Устанавливаем перенос записями протокола
				ctx.transport(nts::context::transport_t::RECORDS);
			break;
		}
		// Устанавливаем пределы изданий протокола
		ctx.versions(cts->lowest, cts->highest);
		// Если наборы шифрования заданы
		if(!cts->suites.empty())
			// Устанавливаем наборы шифрования
			ctx.suites(cts->suites);
		// Если кривые обмена ключами заданы
		if(!cts->groups.empty())
			// Устанавливаем кривые обмена ключами
			ctx.groups(cts->groups);
		// Если схемы подписи заданы
		if(!cts->schemes.empty())
			// Устанавливаем схемы подписи
			ctx.schemes(cts->schemes);
		// Если прикладные протоколы заданы
		if(!cts->alpn.buffer.empty())
			// Устанавливаем прикладные протоколы
			ctx.protocols(::ssl::alpnNames(cts->alpn));
		// Если алгоритмы сжатия цепочки сертификатов заданы
		if(!cts->codecs.empty())
			// Устанавливаем алгоритмы сжатия цепочки сертификатов
			ctx.codecs(cts->codecs, 0);
		// Если подлинность своей стороны задана
		if(!cts->credential.chain.empty())
			// Устанавливаем подлинность своей стороны
			ctx.credential(cts->credential);
		// Если вставка значений GREASE включена
		if(cts->grease)
			// Включаем вставку значений GREASE
			ctx.grease(true);
		// Если выработка следующего протокола включена
		if(cts->npn)
			// Включаем выработку следующего протокола
			ctx.negotiation(true);
		// Если запрос состояния сертификата включён
		if(cts->ocsp)
			// Включаем запрос состояния сертификата
			ctx.status(true);
		/**
		 * Объявляем настройки ALPS по прикладным протоколам
		 */
		for(const auto & item : cts->alps)
			// Объявляем настройки ALPS для протокола
			ctx.settings(item.first, item.second);
		// Если настройки ALPS заданы
		if(!cts->alps.empty())
			// Устанавливаем точку кода расширения ALPS
			ctx.codepoint(cts->alpsType);
		// Если сторона является клиентом
		if constexpr(Client){
			// Если группы долей ключа заданы
			if(!cts->shares.empty())
				// Устанавливаем группы долей ключа
				ctx.shares(cts->shares);
			// Иначе ограничиваем доли ключа классической X25519: гибридная доля не влезает в пакет Initial QUIC
			else ctx.shares(vector <nts::extension::group_t> {nts::extension::group_t::X25519});
			// Если имя узла задано
			if(!cts->host.empty())
				// Устанавливаем имя узла
				ctx.name(cts->host);
			// Если конфигурации ECH заданы
			if(!cts->ech.empty())
				// Устанавливаем конфигурации ECH
				ctx.ech(cts->ech);
			// Если запрос меток времени сертификата включён
			if(cts->sct)
				// Включаем запрос меток времени сертификата
				ctx.timestamps(true);
			// Если идентификатор цифрового отпечатка браузера установлен и хранилище отпечатков задано
			if((cts->fid != 0) && (::__awh_fgp__ != nullptr)){
				// Получаем профиль отпечатка браузера из хранилища
				const nts::fingerprint::profile_t * profile = ::__awh_fgp__->get(cts->fid);
				// Если профиль отпечатка получен
				if(profile != nullptr)
					// Накладываем профиль отпечатка на приветствие клиента
					ctx.profile(* profile);
			}
		}
	}
	/**
	 * @brief Функция вывода отказа заведения связи
	 *
	 * @param cts     объект шаблона контекста безопасности
	 * @param id      идентификатор шаблона контекста безопасности
	 * @param message сообщение об ошибке
	 * @return        результат заведения связи
	 *
	 */
	static __AWH_INLINE__ bool establishFailed(::cts_t * cts, const ::tls::coder::id_t id, string_view message) noexcept {
		// Если функция обратного вызова состояния установлена
		if(cts->callback.state != nullptr)
			// Вызываем функцию обратного вызова состояния
			cts->callback.state(id, ::tls::coder::state_t::FAILED);
		// Если функция обратного вызова ошибки установлена
		if(cts->callback.error != nullptr)
			// Вызываем функцию обратного вызова ошибки
			cts->callback.error(id, ::tls::coder::error_t::TLS_SESSION_FAILED, string{message});
		// Если функция обратного вызова ошибки не установлена
		else {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, message.data());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, message.data());
			#endif
		}
		// Возвращаем отрицательный результат
		return false;
	}
	/**
	 * @brief Функция заведения связи транспортного уровня из настроек шаблона
	 *
	 * @param member объект транспортного уровня передачи
	 * @param cts    объект шаблона контекста безопасности
	 * @param id     идентификатор шаблона контекста безопасности
	 * @return       результат заведения связи
	 *
	 */
	static __AWH_INLINE__ bool establish(::ctl_t * member, ::cts_t * cts, const ::tls::coder::id_t id) noexcept {
		// Если узел является клиентом
		if(cts->node == event::node_t::CLIENT){
			// Создаём связь клиента TLS
			member->client = ::make_unique <nts::client::context_t> ();
			// Применяем настройки шаблона к связи
			::ssl::applySettings<true>(* member->client, cts);
			/**
			 * Транспорт QUIC слой записей TLS не ведёт: рукопожатие выполняет само
			 * соединение собственной связью по уровням, и связь этого уровня заведению
			 * не подлежит - старт требует транспортных параметров, которых у записи
			 * нет (RFC 9001 §4)
			 */
			if(cts->proto != event::protocol_t::QUIC)
				// Заводим связь
				if(member->client->start() != nts::context::error_t::NONE)
					// Выводим отказ заведения связи
					return ::ssl::establishFailed(cts, id, "Could not establish TLS session");
		// Если узел является сервером
		} else if(cts->node == event::node_t::SERVER) {
			// Создаём связь сервера TLS
			member->server = ::make_unique <nts::server::context_t> ();
			// Применяем настройки шаблона к связи
			::ssl::applySettings<false>(* member->server, cts);
			/**
			 * Транспорт QUIC слой записей TLS не ведёт: рукопожатие выполняет само
			 * соединение собственной связью по уровням, и связь этого уровня заведению
			 * не подлежит - старт требует транспортных параметров, которых у записи
			 * нет (RFC 9001 §4)
			 */
			if(cts->proto != event::protocol_t::QUIC){
				/**
				 * Серверу дейтаграммного транспорта адрес собеседника нужен до заведения
				 * связи: метка возврата состояния привязывается к адресу, и без него
				 * старт отвечает отказом настроек (раздел 5.1 RFC 9147). Адрес принятого
				 * соединения известен лишь после заведения записи, поэтому старт
				 * откладывается до установки адреса собеседника
				 */
				if(cts->proto == event::protocol_t::UDP)
					// Отмечаем отложенное заведение связи сервера
					member->state |= state::START_PENDING;
				// Иначе заводим связь
				else if(member->server->start() != nts::context::error_t::NONE)
					// Выводим отказ заведения связи
					return ::ssl::establishFailed(cts, id, "Could not establish TLS session");
			}
		// Во всех остальных случаях
		} else return false;
		// Возвращаем положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки установленности связи транспортного уровня
	 *
	 * @param member объект транспортного уровня передачи
	 * @return       результат проверки
	 *
	 */
	static __AWH_INLINE__ bool established(const ::ctl_t * member) noexcept {
		// Если узел является клиентом
		if(member->node == event::node_t::CLIENT)
			// Возвращаем признак установленности связи клиента
			return (member->client != nullptr) && member->client->established();
		// Если узел является сервером
		if(member->node == event::node_t::SERVER)
			// Возвращаем признак установленности связи сервера
			return (member->server != nullptr) && member->server->established();
		// Возвращаем отрицательный результат
		return false;
	}
	/**
	 * @brief Функция проверки цепочки сертификатов собеседника запасом доверенных
	 *
	 * @param member объект транспортного уровня передачи
	 * @return       результат проверки (false - доверие цепочке не сложилось)
	 *
	 */
	static __AWH_INLINE__ bool verifyPeer(::ctl_t * member) noexcept {
		// Если проверка сертификата собеседника не требуется
		if((member->cts->state & state::CERTIFICATE_VERIFY) == 0)
			// Выводим положительный результат
			return true;
		// Цепочка сертификатов собеседника
		const vector <vector <uint8_t>> & chain = ((member->node == event::node_t::CLIENT) ? member->client->peer() : member->server->peer());
		// Роль собеседника для суждения назначения сертификата
		const nts::trust::role_t role = ((member->node == event::node_t::CLIENT) ? nts::trust::role_t::SERVER : nts::trust::role_t::CLIENT);
		// Если цепочка собеседника не прошла проверку доверия
		if(nts::trust::verify(chain, member->cts->store, ::time(nullptr), role) != nts::trust::error_t::NONE)
			// Выводим отрицательный результат
			return false;
		// Если имя узла не отвечает ни одному имени сертификата
		if(!member->cts->host.empty() && (chain.empty() || (nts::trust::belongs(nts::view_t(chain[0].data(), chain[0].size()), member->cts->host) != nts::trust::error_t::NONE)))
			// Выводим отрицательный результат
			return false;
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция подачи принятых байтов связи транспортного уровня
	 *
	 * @param member объект транспортного уровня передачи
	 * @param buffer буфер принятых байтов
	 * @param size   размер буфера принятых байтов
	 * @return       отказ разбора принятых байтов
	 *
	 */
	static __AWH_INLINE__ nts::context::error_t pushInput(::ctl_t * member, const void * buffer, const size_t size) noexcept {
		// Если узел является клиентом
		if(member->node == event::node_t::CLIENT)
			// Подаём принятые байты связи клиента
			return member->client->push(nts::view_t(reinterpret_cast <const uint8_t *> (buffer), size));
		// Если узел является сервером
		if(member->node == event::node_t::SERVER)
			// Подаём принятые байты связи сервера
			return member->server->push(nts::view_t(reinterpret_cast <const uint8_t *> (buffer), size));
		// Возвращаем отказ уровня
		return nts::context::error_t::LEVEL;
	}
	/**
	 * @brief Функция отправки прикладных данных связи транспортного уровня
	 *
	 * @param member объект транспортного уровня передачи
	 * @param buffer буфер отправляемых данных
	 * @param size   размер буфера отправляемых данных
	 * @return       отказ отправки прикладных данных
	 *
	 */
	static __AWH_INLINE__ nts::context::error_t writeData(::ctl_t * member, const void * buffer, const size_t size) noexcept {
		// Если узел является клиентом
		if(member->node == event::node_t::CLIENT)
			// Отправляем прикладные данные связи клиента
			return member->client->write(nts::view_t(reinterpret_cast <const uint8_t *> (buffer), size));
		// Если узел является сервером
		if(member->node == event::node_t::SERVER)
			// Отправляем прикладные данные связи сервера
			return member->server->write(nts::view_t(reinterpret_cast <const uint8_t *> (buffer), size));
		// Возвращаем отказ уровня
		return nts::context::error_t::LEVEL;
	}
	/**
	 * @brief Функция получения принятых прикладных данных связи транспортного уровня
	 *
	 * @param member объект транспортного уровня передачи
	 * @param result накопитель, в который переносятся принятые данные
	 * @return       отказ получения принятых прикладных данных
	 *
	 */
	static __AWH_INLINE__ nts::context::error_t readData(::ctl_t * member, vector <uint8_t> & result) noexcept {
		// Если узел является клиентом
		if(member->node == event::node_t::CLIENT)
			// Получаем принятые данные связи клиента
			return member->client->read(result);
		// Если узел является сервером
		if(member->node == event::node_t::SERVER)
			// Получаем принятые данные связи сервера
			return member->server->read(result);
		// Возвращаем отказ уровня
		return nts::context::error_t::LEVEL;
	}
	/**
	 * @brief Функция вывода отказа работы транспортного уровня
	 *
	 * @param member  объект транспортного уровня передачи
	 * @param id      идентификатор события
	 * @param error   тип ошибки TLS
	 * @param message дополнительное сообщение об ошибке
	 *
	 */
	static __AWH_INLINE__ void fail(::ctl_t * member, const ::tls::coder::id_t id, const ::tls::coder::error_t error, string_view message) noexcept {
		// Если функция обратного вызова состояния установлена
		if(member->callback.state != nullptr)
			// Вызываем функцию обратного вызова состояния
			member->callback.state(id, ::tls::coder::state_t::FAILED);
		// Получаем текст ошибки
		const string text = ::ssl::error(id, message);
		// Если функция обратного вызова ошибки установлена
		if(member->callback.error != nullptr)
			// Вызываем функцию обратного вызова ошибки
			member->callback.error(id, error, text);
		// Если функция обратного вызова ошибки не установлена
		else {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, text.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, text.c_str());
			#endif
		}
		// Если функция обратного вызова состояния установлена
		if(member->callback.state != nullptr)
			// Вызываем функцию обратного вызова на уничтожение контекста TLS
			member->callback.state(id, ::tls::coder::state_t::DESTROYED);
		// Устанавливаем режим удаления участника обмена защищёнными данными
		member->state |= ::state::GARBAGE_MODE;
	}
	/**
	 * @brief Функция получения кода кривой обмена ключами
	 *
	 * @param group группа обмена ключами AWH
	 * @return      код кривой обмена ключами A-NTS
	 *
	 */
	static constexpr nts::extension::group_t groupCode(const ::tls::group_t group) noexcept {
		/**
		 * Определяем группу обмена ключами
		 */
		switch(static_cast <uint8_t> (group)){
			// Если группа обмена ключами — кривая Curve448
			case static_cast <uint8_t> (::tls::group_t::X448):
				// Возвращаем код группы X448
				return nts::extension::group_t::X448;
			// Если группа обмена ключами — кривая NIST P-256
			case static_cast <uint8_t> (::tls::group_t::P_256):
				// Возвращаем код кривой secp256r1
				return nts::extension::group_t::SECP256R1;
			// Если группа обмена ключами — кривая NIST P-384
			case static_cast <uint8_t> (::tls::group_t::P_384):
				// Возвращаем код кривой secp384r1
				return nts::extension::group_t::SECP384R1;
			// Если группа обмена ключами — кривая NIST P-521
			case static_cast <uint8_t> (::tls::group_t::P_521):
				// Возвращаем код кривой secp521r1
				return nts::extension::group_t::SECP521R1;
			// Если группа обмена ключами — кривая Curve25519
			case static_cast <uint8_t> (::tls::group_t::X25519):
				// Возвращаем код группы X25519
				return nts::extension::group_t::X25519;
			// Если группа обмена ключами — кривая ГОСТ на 256 бит, вариант A
			case static_cast <uint8_t> (::tls::group_t::GC256A):
				// Возвращаем код кривой ГОСТ GC256A
				return nts::extension::group_t::GC256A;
			// Если группа обмена ключами — кривая ГОСТ на 256 бит, вариант B
			case static_cast <uint8_t> (::tls::group_t::GC256B):
				// Возвращаем код кривой ГОСТ GC256B
				return nts::extension::group_t::GC256B;
			// Если группа обмена ключами — кривая ГОСТ на 256 бит, вариант C
			case static_cast <uint8_t> (::tls::group_t::GC256C):
				// Возвращаем код кривой ГОСТ GC256C
				return nts::extension::group_t::GC256C;
			// Если группа обмена ключами — кривая ГОСТ на 256 бит, вариант D
			case static_cast <uint8_t> (::tls::group_t::GC256D):
				// Возвращаем код кривой ГОСТ GC256D
				return nts::extension::group_t::GC256D;
			// Если группа обмена ключами — кривая ГОСТ на 512 бит, вариант A
			case static_cast <uint8_t> (::tls::group_t::GC512A):
				// Возвращаем код кривой ГОСТ GC512A
				return nts::extension::group_t::GC512A;
			// Если группа обмена ключами — кривая ГОСТ на 512 бит, вариант B
			case static_cast <uint8_t> (::tls::group_t::GC512B):
				// Возвращаем код кривой ГОСТ GC512B
				return nts::extension::group_t::GC512B;
			// Если группа обмена ключами — кривая ГОСТ на 512 бит, вариант C
			case static_cast <uint8_t> (::tls::group_t::GC512C):
				// Возвращаем код кривой ГОСТ GC512C
				return nts::extension::group_t::GC512C;
			// Если группа обмена ключами — кривая secp256k1
			case static_cast <uint8_t> (::tls::group_t::SECP256K1):
				// Возвращаем код кривой secp256k1
				return nts::extension::group_t::SECP256K1;
			// Если группа обмена ключами — конечное поле FFDHE на 2048 бит
			case static_cast <uint8_t> (::tls::group_t::FFDHE2048):
				// Возвращаем код группы ffdhe2048
				return nts::extension::group_t::FFDHE2048;
			// Если группа обмена ключами — конечное поле FFDHE на 3072 бит
			case static_cast <uint8_t> (::tls::group_t::FFDHE3072):
				// Возвращаем код группы ffdhe3072
				return nts::extension::group_t::FFDHE3072;
			// Если группа обмена ключами — конечное поле FFDHE на 4096 бит
			case static_cast <uint8_t> (::tls::group_t::FFDHE4096):
				// Возвращаем код группы ffdhe4096
				return nts::extension::group_t::FFDHE4096;
			// Если группа обмена ключами — конечное поле FFDHE на 6144 бит
			case static_cast <uint8_t> (::tls::group_t::FFDHE6144):
				// Возвращаем код группы ffdhe6144
				return nts::extension::group_t::FFDHE6144;
			// Если группа обмена ключами — конечное поле FFDHE на 8192 бит
			case static_cast <uint8_t> (::tls::group_t::FFDHE8192):
				// Возвращаем код группы ffdhe8192
				return nts::extension::group_t::FFDHE8192;
			// Если группа обмена ключами — гибридная X25519 с инкапсуляцией ML-KEM-768
			case static_cast <uint8_t> (::tls::group_t::X25519_MLKEM768):
				// Возвращаем код гибридной группы X25519MLKEM768
				return nts::extension::group_t::X25519MLKEM768;
			// В остальных случаях возвращаем признак отсутствия группы
			default: return nts::extension::group_t::NONE;
		}
	}
	/**
	 * @brief Функция получения кода набора шифрования
	 *
	 * @param cipher набор шифрования AWH
	 * @return       код набора шифрования A-NTS
	 *
	 */
	static constexpr nts::protect::suite_t suiteCode(const ::tls::cipher_t cipher) noexcept {
		/**
		 * Определяем набор шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если набор шифрования — RSA с AES-128-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::AES128_SHA):
				// Возвращаем код набора TLS_RSA_WITH_AES_128_CBC_SHA
				return nts::protect::suite_t::RSA_AES_128_CBC_SHA;
			// Если набор шифрования — RSA с AES-256-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::AES256_SHA):
				// Возвращаем код набора TLS_RSA_WITH_AES_256_CBC_SHA
				return nts::protect::suite_t::RSA_AES_256_CBC_SHA;
			// Если набор шифрования — RSA с AES-128-GCM и SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::AES128_GCM_SHA256):
				// Возвращаем код набора TLS_RSA_WITH_AES_128_GCM_SHA256
				return nts::protect::suite_t::RSA_AES_128_GCM_SHA256;
			// Если набор шифрования — RSA с AES-256-GCM и SHA-384
			case static_cast <uint8_t> (::tls::cipher_t::AES256_GCM_SHA384):
				// Возвращаем код набора TLS_RSA_WITH_AES_256_GCM_SHA384
				return nts::protect::suite_t::RSA_AES_256_GCM_SHA384;
			// Если набор шифрования — PSK с AES-128-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::PSK_AES128_CBC_SHA):
				// Возвращаем код набора TLS_PSK_WITH_AES_128_CBC_SHA
				return nts::protect::suite_t::PSK_AES_128_CBC_SHA;
			// Если набор шифрования — PSK с AES-256-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::PSK_AES256_CBC_SHA):
				// Возвращаем код набора TLS_PSK_WITH_AES_256_CBC_SHA
				return nts::protect::suite_t::PSK_AES_256_CBC_SHA;
			// Если набор шифрования — ECDHE с RSA, AES-128-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_RSA_AES128_SHA):
				// Возвращаем код набора TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
				return nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA;
			// Если набор шифрования — ECDHE с RSA, AES-256-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_RSA_AES256_SHA):
				// Возвращаем код набора TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
				return nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA;
			// Если набор шифрования TLS 1.3 — AES-256-GCM с SHA-384
			case static_cast <uint8_t> (::tls::cipher_t::TLS_AES_256_GCM_SHA384):
				// Возвращаем код набора TLS_AES_256_GCM_SHA384
				return nts::protect::suite_t::AES_256_GCM_SHA384;
			// Если набор шифрования TLS 1.3 — AES-128-GCM с SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::TLS_AES_128_GCM_SHA256):
				// Возвращаем код набора TLS_AES_128_GCM_SHA256
				return nts::protect::suite_t::AES_128_GCM_SHA256;
			// Если набор шифрования — ECDHE с ECDSA, AES-128-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_ECDSA_AES128_SHA):
				// Возвращаем код набора TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
				return nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA;
			// Если набор шифрования — ECDHE с ECDSA, AES-256-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_ECDSA_AES256_SHA):
				// Возвращаем код набора TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA
				return nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA;
			// Если набор шифрования — ECDHE с RSA, AES-128-CBC и SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_RSA_AES128_SHA256):
				// Возвращаем код набора TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
				return nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256;
			// Если набор шифрования — ECDHE с PSK, AES-128-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_PSK_AES128_CBC_SHA):
				// Возвращаем код набора TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA
				return nts::protect::suite_t::ECDHE_PSK_AES_128_CBC_SHA;
			// Если набор шифрования — ECDHE с PSK, AES-256-CBC и SHA-1
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_PSK_AES256_CBC_SHA):
				// Возвращаем код набора TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA
				return nts::protect::suite_t::ECDHE_PSK_AES_256_CBC_SHA;
			// Если набор шифрования — ECDHE с ECDSA, AES-128-CBC и SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_ECDSA_AES128_SHA256):
				// Возвращаем код набора TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
				return nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256;
			// Если набор шифрования — ECDHE с RSA, AES-128-GCM и SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_RSA_AES128_GCM_SHA256):
				// Возвращаем код набора TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
				return nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256;
			// Если набор шифрования — ECDHE с RSA, AES-256-GCM и SHA-384
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_RSA_AES256_GCM_SHA384):
				// Возвращаем код набора TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
				return nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384;
			// Если набор шифрования — ECDHE с RSA и ChaCha20-Poly1305
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_RSA_CHACHA20_POLY1305):
				// Возвращаем код набора TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256
				return nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256;
			// Если набор шифрования — «Магма» в режиме MGM варианта L
			case static_cast <uint8_t> (::tls::cipher_t::GOSTR341112_256_MAGMA_MGM_L):
				// Возвращаем код набора TLS_GOSTR341112_256_WITH_MAGMA_MGM_L
				return nts::protect::suite_t::MAGMA_MGM_L;
			// Если набор шифрования — «Магма» в режиме MGM варианта S
			case static_cast <uint8_t> (::tls::cipher_t::GOSTR341112_256_MAGMA_MGM_S):
				// Возвращаем код набора TLS_GOSTR341112_256_WITH_MAGMA_MGM_S
				return nts::protect::suite_t::MAGMA_MGM_S;
			// Если набор шифрования — ECDHE с PSK и ChaCha20-Poly1305
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_PSK_CHACHA20_POLY1305):
				// Возвращаем код набора TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256
				return nts::protect::suite_t::ECDHE_PSK_CHACHA20_POLY1305_SHA256;
			// Если набор шифрования TLS 1.3 — ChaCha20-Poly1305 с SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::TLS_CHACHA20_POLY1305_SHA256):
				// Возвращаем код набора TLS_CHACHA20_POLY1305_SHA256
				return nts::protect::suite_t::CHACHA20_POLY1305_SHA256;
			// Если набор шифрования — ECDHE с ECDSA, AES-128-GCM и SHA-256
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256):
				// Возвращаем код набора TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
				return nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256;
			// Если набор шифрования — ECDHE с ECDSA, AES-256-GCM и SHA-384
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384):
				// Возвращаем код набора TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
				return nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384;
			// Если набор шифрования — ECDHE с ECDSA и ChaCha20-Poly1305
			case static_cast <uint8_t> (::tls::cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305):
				// Возвращаем код набора TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
				return nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256;
			// Если набор шифрования — «Кузнечик» в режиме MGM варианта L
			case static_cast <uint8_t> (::tls::cipher_t::GOSTR341112_256_KUZNYECHIK_MGM_L):
				// Возвращаем код набора TLS_GOSTR341112_256_WITH_KUZNYECHIK_MGM_L
				return nts::protect::suite_t::KUZNYECHIK_MGM_L;
			// Если набор шифрования — «Кузнечик» в режиме MGM варианта S
			case static_cast <uint8_t> (::tls::cipher_t::GOSTR341112_256_KUZNYECHIK_MGM_S):
				// Возвращаем код набора TLS_GOSTR341112_256_WITH_KUZNYECHIK_MGM_S
				return nts::protect::suite_t::KUZNYECHIK_MGM_S;
			// В остальных случаях возвращаем признак отсутствия набора
			default: return nts::protect::suite_t::NONE;
		}
	}
	/**
	 * @brief Функция получения названия набора шифрования
	 *
	 * @param suite набор шифрования A-NTS
	 * @return      название набора шифрования
	 *
	 */
	static constexpr const char * suiteName(const nts::protect::suite_t suite) noexcept {
		/**
		 * Определяем набор шифрования
		 */
		switch(suite){
			// Если набор шифрования TLS 1.3 — SM4-GCM с SM3
			case nts::protect::suite_t::SM4_GCM_SM3:
				// Возвращаем название набора TLS_SM4_GCM_SM3
				return "TLS_SM4_GCM_SM3";
			// Если набор шифрования TLS 1.3 — SM4-CCM с SM3
			case nts::protect::suite_t::SM4_CCM_SM3:
				// Возвращаем название набора TLS_SM4_CCM_SM3
				return "TLS_SM4_CCM_SM3";
			// Если набор шифрования — «Магма» в режиме MGM варианта L
			case nts::protect::suite_t::MAGMA_MGM_L:
				// Возвращаем название набора TLS_GOSTR341112_256_WITH_MAGMA_MGM_L
				return "TLS_GOSTR341112_256_WITH_MAGMA_MGM_L";
			// Если набор шифрования — «Магма» в режиме MGM варианта S
			case nts::protect::suite_t::MAGMA_MGM_S:
				// Возвращаем название набора TLS_GOSTR341112_256_WITH_MAGMA_MGM_S
				return "TLS_GOSTR341112_256_WITH_MAGMA_MGM_S";
			// Если набор шифрования — «Кузнечик» в режиме MGM варианта L
			case nts::protect::suite_t::KUZNYECHIK_MGM_L:
				// Возвращаем название набора TLS_GOSTR341112_256_WITH_KUZNYECHIK_MGM_L
				return "TLS_GOSTR341112_256_WITH_KUZNYECHIK_MGM_L";
			// Если набор шифрования — «Кузнечик» в режиме MGM варианта S
			case nts::protect::suite_t::KUZNYECHIK_MGM_S:
				// Возвращаем название набора TLS_GOSTR341112_256_WITH_KUZNYECHIK_MGM_S
				return "TLS_GOSTR341112_256_WITH_KUZNYECHIK_MGM_S";
			// Если набор шифрования TLS 1.3 — AES-128-GCM с SHA-256
			case nts::protect::suite_t::AES_128_GCM_SHA256:
				// Возвращаем название набора TLS_AES_128_GCM_SHA256
				return "TLS_AES_128_GCM_SHA256";
			// Если набор шифрования TLS 1.3 — AES-256-GCM с SHA-384
			case nts::protect::suite_t::AES_256_GCM_SHA384:
				// Возвращаем название набора TLS_AES_256_GCM_SHA384
				return "TLS_AES_256_GCM_SHA384";
			// Если набор шифрования — RSA с AES-128-CBC и SHA-1
			case nts::protect::suite_t::RSA_AES_128_CBC_SHA:
				// Возвращаем название набора TLS_RSA_WITH_AES_128_CBC_SHA
				return "TLS_RSA_WITH_AES_128_CBC_SHA";
			// Если набор шифрования — RSA с AES-256-CBC и SHA-1
			case nts::protect::suite_t::RSA_AES_256_CBC_SHA:
				// Возвращаем название набора TLS_RSA_WITH_AES_256_CBC_SHA
				return "TLS_RSA_WITH_AES_256_CBC_SHA";
			// Если набор шифрования — PSK с AES-128-CBC и SHA-1
			case nts::protect::suite_t::PSK_AES_128_CBC_SHA:
				// Возвращаем название набора TLS_PSK_WITH_AES_128_CBC_SHA
				return "TLS_PSK_WITH_AES_128_CBC_SHA";
			// Если набор шифрования — PSK с AES-256-CBC и SHA-1
			case nts::protect::suite_t::PSK_AES_256_CBC_SHA:
				// Возвращаем название набора TLS_PSK_WITH_AES_256_CBC_SHA
				return "TLS_PSK_WITH_AES_256_CBC_SHA";
			// Если набор шифрования — RSA с AES-128-CBC и SHA-256
			case nts::protect::suite_t::RSA_AES_128_CBC_SHA256:
				// Возвращаем название набора TLS_RSA_WITH_AES_128_CBC_SHA256
				return "TLS_RSA_WITH_AES_128_CBC_SHA256";
			// Если набор шифрования — RSA с AES-256-CBC и SHA-256
			case nts::protect::suite_t::RSA_AES_256_CBC_SHA256:
				// Возвращаем название набора TLS_RSA_WITH_AES_256_CBC_SHA256
				return "TLS_RSA_WITH_AES_256_CBC_SHA256";
			// Если набор шифрования — RSA с AES-128-GCM и SHA-256
			case nts::protect::suite_t::RSA_AES_128_GCM_SHA256:
				// Возвращаем название набора TLS_RSA_WITH_AES_128_GCM_SHA256
				return "TLS_RSA_WITH_AES_128_GCM_SHA256";
			// Если набор шифрования — RSA с AES-256-GCM и SHA-384
			case nts::protect::suite_t::RSA_AES_256_GCM_SHA384:
				// Возвращаем название набора TLS_RSA_WITH_AES_256_GCM_SHA384
				return "TLS_RSA_WITH_AES_256_GCM_SHA384";
			// Если набор шифрования — DHE с RSA, AES-128-CBC и SHA-1
			case nts::protect::suite_t::DHE_RSA_AES_128_CBC_SHA:
				// Возвращаем название набора TLS_DHE_RSA_WITH_AES_128_CBC_SHA
				return "TLS_DHE_RSA_WITH_AES_128_CBC_SHA";
			// Если набор шифрования — DHE с RSA, AES-256-CBC и SHA-1
			case nts::protect::suite_t::DHE_RSA_AES_256_CBC_SHA:
				// Возвращаем название набора TLS_DHE_RSA_WITH_AES_256_CBC_SHA
				return "TLS_DHE_RSA_WITH_AES_256_CBC_SHA";
			// Если набор шифрования TLS 1.3 — ChaCha20-Poly1305 с SHA-256
			case nts::protect::suite_t::CHACHA20_POLY1305_SHA256:
				// Возвращаем название набора TLS_CHACHA20_POLY1305_SHA256
				return "TLS_CHACHA20_POLY1305_SHA256";
			// Если набор шифрования — ECDHE с PSK, AES-128-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_PSK_AES_128_CBC_SHA:
				// Возвращаем название набора TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA
				return "TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA";
			// Если набор шифрования — ECDHE с PSK, AES-256-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_PSK_AES_256_CBC_SHA:
				// Возвращаем название набора TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA
				return "TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA";
			// Если набор шифрования — ECDHE с RSA, AES-128-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA:
				// Возвращаем название набора TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
				return "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA";
			// Если набор шифрования — ECDHE с RSA, AES-256-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA:
				// Возвращаем название набора TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
				return "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA";
			// Если набор шифрования — DHE с RSA, AES-128-GCM и SHA-256
			case nts::protect::suite_t::DHE_RSA_AES_128_GCM_SHA256:
				// Возвращаем название набора TLS_DHE_RSA_WITH_AES_128_GCM_SHA256
				return "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256";
			// Если набор шифрования — DHE с RSA, AES-256-GCM и SHA-384
			case nts::protect::suite_t::DHE_RSA_AES_256_GCM_SHA384:
				// Возвращаем название набора TLS_DHE_RSA_WITH_AES_256_GCM_SHA384
				return "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384";
			// Если набор шифрования — ECDHE с ECDSA, AES-256-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA:
				// Возвращаем название набора TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA
				return "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA";
			// Если набор шифрования — ECDHE с ECDSA, AES-128-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA:
				// Возвращаем название набора TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
				return "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA";
			// Если набор шифрования — ECDHE с RSA, AES-128-CBC и SHA-256
			case nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256:
				// Возвращаем название набора TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
				return "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256";
			// Если набор шифрования — ECDHE с RSA, AES-128-GCM и SHA-256
			case nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256:
				// Возвращаем название набора TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
				return "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256";
			// Если набор шифрования — ECDHE с RSA, AES-256-GCM и SHA-384
			case nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384:
				// Возвращаем название набора TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
				return "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384";
			// Если набор шифрования — ECDHE с ECDSA, AES-128-GCM и SHA-256
			case nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256:
				// Возвращаем название набора TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
				return "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256";
			// Если набор шифрования — ECDHE с ECDSA, AES-256-GCM и SHA-384
			case nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384:
				// Возвращаем название набора TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
				return "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384";
			// Если набор шифрования — ECDHE с ECDSA, AES-128-CBC и SHA-256
			case nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256:
				// Возвращаем название набора TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
				return "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256";
			// Если набор шифрования — DHE с RSA и ChaCha20-Poly1305
			case nts::protect::suite_t::DHE_RSA_CHACHA20_POLY1305_SHA256:
				// Возвращаем название набора TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256
				return "TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256";
			// Если набор шифрования — ECDHE с RSA и ChaCha20-Poly1305
			case nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256:
				// Возвращаем название набора TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256
				return "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256";
			// Если набор шифрования — ECDHE с PSK и ChaCha20-Poly1305
			case nts::protect::suite_t::ECDHE_PSK_CHACHA20_POLY1305_SHA256:
				// Возвращаем название набора TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256
				return "TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256";
			// Если набор шифрования — ECDHE с ECDSA и ChaCha20-Poly1305
			case nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256:
				// Возвращаем название набора TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
				return "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256";
			// В остальных случаях возвращаем пустую строку
			default: return "";
		}
	}
	/**
	 * @brief Функция получения кода набора шифрования AWH
	 *
	 * @param suite набор шифрования A-NTS
	 * @return      код набора шифрования AWH
	 *
	 */
	static constexpr ::tls::cipher_t cipherCode(const nts::protect::suite_t suite) noexcept {
		/**
		 * Определяем набор шифрования
		 */
		switch(suite){
			// Если набор шифрования — «Магма» в режиме MGM варианта S
			case nts::protect::suite_t::MAGMA_MGM_S:
				// Возвращаем код AWH GOSTR341112_256_MAGMA_MGM_S
				return ::tls::cipher_t::GOSTR341112_256_MAGMA_MGM_S;
			// Если набор шифрования — «Магма» в режиме MGM варианта L
			case nts::protect::suite_t::MAGMA_MGM_L:
				// Возвращаем код AWH GOSTR341112_256_MAGMA_MGM_L
				return ::tls::cipher_t::GOSTR341112_256_MAGMA_MGM_L;
			// Если набор шифрования — «Кузнечик» в режиме MGM варианта L
			case nts::protect::suite_t::KUZNYECHIK_MGM_L:
				// Возвращаем код AWH GOSTR341112_256_KUZNYECHIK_MGM_L
				return ::tls::cipher_t::GOSTR341112_256_KUZNYECHIK_MGM_L;
			// Если набор шифрования — «Кузнечик» в режиме MGM варианта S
			case nts::protect::suite_t::KUZNYECHIK_MGM_S:
				// Возвращаем код AWH GOSTR341112_256_KUZNYECHIK_MGM_S
				return ::tls::cipher_t::GOSTR341112_256_KUZNYECHIK_MGM_S;
			// Если набор шифрования TLS 1.3 — AES-256-GCM с SHA-384
			case nts::protect::suite_t::AES_256_GCM_SHA384:
				// Возвращаем код AWH TLS_AES_256_GCM_SHA384
				return ::tls::cipher_t::TLS_AES_256_GCM_SHA384;
			// Если набор шифрования TLS 1.3 — AES-128-GCM с SHA-256
			case nts::protect::suite_t::AES_128_GCM_SHA256:
				// Возвращаем код AWH TLS_AES_128_GCM_SHA256
				return ::tls::cipher_t::TLS_AES_128_GCM_SHA256;
			// Если набор шифрования — PSK с AES-128-CBC и SHA-1
			case nts::protect::suite_t::PSK_AES_128_CBC_SHA:
				// Возвращаем код AWH PSK_AES128_CBC_SHA
				return ::tls::cipher_t::PSK_AES128_CBC_SHA;
			// Если набор шифрования — PSK с AES-256-CBC и SHA-1
			case nts::protect::suite_t::PSK_AES_256_CBC_SHA:
				// Возвращаем код AWH PSK_AES256_CBC_SHA
				return ::tls::cipher_t::PSK_AES256_CBC_SHA;
			// Если набор шифрования — RSA с AES-128-CBC и SHA-1
			case nts::protect::suite_t::RSA_AES_128_CBC_SHA:
				// Возвращаем код AWH AES128_SHA
				return ::tls::cipher_t::AES128_SHA;
			// Если набор шифрования — RSA с AES-256-CBC и SHA-1
			case nts::protect::suite_t::RSA_AES_256_CBC_SHA:
				// Возвращаем код AWH AES256_SHA
				return ::tls::cipher_t::AES256_SHA;
			// Если набор шифрования — RSA с AES-128-GCM и SHA-256
			case nts::protect::suite_t::RSA_AES_128_GCM_SHA256:
				// Возвращаем код AWH AES128_GCM_SHA256
				return ::tls::cipher_t::AES128_GCM_SHA256;
			// Если набор шифрования — RSA с AES-256-GCM и SHA-384
			case nts::protect::suite_t::RSA_AES_256_GCM_SHA384:
				// Возвращаем код AWH AES256_GCM_SHA384
				return ::tls::cipher_t::AES256_GCM_SHA384;
			// Если набор шифрования TLS 1.3 — ChaCha20-Poly1305 с SHA-256
			case nts::protect::suite_t::CHACHA20_POLY1305_SHA256:
				// Возвращаем код AWH TLS_CHACHA20_POLY1305_SHA256
				return ::tls::cipher_t::TLS_CHACHA20_POLY1305_SHA256;
			// Если набор шифрования — ECDHE с PSK, AES-128-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_PSK_AES_128_CBC_SHA:
				// Возвращаем код AWH ECDHE_PSK_AES128_CBC_SHA
				return ::tls::cipher_t::ECDHE_PSK_AES128_CBC_SHA;
			// Если набор шифрования — ECDHE с PSK, AES-256-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_PSK_AES_256_CBC_SHA:
				// Возвращаем код AWH ECDHE_PSK_AES256_CBC_SHA
				return ::tls::cipher_t::ECDHE_PSK_AES256_CBC_SHA;
			// Если набор шифрования — ECDHE с RSA, AES-128-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA:
				// Возвращаем код AWH ECDHE_RSA_AES128_SHA
				return ::tls::cipher_t::ECDHE_RSA_AES128_SHA;
			// Если набор шифрования — ECDHE с RSA, AES-256-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA:
				// Возвращаем код AWH ECDHE_RSA_AES256_SHA
				return ::tls::cipher_t::ECDHE_RSA_AES256_SHA;
			// Если набор шифрования — ECDHE с ECDSA, AES-128-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA:
				// Возвращаем код AWH ECDHE_ECDSA_AES128_SHA
				return ::tls::cipher_t::ECDHE_ECDSA_AES128_SHA;
			// Если набор шифрования — ECDHE с ECDSA, AES-256-CBC и SHA-1
			case nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA:
				// Возвращаем код AWH ECDHE_ECDSA_AES256_SHA
				return ::tls::cipher_t::ECDHE_ECDSA_AES256_SHA;
			// Если набор шифрования — ECDHE с RSA, AES-128-GCM и SHA-256
			case nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256:
				// Возвращаем код AWH ECDHE_RSA_AES128_GCM_SHA256
				return ::tls::cipher_t::ECDHE_RSA_AES128_GCM_SHA256;
			// Если набор шифрования — ECDHE с RSA, AES-256-GCM и SHA-384
			case nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384:
				// Возвращаем код AWH ECDHE_RSA_AES256_GCM_SHA384
				return ::tls::cipher_t::ECDHE_RSA_AES256_GCM_SHA384;
			// Если набор шифрования — ECDHE с RSA, AES-128-CBC и SHA-256
			case nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256:
				// Возвращаем код AWH ECDHE_RSA_AES128_SHA256
				return ::tls::cipher_t::ECDHE_RSA_AES128_SHA256;
			// Если набор шифрования — ECDHE с ECDSA, AES-128-GCM и SHA-256
			case nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256:
				// Возвращаем код AWH ECDHE_ECDSA_AES128_GCM_SHA256
				return ::tls::cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256;
			// Если набор шифрования — ECDHE с ECDSA, AES-256-GCM и SHA-384
			case nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384:
				// Возвращаем код AWH ECDHE_ECDSA_AES256_GCM_SHA384
				return ::tls::cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384;
			// Если набор шифрования — ECDHE с ECDSA, AES-128-CBC и SHA-256
			case nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256:
				// Возвращаем код AWH ECDHE_ECDSA_AES128_SHA256
				return ::tls::cipher_t::ECDHE_ECDSA_AES128_SHA256;
			// Если набор шифрования — ECDHE с RSA и ChaCha20-Poly1305
			case nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256:
				// Возвращаем код AWH ECDHE_RSA_CHACHA20_POLY1305
				return ::tls::cipher_t::ECDHE_RSA_CHACHA20_POLY1305;
			// Если набор шифрования — ECDHE с PSK и ChaCha20-Poly1305
			case nts::protect::suite_t::ECDHE_PSK_CHACHA20_POLY1305_SHA256:
				// Возвращаем код AWH ECDHE_PSK_CHACHA20_POLY1305
				return ::tls::cipher_t::ECDHE_PSK_CHACHA20_POLY1305;
			// Если набор шифрования — ECDHE с ECDSA и ChaCha20-Poly1305
			case nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256:
				// Возвращаем код AWH ECDHE_ECDSA_CHACHA20_POLY1305
				return ::tls::cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305;
			// В остальных случаях возвращаем признак неизвестного набора
			default: return ::tls::cipher_t::UNKNOWN;
		}
	}
	/**
	 * @brief Функция получения кода схемы подписи
	 *
	 * @param signature схема подписи AWH
	 * @return         код схемы подписи A-NTS
	 *
	 */
	static constexpr nts::extension::scheme_t schemeCode(const ::tls::signature_t signature) noexcept {
		/**
		 * Определяем схему подписи
		 */
		switch(static_cast <uint8_t> (signature)){
			// Если схема подписи — ГОСТ на 256 бит, вариант A
			case static_cast <uint8_t> (::tls::signature_t::S256A):
				// Возвращаем код схемы ГОСТ S256A
				return nts::extension::scheme_t::S256A;
			// Если схема подписи — ГОСТ на 256 бит, вариант B
			case static_cast <uint8_t> (::tls::signature_t::S256B):
				// Возвращаем код схемы ГОСТ S256B
				return nts::extension::scheme_t::S256B;
			// Если схема подписи — ГОСТ на 256 бит, вариант C
			case static_cast <uint8_t> (::tls::signature_t::S256C):
				// Возвращаем код схемы ГОСТ S256C
				return nts::extension::scheme_t::S256C;
			// Если схема подписи — ГОСТ на 256 бит, вариант D
			case static_cast <uint8_t> (::tls::signature_t::S256D):
				// Возвращаем код схемы ГОСТ S256D
				return nts::extension::scheme_t::S256D;
			// Если схема подписи — ГОСТ на 512 бит, вариант A
			case static_cast <uint8_t> (::tls::signature_t::S512A):
				// Возвращаем код схемы ГОСТ S512A
				return nts::extension::scheme_t::S512A;
			// Если схема подписи — ГОСТ на 512 бит, вариант B
			case static_cast <uint8_t> (::tls::signature_t::S512B):
				// Возвращаем код схемы ГОСТ S512B
				return nts::extension::scheme_t::S512B;
			// Если схема подписи — ГОСТ на 512 бит, вариант C
			case static_cast <uint8_t> (::tls::signature_t::S512C):
				// Возвращаем код схемы ГОСТ S512C
				return nts::extension::scheme_t::S512C;
			// Если схема подписи — EdDSA на кривой Ed448
			case static_cast <uint8_t> (::tls::signature_t::ED448):
				// Возвращаем код схемы ED448
				return nts::extension::scheme_t::ED448;
			// Если схема подписи — EdDSA на кривой Ed25519
			case static_cast <uint8_t> (::tls::signature_t::ED25519):
				// Возвращаем код схемы ED25519
				return nts::extension::scheme_t::ED25519;
			// Если схема подписи — RSA PKCS#1 v1.5 с SHA-256
			case static_cast <uint8_t> (::tls::signature_t::RSA_PKCS1_SHA256):
				// Возвращаем код схемы RSA_PKCS1_SHA256
				return nts::extension::scheme_t::RSA_PKCS1_SHA256;
			// Если схема подписи — RSA PKCS#1 v1.5 с SHA-384
			case static_cast <uint8_t> (::tls::signature_t::RSA_PKCS1_SHA384):
				// Возвращаем код схемы RSA_PKCS1_SHA384
				return nts::extension::scheme_t::RSA_PKCS1_SHA384;
			// Если схема подписи — RSA PKCS#1 v1.5 с SHA-512
			case static_cast <uint8_t> (::tls::signature_t::RSA_PKCS1_SHA512):
				// Возвращаем код схемы RSA_PKCS1_SHA512
				return nts::extension::scheme_t::RSA_PKCS1_SHA512;
			// Если схема подписи — RSA-PSS с SHA-256 и ключом RSA-PSS
			case static_cast <uint8_t> (::tls::signature_t::RSA_PSS_PSS_SHA256):
				// Возвращаем код схемы RSA_PSS_PSS_SHA256
				return nts::extension::scheme_t::RSA_PSS_PSS_SHA256;
			// Если схема подписи — RSA-PSS с SHA-384 и ключом RSA-PSS
			case static_cast <uint8_t> (::tls::signature_t::RSA_PSS_PSS_SHA384):
				// Возвращаем код схемы RSA_PSS_PSS_SHA384
				return nts::extension::scheme_t::RSA_PSS_PSS_SHA384;
			// Если схема подписи — RSA-PSS с SHA-512 и ключом RSA-PSS
			case static_cast <uint8_t> (::tls::signature_t::RSA_PSS_PSS_SHA512):
				// Возвращаем код схемы RSA_PSS_PSS_SHA512
				return nts::extension::scheme_t::RSA_PSS_PSS_SHA512;
			// Если схема подписи — RSA-PSS с SHA-256 и ключом rsaEncryption
			case static_cast <uint8_t> (::tls::signature_t::RSA_PSS_RSAE_SHA256):
				// Возвращаем код схемы RSA_PSS_RSAE_SHA256
				return nts::extension::scheme_t::RSA_PSS_RSAE_SHA256;
			// Если схема подписи — RSA-PSS с SHA-384 и ключом rsaEncryption
			case static_cast <uint8_t> (::tls::signature_t::RSA_PSS_RSAE_SHA384):
				// Возвращаем код схемы RSA_PSS_RSAE_SHA384
				return nts::extension::scheme_t::RSA_PSS_RSAE_SHA384;
			// Если схема подписи — RSA-PSS с SHA-512 и ключом rsaEncryption
			case static_cast <uint8_t> (::tls::signature_t::RSA_PSS_RSAE_SHA512):
				// Возвращаем код схемы RSA_PSS_RSAE_SHA512
				return nts::extension::scheme_t::RSA_PSS_RSAE_SHA512;
			// Если схема подписи — ECDSA на кривой P-256 с SHA-256
			case static_cast <uint8_t> (::tls::signature_t::ECDSA_SECP256R1_SHA256):
				// Возвращаем код схемы ECDSA_SECP256R1_SHA256
				return nts::extension::scheme_t::ECDSA_SECP256R1_SHA256;
			// Если схема подписи — ECDSA на кривой P-384 с SHA-384
			case static_cast <uint8_t> (::tls::signature_t::ECDSA_SECP384R1_SHA384):
				// Возвращаем код схемы ECDSA_SECP384R1_SHA384
				return nts::extension::scheme_t::ECDSA_SECP384R1_SHA384;
			// Если схема подписи — ECDSA на кривой P-521 с SHA-512
			case static_cast <uint8_t> (::tls::signature_t::ECDSA_SECP521R1_SHA512):
				// Возвращаем код схемы ECDSA_SECP521R1_SHA512
				return nts::extension::scheme_t::ECDSA_SECP521R1_SHA512;
			// В остальных случаях возвращаем признак отсутствия схемы
			default: return nts::extension::scheme_t::NONE;
		}
	}
	/**
	 * @brief Функция получения шаблона контекста безопасности участника
	 *
	 * @param member объект участника обмена защищёнными данными
	 * @return       объект шаблона контекста безопасности либо nullptr
	 *
	 */
	static __AWH_INLINE__ ::cts_t * templateOf(const ::member_t * member) noexcept {
		// Если уровень является шаблонным контекстом безопасности
		if(member->layer == layer_t::CTS)
			// Возвращаем объект шаблона контекста безопасности
			return reinterpret_cast <::cts_t *> (const_cast <::member_t *> (member));
		// Если уровень является транспортной передачей данных
		if(member->layer == layer_t::CTL)
			// Возвращаем шаблон транспортного уровня
			return reinterpret_cast <::ctl_t *> (const_cast <::member_t *> (member))->cts;
		// Возвращаем пустой результат
		return nullptr;
	}
	/**
	 * @brief Функция чтения содержимого файла
	 *
	 * @param path   путь к файлу для чтения
	 * @param result накопитель, в который помещается содержимое файла
	 * @return       результат чтения файла
	 *
	 */
	static __AWH_INLINE__ bool readFile(string_view path, vector <uint8_t> & result) noexcept {
		// Очищаем накопитель содержимого
		result.clear();
		// Файл для чтения
		FILE * file = nullptr;
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Открываем файл
			file = ::_wfopen(awh::fmk::convert(string{path}).c_str(), L"rb");
		/**
		 * Для других операционных систем
		 */
		#else
			// Открываем файл
			file = ::fopen(string{path}.c_str(), "rb");
		#endif
		// Если файл открыт удачно
		if(file != nullptr){
			// Устанавливаем указатель чтения в конец файла
			::fseek(file, 0, SEEK_END);
			// Получаем размер файла
			const size_t size = static_cast <size_t> (::ftell(file));
			// Возвращаем указатель чтения в начало файла
			::fseek(file, 0, SEEK_SET);
			// Отводим место под содержимое файла
			result.resize(size);
			// Читаем файл целиком
			const size_t length = ::fread(result.data(), 1, size, file);
			// Закрываем файл
			::fclose(file);
			// Если файл прочитан целиком
			if(length == size)
				// Выводим признак успешности чтения
				return true;
			// Снимаем недописанное содержимое
			result.clear();
		}
		// Выводим признак отказа чтения
		return false;
	}
};

/**
 * @brief Инкапсулируем функции компрессии/декомпрессии в пространство имён compressor
 *
 */
namespace compressor {
	/**
	 * @brief Функция обратного вызова компрессии цепочки сертификатов методом Zlib
	 *
	 * @param source  сжимаемое тело сообщения цепочки сертификатов
	 * @param result  накопитель, в который складывается сжатое тело
	 * @param context указатель контекста потребителя
	 * @return        признак успешности компрессии
	 *
	 */
	static __AWH_INLINE__ bool compressZlib(const nts::view_t & source, vector <uint8_t> & result, [[maybe_unused]] void * context) noexcept {
		// Сжимаем данные
		::__awh_compressor__.compress(source.data, source.size, awh::compressor::method_t::ZLIB, result);
		// Возвращаем признак успешности компрессии
		return !result.empty();
	}
	/**
	 * @brief Функция обратного вызова компрессии цепочки сертификатов методом Brotli
	 *
	 * @param source  сжимаемое тело сообщения цепочки сертификатов
	 * @param result  накопитель, в который складывается сжатое тело
	 * @param context указатель контекста потребителя
	 * @return        признак успешности компрессии
	 *
	 */
	static __AWH_INLINE__ bool compressBrotli(const nts::view_t & source, vector <uint8_t> & result, [[maybe_unused]] void * context) noexcept {
		// Сжимаем данные
		::__awh_compressor__.compress(source.data, source.size, awh::compressor::method_t::BROTLI, result);
		// Возвращаем признак успешности компрессии
		return !result.empty();
	}
	/**
	 * @brief Функция обратного вызова компрессии цепочки сертификатов методом Zstandard
	 *
	 * @param source  сжимаемое тело сообщения цепочки сертификатов
	 * @param result  накопитель, в который складывается сжатое тело
	 * @param context указатель контекста потребителя
	 * @return        признак успешности компрессии
	 *
	 */
	static __AWH_INLINE__ bool compressZstandard(const nts::view_t & source, vector <uint8_t> & result, [[maybe_unused]] void * context) noexcept {
		// Сжимаем данные
		::__awh_compressor__.compress(source.data, source.size, awh::compressor::method_t::ZSTD, result);
		// Возвращаем признак успешности компрессии
		return !result.empty();
	}
	/**
	 * @brief Функция обратного вызова распаковки цепочки сертификатов методом Zlib
	 *
	 * @param source   распаковываемое сжатое тело
	 * @param expected ожидаемая длина тела после распаковки
	 * @param result   накопитель, в который складывается распакованное тело
	 * @param context  указатель контекста потребителя
	 * @return         признак успешности распаковки
	 *
	 */
	static __AWH_INLINE__ bool decompressZlib(const nts::view_t & source, const size_t expected, vector <uint8_t> & result, [[maybe_unused]] void * context) noexcept {
		// Распаковываем данные
		::__awh_compressor__.decompress(source.data, source.size, awh::compressor::method_t::ZLIB, result, expected);
		// Возвращаем результат сверки длины распакованного тела
		return (result.size() == expected);
	}
	/**
	 * @brief Функция обратного вызова распаковки цепочки сертификатов методом Brotli
	 *
	 * @param source   распаковываемое сжатое тело
	 * @param expected ожидаемая длина тела после распаковки
	 * @param result   накопитель, в который складывается распакованное тело
	 * @param context  указатель контекста потребителя
	 * @return         признак успешности распаковки
	 *
	 */
	static __AWH_INLINE__ bool decompressBrotli(const nts::view_t & source, const size_t expected, vector <uint8_t> & result, [[maybe_unused]] void * context) noexcept {
		// Распаковываем данные
		::__awh_compressor__.decompress(source.data, source.size, awh::compressor::method_t::BROTLI, result, expected);
		// Возвращаем результат сверки длины распакованного тела
		return (result.size() == expected);
	}
	/**
	 * @brief Функция обратного вызова распаковки цепочки сертификатов методом Zstandard
	 *
	 * @param source   распаковываемое сжатое тело
	 * @param expected ожидаемая длина тела после распаковки
	 * @param result   накопитель, в который складывается распакованное тело
	 * @param context  указатель контекста потребителя
	 * @return         признак успешности распаковки
	 *
	 */
	static __AWH_INLINE__ bool decompressZstandard(const nts::view_t & source, const size_t expected, vector <uint8_t> & result, [[maybe_unused]] void * context) noexcept {
		// Распаковываем данные
		::__awh_compressor__.decompress(source.data, source.size, awh::compressor::method_t::ZSTD, result, expected);
		// Возвращаем результат сверки длины распакованного тела
		return (result.size() == expected);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::tls::coder::CipherInfo::CipherInfo() noexcept :
 tls13(false),
 name{""}, origin{""},
 cipher(cipher_t::UNKNOWN) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::coder::ALPN::ALPN() noexcept : id(0), protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param id       идентификатор ALPN-протокола
 * @param protocol название ALPN-протокола
 *
 */
awh::tls::coder::ALPN::ALPN(const uint8_t id, const string & protocol) noexcept :
 id(id), protocol(protocol) {}

/**
 * @brief Функция получения версии A-NTS
 *
 * @return версия A-NTS
 *
 */
string awh::tls::coder::version() noexcept {
	// Возвращаем версию A-NTS
	return NTS_VERSION;
}
/**
 * @brief Функция установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 * @details Защищает только глобальный реестр TLS (ids, members, splice_map,
 *          init OpenSSL). Функции модуля после закрепления id выполняются без
 *          глобального lock; синхронизацию вызовов из разных потоков должен
 *          обеспечивать вызывающий код.
 *
 */
void awh::tls::coder::threadSafety(const bool mode) noexcept {
	// Активируем работу мьютекса блокировки глобального состояния TLS
	::__awh_ssl_mutex__.enabled = mode;
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
}
/**
 * @brief Функция подключения объекта для работы с отпечатками TLS
 *
 * @param fgp объект для работы с отпечатками TLS
 *
 */
void awh::tls::coder::fingerprint(const nts::fingerprint::Store * fgp) noexcept {
	// Сохраняем объект для работы с отпечатками TLS
	::__awh_fgp__ = fgp;
}
/**
 * @brief Функция получения общей информации о TLS соединении
 *
 * @param id идентификатор события
 * @return   общая информация о TLS соединении
 *
 */
string awh::tls::coder::info(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL){
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				// Переменная результата
				string result = "";
				// Если узел является клиентом и связь заведена
				if((member->node == event::node_t::CLIENT) && (member->client != nullptr)){
					// Издание протокола
					const uint16_t version = member->client->version();
					// Добавляем издание протокола в результат
					result.append(awh::fmk::format("Protocol: %s\n", (version == 0x0304) ? "TLSv1.3" : "TLSv1.2"));
					// Добавляем набор шифрования в результат
					result.append(awh::fmk::format("Cipher: %s\n", ::ssl::suiteName(member->client->suite())));
					// Добавляем прикладной протокол в результат
					result.append(awh::fmk::format("Protocol ALPN: %s\n", member->client->protocol().c_str()));
				// Если узел является сервером и связь заведена
				} else if((member->node == event::node_t::SERVER) && (member->server != nullptr)) {
					// Издание протокола
					const uint16_t version = member->server->version();
					// Добавляем издание протокола в результат
					result.append(awh::fmk::format("Protocol: %s\n", (version == 0x0304) ? "TLSv1.3" : "TLSv1.2"));
					// Добавляем набор шифрования в результат
					result.append(awh::fmk::format("Cipher: %s\n", ::ssl::suiteName(member->server->suite())));
					// Добавляем прикладной протокол в результат
					result.append(awh::fmk::format("Protocol ALPN: %s\n", member->server->protocol().c_str()));
				}
				// Возвращаем результат
				return result;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция получения информации о одноразовом узле TLS
 *
 * @param id идентификатор события
 * @return   информация о одноразовом узле TLS
 *
 */
string awh::tls::coder::peerInfo(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL){
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Цепочка сертификатов собеседника
				const vector <vector <uint8_t>> & chain = (member->node == event::node_t::CLIENT) ? member->client->peer() : member->server->peer();
				// Если цепочка сертификатов не пустая
				if(!chain.empty()){
					// Объект сертификата
					nts::x509::certificate_t cert;
					// Если сертификат разобран
					if(cert.parse(nts::view_t(chain.front().data(), chain.front().size())) == nts::x509::error_t::NONE){
						// Имя владельца
						string subject = "";
						// Преобразуем имя в текст
						nts::x509::text(cert.subjects(), subject);
						// Возвращаем результат
						return subject;
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция получения информации о шифре
 *
 * @param id идентификатор события
 * @return   информация о шифре
 *
 */
string awh::tls::coder::cipherInfo(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
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
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если узел является клиентом и связь заведена
					if((member->node == event::node_t::CLIENT) && (member->client != nullptr))
						// Извлекаем информацию о шифре
						return ::ssl::suiteName(member->client->suite());
					// Если узел является сервером и связь заведена
					if((member->node == event::node_t::SERVER) && (member->server != nullptr))
						// Извлекаем информацию о шифре
						return ::ssl::suiteName(member->server->suite());
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция получения информации о сертификате
 *
 * @param id идентификатор события
 * @return   информация о сертификате
 *
 */
string awh::tls::coder::certificateInfo(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL){
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Цепочка сертификатов собеседника
				const vector <vector <uint8_t>> & chain = (member->node == event::node_t::CLIENT) ? member->client->peer() : member->server->peer();
				// Если цепочка сертификатов не пустая
				if(!chain.empty()){
					// Объект сертификата
					nts::x509::certificate_t cert;
					// Если сертификат разобран
					if(cert.parse(nts::view_t(chain.front().data(), chain.front().size())) == nts::x509::error_t::NONE){
						// Имя владельца
						string subject = "";
						// Имя издателя
						string issuer = "";
						// Преобразуем имена в текст
						nts::x509::text(cert.subjects(), subject);
						nts::x509::text(cert.issuers(), issuer);
						// Возвращаем результат
						return awh::fmk::format("Subject: %s\nIssuer: %s", subject.c_str(), issuer.c_str());
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция получения информации о списке отзыва сертификатов
 *
 * @param id идентификатор события
 * @return   информация о списке отзыва сертификатов
 *
 */
string awh::tls::coder::certificateRevocationListInfo(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен и запас несёт списки отзыва
			if((cts != nullptr) && (cts->store.lists() > 0)){
				// Переменная результата
				string result = "";
				/**
				 * Перебираем списки отзыва запаса
				 */
				for(size_t i = 0; i < cts->store.lists(); i++){
					// Получаем список отзыва
					const nts::view_t list = cts->store.list(i);
					// Если результат уже сформирован
					if(!result.empty())
						// Добавляем разделитель
						result.append("\n");
					// Добавляем размер списка отзыва в результат
					result.append(awh::fmk::format("Certificate Revocation List: %zu bytes", list.size));
				}
				// Возвращаем результат
				return result;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция получения списка доступных шифров
 *
 * @param id идентификатор события
 * @return   список доступных шифров
 *
 */
vector <awh::tls::coder::cipher_info_t> awh::tls::coder::availableCiphers(const id_t id) noexcept {
	// Переменная результата
	vector <cipher_info_t> result;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Объект для хранения информации о шифре
			cipher_info_t info;
			// Стандартные шифры TLSv1.3
			static const struct { uint16_t code; const char * name; const char * origin; } ciphers[] = {
				{ 0x1301, "AES_128_GCM_SHA256", "TLS_AES_128_GCM_SHA256" },
				{ 0x1302, "AES_256_GCM_SHA384", "TLS_AES_256_GCM_SHA384" },
				{ 0x1303, "CHACHA20_POLY1305_SHA256", "TLS_CHACHA20_POLY1305_SHA256" }
			};
			/**
			 * Добавляем стандартные шифры TLSv1.3 в результат
			 */
			for(const auto & cipher : ciphers){
				// Определяем поддерживает ли шифр TLSv1.3
				info.tls13 = true;
				// Получаем код шифра
				info.cipher = ::ssl::cipherCode(static_cast <nts::protect::suite_t> (cipher.code));
				// Получаем название шифра
				info.name = cipher.name;
				// Получаем стандартное название шифра
				info.origin = cipher.origin;
				// Добавляем информацию о шифре в результат
				result.push_back(::move(info));
			}
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr){
				/**
				 * Добавляем настроенные наборы шифрования издания TLS 1.2 в результат
				 */
				for(const auto & suite : cts->suites){
					// Получаем код шифра
					info.cipher = ::ssl::cipherCode(suite);
					// Если код шифра известен и не относится к TLS 1.3
					if((info.cipher != cipher_t::UNKNOWN) && (static_cast <uint16_t> (suite) < 0x1301)){
						// Определяем что шифр не относится к TLSv1.3
						info.tls13 = false;
						// Получаем название шифра
						info.name = ::ssl::suiteName(suite);
						// Получаем стандартное название шифра
						info.origin = info.name;
						// Добавляем информацию о шифре в результат
						result.push_back(::move(info));
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция извлечения сертификата TLS
 *
 * @param id идентификатор события
 * @return   активный протокол
 *
 */
string awh::tls::coder::certificateExtract(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL){
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Цепочка сертификатов собеседника
				const vector <vector <uint8_t>> & chain = (member->node == event::node_t::CLIENT) ? member->client->peer() : member->server->peer();
				// Если цепочка сертификатов не пустая
				if(!chain.empty()){
					// Переменная результата
					string result = "";
					// Кодируем сертификат в PEM
					nts::pem::encode(nts::pem::block_t{"CERTIFICATE", chain.front()}, result);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция проверки валидности сертификата
 *
 * @note Только для CTL. На CLIENT проверяет сертификат пира (сервера)
 *       по member->host.name (ожидаемое имя/SNI). На SERVER peer-сертификат —
 *       сертификат клиента, host.name — SNI клиента; вызывающий код должен
 *       понимать эту семантику (mTLS и т.п.).
 *
 * @param id идентификатор события
 * @return   результат проверки валидности сертификата
 *
 */
bool awh::tls::coder::validateCertificate(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL){
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				// Цепочка сертификатов собеседника
				const vector <vector <uint8_t>> & chain = (member->node == event::node_t::CLIENT) ? member->client->peer() : member->server->peer();
				// Если цепочка сертификатов не пустая и шаблон контекста безопасности получен
				if(!chain.empty() && (member->cts != nullptr))
					// Проверяем цепочку сертификатов против запаса доверия
					result = (nts::trust::verify(chain, member->cts->store, 0) == nts::trust::error_t::NONE);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки проверки доменного имени сервера
 *
 * @param id   идентификатор события
 * @param mode режим проверки доменного имени сервера
 *
 */
void awh::tls::coder::validateServerNameIndication(const id_t id, const bool mode) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Извлекаем участника обмена защищёнными данными
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Если режим проверки имени хоста сервера установлен
			if(mode)
				// Устанавливаем режим проверки сертификата
				member->state |= state::CERTIFICATE_VERIFY;
			// Снимаем режим проверки сертификата
			else member->state &= ~state::CERTIFICATE_VERIFY;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, mode}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция получения режима работы TLS
 *
 * @param id идентификатор события
 * @return   режим работы TLS
 *
 */
awh::tls::coder::mode_t awh::tls::coder::mode(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
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
					// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем режим работы TLS по умолчанию
	return mode_t::NONE;
}
/**
 * @brief Функция установки режима работы TLS
 *
 * @param id   идентификатор события
 * @param mode режим работы TLS
 *
 */
void awh::tls::coder::mode(const id_t id, const mode_t mode) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
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
									// Ищем запись в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
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
					// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, static_cast <uint16_t> (mode)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция получения доменного имени сервера
 *
 * @param id идентификатор события
 * @return   доменное имя сервера
 *
 */
string awh::tls::coder::serverNameIndication(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS):
					// Извлекаем имя хоста сервера
					return reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id))->host;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL):
					// Извлекаем имя хоста сервера
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Функция установки доменного имени сервера
 *
 * @param id  идентификатор события
 * @param sni доменное имя сервера
 *
 */
void awh::tls::coder::serverNameIndication(const id_t id, string_view sni) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если имя хоста сервера не пустое
		if(!sni.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Если узел является сервером и установлен режим работы с несколькими сертификатами TLS
					if((cts->node == event::node_t::SERVER) && (cts->state & state::MULTICERT_MODE)){
						// Если название хоста уже установлено
						if(!cts->host.empty())
							// Ищем запись в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
							::ssl::registry::spliceErase(cts->proto, cts->host);
						// Добавляем новую запись в глобальную карту сопоставления имён хостов и идентификаторов узлов TLS
						::ssl::registry::spliceEmplace(cts->proto, string{sni}, id);
					}
					// Устанавливаем хост для уровня защищённых сокетов
					cts->host = sni;
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, sni}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция извлечения кэшированного билета возобновления сессии (RFC 9001 §4.6)
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
bool awh::tls::coder::session(const id_t id, string_view key, string & session) noexcept {
	// Сбрасываем результат извлечения билета возобновления
	session.clear();
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден и является шаблонным контекстом безопасности
		if((pin != nullptr) && (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTS)){
			// Извлекаем объект шаблона контекста безопасности
			auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
			// Ищем билет возобновления по ключу сервера
			auto iter = member->sessions.find(string{key});
			// Если билет возобновления найден
			if(iter != member->sessions.end())
				// Возвращаем сохранённый билет возобновления сессии
				session = iter->second;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, key, session}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат извлечения билета возобновления сессии
	return !session.empty();
}
/**
 * @brief Функция сохранения билета возобновления сессии в кэш (RFC 9001 §4.6)
 *
 * @param id      идентификатор шаблонного контекста безопасности
 * @param key     ключ сервера (SNI либо адрес эндпоинта)
 * @param session сериализованный билет возобновления для сохранения
 *
 */
void awh::tls::coder::session(const id_t id, string_view key, string_view session) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден и является шаблонным контекстом безопасности
		if((pin != nullptr) && (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTS)){
			// Извлекаем объект шаблона контекста безопасности
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, key, session}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция сохранения билета возобновления, выданного сервером (RFC 9001 §4.6)
 *
 * @param id     идентификатор шаблонного контекста безопасности
 * @param ticket сериализованный билет возобновления для сохранения
 * @param early  контекст ранних данных связи, выдавшей билет
 *
 */
void awh::tls::coder::ticket(const id_t id, string_view ticket, string_view early) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден и является шаблонным контекстом безопасности
		if((pin != nullptr) && (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTS)){
			// Извлекаем объект шаблона контекста безопасности
			auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
			/**
			 * Запас выданных билетов ограничен: билет пригоден одному возобновлению,
			 * а запас пополняется всякой выдачей, и его неограниченный рост дал бы
			 * утечку памяти серверу под долгой нагрузкой
			 */
			while(member->issued.size() >= 8)
				// Снимаем самый старый билет из запаса
				member->issued.pop_front();
			// Помещаем выданный билет в запас с контекстом ранних данных его связи
			member->issued.emplace_back(string{ticket}, string{early});
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, ticket, early}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция извлечения билетов возобновления, выданных сервером (RFC 9001 §4.6)
 *
 * @param id      идентификатор шаблонного контекста безопасности
 * @param early   контекст ранних данных связи, признающей билеты
 * @param tickets объект для извлечения сериализованных билетов возобновления
 * @return        результат извлечения (найден хотя бы один билет)
 *
 */
bool awh::tls::coder::tickets(const id_t id, string_view early, vector <string> & tickets) noexcept {
	// Сбрасываем результат извлечения билетов возобновления
	tickets.clear();
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден и является шаблонным контекстом безопасности
		if((pin != nullptr) && (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTS)){
			// Извлекаем объект шаблона контекста безопасности
			auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
			/**
			 * Перебираем запас выданных билетов
			 */
			for(const auto & item : member->issued){
				// Если контекст ранних данных разошёлся либо билет уже взят результатом
				if((item.second != early) || (std::find(tickets.begin(), tickets.end(), item.first) != tickets.end()))
					// Переходим к следующему билету
					continue;
				// Добавляем запись билета к результату
				tickets.push_back(item.first);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, early}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат извлечения билетов возобновления
	return !tickets.empty();
}
/**
 * @brief Функция установки адреса и порта отдалённого узла
 *
 * @param id   идентификатор события
 * @param ip   IP-адрес отдалённого узла
 * @param port порт отдалённого узла
 * @return     результат выполнения установки
 *
 */
bool awh::tls::coder::peer(const id_t id, string_view ip, const uint16_t port) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если IP-адрес сервера не пустой и порт сервера задан верно
		if((!ip.empty()) && (port > 0)){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				/**
				 * Определяем уровень транспортной безопасности
				 */
				switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
					// Если уровень является шаблонным контекстом безопасности
					case static_cast <uint8_t> (layer_t::CTS): {
						// Извлекаем объект шаблона контекста безопасности
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
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, ip, port}, log::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, error.c_str());
							#endif
						}
					} break;
					// Если уровень является транспортной передачей данных
					case static_cast <uint8_t> (layer_t::CTL): {
						// Извлекаем объект транспортного уровня передачи
						auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
						// Разбираем IP-адрес
						if(::__awh_addr__.parse(ip)){
							// Инициализируем объект хоста IPv4-адреса
							member->host.peer = make_unique <net::attr_net_t> ();
							// Получаем объект хоста IPv4-адреса
							net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
							/**
							 * Определяем тип IP-адреса
							 */
							switch(static_cast <uint8_t> (::__awh_addr__.type())){
								// Для IPv4-адреса
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Устанавливаем тип адреса однорангового узла
									address->type = net::type_t::IPV4;
									// Устанавливаем порт
									address->port = port;
									// Инициализируем объект IP-адреса
									address->ip = ::__awh_addr__.source(net_addr_t::endian_t::LITTLE);
								} break;
								// Для IPv6-адреса
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Устанавливаем тип адреса однорангового узла
									address->type = net::type_t::IPV6;
									// Устанавливаем порт
									address->port = port;
									// Инициализируем объект IP-адреса
									address->ip = ::__awh_addr__.source(net_addr_t::endian_t::LITTLE);
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
										#if defined(DEBUG_MODE)
											// Записываем ошибку в лог
											log::debug("%s", __PRETTY_FUNCTION__, {id, ip, port}, log::flag_t::CRITICAL, error.c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											log::print("%s", log::flag_t::CRITICAL, error.c_str());
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
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {id, ip, port}, log::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем отрицательный результат
							return false;
						}
						/**
						 * Отложенное заведение связи сервера выполняется здесь: адрес
						 * собеседника получен, и связь дейтаграммного транспорта заводится
						 * привязкой метки возврата состояния к адресу (раздел 5.1 RFC 9147).
						 * Запись адреса собирается из копии: буфер вызова живёт лишь до
						 * возврата, а связь хранит адрес до конца
						 */
						if((member->state & state::START_PENDING) != 0){
							// Снимаем признак отложенного заведения связи сервера
							member->state &= ~state::START_PENDING;
							// Адрес собеседника из записи транспортного уровня
							const net::attr_net_t * stored = awh_cast <const net::attr_net_t *> (member->host.peer.get());
							// Октеты адреса собеседника: адрес и порт собеседника подряд
							uint8_t wire[18];
							// Размер записи адреса собеседника
							const size_t size = ((stored->type == net::type_t::IPV6) ? sizeof(wire) : 6);
							// Копируем октеты IP-адреса собеседника
							if(stored->type == net::type_t::IPV6)
								// Копируем октеты IPv6-адреса собеседника
								::memcpy(wire, awh_cast <const net::addr_net_ipv6_t *> (stored->ip.get())->address.data(), size - 2);
							// Иначе адрес собеседника является адресом IPv4
							else
								// Копируем октеты IPv4-адреса собеседника
								::memcpy(wire, &awh_cast <const net::addr_net_ipv4_t *> (stored->ip.get())->address, size - 2);
							// Дописываем порт собеседника старшим октетом вперёд
							wire[size - 2] = static_cast <uint8_t> ((port >> 8) & 0xFF);
							// Дописываем младший октет порта собеседника
							wire[size - 1] = static_cast <uint8_t> (port & 0xFF);
							// Если адрес в связь сервера установить не удалось либо связь не заведена
							if(!member->server->address(nts::view_t(wire, size)) || (member->server->start() != nts::context::error_t::NONE)){
								// Помечаем участника мусорным: связь негодна
								member->state |= state::GARBAGE_MODE;
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Could not establish DTLS session");
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, error_t::TLS_SESSION_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {id, ip, port}, log::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Возвращаем отрицательный результат
								return false;
							}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, ip, port}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Функция удаления контекста TLS
 *
 * @note После destroy() id помечается GARBAGE_MODE; дальнейшие вызовы функций
 *       с этим id недопустимы. Физическое удаление из реестра — при refs==0.
 *
 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
 * @return   результат выполнения удаления
 *
 */
bool awh::tls::coder::destroy(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		/**
		 * @brief Закрепляем участника в глобальном реестре TLS
		 *
		 * @details Закрепление идёт с допуском помеченных мусорным: участника мог пометить
		 *          путь ошибки внутри самих работ обмена, и отказ в снятии оставил бы его
		 *          память удержанной до конца работы приложения.
		 *
		 */
		const auto pin = ::ssl::registry::pin(id, true);
		// Если идентификатор контекста TLS найден
		if((result = (pin != nullptr))){
			// Извлекаем участника обмена защищёнными данными
			::member_t * member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Признак участника, уже помеченного мусорным путём ошибки
			const bool garbage = ((member->state & ::state::GARBAGE_MODE) > 0);
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (member->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Если узел является сервером
					if(member->node == event::node_t::SERVER){
						// Если название хоста уже установлено
						if(!member->host.empty())
							// Ищем запись в глобальной карте сопоставления имён хостов и идентификаторов узлов TLS
							::ssl::registry::spliceErase(member->proto, member->host);
					}
					/**
					 * @brief Если функция обратного вызова состояния установлена, а участник ещё не помечен
					 *
					 * @details Извещение уходит один раз: путь ошибки внутри работ обмена уже
					 *          известил вызывающую сторону, пометив участника мусорным, и второе
					 *          извещение о том же событии выдало бы одно снятие за два.
					 *
					 */
					if(!garbage && (member->callback.state != nullptr))
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::DESTROYED);
					/**
					 * @brief Устанавливаем режим удаления участника обмена защищёнными данными
					 *
					 * @details Признак снятия ставится вместе с признаком мусорного: память
					 *          возвращается охранником лишь по нему, и ставит его одно только
					 *          снятие по требованию вызывающей стороны.
					 *
					 */
					member->state |= (::state::GARBAGE_MODE | ::state::RELEASED_MODE);
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					/**
					 * @brief Если функция обратного вызова состояния установлена, а участник ещё не помечен
					 *
					 * @details Извещение уходит один раз: путь ошибки внутри работ обмена уже
					 *          известил вызывающую сторону, пометив участника мусорным, и второе
					 *          извещение о том же событии выдало бы одно снятие за два.
					 *
					 */
					if(!garbage && (member->callback.state != nullptr))
						// Вызываем функцию обратного вызова состояния
						member->callback.state(id, state_t::DESTROYED);
					/**
					 * @brief Устанавливаем режим удаления участника обмена защищёнными данными
					 *
					 * @details Признак снятия ставится вместе с признаком мусорного: память
					 *          возвращается охранником лишь по нему, и ставит его одно только
					 *          снятие по требованию вызывающей стороны.
					 *
					 */
					member->state |= (::state::GARBAGE_MODE | ::state::RELEASED_MODE);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция завершения TLS соединения
 *
 * @param id идентификатор события
 * @return   результат выполнения завершения
 *
 */
bool awh::tls::coder::shutdown(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
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
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Если узел является клиентом и связь заведена
					if((member->node == event::node_t::CLIENT) && (member->client != nullptr))
						// Завершаем связь клиента
						result = (member->client->close() == nts::context::error_t::NONE);
					// Если узел является сервером и связь заведена
					else if((member->node == event::node_t::SERVER) && (member->server != nullptr))
						// Завершаем связь сервера
						result = (member->server->close() == nts::context::error_t::NONE);
					/**
					 * Выдаём накопленные байты после завершения связи.
					 * close_notify попадает в накопитель и должен быть отправлен пиру немедленно.
					 */
					if(result && !::ssl::emitOutput(member, id))
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция выполнения TLS рукопожатия
 *
 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
 *
 * @param id идентификатор события
 * @return   результат выполнения рукопожатия
 *
 */
bool awh::tls::coder::handshake(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		/**
		 * Определяем уровень транспортной безопасности
		 */
		switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
			// Если уровень является шаблонным контекстом безопасности
			case static_cast <uint8_t> (layer_t::CTS): {
				// Извлекаем объект шаблона контекста безопасности
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
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("%s", log::flag_t::CRITICAL, error.c_str());
					#endif
				}
			} break;
			// Если уровень является транспортной передачей данных
			case static_cast <uint8_t> (layer_t::CTL): {
				// Извлекаем объект транспортного уровня передачи
				auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
				// Создаём охранника участника обмена защищёнными данными
				::local::guard_t guard(member);
				// Если участник помечен мусорным
				if(member->state & ::state::GARBAGE_MODE)
					// Выходим из переключателя
					break;
				// Если рукопожатие ещё не выполнено
				if(!(result = (member->state & state::HANDSHAKE_MODE))){
					// Выдаём накопленные байты отправки
					result = ::ssl::emitOutput(member, id);
					// Если связь установлена
					if(::ssl::established(member)){
						// Если цепочка сертификатов собеседника не прошла проверку доверия
						if(!::ssl::verifyPeer(member)){
							// Вызываем отказ доверия сертификату собеседника
							::ssl::fail(member, id, error_t::CERT_FAILED, "Certificate is not trusted");
							// Выходим из переключателя
							break;
						}
						// Устанавливаем режим выполненного рукопожатия
						member->state |= state::HANDSHAKE_MODE;
						// Очищаем буфер сборки ClientHello
						member->hello.clear();
						// Индекс протокола
						size_t index = 0;
						// Название активного протокола связи
						const string & proto = (
							(member->node == event::node_t::CLIENT) ?
							member->client->protocol() :
							member->server->protocol()
						);
						/**
						 * Перебираем список поддерживаемых ALPN-протоколов
						 */
						for(size_t i = 0; i < member->alpn.buffer.size();){
							// Извлекаем размер протокола
							const size_t size = member->alpn.buffer[i];
							// Если размер протокола некорректен или выходит за пределы буфера
							if((size == 0) || ((i + 1 + size) > member->alpn.buffer.size()))
								// Выходим из цикла
								break;
							// Если протокол совпадает с активным протоколом
							if((size == proto.size()) && (::memcmp(&member->alpn.buffer[i + 1], proto.data(), size) == 0)){
								// Если индекс протокола не выходит за пределы списка
								if(index < member->alpn.ids.size())
									// Устанавливаем активный протокол
									member->alpn.id = member->alpn.ids[index];
								// Выходим из цикла
								break;
							}
							// Смещаем смещение на размер протокола
							i += 1 + size;
							// Увеличиваем индекс протокола
							index++;
						}
						// Если функция обратного вызова состояния установлена
						if(member->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							member->callback.state(id, state_t::HANDSHAKED);
						// Если узел является сервером и функция обратного вызова отпечатка установлена
						if((member->node == event::node_t::SERVER) && (member->callback.fingerprint != nullptr)){
							// Профиль отпечатка браузера
							nts::fingerprint::profile_t profile;
							// Если профиль отпечатка извлечён
							if(member->server->profile(profile) == nts::context::error_t::NONE)
								// Вызываем функцию обратного вызова отпечатка браузера
								member->callback.fingerprint(id, profile);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция повторной передачи данных
 *
 * @param id идентификатор события
 * @return   результат выполнения повторной передачи
 *
 */
bool awh::tls::coder::retransmit(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
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
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					/**
					 * Дослать данные, задержавшиеся в BIO буфере записи: доставка по UDP
					 * ненадёжна (DTLS/QUIC), и повторная передача снимает застрявшие записи.
					 * Пустой буфер - не ошибка, а успешный холостой вызов: слать нечего,
					 * всё уже отправлено. Ошибкой считается лишь сбой выдачи, который
					 * emitOutput() и отражает, возвращая false
					 */
					result = ::ssl::emitOutput(member, id);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция создания связи TLS с переносом рукопожатия по уровням (QUIC)
 *
 * @param id идентификатор шаблона контекста безопасности
 * @return   связь TLS во владение вызывающему коду либо nullptr
 *
 */
void * awh::tls::coder::quicContext(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Объект уровня транспортной безопасности
			::member_t * member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			/**
			 * Уровнем может быть и транспортная передача: соединение QUIC создают
			 * и по шаблону, и по передаче - передача несёт шаблон сама, и связь
			 * строится по этому шаблону
			 */
			if(member->layer == layer_t::CTL){
				// Извлекаем объект транспортного уровня передачи
				auto ctl = reinterpret_cast <::ctl_t *> (member);
				// Если шаблон контекста безопасности отсутствует - выходим из функции
				if(ctl->cts == nullptr)
					// Возвращаем пустой результат
					return nullptr;
				// Переходим к шаблону контекста безопасности передачи
				member = reinterpret_cast <::member_t *> (ctl->cts);
				// Закрепляем шаблон контекста безопасности в глобальном реестре TLS
				pin = ::ssl::registry::pin(static_cast <id_t> (reinterpret_cast <uintptr_t> (member)));
			}
			// Если уровень является шаблонным контекстом безопасности
			if((pin != nullptr) && (member->layer == layer_t::CTS)){
				// Извлекаем объект шаблона контекста безопасности
				auto cts = reinterpret_cast <::cts_t *> (member);
				// Если узел является клиентом
				if(cts->node == event::node_t::CLIENT){
					// Создаём связь клиента TLS
					auto ctx = ::make_unique <nts::client::context_t> ();
					// Применяем настройки шаблона к связи
					::ssl::applySettings<true>(* ctx, cts);
					// Возвращаем связь во владение вызывающему коду
					return reinterpret_cast <void *> (ctx.release());
				// Если узел является сервером
				} else if(cts->node == event::node_t::SERVER) {
					// Создаём связь сервера TLS
					auto ctx = ::make_unique <nts::server::context_t> ();
					// Применяем настройки шаблона к связи
					::ssl::applySettings<false>(* ctx, cts);
					// Возвращаем связь во владение вызывающему коду
					return reinterpret_cast <void *> (ctx.release());
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустой результат
	return nullptr;
}
/**
 * @brief Функция проверки цепочки сертификатов собеседника запасом доверенных (QUIC)
 *
 * @param id    идентификатор шаблона контекста безопасности
 * @param chain цепочка сертификатов собеседника от своего к издателю
 * @return      результат проверки (false - доверие цепочке не сложилось)
 *
 */
bool awh::tls::coder::verified(const id_t id, const vector <vector <uint8_t>> & chain) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Извлекаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr){
				// Если проверка сертификата собеседника не требуется
				if((cts->state & state::CERTIFICATE_VERIFY) == 0)
					// Выводим положительный результат
					return true;
				// Роль собеседника для суждения назначения сертификата
				const nts::trust::role_t role = ((cts->node == event::node_t::CLIENT) ? nts::trust::role_t::SERVER : nts::trust::role_t::CLIENT);
				// Если цепочка собеседника не прошла проверку доверия
				if(nts::trust::verify(chain, cts->store, ::time(nullptr), role) != nts::trust::error_t::NONE)
					// Выводим отрицательный результат
					return false;
				// Если имя узла не отвечает ни одному имени сертификата
				if(!cts->host.empty() && (chain.empty() || (nts::trust::belongs(nts::view_t(chain[0].data(), chain[0].size()), cts->host) != nts::trust::error_t::NONE)))
					// Выводим отрицательный результат
					return false;
				// Выводим положительный результат
				return true;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Функция создания идентификатора транспортного уровня
 *
 * @param id идентификатор шаблона контекста безопасности
 * @return   идентификатор транспортного уровня
 *
 */
awh::tls::coder::id_t awh::tls::coder::transport(const id_t id) noexcept {
	// Переменная результата
	id_t result = 0;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
					auto cts = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Создаём новый транспортный уровень передачи и добавляем его в контейнер
					auto ret = ::ssl::registry::emplace(::make_unique <::ctl_t> ());
					// Извлекаем объект транспортного уровня передачи
					::ctl_t * member = awh_cast <::ctl_t *> ((* ret).get());
					{
						// Устанавливаем шаблон контекста безопасности
						member->cts = cts;
						// Устанавливаем идентификатор цифрового отпечатка браузера
						member->fid = cts->fid;
						// Устанавливаем ALPN-протоколы
						member->alpn = cts->alpn;
						// Устанавливаем тип узла события
						member->node = cts->node;
						// Устанавливаем тип протокола события
						member->proto = cts->proto;
						// Устанавливаем объект состояния
						member->state = cts->state;
						// Сохраняем итератор уровня защищённых сокетов
						member->iterator = ret;
						// Получаем идентификатор контекста TLS
						result = static_cast <id_t> (reinterpret_cast <uintptr_t> ((* ret).get()));
						// Устанавливаем хост для уровня защищённых сокетов
						member->host.name = cts->host;
						// Копируем список ECHConfig из шаблона контекста безопасности
						member->ech = cts->ech;
						// Если связь завести не удалось
						if(!::ssl::establish(member, cts, id)){
							// Удаляем контекст TLS из контейнера уровней защищённых сокетов
							member->erase(::__awh_ssl_members__);
							// Выходим из функции
							return 0;
						}
						// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
						::ssl::registry::add(result);
						// Если идентификатор цифрового отпечатка браузера установлен — применяем CTL-настройки отпечатка
						if((member->node == event::node_t::CLIENT) && (member->fid != 0) && (::__awh_fgp__ != nullptr))
							// Активируем поддержку наложения цифрового отпечатка браузера на TLS-соединение
							awh::tls::coder::browser(result, member->fid);
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto ctl = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Если шаблон контекста безопасности отсутствует
					if(ctl->cts == nullptr)
						// Выходим из функции
						return 0;
					// Создаём новый транспортный уровень передачи и добавляем его в контейнер
					auto ret = ::ssl::registry::emplace(::make_unique <::ctl_t> ());
					// Извлекаем объект транспортного уровня передачи
					::ctl_t * member = awh_cast <::ctl_t *> ((* ret).get());
					{
						// Устанавливаем шаблон контекста безопасности
						member->cts = ctl->cts;
						// Устанавливаем идентификатор цифрового отпечатка браузера
						member->fid = ctl->fid;
						// Устанавливаем ALPN-протоколы
						member->alpn = ctl->alpn;
						// Устанавливаем тип узла события
						member->node = ctl->node;
						// Устанавливаем тип протокола события
						member->proto = ctl->proto;
						// Устанавливаем объект состояния
						member->state = ctl->state;
						// Сохраняем итератор уровня защищённых сокетов
						member->iterator = ret;
						// Получаем идентификатор контекста TLS
						result = static_cast <id_t> (reinterpret_cast <uintptr_t> ((* ret).get()));
						// Устанавливаем хост для уровня защищённых сокетов
						member->host.name = ctl->host.name;
						// Копируем список ECHConfig из транспортного уровня
						member->ech = ctl->ech;
						// Если связь завести не удалось
						if(!::ssl::establish(member, ctl->cts, id)){
							// Удаляем контекст TLS из контейнера уровней защищённых сокетов
							member->erase(::__awh_ssl_members__);
							// Выходим из функции
							return 0;
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция создания идентификатора шаблона контекста безопасности
 *
 * @param node  тип узла события
 * @param proto тип протокола события
 * @return      идентификатор шаблона контекста безопасности
 *
 */
awh::tls::coder::id_t awh::tls::coder::context(const event::node_t node, const event::protocol_t proto) noexcept {
	// Инициализируем TLS-модуль
	::ssl::init();
	// Переменная результата
	id_t result = 0;
	/**
	 * Перехватываем ошибки
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
		// Получаем идентификатор контекста TLS
		result = static_cast <id_t> (reinterpret_cast <uintptr_t> ((* ret).get()));
		/**
		 * Определяем тип протокола подключения
		 */
		switch(static_cast <uint8_t> (proto)){
			// Если протокол подключения QUIC
			case static_cast <uint8_t> (event::protocol_t::QUIC):
				// Транспорт QUIC допускает исключительно издание TLS 1.3 (RFC 9001 §4.2)
				member->lowest = member->highest = 0x0304;
			break;
			// Во всех остальных случаях
			default:
				// Устанавливаем наименьшее дозволенное издание TLS 1.2
				member->lowest = 0x0303;
				// Устанавливаем наибольшее дозволенное издание TLS 1.3
				member->highest = 0x0304;
			break;
		}
		// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
		::ssl::registry::add(result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug(
				"%s", __PRETTY_FUNCTION__,
				{
					static_cast <uint16_t> (node),
					static_cast <uint16_t> (proto)
				}, log::flag_t::CRITICAL, error.what()
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция получения сериализованного ECHConfigList для публикации в DNS
 *
 * @details Функция возвращает ECHConfigList, сохранённый в памяти после вызова
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
 * @param id идентификатор события
 * @return   байты ECHConfigList для DNS HTTPS-записи (для сервера)
 *           или байты ECHConfigList полученные из DNS (для клиента).
 *           Возвращает пустой вектор если ECH не был настроен.
 *
 */
vector <uint8_t> awh::tls::coder::getKeysECH(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустой вектор
	return {};
}
/**
 * @brief Функция установки ключей EncryptedClientHello (ECH)
 *
 * @details Поведение зависит от
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
 * @param id   идентификатор события
 * @param keys ключи EncryptedClientHello (ECH)
 * @return     результат выполнения установки
 *
 */
bool awh::tls::coder::setKeysECH(const id_t id, const vector <uint8_t> & keys) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr){
				// Сохраняем список ECHConfig в шаблоне контекста
				cts->ech = keys;
				// Устанавливаем положительный результат
				result = true;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, keys.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки ключей EncryptedClientHello (ECH)
 *
 * @details Перегрузка setKeysECH() для сырого указателя вместо vector.
 *          Поведение идентично первой перегрузке; см. ее документацию.
 *
 *          Для сервера если @p keys равен nullptr или @p size равен 0 —
 *          ключ генерируется автоматически.
 *
 * @param id   идентификатор события
 * @param keys ключи EncryptedClientHello (ECH)
 * @param size размер ключей EncryptedClientHello (ECH)
 * @return     результат выполнения установки
 *
 */
bool awh::tls::coder::setKeysECH(const id_t id, const uint8_t * keys, const size_t size) noexcept {
	// Список ключей ECH
	vector <uint8_t> list;
	// Если ключи переданы
	if((keys != nullptr) && (size > 0))
		// Копируем ключи в список
		list.assign(keys, keys + size);
	// Устанавливаем ключи ECH
	return awh::tls::coder::setKeysECH(id, list);
}
/**
 * @brief Функция шифрования данных
 *
 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для шифрования
 * @param size   размер буфера данных для шифрования
 * @return       результат выполнения шифрования
 *
 */
bool awh::tls::coder::encrypt(const id_t id, const void * buffer, const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
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
					// Извлекаем объект шаблона контекста безопасности
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
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id, buffer, size}, log::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Если рукопожатие выполнено успешно
					if(member->state & state::HANDSHAKE_MODE){
						// Если данные отправлены успешно
						if((result = (::ssl::writeData(member, buffer, size) == nts::context::error_t::NONE))){
							// Если функция обратного вызова записи данных установлена
							if(member->callback.write != nullptr)
								// Вызываем функцию обратного вызова записи данных
								member->callback.write(id, event_t::ENCRYPTION, size);
							// Выдаём накопленные байты отправки
							result = ::ssl::emitOutput(member, id);
						// Если данные не отправлены
						} else {
							// Выводим отказ записи данных
							::ssl::fail(member, id, error_t::WRITE_FAILED, "Write is failed");
							// Устанавливаем отрицательный результат
							result = false;
						}
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
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, buffer, size}, log::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::WARNING, error.c_str());
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, buffer, size}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция расшифровки данных
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для расшифровки
 * @param size   размер буфера данных для расшифровки
 * @return       результат выполнения расшифровки
 *
 */
bool awh::tls::coder::decrypt(const id_t id, const void * buffer, const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
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
					// Извлекаем объект шаблона контекста безопасности
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
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id, buffer, size}, log::flag_t::CRITICAL, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::CRITICAL, error.c_str());
						#endif
					}
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
					auto member = reinterpret_cast <::ctl_t *> (static_cast <uintptr_t> (id));
					// Создаём охранника участника обмена защищёнными данными
					::local::guard_t guard(member);
					// Если принятые байты поданы успешно
					if((result = (::ssl::pushInput(member, buffer, size) == nts::context::error_t::NONE))){
						// Если функция обратного вызова записи данных установлена
						if(member->callback.write != nullptr)
							// Вызываем функцию обратного вызова записи данных
							member->callback.write(id, event_t::DECRYPTION, size);
						// Выдаём накопленные байты отправки
						::ssl::emitOutput(member, id);
						// Если рукопожатие ещё не выполнено
						if(!(member->state & state::HANDSHAKE_MODE)){
							// Проводим рукопожатие TLS
							result = awh::tls::coder::handshake(id);
							// Если рукопожатие ещё не выполнено
							if(!result || !(member->state & state::HANDSHAKE_MODE))
								// Возвращаем результат
								return result;
						}
						// Буфер принятых прикладных данных
						vector <uint8_t> plain;
						/**
						 * Читаем все принятые прикладные данные из связи
						 */
						while((::ssl::readData(member, plain) == nts::context::error_t::NONE) && !plain.empty()){
							// Если функция обратного вызова чтения данных установлена
							if(member->callback.read != nullptr)
								// Вызываем функцию обратного вызова чтения данных
								member->callback.read(id, event_t::DECRYPTION, plain.data(), plain.size());
							// Очищаем буфер принятых данных
							plain.clear();
						}
					// Если принятые байты не поданы
					} else {
						// Выводим отказ чтения данных
						::ssl::fail(member, id, error_t::READ_FAILED, "Read is failed");
						// Устанавливаем отрицательный результат
						result = false;
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, buffer, size}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки поддерживаемых групп эллиптических кривых
 *
 * @param id     идентификатор события
 * @param groups список поддерживаемых групп эллиптических кривых
 *
 */
void awh::tls::coder::groups(const id_t id, const vector <group_t> & groups) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список групп эллиптических кривых не пустой
		if(!groups.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Очищаем список кривых обмена ключами
					cts->groups.clear();
					/**
					 * Перебираем все группы эллиптических кривых для установки
					 */
					for(auto & item : groups){
						// Получаем код группы эллиптической кривой
						const nts::extension::group_t code = ::ssl::groupCode(item);
						// Если код группы поддерживается
						if(code != nts::extension::group_t::NONE)
							// Добавляем код группы в список
							cts->groups.push_back(code);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, groups.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки алгоритмов шифрования
 *
 * @param id      идентификатор события
 * @param ciphers список алгоритмов шифрования для установки
 *
 */
void awh::tls::coder::ciphers(const id_t id, const vector <cipher_t> & ciphers) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список алгоритмов шифрования не пустой
		if(!ciphers.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Очищаем список наборов шифрования
					cts->suites.clear();
					/**
					 * Перебираем все алгоритмы шифрования для установки
					 */
					for(auto & item : ciphers){
						// Получаем код набора шифрования
						const nts::protect::suite_t code = ::ssl::suiteCode(item);
						// Если код набора шифрования поддерживается
						if(code != nts::protect::suite_t::NONE)
							// Добавляем код набора шифрования в список
							cts->suites.push_back(code);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, ciphers.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция активации/деактивации GREASE-значений (мусорных кодов)
 *
 * @param id   идентификатор события
 * @param mode режим активации/деактивации
 *
 */
void awh::tls::coder::grease(const id_t id, const event::mode_t mode) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr){
				// Если узел является клиентом
				if(cts->node == event::node_t::CLIENT)
					// Устанавливаем признак вставки значений GREASE
					cts->grease = (mode == event::mode_t::ENABLED);
				// Если узел является сервером
				else {
					// Если функция обратного вызова состояния установлена
					if(cts->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						cts->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "GREASE codes are only allowed to be added for the client");
					// Если функция обратного вызова ошибки установлена
					if(cts->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						cts->callback.error(id, error_t::CTS_FAILED, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id, static_cast <uint16_t> (mode)}, log::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::WARNING, error.c_str());
						#endif
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, static_cast <uint16_t> (mode)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция перемешивания поддерживаемых расширений TLS для имитации поведения различных браузеров
 *
 * @param id   идентификатор события
 * @param mode режим активации/деактивации перемешивания расширений
 *
 */
void awh::tls::coder::permuteExtensions(const id_t id, const event::mode_t mode) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr){
				// Если узел является клиентом
				if(cts->node == event::node_t::CLIENT)
					// Устанавливаем признак перемешивания расширений
					cts->permute = (mode == event::mode_t::ENABLED);
				// Если узел является сервером
				else {
					// Если функция обратного вызова состояния установлена
					if(cts->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						cts->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Permuting extensions is only allowed to be added for the client");
					// Если функция обратного вызова ошибки установлена
					if(cts->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						cts->callback.error(id, error_t::CTS_FAILED, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id, static_cast <uint16_t> (mode)}, log::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::WARNING, error.c_str());
						#endif
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, static_cast <uint16_t> (mode)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция активации поддержки SCT (Signed Certificate Timestamp)
 *
 * @param id идентификатор события
 *
 */
void awh::tls::coder::signedCertificateTimestamp(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr)
				// Включаем запрос меток времени сертификата
				cts->sct = true;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция активации поддержки Stapling (OCSP)
 *
 * @param id идентификатор события
 *
 */
void awh::tls::coder::onlineCertificateStatusProtocol(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr)
				// Включаем запрос состояния сертификата
				cts->ocsp = true;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция активации поддержки расширения Next Protocol Negotiation (NPN)
 *
 * @param id   идентификатор события
 * @param mode режим активации/деактивации поддержки расширения
 *
 */
void awh::tls::coder::nextProtocolNegotiation(const id_t id, const event::mode_t mode) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr)
				// Устанавливаем признак выработки следующего протокола
				cts->npn = (mode == event::mode_t::ENABLED);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, static_cast <uint16_t> (mode)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция активации поддержки наложения цифрового отпечатка браузера на TLS-соединение
 *
 * @param id  идентификатор события
 * @param fid идентификатор цифрового отпечатка браузера
 *
 */
void awh::tls::coder::browser(const id_t id, const uint8_t fid) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Получаем объект шаблона контекста безопасности
			::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
			// Если шаблон контекста безопасности получен
			if(cts != nullptr){
				// Если узел является клиентом и хранилище отпечатков установлено
				if((cts->node == event::node_t::CLIENT) && (::__awh_fgp__ != nullptr)){
					// Устанавливаем идентификатор цифрового отпечатка браузера
					cts->fid = fid;
					// Если шаблон отпечатка браузера пустой
					if((fid != 0) && (::__awh_fgp__->get(fid) == nullptr)){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем предупреждение в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id, fid}, log::flag_t::WARNING, "Browser fingerprint template is empty");
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем предупреждение в лог
							log::print("%s", log::flag_t::WARNING, "Browser fingerprint template is empty");
						#endif
					}
				// Если узел является сервером либо хранилище отпечатков не установлено
				} else {
					// Если функция обратного вызова состояния установлена
					if(cts->callback.state != nullptr)
						// Вызываем функцию обратного вызова состояния
						cts->callback.state(id, state_t::FAILED);
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Activation browser fingerprint for TLS connections, supported for the client only");
					// Если функция обратного вызова ошибки установлена
					if(cts->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						cts->callback.error(id, error_t::CTS_FAILED, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("%s", __PRETTY_FUNCTION__, {id, fid}, log::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("%s", log::flag_t::WARNING, error.c_str());
						#endif
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, fid}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция получения списка поддерживаемых ALPN-протоколов
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
vector <awh::tls::coder::alpn_t> awh::tls::coder::protocols(const id_t id) noexcept {
	// Результирующий список поддерживаемых ALPN-протоколов
	vector <alpn_t> result;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
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
				// Извлекаем объект шаблона контекста безопасности
				auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
				// Получаем список идентификаторов поддерживаемых протоколов
				ids = &member->alpn.ids;
				// Получаем буфер поддерживаемых протоколов
				buffer = &member->alpn.buffer;
			} break;
			// Если уровень является транспортным уровнем передачи
			case static_cast <uint8_t> (layer_t::CTL): {
				// Извлекаем объект транспортного уровня передачи
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
			alpn_t item{0, ""};
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
	 * Перехватываем ошибку
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Функция извлечения активного протокола
 *
 * @param id идентификатор события
 * @return   метод активного протокола
 *
 */
uint8_t awh::tls::coder::alpn(const id_t id) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS):
					// Извлекаем объект транспортного уровня передачи
					return reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id))->alpn.id;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL):
					// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return -1;
}
/**
 * @brief Функция установки поддерживаемых ALPN-протоколов
 *
 * @param id   идентификатор события
 * @param alpn список поддерживаемых ALPN-протоколов
 *
 */
void awh::tls::coder::alpn(const id_t id, const vector <alpn_t> & alpn) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список поддерживаемых ALPN-протоколов не пустой
		if(!alpn.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Сбрасываем список идентификаторов поддерживаемых ALPN-протоколов
					cts->alpn.ids.clear();
					// Сбрасываем буфер поддерживаемых ALPN-протоколов
					cts->alpn.buffer.clear();
					/**
					 * Перебираем весь список поддерживаемых ALPN-протоколов
					 */
					for(const auto & item : alpn){
						// Длина имени ALPN-протокола в wire-формате — один байт (1..255)
						if(item.protocol.empty() || (item.protocol.size() > 255)){
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "ALPN protocol name length must be 1..255 bytes");
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, item.protocol}, log::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::WARNING, error.c_str());
							#endif
							// Пропускаем некорректный протокол
							continue;
						}
						// Добавляем идентификатор протокола в список поддерживаемых протоколов
						cts->alpn.ids.push_back(item.id);
						// Добавляем в буфер длину названия протокола
						cts->alpn.buffer.push_back(static_cast <uint8_t> (item.protocol.size()));
						// Добавляем в буфер название протокола
						cts->alpn.buffer.insert(cts->alpn.buffer.end(), item.protocol.begin(), item.protocol.end());
					}
					// Если после фильтрации не осталось допустимых протоколов
					if(cts->alpn.ids.empty())
						// Завершаем выполнение функции
						return;
					// Устанавливаем идентификатор выбранного ALPN-протокола
					cts->alpn.id = cts->alpn.ids.front();
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, alpn.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки поддерживаемых ALPS-протоколов
 *
 * @param id   идентификатор события
 * @param alps список поддерживаемых ALPS-протоколов
 * @param std  флаг поддерживаемого стандарта
 *
 */
void awh::tls::coder::alps(const id_t id, const vector <alpn_t> & alps, const standard_t std) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список поддерживаемых ALPS-протоколов не пустой
		if(!alps.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Если узел является клиентом
					if(cts->node == event::node_t::CLIENT){
						// Очищаем список настроек ALPS
						cts->alps.clear();
						/**
						 * Перебираем весь список поддерживаемых ALPS-протоколов
						 */
						for(const auto & item : alps){
							// Длина имени ALPS-протокола — один байт (1..255)
							if(item.protocol.empty() || (item.protocol.size() > 255)){
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "ALPS protocol name length must be 1..255 bytes");
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {id, item.protocol}, log::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::WARNING, error.c_str());
								#endif
								// Пропускаем некорректный протокол
								continue;
							}
							// Объявляем настройки ALPS для протокола с пустыми настройками
							cts->alps.emplace_back(item.protocol, vector <uint8_t> {});
						}
						/**
						 * Определяем точку кода расширения ALPS
						 */
						switch(static_cast <uint8_t> (std)){
							// Если стандарт соответствует новому
							case static_cast <uint8_t> (standard_t::NEW):
								// Устанавливаем новую точку кода ALPS
								cts->alpsType = nts::extension::type_t::SETTINGS;
							break;
							// Если стандарт соответствует старому
							case static_cast <uint8_t> (standard_t::OLD):
								// Устанавливаем прежнюю точку кода ALPS
								cts->alpsType = nts::extension::type_t::SETTINGS_OLD;
							break;
						}
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(cts->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							cts->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "ALPS protocols are only allowed to be added for the client");
						// Если функция обратного вызова ошибки установлена
						if(cts->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							cts->callback.error(id, error_t::ALPS_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, alps.size()}, log::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::WARNING, error.c_str());
							#endif
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, alps.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки поддерживаемых алгоритмов подписи
 *
 * @param id         идентификатор события
 * @param signatures список поддерживаемых алгоритмов подписи
 *
 */
void awh::tls::coder::signature(const id_t id, const vector <signature_t> & signatures) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список алгоритмов подписи не пустой
		if(!signatures.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Очищаем список схем подписи
					cts->schemes.clear();
					/**
					 * Перебираем все алгоритмы подписи для установки
					 */
					for(auto & item : signatures){
						// Получаем код схемы подписи
						const nts::extension::scheme_t code = ::ssl::schemeCode(item);
						// Если код схемы подписи поддерживается
						if(code != nts::extension::scheme_t::NONE)
							// Добавляем код схемы подписи в список
							cts->schemes.push_back(code);
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, signatures.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки поддерживаемых алгоритмов компрессии сертификата
 *
 * @param id     идентификатор события
 * @param methods список поддерживаемых алгоритмов компрессии сертификата
 *
 */
void awh::tls::coder::compressors(const id_t id, const vector <compressor::method_t> & methods) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список поддерживаемых алгоритмов компрессии сертификата не пустой
		if(!methods.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Очищаем список алгоритмов сжатия цепочки сертификатов
					cts->codecs.clear();
					/**
					 * Перебираем все поддерживаемые алгоритмы компрессии сертификата
					 */
					for(auto & method : methods){
						/**
						 * Определяем поддерживаемый алгоритм компрессии сертификата
						 */
						switch(static_cast <uint8_t> (method)){
							// Если алгоритм компрессии сертификата соответствует Zlib
							case static_cast <uint8_t> (compressor::method_t::ZLIB):
								// Добавляем алгоритм компрессии сертификата Zlib в список
								cts->codecs.emplace_back(0x01, &::compressor::compressZlib, &::compressor::decompressZlib, nullptr);
							break;
							// Если алгоритм компрессии сертификата соответствует Brotli
							case static_cast <uint8_t> (compressor::method_t::BROTLI):
								// Добавляем алгоритм компрессии сертификата Brotli в список
								cts->codecs.emplace_back(0x02, &::compressor::compressBrotli, &::compressor::decompressBrotli, nullptr);
							break;
							// Если алгоритм компрессии сертификата соответствует Zstandard
							case static_cast <uint8_t> (compressor::method_t::ZSTD):
								// Добавляем алгоритм компрессии сертификата Zstandard в список
								cts->codecs.emplace_back(0x03, &::compressor::compressZstandard, &::compressor::decompressZstandard, nullptr);
							break;
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, methods.size()}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция генерации заранее клиентом эфемерного ключа и отправки серверу для поддерживаемых групп эллиптических кривых
 *
 * @param id     идентификатор события
 * @param groups список поддерживаемых групп эллиптических кривых для ключевого обмена
 * @param grease флаг активации/деактивации ложного ключа EncryptedClientHello (ECH)
 *
 */
void awh::tls::coder::keyShare(const id_t id, const vector <group_t> & groups, const event::mode_t grease) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если список поддерживаемых групп эллиптических кривых не пустой
		if(!groups.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Если узел является клиентом
					if(cts->node == event::node_t::CLIENT){
						// Очищаем список групп долей ключа
						cts->shares.clear();
						/**
						 * Перебираем все группы эллиптических кривых для установки
						 */
						for(auto & item : groups){
							// Получаем код группы эллиптической кривой
							const nts::extension::group_t code = ::ssl::groupCode(item);
							// Если код группы поддерживается
							if(code != nts::extension::group_t::NONE)
								// Добавляем код группы в список долей ключа
								cts->shares.push_back(code);
						}
						// Устанавливаем признак вставки значений GREASE
						cts->grease = (grease == event::mode_t::ENABLED);
					// Если узел является сервером
					} else {
						// Если функция обратного вызова состояния установлена
						if(cts->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							cts->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Groups elliptic curves are only allowed to be added for the client");
						// Если функция обратного вызова ошибки установлена
						if(cts->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							cts->callback.error(id, error_t::CURVE_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, groups.size(), static_cast <uint16_t> (grease)}, log::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::WARNING, error.c_str());
							#endif
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, groups.size(), static_cast <uint16_t> (grease)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки сертификатов доверенных центров сертификации
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу сертификата доверенных центров сертификации
 *
 */
void awh::tls::coder::ca(const id_t id, string_view filename) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если путь к файлу сертификата доверенного центра указан
		if(!filename.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Содержимое файла сертификатов доверенного центра
					vector <uint8_t> content;
					// Если файл сертификатов доверенного центра прочитан
					if(::ssl::readFile(filename, content)){
						// Цепочка сертификатов доверенного центра
						vector <vector <uint8_t>> certs;
						// Если цепочка сертификатов не декодирована
						if(nts::pem::certificates(string(content.begin(), content.end()), certs) != nts::pem::error_t::NONE){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Failed to load CA file");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::CA_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {id, filename}, log::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						/**
						 * Добавляем сертификаты доверенного центра в запас
						 */
						for(auto & cert : certs)
							// Добавляем сертификат в запас доверенных центров
							cts->store.add(nts::view_t(cert.data(), cert.size()));
					// Если файл сертификатов доверенного центра не прочитан
					} else {
						// Если функция обратного вызова состояния установлена
						if(cts->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							cts->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Failed to read CA file");
						// Если функция обратного вызова ошибки установлена
						if(cts->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							cts->callback.error(id, error_t::CA_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, filename}, log::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, error.c_str());
							#endif
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, filename}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки сертификатов доверенных центров сертификации
 *
 * @param id   идентификатор транспортного уровня или шаблона контекста безопасности
 * @param dir  адрес директории с сертификатами доверенных центров сертификации
 * @param file путь к файлу сертификата доверенного центра сертификации
 *
 */
void awh::tls::coder::ca(const id_t id, string_view dir, string_view file) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если путь к файлу сертификата доверенного центра указан
		if(!file.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Содержимое файла сертификатов доверенного центра
					vector <uint8_t> content;
					// Полный путь к файлу сертификатов доверенного центра
					string path{dir};
					// Если каталог указан и не оканчивается разделителем
					if(!path.empty() && (path.back() != AWH_FS_SEPARATOR[0]))
						// Добавляем разделитель каталога
						path.append(AWH_FS_SEPARATOR);
					// Добавляем имя файла к пути
					path.append(file);
					// Если файл сертификатов доверенного центра прочитан
					if(::ssl::readFile(path, content)){
						// Цепочка сертификатов доверенного центра
						vector <vector <uint8_t>> certs;
						// Если цепочка сертификатов не декодирована
						if(nts::pem::certificates(string(content.begin(), content.end()), certs) != nts::pem::error_t::NONE){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Failed to load CA file");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::CA_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {id, dir, file}, log::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Выходим из функции
							return;
						}
						/**
						 * Добавляем сертификаты доверенного центра в запас
						 */
						for(auto & cert : certs)
							// Добавляем сертификат в запас доверенных центров
							cts->store.add(nts::view_t(cert.data(), cert.size()));
					// Если файл сертификатов доверенного центра не прочитан
					} else {
						// Если функция обратного вызова состояния установлена
						if(cts->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							cts->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Failed to read CA file");
						// Если функция обратного вызова ошибки установлена
						if(cts->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							cts->callback.error(id, error_t::CA_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, dir, file}, log::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, error.c_str());
							#endif
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, dir, file}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки списка отзыва сертификатов
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу списка отзыва сертификатов
 *
 */
void awh::tls::coder::certificateRevocationList(const id_t id, string_view filename) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если путь к файлу списка отзыва сертификатов указан
		if(!filename.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Содержимое файла списка отзыва
					vector <uint8_t> content;
					// Если файл списка отзыва прочитан
					if(::ssl::readFile(filename, content)){
						// Если список отзыва не добавлен в запас
						if(!cts->store.addList(nts::view_t(content.data(), content.size()))){
							// Если функция обратного вызова состояния установлена
							if(cts->callback.state != nullptr)
								// Вызываем функцию обратного вызова состояния
								cts->callback.state(id, state_t::FAILED);
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Certificate revocation list cannot be set");
							// Если функция обратного вызова ошибки установлена
							if(cts->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								cts->callback.error(id, error_t::CRL_FAILED, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {id, filename}, log::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, error.c_str());
								#endif
							}
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, filename}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки приватного ключа клиента
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу приватного ключа клиента
 * @param type     тип файла приватного ключа клиента
 *
 */
void awh::tls::coder::privateKey(const id_t id, string_view filename, const type_t type) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если путь к файлу приватного ключа указан
		if(!filename.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Содержимое файла приватного ключа
					vector <uint8_t> content;
					// Если файл приватного ключа прочитан
					if(::ssl::readFile(filename, content)){
						// Если тип файла PEM
						if(type == type_t::PEM){
							// Набор записей PEM
							vector <nts::pem::block_t> blocks;
							// Если записи PEM не разобраны
							if((nts::pem::decode(string(content.begin(), content.end()), blocks) != nts::pem::error_t::NONE) || blocks.empty() || blocks[0].data.empty()){
								// Если функция обратного вызова состояния установлена
								if(cts->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									cts->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Private key cannot be set");
								// Если функция обратного вызова ошибки установлена
								if(cts->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									cts->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Записываем ошибку в лог
										log::debug("%s", __PRETTY_FUNCTION__, {id, filename, static_cast <uint16_t> (type)}, log::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log::print("%s", log::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
							// Кладём DER приватного ключа в подлинность
							cts->credential.secret = std::move(blocks[0].data);
						// Если тип файла ASN1
						} else
							// Кладём DER приватного ключа в подлинность
							cts->credential.secret = std::move(content);
					// Если файл приватного ключа не прочитан
					} else {
						// Если функция обратного вызова состояния установлена
						if(cts->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							cts->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Private key cannot be set");
						// Если функция обратного вызова ошибки установлена
						if(cts->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							cts->callback.error(id, error_t::PRIVATE_KEY_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, filename, static_cast <uint16_t> (type)}, log::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, error.c_str());
							#endif
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, filename, static_cast <uint16_t> (type)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки клиентского сертификата
 *
 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
 * @param filename путь к файлу клиентского сертификата
 * @param type     тип файла клиентского сертификата
 *
 */
void awh::tls::coder::certificate(const id_t id, string_view filename, const type_t type) noexcept {
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Если путь к файлу сертификата указан
		if(!filename.empty()){
			// Закрепляем участника в глобальном реестре TLS
			const auto pin = ::ssl::registry::pin(id);
			// Если идентификатор контекста TLS найден
			if(pin != nullptr){
				// Получаем объект шаблона контекста безопасности
				::cts_t * cts = ::ssl::templateOf(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id)));
				// Если шаблон контекста безопасности получен
				if(cts != nullptr){
					// Содержимое файла сертификата
					vector <uint8_t> content;
					// Если файл сертификата прочитан
					if(::ssl::readFile(filename, content)){
						// Очищаем цепочку сертификатов своей стороны
						cts->credential.chain.clear();
						// Если тип файла PEM
						if(type == type_t::PEM){
							// Если цепочка сертификатов не декодирована
							if(nts::pem::certificates(string(content.begin(), content.end()), cts->credential.chain) != nts::pem::error_t::NONE){
								// Если функция обратного вызова состояния установлена
								if(cts->callback.state != nullptr)
									// Вызываем функцию обратного вызова состояния
									cts->callback.state(id, state_t::FAILED);
								// Получаем текст ошибки
								const string error = ::ssl::error(id, "Certificate cannot be set");
								// Если функция обратного вызова ошибки установлена
								if(cts->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									cts->callback.error(id, error_t::CERT_FAILED, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Записываем ошибку в лог
										log::debug("%s", __PRETTY_FUNCTION__, {id, filename, static_cast <uint16_t> (type)}, log::flag_t::CRITICAL, error.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log::print("%s", log::flag_t::CRITICAL, error.c_str());
									#endif
								}
								// Выходим из функции
								return;
							}
						// Если тип файла ASN1
						} else
							// Кладём DER сертификата в цепочку
							cts->credential.chain.push_back(std::move(content));
					// Если файл сертификата не прочитан
					} else {
						// Если функция обратного вызова состояния установлена
						if(cts->callback.state != nullptr)
							// Вызываем функцию обратного вызова состояния
							cts->callback.state(id, state_t::FAILED);
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Certificate cannot be set");
						// Если функция обратного вызова ошибки установлена
						if(cts->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							cts->callback.error(id, error_t::CERT_FAILED, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {id, filename, static_cast <uint16_t> (type)}, log::flag_t::CRITICAL, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::CRITICAL, error.c_str());
							#endif
						}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id, filename, static_cast <uint16_t> (type)}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Функция установки функции обратного вызова получения данных
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::coder::on(const id_t id, read_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if((result = (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL))){
				// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки функции обратного вызова передачи данных
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::coder::on(const id_t id, write_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if((result = (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL))){
				// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки функции обратного вызова изменения состояния
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::coder::on(const id_t id, state_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if((result = (pin != nullptr))){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Устанавливаем функцию обратного вызова изменения состояния
					member->callback.state = ::move(callback);
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки функции обратного вызова получения ошибок
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::coder::on(const id_t id, error_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if((result = (pin != nullptr))){
			/**
			 * Определяем уровень транспортной безопасности
			 */
			switch(static_cast <uint8_t> (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer)){
				// Если уровень является шаблонным контекстом безопасности
				case static_cast <uint8_t> (layer_t::CTS): {
					// Извлекаем объект шаблона контекста безопасности
					auto member = reinterpret_cast <::cts_t *> (static_cast <uintptr_t> (id));
					// Устанавливаем функцию обратного вызова получения ошибок
					member->callback.error = ::move(callback);
				} break;
				// Если уровень является транспортной передачей данных
				case static_cast <uint8_t> (layer_t::CTL): {
					// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Функция установки функции обратного вызова получения снимка браузера приславшего ClientHello
 *
 * @param id       идентификатор транспортного уровня
 * @param callback функция обратного вызова для установки
 * @return         результат установки функции обратного вызова
 *
 */
bool awh::tls::coder::on(const id_t id, fingerprint_callback_t callback) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Перехватываем ошибки
	 */
	try {
		// Закрепляем участника в глобальном реестре TLS
		const auto pin = ::ssl::registry::pin(id);
		// Если идентификатор контекста TLS найден
		if(pin != nullptr){
			// Если уровень является транспортной передачей данных
			if((result = (reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->layer == layer_t::CTL))){
				// Извлекаем объект транспортного уровня передачи
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {id}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
