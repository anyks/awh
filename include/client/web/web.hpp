/**
 * @file: web.hpp
 * @date: 2023-09-11
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

#ifndef __AWH_WEB_CLIENT__
#define __AWH_WEB_CLIENT__

/**
 * Стандартные модули
 */
#include <map>
#include <stack>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include "../../net/uri.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../sys/hold.hpp"
#include "../../sys/buffer.hpp"
#include "../../sys/callback.hpp"
#include "../../http/http2.hpp"
#include "../../http/client.hpp"
#include "../../core/timer.hpp"
#include "../../core/client.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief Базовый класс web-клиента
		 *
		 */
		typedef class AWHSHARED_EXPORT Web {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					OPEN       = 0x01, // Открытие передачи данных
					CLOSE      = 0x02, // Закрытие передачи данных
					CONNECT    = 0x03, // Флаг подключения
					DISCONNECT = 0x04  // Флаг отключения
				};
				/**
				 * Направления передачи фреймов
				 */
				enum class direct_t : uint8_t {
					NONE = 0x00, // Направление не установлено
					SEND = 0x01, // Направление отправки
					RECV = 0x02  // Направление получения
				};
				/**
				 * Идентификатор агента
				 */
				enum class agent_t : uint8_t {
					NONE      = 0x00, // Агент не определён
					HTTP      = 0x01, // HTTP-клиент
					WEBSOCKET = 0x02  // Websocket-клиент
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE                 = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO              = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP              = 0x03, // Флаг запрета остановки работы базы событий
					NOT_PING              = 0x04, // Флаг запрещающий выполнение пингов
					REDIRECTS             = 0x05, // Флаг разрешающий автоматическое перенаправление запросов
					NO_INIT_SSL           = 0x06, // Флаг запрещающий переключение контекста SSL
					TAKEOVER_CLIENT       = 0x07, // Флаг ожидания входящих сообщений для клиента
					TAKEOVER_SERVER       = 0x08, // Флаг ожидания входящих сообщений для сервера
					WEBSOCKET_ENABLE      = 0x09, // Флаг разрешения использования Websocket-клиента
					CONNECT_METHOD_ENABLE = 0x0A  // Флаг разрешающий метод CONNECT для прокси-сервера
				};
			public:
				/**
				 * @brief Класс HTTP-запроса клиента
				 *
				 */
				typedef class AWHSHARED_EXPORT Request {
					public:
						uint64_t id;                                      // Идентификатор запроса
						agent_t agent;                                    // Агент воркера выполнения запроса
						uri_t::url_t url;                                 // URL-запроса запроса
						web_t::method_t method;                           // Метод запроса
						vector <char> entity;                             // Тело запроса
						vector <http_t::compressor_t> compressors;        // Список поддерживаемых компрессоров
						std::unordered_multimap <string, string> headers; // Заголовки клиента
					public:
						/**
						 * @brief Оператор [=] перемещения параметров запроса
						 *
						 * @param request объект параметров запроса
						 * @return        объект текущего запроса
						 */
						Request & operator = (Request && request) noexcept;
						/**
						 * @brief Оператор [=] присванивания параметров запроса
						 *
						 * @param request объект параметров запроса
						 * @return        объект текущего запроса
						 */
						Request & operator = (const Request & request) noexcept;
					public:
						/**
						 * @brief Оператор сравнения
						 *
						 * @param request объект параметров запроса
						 * @return        результат сравнения
						 */
						bool operator == (const Request & request) noexcept;
					public:
						/**
						 * @brief Конструктор перемещения
						 *
						 * @param request объект параметров запроса
						 */
						Request(Request && request) noexcept;
						/**
						 * @brief Конструктор копирования
						 *
						 * @param request объект параметров запроса
						 */
						Request(const Request & request) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						Request() noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Request() noexcept {}
				} request_t;
			protected:
				/**
				 * @brief Структура работы с прокси-сервером
				 *
				 */
				typedef struct Proxy {
					int32_t sid;     // Идентификатор потока HTTP/2
					bool mode;       // Флаг активации работы прокси-сервера
					bool connect;    // Флаг применения метода CONNECT
					uint32_t answer; // Статус ответа прокси-сервера
					/**
					 * @brief Конструктор
					 *
					 */
					Proxy() noexcept :
					 sid(-1), mode(false),
					 connect(true), answer(0) {}
				} __attribute__((packed)) proxy_t;
				/**
				 * @brief Структура параметров шифрования
				 *
				 */
				typedef struct Encryption {
					bool mode;               // Флаг активности механизма шифрования
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					/**
					 * @brief Конструктор
					 *
					 */
					Encryption() noexcept :
					 mode(false), pass{""}, salt{""},
					 cipher(hash_t::cipher_t::AES128) {}
				} encryption_t;
			protected:
				/**
				 * Этапы обработки
				 */
				enum class status_t : uint8_t {
					STOP = 0x00, // Остановить обработку
					NEXT = 0x01, // Следующий этап обработки
					SKIP = 0x02  // Пропустить этап обработки
				};
				/**
				 * Идентификаторы текущего события
				 */
				enum class event_t : uint8_t {
					NONE          = 0x00, // Событие не установлено
					OPEN          = 0x01, // Событие открытия подключения
					READ          = 0x02, // Событие чтения данных с сервера
					SEND          = 0x03, // Событие отправки данных на сервер
					SUBMIT        = 0x04, // Событие HTTP-запроса на удаленный сервер
					CONNECT       = 0x05, // Событие подключения к серверу
					PROXY_READ    = 0x06, // Событие чтения данных с прокси-сервера
					PROXY_CONNECT = 0x07  // Событие подключения к прокси-серверу
				};
			protected:
				// Идентификатор подключения
				uint64_t _bid;
			protected:
				// Объект работы с URI
				uri_t _uri;
				// Объект параметров работы с прокси-сервером
				proxy_t _proxy;
				// Объект рабочего
				scheme_t _scheme;
				// Хранилище функций обратного вызова
				callback_t _callback;
				// Объект параметров шифрования
				encryption_t _encryption;
			protected:
				// Флаг запрещающий переключение контекста SSL
				bool _nossl;
				// Флаг чтения данных из буфера
				bool _reading;
				// Флаг принудительной остановки
				bool _stopped;
				// Флаг разрешающий выполнение пингов
				bool _pinging;
				// Флаг остановки работы базы событий
				bool _complete;
				// Флаг выполнения редиректов
				bool _redirects;
			protected:
				// Количество попыток
				uint8_t _attempt;
				// Общее количество попыток
				uint8_t _attempts;
			protected:
				// Время отправленного пинга
				uint64_t _sendPing;
			protected:
				// Интервал времени на выполнение пингов
				uint32_t _pingInterval;
			protected:
				// Объект буфера данных
				buffer_t _buffer;
			private:
				// Объект локального таймера
				awh::timer_t _timer;
			protected:
				// Список рабочих событий
				std::stack <event_t> _events;
			protected:
				// Список поддерживаемых компрессоров
				vector <http_t::compressor_t> _compressors;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const client::core_t * _core;
			protected:
				/**
				 * @brief Метод обратного вызова при запуске работы
				 *
				 * @param sid идентификатор схемы сети
				 */
				void openEvent(const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при активации ядра сервера
				 *
				 * @param status флаг запуска/остановки
				 */
				virtual void statusEvent(const awh::core_t::status_t status) noexcept;
			protected:
				/**
				 * @brief Метод обратного вызова при подключении к серверу
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void connectEvent(const uint64_t bid, const uint16_t sid) noexcept = 0;
				/**
				 * @brief Метод обратного вызова при отключении от сервера
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept = 0;
				/**
				 * @brief Метод обратного вызова при чтении сообщения с сервера
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				virtual void readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept = 0;
			protected:
				/**
				 * @brief Метод обратного вызова при подключении к прокси-серверу
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void proxyConnectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при чтении сообщения с прокси-сервера
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				virtual void proxyReadEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод активации зашифрованного канала SSL
				 *
				 * @param url адрес сервера для которого выполняется активация зашифрованного канала SSL
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 * @return    результат активации зашифрованного канала SSL
				 */
				bool enableSSLEvent(const uri_t::url_t & url, const uint64_t bid, const uint16_t sid) noexcept;
			protected:
				/**
				 * @brief Метод отлавливания событий контейнера функций обратного вызова
				 *
				 * @param event обытие контейнера функций обратного вызова
				 * @param fid   идентификатор функции обратного вызова
				 * @param fn    функция обратного вызова в чистом виде
				 */
				virtual void callbackEvent(const callback_t::event_t event, const uint64_t fid, const callback_t::fn_t & fn) noexcept = 0;
			protected:
				/**
				 * @brief Метод обработки получения чанков
				 *
				 * @param bid   идентификатор брокера
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				virtual void chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept;
			protected:
				/**
				 * @brief Метод вывода полученных ошибок протокола
				 *
				 * @param bid     идентификатор брокера
				 * @param flag    флаг типа сообщения
				 * @param error   тип полученной ошибки
				 * @param message сообщение полученной ошибки
				 */
				void errors(const uint64_t bid, const log_t::flag_t flag, const awh::http::error_t error, const string & message) noexcept;
			protected:
				/**
				 * @brief Метод сброса параметров запроса
				 *
				 */
				virtual void flush() noexcept = 0;
			protected:
				/**
				 * @brief Метод таймера выполнения пинга удалённого сервера
				 *
				 * @param tid идентификатор таймера
				 */
				virtual void pinging(const uint16_t tid) noexcept = 0;
			protected:
				/**
				 * @brief Метод выполнения препарирования полученных данных
				 *
				 * @param sid идентификатор запроса
				 * @param bid идентификатор брокера
				 * @return    результат препарирования
				 */
				virtual status_t prepare(const int32_t sid, const uint64_t bid) noexcept = 0;
			public:
				/**
				 * @brief Метод инициализации клиента
				 *
				 * @param dest        адрес назначения удалённого сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & dest, const vector <awh::http_t::compressor_t> & compressors = {}) noexcept;
			public:
				/**
				 * @brief Метод открытия подключения
				 *
				 */
				void open() noexcept;
			public:
				/**
				 * @brief Метод принудительного сброса подключения
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * @brief Метод остановки клиента
				 *
				 */
				virtual void stop() noexcept;
				/**
				 * @brief Метод запуска клиента
				 *
				 */
				virtual void start() noexcept;
			public:
				/**
				 * @brief Метод установки времени ожидания ответа WebSocket-сервера
				 *
				 * @param sec время ожидания в секундах
				 */
				virtual void waitPong(const uint16_t sec) noexcept = 0;
				/**
				 * @brief Метод установки интервала времени выполнения пингов
				 *
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				virtual void pingInterval(const uint16_t sec) noexcept = 0;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				virtual void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
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
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
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
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
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
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam A    тип идентификатора функции
				 * @tparam B    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename A, typename B, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
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
				 * @brief Метод извлечения поддерживаемого протокола подключения
				 *
				 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto() const noexcept;
			public:
				/**
				 * @brief Метод отключения/включения алгоритма TCP/CORK
				 *
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool cork(const engine_t::mode_t mode) noexcept;
				/**
				 * @brief Метод отключения/включения алгоритма Нейгла
				 *
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool nodelay(const engine_t::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сети
				 *
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const string & read = "", const string & write = "") noexcept;
			public:
				/**
				 * @brief Метод ожидания входящих сообщений
				 *
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * @brief Метод детекции сообщений по количеству секунд
				 *
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const uint16_t read = READ_TIMEOUT, const uint16_t write = WRITE_TIMEOUT, const uint16_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации прокси-склиента
				 *
				 * @param work флаг активации/деактивации прокси-клиента
				 */
				virtual void proxy(const client::scheme_t::work_t work) noexcept;
				/**
				 * @brief Метод установки прокси-сервера
				 *
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / IPC)
				 */
				virtual void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * @brief Метод установки общего количества попыток
				 *
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
				/**
				 * @brief Метод установки сетевого ядра
				 *
				 * @param core объект сетевого ядра
				 */
				virtual void core(const client::core_t * core) noexcept;
				/**
				 * @brief Метод установки списка поддерживаемых компрессоров
				 *
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <awh::http_t::compressor_t> & compressors) noexcept;
				/**
				 * @brief Метод установки жизни подключения
				 *
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод установки флагов настроек модуля
				 *
				 * @param flags список флагов настроек модуля для установки
				 */
				virtual void mode(const std::set <flag_t> & flags) noexcept = 0;
			public:
				/**
				 * @brief Метод установки размера чанка
				 *
				 * @param size размер чанка для установки
				 */
				virtual void chunk(const size_t size) noexcept = 0;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				virtual void user(const string & login, const string & password) noexcept = 0;
			public:
				/**
				 * @brief Метод установки User-Agent для HTTP-запроса
				 *
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				virtual void userAgent(const string & userAgent) noexcept;
				/**
				 * @brief Метод установки идентификации клиента
				 *
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				virtual void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * @brief Метод установки типа авторизации
				 *
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				virtual void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept = 0;
				/**
				 * @brief Метод установки типа авторизации прокси-сервера
				 *
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				virtual void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * @brief Метод активации шифрования
				 *
				 * @param mode флаг активации шифрования
				 */
				virtual void encryption(const bool mode) noexcept;
				/**
				 * @brief Метод установки параметров шифрования
				 *
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				virtual void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Web(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Web(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Web() noexcept {}
		} web_t;
		/**
		 * @brief Базовый класс web2-клиента
		 *
		 */
		typedef class Web2 : public web_t {
			protected:
				/**
				 * @brief Структура идентификации сервиса
				 *
				 */
				typedef struct Ident {
					string id;   // Идентификатор сервиса
					string ver;  // Версия сервиса
					string name; // Название сервиса
					/**
					 * @brief Конструктор
					 *
					 */
					Ident() noexcept : id{""}, ver{""}, name{""} {}
				} ident_t;
			protected:
				// Объект идентификации сервиса
				ident_t _ident;
				// Объект работы с фреймами Http2
				http2_t _http2;
			protected:
				// Логин пользователя для авторизации на сервере
				string _login;
				// Пароль пользователя для авторизации на сервере
				string _password;
			protected:
				// User-Agent для HTTP-запроса
				string _userAgent;
			protected:
				// Размер одного чанка бинарных данных
				size_t _chunkSize;
			protected:
				// Список параметров настроек протокола HTTP/2
				std::map <http2_t::settings_t, uint32_t> _settings;
			protected:
				/**
				 * @brief Метод обратного вызова при отправки данных HTTP/2
				 *
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера данных для отправки
				 */
				void sendSignal(const uint8_t * buffer, const size_t size) noexcept;
			protected:
				/**
				 * @brief Метод обратного вызова при получении фрейма заголовков прокси-сервера HTTP/2
				 *
				 * @param sid    идентификатор потока
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаги полученного фрейма
				 * @return       статус полученных данных
				 */
				int32_t frameProxySignal(const int32_t sid, const http2_t::direct_t direct, const http2_t::frame_t frame, const std::set <http2_t::flag_t> & flags) noexcept;
				/**
				 * @brief Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
				 *
				 * @param sid    идентификатор потока
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаги полученного фрейма
				 * @return       статус полученных данных
				 */
				virtual int32_t frameSignal(const int32_t sid, const http2_t::direct_t direct, const http2_t::frame_t frame, const std::set <http2_t::flag_t> & flags) noexcept = 0;
			protected:
				/**
				 * @brief Метод обратного вызова при получении чанка с прокси-сервера HTTP/2
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int32_t chunkProxySignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод обратного вызова при получении чанка с сервера HTTP/2
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				virtual int32_t chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept = 0;
			protected:
				/**
				 * @brief Метод начала получения фрейма заголовков HTTP/2 прокси-сервера
				 *
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				int32_t beginProxySignal(const int32_t sid) noexcept;
				/**
				 * @brief Метод начала получения фрейма заголовков HTTP/2 сервера
				 *
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				virtual int32_t beginSignal(const int32_t sid) noexcept = 0;
			protected:
				/**
				 * @brief Метод завершения работы потока
				 *
				 * @param sid   идентификатор потока
				 * @param error флаг ошибки если присутствует
				 * @return      статус полученных данных
				 */
				virtual int32_t closedSignal(const int32_t sid, const http2_t::error_t error) noexcept = 0;
			protected:
				/**
				 * @brief Метод обратного вызова при получении заголовка HTTP/2 прокси-сервера
				 *
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int32_t headerProxySignal(const int32_t sid, const string & key, const string & val) noexcept;
				/**
				 * @brief Метод обратного вызова при получении заголовка HTTP/2 сервера
				 *
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				virtual int32_t headerSignal(const int32_t sid, const string & key, const string & val) noexcept = 0;
			protected:
				/**
				 * @brief Метод обратного вызова при активации ядра сервера
				 *
				 * @param status флаг запуска/остановки
				 */
				void statusEvent(const awh::core_t::status_t status) noexcept;
				/**
				 * @brief Метод обратного вызова при подключении к серверу
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void connectEvent(const uint64_t bid, const uint16_t sid) noexcept;
			protected:
				/**
				 * @brief Метод обратного вызова при подключении к прокси-серверу
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void proxyConnectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при чтении сообщения с прокси-сервера
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void proxyReadEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод вывода полученного списка разрешённых ресурсов для подключения
				 *
				 * @param origin список разрешённых ресурсов для подключения
				 */
				void originCallback(const vector <string> & origin) noexcept;
				/**
				 * @brief Метод вывода полученного альтернативного сервиса от сервера
				 *
				 * @param origin источник альтернативного сервиса
				 * @param field  поле параметров альтернативного сервиса
				 */
				void altsvcCallback(const string & origin, const string & field) noexcept;
			private:
				/**
				 * @brief Метод выполнения активации сессии HTTP/2
				 *
				 * @param bid идентификатор брокера
				 */
				void implementation(const uint64_t bid) noexcept;
			private:
				/**
				 * @brief Метод выполнения препарирования полученных данных
				 *
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    результат препарирования
				 */
				status_t prepareProxy(const int32_t sid, const uint64_t bid) noexcept;
			protected:
				/**
				 * @brief Метод выполнения пинга сервера
				 *
				 * @return результат работы пинга
				 */
				bool ping() noexcept;
			public:
				/**
				 * @brief Метод выполнения закрытия подключения
				 *
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения на сервер
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept;
			public:
				/**
				 * @brief Метод отправки заголовков на сервер
				 *
				 * @param sid     идентификатор потока
				 * @param headers заголовки отправляемые на сервер
				 * @param flag    флаг передаваемого потока по сети
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const int32_t sid, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept;
			public:
				/**
				 * @brief Метод установки флагов настроек модуля
				 *
				 * @param flags список флагов настроек модуля для установки
				 */
				virtual void mode(const std::set <flag_t> & flags) noexcept;
			public:
				/**
				 * @brief Модуль установки настроек протокола HTTP/2
				 *
				 * @param settings список настроек протокола HTTP/2
				 */
				virtual void settings(const std::map <http2_t::settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * @brief Метод установки размера чанка
				 *
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * @brief Метод установки User-Agent для HTTP-запроса
				 *
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
				/**
				 * @brief Метод установки идентификации клиента
				 *
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Web2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Web2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Web2() noexcept {}
		} web2_t;
	};
};

#endif // __AWH_WEB_CLIENT__
