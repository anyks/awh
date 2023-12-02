/**
 * @file: sample.hpp
 * @date: 2023-10-18
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

#ifndef __AWH_SAMPLE_SERVER__
#define __AWH_SAMPLE_SERVER__

/**
 * Стандартные модули
 */
#include <queue>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <core/server.hpp>
#include <scheme/sample.hpp>

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
		 * Sample Класс работы с SAMPLE сервером
		 */
		typedef class Sample {
			public:
				/**
				 * Режим работы брокера
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Метод подключения брокера
					DISCONNECT = 0x02  // Метод отключения брокера
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOT_INFO   = 0x01, // Флаг запрещающий вывод информационных сообщений
					WAIT_MESS  = 0x02, // Флаг ожидания входящих сообщений
					VERIFY_SSL = 0x03  // Флаг выполнения проверки сертификата SSL
				};
			private:
				// Идентификатор основного процесса
				pid_t _pid;
			private:
				// Флаг долгоживущего подключения
				bool _alive;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
			private:
				// Объект работы с URI ссылками
				uri_t _uri;
			private:
				// Ядро локальных таймеров
				core_t _timer;
			private:
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего
				scheme::sample_t _scheme;
			private:
				// Размер шифрования передаваемых данных
				hash_t::cipher_t _cipher;
			private:
				// Список отключившихся клиентов
				map <uint64_t, time_t> _disconnected;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const server::core_t * _core;
			private:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openCallback(const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * eventsCallback Метод обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с брокера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Метод обратного вызова при записи сообщение брокеру
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * acceptCallback Метод обратного вызова при проверке подключения брокера
				 * @param ip   адрес интернет подключения брокера
				 * @param mac  мак-адрес подключившегося брокера
				 * @param port порт подключившегося брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат разрешения к подключению брокера
				 */
				bool acceptCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * erase Метод удаления отключившихся клиентов
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				void erase(const uint16_t tid, awh::core_t * core) noexcept;
				/**
				 * pinging Метод таймера выполнения пинга клиента
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				void pinging(const uint16_t tid, awh::core_t * core) noexcept;
			public:
				/**
				 * init Метод инициализации Rest брокера
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * init Метод инициализации Rest брокера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const fn_t & callback) noexcept;
			public:
				/**
				 * response Метод отправки сообщения брокеру
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных для отправки
				 * @param size   размер бинарных данных для отправки
				 */
				void send(const uint64_t bid, const char * buffer, const size_t size) const noexcept;
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
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
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
				/**
				 * mode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
			public:
				/**
				 * Sample Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Sample(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Sample Деструктор
				 */
				~Sample() noexcept {}
		} sample_t;
	};
};

#endif // __AWH_SAMPLE_SERVER__
