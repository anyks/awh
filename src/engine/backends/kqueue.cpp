/**
 * @file: kqueue.cpp
 * @date: 2025-10-27
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Если максимальное количество файловых дескрипторов не передано
 */
#ifndef AWH_MAX_COUNT_FDS
	/**
	 * Устанавливаем максимальное количество доступных файловых дескрипторов 131072
	 */
	#define AWH_MAX_COUNT_FDS 0x20000
#endif

/**
 * Если максимальное количество опрашиваемых событий за одну итерацию (64, 128, 256, 512, 1024)
 */
#ifndef AWH_MAX_POLL_EVENTS_COUNT
	/**
	 * Устанавливаем максимальное количество опрашиваемых событий за одну итерацию (64)
	 */
	#define AWH_MAX_POLL_EVENTS_COUNT 0x40
#endif

/**
 * Стандартные модули
 */
#include <cerrno>
#include <atomic>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

/**
 * Подключаем системные заголовки
 */
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>

/**
 * Подключаем наши заголовочные файлы
 */
#include <sys/os.hpp>

/**
 * Подключаем заголовочный файл асинхронного движка ввода-вывода
 */
#include <engine/io.hpp>
#include <engine/fds.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Структура адреса
 *
 */
typedef struct Address {
	// Размер адреса
	uint16_t size;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Address() noexcept : size(0) {}
} __attribute__((packed)) address_t;

/**
 * @brief Структура IPv4-адреса
 *
 */
typedef struct AddressIPv4 : public Address {
	// IP-адрес
	uint32_t address;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressIPv4() noexcept : address(0) {}
} __attribute__((packed)) address_ipv4_t;

/**
 * @brief Структура IPv6-адреса
 *
 */
typedef struct AddressIPv6 : public Address {
	// IP-адрес
	uint8_t address[16];
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressIPv6() noexcept : address{0} {}
} __attribute__((packed)) address_ipv6_t;

/**
 * @brief Структура MAC-адреса
 *
 */
typedef struct AddressMAC : public Address {
	// MAC-адрес
	uint8_t address[6];
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressMAC() noexcept : address{0} {}
} __attribute__((packed)) address_mac_t;

/**
 * @brief Структура сетевого адреса
 *
 */
typedef struct AddressNetwork : public Address {
	// Префикс сети
	uint8_t prefix;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressNetwork() noexcept : prefix(0) {}
} __attribute__((packed)) address_network_t;

/**
 * @brief Структура IPv4 сетевого адреса
 *
 */
typedef struct AddressNetworkIPv4 : public AddressNetwork {
	// IP-адрес сети
	uint32_t address;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressNetworkIPv4() noexcept : address(0) {}
} __attribute__((packed)) address_network_ipv4_t;

/**
 * @brief Структура IPv6 сетевого адреса
 *
 */
typedef struct AddressNetworkIPv6 : public AddressNetwork {
	// IP-адрес сети
	uint8_t address[16];
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressNetworkIPv6() noexcept : address{0} {}
} __attribute__((packed)) address_network_ipv6_t;

/**
 * @brief Структура адреса файловой системы
 *
 */
typedef struct AddressFilesystem : public Address {
	// Путь к файлу, каталогу или сокету
	string address;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit AddressFilesystem() noexcept : address{""} {}
} address_fs_t;

/**
 * @brief Структура хоста
 *
 */
typedef struct Host {
	// Сокет хоста
	int32_t fd;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Host() noexcept : fd(-1) {}
} __attribute__((packed)) host_t;

/**
 * @brief Структура IP-хоста
 *
 */
typedef struct HostIP : public host_t {
	// Порт хоста
	int32_t port;
	// IP-адрес хоста
	address_t address;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit HostIP() noexcept : port(0) {}
} __attribute__((packed)) host_ip_t;

/**
 * @brief Структура UNIX-хоста
 * 
 */
typedef struct HostUDC : public host_t {
	// Путь к сокету
	address_fs_t path;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit HostUDC() noexcept {}
} host_udc_t;

/**
 * @brief Структура состояния события
 *
 */
