/**
 * @file: server.cpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <scheme/server.hpp>

/**
 * clear Метод очистки
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
