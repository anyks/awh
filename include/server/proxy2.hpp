/**
 * @file: proxy.hpp
 * @date: 2023-11-06
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

#ifndef __AWH_HTTP_PROXY__
#define __AWH_HTTP_PROXY__

/**
 * Стандартная библиотека
 */
#include <map>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <client/awh.hpp>
#include <server/awh.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server клиентское пространство имён
	 */
	namespace server {
		/**
		 * Proxy Класс Proxy-сервера
		 */
		typedef class Proxy {
			public:
				/**
				 * Режим событие брокера
				 */
				enum class event_t : uint8_t {
					REQUEST  = 0x01, // Режим запроса
					RESPONSE = 0x02  // Режим ответа
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE          = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO       = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP       = 0x03, // Флаг запрета остановки биндинга
					DECRYPT        = 0x04, // Флаг предписывающий выполнять расшифровку зашифрованного контента при передаче клиенту
					VERIFY_SSL     = 0x05, // Флаг выполнения проверки сертификата SSL
					RECOMPRESS     = 0x06, // Флаг выполнения рекомпрессинга передаваемых данных
					CONNECT_METHOD = 0x07  // Флаг запрещающий метод CONNECT
				};
			private:
				/**
				 * Ident Структура идентификации сервиса
				 */
				typedef struct Ident {
					string id;   // Идентификатор сервиса
					string ver;  // Версия сервиса
					string name; // Название сервиса
					/**
					 * Ident Конструктор
					 */
					Ident() noexcept : id{""}, ver{""}, name{""} {}
				} ident_t;
				/**
				 * Encryption Структура параметров шифрования
				 */
				typedef struct Encryption {
					bool mode;               // Флаг активности механизма шифрования
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					/**
					 * Encryption Конструктор
					 */
					Encryption() noexcept : mode(false), pass{""}, salt{""}, cipher(hash_t::cipher_t::AES128) {}
				} encryption_t;
				/**
				 * Client Объект клиента
				 */
				typedef struct Client {
					// Объект сетевого ядра
					client::core_t core;
					// Объект активного клиента
					client::awh_t awh;
					/**
					 * Client Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Client(const fmk_t * fmk, const log_t * log) noexcept : core(fmk, log), awh(&core, fmk, log) {}
				} client_t;
			private:
				// Объект идентификации сервиса
				ident_t _ident;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект сетевого ядра
				server::core_t _core;
				// Объект активного сервера
				server::awh_t _server;
				// Объект параметров шифрования
				encryption_t _encryption;
			private:
				// Список активных клиентов
				map <uint64_t, unique_ptr <client_t>> _clients;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * passwordCallback Метод извлечения пароля (для авторизации методом Digest)
				 * @param bid   идентификатор брокера (клиента)
				 * @param login логин пользователя
				 * @return      пароль пользователя хранящийся в базе данных
				 */
				string passwordCallback(const uint64_t bid, const string & login) noexcept;
				/**
				 * authCallback Метод проверки авторизации пользователя (для авторизации методом Basic)
				 * @param bid      идентификатор брокера (клиента)
				 * @param login    логин пользователя (от клиента)
				 * @param password пароль пользователя (от клиента)
				 * @return         результат авторизации
				 */
				bool authCallback(const uint64_t bid, const string & login, const string & password) noexcept;
			private:
				/**
				 * acceptCallback Метод активации клиента на сервере
				 * @param ip   адрес интернет подключения
				 * @param mac  аппаратный адрес подключения
				 * @param port порт подключения
				 * @return     результат проверки
				 */
				bool acceptCallback(const string & ip, const string & mac, const u_int port) noexcept;
			private:
				/**
				 * activeCallback Метод идентификации активности на Web сервере
				 * @param bid  идентификатор брокера (клиента)
				 * @param mode режим события подключения
				 */
				void activeCallback(const uint64_t bid, const server::web_t::mode_t mode) noexcept;
			private:
				/**
				 * handshakeCallback Метод получения удачного запроса
				 * @param sid   идентификатор потока
				 * @param bid   идентификатор брокера
				 * @param agent идентификатор агента клиента
				 */
				void handshakeCallback(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent) noexcept;
				/**
				 * requestCallback Метод запроса клиента
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param method метод запроса
				 * @param url    url-адрес запроса
				 */
				void requestCallback(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url) noexcept;
				/**
				 * entityCallback Метод получения тела запроса
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param method метод запроса
				 * @param url    url-адрес запроса
				 * @param entity тело запроса
				 */
				void entityCallback(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity) noexcept;
				/**
				 * headersCallback Метод получения заголовков запроса
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера
				 * @param method  метод запроса
				 * @param url     url-адрес запроса
				 * @param headers заголовки запроса
				 */
				void headersCallback(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept;
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
				 * @param bid идентификатор брокера
				 * @return    объект HTTP-парсера
				 */
				const awh::http_t * parser(const uint64_t bid) const noexcept;
			public:
				/**
				 * init Метод инициализации WEB-сервера
				 * @param socket     unix-сокет для биндинга
				 * @param compressor поддерживаемый компрессор
				 */
				void init(const string & socket, const http_t::compress_t compressor = http_t::compress_t::NONE) noexcept;
				/**
				 * init Метод инициализации WEB-сервера
				 * @param port       порт сервера
				 * @param host       хост сервера
				 * @param compressor поддерживаемый компрессор
				 */
				void init(const u_int port, const string & host = "", const http_t::compress_t compressor = http_t::compress_t::NONE) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const web_t::mode_t)> callback) noexcept;
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
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				u_int port(const uint64_t bid) const noexcept;
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
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
				/**
				 * hosts Метод загрузки файла со списком хостов
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const string & filename) noexcept;
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
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
				void compressors(const vector <http_t::compress_t> & compressors) noexcept;
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
				void settings(const map <web2_t::settings_t, uint32_t> & settings = {}) noexcept;
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
				 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
				 * @param mode флаг состояния разрешения проверки
				 */
				void verifySSL(const bool mode) noexcept;
				/**
				 * ciphers Метод установки алгоритмов шифрования
				 * @param ciphers список алгоритмов шифрования для установки
				 */
				void ciphers(const vector <string> & ciphers) noexcept;
				/**
				 * ca Метод установки доверенного сертификата (CA-файла)
				 * @param trusted адрес доверенного сертификата (CA-файла)
				 * @param path    адрес каталога где находится сертификат (CA-файл)
				 */
				void ca(const string & trusted, const string & path = "") noexcept;
				/**
				 * certificate Метод установки файлов сертификата
				 * @param chain файл цепочки сертификатов
				 * @param key   приватный ключ сертификата
				 */
				void certificate(const string & chain, const string & key) noexcept;
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
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * sonet Метод установки типа сокета подключения
				 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
				 */
				void sonet(const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * family Метод установки типа протокола интернета
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 */
				void family(const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
				/**
				 * bandWidth Метод установки пропускной способности сети
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const size_t bid, const string & read, const string & write) noexcept;
				/**
				 * network Метод установки параметров сети
				 * @param ips    список IP адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				void network(const vector <string> & ips = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
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
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
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
				 * Proxy Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Proxy(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Proxy Деструктор
				 */
				~Proxy() noexcept {}
		} proxy_t;
	};
};

#endif // __AWH_HTTP_PROXY__
