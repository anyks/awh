/**
 * @file machcost.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп времени порождённого машинного кода на выражениях набора замеров
 *
 * @details Колонка машинного кода в наборе замеров зовёт порождённый код прямо,
 *          минуя движок, и щуп зовёт его так же - на тех же выражениях и тех же
 *          текстах, - отчего числа его с колонкой сличимы. Полный прогон шумен:
 *          машина за час дрейфует сильнее, чем разнятся сличаемые правки. Щуп
 *          меряет одну строку лучшим из многих попыток и годен разбирать строку,
 *          удержавшую сдвиг, - прежде чем судить правку по ней.
 *
 *          Сличать надлежит запуском щупа на двух деревьях: щуп сам ничего
 *          не сличает, а лишь печатает время.
 *
 * @note Признак «JIT» при сборке обязателен: порождение машинного кода
 *       по умолчанию снято, и без него щуп выводит «нет кода».
 *
 * Сборка и запуск: tools/regex/probe.sh machcost
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
 * @brief Короткий текст набора замеров
 *
 * @details Снят дословно с набора: обмен по протоколу HTTP, где выражения
 *          «address-*» и «digits-*» ищут свои совпадения.
 *
 */
static const string SHORT = (
	"GET /index.html HTTP/1.1\r\nHost: 192.168.001.100\r\n"
	"Content-Length: 4096\r\nUser-Agent: forman@anyks.com\r\n"
);

/**
 * @brief Функция замера сопоставления порождённым машинным кодом
 *
 * @param pattern текст регулярного выражения
 * @param text    текст, выражению подставляемый
 * @param bytes   размер порождённого машинного кода в байтах
 * @return        время сопоставления в наносекундах, нуль при отсутствии кода
 *
 */
static double measure(const string & pattern, const string & text, size_t & bytes) noexcept {
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
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
	constexpr uint32_t PASSES = 50000, ATTEMPTS = 10;
	// Наименьшее время прохода набора повторений
	double result = 0.;
	/**
	 * Выполняем попытки замера сопоставления
	 */
	for(uint32_t attempt = 0; attempt < ATTEMPTS; attempt++) {
		// Получаем отметку времени начала попытки
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем проходы замера сопоставления порождённым кодом напрямую
		 */
		for(uint32_t i = 0; i < PASSES; i++)
			// Выполняем сопоставление порождённым машинным кодом
			expression.machine->exec(text, 0, captures);
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
	/**
	 * @brief Замеряемое выражение набора
	 *
	 */
	struct probe_t {
		// Имя строки набора замеров
		const char * name;
		// Текст регулярного выражения
		const char * pattern;
	};
	// Набор выражений, щупом замеряемых
	const probe_t PROBES[] = {
		{"address-absent", "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$"},
		{"address-short",  "(?m)^Host: (\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\r?$"},
		{"digits-short",   "[0-9]{3,5}"}
	};
	// Выводим заголовок таблицы щупа
	::printf("%-16s %10s %10s\n", "СТРОКА", "байтов", "нс");
	/**
	 * Выполняем перебор замеряемых выражений
	 */
	for(const auto & probe : PROBES) {
		// Размер порождённого машинного кода
		size_t bytes = 0;
		// Выполняем замер сопоставления выражения
		const double spent = measure(probe.pattern, SHORT, bytes);
		/**
		 * Если машинный код для выражения не порождён
		 */
		if(bytes == 0)
			// Выводим отсутствие машинного кода
			::printf("%-16s %10s\n", probe.name, "нет кода");
		// Выводим строку итога выражения
		else ::printf("%-16s %10zu %10.2f\n", probe.name, bytes, spent);
	}
	// Выводим результат работы щупа
	return 0;
}
