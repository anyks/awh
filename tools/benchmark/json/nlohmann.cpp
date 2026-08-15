/**
 * @file nlohmann.cpp
 * @date 2026-08-14
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
 * @brief Эталонный стенд сравнения контейнера JSON — разбор документа средствами
 *        реализации nlohmann/json с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <nlohmann/json.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обхода собранного дерева документа
 *
 * @param value обходимое значение документа
 *
 */
static void walk(const nlohmann::json & value) noexcept {
	/**
	 * Если значение является объектом
	 */
	if(value.is_object()){
		/**
		 * Выполняем обход всех полей объекта
		 */
		for(auto item = value.begin(); item != value.end(); ++item){
			// Выполняем учёт прочитанного имени поля объекта
			rival::consume(item.key().data(), item.key().size());
			// Выполняем обход значения очередного поля объекта
			walk(item.value());
		}
	/**
	 * Если значение является массивом
	 */
	} else if(value.is_array()) {
		/**
		 * Выполняем обход всех значений массива
		 */
		for(const auto & item : value)
			// Выполняем обход очередного значения массива
			walk(item);
	/**
	 * Если значение является строкой
	 */
	} else if(value.is_string()) {
		// Получаем прочитанное строковое значение
		const std::string & result = value.get_ref <const std::string &> ();
		// Выполняем учёт прочитанного строкового значения
		rival::consume(result.data(), result.size());
	/**
	 * Если значение является числом
	 */
	} else if(value.is_number()) {
		// Выполняем учёт прочитанного числа
		rival::consume(value.get <double> ());
	/**
	 * Если значение является логическим
	 */
	} else if(value.is_boolean())
		// Выполняем учёт прочитанного логического значения
		rival::consume(value.get <bool> ());
	/**
	 * Если значение является пустым
	 */
	else if(value.is_null())
		// Выполняем учёт прочитанного пустого значения
		rival::nothing();
}
/**
 * @brief Функция разбора одного документа
 *
 * @param text разбираемый текст документа
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	/**
	 * Выполняем разбор текста документа
	 */
	try {
		// Выполняем разбор текста документа
		const nlohmann::json doc = nlohmann::json::parse(text);
		// Выполняем обход собранного дерева документа
		walk(doc);
	/**
	 * Если разбор текста документа завершился отказом
	 */
	} catch(const std::exception &) {
		// Выводим признак неудачного разбора
		return false;
	}
	// Выводим признак успешного разбора
	return true;
}
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
	return rival::drive(parse, argc, argv);
}
