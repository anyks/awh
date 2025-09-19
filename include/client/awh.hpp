/**
 * @file: awh.hpp
 * @date: 2023-09-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Наши модули
 */
#include "web/http2.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief Класс работы с WEB-клиентом
		 *
		 */
		typedef class AWHSHARED_EXPORT AWH {
			private:
				/**
				 * @brief Структура ответа сервера
				 *
				 */
				typedef struct Response {
					int32_t sid;                                        // Идентификатор потока
					uint64_t rid;                                       // Идентификатор запроса
					uint32_t code;                                      // Код ответа сервера
					string message;                                     // Сообщение ответа сервера
					vector <char> & entity;                             // Тело ответа
					std::unordered_multimap <string, string> & headers; // Заголовки ответа
					/**
					 * @brief Конструктор
					 *
					 * @param headers заголовки ответа
					 * @param entity  тело ответа
					 */
					Response(std::unordered_multimap <string, string> & headers, vector <char> & entity) noexcept :
					 sid(-1), rid(0), code(0), message{""}, entity(entity), headers(headers) {}
				} response_t;
			private:
				// Объект работы с URI
				uri_t _uri;
				// Объект DNS-резолвера
				dns_t _dns;
				// Объект работы с протоколом HTTP/2
				client::http2_t _http;
			private:
				// Список доступных компрессоров
				vector <awh::http_t::compressor_t> _compressors;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const client::core_t * _core;
			public:
				/**
				 * @brief Метод извлечения поддерживаемого протокола подключения
				 *
				 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto() const noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения об ошибке на сервер Websocket
				 *
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения на сервер
				 *
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const vector <char> & message, const bool text = true) noexcept;
				/**
				 * @brief Метод отправки сообщения на сервер
				 *
				 * @param message передаваемое сообщения в бинарном виде
				 * @param size    размер передаваемого сообещния
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const char * message, const size_t size, const bool text = true) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения на сервер HTTP/2
				 *
				 * @param request параметры запроса на удалённый сервер
				 * @return        идентификатор отправленного запроса
				 */
				int32_t send(const web_t::request_t & request) noexcept;
			public:
				/**
				 * @brief Метод отправки данных в бинарном виде серверу
				 *
				 * @param buffer буфер бинарных данных передаваемых серверу
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const char * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод отправки тела сообщения на сервер
				 *
				 * @param sid    идентификатор потока HTTP
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param end    флаг последнего сообщения после которого поток закрывается
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const int32_t sid, const char * buffer, const size_t size, const bool end) noexcept;
			public:
				/**
				 * @brief Метод отправки заголовков на сервер
				 *
				 * @param sid     идентификатор потока HTTP
				 * @param url     адрес запроса на сервере
				 * @param method  метод запроса на сервере
				 * @param headers заголовки отправляемые на сервер
				 * @param end     размер сообщения в байтах
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const int32_t sid, const uri_t::url_t & url, const awh::web_t::method_t method, const std::unordered_multimap <string, string> & headers, const bool end) noexcept;
			public:
				/**
				 * @brief Метод HTTP/2 отправки сообщения на сервер
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept;
				/**
				 * @brief Метод HTTP/2 отправки заголовков на сервер
				 *
				 * @param sid     идентификатор потока
				 * @param headers заголовки отправляемые на сервер
				 * @param flag    флаг передаваемого потока по сети
				 * @return        идентификатор нового запроса
				 */
				int32_t send2(const int32_t sid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept;
			public:
				/**
				 * @brief Метод установки на паузу клиента Websocket
				 *
				 */
				void pause() noexcept;
			public:
				/**
				 * @brief Метод инициализации клиента
				 *
				 * @param dest        адрес назначения удалённого сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & dest, const vector <awh::http_t::compressor_t> & compressors = {}) noexcept;
			public:
				/**
				 * @brief Метод запроса в формате HTTP методом GET
				 *
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> GET(const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом DEL
				 *
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> DEL(const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * @brief Метод запроса в формате HTTP методом PUT
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const vector <char> & entity, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом PUT
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param size    размер тела запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const char * entity, const size_t size, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом PUT
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const std::unordered_multimap <string, string> & entity, const std::unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * @brief Метод запроса в формате HTTP методом POST
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const vector <char> & entity, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом POST
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param size    размер тела запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const char * entity, const size_t size, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом POST
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const std::unordered_multimap <string, string> & entity, const std::unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * @brief Метод запроса в формате HTTP методом PATCH
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const vector <char> & entity, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом PATCH
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param size    размер тела запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const char * entity, const size_t size, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом PATCH
				 *
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const std::unordered_multimap <string, string> & entity, const std::unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * @brief Метод запроса в формате HTTP методом HEAD
				 *
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				std::unordered_multimap <string, string> HEAD(const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом TRACE
				 *
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				std::unordered_multimap <string, string> TRACE(const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * @brief Метод запроса в формате HTTP методом OPTIONS
				 *
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				std::unordered_multimap <string, string> OPTIONS(const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * @brief Метод выполнения запроса HTTP
				 *
				 * @param method  метод запроса
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 */
				void REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, std::unordered_multimap <string, string> & headers) noexcept;
				/**
				 * @brief Метод выполнения запроса HTTP
				 *
				 * @param method  метод запроса
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param size    размер тела запроса
				 * @param headers заголовки запроса
				 * @param result  результат работы функции
				 */
				void REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, const char * entity, const size_t size, std::unordered_multimap <string, string> & headers, vector <char> & result) noexcept;
			public:
				/**
				 * @brief Метод открытия подключения
				 *
				 */
				void open() noexcept;
			public:
				/**
				 * @brief Метод принудительного сброса подключения
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * @brief Метод остановки клиента
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска клиента
				 *
				 */
				void start() noexcept;
			public:
				/**
				 * @brief Метод установки времени ожидания ответа WebSocket-сервера
				 *
				 * @param sec время ожидания в секундах
				 */
				void waitPong(const uint16_t sec) noexcept;
				/**
				 * @brief Метод установки интервала времени выполнения пингов
				 *
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const uint16_t sec) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const char * name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_http.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const string & name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_http.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_http.on <T> (fid, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam A    тип идентификатора функции
				 * @tparam B    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename A, typename B, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const A fid, Args... args) noexcept -> uint64_t {
					// Если мы получили на вход число
					if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_http.on <B> (static_cast <uint64_t> (fid), args...);
					// Выводим результат по умолчанию
					return 0;
				}
			public:
				/**
				 * @brief Метод установки поддерживаемого сабпротокола
				 *
				 * @param subprotocol сабпротокол для установки
				 */
				void subprotocol(const string & subprotocol) noexcept;
				/**
				 * @brief Метод получения списка выбранных сабпротоколов
				 *
				 * @return список выбранных сабпротоколов
				 */
				const std::unordered_set <string> & subprotocols() const noexcept;
				/**
				 * @brief Метод установки списка поддерживаемых сабпротоколов
				 *
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const std::unordered_set <string> & subprotocols) noexcept;
			public:
				/**
				 * @brief Метод извлечения списка расширений Websocket
				 *
				 * @return список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions() const noexcept;
				/**
				 * @brief Метод установки списка расширений Websocket
				 *
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сети
				 *
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const string & read = "", const string & write = "") noexcept;
			public:
				/**
				 * @brief Метод установки флагов настроек модуля
				 *
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const std::set <web_t::flag_t> & flags) noexcept;
			public:
				/**
				 * @brief Модуль установки настроек протокола HTTP/2
				 *
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const std::map <awh::http2_t::settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * @brief Метод установки размера чанка
				 *
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * @brief Метод установки размеров сегментов фрейма Websocket
				 *
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * @brief Метод установки общего количества попыток
				 *
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * @brief Метод загрузки файла со списком хостов
				 *
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const string & filename) noexcept;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
				/**
				 * @brief Метод установки списка поддерживаемых компрессоров
				 *
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <awh::http_t::compressor_t> & compressors) noexcept;
				/**
				 * @brief Метод установки жизни подключения
				 *
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод активации многопоточности в Websocket
				 *
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
			public:
				/**
				 * @brief Метод установки User-Agent для HTTP-запроса
				 *
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * @brief Метод установки идентификации клиента
				 *
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации прокси-склиента
				 *
				 * @param work флаг активации/деактивации прокси-клиента
				 */
				void proxy(const client::scheme_t::work_t work) noexcept;
				/**
				 * @brief Метод установки прокси-сервера
				 *
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / IPC)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * @brief Метод сброса кэша DNS-резолвера
				 *
				 * @return результат работы функции
				 */
				bool flushDNS() noexcept;
			public:
				/**
				 * @brief Метод установки времени ожидания выполнения запроса DNS-резолвера
				 *
				 * @param sec интервал времени выполнения запроса в секундах
				 */
				void timeoutDNS(const uint8_t sec) noexcept;
			public:
				/**
				 * @brief Метод установки префикса переменной окружения для извлечения серверов имён
				 *
				 * @param prefix префикс переменной окружения для установки
				 */
				void prefixDNS(const string & prefix) noexcept;
			public:
				/**
				 * @brief Метод очистки чёрного списка DNS-резолвера
				 *
				 * @param domain доменное имя для которого очищается чёрный список
				 */
				void clearDNSBlackList(const string & domain) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списока DNS-резолвера
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInDNSBlackList(const string & domain, const string & ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список DNS-резолвера
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void setToDNSBlackList(const string & domain, const string & ip) noexcept;
			public:
				/**
				 * @brief Метод отключения/включения алгоритма TCP/CORK
				 *
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool cork(const engine_t::mode_t mode) noexcept;
				/**
				 * @brief Метод отключения/включения алгоритма Нейгла
				 *
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool nodelay(const engine_t::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод получения флага шифрования
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки
				 */
				bool crypted(const int32_t sid) const noexcept;
			public:
				/**
				 * @brief Метод активации шифрования
				 *
				 * @param mode флаг активации шифрования
				 */
				void encryption(const bool mode) noexcept;
				/**
				 * @brief Метод установки параметров шифрования
				 *
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * @brief Метод установки типа авторизации
				 *
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
				/**
				 * @brief Метод установки типа авторизации прокси-сервера
				 *
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * @brief Метод ожидания входящих сообщений
				 *
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * @brief Метод детекции сообщений по количеству секунд
				 *
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const uint16_t read = READ_TIMEOUT, const uint16_t write = WRITE_TIMEOUT, const uint16_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * @brief Метод установки параметров сети
				 *
				 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
				 */
				void network(const vector <string> & ips = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				AWH(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~AWH() noexcept {}
		} awh_t;
	};
};

#endif // __AWH_CLIENT__
