/**
 * @file: server.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <worker/server.hpp>

/**
 * clear Метод очистки
 */
void awh::server::Worker::clear() noexcept {
	// Очищаем данные вокера
	awh::worker_t::clear();
	// Восстанавливаем порт сервера
	this->port = SERVER_PORT;
	// Восстанавливаем хост сервера
	this->host = SERVER_HOST;
	// Восстанавливаем максимальное количество одновременных подключений
	this->total = SERVER_TOTAL_CONNECT;
}
