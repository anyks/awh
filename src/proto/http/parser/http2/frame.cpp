/**
 * @file: frame.cpp
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
 * Внутренние вспомогательные функции чтения/записи в сетевом (big-endian) порядке
 */
namespace {
	/**
	 * Используем пространство имён внутренних слоёв протокола HTTP/2
	 */
	using namespace awh::http::h2;

	/**
	 * @brief Функция чтения 16-битного числа в сетевом порядке байт
	 *
	 * @param p указатель на данные
	 * @return  прочитанное число
	 */
	inline uint16_t rd16(const uint8_t * p) noexcept {
		// Собираем число из двух байт в сетевом порядке
		return static_cast <uint16_t> ((static_cast <uint16_t> (p[0]) << 8) | p[1]);
	}
	/**
	 * @brief Функция чтения 24-битного числа в сетевом порядке байт
	 *
	 * @param p указатель на данные
	 * @return  прочитанное число
	 */
	inline uint32_t rd24(const uint8_t * p) noexcept {
		// Собираем число из трёх байт в сетевом порядке
		return (static_cast <uint32_t> (p[0]) << 16) | (static_cast <uint32_t> (p[1]) << 8) | p[2];
	}
	/**
	 * @brief Функция чтения 32-битного числа в сетевом порядке байт
	 *
	 * @param p указатель на данные
	 * @return  прочитанное число
	 */
	inline uint32_t rd32(const uint8_t * p) noexcept {
		// Собираем число из четырёх байт в сетевом порядке
		return (static_cast <uint32_t> (p[0]) << 24) | (static_cast <uint32_t> (p[1]) << 16) |
		       (static_cast <uint32_t> (p[2]) << 8)  |  static_cast <uint32_t> (p[3]);
	}
	/**
	 * @brief Функция записи 16-битного числа в сетевом порядке байт
	 *
	 * @param out выходной буфер
	 * @param v   записываемое число
	 */
	inline void wr16(string & out, const uint16_t v) noexcept {
		// Дописываем старший байт числа
		out.push_back(static_cast <char> ((v >> 8) & 0xFF));
		// Дописываем младший байт числа
		out.push_back(static_cast <char> (v & 0xFF));
	}
	/**
	 * @brief Функция записи 24-битного числа в сетевом порядке байт
	 *
	 * @param out выходной буфер
	 * @param v   записываемое число
	 */
	inline void wr24(string & out, const uint32_t v) noexcept {
		// Дописываем старший байт числа
		out.push_back(static_cast <char> ((v >> 16) & 0xFF));
		// Дописываем средний байт числа
		out.push_back(static_cast <char> ((v >> 8) & 0xFF));
		// Дописываем младший байт числа
		out.push_back(static_cast <char> (v & 0xFF));
	}
	/**
	 * @brief Функция записи 32-битного числа в сетевом порядке байт
	 *
	 * @param out выходной буфер
	 * @param v   записываемое число
	 */
	inline void wr32(string & out, const uint32_t v) noexcept {
		// Дописываем старший байт числа
		out.push_back(static_cast <char> ((v >> 24) & 0xFF));
		// Дописываем второй байт числа
		out.push_back(static_cast <char> ((v >> 16) & 0xFF));
		// Дописываем третий байт числа
		out.push_back(static_cast <char> ((v >> 8) & 0xFF));
		// Дописываем младший байт числа
		out.push_back(static_cast <char> (v & 0xFF));
	}
	/**
	 * @brief Функция записи 9-байтового заголовка фрейма (RFC 9113 §4.1)
	 *
	 * @param out      выходной буфер
	 * @param length   длина полезной нагрузки
	 * @param type     тип фрейма
	 * @param flags    флаги фрейма
	 * @param streamId идентификатор потока
	 */
	inline void wrHeader(string & out, const uint32_t length, const frame_t type, const uint8_t flags, const uint32_t streamId) noexcept {
		// Дописываем 24-битную длину полезной нагрузки
		wr24(out, length);
		// Дописываем тип фрейма
		out.push_back(static_cast <char> (type));
		// Дописываем флаги фрейма
		out.push_back(static_cast <char> (flags));
		// Дописываем 31-битный идентификатор потока (сбрасывая reserved-бит)
		wr32(out, streamId & proto::STREAM_ID_MASK);
	}
	/**
	 * @brief Функция снятия padding с нагрузки фреймов DATA/HEADERS/PUSH_PROMISE
	 *
	 * @details При наличии флага PADDED первый байт нагрузки - Pad Length,
	 *          в хвосте нагрузки идёт сам padding
	 *
	 * @param padded признак наличия флага PADDED
	 * @param p      указатель на полезную нагрузку (сдвигается)
	 * @param len    длина полезной нагрузки (уменьшается)
	 * @return       результат снятия (false - Pad Length некорректен, PROTOCOL_ERROR)
	 */
	inline bool stripPadding(const bool padded, const uint8_t *& p, size_t & len) noexcept {
		// Если флаг PADDED не установлен - снимать нечего
		if(!padded)
			// Продолжаем разбор
			return true;
		// Если нагрузка не содержит даже байта Pad Length
		if(len < 1)
			// Нагрузка некорректна
			return false;
		// Извлекаем длину padding
		const uint8_t padLen = p[0];
		// Сдвигаем указатель за байт Pad Length
		p += 1;
		// Уменьшаем длину нагрузки на байт Pad Length
		len -= 1;
		// Если padding не помещается в оставшуюся нагрузку
		if(padLen > len)
			// Нагрузка некорректна
			return false;
		// Отбрасываем padding из хвоста нагрузки
		len -= padLen;
		// Продолжаем разбор
		return true;
	}
}

