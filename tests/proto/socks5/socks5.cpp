/**
 * @file socks5.cpp
 * @date 2026-07-20
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация тестовой фикстуры протокола SOCKS5 —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "socks5.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void Socks5Fixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void Socks5Fixture::TearDown() {}

/**
 * @brief Фабричный метод создания клиента SOCKS5
 *
 * @return сформированный объект клиента SOCKS5
 *
 */
std::unique_ptr <awh::proto::client_socks5_t> Socks5Fixture::makeClient() const noexcept {
	// Создаём и возвращаем объект клиента SOCKS5
	return std::make_unique <awh::proto::client_socks5_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Фабричный метод создания сервера SOCKS5
 *
 * @return сформированный объект сервера SOCKS5
 *
 */
std::unique_ptr <awh::proto::server_socks5_t> Socks5Fixture::makeServer() const noexcept {
	// Создаём и возвращаем объект сервера SOCKS5
	return std::make_unique <awh::proto::server_socks5_t> (this->_fmk.get(), this->_log.get());
}
