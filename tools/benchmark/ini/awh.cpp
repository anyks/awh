/**
 * @file: awh.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения — потоковое чтение текста настроек контейнером AWH
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/ini/reader.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция получения настроек разбора текста настроек
 *
 * @details Выдача примечаний и пустых строк отключена намеренно: сличаемые
 * реализации примечаний не выдают вовсе, и оставлять их включёнными значило бы
 * сравнивать разный объём работы. Чего стоят примечания событиями, показывает
 * набор `benchmark/codec/ini` - там они включены
 *
 * @return настройки разбора текста настроек
 *
 */
static const awh::codec::ini::reader_t::settings_t & settings() noexcept {
	// Настройки разбора текста настроек
	static const awh::codec::ini::reader_t::settings_t result = []() noexcept -> awh::codec::ini::reader_t::settings_t {
		// Собираемые настройки разбора текста настроек
		awh::codec::ini::reader_t::settings_t result;
		// Снимаем выдачу примечаний отдельным событием
		result.emitComments = false;
		// Снимаем выдачу пустых строк отдельным событием
		result.emitBlanks = false;
		// Выводим собранные настройки разбора
		return result;
	}();
	// Выводим настройки разбора текста настроек
	return result;
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект потокового чтения текста настроек
	awh::codec::ini::reader_t reader(settings());
	/**
	 * Если передать текст настроек не удалось
	 */
	if(!reader.feed(text))
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Определяем вид полученного события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			/**
			 * Если получено объявление раздела
			 */
			case static_cast <uint8_t> (awh::codec::ini::event_t::SECTION):
				// Выполняем чтение имени объявленного раздела
				rival::touch(reader.section().section.data(), reader.section().section.size());
			break;
			/**
			 * Если получено свойство со значением
			 */
			case static_cast <uint8_t> (awh::codec::ini::event_t::PROPERTY): {
				// Выполняем учёт обработанного свойства
				rival::entry();
				// Выполняем чтение имени свойства
				rival::touch(reader.key().data(), reader.key().size());
				// Выполняем учёт значения свойства
				rival::consume(reader.text().data(), reader.text().size());
			} break;
		}
	}
	// Выводим признак успешного разбора
	return (reader.state() == awh::codec::ini::state_t::FINISHED);
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
