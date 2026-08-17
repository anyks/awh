/**
 * @file libfyaml.cpp
 * @date 2026-08-17
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
 * @brief Эталонный стенд сравнения — потоковое чтение текста настроек реализацией libfyaml
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 *
 * @note Общее окружение подключается прежде заголовочного файла сравниваемой
 *       реализации намеренно: libfyaml тянет за собою `stdatomic.h`, а тот у
 *       стандартной библиотеки libc++ несовместим с `atomic` прежде наречия C++23.
 *       Подключение окружения первым выводит `atomic` до появления `stdatomic.h` и
 *       тем снимает столкновение
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <libfyaml.h>

/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Содержимое скалярного значения выдаётся отдельным вызовом, а не полем
 * события: реализация хранит значение приметой на исходный текст и приводит его к
 * окончательному виду лишь по требованию. Требование это стенд предъявляет всякому
 * значению - иначе работа по снятию ограды и разбору отменяющих последовательностей
 * попросту не выполнялась бы, и сравнивался бы неполный разбор с полным
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Параметры разбора текста настроек
	struct fy_parse_cfg config;
	// Выполняем сброс параметров разбора текста настроек
	::memset(&config, 0, sizeof(config));
	// Объект разбора текста настроек
	struct fy_parser * parser = fy_parser_create(&config);
	/**
	 * Если завести объект разбора не удалось
	 */
	if(parser == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Признак успешного разбора текста настроек
	bool result = true;
	/**
	 * Если передать разбираемый текст настроек не удалось
	 */
	if(fy_parser_set_string(parser, text.data(), text.size()) != 0)
		// Запоминаем признак неудачного разбора
		result = false;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(result){
		// Получаем очередное событие разбора
		struct fy_event * event = fy_parser_parse(parser);
		/**
		 * Если событий разбора больше нет
		 */
		if(event == nullptr){
			/**
			 * Если разбор прекращён отказом
			 */
			if(fy_parser_get_stream_error(parser))
				// Запоминаем признак неудачного разбора
				result = false;
			// Выходим из перебора событий разбора
			break;
		}
		/**
		 * Если получено скалярное значение
		 */
		if(event->type == FYET_SCALAR){
			// Получаем примету скалярного значения
			struct fy_token * token = fy_event_get_token(event);
			/**
			 * Если примета скалярного значения получена
			 */
			if(token != nullptr){
				// Размер содержимого скалярного значения
				size_t size = 0;
				// Получаем содержимое скалярного значения
				const char * buffer = fy_token_get_text(token, &size);
				// Выполняем учёт обработанного скалярного значения
				rival::entry();
				// Выполняем учёт содержимого скалярного значения
				rival::consume(buffer, size);
			}
		}
		// Выполняем освобождение памяти очередного события разбора
		fy_parser_event_free(parser, event);
	}
	// Выполняем освобождение памяти объекта разбора
	fy_parser_destroy(parser);
	// Выводим признак успешного разбора
	return result;
}

/**
 * @brief Перечень сценариев стенда
 *
 */
static const rival::scenario_t SCENARIOS[] = {
	{"service",   rival::SMALL_ROUNDS,   rival::service,   parse},
	{"large",     rival::LARGE_ROUNDS,   rival::large,     parse},
	{"strings",   rival::FOCUSED_ROUNDS, rival::strings,   parse},
	{"numbers",   rival::FOCUSED_ROUNDS, rival::numbers,   parse},
	{"arrays",    rival::FOCUSED_ROUNDS, rival::arrays,    parse},
	{"blocks",    rival::FOCUSED_ROUNDS, rival::blocks,    parse},
	{"anchors",   rival::FOCUSED_ROUNDS, rival::anchors,   parse},
	{"decorated", rival::FOCUSED_ROUNDS, rival::decorated, parse}
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
