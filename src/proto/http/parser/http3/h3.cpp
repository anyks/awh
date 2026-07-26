/**
 * @file: h3.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация вспомогательных функций протокола HTTP/3 — распознавание зарезервированных и изъятых
 *        из употребления идентификаторов, получение человекочитаемых названий типов кадров,
 *        типов однонаправленных потоков и кодов ошибок для диагностики и логирования
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http3/h3.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция проверки принадлежности идентификатора зарезервированной последовательности
 *
 * @param value проверяемый идентификатор типа кадра либо параметра SETTINGS
 * @return      признак принадлежности зарезервированной последовательности
 *
 */
bool awh::http::h3::reserved(const uint64_t value) noexcept {
	// Если идентификатор меньше начала последовательности
	if(value < proto::GREASE_BASE)
		// Выводим отрицательный результат
		return false;
	/**
	 * Проверяем принадлежность идентификатора последовательности (0x1F * N + 0x21)
	 */
	return (((value - proto::GREASE_BASE) % proto::GREASE_STEP) == 0);
}
/**
 * @brief Функция проверки идентификатора на изъятый из употребления в HTTP/3
 *
 * @param type проверяемый тип кадра
 * @return     признак изъятого из употребления типа кадра
 *
 */
bool awh::http::h3::retired(const uint64_t type) noexcept {
	/**
	 * Типы кадров, занятые в HTTP/2 кадрами PRIORITY, PING, WINDOW_UPDATE и CONTINUATION
	 */
	return ((type == 0x02) || (type == 0x06) || (type == 0x08) || (type == 0x09));
}
/**
 * @brief Функция проверки параметра SETTINGS на изъятый из употребления в HTTP/3
 *
 * @param identifier проверяемый идентификатор параметра
 * @return           признак изъятого из употребления параметра
 *
 */
bool awh::http::h3::retiredSetting(const uint64_t identifier) noexcept {
	/**
	 * Параметры, занятые в HTTP/2 ENABLE_PUSH, MAX_CONCURRENT_STREAMS,
	 * INITIAL_WINDOW_SIZE и MAX_FRAME_SIZE
	 */
	return ((identifier >= 0x02) && (identifier <= 0x05));
}
/**
 * @brief Функция получения человекочитаемого названия типа кадра
 *
 * @param type тип кадра
 * @return     название типа кадра
 *
 */
