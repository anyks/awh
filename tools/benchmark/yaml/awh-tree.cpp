/**
 * @file awh-tree.cpp
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек контейнером AWH
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/yaml/document.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @details Обход ведётся рекурсией по построению дерева, а не стопою: сличаемые
 * реализации обходятся своими средствами обхода, и всякая из них рекурсивна,
 * поэтому стоимость самого обхода у стендов одинакова
 *
 * @param value обходимый узел дерева настроек
 *
 */
static void walk(const awh::codec::yaml::document_t::value_t & value) noexcept {
	// Получаем имя обходимого узла дерева
	const std::string_view name = value.name();
	/**
	 * Если обходимый узел является парой отображения
	 */
	if(!name.empty()){
		// Выполняем учёт обработанного имени пары
		rival::entry();
		// Выполняем учёт содержимого имени пары
		rival::consume(name.data(), name.size());
	}
	/**
	 * Если обходимый узел является построением
	 */
	if(value.is(awh::codec::yaml::type_t::SEQUENCE) || value.is(awh::codec::yaml::type_t::MAPPING)){
		/**
		 * Выполняем перебор всех детей обходимого узла
		 */
		for(awh::codec::yaml::document_t::value_t item = value.begin(); item.valid(); item = item.next())
			// Выполняем обход очередного ребёнка узла
			walk(item);
		// Выходим из обхода узла дерева
		return;
	}
	// Получаем содержимое обходимого скалярного значения
	const std::string_view text = value.text();
	// Выполняем учёт обработанного скалярного значения
	rival::entry();
	// Выполняем учёт содержимого скалярного значения
	rival::consume(text.data(), text.size());
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект дерева настроек
	awh::codec::yaml::document_t document;
	/**
	 * Если разобрать текст настроек не удалось
	 */
	if(!document.parse(text))
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Выполняем перебор всех документов разобранного текста
	 */
	for(size_t i = 0; i < document.documents(); i++)
		// Выполняем обход корня очередного документа
		walk(document.root(i));
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Перечень сценариев стенда
 *
 */
static const rival::scenario_t SCENARIOS[] = {
	{"service",   rival::TREE_SMALL_ROUNDS,   rival::service,   parse},
	{"large",     rival::TREE_LARGE_ROUNDS,   rival::large,     parse},
	{"strings",   rival::TREE_FOCUSED_ROUNDS, rival::strings,   parse},
	{"numbers",   rival::TREE_FOCUSED_ROUNDS, rival::numbers,   parse},
	{"arrays",    rival::TREE_FOCUSED_ROUNDS, rival::arrays,    parse},
	{"blocks",    rival::TREE_FOCUSED_ROUNDS, rival::blocks,    parse},
	{"anchors",   rival::TREE_FOCUSED_ROUNDS, rival::anchors,   parse},
	{"decorated", rival::TREE_FOCUSED_ROUNDS, rival::decorated, parse}
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
