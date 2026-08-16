/**
 * @file tomlplusplus.cpp
 * @date 2026-08-16
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
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией toml++
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <string_view>

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <toml.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @param node обходимый узел дерева настроек
 *
 */
static void walk(const toml::node & node) noexcept {
	/**
	 * Если узлом является таблица
	 */
	if(const toml::table * table = node.as_table()){
		/**
		 * Выполняем перебор всех пар таблицы
		 */
		for(const auto & pair : *table){
			// Выполняем чтение имени ключа очередной пары
			rival::touch(pair.first.str().data(), pair.first.str().size());
			// Выполняем обход значения очередной пары
			walk(pair.second);
		}
		// Выходим из обхода узла дерева настроек
		return;
	}
	/**
	 * Если узлом является перечень значений
	 */
	if(const toml::array * array = node.as_array()){
		/**
		 * Выполняем перебор всех значений перечня
		 */
		for(const toml::node & item : *array)
			// Выполняем обход очередного значения перечня
			walk(item);
		// Выходим из обхода узла дерева настроек
		return;
	}
	// Выполняем учёт обработанной пары
	rival::entry();
	/**
	 * Если значение является строковым
	 */
	if(const toml::value <std::string> * value = node.as_string())
		// Выполняем учёт строкового значения
		rival::consume(value->get().data(), value->get().size());
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
	 * Выполняем перехват возможных исключений разбора
	 */
	try {
		// Выполняем разбор текста настроек
		const toml::table document = toml::parse(std::string_view(text.data(), text.size()));
		// Выполняем обход собранного дерева настроек
		walk(document);
	/**
	 * Если при разборе текста настроек произошла ошибка
	 */
	} catch(const toml::parse_error &){
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
	{"service", rival::SMALL_ROUNDS,   rival::service, parse},
	{"large",   rival::LARGE_ROUNDS,   rival::large,   parse},
	{"strings", rival::FOCUSED_ROUNDS, rival::strings, parse},
	{"numbers", rival::FOCUSED_ROUNDS, rival::numbers, parse},
	{"arrays",  rival::FOCUSED_ROUNDS, rival::arrays,  parse},
	{"tables",  rival::FOCUSED_ROUNDS, rival::tables,  parse}
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
