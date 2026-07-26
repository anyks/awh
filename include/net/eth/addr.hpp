/**
 * @file: addr.hpp
 * @date: 2026-01-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля адресов канального уровня — класс eth::Network_Address для получения,
 *        разбора и представления MAC-адресов и адресов сетевых интерфейсов машины
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён Ethernet протоколов
	 *
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс для работы с сетевыми адресами
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Network_Address {
			private:
				// Объект работы с сетевым интерфейсом
				iface_t _iface;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод заполнения источника сетевых адресов по имени сетевого интерфейса
				 *
				 * @param source объект источника сетевых адресов
				 *
				 */
				void fillSource(net::src_t & source) const noexcept;
				/**
				 * @brief Метод заполнения источника сетевых адресов по заданной сети
				 *
				 * @param net    сетевой адрес подсети (IP-адрес в сетевом порядке байт)
				 * @param source объект источника сетевых адресов
				 *
				 */
				void fillSource(const net::addr_t * net, net::src_t & source) const noexcept;
				/**
				 * @brief Метод заполнения источника сетевых адресов
				 *
				 * @param node   тип узла события
				 * @param source объект источника сетевых адресов
				 *
				 */
				void fillSource(const event::node_t node, net::src_t & source) const noexcept;
			public:
				/**
				 * @brief Метод проверки принадлежности IP-адреса подсети
				 *
				 * @param ip     проверяемый IP-адрес в хостовом порядке
				 * @param net    сетевой адрес подсети в хостовом порядке
				 * @param prefix префикс подсети
				 * @return       результат проверки
				 *
				 */
				bool isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept;
				/**
				 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
				 *
				 * @param first  Первый IPv6-адрес
				 * @param second Второй IPv6-адрес
				 * @param length Длина префикса в битах
				 * @return       Результат сравнения
				 *
				 */
				bool ipv6PrefixEqual(const uint8_t * first, const uint8_t * second, const uint8_t length) const noexcept;
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
				 *
				 */
				uint16_t checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 */
				explicit Network_Address(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Network_Address() noexcept;
		} addr_t;
	};
};

#endif // __AWH_ADDR__
