/**
 * @file cpptoml.cpp
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
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией cpptoml
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <sstream>

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <cpptoml.h>

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
static void walk(const std::shared_ptr <cpptoml::base> & node) noexcept {
	/**
	 * Если узел дерева настроек не передан
	 */
	if(!node)
		// Выходим из обхода узла дерева настроек
		return;
	/**
	 * Если узлом является таблица
	 */
	if(node->is_table()){
		/**
		 * Выполняем перебор всех пар таблицы
		 */
		for(const auto & pair : *node->as_table()){
			// Выполняем чтение имени ключа очередной пары
			rival::touch(pair.first.data(), pair.first.size());
			// Выполняем обход значения очередной пары
			walk(pair.second);
		}
		// Выходим из обхода узла дерева настроек
		return;
	}
	/**
	 * Если узлом является набор таблиц
	 */
	if(node->is_table_array()){
		/**
		 * Выполняем перебор всех таблиц набора
		 */
		for(const auto & table : node->as_table_array()->get())
			// Выполняем обход очередной таблицы набора
			walk(table);
		// Выходим из обхода узла дерева настроек
		return;
	}
	/**
	 * Если узлом является перечень значений
	 */
	if(node->is_array()){
		/**
		 * Выполняем перебор всех значений перечня
		 */
		for(const auto & item : node->as_array()->get())
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
	if(const auto value = node->as <std::string> ())
		// Выполняем учёт строкового значения
		rival::consume(value->get().data(), value->get().size());
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @note Реализация эта разбирает текст лишь из потока стандартной библиотеки:
 *       подачи из памяти она не предлагает вовсе, и стенд несёт на себе стоимость
 *       сборки потока со своей копией текста, которой у прочих стендов нет. Это
 *       не помеха замеру, а часть цены решения работать с потоком, и в отчёте её
 *       следует называть прямо
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
		// Выполняем сборку потока разбираемого текста настроек
		std::istringstream stream(text);
		// Объект разбора текста настроек
		cpptoml::parser parser(stream);
		// Выполняем обход собранного дерева настроек
		walk(parser.parse());
	/**
	 * Если при разборе текста настроек произошла ошибка
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
