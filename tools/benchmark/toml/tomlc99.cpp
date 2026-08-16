/**
 * @file tomlc99.cpp
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
 * @brief Эталонный стенд сравнения — разбор текста настроек реализацией tomlc99
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
extern "C" {
	#include <toml.h>
}

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция обхода собранного перечня значений
 *
 * @param array обходимый перечень значений
 *
 */
static void items(const toml_array_t * array) noexcept;

/**
 * @brief Функция обхода собранной таблицы дерева настроек
 *
 * @details Реализация эта разбор строкового значения откладывает до его чтения:
 * дерево удерживает запись значения дословно, а `toml_string_in` снимает ограду и
 * выдаёт собственную копию, которую потребитель обязан освободить. Работа эта у
 * прочих реализаций выполняется разбором, а здесь - обходом, и оплачена быть
 * должна: без чтения значений стенд сравнивал бы разный объём работы
 *
 * @param table обходимая таблица дерева настроек
 *
 */
static void entries(const toml_table_t * table) noexcept {
	/**
	 * Если таблица дерева настроек не передана
	 */
	if(table == nullptr)
		// Выходим из обхода таблицы дерева настроек
		return;
	/**
	 * Выполняем перебор всех имён таблицы дерева настроек
	 */
	for(int32_t i = 0;; i++){
		// Получаем очередное имя таблицы дерева настроек
		const char * key = ::toml_key_in(table, i);
		/**
		 * Если имена таблицы дерева настроек исчерпаны
		 */
		if(key == nullptr)
			// Выходим из перебора имён таблицы дерева настроек
			break;
		// Выполняем чтение очередного имени таблицы дерева настроек
		rival::touch(key, ::strlen(key));
		/**
		 * Если значением имени является вложенная таблица
		 */
		if(const toml_table_t * nested = ::toml_table_in(table, key)){
			// Выполняем обход вложенной таблицы дерева настроек
			entries(nested);
			// Выполняем переход к следующему имени таблицы
			continue;
		}
		/**
		 * Если значением имени является перечень значений
		 */
		if(const toml_array_t * array = ::toml_array_in(table, key)){
			// Выполняем обход перечня значений
			items(array);
			// Выполняем переход к следующему имени таблицы
			continue;
		}
		// Выполняем учёт обработанной пары
		rival::entry();
		// Выполняем чтение значения очередной пары
		const toml_datum_t value = ::toml_string_in(table, key);
		/**
		 * Если значение является строковым
		 */
		if(value.ok){
			// Выполняем учёт строкового значения
			rival::consume(value.u.s, ::strlen(value.u.s));
			// Выполняем освобождение выданной копии строкового значения
			::free(value.u.s);
		}
	}
}
/**
 * @brief Функция обхода собранного перечня значений
 *
 * @param array обходимый перечень значений
 *
 */
static void items(const toml_array_t * array) noexcept {
	/**
	 * Если перечень значений не передан
	 */
	if(array == nullptr)
		// Выходим из обхода перечня значений
		return;
	// Получаем количество значений перечня
	const int32_t count = ::toml_array_nelem(array);
	/**
	 * Выполняем перебор всех значений перечня
	 */
	for(int32_t i = 0; i < count; i++){
		/**
		 * Если значением перечня является таблица
		 */
		if(const toml_table_t * table = ::toml_table_at(array, i)){
			// Выполняем обход таблицы дерева настроек
			entries(table);
			// Выполняем переход к следующему значению перечня
			continue;
		}
		/**
		 * Если значением перечня является вложенный перечень
		 */
		if(const toml_array_t * nested = ::toml_array_at(array, i)){
			// Выполняем обход вложенного перечня значений
			items(nested);
			// Выполняем переход к следующему значению перечня
			continue;
		}
		// Выполняем учёт обработанной пары
		rival::entry();
		// Выполняем чтение очередного значения перечня
		const toml_datum_t value = ::toml_string_at(array, i);
		/**
		 * Если значение является строковым
		 */
		if(value.ok){
			// Выполняем учёт строкового значения
			rival::consume(value.u.s, ::strlen(value.u.s));
			// Выполняем освобождение выданной копии строкового значения
			::free(value.u.s);
		}
	}
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @note Реализация эта разбирает лишь изменяемую последовательность знаков,
 *       оканчивающуюся нулём: разбор ведётся правкой самого текста на месте.
 *       Копия его потому собирается на всякий прогон, и стоимость её входит в
 *       замер - обойти её потребитель не может тоже
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Собираемая изменяемая копия разбираемого текста настроек
	std::vector <char> buffer(text.size() + 1);
	// Выполняем сборку изменяемой копии разбираемого текста настроек
	::memcpy(buffer.data(), text.data(), text.size());
	// Устанавливаем окончание изменяемой копии текста настроек
	buffer[text.size()] = '\0';
	// Приёмник сообщения об ошибке разбора
	char message[256] = {0};
	// Выполняем разбор текста настроек
	toml_table_t * document = ::toml_parse(buffer.data(), message, sizeof(message));
	/**
	 * Если разбор текста настроек выполнить не удалось
	 */
	if(document == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева настроек
	entries(document);
	// Выполняем освобождение собранного дерева настроек
	::toml_free(document);
	// Выводим признак успешного разбора
	return true;
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
