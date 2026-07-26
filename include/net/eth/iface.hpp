/**
 * @file: iface.hpp
 * @date: 2026-01-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IFACE__
#define __AWH_IFACE__

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
	 * @brief Пространство имён Ethernet протоколов
	 *
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс для работы с сетевым интерфейсом
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Interface {
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод удаления сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат удаления сетевого интерфейса
				 */
				bool destroy(string_view name) const noexcept;
			public:
				/**
				 * @brief Метод получения списка сетевых интерфейсов системы
				 *
				 * @return список сетевых интерфейсов системы
				 */
				unordered_set <string> available() const noexcept;
			public:
				/**
				 * @brief Метод проверки доступности сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат проверки доступности сетевого интерфейса
				 */
				bool isAvailable(string_view name) const noexcept;
			public:
				/**
				 * @brief Метод проверки туннельного сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат проверки туннельного сетевого интерфейса
				 */
				bool isTunnel(string_view name) const noexcept;
				/**
				 * @brief Метод проверки туннельного сетевого интерфейса по адресу
				 *
				 * @param addr адрес сетевого подключения
				 * @return     результат проверки туннельного сетевого интерфейса
				 */
				bool isTunnel(const net::addr_t * addr) const noexcept;
			public:
				/**
				 * @brief Метод проверки виртуального сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат проверки виртуального сетевого интерфейса
				 */
				bool isVirtual(string_view name) const noexcept;
				/**
				 * @brief Метод проверки виртуального сетевого интерфейса по адресу
				 *
				 * @param addr адрес сетевого подключения
				 * @return     результат проверки виртуального сетевого интерфейса
				 */
				bool isVirtual(const net::addr_t * addr) const noexcept;
			public:
				/**
				 * @brief Метод получения имени сетевого интерфейса по адресу
				 *
				 * @param addr адрес сетевого подключения
				 * @return     имя сетевого интерфейса
				 */
				string name(const net::addr_t * addr) const noexcept;
			public:
				/**
				 * @brief Метод создания сетевого интерфейса
				 *
				 * @param type тип сетевого интерфейса
				 * @param name имя сетевого интерфейса
				 * @return     дескриптор созданного сетевого интерфейса
				 */
				net::socket_t create(const event::eth_t type, string & name) const noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     MTU сетевого интерфейса
				 */
				uint16_t mtu(string_view name) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @param mtu  размер MTU интерфейса
				 * @return     результат установки MTU сетевого интерфейса
				 */
				bool mtu(string_view name, const uint16_t mtu) const noexcept;
			public:
				/**
				 * @brief Метод получения установленных флагов сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     флаги сетевого интерфейса
				 */
				unordered_set <event::eth_flag_t> flags(string_view name) const noexcept;
				/**
				 * @brief Метод установки флага сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @param flag флаг сетевого интерфейса
				 * @param mode режим включения/выключения флага
				 * @return     результат установки флага сетевого интерфейса
				 */
				bool flag(string_view name, const event::eth_flag_t flag, const event::mode_t mode) const noexcept;
			public:
				/**
				 * @brief Метод установки IP-адреса на сетевой интерфейс
				 *
				 * @param name   имя сетевого интерфейса
				 * @param ip     адрес сетевого интерфейса для установки
				 * @param peer   адрес удалённого пира (для точка-точка)
				 * @param prefix префикс подсети
				 * @return       результат установки IP-адреса
				 */
				bool setAddress(string_view name, const net::addr_t * ip, const uint8_t prefix) const noexcept;
				/**
				 * @brief Метод получения IP-адреса сетевого интерфейса
				 *
				 * @param name   имя сетевого интерфейса
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       IP-адрес сетевого интерфейса
				 */
				unique_ptr <net::addr_t> getAddress(string_view name, const event::family_t family) const noexcept;
			public:
				/**
				 * @brief Метод установки параметров сетевого интерфейса точка-точка
				 *
				 * @param name   имя сетевого интерфейса
				 * @param ip     адрес сетевого интерфейса для установки
				 * @param peer   адрес удалённого пира (для точка-точка)
				 * @param prefix префикс подсети
				 * @return       результат установки параметров сетевого интерфейса точка-точка
				 */
				bool setAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix) const noexcept;
				/**
				 * @brief Метод изменения параметров сетевого интерфейса точка-точка
				 *
				 * @param name   имя сетевого интерфейса
				 * @param ip     адрес сетевого интерфейса для получения
				 * @param peer   адрес удалённого пира (для точка-точка)
				 * @param prefix префикс подсети
				 * @return       результат изменения параметров сетевого интерфейса точка-точка
				 */
				bool getAddress(string_view name, unique_ptr <net::addr_t> & ip, unique_ptr <net::addr_t> & peer, uint8_t & prefix) const noexcept;
			public:
				/**
				 * @brief Метод комплексной настройки сетевого интерфейса (адрес + MTU + поднятие) за один управляющий сокет
				 *
				 * @param name   имя сетевого интерфейса
				 * @param ip     адрес сетевого интерфейса для установки
				 * @param prefix префикс подсети
				 * @param mtu    размер MTU интерфейса (0 - не изменять)
				 * @return       результат комплексной настройки сетевого интерфейса
				 */
				bool configure(string_view name, const net::addr_t * ip, const uint8_t prefix, const uint16_t mtu = 0) const noexcept;
				/**
				 * @brief Метод комплексной настройки сетевого интерфейса точка-точка (адрес + пир + MTU + поднятие) за один управляющий сокет
				 *
				 * @param name   имя сетевого интерфейса
				 * @param ip     адрес сетевого интерфейса для установки
				 * @param peer   адрес удалённого пира (для точка-точка)
				 * @param prefix префикс подсети
				 * @param mtu    размер MTU интерфейса (0 - не изменять)
				 * @return       результат комплексной настройки сетевого интерфейса
				 */
				bool configure(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix, const uint16_t mtu = 0) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 */
				explicit Interface(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Interface() noexcept;
		} iface_t;
	};
};

#endif // __AWH_IFACE__
