/**
 * @file: simpleini.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией SimpleIni
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <SimpleIni.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Реализация эта хранит примечания вместе с записями и способна
 * переписать файл, их не обеднив, - тем она и близка дереву настроек AWH.
 * Многозначность включена: повторное объявление свойства задаёт перечень
 * значений, как и у дерева AWH
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект разбора текста настроек
	CSimpleIniA document;
	// Устанавливаем признание многозначности свойств
	document.SetMultiKey(true);
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(document.LoadData(text.data(), text.size()) != SI_OK)
		// Выводим признак неудачного разбора
		return false;
	// Перечень объявленных разделов
	CSimpleIniA::TNamesDepend sections;
	// Выполняем получение перечня объявленных разделов
	document.GetAllSections(sections);
	/**
	 * Выполняем перебор всех объявленных разделов
	 */
	for(const CSimpleIniA::Entry & section : sections){
		// Выполняем чтение имени объявленного раздела
		rival::touch(section.pItem, ((section.pItem != nullptr) ? ::strlen(section.pItem) : 0));
		// Перечень имён свойств очередного раздела
		CSimpleIniA::TNamesDepend keys;
		/**
		 * Если перечень имён свойств раздела получить не удалось
		 */
		if(!document.GetAllKeys(section.pItem, keys))
			// Выполняем переход к следующему разделу
			continue;
		/**
		 * Выполняем перебор всех свойств раздела
		 */
		for(const CSimpleIniA::Entry & key : keys){
			// Получаем значение очередного свойства раздела
			const char * value = document.GetValue(section.pItem, key.pItem, "");
			// Выполняем учёт обработанного свойства
			rival::entry();
			// Выполняем чтение имени свойства
			rival::touch(key.pItem, ((key.pItem != nullptr) ? ::strlen(key.pItem) : 0));
			// Выполняем учёт значения свойства
			rival::consume(value, ((value != nullptr) ? ::strlen(value) : 0));
		}
	}
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Перечень сценариев стенда
 *
 */
static const rival::scenario_t SCENARIOS[] = {
	{"service",    rival::SMALL_ROUNDS,   rival::service,    parse},
	{"repository", rival::SMALL_ROUNDS,   rival::repository, parse},
	{"large",      rival::LARGE_ROUNDS,   rival::large,      parse},
	{"annotated",  rival::FOCUSED_ROUNDS, rival::annotated,  parse},
	{"sections",   rival::FOCUSED_ROUNDS, rival::sections,   parse}
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
