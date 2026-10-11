/**
 * @file boundedcost.cpp
 * @date 2026-10-10
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп разбора дефицита сценария bounded-short
 *
 * @details Сценарий «bounded-short» в наборе замеров выражение «(?:HT|TP)/1»
 *          на коротком тексте обмена HTTP отстаёт от PCRE2 на Mac arm64
 *          в 2,5 раза: после снятия 14 нс фасадной платы собственный JIT
 *          ещё в 1,4 раза медленнее PCRE2. Щуп разводит три статьи этой
 *          платы, чтобы источник отставания указать прямо:
 *
 *          - путь Pike (по умолчанию) против пути Codegen (режим JIT):
 *            разность есть цена диспетчеризации толкователя;
 *          - путь с префильтром против пути без него (режим NOSTART):
 *            разность есть цена отбора начальных позиций;
 *          - меры работы (обходы, шаги, попытки) между режимами: меры
 *            меняются скачком, и сдвиг их указывает на отличающуюся
 *            программу исполнения, а не на иной расклад текста.
 *
 * Сборка и запуск: tools/regex/probe.sh boundedcost
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/backtrack.hpp>
#include <regex/probe.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения короткого текста сценария
 *
 * @return короткий текст, снятый с набора дословно
 *
 */
static const string & shortText() noexcept {
	// Короткий текст сопоставления
	static const string result(
		"GET /index.html HTTP/1.1\r\nHost: 192.168.001.100\r\n"
		"Content-Length: 4096\r\nUser-Agent: forman@anyks.com\r\n"
	);
	// Выводим короткий текст сопоставления
	return result;
}

/**
 * @brief Функция замера лучшего из кругов
 *
 * @param repeats количество повторений в круге
 * @param body    замеряемое действие
 * @return        лучшее время одного повторения в наносекундах
 *
 */
