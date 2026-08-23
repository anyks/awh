/**
 * @file awh.cpp
 * @date 2026-08-16
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
 * @brief Эталонный стенд сравнения — потоковое чтение текста настроек контейнером AWH
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/toml/reader.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"
#include <sys/log.hpp>

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
 * @brief Функция получения настроек разбора текста настроек
 *
 * @details Выдача примечаний и пустых строк отключена намеренно: сличаемые
 * реализации ни примечаний, ни пустых строк не выдают вовсе, и оставлять их
 * включёнными значило бы сравнивать разный объём работы. Чего стоят примечания
 * событиями, показывает набор `benchmark/codec/toml` - там они включены
 *
 * @return настройки разбора текста настроек
 *
 */
static const awh::codec::toml::reader_t::settings_t & settings() noexcept {
	// Настройки разбора текста настроек
	static const awh::codec::toml::reader_t::settings_t result = []() noexcept -> awh::codec::toml::reader_t::settings_t {
		// Собираемые настройки разбора текста настроек
		awh::codec::toml::reader_t::settings_t result;
		// Снимаем выдачу примечаний отдельным событием
		result.emitComments = false;
		// Снимаем выдачу пустых строк отдельным событием
		result.emitBlanks = false;
		// Выводим собранные настройки разбора
		return result;
	}();
	// Выводим настройки разбора текста настроек
	return result;
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @details Значения строковые учитываются контрольной суммой, а прочие - лишь
 * количеством: типы значений реализации выдают по-своему, и складывать их в одну
 * сумму значило бы сличать представления, а не выполненную работу
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект потокового чтения текста настроек
	awh::codec::toml::reader_t reader(::logger(), settings());
	/**
	 * Если передать текст настроек не удалось
	 */
	if(!reader.feed(text.data(), text.size(), true))
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
			 * Если получено объявление таблицы либо очередной таблицы набора
			 */
			case static_cast <uint8_t> (awh::codec::toml::event_t::TABLE):
			case static_cast <uint8_t> (awh::codec::toml::event_t::ARRAY_TABLE):
			/**
			 * Если получено имя ключа пары
			 */
			case static_cast <uint8_t> (awh::codec::toml::event_t::KEY): {
				/**
				 * Выполняем перебор всех составных частей полученного имени
				 */
				for(const awh::codec::toml::part_t & part : reader.path())
					// Выполняем чтение очередной составной части имени
					rival::touch(part.name.data(), part.name.size());
			} break;
			/**
			 * Если получено значение пары либо очередное значение перечня
			 */
			case static_cast <uint8_t> (awh::codec::toml::event_t::VALUE): {
				// Получаем полученное значение разбора
				const awh::codec::toml::content_t & value = reader.value();
				// Выполняем учёт обработанной пары
				rival::entry();
				/**
				 * Если значение является строковым
				 */
				if(value.type == awh::codec::toml::type_t::STRING)
					// Выполняем учёт строкового значения
					rival::consume(value.text.data(), value.text.size());
			} break;
		}
	}
	// Выводим признак успешного разбора
	return (reader.state() == awh::codec::toml::state_t::FINISHED);
}

/**
 * @brief Перечень сценариев стенда
 *
 */
static const rival::scenario_t SCENARIOS[] = {
	{"service", rival::SMALL_ROUNDS,   rival::service, parse},
	{"large",   rival::LARGE_ROUNDS,   rival::large,   parse},
	{"strings", rival::FOCUSED_ROUNDS, rival::strings, parse},
	{"numbers", rival::FOCUSED_ROUNDS, rival::numbers, parse},
	{"arrays",  rival::FOCUSED_ROUNDS, rival::arrays,  parse},
	{"tables",  rival::FOCUSED_ROUNDS, rival::tables,  parse}
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
