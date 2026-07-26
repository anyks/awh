/**
 * @file: auth.cpp
 * @date: 2026-07-14
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
 * Подключаем заголовочный файлы проекта
 */
#include "auth.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void AuthFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void AuthFixture::TearDown() {}

/**
 * @brief Фабричный метод создания модуля авторизации
 *
 * @param owner сторона работы (клиент/сервер)
 * @return      сформированный объект модуля авторизации
 */
std::unique_ptr <awh::http::auth_t> AuthFixture::make(const awh::http::auth_t::owner_t owner) const noexcept {
	// Создаём и возвращаем объект модуля авторизации
	return std::make_unique <awh::http::auth_t> (owner, this->_fmk.get(), this->_log.get());
}
