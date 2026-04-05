/**
 * @file: client.hpp
 * @date: 2026-04-05
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Наши модули
 */
#include "../net/tls.hpp"
#include "../units/dns.hpp"
#include "../units/client.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс клиента
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Client {
		protected:
			// Идентификатор TLS для выполнения запросов к серверу
			tls_t::id_t _tid;
			// Идентификатор клиента для выполнения запросов к серверу
			event::id_t _eid;
		protected:
			// Функция обратного вызова для обработки клиента
			callback_t _callback;
		protected:
			// Объект транспортного уровня безопасности
			tls_t * _tls;
			// Объект DNS-резолвера
			unit::dns_t * _dns;
			// Объект клиента
			unit::client_t * _client;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод остановки клиента
			 *
			 */
			virtual void stop() noexcept;
			/**
			 * @brief Метод запуска клиента
			 *
			 */
			virtual void start() noexcept;
		public:
			/**
			 * @brief Метод приостановки работы клиента
			 *
			 * @return результат выполнения приостановки работы
			 */
			virtual bool pause() noexcept;
			/**
			 * @brief Метод возобновления работы клиента
			 *
			 * @return результат выполнения возобновления работы
			 */
			virtual bool resume() noexcept;
		public:
			/**
			 * @brief Метод мультиподключения клиентов к удалённым хостам
			 *
			 * @return результат выполнения подключения
			 */
			virtual bool connect() noexcept;
			/**
			 * @brief Метод отключения клиента от удалённого сервера
			 *
			 * @return результат выполнения отключения
			 */
			virtual bool disconnect() noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 */
			virtual void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 */
			virtual void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Метод отправки данных серверу
			 *
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных серверу
			 */
			virtual size_t send(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(string_view name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
				// Если мы получили идентификатор функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, Args... args) noexcept -> uint32_t {
				// Если мы получили на вход число
				if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @brief Метод установки идентификатора события клиента
			 *
			 * @param eid идентификатор события для установки
			 */
			void setEventId(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод установки идентификатора TLS
			 *
			 * @param tid идентификатор TLS для установки
			 */
			void setSecurityId(const tls_t::id_t tid) noexcept;
		private:
			/**
			 * @brief Конструктор копирования (запрещаем)
			 *
			 */
			Client(const Client &) = delete;
			/**
			 * @brief Оператор копирования (запрещаем)
			 *
			 * @return текущее значение объекта
			 */
			Client & operator = (const Client &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param tls    объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, tls_t * tls, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param dns    объект DNS-резолвера
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param dns    объект DNS-резолвера
			 * @param tls    объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, unit::dns_t * dns, tls_t * tls, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Client() noexcept;
	} client_t;
};

#endif // __AWH_CLIENT__
