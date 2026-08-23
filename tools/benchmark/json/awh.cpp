/**
 * @file awh.cpp
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
 *        библиотеки AWH с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <codec/json/document.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}


/**
 * @brief Функция обхода собранного дерева документа
 *
 * @param value обходимое значение документа
 *
 */
static void walk(const awh::codec::json::document_t::value_t & value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(static_cast <uint8_t> (value.kind())){
		// Если значение является пустым
		case static_cast <uint8_t> (awh::codec::json::kind_t::NUL):
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		/**
		 * Если значение является логическим
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::BOOL): {
			// Прочитанное логическое значение
			bool result = false;
			// Выполняем извлечение логического значения
			value.value(result);
			// Выполняем учёт прочитанного логического значения
			rival::consume(result);
		} break;
		/**
		 * Если значение является числом
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::NUMBER): {
			// Прочитанное число
			double result = 0.;
			// Выполняем извлечение числа
			value.value(result);
			// Выполняем учёт прочитанного числа
			rival::consume(result);
		} break;
		/**
		 * Если значение является строкой
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::STRING): {
			// Получаем прочитанное строковое значение
			const std::string_view result = value.text();
			// Выполняем учёт прочитанного строкового значения
			rival::consume(result.data(), result.size());
		} break;
		/**
		 * Если значение является вместилищем
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::ARRAY):
		case static_cast <uint8_t> (awh::codec::json::kind_t::OBJECT): {
			/**
			 * Выполняем обход всех значений вместилища
			 */
			for(auto item = value.begin(); item.valid(); item = item.next()){
				// Получаем имя поля объекта
				const std::string_view name = item.name();
				/**
				 * Если значение является полем объекта
				 */
				if(!name.empty())
					// Выполняем учёт прочитанного имени поля объекта
					rival::consume(name.data(), name.size());
				// Выполняем обход очередного значения вместилища
				walk(item);
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
	awh::codec::json::document_t doc(::logger());
	/**
	 * Если разбор текста документа выполнить не удалось
	 */
	if(!doc.parse(text))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева документа
	walk(doc.root());
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
