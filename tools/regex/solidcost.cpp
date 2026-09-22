/**
 * @file solidcost.cpp
 * @date 2026-09-22
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп цены точек возврата ряда, кладущихся напрасно
 *
 * @details Щуп solidgap установил, что проверка компилятора Compiler::futile
 *          продолжение классом не принимает, тогда как проверка порождателя
 *          машинного кода codegen.cpp::sealing принимает. Щуп меряет, во что
 *          расхождение это обходится исполнению с возвратом.
 *
 *          Пара выражений подбирается так, чтобы различались они ровно видом
 *          продолжения - символ против класса, - а наборы байтов тела
 *          и продолжения не пересекались в обоих. Разница показаний
 *          есть цена точек возврата, кладущихся напрасно.
 *
 * @note Признак «PROBING» при сборке обязателен: без счётчика SOLIDING
 *       щуп не отличит выставленный признак от невыставленного.
 *
 * Сборка и запуск: tools/regex/probe.sh solidcost
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
 * @brief Набор пар выражений, щупом замеряемых
 *
 * @details Выражения пары ведут себя одинаково: ряд проходится до конца
 *          текста и упирается в продолжение, телу ряда не подходящее.
 *          Различны они лишь видом продолжения
 *
 */
static const struct {
	// Текст регулярного выражения
	const char * pattern;
	// Пояснение случая
	const char * note;
} CASES[] = {
	{"([a-z]+)@",     "продолжение символом, признак ВЫСТАВЛЕН"},
	{"([a-z]+)[0-9]", "продолжение классом, признак не выставлен"},
	{"([a-z]+)[@#]",  "продолжение классом из двух символов"},
	{"(\\w+)@",       "тело классом, продолжение символом"},
	{"(\\w+)[0-9]",   "тело и продолжение классами, ПЕРЕСЕКАЮТСЯ"}
};

/**
 * @brief Функция замера одного выражения
 *
 * @param engine  движок сопоставления
 * @param pattern текст регулярного выражения
 * @param text    текст, выражению подставляемый
 * @param solids  количество пропусков точек возврата ряда
 * @return        время сопоставления в наносекундах
 *
 */
static double measure(awh::regex::engine_t & engine, const char * pattern,
 const string & text, uint64_t & solids) noexcept {
	// Создаём собранное выражение
	awh::regex::expression_t expression;
	// Создаём набор границ ячеек захвата текста
	vector <pair <size_t, size_t>> captures;
	/**
	 * Если сборка выражения не выполнена
	 */
	if(!engine.build(pattern, 0, expression)){
		// Выводим отсутствие показаний замера
		solids = 0;
		// Выводим результат отказа
		return 0.;
	}
	/**
	 * Выполняем прогрев обращением к выражению
	 */
	for(uint16_t i = 0; i < 3; i++)
		// Выполняем сопоставление выражения с текстом
		engine.exec(expression, text, 0, captures);
	// Выполняем сброс счётчиков путей сопоставления
	awh::regex::probe_t::reset();
	// Выполняем сопоставление выражения с текстом
	engine.exec(expression, text, 0, captures);
	// Получаем количество пропусков точек возврата ряда повторения
	solids = awh::regex::probe_t::count(awh::regex::path_t::SOLIDING);
	// Количество проходов замера
	constexpr uint16_t ROUNDS = 50;
	// Получаем отметку времени начала замера
	const auto begin = chrono::steady_clock::now();
	/**
	 * Выполняем проходы замера сопоставления
	 */
	for(uint16_t i = 0; i < ROUNDS; i++)
		// Выполняем сопоставление выражения с текстом
		engine.exec(expression, text, 0, captures);
	// Получаем длительность замера в наносекундах
	const auto spent = chrono::duration_cast <chrono::nanoseconds> (chrono::steady_clock::now() - begin).count();
	// Выводим среднее время одного сопоставления
	return (static_cast <double> (spent) / static_cast <double> (ROUNDS));
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
	// Длины текстов, щупом замеряемых
	const size_t LENGTHS[] = {256, 1024, 4096};
	/**
	 * Выполняем перебор длин замеряемых текстов
	 */
	for(const size_t length : LENGTHS){
		// Создаём текст из строчных букв с хвостом, ряду не подходящим
		const string text = string(length, 'a') + "!";
		// Выводим заголовок таблицы щупа
		::printf("\nТЕКСТ %zu байт строчных букв да знак, ряду не подходящий\n", length);
		// Выводим шапку таблицы щупа
		::printf("%-16s %12s %10s  %s\n", "ВЫРАЖЕНИЕ", "нс", "SOLIDING", "СЛУЧАЙ");
		/**
		 * Выполняем перебор замеряемых выражений
		 */
		for(const auto & item : CASES){
			// Количество пропусков точек возврата ряда повторения
			uint64_t solids = 0;
			// Выполняем замер сопоставления выражения
			const double spent = measure(engine, item.pattern, text, solids);
			// Выводим строку итога выражения
			::printf("%-16s %12.0f %10llu  %s\n", item.pattern, spent,
			 static_cast <unsigned long long> (solids), item.note);
		}
	}
	// Выводим результат работы щупа
	return 0;
}
