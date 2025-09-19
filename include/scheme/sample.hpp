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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_SCHEME_SAMPLE_SERVER__
#define __AWH_SCHEME_SAMPLE_SERVER__

/**
 * Стандартные модули
 */
#include <map>
#include <vector>

/**
 * Наши модули
 */
#include "server.hpp"
#include "../http/server.hpp"

/**
 * @brief пространство имён
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
			 * @brief Структура схемы сети SAMPLE сервера
			 *
			 */
			typedef struct AWHSHARED_EXPORT Sample : public scheme_t {
				public:
					/**
					 * @brief Структура флагов разрешения обменом данных
					 *
					 */
					typedef struct Allow {
						bool send;    // Флаг разрешения отправки данных
						bool receive; // Флаг разрешения чтения данных
						/**
						 * @brief Конструктор
						 *
						 */
						Allow() noexcept : send(true), receive(true) {}
					} allow_t;
				public:
					/**
					 * @brief Структура опций активного клиента
					 *
					 */
					typedef struct Options {
						bool alive;    // Флаг долгоживущего подключения
						bool close;    // Флаг требования закрыть брокера
						bool stopped;  // Флаг принудительной остановки
						allow_t allow; // Объект разрешения обмена данными
						/**
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						 alive(false), close(false), stopped(false) {}
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
					typedef std::map <uint64_t, std::unique_ptr <options_t>> clients_t;
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
					void set(const uint64_t bid) noexcept;
					/**
					 * @brief Метод удаления параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 */
					void rm(const uint64_t bid) noexcept;
				public:
					/**
					 * @brief Метод извлечения списка параметров активных клиентов
					 *
					 * @return список параметров активных клиентов
					 */
					const clients_t & get() const noexcept;
					/**
					 * @brief Метод получения параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 * @return    параметры активного клиента
					 */
					const options_t * get(const uint64_t bid) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Sample(const fmk_t * fmk, const log_t * log) noexcept :
					 scheme_t(fmk, log), _fmk(fmk), _log(log) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~Sample() noexcept {}
			} sample_t;
		};
	};
};

#endif // __AWH_SCHEME_SAMPLE_SERVER__
