/**
 * @file pugixml.cpp
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
 * @brief Эталонный стенд сравнения контейнера XML на реализации pugixml
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
#include <pugixml.hpp>

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
static void walk(const pugi::xml_node & node) noexcept {
	/**
	 * Выполняем перебор всех вложенных узлов дерева
	 */
	for(pugi::xml_node item = node.first_child(); item; item = item.next_sibling()){
		/**
		 * Если вложенный узел является узлом разметки
		 */
		if(item.type() == pugi::node_element){
			// Выполняем учёт обработанного узла разметки
			rival::node();
			// Выполняем чтение имени узла разметки
			rival::touch(item.name(), ::strlen(item.name()));
			/**
			 * Выполняем перебор всех атрибутов узла разметки
			 */
			for(pugi::xml_attribute attribute = item.first_attribute(); attribute; attribute = attribute.next_attribute()){
				// Выполняем чтение имени очередного атрибута
				rival::touch(attribute.name(), ::strlen(attribute.name()));
				// Выполняем чтение значения очередного атрибута
				rival::touch(attribute.value(), ::strlen(attribute.value()));
			}
			/**
			 * Выполняем чтение имени узла разметки за его закрывающую метку
			 *
			 * @note Чтение ведётся дважды намеренно: потоковые реализации выдают имя
			 *       и на открывающей метке, и на закрывающей, и без второго чтения
			 *       объём работы стендов разошёлся бы
			 */
			rival::touch(item.name(), ::strlen(item.name()));
			// Выполняем обход вложенных узлов дерева
			walk(item);
		/**
		 * Если вложенный узел является текстовым содержимым
		 */
		} else if((item.type() == pugi::node_pcdata) || (item.type() == pugi::node_cdata))
			// Выполняем учёт содержимого узла разметки
			rival::consume(item.value(), ::strlen(item.value()));
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
	pugi::xml_document document;
	/**
	 * Выполняем разбор текста разметки
	 *
	 * @note Разбор ведётся по копии исходного текста: реализация размещает
	 *       представления прямо в разбираемом буфере и портит его, а эталонный
	 *       текст обязан пережить все прогоны неизменным
	 */
	const pugi::xml_parse_result result = document.load_buffer(text.data(), text.size());
	/**
	 * Если разбор текста разметки выполнить не удалось
	 */
	if(!result)
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева разметки
	walk(document);
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Функция снятия владеющего поддерева с дерева разметки
 *
 * @details Дерево собирается однажды, при прогреве, и замеряется одно лишь снятие
 *          поддерева в собственное дерево: у этой реализации владеющей копией узла
 *          служит узел другого дерева, заведённый `append_copy`
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного снятия
 *
 */
static bool copy(const std::string & text) noexcept {
	// Дерево разметки, с какого снимается поддерево
	static pugi::xml_document document;
	// Текст разметки, каким дерево собрано
	static const std::string * source = nullptr;
	/**
	 * Если дерево разметки ещё не собрано либо собрано иным текстом
	 */
	if(source != &text){
		/**
		 * Если сборка дерева разметки не удалась
		 */
		if(!document.load_buffer(text.data(), text.size()))
			// Выводим признак неудачного снятия
			return false;
		// Запоминаем текст разметки, каким дерево собрано
		source = &text;
	}
	// Дерево разметки, принимающее снятое поддерево
	pugi::xml_document owned;
	// Выполняем снятие поддерева в собственное дерево
	const pugi::xml_node value = owned.append_copy(document.document_element());
	// Выполняем учёт снятого поддерева
	rival::node();
	// Выполняем чтение имени снятого поддерева
	rival::touch(value.name(), ::strlen(value.name()));
	// Выводим признак успешного снятия
	return !value.empty();
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
	/**
	 * Выполняем установку учётного распределителя памяти
	 *
	 * @note Без крючка этого расход памяти отчитывался бы нулём: реализация берёт
	 *       память `malloc`, а не оператором языка, и замене оператора невидима
	 */
	pugi::set_memory_management_functions(rival::capture, rival::release);
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
