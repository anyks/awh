/**
 * @file: frame.cpp
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
#include <proto/quic/frame.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные функции слоя фреймов
 *
 */
namespace {
	/**
	 * Используем пространство имён протокола QUIC
	 */
	using namespace awh::quic;

	/**
	 * @brief Функция чтения типа фрейма с проверкой соответствия ожидаемому
	 *
	 * @param data     буфер расшифрованной нагрузки (начало фрейма)
	 * @param size     доступно байт
	 * @param expected ожидаемый тип фрейма
	 * @param offset   смещение разбора (сдвигается за тип фрейма)
	 * @return         результат чтения (false - тип не соответствует или данных недостаточно)
	 */
	inline bool rdType(const uint8_t * data, const size_t size, const frame_t expected, size_t & offset) noexcept {
		// Прочитанное значение типа фрейма
		uint64_t value = 0;
		// Читаем тип фрейма
		const size_t count = varint::read(data, size, value);
		// Если данных недостаточно или тип не соответствует ожидаемому
		if((count == 0) || (value != static_cast <uint64_t> (expected)))
			// Чтение невозможно
			return false;
		// Сдвигаем смещение разбора за типом фрейма
		offset = count;
		// Чтение выполнено успешно
		return true;
	}
	/**
	 * @brief Функция чтения целого числа переменной длины со сдвигом смещения
	 *
	 * @param data   буфер расшифрованной нагрузки
	 * @param size   доступно байт
	 * @param offset смещение разбора (сдвигается за числом)
	 * @param value  прочитанное число
	 * @return       результат чтения (false - данных недостаточно)
	 */
	inline bool rdVarint(const uint8_t * data, const size_t size, size_t & offset, uint64_t & value) noexcept {
		// Читаем целое число переменной длины
		const size_t count = varint::read(data + offset, size - offset, value);
		// Если данных недостаточно
		if(count == 0)
			// Чтение невозможно
			return false;
		// Сдвигаем смещение разбора за числом
		offset += count;
		// Чтение выполнено успешно
		return true;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Range::Range() noexcept : low(0), high(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Ack::Ack() noexcept :
 hasEcn(false), delay(0), ect0(0), ect1(0), ce(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Reset_Stream::Reset_Stream() noexcept :
 streamId(0), code(0), finalSize(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Stop_Sending::Stop_Sending() noexcept :
 streamId(0), code(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Crypto::Crypto() noexcept : offset(0), data{""} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Stream::Stream() noexcept :
 fin(false), streamId(0), offset(0), data{""} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::New_Connection_Id::New_Connection_Id() noexcept :
 seq(0), retirePriorTo(0), resetToken{0} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::frame::Connection_Close::Connection_Close() noexcept :
 app(false), code(0), frameType(0), reason{""} {}

/**
 * @brief Функция определения типа очередного фрейма в нагрузке
 *
 * @param data   буфер расшифрованной нагрузки (начало очередного фрейма)
 * @param size   доступно байт
 * @param output определённый тип фрейма (UNKNOWN - тип не распознан)
 * @return       результат определения (true - в буфере было достаточно байт)
 */
bool awh::quic::frame::parser::type(const uint8_t * data, const size_t size, frame_t & output) noexcept {
	// Прочитанное значение типа фрейма
	uint64_t value = 0;
	// Читаем тип фрейма
	if(varint::read(data, size, value) == 0)
		// Данных недостаточно
		return false;
	// Если тип фрейма принадлежит диапазону STREAM (0x08-0x0F)
	if((value >= static_cast <uint64_t> (frame_t::STREAM)) && (value <= (static_cast <uint64_t> (frame_t::STREAM) | 0x07))){
		// Устанавливаем базовый тип фрейма STREAM
		output = frame_t::STREAM;
		// Определение выполнено успешно
		return true;
	}
	// Если тип фрейма принадлежит диапазону DATAGRAM (0x30-0x31)
	if((value >= static_cast <uint64_t> (frame_t::DATAGRAM)) && (value <= (static_cast <uint64_t> (frame_t::DATAGRAM) | 0x01))){
		// Устанавливаем базовый тип фрейма DATAGRAM
		output = frame_t::DATAGRAM;
		// Определение выполнено успешно
		return true;
	}
	// Если тип фрейма известен протоколу
	if(value <= static_cast <uint64_t> (frame_t::HANDSHAKE_DONE))
		// Устанавливаем определённый тип фрейма
		output = static_cast <frame_t> (value);
	// Тип фрейма не распознан
	else output = frame_t::UNKNOWN;
	// Определение выполнено успешно
	return true;
}
/**
 * @brief Функция разбора последовательности фреймов PADDING (RFC 9000 §19.1)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param consumed количество потреблённых октетов (длина серии PADDING)
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::padding(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept {
	// Если буфер не начинается с фрейма PADDING
	if((size < 1) || (data[0] != static_cast <uint8_t> (frame_t::PADDING))){
		// Устанавливаем код ошибки транспорта
		error = error_t::FRAME_ENCODING_ERROR;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Обнуляем количество потреблённых октетов
	consumed = 0;
	/**
	 *  Поглощаем серию фреймов PADDING целиком
	 */
	while((consumed < size) && (data[consumed] == static_cast <uint8_t> (frame_t::PADDING)))
		// Учитываем очередной октет заполнения
		consumed++;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма PING (RFC 9000 §19.2)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::ping(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept {
	// Если буфер не начинается с фрейма PING
	if(!rdType(data, size, frame_t::PING, consumed)){
		// Устанавливаем код ошибки транспорта
		error = error_t::FRAME_ENCODING_ERROR;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма ACK/ACK_ECN (RFC 9000 §19.3)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::ack(const uint8_t * data, const size_t size, ack_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Очищаем список диапазонов от результатов предыдущего разбора
	output.ranges.clear();
	// Смещение разбора
	size_t offset = 0;
	// Прочитанное значение типа фрейма
	uint64_t value = 0;
	// Читаем тип фрейма
	if(!rdVarint(data, size, offset, value))
		// Разбор невозможен
		return status_t::ERROR;
	// Если тип фрейма не соответствует ACK или ACK_ECN
	if((value != static_cast <uint64_t> (frame_t::ACK)) && (value != static_cast <uint64_t> (frame_t::ACK_ECN)))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем флаг наличия счётчиков ECN
	output.hasEcn = (value == static_cast <uint64_t> (frame_t::ACK_ECN));
	// Наибольший подтверждаемый номер пакета
	uint64_t largest = 0;
	// Читаем наибольший подтверждаемый номер пакета
	if(!rdVarint(data, size, offset, largest))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем задержку подтверждения
	if(!rdVarint(data, size, offset, output.delay))
		// Разбор невозможен
		return status_t::ERROR;
	// Количество дополнительных диапазонов
	uint64_t count = 0;
	// Читаем количество дополнительных диапазонов
	if(!rdVarint(data, size, offset, count))
		// Разбор невозможен
		return status_t::ERROR;
	// Если количество диапазонов превышает разумный предел обработки
	if(count >= MAX_ACK_RANGES)
		// Разбор невозможен
		return status_t::ERROR;
	// Резервируем память под диапазоны подтверждаемых пакетов
	output.ranges.reserve(static_cast <size_t> (count) + 1);
	// Длина первого диапазона подтверждаемых пакетов
	uint64_t first = 0;
	// Читаем длину первого диапазона
	if(!rdVarint(data, size, offset, first))
		// Разбор невозможен
		return status_t::ERROR;
	// Если первый диапазон уходит ниже нулевого номера пакета
	if(first > largest)
		// Разбор невозможен
		return status_t::ERROR;
	// Наименьший номер пакета текущего диапазона
	uint64_t smallest = (largest - first);
	// Создаём первый диапазон подтверждаемых пакетов
	range_t range;
	// Устанавливаем границы первого диапазона
	range.low  = smallest;
	range.high = largest;
	// Дописываем первый диапазон в список
	output.ranges.push_back(range);
	/**
	 * Перебираем дополнительные диапазоны
	 */
	for(uint64_t i = 0; i < count; i++){
		// Разрыв между диапазонами
		uint64_t gap = 0;
		// Читаем разрыв между диапазонами
		if(!rdVarint(data, size, offset, gap))
			// Разбор невозможен
			return status_t::ERROR;
		// Длина очередного диапазона
		uint64_t length = 0;
		// Читаем длину очередного диапазона
		if(!rdVarint(data, size, offset, length))
			// Разбор невозможен
			return status_t::ERROR;
		// Если разрыв уходит ниже нулевого номера пакета (наибольший номер диапазона = smallest - gap - 2)
		if(smallest < (gap + 2))
			// Разбор невозможен
			return status_t::ERROR;
		// Наибольший номер очередного диапазона
		const uint64_t high = (smallest - gap - 2);
		// Если длина диапазона уходит ниже нулевого номера пакета
		if(length > high)
			// Разбор невозможен
			return status_t::ERROR;
		// Устанавливаем границы очередного диапазона
		range.low  = (high - length);
		range.high = high;
		// Дописываем очередной диапазон в список
		output.ranges.push_back(range);
		// Запоминаем наименьший номер текущего диапазона
		smallest = range.low;
	}
	// Если фрейм содержит счётчики ECN
	if(output.hasEcn){
		// Читаем счётчик пакетов с маркировкой ECT(0)
		if(!rdVarint(data, size, offset, output.ect0))
			// Разбор невозможен
			return status_t::ERROR;
		// Читаем счётчик пакетов с маркировкой ECT(1)
		if(!rdVarint(data, size, offset, output.ect1))
			// Разбор невозможен
			return status_t::ERROR;
		// Читаем счётчик пакетов с маркировкой CE
		if(!rdVarint(data, size, offset, output.ce))
			// Разбор невозможен
			return status_t::ERROR;
	}
	// Устанавливаем количество потреблённых октетов
	consumed = offset;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма RESET_STREAM (RFC 9000 §19.4)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::resetStream(const uint8_t * data, const size_t size, reset_stream_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с фрейма RESET_STREAM
	if(!rdType(data, size, frame_t::RESET_STREAM, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем идентификатор потока
	if(!rdVarint(data, size, offset, output.streamId))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем код ошибки приложения
	if(!rdVarint(data, size, offset, output.code))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем финальный размер потока
	if(!rdVarint(data, size, offset, output.finalSize))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем количество потреблённых октетов
	consumed = offset;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма STOP_SENDING (RFC 9000 §19.5)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::stopSending(const uint8_t * data, const size_t size, stop_sending_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с фрейма STOP_SENDING
	if(!rdType(data, size, frame_t::STOP_SENDING, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем идентификатор потока
	if(!rdVarint(data, size, offset, output.streamId))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем код ошибки приложения
	if(!rdVarint(data, size, offset, output.code))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем количество потреблённых октетов
	consumed = offset;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма CRYPTO (RFC 9000 §19.6)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::crypto(const uint8_t * data, const size_t size, crypto_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с фрейма CRYPTO
	if(!rdType(data, size, frame_t::CRYPTO, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем смещение данных в потоке хендшейка
	if(!rdVarint(data, size, offset, output.offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Длина данных хендшейка
	uint64_t length = 0;
	// Читаем длину данных хендшейка
	if(!rdVarint(data, size, offset, length))
		// Разбор невозможен
		return status_t::ERROR;
	// Если сумма смещения и длины превышает лимит кодирования (RFC 9000 §19.6)
	if((output.offset + length) > proto::VARINT_MAX)
		// Разбор невозможен
		return status_t::ERROR;
	// Если данные целиком не помещаются в буфер
	if(length > (size - offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Извлекаем данные криптографического хендшейка
	output.data = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (length));
	// Устанавливаем количество потреблённых октетов
	consumed = (offset + static_cast <size_t> (length));
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма NEW_TOKEN (RFC 9000 §19.7)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param token    токен для будущих соединений (zero-copy в буфер нагрузки)
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::newToken(const uint8_t * data, const size_t size, string_view & token, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с фрейма NEW_TOKEN
	if(!rdType(data, size, frame_t::NEW_TOKEN, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Длина токена
	uint64_t length = 0;
	// Читаем длину токена
	if(!rdVarint(data, size, offset, length))
		// Разбор невозможен
		return status_t::ERROR;
	// Если токен пуст (RFC 9000 §19.7) или целиком не помещается в буфер
	if((length == 0) || (length > (size - offset)))
		// Разбор невозможен
		return status_t::ERROR;
	// Извлекаем токен для будущих соединений
	token = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (length));
	// Устанавливаем количество потреблённых октетов
	consumed = (offset + static_cast <size_t> (length));
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма DATAGRAM обоих вариантов 0x30-0x31 (RFC 9221 §4)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   данные датаграммы приложения (zero-copy в буфер нагрузки)
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::datagram(const uint8_t * data, const size_t size, string_view & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Прочитанное значение типа фрейма
	uint64_t value = 0;
	// Читаем тип фрейма
	if(!rdVarint(data, size, offset, value))
		// Разбор невозможен
		return status_t::ERROR;
	// Если тип фрейма не принадлежит диапазону DATAGRAM (0x30-0x31)
	if((value < static_cast <uint64_t> (frame_t::DATAGRAM)) || (value > (static_cast <uint64_t> (frame_t::DATAGRAM) | 0x01)))
		// Разбор невозможен
		return status_t::ERROR;
	// Длина данных датаграммы приложения
	uint64_t length = 0;
	// Если тип фрейма содержит поле Length
	if((value & 0x01) != 0){
		// Читаем длину данных датаграммы приложения
		if(!rdVarint(data, size, offset, length))
			// Разбор невозможен
			return status_t::ERROR;
		// Если данные датаграммы целиком не помещаются в буфер
		if(length > (size - offset))
			// Разбор невозможен
			return status_t::ERROR;
	/**
	 * Если поля Length нет: данные датаграммы занимают весь остаток пакета,
	 * поэтому такой фрейм бывает в пакете только последним (RFC 9221 §4)
	 */
	} else length = (size - offset);
	// Извлекаем данные датаграммы приложения
	output = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (length));
	// Устанавливаем количество потреблённых октетов
	consumed = (offset + static_cast <size_t> (length));
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма STREAM всех вариантов 0x08-0x0F (RFC 9000 §19.8)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::stream(const uint8_t * data, const size_t size, stream_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Прочитанное значение типа фрейма
	uint64_t value = 0;
	// Читаем тип фрейма
	if(!rdVarint(data, size, offset, value))
		// Разбор невозможен
		return status_t::ERROR;
	// Если тип фрейма не принадлежит диапазону STREAM (0x08-0x0F)
	if((value < static_cast <uint64_t> (frame_t::STREAM)) || (value > (static_cast <uint64_t> (frame_t::STREAM) | 0x07)))
		// Разбор невозможен
		return status_t::ERROR;
	// Извлекаем флаг завершения потока (FIN)
	output.fin = ((value & 0x01) != 0);
	// Читаем идентификатор потока
	if(!rdVarint(data, size, offset, output.streamId))
		// Разбор невозможен
		return status_t::ERROR;
	// Если тип фрейма содержит поле Offset
	if((value & 0x04) != 0){
		// Читаем смещение данных в потоке
		if(!rdVarint(data, size, offset, output.offset))
			// Разбор невозможен
			return status_t::ERROR;
	// Смещение данных в потоке нулевое
	} else output.offset = 0;
	// Длина данных потока
	uint64_t length = 0;
	// Если тип фрейма содержит поле Length
	if((value & 0x02) != 0){
		// Читаем длину данных потока
		if(!rdVarint(data, size, offset, length))
			// Разбор невозможен
			return status_t::ERROR;
		// Если данные целиком не помещаются в буфер
		if(length > (size - offset))
			// Разбор невозможен
			return status_t::ERROR;
	// Данные потока занимают буфер до конца
	} else length = (size - offset);
	// Если сумма смещения и длины превышает лимит кодирования (RFC 9000 §19.8)
	if((output.offset + length) > proto::VARINT_MAX){
		// Устанавливаем код ошибки транспорта
		error = error_t::FINAL_SIZE_ERROR;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Извлекаем данные потока приложения
	output.data = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (length));
	// Устанавливаем количество потреблённых октетов
	consumed = (offset + static_cast <size_t> (length));
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фреймов с одним целочисленным полем
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param type     ожидаемый тип фрейма
 * @param value    прочитанное значение поля
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::single(const uint8_t * data, const size_t size, const frame_t type, uint64_t & value, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с ожидаемого типа фрейма
	if(!rdType(data, size, type, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем значение целочисленного поля
	if(!rdVarint(data, size, offset, value))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем количество потреблённых октетов
	consumed = offset;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фреймов с полями идентификатора потока и лимита
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param type     ожидаемый тип фрейма
 * @param streamId прочитанный идентификатор потока
 * @param value    прочитанное значение лимита
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::pair(const uint8_t * data, const size_t size, const frame_t type, uint64_t & streamId, uint64_t & value, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с ожидаемого типа фрейма
	if(!rdType(data, size, type, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем идентификатор потока
	if(!rdVarint(data, size, offset, streamId))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем значение лимита
	if(!rdVarint(data, size, offset, value))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем количество потреблённых октетов
	consumed = offset;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма NEW_CONNECTION_ID (RFC 9000 §19.15)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::newConnectionId(const uint8_t * data, const size_t size, new_connection_id_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с фрейма NEW_CONNECTION_ID
	if(!rdType(data, size, frame_t::NEW_CONNECTION_ID, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем порядковый номер идентификатора соединения
	if(!rdVarint(data, size, offset, output.seq))
		// Разбор невозможен
		return status_t::ERROR;
	// Читаем порядковый номер вывода идентификаторов из обращения
	if(!rdVarint(data, size, offset, output.retirePriorTo))
		// Разбор невозможен
		return status_t::ERROR;
	// Если порядковый номер вывода превышает порядковый номер идентификатора (RFC 9000 §19.15)
	if(output.retirePriorTo > output.seq){
		// Устанавливаем код ошибки транспорта
		error = error_t::PROTOCOL_VIOLATION;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Если в буфере нет октета длины идентификатора
	if(offset >= size)
		// Разбор невозможен
		return status_t::ERROR;
	// Извлекаем длину нового идентификатора соединения
	const size_t length = static_cast <size_t> (data[offset]);
	// Сдвигаем смещение разбора за октетом длины
	offset++;
	// Если длина идентификатора вне диапазона 1-20 (RFC 9000 §19.15)
	if((length < 1) || (length > proto::MAX_CID_SIZE)){
		// Устанавливаем код ошибки транспорта
		error = error_t::FRAME_ENCODING_ERROR;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Если идентификатор с токеном сброса целиком не помещаются в буфер
	if((length + proto::RESET_TOKEN_SIZE) > (size - offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем длину нового идентификатора соединения
	output.cid.size = length;
	// Копируем данные нового идентификатора соединения
	::memcpy(output.cid.data, data + offset, length);
	// Сдвигаем смещение разбора за идентификатором
	offset += length;
	// Копируем токен сброса без сохранения состояния
	::memcpy(output.resetToken, data + offset, proto::RESET_TOKEN_SIZE);
	// Сдвигаем смещение разбора за токеном сброса
	offset += proto::RESET_TOKEN_SIZE;
	// Устанавливаем количество потреблённых октетов
	consumed = offset;
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фреймов PATH_CHALLENGE/PATH_RESPONSE (RFC 9000 §19.17/§19.18)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param type     ожидаемый тип фрейма
 * @param output   извлечённые данные проверки пути (8 октетов)
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::path(const uint8_t * data, const size_t size, const frame_t type, uint8_t output[proto::PATH_DATA_SIZE], size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Если ожидаемый тип не является фреймом проверки пути
	if((type != frame_t::PATH_CHALLENGE) && (type != frame_t::PATH_RESPONSE))
		// Разбор невозможен
		return status_t::ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Если буфер не начинается с ожидаемого типа фрейма
	if(!rdType(data, size, type, offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Если данные проверки пути целиком не помещаются в буфер
	if(proto::PATH_DATA_SIZE > (size - offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Копируем данные проверки пути
	::memcpy(output, data + offset, proto::PATH_DATA_SIZE);
	// Устанавливаем количество потреблённых октетов
	consumed = (offset + proto::PATH_DATA_SIZE);
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма CONNECTION_CLOSE обоих вариантов (RFC 9000 §19.19)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param output   разобранный фрейм
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::connectionClose(const uint8_t * data, const size_t size, connection_close_t & output, size_t & consumed, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::FRAME_ENCODING_ERROR;
	// Смещение разбора
	size_t offset = 0;
	// Прочитанное значение типа фрейма
	uint64_t value = 0;
	// Читаем тип фрейма
	if(!rdVarint(data, size, offset, value))
		// Разбор невозможен
		return status_t::ERROR;
	// Если тип фрейма не соответствует вариантам CONNECTION_CLOSE
	if((value != static_cast <uint64_t> (frame_t::CONNECTION_CLOSE)) && (value != static_cast <uint64_t> (frame_t::CONNECTION_CLOSE_APP)))
		// Разбор невозможен
		return status_t::ERROR;
	// Устанавливаем флаг ошибки приложения
	output.app = (value == static_cast <uint64_t> (frame_t::CONNECTION_CLOSE_APP));
	// Читаем код ошибки
	if(!rdVarint(data, size, offset, output.code))
		// Разбор невозможен
		return status_t::ERROR;
	// Если фрейм содержит ошибку транспорта
	if(!output.app){
		// Читаем тип фрейма, вызвавшего ошибку
		if(!rdVarint(data, size, offset, output.frameType))
			// Разбор невозможен
			return status_t::ERROR;
	// Тип фрейма для ошибки приложения не кодируется
	} else output.frameType = 0;
	// Длина причины завершения
	uint64_t length = 0;
	// Читаем длину причины завершения
	if(!rdVarint(data, size, offset, length))
		// Разбор невозможен
		return status_t::ERROR;
	// Если причина целиком не помещается в буфер
	if(length > (size - offset))
		// Разбор невозможен
		return status_t::ERROR;
	// Извлекаем человекочитаемую причину завершения
	output.reason = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (length));
	// Устанавливаем количество потреблённых октетов
	consumed = (offset + static_cast <size_t> (length));
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора фрейма HANDSHAKE_DONE (RFC 9000 §19.20)
 *
 * @param data     буфер расшифрованной нагрузки (начало фрейма)
 * @param size     доступно байт
 * @param consumed количество потреблённых октетов
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::frame::parser::handshakeDone(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept {
	// Если буфер не начинается с фрейма HANDSHAKE_DONE
	if(!rdType(data, size, frame_t::HANDSHAKE_DONE, consumed)){
		// Устанавливаем код ошибки транспорта
		error = error_t::FRAME_ENCODING_ERROR;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция сборки последовательности фреймов PADDING (заполнение дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param count  количество октетов заполнения
 */
void awh::quic::frame::serialize::padding(string & output, const size_t count) noexcept {
	// Дописываем заполнение нулевыми октетами
	output.append(count, '\0');
}
/**
 * @brief Функция сборки фрейма PING (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 */
void awh::quic::frame::serialize::ping(string & output) noexcept {
	// Дописываем тип фрейма PING
	output.push_back(static_cast <char> (frame_t::PING));
}
/**
 * @brief Функция сборки фрейма ACK/ACK_ECN (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param frame  фрейм подтверждения приёма пакетов
 * @return       результат сборки (false - некорректные диапазоны)
 */
bool awh::quic::frame::serialize::ack(string & output, const ack_t & frame) noexcept {
	// Если список диапазонов пуст
	if(frame.ranges.empty())
		// Сборка невозможна
		return false;
	/**
	 * Проверяем корректность списка диапазонов
	 */
	for(size_t i = 0; i < frame.ranges.size(); i++){
		// Если границы диапазона перепутаны
		if(frame.ranges[i].low > frame.ranges[i].high)
			// Сборка невозможна
			return false;
		// Если очередной диапазон не отделён разрывом от предыдущего
		if((i > 0) && ((frame.ranges[i].high + 2) > frame.ranges[i - 1].low))
			// Сборка невозможна
			return false;
	}
	// Дописываем тип фрейма ACK или ACK_ECN
	varint::write(output, static_cast <uint64_t> (frame.hasEcn ? frame_t::ACK_ECN : frame_t::ACK));
	// Дописываем наибольший подтверждаемый номер пакета
	varint::write(output, frame.ranges.front().high);
	// Дописываем задержку подтверждения
	varint::write(output, frame.delay);
	// Дописываем количество дополнительных диапазонов
	varint::write(output, frame.ranges.size() - 1);
	// Дописываем длину первого диапазона
	varint::write(output, frame.ranges.front().high - frame.ranges.front().low);
	/**
	 * Перебираем дополнительные диапазоны
	 */
	for(size_t i = 1; i < frame.ranges.size(); i++){
		// Дописываем разрыв между диапазонами
		varint::write(output, frame.ranges[i - 1].low - frame.ranges[i].high - 2);
		// Дописываем длину очередного диапазона
		varint::write(output, frame.ranges[i].high - frame.ranges[i].low);
	}
	// Если фрейм содержит счётчики ECN
	if(frame.hasEcn){
		// Дописываем счётчик пакетов с маркировкой ECT(0)
		varint::write(output, frame.ect0);
		// Дописываем счётчик пакетов с маркировкой ECT(1)
		varint::write(output, frame.ect1);
		// Дописываем счётчик пакетов с маркировкой CE
		varint::write(output, frame.ce);
	}
	// Сборка выполнена успешно
	return true;
}
/**
 * @brief Функция сборки фрейма RESET_STREAM (фрейм дописывается в output)
 *
 * @param output    выходной буфер нагрузки пакета
 * @param streamId  идентификатор потока
 * @param code      код ошибки приложения
 * @param finalSize финальный размер потока в октетах
 */
void awh::quic::frame::serialize::resetStream(string & output, const uint64_t streamId, const uint64_t code, const uint64_t finalSize) noexcept {
	// Дописываем тип фрейма RESET_STREAM
	varint::write(output, static_cast <uint64_t> (frame_t::RESET_STREAM));
	// Дописываем идентификатор потока
	varint::write(output, streamId);
	// Дописываем код ошибки приложения
	varint::write(output, code);
	// Дописываем финальный размер потока
	varint::write(output, finalSize);
}
/**
 * @brief Функция сборки фрейма STOP_SENDING (фрейм дописывается в output)
 *
 * @param output   выходной буфер нагрузки пакета
 * @param streamId идентификатор потока
 * @param code     код ошибки приложения
 */
void awh::quic::frame::serialize::stopSending(string & output, const uint64_t streamId, const uint64_t code) noexcept {
	// Дописываем тип фрейма STOP_SENDING
	varint::write(output, static_cast <uint64_t> (frame_t::STOP_SENDING));
	// Дописываем идентификатор потока
	varint::write(output, streamId);
	// Дописываем код ошибки приложения
	varint::write(output, code);
}
/**
 * @brief Функция сборки фрейма CRYPTO (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param offset смещение данных в потоке криптографического хендшейка
 * @param data   данные криптографического хендшейка
 */
void awh::quic::frame::serialize::crypto(string & output, const uint64_t offset, string_view data) noexcept {
	// Дописываем тип фрейма CRYPTO
	varint::write(output, static_cast <uint64_t> (frame_t::CRYPTO));
	// Дописываем смещение данных в потоке хендшейка
	varint::write(output, offset);
	// Дописываем длину данных хендшейка
	varint::write(output, data.size());
	// Если данные не пустые
	if(!data.empty())
		// Дописываем данные криптографического хендшейка
		output.append(data);
}
/**
 * @brief Функция сборки фрейма NEW_TOKEN (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param token  токен для будущих соединений клиента (не пустой)
 * @return       результат сборки (false - пустой токен запрещён)
 */
bool awh::quic::frame::serialize::newToken(string & output, string_view token) noexcept {
	// Если токен пуст (RFC 9000 §19.7)
	if(token.empty())
		// Сборка невозможна
		return false;
	// Дописываем тип фрейма NEW_TOKEN
	varint::write(output, static_cast <uint64_t> (frame_t::NEW_TOKEN));
	// Дописываем длину токена
	varint::write(output, token.size());
	// Дописываем токен для будущих соединений
	output.append(token);
	// Сборка выполнена успешно
	return true;
}
/**
 * @brief Функция сборки фрейма DATAGRAM с полем Length (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param data   данные датаграммы приложения
 */
void awh::quic::frame::serialize::datagram(string & output, string_view data) noexcept {
	// Дописываем тип фрейма с признаком наличия поля Length
	varint::write(output, static_cast <uint64_t> (frame_t::DATAGRAM) | 0x01);
	// Дописываем длину данных датаграммы приложения
	varint::write(output, static_cast <uint64_t> (data.size()));
	// Дописываем данные датаграммы приложения
	output.append(data);
}
/**
 * @brief Функция сборки фрейма STREAM (фрейм дописывается в output)
 *
 * @param output   выходной буфер нагрузки пакета
 * @param streamId идентификатор потока
 * @param offset   смещение данных в потоке
 * @param data     данные потока приложения
 * @param fin      флаг завершения потока (FIN)
 */
void awh::quic::frame::serialize::stream(string & output, const uint64_t streamId, const uint64_t offset, string_view data, const bool fin) noexcept {
	// Тип фрейма STREAM с битом наличия поля Length
	uint64_t type = (static_cast <uint64_t> (frame_t::STREAM) | 0x02);
	// Если смещение данных ненулевое
	if(offset > 0)
		// Устанавливаем бит наличия поля Offset
		type |= 0x04;
	// Если установлен флаг завершения потока
	if(fin)
		// Устанавливаем бит FIN
		type |= 0x01;
	// Дописываем тип фрейма STREAM
	varint::write(output, type);
	// Дописываем идентификатор потока
	varint::write(output, streamId);
	// Если смещение данных ненулевое
	if(offset > 0)
		// Дописываем смещение данных в потоке
		varint::write(output, offset);
	// Дописываем длину данных потока
	varint::write(output, data.size());
	// Если данные не пустые
	if(!data.empty())
		// Дописываем данные потока приложения
		output.append(data);
}
/**
 * @brief Функция сборки фреймов с одним целочисленным полем (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param type   тип фрейма
 * @param value  значение целочисленного поля
 */
void awh::quic::frame::serialize::single(string & output, const frame_t type, const uint64_t value) noexcept {
	// Дописываем тип фрейма
	varint::write(output, static_cast <uint64_t> (type));
	// Дописываем значение целочисленного поля
	varint::write(output, value);
}
/**
 * @brief Функция сборки фреймов с полями идентификатора потока и лимита (фрейм дописывается в output)
 *
 * @param output   выходной буфер нагрузки пакета
 * @param type     тип фрейма
 * @param streamId идентификатор потока
 * @param value    значение лимита
 */
void awh::quic::frame::serialize::pair(string & output, const frame_t type, const uint64_t streamId, const uint64_t value) noexcept {
	// Дописываем тип фрейма
	varint::write(output, static_cast <uint64_t> (type));
	// Дописываем идентификатор потока
	varint::write(output, streamId);
	// Дописываем значение лимита
	varint::write(output, value);
}
/**
 * @brief Функция сборки фрейма NEW_CONNECTION_ID (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param frame  фрейм анонса нового идентификатора соединения
 * @return       результат сборки (false - некорректная длина идентификатора)
 */
bool awh::quic::frame::serialize::newConnectionId(string & output, const new_connection_id_t & frame) noexcept {
	// Если длина идентификатора вне диапазона 1-20 или порядковые номера некорректны
	if((frame.cid.size < 1) || (frame.cid.size > proto::MAX_CID_SIZE) || (frame.retirePriorTo > frame.seq))
		// Сборка невозможна
		return false;
	// Дописываем тип фрейма NEW_CONNECTION_ID
	varint::write(output, static_cast <uint64_t> (frame_t::NEW_CONNECTION_ID));
	// Дописываем порядковый номер идентификатора соединения
	varint::write(output, frame.seq);
	// Дописываем порядковый номер вывода идентификаторов из обращения
	varint::write(output, frame.retirePriorTo);
	// Дописываем октет длины нового идентификатора соединения
	output.push_back(static_cast <char> (frame.cid.size));
	// Дописываем данные нового идентификатора соединения
	output.append(reinterpret_cast <const char *> (frame.cid.data), frame.cid.size);
	// Дописываем токен сброса без сохранения состояния
	output.append(reinterpret_cast <const char *> (frame.resetToken), proto::RESET_TOKEN_SIZE);
	// Сборка выполнена успешно
	return true;
}
/**
 * @brief Функция сборки фреймов PATH_CHALLENGE/PATH_RESPONSE (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 * @param type   тип фрейма (PATH_CHALLENGE или PATH_RESPONSE)
 * @param data   данные проверки пути (8 октетов)
 */
void awh::quic::frame::serialize::path(string & output, const frame_t type, const uint8_t data[proto::PATH_DATA_SIZE]) noexcept {
	// Дописываем тип фрейма проверки пути
	varint::write(output, static_cast <uint64_t> (type));
	// Дописываем данные проверки пути
	output.append(reinterpret_cast <const char *> (data), proto::PATH_DATA_SIZE);
}
/**
 * @brief Функция сборки фрейма CONNECTION_CLOSE (фрейм дописывается в output)
 *
 * @param output    выходной буфер нагрузки пакета
 * @param code      код ошибки (транспорта или приложения)
 * @param frameType тип фрейма, вызвавшего ошибку (игнорируется для ошибки приложения)
 * @param reason    человекочитаемая причина завершения
 * @param app       флаг ошибки приложения (тип фрейма 0x1D)
 */
void awh::quic::frame::serialize::connectionClose(string & output, const uint64_t code, const uint64_t frameType, string_view reason, const bool app) noexcept {
	// Дописываем тип фрейма CONNECTION_CLOSE соответствующего варианта
	varint::write(output, static_cast <uint64_t> (app ? frame_t::CONNECTION_CLOSE_APP : frame_t::CONNECTION_CLOSE));
	// Дописываем код ошибки
	varint::write(output, code);
	// Если фрейм содержит ошибку транспорта
	if(!app)
		// Дописываем тип фрейма, вызвавшего ошибку
		varint::write(output, frameType);
	// Дописываем длину причины завершения
	varint::write(output, reason.size());
	// Если причина не пустая
	if(!reason.empty())
		// Дописываем человекочитаемую причину завершения
		output.append(reason);
}
/**
 * @brief Функция сборки фрейма HANDSHAKE_DONE (фрейм дописывается в output)
 *
 * @param output выходной буфер нагрузки пакета
 */
void awh::quic::frame::serialize::handshakeDone(string & output) noexcept {
	// Дописываем тип фрейма HANDSHAKE_DONE
	varint::write(output, static_cast <uint64_t> (frame_t::HANDSHAKE_DONE));
}
