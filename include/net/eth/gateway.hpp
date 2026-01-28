/**
 * @file: gateway.hpp
 * @date: 2026-01-28
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

#ifndef __AWH_GATEWAY__
#define __AWH_GATEWAY__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

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
	 * @brief Класс для работы с шлюзами
	 *
	 */
	typedef class AWH_SHARED_EXPORT Gateway {
		public:
			/**
			 * @brief Структура маршрута
			 *
			 */
			typedef struct Route {
				// Сетевой интерфейс
				string ifname;
				// Префикс сети
				uint8_t prefix;
				// Адрес назначения
				unique_ptr <net::addr_t> dest;
				// Шлюз
				unique_ptr <net::addr_t> gateway;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Route() noexcept :
				 ifname{""}, prefix(0),
				 dest(nullptr), gateway(nullptr) {}
			} route_t;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод получения маршрута для указанного адреса
			 *
			 * @param route объект для извлечения маршрута
			 * @return      результат получения маршрута
			 */
			bool get(route_t & route) const noexcept;
			/**
			 * @brief Метод добавления маршрута
			 *
			 * @param route объект маршрута для добавления
			 * @return      результат добавления маршрута
			 */
			bool add(const route_t & route) const noexcept;
			/**
			 * @brief Метод удаления маршрута
			 *
			 * @param route объект маршрута для удаления
			 * @return      результат удаления маршрута
			 */
			bool remove(const route_t & route) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Gateway(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Gateway() noexcept {}
	} gateway_t;
};

#endif // __AWH_GATEWAY__
