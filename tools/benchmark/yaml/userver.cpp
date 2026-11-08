/**
 * @file userver.cpp
 * @date 2026-11-07
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек реализацией userver
 *
 * @details Реализация userver собственного разбора настроек не несёт: значение её
 *          построено поверх yaml-cpp, и потому стенд этот меряет не столько
 *          разбор, сколько слой значения, положенный поверх чужого двигателя.
 *          Обход ведётся нарочно публичным обращением, а не доступом к родному
 *          узлу: родной узел мерил бы yaml-cpp, какой уже снят своим стендом
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
#include <userver/formats/yaml.hpp>

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @details Ссылка у этой реализации разрешается общим узлом: помеченное значение
 * и всякая ссылка на него суть один узел дерева, и обход проходит его столько раз,
 * сколько на него ссылались. Работа тем выходит той же, что и у дерева контейнера
 * AWH, где ссылка раскрывается переносом поддерева, а вот память - иной, и то
 * оговорено отчётом
 *
 * @param value обходимое значение дерева настроек
 *
 */
static void walk(const userver::formats::yaml::Value & value) noexcept {
	/**
	 * Если значение является отображением пар
	 */
	if(value.IsObject()){
		/**
		 * Выполняем перебор всех пар отображения
		 */
		for(auto i = value.begin(); i != value.end(); ++i){
			// Имя очередной пары отображения
			const auto key = i.GetName();
			// Выполняем учёт обработанного скалярного значения
			rival::entry();
			// Выполняем учёт имени очередной пары
			rival::consume(key.data(), key.size());
			// Выполняем обход значения очередной пары
			walk(* i);
		}
	/**
	 * Если значение является перечнем значений
	 */
	} else if(value.IsArray()){
		/**
		 * Выполняем перебор всех значений перечня
		 */
		for(const auto & item : value)
			// Выполняем обход очередного значения перечня
			walk(item);
	/**
	 * Если значение является скалярным значением
	 */
	} else if(value.IsString()){
		// Содержимое скалярного значения
		const auto text = value.As <std::string> ();
		// Выполняем учёт обработанного скалярного значения
		rival::entry();
		// Выполняем учёт содержимого скалярного значения
		rival::consume(text.data(), text.size());
	/**
	 * Если значение является пустым либо отсутствует
	 */
	} else {
		// Выполняем учёт обработанного скалярного значения
		rival::entry();
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
		// Значение разобранного дерева настроек
		const auto value = userver::formats::yaml::FromString(text);
		// Выполняем обход собранного дерева настроек
		walk(value);
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
