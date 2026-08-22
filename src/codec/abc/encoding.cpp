/**
 * @file encoding.cpp
 * @date 2026-08-18
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Файл реализации проволочной укладки бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the wire laying of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/encoding.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция укладки целого числа установленной ширины
 *
 * @param result буфер, куда следует уложить запись
 * @param value  укладываемое значение
 * @param width  ширина записи в октетах
 *
 */
void awh::codec::abc::fixed(vector <uint8_t> & result, const uint64_t value, const uint8_t width) noexcept {
	/**
	 * Выполняем перебор всех октетов записи, от младшего к старшему
	 */
	for(uint8_t i = 0; i < width; i++)
		// Выполняем укладку очередного октета записи
		result.push_back(static_cast <uint8_t> ((value >> (i * 8)) & 0xFF));
}
/**
 * @brief Функция укладки метки вместе с ведомым значением
 *
 * @param result буфер, куда следует уложить запись
 * @param group  крупный вид проволочной записи
 * @param value  укладываемое значение
 *
 */
void awh::codec::abc::put(vector <uint8_t> & result, const group_t group, const uint64_t value) noexcept {
	// Выполняем получение наименьшей подробности, вмещающей значение
	const uint8_t detail = abc::fit(value);
	// Выполняем укладку ведущего октета значения
	result.push_back(abc::tag(group, detail));
	// Ширина записи, ведомой подробностью метки
	uint8_t width = 0;
	// Если подробность ведёт за собой запись
	if(abc::width(detail, width) && (width > 0))
		// Выполняем укладку ведомой записи
		abc::fixed(result, value, width);
}
/**
 * @brief Функция укладки метки с заданной подробностью
 *
 * @param result буфер, куда следует уложить запись
 * @param group  крупный вид проволочной записи
 * @param detail подробность ведущего октета
 *
 */
void awh::codec::abc::mark(vector <uint8_t> & result, const group_t group, const uint8_t detail) noexcept {
	// Выполняем укладку ведущего октета значения
	result.push_back(abc::tag(group, detail));
}
/**
 * @brief Функция укладки целого числа со знаком
 *
 * @param result буфер, куда следует уложить запись
 * @param value  укладываемое значение
 *
 */
void awh::codec::abc::integer(vector <uint8_t> & result, const int64_t value) noexcept {
	// Если число не меньше нуля
	if(value >= 0)
		// Выполняем укладку числа крупным видом целого без знака
		abc::put(result, group_t::UNSIGNED, static_cast <uint64_t> (value));
	// Если число меньше нуля
	else {
		/**
		 * Выполняем укладку дополнения до −1: запись хранит `−1 − value`.
		 * Обращение разрядов даёт ровно это и переполнения не знает даже у `INT64_MIN`,
		 * тогда как прямое отрицание его переполнило бы
		 */
		abc::put(result, group_t::NEGATIVE, ~static_cast <uint64_t> (value));
	}
}
/**
 * @brief Функция обращения записи дополнения до −1 в число со знаком
 *
 * @param value  значение, снятое с записи крупного вида `NEGATIVE`
 * @param result обращённое число со знаком
 * @return       признак представимости числа видом `int64_t`
 *
 */
bool awh::codec::abc::negative(const uint64_t value, int64_t & result) noexcept {
	// Выполняем сброс обращённого числа
	result = 0;
	// Если запись не представима видом целого со знаком
	if(value > static_cast <uint64_t> (numeric_limits <int64_t>::max()))
		// Сообщаем, что число видом `int64_t` не представимо
		return false;
	// Выполняем обращение записи дополнения до −1 в число со знаком
	result = (static_cast <int64_t> (-1) - static_cast <int64_t> (value));
	// Сообщаем, что число обращено
	return true;
}
/**
 * @brief Функция укладки дробного числа одинарной точности
 *
 * @param result буфер, куда следует уложить запись
 * @param value  укладываемое значение
 *
 */
