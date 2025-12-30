/**
 * @file: eth.hpp
 * @date: 2025-11-06
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

#ifndef __AWH_ETHERNET__
#define __AWH_ETHERNET__

/**
 * Наши модули
 */
#include "net.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

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
	 * @brief Класс для работы с сетевым уровнем Ethernet
	 */
	typedef class AWH_SHARED_EXPORT Ethernet {
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод применения сетевой оптимизации операционной системы
			 *
			 */
			void netboost() const noexcept;
		public:
			/**
			 * @brief Метод получения имени сетевого интерфейса по адресу
			 *
			 * @param addr адрес сетевого подключения
			 * @return     имя сетевого интерфейса
			 */
			string iface(const unique_ptr <net::addr_t> & addr) const noexcept;
		public:
			/**
			 * @brief Метод заполнения источника сетевых адресов по имени сетевого интерфейса
			 *
			 * @param source объект источника сетевых адресов
			 */
			void fillsource(net::src_t & source) const noexcept;
			/**
			 * @brief Метод заполнения источника сетевых адресов
			 *
			 * @param node   тип узла события
			 * @param source объект источника сетевых адресов
			 */
			void fillsource(const event::node_t node, net::src_t & source) const noexcept;
			/**
			 * @brief Метод заполнения источника сетевых адресов по заданной сети
			 *
			 * @param net    сетевой адрес подсети в хостовом порядке
			 * @param source объект источника сетевых адресов
			 */
			void fillsource(const unique_ptr <net::addr_t> & net, net::src_t & source) const noexcept;
		public:
			/**
			 * @brief Метод проверки принадлежности IP-адреса подсети
			 *
			 * @param ip     проверяемый IP-адрес в хостовом порядке
			 * @param net    сетевой адрес подсети в хостовом порядке
			 * @param prefix префикс подсети
			 * @return       результат проверки
			 */
			bool isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
			 *
			 * @param a      Первый IPv6-адрес
			 * @param b      Второй IPv6-адрес
			 * @param length Длина префикса в битах
			 * @return       Результат сравнения
			 */
			bool ipv6PrefixEqual(const uint8_t * a, const uint8_t * b, const uint8_t length) const noexcept;
		public:
			/**
			 * @brief Метод установки таймаута сокета
			 *
			 * @param sock  сетевой сокет
			 * @param event событие сокета
			 * @param msec  время таймаута в миллисекундах
			 * @return      результат установки таймаута
			 */
			bool timeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера
			 *
			 * @param sock  сетевой сокет
			 * @param event событие сокета
			 * @return      размер буфера сокета
			 */
			int32_t bufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept;
			/**
			 * @brief Метод установки размеров буфера
			 *
			 * @param sock  сетевой сокет
			 * @param event событие сокета
			 * @param size  размер буфера сокета
			 * @return      установленный размер буфера сокета
			 */
			int32_t bufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept;
		public:
			/**
			 * @brief Метод блокировки сигнала SIGILL
			 *
			 * @return результат работы функции
			 */
			bool nosigill() const noexcept;
			/**
			* @brief Метод получения кода ошибки
			*
			* @param sock сетевой сокет
			* @return     код ошибки на сокете если присутствует
			*/
			int32_t error(const net::socket_t sock) const noexcept;
		public:
			/**
			* @brief Метод активации получения SCTP-событий для сокета
			*
			* @param sock сетевой сокет
			* @param type тип сокета
			* @return     результат работы функции
			*/
			bool sctp(const net::socket_t sock, const event::type_t type) const noexcept;
		public:
			/**
			 * @brief Метод активации TCP/CORK
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool tcpcork(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод включающий или отключающий режим отображения IPv4 => IPv6
			 *
			 * @param sock сетевой сокет
			 * @param mode режим активации или деактивации
			 * @return     результат работы функции
			 */
			bool ipv6only(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод разрешающий повторно использовать сокет после его удаления
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool reuseaddr(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод разрешающий повторно использовать один и тот же порт для нескольких сокетов
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool reuseport(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод игнорирования отключения сигнала записи в убитый сокет
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool nosigpipe(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод разрешения широковещательного адреса
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool broadcast(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод отключения алгоритма Нейгла
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool tcpnodelay(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод установки блокирующего сокета
			 *
			 * @param sock сетевого сокета
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool noblocking(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод установки режима автоматического закрытия файлового дескриптора при вызове exec
			 *
			 * @param sock сетевой сокет
			 * @param mode режим активации или деактивации
			 * @return     результат работы функции
			 */
			bool closeonexec(const net::socket_t sock, const net::socket_mode_t mode) const noexcept;
			/**
			 * @brief Метод устанавливает постоянное подключение на сокет
			 *
			 * @param sock  сетевой сокет
			 * @param cnt   максимальное количество попыток
			 * @param idle  время через которое происходит проверка подключения
			 * @param intvl время между попытками
			 * @return      результат работы функции
			 */
			bool keepalive(const net::socket_t sock, const int32_t cnt, const int32_t idle, const int32_t intvl) const noexcept;
			/**
			 * @brief Метод установки включения/отключения заголовков в сокете
			 *
			 * @param sock   сетевой сокет
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param mode   режим активации или деактивации
			 * @return       результат работы функции
			 */
			bool hdrinclude(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод установки сетевого интерфейса для multicast пакетов
			 *
			 * @param sock   сетевой сокет
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param name   имя сетевого интерфейса
			 * @return       результат работы функции
			 */
			bool multicastIface(const net::socket_t sock, const event::family_t family, const string & name) const noexcept;
			/**
			 * @brief Метод установки режима обратной петли для multicast пакетов
			 *
			 * @param sock   сетевой сокет
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param mode   режим активации или деактивации
			 * @return       результат работы функции
			 */
			bool multicastLoop(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param sock     сетевой сокет
			 * @param family   семейство протоколов (IPv4 или IPv6)
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @param hops     максимальное количество хопов
			 * @return         результат работы функции
			 */
			bool hops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const event::hops_t hops) const noexcept;
		public:
			/**
			 * @brief Метод активации/деактивации мультикаст группы события
			 *
			 * @param sock   сетевой сокет
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @return       результат работы функции
			 */
			bool membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept;
		public:
			/**
			 * @brief Метод вычисления контрольной суммы транспортного уровня
			 *
			 * @param family    семейство протоколов (IPv4 или IPv6)
			 * @param protocol  протокол транспортного уровня
			 * @param src       указатель на источник данных
			 * @param dst       указатель на приёмник данных
			 * @param transport указатель на данные транспортного уровня
			 * @param length    длина данных транспортного уровня
			 * @return          вычисленная контрольная сумма
			 */
			uint16_t checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
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
