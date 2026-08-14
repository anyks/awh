/**
 * @file iniparser.cpp
 * @date 2026-08-10
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
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией iniparser
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
extern "C" {
	#include <iniparser.h>
};

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Реализация эта разбирает лишь открытый файл, и текст настроек
 * подаётся ей через `fmemopen`: обращения к файловой системе при этом не
 * происходит, и замер остаётся сравнимым с прочими стендами. Своя доля издержек
 * у такой подачи всё же есть - чтение идёт через `stdio`, - и это часть цены
 * решения работать с файлом, а не с памятью
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Выполняем открытие текста настроек как файла в памяти
	FILE * source = ::fmemopen(const_cast <char *> (text.data()), text.size(), "r");
	/**
	 * Если открыть текст настроек не удалось
	 */
	if(source == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Выполняем разбор текста настроек
	dictionary * result = ::iniparser_load_file(source, "benchmark");
	// Выполняем закрытие текста настроек
	::fclose(source);
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(result == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Получаем количество объявленных разделов
	const int32_t count = ::iniparser_getnsec(result);
	/**
	 * Выполняем перебор всех объявленных разделов
	 */
	for(int32_t i = 0; i < count; i++){
		// Получаем имя очередного объявленного раздела
		const char * section = ::iniparser_getsecname(result, i);
		/**
		 * Если имя объявленного раздела получить не удалось
		 */
		if(section == nullptr)
			// Выполняем переход к следующему разделу
			continue;
		// Выполняем чтение имени объявленного раздела
		rival::touch(section, ::strlen(section));
		// Получаем количество свойств очередного раздела
		const int32_t keys = ::iniparser_getsecnkeys(result, section);
		/**
		 * Если свойств у раздела не объявлено
		 */
		if(keys <= 0)
			// Выполняем переход к следующему разделу
			continue;
		// Перечень имён свойств очередного раздела
		std::vector <const char *> names(static_cast <size_t> (keys), nullptr);
		// Выполняем получение перечня имён свойств раздела
		::iniparser_getseckeys(result, section, names.data());
		/**
		 * Выполняем перебор всех свойств раздела
		 */
		for(size_t j = 0; j < names.size(); j++){
			/**
			 * Если имя очередного свойства получить не удалось
			 */
			if(names.at(j) == nullptr)
				// Выполняем переход к следующему свойству
				continue;
			// Получаем значение очередного свойства раздела
			const char * value = ::iniparser_getstring(result, names.at(j), "");
			// Выполняем учёт обработанного свойства
			rival::entry();
			// Выполняем чтение имени свойства
			rival::touch(names.at(j), ::strlen(names.at(j)));
			// Выполняем учёт значения свойства
			rival::consume(value, ((value != nullptr) ? ::strlen(value) : 0));
		}
	}
	// Выполняем освобождение разобранного словаря
	::iniparser_freedict(result);
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
