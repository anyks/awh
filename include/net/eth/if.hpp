/**
 * @file: if.hpp
 * @date: 2026-01-28
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_IF__
#define __AWH_IF__

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
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;
		/**
		 * @brief Класс для работы с сетевым интерфейсом
		 *
		 */
		typedef class AWH_SHARED_EXPORT Interface {
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
				bool destroy(const string & name) const noexcept;
			public:
				/**
				 * @brief Метод получения списка сетевых интерфейсов системы
				 *
				 * @return список сетевых интерфейсов системы
				 */
				unordered_set <string> available() const noexcept;
			public:
				/**
				 * @brief Метод создания TUN/TAP сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     дескриптор созданного TUN/TAP сетевого интерфейса
				 */
				net::socket_t tunnel(string & name) const noexcept;
			public:
				/**
				 * @brief Метод проверки доступности сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат проверки доступности сетевого интерфейса
				 */
				bool isAvailable(const string & name) const noexcept;
			public:
				/**
				 * @brief Метод проверки туннельного сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат проверки туннельного сетевого интерфейса
				 */
				bool isTunnel(const string & name) const noexcept;
				/**
				 * @brief Метод проверки туннельного сетевого интерфейса по адресу
				 *
				 * @param addr адрес сетевого подключения
				 * @return     результат проверки туннельного сетевого интерфейса
				 */
				bool isTunnel(const unique_ptr <net::addr_t> & addr) const noexcept;
			public:
				/**
				 * @brief Метод проверки виртуального сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     результат проверки виртуального сетевого интерфейса
				 */
				bool isVirtual(const string & name) const noexcept;
				/**
				 * @brief Метод проверки виртуального сетевого интерфейса по адресу
				 *
				 * @param addr адрес сетевого подключения
				 * @return     результат проверки виртуального сетевого интерфейса
				 */
				bool isVirtual(const unique_ptr <net::addr_t> & addr) const noexcept;
			public:
				/**
				 * @brief Метод получения имени сетевого интерфейса по адресу
				 *
				 * @param addr адрес сетевого подключения
				 * @return     имя сетевого интерфейса
				 */
				string name(const unique_ptr <net::addr_t> & addr) const noexcept;
			public:
				/**
				 * @brief Метод получения режима сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     режим сетевого интерфейса
				 */
				event::mode_t mode(const string & name) const noexcept;
				/**
				 * @brief Метод включения/выключения сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @param mode режим включения/выключения интерфейса
				 * @param mtu  размер MTU интерфейса
				 * @return     результат включения/выключения интерфейса
				 */
				bool mode(const string & name, const event::mode_t mode, const int32_t mtu = 1400) const noexcept;
			public:
				/**
				 * @brief Метод получения IP-адреса сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @return     IP-адрес сетевого интерфейса
				 */
				unique_ptr <net::addr_t> ip(const string & name) const noexcept;
				/**
				 * @brief Метод получения IP-адреса сетевого интерфейса
				 *
				 * @param name имя сетевого интерфейса
				 * @param type тип IP-адреса (локальный, глобальный, маска)
				 * @return     IP-адрес сетевого интерфейса
				 */
				string ip(const string & name, const net::ip_type_t type) const noexcept;
				/**
				 * @brief Метод установки IP-адреса на сетевой интерфейс
				 *
				 * @param name   имя сетевого интерфейса
				 * @param addr   адрес сетевого интерфейса для установки
				 * @param peer   адрес удалённого пира (для точка-точка)
				 * @param prefix префикс подсети
				 * @return       результат установки IP-адреса
				 */
				bool ip(const string & name, const unique_ptr <net::addr_t> & addr, const uint8_t prefix) const noexcept;
				/**
				 * @brief Метод установки IP-адреса на сетевой интерфейс
				 *
				 * @param name   имя сетевого интерфейса
				 * @param addr   адрес сетевого интерфейса для установки
				 * @param peer   адрес удалённого пира (для точка-точка)
				 * @param prefix префикс подсети
				 * @return       результат установки IP-адреса
				 */
				bool ip(const string & name, const unique_ptr <net::addr_t> & addr, const unique_ptr <net::addr_t> & peer, const uint8_t prefix) const noexcept;
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
		} if_t;
	};
};

#endif // __AWH_IF__
