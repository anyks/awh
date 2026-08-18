/**
 * @file expat.cpp
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
 * @brief Эталонный стенд сравнения контейнера XML на реализации Expat
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
#include <expat.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обработки начала узла разметки
 *
 * @param name       имя узла разметки
 * @param attributes массив имён и значений атрибутов узла
 *
 */
static void XMLCALL opening(void *, const XML_Char * name, const XML_Char ** attributes) {
	// Выполняем учёт обработанного узла разметки
	rival::node();
	// Выполняем чтение имени узла разметки
	rival::touch(name, ::strlen(name));
	/**
	 * Выполняем перебор всех атрибутов узла разметки
	 */
	for(size_t i = 0; attributes[i] != nullptr; i++)
		// Выполняем чтение имени либо значения очередного атрибута
		rival::touch(attributes[i], ::strlen(attributes[i]));
}
/**
 * @brief Функция обработки конца узла разметки
 *
 * @param name имя узла разметки
 *
 */
static void XMLCALL closing(void *, const XML_Char * name) {
	// Выполняем чтение имени узла разметки
	rival::touch(name, ::strlen(name));
}
/**
 * @brief Функция обработки текстового содержимого узла
 *
 * @param text содержимое узла разметки
 * @param size размер содержимого узла разметки
 *
 */
static void XMLCALL character(void *, const XML_Char * text, int32_t size) {
	// Выполняем учёт содержимого узла разметки
	rival::consume(text, static_cast <size_t> (size));
}
/**
 * @brief Функция разбора одного документа
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	/**
	 * Создаём объект разбора с разрешением пространств имён
	 *
	 * @note Разделитель пространства имён задан намеренно: без него разбор
	 *       пространства имён не разрешает, и работа стенда оказалась бы меньше
	 *       работы прочих реализаций
	 */
	/**
	 * Набор средств выделения памяти с учётом расхода
	 *
	 * @note Без него расход отчитывался бы нулём: реализация написана на языке Си и
	 *       берёт память `malloc`, а не оператором языка. Ноль же означал бы отсутствие
	 *       расхода, и выдавать «не мерено» за «не берёт» нельзя
	 */
	static XML_Memory_Handling_Suite memory = {rival::capture, rival::recapture, rival::release};
	// Объект разбора текста разметки с учётом расхода памяти
	XML_Parser parser = ::XML_ParserCreate_MM(nullptr, &memory, " ");
	/**
	 * Если объект разбора создать не удалось
	 */
	if(parser == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Устанавливаем обработчики начала и конца узла разметки
	::XML_SetElementHandler(parser, opening, closing);
	// Устанавливаем обработчик текстового содержимого узла
	::XML_SetCharacterDataHandler(parser, character);
	// Выполняем разбор текста разметки
	const bool result = (::XML_Parse(parser, text.data(), static_cast <int32_t> (text.size()), 1) == XML_STATUS_OK);
	// Выполняем освобождение объекта разбора
	::XML_ParserFree(parser);
	// Выводим признак успешного разбора
	return result;
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
		{"copy-soap",  rival::SMALL_ROUNDS,       rival::soap,       nullptr},
		{"copy-large", rival::LARGE_ROUNDS,       rival::large,      nullptr}
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
		/**
		 * Если сценарий этой реализации неведом вовсе
		 *
		 * @note Снятия владеющего поддерева у потоковой выдачи не бывает: дерева она не
		 *       заводит, и снимать нечего. Сценарий назван и пропущен явно, а не изъят
		 *       молча: молчание в отчёте неотличимо от недосмотра
		 */
		if(scenario.subject == nullptr){
			// Выводим сообщение о пропуске сценария
			rival::skip(scenario.name, "no tree: streaming only");
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
