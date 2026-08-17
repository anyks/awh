/**
 * @file fkyaml.cpp
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек реализацией fkYAML
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <fkYAML/node.hpp>

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @details Содержимое в исходном виде реализация не хранит: разрешённое значение
 * выдаётся ею числом либо логическим значением языка, и вернуть по нему исходную
 * запись нельзя. Оттого в контрольную сумму складывается содержимое одних лишь
 * строковых значений, а прочие учитываются лишь количеством. Количество это
 * совпадать с остальными стендами обязано, а сумма у этого стенда неполна, и то
 * оговорено отчётом
 *
 * @param node обходимый узел дерева настроек
 *
 */
static void walk(const fkyaml::node & node) noexcept {
	/**
	 * Если обходимый узел является отображением пар
	 */
	if(node.is_mapping()){
		/**
		 * Выполняем перебор всех пар отображения
		 */
		for(const auto & item : node.as_map()){
			// Выполняем обход имени очередной пары
			walk(item.first);
			// Выполняем обход значения очередной пары
			walk(item.second);
		}
		// Выходим из обхода узла дерева
		return;
	}
	/**
	 * Если обходимый узел является перечнем значений
	 */
	if(node.is_sequence()){
		/**
		 * Выполняем перебор всех значений перечня
		 */
		for(const auto & item : node.as_seq())
			// Выполняем обход очередного значения перечня
			walk(item);
		// Выходим из обхода узла дерева
		return;
	}
	// Выполняем учёт обработанного скалярного значения
	rival::entry();
	/**
	 * Если обходимое скалярное значение является строковым
	 */
	if(node.is_string()){
		// Получаем содержимое обходимого скалярного значения
		const std::string & text = node.as_str();
		// Выполняем учёт содержимого скалярного значения
		rival::consume(text.data(), text.size());
	}
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	/**
	 * Выполняем перехват ошибок разбора текста настроек
	 */
	try {
		// Выполняем разбор текста настроек
		const fkyaml::node node = fkyaml::node::deserialize(text);
		// Выполняем обход собранного дерева настроек
		walk(node);
	/**
	 * Если разбор текста настроек прекращён отказом
	 */
	} catch(const std::exception &){
		// Выводим признак неудачного разбора
		return false;
	}
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
