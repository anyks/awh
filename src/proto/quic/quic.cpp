/**
 * @file: quic.cpp
 * @date: 2026-07-21
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
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/quic/quic.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::quic::Cid::Cid() noexcept : size(0), data{0} {}

/**
 * @brief Оператор сравнения идентификаторов соединения
 *
 * @param a первый идентификатор соединения
 * @param b второй идентификатор соединения
 * @return  результат сравнения
 */
bool awh::quic::operator == (const cid_t & a, const cid_t & b) noexcept {
	// Идентификаторы равны при совпадении длины и содержимого
	return ((a.size == b.size) && (::memcmp(a.data, b.data, a.size) == 0));
}
/**
 * @brief Функция получения человекочитаемого названия типа пакета
 *
 * @param type тип пакета
 * @return     название типа пакета
 */
string_view awh::quic::packetName(const packet_t type) noexcept {
	/**
	 * В зависимости от типа пакета, выводим соответствующее название
	 */
	switch(static_cast <uint8_t> (type)){
		// Установка соединения
		case static_cast <uint8_t> (packet_t::INITIAL):
			// Выводим название типа пакета
			return "INITIAL";
		// Ранние данные клиента
		case static_cast <uint8_t> (packet_t::ZERO_RTT):
			// Выводим название типа пакета
			return "0-RTT";
		// Продолжение криптографического хендшейка
		case static_cast <uint8_t> (packet_t::HANDSHAKE):
			// Выводим название типа пакета
			return "HANDSHAKE";
		// Проверка адреса клиента сервером
		case static_cast <uint8_t> (packet_t::RETRY):
			// Выводим название типа пакета
			return "RETRY";
		// Согласование версии протокола
		case static_cast <uint8_t> (packet_t::VERSION_NEGOTIATION):
			// Выводим название типа пакета
			return "VERSION_NEGOTIATION";
		// Пакет с коротким заголовком
		case static_cast <uint8_t> (packet_t::ONE_RTT):
			// Выводим название типа пакета
			return "1-RTT";
	}
	// Тип пакета неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия типа фрейма
 *
 * @param type тип фрейма
 * @return     название типа фрейма
 */
string_view awh::quic::frameName(const frame_t type) noexcept {
	// Значение типа фрейма
	const uint64_t value = static_cast <uint64_t> (type);
	// Если тип фрейма принадлежит диапазону STREAM (0x08-0x0F)
	if((value >= static_cast <uint64_t> (frame_t::STREAM)) && (value <= (static_cast <uint64_t> (frame_t::STREAM) | 0x07)))
		// Выводим название типа фрейма
		return "STREAM";
	/**
	 * В зависимости от типа фрейма, выводим соответствующее название
	 */
	switch(value){
		// Заполнение без полезной нагрузки
		case static_cast <uint64_t> (frame_t::PADDING):
			// Выводим название типа фрейма
			return "PADDING";
		// Проверка живости пути
		case static_cast <uint64_t> (frame_t::PING):
			// Выводим название типа фрейма
			return "PING";
		// Подтверждение приёма пакетов
		case static_cast <uint64_t> (frame_t::ACK):
			// Выводим название типа фрейма
			return "ACK";
		// Подтверждение приёма пакетов со счётчиками ECN
		case static_cast <uint64_t> (frame_t::ACK_ECN):
			// Выводим название типа фрейма
			return "ACK_ECN";
		// Аварийное завершение потока отправителем
		case static_cast <uint64_t> (frame_t::RESET_STREAM):
			// Выводим название типа фрейма
			return "RESET_STREAM";
		// Запрос прекращения передачи потока
		case static_cast <uint64_t> (frame_t::STOP_SENDING):
			// Выводим название типа фрейма
			return "STOP_SENDING";
		// Данные криптографического хендшейка
		case static_cast <uint64_t> (frame_t::CRYPTO):
			// Выводим название типа фрейма
			return "CRYPTO";
		// Токен для будущих соединений клиента
		case static_cast <uint64_t> (frame_t::NEW_TOKEN):
			// Выводим название типа фрейма
			return "NEW_TOKEN";
		// Обновление лимита данных соединения
		case static_cast <uint64_t> (frame_t::MAX_DATA):
			// Выводим название типа фрейма
			return "MAX_DATA";
		// Обновление лимита данных потока
		case static_cast <uint64_t> (frame_t::MAX_STREAM_DATA):
			// Выводим название типа фрейма
			return "MAX_STREAM_DATA";
		// Обновление лимита двунаправленных потоков
		case static_cast <uint64_t> (frame_t::MAX_STREAMS_BIDI):
			// Выводим название типа фрейма
			return "MAX_STREAMS_BIDI";
		// Обновление лимита однонаправленных потоков
		case static_cast <uint64_t> (frame_t::MAX_STREAMS_UNI):
			// Выводим название типа фрейма
			return "MAX_STREAMS_UNI";
		// Блокировка лимитом данных соединения
		case static_cast <uint64_t> (frame_t::DATA_BLOCKED):
			// Выводим название типа фрейма
			return "DATA_BLOCKED";
		// Блокировка лимитом данных потока
		case static_cast <uint64_t> (frame_t::STREAM_DATA_BLOCKED):
			// Выводим название типа фрейма
			return "STREAM_DATA_BLOCKED";
		// Блокировка лимитом двунаправленных потоков
		case static_cast <uint64_t> (frame_t::STREAMS_BLOCKED_BIDI):
			// Выводим название типа фрейма
			return "STREAMS_BLOCKED_BIDI";
		// Блокировка лимитом однонаправленных потоков
		case static_cast <uint64_t> (frame_t::STREAMS_BLOCKED_UNI):
			// Выводим название типа фрейма
			return "STREAMS_BLOCKED_UNI";
		// Анонс нового идентификатора соединения
		case static_cast <uint64_t> (frame_t::NEW_CONNECTION_ID):
			// Выводим название типа фрейма
			return "NEW_CONNECTION_ID";
		// Вывод идентификатора соединения из обращения
		case static_cast <uint64_t> (frame_t::RETIRE_CONNECTION_ID):
			// Выводим название типа фрейма
			return "RETIRE_CONNECTION_ID";
		// Проверка достижимости пути
		case static_cast <uint64_t> (frame_t::PATH_CHALLENGE):
			// Выводим название типа фрейма
			return "PATH_CHALLENGE";
		// Ответ на проверку достижимости пути
		case static_cast <uint64_t> (frame_t::PATH_RESPONSE):
			// Выводим название типа фрейма
			return "PATH_RESPONSE";
		// Завершение соединения с ошибкой транспорта
		case static_cast <uint64_t> (frame_t::CONNECTION_CLOSE):
			// Выводим название типа фрейма
			return "CONNECTION_CLOSE";
		// Завершение соединения с ошибкой приложения
		case static_cast <uint64_t> (frame_t::CONNECTION_CLOSE_APP):
			// Выводим название типа фрейма
			return "CONNECTION_CLOSE_APP";
		// Подтверждение завершения хендшейка сервером
		case static_cast <uint64_t> (frame_t::HANDSHAKE_DONE):
			// Выводим название типа фрейма
			return "HANDSHAKE_DONE";
	}
	// Тип фрейма неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия кода ошибки транспорта
 *
 * @param code код ошибки транспорта
 * @return     название кода ошибки
 */
string_view awh::quic::errorName(const error_t code) noexcept {
	// Значение кода ошибки транспорта
	const uint64_t value = static_cast <uint64_t> (code);
	// Если код ошибки принадлежит диапазону ошибок TLS-хендшейка (0x0100-0x01FF)
	if((value >= static_cast <uint64_t> (error_t::CRYPTO_ERROR)) && (value <= (static_cast <uint64_t> (error_t::CRYPTO_ERROR) | 0xFF)))
		// Выводим название кода ошибки
		return "CRYPTO_ERROR";
	/**
	 * В зависимости от кода ошибки транспорта, выводим соответствующее название
	 */
	switch(value){
		// Штатное завершение
		case static_cast <uint64_t> (error_t::NO_ERROR):
			// Выводим название кода ошибки
			return "NO_ERROR";
		// Внутренняя ошибка реализации
		case static_cast <uint64_t> (error_t::INTERNAL_ERROR):
			// Выводим название кода ошибки
			return "INTERNAL_ERROR";
		// Сервер отказал в приёме соединения
		case static_cast <uint64_t> (error_t::CONNECTION_REFUSED):
			// Выводим название кода ошибки
			return "CONNECTION_REFUSED";
		// Нарушение flow control
		case static_cast <uint64_t> (error_t::FLOW_CONTROL_ERROR):
			// Выводим название кода ошибки
			return "FLOW_CONTROL_ERROR";
		// Превышен лимит числа потоков
		case static_cast <uint64_t> (error_t::STREAM_LIMIT_ERROR):
			// Выводим название кода ошибки
			return "STREAM_LIMIT_ERROR";
		// Фрейм недопустим в текущем состоянии потока
		case static_cast <uint64_t> (error_t::STREAM_STATE_ERROR):
			// Выводим название кода ошибки
			return "STREAM_STATE_ERROR";
		// Нарушение финального размера потока
		case static_cast <uint64_t> (error_t::FINAL_SIZE_ERROR):
			// Выводим название кода ошибки
			return "FINAL_SIZE_ERROR";
		// Некорректное кодирование фрейма
		case static_cast <uint64_t> (error_t::FRAME_ENCODING_ERROR):
			// Выводим название кода ошибки
			return "FRAME_ENCODING_ERROR";
		// Некорректные параметры транспорта
		case static_cast <uint64_t> (error_t::TRANSPORT_PARAMETER_ERROR):
			// Выводим название кода ошибки
			return "TRANSPORT_PARAMETER_ERROR";
		// Превышен лимит идентификаторов соединения
		case static_cast <uint64_t> (error_t::CONNECTION_ID_LIMIT_ERROR):
			// Выводим название кода ошибки
			return "CONNECTION_ID_LIMIT_ERROR";
		// Общее нарушение протокола
		case static_cast <uint64_t> (error_t::PROTOCOL_VIOLATION):
			// Выводим название кода ошибки
			return "PROTOCOL_VIOLATION";
		// Некорректный токен в пакете Initial
		case static_cast <uint64_t> (error_t::INVALID_TOKEN):
			// Выводим название кода ошибки
			return "INVALID_TOKEN";
		// Ошибка приложения
		case static_cast <uint64_t> (error_t::APPLICATION_ERROR):
			// Выводим название кода ошибки
			return "APPLICATION_ERROR";
		// Переполнен буфер CRYPTO-данных
		case static_cast <uint64_t> (error_t::CRYPTO_BUFFER_EXCEEDED):
			// Выводим название кода ошибки
			return "CRYPTO_BUFFER_EXCEEDED";
		// Ошибка обновления ключей
		case static_cast <uint64_t> (error_t::KEY_UPDATE_ERROR):
			// Выводим название кода ошибки
			return "KEY_UPDATE_ERROR";
		// Достигнут лимит использования AEAD-ключей
		case static_cast <uint64_t> (error_t::AEAD_LIMIT_REACHED):
			// Выводим название кода ошибки
			return "AEAD_LIMIT_REACHED";
		// Нет пригодного сетевого пути
		case static_cast <uint64_t> (error_t::NO_VIABLE_PATH):
			// Выводим название кода ошибки
			return "NO_VIABLE_PATH";
		// Согласование версии не дало общей версии
		case static_cast <uint64_t> (error_t::VERSION_NEGOTIATION_ERROR):
			// Выводим название кода ошибки
			return "VERSION_NEGOTIATION_ERROR";
	}
	// Код ошибки неизвестен
	return "UNKNOWN_ERROR";
}
