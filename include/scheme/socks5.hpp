/**
 * @file: socks5.hpp
 * @date: 2025-10-08
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

#ifndef __AWH_SCHEME_SOCKS5_SERVER__
#define __AWH_SCHEME_SOCKS5_SERVER__

/**
 * Стандартные модули
 */
#include <vector>
#include <atomic>
#include <unordered_map>

/**
 * Наши модули
 */
#include "client.hpp"
#include "server.hpp"
#include "../socks5/server.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief серверное пространство имён
	 *
	 */
	namespace server {
		/**
		 * @brief серверное пространство имён
		 *
		 */
		namespace scheme {
			/**
			 * @brief Структура схемы сети Socks5 сервера
			 *
			 */
			typedef struct AWH_SHARED_EXPORT Socks5 : public scheme_t {
				public:
					/**
					 * @brief Структура параметров активного клиента
					 *
					 */
					typedef struct Options {
						uint32_t id;              // Идентификатор активного клиента
						std::atomic_bool locked;  // Флаг блокировки обработки запроса
						std::atomic_bool connect; // Флаг выполненного подключения
						std::atomic_bool stopped; // Флаг принудительной остановки
						client::scheme_t scheme;  // Объект схемы сети клиента
						server::socks5_t socks5;  // Объект для работы с Socks5
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						 id(0), locked(false), connect(false),
						 stopped(false), scheme(fmk, log), socks5(log) {}
						/**
						 * @brief Деструктор
						 *
						 */
						~Options() noexcept {}
					} options_t;
				public:
					/**
					 * Тип данных для хранения опций активных клиентов
					 */
					using clients_t = std::unordered_map <uint32_t, std::unique_ptr <options_t>>;
				private:
					// Список параметров активных клиентов
					clients_t _clients;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * @brief Метод очистки
					 *
					 */
					void clear() noexcept;
				public:
					/**
					 * @brief Метод создания параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 */
					void set(const uint32_t bid) noexcept;
					/**
					 * @brief Метод удаления параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 */
					void rm(const uint32_t bid) noexcept;
					/**
					 * @brief Метод получения параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 * @return    параметры активного клиента
					 */
					const options_t * get(const uint32_t bid) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Socks5(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Socks5() noexcept {}
			} socks5_t;
		};
	};
};

#endif // __AWH_SCHEME_SOCKS5_SERVER__
