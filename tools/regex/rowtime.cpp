/**
 * @file rowtime.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп скорости разбора на строках набора замеров
 *
 * @details Набор замеров меряет на всякой строке три столбца - наш разбор,
 *          наш машинный код и разбор эталона, - и на Эльбрусе проход его
 *          стоит двух с лишним часов. Сличение двух сборок толкователя
 *          требует лишь первого столбца, и щуп меряет его одного: те же
 *          выражения и тексты, взятые из самого набора, тот же порядок замера -
 *          прогрев, проверка вердикта, лучший из пяти проходов, - и число
 *          совпадений в секунду в той же мере.
 *
 *          Строки выбираются доводами щупа по имени; без доводов меряются все.
 *
 * @note Собирать надлежит БЕЗ учёта: учёт вносит меры работы сложением
 *       атомарным на всяком вызове, и щуп мерил бы тогда и его.
 *
 * Сборка и запуск:
 *   PROBING=нет FLAGS="-I tools/benchmark/syscount" tools/regex/probe.sh rowtime [строка]...
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
 * @brief Функция замера строки набора
 *
 * @param scenario сценарий набора замеров
 * @return         совпадений в секунду либо отрицательное значение при отказе
 *
 */
static double measure(const awh::benchmark::matching::scenario_t & scenario) noexcept {
	// Получаем текст сопоставления сценария
	const string & body = awh::benchmark::matching::text(scenario.kind);
	// Получаем количество повторений сопоставления в проходе
	const size_t rounds = awh::benchmark::matching::rounds(scenario.kind);
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Создаём собранное выражение сценария
	awh::regex::expression_t expression;
	/**
	 * Если выражение сценария не собрано
	 */
	if(!engine.build(scenario.pattern, 0, expression))
		// Выводим признак отказа сборки
		return -1.;
	// Создаём набор границ совпадения
	vector <pair <size_t, size_t>> captures;
	/**
	 * Выполняем прогрев сопоставления
	 */
	for(size_t i = 0; i < awh::benchmark::matching::WARMUP; i++)
		// Выполняем сопоставление прогревочное
		engine.exec(expression, body, 0, captures);
	/**
	 * Если вердикт сопоставления сценарию не отвечает
	 */
	if(engine.exec(expression, body, 0, captures) != scenario.matches)
		// Выводим признак отказа вердикта
		return -2.;
	// Наименьшее время прохода в секундах
	double best = 0.;
	/**
	 * Выполняем проходы замера
	 */
	for(size_t attempt = 0; attempt < awh::benchmark::matching::ATTEMPTS; attempt++) {
		// Получаем отметку времени начала прохода
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем повторения сопоставления прохода
		 */
		for(size_t i = 0; i < rounds; i++)
			// Выполняем сопоставление
			engine.exec(expression, body, 0, captures);
		// Получаем время прохода в секундах
		const double spent = chrono::duration <double> (chrono::steady_clock::now() - begin).count();
		/**
		 * Если проход выполнен быстрее прежних
		 */
		if((best == 0.) || (spent < best))
			// Выполняем установку времени прохода
			best = spent;
	}
	// Выводим число совпадений в секунду
	return ((best > 0.) ? (static_cast <double> (rounds) / best) : -3.);
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
	// Количество строк, отказавших в замере
	uint32_t failed = 0;
	/**
	 * Выполняем перебор сценариев набора
	 */
	for(const auto & scenario : awh::benchmark::matching::SCENARIOS) {
		// Признак выбора сценария доводами щупа
		bool chosen = (count < 2);
		/**
		 * Выполняем перебор доводов щупа
		 */
		for(int i = 1; !chosen && (i < count); i++)
			// Выполняем сличение имени сценария с доводом
			chosen = (::strcmp(values[i], scenario.name) == 0);
		/**
		 * Если сценарий доводами не выбран
		 */
		if(!chosen)
			// Переходим к сценарию следующему
			continue;
		// Выполняем замер сценария
		const double value = ::measure(scenario);
		/**
		 * Если замер сценария не выполнен
		 */
		if(value < 0.) {
			// Выводим сообщение об отказе замера
			::printf("regex %-22s ОТКАЗ %s\n", scenario.name,
			 ((value > -1.5) ? "сборки" : ((value > -2.5) ? "вердикта" : "замера")));
			// Увеличиваем количество отказов
			failed++;
		// Выводим скорость сценария в мере набора
		} else ::printf("regex %-22s %16.2f\n", scenario.name, value);
	}
	// Выводим результат исполнения щупа
	return ((failed > 0) ? 1 : 0);
}