string_view awh::http::h3::frameName(const frame_t type) noexcept {
	/**
	 * В зависимости от типа кадра, выводим соответствующее название
	 */
	switch(static_cast <uint64_t> (type)){
		// Данные тела
		case static_cast <uint64_t> (frame_t::DATA):
			// Выводим название типа кадра
			return "DATA";
		// Секция полей заголовков либо трейлеров
		case static_cast <uint64_t> (frame_t::HEADERS):
			// Выводим название типа кадра
			return "HEADERS";
		// Отмена обещанного push
		case static_cast <uint64_t> (frame_t::CANCEL_PUSH):
			// Выводим название типа кадра
			return "CANCEL_PUSH";
		// Параметры соединения
		case static_cast <uint64_t> (frame_t::SETTINGS):
			// Выводим название типа кадра
			return "SETTINGS";
		// Server push - обещание запроса
		case static_cast <uint64_t> (frame_t::PUSH_PROMISE):
			// Выводим название типа кадра
			return "PUSH_PROMISE";
		// Завершение соединения
		case static_cast <uint64_t> (frame_t::GOAWAY):
			// Выводим название типа кадра
			return "GOAWAY";
		// Верхняя граница идентификаторов push
		case static_cast <uint64_t> (frame_t::MAX_PUSH_ID):
			// Выводим название типа кадра
			return "MAX_PUSH_ID";
		// Обновление расширенного приоритета потока запроса
		case static_cast <uint64_t> (frame_t::PRIORITY_UPDATE_REQUEST):
			// Выводим название типа кадра
			return "PRIORITY_UPDATE_REQUEST";
		// Обновление расширенного приоритета потока push
		case static_cast <uint64_t> (frame_t::PRIORITY_UPDATE_PUSH):
			// Выводим название типа кадра
			return "PRIORITY_UPDATE_PUSH";
	}
	// Если тип кадра принадлежит зарезервированной последовательности
	if(reserved(static_cast <uint64_t> (type)))
		// Выводим название зарезервированного типа кадра
		return "RESERVED";
	// Тип кадра неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия типа однонаправленного потока
 *
 * @param type тип однонаправленного потока
 * @return     название типа потока
 *
 */
string_view awh::http::h3::unistreamName(const unistream_t type) noexcept {
	/**
	 * В зависимости от типа потока, выводим соответствующее название
	 */
	switch(static_cast <uint64_t> (type)){
		// Управляющий поток соединения
		case static_cast <uint64_t> (unistream_t::CONTROL):
			// Выводим название типа потока
			return "CONTROL";
		// Поток server push
		case static_cast <uint64_t> (unistream_t::PUSH):
			// Выводим название типа потока
			return "PUSH";
		// Поток инструкций кодера QPACK
		case static_cast <uint64_t> (unistream_t::QPACK_ENCODER):
			// Выводим название типа потока
			return "QPACK_ENCODER";
		// Поток инструкций декодера QPACK
		case static_cast <uint64_t> (unistream_t::QPACK_DECODER):
			// Выводим название типа потока
			return "QPACK_DECODER";
	}
	// Если тип потока принадлежит зарезервированной последовательности
	if(reserved(static_cast <uint64_t> (type)))
		// Выводим название зарезервированного типа потока
		return "RESERVED";
	// Тип потока неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия кода ошибки
 *
 * @param code код ошибки протокола
 * @return     название кода ошибки
 *
 */
string_view awh::http::h3::errorName(const error_t code) noexcept {
	/**
	 * В зависимости от кода ошибки протокола, выводим соответствующее название
	 */
	switch(static_cast <uint64_t> (code)){
		// Штатное завершение
		case static_cast <uint64_t> (error_t::H3_NO_ERROR):
			// Выводим название кода ошибки
			return "H3_NO_ERROR";
		// Нарушение протокола без более точного кода
		case static_cast <uint64_t> (error_t::H3_GENERAL_PROTOCOL_ERROR):
			// Выводим название кода ошибки
			return "H3_GENERAL_PROTOCOL_ERROR";
		// Внутренняя ошибка реализации
		case static_cast <uint64_t> (error_t::H3_INTERNAL_ERROR):
			// Выводим название кода ошибки
			return "H3_INTERNAL_ERROR";
		// Поток создан или использован недопустимым образом
		case static_cast <uint64_t> (error_t::H3_STREAM_CREATION_ERROR):
			// Выводим название кода ошибки
			return "H3_STREAM_CREATION_ERROR";
		// Закрыт поток, обязанный жить всё соединение
		case static_cast <uint64_t> (error_t::H3_CLOSED_CRITICAL_STREAM):
			// Выводим название кода ошибки
			return "H3_CLOSED_CRITICAL_STREAM";
		// Кадр недопустим в этом потоке либо в этот момент
		case static_cast <uint64_t> (error_t::H3_FRAME_UNEXPECTED):
			// Выводим название кода ошибки
			return "H3_FRAME_UNEXPECTED";
		// Кадр нарушает требования к своей нагрузке
		case static_cast <uint64_t> (error_t::H3_FRAME_ERROR):
			// Выводим название кода ошибки
			return "H3_FRAME_ERROR";
		// Обнаружено чрезмерное поведение
		case static_cast <uint64_t> (error_t::H3_EXCESSIVE_LOAD):
			// Выводим название кода ошибки
			return "H3_EXCESSIVE_LOAD";
		// Идентификатор вне допустимых границ
		case static_cast <uint64_t> (error_t::H3_ID_ERROR):
			// Выводим название кода ошибки
			return "H3_ID_ERROR";
		// Недопустимое содержимое кадра SETTINGS
		case static_cast <uint64_t> (error_t::H3_SETTINGS_ERROR):
			// Выводим название кода ошибки
			return "H3_SETTINGS_ERROR";
		// Управляющий поток начат не кадром SETTINGS
		case static_cast <uint64_t> (error_t::H3_MISSING_SETTINGS):
			// Выводим название кода ошибки
			return "H3_MISSING_SETTINGS";
		// Запрос отклонён до обработки
		case static_cast <uint64_t> (error_t::H3_REQUEST_REJECTED):
			// Выводим название кода ошибки
			return "H3_REQUEST_REJECTED";
		// Запрос отменён либо его обработка прекращена
		case static_cast <uint64_t> (error_t::H3_REQUEST_CANCELLED):
			// Выводим название кода ошибки
			return "H3_REQUEST_CANCELLED";
		// Поток запроса закрыт до полной передачи сообщения
		case static_cast <uint64_t> (error_t::H3_REQUEST_INCOMPLETE):
			// Выводим название кода ошибки
			return "H3_REQUEST_INCOMPLETE";
		// Сообщение нарушает семантику HTTP
		case static_cast <uint64_t> (error_t::H3_MESSAGE_ERROR):
			// Выводим название кода ошибки
			return "H3_MESSAGE_ERROR";
		// Соединение метода CONNECT оборвалось либо не установлено
		case static_cast <uint64_t> (error_t::H3_CONNECT_ERROR):
			// Выводим название кода ошибки
			return "H3_CONNECT_ERROR";
		// Запрошенный ресурс доступен только по другой версии HTTP
		case static_cast <uint64_t> (error_t::H3_VERSION_FALLBACK):
			// Выводим название кода ошибки
			return "H3_VERSION_FALLBACK";
		// Секция полей не разбирается - состояние QPACK разошлось
		case static_cast <uint64_t> (error_t::QPACK_DECOMPRESSION_FAILED):
			// Выводим название кода ошибки
			return "QPACK_DECOMPRESSION_FAILED";
		// Ошибка в потоке инструкций кодера QPACK
		case static_cast <uint64_t> (error_t::QPACK_ENCODER_STREAM_ERROR):
			// Выводим название кода ошибки
			return "QPACK_ENCODER_STREAM_ERROR";
		// Ошибка в потоке инструкций декодера QPACK
		case static_cast <uint64_t> (error_t::QPACK_DECODER_STREAM_ERROR):
			// Выводим название кода ошибки
			return "QPACK_DECODER_STREAM_ERROR";
	}
	// Код ошибки неизвестен
	return "UNKNOWN_ERROR";
}
