/**
 * @file barrier.cpp
 * @date 2026-09-22
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп отказа по набору начальных байтов у выражения, привязанного к позиции поиска
 *
 * @details Набор замеров эту работу НЕ меряет, и щуп заведён именно оттого.
 *          Колонка порождённого кода в наборе снимается прямым обращением
 *          к «codegen_t::exec», минуя «Engine::exec», а отказ по одному байту
 *          живёт как раз в «Engine::exec» - выше выбора пути исполнения. Правка,
 *          отказ этот порождённому коду отдающая, в наборе замеров не видна
 *          по устройству самого набора, и судить о ней приходится здесь.
 *
 *          Щуп меряет открытый договор целиком: сборку выражения штатным
 *          движком и сопоставление через «exec». Признак «JIT» при сборке
 *          обязателен: порождение машинного кода по умолчанию снято, и щуп
 *          без него мерил бы разбор программы, о правке ничего не говорящий.
 *
 *          Учёт путей исполнения показывает, каким путём отказ дан: «JITTED» -
 *          порождённым кодом, «BARRING» - отказом по одному байту.
 *
 * @warning Числа снимать надлежит со сборки в режиме выпуска на СВОБОДНОЙ
 *          машине: сборка отладочная замедляет всё равномерно, а чужая
 *          нагрузка гуляет показателем вдвое.
 *
 * Сборка и запуск: tools/regex/probe.sh barrier
 *
 * @copyright Copyright © 2026
 *
 */

#include <regex/regex.hpp>
#include <regex/probe.hpp>

#include <cstdio>
#include <string>
#include <vector>
#include <chrono>

using namespace std;

/**
 * @brief Функция замера лучшего из кругов
 */
template <typename Body>
static double fastest(const size_t rounds, const size_t repeats, Body && body) noexcept {
	// Лучшее время круга
	double result = 0.0;
	// Выполняем круги замера
	for(size_t round = 0; round < rounds; round++) {
		// Получаем время начала круга
		const auto begin = chrono::steady_clock::now();
		// Выполняем повторения замеряемого действия
		for(size_t i = 0; i < repeats; i++)
			body();
		// Получаем время завершения круга
		const auto finish = chrono::steady_clock::now();
		// Получаем время одного повторения в наносекундах
		const double value = (static_cast <double> (chrono::duration_cast <chrono::nanoseconds> (finish - begin).count()) / static_cast <double> (repeats));
		// Выполняем отбор лучшего круга
		if((result == 0.0) || (value < result))
			result = value;
	}
	// Выводим лучшее время повторения
	return result;
}

/**
 * @brief Набор разбираемых сценариев щупа
 */
static const struct {
	// Имя сценария
	const char * name;
	// Текст регулярного выражения
	const char * pattern;
	// Текст сопоставления
	const char * text;
	// Ожидаемый вердикт сопоставления
	bool matches;
} SCENARIOS[] = {
	{"address-absent",  "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", "GET /index.html HTTP/1.1", false},
	{"address-present", "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", "192.168.001.100",          true},
	{"request-absent",  "^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)$",              "Host: anyks.com",          false},
	{"request-present", "^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)$",              "GET /index.html HTTP/1.1", true},
	{"anchored-absent", "^[A-Za-z0-9-]+: .+$",                                 "GET /index.html HTTP/1.1", false}
};

/**
 * @brief Функция запуска щупа
 */
int main() noexcept {
	/**
	 * Если сборка щупа учёта путей исполнения не несёт
	 *
	 * @details Отказ здесь намеренный: без учёта щуп выводил бы нули, отказа
	 *          не давая, и путь исполнения остался бы неустановленным
	 *
	 */
	if(!awh::regex::probe_t::enabled()) {
		// Выводим сообщение об отказе
		::printf("щуп собран без признака «AWH_REGEX_PROBING», путь исполнения неустановим\n");
		// Выводим результат отказа
		return 1;
	}
	// Выводим заголовок таблицы щупа
	::printf("%-18s %10s %10s %10s %8s\n", "СЦЕНАРИЙ", "нс", "JITTED", "BARRING", "ВЕРДИКТ");
	/**
	 * Выполняем перебор разбираемых сценариев
	 */
	for(const auto & scenario : SCENARIOS) {
		// Создаём движок сопоставления
		awh::regex::engine_t engine;
		// Создаём собранное выражение
		awh::regex::expression_t expression;
		/**
		 * Если сборка выражения не выполнена
		 */
		if(!engine.build(scenario.pattern, static_cast <uint32_t> (awh::regex::flag_t::JIT), expression)) {
			// Выводим сообщение об отказе сборки
			::printf("%-18s ОТКАЗ СБОРКИ\n", scenario.name);
			// Выводим результат отказа
			return 1;
		}
		/**
		 * Если выражение порождения машинного кода не получило
		 *
		 * @details Щуп меряет именно путь порождённого кода: выражение без него
		 *          мерило бы работу иную, о правке ничего не говорящую
		 *
		 */
		if(!expression.machine) {
			// Выводим сообщение об отсутствии порождённого кода
			::printf("%-18s ПОРОЖДЕНИЯ КОДА НЕТ\n", scenario.name);
			// Выполняем переход к сценарию следующему
			continue;
		}
		// Получаем текст сопоставления
		const string text = scenario.text;
		// Набор границ обнаруженного совпадения
		vector <pair <size_t, size_t>> captures;
		/**
		 * Если вердикт сопоставления ожидаемому не отвечает
		 */
		if(engine.exec(expression, text, 0, captures) != scenario.matches) {
			// Выводим сообщение о расхождении вердикта
			::printf("%-18s ВЕРДИКТ РАЗОШЁЛСЯ\n", scenario.name);
			// Выводим результат отказа
			return 1;
		}
		// Выполняем сброс счётчиков путей исполнения
		awh::regex::probe_t::reset();
		// Выполняем сопоставление единожды под снятие счётчиков путей
		engine.exec(expression, text, 0, captures);
		// Получаем количество сопоставлений порождённым машинным кодом
		const uint64_t jitted = awh::regex::probe_t::count(awh::regex::path_t::JITTED);
		// Получаем количество отказов по набору начальных байтов
		const uint64_t barring = awh::regex::probe_t::count(awh::regex::path_t::BARRING);
		/**
		 * Выполняем замер пропускной способности сопоставления
		 *
		 * @details Учёт путей исполнения на время замера остаётся включённым:
		 *          он одинаков у обеих сличаемых сборок щупа и разницы между
		 *          ними не создаёт, тогда как его отключение потребовало бы
		 *          сборки второй, от измеряемой отличной
		 *
		 */
		const double value = fastest(7, 200000, [&engine, &expression, &text, &captures]() noexcept {
			// Выполняем сопоставление выражения с текстом
			engine.exec(expression, text, 0, captures);
		});
		// Выводим строку итога сценария
		::printf(
			"%-18s %10.1f %10llu %10llu %8s\n", scenario.name, value,
			static_cast <unsigned long long> (jitted),
			static_cast <unsigned long long> (barring),
			(scenario.matches ? "совпало" : "отказ")
		);
	}
	// Выводим результат работы щупа
	return 0;
}
