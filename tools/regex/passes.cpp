/**
 * @file passes.cpp
 * @brief Щуп разыскания: куда уходит время разбора на коротком тексте
 *
 * @details Замер делит стоимость «exec» на составляющие через открытый договор:
 *          «test» проходит текст детерминированным исполнением и выводит лишь
 *          наличие совпадения, тогда как «exec» доигрывает исполнением
 *          с возвратом ради границ. Разность даёт цену прохода второго.
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
