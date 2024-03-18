/**
 * @file: core.hpp
 * @date: 2024-03-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_CORE__
#define __AWH_CORE__

/**
 * Стандартные модули
 */
#include <mutex>
#include <chrono>
#include <string>
#include <cstdlib>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>

/**
 * Если операционной системой является Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Подключаем заголовочный файл
	#include <tchar.h>
	#include <synchapi.h>
/**
 * Для всех остальных операционных систем
 */
#else
	// Подключаем заголовочный файл
	#include <unistd.h>
#endif

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <scheme/core.hpp>
#include <sys/signals.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс ядра биндинга TCP/IP
	 */
	typedef class Core {
		private:
			/**
			 * Scheme Устанавливаем дружбу с схемой сети
			 */
			friend class Scheme;
		public:
			/**
			 * Статус работы сетевого ядра
			 */
			enum class status_t : uint8_t {
				STOP  = 0x02, // Статус остановки
				START = 0x01  // Статус запуска
			};
		protected:
			/**
			 * Mutex Объект основных мютексов
			 */
			typedef struct Mutex {
				// Для работы с параметрами модуля
				recursive_mutex main;
				// Для работы с биндингом сетевых ядер
				recursive_mutex bind;
				// Для контроля запуска модуля
				recursive_mutex status;
			} mtx_t;
		private:
			/**
			 * Dispatch Класс работы с событиями
			 */
			typedef class Dispatch {
				private:
					// Флаг простого чтения базы событий
					bool _easy;
					// Флаг работы модуля
					bool _work;
					// Флаг инициализации базы событий
					bool _init;
					// Флаг виртуальной базы данных
					bool _virt;
					// Флаг заморозки получения данных
					bool _freeze;
				private:
					// Мютекс для блокировки потока
					recursive_mutex _mtx;
				private:
					// Частота обновления базы событий
					chrono::milliseconds _freq;
				public:
					// База данных событий
					struct event_base * base;
				private:
					// Функция обратного вызова при запуске модуля
					function <void (const bool, const bool)> _launching;
					// Функция обратного вызова при остановки модуля
					function <void (const bool, const bool)> _closedown;
				private:
					// Создаём объект работы с логами
					const log_t * _log;
				private:
					/**
					 * Если операционной системой является Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Объект данных запроса
						WSADATA _wsaData;
						// Флаг инициализации WinSocksAPI
						bool _winSockInit;
					#endif
				public:
					/**
					 * kick Метод отправки пинка
					 */
					void kick() noexcept;
					/**
					 * stop Метод остановки чтения базы событий
					 */
					void stop() noexcept;
					/**
					 * start Метод запуска чтения базы событий
					 */
					void start() noexcept;
				public:
					/**
					 * virt Метод активации работы базы событий как виртуальной
					 * @param mode флаг активации
					 */
					void virt(const bool mode) noexcept;
				public:
					/**
					 * rebase Метод пересоздания базы событий
					 */
					void rebase() noexcept;
					/**
					 * freeze Метод заморозки чтения данных
					 * @param mode флаг активации
					 */
					void freeze(const bool mode) noexcept;
					/**
					 * easily Метод активации простого режима чтения базы событий
					 * @param mode флаг активации
					 */
					void easily(const bool mode) noexcept;
				public:
					/**
					 * frequency Метод установки частоты обновления базы событий
					 * @param msec частота обновления базы событий в миллисекундах
					 */
					void frequency(const uint8_t msec = 10) noexcept;
				public:
					/**
					 * on Метод установки функции обратного вызова
					 * @param status   статус которому соответствует функция
					 * @param callback функция обратного вызова
					 */
					void on(const status_t status, function <void (const bool, const bool)> callback) noexcept;
				public:
					/**
					 * Dispatch Конструктор
					 * @param log объект для работы с логами
					 */
					Dispatch(const log_t * log) noexcept;
					/**
					 * ~Dispatch Деструктор
					 */
					~Dispatch() noexcept;
			} dispatch_t;
		protected:
			// Идентификатор процесса
			pid_t _pid;
		protected:
			// Флаг разрешения работы
			bool _mode;
			// Флаг разрешающий вывод информационных сообщений
			bool _verb;
		private:
			// Количество подключённых внешних ядер
			uint32_t _cores;
		protected:
			// Хранилище функций обратного вызова
			fn_t _callbacks;
			// Объект для работы с чтением базы событий
			dispatch_t _dispatch;
		private:
			// Объект работы с сигналами
			sig_t _sig;
		protected:
			// Мютекс для блокировки основного потока
			mtx_t _mtx;
		protected:
			// Статус сетевого ядра
			status_t _status;
			// Тип запускаемого ядра
			engine_t::type_t _type;
			// Флаг обработки сигналов
			scheme_t::mode_t _signals;
		protected:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * signal Метод вывода полученного сигнала
			 */
			void signal(const int signal) noexcept;
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
		public:
			/**
			 * bind Метод подключения модуля ядра к текущей базе событий
			 * @param core модуль ядра для подключения
			 */
			void bind(Core * core) noexcept;
			/**
			 * unbind Метод отключения модуля ядра от текущей базы событий
			 * @param core модуль ядра для отключения
			 */
			void unbind(Core * core) noexcept;
		public:
			/**
			 * stop Метод остановки клиента
			 */
			virtual void stop() noexcept;
			/**
			 * start Метод запуска клиента
			 */
			virtual void start() noexcept;
		protected:
			/**
			 * launching Метод вызова при активации базы событий
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			virtual void launching(const bool mode, const bool status) noexcept;
			/**
			 * closedown Метод вызова при деакцтивации базы событий
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			virtual void closedown(const bool mode, const bool status) noexcept;
		public:
			/**
			 * callbacks Метод установки функций обратного вызова
			 * @param callbacks функции обратного вызова
			 */
			virtual void callbacks(const fn_t & callbacks) noexcept;
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
			 * working Метод проверки на запуск работы
			 * @return результат проверки
			 */
			bool working() const noexcept;
		public:
			/**
			 * easily Метод активации простого режима чтения базы событий
			 * @param mode флаг активации простого чтения базы событий
			 */
			void easily(const bool mode = true) noexcept;
			/**
			 * freeze Метод заморозки чтения данных
			 * @param mode флаг активации заморозки чтения данных
			 */
			void freeze(const bool mode = true) noexcept;
			/**
			 * verbose Метод установки флага запрета вывода информационных сообщений
			 * @param mode флаг запрета вывода информационных сообщений
			 */
			void verbose(const bool mode = true) noexcept;
		public:
			/**
			 * frequency Метод установки частоты обновления базы событий
			 * @param msec частота обновления базы событий в миллисекундах
			 */
			void frequency(const uint8_t msec = 10) noexcept;
			/**
			 * signalInterception Метод активации перехвата сигналов
			 * @param mode флаг активации
			 */
			void signalInterception(const scheme_t::mode_t mode = scheme_t::mode_t::DISABLED) noexcept;
		public:
			/**
			 * Core Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Core(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Core Деструктор
			 */
			virtual ~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
