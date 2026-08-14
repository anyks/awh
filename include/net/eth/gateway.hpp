/**
 * @file gateway.hpp
 * @date 2026-01-28
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
 * @brief Заголовочный файл модуля работы со шлюзами — класс eth::Gateway для получения таблицы маршрутизации,
 *        определения шлюза по умолчанию и разбора параметров маршрута на всех поддерживаемых операционных системах
 *
 * \~english
 * @brief Header file of the module of working with the gateways — the eth::Gateway class for getting the routing table,
 *        determining the default gateway and parsing the parameters of a route on all the supported operating systems
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
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
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён Ethernet протоколов
	 *
	 * \~english
	 * @brief Namespace of the Ethernet protocols
	 *
	 * \~
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс для работы с шлюзами
		 *
		 * @details Ведает таблицей маршрутов - той, по которой система решает, куда
		 * отправить пакет для заданного адреса. Позволяет узнать нужный
		 * маршрут, а также добавить свой и убрать его
		 *
		 * Нужно это тем, кто сам прокладывает пути: туннелям, которым надо
		 * завернуть на себя часть движения, и приложениям, выбирающим
		 * исходящее устройство осознанно
		 *
		 * @warning Правка таблицы меняет маршрутизацию **всей машины**, требует
		 * надзорных прав и переживает завершение процесса. Добавленный маршрут
		 * следует убирать за собой - иначе он останется и после выхода, а
		 * ошибочный способен отрезать машину от сети
		 *
		 * \~english
		 * @brief Class for working with the gateways
		 * @details Is in charge of the table of the routes — the one by which the system decides where
		 * to send a packet for the given address. Allows the needed route to be found out,
		 * as well as one's own to be added and removed
		 * This is needed by those who lay the paths themselves: by the tunnels that need
		 * to turn a part of the traffic onto themselves, and by the applications choosing
		 * the outgoing device deliberately
		 * @warning A correction of the table changes the routing of the **whole machine**, requires
		 * supervisory rights and outlives the completion of the process. An added route
		 * should be removed after oneself — otherwise it will remain after the exit as well, and
		 * an erroneous one is capable of cutting the machine off the network
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Gateway {
			public:
				/**
				 * \~russian
				 * @brief Структура маршрута
				 *
				 * @details Правило вида «пакеты для такой-то сети слать туда-то»: сеть
				 * задаётся адресом назначения вместе с длиной префикса, а путь -
				 * шлюзом либо устройством
				 *
				 * @note При получении маршрута заполнять следует адрес назначения -
				 * остальное дозаполнит сама система, отвечая, как она этот адрес
				 * маршрутизирует
				 *
				 * @note Шлюз нужен не всякому маршруту: сеть, до которой машина
				 * достаёт напрямую, задаётся одним устройством, без него
				 *
				 * \~english
				 * @brief Structure of a route
				 * @details A rule of the kind «the packets for such-and-such a network should be sent there-and-there»: the network
				 * is set by the address of the destination together with the length of the prefix, and the path —
				 * by a gateway or by a device
				 * @note At the getting of a route the address of the destination should be filled —
				 * the rest will be filled up by the system itself, answering how it
				 * routes that address
				 * @note A gateway is needed not by every route: a network the machine
				 * reaches directly is set by a device alone, without it
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Route {
					// Сетевой интерфейс
					string ifname;
					// Префикс сети
					uint8_t prefix;
					// Шлюз маршрута
					unique_ptr <net::addr_t> gateway;
					// Адрес назначения
					unique_ptr <net::addr_t> destination;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Route() noexcept;
				} route_t;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод получения маршрута для указанного адреса
				 *
				 * @details Спрашивает у системы, каким путём она отправит пакет для
				 * заданного адреса, и дозаполняет объект маршрута ответом
				 *
				 * @note Довод здесь и входной, и выходной: адрес назначения задаёт
				 * спрашивающий, а устройство, шлюз и префикс проставляет система
				 *
				 * @param route объект для извлечения маршрута
				 * @return      результат получения маршрута
				 *
				 * \~english
				 * @brief Method of getting the route for the specified address
				 * @details Asks the system by which path it will send a packet for
				 * the given address, and fills up the route object with the answer
				 * @note The argument here is both an input and an output one: the address of the destination is set by
				 * the asking side, and the device, the gateway and the prefix are put down by the system
				 * @param route object to extract the route into
				 * @return      result of the getting of the route
				 *
				 * \~
				 */
				bool get(route_t & route) const noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления маршрута
				 *
				 * @details Добавляет правило в таблицу маршрутов системы
				 *
				 * @warning Действует на всю машину и требует надзорных прав. Правило
				 * переживает завершение процесса, поэтому добавленное следует убирать
				 * методом удаления
				 *
				 * @param route объект маршрута для добавления
				 * @return      результат добавления маршрута
				 *
				 * \~english
				 * @brief Method of adding a route
				 * @details Adds a rule into the table of the routes of the system
				 * @warning Is in force for the whole machine and requires supervisory rights. The rule
				 * outlives the completion of the process, and therefore what has been added should be removed
				 * by the method of the removal
				 * @param route object of the route to add
				 * @return      result of the addition of the route
				 *
				 * \~
				 */
				bool add(const route_t & route) const noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления маршрута
				 *
				 * @details Убирает правило из таблицы маршрутов системы
				 *
				 * @note Правило отыскивается по адресу назначения и префиксу, поэтому
				 * объект должен описывать тот же маршрут, что добавлялся
				 *
				 * @param route объект маршрута для удаления
				 * @return      результат удаления маршрута
				 *
				 * \~english
				 * @brief Method of removing a route
				 * @details Removes a rule from the table of the routes of the system
				 * @note The rule is found by the address of the destination and by the prefix, and therefore
				 * the object must describe the same route that was added
				 * @param route object of the route to remove
				 * @return      result of the removal of the route
				 *
				 * \~
				 */
				bool remove(const route_t & route) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Gateway(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Gateway() noexcept;
		} gateway_t;
	};
};

#endif // __AWH_GATEWAY__
