/**
 * @file awh.cpp
 * @date 2026-08-02
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
 * @brief Эталонный стенд сравнения контейнера XML на реализации AWH
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
#include <sys/log.hpp>
#include <codec/xml/reader.hpp>
#include <codec/xml/value.hpp>

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
 * @brief Функция разбора одного документа
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект потокового чтения текста разметки
	awh::codec::xml::reader_t reader(::logger());
	/**
	 * Если передать текст разметки не удалось
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
			 * Если получено начало узла разметки
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::ELEMENT_OPEN): {
				// Выполняем учёт обработанного узла разметки
				rival::node();
				// Выполняем чтение имени узла разметки
				rival::touch(reader.name().local.data(), reader.name().local.size());
				/**
				 * Выполняем перебор всех атрибутов узла разметки
				 */
				for(const awh::codec::xml::attribute_t & attribute : reader.attributes()){
					// Выполняем чтение имени очередного атрибута
					rival::touch(attribute.name.local.data(), attribute.name.local.size());
					// Выполняем чтение значения очередного атрибута
					rival::touch(attribute.value.data(), attribute.value.size());
				}
			} break;
			/**
			 * Если получен конец узла разметки
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::ELEMENT_CLOSE):
				// Выполняем чтение имени узла разметки
				rival::touch(reader.name().local.data(), reader.name().local.size());
			break;
			/**
			 * Если получено текстовое содержимое узла
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::TEXT):
			/**
			 * Если получен раздел дословного текста
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::CDATA):
				// Выполняем учёт содержимого узла разметки
				rival::consume(reader.text().data(), reader.text().size());
			break;
		}
	}
	// Выводим признак успешного разбора по состоянию чтения
	return (reader.state() == awh::codec::xml::state_t::FINISHED);
}

/**
 * @brief Функция снятия владеющего поддерева с дерева разметки
 *
 * @details Дерево собирается однажды, при прогреве, и замеряется одно лишь снятие:
 *          сравниваются модели владения, а не скорость разбора, уже сравнённая выше
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного снятия
 *
 */
static bool copy(const std::string & text) noexcept {
	// Дерево разметки, с какого снимается поддерево
	static awh::codec::xml::document_t document(::logger());
	// Текст разметки, каким дерево собрано
	static const std::string * source = nullptr;
	/**
	 * Если дерево разметки ещё не собрано либо собрано иным текстом
	 */
	if(source != &text){
		/**
		 * Если сборка дерева разметки не удалась
		 */
		if(!document.parse(text))
			// Выводим признак неудачного снятия
			return false;
		// Запоминаем текст разметки, каким дерево собрано
		source = &text;
	}
	// Выполняем снятие владеющего поддерева с дерева разметки
	const awh::codec::xml::value_t value(document.element());
	// Выполняем учёт снятого поддерева
	rival::node();
	// Выполняем чтение имени снятого поддерева
	rival::touch(value.local().data(), value.local().size());
	// Выводим признак успешного снятия
	return value.valid();
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
	// Функция получения разбираемого текста разметки
	const std::string & (* text)() noexcept;
	// Функция разбора одного документа
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
		{"soap",       rival::SMALL_ROUNDS,       rival::soap,       parse},
		{"device",     rival::SMALL_ROUNDS / 4,   rival::device,     parse},
		{"large",      rival::LARGE_ROUNDS,       rival::large,      parse},
		{"attributes", rival::FOCUSED_ROUNDS,     rival::attributes, parse},
		{"content",    rival::FOCUSED_ROUNDS,     rival::content,    parse},
		{"nested",     rival::SMALL_ROUNDS,       rival::nested,     parse},
		{"copy-soap",  rival::SMALL_ROUNDS,       rival::soap,       copy},
		{"copy-large", rival::LARGE_ROUNDS,       rival::large,      copy}
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
	rival::outcome_t outcome{0, 0, 0.0, 0, 0};
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