void awh::codec::abc::real(vector <uint8_t> & result, const float value) noexcept {
	// Разрядная запись дробного числа
	uint32_t bits = 0;
	// Выполняем снятие разрядной записи дробного числа
	::memcpy(&bits, &value, sizeof(bits));
	// Выполняем укладку ведущего октета значения
	abc::mark(result, group_t::SINGLE, static_cast <uint8_t> (single_t::FLOAT));
	// Выполняем укладку разрядной записи дробного числа
	abc::fixed(result, static_cast <uint64_t> (bits), 4);
}
/**
 * @brief Функция укладки дробного числа двойной точности
 *
 * @param result буфер, куда следует уложить запись
 * @param value  укладываемое значение
 *
 */
void awh::codec::abc::real(vector <uint8_t> & result, const double value) noexcept {
	// Разрядная запись дробного числа
	uint64_t bits = 0;
	// Выполняем снятие разрядной записи дробного числа
	::memcpy(&bits, &value, sizeof(bits));
	// Выполняем укладку ведущего октета значения
	abc::mark(result, group_t::SINGLE, static_cast <uint8_t> (single_t::DOUBLE));
	// Выполняем укладку разрядной записи дробного числа
	abc::fixed(result, bits, 8);
}
/**
 * @brief Функция снятия целого числа установленной ширины
 *
 * @param buffer буфер поданной записи
 * @param width  ширина записи в октетах
 * @return       снятое значение
 *
 */
uint64_t awh::codec::abc::gather(const uint8_t * buffer, const uint8_t width) noexcept {
	// Собираемое значение
	uint64_t result = 0;
	// Если буфер поданной записи не существует
	if(buffer == nullptr)
		// Выводим собранное значение
		return result;
	/**
	 * Выполняем перебор всех октетов записи, от младшего к старшему
	 */
	for(uint8_t i = 0; i < width; i++)
		// Выполняем сборку значения из очередного октета записи
		result |= (static_cast <uint64_t> (buffer[i]) << (i * 8));
	// Выводим собранное значение
	return result;
}
/**
 * @brief Функция снятия единицы проволочной записи
 *
 * @param buffer буфер поданной записи
 * @param size   размер поданной записи в октетах
 * @param offset смещение, с какого следует снимать единицу
 * @param item   снятая единица проволочной записи
 * @param error  код отказа, если снять единицу не удалось
 * @return       признак успешно снятой единицы
 *
 */
