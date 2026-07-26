/**
 * @file: eth.hpp
 * @date: 2025-11-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля работы с сетевым уровнем Ethernet — класс Ethernet,
 *        объединяющий работу с сетевыми интерфейсами, шлюзами, маршрутами,
 *        пробросом портов и сокетами канального уровня
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ETHERNET__
#define __AWH_ETHERNET__

/**
 * Наши модули
 */
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * Для операционной системы Linux или FreeBSD
 */
#if __linux__ || __FreeBSD__
	/**
	 * Подключаем заголовочный модуль SCTP протокола
	 */
	#include "sctp.hpp"
#endif

/**
 * Подключаем заголовочный модуль работы с сетевым интерфейсом
 */
#include "addr.hpp"
#include "iface.hpp"
#include "socket.hpp"
#include "gateway.hpp"
#include "portmap.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс для работы с сетевым уровнем Ethernet
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Ethernet {
		public:
			// Объект работы с сетевыми адресами
			eth::addr_t addr;
			// Объект управления сетевым интерфейсом
			eth::iface_t iface;
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				// Объект управления протоколом передачи с управлением потоком
				eth::sctp_t sctp;
			#endif
			// Объект управления сокетами
			eth::socket_t socket;
			// Объект управления шлюзами
			eth::gateway_t gateway;
			// Объект управления пробросом портов
			eth::portmap_t portmap;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 *
			 */
			explicit Ethernet(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Ethernet() noexcept;
	} eth_t;
};

#endif // __AWH_ETHERNET__
