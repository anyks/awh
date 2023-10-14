/**
 * @file: ws2.hpp
 * @date: 2023-10-03
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

#ifndef __AWH_WEB_WS2_SERVER__
#define __AWH_WEB_WS2_SERVER__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <scheme/ws.hpp>
#include <sys/threadpool.hpp>
#include <server/web/ws1.hpp>
#include <server/web/web.hpp>

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
		 * WebSocket2 Класс WebSocket-сервера
		 */
		typedef class WebSocket2 : public web2_t {
			private:
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 сервера
				 */
				friend class Http2;
			private:
				// Количество активных ядер
				uint16_t _threads;
			private:
				// Минимальный размер сегмента
				size_t _frameSize;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
				// Объект работы с WebSocket-клиентом HTTP/1.1
				ws1_t _ws1;
			private:
				// Объект рабочего
				ws_scheme_t _scheme;
			private:
				// Объект партнёра клиента
				ws_scheme_t::partner_t _client;
				// Объект партнёра сервера
				ws_scheme_t::partner_t _server;
			private:
				// Поддерживаемые сабпротоколы
				set <string> _subprotocols;
			private:
				// Список поддверживаемых расширений
				vector <vector <string>> _extensions;
			private:
				// Полученные HTTP заголовки
				unordered_multimap <string, string> _headers;
			private:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении клиента
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Функция обратного вызова при записи сообщение адъютанту
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
				 * @param sid    идентификатор потока
				 * @param aid    идентификатор адъютанта
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int chunkSignal(const int32_t sid, const uint64_t aid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
				 * @param sid    идентификатор потока
				 * @param aid    идентификатор адъютанта
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаг полученного фрейма
				 * @return       статус полученных данных
				 */
				int frameSignal(const int32_t sid, const uint64_t aid, const nghttp2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept;
			private:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2
				 * @param sid идентификатор потока
				 * @param aid идентификатор адъютанта
				 * @return    статус полученных данных
				 */
				int beginSignal(const int32_t sid, const uint64_t aid) noexcept;
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param aid   идентификатор адъютанта
				 * @param error флаг ошибки HTTP/2 если присутствует
				 * @return      статус полученных данных
				 */
				int closedSignal(const int32_t sid, const uint64_t aid, const uint32_t error) noexcept;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
				 * @param sid идентификатор потока
				 * @param aid идентификатор адъютанта
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int headerSignal(const int32_t sid, const uint64_t aid, const string & key, const string & val) noexcept;
			private:
				/**
				 * error Метод вывода сообщений об ошибках работы адъютанта
				 * @param aid     идентификатор адъютанта
				 * @param message сообщение с описанием ошибки
				 */
				void error(const uint64_t aid, const ws::mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param aid    идентификатор адъютанта
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param text   данные передаются в текстовом виде
				 */
				void extraction(const uint64_t aid, const vector <char> & buffer, const bool text) noexcept;
			private:
				/**
				 * pong Метод ответа на проверку о доступности сервера
				 * @param aid  идентификатор адъютанта
				 * @param core объект сетевого ядра
				 * @param      message сообщение для отправки
				 */
				void pong(const uint64_t aid, awh::core_t * core, const string & message = "") noexcept;
				/**
				 * ping Метод проверки доступности сервера
				 * @param aid  идентификатор адъютанта
				 * @param core объект сетевого ядра
				 * @param      message сообщение для отправки
				 */
				void ping(const uint64_t aid, awh::core_t * core, const string & message = "") noexcept;
			private:
				/**
				 * erase Метод удаления отключившихся адъютантов
				 * @param aid идентификатор адъютанта
				 */
				void erase(const uint64_t aid = 0) noexcept;
				/**
				 * disconnect Метод отключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void disconnect(const uint64_t aid) noexcept;
			public:
				/**
				 * init Метод инициализации WebSocket-сервера
				 * @param socket   unix-сокет для биндинга
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const string & socket, const http_t::compress_t compress = http_t::compress_t::DEFLATE) noexcept;
				/**
				 * init Метод инициализации WebSocket-сервера
				 * @param port     порт сервера
				 * @param host     хост сервера
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const u_int port, const string & host = "", const http_t::compress_t compress = http_t::compress_t::DEFLATE) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param aid  идентификатор адъютанта
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const uint64_t aid, const ws::mess_t & mess) noexcept;
				/**
				 * sendMessage Метод отправки сообщения клиенту
				 * @param aid     идентификатор адъютанта
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 */
				void sendMessage(const uint64_t aid, const vector <char> & message, const bool text = true) noexcept;
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
				 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
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
				 * port Метод получения порта подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    порт подключения адъютанта
				 */
				u_int port(const uint64_t aid) const noexcept;
				/**
				 * agent Метод извлечения агента клиента
				 * @param aid идентификатор адъютанта
				 * @return    агент к которому относится подключённый клиент
				 */
				agent_t agent(const uint64_t aid) const noexcept;
				/**
				 * ip Метод получения IP-адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес интернет подключения адъютанта
				 */
				const string & ip(const uint64_t aid) const noexcept;
				/**
				 * mac Метод получения MAC-адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес устройства адъютанта
				 */
				const string & mac(const uint64_t aid) const noexcept;
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
				void close(const uint64_t aid) noexcept;
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
				 * @param aid идентификатор адъютанта
				 * @return    список выбранных сабпротоколов
				 */
				const set <string> & subprotocols(const uint64_t aid) const noexcept;
			public:
				/**
				 * extensions Метод установки списка расширений
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
				/**
				 * extensions Метод извлечения списка расширений
				 * @param aid идентификатор адъютанта
				 * @return    список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions(const uint64_t aid) const noexcept;
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
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const map <settings_t, uint32_t> & settings = {}) noexcept;
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
			public:
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const server::core_t * core) noexcept;
			public:
				/**
				 * setHeaders Метод установки списка заголовков
				 * @param headers список заголовков для установки
				 */
				void setHeaders(const unordered_multimap <string, string> & headers) noexcept;
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
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * WebSocket2 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				WebSocket2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * WebSocket2 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				WebSocket2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WebSocket2 Деструктор
				 */
				~WebSocket2() noexcept;
		} ws2_t;
	};
};

#endif // __AWH_WEB_WS2_SERVER__
