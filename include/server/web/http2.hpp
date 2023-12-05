/**
 * @file: http2.hpp
 * @date: 2023-10-12
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

#ifndef __AWH_WEB_HTTP2_SERVER__
#define __AWH_WEB_HTTP2_SERVER__

/**
 * Наши модули
 */
#include <scheme/web2.hpp>
#include <server/web/web.hpp>
#include <server/web/ws2.hpp>
#include <server/web/http1.hpp>

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
		 * Http2 Класс HTTP-сервера
		 */
		typedef class Http2 : public web2_t {
			private:
				// Флаг разрешения использования протокол Websocket
				bool _webSocket;
			private:
				// Идентичность протокола
				http_t::identity_t _identity;
			private:
				// Объект для работы c Websocket
				ws2_t _ws2;
				// Объект для работы с HTTP/1.1
				http1_t _http1;
			private:
				// Объект рабочего
				scheme::web2_t _scheme;
			private:
				// Объект партнёра клиента
				scheme::ws_t::partner_t _client;
				// Объект партнёра сервера
				scheme::ws_t::partner_t _server;
			private:
				// Поддерживаемые сабпротоколы
				set <string> _subprotocols;
			private:
				// Список активных агентов
				map <uint64_t, agent_t> _agents;
			private:
				// Список поддверживаемых расширений
				vector <vector <string>> _extensions;
			private:
				// Полученные HTTP заголовки
				unordered_multimap <string, string> _headers;
			private:
				/**
				 * connectEvents Метод обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectEvents(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectEvents Метод обратного вызова при отключении клиента
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectEvents(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * readEvents Метод обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * writeEvents Метод обратного вызова при записи сообщение брокеру
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * callbacksEvents Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param idw   идентификатор функции обратного вызова
				 * @param name  название функции обратного вызова
				 * @param dump  дамп данных функции обратного вызова
				 */
				void callbacksEvents(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept;
			private:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    статус полученных данных
				 */
				int beginSignal(const int32_t sid, const uint64_t bid) noexcept;
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param bid   идентификатор брокера
				 * @param error флаг ошибки если присутствует
				 * @return      статус полученных данных
				 */
				int closedSignal(const int32_t sid, const uint64_t bid, const awh::http2_t::error_t error) noexcept;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int headerSignal(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept;
				/**
				 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int chunkSignal(const int32_t sid, const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param direct направление передачи фрейма
				 * @param frame  тип полученного фрейма
				 * @param flags  флаги полученного фрейма
				 * @return       статус полученных данных
				 */
				int frameSignal(const int32_t sid, const uint64_t bid, const awh::http2_t::direct_t direct, const awh::http2_t::frame_t frame, const set <awh::http2_t::flag_t> & flags) noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param sid  идентификатор потока
				 * @param bid  идентификатор брокера
				 * @param core объект сетевого ядра
				 */
				void prepare(const int32_t sid, const uint64_t bid, server::core_t * core) noexcept;
				/**
				 * websocket Метод инициализации Websocket протокола
				 * @param sid  идентификатор потока
				 * @param bid  идентификатор брокера
				 * @param core объект сетевого ядра
				 */
				void websocket(const int32_t sid, const uint64_t bid, server::core_t * core) noexcept;
			private:
				/**
				 * erase Метод удаления отключившихся брокеров
				 * @param bid идентификатор брокера
				 */
				void erase(const uint64_t bid = 0) noexcept;
				/**
				 * disconnect Метод отключения брокера
				 * @param bid идентификатор брокера
				 */
				void disconnect(const uint64_t bid) noexcept;
			private:
				/**
				 * pinging Метод таймера выполнения пинга клиента
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				void pinging(const uint16_t tid, awh::core_t * core) noexcept;
			public:
				/**
				 * proto Метод извлечения поддерживаемого протокола подключения
				 * @param bid идентификатор брокера
				 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto(const uint64_t bid) const noexcept;
			public:
				/**
				 * parser Метод извлечения объекта HTTP-парсера
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    объект HTTP-парсера
				 */
				const awh::http_t * parser(const int32_t sid, const uint64_t bid) const noexcept;
			public:
				/**
				 * trailers Метод получения запроса на передачу трейлеров
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    флаг запроса клиентом передачи трейлеров
				 */
				bool trailers(const int32_t sid, const uint64_t bid) const noexcept;
				/**
				 * trailer Метод установки трейлера
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @param key ключ заголовка
				 * @param val значение заголовка
				 */
				void trailer(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept;
			public:
				/**
				 * init Метод инициализации Websocket-сервера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compress_t> & compressors = {}) noexcept;
				/**
				 * init Метод инициализации Websocket-сервера
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
				 * send Метод отправки трейлеров
				 * @param sid     идентификатор потока HTTP/2
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @return        результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept;
			public:
				/**
				 * send Метод отправки тела сообщения клиенту
				 * @param sid    идентификатор потока HTTP
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @param end    флаг последнего сообщения после которого поток закрывается
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept;
				/**
				 * send Метод отправки заголовков клиенту
				 * @param sid     идентификатор потока HTTP
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param headers заголовки отправляемые клиенту
				 * @param end     размер сообщения в байтах
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const int32_t sid, const uint64_t bid, const u_int code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept;
				/**
				 * send Метод отправки сообщения брокеру
				 * @param sid     идентификатор потока HTTP
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void send(const int32_t sid, const uint64_t bid, const u_int code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * shutdown2 Метод HTTP/2 отправки клиенту сообщения корректного завершения
				 * @param bid идентификатор брокера
				 * @return    результат выполнения операции
				 */
				bool shutdown2(const uint64_t bid) noexcept;
			public:
				/**
				 * reject2 Метод HTTP/2 выполнения сброса подключения
				 * @param sid   идентификатор потока
				 * @param bid   идентификатор брокера
				 * @param error код отправляемой ошибки
				 * @return      результат отправки сообщения
				 */
				bool reject2(const int32_t sid, const uint64_t bid, const awh::http2_t::error_t error) noexcept;
			public:
				/**
				 * goaway2 Метод HTTP/2 отправки сообщения закрытия всех потоков
				 * @param last   идентификатор последнего потока
				 * @param bid    идентификатор брокера
				 * @param error  код отправляемой ошибки
				 * @param buffer буфер отправляемых данных если требуется
				 * @param size   размер отправляемого буфера данных
				 * @return       результат отправки данных фрейма
				 */
				bool goaway2(const int32_t last, const uint64_t bid, const awh::http2_t::error_t error, const uint8_t * buffer = nullptr, const size_t size = 0) noexcept;
			public:
				/**
				 * send2 Метод HTTP/2 отправки трейлеров
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @return        результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept;
				/**
				 * send2 Метод HTTP/2 отправки сообщения клиенту
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept;
				/**
				 * send2 Метод HTTP/2 отправки заголовков
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @param flag    флаг передаваемого потока по сети
				 * @return        флаг последнего сообщения после которого поток закрывается
				 */
				int32_t send2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept;
			public:
				/**
				 * push2 Метод HTTP/2 отправки push-уведомлений
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @param flag    флаг передаваемого потока по сети
				 * @return        флаг последнего сообщения после которого поток закрывается
				 */
				int32_t push2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
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
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * maxRequests Метод установки максимального количества запросов
				 * @param max максимальное количество запросов
				 */
				void maxRequests(const size_t max) noexcept;
			public:
				/**
				 * ident Метод установки идентификации сервера
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * crypted Метод получения флага шифрования
				 * @param sid идентификатор потока HTTP
				 * @param bid идентификатор брокера
				 * @return    результат проверки
				 */
				bool crypted(const int32_t sid, const uint64_t bid) const noexcept;
				/**
				 * encrypt Метод активации шифрования для клиента
				 * @param sid  идентификатор потока HTTP
				 * @param bid  идентификатор брокера
				 * @param mode флаг активации шифрования
				 */
				void encrypt(const int32_t sid, const uint64_t bid, const bool mode) noexcept;
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
				 * Http2 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Http2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Http2 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Http2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http2 Деструктор
				 */
				~Http2() noexcept;
		} http2_t;
	};
};

#endif // __AWH_WEB_HTTP2_SERVER__
