/**
 * @file stackscan.cpp
 * @date 2026-09-30
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп зависимости разбора от положения стека
 *
 * @details Стенд сличения меряет всякий проход при четырёх положениях стека
 *          через шестьдесят четыре байта и выводит отношение худшего к лучшему.
 *          Четыре положения покрывают окно в двести пятьдесят шесть байтов, а
 *          медленное положение разбора на ARM64 было одно на страницу в
 *          шестнадцать килобайтов: стенд ловил его, лишь когда среда запуска
 *          приводила окно к нему. Щуп обходит страницу целиком - всякую строку
 *          набора он меряет при каждом положении стека через шестнадцать байтов
 *          и выводит середину по положениям, худшее к середине и медленные
 *          положения адресом по модулю страницы с прибавкой к середине.
 *
 *          Положение задаётся прокладкой в кадре функции прохода: сдвиг кадра
 *          сдвигает кадры всех вызовов из него. Страница берётся у системы;
 *          у Apple на ARM64 она в шестнадцать килобайтов, и обход её вчетверо
 *          дольше. Всякое положение меряется трижды, берётся лучший проход.
 *
 *          Строки выбираются доводами щупа по имени; без доводов меряются все.
 *          Ключ «--jit» собирает выражения с порождением машинного кода, и
 *          обход меряет тогда порождённый код тем же путём потребителя.
 *
 * @note Собирать надлежит БЕЗ учёта: учёт вносит меры работы сложением
 *       атомарным на всяком вызове, и щуп мерил бы тогда и его.
 *
 * Сборка и запуск:
 *   PROBING=нет FLAGS="-I tools/benchmark/syscount" tools/regex/probe.sh stackscan [--jit] [строка]...
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
 * @details Выражения и тексты берутся самим набором, а не выписываются
 *          заново: выписанные руками, они расходились бы с набором молча
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
#include <unistd.h>
#include <algorithm>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Количество проходов на всякое положение стека
 */
static constexpr size_t PASSES = 3;
/**
 * Время прохода на положение стека в наносекундах
 */
static constexpr double BUDGET = 1500000.;
/**
 * Шаг положений стека: стек выровнен по шестнадцати байтам
 */
static constexpr size_t STEP = 16;
/**
 * Прибавка к середине, с какой положение числится медленным
 */
static constexpr double SLOW = 1.10;

/**
 * @brief Приёмник прокладки стека
 *
 * @details Вызов через изменчивый указатель собирателю непрозрачен: прокладку,
 *          какой никто не читает, собиратель сжал бы до нуля, и стек не
 *          сдвинулся бы. Адрес прокладки уходит наружу, и вызов переходом,
 *          снявшим бы кадр с прокладкой прежде прохода, собирателю запрещён.
 *
 */
static void (* volatile sink)(uint8_t *) noexcept = [](uint8_t *) noexcept -> void {};

/**
 * @brief Функция прохода набора повторений при сдвинутом стеке
 *
 * @param shift      величина сдвига стека в октетах
 * @param rounds     количество повторений сопоставления в проходе
 * @param engine     движок сопоставления
 * @param expression собранное выражение сценария
 * @param body       текст сопоставления сценария
 * @param captures   набор границ совпадения
 * @param position   положение прокладки, адрес её начала
 * @return           время одного сопоставления в наносекундах
 *
 */
static AWH_REGEX_NOINLINE double pass(const size_t shift, const size_t rounds, awh::regex::engine_t & engine,
 const awh::regex::expression_t & expression, const string & body, vector <pair <size_t, size_t>> & captures, uintptr_t & position) noexcept {
	// Выполняем размещение прокладки, сдвигающей стек прохода
	uint8_t * pad = static_cast <uint8_t *> (__builtin_alloca(shift + STEP));
	// Выполняем передачу прокладки приёмнику
	::sink(pad);
	// Выполняем установку положения прокладки
	position = reinterpret_cast <uintptr_t> (pad);
	// Получаем отметку времени начала прохода
	const auto begin = chrono::steady_clock::now();
	/**
	 * Выполняем повторения сопоставления прохода
	 */
	for(size_t i = 0; i < rounds; i++)
		// Выполняем сопоставление
		engine.exec(expression, body, 0, captures);
	// Выводим время одного сопоставления в наносекундах
	return (chrono::duration <double, nano> (chrono::steady_clock::now() - begin).count() / static_cast <double> (rounds));
}

/**
 * @brief Функция обхода положений стека строкой набора
 *
 * @param scenario сценарий набора замеров
 * @param page     размер страницы системы в октетах
 * @param flags    режимы сборки выражения
 * @return         результат обхода: ноль либо код отказа
 *
 */
