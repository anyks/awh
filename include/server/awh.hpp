/**
 * @file: awh.hpp
 * @date: 2023-10-02
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

#ifndef __AWH_SERVER__
#define __AWH_SERVER__

/**
 * Наши модули
 */
#include <server/web/http2.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * AWH Класс работы с WEB-сервером
		 */
		typedef class AWHSHARED_EXPORT AWH {
			private:
				// Объект DNS-резолвера
				dns_t _dns;
				// Объект работы с протоколом HTTP/2
				server::http2_t _http;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
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
				 * @param bid идентификатор брокера
				 * @param sid идентификатор потока
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
				 * init Метод инициализации WEB-сервера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compressor_t> & compressors = {}) noexcept;
				/**
				 * init Метод инициализации WEB-сервера
				 * @param port        порт сервера
				 * @param host        хост сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const uint32_t port, const string & host = "", const vector <http_t::compressor_t> & compressors = {}) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param bid  идентификатор брокера
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const uint64_t bid, const ws::mess_t & mess) noexcept;
			public:
				/**
				 * sendMessage Метод отправки сообщения клиенту
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const vector <char> & message, const bool text = true) noexcept;
				/**
				 * sendMessage Метод отправки сообщения на сервер
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param size    размер передаваемого сообещния
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const char * message, const size_t size, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде клиенту
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const uint64_t bid, const char * buffer, const size_t size) noexcept;
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
				int32_t send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept;
			public:
				/**
				 * send Метод отправки сообщения брокеру
				 * @param sid     идентификатор потока HTTP
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param buffer  данные полезной нагрузки (тело сообщения)
				 * @param size    размер данных полезной нагрузки (размер тела сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const char * buffer, const size_t size, const unordered_multimap <string, string> & headers) noexcept;
				/**
				 * send Метод отправки сообщения брокеру
				 * @param sid     идентификатор потока HTTP
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void send(const int32_t sid, const uint64_t bid, const uint32_t code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) noexcept;
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
				 * send2 HTTP/2 Метод отправки трейлеров
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @return        результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept;
				/**
				 * send2 HTTP/2 Метод отправки сообщения клиенту
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept;
				/**
				 * send2 HTTP/2 Метод отправки заголовков
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param headers заголовки отправляемые
				 * @param flag    флаг передаваемого потока по сети
				 * @return        флаг последнего сообщения после которого поток закрывается
				 */
				int32_t send2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept;
			public:
				/**
				 * push2 HTTP/2 Метод отправки пуш-уведомлений
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
					// Устанавливаем функцию обратного вызова
					this->_http.callback(idw, fn);
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
					// Устанавливаем функцию обратного вызова
					this->_http.callback(name, fn);
				}
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				uint32_t port(const uint64_t bid) const noexcept;
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
				/**
				 * agent Метод извлечения агента клиента
				 * @param bid идентификатор брокера
				 * @return    агент к которому относится подключённый клиент
				 */
				web_t::agent_t agent(const uint64_t bid) const noexcept;
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
				 * waitPong Метод установки времени ожидания ответа WebSocket-клиента
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
				 * extensions Метод извлечения списка поддерживаемых расширений
				 * @param bid идентификатор брокера
				 * @return    список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions(const uint64_t bid) const noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t total) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
			public:
				/**
				 * hosts Метод загрузки файла со списком хостов
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const string & filename) noexcept;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <http_t::compressor_t> & compressors) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <web_t::flag_t> & flags) noexcept;
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
				 * setAltSvc Метод установки списка разрешённых источников
				 * @param origins список альтернативных сервисов
				 */
				void setAltSvc(const unordered_multimap <string, string> & origins) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const map <awh::http2_t::settings_t, uint32_t> & settings = {}) noexcept;
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
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки времени жизни подключения
				 * @param sec время жизни подключения
				 */
				void alive(const time_t sec) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * identity Метод установки идентичности протокола модуля
				 * @param identity идентичность протокола модуля
				 */
				void identity(const http_t::identity_t identity) noexcept;
			public:
				/**
				 * waitMessage Метод ожидания входящих сообщений
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const time_t sec) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const time_t read, const time_t write) noexcept;
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
				 * AWH Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				AWH(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~AWH Деструктор
				 */
				~AWH() noexcept {}
		} awh_t;
	};
};

#endif // __AWH_SERVER__