typedef struct State {
	bool onlyIPv6;                   // Флаг активации только IPv6
	awh::event::mode_t mode;         // Флаг режима события
	awh::event::node_t node;         // Флаг узла события
	awh::event::type_t type;         // Флаг типа события
	awh::event::family_t family;     // Флаг семейства события
	awh::event::status_t status;     // Флаг статуса события
	awh::event::address_t address;   // Флаг адреса события
	awh::event::protocol_t protocol; // Флаг протокола события
	/**
	 * @brief Конструктор
	 *
	 */
	explicit State() noexcept :
	 onlyIPv6(false),
	 mode(awh::event::mode_t::NONE),
	 node(awh::event::node_t::NONE),
	 type(awh::event::type_t::NONE),
	 family(awh::event::family_t::NONE),
	 status(awh::event::status_t::NONE),
	 address(awh::event::address_t::NONE),
	 protocol(awh::event::protocol_t::NONE) {}
} __attribute__((packed)) state_t;

/**
 * @brief Структура обратных вызовов события
 *
 */
typedef struct Callbacks {
	// Обратный вызов при ошибке события
	awh::engine_t::errorCallback error;
	// Обратный вызов при изменении статуса события
	awh::engine_t::statusCallback status;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Callbacks() noexcept : error(nullptr), status(nullptr) {}
} callbacks_t;

/**
 * @brief Структура обратных вызовов сервера
 *
 */
typedef struct CallbacksServer : public Callbacks {
	// Обратный вызов при принятии события
	awh::engine_t::acceptCallback accept;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit CallbacksServer() noexcept : accept(nullptr) {}
} callbacks_server_t;

/**
 * @brief Структура обратных вызовов клиента
 *
 */
typedef struct CallbacksClient : public Callbacks {
	// Обратный вызов при чтении события
	awh::engine_t::readCallback read;
	// Обратный вызов при записи события
	awh::engine_t::writeCallback write;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit CallbacksClient() noexcept : read(nullptr), write(nullptr) {}
} callbacks_client_t;

/**
 * @brief Структура узла события
 *
 */
typedef struct Node {
	// Состояние события
	state_t state;
} node_t;

/**
 * @brief Структура таймера
 *
 */
typedef struct Timer : public Node {
	// Задержка времени таймера в миллисекундах
	uint32_t delay;
	// Обратные вызовы события
	callbacks_t callbacks;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Timer() noexcept : delay(0) {}
} timer_t;

/**
 * @brief Структура файловой системы
 *
 */
typedef struct Filesystem : public Node {
	// Файловый дескриптор
	int32_t fd;
	// Путь к файлу, каталогу или сокету
	address_fs_t path;
	// Обратные вызовы события
	callbacks_client_t callbacks;
	// Чёрный список адресов которым запрещён доступ
	unordered_set <unique_ptr <address_t>> blacklist;
	// Белый список адресов которым разрешён доступ
	unordered_set <unique_ptr <address_t>> whitelist;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Filesystem() noexcept : fd(-1) {}
} fs_t;

/**
 * @brief Структура сервера
 *
 */
typedef struct Server : public Node {
	// Хост события
	host_t host;
	// Размер очереди ожидания подключения
	uint32_t backlog;
	// MAC-адрес сетевого интерфейса
	address_mac_t macAddress;
	// Обратные вызовы события
	callbacks_server_t callbacks;
	// Сетевые интерфейсы события
	unordered_set <string> interfaces;
	// Опции активных событий
	unordered_set <awh::event::option_t> options;
	// Чёрный список пиров которым запрещён доступ
	unordered_set <unique_ptr <address_t>> blacklist;
	// Белый список пиров которым разрешён доступ
	unordered_set <unique_ptr <address_t>> whitelist;
	// Сетевые адреса для выхода в интернет
	unordered_set <unique_ptr <address_network_t>> networks;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Server() noexcept : backlog{SOMAXCONN} {}
} server_t;

/**
 * @brief Структура клиента
 *
 */
typedef struct Client : public Node {
	// Хост события
	host_t host;
	// Обратные вызовы события
	callbacks_client_t callbacks;
	// Сетевые интерфейсы события
	unordered_set <string> interfaces;
	// Опции активных событий
	unordered_set <awh::event::option_t> options;
	// Чёрный список серверов к которым запрещёно подключение
	unordered_set <unique_ptr <address_t>> blacklist;
	// Белый список серверов к которым разрешено подключение
	unordered_set <unique_ptr <address_t>> whitelist;
	// Сетевые адреса для выхода в интернет
	unordered_set <unique_ptr <address_network_t>> networks;
	// Размеры активных буферов события
	unordered_map <awh::event::action_t, size_t> bufferSize;
	// Активные таймауты события
	unordered_map <awh::event::action_t, uint32_t> timeouts;
	// Активные действия события
	unordered_map <awh::event::action_t, awh::event::notify_t> actions;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Client() noexcept {}
} client_t;

