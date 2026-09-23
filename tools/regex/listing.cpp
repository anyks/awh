/**
 * @file listing.cpp
 * @date 2026-09-22
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп печати программы регулярного выражения
 *
 * @details Разыскание по счётчикам мер работы упирается в вопрос, сколько
 *          инструкций порождает та или иная запись выражения: счётчик шагов
 *          цикла числом говорит, а устройством - нет. Щуп печатает набор
 *          инструкций собранной программы, чем вопрос и закрывает.
 *
 * Сборка и запуск: tools/regex/probe.sh listing [выражение]
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdint>
#include <cstring>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Набор имён кодов операций
 *
 */
static const char * NAMES[] = {
	"CHAR", "?01", "CLASS", "ANY", "CODEUNIT", "SPLIT", "JUMP", "SAVE",
	"ANCHOR", "MATCH", "KEEP", "MARK", "CUT", "BACKREF", "PROGRESS", "LOOK",
	"CALL", "RETURN", "CONDITION", "RESUME", "GRAPHEME", "ACCEPT", "CONTROL",
	"RESET", "SCRIPT"
};

/**
 * @brief Функция печати программы одного выражения
 *
 * @param engine  движок сопоставления
 * @param pattern текст регулярного выражения
 *
 */
static void listing(awh::regex::engine_t & engine, const char * pattern) noexcept {
	// Создаём собранное выражение
	awh::regex::expression_t expression;
	/**
	 * Если сборка выражения не выполнена
	 */
	if(!engine.build(pattern, 0, expression)){
		// Выводим сообщение об отказе сборки
		::printf("\n%s — ОТКАЗ СБОРКИ\n", pattern);
		// Выходим из печати программы
		return;
	}
	// Получаем набор инструкций собранной программы
	const auto & instructions = expression.forward.instructions;
	// Выводим заголовок печати программы
	::printf("\n%s — инструкций %zu, классов %zu, диапазонов %zu, байт на инструкцию %zu\n", pattern,
	 static_cast <size_t> (instructions.size()),
	 static_cast <size_t> (expression.forward.classes.size()),
	 static_cast <size_t> (expression.forward.ranges.size()),
	 sizeof(awh::regex::instruction_t));
	/**
	 * Выполняем обход инструкций собранной программы
	 */
	for(size_t i = 0; i < instructions.size(); i++){
		// Получаем очередную инструкцию программы
		const auto & instruction = instructions.at(i);
		// Получаем код операции инструкции
		const uint8_t code = static_cast <uint8_t> (instruction.type);
		// Получаем имя кода операции инструкции
		const char * name = ((code < (sizeof(NAMES) / sizeof(NAMES[0]))) ? NAMES[code] : "?");
		/**
		 * Если инструкция выполняет переход по двум ветвям
		 */
		if(instruction.type == awh::regex::opcode_t::SPLIT){
			// Выводим строку инструкции перехода по двум ветвям
			::printf("  %3zu  %-9s first=%u second=%u run=%d lazily=%u solid=%u most=%u\n", i, name,
			 instruction.split.first, instruction.split.second,
			 static_cast <int32_t> (instruction.split.run),
			 static_cast <uint32_t> (instruction.split.lazily),
			 static_cast <uint32_t> (instruction.split.solid),
			 static_cast <uint32_t> (instruction.split.most));
		/**
		 * Если инструкция выполняет безусловный переход
		 */
		} else if(instruction.type == awh::regex::opcode_t::JUMP)
			// Выводим строку инструкции безусловного перехода
			::printf("  %3zu  %-9s target=%u\n", i, name, instruction.jump.target);
		/**
		 * Если инструкция сохраняет позицию в ячейке захвата
		 */
		else if(instruction.type == awh::regex::opcode_t::SAVE)
			// Выводим строку инструкции сохранения позиции
			::printf("  %3zu  %-9s slot=%u\n", i, name, instruction.save.slot);
		/**
		 * Если инструкция сопоставляет символ из класса символов
		 */
		else if(instruction.type == awh::regex::opcode_t::CLASS)
			// Выводим строку инструкции сопоставления класса
			::printf("  %3zu  %-9s index=%u repeat=%u\n", i, name, instruction.charclass.index,
			 static_cast <uint32_t> (instruction.repeat));
		/**
		 * Инструкция прочая печатается одним именем
		 */
		else ::printf("  %3zu  %-9s repeat=%u\n", i, name, static_cast <uint32_t> (instruction.repeat));
	}
}

/**
 * @brief Функция печати сводки по выражениям набора замеров
 *
 * @param engine движок сопоставления
 *
 */
static void summary(awh::regex::engine_t & engine) noexcept {
	/**
	 * @brief Набор выражений набора замеров сопоставления
	 *
	 */
	static const char * PATTERNS[] = {
		"[0-9]{3,5}", "\\w+@\\w+\\.\\w+", "GET|POST|PUT|DELETE|HEAD|OPTIONS",
		"(?m)^[A-Za-z0-9-]+: .+$", "([A-Za-z0-9-]+): (.+)",
		"(?m)^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)\\r?$",
		"(?m)^Host: (\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\r?$",
		"\\w+?@\\w+?\\.", "(?:[a-z]+ )+dog", "\\((?:[^()]|\\([^()]*\\))*\\)",
		"(?:(\\w+) )+forman", "(\\w+) \\1", "\\w+(?=@)", "(?<=@)\\w+",
		"(?>\\w+)@\\w+", "\\((?:[^()]|(?R))*\\)"
	};
	// Общее количество классов и диапазонов
	size_t classes = 0, ranges = 0;
	// Выводим заголовок сводки
	::printf("%-58s %9s %12s\n", "ВЫРАЖЕНИЕ", "классов", "диапазонов");
	/**
	 * Выполняем обход выражений набора замеров
	 */
	for(const char * pattern : PATTERNS){
		// Создаём собранное выражение
		awh::regex::expression_t expression;
		/**
		 * Если сборка выражения не выполнена
		 */
		if(!engine.build(pattern, 0, expression)){
			// Выводим сообщение об отказе сборки
			::printf("%-58s %9s\n", pattern, "ОТКАЗ");
			// Выполняем переход к выражению следующему
			continue;
		}
		// Получаем количество классов и диапазонов выражения
		const size_t count = static_cast <size_t> (expression.forward.classes.size());
		const size_t spans = static_cast <size_t> (expression.forward.ranges.size());
		// Выполняем учёт классов и диапазонов
		classes += count;
		ranges += spans;
		// Выводим строку итога выражения
		::printf("%-58s %9zu %12zu\n", pattern, count, spans);
	}
	// Выводим общий итог сводки
	::printf("%-58s %9zu %12zu\n", "ВСЕГО", classes, ranges);
}
/**
 * @brief Функция запуска щупа
 *
 * @param argc количество полученных аргументов
 * @param argv набор полученных аргументов
 * @return     результат исполнения щупа
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	/**
	 * Если щупу задано выражение
	 */
	if(argc > 1){
		// Выполняем печать программы заданного выражения
		listing(engine, argv[1]);
		// Выводим результат работы щупа
		return 0;
	}
	// Выполняем печать сводки по выражениям набора замеров
	summary(engine);
	// Выполняем печать программ выражений, разысканием затронутых
	listing(engine, "[0-9]{3,5}");
	listing(engine, "[0-9]+");
	listing(engine, "\\d{1,3}");
	// Выводим результат работы щупа
	return 0;
}
