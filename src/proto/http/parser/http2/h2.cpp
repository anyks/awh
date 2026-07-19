/**
 * @file: h2.cpp
 * @date: 2026-07-19
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
#include <proto/http/parser/http2/h2.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения человекочитаемого названия типа фрейма
 *
 * @param type тип фрейма
 * @return     название типа фрейма
 */
const char * awh::http::h2::frameName(const frame_t type) noexcept {
	/**
	 * В зависимости от типа фрейма, выводим соответствующее название
	 */
	switch(static_cast <uint8_t> (type)){
		// Данные тела
		case static_cast <uint8_t> (frame_t::DATA):
			// Выводим название типа фрейма
			return "DATA";
		// Блок заголовков
		case static_cast <uint8_t> (frame_t::HEADERS):
			// Выводим название типа фрейма
			return "HEADERS";
		// Приоритет (deprecated)
		case static_cast <uint8_t> (frame_t::PRIORITY):
			// Выводим название типа фрейма
			return "PRIORITY";
		// Аварийное закрытие потока
		case static_cast <uint8_t> (frame_t::RST_STREAM):
			// Выводим название типа фрейма
			return "RST_STREAM";
		// Параметры соединения
		case static_cast <uint8_t> (frame_t::SETTINGS):
			// Выводим название типа фрейма
			return "SETTINGS";
		// Server push - резервирование потока
		case static_cast <uint8_t> (frame_t::PUSH_PROMISE):
			// Выводим название типа фрейма
			return "PUSH_PROMISE";
		// Проверка живости / измерение RTT
		case static_cast <uint8_t> (frame_t::PING):
			// Выводим название типа фрейма
			return "PING";
		// Завершение соединения
		case static_cast <uint8_t> (frame_t::GOAWAY):
			// Выводим название типа фрейма
			return "GOAWAY";
		// Обновление окна flow control
		case static_cast <uint8_t> (frame_t::WINDOW_UPDATE):
			// Выводим название типа фрейма
			return "WINDOW_UPDATE";
		// Продолжение блока заголовков
		case static_cast <uint8_t> (frame_t::CONTINUATION):
			// Выводим название типа фрейма
			return "CONTINUATION";
	}
	// Тип фрейма неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия кода ошибки
 *
 * @param code код ошибки протокола
 * @return     название кода ошибки
 */
const char * awh::http::h2::errorName(const error_t code) noexcept {
	/**
	 * В зависимости от кода ошибки протокола, выводим соответствующее название
	 */
	switch(static_cast <uint32_t> (code)){
		// Штатное завершение
		case static_cast <uint32_t> (error_t::NO_ERROR):
			// Выводим название кода ошибки
			return "NO_ERROR";
		// Нарушение протокола
		case static_cast <uint32_t> (error_t::PROTOCOL_ERROR):
			// Выводим название кода ошибки
			return "PROTOCOL_ERROR";
		// Внутренняя ошибка реализации
		case static_cast <uint32_t> (error_t::INTERNAL_ERROR):
			// Выводим название кода ошибки
			return "INTERNAL_ERROR";
		// Нарушение flow control
		case static_cast <uint32_t> (error_t::FLOW_CONTROL_ERROR):
			// Выводим название кода ошибки
			return "FLOW_CONTROL_ERROR";
		// Не получен ACK на SETTINGS
		case static_cast <uint32_t> (error_t::SETTINGS_TIMEOUT):
			// Выводим название кода ошибки
			return "SETTINGS_TIMEOUT";
		// Фрейм для закрытого потока
		case static_cast <uint32_t> (error_t::STREAM_CLOSED):
			// Выводим название кода ошибки
			return "STREAM_CLOSED";
		// Некорректный размер фрейма
		case static_cast <uint32_t> (error_t::FRAME_SIZE_ERROR):
			// Выводим название кода ошибки
			return "FRAME_SIZE_ERROR";
		// Поток отклонён до обработки
		case static_cast <uint32_t> (error_t::REFUSED_STREAM):
			// Выводим название кода ошибки
			return "REFUSED_STREAM";
		// Поток больше не нужен
		case static_cast <uint32_t> (error_t::CANCEL):
			// Выводим название кода ошибки
			return "CANCEL";
		// Ошибка состояния HPACK
		case static_cast <uint32_t> (error_t::COMPRESSION_ERROR):
			// Выводим название кода ошибки
			return "COMPRESSION_ERROR";
		// Ошибка соединения для метода CONNECT
		case static_cast <uint32_t> (error_t::CONNECT_ERROR):
			// Выводим название кода ошибки
			return "CONNECT_ERROR";
		// Обнаружено чрезмерное поведение (флуд)
		case static_cast <uint32_t> (error_t::ENHANCE_YOUR_CALM):
			// Выводим название кода ошибки
			return "ENHANCE_YOUR_CALM";
		// Недостаточный уровень безопасности TLS
		case static_cast <uint32_t> (error_t::INADEQUATE_SECURITY):
			// Выводим название кода ошибки
			return "INADEQUATE_SECURITY";
		// Требуется откат на HTTP/1.1
		case static_cast <uint32_t> (error_t::HTTP_1_1_REQUIRED):
			// Выводим название кода ошибки
			return "HTTP_1_1_REQUIRED";
	}
	// Код ошибки неизвестен
	return "UNKNOWN_ERROR";
}
