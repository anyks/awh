/**
 * @file: gkeyfile.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией GKeyFile библиотеки GLib
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <glib.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Реализация эта - образцовая для записи файлов описания рабочего
 * места, и примечания она сохраняет по договору. Разбор её строже прочих:
 * свойства до первого объявления раздела она отвергает, а из знаков примечания
 * признаёт лишь решётку
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект разбора текста настроек
	GKeyFile * document = ::g_key_file_new();
	/**
	 * Если объект разбора завести не удалось
	 */
	if(document == nullptr)
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Устанавливаем сохранение примечаний и переводов текста
	 *
	 * @note Без этого признака примечания при разборе отбрасываются, и
	 *       сравниваемая работа оказалась бы меньше, чем у прочих стендов
	 */
	const GKeyFileFlags flags = static_cast <GKeyFileFlags> (G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(!::g_key_file_load_from_data(document, text.data(), text.size(), flags, nullptr)){
		// Выполняем освобождение объекта разбора
		::g_key_file_free(document);
		// Выводим признак неудачного разбора
		return false;
	}
	// Количество объявленных разделов
	gsize count = 0;
	// Выполняем получение перечня объявленных разделов
	gchar ** sections = ::g_key_file_get_groups(document, &count);
	/**
	 * Выполняем перебор всех объявленных разделов
	 */
	for(gsize i = 0; ((sections != nullptr) && (i < count)); i++){
		// Выполняем чтение имени объявленного раздела
		rival::touch(sections[i], ::strlen(sections[i]));
		// Количество свойств очередного раздела
		gsize amount = 0;
		// Выполняем получение перечня имён свойств раздела
		gchar ** keys = ::g_key_file_get_keys(document, sections[i], &amount, nullptr);
		/**
		 * Выполняем перебор всех свойств раздела
		 */
		for(gsize j = 0; ((keys != nullptr) && (j < amount)); j++){
			// Получаем значение очередного свойства раздела
			gchar * value = ::g_key_file_get_value(document, sections[i], keys[j], nullptr);
			// Выполняем учёт обработанного свойства
			rival::entry();
			// Выполняем чтение имени свойства
			rival::touch(keys[j], ::strlen(keys[j]));
			// Выполняем учёт значения свойства
			rival::consume(value, ((value != nullptr) ? ::strlen(value) : 0));
			// Выполняем освобождение значения свойства
			::g_free(value);
		}
		// Выполняем освобождение перечня имён свойств раздела
		::g_strfreev(keys);
	}
	// Выполняем освобождение перечня объявленных разделов
	::g_strfreev(sections);
	// Выполняем освобождение объекта разбора
	::g_key_file_free(document);
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
