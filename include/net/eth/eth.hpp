/**
 * @file eth.hpp
 * @date 2025-11-06
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля работы с сетевым уровнем Ethernet — семейный заголовок,
 *        объединяющий работу с сетевыми интерфейсами, шлюзами, маршрутами,
 *        пробросом портов и сокетами канального уровня
 *
 * \~english
 * @brief Header file of the module of working with the Ethernet network level — the family header,
 *        uniting the work with the network interfaces, the gateways, the routes,
 *        the port forwarding and the sockets of the link level
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и OpenIndiana
 */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__sun) || defined(DOXYGEN)
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
#include "../../sys/macro/global.hpp"
