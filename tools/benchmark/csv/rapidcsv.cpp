/**
 * @file: rapidcsv.cpp
 * @date: 2026-08-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения контейнера CSV на реализации rapidcsv
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <sstream>

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
#include <rapidcsv.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция разбора одной таблицы
 *
 * @details Реализация держит таблицу целиком и выдаёт поля обращением по номеру: это
 * иная задача, нежели у потоковых стендов, и мерится здесь она целиком - разбор вместе
 * с обходом собранного, иначе часть работы осталась бы не выполненной
 *
 * @note Таблица подаётся потоком в памяти, а не файлом: чтение с диска замерялось бы
 * наравне с разбором, и сравнение вышло бы о скорости диска
 *
 * @param text разбираемый текст таблицы
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	/**
	 * Выполняем разбор текста таблицы
	 */
	try {
		// Поток разбираемого текста таблицы
		std::istringstream stream(text);
		// Объект контейнера таблицы без выделения заголовка и нумерации записей
		rapidcsv::Document document(stream,
			rapidcsv::LabelParams(-1, -1),
			/**
			 * Знак-разделитель полей, сохранение обвязки, возврат каретки в конце
			 * записи, признание переводов строки внутри кавычек и снятие отмены кавычек
			 */
			rapidcsv::SeparatorParams(',', false, true, true, true)
		);
		// Получаем количество записей таблицы
		const size_t rows = document.GetRowCount();
		/**
		 * Выполняем перебор всех записей таблицы
		 */
		for(size_t i = 0; i < rows; i++){
			// Получаем очередную запись таблицы
			const std::vector <std::string> row = document.GetRow <std::string> (i);
			/**
			 * Выполняем перебор всех полей записи
			 */
			for(const std::string & value : row)
				// Выполняем учёт содержимого поля записи
				rival::consume(value.data(), value.size());
			// Выполняем учёт обработанной записи
			rival::record();
		}
	/**
	 * Если разбор текста таблицы отвергнут
	 */
	} catch(const std::exception &) {
		// Выводим признак неудачного разбора
		return false;
	}
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Структура сценария стенда
 *
 */
struct scenario_t {
	// Название сценария
	const char * name;
	// Количество прогонов разбора
	size_t rounds;
	// Функция получения разбираемого текста таблицы
	const std::string & (* text)() noexcept;
	// Функция разбора одной таблицы
	bool (* subject)(const std::string &) noexcept;
};

/**
 * @brief Метод получения перечня сценариев стенда
 *
 * @return перечень сценариев стенда
 *
 */
static const std::vector <scenario_t> & scenarios() noexcept {
	// Перечень сценариев стенда
	static const std::vector <scenario_t> result = {
		{"narrow",    rival::LARGE_ROUNDS,   rival::narrow,    parse},
		{"wide",      rival::LARGE_ROUNDS,   rival::wide,      parse},
		{"quoted",    rival::FOCUSED_ROUNDS, rival::quoted,    parse},
		{"multiline", rival::FOCUSED_ROUNDS, rival::multiline, parse},
		{"small",     rival::SMALL_ROUNDS,   rival::small,     parse}
	};
	// Выводим перечень сценариев стенда
	return result;
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
	// Получаем отбор сценариев по вхождению в название
	const char * filter = rival::filter(argc, argv);
	// Итоги прогона сценария
	rival::outcome_t outcome{0, 0, 0.0};
	/**
	 * Выполняем перебор всех сценариев стенда
	 */
	for(const auto & scenario : scenarios()){
		/**
		 * Если сценарий отбором не выбран
		 */
		if(!rival::selected(scenario.name, filter))
			// Выполняем переход к следующему сценарию
			continue;
		/**
		 * Если прогон сценария выполнить не удалось
		 */
		if(!rival::parsing(scenario.subject, scenario.text(), scenario.rounds, outcome)){
			// Выводим сообщение о пропуске сценария
			rival::skip(scenario.name, "parsing failed");
			// Выполняем переход к следующему сценарию
			continue;
		}
		// Выводим результат прогона сценария
		rival::report(scenario.name, outcome);
	}
	// Выводим контрольную сумму работы, выполненной стендом
	rival::digest(argc, argv);
	// Выводим успешный код выхода из стенда
	return EXIT_SUCCESS;
}
