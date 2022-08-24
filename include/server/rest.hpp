/**
 * @file: rest.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_REST_SERVER__
#define __AWH_REST_SERVER__

/**
 * Наши модули
 */
#include <core/server.hpp>
#include <worker/rest.hpp>
#include <sys/threadpool.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Rest Класс работы с REST сервером
		 */
		typedef class Rest {
			public:
				/**
				 * Режим работы адъютанта
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Метод подключения адъютанта
					DISCONNECT = 0x02  // Метод отключения адъютанта
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOINFO   = 0x01, // Флаг запрещающий вывод информационных сообщений
					WAITMESS = 0x02  // Флаг ожидания входящих сообщений
				};
			private:
				/**
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Функция обратного вызова для извлечения пароля
					function <string (const string &)> extractPass;
					// Функция обратного вызова для обработки авторизации
					function <bool (const string &, const string &)> checkAuth;
					// Функция обратного вызова, при запуске или остановки подключения к серверу
					function <void (const size_t, const mode_t, Rest *)> active;
					// Функция обратного вызова, при получении сообщения с сервера
					function <void (const size_t, const awh::http_t *, Rest *)> message;
					// Функция обратного вызова, при получении HTTP чанков от адъютанта
					function <void (const vector <char> &, const awh::http_t *)> chunking;
					// Функция разрешения подключения адъютанта на сервере
					function <bool (const string &, const string &, const u_int, Rest *)> accept;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept : extractPass(nullptr), checkAuth(nullptr), active(nullptr), message(nullptr), chunking(nullptr), accept(nullptr) {}
				} fn_t;
			private:
				// Идентификатор основного процесса
				pid_t _pid;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
			private:
				// Объект для работы с сетью
				network_t _nwk;
			private:
				// Объект работы с URI ссылками
				uri_t _uri;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего
				rest_worker_t _worker;
			private:
				// Идентификатор сервера
				string _sid;
				// Версия сервера
				string _ver;
				// Название сервера
				string _name;
			private:
				// Название сервера
				string _realm;
				// Временный ключ сессии сервера
				string _opaque;
			private:
				// Пароль шифрования передаваемых данных
				string _pass;
				// Соль шифрования передаваемых данных
				string _salt;
				// Размер шифрования передаваемых данных
				hash_t::cipher_t _cipher;
			private:
				// Алгоритм шифрования для Digest авторизации
				auth_t::hash_t _authHash;
				// Тип авторизации
				auth_t::type_t _authType;
			private:
				// Флаг шифрования сообщений
				bool _crypt;
				// Флаг долгоживущего подключения
				bool _alive;
			private:
				// Размер одного чанка
				size_t _chunkSize;
				// Максимальный интервал времени жизни подключения
				size_t _timeAlive;
				// Максимальное количество запросов
				size_t _maxRequests;
			private:
				// Количество активных потоков
				size_t _threadsCount;
				// Флаг активации работы тредпула
				bool _threadsEnabled;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект биндинга TCP/IP
				const server::core_t * _core;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * openCallback Функция обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void openCallback(const size_t wid, awh::core_t * core) noexcept;
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void persistCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * connectCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void connectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * readCallback Функция обратного вызова при чтении сообщения с адъютанта
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Функция обратного вызова при записи сообщение адъютанту
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
				 * @param ip   адрес интернет подключения адъютанта
				 * @param mac  мак-адрес подключившегося адъютанта
				 * @param port порт подключившегося адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @return     результат разрешения к подключению адъютанта
				 */
				bool acceptCallback(const string & ip, const string & mac, const u_int port, const size_t wid, awh::core_t * core) noexcept;
			private:
				/**
				 * handler Метод управления входящими методами
				 * @param aid идентификатор адъютанта
				 */
				void handler(const size_t aid) noexcept;
			private:
				/**
				 * actionRead Метод обработки экшена чтения с сервера
				 * @param aid идентификатор адъютанта
				 */
				void actionRead(const size_t aid) noexcept;
				/**
				 * actionConnect Метод обработки экшена подключения к серверу
				 * @param aid идентификатор адъютанта
				 */
				void actionConnect(const size_t aid) noexcept;
				/**
				 * actionDisconnect Метод обработки экшена отключения от сервера
				 * @param aid идентификатор адъютанта
				 */
				void actionDisconnect(const size_t aid) noexcept;
			public:
				/**
				 * init Метод инициализации Rest адъютанта
				 * @param socket   unix socket для биндинга
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const string & socket, const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
				/**
				 * init Метод инициализации Rest адъютанта
				 * @param port     порт сервера
				 * @param host     хост сервера
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const u_int port, const string & host = "", const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const mode_t, Rest *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const awh::http_t *, Rest *)> callback) noexcept;
			public:
				/**
				 * on Метод добавления функции извлечения пароля
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void on(function <string (const string &)> callback) noexcept;
				/**
				 * on Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void on(function <bool (const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для получения чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int, Rest *)> callback) noexcept;
			public:
				/**
				 * reject Метод отправки сообщения об ошибке
				 * @param aid     идентификатор адъютанта
				 * @param code    код сообщения для адъютанта
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void reject(const size_t aid, const u_int code, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) const noexcept;
				/**
				 * response Метод отправки сообщения адъютанту
				 * @param aid     идентификатор адъютанта
				 * @param code    код сообщения для адъютанта
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void response(const size_t aid, const u_int code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) const noexcept;
			public:
				/**
				 * port Метод получения порта подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    порт подключения адъютанта
				 */
				u_int port(const size_t aid) const noexcept;
				/**
				 * ip Метод получения IP адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес интернет подключения адъютанта
				 */
				const string & ip(const size_t aid) const noexcept;
				/**
				 * mac Метод получения MAC адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес устройства адъютанта
				 */
				const string & mac(const size_t aid) const noexcept;
			public:
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки времени жизни подключения
				 * @param time время жизни подключения
				 */
				void alive(const size_t time) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param aid  идентификатор адъютанта
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const size_t aid, const bool mode) noexcept;
			public:
				/**
				 * stop Метод остановки сервера
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска сервера
				 */
				void start() noexcept;
			public:
				/**
				 * close Метод закрытия подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param threads количество потоков для активации
				 * @param mode    флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const size_t threads = 0, const bool mode = true) noexcept;
			public:
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const time_t read, const time_t write) noexcept;
				/**
				 * bytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void bytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * mode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void mode(const u_short flag) noexcept;
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
				/**
				 * chunkSize Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunkSize(const size_t size) noexcept;
				/**
				 * maxRequests Метод установки максимального количества запросов
				 * @param max максимальное количество запросов
				 */
				void maxRequests(const size_t max) noexcept;
				/**
				 * compress Метод установки метода сжатия
				 * @param метод сжатия сообщений
				 */
				void compress(const http_t::compress_t compress) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
				/**
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void serv(const string & id, const string & name, const string & ver) noexcept;
				/**
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Rest Конструктор
				 * @param core объект биндинга TCP/IP
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Rest(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Rest Деструктор
				 */
				~Rest() noexcept {}
		} rest_t;
	};
};

#endif // __AWH_REST_SERVER__
