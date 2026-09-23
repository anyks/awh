/**
 * @file lazyhunt.cpp
 * @date 2026-09-21
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Щуп разыскания по сценариям, эталону уступающим — путь исполнения
 *        и цена сопоставления у каждого
 *
 * @details Щуп берёт сценарии, где доля разбора к эталону ниже порога,
 *          и отвечает на два вопроса: каким путём исполнения идёт каждый
 *          и во что ему обходится сопоставление. Пути снимаются учётом
 *          «AWH_REGEX_PROBING», отчего признак этот в сборке обязателен:
 *          без него перечень путей выходит пустым.
 *
 *          Имя сценария, доводом переданное, включает прогон долгий —
 *          десять секунд одного сопоставления подряд, — под снятие образцов
 *          стека сторонним средством:
 *          @code
 *          ./lazyhunt lazy-short & sample $! 5 1 -file /tmp/lazy.txt
 *          @endcode
 *
 *          Тексты сопоставления и выражения повторяют набор замеров
 *          «benchmark/regex/matching» дословно: щуп, от набора отступивший,
 *          отвечал бы за другое
 *
 *          Щуп собирается и запускается стендом своим - признак
 *          «AWH_REGEX_PROBING» и состав исходных текстов ведутся там:
 *          @code
 *          sh tools/regex/probe.sh lazyhunt [имя сценария]
 *          @endcode
 *
 * @warning Числа снимать надлежит со сборки в режиме выпуска на СВОБОДНОЙ
 *          машине: сборка отладочная замедляет всё равномерно и изображает
 *          точечную просадку там, где её нет, а чужая нагрузка гуляет
 *          показателем вдвое. Правку, выбор пути исполнения меняющую,
 *          судить надлежит полным набором замеров, а не щупом этим.
 *
 * @copyright Copyright © 2026
 *
 */

#include <regex/regex.hpp>
#include <regex/probe.hpp>

#include <cstdio>
#include <cstring>
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
 * @brief Функция получения короткого текста сопоставления
 */
static const string & shortText() noexcept {
	// Короткий текст сопоставления, снятый с обычного обмена по протоколу HTTP
	static const string result(
		"GET /index.html HTTP/1.1\r\nHost: 192.168.001.100\r\n"
		"Content-Length: 4096\r\nUser-Agent: forman@anyks.com\r\n"
	);
	// Выводим короткий текст сопоставления
	return result;
}

/**
 * @brief Функция получения длинного текста сопоставления
 */
static const string & longText() noexcept {
	// Длинный текст сопоставления
	static const string result = []() noexcept -> string {
		// Создаём основу длинного текста сопоставления
		string outcome;
		// Выполняем размещение длинного текста сопоставления
		outcome.reserve(262144 + 128);
		// Выполняем наполнение текста до заданной длины
		while(outcome.size() < 262144)
			outcome.append("the quick brown fox jumps over the lazy dog 1234 ");
		// Выполняем добавление искомых последовательностей у конца текста
		outcome.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
		// Выводим длинный текст сопоставления
		return outcome;
	}();
	// Выводим длинный текст сопоставления
	return result;
}

/**
 * @brief Функция получения текста сопоставления среднего размера
 */
static const string & mediumText() noexcept {
	// Текст сопоставления среднего размера
	static const string result = []() noexcept -> string {
		// Создаём основу текста сопоставления
		string outcome;
		// Выполняем размещение текста сопоставления
		outcome.reserve(2048 + 128);
		// Номер порождаемого обмена по протоколу
		size_t number = 0;
		// Выполняем наполнение текста до заданной длины
		while(outcome.size() < 2048) {
			// Выполняем добавление строки запроса обмена по протоколу
			outcome.append("GET /api/v1/items/");
			// Выполняем добавление номера запрашиваемого ресурса
			outcome.append(std::to_string(number++));
			// Выполняем добавление заголовков обмена по протоколу
			outcome.append(" HTTP/1.1\r\nHost: node-07.anyks.com\r\n"
			 "User-Agent: awh/5.0\r\nAccept: application/json\r\n"
			 "X-Request-Id: 7f3a9c2e-41bd-4e88-9a1f-0c5d6e8b2a34\r\n"
			 "Content-Length: 512\r\n\r\n");
		}
		// Выполняем добавление искомых последовательностей у конца текста
		outcome.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
		// Выводим текст сопоставления среднего размера
		return outcome;
	}();
	// Выводим текст сопоставления среднего размера
	return result;
}

/**
 * @brief Функция получения текста сопоставления исполнения с возвратом
 */
static const string & heavyText() noexcept {
	// Текст сопоставления исполнения с возвратом
	static const string result(
		"lorem ipsum dolor sit amet consectetur adipiscing elit sed do "
		"eiusmod tempor incididunt ut labore et dolore magna aliqua (nested "
		"(parenthesis (here)) done) repeat repeat forman@anyks.com tail"
	);
	// Выводим текст сопоставления исполнения с возвратом
	return result;
}

