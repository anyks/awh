/**
 * @file boost.cpp
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
 *        реализации Boost.JSON с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <boost/json.hpp>

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
static void walk(const boost::json::value & value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(value.kind()){
		// Если значение является пустым
		case boost::json::kind::null:
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		// Если значение является логическим
		case boost::json::kind::bool_:
			// Выполняем учёт прочитанного логического значения
			rival::consume(value.get_bool());
		break;
		// Если значение является целым числом со знаком
		case boost::json::kind::int64:
			// Выполняем учёт прочитанного числа
			rival::consume(static_cast <double> (value.get_int64()));
		break;
		// Если значение является целым числом без знака
		case boost::json::kind::uint64:
			// Выполняем учёт прочитанного числа
			rival::consume(static_cast <double> (value.get_uint64()));
		break;
		// Если значение является числом с плавающей запятой
		case boost::json::kind::double_:
			// Выполняем учёт прочитанного числа
			rival::consume(value.get_double());
		break;
		/**
		 * Если значение является строкой
		 */
		case boost::json::kind::string: {
			// Получаем прочитанное строковое значение
			const boost::json::string & result = value.get_string();
			// Выполняем учёт прочитанного строкового значения
			rival::consume(result.data(), result.size());
		} break;
		/**
		 * Если значение является массивом
		 */
		case boost::json::kind::array: {
			/**
			 * Выполняем обход всех значений массива
			 */
			for(const auto & item : value.get_array())
				// Выполняем обход очередного значения массива
				walk(item);
		} break;
		/**
		 * Если значение является объектом
		 */
		case boost::json::kind::object: {
			/**
			 * Выполняем обход всех полей объекта
			 */
			for(const auto & item : value.get_object()){
				// Выполняем учёт прочитанного имени поля объекта
				rival::consume(item.key().data(), item.key().size());
				// Выполняем обход значения очередного поля объекта
				walk(item.value());
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
	/**
	 * Выполняем разбор текста документа
	 */
	try {
		/**
		 * Выделение памяти ведётся однонаправленным распределителем
		 *
		 * @note Распределитель этот - главное, чем Boost.JSON быстрее прочих
		 *       реализаций с деревом объектов, и стенд без него сличал бы не
		 *       реализацию, а способ её употребления
		 */
		boost::json::monotonic_resource resource;
		/**
		 * Настройки разбора текста документа
		 *
		 * @note Наибольшая глубина вложенности поднята с умолчания намеренно: у этой
		 *       реализации она равна тридцати двум, а эталонный документ с вложенностью
		 *       глубже. Оставить умолчание значило бы сличать не скорость разбора, а
		 *       настройку по умолчанию
		 */
		boost::json::parse_options options;
		// Устанавливаем наибольшую допустимую глубину вложенности
		options.max_depth = 1024;
		// Выполняем разбор текста документа
		const boost::json::value doc = boost::json::parse(text, &resource, options);
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
