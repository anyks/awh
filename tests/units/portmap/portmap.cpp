/**
 * @file: portmap.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля перенаправления портов —
 *        создание объектов тестового окружения перед каждым тестом и ожидание итога обращения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "portmap.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>

/**
 * @brief Метод настройки тестового окружения
 *
 */
void PortmapUnitFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Устанавливаем объект работы с логами
	this->_fmk->setLogger(this->_log.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void PortmapUnitFixture::TearDown() {}

/**
 * @brief Метод ожидания итога обращения к маршрутизатору
 *
 * @param portmap объект модуля перенаправления портов
 * @param action  просьба, с которой ведётся обращение
 * @return        итог обращения к маршрутизатору
 *
 */
PortmapUnitFixture::outcome_t PortmapUnitFixture::await(awh::unit::portmap_t & portmap, const awh::unit::portmap_t::action_t action) const noexcept {
	// Собираемый итог обращения к маршрутизатору
	outcome_t result;
	// Время начала обращения к маршрутизатору
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Устанавливаем функцию обратного вызова на отказ перенаправления
	 */
	portmap.on <void (const awh::unit::portmap_t::error_t, const awh::unit::portmap_t::type_t)> (
		"failure", [&portmap, &result, &start](const awh::unit::portmap_t::error_t error, const awh::unit::portmap_t::type_t type) noexcept -> void {
			// Запоминаем время, потраченное на обращение
			result.spent = std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count();
			// Запоминаем, что обращение завершилось отказом
			result.failed = true;
			// Запоминаем код причины отказа перенаправления
			result.error = error;
			// Запоминаем договор, по которому получен итог
			result.type = type;
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1, std::placeholders::_2
	);
	/**
	 * Устанавливаем функцию обратного вызова на получение внешнего адреса маршрутизатора
	 *
	 * @note Ответ здесь означает, что обмен ушёл не туда, куда задумано: тесты ведутся
	 *       там, откуда ответ прийти не может, и пришедший ответ проверку обесценивает
	 */
	portmap.on <void (const std::string &, const awh::unit::portmap_t::type_t)> (
		"external", [&portmap, &result](const std::string &, const awh::unit::portmap_t::type_t type) noexcept -> void {
			// Запоминаем, что маршрутизатор ответил
			result.answered = true;
			// Запоминаем договор, по которому получен итог
			result.type = type;
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1, std::placeholders::_2
	);
	/**
	 * Устанавливаем функцию обратного вызова на заведение перенаправления порта
	 */
	portmap.on <void (const awh::unit::portmap_t::mapping_t &, const awh::unit::portmap_t::type_t)> (
		"mapped", [&portmap, &result](const awh::unit::portmap_t::mapping_t &, const awh::unit::portmap_t::type_t type) noexcept -> void {
			// Запоминаем, что маршрутизатор ответил
			result.answered = true;
			// Запоминаем договор, по которому получен итог
			result.type = type;
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1, std::placeholders::_2
	);
	/**
	 * Определяем просьбу, с которой ведётся обращение
	 */
	switch(static_cast <uint8_t> (action)){
		// Если запрашивается внешний адрес маршрутизатора
		case static_cast <uint8_t> (awh::unit::portmap_t::action_t::EXTERNAL):
			// Выполняем запрос внешнего адреса маршрутизатора
			portmap.external();
		break;
		// Если заводится перенаправление порта
		case static_cast <uint8_t> (awh::unit::portmap_t::action_t::OPEN): {
			// Заводимое перенаправление порта
			awh::unit::portmap_t::mapping_t mapping;
			// Устанавливаем договор перенаправляемого порта
			mapping.proto = awh::unit::portmap_t::proto_t::TCP;
			// Устанавливаем внутренний порт перенаправления
			mapping.internalPort = 8081;
			// Устанавливаем срок жизни перенаправления в секундах
			mapping.lifeTime = 60;
			// Выполняем заведение перенаправления порта
			portmap.open(mapping);
		} break;
	}
	// Выполняем запуск работы модуля
	portmap.start();
	// Выводим собранный итог обращения к маршрутизатору
	return result;
}
