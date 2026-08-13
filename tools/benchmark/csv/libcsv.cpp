/**
 * @file: libcsv.cpp
 * @date: 2026-08-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения контейнера CSV на реализации libcsv
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
extern "C" {
	#include <csv.h>
}

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Обработчик очередного поля записи
 *
 * @param buffer содержимое поля записи
 * @param size   размер содержимого поля записи
 *
 */
static void field(void * buffer, size_t size, void *) {
	// Выполняем учёт содержимого поля записи
	rival::consume(buffer, size);
}
/**
 * @brief Обработчик конца записи
 *
 */
static void finish(int, void *) {
	// Выполняем учёт обработанной записи
	rival::record();
}

/**
 * @brief Функция разбора одной таблицы
 *
 * @param text разбираемый текст таблицы
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект разбора текста таблицы
	struct csv_parser parser;
	/**
	 * Если завести разбор текста таблицы не удалось
	 */
	if(csv_init(&parser, 0) != 0)
		// Выводим признак неудачного разбора
		return false;
	// Признак успешного разбора текста таблицы
	bool result = (csv_parse(&parser, text.data(), text.size(), field, finish, nullptr) == text.size());
	/**
	 * Если разбор текста таблицы удался
	 */
	if(result)
		// Выполняем завершение разбора текста таблицы
		result = (csv_fini(&parser, field, finish, nullptr) == 0);
	// Выполняем освобождение разбора текста таблицы
	csv_free(&parser);
	// Выводим признак успешного разбора
	return result;
}

/**
 * @brief Структура сценария стенда
 *
 */
struct scenario_t {
	// Название сценария
	const char * name;
	// Количество прогонов разбора
	size_t rounds;
	// Функция получения разбираемого текста таблицы
	const std::string & (* text)() noexcept;
	// Функция разбора одной таблицы
	bool (* subject)(const std::string &) noexcept;
};

/**
 * @brief Метод получения перечня сценариев стенда
 *
 * @return перечень сценариев стенда
 *
 */
static const std::vector <scenario_t> & scenarios() noexcept {
	// Перечень сценариев стенда
	static const std::vector <scenario_t> result = {
		{"narrow",    rival::LARGE_ROUNDS,   rival::narrow,    parse},
		{"wide",      rival::LARGE_ROUNDS,   rival::wide,      parse},
		{"quoted",    rival::FOCUSED_ROUNDS, rival::quoted,    parse},
		{"multiline", rival::FOCUSED_ROUNDS, rival::multiline, parse},
		{"small",     rival::SMALL_ROUNDS,   rival::small,     parse}
	};
	// Выводим перечень сценариев стенда
	return result;
}

/**
 * @brief Главная функция стенда
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Получаем отбор сценариев по вхождению в название
	const char * filter = rival::filter(argc, argv);
	// Итоги прогона сценария
	rival::outcome_t outcome{0, 0, 0.0};
	/**
	 * Выполняем перебор всех сценариев стенда
	 */
	for(const auto & scenario : scenarios()){
		/**
		 * Если сценарий отбором не выбран
		 */
		if(!rival::selected(scenario.name, filter))
			// Выполняем переход к следующему сценарию
			continue;
		/**
		 * Если прогон сценария выполнить не удалось
		 */
		if(!rival::parsing(scenario.subject, scenario.text(), scenario.rounds, outcome)){
			// Выводим сообщение о пропуске сценария
			rival::skip(scenario.name, "parsing failed");
			// Выполняем переход к следующему сценарию
			continue;
		}
		// Выводим результат прогона сценария
		rival::report(scenario.name, outcome);
	}
	// Выводим контрольную сумму работы, выполненной стендом
	rival::digest(argc, argv);
	// Выводим успешный код выхода из стенда
	return EXIT_SUCCESS;
}
