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
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Стандартная библиотека
 */
#include <nlohmann/json.hpp>

/**
 * Наши модули
 */
#include <client/web/http2.hpp>

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * AWH Класс работы с WEB-клиентом
		 */
		typedef class AWH {
			private:
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект DNS-резолвера
				dns_t _dns;
				// Объект работы с протоколом HTTP/2
				client::http2_t _http;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const client::core_t * _core;
			public:
				/**
				 * proto Метод извлечения поддерживаемого протокола подключения
				 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto() const noexcept;
			public:
				/**
				 * sendTimeout Метод отправки сигнала таймаута
				 */
				void sendTimeout() noexcept;
				/**
				 * sendError Метод отправки сообщения об ошибке на сервер WebSocket
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
				/**
				 * sendMessage Метод отправки сообщения на сервер
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 */
				void sendMessage(const vector <char> & message, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки сообщения на сервер HTTP/2
				 * @param request параметры запроса на удалённый сервер
				 * @return        идентификатор отправленного запроса
				 */
				int32_t send(const web_t::request_t & request) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде серверу
				 * @param buffer буфер бинарных данных передаваемых серверу
				 * @param size   размер сообщения в байтах
				 */
				void send(const char * buffer, const size_t size) noexcept;
			public:
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
				 * windowUpdate2 Метод HTTP/2 обновления размера окна фрейма
				 * @param sid  идентификатор потока
				 * @param size размер нового окна
				 * @return     результат установки размера офна фрейма
				 */
				bool windowUpdate2(const int32_t sid, const int32_t size) noexcept;
			public:
				/**
				 * send2 Метод HTTP/2 отправки сообщения на сервер
				 * @param sid    идентификатор потока
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send2(const int32_t sid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept;
				/**
				 * send2 Метод HTTP/2 отправки заголовков на сервер
				 * @param sid     идентификатор потока
				 * @param headers заголовки отправляемые на сервер
				 * @param flag    флаг передаваемого потока по сети
				 * @return        идентификатор нового запроса
				 */
				int32_t send2(const int32_t sid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept;
			public:
				/**
				 * pause Метод установки на паузу клиента WebSocket
				 */
				void pause() noexcept;
			public:
				/**
				 * init Метод инициализации клиента
				 * @param dest        адрес назначения удалённого сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & dest, const vector <awh::http_t::compress_t> & compressors = {}) noexcept;
			public:
				/**
				 * GET Метод запроса в формате HTTP методом GET
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * DEL Метод запроса в формате HTTP методом DEL
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * PUT Метод запроса в формате HTTP методом PUT
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PUT Метод запроса в формате HTTP методом PUT
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PUT Метод запроса в формате HTTP методом PUT
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * POST Метод запроса в формате HTTP методом POST
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * POST Метод запроса в формате HTTP методом POST
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * POST Метод запроса в формате HTTP методом POST
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * PATCH Метод запроса в формате HTTP методом PATCH
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PATCH Метод запроса в формате HTTP методом PATCH
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PATCH Метод запроса в формате HTTP методом PATCH
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * HEAD Метод запроса в формате HTTP методом HEAD
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				unordered_multimap <string, string> HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * TRACE Метод запроса в формате HTTP методом TRACE
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				unordered_multimap <string, string> TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				unordered_multimap <string, string> OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * REQUEST Метод выполнения запроса HTTP
				 * @param method  метод запроса
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 */
				void REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, unordered_multimap <string, string> & headers) noexcept;
			public:
				/**
				 * open Метод открытия подключения
				 */
				void open() noexcept;
			public:
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
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const fn_t & callback) noexcept;
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
				 * extensions Метод извлечения списка расширений WebSocket
				 * @return список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions() const noexcept;
				/**
				 * extensions Метод установки списка расширений WebSocket
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <web_t::flag_t> & flags) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const map <awh::http2_t::settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма WebSocket
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * attempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * hosts Метод загрузки файла со списком хостов
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const string & filename) noexcept;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
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
				void compressors(const vector <awh::http_t::compress_t> & compressors) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности в WebSocket
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
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
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * flushDNS Метод сброса кэша DNS-резолвера
				 * @return результат работы функции
				 */
				bool flushDNS() noexcept;
			public:
				/**
				 * timeoutDNS Метод установки времени ожидания выполнения запроса
				 * @param sec интервал времени выполнения запроса в секундах
				 */
				void timeoutDNS(const uint8_t sec) noexcept;
				/**
				 * timeToLiveDNS Метод установки времени жизни DNS-кэша
				 * @param ttl время жизни DNS-кэша в миллисекундах
				 */
				void timeToLiveDNS(const time_t ttl) noexcept;
			public:
				/**
				 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
				 * @param prefix префикс переменной окружения для установки
				 */
				void prefixDNS(const string & prefix) noexcept;
			public:
				/**
				 * clearDNSBlackList Метод очистки чёрного списка
				 * @param domain доменное имя для которого очищается чёрный список
				 */
				void clearDNSBlackList(const string & domain) noexcept;
				/**
				 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInDNSBlackList(const string & domain, const string & ip) noexcept;
				/**
				 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void setToDNSBlackList(const string & domain, const string & ip) noexcept;
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
				 * bytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const time_t read = READ_TIMEOUT, const time_t write = WRITE_TIMEOUT, const time_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * network Метод установки параметров сети
				 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 */
				void network(const vector <string> & ips = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * AWH Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				AWH(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~AWH Деструктор
				 */
				~AWH() noexcept {}
		} awh_t;
	};
};

#endif // __AWH_CLIENT__
