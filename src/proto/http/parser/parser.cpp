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
		// Выполняем копирование партиции текущего состояния парсера
		this->part = message.part;
		// Выполняем копирование фазы разбора HTTP-сообщения
		this->phase = message.phase;
		// Выполняем копирование флагов состояния парсера
		this->flags = message.flags;
		// Выполняем перемещение провайдера заголовков сообщения
		this->provider = ::move(message.provider);
		// Выполняем копирование размера тела сообщения
		this->bodySize = message.bodySize;
		// Сбрасываем размер тела сообщения
		message.bodySize = -1;
		// Сбрасываем партицию текущего состояния парсера
		message.part = part_t::NONE;
		// Сбрасываем фазу разбора HTTP-сообщения
		message.phase = phase_t::NONE;
		// Сбрасываем флаги состояния парсера
		message.flags = flags_t();
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
		// Выполняем копирование партиции текущего состояния парсера
		this->part = message.part;
		// Выполняем копирование фазы разбора HTTP-сообщения
		this->phase = message.phase;
		// Выполняем копирование флагов состояния парсера
		this->flags = message.flags;
		// Выполняем копирование провайдера заголовков сообщения
		this->provider = message.provider->clone();
		// Выполняем копирование размера тела сообщения
		this->bodySize = message.bodySize;
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
	// Выполняем сравнение всех параметров сообщения кроме провайдера
	const bool result = (
		(this->part == message.part) &&
		(this->phase == message.phase) &&
		(this->flags.chunked == message.flags.chunked) &&
		(this->flags.complete == message.flags.complete) &&
		(this->flags.keepAlive == message.flags.keepAlive) &&
		(this->flags.upgrade == message.flags.upgrade) &&
		(this->flags.expectContinue == message.flags.expectContinue) &&
		(this->bodySize == message.bodySize)
	);
	// Если базовые параметры сообщения не совпадают - дальше сравнивать нет смысла
	if(!result)
		// Сообщения не эквивалентны
		return false;
	// Если оба провайдера отсутствуют - сообщения эквивалентны
	if((this->provider == nullptr) && (message.provider == nullptr))
		// Сообщения эквивалентны
		return true;
	// Если провайдер присутствует только у одного из сообщений
	if((this->provider == nullptr) || (message.provider == nullptr))
		// Сообщения не эквивалентны
		return false;
	// Если направления трафика провайдеров не совпадают
	if(this->provider->direct != message.provider->direct)
		// Сообщения не эквивалентны
		return false;
	/**
	 * В зависимости от направления трафика провайдера, сравниваем содержимое провайдеров
	 */
	switch(static_cast <uint8_t> (this->provider->direct)){
		// Если провайдер содержит запрос клиента
		case static_cast <uint8_t> (direct_t::REQUEST):
			// Сравниваем содержимое запросов клиента
			return ((* static_cast <const request_t *> (this->provider.get())) == (* static_cast <const request_t *> (message.provider.get())));
		// Если провайдер содержит ответ сервера
		case static_cast <uint8_t> (direct_t::RESPONSE):
			// Сравниваем содержимое ответов сервера
			return ((* static_cast <const response_t *> (this->provider.get())) == (* static_cast <const response_t *> (message.provider.get())));
	}
	// Сообщения эквивалентны
	return true;
}
/**
 * @brief Оператор сравнения
 *
 * @param message объект сообщения для сравнения
 * @return        результат сравнения
 */
bool awh::http::Parser::Message::operator != (const Message & message) noexcept {
	// Выполняем сравнение всех параметров сообщения
	return !((* this) == message);
}
/**
 * @brief Конструктор перемещения
 *
 * @param message объект сообщения для перемещения
 */
awh::http::Parser::Message::Message(Message && message) noexcept :
 part(message.part),
 phase(message.phase),
 flags(message.flags),
 bodySize(message.bodySize),
 provider(::move(message.provider)) {}
/**
 * @brief Конструктор копирования
 *
 * @param message объект сообщения для копирования
 */
awh::http::Parser::Message::Message(const Message & message) noexcept :
 part(message.part),
 phase(message.phase),
 flags(message.flags),
 bodySize(message.bodySize),
 provider(message.provider->clone()) {}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser::Message::Message() noexcept :
 part(part_t::NONE), phase(phase_t::NONE), bodySize(-1), provider(nullptr) {}