/**
 * @brief Точка входа щупа
 */
int main(int argc, char ** argv) {
	// Отключаем буферизацию вывода
	::setvbuf(stdout, nullptr, _IONBF, 0);
	// Набор разбираемых выражений
	const struct {
		const char * name;
		const char * pattern;
		const string & text;
		size_t repeats;
	} SCENARIOS[] = {
		{"literal-short",           "Content-Length",                                      shortText(),  40000},
		{"literal-long",            "needle-in-haystack",                                  longText(),  78},
		{"literal-absent",          "no-such-sequence-here",                               longText(),  78},
		{"digits-short",            "[0-9]{3,5}",                                          shortText(),  26000},
		{"digits-long",             "[0-9]{3,5}",                                          longText(),  36000},
		{"word-long",               "\\w+@\\w+\\.\\w+",                                    longText(),  10},
		{"dotstar-long",            ".*needle",                                            longText(),  56},
		{"alternate-short",         "GET|POST|PUT|DELETE|HEAD|OPTIONS",                    shortText(),  40000},
		{"alternate-long",          "alpha|bravo|charlie|delta|echo|foxtrot",              longText(),  10},
		{"anchored-short",          "(?m)^[A-Za-z0-9-]+: .+$",                             shortText(),  5000},
		{"anchored-absent",         "^[A-Za-z0-9-]+: .+$",                                 shortText(),  40000},
		{"boundary-long",           "\\bneedle\\b",                                        longText(),  132},
		{"captures-short",          "([A-Za-z0-9-]+): (.+)",                               shortText(),  3200},
		{"captures-long",           "(\\w+)@(\\w+)\\.(\\w+)",                              longText(),  10},
		{"request-short",           "(?m)^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)\\r?$",      shortText(),  19600},
		{"request-absent",          "^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)$",              shortText(),  40000},
		{"address-short",           "(?m)^Host: (\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\r?$", shortText(),  19000},
		{"address-absent",          "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", shortText(),  40000},
		{"lazy-short",              "\\w+?@\\w+?\\.",                                      shortText(),  880},
		{"lazy-long",               "\\w+?@\\w+?\\.",                                      longText(),  10},
		{"lazy-dotstar",            ".*?needle",                                           longText(),  10},
		{"region-fixed-short",      "(?:HT|TP)+/",                                         shortText(),  26000},
		{"region-absent-short",     "(?:ab|cd)+z",                                         shortText(),  40000},
		{"region-fixed-long",       "(?:\\w\\w)+ 4096",                                    longText(),  10},
		{"region-varied-long",      "(?:[a-z]+ )+dog",                                     longText(),  2000},
		{"region-nested-heavy",     "\\((?:[^()]|\\([^()]*\\))*\\)",                       heavyText(),  8400},
		{"region-capture-heavy",    "(?:(\\w+) )+forman",                                  heavyText(),  26},
		{"bounded-short",           "(?:HT|TP)/1",                                         shortText(),  11000},
		{"bounded-long",            "(?:fox|dog)trot",                                     longText(),  10},
		{"bounded-absent",          "(?:fox|dog)trap",                                     longText(),  260},
		{"literal-medium",          "needle-in-haystack",                                  mediumText(),  12400},
		{"alternate-medium",        "alpha|bravo|charlie|delta|echo|foxtrot",              mediumText(),  194},
		{"captures-medium",         "(\\w+)@(\\w+)\\.(\\w+)",                              mediumText(),  10},
		{"bounded-medium",          "(?:fox|dog)trot",                                     mediumText(),  138},
		{"region-medium",           "(?:[a-z]+/)+v1",                                      mediumText(),  2000},
		{"region-lazy-short",       "(?:HT|TP)+?/",                                        shortText(),  20000},
		{"region-lazy-long",        "(?:[a-z]+ )+?dog",                                    longText(),  1600},
		{"region-empty-long",       "(?:[a-z]* ?)*dog",                                    longText(),  7200},
		{"region-empty-lazy",       "(?:[a-z]* ?)*?dog",                                   longText(),  15200},
		{"region-lazy-heavy",       "\\((?:[^()]|\\([^()]*\\))*?\\)",                      heavyText(),  5000},
		{"backref-heavy",           "(\\w+) \\1",                                          heavyText(),  1120},
		{"lookahead-heavy",         "\\w+(?=@)",                                           heavyText(),  480},
		{"lookbehind-heavy",        "(?<=@)\\w+",                                          heavyText(),  1220},
		{"atomic-heavy",            "(?>\\w+)@\\w+",                                       heavyText(),  360},
		{"recurse-heavy",           "\\((?:[^()]|(?R))*\\)",                               heavyText(),  5600}
	};

	/**
	 * Набор имён путей исполнения
	 *
	 * @details Длина набора обязана равняться «path_t::COUNT»: недостача
	 *          одного имени валит щуп разбором пустого указателя, и находка
	 *          эта принадлежит самому щупу, а не измеряемому модулю
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
	/**
	 * Если щупу задано имя сценария, выполняем долгий прогон под снятие образцов стека
	 */
	if(argc > 1) {
		// Выполняем перебор разбираемых выражений
		for(const auto & scenario : SCENARIOS) {
			// Если имя сценария не совпадает с заданным
			if(::strcmp(scenario.name, argv[1]) != 0)
				// Выполняем переход к выражению следующему
				continue;
			// Создаём движок сопоставления
			awh::regex::engine_t engine;
			// Создаём собранное выражение
			awh::regex::expression_t expression;
			// Выполняем сборку выражения
			if(!engine.build(scenario.pattern, 0, expression))
				// Выводим результат отказа сборки
				return 1;
			// Набор границ обнаруженного совпадения
			vector <pair <size_t, size_t>> captures;
			// Получаем время начала прогона
			const auto begin = chrono::steady_clock::now();
			// Выводим сообщение о начале прогона
			::printf("прогон сценария «%s» десять секунд\n", scenario.name);
			// Выполняем прогон до истечения срока
			while(chrono::duration_cast <chrono::seconds> (chrono::steady_clock::now() - begin).count() < 10) {
				// Выполняем очередную пачку сопоставлений
				for(size_t i = 0; i < scenario.repeats; i++)
					engine.exec(expression, scenario.text, 0, captures);
			}
			// Выводим результат работы щупа
			return 0;
		}
		// Выводим результат отсутствия сценария
		return 1;
	}
	// Выводим заголовок таблицы
	::printf("%-22s %10s %8s %8s %8s %8s %8s %8s   %s\n", "сценарий", "нс",
	 "шагов", "записей", "проверок", "точек", "кадров", "обходов", "пути исполнения");
	/**
	 * Выполняем перебор разбираемых выражений
	 */
	for(const auto & scenario : SCENARIOS) {
		// Создаём движок сопоставления
		awh::regex::engine_t engine;
		// Создаём собранное выражение
		awh::regex::expression_t expression;
		/**
		 * Если выражение не собрано
		 */
		if(!engine.build(scenario.pattern, 0, expression)) {
			// Выводим сообщение об ошибке
			::printf("%-22s ОТКАЗ СБОРКИ\n", scenario.name);
			// Выполняем переход к выражению следующему
			continue;
		}
		// Набор границ обнаруженного совпадения
		vector <pair <size_t, size_t>> captures;
		// Выполняем прогрев пути исполнения
		const bool found = engine.exec(expression, scenario.text, 0, captures);
		// Выполняем сброс счётчиков путей исполнения
		awh::regex::probe_t::reset();
		// Выполняем снятие путей исполнения одним сопоставлением
		engine.exec(expression, scenario.text, 0, captures);
		// Набор снятых мер работы сопоставления
		uint64_t works[static_cast <size_t> (awh::regex::work_t::COUNT)];
		/**
		 * Выполняем обход всех учитываемых мер работы сопоставления
		 */
		for(uint8_t i = 0; i < static_cast <uint8_t> (awh::regex::work_t::COUNT); i++)
			// Получаем количество операций очередной меры работы
			works[i] = awh::regex::probe_t::amount(static_cast <awh::regex::work_t> (i));
		// Строка перечня путей исполнения
		string paths;
		/**
		 * Выполняем обход всех учитываемых путей исполнения
		 */
		for(uint8_t i = 0; i < static_cast <uint8_t> (awh::regex::path_t::COUNT); i++) {
			// Получаем количество проходов пути исполнения
			const uint64_t value = awh::regex::probe_t::count(static_cast <awh::regex::path_t> (i));
			/**
			 * Если путь исполнения задействован
			 */
			if(value > 0) {
				// Выполняем добавление разделителя перечня
				if(!paths.empty())
					paths.append(" ");
				// Выполняем добавление имени пути исполнения
				paths.append(PATHS[i]).append("=").append(std::to_string(value));
			}
		}
		// Выполняем замер сопоставления целиком
		const double matching = ::fastest(6, scenario.repeats, [&]() noexcept -> void {
			engine.exec(expression, scenario.text, 0, captures);
		});
		// Выводим строку таблицы
		::printf("%-22s %10.0f %8llu %8llu %8llu %8llu %8llu %8llu   %s\n", scenario.name, matching,
		 static_cast <unsigned long long> (works[0]), static_cast <unsigned long long> (works[1]),
		 static_cast <unsigned long long> (works[2]), static_cast <unsigned long long> (works[3]),
		 static_cast <unsigned long long> (works[4]), static_cast <unsigned long long> (works[5]),
		 paths.c_str());
	}
	// Выводим результат работы щупа
	return ((argc > 1) ? (argv[0] != nullptr) : 0);
}
