/**
 * @file: server.hpp
 * @date: 2022-09-03
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

#ifndef __AWH_SCHEME_SERVER__
#define __AWH_SCHEME_SERVER__

/**
 * Наши модули
 */
#include "core.hpp"
#include "../net/engine.hpp"

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
		 * @brief Структура схемы сети сервера
		 *
		 */
		typedef struct AWHSHARED_EXPORT Scheme : public awh::scheme_t {
			private:
				/**
				 * @brief Core Устанавливаем дружбу с серверным классом ядра
				 *
				 */
				friend class Core;
			protected:
				// Хост сервера
				string _host;
				// Порт сервера
				uint32_t _port;
			protected:
				// Максимальное количество одновременных подключений
				uint32_t _total;
			protected:
				// Контекст двигателя для работы с передачей данных
				engine_t::ctx_t _ectx;
				// Объект подключения
				engine_t::addr_t _addr;
			public:
				/**
				 * @brief Метод очистки
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Scheme(const fmk_t * fmk, const log_t * log) noexcept :
				 awh::scheme_t(fmk, log),
				 _host(SERVER_HOST), _port(SERVER_PORT),
				 _total(SERVER_TOTAL_CONNECT),
				 _ectx(fmk, log), _addr(fmk, log) {}
				/**
				 * @brief Деструктор
				 *
				 */
				~Scheme() noexcept {}
		} scheme_t;
	};
};

#endif // __AWH_SCHEME_SERVER__