/**
 * @brief Метод полной очистки всех данных парсера
 *
 * @details Помимо сброса состояния разбора возвращает лимиты безопасности к значениям по умолчанию
 */
void awh::http::Parser::clear() noexcept {
	// Выполняем сброс состояния разбора
	this->reset();
	// Возвращаем лимиты безопасности к значениям по умолчанию
	this->_limits = limits_t();
}
/**
 * @brief Метод сброса парсера для разбора следующего сообщения в том же соединении
 *
 * @details Дешёвый сброс между сообщениями (keep-alive/pipelining): сохраняет лимиты
 *          безопасности и установленные функции обратного вызова, провайдер заголовков
 *          не пересоздаётся, а очищается (переиспользуется выделенная память).
 */
void awh::http::Parser::reset() noexcept {
	// Сбрасываем код ошибки разбора
	this->_error = error_t::NONE;
	// Сбрасываем итоговый статус разбора
	this->_status = status_t::NONE;
	// Сбрасываем флаги состояния парсера
	this->_message.flags = flags_t();
	// Сбрасываем партицию текущего состояния парсера
	this->_message.part = part_t::NONE;
	// Сбрасываем фазу разбора HTTP-сообщения
	this->_message.phase = phase_t::NONE;
	// Сбрасываем размер тела сообщения
	this->_message.bodySize = -1;
	// Если провайдер заголовков сообщения существует
	if(this->_message.provider != nullptr){
		/**
		 * В зависимости от направления потока данных, очищаем содержимое провайдера
		 */
		switch(static_cast <uint8_t> (this->_direct)){
			// Если выполняется разбор запроса клиента
			case static_cast <uint8_t> (direct_t::REQUEST): {
				// Получаем объект провайдера заголовков запроса клиента
				request_t * provider = static_cast <request_t *> (this->_message.provider.get());
				// Очищаем параметры URI-запроса (выделенная память строки сохраняется)
				provider->uri.clear();
				// Очищаем оригинальное написание метода запроса (выделенная память строки сохраняется)
				provider->methodName.clear();
				// Сбрасываем метод запроса клиента
				provider->method = method_t::NONE;
				// Сбрасываем версию протокола
				provider->version = version_t::NONE;
			} break;
			// Если выполняется разбор ответа сервера
			case static_cast <uint8_t> (direct_t::RESPONSE): {
				// Получаем объект провайдера заголовков ответа сервера
				response_t * provider = static_cast <response_t *> (this->_message.provider.get());
				// Сбрасываем код ответа сервера
				provider->code = 0;
				// Очищаем сообщение сервера (выделенная память строки сохраняется)
				provider->message.clear();
				// Сбрасываем версию протокола
				provider->version = version_t::NONE;
			} break;
		}
	}
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
 * @brief Метод получения итогового статуса разбора
 *
 * @return итоговый статус разбора
 */
awh::http::Parser::status_t awh::http::Parser::status() const noexcept {
	// Выводим итоговый статус разбора
	return this->_status;
}
/**
 * @brief Метод получения человекочитаемого названия кода ошибки
 *
 * @param error код ошибки разбора
 * @return      название кода ошибки
 */
string awh::http::Parser::errorName(const error_t error) noexcept {
	/**
	 * В зависимости от кода ошибки разбора, выводим соответствующее название
	 */
	switch(static_cast <uint8_t> (error)){
		// Ошибок нет
		case static_cast <uint8_t> (error_t::NONE):
			// Выводим название кода ошибки
			return "NONE";
		// Внутренняя ошибка состояния
		case static_cast <uint8_t> (error_t::INTERNAL):
			// Выводим название кода ошибки
			return "INTERNAL";
		// Ожидался LF после CR
		case static_cast <uint8_t> (error_t::INVALID_EOL):
			// Выводим название кода ошибки
			return "INVALID_EOL";
		// Недопустимый символ в методе
		case static_cast <uint8_t> (error_t::INVALID_METHOD):
			// Выводим название кода ошибки
			return "INVALID_METHOD";
		// Недопустимый символ в request-target
		case static_cast <uint8_t> (error_t::INVALID_TARGET):
			// Выводим название кода ошибки
			return "INVALID_TARGET";
		// Неверный статус-код ответа
		case static_cast <uint8_t> (error_t::INVALID_STATUS):
			// Выводим название кода ошибки
			return "INVALID_STATUS";
		// Неверная строка версии (HTTP/x.y)
		case static_cast <uint8_t> (error_t::INVALID_VERSION):
			// Выводим название кода ошибки
			return "INVALID_VERSION";
		// Ожидался литеральный символ (например, в "HTTP/")
		case static_cast <uint8_t> (error_t::INVALID_CONSTANT):
			// Выводим название кода ошибки
			return "INVALID_CONSTANT";
		// Неверный размер чанка
		case static_cast <uint8_t> (error_t::INVALID_CHUNK_SIZE):
			// Выводим название кода ошибки
			return "INVALID_CHUNK_SIZE";
		// Недопустимый символ в имени заголовка / obs-fold
		case static_cast <uint8_t> (error_t::INVALID_HEADER_TOKEN):
			// Выводим название кода ошибки
			return "INVALID_HEADER_TOKEN";
		// Недопустимый символ в значении заголовка
		case static_cast <uint8_t> (error_t::INVALID_HEADER_VALUE):
			// Выводим название кода ошибки
			return "INVALID_HEADER_VALUE";
		// Content-Length не число / некорректен
		case static_cast <uint8_t> (error_t::INVALID_CONTENT_LENGTH):
			// Выводим название кода ошибки
			return "INVALID_CONTENT_LENGTH";
		// Нет CRLF после данных чанка
		case static_cast <uint8_t> (error_t::INVALID_CHUNK_TERMINATOR):
			// Выводим название кода ошибки
			return "INVALID_CHUNK_TERMINATOR";
		// Некорректный Transfer-Encoding (chunked не последний и т.п.)
		case static_cast <uint8_t> (error_t::INVALID_TRANSFER_ENCODING):
			// Выводим название кода ошибки
			return "INVALID_TRANSFER_ENCODING";
		// Разбор прерван пользовательским callback'ом
		case static_cast <uint8_t> (error_t::ABORTED):
			// Выводим название кода ошибки
			return "ABORTED";
		// Превышен лимит длины request-line
		case static_cast <uint8_t> (error_t::URL_OVERFLOW):
			// Выводим название кода ошибки
			return "URL_OVERFLOW";
		// Превышен лимит размера тела
		case static_cast <uint8_t> (error_t::BODY_OVERFLOW):
			// Выводим название кода ошибки
			return "BODY_OVERFLOW";
		// Соединение закрыто посреди незавершённого сообщения
		case static_cast <uint8_t> (error_t::PREMATURE_EOF):
			// Выводим название кода ошибки
			return "PREMATURE_EOF";
		// Превышен лимит размера чанка
		case static_cast <uint8_t> (error_t::CHUNK_OVERFLOW):
			// Выводим название кода ошибки
			return "CHUNK_OVERFLOW";
		// Превышен лимит размера заголовков
		case static_cast <uint8_t> (error_t::HEADER_OVERFLOW):
			// Выводим название кода ошибки
			return "HEADER_OVERFLOW";
		// Превышено число заголовков
		case static_cast <uint8_t> (error_t::TOO_MANY_HEADERS):
			// Выводим название кода ошибки
			return "TOO_MANY_HEADERS";
		// CL+TE или несколько разных Content-Length (request smuggling)
		case static_cast <uint8_t> (error_t::CONTENT_LENGTH_CONFLICT):
			// Выводим название кода ошибки
			return "CONTENT_LENGTH_CONFLICT";
	}
	// Код ошибки неизвестен
	return "UNKNOWN_ERROR";
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
const awh::http::Parser::message_t & awh::http::Parser::message() const noexcept {
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
 _error(error_t::NONE), _status(status_t::NONE), _direct(direct), _fmk(fmk), _log(log) {
	/**
	 * В зависимости от направления потока данных, формируем объект провайдера заголовков сообщения
	 */
	switch(static_cast <uint8_t> (direct)){
		// Если передан запрос клиента
		case static_cast <uint8_t> (http::direct_t::REQUEST):
			// Формируем объект провайдера заголовков запроса клиента
			this->_message.provider = make_unique <request_t> ();
		break;
		// Если передан ответ сервера
		case static_cast <uint8_t> (http::direct_t::RESPONSE):
			// Формируем объект провайдера заголовков ответа сервера
			this->_message.provider = make_unique <response_t> ();
		break;
	}
}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser::~Parser() noexcept {}
