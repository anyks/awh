/**
 * @file codepath.cpp
 * @date 2026-09-24
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп пути порождённого кода на строках набора замеров
 *
 * @details Колонка кода в наборе замеров зовёт «codegen_t::exec» напрямую,
 *          минуя «Engine::exec», и о пути потребителя ничего не говорит:
 *          потребитель собирает выражение с признаком «JIT» и зовёт «exec».
 *          Щуп меряет на строках набора три столбца рядом - разбор программы,
 *          путь потребителя и прямой вызов порождённого кода - тем же
 *          порядком, что и набор: прогрев, проверка вердикта, лучший из пяти
 *          проходов, - и выводит способ отбора позиций, порождением избранный.
 *
 *          Заведён щуп на Эльбрусе: выигрыш кода над разбором там вдвое меньше,
 *          чем на ARM64, а на строках длинного текста проваливается и вовсе -
 *          «digits-long» и «region-fixed-long» идут кодом медленнее разбора.
 *          Способ отбора позиций и говорит, чей это ход по тексту.
 *
 *          Строки выбираются доводами щупа по имени; без доводов меряются все.
 *
 * @note Собирать надлежит БЕЗ учёта: учёт вносит меры работы сложением
 *       атомарным на всяком вызове, и щуп мерил бы тогда и его.
 *
 * Сборка и запуск:
 *   PROBING=нет FLAGS="-I tools/benchmark/syscount" tools/regex/probe.sh codepath [строка]...
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>

/**
 * Подключаем набор сценариев замеров
 *
 * @details Выражения, тексты и количества повторений берутся самим набором,
 *          а не выписываются заново: выписанные руками, они расходились бы
 *          с набором молча
 *
 */
#include "../../benchmark/regex/matching/matching.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <utility>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция замера лучшего из проходов набора
 *
 * @param rounds количество повторений прохода
 * @param body   замеряемое действие
 * @return       совпадений в секунду либо нуль при неверном замере
 *
 */
template <typename Body>
static double fastest(const size_t rounds, Body && body) noexcept {
	double best = 0.;
	/**
	 * Выполняем проходы замера
	 */
	for(size_t attempt = 0; attempt < awh::benchmark::matching::ATTEMPTS; attempt++) {
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем повторения сопоставления прохода
		 */
		for(size_t i = 0; i < rounds; i++)
			body();
		const double spent = chrono::duration <double> (chrono::steady_clock::now() - begin).count();
		/**
		 * Если проход выполнен быстрее прежних
		 */
		if((best == 0.) || (spent < best))
			best = spent;
	}
	return ((best > 0.) ? (static_cast <double> (rounds) / best) : 0.);
}

/**
 * @brief Функция замера сопоставления открытым договором движка
 *
 * @param scenario сценарий набора замеров
 * @param jit      признак сборки выражения с порождением машинного кода
 * @return         совпадений в секунду, нуль при отсутствии кода либо
 *                 отрицательное значение при отказе
 *
 */
static double contract(const awh::benchmark::matching::scenario_t & scenario, const bool jit) noexcept {
	const string & body = awh::benchmark::matching::text(scenario.kind);
	const size_t rounds = awh::benchmark::matching::rounds(scenario.kind);
	awh::regex::engine_t engine;
	awh::regex::expression_t expression;
	/**
	 * Если выражение сценария не собрано
	 */
	if(!engine.build(scenario.pattern, (jit ? static_cast <uint32_t> (awh::regex::flag_t::JIT) : 0), expression))
		return -1.;
	/**
	 * Если путь потребителя машинного кода не получил
	 *
	 * @details Путь такой исполняет программу разбором, и столбец его
	 *          повторял бы первый, выдавая себя за путь порождённого кода
	 *
	 */
	if(jit && !expression.machine)
		return 0.;
	vector <pair <size_t, size_t>> captures;
	/**
	 * Выполняем прогрев сопоставления
	 */
	for(size_t i = 0; i < awh::benchmark::matching::WARMUP; i++)
		engine.exec(expression, body, 0, captures);
	/**
	 * Если вердикт сопоставления сценарию не отвечает
	 */
	if(engine.exec(expression, body, 0, captures) != scenario.matches)
		return -2.;
	return fastest(rounds, [&engine, &expression, &body, &captures]() noexcept {
		engine.exec(expression, body, 0, captures);
	});
}

