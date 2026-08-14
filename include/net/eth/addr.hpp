/**
 * @file addr.hpp
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
 * @brief Заголовочный файл модуля адресов канального уровня — класс eth::Network_Address для получения,
 *        разбора и представления MAC-адресов и адресов сетевых интерфейсов машины
 *
 * \~english
 * @brief Header file of the module of the addresses of the link level — the eth::Network_Address class for getting,
 *        parsing and representing the MAC addresses and the addresses of the network interfaces of the machine
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ADDR__
#define __AWH_ADDR__

/**
 * Наши модули
 */
#include "iface.hpp"
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
		 * @brief Класс для работы с сетевыми адресами
		 *
		 * @details Отвечает на вопрос, через какое устройство и с каким адресом
		 *          машина выходит в сеть, - и подсобляет в мелочах, которые нужны
		 *          при работе с сырыми пакетами: сравнении адресов по префиксу и
		 *          подсчёте контрольных сумм
		 *
		 *          Отличается от `net_addr_t` кругом занятий. Тот ведает **одним**
		 *          адресом: разбирает, выводит, сравнивает. Этот же обращается к
		 *          самой системе и выясняет, какие устройства есть и что на них
		 *          настроено
		 *
		 * @note Определение исходящего адреса **запоминается** на пять секунд,
		 *       порознь для IPv4 и IPv6: опрос устройств обходится недёшево, а
		 *       настройки сети меняются редко. Правка настроек потому доходит не
		 *       сразу
		 *
		 * \~english
		 * @brief Class for working with the network addresses
		 * @details Answers the question through which device and with which address
		 *          the machine goes out into the network, — and helps in the small things that are needed
		 *          when working with the raw packets: the comparison of the addresses by a prefix and
		 *          the counting of the checksums
		 *          Differs from `net_addr_t` by the circle of its occupations. That one is in charge of **one**
		 *          address: it parses, yields, compares. This one addresses
		 *          the system itself and finds out which devices are present and what is
		 *          set up on them
		 * @note The determination of the outgoing address is **remembered** for five seconds,
		 *       separately for IPv4 and IPv6: polling the devices costs not cheap, and
		 *       the settings of the network change rarely. A correction of the settings therefore reaches
		 *       not at once
		 *
		 * \~
		 */
		/**
		 * \~russian
		 * @brief Предварительное объявление объекта управления шлюзами
		 *
		 * \~english
		 * @brief Forward declaration of the object of the management of the gateways
		 *
		 * \~
		 */
		class Gateway;

		typedef class __AWH_SHARED_EXPORT__ Network_Address {
			private:
				// Объект работы с сетевым интерфейсом
				iface_t _iface;
			private:
				// Объект управления шлюзами, которым спрашивается маршрут
				const Gateway * _gateway;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод установки объекта управления шлюзами
				 *
				 * @details Исходящий адрес определяется подбором маршрута, а подбор этот
				 *          ведёт объект управления шлюзами. Прежде адрес добывался
				 *          подключением к одному из зашитых серверов имён, отчего
				 *          определение своего адреса требовало выхода в интернет,
				 *          зависело от случая при выборе сервера и на машине с
				 *          раздельным туннелем возвращало адрес туннеля вместо адреса
				 *          своей сети
				 *
				 * @param gateway объект управления шлюзами для установки
				 *
				 * \~english
				 * @brief Method of setting the object of the management of the gateways
				 * @details The outgoing address is determined by the picking of a route, and that picking
				 *          is performed by the object of the management of the gateways. Formerly the address was obtained by
				 *          a connection to one of the hardcoded name servers, and therefore
				 *          the determination of one's own address required going out into the internet,
				 *          depended on the chance at the choice of the server and on a machine with
				 *          a split tunnel returned the address of the tunnel instead of the address
				 *          of one's own network
				 * @param gateway object of the management of the gateways to set
				 *
				 * \~
				 */
				void gateway(const Gateway * gateway) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @details Включает защиту запомненных сведений об исходящем адресе от
				 * одновременного обращения из разных потоков
				 *
				 * @warning Настройка эта **общая на весь процесс**, а не своя у каждого
				 * объекта: выставленная через один объект, она действует на все. По
				 * умолчанию защита **выключена** - в расчёте на однопоточную работу, -
				 * и включать её следует до того, как заработает второй поток
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 * \~english
				 * @brief Method of setting the thread safety of the work
				 * @details Switches on the protection of the remembered information about the outgoing address from
				 * a simultaneous address from different threads
				 * @warning This setting is a **common one for the whole process**, and not its own for every
				 * object: set through one object, it is in force for all of them. By
				 * default the protection is **switched off** — with the reckoning on a single-threaded work, —
				 * and it should be switched on before the second thread starts working
				 * @param mode flag of the thread safety mode
				 *
				 * \~
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заполнения источника сетевых адресов по имени сетевого интерфейса
				 *
				 * @details Дозаполняет связку по уже известному названию устройства:
				 * отыскивает его среди имеющихся и берёт с него адрес сети и
				 * аппаратный адрес
				 *
				 * @note Разновидность адреса берётся из **уже заведённого** в связке
				 * объекта адреса - по его длине. Заводить его следует до вызова
				 *
				 * @note Пустое название устройства обращает вызов к разновидности,
				 * определяющей устройство выхода во внешнюю сеть самостоятельно
				 *
				 * @param source объект источника сетевых адресов
				 *
				 * \~english
				 * @brief Method of filling the source of the network addresses by the name of a network interface
				 * @details Fills up the bundle by the already known name of the device:
				 * finds it among the present ones and takes from it the address of the network and
				 * the hardware address
				 * @note The kind of the address is taken from the **already started** address object
				 * in the bundle — by its length. It should be started before the call
				 * @note An empty name of the device turns the call to the kind
				 * determining the device of the exit into the external network on its own
				 * @param source object of the source of the network addresses
				 *
				 * \~
				 */
				void fillSource(net::src_t & source) const noexcept;
				/**
				 * \~russian
				 * @brief Метод заполнения источника сетевых адресов по заданной сети
				 *
				 * @details Отыскивает устройство, чей адрес лежит в указанной сети, и
				 * заполняет связку его сведениями. Нужно это при выборе исходящего
				 * адреса: из нескольких устройств машины берётся то, что смотрит в
				 * нужную сторону
				 *
				 * @warning Довод с адресом сети разыменовывается **без проверки на
				 * пустоту**. Передавать пустой указатель нельзя - для случая, когда
				 * сеть не задана, есть разновидность с одним доводом
				 *
				 * @param net    сетевой адрес подсети (IP-адрес в сетевом порядке байт)
				 * @param source объект источника сетевых адресов
				 *
				 * \~english
				 * @brief Method of filling the source of the network addresses by the given network
				 * @details Finds the device whose address lies in the specified network, and
				 * fills the bundle with its information. This is needed at the choice of the outgoing
				 * address: of the several devices of the machine the one is taken that looks in
				 * the needed direction
				 * @warning The argument with the address of the network is dereferenced **without a check for
				 * emptiness**. Passing a null pointer is not allowed — for the case when
				 * the network is not set, there is the kind with one argument
				 * @param net    network address of the subnet (IP address in the network byte order)
				 * @param source object of the source of the network addresses
				 *
				 * \~
				 */
				void fillSource(const net::addr_t * net, net::src_t & source) const noexcept;
				/**
				 * \~russian
				 * @brief Метод заполнения источника сетевых адресов
				 *
				 * @details Определяет устройство, через которое машина выходит наружу, и
				 * заполняет связку его сведениями. Вид узла уточняет отбор
				 *
				 * @note Итог **запоминается** на пять секунд и повторные вызовы в этот
				 * срок систему не опрашивают. Свежие настройки сети видны не сразу
				 *
				 * @param node   тип узла события
				 * @param source объект источника сетевых адресов
				 *
				 * \~english
				 * @brief Method of filling the source of the network addresses
				 * @details Determines the device through which the machine goes outwards, and
				 * fills the bundle with its information. The kind of the node refines the selection
				 * @note The result is **remembered** for five seconds and the repeated calls within this
				 * time do not poll the system. The fresh settings of the network are seen not at once
				 * @param node   type of the node of the event
				 * @param source object of the source of the network addresses
				 *
				 * \~
				 */
				void fillSource(const event::node_t node, net::src_t & source) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки принадлежности IP-адреса подсети
				 *
				 * @details Отвечает, лежит ли адрес в указанной сети
				 *
				 * @warning Все три довода ждут **хостового** порядка байт, а не
				 * сетевого. Адрес, взятый из структур сокетов, следует переставить до
				 * вызова, иначе итог будет неверным, но не ошибочным на вид
				 *
				 * @param ip     проверяемый IP-адрес в хостовом порядке
				 * @param net    сетевой адрес подсети в хостовом порядке
				 * @param prefix префикс подсети
				 * @return       результат проверки
				 *
				 * \~english
				 * @brief Method of checking the belonging of an IP address to a subnet
				 * @details Answers whether the address lies in the specified network
				 * @warning All three arguments expect the **host** byte order, and not the network
				 * one. An address taken from the structures of the sockets should be swapped before
				 * the call, otherwise the result will be wrong, but not erroneous on the face of it
				 * @param ip     checked IP address in the host order
				 * @param net    network address of the subnet in the host order
				 * @param prefix prefix of the subnet
				 * @return       result of the check
				 *
				 * \~
				 */
				bool isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept;
				/**
				 * \~russian
				 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
				 *
				 * @details Сравнивает лишь старшие разряды адресов - столько, сколько
				 * отведено префиксу, - и тем отвечает, лежат ли оба адреса в одной сети
				 *
				 * @note Длина префикса в разрядах, а не в байтах, и границе байта
				 * отвечать не обязана: неполный байт досравнивается поразрядно
				 *
				 * @param first  Первый IPv6-адрес
				 * @param second Второй IPv6-адрес
				 * @param length Длина префикса в битах
				 * @return       Результат сравнения
				 *
				 * \~english
				 * @brief Method of comparing two IPv6 addresses by a prefix (in bits)
				 * @details Compares only the higher digits of the addresses — as many as
				 * are given over to the prefix, — and thereby answers whether both addresses lie in one network
				 * @note The length of the prefix is in bits, and not in bytes, and is not obliged to answer
				 * the boundary of a byte: an incomplete byte is compared bit by bit
				 * @param first  First IPv6 address
				 * @param second Second IPv6 address
				 * @param length Length of the prefix in bits
				 * @return       Result of the comparison
				 *
				 * \~
				 */
				bool ipv6PrefixEqual(const uint8_t * first, const uint8_t * second, const uint8_t length) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод вычисления контрольной суммы транспортного уровня
				 *
				 * @details Считает контрольную сумму так, как того требуют протоколы
				 * транспортного уровня: не по одним лишь данным, а вместе с выдуманным
				 * заголовком из адресов отправителя и получателя. Затем и нужны здесь
				 * адреса, хотя к транспортному уровню они не относятся
				 *
				 * @note Нужно это лишь при работе с сырыми сокетами, где заголовки
				 * собираются вручную. Обычный обмен сумму считает сам - средствами
				 * системы или самого устройства
				 *
				 * @param family    семейство протоколов (IPv4 или IPv6)
				 * @param protocol  протокол транспортного уровня
				 * @param src       указатель на источник данных
				 * @param dst       указатель на приёмник данных
				 * @param transport указатель на данные транспортного уровня
				 * @param length    длина данных транспортного уровня
				 * @return          вычисленная контрольная сумма
				 *
				 * \~english
				 * @brief Method of computing the checksum of the transport level
				 * @details Counts the checksum the way the protocols of the transport level
				 * require it: not by the data alone, but together with a pseudo
				 * header of the addresses of the sender and of the receiver. That is why the addresses are needed here,
				 * although they do not belong to the transport level
				 * @note This is needed only when working with the raw sockets, where the headers
				 * are assembled by hand. An ordinary exchange counts the sum by itself — by the means
				 * of the system or of the device itself
				 * @param family    family of the protocols (IPv4 or IPv6)
				 * @param protocol  protocol of the transport level
				 * @param src       pointer to the source of the data
				 * @param dst       pointer to the receiver of the data
				 * @param transport pointer to the data of the transport level
				 * @param length    length of the data of the transport level
				 * @return          the computed checksum
				 *
				 * \~
				 */
				uint16_t checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept;
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
				explicit Network_Address(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Network_Address() noexcept;
		} addr_t;
	};
};

#endif // __AWH_ADDR__
