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
			 * @brief Метод блокировки сигнала SIGILL
			 *
			 * @return результат работы функции
			 */
			bool nosigill() const noexcept;
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
