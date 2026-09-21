/**
 * @file passes.cpp
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
 * @brief Щуп разыскания цены прохода по тексту на КОРОТКОМ тексте —
 *        деление стоимости «exec» на проход автомата и доигрывание с возвратом
 *
 * @details Замер делит стоимость «exec» на составляющие через открытый
 *          договор: «test» проходит текст детерминированным исполнением
 *          и выводит лишь наличие совпадения, тогда как «exec» доигрывает
 *          исполнением с возвратом ради границ. Разность даёт цену прохода
 *          второго. Щуп этот и вскрыл, что на коротком тексте текст
 *          проходился дважды.
 *
 *          Несообразность «test дороже exec» изъяном НЕ является: вопросы
 *          у путей разные — проверке довольно наличия совпадения, и автомат
 *          отвечает не разыскивая начала. Довод записан в «engine.cpp»
 *          при выборе пути
 *
 *          Щуп собирается и запускается стендом своим - признак
 *          «AWH_REGEX_PROBING» и состав исходных текстов ведутся там:
 *          @code
 *          sh tools/regex/probe.sh passes
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

#include <cstdio>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

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
 * @brief Точка входа щупа
 */
int main() {
	// Отключаем буферизацию вывода
	::setvbuf(stdout, nullptr, _IONBF, 0);
	// Короткий текст сопоставления, с набора замеров снятый
	const string text(
		"GET /index.html HTTP/1.1\r\nHost: 192.168.001.100\r\n"
		"Content-Length: 4096\r\nUser-Agent: forman@anyks.com\r\n"
	);
	// Набор разбираемых выражений
	const struct {
		const char * name;
		const char * pattern;
	} SCENARIOS[] = {
		{"anchored-short", "(?m)^[A-Za-z0-9-]+: .+$"},
		{"captures-short", "([A-Za-z0-9-]+): (.+)"},
		{"lazy-short",     "\\w+?@\\w+?\\."},
		{"digits-short",   "[0-9]{3,5}"},
		{"literal-short",  "Content-Length"}
	};
	// Выводим заголовок таблицы
	::printf("%-16s %10s %10s %10s %8s\n", "сценарий", "test нс", "exec нс", "разность", "доля");
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
			::printf("%-16s ОТКАЗ СБОРКИ\n", scenario.name);
			// Выполняем переход к выражению следующему
			continue;
		}
		// Набор границ обнаруженного совпадения
		vector <pair <size_t, size_t>> captures;
		// Выполняем прогрев обоих путей
		engine.test(expression, text, 0);
		engine.exec(expression, text, 0, captures);
		// Выполняем замер прохода детерминированного исполнения
		const double checking = ::fastest(8, 20000, [&]() noexcept -> void {
			engine.test(expression, text, 0);
		});
		// Выполняем замер сопоставления целиком
		const double matching = ::fastest(8, 20000, [&]() noexcept -> void {
			engine.exec(expression, text, 0, captures);
		});
		// Выводим строку таблицы
		::printf("%-16s %10.1f %10.1f %10.1f %8.2f\n", scenario.name, checking, matching,
		 (matching - checking), (checking / matching));
	}
	// Выводим результат работы щупа
	return 0;
}
