/**
 * @file awh-tree.cpp
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
 * @brief Эталонный стенд сравнения — сборка дерева настроек контейнером AWH
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/ini/document.hpp>

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
 * @brief Функция разбора одного файла настроек
 *
 * @details Собранное дерево обходится целиком - по разделам и свойствам каждого
 * из них: сличаемые реализации выдают дерево, и мерить одну лишь его сборку
 * было бы неполно
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Дерево настроек
	awh::codec::ini::document_t document(::logger());
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(!document.parse(text))
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Выполняем перебор всех объявленных разделов
	 */
	for(const awh::codec::ini::name_t & section : document.sections()){
		// Выполняем чтение имени объявленного раздела
		rival::touch(section.section.data(), section.section.size());
		/**
		 * Выполняем перебор всех имён свойств раздела
		 */
		for(const std::string_view & key : document.keys(section.section, section.subsection)){
			// Получаем значение очередного свойства раздела
			const std::string_view value = document.get(key, section.section, section.subsection);
			// Выполняем учёт обработанного свойства
			rival::entry();
			// Выполняем чтение имени свойства
			rival::touch(key.data(), key.size());
			// Выполняем учёт значения свойства
			rival::consume(value.data(), value.size());
		}
	}
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
