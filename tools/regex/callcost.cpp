/**
 * @file callcost.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп постоянной платы за вызов сопоставления
 *
 * @details Худшие строки набора замеров - «alternate-short», «region-medium»,
 *          «request-short» - крохотны: восемь-сорок единиц работы и три-пять
 *          наносекунд на единицу там, где единица должна стоить одной. Это
 *          означает, что правит не цикл исполнения, а ПОСТОЯННАЯ плата
 *          за вызов - подготовка состояния, отбор пути, прогрев наборов.
 *
 *          Щуп меряет пол этой платы: сопоставление, работы почти не несущее.
 *          Выражение «a» с текстом «a» даёт две-три единицы работы, и всё
 *          время сверх них есть плата за вызов. Наклон же выводится рядом
 *          выражений с работой растущей: разность их времён, делённая
 *          на разность работ, даёт цену единицы, а свободный член - плату.
 *
 * @note Признак «PROBING» при сборке обязателен: щуп выводит обходы наравне
 *       со временем, и без них судить не о чем.
 *
 * Сборка и запуск: tools/regex/probe.sh seriescost
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
#include <cstring>
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
	// Количество проходов замера и количество попыток
	constexpr uint32_t PASSES = 20000, ATTEMPTS = 8;
	// Наименьшее время прохода набора повторений
	double result = 0.;
	/**
	 * Выполняем попытки замера сопоставления
	 *
	 * @details Берётся ЛУЧШАЯ попытка, а не средняя: так меряет и набор замеров,
	 *          с каким щуп сличается, и так велит двумодальность ядер этой машины -
	 *          середина внутри прогона смещена целиком, а не отдельными выбросами.
	 *          Среднее по единственному проходу давало здесь вдвое завышенное
	 *          время и расходилось с набором замеров на то же самое.
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
	/**
	 * Если учёт путей сопоставления при сборке не включён
	 */
	if(!awh::regex::probe_t::enabled())
		/**
		 * Выводим предупреждение об отсутствии учёта
		 *
		 * @details Щуп отказа здесь не даёт намеренно: учёт сам стоит времени -
		 *          приращение счётчиков на каждой единице работы да свод их
		 *          по завершении сопоставления, - и время, с ним снятое,
		 *          завышено. Сличать надлежит ДВА прогона: с учётом ради чисел
		 *          работы и без учёта ради времени.
		 *
		 */
		::printf("учёт при сборке не заведён: работа и обходы выйдут нулями, время же верно\n");
	/**
	 * @brief Набор имён путей исполнения сопоставления
	 *
	 * @details Перечень сверяется с перечислением при сборке: разойдись они,
	 *          и щуп печатал бы имена, путям не отвечающие
	 *
	 */
	static const char * PATHS[] = {
		"JITTED", "PLAIN", "SEEKING", "CACHING", "PIKING", "TRACKING",
		"BOUNDING", "PRESUMING", "DENYING", "VERIFYING", "SWEEPING",
		"HALTING", "REUSING", "SUBSETTING", "TABULATING", "PROBING",
		"LINING", "SOLIDING", "BARRING", "SLIDING", "CHAINING"
	};
	static_assert(
		(sizeof(PATHS) / sizeof(PATHS[0])) == static_cast <size_t> (awh::regex::path_t::COUNT),
		"перечень имён путей исполнения разошёлся с перечислением «path_t»"
	);
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	/**
	 * @brief Замеряемое выражение с подставляемым текстом
	 *
	 */
	struct probe_t {
		// Текст регулярного выражения
		const char * pattern;
		// Текст, выражению подставляемый
		const char * text;
	};
	// Набор выражений, щупом замеряемых
	const probe_t PROBES[] = {
		{"a",                                "a"},
		{"abc",                              "abc"},
		{"abcdefgh",                         "abcdefgh"},
		{"(a)",                              "a"},
		{"(a)(b)(c)(d)",                     "abcd"},
		{"GET|POST|PUT|DELETE|HEAD|OPTIONS", "GET /x"},
		{"(?:[a-z]+/)+v1",                   "api/v1"},
		{"(?m)^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)\\r?$", "GET /x HTTP/1.1"},
		/**
		 * Ступени захода, разностью времён разделяемые
		 *
		 * @details Выражение «abc» останавливается путём предфильтра и меряет
		 *          пролог движка. Выражение «(q)zzzz» с текстом, литерала
		 *          не несущим, доходит до захода в исполнение с возвратом
		 *          и возвращается отказом ДО единой попытки - им меряется
		 *          подготовка захода. Выражение «(a)» попытку выполняет,
		 *          и разность даёт цену самой попытки.
		 *
		 */
		{"(q)zzzz",                          "aaaaaaaaaaaaaaaa"},
		{"(a)",                              "aaaaaaaaaaaaaaaa"},
		/**
		 * Попытки числом растущим при работе на попытку неизменной
		 *
		 * @details «(a)b» с текстом из одних «a» отказывает на всякой позиции,
		 *          и попыток выходит по числу букв, а работы на попытку - поровну.
		 *          Растёт время ровно с попытками - значит плата сидит в заходе
		 *          в цикл исполнения, а не в подготовке сопоставления.
		 *
		 */
		{"(a)b",                             "aaaa"},
		{"(a)b",                             "aaaaaaaaaaaaaaaa"},
		{"(a)b",                             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
		/**
		 * Те же попытки при предфильтре недействующем
		 *
		 * @details «(.)b» допускает всякий байт началом, отчего набор начальных
		 *          байтов пользы не несёт и отбор позиций не ведётся вовсе.
		 *          Разность с «(a)b» даёт цену отбора, на попытку приходящуюся.
		 *
		 */
		{"(.)[0-9]",                         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
		{"(a)[0-9]",                         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}
	};
	// Выводим заголовок таблицы щупа
	::printf("%-38s %-7s %10s %8s %8s   %s\n", "ВЫРАЖЕНИЕ", "текст", "нс", "работы", "обходов", "пути исполнения");
	/**
	 * Выполняем перебор замеряемых выражений
	 */
	for(const auto & probe : PROBES){
		// Количество обходов и единиц работы
		uint64_t rounds = 0, steps = 0;
		// Выполняем замер сопоставления выражения
		const double spent = measure(engine, probe.pattern, probe.text, rounds, steps);
		// Выводим строку итога выражения
		::printf("%-38s т=%-5zu %10.1f %8llu %8llu   ", probe.pattern, ::strlen(probe.text), spent,
		 static_cast <unsigned long long> (steps), static_cast <unsigned long long> (rounds));
		/**
		 * Выполняем перебор путей исполнения сопоставления
		 */
		for(uint8_t i = 0; i < static_cast <uint8_t> (awh::regex::path_t::COUNT); i++) {
			// Получаем количество прохождений пути исполнения
			const uint64_t count = awh::regex::probe_t::count(static_cast <awh::regex::path_t> (i));
			// Если путь исполнения пройден, выводим его имя со счётом
			if(count > 0) ::printf("%s=%llu ", PATHS[i], static_cast <unsigned long long> (count));
		}
		// Переходим к строке следующей
		::printf("\n");
	}
	/**
	 * Выполняем разделение платы постоянной и цены единицы работы
	 *
	 * @details Одно выражение с текстом растущим даёт ряд точек «работа - время»,
	 *          и прямая по ним проведённая свободным членом даёт плату за вызов,
	 *          а наклоном - цену единицы работы. Прежде числа эти сливались:
	 *          выражение крохотное несёт их вместе, и по одному замеру
	 *          не разделяются они никак.
	 *
	 */
	::printf("\n%-46s %10s %8s %8s   %s\n", "РАЗДЕЛЕНИЕ ПЛАТЫ", "нс", "работы", "обходов", "пути исполнения");
	// Длины текста, щупом замеряемые
	const size_t LENGTHS[] = {1, 4, 16, 64, 256};
	// Точки замера для проведения прямой
	double works[5] = {0.}, times[5] = {0.};
	// Номер замеряемой точки
	size_t point = 0;
	/**
	 * Выполняем перебор длин замеряемого текста
	 */
	for(const size_t length : LENGTHS){
		// Создаём текст заданной длины
		const string text(length, 'a');
		// Количество обходов и единиц работы
		uint64_t rounds = 0, steps = 0;
		// Выполняем замер сопоставления выражения
		const double spent = measure(engine, "(a+)b?", text, rounds, steps);
		// Выполняем сохранение точки замера
		works[point] = static_cast <double> (steps);
		// Выполняем сохранение времени точки замера
		times[point] = spent;
		// Переходим к точке замера следующей
		point++;
		// Выводим строку итога замера
		::printf("(a+)b? на тексте в %-30zu %10.1f %8llu %8llu\n", length, spent,
		 static_cast <unsigned long long> (steps), static_cast <unsigned long long> (rounds));
	}
	/**
	 * Если точек замера довольно для проведения прямой
	 */
	if((point > 1) && (works[point - 1] > works[0])) {
		// Получаем наклон прямой - цену единицы работы
		const double slope = ((times[point - 1] - times[0]) / (works[point - 1] - works[0]));
		// Получаем свободный член прямой - плату за вызов
		const double fixed = (times[0] - (slope * works[0]));
		// Выводим итог разделения платы
		::printf("\nцена единицы работы %.2f нс, ПЛАТА ЗА ВЫЗОВ %.1f нс\n", slope, fixed);
	}
	// Выводим результат работы щупа
	return 0;
}
