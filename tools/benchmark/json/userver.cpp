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
 * @brief Эталонный стенд сравнения контейнера JSON — разбор документа средствами
 *        реализации userver с полным обходом собранного дерева
 *
 * @details Реализация userver собственного разбора JSON не несёт: значение её
 *          построено поверх RapidJSON, и потому стенд этот меряет не столько
 *          разбор, сколько слой значения, положенный поверх чужого двигателя.
 *          Обход ведётся нарочно публичным обращением, а не доступом к родному
 *          значению: родное значение меряло бы RapidJSON, какой уже снят своим
 *          стендом
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <userver/formats/json.hpp>

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
static void walk(const userver::formats::json::Value & value) noexcept {
	/**
	 * Если значение является объектом
	 */
	if(value.IsObject()){
		/**
		 * Выполняем обход всех полей объекта
		 */
		for(const auto & item : userver::formats::json::Items(value)){
			// Выполняем учёт прочитанного имени поля объекта
			rival::consume(item.key.data(), item.key.size());
			// Выполняем обход значения очередного поля объекта
			walk(item.value);
		}
	/**
	 * Если значение является массивом
	 */
	} else if(value.IsArray()){
		/**
		 * Выполняем обход всех значений массива
		 */
		for(const auto & item : value)
			// Выполняем обход очередного значения массива
			walk(item);
	/**
	 * Если значение является строкой
	 */
	} else if(value.IsString()){
		// Строковое значение документа
		const auto text = value.As <std::string> ();
		// Выполняем учёт прочитанного строкового значения
		rival::consume(text.data(), text.size());
	/**
	 * Если значение является числом
	 */
	} else if(value.IsNumber()){
		// Выполняем учёт прочитанного числа
		rival::consume(value.As <double> ());
	/**
	 * Если значение является пустым
	 */
	} else if(value.IsNull()){
		// Выполняем учёт прочитанного пустого значения
		rival::nothing();
	/**
	 * Если значение является логическим
	 */
	} else if(value.IsBool()){
		// Выполняем учёт прочитанного логического значения
		rival::consume(value.As <bool> ());
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
	/**
	 * Выполняем перехват ошибок разбора текста документа
	 */
	try {
		// Значение разобранного документа
		const auto value = userver::formats::json::FromString(text);
		// Выполняем обход собранного дерева документа
		walk(value);
	/**
	 * Если разбор текста документа прекращён отказом
	 */
	} catch(const std::exception &){
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