bool awh::codec::abc::take(const uint8_t * buffer, const size_t size, size_t & offset, item_t & item, error_t & error) noexcept {
	// Выполняем сброс кода отказа
	error = error_t::NONE;
	// Выполняем сброс снятой единицы проволочной записи
	item = item_t();
	/**
	 * Если буфер поданной записи не существует, а октеты в ней объявлены.
	 * Пустая же запись отказом не является: у пустого вместилища `data()`
	 * выдаёт ноль, и внутренний отказ здесь означал бы, что дожидаться
	 * первой подачи нельзя вовсе
	 */
	if((buffer == nullptr) && (size > 0)){
		// Выполняем установку кода внутреннего отказа
		error = error_t::INTERNAL;
		// Сообщаем, что единица не снята
		return false;
	}
	// Если ведущего октета в поданной записи нет
	if(offset >= size){
		// Выполняем установку кода отказа обрыва записи
		error = error_t::UNEXPECTED_EOF;
		// Сообщаем, что единица не снята
		return false;
	}
	// Выполняем снятие ведущего октета значения
	const uint8_t tag = buffer[offset];
	// Выполняем снятие крупного вида проволочной записи
	item.group = abc::group(tag);
	// Выполняем снятие подробности ведущего октета
	item.detail = abc::detail(tag);
	/**
	 * Определяем крупный вид проволочной записи
	 */
	switch(static_cast <uint8_t> (item.group)){
		/**
		 * Если значение является одиночным
		 */
		case static_cast <uint8_t> (group_t::SINGLE): {
			/**
			 * Определяем разновидность одиночного значения
			 */
			switch(item.detail){
				// Если разновидность одиночного значения опознана
				case static_cast <uint8_t> (single_t::NUL):
				case static_cast <uint8_t> (single_t::FALSE):
				case static_cast <uint8_t> (single_t::TRUE):
				case static_cast <uint8_t> (single_t::FLOAT):
				case static_cast <uint8_t> (single_t::DOUBLE):
				case static_cast <uint8_t> (single_t::TIME):
				case static_cast <uint8_t> (single_t::UUID):
				case static_cast <uint8_t> (single_t::BREAK): {
					// Выполняем установку разновидности значением единицы
					item.value = static_cast <uint64_t> (item.detail);
					// Выполняем сдвиг смещения на ведущий октет
					offset++;
					// Сообщаем, что единица снята
					return true;
				}
			}
			// Выполняем установку кода отказа отведённой метки
			error = error_t::RESERVED_TAG;
			// Сообщаем, что единица не снята
			return false;
		}
		/**
		 * Если значение является расширением
		 */
		case static_cast <uint8_t> (group_t::EXTEND): {
			/**
			 * Определяем разновидность расширения
			 */
			switch(item.detail){
				// Если разновидность расширения опознана
				case static_cast <uint8_t> (extend_t::BIGNUM):
				case static_cast <uint8_t> (extend_t::DECIMAL):
				case static_cast <uint8_t> (extend_t::CUSTOM):
				case static_cast <uint8_t> (extend_t::SPANNED): {
					// Выполняем установку разновидности значением единицы
					item.value = static_cast <uint64_t> (item.detail);
					// Выполняем сдвиг смещения на ведущий октет
					offset++;
					// Сообщаем, что единица снята
					return true;
				}
			}
			// Выполняем установку кода отказа отведённой метки
			error = error_t::RESERVED_TAG;
			// Сообщаем, что единица не снята
			return false;
		}
	}
	// Если подробность означает неопределённую длину
	if(item.detail == static_cast <uint8_t> (single_t::BREAK)){
		/**
		 * Если крупный вид неопределённой длины не допускает.
		 *
		 * Неопределённую длину несут вместимые, а также строка и двоичные данные:
		 * последние собираются кусками, когда длина их наперёд неизвестна
		 */
		if((item.group != group_t::ARRAY) && (item.group != group_t::MAP) &&
		   (item.group != group_t::STRING) && (item.group != group_t::BLOB)){
			// Выполняем установку кода отказа неопознанной метки
			error = error_t::UNKNOWN_TAG;
			// Сообщаем, что единица не снята
			return false;
		}
		// Выполняем установку признака неопределённой длины вместимого
		item.indefinite = true;
		// Выполняем сдвиг смещения на ведущий октет
		offset++;
		// Сообщаем, что единица снята
		return true;
	}
	// Ширина записи, ведомой подробностью метки
	uint8_t width = 0;
	// Если подробность ведущего октета не опознана
	if(!abc::width(item.detail, width)){
		// Выполняем установку кода отказа отведённой метки
		error = error_t::RESERVED_TAG;
		// Сообщаем, что единица не снята
		return false;
	}
	// Если ведомая запись в поданную не умещается
	if((size - offset - 1) < static_cast <size_t> (width)){
		// Выполняем установку кода отказа обрыва записи
		error = error_t::UNEXPECTED_EOF;
		// Сообщаем, что единица не снята
		return false;
	}
	// Если подробность несёт само значение
	if(width == 0)
		// Выполняем установку подробности значением единицы
		item.value = static_cast <uint64_t> (item.detail);
	// Если подробность ведёт за собой запись
	else item.value = abc::gather(buffer + offset + 1, width);
	// Выполняем сдвиг смещения на ведущий октет вместе с ведомой записью
	offset += (static_cast <size_t> (width) + 1);
	// Сообщаем, что единица снята
	return true;
}
/**
 * @brief Функция проверки строки на соответствие кодировке UTF-8
 *
 * @param buffer   буфер проверяемой строки
 * @param size     размер проверяемой строки в октетах
 * @param position смещение первой негодной последовательности
 * @return         признак соответствия строки кодировке
 *
 */
