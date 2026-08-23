/**
 * @file rapidyaml.cpp
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек реализацией rapidyaml
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Требуем от одиночного заголовочного файла выложить реализацию свою
 *
 * @note Реализация выкладывается одной единицей трансляции, и она же единственная:
 *       стенд состоит из одного файла, и заводить ради выкладки вторую было бы
 *       незачем
 */
#define RYML_SINGLE_HDR_DEFINE_NOW

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <ryml.hpp>
#include <ryml_std.hpp>

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @param tree обходимое дерево настроек
 * @param id   номер обходимого узла дерева
 *
 */
static void walk(const ryml::Tree & tree, const size_t id) noexcept {
	/**
	 * Если обходимый узел несёт имя пары отображения
	 */
	if(tree.has_key(id)){
		// Получаем имя обходимой пары отображения
		const ryml::csubstr name = tree.key(id);
		// Выполняем учёт обработанного имени пары
		rival::entry();
		// Выполняем учёт содержимого имени пары
		rival::consume(name.data(), name.size());
	}
	/**
	 * Если обходимый узел несёт скалярное значение
	 */
	if(tree.has_val(id)){
		// Получаем содержимое обходимого скалярного значения
		const ryml::csubstr text = tree.val(id);
		// Выполняем учёт обработанного скалярного значения
		rival::entry();
		// Выполняем учёт содержимого скалярного значения
		rival::consume(text.data(), text.size());
		// Выходим из обхода узла дерева
		return;
	}
	/**
	 * Выполняем перебор всех детей обходимого узла
	 */
	for(size_t i = tree.first_child(id); i != ryml::NONE; i = tree.next_sibling(i))
		// Выполняем обход очередного ребёнка узла
		walk(tree, i);
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Разрешение ссылок затребовано отдельным вызовом намеренно: без него
 * реализация оставляет ссылку узлом ссылки и работы по раскрытию её не выполняет
 * вовсе, тогда как дерево контейнера AWH раскрывает её переносом поддерева. Без
 * этого вызова сравнивался бы разбор с раскрытием против разбора без раскрытия
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Выполняем разбор текста настроек
	ryml::Tree tree = ryml::parse_in_arena(ryml::csubstr(text.data(), text.size()));
	/**
	 * Если дерево настроек собрано пустым
	 */
	if(tree.empty())
		// Выводим признак неудачного разбора
		return false;
	// Выполняем разрешение всех ссылок собранного дерева
	tree.resolve();
	// Выполняем обход собранного дерева настроек
	walk(tree, tree.root_id());
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