/**
 * @brief Структура подключённого клиента
 * 
 */
typedef struct Peer : public Node {
	// Хост события
	host_t host;
	// MAC-адрес сетевого интерфейса
	address_mac_t macAddress;
	// Обратные вызовы события
	callbacks_client_t callbacks;
	// Опции активных событий
	unordered_set <awh::event::option_t> options;
	// Размеры активных буферов события
	unordered_map <awh::event::action_t, size_t> bufferSize;
	// Активные таймауты события
	unordered_map <awh::event::action_t, uint32_t> timeouts;
	// Активные действия события
	unordered_map <awh::event::action_t, awh::event::notify_t> actions;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Peer() noexcept {}
} peer_t;

/**
 * Глобальная переменная списка узлов событий
 */
static unordered_map <awh::event::id_t, unique_ptr <node_t>> __awh_nodes__;


/**
 * @brief Функция генерации уникального идентификатора
 * 
 * @return уникальный идентификатор
 */
static uint32_t identifier() noexcept {
	// Начинаем с 1 (0 можно оставить как "invalid")
	static atomic_uint32_t id{1};
	// Выводим новое значение идентификатора
	return id.fetch_add(1, memory_order_relaxed);
}
/**
 * @brief Функция применения сетевой оптимизации операционной системы
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void boostingNetwork([[maybe_unused]] const awh::fmk_t * fmk, const awh::log_t * log) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем инициализацию объекта работы с операционноы системы
		awh::os_t os(log);
		// Выполняем инициализацию объекта работы с файловыми дескрипторами
		awh::fds_t fds(log);
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Структура лимитов дампов
			struct rlimit limit;
			// Устанавливаем текущий лимит равный бесконечности
			limit.rlim_cur = RLIM_INFINITY;
			// Устанавливаем максимальный лимит равный бесконечности
			limit.rlim_max = RLIM_INFINITY;
			// Выводим результат установки лимита дампов ядра
			if(::setrlimit(RLIMIT_CORE, &limit) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		#endif
		/**
		 * Выполняем установку нужного нам количества файловых дескрипторов
		 */
		if(!fds.limit(AWH_MAX_COUNT_FDS)){
			// Получаем лимиты файловых дескрипторов
			const auto & limits = fds.limit();
			// Если текущий лимит меньше желаемого
			if(limits.first < AWH_MAX_COUNT_FDS)
				// Выводим сообщение подсказки
				fds.help(limits.first, AWH_MAX_COUNT_FDS);
		}
		/**
		 * Если необходимо выполнить тюннинг операционной системы
		 */
		#if AWH_BOOSTING_NET
			/**
			 * Для операционной системы MacOS X
			 */
			#if __APPLE__ || __MACH__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					// Устанавливаем максимальное количество подключений
					os.sysctl("kern.ipc.somaxconn", 49152);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок MacOS X
					 */
					os.sysctl("kern.ipc.maxsockbuf", 6291456);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvspace", 1042560);
					// В MacOS X значение по умолчанию 3, что очень мало
					os.sysctl("net.inet.tcp.r", 8);
					// Увеличиваем максимумы автонастройки MacOS X TCP
					os.sysctl("net.inet.tcp.autorcvbufmax", 33554432);
					os.sysctl("net.inet.tcp.autosndbufmax", 33554432);
					// Устанавливаем прочие настройки
					os.sysctl("net.inet.tcp.slowstart_flightsize", 20);
					os.sysctl("net.inet.tcp.local_slowstart_flightsize", 20);
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD или OpenBSD
			 */
			#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					/**
					 * Данные оптимизаций операционной системы берет от сюда: http://fasterdata.es.net/host-tuning/freebsd
					 */
					// Активируем контроль работы временной марки и масштабируемого окна
					os.sysctl("net.inet.tcp.rfc1323", 1);
					// Устанавливаем максимальное количество подключений
					os.sysctl("kern.ipc.somaxconn", 49152);
					// Активируем автоматическую отправку и получение
					os.sysctl("net.inet.tcp.sendbuf_auto", 1);
					os.sysctl("net.inet.tcp.recvbuf_auto", 1);
					// Увеличиваем размер шага автонастройки
					os.sysctl("net.inet.tcp.sendbuf_inc", 8192);
					os.sysctl("net.inet.tcp.recvbuf_inc", 16384);
					// Активируем нормальное нормальное TCP Reno
					os.sysctl("net.inet.tcp.inflight.enable", 0);
					// Активируем на хостах тестирования/измерений
					os.sysctl("net.inet.tcp.hostcache.expire", 1);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок FreeBSD
					 */
					os.sysctl("kern.ipc.maxsockbuf", 16777216);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvspace", 1042560);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendbuf_max", 16777216);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvbuf_max", 16777216);
					/**
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.inet.tcp.cc.available
					 */
					const string & algorithm = os.sysctl <string> ("net.inet.tcp.cc.available");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм cubic
						if(fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "cubic");
						// Если же найден алгоритм htcp
						else if(fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "htcp");
					}
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					#endif
				}
			#endif
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}