/**
 * @brief Функция замера прямого вызова порождённого кода
 *
 * @details Замер повторяет колонку кода набора дословно: выражение собирается
 *          без признака «JIT», а код порождается отдельным объектом из прямой
 *          программы выражения.
 *
 * @param scenario сценарий набора замеров
 * @param filter   способ отбора позиций, порождением избранный
 * @return         совпадений в секунду, нуль при отсутствии кода либо
 *                 отрицательное значение при отказе
 *
 */
static double direct(const awh::benchmark::matching::scenario_t & scenario, const char * & filter) noexcept {
	const string & body = awh::benchmark::matching::text(scenario.kind);
	const size_t rounds = awh::benchmark::matching::rounds(scenario.kind);
	awh::regex::engine_t engine;
	awh::regex::expression_t expression;
	/**
	 * Если выражение сценария не собрано
	 */
	if(!engine.build(scenario.pattern, 0, expression))
		return -1.;
	awh::regex::codegen_t codegen;
	/**
	 * Если выражение порождения машинного кода не получает
	 *
	 * @details Выражение литеральное движок исполняет прямым поиском
	 *          последовательности и порождённого кода ему не отдаёт вовсе
	 *
	 */
	if(expression.forward.plain || !codegen.compile(expression.forward))
		return 0.;
	/**
	 * Определяем способ отбора позиций, порождением избранный
	 */
	switch(static_cast <uint8_t> (codegen.filter())) {
		case static_cast <uint8_t> (awh::regex::filter_t::SEEK): filter = "SEEK"; break;
		case static_cast <uint8_t> (awh::regex::filter_t::LINING): filter = "LINING"; break;
		case static_cast <uint8_t> (awh::regex::filter_t::SIFTING): filter = "SIFTING"; break;
		case static_cast <uint8_t> (awh::regex::filter_t::NARROWING): filter = "NARROWING"; break;
		default: filter = "NONE";
	}
	vector <pair <size_t, size_t>> bounds;
	/**
	 * Выполняем прогрев сопоставления
	 */
	for(size_t i = 0; i < awh::benchmark::matching::WARMUP; i++)
		codegen.exec(body, 0, bounds);
	/**
	 * Если вердикт порождённого кода сценарию не отвечает
	 */
	if(codegen.exec(body, 0, bounds) != scenario.matches)
		return -2.;
	return fastest(rounds, [&codegen, &body, &bounds]() noexcept {
		codegen.exec(body, 0, bounds);
	});
}

/**
 * @brief Функция запуска щупа
 *
 * @param count  количество доводов щупа
 * @param values доводы щупа - имена строк набора
 * @return       результат исполнения щупа
 *
 */
int main(int count, char ** values) noexcept {
	uint32_t failed = 0;
	::printf("%-24s %-10s %14s %14s %14s %10s %10s\n", "СТРОКА", "ОТБОР", "разбор", "потребитель", "напрямую", "потр/разб", "напр/разб");
	/**
	 * Выполняем перебор сценариев набора
	 */
	for(const auto & scenario : awh::benchmark::matching::SCENARIOS) {
		bool chosen = (count < 2);
		/**
		 * Выполняем перебор доводов щупа
		 */
		for(int i = 1; !chosen && (i < count); i++)
			chosen = (::strcmp(values[i], scenario.name) == 0);
		/**
		 * Если сценарий доводами не выбран
		 */
		if(!chosen)
			continue;
		const char * filter = "—";
		const double interpreted = ::contract(scenario, false);
		const double consumer = ::contract(scenario, true);
		const double machine = ::direct(scenario, filter);
		/**
		 * Если замер строки отказал хотя бы одним столбцом
		 *
		 * @details Нуль отказом не считается: он означает, что выражение
		 *          кода не получает, и столбец остаётся пустым
		 *
		 */
		if((interpreted <= 0.) || (consumer < 0.) || (machine < 0.)) {
			::printf("regex %-18s ОТКАЗ: разбор %.0f, потребитель %.0f, напрямую %.0f\n",
			 scenario.name, interpreted, consumer, machine);
			failed++;
			continue;
		}
		::printf("regex %-18s %-10s %14.2f %14.2f %14.2f %10.3f %10.3f\n", scenario.name, filter,
		 interpreted, consumer, machine, (consumer / interpreted), (machine / interpreted));
	}
	return ((failed > 0) ? 1 : 0);
}
