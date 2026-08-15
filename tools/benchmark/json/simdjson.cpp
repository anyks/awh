/**
 * @file simdjson.cpp
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
 *        реализации simdjson с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <simdjson.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обхода собранного дерева документа
 *
 * @details Разбор здесь отложенный: дерево строится по мере обхода, и стоимость его
 * входит в стоимость обхода, а не разбора. Оттого стенд сличается по разбору вместе
 * с обходом - разбор без обхода у этой реализации не делает почти ничего
 *
 * @param value обходимое значение документа
 *
 */
static void walk(simdjson::ondemand::value value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(value.type().value_unsafe()){
		/**
		 * Если значение является массивом
		 */
		case simdjson::ondemand::json_type::array: {
			/**
			 * Выполняем обход всех значений массива
			 */
			for(auto item : value.get_array().value_unsafe())
				// Выполняем обход очередного значения массива
				walk(item.value_unsafe());
		} break;
		/**
		 * Если значение является объектом
		 */
		case simdjson::ondemand::json_type::object: {
			/**
			 * Выполняем обход всех полей объекта
			 */
			for(auto item : value.get_object().value_unsafe()){
				// Получаем имя очередного поля объекта
				const std::string_view name = item.unescaped_key().value_unsafe();
				// Выполняем учёт прочитанного имени поля объекта
				rival::consume(name.data(), name.size());
				// Выполняем обход значения очередного поля объекта
				walk(item.value());
			}
		} break;
		/**
		 * Если значение является числом
		 */
		case simdjson::ondemand::json_type::number:
			// Выполняем учёт прочитанного числа
			rival::consume(value.get_double().value_unsafe());
		break;
		/**
		 * Если значение является строкой
		 */
		case simdjson::ondemand::json_type::string: {
			// Получаем прочитанное строковое значение
			const std::string_view result = value.get_string().value_unsafe();
			// Выполняем учёт прочитанного строкового значения
			rival::consume(result.data(), result.size());
		} break;
		// Если значение является логическим
		case simdjson::ondemand::json_type::boolean:
			// Выполняем учёт прочитанного логического значения
			rival::consume(value.get_bool().value_unsafe());
		break;
		// Если значение является пустым
		case simdjson::ondemand::json_type::null:
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		/**
		 * Если вид значения не опознан
		 */
		default: break;
	}
}
/**
 * @brief Функция разбора одного документа
 *
 * @param text разбираемый текст документа
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект разбора текста документа
	static simdjson::ondemand::parser parser;
	// Приведённый к требованиям реализации текст документа
	static simdjson::padded_string padded;
	/**
	 * Если приведённый текст документа ещё не собран либо сменился
	 *
	 * @note Приведение это - требование реализации: она читает текст словами и
	 *       требует запаса за концом его. Собирается оно единожды на документ,
	 *       и стоимость его в замер намеренно не входит
	 */
	if(padded.size() != text.size())
		// Выполняем сборку приведённого текста документа
		padded = simdjson::padded_string(text);
	// Собранное дерево документа
	simdjson::ondemand::document doc;
	/**
	 * Если разбор текста документа выполнить не удалось
	 */
	if(parser.iterate(padded).get(doc) != simdjson::SUCCESS)
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Если корнем документа является одиночное значение
	 */
	if(doc.is_scalar().value_unsafe()){
		// Выполняем учёт прочитанного одиночного значения
		rival::nothing();
		// Выводим признак успешного разбора
		return true;
	}
	// Выполняем обход собранного дерева документа
	walk(doc.get_value().value_unsafe());
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
