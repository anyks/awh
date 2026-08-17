/**
 * @file libyaml.cpp
 * @date 2026-08-17
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
 * @brief Эталонный стенд сравнения — потоковое чтение текста настроек реализацией libyaml
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <yaml.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Реализация ведёт разбор наречием 1.1 и вида скалярного значения не
 * разрешает вовсе: всякое значение выдаётся отрезком текста, ограды лишённым.
 * Оттого учитывается содержимое всех значений без изъятия - тем же правилом, каким
 * учитывает его стенд контейнера AWH
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект разбора текста настроек
	yaml_parser_t parser;
	/**
	 * Если завести объект разбора не удалось
	 */
	if(yaml_parser_initialize(&parser) == 0)
		// Выводим признак неудачного разбора
		return false;
	// Выполняем передачу разбираемого текста настроек
	yaml_parser_set_input_string(&parser, reinterpret_cast <const unsigned char *> (text.data()), text.size());
	// Признак успешного разбора текста настроек
	bool result = true;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(result){
		// Очередное событие разбора
		yaml_event_t event;
		/**
		 * Если получить очередное событие разбора не удалось
		 */
		if(yaml_parser_parse(&parser, &event) == 0){
			// Запоминаем признак неудачного разбора
			result = false;
			// Выходим из перебора событий разбора
			break;
		}
		/**
		 * Если получено скалярное значение
		 */
		if(event.type == YAML_SCALAR_EVENT){
			// Выполняем учёт обработанного скалярного значения
			rival::entry();
			// Выполняем учёт содержимого скалярного значения
			rival::consume(event.data.scalar.value, event.data.scalar.length);
		}
		// Запоминаем достижение конца потока документов
		const bool finished = (event.type == YAML_STREAM_END_EVENT);
		// Выполняем освобождение памяти очередного события разбора
		yaml_event_delete(&event);
		/**
		 * Если поток документов разобран до конца
		 */
		if(finished)
			// Выходим из перебора событий разбора
			break;
	}
	// Выполняем освобождение памяти объекта разбора
	yaml_parser_delete(&parser);
	// Выводим признак успешного разбора
	return result;
}

/**
 * @brief Перечень сценариев стенда
 *
 */
static const rival::scenario_t SCENARIOS[] = {
	{"service",   rival::SMALL_ROUNDS,   rival::service,   parse},
	{"large",     rival::LARGE_ROUNDS,   rival::large,     parse},
	{"strings",   rival::FOCUSED_ROUNDS, rival::strings,   parse},
	{"numbers",   rival::FOCUSED_ROUNDS, rival::numbers,   parse},
	{"arrays",    rival::FOCUSED_ROUNDS, rival::arrays,    parse},
	{"blocks",    rival::FOCUSED_ROUNDS, rival::blocks,    parse},
	{"anchors",   rival::FOCUSED_ROUNDS, rival::anchors,   parse},
	{"decorated", rival::FOCUSED_ROUNDS, rival::decorated, parse}
};

/**
 * @brief Главная функция стенда
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Выполняем прогон всех сценариев стенда
	return rival::run(argc, argv, SCENARIOS, (sizeof(SCENARIOS) / sizeof(SCENARIOS[0])));
}
