/**
 * @file forgefilter.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп вреда от поддельного предварительного отбора позиций
 *
 * @details Запись хранилища несёт предварительный отбор позиций целиком:
 *          набор допустимых начальных байтов, признак единственного байта
 *          и сам этот байт, ведущий и обязательный литералы, удаление
 *          литерала. Все исполнители отбору ДОВЕРЯЮТ: позиция, им отвергнутая,
 *          попытки не получает вовсе. Щуп подделывает отбор прямо в собранной
 *          программе тремя способами и сопоставляет выражение текстом, какому
 *          оно отвечать обязано:
 *
 *          - набор допустимых байтов без настоящего первого байта;
 *          - признак единственного байта с байтом посторонним;
 *          - обязательный литерал, в тексте отсутствующий.
 *
 *          Щуп ЖДЁТ отказа поверки: заслон на месте - и подделка до
 *          сопоставления не доходит. Совпадение, подделкой снятое, есть вред:
 *          запись меняет вердикт молча.
 *
 * Сборка и запуск: tools/regex/probe.sh forgefilter
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/storage.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <string>
#include <cstdint>
#include <functional>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Функция испытания одной подделки отбора
 *
 * @param regexp  объект работы с регулярными выражениями
 * @param storage объект хранилища собранных выражений
 * @param exp     собранное выражение
 * @param name    имя подделки
 * @param forge   действие подделки над программой
 * @return        истина, если подделка сняла совпадение
 *
 */
static bool attempt(const regexp_t & regexp, const regex::storage_t & storage, const regexp_t::exp_t & exp,
 const char * name, const function <void (regex::prefilter_t &)> & forge) noexcept {
	// Создаём подделываемое собранное выражение
	auto twisted = make_shared <regex::expression_t> (* exp);
	// Выполняем подделку отбора прямой программы
	forge(twisted->forward.prefilter);
	// Запись хранилища подделанного выражения
	string record;
	/**
	 * Если запись подделанного выражения не удалась
	 */
	if(!storage.save({twisted}, record)) {
		// Выводим сообщение об отказе записи выражения
		::printf("%-36s запись не удалась\n", name);
		// Выводим отсутствие вреда
		return false;
	}
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	/**
	 * Если восстановление записи подделанной отвергнуто
	 */
	if(!storage.load(record, restored)) {
		// Выводим сообщение об отказе восстановления записи
		::printf("%-36s ОТВЕРГНУТО поверкой, код %u — заслон на месте\n",
		 name, static_cast <uint32_t> (storage.error()));
		// Выводим отсутствие вреда
		return false;
	}
	// Получаем итог сопоставления выражением подделанным
	const bool matched = regexp.test("xx-abc-xx", restored.at(0));
	// Выводим итог сопоставления выражением подделанным
	::printf("%-36s принято, «xx-abc-xx» → %s%s\n", name, (matched ? "да" : "нет"),
	 (matched ? "" : "   ВРЕД: совпадение снято записью молча"));
	// Выводим признак вреда
	return !matched;
}

/**
 * @brief Функция запуска приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main() noexcept {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	/**
	 * Выполняем сборку регулярного выражения
	 *
	 * @details Обратная ссылка уводит сопоставление в исполнение с возвратом,
	 *          а отбор позиций у выражения есть: начальный байт один - «a»,
	 *          обязательный литерал - «bc»
	 *
	 */
	const auto exp = regexp.build("(a)bc\\1?");
	/**
	 * Если сборка регулярного выражения не удалась
	 */
	if(!exp) {
		// Выводим сообщение об отказе сборки выражения
		::printf("сборка выражения не удалась\n");
		// Выходим из приложения с кодом отказа
		return 1;
	}
	// Получаем отбор собранной программы
	const auto & prefilter = exp->forward.prefilter;
	// Выводим устройство отбора собранной программы
	::printf("отбор: действует %s, единственный байт %s «%c», литерал «%s», ведущий «%s»\n",
	 (prefilter.active ? "да" : "нет"), (prefilter.unique ? "да" : "нет"), prefilter.letter,
	 prefilter.literal.c_str(), prefilter.leading.c_str());
	// Выполняем проверку сопоставления выражением собранным
	::printf("%-36s «xx-abc-xx» → %s\n", "собранное", (regexp.test("xx-abc-xx", exp) ? "да" : "нет"));
	// Количество подделок, снявших совпадение
	uint32_t harmed = 0;
	// Выполняем подделку набора допустимых байтов без настоящего первого байта
	harmed += ::attempt(regexp, storage, exp, "набор байтов без «a»", [](regex::prefilter_t & forged) noexcept -> void {
		forged.active = true;
		forged.bytes[static_cast <uint8_t> ('a')] = false;
		forged.bytes[static_cast <uint8_t> ('z')] = true;
		forged.unique = false;
		forged.leading.clear();
	}) ? 1 : 0;
	// Выполняем подделку единственного байта байтом посторонним
	harmed += ::attempt(regexp, storage, exp, "единственный байт «z»", [](regex::prefilter_t & forged) noexcept -> void {
		forged.active = true;
		forged.unique = true;
		forged.letter = 'z';
		forged.leading.clear();
	}) ? 1 : 0;
	// Выполняем подделку обязательного литерала отсутствующим
	harmed += ::attempt(regexp, storage, exp, "обязательный литерал «qq»", [](regex::prefilter_t & forged) noexcept -> void {
		forged.literal = "qq";
		forged.distance = string_view::npos;
	}) ? 1 : 0;
	// Выводим итог щупа
	::printf("подделок, снявших совпадение: %u из 3\n", harmed);
	// Выходим из приложения с кодом успеха
	return 0;
}