/**
 * @brief Метод опроса событий
 *
 * @param timeout таймаут опроса в миллисекундах
 * @return        результат выполнения опроса
 */
bool awh::IO::poll(const int32_t timeout) noexcept {

	return false;
}
/**
 * @brief Метод получения порта события
 *
 * @param id идентификатор события
 * @return   порт события
 */
uint32_t awh::IO::port(const event::id_t id) const noexcept {

	return 0;
}
/**
 * @brief Метод установки порта события
 *
 * @param id   идентификатор события
 * @param port порт события
 * @return     результат выполнения установки
 */
bool awh::IO::port(const event::id_t id, const uint32_t port) noexcept {

	return false;
}
/**
 * @brief Метод получения хоста события
 *
 * @param id идентификатор события
 * @return   хост события
 */
string awh::IO::host(const event::id_t id) const noexcept {
	return "";
}
/**
 * @brief Метод установки хоста события
 *
 * @param id   идентификатор события
 * @param host хост события
 * @return     результат выполнения установки
 */
bool awh::IO::host(const event::id_t id, const string & host) noexcept {

	return false;
}
/**
 * @brief Метод получения адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @return        значение адреса события
 */
string awh::IO::address(const event::id_t id, const event::address_t address) const noexcept {
	
	return "";
}
/**
 * @brief Метод установки адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @param value   значение адреса события
 * @return        результат выполнения установки
 */
bool awh::IO::address(const event::id_t id, const event::address_t address, const string & value) noexcept {
	
	return false;
}
/**
 * @brief Метод настройки события
 *
 * @param id    идентификатор события
 * @param delay задержка таймера события в миллисекундах
 * @return      результат выполнения настройки
 */
bool awh::IO::setup(const event::id_t id, const uint32_t delay) noexcept {
	
	return false;
}
/**
 * @brief Метод настройки события
 *
 * @param id   идентификатор события
 * @param node тип узла события
 * @return     результат выполнения настройки
 */
bool awh::IO::setup(const event::id_t id, const event::node_t node) noexcept {
	return false;
}
/**
 * @brief Метод удаления события
 *
 * @param id идентификатор события
 * @return   результат выполнения удаления
 */
bool awh::IO::destroy(const event::id_t id) noexcept {
	
	return false;
}
/**
 * @brief Метод получения пары событий для сокета
 *
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         пара идентификаторов созданных событий
 */
