/**
 * @file yaml-cpp.cpp
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек реализацией yaml-cpp
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
#include <yaml-cpp/yaml.h>

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @details Ссылка у этой реализации разрешается общим узлом: помеченное значение
 * и всякая ссылка на него суть один узел дерева, и обход проходит его столько раз,
 * сколько на него ссылались. Работа тем выходит той же, что и у дерева контейнера
 * AWH, где ссылка раскрывается переносом поддерева, а вот память - иной, и то
 * оговорено отчётом
 *
 * @param node обходимый узел дерева настроек
 *
 */
static void walk(const YAML::Node & node) noexcept {
	/**
	 * Определяем вид обходимого узла дерева настроек
	 */
	switch(node.Type()){
		// Если обходимый узел является отображением пар
		case YAML::NodeType::Map: {
			/**
			 * Выполняем перебор всех пар отображения
			 */
			for(YAML::const_iterator i = node.begin(); i != node.end(); ++i){
				// Выполняем обход имени очередной пары
				walk(i->first);
				// Выполняем обход значения очередной пары
				walk(i->second);
			}
		} break;
		// Если обходимый узел является перечнем значений
		case YAML::NodeType::Sequence: {
			/**
			 * Выполняем перебор всех значений перечня
			 */
			for(YAML::const_iterator i = node.begin(); i != node.end(); ++i)
				// Выполняем обход очередного значения перечня
				walk(*i);
		} break;
		// Если обходимый узел является скалярным значением
		case YAML::NodeType::Scalar: {
			// Получаем содержимое обходимого скалярного значения
			const std::string & text = node.Scalar();
			// Выполняем учёт обработанного скалярного значения
			rival::entry();
			// Выполняем учёт содержимого скалярного значения
			rival::consume(text.data(), text.size());
		} break;
		/**
		 * Если обходимый узел является пустым значением либо недействителен
		 */
		default: {
			// Выполняем учёт обработанного скалярного значения
			rival::entry();
		}
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
		const YAML::Node node = YAML::Load(text);
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