static int measure(const awh::benchmark::matching::scenario_t & scenario, const size_t page, const uint32_t flags) noexcept {
	// Получаем текст сопоставления сценария
	const string & body = awh::benchmark::matching::text(scenario.kind);
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Создаём собранное выражение сценария
	awh::regex::expression_t expression;
	/**
	 * Если выражение сценария не собрано
	 */
	if(!engine.build(scenario.pattern, flags, expression))
		// Выводим признак отказа сборки
		return 1;
	// Создаём набор границ совпадения
	vector <pair <size_t, size_t>> captures;
	/**
	 * Если вердикт сопоставления сценарию не отвечает
	 */
	if(engine.exec(expression, body, 0, captures) != scenario.matches)
		// Выводим признак отказа вердикта
		return 2;
	// Положение прокладки прохода
	uintptr_t position = 0;
	// Количество повторений сопоставления в проходе
	size_t rounds = 1;
	/**
	 * Выполняем прогрев и подбор количества повторений под время прохода
	 */
	while((::pass(0, rounds, engine, expression, body, captures, position) * static_cast <double> (rounds)) < (BUDGET / 4.)) {
		/**
		 * Если количество повторений достигло предела
		 */
		if(rounds >= (static_cast <size_t> (1) << 26))
			// Выходим из подбора
			break;
		// Выполняем удвоение количества повторений
		rounds *= 2;
	}
	// Получаем время одного сопоставления после прогрева
	const double once = ::pass(0, rounds, engine, expression, body, captures, position);
	// Выполняем установку количества повторений под время прохода
	rounds = (((once > 0.) && ((BUDGET / once) > 1.)) ? static_cast <size_t> (BUDGET / once) : 1);
	// Лучшее время при каждом положении стека
	vector <double> times(page / STEP, 0.);
	// Положение прокладки по модулю страницы при каждом сдвиге
	vector <size_t> places(page / STEP, 0);
	/**
	 * Выполняем проходы обхода страницы
	 */
	for(size_t k = 0; k < PASSES; k++) {
		/**
		 * Выполняем обход положений стека на странице
		 */
		for(size_t i = 0; i < times.size(); i++) {
			// Выполняем проход при сдвинутом стеке
			const double spent = ::pass(i * STEP, rounds, engine, expression, body, captures, position);
			// Выполняем установку положения прокладки по модулю страницы
			places[i] = static_cast <size_t> (position % page);
			/**
			 * Если проход выполнен быстрее прежних
			 */
			if((times[i] == 0.) || (spent < times[i]))
				// Выполняем установку лучшего времени положения
				times[i] = spent;
		}
	}
	// Получаем упорядоченное время положений
	vector <double> sorted = times;
	// Выполняем упорядочивание времени положений
	sort(sorted.begin(), sorted.end());
	// Получаем середину по положениям
	const double middle = sorted[sorted.size() / 2];
	// Перечень медленных положений
	string slow;
	// Количество медленных положений
	size_t count = 0;
	/**
	 * Выполняем обход положений стека
	 */
	for(size_t i = 0; i < times.size(); i++) {
		/**
		 * Если положение медленнее середины заметно
		 */
		if(times[i] > (middle * SLOW)) {
			/**
			 * Если перечень медленных положений не переполнен
			 */
			if(++count <= 12) {
				// Буфер записи положения
				char buffer[48];
				// Выполняем запись положения и прибавки к середине
				::snprintf(buffer, sizeof(buffer), " %zu(+%.0f%%)", places[i], ((times[i] / middle) - 1.) * 100.);
				// Выполняем добавление положения в перечень
				slow.append(buffer);
			}
		}
	}
	// Выводим итог обхода строки
	::printf("regex %-22s середина %12.1f нс  худшее ×%.2f  медленных %zu:%s\n",
	 scenario.name, middle, (sorted.back() / middle), count, slow.c_str());
	// Выполняем сброс вывода
	::fflush(stdout);
	// Выводим признак успешного обхода
	return 0;
}

/**
 * @brief Функция запуска щупа
 *
 * @param count  количество доводов щупа
 * @param values доводы щупа - ключ «--jit» и имена строк набора
 * @return       результат исполнения щупа
 *
 */
int main(int count, char ** values) noexcept {
	// Получаем размер страницы системы
	const size_t page = static_cast <size_t> (::getpagesize());
	// Режимы сборки выражений
	uint32_t flags = 0;
	// Количество имён строк среди доводов
	int names = 0;
	/**
	 * Выполняем перебор доводов щупа
	 */
	for(int i = 1; i < count; i++) {
		/**
		 * Если довод требует порождения машинного кода
		 */
		if(::strcmp(values[i], "--jit") == 0)
			// Выполняем установку режима порождения машинного кода
			flags |= static_cast <uint32_t> (awh::regex::flag_t::JIT);
		// Выполняем учёт имени строки
		else names++;
	}
	// Выводим условия обхода
	::printf("страница %zu байт, положений %zu, проходов %zu, прибавка медленного %.0f%%, %s\n", page, (page / STEP), PASSES, (SLOW - 1.) * 100., ((flags != 0) ? "порождённый код" : "разбор"));
	// Количество строк, отказавших в обходе
	uint32_t failed = 0;
	/**
	 * Выполняем перебор сценариев набора
	 */
	for(const auto & scenario : awh::benchmark::matching::SCENARIOS) {
		// Признак выбора сценария доводами щупа
		bool chosen = (names == 0);
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
		// Выполняем обход положений стека строкой
		const int result = ::measure(scenario, page, flags);
		/**
		 * Если обход строки не выполнен
		 */
		if(result != 0) {
			// Выводим сообщение об отказе обхода
			::printf("regex %-22s ОТКАЗ %s\n", scenario.name, ((result == 1) ? "сборки" : "вердикта"));
			// Увеличиваем количество отказов
			failed++;
		}
	}
	// Выводим результат исполнения щупа
	return ((failed > 0) ? 1 : 0);
}
