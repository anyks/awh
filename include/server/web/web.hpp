/**
 * @file: web.hpp
 * @date: 2023-09-27
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_WEB_SERVER__
#define __AWH_WEB_SERVER__

/**
 * Стандартные модули
 */
#include <map>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/uri.hpp>
#include <http/http2.hpp>
#include <http/server.hpp>
#include <core/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server клиентское пространство имён
	 */
	namespace server {
		/**
		 * Web Базовый класс web-сервера
		 */
		typedef class Web {
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
					TAKEOVER_CLIENT       = 0x04, // Флаг ожидания входящих сообщений для клиента
					TAKEOVER_SERVER       = 0x05, // Флаг ожидания входящих сообщений для сервера
					WEBSOCKET_ENABLE      = 0x06, // Флаг разрешения использования Websocket-сервера
					CONNECT_METHOD_ENABLE = 0x07  // Флаг разрешающий метод CONNECT для сервера
				};
			protected:
				/**
				 * Ident Структура идентификации сервиса
				 */
				typedef struct Ident {
					string id;   // Идентификатор сервиса
					string ver;  // Версия сервиса
					string name; // Название сервиса
					/**
					 * Ident Конструктор
					 */
					Ident() noexcept : id{AWH_SHORT_NAME}, ver{AWH_VERSION}, name{AWH_NAME} {}
				} ident_t;
				/**
				 * Encryption Структура параметров шифрования
				 */
				typedef struct Encryption {
					bool mode;               // Флаг активности механизма шифрования
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					/**
					 * Encryption Конструктор
					 */
					Encryption() noexcept : mode(false), pass{""}, salt{""}, cipher(hash_t::cipher_t::AES128) {}
				} encryption_t;
				/**
				 * Service Структура сервиса
				 */
				typedef struct Service {
					bool alive;          // Флаг долгоживущего подключения
					u_int port;          // Порт сервера
					string host;         // Хости сервера
					string realm;        // Название сервера
					string opaque;       // Временный ключ сессии сервера
					auth_t::hash_t hash; // Алгоритм шифрования для Digest авторизации
					auth_t::type_t type; // Тип авторизации
					/**
					 * Service Конструктор
					 */
					Service() noexcept :
					 alive(false), port(SERVER_PORT),
					 host{""}, realm{""}, opaque{""},
					 hash(auth_t::hash_t::MD5),
					 type(auth_t::type_t::NONE) {}
				} service_t;
			protected:
				// Идентификатор основного процесса
				pid_t _pid;
			protected:
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект идентификации сервиса
				ident_t _ident;
				// Хранилище функций обратного вызова
				fn_t _callbacks;
				// Объект параметров сервиса
				service_t _service;
				// Объект параметров шифрования
				encryption_t _encryption;
			private:
				// Ядро для локального таймера
				timer_t _timer;
			protected:
				// Флаг остановки работы базы событий
				bool _complete;
			protected:
				// Максимальный интервал времени жизни подключения
				time_t _timeAlive;
				// Размер одного чанка
				size_t _chunkSize;
				// Максимальное количество запросов
				size_t _maxRequests;
			protected:
				// Список мусорных брокеров
				map <uint64_t, time_t> _disconected;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const server::core_t * _core;
			protected:
				/**
				 * openEvents Метод обратного вызова при запуске работы
				 * @param sid идентификатор схемы сети
				 */
				void openEvents(const uint16_t sid) noexcept;
				/**
				 * statusEvents Метод обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 */
				virtual void statusEvents(const awh::core_t::status_t status) noexcept;
			protected:
				/**
				 * connectEvents Метод обратного вызова при подключении к серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void connectEvents(const uint64_t bid, const uint16_t sid) noexcept = 0;
				/**
				 * disconnectEvents Метод обратного вызова при отключении клиента
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void disconnectEvents(const uint64_t bid, const uint16_t sid) noexcept = 0;
			protected:
				/**
				 * readEvents Метод обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				virtual void readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept = 0;
				/**
				 * writeEvents Метод обратного вызова при записи сообщение брокеру
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				virtual void writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept = 0;
			protected:
				/**
				 * acceptEvents Метод обратного вызова при проверке подключения брокера
				 * @param ip   адрес интернет подключения брокера
				 * @param mac  мак-адрес подключившегося брокера
				 * @param port порт подключившегося брокера
				 * @param sid  идентификатор схемы сети
				 * @return     результат разрешения к подключению брокера
				 */
				bool acceptEvents(const string & ip, const string & mac, const u_int port, const uint16_t sid) noexcept;
			protected:
				/**
				 * chunking Метод обработки получения чанков
				 * @param bid   идентификатор брокера
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				virtual void chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept;
			protected:
				/**
				 * callbacksEvents Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param idw   идентификатор функции обратного вызова
				 * @param name  название функции обратного вызова
				 * @param dump  дамп данных функции обратного вызова
				 */
				virtual void callbacksEvents(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept;
			private:
				/**
				 * clusterEvents Метод вывода статуса кластера
				 * @param family флаг семейства кластера
				 * @param sid    идентификатор схемы сети
				 * @param pid    идентификатор процесса
				 * @param event  идентификатор события
				 */
				void clusterEvents(const cluster_t::family_t family, const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept;
			protected:
				/**
				 * erase Метод удаления отключившихся брокеров
				 * @param bid идентификатор брокера
				 */
				virtual void erase(const uint64_t bid = 0) noexcept;
				/**
				 * disconnect Метод отключения брокера
				 * @param bid идентификатор брокера
				 */
				virtual void disconnect(const uint64_t bid) noexcept;
				/**
				 * disconected Метод удаления отключившихся брокеров
				 * @param tid идентификатор таймера
				 */
				void disconected(const uint16_t tid) noexcept;
			protected:
				/**
				 * pinging Метод таймера выполнения пинга клиента
				 * @param tid идентификатор таймера
				 */
				virtual void pinging(const uint16_t tid) noexcept = 0;
			public:
				/**
				 * init Метод инициализации WEB брокера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				virtual void init(const string & socket, const vector <http_t::compressor_t> & compressors = {}) noexcept;
				/**
				 * init Метод инициализации WEB брокера
				 * @param port        порт сервера
				 * @param host        хост сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				virtual void init(const u_int port, const string & host = "", const vector <http_t::compressor_t> & compressors = {}) noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				virtual void callbacks(const fn_t & callbacks) noexcept;
			public:
				/**
				 * callback Шаблон метода установки финкции обратного вызова
				 * @tparam A тип функции обратного вызова
				 */
				template <typename A>
				/**
				 * callback Метод установки функции обратного вызова
				 * @param idw идентификатор функции обратного вызова
				 * @param fn  функция обратного вызова для установки
				 */
				void callback(const uint64_t idw, function <A> fn) noexcept {
					// Если функция обратного вызова передана
					if((idw > 0) && (fn != nullptr))
						// Выполняем установку функции обратного вызова
						this->_callbacks.set <A> (idw, fn);
				}
				/**
				 * callback Шаблон метода установки финкции обратного вызова
				 * @tparam A тип функции обратного вызова
				 */
				template <typename A>
				/**
				 * callback Метод установки функции обратного вызова
				 * @param name название функции обратного вызова
				 * @param fn   функция обратного вызова для установки
				 */
				void callback(const string & name, function <A> fn) noexcept {
					// Если функция обратного вызова передана
					if(!name.empty() && (fn != nullptr))
						// Выполняем установку функции обратного вызова
						this->_callbacks.set <A> (name, fn);
				}
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				virtual u_int port(const uint64_t bid) const noexcept = 0;
				/**
				 * agent Метод извлечения агента клиента
				 * @param bid идентификатор брокера
				 * @return    агент к которому относится подключённый клиент
				 */
				virtual agent_t agent(const uint64_t bid) const noexcept = 0;
				/**
				 * ip Метод получения IP адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес интернет подключения брокера
				 */
				virtual const string & ip(const uint64_t bid) const noexcept = 0;
				/**
				 * mac Метод получения MAC адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес устройства брокера
				 */
				virtual const string & mac(const uint64_t bid) const noexcept = 0;
			public:
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				virtual void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки времени жизни подключения
				 * @param time время жизни подключения
				 */
				virtual void alive(const time_t time) noexcept;
			public:
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				virtual void core(const server::core_t * core) noexcept;
			public:
				/**
				 * stop Метод остановки сервера
				 */
				virtual void stop() noexcept;
				/**
				 * start Метод запуска сервера
				 */
				virtual void start() noexcept;
			public:
				/**
				 * close Метод закрытия подключения брокера
				 * @param bid идентификатор брокера
				 */
				virtual void close(const uint64_t bid) noexcept = 0;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				virtual void mode(const set <flag_t> & flags) noexcept = 0;
			public:
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				virtual void waitTimeDetect(const time_t read, const time_t write) noexcept = 0;
				/**
				 * bytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				virtual void bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept = 0;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				virtual void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				virtual void opaque(const string & opaque) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				virtual void chunk(const size_t size) noexcept;
				/**
				 * maxRequests Метод установки максимального количества запросов
				 * @param max максимальное количество запросов
				 */
				virtual void maxRequests(const size_t max) noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				virtual void total(const u_short total) noexcept = 0;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				virtual void keepAlive(const int cnt, const int idle, const int intvl) noexcept = 0;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param compressors список поддерживаемых компрессоров
				 */
				virtual void compressors(const vector <http_t::compressor_t> & compressors) noexcept = 0;
			public:
				/**
				 * ident Метод установки идентификации сервера
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				virtual void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				virtual void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * encryption Метод активации шифрования
				 * @param mode флаг активации шифрования
				 */
				virtual void encryption(const bool mode) noexcept;
				/**
				 * encryption Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				virtual void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Web Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Web(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Web Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Web(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Web Деструктор
				 */
				virtual ~Web() noexcept {}
		} web_t;
		/**
		 * Web2 Базовый класс web2-сервера
		 */
		typedef class Web2 : public web_t {
			protected:
				// Список доступных источников для подключения HTTP/2
				vector <string> _origins;
			protected:
				// Список отправляемых альтернативных сервисов HTTP/2
				unordered_multimap <string, string> _altsvc;
			protected:
				// Список параметров настроек протокола HTTP/2
				map <http2_t::settings_t, uint32_t> _settings;
			protected:
				// Список активных сессий HTTP/2
				map <uint64_t, unique_ptr <http2_t>> _sessions;
			protected:
				/**
				 * statusEvents Метод обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 */
				void statusEvents(const awh::core_t::status_t status) noexcept;
				/**
				 * connectEvents Метод обратного вызова при подключении к серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				virtual void connectEvents(const uint64_t bid, const uint16_t sid) noexcept;
			protected:
				/**
				 * sendSignal Метод обратного вызова при отправки данных HTTP/2
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера данных для отправки
				 */
				void sendSignal(const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept;
			protected:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    статус полученных данных
				 */
				virtual int beginSignal(const int32_t sid, const uint64_t bid) noexcept = 0;
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param bid   идентификатор брокера
				 * @param error флаг ошибки если присутствует
				 * @return      статус полученных данных
				 */
				virtual int closedSignal(const int32_t sid, const uint64_t bid, const http2_t::error_t error) noexcept = 0;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				virtual int headerSignal(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept = 0;
				/**
				 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				virtual int chunkSignal(const int32_t sid, const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept = 0;
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param direct направление передачи фрейма
				 * @param frame  тип полученного фрейма
				 * @param flags  флаги полученного фрейма
				 * @return       статус полученных данных
				 */
				virtual int frameSignal(const int32_t sid, const uint64_t bid, const http2_t::direct_t direct, const http2_t::frame_t frame, const set <http2_t::flag_t> & flags) noexcept = 0;
			public:
				/**
				 * close Метод выполнения закрытия подключения
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			protected:
				/**
				 * ping Метод выполнения пинга клиента
				 * @param bid идентификатор брокера
				 * @return    результат работы пинга
				 */
				bool ping(const uint64_t bid) noexcept;
			public:
				/**
				 * shutdown Метод отправки клиенту сообщения корректного завершения
				 * @param bid идентификатор брокера
				 * @return    результат выполнения операции
				 */
				bool shutdown(const uint64_t bid) noexcept;
			public:
				/**
				 * reject Метод выполнения сброса подключения
				 * @param sid   идентификатор потока
				 * @param bid   идентификатор брокера
				 * @param error код отправляемой ошибки
				 * @return      результат отправки сообщения
				 */
				bool reject(const int32_t sid, const uint64_t bid, http2_t::error_t error) noexcept;
			public:
				/**
				 * goaway Метод отправки сообщения закрытия всех потоков
				 * @param last   идентификатор последнего потока
				 * @param bid    идентификатор брокера
				 * @param error  код отправляемой ошибки
				 * @param buffer буфер отправляемых данных если требуется
				 * @param size   размер отправляемого буфера данных
				 * @return       результат отправки данных фрейма
				 */
				bool goaway(const int32_t last, const uint64_t bid, const http2_t::error_t error, const uint8_t * buffer = nullptr, const size_t size = 0) noexcept;
			public:
				/**
				 * send Метод отправки трейлеров
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @return        результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept;
				/**
				 * send Метод отправки сообщения клиенту
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept;
			public:
				/**
				 * send Метод отправки заголовков
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @param flag    флаг передаваемого потока по сети
				 * @return        флаг последнего сообщения после которого поток закрывается
				 */
				int32_t send(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept;
			public:
				/**
				 * push Метод отправки пуш-уведомлений
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @param flag    флаг передаваемого потока по сети
				 * @return        флаг последнего сообщения после которого поток закрывается
				 */
				int32_t push(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept;
			public:
				/**
				 * addOrigin Метод добавления разрешённого источника
				 * @param origin разрешённый источнико
				 */
				void addOrigin(const string & origin) noexcept;
				/**
				 * setOrigin Метод установки списка разрешённых источников
				 * @param origins список разрешённых источников
				 */
				void setOrigin(const vector <string> & origins) noexcept;
			public:
				/**
				 * addAltSvc Метод добавления альтернативного сервиса
				 * @param origin название альтернативного сервиса
				 * @param field  поле альтернативного сервиса
				 */
				void addAltSvc(const string & origin, const string & field) noexcept;
				/**
				 * setAltSvc Метод установки списка альтернативных сервисов
				 * @param origins список альтернативных сервисов
				 */
				void setAltSvc(const unordered_multimap <string, string> & origins) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const map <http2_t::settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * Web2 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Web2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Web2 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Web2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Web2 Деструктор
				 */
				virtual ~Web2() noexcept {}
		} web2_t;
	};
};

#endif // __AWH_WEB_SERVER__
