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

#ifndef __AWH_SAMPLE_CLIENT__
#define __AWH_SAMPLE_CLIENT__

/**
 * Стандартные модули
 */
#include <stack>
#include <functional>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/hold.hpp>
#include <core/client.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Sample Класс работы с примером клиента
		 */
		typedef class Sample {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01,
					DISCONNECT = 0x02
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE      = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO   = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP   = 0x04, // Флаг запрета остановки биндинга
					WAIT_MESS  = 0x08, // Флаг ожидания входящих сообщений
					VERIFY_SSL = 0x10  // Флаг выполнения проверки сертификата SSL
				};
			private:
				/**
				 * Идентификаторы текущего события
				 */
				enum class event_t : uint8_t {
					NONE    = 0x00, // Событие не установлено
					OPEN    = 0x01, // Событие открытия подключения
					READ    = 0x02, // Событие чтения данных с сервера
					SEND    = 0x03, // Событие отправки данных на сервер
					CONNECT = 0x04  // Событие подключения к серверу
				};
			private:
				// Объект для работы с сетью
				net_t _net;
				// Хранилище функций обратного вызова
				fn_t _callbacks;
				// Объект сетевой схемы
				scheme_t _scheme;
			private:
				// Список рабочих событий
				stack <event_t> _events;
			private:
				// Идентификатор подключения
				uint64_t _bid;
			private:
				// Выполнять анбиндинг после завершения запроса
				bool _unbind;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const client::core_t * _core;
			private:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param sid идентификатор схемы сети
				 */
				void openCallback(const uint16_t sid) noexcept;
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 */
				void eventsCallback(const awh::core_t::status_t status) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void connectCallback(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void disconnectCallback(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
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
				 */
				void close() noexcept;
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
				void init(const u_int port, const string & host) noexcept;
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
					// Если функция обратного вызова передана
					if((idw > 0) && (fn != nullptr))
						// Выполняем установку функции обратного вызова
						this->_callbacks.set <A> (idw, fn);
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
					// Если функция обратного вызова передана
					if(!name.empty() && (fn != nullptr))
						// Выполняем установку функции обратного вызова
						this->_callbacks.set <A> (name, fn);
				}
			public:
				/**
				 * response Метод отправки сообщения брокеру
				 * @param buffer буфер бинарных данных для отправки
				 * @param size   размер бинарных данных для отправки
				 */
				void send(const char * buffer, const size_t size) noexcept;
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
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
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
				Sample(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Sample Деструктор
				 */
				~Sample() noexcept {}
		} sample_t;
	};
};

#endif // __AWH_SAMPLE_CLIENT__
