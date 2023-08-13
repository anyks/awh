/**
 * @file: sample.hpp
 * @date: 2022-09-01
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

#ifndef __AWH_SAMPLE_SERVER__
#define __AWH_SAMPLE_SERVER__

/**
 * Стандартные модули
 */
#include <queue>

/**
 * Наши модули
 */
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
				 * Режим работы адъютанта
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Метод подключения адъютанта
					DISCONNECT = 0x02  // Метод отключения адъютанта
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
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Функция обратного вызова, при запуске или остановки подключения к серверу
					function <void (const size_t, const mode_t, Sample *)> active;
					// Функция обратного вызова, при получении сообщения с сервера
					function <void (const size_t, const vector <char> &, Sample *)> message;
					// Функция разрешения подключения адъютанта на сервере
					function <bool (const string &, const string &, const u_int, Sample *)> accept;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept : active(nullptr), message(nullptr), accept(nullptr) {}
				} fn_t;
			private:
				// Идентификатор основного процесса
				pid_t _pid;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
			private:
				// Объект работы с URI ссылками
				uri_t _uri;
			private:
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего
				sample_scheme_t _scheme;
			private:
				// Размер шифрования передаваемых данных
				hash_t::cipher_t _cipher;
			private:
				// Флаг долгоживущего подключения
				bool _alive;
			private:
				// Список отключившихся адъютантов
				queue <size_t> _closed;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const server::core_t * _core;
			private:
				/**
				 * openCallback Функция обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openCallback(const size_t sid, awh::core_t * core) noexcept;
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * connectCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Функция обратного вызова при чтении сообщения с адъютанта
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Функция обратного вызова при записи сообщение адъютанту
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
				 * @param ip   адрес интернет подключения адъютанта
				 * @param mac  мак-адрес подключившегося адъютанта
				 * @param port порт подключившегося адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат разрешения к подключению адъютанта
				 */
				bool acceptCallback(const string & ip, const string & mac, const u_int port, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * handler Метод управления входящими методами
				 * @param aid идентификатор адъютанта
				 */
				void handler(const size_t aid) noexcept;
			private:
				/**
				 * actionRead Метод обработки экшена чтения с сервера
				 * @param aid идентификатор адъютанта
				 */
				void actionRead(const size_t aid) noexcept;
				/**
				 * actionConnect Метод обработки экшена подключения к серверу
				 * @param aid идентификатор адъютанта
				 */
				void actionConnect(const size_t aid) noexcept;
				/**
				 * actionDisconnect Метод обработки экшена отключения от сервера
				 * @param aid идентификатор адъютанта
				 */
				void actionDisconnect(const size_t aid) noexcept;
			private:
				/**
				 * remove Метод удаления отключившихся адъютантов
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				void remove(const u_short tid, awh::core_t * core) noexcept;
			public:
				/**
				 * init Метод инициализации Rest адъютанта
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * init Метод инициализации Rest адъютанта
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const mode_t, Sample *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const vector <char> &, Sample *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int, Sample *)> callback) noexcept;
			public:
				/**
				 * response Метод отправки сообщения адъютанту
				 * @param aid    идентификатор адъютанта
				 * @param buffer буфер бинарных данных для отправки
				 * @param size   размер бинарных данных для отправки
				 */
				void send(const size_t aid, const char * buffer, const size_t size) const noexcept;
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
				 * alive Метод установки долгоживущего подключения
				 * @param aid  идентификатор адъютанта
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const size_t aid, const bool mode) noexcept;
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
