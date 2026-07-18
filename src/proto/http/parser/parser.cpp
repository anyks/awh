/**
 * @file: parser.cpp
 * @date: 2026-07-18
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
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/parser.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Оператор перемещающего присваивания параметров сообщения
 *
 * @param message объект сообщения для перемещения
 * @return        текущее сообщение
 */
awh::http::Parser::Message & awh::http::Parser::Message::operator = (Message && message) noexcept {
	// Если перемещаемое сообщение не является текущим объектом
	if(this != &message){
		// Выполняем копирование флагов состояния парсера
		this->flags = message.flags;
		// Выполняем копирование версии протокола HTTP
		this->version = message.version;
		// Выполняем копирование размера тела сообщения
		this->body.size = message.body.size;
		// Выполняем перемещение данных тела сообщения
		this->body.body = ::move(message.body.body);
		// Выполняем перемещение заголовков сообщения
		this->headers = ::move(message.headers);
		// Выполняем перемещение трейлеров сообщения
		this->trailers = ::move(message.trailers);
		// Сбрасываем размер тела сообщения
		message.body.size = 0;
		// Сбрасываем флаги состояния парсера
		message.flags = flags_t();
		// Сбрасываем версию протокола HTTP
		message.version = version_t();
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Оператор присваивания параметров сообщения
 *
 * @param message объект сообщения для копирования
 * @return        текущее сообщение
 */
awh::http::Parser::Message & awh::http::Parser::Message::operator = (const Message & message) noexcept {
	// Если копируемое сообщение не является текущим объектом
	if(this != &message){
		// Выполняем копирование флагов состояния парсера
		this->flags = message.flags;
		// Выполняем копирование версии протокола HTTP
		this->version = message.version;
		// Выполняем копирование размера тела сообщения
		this->body.size = message.body.size;
		// Выполняем копирование данных тела сообщения
		this->body.body = message.body.body;
		// Выполняем копирование заголовков сообщения
		this->headers = message.headers;
		// Выполняем копирование трейлеров сообщения
		this->trailers = message.trailers;
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Оператор сравнения
 *
 * @param message объект сообщения для сравнения
 * @return        результат сравнения
 */
bool awh::http::Parser::Message::operator == (const Message & message) noexcept {
	// Выполняем сравнение всех параметров сообщения
	return (
		(this->version.major == message.version.major) &&
		(this->version.minor == message.version.minor) &&
		(this->flags.chunked == message.flags.chunked) &&
		(this->flags.complete == message.flags.complete) &&
		(this->flags.keepAlive == message.flags.keepAlive) &&
		(this->flags.hasContentLength == message.flags.hasContentLength) &&
		(this->body.size == message.body.size) &&
		(this->body.body == message.body.body) &&
		(this->headers == message.headers) &&
		(this->trailers == message.trailers)
	);
}
/**
 * @brief Конструктор перемещения
 *
 * @param message объект сообщения для перемещения
 */
awh::http::Parser::Message::Message(Message && message) noexcept :
 flags(message.flags), body(::move(message.body)),
 headers(::move(message.headers)), trailers(::move(message.trailers)),
 version(message.version) {}
/**
 * @brief Конструктор копирования
 *
 * @param message объект сообщения для копирования
 */
awh::http::Parser::Message::Message(const Message & message) noexcept :
 flags(message.flags), body(message.body),
 headers(message.headers), trailers(message.trailers),
 version(message.version) {}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::http::Parser::Message::Message(const fmk_t * fmk, const log_t * log) noexcept :
 body(fmk, log), headers(fmk, log), trailers(fmk, log) {}

/**
 * @brief Метод очистки всех данных парсера
 *
 */
void awh::http::Parser::clear() noexcept {
	// Сбрасываем код ошибки разбора
	this->_error = error_t::NONE;
	// Сбрасываем флаги состояния парсера
	this->_message.flags = flags_t();
	// Сбрасываем версию протокола HTTP
	this->_message.version = version_t();
	// Обнуляем размер тела сообщения
	this->_message.body.size = 0;
	// Очищаем буфер тела сообщения
	this->_message.body.body.clear();
	// Очищаем заголовки сообщения
	this->_message.headers.clear();
	// Очищаем трейлеры сообщения
	this->_message.trailers.clear();
}
/**
 * @brief Метод получения кода ошибки разбора
 *
 * @return код ошибки
 */
awh::http::Parser::error_t awh::http::Parser::error() const noexcept {
	// Выводим код ошибки разбора
	return this->_error;
}
/**
 * @brief Метод получения лимитов безопасности
 *
 * @return лимиты безопасности
 */
const awh::http::Parser::limits_t & awh::http::Parser::limits() const noexcept {
	// Выводим настроенные лимиты безопасности
	return this->_limits;
}
/**
 * @brief Метод установки лимитов безопасности
 *
 * @param limits лимиты безопасности
 */
void awh::http::Parser::limits(const limits_t & limits) noexcept {
	// Устанавливаем новые лимиты безопасности
	this->_limits = limits;
}
/**
 * @brief Метод получения разобранного сообщения
 *
 * @return разобранное сообщение
 */
const awh::http::Parser::message_t & awh::http::Parser::message() noexcept {
	// Выводим результат разбора сообщения
	return this->_message;
}
/**
 * @brief Конструктор
 *
 * @param direct направление потока данных
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::http::Parser::Parser(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 _error(error_t::NONE), _message(fmk, log), _fmk(fmk), _log(log) {
	/**
	 * В зависимости от направления потока данных, формируем объект провайдера заголовков сообщения
	 */
	switch(static_cast <uint8_t> (direct)){
		// Если передан запрос клиента
		case static_cast <uint8_t> (http::direct_t::REQUEST):
			// Формируем объект провайдера заголовков запроса клиента
			this->_message.headers.provider(make_unique <request_t> ());
		break;
		// Если передан ответ сервера
		case static_cast <uint8_t> (http::direct_t::RESPONSE):
			// Формируем объект провайдера заголовков ответа сервера
			this->_message.headers.provider(make_unique <response_t> ());
		break;
	}
}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser::~Parser() noexcept {}
