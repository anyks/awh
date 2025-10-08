/**
 * @file: if.hpp
 * @date: 2021-12-19
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

#ifndef __AWH_IFNET__
#define __AWH_IFNET__

/**
 * Стандартные библиотеки
 */
#include <string>
#include <unordered_map>

/**
 * Наши модули
 */
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
	 * @brief Класс работы с сетевыми интерфейсами
	 *
	 */
	typedef class AWH_SHARED_EXPORT IfNet {
		private:
			// Список сетевых интерфейсов
			std::unordered_map <string, string> _ifs;
			// Список интернет-адресов
			std::unordered_map <string, string> _ips;
			// Список интернет-адресов
			std::unordered_map <string, string> _ips6;
		private:
			/**
			 * Максимальный размер сетевого буфера
			 */
			static constexpr uint16_t IF_BUFFER_SIZE = 0xFA0;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод извлечения IP-адресов
			 *
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void getIPAddresses(const int32_t family) noexcept;
			/**
			 * @brief Метод извлечения MAC-адресов
			 *
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void getHWAddresses(const int32_t family) noexcept;
		private:
			/**
			 * Метод закрытие подключения
			 * @param sock сетевой сокет
			 */
			void close(const int32_t sock) const noexcept;
		public:
			/**
			 * @brief Метод инициализации сбора информации
			 *
			 */
			void init() noexcept;
			/**
			 * @brief Метод очистки собранных данных
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * Метод вывода списка MAC-адресов
			 * @return список MAC-адресов
			 */
			const std::unordered_map <string, string> & hws() const noexcept;
		public:
			/**
			 * @brief Метод запроса названия сетевого интерфейса
			 *
			 * @param eth идентификатор сетевого интерфейса
			 * @return    название сетевого интерфейса
			 */
			string name(const string & eth) const noexcept;
		public:
			/**
			 * @brief Метод получения MAC-адреса по IP-адресу клиента
			 *
			 * @param ip     адрес интернет-подключения клиента
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       аппаратный адрес сетевого интерфейса клиента
			 */
			string mac(const string & ip, const int32_t family) const noexcept;
			/**
			 * @brief Метод определения мак адреса клиента
			 *
			 * @param sin    объект подключения
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       данные мак адреса
			 */
			string mac(struct sockaddr * sin, const int32_t family) const noexcept;
		public:
			/**
			 * @brief Метод получения основного IP-адреса на сервере
			 *
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			string ip(const int32_t family) const noexcept;
			/**
			 * @brief Метод получения IP-адреса из подключения
			 *
			 * @param sin    объект подключения
			 * @param family тип интернет протокола
			 * @return       данные ip адреса
			 */
			string ip(struct sockaddr * sin, const int32_t family) const noexcept;
			/**
			 * @brief Метод вывода IP-адреса соответствующего сетевому интерфейсу
			 *
			 * @param eth    идентификатор сетевого интерфейса
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP-адрес соответствующий сетевому интерфейсу
			 */
			const string & ip(const string & eth, const int32_t family) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			IfNet(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~IfNet() noexcept {}
	} ifnet_t;
};

#endif // __AWH_IFNET__
