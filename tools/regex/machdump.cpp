/**
 * @file machdump.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп выгрузки порождённого машинного кода для разбора
 *
 * @details Удержавшийся сдвиг времени порождённого кода судить по одному
 *          времени нельзя: пара тактов на пути в два десятка команд - это
 *          и выравнивание, и лишняя команда пролога, и перестановка ветвей.
 *          Щуп выгружает порождённый код текстом для собирателя - по байту
 *          директивой «.byte», - и объектный файл, из него собранный, разбирает
 *          разборщик системы. Два дерева дают два листинга, и различие видно
 *          командой, а не догадкой.
 *
 *          Код берётся из записи сопоставителя: запись несёт его байтами как
 *          есть, следом за байтом набора команд, байтом признаков и тремя
 *          числами переменной длины - размером записи кадра, числом записей
 *          и длиною кода.
 *
 * Сборка и запуск: tools/regex/probe.sh machdump [выражение] > code.s
 *                  clang -c -arch arm64 code.s -o code.o && otool -tvV code.o
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <string>
#include <cstdint>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция чтения числа переменной длины из записи
 *
 * @param data   запись порождённого сопоставителя
 * @param offset позиция чтения записи
 * @param value  прочитанное число
 * @return       результат чтения числа
 *
 */
static bool size(const string & data, size_t & offset, uint64_t & value) noexcept {
	// Выполняем сброс прочитанного числа
	value = 0;
	/**
	 * Выполняем чтение долей числа по семь разрядов
	 */
	for(uint8_t shift = 0; (shift < 64) && (offset < data.size()); shift += 7) {
		// Получаем очередную долю числа
		const uint8_t part = static_cast <uint8_t> (data[offset++]);
		// Выполняем добавление доли к числу
		value |= (static_cast <uint64_t> (part & 0x7F) << shift);
		/**
		 * Если доля последняя
		 */
		if((part & 0x80) == 0)
			// Выводим результат чтения числа
			return true;
	}
	// Выводим отказ чтения числа
	return false;
}

/**
 * @brief Функция запуска щупа
 *
 * @param argc количество доводов
 * @param argv набор доводов
 * @return     результат исполнения щупа
 *
 */
int main(int argc, char ** argv) noexcept {
	// Получаем выгружаемое выражение
	const string pattern = ((argc > 1) ? argv[1] : "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$");
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Создаём собранное выражение
	awh::regex::expression_t expression;
	/**
	 * Если сборка выражения с порождением кода не выполнена
	 */
	if(!engine.build(pattern, static_cast <uint32_t> (awh::regex::flag_t::JIT), expression) || !expression.machine) {
		// Выводим сообщение об отсутствии кода
		::fprintf(stderr, "нет кода для «%s»\n", pattern.c_str());
		// Выходим с кодом отказа
		return 1;
	}
	// Запись порождённого сопоставителя
	string record;
	/**
	 * Если запись сопоставителя не выполнена
	 */
	if(!expression.machine->save(record) || (record.size() < 2)) {
		// Выводим сообщение об отказе записи
		::fprintf(stderr, "запись сопоставителя не выполнена\n");
		// Выходим с кодом отказа
		return 1;
	}
	// Позиция чтения записи за байтами набора команд и признаков
	size_t offset = 2;
	// Размер записи кадра, число записей и длина кода
	uint64_t frame = 0, levels = 0, length = 0;
	/**
	 * Если заголовок записи не прочтён либо код за пределами записи
	 */
	if(!size(record, offset, frame) || !size(record, offset, levels) ||
	 !size(record, offset, length) || ((offset + length) > record.size())) {
		// Выводим сообщение о несообразной записи
		::fprintf(stderr, "запись сопоставителя несообразна\n");
		// Выходим с кодом отказа
		return 1;
	}
	// Выводим заголовок текста для собирателя
	::printf("// «%s»: код %llu байт, кадр %llu байт, записей %llu, признаки: проверка возможности %u, пропуск %u, отбор %u\n", pattern.c_str(),
	 static_cast <unsigned long long> (length), static_cast <unsigned long long> (frame),
	 static_cast <unsigned long long> (levels), static_cast <unsigned> (static_cast <uint8_t> (record[1]) & 1),
	 static_cast <unsigned> ((static_cast <uint8_t> (record[1]) >> 1) & 1), static_cast <unsigned> (static_cast <uint8_t> (record[1]) >> 2));
	::printf(".text\n.globl _matcher\n_matcher:\n");
	/**
	 * Выполняем выгрузку байтов порождённого кода
	 */
	for(uint64_t i = 0; i < length; i++)
		// Выводим очередной байт директивой собирателя
		::printf("%s0x%02x%s", ((i % 16) == 0) ? ".byte " : "",
		 static_cast <unsigned> (static_cast <uint8_t> (record[offset + i])),
		 (((i % 16) == 15) || ((i + 1) == length)) ? "\n" : ",");
	// Выводим результат работы щупа
	return 0;
}
