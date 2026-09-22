/**
 * @file solidgap.cpp
 * @date 2026-09-22
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп расхождения проверок бесплодного возврата в ряд повторения
 *
 * @details Решение «ряд проходится без отдачи» принимается в модуле дважды
 *          и независимо: компилятор считает его в Compiler::futile и кладёт
 *          в признак split.solid, а порождатель машинного кода считает его
 *          заново в codegen.cpp::sealing. Щуп проверяет, совпадают ли обе
 *          проверки в охвате.
 *
 *          Счётчик пути SOLIDING отмечает пропуск точек возврата ряда
 *          исполнением с возвратом. Ноль его при непересекающихся наборах
 *          байтов означает, что признак не выставлен и толкователь точки
 *          кладёт напрасно.
 *
 * @note Признак «PROBING» при сборке обязателен, иначе счётчики молчат.
 *
 * Сборка и запуск: tools/regex/probe.sh solidgap
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/probe.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdint>
#include <vector>
#include <utility>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Набор случаев, щупом проверяемых
 *
 * @details Наборы байтов тела повторения и продолжения не пересекаются
 *          во всех случаях, отчего возврат в ряд бесплоден в каждом из них:
 *          признак ожидается выставленным везде
 *
 */
static const struct {
	// Текст регулярного выражения
	const char * pattern;
	// Текст, выражению подставляемый
	const char * text;
	// Пояснение случая
	const char * note;
} CASES[] = {
	{"[a-z]+@",        "forman@anyks.com",   "опора: продолжение символом"},
	{"[a-z]+[0-9]",    "forman7",            "ось 1: продолжение классом"},
	{"(?:[a-z]+)@",    "forman@anyks.com",   "ось 2: продолжение за скобками"},
	{"[a-z]+(?:@)",    "forman@anyks.com",   "ось 2: продолжение внутри скобок"},
	{"(?i)[a-z]+@",    "FORMAN@anyks.com",   "ось 3: свёртка регистра в ASCII"},
	{"(?i)[a-z]+\\xC0", "forman\\xC0",        "ось 3: свёртка регистра вне ASCII"}
};

/**
 * @brief Функция запуска щупа
 *
 * @return результат исполнения щупа
 *
 */
int main() noexcept {
	/**
	 * Если учёт путей сопоставления при сборке не включён
	 */
	if(!awh::regex::probe_t::enabled()){
		// Выводим сообщение об отсутствии учёта путей
		::printf("Щуп собран без признака PROBING: счётчики молчат\n");
		// Выводим результат отказа
		return 1;
	}
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Выводим заголовок таблицы щупа
	::printf("%-18s %-20s %10s %8s  %s\n", "ВЫРАЖЕНИЕ", "ТЕКСТ", "SOLIDING", "СОВП.", "СЛУЧАЙ");
	/**
	 * Выполняем перебор проверяемых случаев
	 */
	for(const auto & item : CASES){
		// Создаём собранное выражение
		awh::regex::expression_t expression;
		/**
		 * Если сборка выражения не выполнена
		 */
		if(!engine.build(item.pattern, 0, expression)){
			// Выводим сообщение об отказе сборки
			::printf("%-18s %-20s %10s %8s  %s\n", item.pattern, item.text, "ОТКАЗ", "-", item.note);
			// Выполняем переход к случаю следующему
			continue;
		}
		// Создаём набор границ ячеек захвата текста
		vector <pair <size_t, size_t>> captures;
		// Выполняем сброс счётчиков путей сопоставления
		awh::regex::probe_t::reset();
		// Выполняем сопоставление выражения с текстом
		const bool matched = engine.exec(expression, item.text, 0, captures);
		// Получаем количество пропусков точек возврата ряда повторения
		const uint64_t solids = awh::regex::probe_t::count(awh::regex::path_t::SOLIDING);
		// Выводим строку итога случая
		::printf("%-18s %-20s %10llu %8s  %s\n", item.pattern, item.text,
		 static_cast <unsigned long long> (solids), (matched ? "есть" : "НЕТ"), item.note);
	}
	// Выводим результат работы щупа
	return 0;
}