bool awh::codec::abc::utf8(const uint8_t * buffer, const size_t size, size_t & position) noexcept {
	// Выполняем сброс смещения первой негодной последовательности
	position = 0;
	// Если буфер проверяемой строки не существует
	if((buffer == nullptr) && (size > 0))
		// Сообщаем, что строка кодировке не отвечает
		return false;
	// Смещение разбираемого октета строки
	size_t offset = 0;
	/**
	 * Выполняем перебор всех октетов проверяемой строки
	 */
	while(offset < size){
		// Выполняем снятие ведущего октета кодовой точки
		const uint8_t lead = buffer[offset];
		// Количество продолжающих октетов кодовой точки
		size_t length = 0;
		// Наименьшая кодовая точка, требующая записи такой длины
		uint32_t least = 0;
		// Собираемая кодовая точка
		uint32_t code = 0;
		// Если кодовая точка записана одним октетом
		if(lead < 0x80){
			// Выполняем сдвиг смещения на разобранный октет
			offset++;
			// Продолжаем разбор строки
			continue;
		// Если кодовая точка записана двумя октетами
		} else if((lead & 0xE0) == 0xC0) {
			// Выполняем установку количества продолжающих октетов
			length = 1;
			// Выполняем установку наименьшей кодовой точки такой длины
			least = 0x80;
			// Выполняем снятие старших разрядов кодовой точки
			code = static_cast <uint32_t> (lead & 0x1F);
		// Если кодовая точка записана тремя октетами
		} else if((lead & 0xF0) == 0xE0) {
			// Выполняем установку количества продолжающих октетов
			length = 2;
			// Выполняем установку наименьшей кодовой точки такой длины
			least = 0x800;
			// Выполняем снятие старших разрядов кодовой точки
			code = static_cast <uint32_t> (lead & 0x0F);
		// Если кодовая точка записана четырьмя октетами
		} else if((lead & 0xF8) == 0xF0) {
			// Выполняем установку количества продолжающих октетов
			length = 3;
			// Выполняем установку наименьшей кодовой точки такой длины
			least = 0x10000;
			// Выполняем снятие старших разрядов кодовой точки
			code = static_cast <uint32_t> (lead & 0x07);
		// Если октет ведущим не является
		} else {
			// Выполняем установку смещения негодной последовательности
			position = offset;
			// Сообщаем, что строка кодировке не отвечает
			return false;
		}
		// Если продолжающих октетов в строке недостаёт
		if((size - offset - 1) < length){
			// Выполняем установку смещения негодной последовательности
			position = offset;
			// Сообщаем, что строка кодировке не отвечает
			return false;
		}
		/**
		 * Выполняем перебор всех продолжающих октетов кодовой точки
		 */
		for(size_t i = 1; i <= length; i++){
			// Выполняем снятие продолжающего октета кодовой точки
			const uint8_t next = buffer[offset + i];
			// Если октет продолжающим не является
			if((next & 0xC0) != 0x80){
				// Выполняем установку смещения негодной последовательности
				position = offset;
				// Сообщаем, что строка кодировке не отвечает
				return false;
			}
			// Выполняем сборку кодовой точки из продолжающего октета
			code = ((code << 6) | static_cast <uint32_t> (next & 0x3F));
		}
		// Если запись кодовой точки длиннее необходимого
		if(code < least){
			// Выполняем установку смещения негодной последовательности
			position = offset;
			// Сообщаем, что строка кодировке не отвечает
			return false;
		}
		// Если кодовая точка является суррогатом
		if((code >= 0xD800) && (code <= 0xDFFF)){
			// Выполняем установку смещения негодной последовательности
			position = offset;
			// Сообщаем, что строка кодировке не отвечает
			return false;
		}
		// Если кодовая точка выходит за предел Юникода
		if(code > 0x10FFFF){
			// Выполняем установку смещения негодной последовательности
			position = offset;
			// Сообщаем, что строка кодировке не отвечает
			return false;
		}
		// Выполняем сдвиг смещения на разобранную кодовую точку
		offset += (length + 1);
	}
	// Сообщаем, что строка отвечает кодировке
	return true;
}
