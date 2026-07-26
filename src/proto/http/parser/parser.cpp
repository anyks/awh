/**
 * @file: parser.cpp
 * @date: 2026-07-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация базового класса HTTP-парсера — общая логика управления фазами разбора, частями сообщения,
 *        статусами и базовыми лимитами, разделяемая парсерами HTTP/1.x и HTTP/2
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/parser.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::http::Parser::Limits::Limits() noexcept :
 maxHeaderName(MAX_HEADER_NAME),
 maxHeaderValue(MAX_HEADER_VALUE),
 maxHeaderCount(MAX_HEADER_COUNT),
 maxHeadersTotal(MAX_HEADERS_TOTAL),
 maxBodySize(MAX_BODY_SIZE) {}

/**
 * @brief Метод получения итогового статуса разбора
 *
 * @return итоговый статус разбора
 *
 */
awh::http::Parser::status_t awh::http::Parser::status() const noexcept {
	// Выводим итоговый статус разбора
	return this->_status;
}
/**
 * @brief Метод полной очистки всех данных парсера
 *
 * @details Помимо сброса состояния разбора возвращает настройки парсера
 *          (лимиты безопасности, функции обратного вызова) к значениям
 *          по умолчанию — детали определяются классом-наследником
 *
 */
void awh::http::Parser::clear() noexcept {
	// Выполняем сброс состояния разбора
	this->reset();
}
/**
 * @brief Метод сброса состояния парсера
 *
 * @details Дешёвый сброс с сохранением настроек (лимиты безопасности,
 *          функции обратного вызова): для HTTP/1.x — подготовка к разбору
 *          следующего сообщения в том же соединении (keep-alive/pipelining),
 *          для мультиплексируемых протоколов — полный сброс соединения
 *
 */
void awh::http::Parser::reset() noexcept {
	// Сбрасываем итоговый статус разбора
	this->_status = status_t::NONE;
}
/**
 * @brief Конструктор
 *
 * @param direct направление потока данных
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::http::Parser::Parser(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 _status(status_t::NONE), _direct(direct), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser::~Parser() noexcept {}
