/**
 * @file yyjson.cpp
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
 *        реализации yyjson с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <yyjson.h>

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
static void walk(yyjson_val * value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(yyjson_get_type(value)){
		// Если значение является пустым
		case YYJSON_TYPE_NULL:
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		// Если значение является логическим
		case YYJSON_TYPE_BOOL:
			// Выполняем учёт прочитанного логического значения
			rival::consume(yyjson_get_bool(value));
		break;
		// Если значение является числом
		case YYJSON_TYPE_NUM:
			// Выполняем учёт прочитанного числа
			rival::consume(yyjson_get_num(value));
		break;
		// Если значение является строкой
		case YYJSON_TYPE_STR:
			// Выполняем учёт прочитанного строкового значения
			rival::consume(yyjson_get_str(value), yyjson_get_len(value));
		break;
		/**
		 * Если значение является массивом
		 */
		case YYJSON_TYPE_ARR: {
			// Обходчик значений массива
			yyjson_arr_iter iter;
			// Выполняем заведение обходчика значений массива
			yyjson_arr_iter_init(value, &iter);
			// Очередное значение массива
			yyjson_val * item = nullptr;
			/**
			 * Выполняем обход всех значений массива
			 */
			while((item = yyjson_arr_iter_next(&iter)) != nullptr)
				// Выполняем обход очередного значения массива
				walk(item);
		} break;
		/**
		 * Если значение является объектом
		 */
		case YYJSON_TYPE_OBJ: {
			// Обходчик полей объекта
			yyjson_obj_iter iter;
			// Выполняем заведение обходчика полей объекта
			yyjson_obj_iter_init(value, &iter);
			// Имя очередного поля объекта
			yyjson_val * name = nullptr;
			/**
			 * Выполняем обход всех полей объекта
			 */
			while((name = yyjson_obj_iter_next(&iter)) != nullptr){
				// Выполняем учёт прочитанного имени поля объекта
				rival::consume(yyjson_get_str(name), yyjson_get_len(name));
				// Выполняем обход значения очередного поля объекта
				walk(yyjson_obj_iter_get_val(name));
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
	// Выполняем разбор текста документа
	yyjson_doc * doc = yyjson_read(text.data(), text.size(), 0);
	/**
	 * Если разбор текста документа выполнить не удалось
	 */
	if(doc == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева документа
	walk(yyjson_doc_get_root(doc));
	// Выполняем освобождение собранного дерева документа
	yyjson_doc_free(doc);
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
