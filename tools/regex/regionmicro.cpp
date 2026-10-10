/**
 * @file regionmicro.cpp
 * @date 2026-10-10
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп микроразбора платы фасада regexp_t::match
 *
 * @details Regioncost показал: engine.exec работает на 14 нс, regexp.match
 *          через фасад — на 28 нс. Чтобы источник 14 нс платы фасада указать,
 *          щуп мерит по очереди:
 *
 *          - только `engine.exec` (чистый путь, без фасада);
 *          - `engine.exec` с предварительной очисткой `captures` снаружи;
 *          - `regexp.match` (полный фасад);
 *          - `regexp.match` с уже снятыми `captures` (если фасад где-то
 *            сам делает лишнее `clear`).
 *
 * Сборка и запуск: tools/regex/probe.sh regionmicro
 *
 * @copyright Copyright © 2026
 *
 */

#include <regex/regex.hpp>
#include <regex/probe.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <utility>

using namespace std;

static const string TEXT = []() noexcept -> string {
	string outcome;
	size_t number = 0;
	while(outcome.size() < 2048){
		outcome.append("GET /api/v1/items/");
		outcome.append(to_string(number++));
		outcome.append(" HTTP/1.1\r\nHost: node-07.anyks.com\r\n"
		 "User-Agent: awh/5.0\r\nAccept: application/json\r\n"
		 "X-Request-Id: 7f3a9c2e-41bd-4e88-9a1f-0c5d6e8b2a34\r\n"
		 "Content-Length: 512\r\n\r\n");
	}
	outcome.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
	return outcome;
}();

template <typename Body>
static double fastest(const size_t repeats, Body && body) noexcept {
	double result = 0.;
	for(uint32_t round = 0; round < 12; round++){
		const auto begin = chrono::steady_clock::now();
		for(size_t i = 0; i < repeats; i++) body();
		const double spent = (static_cast <double> (chrono::duration_cast <chrono::nanoseconds> (
		 chrono::steady_clock::now() - begin).count()) / static_cast <double> (repeats));
		if((result == 0.) || (spent < result)) result = spent;
	}
	return result;
}

int main() noexcept {
	awh::fmk::initialize();
	awh::log::mode({});
	awh::regex::engine_t engine;
	awh::regexp_t regexp;
	vector <pair <size_t, size_t>> captures;
	const auto expression = regexp.build("(?:[a-z]+/)+v1",
	 static_cast <uint32_t> (awh::regex::flag_t::JIT));
	if(!expression){
		::printf("ОТКАЗ СБОРКИ\n");
		return 1;
	}
	// Прогрев движка
	for(uint16_t i = 0; i < 5; i++)
		regexp.match(TEXT, expression, captures);
	::printf("%-30s %10s %10s\n", "ВЫЗОВ", "нс", "М/с");
	struct line_t { const char * name; double spent; };
	// 1. Чистый engine.exec
	{
		double s = fastest(40000, [&]() noexcept -> void {
			engine.exec(* expression, TEXT, 0, captures);
		});
		::printf("%-30s %10.0f %10.0f\n", "engine.exec", s, (s > 0. ? 1e9 / s : 0.));
	}
	// 2. regexp.match (фасад)
	{
		double s = fastest(40000, [&]() noexcept -> void {
			regexp.match(TEXT, expression, captures);
		});
		::printf("%-30s %10.0f %10.0f\n", "regexp.match", s, (s > 0. ? 1e9 / s : 0.));
	}
	// 3. engine.exec с предварительной очисткой captures (как фасад делает)
	{
		double s = fastest(40000, [&]() noexcept -> void {
			captures.clear();
			engine.exec(* expression, TEXT, 0, captures);
		});
		::printf("%-30s %10.0f %10.0f\n", "engine.exec + clear", s, (s > 0. ? 1e9 / s : 0.));
	}
	// 4. Пустой цикл для измерения шума замера
	{
		double s = fastest(40000, [&]() noexcept -> void {
			(void) captures;
		});
		::printf("%-30s %10.0f %10.0f\n", "empty loop (noise)", s, (s > 0. ? 1e9 / s : 0.));
	}
	return 0;
}
