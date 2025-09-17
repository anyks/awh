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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_CORE__
#define __AWH_CORE__

/**
 * Стандартные модули
 */
#include <mutex>
#include <string>
#include <cstdlib>

/**
 * Наши модули
 */
#include "../scheme/core.hpp"
#include "../sys/signals.hpp"
#include "../sys/callback.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Core Класс ядра биндинга TCP/IP
	 */
	typedef class AWHSHARED_EXPORT Core {
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
				std::recursive_mutex main;
				// Для работы с биндингом сетевых ядер
				std::recursive_mutex bind;
				// Для контроля запуска модуля
				std::recursive_mutex status;
			} mtx_t;
		private:
			/**
			 * Dispatch Класс работы с событиями
			 */
			typedef class AWHSHARED_EXPORT Dispatch {
				private:
					// Идентификатор процесса
					pid_t _pid;
				private:
					// Мютекс для блокировки потока
					std::mutex _mtx;
				private:
					// Флаг работы модуля
					std::atomic_bool _work;
					// Флаг инициализации базы событий
					std::atomic_bool _init;
					// Флаг виртуальной базы данных
					std::atomic_bool _virt;
				private:
					// Функция обратного вызова при запуске модуля
					function <void (const bool, const bool)> _launching;
					// Функция обратного вызова при остановки модуля
					function <void (const bool, const bool)> _closedown;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
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
					 * rebase Метод пересоздания базы событий
					 */
					void rebase() noexcept;
					/**
					 * reinit Метод реинициализации базы событий
					 */
					void reinit() noexcept;
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
					 * baserate Метод установки времени блокировки базы событий в ожидании событий
					 * @param msec время ожидания событий в миллисекундах
					 */
					void baserate(const uint8_t msec = 10) noexcept;
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
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Dispatch(const fmk_t * fmk, const log_t * log) noexcept;
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
			bool _info;
		protected:
			// Объект для работы с чтением базы событий
			dispatch_t _dispatch;
			// Хранилище функций обратного вызова
			callback_t _callback;
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
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * signal Метод вывода полученного сигнала
			 */
			void signal(const int32_t signal) noexcept;
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
			/**
			 * reinit Метод реинициализации базы событий
			 */
			void reinit() noexcept;
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
			 * kick Метод отправки пинка
			 */
			void kick() noexcept;
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
			 * callback Метод установки функций обратного вызова
			 * @param callback функции обратного вызова
			 */
			virtual void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param A    тип идентификатора функции
			 * @param B    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, Args... args) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * working Метод проверки на запуск работы
			 * @return результат проверки
			 */
			bool working() const noexcept;
		public:
			/**
			 * eventBase Метод получения базы событий
			 * @return инициализированная база событий
			 */
			base_t * eventBase() noexcept;
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
			 * baserate Метод установки времени блокировки базы событий в ожидании событий
			 * @param msec время ожидания событий в миллисекундах
			 */
			void baserate(const uint8_t msec = 10) noexcept;
			/**
			 * signalInterception Метод активации перехвата сигналов
			 * @param mode флаг активации
			 */
			void signalInterception(const scheme_t::mode_t mode = scheme_t::mode_t::DISABLED) noexcept;
		public:
			/**
			 * sendUpstream Метод отправки сообщения между потоками
			 * @param sock сокет межпотокового передатчика
			 * @param tid  идентификатор трансферной передачи
			 */
			void sendUpstream(const SOCKET sock, const uint64_t tid) noexcept;
			/**
			 * deactivationUpstream Метод деактивации межпотокового передатчика
			 * @param sock сокет межпотокового передатчика
			 */
			void deactivationUpstream(const SOCKET sock) noexcept;
			/**
			 * activationUpstream Метод активации межпотокового передатчика
			 * @param callback функция обратного вызова
			 * @return         сокет межпотокового передатчика
			 */
			SOCKET activationUpstream(function <void (const uint64_t)> callback) noexcept;
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
