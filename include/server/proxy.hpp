/**
 * @file: proxy.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_PROXY_SERVER__
#define __AWH_PROXY_SERVER__

/**
 * Наши модули
 */
#include <core/client.hpp>
#include <core/server.hpp>
#include <scheme/proxy.hpp>

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
		 * Proxy Класс работы с HTTP PROXY сервером
		 */
		typedef class Proxy {
			public:
				/**
				 * Режим работы адъютанта
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Режим подключения
					DISCONNECT = 0x02  // Режим отключения
				};
				/**
				 * Режим событие адъютанта
				 */
				enum class event_t : uint8_t {
					REQUEST  = 0x01, // Режим запроса
					RESPONSE = 0x02  // Режим ответа
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOINFO    = 0x01, // Флаг запрещающий вывод информационных сообщений
					WAITMESS  = 0x02, // Флаг ожидания входящих сообщений
					NOCONNECT = 0x04  // Флаг запрещающий метод CONNECT
				};
			private:
				/**
				 * Core Объект биндинга TCP/IP
				 */
				typedef struct Core {
					client::core_t client; // Объект биндинга TCP/IP для клиента
					server::core_t server; // Объект биндинга TCP/IP для сервера
					/**
					 * Core Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Core(const fmk_t * fmk, const log_t * log) noexcept :
					 client(fmk, log), server(awh::core_t::affiliation_t::PRIMARY, fmk, log) {}
				} core_t;
				/**
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Функция обратного вызова для извлечения пароля
					function <string (const string &)> extractPass;
					// Функция обратного вызова для обработки авторизации
					function <bool (const string &, const string &)> checkAuth;
					// Функция обратного вызова, при запуске или остановки подключения к серверу
					function <void (const size_t, const mode_t, Proxy *)> active;
					// Функция обратного вызова, при получении HTTP чанков от адъютанта
					function <void (const vector <char> &, const awh::http_t *)> chunking;
					// Функция обратного вызова, при получении сообщения с сервера
					function <bool (const size_t, const event_t, http_t *, Proxy *)> message;
					// Функция разрешения подключения адъютанта на сервере
					function <bool (const string &, const string &, const u_int, Proxy *)> accept;
					// Функция обратного вызова, при получении сообщения с сервера
					function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> binary;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept :
					 extractPass(nullptr), checkAuth(nullptr), active(nullptr),
					 chunking(nullptr), message(nullptr), accept(nullptr), binary(nullptr) {}
				} fn_t;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
				// unix-сокет сервера
				string _usock;
			private:
				// Объект биндинга TCP/IP
				core_t _core;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего для сервера
				proxy_scheme_t _scheme;
			private:
				// Идентификатор сервера
				string _sid;
				// Версия сервера
				string _ver;
				// Название сервера
				string _name;
			private:
				// Название сервера
				string _realm;
				// Временный ключ сессии сервера
				string _opaque;
			private:
				// Пароль шифрования передаваемых данных
				string _pass;
				// Соль шифрования передаваемых данных
				string _salt;
				// Размер шифрования передаваемых данных
				hash_t::cipher_t _cipher;
			private:
				// Алгоритм шифрования для Digest авторизации
				auth_t::hash_t _authHash;
				// Тип авторизации
				auth_t::type_t _authType;
			private:
				// Флаг шифрования сообщений
				bool _crypt;
				// Флаг долгоживущего подключения
				bool _alive;
				// Флаг запрещающий метод CONNECT
				bool _noConnect;
			private:
				// Размер одного чанка
				size_t _chunkSize;
				// Максимальный интервал времени жизни подключения
				size_t _timeAlive;
				// Максимальное количество запросов
				size_t _maxRequests;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
			private:
				/**
				 * runCallback Функция обратного вызова при активации ядра сервера
				 * @param mode флаг запуска/остановки
				 * @param core объект сетевого ядра
				 */
				void runCallback(const bool mode, awh::core_t * core) noexcept;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * persistServerCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * openServerCallback Функция обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openServerCallback(const size_t sid, awh::core_t * core) noexcept;
				/**
				 * connectClientCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectClientCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * connectServerCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectServerCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectClientCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectClientCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectServerCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectServerCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * acceptServerCallback Функция обратного вызова при проверке подключения адъютанта
				 * @param ip   адрес интернет подключения адъютанта
				 * @param mac  мак-адрес подключившегося адъютанта
				 * @param port порт подключившегося адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат разрешения к подключению адъютанта
				 */
				bool acceptServerCallback(const string & ip, const string & mac, const u_int port, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * readServerCallback Функция обратного вызова при чтении сообщения с адъютанта
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * writeServerCallback Функция обратного вызова при записи сообщения на адъютанте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * prepare Метод обработки входящих данных
				 * @param aid идентификатор адъютанта
				 * @param sid идентификатор схемы сети
				 */
				void prepare(const size_t aid, const size_t sid) noexcept;
			public:
				/**
				 * init Метод инициализации WebSocket адъютанта
				 * @param socket   unix-сокет для биндинга
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const string & socket, const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
				/**
				 * init Метод инициализации WebSocket адъютанта
				 * @param port     порт сервера
				 * @param host     хост сервера
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const u_int port, const string & host = "", const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const mode_t, Proxy *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const size_t, const event_t, http_t *, Proxy *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> callback) noexcept;
			public:
				/**
				 * on Метод добавления функции извлечения пароля
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void on(function <string (const string &)> callback) noexcept;
				/**
				 * on Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void on(function <bool (const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для получения чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const http_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int, Proxy *)> callback) noexcept;
			public:
				/**
				 * reject Метод отправки сообщения об ошибке
				 * @param aid     идентификатор адъютанта
				 * @param code    код сообщения для адъютанта
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void reject(const size_t aid, const u_int code, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * response Метод отправки сообщения адъютанту
				 * @param aid     идентификатор адъютанта
				 * @param code    код сообщения для адъютанта
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void response(const size_t aid, const u_int code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * port Метод получения порта подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    порт подключения адъютанта
				 */
				u_int port(const size_t aid) const noexcept;
				/**
				 * ip Метод получения IP адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес интернет подключения адъютанта
				 */
				const string & ip(const size_t aid) const noexcept;
				/**
				 * mac Метод получения MAC адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес устройства адъютанта
				 */
				const string & mac(const size_t aid) const noexcept;
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
				void alive(const size_t time) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param aid  идентификатор адъютанта
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const size_t aid, const bool mode) noexcept;
			public:
				/**
				 * stop Метод остановки адъютанта
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска адъютанта
				 */
				void start() noexcept;
			public:
				/**
				 * close Метод закрытия подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
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
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * mode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void mode(const u_short flag) noexcept;
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
				/**
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const size_t size = 0) noexcept;
			public:
				/**
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
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
				 * @param aid   идентификатор адъютанта
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const size_t aid, const string & read, const string & write) noexcept;
				/**
				 * network Метод установки параметров сети
				 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				void network(const vector <string> & ip = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
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
				 * serverName Метод добавления названия сервера
				 * @param name название сервера для добавления
				 */
				void serverName(const string & name = "") noexcept;
			public:
				/**
				 * chunkSize Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunkSize(const size_t size) noexcept;
				/**
				 * maxRequests Метод установки максимального количества запросов
				 * @param max максимальное количество запросов
				 */
				void maxRequests(const size_t max) noexcept;
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
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void serv(const string & id, const string & name, const string & ver) noexcept;
				/**
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
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

#endif // __AWH_PROXY_SERVER__
