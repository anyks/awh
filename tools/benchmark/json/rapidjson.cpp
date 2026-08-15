/**
 * @file rapidjson.cpp
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
 *        реализации RapidJSON с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <rapidjson/document.h>

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
static void walk(const rapidjson::Value & value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(value.GetType()){
		// Если значение является пустым
		case rapidjson::kNullType:
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		// Если значение является ложью
		case rapidjson::kFalseType:
			// Выполняем учёт прочитанного логического значения
			rival::consume(false);
		break;
		// Если значение является истиной
		case rapidjson::kTrueType:
			// Выполняем учёт прочитанного логического значения
			rival::consume(true);
		break;
		// Если значение является числом
		case rapidjson::kNumberType:
			// Выполняем учёт прочитанного числа
			rival::consume(value.GetDouble());
		break;
		// Если значение является строкой
		case rapidjson::kStringType:
			// Выполняем учёт прочитанного строкового значения
			rival::consume(value.GetString(), value.GetStringLength());
		break;
		/**
		 * Если значение является массивом
		 */
		case rapidjson::kArrayType: {
			/**
			 * Выполняем обход всех значений массива
			 */
			for(auto item = value.Begin(); item != value.End(); ++item)
				// Выполняем обход очередного значения массива
				walk(* item);
		} break;
		/**
		 * Если значение является объектом
		 */
		case rapidjson::kObjectType: {
			/**
			 * Выполняем обход всех полей объекта
			 */
			for(auto item = value.MemberBegin(); item != value.MemberEnd(); ++item){
				// Выполняем учёт прочитанного имени поля объекта
				rival::consume(item->name.GetString(), item->name.GetStringLength());
				// Выполняем обход значения очередного поля объекта
				walk(item->value);
			}
		} break;
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
	// Объект документа
	rapidjson::Document doc;
	// Выполняем разбор текста документа
	doc.Parse(text.data(), text.size());
	/**
	 * Если разбор текста документа завершился отказом
	 */
	if(doc.HasParseError())
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева документа
	walk(doc);
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
