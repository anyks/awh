/**
 * @file: headers.cpp
 * @date: 2026-07-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры контейнера HTTP-заголовков —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "headers.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void HeadersFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект контейнера HTTP-заголовков
	this->_headers = std::make_unique <awh::http::headers_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void HeadersFixture::TearDown() {}

/**
 * @brief Фабричный метод создания HTTP-заголовка
 *
 * @param name  название HTTP-заголовка
 * @param value значение HTTP-заголовка
 * @return      сформированный объект HTTP-заголовка
 *
 */
awh::http::headers_t::header_t HeadersFixture::header(const std::string & name, const std::string & value) const noexcept {
	// Результирующий объект HTTP-заголовка
	awh::http::headers_t::header_t result;
	// Заполняем название и значение заголовка через фабричный метод
	result.from(name, value);
	// Выводим сформированный заголовок
	return result;
}
