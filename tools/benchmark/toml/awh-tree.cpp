/**
 * @file awh-tree.cpp
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек контейнером AWH
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/toml/document.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Собранное дерево обходом не читается: перебора дочерних имён дерево
 * настроек TOML не предлагает вовсе - обход целиком выполняется потоковым чтением,
 * а дерево отвечает на запрос по имени. Стенд потому меряет одну лишь сборку
 * дерева
 *
 * @warning Читать показатель следует с этой поправкой: сличаемые реализации
 * собранное дерево обходят и оплачивают обход, а стенд этот - нет. Работу,
 * равную обходу, выполняет стенд `awh` потоковым чтением, и сличать с обходом
 * следует его. Количество прочитанных пар стенд этот не выдаёт по той же причине
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Дерево настроек
	awh::codec::toml::document_t document;
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(!document.parse(text))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем чтение количества собранных узлов дерева
	rival::touch(nullptr, document.size());
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
