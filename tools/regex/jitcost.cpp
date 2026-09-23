/**
 * @file jitcost.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп цены копии счётного повторения в порождённом машинном коде
 *
 * @details Кодогенератор пометок ряда и цепочки не читает: всякую копию
 *          счётного повторения он порождает отдельным куском - сверкой
 *          с концом текста, чтением байта, обращением к таблице, переходом
 *          да продвижением. Щуп меряет цену копии двумя мерами разом:
 *          байтами кода, от машины не зависящими, и временем сопоставления.
 *
 *          Ряд обязательный и цепочка необязательная меряются порознь:
 *          «[0-9]{N}» несёт одни копии ряда, «[0-9]{0,N}» - одни звенья
 *          цепочки. Разность соседних строк, делённая на разность N,
 *          и даёт цену копии.
 *
 * @note Признак «JIT» при сборке обязателен: порождение машинного кода
 *       по умолчанию снято, и без него щуп выводит «нет кода».
 *
 * Сборка и запуск: tools/regex/probe.sh jitcost
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
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция замера сопоставления порождённым машинным кодом
 *
 * @param engine  движок сопоставления
 * @param pattern текст регулярного выражения
 * @param text    текст, выражению подставляемый
 * @param bytes   размер порождённого машинного кода в байтах
 * @return        время сопоставления в наносекундах, нуль при отсутствии кода
 *
 */
static double measure(awh::regex::engine_t & engine, const string & pattern, const string & text, size_t & bytes) noexcept {
	// Создаём собранное выражение
	awh::regex::expression_t expression;
	// Создаём набор границ ячеек захвата текста
	vector <pair <size_t, size_t>> captures;
	// Выполняем сброс размера порождённого кода
	bytes = 0;
	/**
	 * Если сборка выражения с порождением кода не выполнена
	 */
	if(!engine.build(pattern, static_cast <uint32_t> (awh::regex::flag_t::JIT), expression) || !expression.machine)
		// Выводим отсутствие показаний замера
		return 0.;
	// Выполняем получение размера порождённого машинного кода
	bytes = expression.machine->length();
	// Количество проходов замера и количество попыток
	constexpr uint32_t PASSES = 20000, ATTEMPTS = 8;
	// Наименьшее время прохода набора повторений
	double result = 0.;
	/**
	 * Выполняем попытки замера сопоставления
	 *
	 * @details Берётся лучшая попытка: ядра этой машины двумодальны, и середина
	 *          внутри прогона смещена целиком, а не отдельными выбросами.
	 *
	 */
	for(uint32_t attempt = 0; attempt < ATTEMPTS; attempt++) {
		// Получаем отметку времени начала попытки
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем проходы замера сопоставления
		 */
		for(uint32_t i = 0; i < PASSES; i++)
			// Выполняем сопоставление выражения с текстом
			engine.exec(expression, text, 0, captures);
		// Получаем длительность попытки в наносекундах
		const double spent = static_cast <double> (chrono::duration_cast <chrono::nanoseconds> (
		 chrono::steady_clock::now() - begin).count()) / static_cast <double> (PASSES);
		/**
		 * Если попытка выполнена быстрее прежних
		 */
		if((result == 0.) || (spent < result))
			// Выполняем установку времени попытки
			result = spent;
	}
	// Выводим наименьшее время одного сопоставления
	return result;
}

/**
 * @brief Функция запуска щупа
 *
 * @return результат исполнения щупа
 *
 */
int main() noexcept {
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Количества копий, щупом замеряемые
	const size_t COUNTS[] = {1, 2, 4, 8, 16};
	// Выводим заголовок таблицы щупа
	::printf("%-16s %10s %10s\n", "ВЫРАЖЕНИЕ", "байтов", "нс");
	/**
	 * Выполняем перебор видов повторения: ряда обязательного и цепочки
	 */
	for(const bool optional : {false, true}) {
		/**
		 * Выполняем перебор количеств копий
		 */
		for(const size_t count : COUNTS) {
			// Создаём выражение счётного повторения заданного вида
			const string pattern = (optional ?
			 ("[0-9]{0," + to_string(count) + "}x") : ("[0-9]{" + to_string(count) + "}x"));
			// Создаём текст из цифр по числу копий с завершающим литералом
			const string text = (string(count, '7') + "x");
			// Размер порождённого машинного кода
			size_t bytes = 0;
			// Выполняем замер сопоставления выражения
			const double spent = measure(engine, pattern, text, bytes);
			/**
			 * Если машинный код для выражения не порождён
			 */
			if(bytes == 0)
				// Выводим отсутствие машинного кода
				::printf("%-16s %10s\n", pattern.c_str(), "нет кода");
			// Выводим строку итога выражения
			else ::printf("%-16s %10zu %10.1f\n", pattern.c_str(), bytes, spent);
		}
	}
	// Выводим результат работы щупа
	return 0;
}
