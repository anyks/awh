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
 * Стандартная библиотека
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
#include <http/server.hpp>
#include <core/server.hpp>
#include <http/nghttp2.hpp>

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
				 * Идентификатор агента
				 */
				enum class agent_t : uint8_t {
					HTTP      = 0x01, // HTTP-клиент
					WEBSOCKET = 0x02  // WebSocket-клиент
				};
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
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE           = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO        = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP        = 0x03, // Флаг запрета остановки биндинга
					WAIT_MESS       = 0x04, // Флаг ожидания входящих сообщений
					VERIFY_SSL      = 0x05, // Флаг выполнения проверки сертификата SSL
					TAKEOVER_CLIENT = 0x06, // Флаг ожидания входящих сообщений для клиента
					TAKEOVER_SERVER = 0x07  // Флаг ожидания входящих сообщений для сервера
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
					Ident() noexcept : id{""}, ver{""}, name{""} {}
				} ident_t;
				/**
				 * Crypto Структура параметров шифрования
				 */
				typedef struct Crypto {
					bool mode;               // Флаг активности механизма шифрования
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					/**
					 * Crypto Конструктор
					 */
					Crypto() noexcept : mode(false), pass{""}, salt{""}, cipher(hash_t::cipher_t::AES128) {}
				} crypto_t;
				/**
				 * Service Структура сервиса
				 */
				typedef struct Service {
					// Флаг долгоживущего подключения
					bool alive;
					// Порт сервера
					u_int port;
					// Хости сервера
					string host;
					// Название сервера
					string realm;
					// Временный ключ сессии сервера
					string opaque;
					/**
					 * Service Конструктор
					 */
					Service() noexcept : alive(false), port(SERVER_PORT), host{""}, realm{""}, opaque{""} {}
				} service_t;
			protected:
				// Идентификатор основного процесса
				pid_t _pid;
			protected:
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект идентификации сервиса
				ident_t _ident;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект параметров шифрования
				crypto_t _crypto;
				// Объект параметров сервиса
				service_t _service;
			protected:
				// Ядро локальных таймеров
				awh::core_t _timer;
			protected:
				// Алгоритм шифрования для Digest авторизации
				auth_t::hash_t _authHash;
				// Тип авторизации
				auth_t::type_t _authType;
			protected:
				// Выполнять анбиндинг после завершения запроса
				bool _unbind;
			protected:
				// Максимальный интервал времени жизни подключения
				time_t _timeAlive;
				// Размер одного чанка
				size_t _chunkSize;
				// Максимальное количество запросов
				size_t _maxRequests;
			protected:
				// Список мусорных адъютантов
				map <uint64_t, time_t> _garbage;
			protected:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const server::core_t * _core;
			protected:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openCallback(const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				virtual void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
			protected:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * disconnectCallback Метод обратного вызова при отключении клиента
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				virtual void readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * writeCallback Функция обратного вызова при записи сообщение адъютанту
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				virtual void writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept = 0;
			protected:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept = 0;
			protected:
				/**
				 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
				 * @param ip   адрес интернет подключения адъютанта
				 * @param mac  мак-адрес подключившегося адъютанта
				 * @param port порт подключившегося адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат разрешения к подключению адъютанта
				 */
				bool acceptCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept;
			protected:
				/**
				 * chunking Метод обработки получения чанков
				 * @param aid   идентификатор адъютанта
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				virtual void chunking(const uint64_t aid, const vector <char> & chunk, const awh::http_t * http) noexcept;
			protected:
				/**
				 * garbage Метод удаления мусорных адъютантов
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				virtual void garbage(const u_short tid, awh::core_t * core) noexcept;
			public:
				/**
				 * init Метод инициализации WEB адъютанта
				 * @param socket   unix-сокет для биндинга
				 * @param compress метод сжатия передаваемых сообщений
				 */
				virtual void init(const string & socket, const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
				/**
				 * init Метод инициализации WEB адъютанта
				 * @param port     порт сервера
				 * @param host     хост сервера
				 * @param compress метод сжатия передаваемых сообщений
				 */
				virtual void init(const u_int port, const string & host = "", const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const uint64_t, const mode_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для извлечения пароля
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <string (const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для обработки авторизации
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <bool (const string &, const string &, const u_int)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибки
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функция обратного вызова при полном получении запроса клиента
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t)> callback) noexcept;
				/**
				 * on Метод установки функция обратного вызова активности потока
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с клиента
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода запроса клиента к серверу
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного тела данных с клиента
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученных заголовков с клиента
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * port Метод получения порта подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    порт подключения адъютанта
				 */
				virtual u_int port(const uint64_t aid) const noexcept = 0;
				/**
				 * ip Метод получения IP адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес интернет подключения адъютанта
				 */
				virtual const string & ip(const uint64_t aid) const noexcept = 0;
				/**
				 * mac Метод получения MAC адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес устройства адъютанта
				 */
				virtual const string & mac(const uint64_t aid) const noexcept = 0;
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
				 * close Метод закрытия подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				virtual void close(const uint64_t aid) noexcept = 0;
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
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				virtual void clusterAutoRestart(const bool mode) noexcept = 0;
				/**
				 * compress Метод установки метода сжатия
				 * @param метод сжатия сообщений
				 */
				virtual void compress(const http_t::compress_t compress) noexcept = 0;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				virtual void keepAlive(const int cnt, const int idle, const int intvl) noexcept = 0;
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
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				virtual void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
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
			public:
				// Количество потоков по умолчанию
				static constexpr uint32_t CONCURRENT_STREAMS = 128;
				// Максимальный размер таблицы заголовков по умолчанию
				static constexpr uint32_t HEADER_TABLE_SIZE = 4096;
				// Минимальный размер фрейма по умолчанию
				static constexpr uint32_t MAX_FRAME_SIZE_MIN = 16384;
				// Максимальный размер фрейма по умолчанию
				static constexpr uint32_t MAX_FRAME_SIZE_MAX = 16777215;
				// Максимальный размер окна по умолчанию
				static constexpr uint32_t MAX_WINDOW_SIZE = 2147483647;
			public:
				/**
				 * Параметры настроек HTTP/2
				 */
				enum class settings_t : uint8_t {
					NONE              = 0x00, // Настройки не установлены
					STREAMS           = 0x01, // Максимальное количество потоков
					FRAME_SIZE        = 0x02, // Максимальный размер фрейма
					ENABLE_PUSH       = 0x03, // Разрешение присылать пуш-уведомления
					WINDOW_SIZE       = 0x04, // Максимальный размер окна полезной нагрузки
					HEADER_TABLE_SIZE = 0x05  // Максимальный размер таблицы заголовков
				};
			protected:
				// Список доступных источников
				vector <string> _origins;
			private:
				// Список параметров настроек протокола HTTP/2
				map <settings_t, uint32_t> _settings;
			protected:
				// Список активных сессий HTTP/2
				map <uint64_t, unique_ptr <nghttp2_t>> _sessions;
			protected:
				/**
				 * sendSignal Метод обратного вызова при отправки данных HTTP/2
				 * @param aid    идентификатор адъютанта
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера данных для отправки
				 */
				void sendSignal(const uint64_t aid, const uint8_t * buffer, const size_t size) noexcept;
			protected:
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
				 * @param sid   идентификатор потока
				 * @param aid   идентификатор адъютанта
				 * @param type  тип полученного фрейма
				 * @param flags флаг полученного фрейма
				 * @return      статус полученных данных
				 */
				virtual int frameSignal(const int32_t sid, const uint64_t aid, const uint8_t type, const uint8_t flags) noexcept = 0;
			protected:
				/**
				 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
				 * @param sid    идентификатор потока
				 * @param aid    идентификатор адъютанта
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				virtual int chunkSignal(const int32_t sid, const uint64_t aid, const uint8_t * buffer, const size_t size) noexcept = 0;
			protected:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2
				 * @param sid идентификатор потока
				 * @param aid идентификатор адъютанта
				 * @return    статус полученных данных
				 */
				virtual int beginSignal(const int32_t sid, const uint64_t aid) noexcept = 0;
			protected:
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param aid   идентификатор адъютанта
				 * @param error флаг ошибки HTTP/2 если присутствует
				 * @return      статус полученных данных
				 */
				virtual int closedSignal(const int32_t sid, const uint64_t aid, const uint32_t error) noexcept = 0;
			protected:
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
				 * @param sid идентификатор потока
				 * @param aid идентификатор адъютанта
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				virtual int headerSignal(const int32_t sid, const uint64_t aid, const string & key, const string & val) noexcept = 0;
			protected:
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			protected:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			protected:
				/**
				 * ping Метод выполнения пинга клиента
				 * @param aid идентификатор адъютанта
				 * @return    результат работы пинга
				 */
				bool ping(const uint64_t aid) noexcept;
			public:
				/**
				 * send Метод отправки сообщения клиенту
				 * @param id      идентификатор потока HTTP/2
				 * @param aid     идентификатор адъютанта
				 * @param message сообщение передаваемое клиенту
				 * @param size    размер сообщения в байтах
				 * @param end     флаг последнего сообщения после которого поток закрывается
				 */
				void send(const int32_t id, const uint64_t aid, const char * message, const size_t size, const bool end) noexcept;
			public:
				/**
				 * setOrigin Метод установки списка разрешенных источников
				 * @param origins список разрешённых источников
				 */
				void setOrigin(const vector <string> & origins) noexcept;
				/**
				 * sendOrigin Метод отправки списка разрешенных источников
				 * @param aid     идентификатор адъютанта
				 * @param origins список разрешённых источников
				 */
				void sendOrigin(const uint64_t aid, const vector <string> & origins) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const map <settings_t, uint32_t> & settings = {}) noexcept;
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