/**
 * @brief Функция разбора 9-байтового заголовка фрейма
 *
 * @param data входной буфер
 * @param size доступно байт
 * @param out  разобранный заголовок фрейма
 * @return     результат разбора (true - в буфере было достаточно байт и заголовок разобран)
 */
bool awh::http::h2::frame::parseHeader(const uint8_t * data, const size_t size, header_t & out) noexcept {
	// Если в буфере недостаточно байт для заголовка фрейма
	if(size < proto::FRAME_HEADER_SIZE)
		// Заголовок разобрать невозможно
		return false;
	// Извлекаем 24-битную длину полезной нагрузки
	out.length = ::rd24(data);
	// Извлекаем тип фрейма
	out.type = static_cast <frame_t> (data[3]);
	// Извлекаем флаги фрейма
	out.flags = data[4];
	// Извлекаем 31-битный идентификатор потока (сбрасывая reserved-бит)
	out.streamId = (::rd32(data + 5) & proto::STREAM_ID_MASK);
	// Заголовок фрейма разобран
	return true;
}
/**
 * @brief Функция разбора полезной нагрузки DATA
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param out     разобранная полезная нагрузка
 * @param err     код ошибки протокола (PROTOCOL_ERROR на некорректном padding)
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseData(const header_t & h, const uint8_t * payload, data_t & out, error_t & err) noexcept {
	// Если фрейм не принадлежит потоку (DATA обязан иметь stream id != 0)
	if(h.streamId == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Указатель на текущую позицию полезной нагрузки
	const uint8_t * p = payload;
	// Оставшаяся длина полезной нагрузки
	size_t len = h.length;
	// Если снятие padding не удалось
	if(!::stripPadding(h.flags & flag::PADDED, p, len)){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем данные тела (zero-copy во входной буфер)
	out.data = string_view(reinterpret_cast <const char *> (p), len);
	// Извлекаем флаг завершения потока
	out.endStream = ((h.flags & flag::END_STREAM) != 0);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки HEADERS (с учётом padding и приоритета)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param out     разобранная полезная нагрузка
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseHeaders(const header_t & h, const uint8_t * payload, headers_t & out, error_t & err) noexcept {
	// Если фрейм не принадлежит потоку (HEADERS обязан иметь stream id != 0)
	if(h.streamId == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Указатель на текущую позицию полезной нагрузки
	const uint8_t * p = payload;
	// Оставшаяся длина полезной нагрузки
	size_t len = h.length;
	// Если снятие padding не удалось
	if(!::stripPadding(h.flags & flag::PADDED, p, len)){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем флаг завершения потока
	out.endStream = ((h.flags & flag::END_STREAM) != 0);
	// Извлекаем флаг завершения блока заголовков
	out.endHeaders = ((h.flags & flag::END_HEADERS) != 0);
	// Извлекаем флаг наличия полей приоритета
	out.hasPriority = ((h.flags & flag::PRIORITY) != 0);
	// Если поля приоритета присутствуют (RFC 7540, deprecated)
	if(out.hasPriority){
		// Если нагрузка не содержит 5 байт полей приоритета
		if(len < 5){
			// Фиксируем некорректный размер фрейма
			err = error_t::FRAME_SIZE_ERROR;
			// Выводим ошибку разбора
			return status_t::ERROR;
		}
		// Извлекаем зависимость потока (32 бита)
		const uint32_t dep = ::rd32(p);
		// Извлекаем флаг эксклюзивной зависимости (старший бит)
		out.exclusive = ((dep & 0x80000000u) != 0);
		// Извлекаем идентификатор потока, от которого зависит текущий
		out.streamDep = (dep & proto::STREAM_ID_MASK);
		// Извлекаем вес потока
		out.weight = p[4];
		// Сдвигаем указатель за поля приоритета
		p += 5;
		// Уменьшаем длину нагрузки на поля приоритета
		len -= 5;
	}
	// Извлекаем фрагмент блока заголовков (zero-copy во входной буфер)
	out.block = string_view(reinterpret_cast <const char *> (p), len);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки PRIORITY (требует ровно 5 байт)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param out     разобранная полезная нагрузка
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parsePriority(const header_t & h, const uint8_t * payload, priority_t & out, error_t & err) noexcept {
	// Если фрейм не принадлежит потоку (PRIORITY обязан иметь stream id != 0)
	if(h.streamId == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если размер нагрузки не равен ровно 5 байтам
	if(h.length != 5){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем зависимость потока (32 бита)
	const uint32_t dep = ::rd32(payload);
	// Извлекаем флаг эксклюзивной зависимости (старший бит)
	out.exclusive = ((dep & 0x80000000u) != 0);
	// Извлекаем идентификатор потока, от которого зависит текущий
	out.streamDep = (dep & proto::STREAM_ID_MASK);
	// Извлекаем вес потока
	out.weight = payload[4];
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки RST_STREAM (требует ровно 4 байта)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param code    код ошибки, с которым сброшен поток
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseRstStream(const header_t & h, const uint8_t * payload, error_t & code, error_t & err) noexcept {
	// Если фрейм не принадлежит потоку (RST_STREAM обязан иметь stream id != 0)
	if(h.streamId == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если размер нагрузки не равен ровно 4 байтам
	if(h.length != 4){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем код ошибки, с которым сброшен поток
	code = static_cast <error_t> (::rd32(payload));
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки SETTINGS (длина кратна 6)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param out     список разобранных параметров
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseSettings(const header_t & h, const uint8_t * payload, vector <setting_entry_t> & out, error_t & err) noexcept {
	// Если фрейм принадлежит потоку (SETTINGS относится к соединению, stream id == 0)
	if(h.streamId != 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если получен фрейм подтверждения (ACK)
	if(h.flags & flag::ACK){
		// Если нагрузка подтверждения не пустая (ACK обязан быть пустым)
		if(h.length != 0){
			// Фиксируем некорректный размер фрейма
			err = error_t::FRAME_SIZE_ERROR;
			// Выводим ошибку разбора
			return status_t::ERROR;
		}
		// Полезная нагрузка разобрана
		return status_t::OK;
	}
	// Если размер нагрузки не кратен 6 байтам (размер одного параметра)
	if((h.length % 6) != 0){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Определяем количество параметров в нагрузке
	const size_t count = (h.length / 6);
	// Резервируем память под все параметры
	out.reserve(out.size() + count);
	// Указатель на текущую позицию полезной нагрузки
	const uint8_t * p = payload;
	/**
	 * Выполняем перебор всех параметров нагрузки
	 */
	for(size_t i = 0; i < count; ++i){
		// Создаём объект параметра
		setting_entry_t entry;
		// Извлекаем идентификатор параметра
		entry.id = static_cast <setting_t> (::rd16(p));
		// Извлекаем значение параметра
		entry.value = ::rd32(p + 2);
		// Дописываем параметр в список
		out.push_back(entry);
		// Сдвигаем указатель на следующий параметр
		p += 6;
	}
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки PUSH_PROMISE (с учётом padding)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param out     разобранная полезная нагрузка
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parsePushPromise(const header_t & h, const uint8_t * payload, push_promise_t & out, error_t & err) noexcept {
	// Если фрейм не принадлежит потоку (PUSH_PROMISE обязан иметь stream id != 0)
	if(h.streamId == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Указатель на текущую позицию полезной нагрузки
	const uint8_t * p = payload;
	// Оставшаяся длина полезной нагрузки
	size_t len = h.length;
	// Если снятие padding не удалось
	if(!::stripPadding(h.flags & flag::PADDED, p, len)){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если нагрузка не содержит 4 байта Promised Stream ID
	if(len < 4){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем идентификатор обещанного потока (сбрасывая reserved-бит)
	out.promisedStreamId = (::rd32(p) & proto::STREAM_ID_MASK);
	// Извлекаем флаг завершения блока заголовков
	out.endHeaders = ((h.flags & flag::END_HEADERS) != 0);
	// Извлекаем фрагмент блока заголовков (zero-copy во входной буфер)
	out.block = string_view(reinterpret_cast <const char *> (p + 4), len - 4);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки PING (требует ровно 8 байт opaque-данных)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param opaque  извлечённые opaque-данные
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parsePing(const header_t & h, const uint8_t * payload, uint8_t opaque[8], error_t & err) noexcept {
	// Если фрейм принадлежит потоку (PING относится к соединению, stream id == 0)
	if(h.streamId != 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если размер нагрузки не равен ровно 8 байтам
	if(h.length != 8){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
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
 * @brief Функция разбора полезной нагрузки GOAWAY (минимум 8 байт)
 *
 * @param h       заголовок фрейма
 * @param payload полезная нагрузка фрейма
 * @param out     разобранная полезная нагрузка
 * @param err     код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseGoaway(const header_t & h, const uint8_t * payload, goaway_t & out, error_t & err) noexcept {
	// Если фрейм принадлежит потоку (GOAWAY относится к соединению, stream id == 0)
	if(h.streamId != 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Если нагрузка не содержит обязательные 8 байт
	if(h.length < 8){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем наибольший идентификатор обработанного потока (сбрасывая reserved-бит)
	out.lastStreamId = (::rd32(payload) & proto::STREAM_ID_MASK);
	// Извлекаем код ошибки завершения соединения
	out.code = static_cast <error_t> (::rd32(payload + 4));
	// Извлекаем отладочные данные (zero-copy во входной буфер)
	out.debugData = string_view(reinterpret_cast <const char *> (payload + 8), h.length - 8);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки WINDOW_UPDATE (требует ровно 4 байта)
 *
 * @param h         заголовок фрейма
 * @param payload   полезная нагрузка фрейма
 * @param increment извлечённый инкремент окна
 * @param err       код ошибки протокола
 * @return          результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseWindowUpdate(const header_t & h, const uint8_t * payload, uint32_t & increment, error_t & err) noexcept {
	// Если размер нагрузки не равен ровно 4 байтам
	if(h.length != 4){
		// Фиксируем некорректный размер фрейма
		err = error_t::FRAME_SIZE_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем инкремент окна (сбрасывая reserved-бит)
	increment = (::rd32(payload) & proto::STREAM_ID_MASK);
	// Если получен нулевой инкремент (PROTOCOL_ERROR по RFC 9113 §6.9)
	if(increment == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция разбора полезной нагрузки CONTINUATION (фрагмент блока заголовков)
 *
 * @param h          заголовок фрейма
 * @param payload    полезная нагрузка фрейма
 * @param block      фрагмент блока заголовков (zero-copy во входной буфер)
 * @param endHeaders флаг завершения блока заголовков
 * @param err        код ошибки протокола
 * @return           результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::frame::parseContinuation(const header_t & h, const uint8_t * payload, string_view & block, bool & endHeaders, error_t & err) noexcept {
	// Если фрейм не принадлежит потоку (CONTINUATION обязан иметь stream id != 0)
	if(h.streamId == 0){
		// Фиксируем нарушение протокола
		err = error_t::PROTOCOL_ERROR;
		// Выводим ошибку разбора
		return status_t::ERROR;
	}
	// Извлекаем фрагмент блока заголовков (zero-copy во входной буфер)
	block = string_view(reinterpret_cast <const char *> (payload), h.length);
	// Извлекаем флаг завершения блока заголовков
	endHeaders = ((h.flags & flag::END_HEADERS) != 0);
	// Полезная нагрузка разобрана
	return status_t::OK;
}
/**
 * @brief Функция сборки фрейма DATA (заголовок + нагрузка дописываются в out)
 *
 * @param out       выходной буфер соединения
 * @param streamId  идентификатор потока
 * @param data      данные тела
 * @param endStream флаг завершения потока
 */
void awh::http::h2::frame::serializeData(string & out, const uint32_t streamId, string_view data, const bool endStream) noexcept {
	// Дописываем заголовок фрейма DATA
	::wrHeader(out, static_cast <uint32_t> (data.size()), frame_t::DATA, (endStream ? flag::END_STREAM : flag::NONE), streamId);
	// Дописываем данные тела
	out.append(data.data(), data.size());
}
/**
 * @brief Функция сборки фрейма HEADERS (заголовок + нагрузка дописываются в out)
 *
 * @param out        выходной буфер соединения
 * @param streamId   идентификатор потока
 * @param block      фрагмент блока заголовков HPACK
 * @param endStream  флаг завершения потока
 * @param endHeaders флаг завершения блока заголовков
 */
void awh::http::h2::frame::serializeHeaders(string & out, const uint32_t streamId, string_view block, const bool endStream, const bool endHeaders) noexcept {
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
	::wrHeader(out, static_cast <uint32_t> (block.size()), frame_t::HEADERS, flags, streamId);
	// Дописываем фрагмент блока заголовков
	out.append(block.data(), block.size());
}
/**
 * @brief Функция сборки фрейма CONTINUATION (заголовок + нагрузка дописываются в out)
 *
 * @param out        выходной буфер соединения
 * @param streamId   идентификатор потока
 * @param block      фрагмент блока заголовков HPACK
 * @param endHeaders флаг завершения блока заголовков
 */
void awh::http::h2::frame::serializeContinuation(string & out, const uint32_t streamId, string_view block, const bool endHeaders) noexcept {
	// Дописываем заголовок фрейма CONTINUATION
	::wrHeader(out, static_cast <uint32_t> (block.size()), frame_t::CONTINUATION, (endHeaders ? flag::END_HEADERS : flag::NONE), streamId);
	// Дописываем фрагмент блока заголовков
	out.append(block.data(), block.size());
}
/**
 * @brief Функция сборки HPACK-блока в HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
 *
 * @param out             выходной буфер соединения
 * @param streamId        идентификатор потока
 * @param block           блок заголовков HPACK целиком
 * @param endStream       флаг завершения потока
 * @param maxFramePayload максимальный размер полезной нагрузки одного фрейма (SETTINGS_MAX_FRAME_SIZE пира)
 */
void awh::http::h2::frame::serializeHeaderBlock(string & out, const uint32_t streamId, string_view block, const bool endStream, uint32_t maxFramePayload) noexcept {
	// Если максимальный размер нагрузки не задан или превышает допустимый протоколом
	if((maxFramePayload == 0) || (maxFramePayload > proto::MAX_FRAME_LENGTH))
		// Используем размер нагрузки по умолчанию
		maxFramePayload = proto::DEFAULT_MAX_FRAME_SIZE;
	// Максимальный размер одного фрагмента блока
	const size_t maxChunk = static_cast <size_t> (maxFramePayload);
	// Если блок помещается в один фрейм HEADERS
	if(block.size() <= maxChunk){
		// Дописываем единственный фрейм HEADERS с завершением блока
		serializeHeaders(out, streamId, block, endStream, true);
		// Выходим из функции
		return;
	}
	// Текущее смещение в блоке заголовков
	size_t off = 0;
	// Признак первого фрейма последовательности
	bool first = true;
	/**
	 * Выполняем нарезку блока на фреймы HEADERS + CONTINUATION
	 */
	while(off < block.size()){
		// Определяем размер очередного фрагмента блока
		const size_t chunk = ::min(block.size() - off, maxChunk);
		// Определяем признак последнего фрагмента блока
		const bool last = ((off + chunk) >= block.size());
		// Формируем очередной фрагмент блока
		const string_view frag(block.data() + off, chunk);
		// Если это первый фрейм последовательности
		if(first){
			// Дописываем фрейм HEADERS (END_STREAM ставится только на него)
			serializeHeaders(out, streamId, frag, endStream, last);
			// Сбрасываем признак первого фрейма
			first = false;
		// Дописываем очередной фрейм CONTINUATION
		} else serializeContinuation(out, streamId, frag, last);
		// Сдвигаем смещение на размер фрагмента
		off += chunk;
	}
}
/**
 * @brief Функция сборки HPACK-блока обещанного запроса в PUSH_PROMISE + CONTINUATION
 *
 * @param out              выходной буфер соединения
 * @param streamId         идентификатор ассоциированного потока клиента
 * @param promisedStreamId идентификатор обещанного потока
 * @param block            блок заголовков HPACK целиком
 * @param maxFramePayload  максимальный размер полезной нагрузки одного фрейма
 */
void awh::http::h2::frame::serializePushPromiseBlock(string & out, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, uint32_t maxFramePayload) noexcept {
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
		serializePushPromise(out, streamId, promisedStreamId, block, true);
		// Выходим из функции
		return;
	}
	// Размер первого фрагмента блока
	const size_t firstChunk = firstMax;
	// Дописываем первый фрейм PUSH_PROMISE без завершения блока
	serializePushPromise(out, streamId, promisedStreamId, block.substr(0, firstChunk), false);
	// Текущее смещение в блоке заголовков
	size_t off = firstChunk;
	// Максимальный размер одного фрагмента блока
	const size_t maxChunk = static_cast <size_t> (maxFramePayload);
	/**
	 * Выполняем нарезку остатка блока на фреймы CONTINUATION
	 */
	while(off < block.size()){
		// Определяем размер очередного фрагмента блока
		const size_t chunk = ::min(block.size() - off, maxChunk);
		// Определяем признак последнего фрагмента блока
		const bool last = ((off + chunk) >= block.size());
		// Дописываем очередной фрейм CONTINUATION
		serializeContinuation(out, streamId, string_view(block.data() + off, chunk), last);
		// Сдвигаем смещение на размер фрагмента
		off += chunk;
	}
}
/**
 * @brief Функция сборки фрейма PRIORITY (заголовок + нагрузка дописываются в out)
 *
 * @param out       выходной буфер соединения
 * @param streamId  идентификатор потока
 * @param exclusive флаг эксклюзивной зависимости потока
 * @param streamDep идентификатор потока, от которого зависит текущий
 * @param weight    вес потока
 */
void awh::http::h2::frame::serializePriority(string & out, const uint32_t streamId, const bool exclusive, const uint32_t streamDep, const uint8_t weight) noexcept {
	// Дописываем заголовок фрейма PRIORITY
	::wrHeader(out, 5, frame_t::PRIORITY, flag::NONE, streamId);
	// Дописываем зависимость потока с флагом эксклюзивности в старшем бите
	::wr32(out, (streamDep & proto::STREAM_ID_MASK) | (exclusive ? 0x80000000u : 0));
	// Дописываем вес потока
	out.push_back(static_cast <char> (weight));
}
/**
 * @brief Функция сборки фрейма RST_STREAM (заголовок + нагрузка дописываются в out)
 *
 * @param out      выходной буфер соединения
 * @param streamId идентификатор потока
 * @param code     код ошибки, с которым сбрасывается поток
 */
void awh::http::h2::frame::serializeRstStream(string & out, const uint32_t streamId, const error_t code) noexcept {
	// Дописываем заголовок фрейма RST_STREAM
	::wrHeader(out, 4, frame_t::RST_STREAM, flag::NONE, streamId);
	// Дописываем код ошибки сброса потока
	::wr32(out, static_cast <uint32_t> (code));
}
/**
 * @brief Функция сборки фрейма SETTINGS (заголовок + нагрузка дописываются в out)
 *
 * @param out   выходной буфер соединения
 * @param items список параметров (для ACK игнорируется)
 * @param count количество параметров
 * @param ack   флаг подтверждения получения SETTINGS пира
 */
void awh::http::h2::frame::serializeSettings(string & out, const setting_entry_t * items, const size_t count, const bool ack) noexcept {
	// Если требуется подтверждение получения SETTINGS пира
	if(ack){
		// Дописываем пустой фрейм SETTINGS с флагом ACK
		::wrHeader(out, 0, frame_t::SETTINGS, flag::ACK, 0);
		// Выходим из функции
		return;
	}
	// Дописываем заголовок фрейма SETTINGS
	::wrHeader(out, static_cast <uint32_t> (count * 6), frame_t::SETTINGS, flag::NONE, 0);
	/**
	 * Выполняем перебор всех параметров
	 */
	for(size_t i = 0; i < count; ++i){
		// Дописываем идентификатор параметра
		::wr16(out, static_cast <uint16_t> (items[i].id));
		// Дописываем значение параметра
		::wr32(out, items[i].value);
	}
}
/**
 * @brief Функция сборки фрейма PUSH_PROMISE (заголовок + нагрузка дописываются в out)
 *
 * @param out              выходной буфер соединения
 * @param streamId         идентификатор ассоциированного потока клиента
 * @param promisedStreamId идентификатор обещанного потока
 * @param block            фрагмент блока заголовков HPACK
 * @param endHeaders       флаг завершения блока заголовков
 */
void awh::http::h2::frame::serializePushPromise(string & out, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const bool endHeaders) noexcept {
	// Дописываем заголовок фрейма PUSH_PROMISE (нагрузка включает 4 октета Promised Stream ID)
	::wrHeader(out, static_cast <uint32_t> (block.size() + 4), frame_t::PUSH_PROMISE, (endHeaders ? flag::END_HEADERS : flag::NONE), streamId);
	// Дописываем идентификатор обещанного потока (сбрасывая reserved-бит)
	::wr32(out, promisedStreamId & proto::STREAM_ID_MASK);
	// Дописываем фрагмент блока заголовков
	out.append(block.data(), block.size());
}
/**
 * @brief Функция сборки фрейма PING (заголовок + нагрузка дописываются в out)
 *
 * @param out    выходной буфер соединения
 * @param opaque произвольные opaque-данные (8 байт)
 * @param ack    флаг подтверждения получения PING пира
 */
void awh::http::h2::frame::serializePing(string & out, const uint8_t opaque[8], const bool ack) noexcept {
	// Дописываем заголовок фрейма PING
	::wrHeader(out, 8, frame_t::PING, (ack ? flag::ACK : flag::NONE), 0);
	// Дописываем opaque-данные
	out.append(reinterpret_cast <const char *> (opaque), 8);
}
/**
 * @brief Функция сборки фрейма GOAWAY (заголовок + нагрузка дописываются в out)
 *
 * @param out          выходной буфер соединения
 * @param lastStreamId наибольший идентификатор обработанного потока
 * @param code         код ошибки завершения соединения
 * @param debugData    необязательные отладочные данные
 */
void awh::http::h2::frame::serializeGoaway(string & out, const uint32_t lastStreamId, const error_t code, string_view debugData) noexcept {
	// Дописываем заголовок фрейма GOAWAY
	::wrHeader(out, static_cast <uint32_t> (debugData.size() + 8), frame_t::GOAWAY, flag::NONE, 0);
	// Дописываем наибольший идентификатор обработанного потока (сбрасывая reserved-бит)
	::wr32(out, lastStreamId & proto::STREAM_ID_MASK);
	// Дописываем код ошибки завершения соединения
	::wr32(out, static_cast <uint32_t> (code));
	// Дописываем отладочные данные
	out.append(debugData.data(), debugData.size());
}
/**
 * @brief Функция сборки фрейма WINDOW_UPDATE (заголовок + нагрузка дописываются в out)
 *
 * @param out       выходной буфер соединения
 * @param streamId  идентификатор потока (0 - окно всего соединения)
 * @param increment инкремент окна flow control
 */
void awh::http::h2::frame::serializeWindowUpdate(string & out, const uint32_t streamId, const uint32_t increment) noexcept {
	// Дописываем заголовок фрейма WINDOW_UPDATE
	::wrHeader(out, 4, frame_t::WINDOW_UPDATE, flag::NONE, streamId);
	// Дописываем инкремент окна (сбрасывая reserved-бит)
	::wr32(out, increment & proto::STREAM_ID_MASK);
}
