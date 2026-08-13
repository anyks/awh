/**
 * @file: fast-cpp.cpp
 * @date: 2026-08-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения контейнера CSV на реализации Fast C++ CSV Parser
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <vector>
#include <utility>

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
#include <csv.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Шаблон количества столбцов разбираемой таблицы
 *
 * @tparam COLUMNS количество столбцов разбираемой таблицы
 *
 */
template <uint32_t COLUMNS>
/**
 * @brief Тип разбора текста таблицы с отменой кавычек удвоением
 *
 * @note Обвязка не снимается: прочие стенды её сохраняют, и снятие здесь означало бы
 * иной объём работы, а не иную скорость
 *
 */
using reader_t = io::CSVReader <
	COLUMNS,
	io::trim_chars <>,
	io::double_quote_escape <',', '"'>
>;

/**
 * @brief Шаблон количества столбцов разбираемой таблицы
 *
 * @tparam COLUMNS количество столбцов разбираемой таблицы
 * @tparam INDICES порядковые номера полей записи
 *
 */
template <uint32_t COLUMNS, size_t... INDICES>
/**
 * @brief Метод чтения одной записи таблицы
 *
 * @details Реализация выдаёт поля записи отдельными доводами, а не перечнем: количество
 * их известно во время сборки, и развернуть их в довод за доводом иначе нельзя
 *
 * @param reader чтение текста таблицы
 * @param values приёмники содержимого полей записи
 * @return       признак прочтения очередной записи
 *
 */
static inline bool line(reader_t <COLUMNS> & reader, std::array <std::string, COLUMNS> & values, std::index_sequence <INDICES...>) {
	// Выводим признак прочтения очередной записи
	return reader.read_row(values[INDICES]...);
}

/**
 * @brief Шаблон количества столбцов разбираемой таблицы
 *
 * @tparam COLUMNS количество столбцов разбираемой таблицы
 *
 */
template <uint32_t COLUMNS>
/**
 * @brief Функция разбора одной таблицы
 *
 * @details Первая запись таблицы читается наравне с прочими: заголовка реализация без
 * особого указания не выделяет, и прочие стенды его тоже не выделяют
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
		// Объект разбора текста таблицы
		reader_t <COLUMNS> reader("rival", text.data(), text.data() + text.size());
		// Приёмники содержимого полей записи
		std::array <std::string, COLUMNS> values;
		/**
		 * Выполняем перебор всех записей таблицы
		 */
		while(line <COLUMNS> (reader, values, std::make_index_sequence <COLUMNS> {})){
			/**
			 * Выполняем перебор всех полей записи
			 */
			for(const std::string & value : values)
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
	// Причина пропуска сценария, пусто - сценарий выполняется
	const char * reason;
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
		{"narrow",    rival::LARGE_ROUNDS,   rival::narrow,    parse <5>,                  nullptr},
		{"wide",      rival::LARGE_ROUNDS,   rival::wide,      parse <rival::WIDE_COLUMNS>, nullptr},
		{"quoted",    rival::FOCUSED_ROUNDS, rival::quoted,    parse <4>,                  nullptr},
		/**
		 * Записи, занимающие по нескольку строк, реализация не разбирает: о том сказано
		 * прямо в её описании - «Quoted strings may not contain unescaped newlines»
		 */
		{"multiline", rival::FOCUSED_ROUNDS, rival::multiline, parse <3>,                  "no support for newlines inside quoted fields"},
		{"small",     rival::SMALL_ROUNDS,   rival::small,     parse <5>,                  nullptr}
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
		 * Если сценарий реализации не по силам
		 */
		if(scenario.reason != nullptr){
			// Выводим сообщение о пропуске сценария
			rival::skip(scenario.name, scenario.reason);
			// Выполняем переход к следующему сценарию
			continue;
		}
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
