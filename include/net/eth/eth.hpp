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
 * \~russian
 * @brief Заголовочный файл модуля работы с сетевым уровнем Ethernet — класс Ethernet,
 *        объединяющий работу с сетевыми интерфейсами, шлюзами, маршрутами,
 *        пробросом портов и сокетами канального уровня
 *
 * \~english
 * @brief Header file of the module of working with the Ethernet network level — the Ethernet class,
 *        uniting the work with the network interfaces, the gateways, the routes,
 *        the port forwarding and the sockets of the link level
 *
 * \~
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
 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
 */
#if __linux__ || __FreeBSD__ || __sun
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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс для работы с сетевым уровнем Ethernet
	 *
	 * @details Сводит воедино средства работы с сетью на уровне системы - те, что
	 *          лежат ниже отдельного подключения и касаются машины целиком:
	 *          сетевые устройства и их настройки, адреса и маршруты, шлюзы,
	 *          проброс портов, свойства сокетов
	 *
	 *          Собственной работы класс не ведёт и служит лишь общей точкой
	 *          доступа: за каждое направление отвечает свой вложенный объект, к
	 *          которому и обращаются напрямую. Заведён он затем, чтобы движку
	 *          хватало одной ссылки вместо полудюжины
	 *
	 * @note Средства протокола с управлением потоком доступны лишь на Linux и
	 *       FreeBSD - на прочих системах соответствующего объекта попросту нет, и
	 *       обращение к нему не соберётся
	 *
	 * @warning Многое здесь меняет настройки **всей машины**, а не одного
	 *          подключения: поднятие устройства, правка маршрутов, проброс портов.
	 *          Права на такое нужны надзорные, а последствия переживают завершение
	 *          процесса - выставленное следует снимать за собой
	 *
	 * \~english
	 * @brief Class for working with the Ethernet network level
	 * @details Brings together the means of working with the network at the level of the system — those that
	 *          lie below a separate connection and concern the machine entirely:
	 *          the network devices and their settings, the addresses and the routes, the gateways,
	 *          the port forwarding, the properties of the sockets
	 *          The class performs no work of its own and serves only as a common point of
	 *          access: each direction is answered for by its own nested object, which
	 *          is addressed directly. It is started so that one reference would be enough
	 *          for the engine instead of half a dozen
	 * @note The means of the protocol with the flow control are available only on Linux and
	 *       FreeBSD — on the other systems the corresponding object is simply absent, and
	 *       an address to it will not be built
	 * @warning Much of what is here changes the settings of the **whole machine**, and not of one
	 *          connection: bringing a device up, correcting the routes, forwarding the ports.
	 *          The rights for such things are supervisory ones, and the consequences outlive the completion
	 *          of the process — what has been set should be removed after oneself
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Ethernet {
		public:
			// Объект работы с сетевыми адресами
			eth::addr_t addr;
			// Объект управления сетевым интерфейсом
			eth::iface_t iface;
			/**
			 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
			 */
			#if __linux__ || __FreeBSD__ || __sun
				// Объект управления протоколом передачи с управлением потоком
				eth::sctp_t sctp;
			#endif
			// Объект управления сокетами
			eth::socket_t socket;
			// Объект управления шлюзами
			eth::gateway_t gateway;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
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
			explicit Ethernet(const fmk_t * fmk, const log_t * log) noexcept;
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
			~Ethernet() noexcept;
	} eth_t;
};

#endif // __AWH_ETHERNET__
