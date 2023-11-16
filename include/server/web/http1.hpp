/**
 * @file: http1.hpp
 * @date: 2023-10-01
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

#ifndef __AWH_WEB_HTTP1_SERVER__
#define __AWH_WEB_HTTP1_SERVER__

/**
 * Наши модули
 */
#include <scheme/web.hpp>
#include <server/web/web.hpp>
#include <server/web/ws1.hpp>

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
		 * Http2 Прототип класса HTTP/2 сервера
		 */
		class Http2;
		/**
		 * Http1 Класс HTTP-сервера
		 */
		typedef class Http1 : public web_t {
			private:
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 сервера
				 */
				friend class Http2;
			private:
				// Флаг разрешения использования протокол WebSocket
				bool _webSocket;
			private:
				// Идентичность протокола
				http_t::identity_t _identity;
			private:
				// Объект работы с WebSocket-сервером
				ws1_t _ws1;
			private:
				// Объект рабочего
				scheme::web_t _scheme;
			private:
				// Список активных агентов
				map <uint64_t, agent_t> _agents;
			private:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении клиента
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Функция обратного вызова при записи сообщение брокеру
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * websocket Метод инициализации WebSocket протокола
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void websocket(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * erase Метод удаления отключившихся брокеров
				 * @param bid идентификатор брокера
				 */
				void erase(const uint64_t bid = 0) noexcept;
			private:
				/**
				 * pinging Метод таймера выполнения пинга клиента
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				void pinging(const uint16_t tid, awh::core_t * core) noexcept;
			public:
				/**
				 * parser Метод извлечения объекта HTTP-парсера
				 * @param bid идентификатор брокера
				 * @return    объект HTTP-парсера
				 */
				const awh::http_t * parser(const uint64_t bid) const noexcept;
			public:
				/**
				 * trailers Метод получения запроса на передачу трейлеров
				 * @param bid идентификатор брокера
				 * @return    флаг запроса клиентом передачи трейлеров
				 */
				bool trailers(const uint64_t bid) const noexcept;
				/**
				 * trailer Метод установки трейлера
				 * @param bid идентификатор брокера
				 * @param key ключ заголовка
				 * @param val значение заголовка
				 */
				void trailer(const uint64_t bid, const string & key, const string & val) noexcept;
			public:
				/**
				 * init Метод инициализации WEB-сервера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compress_t> & compressors = {}) noexcept;
				/**
				 * init Метод инициализации WEB-сервера
				 * @param port        порт сервера
				 * @param host        хост сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const u_int port, const string & host = "", const vector <http_t::compress_t> & compressors = {}) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param bid  идентификатор брокера
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const uint64_t bid, const ws::mess_t & mess) noexcept;
				/**
				 * sendMessage Метод отправки сообщения клиенту
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 */
				void sendMessage(const uint64_t bid, const vector <char> & message, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде клиенту
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 */
				void send(const uint64_t bid, const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * send Метод отправки тела сообщения клиенту
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @param end    флаг последнего сообщения после которого поток закрывается
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept;
				/**
				 * send Метод отправки заголовков клиенту
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param headers заголовки отправляемые клиенту
				 * @param end     размер сообщения в байтах
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const uint64_t bid, const u_int code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept;
				/**
				 * send Метод отправки сообщения брокеру
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void send(const uint64_t bid, const u_int code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const mode_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для извлечения пароля
				 * @param callback функция обратного вызова
				 */
				void on(function <string (const uint64_t, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для обработки авторизации
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие активации брокера на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибок
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибки
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода бинарных данных в сыром виде полученных с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const char *, const size_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функция обратного вызова активности потока
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept;
				/**
				 * on Метод установки функция обратного вызова при выполнении рукопожатия
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const agent_t)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова при завершении запроса
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const direct_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода запроса клиента к серверу
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного тела данных с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученных заголовков с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				u_int port(const uint64_t bid) const noexcept;
				/**
				 * agent Метод извлечения агента клиента
				 * @param bid идентификатор брокера
				 * @return    агент к которому относится подключённый клиент
				 */
				agent_t agent(const uint64_t bid) const noexcept;
				/**
				 * ip Метод получения IP-адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес интернет подключения брокера
				 */
				const string & ip(const uint64_t bid) const noexcept;
				/**
				 * mac Метод получения MAC-адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес устройства брокера
				 */
				const string & mac(const uint64_t bid) const noexcept;
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
				 * close Метод закрытия подключения брокера
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * subprotocol Метод установки поддерживаемого сабпротокола
				 * @param subprotocol сабпротокол для установки
				 */
				void subprotocol(const string & subprotocol) noexcept;
				/**
				 * subprotocols Метод установки списка поддерживаемых сабпротоколов
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const set <string> & subprotocols) noexcept;
				/**
				 * subprotocol Метод получения списка выбранных сабпротоколов
				 * @param bid идентификатор брокера
				 * @return    список выбранных сабпротоколов
				 */
				const set <string> & subprotocols(const uint64_t bid) const noexcept;
			public:
				/**
				 * extensions Метод установки списка расширений
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
				/**
				 * extensions Метод извлечения списка расширений
				 * @param bid идентификатор брокера
				 * @return    список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions(const uint64_t bid) const noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <http_t::compress_t> & compressors) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
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
				void alive(const time_t time) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const server::core_t * core) noexcept;
			public:
				/**
				 * identity Метод установки идентичности протокола модуля
				 * @param identity идентичность протокола модуля
				 */
				void identity(const http_t::identity_t identity) noexcept;
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
				void bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept;
			public:
				/**
				 * crypted Метод получения флага шифрования
				 * @param bid идентификатор брокера
				 * @return    результат проверки
				 */
				bool crypted(const uint64_t bid) const noexcept;
				/**
				 * encrypt Метод активации шифрования для клиента
				 * @param bid  идентификатор брокера
				 * @param mode флаг активации шифрования
				 */
				void encrypt(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * encryption Метод активации шифрования
				 * @param mode флаг активации шифрования
				 */
				void encryption(const bool mode) noexcept;
				/**
				 * encryption Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Http1 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Http1(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Http1 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Http1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http1 Деструктор
				 */
				~Http1() noexcept {}
		} http1_t;
	};
};

#endif // __AWH_WEB_HTTP1_SERVER__
