/**
 * @file netlink.hpp
 * @date 2026-08-06
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
 * @brief Заголовочный файл модуля опроса ядра Linux через netlink — единая выборка
 *        сведений о сетевых устройствах, адресах, маршрутах и соседях канального уровня
 *
 * @details Сведения эти у BSD читаются вызовом `sysctl` с ветвью `PF_ROUTE`: ядро
 *          отдаёт таблицу целиком одним куском памяти. У Linux устроено иначе - там
 *          заведён отдельный сокет к ядру, `AF_NETLINK`, в него отправляется запрос, а
 *          ответ приходит **потоком сообщений**, который может не поместиться в один
 *          приём и завершается особым сообщением конца
 *
 *          Модуль этот скрывает переписку целиком: отправку запроса, сбор ответа по
 *          частям, разбор заголовков и признание конца выборки. Наружу выдаётся одно -
 *          обход готовых сообщений
 *
 * @par Намеренные решения
 *
 *      **Своя переписка, а не библиотека libmnl или libnl.** Обе решают ту же задачу и
 *      обе пришлось бы тянуть внешней зависимостью в набор, который её не требует:
 *      нужна здесь одна выборка с обходом, а не полное управление подсистемой сети.
 *      Разбор же сообщений ведут макросы `NLMSG_*` и `RTA_*` из заголовков самого ядра,
 *      и они доступны без всякой библиотеки
 *
 *      **Обход через обратный вызов, а не выдача набора.** Выборка бывает крупной -
 *      таблица маршрутов машины с многими устройствами измеряется тысячами записей, - и
 *      складывать её целиком лишь затем, чтобы найти одну строку, значило бы взять
 *      память под то, что тут же и выбросят. Обратный вызов волен прервать обход, едва
 *      нашёл искомое
 *
 * @warning Ответ ядра приходит потоком, и обрывать приём на первом же сообщении с
 *          признаком конца выборки нельзя: ядро вправе прислать несколько потоков
 *          подряд, и остаток непрочитанного достанется следующему запросу по этому же
 *          сокету. Модуль дочитывает выборку до конца всегда, даже когда обратный вызов
 *          прервал обход раньше
 *
 * \~english
 * @brief Header file of the module of polling the Linux kernel through netlink — a single dump
 *        of the information about the network devices, the addresses, the routes and the neighbours of the link level
 * @details At BSD this information is read by a `sysctl` call with the `PF_ROUTE` branch: the kernel
 *          gives back the table entirely as one piece of memory. At Linux it is arranged otherwise — there
 *          a separate socket to the kernel is started, `AF_NETLINK`, a request is sent into it, and
 *          the answer comes as a **stream of messages**, which may not fit into one
 *          reception and ends with a special message of the end
 *          This module hides the correspondence entirely: the sending of the request, the gathering of the answer in
 *          parts, the parsing of the headers and the recognition of the end of the dump. Outwards one thing is given —
 *          the traversal of the ready messages
 * @par Deliberate decisions
 *      **Own correspondence, and not the libmnl or libnl library.** Both solve the same task and
 *      both would have to be dragged in as an external dependency into a set that does not require it:
 *      what is needed here is one dump with a traversal, and not a full management of the network subsystem.
 *      The parsing of the messages, though, is performed by the `NLMSG_*` and `RTA_*` macros from the headers of the kernel itself,
 *      and they are available without any library
 *      **The traversal through a callback, and not the issuing of a set.** A dump happens to be a large one —
 *      the table of the routes of a machine with many devices is measured in thousands of records, — and
 *      to fold it entirely only to find one line would mean taking the memory for what will be thrown away right away. The callback is free to interrupt the traversal as soon as it has
 *      found what it looked for
 * @warning The answer of the kernel comes as a stream, and it is not allowed to break off the reception at the very first message with
 *          the sign of the end of the dump: the kernel is free to send several streams
 *          in a row, and the remainder of the unread will go to the next request over the same
 *          socket. The module reads the dump to the end always, even when the callback
 *          has interrupted the traversal earlier
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GNU_NETLINK__
#define __AWH_GNU_NETLINK__

/**
 * Модуль предназначен только для операционной системы Linux
 */
#if __linux__

/**
 * Стандартная библиотека
 */
#include <cstdint>
#include <string>
#include <functional>

/**
 * Системные заголовочные файлы
 */
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/**
 * Наши модули
 */