template <typename Body>
static double fastest(const size_t repeats, Body && body) noexcept {
	// Лучшее время повторения
	double result = 0.;
	/**
	 * Выполняем круги замера
	 *
	 * @details Берётся лучший круг: ядра этой машины двумодальны, и середина
	 *          внутри прогона смещена целиком, а не отдельными выбросами
	 *
	 */
	for(uint32_t round = 0; round < 12; round++){
		// Получаем отметку времени начала круга
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем повторения замеряемого действия
		 */
		for(size_t i = 0; i < repeats; i++)
			// Выполняем замеряемое действие
			body();
		// Получаем время одного повторения в наносекундах
		const double spent = (static_cast <double> (chrono::duration_cast <chrono::nanoseconds> (
		 chrono::steady_clock::now() - begin).count()) / static_cast <double> (repeats));
		/**
		 * Если круг выполнен быстрее прежних
		 */
		if((result == 0.) || (spent < result))
			// Выполняем установку времени круга
			result = spent;
	}
	// Выводим лучшее время повторения
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
	if(!awh::regex::probe_t::enabled()){
		// Выводим сообщение об отсутствии учёта путей
		::printf("Щуп собран без признака PROBING: счётчики молчат\n");
		// Выводим результат отказа
		return 1;
	}
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Создаём набор границ совпадения
	vector <pair <size_t, size_t>> captures;
	/**
	 * @brief Режим сборки выражения щупом замеряемый
	 *
	 */
	struct mode_t {
		// Имя режима сборки
		const char * name;
		// Набор признаков сборки
		uint32_t flags;
	};
	// Режимы сборки, щупом сравниваемые
	const mode_t MODES[] = {
		{"pike",        0},
		{"pikeNostart", static_cast <uint32_t> (awh::regex::flag_t::NOSTART)},
		{"jit",         static_cast <uint32_t> (awh::regex::flag_t::JIT)},
		{"jitNostart",  (static_cast <uint32_t> (awh::regex::flag_t::JIT) |
		 static_cast <uint32_t> (awh::regex::flag_t::NOSTART))},
		{"jitUnchecked", (static_cast <uint32_t> (awh::regex::flag_t::JIT) |
		 static_cast <uint32_t> (awh::regex::flag_t::UNCHECKED))}
	};
	// Получаем текст сценария дословно
	const string & text = shortText();
	// Текст выражения сценария bounded-short
	const char * pattern = "(?:HT|TP)/1";
	// Выводим заголовок таблицы щупа
	::printf("%-12s %-12s %10s %10s %10s %10s %10s\n",
	 "ВЫЗОВ", "РЕЖИМ", "нс", "обходы", "шаги", "попытки", "нс на обход");
	/**
	 * Выполняем перебор режимов сборки
	 */
	for(const auto & mode : MODES){
		// Создаём собранное выражение режима
		awh::regex::expression_t expression;
		/**
		 * Если сборка выражения в режиме не выполнена
		 */
		if(!engine.build(pattern, mode.flags, expression)){
			// Выводим сообщение об отказе сборки
			::printf("%-12s ОТКАЗ СБОРКИ\n", mode.name);
			// Выполняем переход к режиму следующему
			continue;
		}
		/**
		 * Выполняем прогрев обращением к выражению
		 */
		for(uint16_t i = 0; i < 3; i++)
			// Выполняем сопоставление выражения с текстом
			engine.exec(expression, text, 0, captures);
		// Выполняем сброс счётчиков мер работы
		awh::regex::probe_t::reset();
		// Выполняем единственное сопоставление ради снятия мер работы
		engine.exec(expression, text, 0, captures);
		// Получаем снятые меры работы сопоставления
		const uint64_t rounds   = awh::regex::probe_t::amount(awh::regex::work_t::ROUNDS);
		const uint64_t steps    = awh::regex::probe_t::amount(awh::regex::work_t::STEPS);
		const uint64_t attempts = awh::regex::probe_t::amount(awh::regex::work_t::ATTEMPTS);
		// Выполняем замер времени сопоставления режима
		const double spent = ::fastest(200000, [&]() noexcept -> void {
			// Выполняем сопоставление выражения с текстом
			engine.exec(expression, text, 0, captures);
		});
		// Выводим строку итога режима
		::printf("engine     %-12s %10.0f %10llu %10llu %10llu %10.1f\n",
		 mode.name, spent,
		 static_cast <unsigned long long> (rounds),
		 static_cast <unsigned long long> (steps),
		 static_cast <unsigned long long> (attempts),
		 (rounds > 0 ? (spent / static_cast <double> (rounds)) : 0.));
	}
	// Создаём второй движок через фасад regexp_t
	awh::regexp_t regexp;
	/**
	 * Собираем выражение фасадом один раз
	 */
	const auto expression = regexp.build(pattern,
	 static_cast <uint32_t> (awh::regex::flag_t::JIT));
	if(!expression){
		// Выводим сообщение об отказе сборки
		::printf("фасад: ОТКАЗ СБОРКИ\n");
		// Выводим результат работы щупа
		return 1;
	}
	// Проверяем, заполнились ли alts
	::printf("\nalts.size() = %zu, anchor = %d\n",
	 expression->forward.prefilter.alts.size(),
	 static_cast <int> (static_cast <unsigned char> (expression->forward.prefilter.anchor)));
	for(size_t i = 0; i < expression->forward.prefilter.alts.size(); i++)
		::printf("  alts[%zu] = '%s' (len %zu)\n",
		 i, expression->forward.prefilter.alts[i].c_str(),
		 expression->forward.prefilter.alts[i].size());
	// Набор границ, фасадом переиспользуемый
	static vector <pair <size_t, size_t>> bounds;
	/**
	 * Выполняем прогрев фасадом
	 */
	for(uint16_t i = 0; i < 3; i++)
		// Выполняем сопоставление выражения с текстом фасадом
		regexp.match(text, expression, bounds);
	// Выполняем замер фасада
	const double spent = ::fastest(200000, [&]() noexcept -> void {
		// Выполняем сопоставление выражения с текстом фасадом
		regexp.match(text, expression, bounds);
	});
	// Выводим строку итога режима через фасад
	::printf("facade     %-12s %10.0f\n", "jit", spent);
	// Выводим результат работы щупа
	return 0;
}
