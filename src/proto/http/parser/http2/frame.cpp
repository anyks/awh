/**
 * @file: frame.cpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация слоя фреймов HTTP/2 (RFC 9113) — разбор и сборка полезной нагрузки фреймов DATA, HEADERS,
 *        PRIORITY, SETTINGS, GOAWAY и PUSH_PROMISE,
 *        снятие и добавление выравнивания без хранения состояния соединения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http2/frame.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные функции чтения/записи в сетевом (big-endian) порядке
 *
 */
namespace {
	/**
	 * Используем пространство имён внутренних слоёв протокола HTTP/2
	 */
	using namespace awh::http::h2;

	/**
	 * @brief Функция чтения 16-битного числа в сетевом порядке байт
	 *
	 * @param buffer буфер с данными числа
	 * @return       прочитанное число
	 *
	 */
	inline uint16_t rd16(const uint8_t * buffer) noexcept {
		// Собираем число из двух байт в сетевом порядке
		return static_cast <uint16_t> ((static_cast <uint16_t> (buffer[0]) << 8) | buffer[1]);
	}
	/**
	 * @brief Функция чтения 24-битного числа в сетевом порядке байт
	 *
	 * @param buffer буфер с данными числа
	 * @return       прочитанное число
	 *
	 */
	inline uint32_t rd24(const uint8_t * buffer) noexcept {
		// Собираем число из трёх байт в сетевом порядке
		return (static_cast <uint32_t> (buffer[0]) << 16) | (static_cast <uint32_t> (buffer[1]) << 8) | buffer[2];
	}
	/**
	 * @brief Функция чтения 32-битного числа в сетевом порядке байт
	 *
	 * @param buffer буфер с данными числа
	 * @return       прочитанное число
	 *
	 */
	inline uint32_t rd32(const uint8_t * buffer) noexcept {
		// Собираем число из четырёх байт в сетевом порядке
		return (static_cast <uint32_t> (buffer[0]) << 24) | (static_cast <uint32_t> (buffer[1]) << 16) |
		       (static_cast <uint32_t> (buffer[2]) << 8)  |  static_cast <uint32_t> (buffer[3]);
	}
	/**
	 * @brief Функция записи 16-битного числа в сетевом порядке байт
	 *
	 * @param output выходной буфер
	 * @param num записываемое число
	 *
	 */
	inline void wr16(string & output, const uint16_t num) noexcept {
		// Дописываем старший байт числа
		output.push_back(static_cast <char> ((num >> 8) & 0xFF));
		// Дописываем младший байт числа
		output.push_back(static_cast <char> (num & 0xFF));
	}
	/**
	 * @brief Функция записи 32-битного числа в сетевом порядке байт
	 *
	 * @param output выходной буфер
	 * @param num записываемое число
	 *
	 */
	inline void wr32(string & output, const uint32_t num) noexcept {
		// Дописываем старший байт числа
		output.push_back(static_cast <char> ((num >> 24) & 0xFF));
		// Дописываем второй байт числа
		output.push_back(static_cast <char> ((num >> 16) & 0xFF));
		// Дописываем третий байт числа
		output.push_back(static_cast <char> ((num >> 8) & 0xFF));
		// Дописываем младший байт числа
		output.push_back(static_cast <char> (num & 0xFF));
	}
	/**
	 * @brief Функция снятия padding с нагрузки фреймов DATA/HEADERS/PUSH_PROMISE
	 *
	 * @details При наличии флага PADDED первый байт нагрузки - Pad Length,
	 *          в хвосте нагрузки идёт сам padding
	 *
	 * @param padded  признак наличия флага PADDED
	 * @param payload полезная нагрузка (сдвигается)
	 * @param length  длина полезной нагрузки (уменьшается)
	 * @return        результат снятия (false - Pad Length некорректен, PROTOCOL_ERROR)
	 *
	 */
	inline bool stripPadding(const bool padded, const uint8_t *& payload, size_t & length) noexcept {
		// Если флаг PADDED не установлен - снимать нечего
		if(!padded)
			// Продолжаем разбор
			return true;
		// Если нагрузка не содержит даже байта Pad Length
		if(length < 1)
			// Нагрузка некорректна
			return false;
		// Извлекаем длину padding
		const uint8_t padLen = payload[0];
		// Сдвигаем указатель за байт Pad Length
		payload += 1;
		// Уменьшаем длину нагрузки на байт Pad Length
		length -= 1;
		// Если padding не помещается в оставшуюся нагрузку
		if(padLen > length)
			// Нагрузка некорректна
			return false;
		// Отбрасываем padding из хвоста нагрузки
		length -= padLen;
		// Продолжаем разбор
		return true;
	}
	/**
	 * @brief Функция записи 9-байтового заголовка фрейма (RFC 9113 §4.1)
	 *
	 * @param output      выходной буфер
	 * @param length   длина полезной нагрузки
	 * @param type     тип фрейма
	 * @param flags    флаги фрейма
	 * @param streamId идентификатор потока
	 *
	 */
	inline void wrHeader(string & output, const uint32_t length, const frame_t type, const uint8_t flags, const uint32_t streamId) noexcept {
		/**
		 * Заголовок собирается на стеке и дописывается одной операцией: поочерёдная
		 * запись девяти октетов стоит девяти проверок ёмкости строки, а размер
		 * заголовка фиксирован протоколом и известен заранее (RFC 9113 §4.1)
		 */
		char buffer[proto::FRAME_HEADER_SIZE];
		// Записываем старший байт 24-битной длины полезной нагрузки
		buffer[0] = static_cast <char> ((length >> 16) & 0xFF);
		// Записываем средний байт длины полезной нагрузки
		buffer[1] = static_cast <char> ((length >> 8) & 0xFF);
		// Записываем младший байт длины полезной нагрузки
		buffer[2] = static_cast <char> (length & 0xFF);
		// Записываем тип фрейма
		buffer[3] = static_cast <char> (type);
		// Записываем флаги фрейма
		buffer[4] = static_cast <char> (flags);
		// Сбрасываем reserved-бит идентификатора потока
		const uint32_t id = (streamId & proto::STREAM_ID_MASK);
		// Записываем старший байт идентификатора потока
		buffer[5] = static_cast <char> ((id >> 24) & 0xFF);
		// Записываем второй байт идентификатора потока
		buffer[6] = static_cast <char> ((id >> 16) & 0xFF);
		// Записываем третий байт идентификатора потока
		buffer[7] = static_cast <char> ((id >> 8) & 0xFF);
		// Записываем младший байт идентификатора потока
		buffer[8] = static_cast <char> (id & 0xFF);
		// Дописываем собранный заголовок фрейма
		output.append(buffer, proto::FRAME_HEADER_SIZE);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Setting::Setting() noexcept :
 id(setting_t::HEADER_TABLE_SIZE), value(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Data::Data() noexcept : endStream(false), data{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Priority::Priority() noexcept :
 exclusive(false), weight(0), streamDep(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Goaway::Goaway() noexcept :
 code(error_t::NO_ERROR), lastStreamId(0), debugData{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Header::Header() noexcept :
 type(frame_t::DATA), flags(flag::NONE), length(0), streamId(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Headers::Headers() noexcept :
 exclusive(false),
 endStream(false),
 endHeaders(false),
 hasPriority(false),
 weight(0), streamDep(0),
 block{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::frame::Push_Promise::Push_Promise() noexcept :
 endHeaders(false), promisedStreamId(0), block{""} {}

/**
 * @brief Функция разбора 9-байтового заголовка фрейма
 *
 * @param data   входной буфер
 * @param size   доступно байт
 * @param output разобранный заголовок фрейма
 * @return       результат разбора (true - в буфере было достаточно байт и заголовок разобран)
 *
 */
bool awh::http::h2::frame::parser::header(const uint8_t * data, const size_t size, header_t & output) noexcept {
	// Если в буфере недостаточно байт для заголовка фрейма
	if(size < proto::FRAME_HEADER_SIZE)
		// Заголовок разобрать невозможно
		return false;
	// Извлекаем 24-битную длину полезной нагрузки
	output.length = ::rd24(data);
	// Извлекаем тип фрейма
	output.type = static_cast <frame_t> (data[3]);
	// Извлекаем флаги фрейма
	output.flags = data[4];
	// Извлекаем 31-битный идентификатор потока (сбрасывая reserved-бит)
	output.streamId = (::rd32(data + 5) & proto::STREAM_ID_MASK);
	// Заголовок фрейма разобран
	return true;
}
/**
 * @brief Функция разбора полезной нагрузки DATA
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола (PROTOCOL_ERROR на некорректном padding)
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::data(const header_t & header, const uint8_t * payload, data_t & output, error_t & error) noexcept {
	// Если фрейм не принадлежит потоку (DATA обязан иметь stream id != 0)
	if(header.streamId == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Оставшаяся длина полезной нагрузки
	size_t length = header.length;
	// Буфер текущей позиции полезной нагрузки
	const uint8_t * buffer = payload;
	// Если снятие padding не удалось
	if(!::stripPadding(header.flags & flag::PADDED, buffer, length)){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем данные тела (zero-copy во входной буфер)
	output.data = string_view(reinterpret_cast <const char *> (buffer), length);
	// Извлекаем флаг завершения потока
	output.endStream = ((header.flags & flag::END_STREAM) != 0);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки PING (требует ровно 8 байт opaque-данных)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param opaque  извлечённые opaque-данные
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::ping(const header_t & header, const uint8_t * payload, uint8_t opaque[8], error_t & error) noexcept {
	// Если фрейм принадлежит потоку (PING относится к соединению, stream id == 0)
	if(header.streamId != 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если размер нагрузки не равен ровно 8 байтам
	if(header.length != 8){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	/**
	 * Выполняем копирование всех opaque-данных
	 */
	for(uint8_t i = 0; i < 8; ++i)
		// Копируем очередной байт opaque-данных
		opaque[i] = payload[i];
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки RST_STREAM (требует ровно 4 байта)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param code    код ошибки, с которым сброшен поток
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::rstStream(const header_t & header, const uint8_t * payload, error_t & code, error_t & error) noexcept {
	// Если фрейм не принадлежит потоку (RST_STREAM обязан иметь stream id != 0)
	if(header.streamId == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если размер нагрузки не равен ровно 4 байтам
	if(header.length != 4){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем код ошибки, с которым сброшен поток
	code = static_cast <error_t> (::rd32(payload));
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки GOAWAY (минимум 8 байт)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::goaway(const header_t & header, const uint8_t * payload, goaway_t & output, error_t & error) noexcept {
	// Если фрейм принадлежит потоку (GOAWAY относится к соединению, stream id == 0)
	if(header.streamId != 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если нагрузка не содержит обязательные 8 байт
	if(header.length < 8){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем наибольший идентификатор обработанного потока (сбрасывая reserved-бит)
	output.lastStreamId = (::rd32(payload) & proto::STREAM_ID_MASK);
	// Извлекаем код ошибки завершения соединения
	output.code = static_cast <error_t> (::rd32(payload + 4));
	// Извлекаем отладочные данные (zero-copy во входной буфер)
	output.debugData = string_view(reinterpret_cast <const char *> (payload + 8), header.length - 8);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки HEADERS (с учётом padding и приоритета)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::headers(const header_t & header, const uint8_t * payload, headers_t & output, error_t & error) noexcept {
	// Если фрейм не принадлежит потоку (HEADERS обязан иметь stream id != 0)
	if(header.streamId == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Оставшаяся длина полезной нагрузки
	size_t length = header.length;
	// Буфер текущей позиции полезной нагрузки
	const uint8_t * buffer = payload;
	// Если снятие padding не удалось
	if(!::stripPadding(header.flags & flag::PADDED, buffer, length)){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем флаг завершения потока
	output.endStream = ((header.flags & flag::END_STREAM) != 0);
	// Извлекаем флаг наличия полей приоритета
	output.hasPriority = ((header.flags & flag::PRIORITY) != 0);
	// Извлекаем флаг завершения блока заголовков
	output.endHeaders = ((header.flags & flag::END_HEADERS) != 0);
	// Если поля приоритета присутствуют (RFC 7540, deprecated)
	if(output.hasPriority){
		// Если нагрузка не содержит 5 байт полей приоритета
		if(length < 5){
			// Фиксируем некорректный размер фрейма
			error = error_t::FRAME_SIZE_ERROR;
			// Выводим ошибку разбора
			return status_t::ERROR;
		}
		// Извлекаем зависимость потока (32 бита)
		const uint32_t dep = ::rd32(buffer);
		// Извлекаем флаг эксклюзивной зависимости (старший бит)
		output.exclusive = ((dep & 0x80000000u) != 0);
		// Извлекаем идентификатор потока, от которого зависит текущий
		output.streamDep = (dep & proto::STREAM_ID_MASK);
		// Извлекаем вес потока
		output.weight = buffer[4];
		// Сдвигаем указатель за поля приоритета
		buffer += 5;
		// Уменьшаем длину нагрузки на поля приоритета
		length -= 5;
	}
	// Извлекаем фрагмент блока заголовков (zero-copy во входной буфер)
	output.block = string_view(reinterpret_cast <const char *> (buffer), length);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки PRIORITY (требует ровно 5 байт)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::priority(const header_t & header, const uint8_t * payload, priority_t & output, error_t & error) noexcept {
	// Если фрейм не принадлежит потоку (PRIORITY обязан иметь stream id != 0)
	if(header.streamId == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если размер нагрузки не равен ровно 5 байтам
	if(header.length != 5){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем зависимость потока (32 бита)
	const uint32_t dep = ::rd32(payload);
	// Извлекаем флаг эксклюзивной зависимости (старший бит)
	output.exclusive = ((dep & 0x80000000u) != 0);
	// Извлекаем идентификатор потока, от которого зависит текущий
	output.streamDep = (dep & proto::STREAM_ID_MASK);
	// Извлекаем вес потока
	output.weight = payload[4];
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки WINDOW_UPDATE (требует ровно 4 байта)
 *
 * @param header    заголовок фрейма
 * @param payload   полезная нагрузка фрейма
 * @param increment извлечённый инкремент окна
 * @param error     код ошибки протокола
 * @return          результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::windowUpdate(const header_t & header, const uint8_t * payload, uint32_t & increment, error_t & error) noexcept {
	// Если размер нагрузки не равен ровно 4 байтам
	if(header.length != 4){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем инкремент окна (сбрасывая reserved-бит)
	increment = (::rd32(payload) & proto::STREAM_ID_MASK);
	// Если получен нулевой инкремент (PROTOCOL_ERROR по RFC 9113 §6.9)
	if(increment == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки PUSH_PROMISE (с учётом padding)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::pushPromise(const header_t & header, const uint8_t * payload, push_promise_t & output, error_t & error) noexcept {
	// Если фрейм не принадлежит потоку (PUSH_PROMISE обязан иметь stream id != 0)
	if(header.streamId == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Оставшаяся длина полезной нагрузки
	size_t length = header.length;
	// Буфер текущей позиции полезной нагрузки
	const uint8_t * buffer = payload;
	// Если снятие padding не удалось
	if(!::stripPadding(header.flags & flag::PADDED, buffer, length)){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если нагрузка не содержит 4 байта Promised Stream ID
	if(length < 4){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем флаг завершения блока заголовков
	output.endHeaders = ((header.flags & flag::END_HEADERS) != 0);
	// Извлекаем идентификатор обещанного потока (сбрасывая reserved-бит)
	output.promisedStreamId = (::rd32(buffer) & proto::STREAM_ID_MASK);
	// Извлекаем фрагмент блока заголовков (zero-copy во входной буфер)
	output.block = string_view(reinterpret_cast <const char *> (buffer + 4), length - 4);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки SETTINGS (длина кратна 6)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param output  список разобранных параметров
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::settings(const header_t & header, const uint8_t * payload, vector <setting_entry_t> & output, error_t & error) noexcept {
	// Если фрейм принадлежит потоку (SETTINGS относится к соединению, stream id == 0)
	if(header.streamId != 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если получен фрейм подтверждения (ACK)
	if(header.flags & flag::ACK){
		// Если нагрузка подтверждения не пустая (ACK обязан быть пустым)
		if(header.length != 0){
			// Фиксируем некорректный размер фрейма
			error = error_t::FRAME_SIZE_ERROR;
			// Выводим ошибку разбора
			return status_t::ERROR;
		}
		// Полезная нагрузка разобрана
		return status_t::OK;
	}
	// Если размер нагрузки не кратен 6 байтам (размер одного параметра)
	if((header.length % 6) != 0){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Определяем количество параметров в нагрузке
	const size_t count = (header.length / 6);
	// Резервируем память под все параметры
	output.reserve(output.size() + count);
	// Буфер текущей позиции полезной нагрузки
	const uint8_t * buffer = payload;
	/**
	 * Выполняем перебор всех параметров нагрузки
	 */
	for(size_t i = 0; i < count; ++i){
		// Создаём объект параметра
		setting_entry_t entry;
		// Извлекаем идентификатор параметра
		entry.id = static_cast <setting_t> (::rd16(buffer));
		// Извлекаем значение параметра
		entry.value = ::rd32(buffer + 2);
		// Дописываем параметр в список
		output.push_back(entry);
		// Сдвигаем указатель на следующий параметр
		buffer += 6;
	}
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки CONTINUATION (фрагмент блока заголовков)
 *
 * @param header     заголовок фрейма
 * @param payload    полезная нагрузка фрейма
 * @param block      фрагмент блока заголовков (zero-copy во входной буфер)
 * @param endHeaders флаг завершения блока заголовков
 * @param error      код ошибки протокола
 * @return           результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::continuation(const header_t & header, const uint8_t * payload, string_view & block, bool & endHeaders, error_t & error) noexcept {
	// Если фрейм не принадлежит потоку (CONTINUATION обязан иметь stream id != 0)
	if(header.streamId == 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем фрагмент блока заголовков (zero-copy во входной буфер)
	block = string_view(reinterpret_cast <const char *> (payload), header.length);
	// Извлекаем флаг завершения блока заголовков
	endHeaders = ((header.flags & flag::END_HEADERS) != 0);
	// Полезная нагрузка разобрана
	return status_t::OK;
}

/**
 * @brief Функция сборки фрейма PING (заголовок + нагрузка дописываются в output)
 *
 * @param output выходной буфер соединения
 * @param opaque произвольные opaque-данные (8 байт)
 * @param ack    флаг подтверждения получения PING пира
 *
 */
void awh::http::h2::frame::serialize::ping(string & output, const uint8_t opaque[8], const bool ack) noexcept {
	// Дописываем заголовок фрейма PING
	::wrHeader(output, 8, frame_t::PING, (ack ? flag::ACK : flag::NONE), 0);
	// Дописываем opaque-данные
	output.append(reinterpret_cast <const char *> (opaque), 8);
}
/**
 * @brief Функция сборки фрейма RST_STREAM (заголовок + нагрузка дописываются в output)
 *
 * @param output   выходной буфер соединения
 * @param streamId идентификатор потока
 * @param error    код ошибки, с которым сбрасывается поток
 *
 */
void awh::http::h2::frame::serialize::rstStream(string & output, const uint32_t streamId, const error_t error) noexcept {
	// Дописываем заголовок фрейма RST_STREAM
	::wrHeader(output, 4, frame_t::RST_STREAM, flag::NONE, streamId);
	// Дописываем код ошибки сброса потока
	::wr32(output, static_cast <uint32_t> (error));
}
/**
 * @brief Функция сборки фрейма WINDOW_UPDATE (заголовок + нагрузка дописываются в output)
 *
 * @param output    выходной буфер соединения
 * @param streamId  идентификатор потока (0 - окно всего соединения)
 * @param increment инкремент окна flow control
 *
 */
void awh::http::h2::frame::serialize::windowUpdate(string & output, const uint32_t streamId, const uint32_t increment) noexcept {
	// Дописываем заголовок фрейма WINDOW_UPDATE
	::wrHeader(output, 4, frame_t::WINDOW_UPDATE, flag::NONE, streamId);
	// Дописываем инкремент окна (сбрасывая reserved-бит)
	::wr32(output, increment & proto::STREAM_ID_MASK);
}
/**
 * @brief Функция сборки фрейма DATA (заголовок + нагрузка дописываются в output)
 *
 * @param output    выходной буфер соединения
 * @param streamId  идентификатор потока
 * @param data      данные тела
 * @param endStream флаг завершения потока
 *
 */
void awh::http::h2::frame::serialize::data(string & output, const uint32_t streamId, string_view data, const bool endStream) noexcept {
	// Дописываем заголовок фрейма DATA
	::wrHeader(output, static_cast <uint32_t> (data.size()), frame_t::DATA, (endStream ? flag::END_STREAM : flag::NONE), streamId);
	// Дописываем данные тела
	output.append(data.data(), data.size());
}
/**
 * @brief Функция сборки фрейма SETTINGS (заголовок + нагрузка дописываются в output)
 *
 * @param output выходной буфер соединения
 * @param items  список параметров (для ACK игнорируется)
 * @param count  количество параметров
 * @param ack    флаг подтверждения получения SETTINGS пира
 *
 */
void awh::http::h2::frame::serialize::settings(string & output, const setting_entry_t * items, const size_t count, const bool ack) noexcept {
	// Если требуется подтверждение получения SETTINGS пира
	if(ack){
		// Дописываем пустой фрейм SETTINGS с флагом ACK
		::wrHeader(output, 0, frame_t::SETTINGS, flag::ACK, 0);
		// Выходим из функции
		return;
	}
	// Дописываем заголовок фрейма SETTINGS
	::wrHeader(output, static_cast <uint32_t> (count * 6), frame_t::SETTINGS, flag::NONE, 0);
	/**
	 * Выполняем перебор всех параметров
	 */
	for(size_t i = 0; i < count; ++i){
		// Дописываем идентификатор параметра
		::wr16(output, static_cast <uint16_t> (items[i].id));
		// Дописываем значение параметра
		::wr32(output, items[i].value);
	}
}
/**
 * @brief Функция сборки фрейма GOAWAY (заголовок + нагрузка дописываются в output)
 *
 * @param output       выходной буфер соединения
 * @param lastStreamId наибольший идентификатор обработанного потока
 * @param error        код ошибки завершения соединения
 * @param debugData    необязательные отладочные данные
 *
 */
void awh::http::h2::frame::serialize::goaway(string & output, const uint32_t lastStreamId, const error_t error, string_view debugData) noexcept {
	// Дописываем заголовок фрейма GOAWAY
	::wrHeader(output, static_cast <uint32_t> (debugData.size() + 8), frame_t::GOAWAY, flag::NONE, 0);
	// Дописываем наибольший идентификатор обработанного потока (сбрасывая reserved-бит)
	::wr32(output, lastStreamId & proto::STREAM_ID_MASK);
	// Дописываем код ошибки завершения соединения
	::wr32(output, static_cast <uint32_t> (error));
	// Дописываем отладочные данные
	output.append(debugData.data(), debugData.size());
}
/**
 * @brief Функция сборки фрейма CONTINUATION (заголовок + нагрузка дописываются в output)
 *
 * @param output     выходной буфер соединения
 * @param streamId   идентификатор потока
 * @param block      фрагмент блока заголовков HPACK
 * @param endHeaders флаг завершения блока заголовков
 *
 */
void awh::http::h2::frame::serialize::continuation(string & output, const uint32_t streamId, string_view block, const bool endHeaders) noexcept {
	// Дописываем заголовок фрейма CONTINUATION
	::wrHeader(output, static_cast <uint32_t> (block.size()), frame_t::CONTINUATION, (endHeaders ? flag::END_HEADERS : flag::NONE), streamId);
	// Дописываем фрагмент блока заголовков
	output.append(block.data(), block.size());
}
/**
 * @brief Функция сборки фрейма HEADERS (заголовок + нагрузка дописываются в output)
 *
 * @param output     выходной буфер соединения
 * @param streamId   идентификатор потока
 * @param block      фрагмент блока заголовков HPACK
 * @param endStream  флаг завершения потока
 * @param endHeaders флаг завершения блока заголовков
 *
 */
void awh::http::h2::frame::serialize::headers(string & output, const uint32_t streamId, string_view block, const bool endStream, const bool endHeaders) noexcept {
	// Собираем флаги фрейма
	uint8_t flags = flag::NONE;
	// Если требуется завершение потока
	if(endStream)
		// Устанавливаем флаг завершения потока
		flags |= flag::END_STREAM;
	// Если блок заголовков завершён
	if(endHeaders)
		// Устанавливаем флаг завершения блока заголовков
		flags |= flag::END_HEADERS;
	// Дописываем заголовок фрейма HEADERS
	::wrHeader(output, static_cast <uint32_t> (block.size()), frame_t::HEADERS, flags, streamId);
	// Дописываем фрагмент блока заголовков
	output.append(block.data(), block.size());
}
/**
 * @brief Функция сборки фрейма PRIORITY (заголовок + нагрузка дописываются в output)
 *
 * @param output    выходной буфер соединения
 * @param streamId  идентификатор потока
 * @param exclusive флаг эксклюзивной зависимости потока
 * @param streamDep идентификатор потока, от которого зависит текущий
 * @param weight    вес потока
 *
 */
void awh::http::h2::frame::serialize::priority(string & output, const uint32_t streamId, const bool exclusive, const uint32_t streamDep, const uint8_t weight) noexcept {
	// Дописываем заголовок фрейма PRIORITY
	::wrHeader(output, 5, frame_t::PRIORITY, flag::NONE, streamId);
	// Дописываем зависимость потока с флагом эксклюзивности в старшем бите
	::wr32(output, (streamDep & proto::STREAM_ID_MASK) | (exclusive ? 0x80000000u : 0));
	// Дописываем вес потока
	output.push_back(static_cast <char> (weight));
}
/**
 * @brief Функция сборки HPACK-блока в HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
 *
 * @param output          выходной буфер соединения
 * @param streamId        идентификатор потока
 * @param block           блок заголовков HPACK целиком
 * @param endStream       флаг завершения потока
 * @param maxFramePayload максимальный размер полезной нагрузки одного фрейма (SETTINGS_MAX_FRAME_SIZE пира)
 *
 */
void awh::http::h2::frame::serialize::headerBlock(string & output, const uint32_t streamId, string_view block, const bool endStream, uint32_t maxFramePayload) noexcept {
	// Если максимальный размер нагрузки не задан или превышает допустимый протоколом
	if((maxFramePayload == 0) || (maxFramePayload > proto::MAX_FRAME_LENGTH))
		// Используем размер нагрузки по умолчанию
		maxFramePayload = proto::DEFAULT_MAX_FRAME_SIZE;
	// Максимальный размер одного фрагмента блока
	const size_t maxChunk = static_cast <size_t> (maxFramePayload);
	// Если блок помещается в один фрейм HEADERS
	if(block.size() <= maxChunk){
		// Дописываем единственный фрейм HEADERS с завершением блока
		headers(output, streamId, block, endStream, true);
		// Выходим из функции
		return;
	}
	// Признак первого фрейма последовательности
	bool first = true;
	// Текущее смещение в блоке заголовков
	size_t offset = 0;
	/**
	 * Выполняем нарезку блока на фреймы HEADERS + CONTINUATION
	 */
	while(offset < block.size()){
		// Определяем размер очередного фрагмента блока
		const size_t chunk = ::min(block.size() - offset, maxChunk);
		// Определяем признак последнего фрагмента блока
		const bool last = ((offset + chunk) >= block.size());
		// Формируем очередной фрагмент блока
		const string_view frag(block.data() + offset, chunk);
		// Если это первый фрейм последовательности
		if(first){
			// Дописываем фрейм HEADERS (END_STREAM ставится только на него)
			headers(output, streamId, frag, endStream, last);
			// Сбрасываем признак первого фрейма
			first = false;
		// Дописываем очередной фрейм CONTINUATION
		} else continuation(output, streamId, frag, last);
		// Сдвигаем смещение на размер фрагмента
		offset += chunk;
	}
}
/**
 * @brief Функция сборки фрейма PUSH_PROMISE (заголовок + нагрузка дописываются в output)
 *
 * @param output           выходной буфер соединения
 * @param streamId         идентификатор ассоциированного потока клиента
 * @param promisedStreamId идентификатор обещанного потока
 * @param block            фрагмент блока заголовков HPACK
 * @param endHeaders       флаг завершения блока заголовков
 *
 */
void awh::http::h2::frame::serialize::pushPromise(string & output, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const bool endHeaders) noexcept {
	// Дописываем заголовок фрейма PUSH_PROMISE (нагрузка включает 4 октета Promised Stream ID)
	::wrHeader(output, static_cast <uint32_t> (block.size() + 4), frame_t::PUSH_PROMISE, (endHeaders ? flag::END_HEADERS : flag::NONE), streamId);
	// Дописываем идентификатор обещанного потока (сбрасывая reserved-бит)
	::wr32(output, promisedStreamId & proto::STREAM_ID_MASK);
	// Дописываем фрагмент блока заголовков
	output.append(block.data(), block.size());
}
/**
 * @brief Функция сборки HPACK-блока обещанного запроса в PUSH_PROMISE + CONTINUATION
 *
 * @param output           выходной буфер соединения
 * @param streamId         идентификатор ассоциированного потока клиента
 * @param promisedStreamId идентификатор обещанного потока
 * @param block            блок заголовков HPACK целиком
 * @param maxFramePayload  максимальный размер полезной нагрузки одного фрейма
 *
 */
void awh::http::h2::frame::serialize::pushPromiseBlock(string & output, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, uint32_t maxFramePayload) noexcept {
	// Если максимальный размер нагрузки не задан или превышает допустимый протоколом
	if((maxFramePayload == 0) || (maxFramePayload > proto::MAX_FRAME_LENGTH))
		// Используем размер нагрузки по умолчанию
		maxFramePayload = proto::DEFAULT_MAX_FRAME_SIZE;
	// Если максимальный размер нагрузки меньше размера Promised Stream ID
	if(maxFramePayload < 4)
		// Устанавливаем минимально возможный размер нагрузки
		maxFramePayload = 4;
	// Максимальный размер первого фрагмента (за вычетом 4 октетов Promised Stream ID)
	const size_t firstMax = (static_cast <size_t> (maxFramePayload) - 4);
	// Если блок помещается в один фрейм PUSH_PROMISE
	if(block.size() <= firstMax){
		// Дописываем единственный фрейм PUSH_PROMISE с завершением блока
		pushPromise(output, streamId, promisedStreamId, block, true);
		// Выходим из функции
		return;
	}
	// Размер первого фрагмента блока
	const size_t firstChunk = firstMax;
	// Дописываем первый фрейм PUSH_PROMISE без завершения блока
	pushPromise(output, streamId, promisedStreamId, block.substr(0, firstChunk), false);
	// Текущее смещение в блоке заголовков
	size_t offset = firstChunk;
	// Максимальный размер одного фрагмента блока
	const size_t maxChunk = static_cast <size_t> (maxFramePayload);
	/**
	 * Выполняем нарезку остатка блока на фреймы CONTINUATION
	 */
	while(offset < block.size()){
		// Определяем размер очередного фрагмента блока
		const size_t chunk = ::min(block.size() - offset, maxChunk);
		// Определяем признак последнего фрагмента блока
		const bool last = ((offset + chunk) >= block.size());
		// Дописываем очередной фрейм CONTINUATION
		continuation(output, streamId, string_view(block.data() + offset, chunk), last);
		// Сдвигаем смещение на размер фрагмента
		offset += chunk;
	}
}
/**
 * @brief Функция разбора полезной нагрузки PRIORITY_UPDATE (RFC 9218 §7.1)
 *
 * @param header   заголовок фрейма
 * @param payload  полезная нагрузка фрейма
 * @param streamId идентификатор приоритизируемого потока
 * @param value    значение поля приоритета (zero-copy во входной буфер)
 * @param error    код ошибки протокола
 * @return         результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::frame::parser::priorityUpdate(const header_t & header, const uint8_t * payload, uint32_t & streamId, string_view & value, error_t & error) noexcept {
	// Если фрейм принадлежит потоку (PRIORITY_UPDATE относится к соединению, stream id == 0)
	if(header.streamId != 0){
		// Фиксируем нарушение протокола
		error = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если нагрузка не содержит обязательные 4 байта идентификатора потока
	if(header.length < 4){
		// Фиксируем некорректный размер фрейма
		error = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем идентификатор приоритизируемого потока (сбрасывая reserved-бит)
	streamId = (::rd32(payload) & proto::STREAM_ID_MASK);
	// Извлекаем значение поля приоритета (zero-copy во входной буфер)
	value = string_view(reinterpret_cast <const char *> (payload + 4), header.length - 4);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция сборки фрейма PRIORITY_UPDATE (RFC 9218 §7.1)
 *
 * @param output   выходной буфер соединения
 * @param streamId идентификатор приоритизируемого потока
 * @param value    значение поля приоритета (структурированный словарь, например "u=2, i")
 *
 */
void awh::http::h2::frame::serialize::priorityUpdate(string & output, const uint32_t streamId, string_view value) noexcept {
	// Дописываем заголовок фрейма PRIORITY_UPDATE (нагрузка включает 4 октета идентификатора потока)
	::wrHeader(output, static_cast <uint32_t> (value.size() + 4), frame_t::PRIORITY_UPDATE, flag::NONE, 0);
	// Дописываем идентификатор приоритизируемого потока (сбрасывая reserved-бит)
	::wr32(output, streamId & proto::STREAM_ID_MASK);
	// Дописываем значение поля приоритета
	output.append(value.data(), value.size());
}
/**
 * @brief Функция разбора полезной нагрузки ALTSVC (RFC 7838 §4)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param origin  origin анонсируемого сервиса (zero-copy во входной буфер)
 * @param value   значение поля Alt-Svc (zero-copy во входной буфер)
 * @return        признак пригодности кадра к обработке
 *
 */
bool awh::http::h2::frame::parser::altsvc(const header_t & header, const uint8_t * payload, string_view & origin, string_view & value) noexcept {
	// Если нагрузка не содержит обязательные два октета длины origin
	if(header.length < 2)
		// Кадр подлежит игнорированию
		return false;
	// Извлекаем длину origin
	const uint16_t length = ::rd16(payload);
	// Если origin не помещается в нагрузку
	if((static_cast <uint32_t> (length) + 2) > header.length)
		// Кадр подлежит игнорированию
		return false;
	/**
	 * Кадр соединения обязан нести origin, кадр потока - не нести: origin
	 * там определяется самим потоком, и присланный вместе с ним RFC 7838 §4
	 * предписывает игнорировать
	 */
	if((header.streamId == 0) == (length == 0))
		// Кадр подлежит игнорированию
		return false;
	// Извлекаем origin анонсируемого сервиса
	origin = string_view(reinterpret_cast <const char *> (payload + 2), length);
	// Извлекаем значение поля Alt-Svc
	value = string_view(reinterpret_cast <const char *> (payload + 2 + length), (header.length - 2 - length));
	// Кадр пригоден к обработке
	return true;
}
/**
 * @brief Функция разбора полезной нагрузки ORIGIN (RFC 8336 §2)
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param origins набор origin (zero-copy во входной буфер)
 * @return        признак пригодности кадра к обработке
 *
 */
bool awh::http::h2::frame::parser::origin(const header_t & header, const uint8_t * payload, vector <string_view> & origins) noexcept {
	// Выполняем очистку набора origin
	origins.clear();
	/**
	 * Кадр относится к соединению целиком, поэтому в потоке он бессмыслен
	 * и подлежит игнорированию (RFC 8336 §2.1)
	 */
	if(header.streamId != 0)
		// Кадр подлежит игнорированию
		return false;
	// Позиция разбора в полезной нагрузке
	uint32_t pos = 0;
	/**
	 * Выполняем разбор всех записей набора origin
	 */
	while(pos < header.length){
		// Если запись не содержит обязательные два октета длины
		if((header.length - pos) < 2){
			// Выполняем очистку набора origin
			origins.clear();
			// Кадр подлежит игнорированию целиком
			return false;
		}
		// Извлекаем длину очередного origin
		const uint16_t length = ::rd16(payload + pos);
		// Сдвигаем позицию разбора за длину
		pos += 2;
		// Если origin не помещается в оставшуюся нагрузку
		if(length > (header.length - pos)){
			// Выполняем очистку набора origin
			origins.clear();
			// Кадр подлежит игнорированию целиком
			return false;
		}
		// Дописываем очередной origin в набор
		origins.emplace_back(reinterpret_cast <const char *> (payload + pos), length);
		// Сдвигаем позицию разбора за origin
		pos += length;
	}
	// Кадр пригоден к обработке
	return true;
}
/**
 * @brief Функция сборки фрейма ALTSVC (RFC 7838 §4)
 *
 * @param output   выходной буфер соединения
 * @param streamId идентификатор потока, либо 0 для соединения
 * @param origin   origin анонсируемого сервиса
 * @param value    значение поля Alt-Svc (RFC 7838 §3)
 *
 */
void awh::http::h2::frame::serialize::altsvc(string & output, const uint32_t streamId, string_view origin, string_view value) noexcept {
	// Дописываем заголовок фрейма ALTSVC (нагрузка включает 2 октета длины origin)
	::wrHeader(output, static_cast <uint32_t> (origin.size() + value.size() + 2), frame_t::ALTSVC, flag::NONE, streamId);
	// Дописываем длину origin анонсируемого сервиса
	::wr16(output, static_cast <uint16_t> (origin.size()));
	// Дописываем origin анонсируемого сервиса
	output.append(origin.data(), origin.size());
	// Дописываем значение поля Alt-Svc
	output.append(value.data(), value.size());
}
/**
 * @brief Функция сборки фрейма ORIGIN (RFC 8336 §2)
 *
 * @param output  выходной буфер соединения
 * @param origins набор origin, обслуживаемых соединением
 *
 */
void awh::http::h2::frame::serialize::origin(string & output, const vector <string> & origins) noexcept {
	// Размер полезной нагрузки кадра
	size_t length = 0;
	/**
	 * Выполняем подсчёт размера полезной нагрузки заранее: он входит в заголовок,
	 * а тот пишется до самой нагрузки
	 */
	for(auto & item : origins)
		// Наращиваем размер нагрузки на запись набора
		length += (item.size() + 2);
	// Дописываем заголовок фрейма ORIGIN
	::wrHeader(output, static_cast <uint32_t> (length), frame_t::ORIGIN, flag::NONE, 0);
	/**
	 * Выполняем запись всех записей набора origin
	 */
	for(auto & item : origins){
		// Дописываем длину очередного origin
		::wr16(output, static_cast <uint16_t> (item.size()));
		// Дописываем очередной origin
		output.append(item);
	}
}