std::array <awh::event::id_t, 2> awh::IO::events(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {

	return {0, 0};
}
/**
 * @brief Метод создания нового события
 *
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Определяем семейство сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства UDP
		case static_cast <uint8_t> (event::family_t::UDP): {
			/**
			 * Определяем тип сокета
			 */
			switch(static_cast <uint8_t> (type)){
				// Для типа сокета RAW
				case static_cast <uint8_t> (event::type_t::RAW):
				// Для типа сокета DATAGRAM
				case static_cast <uint8_t> (event::type_t::DATAGRAM): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для неизвестного типа сокета
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("An event for a UDP socket cannot be created because it has an invalid initialization type", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
					#endif
				}
			}
		} break;
		// Для семейства UNIX-доменных сокетов
		case static_cast <uint8_t> (event::family_t::UDS): {
			/**
			 * Определяем тип сокета
			 */
			switch(static_cast <uint8_t> (type)){
				// Для типа сокета STREAM
				case static_cast <uint8_t> (event::type_t::STREAM):
				// Для типа сокета DATAGRAM
				case static_cast <uint8_t> (event::type_t::DATAGRAM): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для неизвестного типа сокета
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("An event for a Unix socket cannot be created because it has an invalid initialization type", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
					#endif
				}
			}
		} break;
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4):
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			/**
			 * Определяем тип сокета
			 */
			switch(static_cast <uint8_t> (type)){
				// Для типа сокета STREAM
				case static_cast <uint8_t> (event::type_t::STREAM):
				// Для типа сокета SEQPACKET
				case static_cast <uint8_t> (event::type_t::SEQPACKET): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для неизвестного типа сокета
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("An event for a IP socket cannot be created because it has an invalid initialization type", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
					#endif
				}
			}
		} break;
		// Для семейства директорий
		case static_cast <uint8_t> (event::family_t::DIR):
		// Для семейства файловой системы
		case static_cast <uint8_t> (event::family_t::FILE): {
			// Выполняем создание события
			auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <fs_t> ());
			// Устанавливаем флаг семейства сокета
			ret.first->second->state.family = family;
			// Устанавливаем флаг типа сокета
			ret.first->second->state.type = type;
			// Устанавливаем флаг режима сокета
			ret.first->second->state.mode = mode;
			// Устанавливаем флаг протокола сокета
			ret.first->second->state.protocol = protocol;
			// Возвращаем идентификатор созданного события
			result = ret.first->first;
		} break;
		// Для семейства таймеров
		case static_cast <uint8_t> (event::family_t::TIMER):
		// Для семейства интервалов
		case static_cast <uint8_t> (event::family_t::INTERVAL): {
			// Выполняем создание события
			auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <timer_t> ());
			// Устанавливаем флаг семейства сокета
			ret.first->second->state.family = family;
			// Устанавливаем флаг типа сокета
			ret.first->second->state.type = type;
			// Устанавливаем флаг режима сокета
			ret.first->second->state.mode = mode;
			// Устанавливаем флаг протокола сокета
			ret.first->second->state.protocol = protocol;
			// Возвращаем идентификатор созданного события
			result = ret.first->first;
		} break;
		// Для неизвестного семейства
		default: {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Event cannot be created because the family it belongs to is not defined", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Event cannot be created because the family it belongs to is not defined", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод создания нового события на основе существующего
 *
 * @param id       идентификатор существующего события
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::id_t id, const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {

	return 0;
}
/**
 * @brief Метод получения режима действия события
 *
 * @param id     идентификатор события
 * @param action действие события
 * @return       режим действия события
 */
awh::event::notify_t awh::IO::action(const event::id_t id, const event::action_t action) noexcept {

	return event::notify_t::DISABLED;
}
/**
 * @brief Метод установки режима действия события
 *
 * @param id     идентификатор события
 * @param action действие события
 * @param notify уведомления события
 * @return       результат выполнения установки
 */
bool awh::IO::action(const event::id_t id, const event::action_t action, const event::notify_t notify) noexcept {

	return false;
}
/**
 * @brief Метод установки флага только IPv6 для события
 *
 * @param id     идентификатор события
 * @param enable флаг только IPv6
 * @return       результат выполнения установки
 */
bool awh::IO::onlyIPv6(const event::id_t id, const bool enable) noexcept {

	return false;
}
/**
 * @brief Метод установки опции события
 *
 * @param id     идентификатор события
 * @param option опция события
 * @param value  значение опции события
 * @return       результат выполнения установки
 */
bool awh::IO::option(const event::id_t id, const event::option_t option, const int32_t value) noexcept {
	
	return false;
}
/**
 * @brief Метод отключения события
 *
 * @param id идентификатор события
 * @return   результат выполнения отключения
 */
bool awh::IO::disconnect(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод подключения события к удалённому хосту
 *
 * @param id    идентификатор события
 * @param async флаг асинхронного подключения
 * @return      результат выполнения подключения
 */
bool awh::IO::connect(const event::id_t id, const bool async) noexcept {

	return false;
}
/**
 * @brief Метод принятия входящего соединения события
 *
 * @param id    идентификатор события
 * @param max   максимальное количество входящих соединений
 * @param async флаг асинхронного принятия соединения
 * @return      результат выполнения принятия соединения
 */
bool awh::IO::accept(const event::id_t id, const uint32_t max, const bool async) noexcept {

	return false;
}
/**
 * @brief Метод отправки события
 *
 * @param value значение события для отправки
 * @return      результат выполнения отправки
 */
bool awh::IO::post(const uint32_t value) noexcept {
	
	return false;
}
/**
 * @brief Метод отправки данных события
 *
 * @param id   идентификатор события
 * @param data указатель на данные для отправки
 * @param size размер данных для отправки
 * @return     результат выполнения отправки
 */
bool awh::IO::send(const event::id_t id, const char * data, const size_t size) noexcept {

	return false;
}
/**
 * @brief Метод очистки всех адресов сетей для выхода в интернет
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearNetworks(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод получения списка адресов сетей для выхода в интернет
 *
 * @param id идентификатор события
 * @return   список адресов сетей события
 */
std::unordered_set <string> awh::IO::networks(const event::id_t id) const noexcept {
	
	return {};
}
/**
 * @brief Метод добавления адреса сети для выхода в интернет
 *
 * @param id      идентификатор события
 * @param network адрес сети для добавления
 * @return        результат выполнения добавления
 */
bool awh::IO::addNetwork(const event::id_t id, const string & network) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления адреса сети для выхода в интернет
 *
 * @param id      идентификатор события
 * @param network адрес сети для удаления
 * @return        результат выполнения удаления
 */
bool awh::IO::removeNetwork(const event::id_t id, const string & network) noexcept {
	
	return false;
}
/**
 * @brief Метод добавления списка адресов сетей для выхода в интернет
 *
 * @param id       идентификатор события
 * @param networks список адресов сетей для добавления
 * @return         результат выполнения добавления
 */
bool awh::IO::addNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления списка адресов сетей для выхода в интернет
 *
 * @param id       идентификатор события
 * @param networks список адресов сетей для удаления
 * @return         результат выполнения удаления
 */
bool awh::IO::removeNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept {

	return false;
}
/**
 * @brief Метод очистки всех сетевых интерфейсов события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearNetworkInterfaces(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод получения списка сетевых интерфейсов события
 *
 * @param id идентификатор события
 * @return   список сетевых интерфейсов события
 */
std::unordered_set <string> awh::IO::networkInterfaces(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод добавления сетевого интерфейса для события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для добавления
 * @return     результат выполнения добавления
 */
bool awh::IO::addNetworkInterface(const event::id_t id, const string & name) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления сетевого интерфейса для события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для удаления
 * @return     результат выполнения удаления
 */
bool awh::IO::removeNetworkInterface(const event::id_t id, const string & name) noexcept {
	
	return false;
}
/**
 * @brief Метод добавления списка сетевых интерфейсов для события
 *
 * @param id    идентификатор события
 * @param names список сетевых интерфейсов для добавления
 * @return      результат выполнения добавления
 */
bool awh::IO::addNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления списка сетевых интерфейсов для события
 *
 * @param id    идентификатор события
 * @param names список сетевых интерфейсов для удаления
 * @return      результат выполнения удаления
 */
bool awh::IO::removeNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept {
	
	return false;
}
/**
 * @brief Метод присоединения события к мультикаст группе
 *
 * @param id               идентификатор события
 * @param multicastAddress адрес мультикаст группы для присоединения
 * @return                 результат выполнения присоединения
 */
bool awh::IO::multicastJoin(const event::id_t id, const string & multicastAddress) noexcept {

	return false;
}
/**
 * @brief Метод выхода события из мультикаст группы
 *
 * @param id               идентификатор события
 * @param multicastAddress адрес мультикаст группы для выхода
 * @return                 результат выполнения выхода
 */
bool awh::IO::multicastLeave(const event::id_t id, const string & multicastAddress) noexcept {

	return false;
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearBlacklist(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param id      идентификатор события
 * @param address адрес для добавления в чёрный список
 * @return        результат выполнения добавления
 */
bool awh::IO::addToBlacklist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param id      идентификатор события
 * @param address адрес для удаления из чёрного списка
 * @return        результат выполнения удаления
 */
bool awh::IO::removeFromBlacklist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param id идентификатор события
 * @return   чёрный список события
 */
std::unordered_map <awh::event::address_t, string> awh::IO::blacklist(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearWhitelist(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод добавления адреса в белый список события
 * @param id      идентификатор события
 * @param address адрес для добавления в белый список
 * @return        результат выполнения добавления
 */
bool awh::IO::addToWhitelist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод удаления адреса из белого списка события
 *
 * @param id      идентификатор события
 * @param address адрес для удаления из белого списка
 * @return        результат выполнения удаления
 */
bool awh::IO::removeFromWhitelist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод получения белого списка события
 *
 * @param id идентификатор события
 * @return   белый список события
 */
std::unordered_map <awh::event::address_t, string> awh::IO::whitelist(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод установки таймаута на чтение события
 *
 * @param id      идентификатор события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::readTimeout(const event::id_t id, const uint32_t timeout) noexcept {

	
}
/**
 * @brief Метод установки таймаута на запись события
 *
 * @param id      идентификатор события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::writeTimeout(const event::id_t id, const uint32_t timeout) noexcept {

}
/**
 * @brief Метод установки глубины очереди принятия входящих соединений события
 *
 * @param id       идентификатор события
 * @param depth    глубина очереди принятия входящих соединений
 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
 */
void awh::IO::backlog(const event::id_t id, const uint32_t depth, const bool adaptive) noexcept {

}
/**
 * @brief Метод получения размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия с буфером
 * @return       размер буфера события
 */
size_t awh::IO::bufferSize(const event::id_t id, const event::action_t action) noexcept {

	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия с буфером
 * @param size   размер буфера события
 * @return       результат выполнения установки
 */
bool awh::IO::bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept {

	return false;
}
/**
 * @brief Метод установки параметров keep-alive для события
 *
 * @param id    идентификатор события
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::IO::keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {

	return false;
}
/**
 * @brief Метод приостановки события
 *
 * @param id идентификатор события
 * @return   результат выполнения приостановки
 */
bool awh::IO::pause(const event::id_t id) noexcept {
	
	return false;
}
/**
 * @brief Метод возобновления события
 *
 * @param id идентификатор события
 * @return   результат выполнения возобновления
 */
bool awh::IO::resume(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод проверки состояния события
 *
 * @param id идентификатор события
 * @return   состояние события
 */
bool awh::IO::isAlive(const event::id_t id) const noexcept {

	return false;
}
/**
 * @brief Метод инициализации основного движка фреймворка
 *
 * @return результат выполнения инициализации
 */
bool awh::IO::initialize() noexcept {
	/**
	 * Выполняем настройку сетевых параметров
	 */
	::boostingNetwork(this->_fmk, this->_log);


	return false;
}
/**
 * @brief Метод деинициализации основного движка фреймворка
 *
 * @return результат выполнения деинициализации
 */
bool awh::IO::deinitialize() noexcept {

	return false;
}
/**
 * @brief Метод проверки состояния инициализации основного движка фреймворка
 *
 * @return состояние инициализации
 */
bool awh::IO::isInitialized() const noexcept {

	return false;
}
/**
 * @brief Метод получения режима события
 *
 * @param id идентификатор события
 * @return   режим события
 */
awh::event::mode_t awh::IO::mode(const event::id_t id) noexcept {
	
	return event::mode_t::NONE;
}
/**
 * @brief Метод получения узла события
 *
 * @param id идентификатор события
 * @return   узел события
 */
awh::event::node_t awh::IO::node(const event::id_t id) noexcept {

	return event::node_t::NONE;
}
/**
 * @brief Метод получения типа события
 *
 * @param id идентификатор события
 * @return   тип события
 */
awh::event::type_t awh::IO::type(const event::id_t id) noexcept {

	return event::type_t::NONE;
}
/**
 * @brief Метод получения семейства события
 *
 * @param id идентификатор события
 * @return   семейство события
 */
awh::event::family_t awh::IO::family(const event::id_t id) noexcept {

	return event::family_t::NONE;
}
/**
 * @brief Метод получения статуса события
 *
 * @param id идентификатор события
 * @return   статус события
 */
awh::event::status_t awh::IO::status(const event::id_t id) noexcept {

	return event::status_t::NONE;
}
/**
 * @brief Методы установки функции обратного вызова на чтение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const readCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на запись события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const writeCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на ошибку события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const errorCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на изменение статуса события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const statusCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на принятие события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const acceptCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на подключение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const connectCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на получение пользовательского события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const userEventCallback & cb) noexcept {

}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::IO::IO(const fmk_t * fmk, const log_t * log) noexcept : engine_t(fmk, log) {

}
/**
 * @brief Деструктор
 *
 */
awh::IO::~IO() noexcept {

}
