/**
 * @file longpass.cpp
 * @brief Щуп разыскания: куда уходит время разбора на ДЛИННОМ тексте
 *
 * @details Текст сложен тем же порядком, что и у набора замеров: связная проза
 *          до заданной длины, а искомые последовательности - у самого конца,
 *          отчего текст приходится пройти весь.
 */

#include <regex/regex.hpp>

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
 * @brief Точка входа щупа
 */
int main(int argc, char ** argv) {
	// Отключаем буферизацию вывода
	::setvbuf(stdout, nullptr, _IONBF, 0);
	// Создаём длинный текст сопоставления
	string text;
	// Выполняем размещение длинного текста сопоставления
	text.reserve(262144 + 128);
	// Выполняем наполнение текста до заданной длины
	while(text.size() < 262144)
		text.append("the quick brown fox jumps over the lazy dog 1234 ");
	// Выполняем добавление искомых последовательностей у конца текста
	text.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
	// Набор разбираемых выражений
	const struct {
		const char * name;
		const char * pattern;
	} SCENARIOS[] = {
		{"digits-long",        "[0-9]{3,5}"},
		{"region-varied-long", "(?:[a-z]+ )+dog"},
		{"region-lazy-long",   "(?:[a-z]+ )+?dog"},
		{"lazy-dotstar",       ".*?needle"},
		{"boundary-long",      "\\bneedle\\b"}
	};
	// Выводим заголовок таблицы
	::printf("%-20s %12s %12s %12s %8s\n", "сценарий", "test нс", "exec нс", "разность", "доля");
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
			::printf("%-20s ОТКАЗ СБОРКИ\n", scenario.name);
			// Выполняем переход к выражению следующему
			continue;
		}
		// Набор границ обнаруженного совпадения
		vector <pair <size_t, size_t>> captures;
		// Выполняем прогрев обоих путей
		engine.test(expression, text, 0);
		const bool found = engine.exec(expression, text, 0, captures);
		// Выполняем замер прохода детерминированного исполнения
		const double checking = ::fastest(6, 40, [&]() noexcept -> void {
			engine.test(expression, text, 0);
		});
		// Выполняем замер сопоставления целиком
		const double matching = ::fastest(6, 40, [&]() noexcept -> void {
			engine.exec(expression, text, 0, captures);
		});
		// Выводим строку таблицы
		::printf("%-20s %12.0f %12.0f %12.0f %8.2f  %s\n", scenario.name, checking, matching,
		 (matching - checking), (checking / matching), (found ? "совпало" : "нет совпадения"));
	}
	// Выводим результат работы щупа
	return ((argc > 1) ? (argv[0] != nullptr) : 0);
}
