/**
 * @file: ws2.hpp
 * @date: 2023-09-14
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

#ifndef __AWH_WEB_WS2_CLIENT__
#define __AWH_WEB_WS2_CLIENT__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/client.hpp>
#include <sys/threadpool.hpp>
#include <client/web/web.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * WebSocket2 Класс WebSocket-клиента
		 */
		typedef class WebSocket2 : public web2_t {
			private:
				/**
				 * Allow Структура флагов разрешения обменом данных
				 */
				typedef struct Allow {
					bool send;    // Флаг разрешения отправки данных
					bool receive; // Флаг разрешения чтения данных
					/**
					 * Allow Конструктор
					 */
					Allow() noexcept : send(true), receive(true) {}
				} __attribute__((packed)) allow_t;
				/**
				 * Partner Структура партнёра
				 */
				typedef struct Partner {
					short wbit;    // Размер скользящего окна
					bool takeOver; // Флаг скользящего контекста сжатия
					/**
					 * Partner Конструктор
					 */
					Partner() noexcept : wbit(0), takeOver(false) {}
				} __attribute__((packed)) partner_t;
				/**
				 * Frame Объект фрейма WebSocket
				 */
				typedef struct Frame {
					size_t size;                  // Минимальный размер сегмента
					ws::frame_t methods;          // Методы работы с фреймом WebSocket
					ws::frame_t::opcode_t opcode; // Полученный опкод сообщения
					/**
					 * Frame Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Frame(const fmk_t * fmk, const log_t * log) noexcept :
					 size(0xFA000), methods(fmk, log), opcode(ws::frame_t::opcode_t::TEXT) {}
				} frame_t;
				/**
				 * Worker Структура активного воркера
				 */
				typedef struct Worker {
					bool crypt;            // Флаг шифрования сообщений
					bool deflate;          // Флаг переданных сжатых данных
					uint8_t attempt;       // Количество попыток
					uri_t uri;             // Объект работы с URI ссылками
					ws_t http;             // Объект для работы с HTTP
					hash_t hash;           // Объект для компрессии-декомпрессии данных
					fn_t callback;         // Объект функций обратного вызова
					frame_t frame;         // Объект для работы с фреймом WebSocket
					agent_t agent;         // Агент воркера
					ws::mess_t mess;       // Объект отправляемого сообщения
					partner_t client;      // Объект партнёра клиента
					partner_t server;      // Объект партнёра сервера
					vector <char> fragmes; // Данные фрагметрированного сообщения
					/**
					 * Worker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Worker(const fmk_t * fmk, const log_t * log) noexcept :
					 crypt(false), deflate(false), attempt(0), uri(fmk), http(fmk, log, &uri),
					 hash(log), callback(log), frame(fmk, log), agent(agent_t::WEBSOCKET) {}
				} worker_t;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
				// Объект разрешения обмена данными
				allow_t _allow;
			private:
				// Флаг завершения работы клиента
				bool _close;
				// Флаг запрета вывода информационных сообщений
				bool _noinfo;
				// Флаг фриза работы клиента
				bool _freeze;
				// Контрольная точка ответа на пинг
				time_t _point;
			private:
				// Объект партнёра клиента
				partner_t _client;
				// Объект партнёра сервера
				partner_t _server;
			private:
				// Список активных врокеров
				map <int32_t, unique_ptr <worker_t>> _workers;
			private:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Метод обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * receivedFrame Метод обратного вызова при получении фрейма заголовков HTTP/2 с сервера
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @return        статус полученных данных
				 */
				int receivedFrame(const nghttp2_frame * frame) noexcept;
				/**
				 * receivedChunk Метод обратного вызова при получении чанка с сервера HTTP/2
				 * @param sid    идентификатор сессии HTTP/2
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int receivedChunk(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * receivedBeginHeaders Метод начала получения фрейма заголовков HTTP/2
				 * @param sid идентификатор сессии HTTP/2
				 * @return    статус полученных данных
				 */
				int receivedBeginHeaders(const int32_t sid) noexcept;
				/**
				 * receivedHeader Метод обратного вызова при получении заголовка HTTP/2
				 * @param sid идентификатор сессии HTTP/2
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int receivedHeader(const int32_t sid, const string & key, const string & val) noexcept;
			private:
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
				void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * response Метод получения ответа сервера
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 */
				void response(const u_int code, const string & message) noexcept;
			private:
				/**
				 * header Метод получения заголовка
				 * @param key   ключ заголовка
				 * @param value значение заголовка
				 */
				void header(const string & key, const string & value) noexcept;
				/**
				 * headers Метод получения заголовков
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param headers заголовки ответа сервера
				 */
				void headers(const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			private:
				/**
				 * ping Метод проверки доступности сервера
				 * @param message сообщение для отправки
				 */
				void ping(const string & message = "") noexcept;
				/**
				 * pong Метод ответа на проверку о доступности сервера
				 * @param message сообщение для отправки
				 */
				void pong(const string & message = "") noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param id   идентификатор запроса
				 * @param aid  идентификатор адъютанта
				 * @param core объект сетевого ядра
				 * @return     результат препарирования
				 */
				status_t prepare(const int32_t id, const size_t aid, client::core_t * core) noexcept;
			private:
				/**
				 * error Метод вывода сообщений об ошибках работы клиента
				 * @param message сообщение с описанием ошибки
				 */
				void error(const ws::mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param utf8   данные передаются в текстовом виде
				 */
				void extraction(const vector <char> & buffer, const bool utf8) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
				/**
				 * send Метод отправки сообщения на сервер
				 * @param message буфер сообщения в бинарном виде
				 * @param size    размер сообщения в байтах
				 * @param utf8    данные передаются в текстовом виде
				 */
				void send(const char * message, const size_t size, const bool utf8 = true) noexcept;
			public:
				/**
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
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
				 * on Метод установки функции обратного вызова на событие получения ошибок
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const bool)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученных заголовков с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * sub Метод получения выбранного сабпротокола
				 * @return выбранный сабпротокол
				 */
				const string & sub() const noexcept;
				/**
				 * sub Метод установки подпротокола поддерживаемого сервером
				 * @param sub подпротокол для установки
				 */
				void sub(const string & sub) noexcept;
				/**
				 * subs Метод установки списка подпротоколов поддерживаемых сервером
				 * @param subs подпротоколы для установки
				 */
				void subs(const vector <string> & subs) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const client::core_t * core) noexcept;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
			public:
				/**
				 * userAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void serv(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param threads количество потоков для активации
				 * @param mode    флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const size_t threads = 0, const bool mode = true) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
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
				WebSocket2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WebSocket2 Деструктор
				 */
				~WebSocket2() noexcept;
		} ws2_t;
	};
};

#endif // __AWH_WEB_WS2_CLIENT__