#include "../../../sys/log.hpp"

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
	 * @brief Пространство имён средств операционной системы Linux
	 *
	 * \~english
	 * @brief Namespace of the means of the Linux operating system
	 *
	 * \~
	 */
	namespace gnu {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс опроса ядра через netlink
		 *
		 * \~english
		 * @brief Class of the polling of the kernel through netlink
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Netlink {
			public:
				/**
				 * \~russian
				 * @brief Тип функции обхода сообщений выборки
				 *
				 * @details Обратный вызов получает очередное сообщение ядра и отвечает,
				 *          продолжать ли обход: ложь прекращает его немедленно
				 *
				 * \~english
				 * @brief Type of the function of the traversal of the messages of a dump
				 * @details The callback receives the next message of the kernel and answers
				 *          whether the traversal should be continued: falsehood stops it immediately
				 *
				 * \~
				 */
				using handler_t = function <bool (const struct nlmsghdr *)>;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод выборки сведений у ядра
				 *
				 * @details Отправляет запрос выборки и обходит пришедшие сообщения, пока
				 *          обратный вызов не прервёт обход либо выборка не иссякнет
				 *
				 * @note Признак выборки (NLM_F_DUMP) выставляется здесь сам: иных
				 *       обращений модуль не ведёт, а без него ядро ответило бы одной
				 *       записью вместо таблицы
				 *
				 * @param type     тип запроса (RTM_GETNEIGH, RTM_GETLINK, RTM_GETADDR, RTM_GETROUTE)
				 * @param family   семейство протоколов (AF_INET, AF_INET6 либо AF_UNSPEC)
				 * @param callback функция обхода полученных сообщений
				 * @return         результат выполнения выборки
				 *
				 * \~english
				 * @brief Method of the dump of the information from the kernel
				 * @details Sends a request of a dump and traverses the arrived messages until
				 *          the callback interrupts the traversal or the dump runs dry
				 * @note The sign of a dump (NLM_F_DUMP) is set out here by itself: the module performs no other
				 *       addresses, and without it the kernel would answer with one
				 *       record instead of a table
				 * @param type     type of the request (RTM_GETNEIGH, RTM_GETLINK, RTM_GETADDR, RTM_GETROUTE)
				 * @param family   family of the protocols (AF_INET, AF_INET6 or AF_UNSPEC)
				 * @param callback function of the traversal of the obtained messages
				 * @return         result of the performance of the dump
				 *
				 * \~
				 */
				bool dump(const uint16_t type, const uint8_t family, const handler_t & callback) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения виртуального сетевого устройства
				 *
				 * @details Устройства эти - мост, туннель, логический сегмент - у BSD
				 *          заводятся запросом SIOCIFCREATE к клонирующему драйверу. У Linux
				 *          клонирующих драйверов нет вовсе, и устройство заводится
				 *          сообщением ядру: RTM_NEWLINK с указанием рода устройства
				 *
				 * @note Род задаётся строкой - "bridge", "vlan", "gre" и прочие, - и
				 *       перечня их ядро наружу не выставляет: набор родов задаётся тем,
				 *       какие модули ядру доступны
				 *
				 * @param name имя заводимого устройства
				 * @param kind род заводимого устройства
				 * @return     результат заведения устройства
				 *
				 * \~english
				 * @brief Method of starting a virtual network device
				 * @details These devices — a bridge, a tunnel, a logical segment — at BSD
				 *          are started by a SIOCIFCREATE request to a cloning driver. At Linux there are
				 *          no cloning drivers at all, and a device is started by
				 *          a message to the kernel: RTM_NEWLINK with the genus of the device specified
				 * @note The genus is set by a string — "bridge", "vlan", "gre" and the others, — and
				 *       the kernel does not expose their list outwards: the set of the genera is set by
				 *       which modules are available to the kernel
				 * @param name name of the started device
				 * @param kind genus of the started device
				 * @return     result of the starting of the device
				 *
				 * \~
				 */
				bool link(string_view name, string_view kind) const noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия виртуального сетевого устройства
				 *
				 * @param name имя снимаемого устройства
				 * @return     результат снятия устройства
				 *
				 * \~english
				 * @brief Method of removing a virtual network device
				 * @param name name of the removed device
				 * @return     result of the removal of the device
				 *
				 * \~
				 */
				bool unlink(string_view name) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод запроса сведений у ядра
				 *
				 * @details Отличается от выборки тем, что запрос задаётся готовым
				 *          сообщением: так спрашивают об одной записи, а не о таблице
				 *          целиком - например, каким путём ядро отправит пакет для
				 *          указанного адреса
				 *
				 * @param message  сообщение запроса
				 * @param size     размер сообщения запроса
				 * @param callback функция обхода полученных сообщений
				 * @return         результат выполнения запроса
				 *
				 * \~english
				 * @brief Method of requesting the information from the kernel
				 * @details Differs from a dump in that the request is set by a ready
				 *          message: that is how one asks about one record, and not about the table
				 *          entirely — for example, by which path the kernel will send a packet for
				 *          the specified address
				 * @param message  message of the request
				 * @param size     size of the message of the request
				 * @param callback function of the traversal of the obtained messages
				 * @return         result of the performance of the request
				 *
				 * \~
				 */
				bool request(const void * message, const size_t size, const handler_t & callback) const noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки ядру сообщения изменения
				 *
				 * @details Ядро отвечает на изменение одним сообщением: успехом либо кодом
				 *          ошибки. Признак подтверждения выставляет отправитель, и без него
				 *          отклика не будет вовсе - отказ прошёл бы незамеченным
				 *
				 * @param message сообщение изменения
				 * @param size    размер сообщения изменения
				 * @return        результат выполнения изменения
				 *
				 * \~english
				 * @brief Method of sending a message of a change to the kernel
				 * @details The kernel answers a change with one message: with a success or with a code
				 *          of an error. The sign of the acknowledgement is set out by the sender, and without it
				 *          there will be no response at all — a refusal would pass unnoticed
				 * @param message message of the change
				 * @param size    size of the message of the change
				 * @return        result of the performance of the change
				 *
				 * \~
				 */
				bool commit(const void * message, const size_t size) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param log объект работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Netlink(const log_t * log) noexcept : _log(log) {}
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
				~Netlink() noexcept {}
		} netlink_t;
	};
};

#endif // __linux__

#endif // __AWH_GNU_NETLINK__
