/**
 * @file: socks5.cpp
 * @date: 2026-05-24
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

/**
 * Подключаем заголовочный файл
 */
#include <proto/socks5/socks5.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод установки адреса хоста для подключения
 *
 * @param host параметры адреса хоста для подключения
 * @return     результат установки адреса хоста для подключения
 */
bool awh::proto::Socks5::setHost(const net::attr_t * host) noexcept {
	// Результат работы функции
	bool result = false;
	// Если хост для подключения передан
	if((result = (host != nullptr))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип адреса хоста для подключения
			 */
			switch(static_cast <uint8_t> (host->type)){
				// Если тип адреса соответствует FQDN
				case static_cast <uint8_t> (net::type_t::FQDN): {
					// Выполняем инициализацию объекта хоста
					this->_host = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем тип адреса события
					this->_host->type = host->type;
					// Устанавливаем порт хоста для подключения
					awh_cast <net::attr_fqdn_t *> (this->_host.get())->port = awh_cast <const net::attr_fqdn_t *> (host)->port;
					// Устанавливаем доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (this->_host.get())->domain = awh_cast <const net::attr_fqdn_t *> (host)->domain;
				} break;
				// Если тип адреса соответствует IPv4
				case static_cast <uint8_t> (net::type_t::IPV4): {
					// Выполняем инициализацию объекта хоста
					this->_host = make_unique <net::attr_net_t> ();
					// Устанавливаем тип адреса события
					this->_host->type = host->type;
					// Устанавливаем порт хоста для подключения
					awh_cast <net::attr_net_t *> (this->_host.get())->port = awh_cast <const net::attr_net_t *> (host)->port;
					// Устанавливаем IP-адрес хоста для подключения
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (host)->ip.get())->address;
				} break;
				// Если тип адреса соответствует IPv6
				case static_cast <uint8_t> (net::type_t::IPV6): {
					// Выполняем инициализацию объекта хоста
					this->_host = make_unique <net::attr_net_t> ();
					// Устанавливаем тип адреса события
					this->_host->type = host->type;
					// Устанавливаем порт хоста для подключения
					awh_cast <net::attr_net_t *> (this->_host.get())->port = awh_cast <const net::attr_net_t *> (host)->port;
					// Устанавливаем IP-адрес хоста для подключения
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (host)->ip.get())->address[0], 16);
				} break;
				// Если тип адреса не соответствует ни одному из поддерживаемых типов, сбрасываем результат
				default: result = false;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения адреса хоста для подключения
 *
 * @param host указатель на объект для извлечения параметров адреса хоста для подключения
 * @return     результат извлечения параметров адреса хоста для подключения
 */
bool awh::proto::Socks5::getHost(net::attr_t ** host) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если хост для подключения установлен
	if((result = (this->_host != nullptr)))
		// Устанавливаем результат извлечения параметров адреса хоста для подключения
		(* host) = this->_host.get();
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения сообщения
 *
 * @param code код статуса
 * @return     текстовое значение кода статуса
 */
string awh::proto::Socks5::statusMessage(const status_t code) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем текстовое значение кода статуса
		 */
		switch(static_cast <uint8_t> (code)){
			// Если код статуса соответствует успешному завершению
			case static_cast <uint8_t> (status_t::SUCCESS):
				// Устанавливаем результат текстового значения кода статуса
				result = "Successful completion";
			break;
			// Если код статуса соответствует ошибке SOCKS-сервера
			case static_cast <uint8_t> (status_t::SOCKSERR):
				// Устанавливаем результат текстового значения кода статуса
				result = "SOCKS server error";
			break;
			// Если код статуса соответствует запрещённому соединению набором правил
			case static_cast <uint8_t> (status_t::FORBIDDEN):
				// Устанавливаем результат текстового значения кода статуса
				result = "Connection forbidden by ruleset";
			break;
			// Если код статуса соответствует недоступности сети
			case static_cast <uint8_t> (status_t::UNAVNET):
				// Устанавливаем результат текстового значения кода статуса
				result = "Network unreachable";
			break;
			// Если код статуса соответствует недоступности хоста
			case static_cast <uint8_t> (status_t::UNAVHOST):
				// Устанавливаем результат текстового значения кода статуса
				result = "Host unreachable";
			break;
			// Если код статуса соответствует отказу в соединении
			case static_cast <uint8_t> (status_t::DENIED):
				// Устанавливаем результат текстового значения кода статуса
				result = "Connection denied";
			break;
			// Если код статуса соответствует истечению TTL
			case static_cast <uint8_t> (status_t::TIMETTL):
				// Устанавливаем результат текстового значения кода статуса
				result = "Connection timed out";
			break;
			// Если код статуса соответствует отсутствию поддерживаемой команды
			case static_cast <uint8_t> (status_t::NOCOMMAND):
				// Устанавливаем результат текстового значения кода статуса
				result = "Command not supported";
			break;
			// Если код статуса соответствует отсутствию поддерживаемого типа адреса
			case static_cast <uint8_t> (status_t::NOADDR):
				// Устанавливаем результат текстового значения кода статуса
				result = "Address type not supported";
			break;
			// Если код статуса соответствует общей ошибке SOCKS-сервера
			case static_cast <uint8_t> (status_t::NOSUPPORT):
				// Устанавливаем результат текстового значения кода статуса
				result = "General SOCKS server failure";
			// Если код статуса не соответствует ни одному из известных кодов, устанавливаем результат текстового значения кода статуса как неизвестная ошибка
			default:
				// Устанавливаем результат текстового значения кода статуса
				result = "Unknown status";
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (code)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::proto::Socks5::Socks5(const fmk_t * fmk, const log_t * log) noexcept :
 _host(nullptr), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::proto::Socks5::~Socks5() noexcept {}
