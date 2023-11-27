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
#include <sys/fn.hpp>
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
		 * ProxySocks5 Класс работы с SOCKS5 сервером
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
					NOT_INFO  = 0x01, // Флаг запрещающий вывод информационных сообщений
					WAIT_MESS = 0x02  // Флаг ожидания входящих сообщений
				};
			private:
				/**
				 * Core Объект биндинга TCP/IP
				 */
				typedef struct Core {
					dns_t          dns;    // Объект DNS-резолвера
					core_t         timer;  // Объект биндинга TCP/IP для таймера
					client::core_t client; // Объект биндинга TCP/IP для клиента
					server::core_t server; // Объект биндинга TCP/IP для сервера
					/**
					 * Core Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Core(const fmk_t * fmk, const log_t * log) noexcept :
					 dns(fmk, log), timer(fmk, log), client(&dns, fmk, log), server(&dns, fmk, log) {}
				} core_t;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
				// unix-сокет сервера
				string _socket;
			private:
				// Объект для работы с сетью
				net_t _net;
				// Объект биндинга TCP/IP
				core_t _core;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего для сервера
				scheme::socks5_t _scheme;
			private:
				// Список отключившихся клиентов
				map <uint64_t, time_t> _disconnected;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
			private:
				/**
				 * openServerCallback Функция обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openServerCallback(const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
				/**
				 * connectClientCallback Функция обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectClientCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * connectServerCallback Функция обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectServerCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectClientCallback Функция обратного вызова при отключении от сервера
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectClientCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectServerCallback Функция обратного вызова при отключении от сервера
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectServerCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
				 * @param ip   адрес интернет подключения клиента
				 * @param mac  мак-адрес подключившегося клиента
				 * @param port порт подключившегося брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат разрешения к подключению клиента
				 */
				bool acceptServerCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readClientCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readServerCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeServerCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * erase Метод удаления отключённых клиентов
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				void erase(const uint16_t tid, awh::core_t * core) noexcept;
			public:
				/**
				 * init Метод инициализации брокера
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * init Метод инициализации брокера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const u_int port = 1080, const string & host = "") noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const mode_t)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const uint64_t, const event_t, const char *, const size_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие активации клиента на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для обработки авторизации
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept;
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
				 * close Метод закрытия подключения
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
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
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
			public:
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
				/**
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const uint16_t size = 0) noexcept;
			public:
				/**
				 * mode Метод установки флагов модуля
				 * @param flags список флагов модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
				/**
				 * ciphers Метод установки алгоритмов шифрования
				 * @param ciphers список алгоритмов шифрования для установки
				 */
				void ciphers(const vector <string> & ciphers) noexcept;
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
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const uint64_t bid, const string & read, const string & write) noexcept;
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
				 * signalInterception Метод активации перехвата сигналов
				 * @param mode флаг активации
				 */
				void signalInterception(const awh::core_t::mode_t mode) noexcept;
			public:
				/**
				 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
				 * @param mode флаг состояния разрешения проверки
				 */
				void verifySSL(const bool mode) noexcept;
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
