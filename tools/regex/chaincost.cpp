/**
 * @file chaincost.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп цены прохода цепочки ограниченного повторения
 *
 * @details Повторение ограниченное компилируется цепочкой переходов с выходом
 *          общим, и проход её стоил двух инструкций и точки возврата на каждую
 *          копию. Пометка даёт проход одним ходом и точкой единственной.
 *
 *          Щуп берёт цепочки ДОЛГИЕ: там выигрыш обязан выйти кратным,
 *          а не процентным, и разбросом машины не скрывается. Сличать его
 *          надлежит запуском того же щупа на дереве эталонном - оттого
 *          выражение щупа и оставлено таким, какое соберётся деревом любым.
 *
 * @note Признак «PROBING» при сборке обязателен: щуп выводит обходы наравне
 *       со временем, и без них судить не о чем.
 *
 * Сборка и запуск: tools/regex/probe.sh chaincost
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/probe.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <utility>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция замера одного выражения
 *
 * @param engine  движок сопоставления
 * @param pattern текст регулярного выражения
 * @param text    текст, выражению подставляемый
 * @param rounds  количество обходов цикла исполнения
 * @param steps   количество единиц объёма работы
 * @return        время сопоставления в наносекундах
 *
 */
static double measure(awh::regex::engine_t & engine, const string & pattern,
 const string & text, uint64_t & rounds, uint64_t & steps) noexcept {
	// Создаём собранное выражение
	awh::regex::expression_t expression;
	// Создаём набор границ ячеек захвата текста
	vector <pair <size_t, size_t>> captures;
	/**
	 * Если сборка выражения не выполнена
	 */
	if(!engine.build(pattern, 0, expression)){
		// Выводим отсутствие показаний замера
		rounds = steps = 0;
		// Выводим результат отказа
		return 0.;
	}
	/**
	 * Выполняем прогрев обращением к выражению
	 */
	for(uint16_t i = 0; i < 3; i++)
		// Выполняем сопоставление выражения с текстом
		engine.exec(expression, text, 0, captures);
	// Выполняем сброс счётчиков мер работы
	awh::regex::probe_t::reset();
	// Выполняем сопоставление выражения с текстом
	engine.exec(expression, text, 0, captures);
	// Получаем снятые меры работы сопоставления
	rounds = awh::regex::probe_t::amount(awh::regex::work_t::ROUNDS);
	steps = awh::regex::probe_t::amount(awh::regex::work_t::STEPS);
	// Количество проходов замера
	constexpr uint16_t PASSES = 200;
	// Получаем отметку времени начала замера
	const auto begin = chrono::steady_clock::now();
	/**
	 * Выполняем проходы замера сопоставления
	 */
	for(uint16_t i = 0; i < PASSES; i++)
		// Выполняем сопоставление выражения с текстом
		engine.exec(expression, text, 0, captures);
	// Получаем длительность замера в наносекундах
	const auto spent = chrono::duration_cast <chrono::nanoseconds> (chrono::steady_clock::now() - begin).count();
	// Выводим среднее время одного сопоставления
	return (static_cast <double> (spent) / static_cast <double> (PASSES));
}

/**
 * @brief Функция запуска щупа
 *
 * @return результат исполнения щупа
 *
 */
int main() noexcept {
	/**
	 * Если учёт путей сопоставления при сборке не включён
	 */
	if(!awh::regex::probe_t::enabled()){
		// Выводим сообщение об отсутствии учёта путей
		::printf("Щуп собран без признака PROBING: счётчики молчат\n");
		// Выводим результат отказа
		return 1;
	}
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Длины цепочек, щупом замеряемые
	const size_t LENGTHS[] = {8, 32, 128, 512};
	// Выводим заголовок таблицы щупа
	::printf("%-22s %12s %10s %10s %12s\n", "ВЫРАЖЕНИЕ", "нс", "работы", "обходов", "нс на обход");
	/**
	 * Выполняем перебор длин замеряемых рядов
	 */
	for(const size_t length : LENGTHS){
		// Создаём текст из цифр длиною вдвое больше цепочки
		const string text(length * 2, '7');
		// Создаём выражение ограниченного повторения класса заданной длины
		const string pattern = ("[0-9]{0," + to_string(length) + "}");
		// Количество обходов и единиц работы
		uint64_t rounds = 0, steps = 0;
		// Выполняем замер сопоставления выражения
		const double spent = measure(engine, pattern, text, rounds, steps);
		// Выводим строку итога выражения
		::printf("%-22s %12.0f %10llu %10llu %12.1f\n", pattern.c_str(), spent,
		 static_cast <unsigned long long> (steps), static_cast <unsigned long long> (rounds),
		 (rounds > 0 ? (spent / static_cast <double> (rounds)) : 0.));
	}
	// Выводим результат работы щупа
	return 0;
}
