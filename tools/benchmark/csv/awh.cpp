/**
 * @file awh.cpp
 * @date 2026-08-13
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
 * @brief Эталонный стенд сравнения контейнера CSV на реализации AWH
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
#include <codec/csv/reader.hpp>

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
 * @brief Функция разбора одной таблицы
 *
 * @param text разбираемый текст таблицы
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект потокового чтения текста таблицы
	awh::codec::csv::reader_t reader(::logger());
	// Выполняем подачу текста таблицы разбору
	reader.feed(text.data(), text.size(), true);
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Определяем вид полученного события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			// Если получено очередное поле записи
			case static_cast <uint8_t> (awh::codec::csv::event_t::FIELD):
				// Выполняем учёт содержимого поля записи
				rival::consume(reader.field().value.data(), reader.field().value.size());
			break;
			// Если получен конец записи
			case static_cast <uint8_t> (awh::codec::csv::event_t::RECORD):
				// Выполняем учёт обработанной записи
				rival::record();
			break;
		}
	}
	// Выводим признак успешного разбора по коду ошибки
	return (reader.error() == awh::codec::csv::error_t::NONE);
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
