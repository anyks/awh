/**
 * @file libxml2.cpp
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
 * @brief Эталонный стенд сравнения контейнера XML на реализации libxml2
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
#include <libxml/parser.h>
#include <libxml/parserInternals.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обработки начала узла разметки
 *
 * @param name       местное имя узла разметки
 * @param count      количество атрибутов узла разметки
 * @param attributes массив описаний атрибутов узла разметки
 *
 */
static void opening(void *, const xmlChar * name, const xmlChar *, const xmlChar *, int32_t, const xmlChar **, int32_t count, int32_t, const xmlChar ** attributes) {
	// Выполняем учёт обработанного узла разметки
	rival::node();
	// Выполняем чтение имени узла разметки
	rival::touch(name, ::strlen(reinterpret_cast <const char *> (name)));
	/**
	 * Выполняем перебор всех атрибутов узла разметки
	 */
	for(int32_t i = 0; i < count; i++){
		// Получаем описание очередного атрибута узла разметки
		const xmlChar ** attribute = (attributes + (i * 5));
		// Выполняем чтение имени очередного атрибута
		rival::touch(attribute[0], ::strlen(reinterpret_cast <const char *> (attribute[0])));
		// Выполняем чтение значения очередного атрибута
		rival::touch(attribute[3], static_cast <size_t> (attribute[4] - attribute[3]));
	}
}
/**
 * @brief Функция обработки конца узла разметки
 *
 * @param name местное имя узла разметки
 *
 */
static void closing(void *, const xmlChar * name, const xmlChar *, const xmlChar *) {
	// Выполняем чтение имени узла разметки
	rival::touch(name, ::strlen(reinterpret_cast <const char *> (name)));
}
/**
 * @brief Функция обработки текстового содержимого узла
 *
 * @param text содержимое узла разметки
 * @param size размер содержимого узла разметки
 *
 */
static void character(void *, const xmlChar * text, int32_t size) {
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
	// Обработчики разбора текста разметки
	static xmlSAXHandler handler = []() noexcept -> xmlSAXHandler {
		// Собираемые обработчики разбора
		xmlSAXHandler result;
		// Выполняем обнуление обработчиков разбора
		::memset(&result, 0, sizeof(result));
		// Устанавливаем издание обработчиков с разрешением пространств имён
		result.initialized = XML_SAX2_MAGIC;
		// Устанавливаем обработчик начала узла разметки
		result.startElementNs = opening;
		// Устанавливаем обработчик конца узла разметки
		result.endElementNs = closing;
		// Устанавливаем обработчик текстового содержимого узла
		result.characters = character;
		// Выводим собранные обработчики разбора
		return result;
	}();
	// Создаём объект разбора текста разметки
	xmlParserCtxtPtr context = ::xmlCreateMemoryParserCtxt(text.data(), static_cast <int32_t> (text.size()));
	/**
	 * Если объект разбора создать не удалось
	 */
	if(context == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Устанавливаем обработчики разбора текста разметки
	context->sax = &handler;
	// Выполняем разбор текста разметки
	::xmlParseDocument(context);
	// Получаем признак правильного построения текста разметки
	const bool result = (context->wellFormed != 0);
	// Снимаем обработчики разбора во избежание их освобождения
	context->sax = nullptr;
	// Выполняем освобождение объекта разбора
	::xmlFreeParserCtxt(context);
	// Выводим признак успешного разбора
	return result;
}

/**
 * @brief Функция снятия владеющего поддерева с дерева разметки
 *
 * @details Дерево собирается однажды, при прогреве, и замеряется одно лишь снятие
 *          поддерева глубокой копией. У этой реализации снятие владеющее: `xmlCopyNode`
 *          переносит и знаки имён с содержимым, а не одни указания на них
 *
 * @note Прочие сценарии стенда идут потоковой выдачей SAX2, дерева не заводя вовсе.
 *       Здесь дерево заводится намеренно: сравнивается снятие поддерева, а его без
 *       дерева не бывает
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного снятия
 *
 */
static bool copy(const std::string & text) noexcept {
	// Дерево разметки, с какого снимается поддерево
	static xmlDocPtr document = nullptr;
	// Текст разметки, каким дерево собрано
	static const std::string * source = nullptr;
	/**
	 * Если дерево разметки ещё не собрано либо собрано иным текстом
	 */
	if(source != &text){
		/**
		 * Если дерево разметки уже собрано прежним текстом
		 */
		if(document != nullptr)
			// Выполняем снос прежде собранного дерева разметки
			::xmlFreeDoc(document);
		// Выполняем сборку дерева разметки
		document = ::xmlReadMemory(text.data(), static_cast <int32_t> (text.size()), nullptr, nullptr, 0);
		/**
		 * Если сборка дерева разметки не удалась
		 */
		if(document == nullptr)
			// Выводим признак неудачного снятия
			return false;
		// Запоминаем текст разметки, каким дерево собрано
		source = &text;
	}
	// Получаем корневой узел разметки собранного дерева
	xmlNodePtr root = ::xmlDocGetRootElement(document);
	/**
	 * Если корневого узла разметки у дерева нет
	 */
	if(root == nullptr)
		// Выводим признак неудачного снятия
		return false;
	// Выполняем снятие поддерева глубокой копией
	xmlNodePtr value = ::xmlCopyNode(root, 1);
	/**
	 * Если снятие поддерева не удалось
	 */
	if(value == nullptr)
		// Выводим признак неудачного снятия
		return false;
	// Выполняем учёт снятого поддерева
	rival::node();
	// Выполняем чтение имени снятого поддерева
	rival::touch(reinterpret_cast <const char *> (value->name), ::strlen(reinterpret_cast <const char *> (value->name)));
	// Выполняем снос снятого поддерева
	::xmlFreeNode(value);
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
	/**
	 * Выполняем установку учётного распределителя памяти
	 *
	 * @note Без крючка этого расход памяти отчитывался бы нулём: реализация написана
	 *       на языке Си и берёт память `malloc`, а не оператором языка
	 */
	if(::xmlMemSetup(rival::release, rival::capture, rival::recapture, rival::duplicate) != 0)
		/**
		 * Запоминаем, что учёт выделений памяти неработоспособен
		 *
		 * @note Отказ этот - свойство сборки libxml2, а не стенда: крючки памяти
		 *       принимаются лишь до заведения разбора, а собранная в состав системы
		 *       библиотека заводит его прежде. Расход её отчитывается «не мерено», и
		 *       нулём его выдавать нельзя - ноль означал бы отсутствие расхода
		 */
		rival::counter.available = false;
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
