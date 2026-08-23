/**
 * @file ablation.cpp
 * @date 2026-08-10
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
 * @brief Стенд поэлементного снятия возможностей чтения текста настроек
 *
 * @details Стенд отвечает на вопрос, куда уходит время чтения: тот же текст
 *          читается несколько раз, и всякий раз снимается одна из возможностей,
 *          которых у сличаемых реализаций нет. Разница между прогонами и есть
 *          цена снятой возможности
 *
 * @copyright Copyright © 2026
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
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: посредник этот нужен одному лишь
 *       файлу, и вынос его наружу связал бы стенды между собою без нужды
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
 * @brief Разбираемый в настоящее время набор настроек чтения
 *
 * @note Набор передаётся через хранилище, а не доводом: договор функции разбора
 *       у всех стендов один, и менять его ради одного стенда было бы неверно
 *
 */
static awh::codec::ini::reader_t::settings_t CURRENT;

/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект потокового чтения текста настроек
	awh::codec::ini::reader_t reader(::logger(), CURRENT);
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
 * @brief Функция получения исходного набора настроек чтения
 *
 * @return исходный набор настроек чтения
 *
 */
static awh::codec::ini::reader_t::settings_t initial() noexcept {
	// Собираемые настройки разбора текста настроек
	awh::codec::ini::reader_t::settings_t result;
	// Снимаем выдачу примечаний отдельным событием
	result.emitComments = false;
	// Снимаем выдачу пустых строк отдельным событием
	result.emitBlanks = false;
	// Выводим собранные настройки разбора
	return result;
}

/**
 * @brief Структура снимаемой возможности чтения
 *
 */
typedef struct Ablation {
	// Название снимаемой возможности
	const char * name;
	// Функция снятия возможности с настроек чтения
	void (* apply)(awh::codec::ini::reader_t::settings_t &);
} ablation_t;

/**
 * @brief Перечень снимаемых возможностей чтения
 *
 * @note Возможности снимаются накопительно: всякий следующий прогон идёт без
 *       всех прежде снятых. Снятие по одной от исходного набора показало бы цену
 *       возможности в отрыве от прочих, а нужен путь от нашего чтения к чужому
 *
 */
static const ablation_t ABLATIONS[] = {
	{"исходное чтение", [](awh::codec::ini::reader_t::settings_t &) noexcept {}},
	{"без снятия кавычек", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Оставляем кавычки частью значения свойства
		settings.quotes = awh::codec::ini::quote_t::KEEP;
	}},
	{"без управляющих последовательностей", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Снимаем разбор управляющих последовательностей
		settings.escapes = false;
	}},
	{"без примечаний в конце строки", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Снимаем признание примечания в конце строки со значением
		settings.inlineComments = false;
	}},
	{"без продолжений строк", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Снимаем сборку логической строки из продолжений
		settings.continuations = false;
		// Снимаем продолжение строки отступом
		settings.indents = false;
	}},
	{"без свойств без значения", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Снимаем признание свойства, записанного без разделителя
		settings.valueless = false;
		// Снимаем признание перечня значений записью «ключ[]»
		settings.arrays = false;
	}},
	{"без подразделов", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Снимаем выделение подраздела из имени раздела
		settings.subsections = awh::codec::ini::subsection_t::NONE;
	}},
	{"без обрезки обвязки", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Снимаем обрезку пробельной обвязки имён и значений
		settings.trim = false;
	}},
	{"без проверки кодировки", [](awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Устанавливаем кодировку, проверки правильности не требующую
		settings.encoding = awh::codec::ini::encoding_t::LATIN1;
	}}
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
	// Код выхода из стенда
	int32_t result = 0;
	// Собираемые настройки разбора текста настроек
	awh::codec::ini::reader_t::settings_t settings = initial();
	/**
	 * Выполняем перебор всех снимаемых возможностей чтения
	 */
	for(const ablation_t & ablation : ABLATIONS){
		// Выполняем снятие очередной возможности с настроек чтения
		ablation.apply(settings);
		// Запоминаем настройки разбора текущего прогона
		CURRENT = settings;
		// Перечень сценариев очередного прогона
		const rival::scenario_t scenarios[] = {
			{ablation.name, rival::LARGE_ROUNDS, rival::large, parse}
		};
		// Выполняем прогон сценария со снятой возможностью
		result |= rival::run(argc, argv, scenarios, 1);
	}
	// Выводим код выхода из стенда
	return result;
}
