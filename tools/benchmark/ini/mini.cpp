/**
 * @file: mini.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией mINI
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <string>
#include <fstream>
#include <filesystem>

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <mini/ini.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция получения пути временного файла с текстом настроек
 *
 * @details Реализация эта разбирает лишь файл на файловой системе: подачи из
 * памяти она не предлагает вовсе. Текст настроек кладётся во временный файл
 * однажды, и все прогоны читают его уже из страничного кэша - обращений к
 * устройству при замере не происходит
 *
 * @warning Стенд этот всё же несёт на себе стоимость открытия файла и чтения
 * через `std::ifstream`, которой у прочих стендов нет. Это не помеха замеру, а
 * часть цены решения работать с файлом, а не с памятью, и в отчёте её следует
 * называть прямо
 *
 * @param text размещаемый во временном файле текст настроек
 * @return     путь размещённого временного файла
 *
 */
static const std::string & source(const std::string & text) noexcept {
	// Путь размещённого временного файла
	static std::string result;
	// Размер размещённого текста настроек
	static size_t size = 0;
	/**
	 * Если текст настроек во временном файле уже размещён
	 */
	if(!result.empty() && (size == text.size()))
		// Выводим путь размещённого временного файла
		return result;
	// Собираем путь временного файла с текстом настроек
	result = (std::filesystem::temp_directory_path() / ("rival-ini-" + std::to_string(text.size()) + ".ini")).string();
	// Выполняем открытие временного файла на запись
	std::ofstream file(result, std::ios::out | std::ios::binary | std::ios::trunc);
	/**
	 * Если временный файл открыть удалось
	 */
	if(file.is_open()){
		// Выполняем запись текста настроек во временный файл
		file.write(text.data(), static_cast <std::streamsize> (text.size()));
		// Выполняем закрытие временного файла
		file.close();
	}
	// Запоминаем размер размещённого текста настроек
	size = text.size();
	// Выводим путь размещённого временного файла
	return result;
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект разбора текста настроек
	mINI::INIFile file(::source(text));
	// Собираемое дерево настроек
	mINI::INIStructure document;
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(!file.read(document))
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Выполняем перебор всех объявленных разделов
	 */
	for(const auto & section : document){
		// Выполняем чтение имени объявленного раздела
		rival::touch(section.first.data(), section.first.size());
		/**
		 * Выполняем перебор всех свойств раздела
		 */
		for(const auto & property : section.second){
			// Выполняем учёт обработанного свойства
			rival::entry();
			// Выполняем чтение имени свойства
			rival::touch(property.first.data(), property.first.size());
			// Выполняем учёт значения свойства
			rival::consume(property.second.data(), property.second.size());
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
