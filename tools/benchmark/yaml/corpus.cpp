/**
 * @file corpus.cpp
 * @date 2026-08-24
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Сличение эталонных текстов стенда соперников с текстами собственного набора замеров
 *
 * @details Описание стенда утверждает, что эталонные тексты его совпадают с текстами
 *          собственного набора замеров, и что расхождение хотя бы в одном тексте
 *          обесценивает отчёт целиком. Утверждение это держалось одним лишь словом:
 *          тексты собираются двумя телами в двух местах, и правка одного из них
 *          расхождения ничем не выдавала - отчёт выходил правдоподобным и негодным
 *
 * @note Сличаются сами собранные тексты, а не исходные их построения: сличение текстов
 *       исходных спотыкалось об имена переменных да пространства имён, расхождением не
 *       являющиеся вовсе
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <string>

/**
 * Подключаем заголовочные файлы стенда соперников
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы собственного набора замеров
 */
#include <yaml.hpp>

/**
 * @brief Функция запуска сличения эталонных текстов
 *
 * @return код выхода из сличения
 *
 */
int32_t main() noexcept {
	/**
	 * @brief Описание сличаемой пары эталонных текстов
	 *
	 */
	struct pair_t {
		// Название сценария замера
		const char * name;
		// Тело, эталонный текст стенда соперников выдающее
		const std::string & (* stand)() noexcept;
		// Тело, эталонный текст собственного набора замеров выдающее
		const std::string & (* suite)() noexcept;
	};
	// Перечень сличаемых пар эталонных текстов
	static const pair_t PAIRS[] = {
		{"service", rival::service, awh::benchmark::manifest::service},
		{"large", rival::large, awh::benchmark::manifest::large},
		{"strings", rival::strings, awh::benchmark::manifest::strings},
		{"numbers", rival::numbers, awh::benchmark::manifest::numbers},
		{"arrays", rival::arrays, awh::benchmark::manifest::arrays},
		{"blocks", rival::blocks, awh::benchmark::manifest::blocks},
		{"anchors", rival::anchors, awh::benchmark::manifest::anchors},
		{"decorated", rival::decorated, awh::benchmark::manifest::decorated},
	};
	// Количество расхождений, сличением найденных
	size_t diverged = 0;
	/**
	 * Выполняем перебор всех сличаемых пар эталонных текстов
	 */
	for(const pair_t & pair : PAIRS){
		// Получаем эталонный текст стенда соперников
		const std::string & stand = pair.stand();
		// Получаем эталонный текст собственного набора замеров
		const std::string & suite = pair.suite();
		/**
		 * Если эталонные тексты расходятся
		 */
		if(stand != suite){
			// Выполняем учёт найденного расхождения
			diverged++;
			// Выводим сообщение о расхождении эталонных текстов
			::fprintf(stderr, "сценарий «%s»: текст стенда %zu байт, текст набора %zu байт\n",
			 pair.name, stand.size(), suite.size());
		}
	}
	/**
	 * Если расхождения найдены
	 */
	if(diverged > 0){
		// Выводим сообщение о количестве найденных расхождений
		::fprintf(stderr, "эталонные тексты расходятся у %zu сценариев из %zu: отчёт сличения негоден\n",
		 diverged, sizeof(PAIRS) / sizeof(PAIRS[0]));
		// Выводим отрицательный код выхода из сличения
		return EXIT_FAILURE;
	}
	// Выводим сообщение о совпадении эталонных текстов
	::fprintf(stderr, "эталонные тексты совпадают у всех %zu сценариев\n", sizeof(PAIRS) / sizeof(PAIRS[0]));
	// Выводим успешный код выхода из сличения
	return EXIT_SUCCESS;
}
