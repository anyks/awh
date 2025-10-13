/**
 * @file: server.cpp
 * @date: 2025-10-08
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

/**
 * Подключаем заголовочный файл
 */
#include <scheme/server.hpp>

/**
 * @brief Метод очистки
 *
 */
void awh::server::Scheme::clear() noexcept {
	// Очищаем данные вокера
	awh::scheme_t::clear();
	// Восстанавливаем порт сервера
	this->_port = SERVER_PORT;
	// Восстанавливаем хост сервера
	this->_host = SERVER_HOST;
	// Восстанавливаем максимальное количество одновременных подключений
	this->_total = SERVER_TOTAL_CONNECT;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Scheme::Scheme(const fmk_t * fmk, const log_t * log) noexcept :
 awh::scheme_t(fmk, log),
 _host(SERVER_HOST), _port(SERVER_PORT),
 _total(SERVER_TOTAL_CONNECT),
 _ectx(fmk, log), _addr(fmk, log) {}
