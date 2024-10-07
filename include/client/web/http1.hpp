/**
 * @file: http1.hpp
 * @date: 2023-09-12
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

#ifndef __AWH_WEB_HTTP1_CLIENT__
#define __AWH_WEB_HTTP1_CLIENT__

/**
 * Наши модули
 */
#include <client/web/web.hpp>
#include <client/web/ws1.hpp>

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
		 * Http2 Прототип класса HTTP/2 клиента
		 */
		class Http2;
		/**
		 * Http1 Класс HTTP-клиента
		 */
		typedef class AWHSHARED_EXPORT Http1 : public web_t {
			private:
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 клиента
				 */
				friend class Http2;
			private:
				// Флаг открытия подключения
				bool _mode;
				// Флаг разрешения использования протокол Websocket
				bool _webSocket;
			private:
				// Объект для работы c Websocket
				ws1_t _ws1;
				// Объект для работы с HTTP-протколом
				http_t _http;
			private:
				// Агент воркера выполнения запроса
				agent_t _agent;
			private:
				// Количество активных ядер
				int16_t _threads;
			private:
				// Хранилище функций обратного вызова для вывода результата
				fn_t _resultCallback;
			private:
				// Список активых запросов
				map <int32_t, request_t> _requests;
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
				 * answer Метод получение статуса ответа сервера
				 * @param sid    идентификатор потока
				 * @param rid    идентификатор запроса
				 * @param status статус ответа сервера
				 */
				void answer(const int32_t sid, const uint64_t rid, const awh::http_t::status_t status) noexcept;
			private:
				/**
				 * redirect Метод выполнения редиректа если требуется
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 * @return    результат выполнения редиректа
				 */
				bool redirect(const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * response Метод получения ответа сервера
				 * @param bid     идентификатор брокера
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 */
				void response(const uint64_t bid, const uint32_t code, const string & message) noexcept;
			private:
				/**
				 * header Метод получения заголовка
				 * @param bid   идентификатор брокера
				 * @param key   ключ заголовка
				 * @param value значение заголовка
				 */
				void header(const uint64_t bid, const string & key, const string & value) noexcept;
				/**
				 * headers Метод получения заголовков
				 * @param bid     идентификатор брокера
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param headers заголовки ответа сервера
				 */
				void headers(const uint64_t bid, const uint32_t code, const string & message, const unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param bid   идентификатор брокера
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept;
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
				 * result Метод завершения выполнения запроса
				 * @param sid идентификатор запроса
				 */
				void result(const int32_t sid) noexcept;
			private:
				/**
				 * pinging Метод таймера выполнения пинга удалённого сервера
				 * @param tid идентификатор таймера
				 */
				void pinging(const uint16_t tid) noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param sid идентификатор запроса
				 * @param bid идентификатор брокера
				 * @return    результат препарирования
				 */
				status_t prepare(const int32_t sid, const uint64_t bid) noexcept;
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
			private:
				/** 
				 * submit Метод выполнения удалённого запроса на сервер
				 * @param request объект запроса на удалённый сервер
				 */
				void submit(const request_t & request) noexcept;
			public:
				/**
				 * send Метод отправки сообщения на сервер
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
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param end    флаг последнего сообщения после которого поток закрывается
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const char * buffer, const size_t size, const bool end) noexcept;
			public:
				/**
				 * send Метод отправки заголовков на сервер
				 * @param url     адрес запроса на сервере
				 * @param method  метод запроса на сервере
				 * @param headers заголовки отправляемые на сервер
				 * @param end     размер сообщения в байтах
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept;
			public:
				/**
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
			public:
				/**
				 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
				 * @param sec время ожидания в секундах
				 */
				void waitPong(const time_t sec) noexcept;
				/**
				 * pingInterval Метод установки интервала времени выполнения пингов
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const time_t sec) noexcept;
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
				 * proxy Метод активации/деактивации прокси-склиента
				 * @param work флаг активации/деактивации прокси-клиента
				 */
				void proxy(const client::scheme_t::work_t work) noexcept;
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
				 * @return результат проверки
				 */
				bool crypted() const noexcept;
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
				Http1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http1 Деструктор
				 */
				~Http1() noexcept;
		} http1_t;
	};
};

#endif // __AWH_WEB_HTTP1_CLIENT__
