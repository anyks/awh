/**
 * @file inih.cpp
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
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией inih
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
extern "C" {
	#include <ini.h>
};

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обратного вызова разбора очередного свойства
 *
 * @details Реализация эта примечаний не выдаёт и разделы отдельным событием не
 * объявляет: имя раздела приходит с каждым свойством. Оттого имя раздела
 * читается здесь при всяком свойстве, а не однажды при его объявлении
 *
 * @param user    указатель на пользовательские данные
 * @param section имя раздела разобранного свойства
 * @param key     имя разобранного свойства
 * @param value   значение разобранного свойства
 * @return        признак продолжения разбора
 *
 */
static int handler(void * user, const char * section, const char * key, const char * value) {
	// Блокируем неиспользуемую переменную
	(void) user;
	// Выполняем учёт обработанного свойства
	rival::entry();
	// Выполняем чтение имени раздела
	rival::touch(section, ((section != nullptr) ? ::strlen(section) : 0));
	// Выполняем чтение имени свойства
	rival::touch(key, ((key != nullptr) ? ::strlen(key) : 0));
	// Выполняем учёт значения свойства
	rival::consume(value, ((value != nullptr) ? ::strlen(value) : 0));
	// Выводим признак продолжения разбора
	return 1;
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Выводим признак успешного разбора
	return (::ini_parse_string_length(text.data(), text.size(), handler, nullptr) == 0);
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
