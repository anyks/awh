/**
 * @file rapidxml.cpp
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
 * @brief Эталонный стенд сравнения контейнера XML на реализации RapidXML
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Заголовочные файлы сравниваемой реализации
 */
#include <rapidxml.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обхода собранного дерева разметки
 *
 * @param node обходимый узел дерева разметки
 *
 */
static void walk(const rapidxml::xml_node <char> * node) noexcept {
	/**
	 * Выполняем перебор всех вложенных узлов дерева
	 */
	for(const rapidxml::xml_node <char> * item = node->first_node(); item != nullptr; item = item->next_sibling()){
		/**
		 * Если вложенный узел является узлом разметки
		 */
		if(item->type() == rapidxml::node_element){
			// Выполняем учёт обработанного узла разметки
			rival::node();
			// Выполняем чтение имени узла разметки
			rival::touch(item->name(), item->name_size());
			/**
			 * Выполняем перебор всех атрибутов узла разметки
			 */
			for(const rapidxml::xml_attribute <char> * attribute = item->first_attribute(); attribute != nullptr; attribute = attribute->next_attribute()){
				// Выполняем чтение имени очередного атрибута
				rival::touch(attribute->name(), attribute->name_size());
				// Выполняем чтение значения очередного атрибута
				rival::touch(attribute->value(), attribute->value_size());
			}
			/**
			 * Выполняем чтение имени узла разметки за его закрывающую метку
			 */
			rival::touch(item->name(), item->name_size());
			// Выполняем обход вложенных узлов дерева
			walk(item);
		/**
		 * Если вложенный узел является текстовым содержимым
		 */
		} else if((item->type() == rapidxml::node_data) || (item->type() == rapidxml::node_cdata))
			// Выполняем учёт содержимого узла разметки
			rival::consume(item->value(), item->value_size());
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
	// Собираемое дерево разметки
	rapidxml::xml_document <char> document;
	/**
	 * Копия разбираемого текста разметки
	 *
	 * @note Разбор ведётся по копии намеренно: реализация размещает представления
	 *       прямо в разбираемом буфере и портит его, а эталонный текст обязан
	 *       пережить все прогоны неизменным
	 */
	static std::vector <char> buffer;
	// Отводим место под копию разбираемого текста разметки
	buffer.assign(text.begin(), text.end());
	// Выполняем завершение копии разбираемого текста разметки
	buffer.push_back('\0');
	/**
	 * Выполняем разбор текста разметки
	 */
	try {
		// Выполняем разбор текста разметки
		document.parse <0> (buffer.data());
	/**
	 * Если разбор текста разметки выбросил исключение
	 */
	} catch(const rapidxml::parse_error &) {
		// Выводим признак неудачного разбора
		return false;
	}
	// Выполняем обход собранного дерева разметки
	walk(&document);
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Функция снятия владеющего поддерева с дерева разметки
 *
 * @details Дерево собирается однажды, при прогреве, и замеряется одно лишь снятие
 *          поддерева в собственное дерево
 *
 * @warning Снятие это владеющим **не является**: `clone_node` переносит указания на
 *          знаки исходного буфера, а не сами знаки, и снятое поддерево живёт ровно
 *          столько, сколько живёт разбираемый текст. Равным участником этого
 *          сценария реализация не является - приведена отметкой потолка
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного снятия
 *
 */
static bool copy(const std::string & text) noexcept {
	// Дерево разметки, с какого снимается поддерево
	static rapidxml::xml_document <char> document;
	// Копия разбираемого текста разметки, дереву принадлежащая
	static std::vector <char> storage;
	// Текст разметки, каким дерево собрано
	static const std::string * source = nullptr;
	/**
	 * Если дерево разметки ещё не собрано либо собрано иным текстом
	 */
	if(source != &text){
		// Отводим место под копию разбираемого текста разметки
		storage.assign(text.begin(), text.end());
		// Выполняем завершение копии разбираемого текста разметки
		storage.push_back('\0');
		/**
		 * Выполняем разбор текста разметки
		 */
		try {
			// Выполняем разбор текста разметки
			document.parse <0> (storage.data());
		/**
		 * Если разбор текста разметки выбросил исключение
		 */
		} catch(const rapidxml::parse_error &) {
			// Выводим признак неудачного снятия
			return false;
		}
		// Запоминаем текст разметки, каким дерево собрано
		source = &text;
	}
	// Дерево разметки, принимающее снятое поддерево
	rapidxml::xml_document <char> owned;
	// Выполняем снятие поддерева в собственное дерево
	rapidxml::xml_node <char> * value = owned.clone_node(document.first_node());
	/**
	 * Если снятие поддерева не удалось
	 */
	if(value == nullptr)
		// Выводим признак неудачного снятия
		return false;
	// Выполняем добавление снятого поддерева к принимающему дереву
	owned.append_node(value);
	// Выполняем учёт снятого поддерева
	rival::node();
	// Выполняем чтение имени снятого поддерева
	rival::touch(value->name(), value->name_size());
	// Выводим признак успешного снятия
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
