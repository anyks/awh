/**
 * @file: sample.hpp
 * @date: 2022-09-01
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

#ifndef __AWH_SAMPLE_CLIENT__
#define __AWH_SAMPLE_CLIENT__

/**
 * Стандартные модули
 */
#include <stack>
#include <functional>

/**
 * Наши модули
 */
#include <sys/hold.hpp>
#include <sys/buffer.hpp>
#include <sys/callback.hpp>
#include <core/client.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Sample Класс работы с примером клиента
		 */
		typedef class AWHSHARED_EXPORT Sample {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01,
					DISCONNECT = 0x02
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE    = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP = 0x03  // Флаг запрета остановки работы базы событий
				};
			private:
				/**
				 * Идентификаторы текущего события
				 */
				enum class event_t : uint8_t {
					NONE          = 0x00, // Событие не установлено
					OPEN          = 0x01, // Событие открытия подключения
					READ          = 0x02, // Событие чтения данных с сервера
					SEND          = 0x03, // Событие отправки данных на сервер
					CONNECT       = 0x04, // Событие подключения к серверу
					PROXY_READ    = 0x05, // Событие чтения данных с прокси-сервера
					PROXY_CONNECT = 0x06  // Событие подключения к прокси-серверу
				};
			private:
				/**
				 * Proxy Структура работы с прокси-сервером
				 */
				typedef struct Proxy {
					bool mode;       // Флаг активации работы прокси-сервера
					uint32_t answer; // Статус ответа прокси-сервера
					/**
					 * Proxy Конструктор
					 */
					Proxy() noexcept : mode(false), answer(0) {}
				} __attribute__((packed)) proxy_t;
			private:
				// Идентификатор подключения
				uint64_t _bid;
			private:
				// Флаг чтения данных из буфера
				bool _reading;
				// Флаг остановки работы базы событий
				bool _complete;
			private:
				// Количество попыток
				uint8_t _attempt;
				// Общее количество попыток
				uint8_t _attempts;
			private:
				// Объект IP-адресов
				net_t _net;
				// Объект работы с URI
				uri_t _uri;
				// Объект параметров работы с прокси-сервером
				proxy_t _proxy;
				// Объект сетевой схемы
				scheme_t _scheme;
				// Объект буфера данных
				buffer_t _buffer;
				// Хранилище функций обратного вызова
				callback_t _callback;
			private:
				// Список рабочих событий
				std::stack <event_t> _events;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const client::core_t * _core;
			private:
				/**
				 * openEvent Метод обратного вызова при запуске работы
				 * @param sid идентификатор схемы сети
				 */
				void openEvent(const uint16_t sid) noexcept;
				/**
				 * statusEvent Метод обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 */
				void statusEvent(const awh::core_t::status_t status) noexcept;
				/**
				 * connectEvent Метод обратного вызова при подключении к серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void connectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * disconnectEvent Метод обратного вызова при отключении от сервера
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * readEvent Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * enableSSLEvent Метод активации зашифрованного канала SSL
				 * @param url адрес сервера для которого выполняется активация зашифрованного канала SSL
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 * @return    результат активации зашифрованного канала SSL
				 */
				bool enableSSLEvent(const uri_t::url_t & url, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * chunking Метод обработки получения чанков
				 * @param bid   идентификатор брокера
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * proxyConnectEvent Метод обратного вызова при подключении к прокси-серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void proxyConnectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * proxyReadEvent Метод обратного вызова при чтении сообщения с прокси-сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void proxyReadEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			public:
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
			public:
				/**
				 * close Метод закрытия подключения клиента
				 */
				void close() noexcept;
			public:
				/**
				 * init Метод инициализации Rest брокера
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * init Метод инициализации Rest брокера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const uint32_t port, const string & host) noexcept;
			public:
				/**
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const char * name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const string & name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (fid, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param A    тип идентификатора функции
				 * @param B    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename A, typename B, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const A fid, Args... args) noexcept -> uint64_t {
					// Если мы получили на вход число
					if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
					// Выводим результат по умолчанию
					return 0;
				}
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const std::set <flag_t> & flags) noexcept;
			public:
				/**
				 * cork Метод отключения/включения алгоритма TCP/CORK
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool cork(const engine_t::mode_t mode) noexcept;
				/**
				 * nodelay Метод отключения/включения алгоритма Нейгла
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool nodelay(const engine_t::mode_t mode) noexcept;
			public:
				/**
				 * response Метод отправки сообщения брокеру
				 * @param buffer буфер бинарных данных для отправки
				 * @param size   размер бинарных данных для отправки
				 */
				void send(const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * bandwidth Метод установки пропускной способности сети
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const string & read = "", const string & write = "") noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * waitMessage Метод ожидания входящих сообщений
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const uint16_t read = READ_TIMEOUT, const uint16_t write = WRITE_TIMEOUT, const uint16_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * userAgentProxy Метод установки User-Agent для HTTP-запроса прокси-сервера
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				void userAgentProxy(const string & userAgent) noexcept;
				/**
				 * identProxy Метод установки идентификации клиента прокси-сервера
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void identProxy(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * proxy Метод активации/деактивации прокси-склиента
				 * @param work флаг активации/деактивации прокси-клиента
				 */
				void proxy(const client::scheme_t::work_t work) noexcept;
				/**
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / IPC)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * Sample Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Sample(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Sample Деструктор
				 */
				~Sample() noexcept {}
		} sample_t;
	};
};

#endif // __AWH_SAMPLE_CLIENT__
