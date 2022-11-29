/**
 * @file: socks5.hpp
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

#ifndef __AWH_PROXY_SOCKS5_SERVER__
#define __AWH_PROXY_SOCKS5_SERVER__

/**
 * Наши модули
 */
#include <core/client.hpp>
#include <core/server.hpp>
#include <scheme/socks5.hpp>

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
		 * ProxySocks5 Класс работы с SOCKS5 PROXY сервером
		 */
		typedef class ProxySocks5 {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Режим подключения
					DISCONNECT = 0x02  // Режим отключения
				};
				/**
				 * Режим событие клиента
				 */
				enum class event_t : uint8_t {
					REQUEST  = 0x01, // Режим запроса
					RESPONSE = 0x02  // Режим ответа
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOINFO   = 0x01, // Флаг запрещающий вывод информационных сообщений
					WAITMESS = 0x02  // Флаг ожидания входящих сообщений
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
					Core(const fmk_t * fmk, const log_t * log) noexcept : client(fmk, log), server(fmk, log) {}
				} core_t;
				/**
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Функция обратного вызова для обработки авторизации
					function <bool (const string &, const string &)> checkAuth;
					// Функция обратного вызова, при запуске или остановки подключения к серверу
					function <void (const size_t, const mode_t, ProxySocks5 *)> active;
					// Функция разрешения подключения адъютанта на сервере
					function <bool (const string &, const string &, const u_int, ProxySocks5 *)> accept;
					// Функция обратного вызова, при получении сообщения с сервера
					function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5 *)> message;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept : checkAuth(nullptr), active(nullptr), accept(nullptr), message(nullptr) {}
				} fn_t;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
				// unix-сокет сервера
				string _usock;
			private:
				// Объект для работы с сетью
				network_t _nwk;
			private:
				// Объект биндинга TCP/IP
				core_t _core;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего для сервера
				socks5_scheme_t _scheme;
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
				 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
				 * @param ip   адрес интернет подключения клиента
				 * @param mac  мак-адрес подключившегося клиента
				 * @param port порт подключившегося адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат разрешения к подключению клиента
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
				 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			public:
				/**
				 * init Метод инициализации WebSocket адъютанта
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * init Метод инициализации WebSocket клиента
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const mode_t, ProxySocks5 *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5 *)> callback) noexcept;
			public:
				/**
				 * on Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void on(function <bool (const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие активации клиента на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int, ProxySocks5 *)> callback) noexcept;
			public:
				/**
				 * serverName Метод добавления названия сервера
				 * @param name название сервера для добавления
				 */
				void serverName(const string & name = "") noexcept;
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
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
			public:
				/**
				 * close Метод закрытия подключения клиента
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
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
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
				/**
				 * signalInterception Метод активации перехвата сигналов
				 * @param mode флаг активации
				 */
				void signalInterception(const awh::core_t::signals_t mode) noexcept;
			public:
				/**
				 * ProxySocks5 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~ProxySocks5 Деструктор
				 */
				~ProxySocks5() noexcept {}
		} proxy_socks5_t;
	};
};

#endif // __AWH_PROXY_SOCKS5_SERVER__
