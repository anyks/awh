/**
 * @file: http2.hpp
 * @date: 2023-09-18
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

#ifndef __AWH_WEB_HTTP2_CLIENT__
#define __AWH_WEB_HTTP2_CLIENT__

/**
 * Наши модули
 */
#include <client/web/web.hpp>
#include <client/web/ws2.hpp>
#include <client/web/http1.hpp>

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
		 * Http2 Класс HTTP2-клиента
		 */
		typedef class Http2 : public web2_t {
			private:
				/**
				 * Service Структура сервиса
				 */
				typedef struct Service {
					string login;        // Логин пользователя для авторизации
					string password;     // Пароль пользователя для авторизации
					auth_t::hash_t hash; // Алгоритм шифрования для Digest авторизации
					auth_t::type_t type; // Тип авторизации
					/**
					 * Service Конструктор
					 */
					Service() noexcept :
					 login{""}, password{""},
					 hash(auth_t::hash_t::MD5),
					 type(auth_t::type_t::NONE) {}
				} service_t;
				/**
				 * Worker Структура активного воркера
				 */
				typedef struct Worker {
					uint64_t id;             // Идентификатор запроса
					bool update;             // Флаг обновления параметров запроса
					http_t http;             // Объект для работы с HTTP
					fn_t callback;           // Хранилище функций обратного вызова
					agent_t agent;           // Агент воркера
					engine_t::proto_t proto; // Активный прототип интернета
					/**
					 * Worker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Worker(const fmk_t * fmk, const log_t * log) noexcept :
					 id(0), update(false), http(fmk, log), callback(log),
					 agent(agent_t::HTTP), proto(engine_t::proto_t::HTTP1_1) {}
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept {}
				} worker_t;
			private:
				// Объект для работы c Websocket
				ws2_t _ws2;
				// Объект для работы с HTTP/1.1 клиентом
				http1_t _http1;
			private:
				// Объект для работы с HTTP-протколом
				http_t _http;
				// Объект параметров сервиса
				service_t _service;
			private:
				// Флаг разрешения использования протокол Websocket
				bool _webSocket;
			private:
				// Количество активных ядер
				int16_t _threads;
			private:
				// Список активных маршрутов запросов
				unordered_map <string, string> _route;
			private:
				// Список активных воркеров
				map <int32_t, unique_ptr <worker_t>> _workers;
				// Список активых запросов
				map <int32_t, unique_ptr <request_t>> _requests;
			private:
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
				/**
				 * writeCallback Метод обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int32_t chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаги полученного фрейма
				 * @return       статус полученных данных
				 */
				int32_t frameSignal(const int32_t sid, const awh::http2_t::direct_t direct, const awh::http2_t::frame_t frame, const set <awh::http2_t::flag_t> & flags) noexcept;
			private:
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param error флаг ошибки если присутствует
				 * @return      статус полученных данных
				 */
				int32_t closedSignal(const int32_t sid, const awh::http2_t::error_t error) noexcept;
			private:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				int32_t beginSignal(const int32_t sid) noexcept;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int32_t headerSignal(const int32_t sid, const string & key, const string & val) noexcept;
			private:
				/**
				 * end Метод завершения работы потока
				 * @param sid    идентификатор потока
				 * @param rid    идентификатор запроса
				 * @param direct направление передачи данных
				 */
				void end(const int32_t sid, const uint64_t rid, const direct_t direct) noexcept;
				/**
				 * answer Метод получение статуса ответа сервера
				 * @param sid    идентификатор потока
				 * @param rid    идентификатор запроса
				 * @param status статус ответа сервера
				 */
				void answer(const int32_t sid, const uint64_t rid, const awh::http_t::status_t status) noexcept;
			private:
				/**
				 * redirect Метод выполнения смены потоков
				 * @param from идентификатор предыдущего потока
				 * @param to   идентификатор нового потока
				 */
				void redirect(const int32_t from, const int32_t to) noexcept;
				/**
				 * redirect Метод выполнения редиректа если требуется
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 * @return    результат выполнения редиректа
				 */
				bool redirect(const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * eventCallback Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param idw   идентификатор функции обратного вызова
				 * @param name  название функции обратного вызова
				 * @param dump  дамп данных функции обратного вызова
				 */
				void eventCallback(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			private:
				/**
				 * update Метод обновления параметров запроса для переадресации
				 * @param request параметры запроса на удалённый сервер
				 * @return        предыдущий идентификатор потока, если произошла переадресация
				 */
				int32_t update(request_t & request) noexcept;
			private:
				/**
				 * result Метод завершения выполнения запроса
				 * @param sid идентификатор потока
				 * @param rid идентификатор запроса
				 */
				void result(const int32_t sid, const uint64_t rid) noexcept;
			private:
				/**
				 * pinging Метод таймера выполнения пинга удалённого сервера
				 * @param tid идентификатор таймера
				 */
				void pinging(const uint16_t tid) noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    результат препарирования
				 */
				status_t prepare(const int32_t sid, const uint64_t bid) noexcept;
			private:
				/**
				 * stream Метод вывода статус потока
				 * @param sid  идентификатор потока
				 * @param mode активный статус потока
				 */
				void stream(const int32_t sid, const mode_t mode) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
				/**
				 * sendMessage Метод отправки сообщения на сервер
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const vector <char> & message, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки сообщения на сервер
				 * @param agent   агент воркера
				 * @param request параметры запроса на удалённый сервер
				 * @return        идентификатор отправленного запроса
				 */
				int32_t send(const request_t & request) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде серверу
				 * @param buffer буфер бинарных данных передаваемых серверу
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const char * buffer, const size_t size) noexcept;
				/**
				 * send Метод отправки тела сообщения на сервер
				 * @param sid    идентификатор потока HTTP
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param end    флаг последнего сообщения после которого поток закрывается
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const char * buffer, const size_t size, const bool end) noexcept;
			public:
				/**
				 * send Метод отправки заголовков на сервер
				 * @param sid     идентификатор потока HTTP
				 * @param url     адрес запроса на сервере
				 * @param method  метод запроса на сервере
				 * @param headers заголовки отправляемые на сервер
				 * @param end     размер сообщения в байтах
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const int32_t sid, const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept;
			public:
				/**
				 * send2 Метод HTTP/2 отправки сообщения на сервер
				 * @param sid    идентификатор потока HTTP/2
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept;
				/**
				 * send2 Метод HTTP/2 отправки заголовков на сервер
				 * @param sid     идентификатор потока HTTP/2
				 * @param headers заголовки отправляемые на сервер
				 * @param flag    флаг передаваемого потока по сети
				 * @return        идентификатор нового запроса
				 */
				int32_t send2(const int32_t sid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept;
			public:
				/**
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
			public:
				/**
				 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
				 * @param time время ожидания в миллисекундах
				 */
				void waitPong(const time_t time) noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
			public:
				/**
				 * subprotocol Метод установки поддерживаемого сабпротокола
				 * @param subprotocol сабпротокол для установки
				 */
				void subprotocol(const string & subprotocol) noexcept;
				/**
				 * subprotocol Метод получения списка выбранных сабпротоколов
				 * @return список выбранных сабпротоколов
				 */
				const set <string> & subprotocols() const noexcept;
				/**
				 * subprotocols Метод установки списка поддерживаемых сабпротоколов
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const set <string> & subprotocols) noexcept;
			public:
				/**
				 * extensions Метод извлечения списка расширений
				 * @return список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions() const noexcept;
				/**
				 * extensions Метод установки списка расширений
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
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
				 * userAgent Метод установки User-Agent для HTTP-запроса
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * ident Метод установки идентификации клиента
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const int16_t count = 0, const bool mode = true) noexcept;
			public:
				/**
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * crypted Метод получения флага шифрования
				 * @param sid идентификатор потока
				 * @return    результат проверки
				 */
				bool crypted(const int32_t sid) const noexcept;
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
				Http2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http2 Деструктор
				 */
				~Http2() noexcept;
		} http2_t;
	};
};

#endif // __AWH_WEB_HTTP2_CLIENT__
