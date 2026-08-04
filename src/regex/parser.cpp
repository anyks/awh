/**
 * @file: parser.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация синтаксического разбора регулярных выражений — рекурсивный спуск по тексту
 *        регулярного выражения синтаксиса PCRE с формированием синтаксического дерева в арене узлов,
 *        разбором классов символов, групп, кванторов повторения и экранированных последовательностей
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/parser.hpp>
#include <unicode/unicode.hpp>
#include <sys/ascii.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён вспомогательных функций разбора
 *
 */
namespace {
	/**
	 * @brief Функция приведения режима компиляции к его числовому значению
	 *
	 * @param value режим компиляции регулярного выражения
	 * @return      числовое значение режима компиляции
	 *
	 */
	inline uint32_t flagOf(const regex::flag_t value) noexcept {
		// Выполняем приведение режима компиляции к числовому значению
		return static_cast <uint32_t> (value);
	}
	/**
	 * @brief Функция проверки установки режима компиляции
	 *
	 * @param flags набор режимов компиляции регулярного выражения
	 * @param value проверяемый режим компиляции регулярного выражения
	 * @return      результат проверки установки режима компиляции
	 *
	 */
	inline bool hasFlag(const uint32_t flags, const regex::flag_t value) noexcept {
		// Выполняем проверку установки режима компиляции
		return ((flags & flagOf(value)) != 0);
	}
	/**
	 * @brief Функция извлечения числового значения шестнадцатеричной цифры
	 *
	 * @param letter символ шестнадцатеричной цифры
	 * @return       числовое значение шестнадцатеричной цифры
	 *
	 */
	inline uint32_t hexValue(const char letter) noexcept {
		/**
		 * Если символ является десятичной цифрой
		 */
		if(ascii::isDigit(letter))
			// Выводим числовое значение десятичной цифры
			return static_cast <uint32_t> (letter - '0');
		// Выводим числовое значение шестнадцатеричной цифры
		return (static_cast <uint32_t> (ascii::toLower(letter) - 'a') + 10);
	}
	/**
	 * @brief Функция проверки допустимости символа имени группы
	 *
	 * @param letter проверяемый символ имени группы
	 * @param first  флаг проверки первого символа имени группы
	 * @return       результат проверки допустимости символа имени группы
	 *
	 */
	inline bool isNameChar(const char letter, const bool first) noexcept {
		/**
		 * Если проверяется первый символ имени группы
		 */
		if(first)
			// Имя группы начинается с буквы либо знака подчёркивания
			return (ascii::isAlpha(letter) || (letter == '_'));
		// Прочие символы имени группы допускают также десятичные цифры
		return (ascii::isAlnum(letter) || (letter == '_'));
	}
};

/**
 * @brief Конструктор
 *
 */
awh::regex::Parser::Parser() noexcept :
 _pos(0), _root(INVALID_NODE), _depth(0), _captures(0), _total(0),
 _flags(0), _options(0), _look(0), _error(error_t::NONE), _errorPos(0) {
	// Резервируем память под арену узлов синтаксического дерева
	this->_nodes.reserve(64);
}
/**
 * @brief Метод сброса результатов разбора
 *
 */
void awh::regex::Parser::reset() noexcept {
	// Выполняем сброс позиции разбора
	this->_pos = 0;
	// Выполняем сброс индекса корневого узла
	this->_root = INVALID_NODE;
	// Выполняем сброс глубины вложенности групп
	this->_depth = 0;
	// Выполняем сброс количества обнаруженных захватывающих групп
	this->_captures = 0;
	// Выполняем сброс общего количества захватывающих групп
	this->_total = 0;
	// Выполняем сброс набора режимов компиляции
	this->_flags = 0;
	// Выполняем сброс исходного набора режимов компиляции
	this->_options = 0;
	// Выполняем сброс глубины вложенности проверок окружения
	this->_look = 0;
	// Выполняем сброс кода ошибки разбора
	this->_error = error_t::NONE;
	// Выполняем сброс смещения ошибки разбора
	this->_errorPos = 0;
	// Выполняем очистку арены узлов синтаксического дерева
	this->_nodes.clear();
	// Выполняем очистку хранилища классов символов
	this->_classes.clear();
	// Выполняем очистку хранилища имён именованных групп
	this->_names.clear();
	// Выполняем очистку хранилища последовательностей символов
	this->_strings.clear();
	// Выполняем очистку набора отложенных ссылок
	this->_deferred.clear();
	// Выполняем очистку соответствия имён групп их номерам
	this->_groups.clear();
}
/**
 * @brief Метод установки ошибки разбора
 *
 * @param error код ошибки разбора регулярного выражения
 * @param pos   смещение ошибки в тексте регулярного выражения
 * @return      индекс отсутствующего узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::fail(const error_t error, const size_t pos) noexcept {
	/**
	 * Если ошибка разбора ещё не установлена
	 */
	if(this->_error == error_t::NONE) {
		// Выполняем установку кода ошибки разбора
		this->_error = error;
		// Выполняем установку смещения ошибки разбора
		this->_errorPos = pos;
	}
	// Выводим индекс отсутствующего узла синтаксического дерева
	return INVALID_NODE;
}
/**
 * @brief Метод извлечения кода ошибки разбора
 *
 * @return код ошибки последней операции разбора
 *
 */
awh::regex::error_t awh::regex::Parser::error() const noexcept {
	// Выводим код ошибки последней операции разбора
	return this->_error;
}
/**
 * @brief Метод извлечения смещения ошибки разбора
 *
 * @return смещение ошибки в тексте регулярного выражения
 *
 */
size_t awh::regex::Parser::errorPos() const noexcept {
	// Выводим смещение ошибки в тексте регулярного выражения
	return this->_errorPos;
}
/**
 * @brief Метод извлечения текста ошибки разбора
 *
 * @return текст ошибки последней операции разбора
 *
 */
string awh::regex::Parser::message() const noexcept {
	/**
	 * Определяем код ошибки последней операции разбора
	 */
	switch(static_cast <uint8_t> (this->_error)) {
		// Ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE): return "no error";
		// Внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL): return "internal error";
		// Обратная косая черта в конце выражения
		case static_cast <uint8_t> (error_t::TRAILING_BACKSLASH): return "pattern ends with a backslash";
		// Неизвестная экранированная последовательность
		case static_cast <uint8_t> (error_t::UNKNOWN_ESCAPE): return "unrecognized escape sequence";
		// Непарная круглая скобка
		case static_cast <uint8_t> (error_t::UNMATCHED_PAREN): return "unmatched parenthesis";
		// Непарная квадратная скобка
		case static_cast <uint8_t> (error_t::UNMATCHED_BRACKET): return "missing terminating character for character class";
		// Непарная фигурная скобка
		case static_cast <uint8_t> (error_t::UNMATCHED_BRACE): return "unmatched brace";
		// Некорректный квантор повторения
		case static_cast <uint8_t> (error_t::BAD_QUANTIFIER): return "invalid quantifier";
		// Квантор повторения без предшествующего элемента
		case static_cast <uint8_t> (error_t::QUANTIFIER_NO_ATOM): return "quantifier does not follow a repeatable item";
		// Значение кванта повторения превышает допустимое
		case static_cast <uint8_t> (error_t::QUANTIFIER_TOO_BIG): return "number too big in quantifier";
		// Некорректный диапазон в классе символов
		case static_cast <uint8_t> (error_t::BAD_CLASS_RANGE): return "range out of order in character class";
		// Некорректная шестнадцатеричная последовательность
		case static_cast <uint8_t> (error_t::BAD_ESCAPE_HEX): return "invalid hexadecimal escape sequence";
		// Некорректная восьмеричная последовательность
		case static_cast <uint8_t> (error_t::BAD_ESCAPE_OCTAL): return "invalid octal escape sequence";
		// Неизвестное свойство Юникода
		case static_cast <uint8_t> (error_t::BAD_PROPERTY): return "unknown unicode property name";
		// Неизвестный класс символов POSIX
		case static_cast <uint8_t> (error_t::BAD_POSIX_CLASS): return "unknown posix class name";
		// Некорректный синтаксис группы
		case static_cast <uint8_t> (error_t::BAD_GROUP_SYNTAX): return "unrecognized character after (?";
		// Некорректное имя именованной группы
		case static_cast <uint8_t> (error_t::BAD_GROUP_NAME): return "invalid group name";
		// Повторное объявление имени группы
		case static_cast <uint8_t> (error_t::DUPLICATE_NAME): return "two named groups have the same name";
		// Ссылка на несуществующую группу
		case static_cast <uint8_t> (error_t::BAD_BACKREFERENCE): return "reference to non-existent subpattern";
		// Некорректный условный шаблон
		case static_cast <uint8_t> (error_t::BAD_CONDITION): return "malformed conditional group";
		// Некорректный рекурсивный вызов
		case static_cast <uint8_t> (error_t::BAD_RECURSION): return "invalid recursive call";
		// Некорректные встроенные опции
		case static_cast <uint8_t> (error_t::BAD_OPTIONS): return "invalid inline option";
		// Превышена допустимая глубина вложенности
		case static_cast <uint8_t> (error_t::NESTING_TOO_DEEP): return "parentheses are too deeply nested";
		// Регулярное выражение превышает допустимый размер
		case static_cast <uint8_t> (error_t::PATTERN_TOO_LARGE): return "pattern is too large";
		// Некорректная последовательность UTF-8 в выражении
		case static_cast <uint8_t> (error_t::BAD_UTF8): return "invalid utf-8 sequence in pattern";
		// Некорректная последовательность UTF-8 в тексте сопоставления
		case static_cast <uint8_t> (error_t::BAD_UTF8_SUBJECT): return "invalid utf-8 sequence in subject";
		// Недопустимая ретроспективная проверка
		case static_cast <uint8_t> (error_t::LOOKBEHIND_INVALID): return "invalid lookbehind assertion";
		// Конструкция не поддерживается модулем
		case static_cast <uint8_t> (error_t::UNSUPPORTED): return "unsupported pattern construct";
		// Выводим текст ошибки превышения допустимого объёма работы сопоставления
		case static_cast <uint8_t> (error_t::BUDGET_EXCEEDED): return "match budget exceeded";
	}
	// Выводим текст неизвестной ошибки разбора
	return "unknown error";
}
/**
 * @brief Метод извлечения индекса корневого узла синтаксического дерева
 *
 * @return индекс корневого узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::root() const noexcept {
	// Выводим индекс корневого узла синтаксического дерева
	return this->_root;
}
/**
 * @brief Метод извлечения количества захватывающих групп
 *
 * @return количество захватывающих групп регулярного выражения
 *
 */
uint32_t awh::regex::Parser::captures() const noexcept {
	// Выводим количество захватывающих групп регулярного выражения
	return this->_captures;
}
/**
 * @brief Метод извлечения исходного набора режимов компиляции
 *
 * @return исходный набор режимов компиляции регулярного выражения
 *
 */
uint32_t awh::regex::Parser::options() const noexcept {
	// Выводим исходный набор режимов компиляции регулярного выражения
	return this->_options;
}
/**
 * @brief Метод извлечения узла синтаксического дерева
 *
 * @param id индекс узла в арене узлов
 * @return   узел синтаксического дерева
 *
 */
const awh::regex::node_data_t & awh::regex::Parser::node(const node_id_t id) const noexcept {
	// Создаём узел пустого выражения для отсутствующего индекса
	static const node_data_t empty;
	/**
	 * Если индекс узла находится за пределами арены узлов
	 */
	if(id >= static_cast <node_id_t> (this->_nodes.size()))
		// Выводим узел пустого выражения
		return empty;
	// Выводим узел синтаксического дерева
	return this->_nodes.at(id);
}
/**
 * @brief Метод извлечения арены узлов синтаксического дерева
 *
 * @return арена узлов синтаксического дерева
 *
 */
const vector <awh::regex::node_data_t> & awh::regex::Parser::nodes() const noexcept {
	// Выводим арену узлов синтаксического дерева
	return this->_nodes;
}
/**
 * @brief Метод извлечения класса символов
 *
 * @param index индекс класса символов в хранилище классов
 * @return      класс символов регулярного выражения
 *
 */
const awh::regex::class_t & awh::regex::Parser::charClass(const uint32_t index) const noexcept {
	// Создаём пустой класс символов для отсутствующего индекса
	static const class_t empty;
	/**
	 * Если индекс класса находится за пределами хранилища классов
	 */
	if(index >= static_cast <uint32_t> (this->_classes.size()))
		// Выводим пустой класс символов
		return empty;
	// Выводим класс символов регулярного выражения
	return this->_classes.at(index);
}
/**
 * @brief Метод извлечения имени именованной группы
 *
 * @param index индекс имени в хранилище имён
 * @return      имя именованной группы
 *
 */
const string & awh::regex::Parser::name(const uint32_t index) const noexcept {
	// Создаём пустое имя для отсутствующего индекса
	static const string empty;
	/**
	 * Если индекс имени находится за пределами хранилища имён
	 */
	if(index >= static_cast <uint32_t> (this->_names.size()))
		// Выводим пустое имя именованной группы
		return empty;
	// Выводим имя именованной группы
	return this->_names.at(index);
}
/**
 * @brief Метод извлечения соответствия имён групп их номерам
 *
 * @return соответствие имён именованных групп их номерам
 *
 */
const unordered_map <string, vector <uint32_t>> & awh::regex::Parser::groups() const noexcept {
	// Выводим соответствие имён именованных групп наборам их номеров
	return this->_groups;
}
/**
 * @brief Метод извлечения последовательности символов узла
 *
 * @param offset смещение начала последовательности в хранилище строк
 * @param length длина последовательности в кодовых значениях
 * @return       адрес начала последовательности кодовых значений
 *
 */
const uint32_t * awh::regex::Parser::sequence(const uint32_t offset, const uint32_t length) const noexcept {
	/**
	 * Если последовательность выходит за пределы хранилища строк
	 */
	if((static_cast <size_t> (offset) + static_cast <size_t> (length)) > this->_strings.size())
		// Выводим отсутствие последовательности символов
		return nullptr;
	// Выводим адрес начала последовательности кодовых значений
	return (this->_strings.data() + offset);
}
/**
 * @brief Метод создания узла синтаксического дерева
 *
 * @param type тип создаваемого узла синтаксического дерева
 * @return     индекс созданного узла в арене узлов
 *
 */
awh::regex::node_id_t awh::regex::Parser::createNode(const node_t type) noexcept {
	// Получаем индекс создаваемого узла в арене узлов
	const node_id_t result = static_cast <node_id_t> (this->_nodes.size());
	// Создаём новый узел синтаксического дерева
	this->_nodes.emplace_back();
	// Выполняем установку типа узла синтаксического дерева
	this->_nodes.back().type = type;
	// Выполняем установку набора режимов компиляции узла
	this->_nodes.back().flags = this->_flags;
	// Выполняем установку смещения узла в тексте регулярного выражения
	this->_nodes.back().offset = static_cast <uint32_t> (this->_pos);
	// Выводим индекс созданного узла в арене узлов
	return result;
}
/**
 * @brief Метод добавления дочернего узла синтаксического дерева
 *
 * @param parent индекс родительского узла в арене узлов
 * @param child  индекс добавляемого дочернего узла в арене узлов
 *
 */
void awh::regex::Parser::appendChild(const node_id_t parent, const node_id_t child) noexcept {
	/**
	 * Если один из переданных индексов отсутствует
	 */
	if((parent == INVALID_NODE) || (child == INVALID_NODE))
		// Выходим из функции
		return;
	/**
	 * Если родительский узел не содержит дочерних узлов
	 */
	if(this->_nodes.at(parent).child == INVALID_NODE) {
		// Выполняем установку первого дочернего узла
		this->_nodes.at(parent).child = child;
		// Выходим из функции
		return;
	}
	// Получаем индекс последнего дочернего узла
	node_id_t last = this->_nodes.at(parent).child;
	/**
	 * Выполняем поиск последнего дочернего узла
	 */
	while(this->_nodes.at(last).next != INVALID_NODE)
		// Переходим к следующему дочернему узлу
		last = this->_nodes.at(last).next;
	// Выполняем добавление дочернего узла в конец списка
	this->_nodes.at(last).next = child;
}
/**
 * @brief Метод создания узла из набора дочерних узлов
 *
 * @param type  тип создаваемого узла синтаксического дерева
 * @param items набор индексов дочерних узлов в арене узлов
 * @return      индекс созданного узла в арене узлов
 *
 */
awh::regex::node_id_t awh::regex::Parser::makeList(const node_t type, const vector <node_id_t> & items) noexcept {
	/**
	 * Если набор дочерних узлов пуст
	 */
	if(items.empty())
		// Выводим индекс созданного узла пустого выражения
		return this->createNode(node_t::EMPTY);
	/**
	 * Если набор состоит из единственного узла
	 */
	if(items.size() == 1)
		// Выводим индекс единственного узла набора без обёртки
		return items.front();
	// Выполняем создание узла синтаксического дерева
	const node_id_t result = this->createNode(type);
	// Выполняем установку первого дочернего узла
	this->_nodes.at(result).child = items.front();
	/**
	 * Выполняем связывание дочерних узлов между собой
	 */
	for(size_t i = 1; i < items.size(); i++)
		// Выполняем установку следующего узла того же уровня вложенности
		this->_nodes.at(items.at(i - 1)).next = items.at(i);
	// Выводим индекс созданного узла в арене узлов
	return result;
}
/**
 * @brief Метод создания узла последовательности символов
 *
 * @param codes набор кодовых значений символов последовательности
 * @return      индекс созданного узла в арене узлов
 *
 */
awh::regex::node_id_t awh::regex::Parser::makeString(const vector <uint32_t> & codes) noexcept {
	/**
	 * Если набор кодовых значений символов пуст
	 */
	if(codes.empty())
		// Выводим индекс созданного узла пустого выражения
		return this->createNode(node_t::EMPTY);
	/**
	 * Если набор состоит из единственного кодового значения
	 */
	if(codes.size() == 1) {
		// Выполняем создание узла одиночного символа
		const node_id_t result = this->createNode(node_t::LITERAL);
		// Выполняем установку кодового значения символа
		this->_nodes.at(result).literal.code = codes.front();
		// Выводим индекс созданного узла в арене узлов
		return result;
	}
	// Получаем смещение начала последовательности в хранилище строк
	const uint32_t offset = static_cast <uint32_t> (this->_strings.size());
	// Выполняем размещение последовательности в хранилище строк
	this->_strings.insert(this->_strings.end(), codes.begin(), codes.end());
	// Выполняем создание узла последовательности символов
	const node_id_t result = this->createNode(node_t::STRING);
	// Выполняем установку смещения начала последовательности
	this->_nodes.at(result).string.offset = offset;
	// Выполняем установку длины последовательности
	this->_nodes.at(result).string.length = static_cast <uint32_t> (codes.size());
	// Выводим индекс созданного узла в арене узлов
	return result;
}
/**
 * @brief Метод создания узла рекурсивного вызова
 *
 * @param number номер вызываемой группы либо нуль для вызова выражения целиком
 * @param index  индекс имени вызываемой группы в хранилище имён
 * @param offset смещение вызова в тексте регулярного выражения
 * @return       индекс созданного узла в арене узлов
 *
 */
awh::regex::node_id_t awh::regex::Parser::makeRecurse(const uint32_t number, const uint32_t index, const size_t offset) noexcept {
	// Выполняем создание узла рекурсивного вызова
	const node_id_t result = this->createNode(node_t::RECURSE);
	// Выполняем установку номера вызываемой группы
	this->_nodes.at(result).recurse.number = number;
	// Выполняем установку индекса имени вызываемой группы
	this->_nodes.at(result).recurse.name = index;
	/**
	 * Если рекурсивный вызов задан именем группы
	 */
	if(index != NO_NAME) {
		// Создаём отложенную ссылку на именованную группу
		deferred_t deferred;
		// Выполняем установку индекса узла вызова
		deferred.node = result;
		// Выполняем установку индекса имени группы
		deferred.name = index;
		// Выполняем установку смещения вызова
		deferred.offset = static_cast <uint32_t> (offset);
		// Выполняем добавление отложенной ссылки
		this->_deferred.push_back(deferred);
	}
	// Выводим индекс созданного узла в арене узлов
	return result;
}
/**
 * @brief Метод создания узла ссылки на именованную группу
 *
 * @param index  индекс имени группы в хранилище имён
 * @param offset смещение ссылки в тексте регулярного выражения
 * @return       индекс созданного узла в арене узлов
 *
 */
awh::regex::node_id_t awh::regex::Parser::makeBackref(const uint32_t index, const size_t offset) noexcept {
	// Выполняем создание узла ссылки на захваченную группу
	const node_id_t result = this->createNode(node_t::BACKREF);
	// Выполняем установку индекса имени группы
	this->_nodes.at(result).backref.name = index;
	// Выполняем установку неразрешённого номера группы
	this->_nodes.at(result).backref.number = 0;
	// Создаём отложенную ссылку на именованную группу
	deferred_t deferred;
	// Выполняем установку индекса узла ссылки
	deferred.node = result;
	// Выполняем установку индекса имени группы
	deferred.name = index;
	// Выполняем установку смещения ссылки
	deferred.offset = static_cast <uint32_t> (offset);
	// Выполняем добавление отложенной ссылки
	this->_deferred.push_back(deferred);
	// Выводим индекс созданного узла в арене узлов
	return result;
}
/**
 * @brief Метод приведения класса символов к нормальному виду
 *
 * @param value класс символов для приведения к нормальному виду
 *
 */
void awh::regex::Parser::normalize(class_t & value) const noexcept {
	/**
	 * Если набор диапазонов класса символов пуст
	 */
	if(value.ranges.empty())
		// Выходим из функции
		return;
	/**
	 * Выполняем упорядочивание диапазонов по нижней границе
	 */
	::sort(value.ranges.begin(), value.ranges.end(), [](const range_t & a, const range_t & b) noexcept -> bool {
		// Выводим результат сравнения границ диапазонов
		return ((a.begin < b.begin) || ((a.begin == b.begin) && (a.end < b.end)));
	});
	// Получаем позицию записи объединённого диапазона
	size_t index = 0;
	/**
	 * Выполняем объединение пересекающихся и смежных диапазонов
	 */
	for(size_t i = 1; i < value.ranges.size(); i++) {
		/**
		 * Если очередной диапазон пересекается с объединяемым либо смежен ему
		 */
		if(value.ranges.at(i).begin <= (value.ranges.at(index).end + 1)) {
			/**
			 * Если очередной диапазон расширяет верхнюю границу
			 */
			if(value.ranges.at(i).end > value.ranges.at(index).end)
				// Выполняем расширение верхней границы объединяемого диапазона
				value.ranges.at(index).end = value.ranges.at(i).end;
		// Если очередной диапазон не пересекается с объединяемым
		} else {
			// Переходим к следующей позиции записи
			index++;
			// Выполняем запись очередного диапазона
			value.ranges.at(index) = value.ranges.at(i);
		}
	}
	// Выполняем усечение набора диапазонов до объединённого размера
	value.ranges.resize(index + 1);
}
/**
 * @brief Метод создания узла класса символов
 *
 * @param value класс символов для размещения в хранилище классов
 * @return      индекс созданного узла в арене узлов
 *
 */
awh::regex::node_id_t awh::regex::Parser::makeClass(class_t & value) noexcept {
	// Выполняем приведение класса символов к нормальному виду
	this->normalize(value);
	// Получаем индекс класса символов в хранилище классов
	const uint32_t index = static_cast <uint32_t> (this->_classes.size());
	// Выполняем размещение класса символов в хранилище классов
	this->_classes.push_back(value);
	// Выполняем создание узла класса символов
	const node_id_t result = this->createNode(node_t::CLASS);
	// Выполняем установку индекса класса символов
	this->_nodes.at(result).charclass.index = index;
	// Выводим индекс созданного узла в арене узлов
	return result;
}
/**
 * @brief Метод добавления сокращённого класса символов
 *
 * @param letter буква сокращённого класса символов
 * @param result класс символов для добавления диапазонов
 * @return       результат добавления сокращённого класса символов
 *
 */
bool awh::regex::Parser::shorthand(const char letter, class_t & result) const noexcept {
	// Создаём набор диапазонов сокращённого класса символов
	vector <range_t> ranges;
	/**
	 * Если установлен режим соответствия сокращённых классов свойствам Юникода
	 *
	 * @details Режим «UCP» заменяет наборы символов ASCII, задающие сокращённые классы,
	 *          соответствующими свойствами Юникода, благодаря чему сокращённые классы
	 *          соответствуют символам за пределами набора ASCII.
	 *
	 */
	if(hasFlag(this->_flags, flag_t::UCP)) {
		// Идентификатор свойства Юникода сокращённого класса символов
		uint16_t id = static_cast <uint16_t> (property_id_t::UNKNOWN);
		/**
		 * Определяем букву сокращённого класса символов
		 */
		switch(ascii::toLower(letter)) {
			// Выполняем установку свойства десятичных цифр
			case 'd': id = static_cast <uint16_t> (property_id_t::Nd); break;
			// Выполняем установку свойства символов слова
			case 'w': id = static_cast <uint16_t> (property_id_t::UCP_WORD); break;
			// Выполняем установку свойства пробельных символов
			case 's': id = static_cast <uint16_t> (property_id_t::UCP_SPACE); break;
		}
		/**
		 * Если свойство Юникода сокращённого класса определено
		 */
		if(id != static_cast <uint16_t> (property_id_t::UNKNOWN)) {
			// Выполняем добавление свойства Юникода в класс символов
			result.properties.emplace_back(id, ascii::isUpper(letter));
			// Выводим результат формирования сокращённого класса символов
			return true;
		}
	}
	/**
	 * Определяем букву сокращённого класса символов
	 */
	switch(ascii::toLower(letter)) {
		// Выполняем формирование класса десятичных цифр
		case 'd': ranges.emplace_back(0x30, 0x39); break;
		// Выполняем формирование класса символов слова
		case 'w': {
			// Добавляем диапазон десятичных цифр
			ranges.emplace_back(0x30, 0x39);
			// Добавляем диапазон прописных букв
			ranges.emplace_back(0x41, 0x5A);
			// Добавляем знак подчёркивания
			ranges.emplace_back(0x5F, 0x5F);
			// Добавляем диапазон строчных букв
			ranges.emplace_back(0x61, 0x7A);
		} break;
		// Выполняем формирование класса пробельных символов
		case 's': {
			// Добавляем диапазон управляющих пробельных символов
			ranges.emplace_back(0x09, 0x0D);
			// Добавляем символ пробела
			ranges.emplace_back(0x20, 0x20);
		} break;
		// Выполняем формирование класса горизонтальных пробельных символов
		case 'h': {
			// Добавляем символ горизонтальной табуляции
			ranges.emplace_back(0x09, 0x09);
			// Добавляем символ пробела
			ranges.emplace_back(0x20, 0x20);
			// Добавляем неразрывный пробел
			ranges.emplace_back(0xA0, 0xA0);
			// Добавляем пробел огама
			ranges.emplace_back(0x1680, 0x1680);
			// Добавляем разделитель монгольской гласной
			ranges.emplace_back(0x180E, 0x180E);
			// Добавляем диапазон пробелов различной ширины
			ranges.emplace_back(0x2000, 0x200A);
			// Добавляем узкий неразрывный пробел
			ranges.emplace_back(0x202F, 0x202F);
			// Добавляем математический пробел средней ширины
			ranges.emplace_back(0x205F, 0x205F);
			// Добавляем идеографический пробел
			ranges.emplace_back(0x3000, 0x3000);
		} break;
		// Выполняем формирование класса вертикальных пробельных символов
		case 'v': {
			// Добавляем диапазон символов перевода строки
			ranges.emplace_back(0x0A, 0x0D);
			// Добавляем символ следующей строки
			ranges.emplace_back(0x85, 0x85);
			// Добавляем разделители строк и абзацев
			ranges.emplace_back(0x2028, 0x2029);
		} break;
		// Выводим отсутствие сокращённого класса символов
		default: return false;
	}
	/**
	 * Если сокращённый класс символов задан прописной буквой
	 */
	if(ascii::isUpper(letter)) {
		// Получаем наибольшее кодовое значение символа для текущего режима
		const uint32_t maximum = (hasFlag(this->_flags, flag_t::UTF) ? MAX_CODEPOINT : 0xFF);
		// Получаем нижнюю границу дополняемого диапазона
		uint32_t begin = 0;
		/**
		 * Выполняем формирование дополнения набора диапазонов
		 */
		for(auto & range : ranges) {
			/**
			 * Если дополняемый диапазон не пуст
			 */
			if(range.begin > begin)
				// Добавляем дополняемый диапазон в класс символов
				result.ranges.emplace_back(begin, range.begin - 1);
			/**
			 * Если верхняя граница диапазона не достигла предела
			 */
			if(range.end < maximum)
				// Переходим к следующему дополняемому диапазону
				begin = (range.end + 1);
			// Если верхняя граница диапазона достигла предела
			else {
				// Выполняем сброс нижней границы дополняемого диапазона
				begin = maximum;
				// Выходим из цикла формирования дополнения
				break;
			}
		}
		/**
		 * Если последний дополняемый диапазон не пуст
		 */
		if(begin <= maximum)
			// Добавляем последний дополняемый диапазон в класс символов
			result.ranges.emplace_back(begin, maximum);
		// Выводим результат добавления сокращённого класса символов
		return true;
	}
	// Выполняем добавление диапазонов в класс символов
	result.ranges.insert(result.ranges.end(), ranges.begin(), ranges.end());
	// Выводим результат добавления сокращённого класса символов
	return true;
}
/**
 * @brief Метод извлечения кодового значения символа в текущей позиции
 *
 * @param code кодовое значение извлечённого символа
 * @return     результат извлечения кодового значения символа
 *
 */
bool awh::regex::Parser::readCode(uint32_t & code) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	/**
	 * Если позиция разбора достигла конца регулярного выражения
	 */
	if(this->_pos >= size)
		// Выводим отсутствие кодового значения символа
		return false;
	// Получаем первый байт последовательности
	const uint8_t first = static_cast <uint8_t> (this->_pattern.at(this->_pos));
	/**
	 * Если режим разбора UTF-8 не установлен либо байт является символом ASCII
	 */
	if(!hasFlag(this->_flags, flag_t::UTF) || (first < 0x80)) {
		// Выполняем установку кодового значения символа
		code = static_cast <uint32_t> (first);
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
		// Выводим результат извлечения кодового значения символа
		return true;
	}
	// Количество продолжающих байтов последовательности
	size_t extra = 0;
	// Кодовое значение разбираемого символа
	uint32_t value = 0;
	/**
	 * Если последовательность состоит из двух байтов
	 */
	if((first & 0xE0) == 0xC0) {
		// Выполняем установку количества продолжающих байтов
		extra = 1;
		// Выполняем установку старших битов кодового значения
		value = static_cast <uint32_t> (first & 0x1F);
	/**
	 * Если последовательность состоит из трёх байтов
	 */
	} else if((first & 0xF0) == 0xE0) {
		// Выполняем установку количества продолжающих байтов
		extra = 2;
		// Выполняем установку старших битов кодового значения
		value = static_cast <uint32_t> (first & 0x0F);
	/**
	 * Если последовательность состоит из четырёх байтов
	 */
	} else if((first & 0xF8) == 0xF0) {
		// Выполняем установку количества продолжающих байтов
		extra = 3;
		// Выполняем установку старших битов кодового значения
		value = static_cast <uint32_t> (first & 0x07);
	// Если первый байт последовательности некорректен
	} else
		// Выводим отсутствие кодового значения символа
		return false;
	/**
	 * Если последовательность выходит за пределы регулярного выражения
	 */
	if((this->_pos + extra) >= size)
		// Выводим отсутствие кодового значения символа
		return false;
	/**
	 * Выполняем разбор продолжающих байтов последовательности
	 */
	for(size_t i = 1; i <= extra; i++) {
		// Получаем очередной продолжающий байт последовательности
		const uint8_t next = static_cast <uint8_t> (this->_pattern.at(this->_pos + i));
		/**
		 * Если байт не является продолжающим
		 */
		if((next & 0xC0) != 0x80)
			// Выводим отсутствие кодового значения символа
			return false;
		// Выполняем добавление битов кодового значения
		value = ((value << 6) | static_cast <uint32_t> (next & 0x3F));
	}
	// Создаём таблицу наименьших кодовых значений для длин последовательности
	static const uint32_t minimum[4] = {0x0, 0x80, 0x800, 0x10000};
	/**
	 * Если кодовое значение записано избыточной последовательностью
	 */
	if(value < minimum[extra])
		// Выводим отсутствие кодового значения символа
		return false;
	/**
	 * Если кодовое значение превышает предел либо является суррогатным
	 */
	if((value > MAX_CODEPOINT) || ((value >= 0xD800) && (value <= 0xDFFF)))
		// Выводим отсутствие кодового значения символа
		return false;
	// Переходим к следующему символу регулярного выражения
	this->_pos += (extra + 1);
	// Выполняем установку кодового значения символа
	code = value;
	// Выводим результат извлечения кодового значения символа
	return true;
}
/**
 * @brief Метод извлечения целого числа в текущей позиции
 *
 * @param result извлечённое значение целого числа
 * @return       результат извлечения целого числа
 *
 */
bool awh::regex::Parser::readNumber(uint32_t & result) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Выполняем сброс извлекаемого значения целого числа
	result = 0;
	// Количество разобранных десятичных цифр
	size_t count = 0;
	/**
	 * Выполняем разбор последовательности десятичных цифр
	 */
	while((this->_pos < size) && ascii::isDigit(this->_pattern.at(this->_pos))) {
		/**
		 * Если разобранное значение не превышает допустимого предела
		 *
		 * @details Значение, превышающее предел, фиксируется на предельном значении,
		 *          а не приводит к отказу извлечения. Извлечение остаётся успешным,
		 *          благодаря чему превышение предела обнаруживается потребителем
		 *          как ошибка значения, а не как отсутствие числа.
		 *
		 */
		if(result <= MAX_REPEAT)
			// Выполняем добавление очередной десятичной цифры
			result = ((result * 10) + static_cast <uint32_t> (this->_pattern.at(this->_pos) - '0'));
		// Выполняем фиксацию значения на предельном значении
		else result = (MAX_REPEAT + 1);
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
		// Увеличиваем количество разобранных десятичных цифр
		count++;
	}
	// Выводим результат извлечения целого числа
	return (count > 0);
}
/**
 * @brief Метод проверки начала квантора повторения
 *
 * @param pos позиция проверяемой последовательности
 * @return    результат проверки начала квантора повторения
 *
 */
bool awh::regex::Parser::isQuantifier(const size_t pos) const noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	/**
	 * Если последовательность не начинается открывающей фигурной скобкой
	 */
	if((pos >= size) || (this->_pattern.at(pos) != '{'))
		// Выводим результат проверки начала квантора повторения
		return false;
	// Получаем позицию разбора наименьшего числа повторений
	size_t index = (pos + 1);
	// Количество разобранных десятичных цифр наименьшего числа повторений
	size_t count = 0;
	/**
	 * Выполняем разбор наименьшего числа повторений
	 */
	while((index < size) && ascii::isDigit(this->_pattern.at(index))) {
		// Переходим к следующему символу регулярного выражения
		index++;
		// Увеличиваем количество разобранных десятичных цифр
		count++;
	}
	/**
	 * Если квантор повторения не завершён
	 */
	if(index >= size)
		// Выводим результат проверки начала квантора повторения
		return false;
	/**
	 * Если наименьшее число повторений опущено
	 *
	 * @details Квантор повторения допускает опускание наименьшего числа
	 *          повторений, которое в этом случае принимается равным нулю.
	 *          Отсутствие обеих границ кванта повторения квантора не образует.
	 *
	 */
	if((count == 0) && (this->_pattern.at(index) != ','))
		// Выводим результат проверки начала квантора повторения
		return false;
	/**
	 * Если квантор задаёт точное число повторений
	 */
	if(this->_pattern.at(index) == '}')
		// Выводим результат проверки начала квантора повторения
		return true;
	/**
	 * Если квантор не содержит разделителя границ диапазона
	 */
	if(this->_pattern.at(index) != ',')
		// Выводим результат проверки начала квантора повторения
		return false;
	// Переходим к символу за разделителем границ диапазона
	index++;
	/**
	 * Выполняем разбор наибольшего числа повторений
	 */
	while((index < size) && ascii::isDigit(this->_pattern.at(index)))
		// Переходим к следующему символу регулярного выражения
		index++;
	// Выводим результат проверки начала квантора повторения
	return ((index < size) && (this->_pattern.at(index) == '}'));
}
/**
 * @brief Метод пропуска пробельных символов и комментариев
 *
 */
void awh::regex::Parser::skipSpaces() noexcept {
	/**
	 * Если режим игнорирования пробельных символов не установлен
	 */
	if(!hasFlag(this->_flags, flag_t::EXTENDED))
		// Выходим из функции
		return;
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	/**
	 * Выполняем пропуск пробельных символов и комментариев
	 */
	while(this->_pos < size) {
		// Получаем очередной символ регулярного выражения
		const char letter = this->_pattern.at(this->_pos);
		/**
		 * Если символ является пробельным
		 */
		if(ascii::isSpace(letter)) {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Переходим к следующей итерации пропуска
			continue;
		}
		/**
		 * Если символ начинает комментарий
		 */
		if(letter == '#') {
			/**
			 * Выполняем пропуск комментария до конца строки
			 */
			while((this->_pos < size) && (this->_pattern.at(this->_pos) != '\n'))
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
			// Переходим к следующей итерации пропуска
			continue;
		}
		// Выходим из цикла пропуска пробельных символов
		break;
	}
}
/**
 * @brief Метод извлечения кодового значения экранированного символа
 *
 * @param code кодовое значение разобранного символа
 * @return     результат извлечения кодового значения символа
 *
 */
bool awh::regex::Parser::readEscapeCode(uint32_t & code) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	/**
	 * Если позиция разбора достигла конца регулярного выражения
	 */
	if(this->_pos >= size)
		// Выводим отсутствие кодового значения символа
		return false;
	// Получаем букву экранированной последовательности
	const char letter = this->_pattern.at(this->_pos);
	/**
	 * Определяем букву экранированной последовательности
	 */
	switch(letter) {
		// Выполняем разбор символа звонка
		case 'a': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения символа звонка
			code = 0x07;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор символа переключения
		case 'e': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения символа переключения
			code = 0x1B;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор символа перевода страницы
		case 'f': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения символа перевода страницы
			code = 0x0C;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор символа перевода строки
		case 'n': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения символа перевода строки
			code = 0x0A;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор символа возврата каретки
		case 'r': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения символа возврата каретки
			code = 0x0D;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор символа горизонтальной табуляции
		case 't': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения символа табуляции
			code = 0x09;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор управляющей последовательности
		case 'c': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Если управляющая последовательность не содержит символа
			 */
			if(this->_pos >= size)
				// Выводим отсутствие кодового значения символа
				return false;
			// Получаем символ управляющей последовательности
			const uint8_t control = static_cast <uint8_t> (this->_pattern.at(this->_pos));
			/**
			 * Если символ управляющей последовательности не является символом ASCII
			 */
			if(control > 0x7F)
				// Выводим отсутствие кодового значения символа
				return false;
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку кодового значения управляющего символа
			code = (static_cast <uint32_t> (ascii::toUpper(static_cast <char> (control))) ^ 0x40);
			// Выводим результат извлечения кодового значения символа
			return true;
		}
		// Выполняем разбор шестнадцатеричной последовательности
		case 'x': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем сброс кодового значения символа
			code = 0;
			/**
			 * Если шестнадцатеричная последовательность заключена в фигурные скобки
			 */
			if((this->_pos < size) && (this->_pattern.at(this->_pos) == '{')) {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Количество разобранных шестнадцатеричных цифр
				size_t count = 0;
				/**
				 * Выполняем разбор последовательности шестнадцатеричных цифр
				 */
				while((this->_pos < size) && ascii::isHex(this->_pattern.at(this->_pos))) {
					// Выполняем добавление очередной шестнадцатеричной цифры
					code = ((code << 4) | hexValue(this->_pattern.at(this->_pos)));
					/**
					 * Если разобранное значение превышает допустимый предел
					 */
					if(code > MAX_CODEPOINT)
						// Выводим отсутствие кодового значения символа
						return false;
					// Переходим к следующему символу регулярного выражения
					this->_pos++;
					// Увеличиваем количество разобранных шестнадцатеричных цифр
					count++;
				}
				/**
				 * Если последовательность пуста либо не завершена фигурной скобкой
				 */
				if((count == 0) || (this->_pos >= size) || (this->_pattern.at(this->_pos) != '}'))
					// Выводим отсутствие кодового значения символа
					return false;
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Выводим результат извлечения кодового значения символа
				return true;
			}
			// Количество разобранных шестнадцатеричных цифр
			size_t digits = 0;
			/**
			 * Выполняем разбор не более двух шестнадцатеричных цифр
			 */
			for(size_t i = 0; i < 2; i++) {
				/**
				 * Если очередной символ не является шестнадцатеричной цифрой
				 */
				if((this->_pos >= size) || !ascii::isHex(this->_pattern.at(this->_pos)))
					// Выходим из цикла разбора шестнадцатеричных цифр
					break;
				// Выполняем добавление очередной шестнадцатеричной цифры
				code = ((code << 4) | hexValue(this->_pattern.at(this->_pos)));
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Увеличиваем количество разобранных шестнадцатеричных цифр
				digits++;
			}
			// Выводим результат извлечения кодового значения символа
			return (digits > 0);
		}
		// Выполняем разбор восьмеричной последовательности в фигурных скобках
		case 'o': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Если последовательность не начинается фигурной скобкой
			 */
			if((this->_pos >= size) || (this->_pattern.at(this->_pos) != '{'))
				// Выводим отсутствие кодового значения символа
				return false;
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем сброс кодового значения символа
			code = 0;
			// Количество разобранных восьмеричных цифр
			size_t count = 0;
			/**
			 * Выполняем разбор последовательности восьмеричных цифр
			 */
			while((this->_pos < size) && ascii::isOctal(this->_pattern.at(this->_pos))) {
				// Выполняем добавление очередной восьмеричной цифры
				code = ((code << 3) | static_cast <uint32_t> (this->_pattern.at(this->_pos) - '0'));
				/**
				 * Если разобранное значение превышает допустимый предел
				 */
				if(code > MAX_CODEPOINT)
					// Выводим отсутствие кодового значения символа
					return false;
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Увеличиваем количество разобранных восьмеричных цифр
				count++;
			}
			/**
			 * Если последовательность пуста либо не завершена фигурной скобкой
			 */
			if((count == 0) || (this->_pos >= size) || (this->_pattern.at(this->_pos) != '}'))
				// Выводим отсутствие кодового значения символа
				return false;
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выводим результат извлечения кодового значения символа
			return true;
		}
	}
	/**
	 * Если последовательность является восьмеричной
	 */
	if(ascii::isOctal(letter)) {
		// Выполняем сброс кодового значения символа
		code = 0;
		/**
		 * Выполняем разбор не более трёх восьмеричных цифр
		 */
		for(size_t i = 0; i < 3; i++) {
			/**
			 * Если очередной символ не является восьмеричной цифрой
			 */
			if((this->_pos >= size) || !ascii::isOctal(this->_pattern.at(this->_pos)))
				// Выходим из цикла разбора восьмеричных цифр
				break;
			// Выполняем добавление очередной восьмеричной цифры
			code = ((code << 3) | static_cast <uint32_t> (this->_pattern.at(this->_pos) - '0'));
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
		}
		// Выводим результат извлечения кодового значения символа
		return true;
	}
	// Выводим отсутствие кодового значения символа
	return false;
}
/**
 * @brief Метод разбора имени именованной группы
 *
 * @param terminator символ завершения имени группы
 * @param index      индекс имени в хранилище имён
 * @return           результат выполнения разбора
 *
 */
bool awh::regex::Parser::parseName(const char terminator, uint32_t & index) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала имени группы
	const size_t offset = this->_pos;
	/**
	 * Выполняем поиск завершающего символа имени группы
	 */
	while((this->_pos < size) && (this->_pattern.at(this->_pos) != terminator)) {
		/**
		 * Если очередной символ недопустим в имени группы
		 */
		if(!isNameChar(this->_pattern.at(this->_pos), (this->_pos == offset))) {
			// Выполняем установку ошибки некорректного имени группы
			this->fail(error_t::BAD_GROUP_NAME, this->_pos);
			// Выводим результат выполнения разбора
			return false;
		}
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
	}
	/**
	 * Если имя группы пусто либо не завершено ожидаемым символом
	 */
	if((this->_pos == offset) || (this->_pos >= size)) {
		// Выполняем установку ошибки некорректного имени группы
		this->fail(error_t::BAD_GROUP_NAME, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	// Получаем имя именованной группы
	const string name(this->_pattern.substr(offset, this->_pos - offset));
	// Переходим к символу за завершающим символом имени группы
	this->_pos++;
	/**
	 * Выполняем поиск имени группы в хранилище имён
	 */
	for(size_t i = 0; i < this->_names.size(); i++) {
		/**
		 * Если имя группы уже размещено в хранилище имён
		 */
		if(this->_names.at(i).compare(name) == 0) {
			// Выполняем установку индекса имени в хранилище имён
			index = static_cast <uint32_t> (i);
			// Выводим результат выполнения разбора
			return true;
		}
	}
	// Выполняем установку индекса имени в хранилище имён
	index = static_cast <uint32_t> (this->_names.size());
	// Выполняем размещение имени группы в хранилище имён
	this->_names.push_back(name);
	// Выводим результат выполнения разбора
	return true;
}
/**
 * @brief Метод разбора свойства Юникода
 *
 * @param negative флаг отрицания свойства
 * @param result   класс символов для добавления свойства
 * @return         результат выполнения разбора
 *
 */
bool awh::regex::Parser::parseProperty(const bool negative, class_t & result) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала свойства Юникода
	const size_t offset = this->_pos;
	// Имя свойства Юникода
	string name;
	/**
	 * Если имя свойства заключено в фигурные скобки
	 */
	if((this->_pos < size) && (this->_pattern.at(this->_pos) == '{')) {
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
		// Получаем смещение начала имени свойства
		const size_t begin = this->_pos;
		/**
		 * Выполняем поиск завершающей фигурной скобки
		 */
		while((this->_pos < size) && (this->_pattern.at(this->_pos) != '}'))
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
		/**
		 * Если имя свойства не завершено фигурной скобкой
		 */
		if(this->_pos >= size) {
			// Выполняем установку ошибки неизвестного свойства Юникода
			this->fail(error_t::BAD_PROPERTY, offset);
			// Выводим результат выполнения разбора
			return false;
		}
		// Получаем имя свойства Юникода
		name = this->_pattern.substr(begin, this->_pos - begin);
		// Переходим к символу за завершающей фигурной скобкой
		this->_pos++;
	/**
	 * Если имя свойства задано единственной буквой
	 */
	} else if(this->_pos < size) {
		// Получаем имя свойства Юникода
		name.assign(1, this->_pattern.at(this->_pos));
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
	// Если имя свойства отсутствует
	} else {
		// Выполняем установку ошибки неизвестного свойства Юникода
		this->fail(error_t::BAD_PROPERTY, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	// Флаг отрицания свойства Юникода
	bool inverse = negative;
	/**
	 * Если имя свойства начинается со знака отрицания
	 */
	if(!name.empty() && (name.front() == '^')) {
		// Выполняем инвертирование флага отрицания свойства
		inverse = !inverse;
		// Выполняем удаление знака отрицания из имени свойства
		name.erase(0, 1);
	}
	// Выполняем извлечение идентификатора свойства Юникода по его имени
	const uint16_t id = awh::unicode::property(name);
	/**
	 * Если имя свойства Юникода не распознано
	 */
	if(id == static_cast <uint16_t> (property_id_t::UNKNOWN)) {
		// Выполняем установку ошибки неизвестного свойства Юникода
		this->fail(error_t::BAD_PROPERTY, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	result.properties.emplace_back(id, inverse);
	// Выводим результат выполнения разбора
	return true;
}
/**
 * @brief Метод разбора класса символов POSIX
 *
 * @param result класс символов для добавления диапазонов
 * @return       результат выполнения разбора
 *
 */
bool awh::regex::Parser::parsePosix(class_t & result) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала класса символов POSIX
	const size_t offset = this->_pos;
	// Переходим к символу за открывающей последовательностью
	this->_pos += 2;
	// Флаг отрицания класса символов POSIX
	bool negative = false;
	/**
	 * Если класс символов POSIX начинается со знака отрицания
	 */
	if((this->_pos < size) && (this->_pattern.at(this->_pos) == '^')) {
		// Выполняем установку флага отрицания класса символов
		negative = true;
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
	}
	// Получаем смещение начала имени класса символов POSIX
	const size_t begin = this->_pos;
	/**
	 * Выполняем поиск завершающей последовательности класса символов
	 */
	while((this->_pos < size) && ascii::isAlpha(this->_pattern.at(this->_pos)))
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
	/**
	 * Если класс символов POSIX не завершён ожидаемой последовательностью
	 */
	if(((this->_pos + 1) >= size) || (this->_pattern.at(this->_pos) != ':') || (this->_pattern.at(this->_pos + 1) != ']')) {
		// Выполняем установку ошибки неизвестного класса символов POSIX
		this->fail(error_t::BAD_POSIX_CLASS, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	// Получаем имя класса символов POSIX
	const string name(this->_pattern.substr(begin, this->_pos - begin));
	// Переходим к символу за завершающей последовательностью
	this->_pos += 2;
	// Создаём набор диапазонов класса символов POSIX
	vector <range_t> ranges;
	/**
	 * Если установлен режим соответствия сокращённых классов свойствам Юникода
	 *
	 * @details Режим «UCP» заменяет наборы символов ASCII, задающие классы POSIX,
	 *          соответствующими свойствами Юникода, - ровно так же, как поступает
	 *          он с сокращёнными классами. Состав каждого класса установлен
	 *          сличением с эталонной реализацией, а не выведен из названия: классы
	 *          «punct», «graph», «print» и «xdigit» общей категории Юникода не
	 *          отвечают вовсе и заданы свойствами составными.
	 *
	 *          Классы «blank» и «ascii» заданы диапазонами, а не свойствами:
	 *          первый совпадает с сокращённым классом «\h», второй от режима не
	 *          зависит, - а отрицание набора диапазонов выполняется дополнением,
	 *          отрицание же свойства ведётся признаком самого свойства.
	 */
	if(hasFlag(this->_flags, flag_t::UCP)) {
		// Идентификатор свойства Юникода класса символов POSIX
		uint16_t id = static_cast <uint16_t> (property_id_t::UNKNOWN);
		/**
		 * Если класс символов содержит буквы
		 */
		if(name.compare("alpha") == 0)
			// Выполняем установку свойства букв
			id = static_cast <uint16_t> (property_id_t::L);
		/**
		 * Если класс символов содержит буквы и цифры
		 */
		else if(name.compare("alnum") == 0)
			// Выполняем установку свойства букв и цифр
			id = static_cast <uint16_t> (property_id_t::XAN);
		/**
		 * Если класс символов содержит десятичные цифры
		 */
		else if(name.compare("digit") == 0)
			// Выполняем установку свойства десятичных цифр
			id = static_cast <uint16_t> (property_id_t::Nd);
		/**
		 * Если класс символов содержит пробельные символы
		 */
		else if(name.compare("space") == 0)
			// Выполняем установку свойства пробельных символов
			id = static_cast <uint16_t> (property_id_t::XPS);
		/**
		 * Если класс символов содержит прописные буквы
		 */
		else if(name.compare("upper") == 0)
			// Выполняем установку свойства прописных букв
			id = static_cast <uint16_t> (property_id_t::Lu);
		/**
		 * Если класс символов содержит строчные буквы
		 */
		else if(name.compare("lower") == 0)
			// Выполняем установку свойства строчных букв
			id = static_cast <uint16_t> (property_id_t::Ll);
		/**
		 * Если класс символов содержит символы слова
		 */
		else if(name.compare("word") == 0)
			// Выполняем установку свойства символов слова
			id = static_cast <uint16_t> (property_id_t::XWD);
		/**
		 * Если класс символов содержит управляющие символы
		 */
		else if(name.compare("cntrl") == 0)
			// Выполняем установку свойства управляющих символов
			id = static_cast <uint16_t> (property_id_t::Cc);
		/**
		 * Если класс символов содержит знаки пунктуации
		 */
		else if(name.compare("punct") == 0)
			// Выполняем установку свойства знаков пунктуации
			id = static_cast <uint16_t> (property_id_t::PX_PUNCT);
		/**
		 * Если класс символов содержит видимые символы
		 */
		else if(name.compare("graph") == 0)
			// Выполняем установку свойства видимых символов
			id = static_cast <uint16_t> (property_id_t::PX_GRAPH);
		/**
		 * Если класс символов содержит печатаемые символы
		 */
		else if(name.compare("print") == 0)
			// Выполняем установку свойства печатаемых символов
			id = static_cast <uint16_t> (property_id_t::PX_PRINT);
		/**
		 * Если класс символов содержит шестнадцатеричные цифры
		 */
		else if(name.compare("xdigit") == 0)
			// Выполняем установку свойства шестнадцатеричных цифр
			id = static_cast <uint16_t> (property_id_t::PX_XDIGIT);
		/**
		 * Если свойство Юникода класса символов POSIX определено
		 */
		if(id != static_cast <uint16_t> (property_id_t::UNKNOWN)) {
			// Выполняем добавление свойства Юникода в класс символов
			result.properties.emplace_back(id, negative);
			// Выводим результат выполнения разбора
			return true;
		}
		/**
		 * Если класс символов содержит горизонтальные пробельные символы
		 */
		if(name.compare("blank") == 0) {
			// Добавляем символ горизонтальной табуляции
			ranges.emplace_back(0x09, 0x09);
			// Добавляем символ пробела
			ranges.emplace_back(0x20, 0x20);
			// Добавляем неразрывный пробел
			ranges.emplace_back(0xA0, 0xA0);
			// Добавляем пробел огама
			ranges.emplace_back(0x1680, 0x1680);
			// Добавляем разделитель гласной монгольского письма
			ranges.emplace_back(0x180E, 0x180E);
			// Добавляем диапазон пробелов различной ширины
			ranges.emplace_back(0x2000, 0x200A);
			// Добавляем узкий неразрывный пробел
			ranges.emplace_back(0x202F, 0x202F);
			// Добавляем средний математический пробел
			ranges.emplace_back(0x205F, 0x205F);
			// Добавляем идеографический пробел
			ranges.emplace_back(0x3000, 0x3000);
		}
	}
	/**
	 * Если набор диапазонов сформирован режимом соответствия Юникоду
	 *
	 * @details Набор непустой означает, что класс уже задан выше режимом «UCP»,
	 *          и набор диапазонов набора ASCII, ниже собираемый, к делу не идёт.
	 */
	if(!ranges.empty())
		// Пропускаем формирование набора диапазонов набора ASCII
		;
	/**
	 * Если класс символов содержит буквы
	 */
	else if(name.compare("alpha") == 0) {
		// Добавляем диапазон прописных букв
		ranges.emplace_back(0x41, 0x5A);
		// Добавляем диапазон строчных букв
		ranges.emplace_back(0x61, 0x7A);
	/**
	 * Если класс символов содержит десятичные цифры
	 */
	} else if(name.compare("digit") == 0)
		// Добавляем диапазон десятичных цифр
		ranges.emplace_back(0x30, 0x39);
	/**
	 * Если класс символов содержит буквы и цифры
	 */
	else if(name.compare("alnum") == 0) {
		// Добавляем диапазон десятичных цифр
		ranges.emplace_back(0x30, 0x39);
		// Добавляем диапазон прописных букв
		ranges.emplace_back(0x41, 0x5A);
		// Добавляем диапазон строчных букв
		ranges.emplace_back(0x61, 0x7A);
	/**
	 * Если класс символов содержит пробельные символы
	 */
	} else if(name.compare("space") == 0) {
		// Добавляем диапазон управляющих пробельных символов
		ranges.emplace_back(0x09, 0x0D);
		// Добавляем символ пробела
		ranges.emplace_back(0x20, 0x20);
	/**
	 * Если класс символов содержит прописные буквы
	 */
	} else if(name.compare("upper") == 0)
		// Добавляем диапазон прописных букв
		ranges.emplace_back(0x41, 0x5A);
	/**
	 * Если класс символов содержит строчные буквы
	 */
	else if(name.compare("lower") == 0)
		// Добавляем диапазон строчных букв
		ranges.emplace_back(0x61, 0x7A);
	/**
	 * Если класс символов содержит знаки пунктуации
	 */
	else if(name.compare("punct") == 0) {
		// Добавляем диапазон знаков пунктуации до цифр
		ranges.emplace_back(0x21, 0x2F);
		// Добавляем диапазон знаков пунктуации после цифр
		ranges.emplace_back(0x3A, 0x40);
		// Добавляем диапазон знаков пунктуации после прописных букв
		ranges.emplace_back(0x5B, 0x60);
		// Добавляем диапазон знаков пунктуации после строчных букв
		ranges.emplace_back(0x7B, 0x7E);
	/**
	 * Если класс символов содержит печатаемые символы
	 */
	} else if(name.compare("print") == 0)
		// Добавляем диапазон печатаемых символов
		ranges.emplace_back(0x20, 0x7E);
	/**
	 * Если класс символов содержит видимые символы
	 */
	else if(name.compare("graph") == 0)
		// Добавляем диапазон видимых символов
		ranges.emplace_back(0x21, 0x7E);
	/**
	 * Если класс символов содержит управляющие символы
	 */
	else if(name.compare("cntrl") == 0) {
		// Добавляем диапазон управляющих символов
		ranges.emplace_back(0x00, 0x1F);
		// Добавляем символ удаления
		ranges.emplace_back(0x7F, 0x7F);
	/**
	 * Если класс символов содержит шестнадцатеричные цифры
	 */
	} else if(name.compare("xdigit") == 0) {
		// Добавляем диапазон десятичных цифр
		ranges.emplace_back(0x30, 0x39);
		// Добавляем диапазон прописных шестнадцатеричных цифр
		ranges.emplace_back(0x41, 0x46);
		// Добавляем диапазон строчных шестнадцатеричных цифр
		ranges.emplace_back(0x61, 0x66);
	/**
	 * Если класс символов содержит горизонтальные пробельные символы
	 */
	} else if(name.compare("blank") == 0) {
		// Добавляем символ горизонтальной табуляции
		ranges.emplace_back(0x09, 0x09);
		// Добавляем символ пробела
		ranges.emplace_back(0x20, 0x20);
	/**
	 * Если класс символов содержит символы слова
	 */
	} else if(name.compare("word") == 0) {
		// Добавляем диапазон десятичных цифр
		ranges.emplace_back(0x30, 0x39);
		// Добавляем диапазон прописных букв
		ranges.emplace_back(0x41, 0x5A);
		// Добавляем знак подчёркивания
		ranges.emplace_back(0x5F, 0x5F);
		// Добавляем диапазон строчных букв
		ranges.emplace_back(0x61, 0x7A);
	/**
	 * Если класс символов содержит символы ASCII
	 */
	} else if(name.compare("ascii") == 0)
		// Добавляем диапазон символов ASCII
		ranges.emplace_back(0x00, 0x7F);
	// Если класс символов POSIX неизвестен
	else {
		// Выполняем установку ошибки неизвестного класса символов POSIX
		this->fail(error_t::BAD_POSIX_CLASS, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	/**
	 * Если класс символов POSIX задан со знаком отрицания
	 */
	if(negative) {
		// Получаем наибольшее кодовое значение символа для текущего режима
		const uint32_t maximum = (hasFlag(this->_flags, flag_t::UTF) ? MAX_CODEPOINT : 0xFF);
		// Получаем нижнюю границу дополняемого диапазона
		uint32_t begin = 0;
		/**
		 * Выполняем формирование дополнения набора диапазонов
		 */
		for(auto & range : ranges) {
			/**
			 * Если дополняемый диапазон не пуст
			 */
			if(range.begin > begin)
				// Добавляем дополняемый диапазон в класс символов
				result.ranges.emplace_back(begin, range.begin - 1);
			// Переходим к следующему дополняемому диапазону
			begin = (range.end + 1);
		}
		/**
		 * Если последний дополняемый диапазон не пуст
		 */
		if(begin <= maximum)
			// Добавляем последний дополняемый диапазон в класс символов
			result.ranges.emplace_back(begin, maximum);
		// Выводим результат выполнения разбора
		return true;
	}
	// Выполняем добавление диапазонов в класс символов
	result.ranges.insert(result.ranges.end(), ranges.begin(), ranges.end());
	// Выводим результат выполнения разбора
	return true;
}
/**
 * @brief Метод разбора экранированной последовательности внутри класса символов
 *
 * @param result класс символов для добавления диапазонов
 * @param code   кодовое значение разобранного одиночного символа
 * @param single флаг разбора одиночного символа
 * @return       результат выполнения разбора
 *
 */
bool awh::regex::Parser::parseClassEscape(class_t & result, uint32_t & code, bool & single) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала экранированной последовательности
	const size_t offset = this->_pos;
	// Переходим к символу за обратной косой чертой
	this->_pos++;
	/**
	 * Если экранированная последовательность не содержит символа
	 */
	if(this->_pos >= size) {
		// Выполняем установку ошибки обратной косой черты в конце выражения
		this->fail(error_t::TRAILING_BACKSLASH, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	// Выполняем сброс флага разбора одиночного символа
	single = false;
	// Получаем букву экранированной последовательности
	const char letter = this->_pattern.at(this->_pos);
	/**
	 * Определяем букву экранированной последовательности
	 */
	switch(letter) {
		// Выполняем разбор сокращённых классов символов
		case 'd': case 'D': case 'w': case 'W':
		case 's': case 'S': case 'h': case 'H':
		case 'v': case 'V': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем добавление сокращённого класса символов
			return this->shorthand(letter, result);
		}
		// Выполняем разбор свойства Юникода
		case 'p': case 'P': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем разбор свойства Юникода
			return this->parseProperty((letter == 'P'), result);
		}
		// Выполняем разбор завершения экранирования последовательности символов
		case 'E': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выводим результат выполнения разбора без добавления символов
			return true;
		}
		// Выполняем разбор экранирования последовательности символов
		case 'Q': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Выполняем разбор экранируемой последовательности символов
			 */
			while(this->_pos < size) {
				/**
				 * Если экранирование последовательности завершено
				 */
				if((this->_pattern.at(this->_pos) == '\\') && ((this->_pos + 1) < size) && (this->_pattern.at(this->_pos + 1) == 'E')) {
					// Переходим к символу за завершающей последовательностью
					this->_pos += 2;
					// Выводим результат выполнения разбора
					return true;
				}
				// Кодовое значение очередного символа последовательности
				uint32_t value = 0;
				/**
				 * Если извлечение кодового значения символа не выполнено
				 */
				if(!this->readCode(value)) {
					// Выполняем установку ошибки некорректной последовательности UTF-8
					this->fail(error_t::BAD_UTF8, this->_pos);
					// Выводим результат выполнения разбора
					return false;
				}
				// Выполняем добавление символа последовательности в класс символов
				result.ranges.emplace_back(value, value);
			}
			// Выводим результат выполнения разбора
			return true;
		}
		// Выполняем разбор литералов букв ссылок на группы
		case 'g': case 'k': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку флага разбора одиночного символа
			single = true;
			// Выполняем установку кодового значения буквы
			code = static_cast <uint32_t> (letter);
			// Выводим результат выполнения разбора
			return true;
		}
		// Выполняем разбор литералов десятичных цифр вне восьмеричного диапазона
		case '8': case '9': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку флага разбора одиночного символа
			single = true;
			// Выполняем установку кодового значения десятичной цифры
			code = static_cast <uint32_t> (letter);
			// Выводим результат выполнения разбора
			return true;
		}
		// Выполняем разбор символа возврата на позицию
		case 'b': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку флага разбора одиночного символа
			single = true;
			// Выполняем установку кодового значения символа возврата
			code = 0x08;
			// Выводим результат выполнения разбора
			return true;
		}
	}
	/**
	 * Если экранированная последовательность обозначает одиночный символ
	 */
	if(this->readEscapeCode(code)) {
		// Выполняем установку флага разбора одиночного символа
		single = true;
		// Выводим результат выполнения разбора
		return true;
	}
	/**
	 * Если ошибка разбора уже установлена
	 */
	if(this->_error != error_t::NONE)
		// Выводим результат выполнения разбора
		return false;
	/**
	 * Если экранированная последовательность содержит букву либо цифру
	 */
	if(ascii::isAlnum(letter)) {
		// Выполняем установку ошибки неизвестной экранированной последовательности
		this->fail(error_t::UNKNOWN_ESCAPE, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	// Выполняем установку флага разбора одиночного символа
	single = true;
	/**
	 * Если извлечение кодового значения экранированного символа не выполнено
	 */
	if(!this->readCode(code)) {
		// Выполняем установку ошибки некорректной последовательности UTF-8
		this->fail(error_t::BAD_UTF8, offset);
		// Выводим результат выполнения разбора
		return false;
	}
	// Выводим результат выполнения разбора
	return true;
}
/**
 * @brief Метод разбора класса символов
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseClass() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала класса символов
	const size_t offset = this->_pos;
	// Переходим к символу за открывающей квадратной скобкой
	this->_pos++;
	// Создаём формируемый класс символов
	class_t result;
	/**
	 * Если класс символов начинается со знака отрицания
	 */
	if((this->_pos < size) && (this->_pattern.at(this->_pos) == '^')) {
		// Выполняем установку флага отрицания класса символов
		result.negative = true;
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
	}
	/**
	 * Выполняем проверку размещения класса символов POSIX вне класса символов
	 *
	 * @details Класс символов POSIX допускается исключительно внутри класса символов,
	 *          поэтому последовательность вида «[:alpha:]», разобранная как класс
	 *          символов целиком, считается ошибкой размещения.
	 *
	 */
	if((this->_pos < size) && (this->_pattern.at(this->_pos) == ':')) {
		// Получаем позицию поиска завершения класса символов POSIX
		size_t position = (this->_pos + 1);
		/**
		 * Если класс символов POSIX начинается со знака отрицания
		 */
		if((position < size) && (this->_pattern.at(position) == '^'))
			// Пропускаем знак отрицания класса символов POSIX
			position++;
		// Получаем позицию начала имени класса символов POSIX
		const size_t begin = position;
		/**
		 * Выполняем поиск завершения имени класса символов POSIX
		 */
		while((position < size) && ascii::isAlpha(this->_pattern.at(position)))
			// Переходим к следующему символу регулярного выражения
			position++;
		/**
		 * Если последовательность является классом символов POSIX
		 */
		if((position > begin) && ((position + 1) < size) &&
		   (this->_pattern.at(position) == ':') && (this->_pattern.at(position + 1) == ']'))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_POSIX_CLASS, offset);
	}
	// Флаг разбора первого элемента класса символов
	bool first = true;
	/**
	 * Выполняем разбор элементов класса символов
	 */
	while(this->_pos < size) {
		/**
		 * Если класс символов завершён закрывающей квадратной скобкой
		 */
		if((this->_pattern.at(this->_pos) == ']') && !first) {
			// Переходим к символу за закрывающей квадратной скобкой
			this->_pos++;
			// Выводим индекс сформированного узла класса символов
			return this->makeClass(result);
		}
		/**
		 * Если элемент завершает экранирование последовательности символов
		 *
		 * @details Последовательность «\\E» удаляется из класса символов целиком
		 *          и элементом класса не является, поэтому закрывающая квадратная
		 *          скобка, следующая за ней в начале класса, остаётся литералом.
		 *
		 */
		if((this->_pattern.at(this->_pos) == '\\') && ((this->_pos + 1) < size) && (this->_pattern.at(this->_pos + 1) == 'E')) {
			// Переходим к символу за завершением экранирования
			this->_pos += 2;
			// Переходим к следующему элементу класса символов
			continue;
		}
		// Выполняем сброс флага разбора первого элемента
		first = false;
		/**
		 * Если элемент класса символов является классом символов POSIX
		 */
		if((this->_pattern.at(this->_pos) == '[') && ((this->_pos + 1) < size) && (this->_pattern.at(this->_pos + 1) == ':')) {
			/**
			 * Если разбор класса символов POSIX не выполнен
			 */
			if(!this->parsePosix(result))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
			// Переходим к следующему элементу класса символов
			continue;
		}
		// Кодовое значение нижней границы диапазона
		uint32_t begin = 0;
		// Флаг разбора одиночного символа
		bool single = true;
		/**
		 * Если элемент класса символов является экранированной последовательностью
		 */
		if(this->_pattern.at(this->_pos) == '\\') {
			/**
			 * Если разбор экранированной последовательности не выполнен
			 */
			if(!this->parseClassEscape(result, begin, single))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
		/**
		 * Если элемент класса символов является одиночным символом
		 */
		} else if(!this->readCode(begin))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_UTF8, this->_pos);
		/**
		 * Если элемент класса символов не является одиночным символом
		 */
		if(!single) {
			/**
			 * Если за элементом класса символов следует знак диапазона
			 *
			 * @details Сокращённый класс символов и свойство Юникода задают набор
			 *          символов, а не одиночный символ, поэтому границей диапазона
			 *          выступать не могут.
			 *
			 */
			if(((this->_pos + 1) < size) && (this->_pattern.at(this->_pos) == '-') && (this->_pattern.at(this->_pos + 1) != ']'))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_CLASS_RANGE, this->_pos);
			// Переходим к следующему элементу класса символов
			continue;
		}
		/**
		 * Если за элементом класса символов следует знак диапазона
		 */
		if(((this->_pos + 1) < size) && (this->_pattern.at(this->_pos) == '-') && (this->_pattern.at(this->_pos + 1) != ']')) {
			// Получаем смещение знака диапазона
			const size_t position = this->_pos;
			// Переходим к символу за знаком диапазона
			this->_pos++;
			// Кодовое значение верхней границы диапазона
			uint32_t end = 0;
			// Флаг разбора одиночного символа верхней границы
			bool bound = true;
			/**
			 * Если верхняя граница диапазона является экранированной последовательностью
			 */
			if(this->_pattern.at(this->_pos) == '\\') {
				/**
				 * Если разбор экранированной последовательности не выполнен
				 */
				if(!this->parseClassEscape(result, end, bound))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return INVALID_NODE;
				/**
				 * Если верхняя граница диапазона не является одиночным символом
				 */
				if(!bound)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_CLASS_RANGE, position);
			/**
			 * Если верхняя граница диапазона является одиночным символом
			 */
			} else if(!this->readCode(end))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_UTF8, this->_pos);
			/**
			 * Если границы диапазона указаны в обратном порядке
			 */
			if(end < begin)
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_CLASS_RANGE, position);
			// Выполняем добавление диапазона в класс символов
			result.ranges.emplace_back(begin, end);
			// Переходим к следующему элементу класса символов
			continue;
		}
		// Выполняем добавление одиночного символа в класс символов
		result.ranges.emplace_back(begin, begin);
	}
	// Выводим индекс отсутствующего узла синтаксического дерева
	return this->fail(error_t::UNMATCHED_BRACKET, offset);
}
/**
 * @brief Метод разбора экранированной последовательности вне класса символов
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseEscape() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала экранированной последовательности
	const size_t offset = this->_pos;
	// Переходим к символу за обратной косой чертой
	this->_pos++;
	/**
	 * Если экранированная последовательность не содержит символа
	 */
	if(this->_pos >= size)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::TRAILING_BACKSLASH, offset);
	// Получаем букву экранированной последовательности
	const char letter = this->_pattern.at(this->_pos);
	/**
	 * Определяем букву экранированной последовательности
	 */
	switch(letter) {
		// Выполняем разбор привязок к позиции в тексте
		case 'A': case 'z': case 'Z': case 'b': case 'B': case 'G': case 'K': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Если сброс начала совпадения размещён внутри проверки окружения
			 */
			if((letter == 'K') && (this->_look > 0))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::UNSUPPORTED, offset);
			// Выполняем создание узла привязки к позиции в тексте
			const node_id_t result = this->createNode(node_t::ANCHOR);
			/**
			 * Определяем букву привязки к позиции в тексте
			 */
			switch(letter) {
				// Выполняем установку привязки к началу текста
				case 'A': this->_nodes.at(result).anchor.type = anchor_t::TEXT_BEGIN; break;
				// Выполняем установку привязки к концу текста
				case 'z': this->_nodes.at(result).anchor.type = anchor_t::TEXT_END; break;
				// Выполняем установку привязки к концу текста с переводом строки
				case 'Z': this->_nodes.at(result).anchor.type = anchor_t::TEXT_FINISH; break;
				// Выполняем установку привязки к границе слова
				case 'b': this->_nodes.at(result).anchor.type = anchor_t::WORD_EDGE; break;
				// Выполняем установку привязки к положению вне границы слова
				case 'B': this->_nodes.at(result).anchor.type = anchor_t::WORD_INNER; break;
				// Выполняем установку привязки к началу попытки поиска
				case 'G': this->_nodes.at(result).anchor.type = anchor_t::SEARCH_HEAD; break;
				// Выполняем установку сброса начала совпадения
				case 'K': this->_nodes.at(result).anchor.type = anchor_t::KEEP_OUT; break;
			}
			// Выводим индекс сформированного узла синтаксического дерева
			return result;
		}
		// Выполняем разбор сокращённых классов символов
		case 'd': case 'D': case 'w': case 'W':
		case 's': case 'S': case 'h': case 'H':
		case 'v': case 'V': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Создаём формируемый класс символов
			class_t result;
			/**
			 * Если добавление сокращённого класса символов не выполнено
			 */
			if(!this->shorthand(letter, result))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::UNKNOWN_ESCAPE, offset);
			// Выводим индекс сформированного узла класса символов
			return this->makeClass(result);
		}
		// Выполняем разбор свойства Юникода
		case 'p': case 'P': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Создаём формируемый класс символов
			class_t result;
			/**
			 * Если разбор свойства Юникода не выполнен
			 */
			if(!this->parseProperty((letter == 'P'), result))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
			// Выводим индекс сформированного узла класса символов
			return this->makeClass(result);
		}
		// Выполняем разбор любого символа, кроме перевода строки
		case 'N': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Если последовательность задаёт символ его именем Юникода
			 */
			if((this->_pos < size) && (this->_pattern.at(this->_pos) == '{') && !this->isQuantifier(this->_pos))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::UNSUPPORTED, offset);
			// Сохраняем набор режимов компиляции
			const uint32_t saved = this->_flags;
			// Выполняем снятие режима соответствия точки переводу строки
			this->_flags &= ~flagOf(flag_t::DOTALL);
			// Выполняем создание узла любого символа
			const node_id_t result = this->createNode(node_t::ANY);
			// Выполняем восстановление набора режимов компиляции
			this->_flags = saved;
			// Выводим индекс сформированного узла синтаксического дерева
			return result;
		}
		// Выполняем разбор одиночной единицы кодирования
		case 'C': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выводим индекс созданного узла одиночной единицы кодирования
			return this->createNode(node_t::CODEUNIT);
		}
		// Выполняем разбор расширенного графемного кластера
		case 'X': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выводим индекс созданного узла расширенного графемного кластера
			return this->createNode(node_t::GRAPHEME);
		}
		// Выполняем разбор последовательности перевода строки
		case 'R': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Создаём набор кодовых значений последовательности возврата каретки
			const vector <uint32_t> codes = {0x0D, 0x0A};
			// Выполняем создание узла последовательности возврата каретки
			const node_id_t sequence = this->makeString(codes);
			// Создаём формируемый класс вертикальных пробельных символов
			class_t vertical;
			// Выполняем добавление класса вертикальных пробельных символов
			this->shorthand('v', vertical);
			// Выполняем создание узла класса вертикальных пробельных символов
			const node_id_t single = this->makeClass(vertical);
			// Создаём набор ветвей последовательности перевода строки
			const vector <node_id_t> items = {sequence, single};
			// Выводим индекс сформированного узла выбора одной из ветвей
			return this->makeList(node_t::ALTERNATE, items);
		}
		// Выполняем разбор экранирования последовательности символов
		case 'Q': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Создаём набор кодовых значений экранируемой последовательности
			vector <uint32_t> codes;
			/**
			 * Выполняем разбор экранируемой последовательности символов
			 */
			while(this->_pos < size) {
				/**
				 * Если экранирование последовательности завершено
				 */
				if((this->_pattern.at(this->_pos) == '\\') && ((this->_pos + 1) < size) && (this->_pattern.at(this->_pos + 1) == 'E')) {
					// Переходим к символу за завершающей последовательностью
					this->_pos += 2;
					// Выходим из цикла разбора экранируемой последовательности
					break;
				}
				// Кодовое значение очередного символа последовательности
				uint32_t code = 0;
				/**
				 * Если извлечение кодового значения символа не выполнено
				 */
				if(!this->readCode(code))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_UTF8, this->_pos);
				// Выполняем добавление кодового значения символа
				codes.push_back(code);
			}
			// Выводим индекс сформированного узла последовательности символов
			return this->makeString(codes);
		}
		// Выполняем разбор завершения экранирования последовательности символов
		case 'E': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выводим индекс созданного узла пустого выражения
			return this->createNode(node_t::EMPTY);
		}
		// Выполняем разбор ссылки на именованную группу
		case 'k': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Если ссылка на именованную группу не содержит имени
			 */
			if(this->_pos >= size)
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_GROUP_NAME, offset);
			// Символ завершения имени именованной группы
			char terminator = '\0';
			/**
			 * Определяем символ начала имени именованной группы
			 */
			switch(this->_pattern.at(this->_pos)) {
				// Выполняем установку завершения имени угловой скобкой
				case '<': terminator = '>'; break;
				// Выполняем установку завершения имени фигурной скобкой
				case '{': terminator = '}'; break;
				// Выполняем установку завершения имени апострофом
				case '\'': terminator = '\''; break;
				// Выводим индекс отсутствующего узла синтаксического дерева
				default: return this->fail(error_t::BAD_GROUP_NAME, this->_pos);
			}
			// Переходим к символу за началом имени именованной группы
			this->_pos++;
			// Индекс имени группы в хранилище имён
			uint32_t index = 0;
			/**
			 * Если разбор имени именованной группы не выполнен
			 */
			if(!this->parseName(terminator, index))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
			// Выводим индекс созданного узла ссылки на захваченную группу
			return this->makeBackref(index, offset);
		}
		// Выполняем разбор ссылки на группу либо рекурсивного вызова
		case 'g': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			/**
			 * Если последовательность не содержит продолжения
			 */
			if(this->_pos >= size)
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_BACKREFERENCE, offset);
			// Получаем символ, следующий за признаком последовательности
			const char next = this->_pattern.at(this->_pos);
			// Флаг разбора рекурсивного вызова группы
			const bool recursion = ((next == '<') || (next == '\''));
			/**
			 * Если ссылка на группу заключена в ограничители
			 */
			if(recursion || (next == '{')) {
				// Получаем символ завершения ссылки на группу
				const char terminator = ((next == '<') ? '>' : ((next == '{') ? '}' : '\''));
				// Переходим к первому символу ссылки на группу
				this->_pos++;
				/**
				 * Если ссылка на группу не содержит продолжения
				 */
				if(this->_pos >= size)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_BACKREFERENCE, offset);
				/**
				 * Если ссылка на группу задана именем группы
				 */
				if((this->_pos < size) && !ascii::isDigit(this->_pattern.at(this->_pos)) &&
				   (this->_pattern.at(this->_pos) != '-') && (this->_pattern.at(this->_pos) != '+')) {
					// Индекс имени группы в хранилище имён
					uint32_t target = NO_NAME;
					/**
					 * Если разбор имени группы не выполнен
					 */
					if(!this->parseName(terminator, target))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return INVALID_NODE;
					/**
					 * Если разбирается рекурсивный вызов группы
					 */
					if(recursion)
						// Выводим индекс созданного узла рекурсивного вызова
						return this->makeRecurse(0, target, offset);
					// Выводим индекс созданного узла ссылки на захваченную группу
					return this->makeBackref(target, offset);
				}
				// Флаг задания номера группы относительно текущей позиции
				const bool relative = ((this->_pattern.at(this->_pos) == '+') || (this->_pattern.at(this->_pos) == '-'));
				// Флаг отсчёта номера группы в обратном направлении
				const bool backward = (this->_pattern.at(this->_pos) == '-');
				/**
				 * Если номер группы задан относительно текущей позиции
				 */
				if(relative)
					// Переходим к первому символу номера группы
					this->_pos++;
				// Номер группы, на которую выполняется ссылка
				uint32_t target = 0;
				/**
				 * Если извлечение номера группы не выполнено
				 */
				if(!this->readNumber(target))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_BACKREFERENCE, offset);
				/**
				 * Если ссылка на группу не завершена ожидаемым символом
				 */
				if((this->_pos >= size) || (this->_pattern.at(this->_pos) != terminator))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_BACKREFERENCE, offset);
				// Переходим к символу за завершением ссылки на группу
				this->_pos++;
				/**
				 * Если номер группы задан относительно текущей позиции
				 */
				if(relative) {
					/**
					 * Если номер группы отсчитывается в обратном направлении
					 */
					if(backward) {
						/**
						 * Если номер группы выходит за пределы объявленных групп
						 */
						if((target == 0) || (target > this->_captures))
							// Выводим индекс отсутствующего узла синтаксического дерева
							return this->fail(error_t::BAD_BACKREFERENCE, offset);
						// Выполняем вычисление номера группы
						target = ((this->_captures - target) + 1);
					// Выполняем вычисление номера группы
					} else target = (this->_captures + target);
				}
				/**
				 * Если разбирается рекурсивный вызов группы
				 */
				if(recursion)
					// Выводим индекс созданного узла рекурсивного вызова
					return this->makeRecurse(target, NO_NAME, offset);
				// Выполняем создание узла ссылки на захваченную группу
				const node_id_t result = this->createNode(node_t::BACKREF);
				// Выполняем установку номера группы
				this->_nodes.at(result).backref.number = target;
				// Выполняем установку отсутствия имени группы
				this->_nodes.at(result).backref.name = NO_NAME;
				// Выводим индекс сформированного узла синтаксического дерева
				return result;
			}
			// Флаг отсчёта номера группы в обратном направлении
			const bool backward = (next == '-');
			/**
			 * Если номер группы отсчитывается в обратном направлении
			 */
			if(backward)
				// Переходим к первому символу номера группы
				this->_pos++;
			// Номер группы, на которую выполняется ссылка
			uint32_t target = 0;
			/**
			 * Если извлечение номера группы не выполнено
			 */
			if(!this->readNumber(target))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_BACKREFERENCE, offset);
			/**
			 * Если номер группы отсчитывается в обратном направлении
			 */
			if(backward) {
				/**
				 * Если номер группы выходит за пределы объявленных групп
				 */
				if((target == 0) || (target > this->_captures))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_BACKREFERENCE, offset);
				// Выполняем вычисление номера группы
				target = ((this->_captures - target) + 1);
			}
			// Выполняем создание узла ссылки на захваченную группу
			const node_id_t result = this->createNode(node_t::BACKREF);
			// Выполняем установку номера группы
			this->_nodes.at(result).backref.number = target;
			// Выполняем установку отсутствия имени группы
			this->_nodes.at(result).backref.name = NO_NAME;
			// Выводим индекс сформированного узла синтаксического дерева
			return result;
		}
	}
	/**
	 * Если экранированная последовательность является ссылкой на захваченную группу
	 */
	if((letter >= '1') && (letter <= '9')) {
		// Сохраняем позицию разбора экранированной последовательности
		const size_t saved = this->_pos;
		// Номер группы, на которую выполняется ссылка
		uint32_t number = 0;
		/**
		 * Если извлечение номера группы выполнено и группа существует
		 */
		if(this->readNumber(number) && ((number < 10) || (number <= this->_total))) {
			// Выполняем создание узла ссылки на захваченную группу
			const node_id_t result = this->createNode(node_t::BACKREF);
			// Выполняем установку номера группы
			this->_nodes.at(result).backref.number = number;
			// Выполняем установку отсутствия имени группы
			this->_nodes.at(result).backref.name = NO_NAME;
			// Выводим индекс сформированного узла синтаксического дерева
			return result;
		}
		// Выполняем восстановление позиции разбора
		this->_pos = saved;
	}
	// Кодовое значение экранированного символа
	uint32_t code = 0;
	/**
	 * Если экранированная последовательность обозначает одиночный символ
	 */
	if(this->readEscapeCode(code)) {
		// Выполняем создание узла одиночного символа
		const node_id_t result = this->createNode(node_t::LITERAL);
		// Выполняем установку кодового значения символа
		this->_nodes.at(result).literal.code = code;
		// Выводим индекс сформированного узла синтаксического дерева
		return result;
	}
	/**
	 * Если экранированная последовательность содержит букву либо цифру
	 */
	if(ascii::isAlnum(letter))
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::UNKNOWN_ESCAPE, offset);
	/**
	 * Если извлечение кодового значения экранированного символа не выполнено
	 */
	if(!this->readCode(code))
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::BAD_UTF8, offset);
	// Выполняем создание узла одиночного символа
	const node_id_t result = this->createNode(node_t::LITERAL);
	// Выполняем установку кодового значения символа
	this->_nodes.at(result).literal.code = code;
	// Выводим индекс сформированного узла синтаксического дерева
	return result;
}
/**
 * @brief Метод разбора встроенных опций регулярного выражения
 *
 * @param enable  набор устанавливаемых режимов компиляции
 * @param disable набор снимаемых режимов компиляции
 * @return        результат выполнения разбора
 *
 */
bool awh::regex::Parser::parseOptions(uint32_t & enable, uint32_t & disable) noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Выполняем сброс набора устанавливаемых режимов компиляции
	enable = 0;
	// Выполняем сброс набора снимаемых режимов компиляции
	disable = 0;
	// Флаг разбора снимаемых режимов компиляции
	bool negative = false;
	/**
	 * Выполняем разбор последовательности букв опций
	 */
	while(this->_pos < size) {
		// Получаем очередную букву опции
		const char letter = this->_pattern.at(this->_pos);
		// Числовое значение режима компиляции
		uint32_t value = 0;
		/**
		 * Определяем очередную букву опции
		 */
		switch(letter) {
			// Выполняем установку режима сопоставления без учёта регистра
			case 'i': value = flagOf(flag_t::CASELESS); break;
			// Выполняем установку режима соответствия привязок границам строк
			case 'm': value = flagOf(flag_t::MULTILINE); break;
			// Выполняем установку режима соответствия точки переводу строки
			case 's': value = flagOf(flag_t::DOTALL); break;
			// Выполняем установку режима игнорирования пробельных символов
			case 'x': value = flagOf(flag_t::EXTENDED); break;
			// Выполняем установку режима инвертирования жадности кванторов
			case 'U': value = flagOf(flag_t::UNGREEDY); break;
			// Выполняем установку режима повторного объявления имён групп
			case 'J': value = flagOf(flag_t::DUPNAMES); break;
			// Выполняем установку режима отключения захвата круглыми скобками
			case 'n': value = flagOf(flag_t::NOCAPTURE); break;
			// Выполняем установку режима ограничения сопоставления письменностью
			case 'r': value = flagOf(flag_t::RESTRICT); break;
			/**
			 * Выполняем разбор режима ограничения набора символов
			 */
			case 'a': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				/**
				 * Если режим ограничения задан для отдельного класса символов
				 *
				 * @details Буква режима допускает уточняющую букву, ограничивающую
				 *          набором символов ASCII отдельный сокращённый класс символов.
				 *
				 */
				if(this->_pos < size) {
					// Получаем уточняющую букву режима ограничения
					const char next = this->_pattern.at(this->_pos);
					/**
					 * Если уточняющая буква режима ограничения указана
					 */
					if((next == 'P') || (next == 'S') || (next == 'T') || (next == 'W') || (next == 'D'))
						// Переходим к следующему символу регулярного выражения
						this->_pos++;
				}
				/**
				 * Если разбираются снимаемые режимы компиляции
				 */
				if(negative)
					// Выполняем добавление снимаемого режима компиляции
					disable |= flagOf(flag_t::ASCII);
				// Выполняем добавление устанавливаемого режима компиляции
				else enable |= flagOf(flag_t::ASCII);
				// Переходим к следующей итерации разбора
				continue;
			}
			// Выполняем разбор знака снятия режимов компиляции
			case '-': {
				/**
				 * Если знак снятия режимов компиляции уже разобран
				 */
				if(negative) {
					// Выполняем установку ошибки некорректных встроенных опций
					this->fail(error_t::BAD_OPTIONS, this->_pos);
					// Выводим результат выполнения разбора
					return false;
				}
				// Выполняем установку флага разбора снимаемых режимов
				negative = true;
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Переходим к следующей итерации разбора
				continue;
			}
			// Выводим результат выполнения разбора
			default: return true;
		}
		/**
		 * Если разбираются снимаемые режимы компиляции
		 */
		if(negative)
			// Выполняем добавление снимаемого режима компиляции
			disable |= value;
		// Если разбираются устанавливаемые режимы компиляции
		else enable |= value;
		// Переходим к следующему символу регулярного выражения
		this->_pos++;
	}
	// Выводим результат выполнения разбора
	return true;
}
/**
 * @brief Метод разбора условного выражения
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseCondition() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала условия
	const size_t offset = this->_pos;
	// Вид условия условного выражения
	condition_t type = condition_t::GROUP_SET;
	// Номер проверяемой группы
	uint32_t number = 0;
	// Индекс имени проверяемой группы
	uint32_t index = NO_NAME;
	// Индекс узла проверки окружения, задающей условие
	node_id_t assertion = INVALID_NODE;
	/**
	 * Если условие условного выражения отсутствует
	 */
	if((this->_pos + 1) >= size)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::BAD_CONDITION, offset);
	// Получаем первый символ условия условного выражения
	const char letter = this->_pattern.at(this->_pos + 1);
	/**
	 * Если условие задано проверкой окружения
	 */
	if(letter == '?') {
		// Выполняем установку вида условия, заданного проверкой окружения
		type = condition_t::ASSERTION;
		// Выполняем разбор проверки окружения, задающей условие
		assertion = this->parseGroup();
		/**
		 * Если разбор проверки окружения не выполнен
		 */
		if(assertion == INVALID_NODE)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return INVALID_NODE;
		/**
		 * Если условие задано конструкцией, не являющейся проверкой окружения
		 */
		if(this->_nodes.at(assertion).type != node_t::LOOKAROUND)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_CONDITION, offset);
	// Если условие задано ссылкой на группу
	} else {
		// Переходим к первому символу условия условного выражения
		this->_pos++;
		/**
		 * Если условие является блоком определения групп
		 */
		if(((size - this->_pos) >= 6) && (this->_pattern.compare(this->_pos, 6, "DEFINE") == 0)) {
			// Переходим к символу за именем блока определения групп
			this->_pos += 6;
			// Выполняем установку вида условия блока определения групп
			type = condition_t::DEFINE;
		/**
		 * Если условие является проверкой выполнения рекурсивного вызова
		 */
		} else if(letter == 'R') {
			// Переходим к символу за признаком рекурсивного вызова
			this->_pos++;
			// Выполняем установку вида условия проверки рекурсивного вызова
			type = condition_t::RECURSING;
			/**
			 * Если проверяется рекурсивный вызов именованной группы
			 */
			if((this->_pos < size) && (this->_pattern.at(this->_pos) == '&')) {
				// Переходим к символу за признаком имени группы
				this->_pos++;
				/**
				 * Если разбор имени проверяемой группы не выполнен
				 */
				if(!this->parseName(')', index))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return INVALID_NODE;
				// Возвращаемся к завершающей круглой скобке условия
				this->_pos--;
			/**
			 * Если проверяется рекурсивный вызов группы по номеру
			 */
			} else if((this->_pos < size) && ascii::isDigit(this->_pattern.at(this->_pos))) {
				/**
				 * Если извлечение номера проверяемой группы не выполнено
				 */
				if(!this->readNumber(number))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_CONDITION, offset);
			}
		/**
		 * Если условие задано именем группы в скобках либо апострофах
		 */
		} else if((letter == '<') || (letter == '\'')) {
			// Получаем символ завершения имени проверяемой группы
			const char terminator = ((letter == '<') ? '>' : '\'');
			// Переходим к первому символу имени проверяемой группы
			this->_pos++;
			/**
			 * Если разбор имени проверяемой группы не выполнен
			 */
			if(!this->parseName(terminator, index))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
			// Выполняем установку вида условия проверки именованной группы
			type = condition_t::NAME_SET;
		/**
		 * Если условие задано номером группы
		 */
		} else if(ascii::isDigit(letter) || (letter == '+') || (letter == '-')) {
			// Флаг задания номера группы относительно текущей позиции
			const bool relative = ((letter == '+') || (letter == '-'));
			// Флаг отсчёта номера группы в обратном направлении
			const bool backward = (letter == '-');
			/**
			 * Если номер группы задан относительно текущей позиции
			 */
			if(relative)
				// Переходим к первому символу номера группы
				this->_pos++;
			/**
			 * Если извлечение номера проверяемой группы не выполнено
			 */
			if(!this->readNumber(number))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_CONDITION, offset);
			/**
			 * Если номер группы задан относительно текущей позиции
			 */
			if(relative) {
				/**
				 * Если номер группы отсчитывается в обратном направлении
				 */
				if(backward) {
					/**
					 * Если номер группы выходит за пределы объявленных групп
					 */
					if((number == 0) || (number > this->_captures))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return this->fail(error_t::BAD_CONDITION, offset);
					// Выполняем вычисление номера проверяемой группы
					number = ((this->_captures - number) + 1);
				// Выполняем вычисление номера проверяемой группы
				} else number = (this->_captures + number);
			}
		// Если условие задано именем группы без ограничителей
		} else {
			/**
			 * Если разбор имени проверяемой группы не выполнен
			 */
			if(!this->parseName(')', index))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
			// Возвращаемся к завершающей круглой скобке условия
			this->_pos--;
			// Выполняем установку вида условия проверки именованной группы
			type = condition_t::NAME_SET;
		}
		/**
		 * Если условие не завершено закрывающей круглой скобкой
		 */
		if((this->_pos >= size) || (this->_pattern.at(this->_pos) != ')'))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_CONDITION, offset);
		// Переходим к символу за завершающей круглой скобкой условия
		this->_pos++;
	}
	// Выполняем разбор ветви выполненного условия
	const node_id_t yes = this->parseConcat();
	/**
	 * Если разбор ветви выполненного условия не выполнен
	 */
	if(yes == INVALID_NODE)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return INVALID_NODE;
	// Индекс узла ветви невыполненного условия
	node_id_t no = INVALID_NODE;
	/**
	 * Если условное выражение содержит ветвь невыполненного условия
	 */
	if((this->_pos < size) && (this->_pattern.at(this->_pos) == '|')) {
		// Переходим к символу за разделителем ветвей
		this->_pos++;
		// Выполняем разбор ветви невыполненного условия
		no = this->parseConcat();
		/**
		 * Если разбор ветви невыполненного условия не выполнен
		 */
		if(no == INVALID_NODE)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return INVALID_NODE;
		/**
		 * Если условное выражение содержит более двух ветвей
		 */
		if((this->_pos < size) && (this->_pattern.at(this->_pos) == '|'))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_CONDITION, this->_pos);
	}
	/**
	 * Если условное выражение не завершено закрывающей круглой скобкой
	 */
	if((this->_pos >= size) || (this->_pattern.at(this->_pos) != ')'))
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::UNMATCHED_PAREN, offset);
	// Переходим к символу за закрывающей круглой скобкой
	this->_pos++;
	// Выполняем создание узла условного выражения
	const node_id_t result = this->createNode(node_t::CONDITION);
	// Выполняем установку вида условия условного выражения
	this->_nodes.at(result).condition.type = type;
	// Выполняем установку номера проверяемой группы
	this->_nodes.at(result).condition.number = number;
	// Выполняем установку индекса имени проверяемой группы
	this->_nodes.at(result).condition.name = index;
	/**
	 * Если условие задано проверкой окружения
	 */
	if(assertion != INVALID_NODE)
		// Выполняем добавление проверки окружения, задающей условие
		this->appendChild(result, assertion);
	// Выполняем добавление ветви выполненного условия
	this->appendChild(result, yes);
	/**
	 * Если условное выражение содержит ветвь невыполненного условия
	 */
	if(no != INVALID_NODE)
		// Выполняем добавление ветви невыполненного условия
		this->appendChild(result, no);
	/**
	 * Если условие задано именем группы
	 */
	if(index != NO_NAME) {
		// Создаём отложенную ссылку на именованную группу
		deferred_t deferred;
		// Выполняем установку индекса узла условного выражения
		deferred.node = result;
		// Выполняем установку индекса имени группы
		deferred.name = index;
		// Выполняем установку смещения условия
		deferred.offset = static_cast <uint32_t> (offset);
		// Выполняем добавление отложенной ссылки
		this->_deferred.push_back(deferred);
	}
	// Выводим индекс сформированного узла синтаксического дерева
	return result;
}
/**
 * @brief Метод разбора группы регулярного выражения
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseGroup() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Получаем смещение начала группы
	const size_t offset = this->_pos;
	// Переходим к символу за открывающей круглой скобкой
	this->_pos++;
	// Сохраняем набор режимов компиляции, действующих до начала группы
	const uint32_t saved = this->_flags;
	// Вид разбираемой группы синтаксического дерева
	group_t type = group_t::CAPTURE;
	// Направление и знак проверки окружения
	look_t direction = look_t::AHEAD;
	// Флаг разбора проверки окружения
	bool assertion = false;
	// Номер захватывающей группы
	uint32_t number = 0;
	// Индекс имени именованной группы
	uint32_t index = NO_NAME;
	/**
	 * Если группа начинается с признака расширенного синтаксиса
	 */
	if((this->_pos < size) && (this->_pattern.at(this->_pos) == '?')) {
		// Переходим к символу за признаком расширенного синтаксиса
		this->_pos++;
		/**
		 * Если признак расширенного синтаксиса не содержит продолжения
		 */
		if(this->_pos >= size)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_GROUP_SYNTAX, offset);
		// Получаем букву расширенного синтаксиса группы
		const char letter = this->_pattern.at(this->_pos);
		/**
		 * Определяем букву расширенного синтаксиса группы
		 */
		switch(letter) {
			// Выполняем разбор комментария
			case '#': {
				/**
				 * Выполняем поиск завершения комментария
				 */
				while((this->_pos < size) && (this->_pattern.at(this->_pos) != ')'))
					// Переходим к следующему символу регулярного выражения
					this->_pos++;
				/**
				 * Если комментарий не завершён круглой скобкой
				 */
				if(this->_pos >= size)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::UNMATCHED_PAREN, offset);
				// Переходим к символу за завершающей круглой скобкой
				this->_pos++;
				// Выводим индекс созданного узла пустого выражения
				return this->createNode(node_t::EMPTY);
			}
			// Выполняем разбор группы без захвата
			case ':': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Выполняем установку вида группы без захвата
				type = group_t::NONCAPTURE;
			} break;
			// Выполняем разбор атомарной группы
			case '>': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Выполняем установку вида атомарной группы
				type = group_t::ATOMIC;
			} break;
			// Выполняем разбор положительной опережающей проверки
			case '=': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Выполняем установку флага разбора проверки окружения
				assertion = true;
				// Выполняем установку положительной опережающей проверки
				direction = look_t::AHEAD;
			} break;
			// Выполняем разбор отрицательной опережающей проверки
			case '!': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Выполняем установку флага разбора проверки окружения
				assertion = true;
				// Выполняем установку отрицательной опережающей проверки
				direction = look_t::AHEAD_NEG;
			} break;
			// Выполняем разбор ретроспективной проверки либо именованной группы
			case '<': {
				/**
				 * Если признак ретроспективной проверки не содержит продолжения
				 */
				if((this->_pos + 1) >= size)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_GROUP_SYNTAX, offset);
				// Получаем букву признака ретроспективной проверки
				const char next = this->_pattern.at(this->_pos + 1);
				/**
				 * Если разбирается положительная ретроспективная проверка
				 */
				if(next == '=') {
					// Переходим к символу за признаком ретроспективной проверки
					this->_pos += 2;
					// Выполняем установку флага разбора проверки окружения
					assertion = true;
					// Выполняем установку положительной ретроспективной проверки
					direction = look_t::BEHIND;
				/**
				 * Если разбирается отрицательная ретроспективная проверка
				 */
				} else if(next == '!') {
					// Переходим к символу за признаком ретроспективной проверки
					this->_pos += 2;
					// Выполняем установку флага разбора проверки окружения
					assertion = true;
					// Выполняем установку отрицательной ретроспективной проверки
					direction = look_t::BEHIND_NEG;
				// Если разбирается именованная группа
				} else {
					// Переходим к символу за началом имени именованной группы
					this->_pos++;
					/**
					 * Если разбор имени именованной группы не выполнен
					 */
					if(!this->parseName('>', index))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return INVALID_NODE;
					// Выполняем установку вида именованной группы
					type = group_t::NAMED;
				}
			} break;
			// Выполняем разбор именованной группы с именем в апострофах
			case '\'': {
				// Переходим к символу за началом имени именованной группы
				this->_pos++;
				/**
				 * Если разбор имени именованной группы не выполнен
				 */
				if(!this->parseName('\'', index))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return INVALID_NODE;
				// Выполняем установку вида именованной группы
				type = group_t::NAMED;
			} break;
			// Выполняем разбор группы со сбросом нумерации ветвей
			case '|': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Выполняем установку вида группы со сбросом нумерации ветвей
				type = group_t::RESET;
			} break;
			// Выполняем разбор условного выражения
			case '(': return this->parseCondition();
			// Выполняем разбор пустого набора встроенных опций
			case ')': {
				// Переходим к символу за закрывающей круглой скобкой
				this->_pos++;
				// Выводим индекс созданного узла пустого выражения
				return this->createNode(node_t::EMPTY);
			}
			// Выполняем разбор рекурсивного вызова выражения целиком
			case 'R': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				/**
				 * Если рекурсивный вызов не завершён закрывающей круглой скобкой
				 */
				if((this->_pos >= size) || (this->_pattern.at(this->_pos) != ')'))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_RECURSION, offset);
				// Переходим к символу за закрывающей круглой скобкой
				this->_pos++;
				// Выводим индекс созданного узла рекурсивного вызова
				return this->makeRecurse(0, NO_NAME, offset);
			}
			// Выполняем разбор рекурсивного вызова именованной группы
			case '&': {
				// Переходим к первому символу имени вызываемой группы
				this->_pos++;
				// Индекс имени вызываемой группы
				uint32_t target = NO_NAME;
				/**
				 * Если разбор имени вызываемой группы не выполнен
				 */
				if(!this->parseName(')', target))
					// Выводим индекс отсутствующего узла синтаксического дерева
					return INVALID_NODE;
				// Выводим индекс созданного узла рекурсивного вызова
				return this->makeRecurse(0, target, offset);
			}
			// Выполняем разбор конструкций синтаксиса языка Python
			case 'P': {
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				/**
				 * Если конструкция не содержит продолжения
				 */
				if(this->_pos >= size)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_GROUP_SYNTAX, offset);
				// Получаем букву конструкции синтаксиса языка Python
				const char next = this->_pattern.at(this->_pos);
				/**
				 * Если конструкция объявляет именованную группу
				 */
				if(next == '<') {
					// Переходим к первому символу имени именованной группы
					this->_pos++;
					/**
					 * Если разбор имени именованной группы не выполнен
					 */
					if(!this->parseName('>', index))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return INVALID_NODE;
					// Выполняем установку вида именованной группы
					type = group_t::NAMED;
					// Выходим из разбора расширенного синтаксиса группы
					break;
				}
				// Индекс имени группы конструкции
				uint32_t target = NO_NAME;
				/**
				 * Если конструкция является ссылкой на именованную группу
				 */
				if(next == '=') {
					// Переходим к первому символу имени группы
					this->_pos++;
					/**
					 * Если разбор имени группы не выполнен
					 */
					if(!this->parseName(')', target))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return INVALID_NODE;
					// Выводим индекс созданного узла ссылки на захваченную группу
					return this->makeBackref(target, offset);
				}
				/**
				 * Если конструкция является рекурсивным вызовом именованной группы
				 */
				if(next == '>') {
					// Переходим к первому символу имени группы
					this->_pos++;
					/**
					 * Если разбор имени группы не выполнен
					 */
					if(!this->parseName(')', target))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return INVALID_NODE;
					// Выводим индекс созданного узла рекурсивного вызова
					return this->makeRecurse(0, target, offset);
				}
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_GROUP_SYNTAX, offset);
			}
			/**
			 * Выполняем разбор прочих конструкций расширенного синтаксиса группы
			 */
			default: {
				/**
				 * Определяем задание номера вызываемой группы относительно текущей позиции
				 *
				 * @details Знак «-» начинает как относительный вызов группы, так и снятие
				 *          встроенных опций, поэтому вызовом группы последовательность
				 *          считается только при наличии десятичной цифры за знаком.
				 *
				 */
				const bool signedNumber = (((letter == '+') || (letter == '-')) &&
				 ((this->_pos + 1) < size) && ascii::isDigit(this->_pattern.at(this->_pos + 1)));
				/**
				 * Если конструкция является рекурсивным вызовом группы по номеру
				 */
				if(ascii::isDigit(letter) || signedNumber) {
					// Флаг задания номера группы относительно текущей позиции
					const bool relative = ((letter == '+') || (letter == '-'));
					// Флаг отсчёта номера группы в обратном направлении
					const bool backward = (letter == '-');
					/**
					 * Если номер группы задан относительно текущей позиции
					 */
					if(relative)
						// Переходим к первому символу номера группы
						this->_pos++;
					// Номер вызываемой группы
					uint32_t target = 0;
					/**
					 * Если извлечение номера вызываемой группы не выполнено
					 */
					if(!this->readNumber(target))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return this->fail(error_t::BAD_RECURSION, offset);
					/**
					 * Если рекурсивный вызов не завершён закрывающей круглой скобкой
					 */
					if((this->_pos >= size) || (this->_pattern.at(this->_pos) != ')'))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return this->fail(error_t::BAD_RECURSION, offset);
					// Переходим к символу за закрывающей круглой скобкой
					this->_pos++;
					/**
					 * Если номер группы задан относительно текущей позиции
					 */
					if(relative) {
						/**
						 * Если номер группы отсчитывается в обратном направлении
						 */
						if(backward) {
							/**
							 * Если номер группы выходит за пределы объявленных групп
							 */
							if((target == 0) || (target > this->_captures))
								// Выводим индекс отсутствующего узла синтаксического дерева
								return this->fail(error_t::BAD_RECURSION, offset);
							// Выполняем вычисление номера вызываемой группы
							target = ((this->_captures - target) + 1);
						// Выполняем вычисление номера вызываемой группы
						} else target = (this->_captures + target);
					}
					/**
					 * Если номер вызываемой группы отсутствует и задан относительно позиции
					 *
					 * @details Нулевой номер вызываемой группы, заданный без знака,
					 *          обозначает рекурсивный вызов выражения целиком.
					 *
					 */
					if((target == 0) && relative)
						// Выводим индекс отсутствующего узла синтаксического дерева
						return this->fail(error_t::BAD_RECURSION, offset);
					// Выводим индекс созданного узла рекурсивного вызова
					return this->makeRecurse(target, NO_NAME, offset);
				}
				/**
				 * Если конструкция является встроенными опциями регулярного выражения
				 */
				if((letter == 'i') || (letter == 'm') || (letter == 's') || (letter == 'x') ||
				   (letter == 'U') || (letter == 'J') || (letter == '^') || (letter == '-') ||
				   (letter == 'a') || (letter == 'n') || (letter == 'r')) {
					/**
					 * Если конструкция сбрасывает режимы компиляции к исходным
					 */
					if(letter == '^') {
						// Переходим к следующему символу регулярного выражения
						this->_pos++;
						// Выполняем снятие изменяемых режимов компиляции
						this->_flags &= ~(flagOf(flag_t::CASELESS) | flagOf(flag_t::MULTILINE) |
						                  flagOf(flag_t::DOTALL) | flagOf(flag_t::EXTENDED) |
						                  flagOf(flag_t::UNGREEDY));
					}
					// Набор устанавливаемых режимов компиляции
					uint32_t enable = 0;
					// Набор снимаемых режимов компиляции
					uint32_t disable = 0;
					/**
					 * Если разбор встроенных опций не выполнен
					 */
					if(!this->parseOptions(enable, disable))
						// Выводим индекс отсутствующего узла синтаксического дерева
						return INVALID_NODE;
					// Выполняем установку разобранных режимов компиляции
					this->_flags |= enable;
					// Выполняем снятие разобранных режимов компиляции
					this->_flags &= ~disable;
					/**
					 * Если встроенные опции не завершены
					 */
					if(this->_pos >= size)
						// Выводим индекс отсутствующего узла синтаксического дерева
						return this->fail(error_t::BAD_OPTIONS, offset);
					/**
					 * Если встроенные опции действуют до конца охватывающей группы
					 */
					if(this->_pattern.at(this->_pos) == ')') {
						// Переходим к символу за закрывающей круглой скобкой
						this->_pos++;
						// Выводим индекс созданного узла пустого выражения
						return this->createNode(node_t::EMPTY);
					}
					/**
					 * Если встроенные опции действуют в пределах группы
					 */
					if(this->_pattern.at(this->_pos) == ':') {
						// Переходим к символу за разделителем опций и тела группы
						this->_pos++;
						// Выполняем установку вида группы без захвата
						type = group_t::NONCAPTURE;
						// Выходим из разбора расширенного синтаксиса группы
						break;
					}
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::BAD_OPTIONS, this->_pos);
				}
				/**
				 * Если конструкция является вызовом внешней функции
				 */
				if(letter == 'C')
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::UNSUPPORTED, offset);
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_GROUP_SYNTAX, this->_pos);
			}
		}
	/**
	 * Если разбирается группа без захвата, заданная режимом компиляции
	 */
	} else if(hasFlag(this->_flags, flag_t::NOCAPTURE))
		// Выполняем установку вида группы без захвата
		type = group_t::NONCAPTURE;
	// Если разбирается захватывающая группа
	else
		// Выполняем установку номера захватывающей группы
		number = ++this->_captures;
	/**
	 * Если разбирается именованная захватывающая группа
	 */
	if(type == group_t::NAMED) {
		// Выполняем установку номера захватывающей группы
		number = ++this->_captures;
		/**
		 * Если имя группы уже объявлено и повторное объявление запрещено
		 */
		if((this->_groups.count(this->_names.at(index)) > 0) && !hasFlag(this->_flags, flag_t::DUPNAMES))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::DUPLICATE_NAME, offset);
		// Выполняем сохранение соответствия имени группы её номеру
		this->_groups[this->_names.at(index)].push_back(number);
	}
	/**
	 * Если превышена допустимая глубина вложенности групп
	 */
	if(++this->_depth > MAX_NESTING)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::NESTING_TOO_DEEP, offset);
	// Индекс сформированного узла синтаксического дерева
	node_id_t result = INVALID_NODE;
	/**
	 * Если разбирается проверка окружения
	 */
	if(assertion) {
		// Выполняем создание узла проверки окружения
		result = this->createNode(node_t::LOOKAROUND);
		// Выполняем установку направления и знака проверки окружения
		this->_nodes.at(result).look.type = direction;
		// Выполняем установку наименьшей длины проверяемой последовательности
		this->_nodes.at(result).look.min = 0;
		// Выполняем установку наибольшей длины проверяемой последовательности
		this->_nodes.at(result).look.max = 0;
		// Увеличиваем глубину вложенности проверок окружения
		this->_look++;
		// Выполняем разбор тела проверки окружения
		const node_id_t body = this->parseAlternate();
		// Уменьшаем глубину вложенности проверок окружения
		this->_look--;
		/**
		 * Если разбор тела проверки окружения не выполнен
		 */
		if(body == INVALID_NODE)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return INVALID_NODE;
		// Выполняем добавление тела проверки окружения
		this->appendChild(result, body);
		// Наименьшая длина проверяемой последовательности
		uint32_t least = 0;
		// Наибольшая длина проверяемой последовательности
		uint32_t most = 0;
		// Выполняем вычисление длины проверяемой последовательности
		this->measure(body, least, most);
		// Выполняем установку наименьшей длины проверяемой последовательности
		this->_nodes.at(result).look.min = least;
		// Выполняем установку наибольшей длины проверяемой последовательности
		this->_nodes.at(result).look.max = most;
		/**
		 * Если длина последовательности ретроспективной проверки не ограничена
		 */
		if(((direction == look_t::BEHIND) || (direction == look_t::BEHIND_NEG)) && (most == UNBOUNDED))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::LOOKBEHIND_INVALID, offset);
	/**
	 * Если разбирается группа со сбросом нумерации ветвей
	 */
	} else if(type == group_t::RESET) {
		// Получаем номер захватывающей группы до начала разбора ветвей
		const uint32_t begin = this->_captures;
		// Наибольший номер захватывающей группы среди разобранных ветвей
		uint32_t maximum = begin;
		// Создаём набор ветвей группы со сбросом нумерации
		vector <node_id_t> items;
		/**
		 * Выполняем разбор ветвей группы со сбросом нумерации
		 */
		while(true) {
			// Выполняем сброс номера захватывающей группы
			this->_captures = begin;
			// Выполняем разбор очередной ветви группы
			const node_id_t branch = this->parseConcat();
			/**
			 * Если разбор очередной ветви группы не выполнен
			 */
			if(branch == INVALID_NODE)
				// Выводим индекс отсутствующего узла синтаксического дерева
				return INVALID_NODE;
			// Выполняем добавление разобранной ветви группы
			items.push_back(branch);
			/**
			 * Если номер захватывающей группы превышает наибольший
			 */
			if(this->_captures > maximum)
				// Выполняем установку наибольшего номера захватывающей группы
				maximum = this->_captures;
			/**
			 * Если ветви группы разобраны полностью
			 */
			if((this->_pos >= size) || (this->_pattern.at(this->_pos) != '|'))
				// Выходим из цикла разбора ветвей группы
				break;
			// Переходим к символу за разделителем ветвей
			this->_pos++;
		}
		// Выполняем установку наибольшего номера захватывающей группы
		this->_captures = maximum;
		// Выполняем создание узла группы со сбросом нумерации ветвей
		result = this->createNode(node_t::GROUP);
		// Выполняем установку вида группы со сбросом нумерации ветвей
		this->_nodes.at(result).group.type = group_t::RESET;
		// Выполняем установку отсутствия номера захватывающей группы
		this->_nodes.at(result).group.number = 0;
		// Выполняем установку отсутствия имени группы
		this->_nodes.at(result).group.name = NO_NAME;
		// Выполняем формирование узла выбора одной из ветвей группы
		const node_id_t body = this->makeList(node_t::ALTERNATE, items);
		// Выполняем добавление тела группы
		this->appendChild(result, body);
	// Если разбирается прочая группа регулярного выражения
	} else {
		// Выполняем создание узла группы
		result = this->createNode(node_t::GROUP);
		// Выполняем установку вида группы синтаксического дерева
		this->_nodes.at(result).group.type = type;
		// Выполняем установку номера захватывающей группы
		this->_nodes.at(result).group.number = number;
		// Выполняем установку индекса имени группы
		this->_nodes.at(result).group.name = index;
		// Выполняем разбор тела группы
		const node_id_t body = this->parseAlternate();
		/**
		 * Если разбор тела группы не выполнен
		 */
		if(body == INVALID_NODE)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return INVALID_NODE;
		// Выполняем добавление тела группы
		this->appendChild(result, body);
	}
	// Выполняем уменьшение глубины вложенности групп
	this->_depth--;
	/**
	 * Если группа не завершена закрывающей круглой скобкой
	 */
	if((this->_pos >= size) || (this->_pattern.at(this->_pos) != ')'))
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::UNMATCHED_PAREN, offset);
	// Переходим к символу за закрывающей круглой скобкой
	this->_pos++;
	// Выполняем восстановление набора режимов компиляции
	this->_flags = saved;
	// Выводим индекс сформированного узла синтаксического дерева
	return result;
}
/**
 * @brief Метод разбора одиночного элемента выражения
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseAtom() noexcept {
	// Выполняем пропуск пробельных символов и комментариев
	this->skipSpaces();
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	/**
	 * Если позиция разбора достигла конца регулярного выражения
	 */
	if(this->_pos >= size)
		// Выводим индекс созданного узла пустого выражения
		return this->createNode(node_t::EMPTY);
	// Получаем смещение начала элемента выражения
	const size_t offset = this->_pos;
	/**
	 * Определяем символ начала элемента выражения
	 */
	switch(this->_pattern.at(this->_pos)) {
		// Выполняем разбор группы регулярного выражения
		case '(': return this->parseGroup();
		// Выполняем разбор класса символов
		case '[': return this->parseClass();
		// Выполняем разбор экранированной последовательности
		case '\\': return this->parseEscape();
		// Выполняем разбор любого символа
		case '.': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выводим индекс созданного узла любого символа
			return this->createNode(node_t::ANY);
		}
		// Выполняем разбор привязки к началу текста либо строки
		case '^': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем создание узла привязки к позиции в тексте
			const node_id_t result = this->createNode(node_t::ANCHOR);
			// Выполняем установку привязки к началу текста либо строки
			this->_nodes.at(result).anchor.type = anchor_t::LINE_BEGIN;
			// Выводим индекс сформированного узла синтаксического дерева
			return result;
		}
		// Выполняем разбор привязки к концу текста либо строки
		case '$': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем создание узла привязки к позиции в тексте
			const node_id_t result = this->createNode(node_t::ANCHOR);
			// Выполняем установку привязки к концу текста либо строки
			this->_nodes.at(result).anchor.type = anchor_t::LINE_END;
			// Выводим индекс сформированного узла синтаксического дерева
			return result;
		}
		// Выводим индекс отсутствующего узла синтаксического дерева
		case '*': case '+': case '?': return this->fail(error_t::QUANTIFIER_NO_ATOM, offset);
		/**
		 * Выполняем разбор открывающей фигурной скобки
		 */
		case '{': {
			/**
			 * Если последовательность образует квантор повторения
			 *
			 * @details Корректный квантор повторения, которому не предшествует
			 *          повторяемый элемент, считается ошибкой. Последовательность,
			 *          не образующая квантора, разбирается как литерал.
			 *
			 */
			if(this->isQuantifier(this->_pos))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::QUANTIFIER_NO_ATOM, offset);
		} break;
	}
	// Кодовое значение одиночного символа
	uint32_t code = 0;
	/**
	 * Если извлечение кодового значения символа не выполнено
	 */
	if(!this->readCode(code))
		// Выводим индекс отсутствующего узла синтаксического дерева
		return this->fail(error_t::BAD_UTF8, offset);
	// Выполняем создание узла одиночного символа
	const node_id_t result = this->createNode(node_t::LITERAL);
	// Выполняем установку кодового значения символа
	this->_nodes.at(result).literal.code = code;
	// Выводим индекс сформированного узла синтаксического дерева
	return result;
}
/**
 * @brief Метод разбора кванторов повторения элемента выражения
 *
 * @param node индекс узла, к которому применяется квантор повторения
 * @return     индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseQuantifier(const node_id_t node) noexcept {
	/**
	 * Если переданный узел отсутствует
	 */
	if(node == INVALID_NODE)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return INVALID_NODE;
	// Выполняем пропуск пробельных символов и комментариев
	this->skipSpaces();
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	/**
	 * Если позиция разбора достигла конца регулярного выражения
	 */
	if(this->_pos >= size)
		// Выводим индекс переданного узла синтаксического дерева
		return node;
	// Получаем смещение начала квантора повторения
	const size_t offset = this->_pos;
	/**
	 * Определяем допустимость повторения переданного узла
	 *
	 * @details Привязки к позиции в тексте и пустые выражения не сопоставляют
	 *          последовательности символов, поэтому их повторение не имеет смысла
	 *          и считается ошибкой. Проверки окружения повторение допускают.
	 *
	 */
	const bool repeatable = ((this->_nodes.at(node).type != node_t::ANCHOR) &&
	 (this->_nodes.at(node).type != node_t::EMPTY) && (this->_nodes.at(node).type != node_t::REPEAT));
	// Наименьшее число повторений элемента выражения
	uint32_t min = 0;
	// Наибольшее число повторений элемента выражения
	uint32_t max = 0;
	/**
	 * Определяем символ квантора повторения
	 */
	switch(this->_pattern.at(this->_pos)) {
		// Выполняем проверку допустимости повторения переданного узла
		case '*': case '+': case '?': case '{': {
			/**
			 * Если повторение переданного узла недопустимо
			 */
			if(!repeatable && (this->_pattern.at(this->_pos) != '{'))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::QUANTIFIER_NO_ATOM, offset);
		} break;
	}
	/**
	 * Определяем символ квантора повторения
	 */
	switch(this->_pattern.at(this->_pos)) {
		// Выполняем разбор квантора произвольного числа повторений
		case '*': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку наименьшего числа повторений
			min = 0;
			// Выполняем установку отсутствия ограничения повторений
			max = UNBOUNDED;
		} break;
		// Выполняем разбор квантора одного и более повторений
		case '+': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку наименьшего числа повторений
			min = 1;
			// Выполняем установку отсутствия ограничения повторений
			max = UNBOUNDED;
		} break;
		// Выполняем разбор квантора необязательного элемента
		case '?': {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку наименьшего числа повторений
			min = 0;
			// Выполняем установку наибольшего числа повторений
			max = 1;
		} break;
		// Выполняем разбор квантора заданного числа повторений
		case '{': {
			// Сохраняем позицию разбора квантора повторения
			const size_t saved = this->_pos;
			// Переходим к символу за открывающей фигурной скобкой
			this->_pos++;
			// Флаг успешного разбора квантора повторения
			bool success = false;
			// Выполняем сброс наименьшего числа повторений
			min = 0;
			/**
			 * Определяем наличие наименьшего числа повторений
			 *
			 * @details Опущенное наименьшее число повторений принимается равным нулю.
			 *
			 */
			const bool least = ((this->_pos < size) && (this->_pattern.at(this->_pos) == ','));
			/**
			 * Если извлечение наименьшего числа повторений выполнено
			 */
			if(least || this->readNumber(min)) {
				/**
				 * Если квантор задаёт точное число повторений
				 */
				if((this->_pos < size) && (this->_pattern.at(this->_pos) == '}')) {
					// Переходим к символу за закрывающей фигурной скобкой
					this->_pos++;
					// Выполняем установку наибольшего числа повторений
					max = min;
					// Выполняем установку флага успешного разбора
					success = true;
				/**
				 * Если квантор задаёт диапазон числа повторений
				 */
				} else if((this->_pos < size) && (this->_pattern.at(this->_pos) == ',')) {
					// Переходим к символу за разделителем границ диапазона
					this->_pos++;
					/**
					 * Если квантор не задаёт наибольшего числа повторений
					 */
					if((this->_pos < size) && (this->_pattern.at(this->_pos) == '}')) {
						// Переходим к символу за закрывающей фигурной скобкой
						this->_pos++;
						// Выполняем установку отсутствия ограничения повторений
						max = UNBOUNDED;
						// Выполняем установку флага успешного разбора
						success = true;
					/**
					 * Если извлечение наибольшего числа повторений выполнено
					 */
					} else if(this->readNumber(max) && (this->_pos < size) && (this->_pattern.at(this->_pos) == '}')) {
						// Переходим к символу за закрывающей фигурной скобкой
						this->_pos++;
						// Выполняем установку флага успешного разбора
						success = true;
					}
				}
			}
			/**
			 * Если разбор квантора повторения не выполнен
			 */
			if(!success) {
				// Выполняем восстановление позиции разбора
				this->_pos = saved;
				// Выводим индекс переданного узла синтаксического дерева
				return node;
			}
			/**
			 * Если повторение переданного узла недопустимо
			 */
			if(!repeatable)
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::QUANTIFIER_NO_ATOM, offset);
			/**
			 * Если границы кванта повторения превышают допустимое значение
			 */
			if((min > MAX_REPEAT) || ((max != UNBOUNDED) && (max > MAX_REPEAT)))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::QUANTIFIER_TOO_BIG, offset);
			/**
			 * Если границы кванта повторения указаны в обратном порядке
			 */
			if((max != UNBOUNDED) && (max < min))
				// Выводим индекс отсутствующего узла синтаксического дерева
				return this->fail(error_t::BAD_QUANTIFIER, offset);
		} break;
		// Выводим индекс переданного узла синтаксического дерева
		default: return node;
	}
	// Режим жадности квантора повторения
	greed_t greed = greed_t::GREEDY;
	/**
	 * Если квантор повторения содержит признак режима жадности
	 */
	if(this->_pos < size) {
		/**
		 * Если квантор повторения является ленивым
		 */
		if(this->_pattern.at(this->_pos) == '?') {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку ленивого режима жадности
			greed = greed_t::LAZY;
		/**
		 * Если квантор повторения является захватывающим
		 */
		} else if(this->_pattern.at(this->_pos) == '+') {
			// Переходим к следующему символу регулярного выражения
			this->_pos++;
			// Выполняем установку захватывающего режима жадности
			greed = greed_t::POSSESSIVE;
		}
	}
	/**
	 * Если установлен режим инвертирования жадности кванторов
	 */
	if(hasFlag(this->_flags, flag_t::UNGREEDY) && (greed != greed_t::POSSESSIVE))
		// Выполняем инвертирование режима жадности квантора повторения
		greed = ((greed == greed_t::GREEDY) ? greed_t::LAZY : greed_t::GREEDY);
	// Выполняем создание узла повторения
	const node_id_t result = this->createNode(node_t::REPEAT);
	// Выполняем установку наименьшего числа повторений
	this->_nodes.at(result).repeat.min = min;
	// Выполняем установку наибольшего числа повторений
	this->_nodes.at(result).repeat.max = max;
	// Выполняем установку режима жадности квантора повторения
	this->_nodes.at(result).repeat.greed = greed;
	// Выполняем установку повторяемого элемента выражения
	this->_nodes.at(result).child = node;
	/**
	 * Если за квантором повторения следует ещё один квантор
	 */
	if(this->_pos < size) {
		// Получаем символ, следующий за квантором повторения
		const char letter = this->_pattern.at(this->_pos);
		/**
		 * Если следующий символ является квантором повторения
		 */
		if((letter == '*') || (letter == '+') || (letter == '?'))
			// Выводим индекс отсутствующего узла синтаксического дерева
			return this->fail(error_t::BAD_QUANTIFIER, this->_pos);
	}
	// Выводим индекс сформированного узла синтаксического дерева
	return result;
}
/**
 * @brief Метод разбора элемента выражения с квантором повторения
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseRepeat() noexcept {
	// Выполняем разбор одиночного элемента выражения
	const node_id_t node = this->parseAtom();
	/**
	 * Если разбор одиночного элемента выражения не выполнен
	 */
	if(node == INVALID_NODE)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return INVALID_NODE;
	// Выводим результат разбора кванторов повторения элемента выражения
	return this->parseQuantifier(node);
}
/**
 * @brief Метод разбора последовательности элементов выражения
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseConcat() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Создаём набор элементов последовательности
	vector <node_id_t> items;
	/**
	 * Флаг размещения барьера перед текущей позицией разбора
	 *
	 * @details Встроенные опции не сопоставляют символов, однако разрывают связь
	 *          квантора повторения с предшествующим элементом выражения, поэтому
	 *          квантор, следующий за встроенными опциями, считается ошибкой.
	 *
	 */
	bool barrier = false;
	/**
	 * Выполняем разбор элементов последовательности
	 */
	while(true) {
		// Выполняем пропуск пробельных символов и комментариев
		this->skipSpaces();
		/**
		 * Если позиция разбора достигла конца регулярного выражения
		 */
		if(this->_pos >= size)
			// Выходим из цикла разбора элементов последовательности
			break;
		// Получаем очередной символ регулярного выражения
		const char letter = this->_pattern.at(this->_pos);
		/**
		 * Если последовательность элементов завершена
		 */
		if((letter == '|') || (letter == ')'))
			// Выходим из цикла разбора элементов последовательности
			break;
		/**
		 * Если элемент завершает экранирование последовательности символов
		 *
		 * @details Последовательность «\\E» не сопоставляет символов и не образует
		 *          элемента выражения, поэтому следующий за ней квантор повторения
		 *          применяется к элементу, предшествующему завершению экранирования.
		 *
		 */
		if(((letter == '\\') && ((this->_pos + 1) < size) && (this->_pattern.at(this->_pos + 1) == 'E')) ||
		   ((letter == '(') && ((this->_pos + 2) < size) && (this->_pattern.at(this->_pos + 1) == '?') &&
		    (this->_pattern.at(this->_pos + 2) == '#'))) {
			/**
			 * Если элемент является комментарием
			 */
			if(letter == '(') {
				/**
				 * Выполняем поиск завершения комментария
				 */
				while((this->_pos < size) && (this->_pattern.at(this->_pos) != ')'))
					// Переходим к следующему символу регулярного выражения
					this->_pos++;
				/**
				 * Если комментарий не завершён круглой скобкой
				 */
				if(this->_pos >= size)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return this->fail(error_t::UNMATCHED_PAREN, this->_pos);
				// Переходим к символу за завершающей круглой скобкой
				this->_pos++;
			// Переходим к символу за завершением экранирования
			} else this->_pos += 2;
			/**
			 * Если за удалённым элементом следует признак режима жадности
			 *
			 * @details Удаляемый элемент выражения не разделяет квантор повторения
			 *          и следующий за ним признак режима жадности, поэтому признак
			 *          применяется к предшествующему квантору повторения.
			 *
			 */
			if(!items.empty() && !barrier && (this->_pos < size) &&
			   (this->_nodes.at(items.back()).type == node_t::REPEAT) &&
			   (this->_nodes.at(items.back()).repeat.greed == greed_t::GREEDY) &&
			   ((this->_pattern.at(this->_pos) == '?') || (this->_pattern.at(this->_pos) == '+'))) {
				// Выполняем установку режима жадности квантора повторения
				this->_nodes.at(items.back()).repeat.greed =
				 ((this->_pattern.at(this->_pos) == '?') ? greed_t::LAZY : greed_t::POSSESSIVE);
				// Переходим к следующему символу регулярного выражения
				this->_pos++;
				// Переходим к следующему элементу последовательности
				continue;
			}
			/**
			 * Если квантор повторения допускает связь с предшествующим элементом
			 */
			if(!items.empty() && !barrier) {
				// Выполняем разбор кванторов повторения предшествующего элемента
				items.back() = this->parseQuantifier(items.back());
				/**
				 * Если разбор кванторов повторения не выполнен
				 */
				if(items.back() == INVALID_NODE)
					// Выводим индекс отсутствующего узла синтаксического дерева
					return INVALID_NODE;
			}
			// Переходим к следующему элементу последовательности
			continue;
		}
		// Выполняем разбор очередного элемента последовательности
		const node_id_t node = this->parseRepeat();
		/**
		 * Если разбор очередного элемента последовательности не выполнен
		 */
		if(node == INVALID_NODE)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return INVALID_NODE;
		/**
		 * Если разобранный элемент является пустым выражением
		 */
		if(this->_nodes.at(node).type == node_t::EMPTY) {
			// Выполняем установку флага размещения барьера
			barrier = true;
			// Переходим к следующему элементу последовательности
			continue;
		}
		// Выполняем сброс флага размещения барьера
		barrier = false;
		// Выполняем добавление разобранного элемента последовательности
		items.push_back(node);
	}
	// Выводим индекс сформированного узла последовательности элементов
	return this->makeList(node_t::CONCAT, items);
}
/**
 * @brief Метод разбора выражения выбора одной из ветвей
 *
 * @return индекс сформированного узла синтаксического дерева
 *
 */
awh::regex::node_id_t awh::regex::Parser::parseAlternate() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Выполняем разбор первой ветви выражения
	const node_id_t first = this->parseConcat();
	/**
	 * Если разбор первой ветви выражения не выполнен
	 */
	if(first == INVALID_NODE)
		// Выводим индекс отсутствующего узла синтаксического дерева
		return INVALID_NODE;
	/**
	 * Если выражение не содержит выбора одной из ветвей
	 */
	if((this->_pos >= size) || (this->_pattern.at(this->_pos) != '|'))
		// Выводим индекс сформированного узла первой ветви
		return first;
	// Создаём набор ветвей выражения
	vector <node_id_t> items;
	// Выполняем добавление первой ветви выражения
	items.push_back(first);
	/**
	 * Выполняем разбор прочих ветвей выражения
	 */
	while((this->_pos < size) && (this->_pattern.at(this->_pos) == '|')) {
		// Переходим к символу за разделителем ветвей
		this->_pos++;
		// Выполняем разбор очередной ветви выражения
		const node_id_t branch = this->parseConcat();
		/**
		 * Если разбор очередной ветви выражения не выполнен
		 */
		if(branch == INVALID_NODE)
			// Выводим индекс отсутствующего узла синтаксического дерева
			return INVALID_NODE;
		// Выполняем добавление разобранной ветви выражения
		items.push_back(branch);
	}
	// Выводим индекс сформированного узла выбора одной из ветвей
	return this->makeList(node_t::ALTERNATE, items);
}
/**
 * @brief Метод предварительного прохода по регулярному выражению
 *
 * @return результат выполнения предварительного прохода
 *
 */
bool awh::regex::Parser::prescan() noexcept {
	// Получаем размер текста регулярного выражения
	const size_t size = this->_pattern.size();
	// Флаг нахождения позиции разбора внутри класса символов
	bool inClass = false;
	/**
	 * Выполняем проход по тексту регулярного выражения
	 *
	 * @details Проход не учитывает сброса нумерации ветвей конструкцией «(?|...)»,
	 *          поэтому для выражений с такой конструкцией количество групп может быть
	 *          завышено. Завышение количества влияет лишь на различение ссылок на
	 *          захваченные группы и восьмеричных последовательностей, разрешая ссылку
	 *          там, где её корректность проверяется на этапе разрешения ссылок.
	 *
	 */
	for(size_t i = 0; i < size; i++) {
		// Получаем очередной символ регулярного выражения
		const char letter = this->_pattern.at(i);
		/**
		 * Если очередной символ является обратной косой чертой
		 */
		if(letter == '\\') {
			// Выполняем пропуск экранированного символа
			i++;
			// Переходим к следующему символу регулярного выражения
			continue;
		}
		/**
		 * Если позиция прохода находится внутри класса символов
		 */
		if(inClass) {
			/**
			 * Если класс символов завершён закрывающей квадратной скобкой
			 */
			if(letter == ']')
				// Выполняем сброс флага нахождения внутри класса символов
				inClass = false;
			// Переходим к следующему символу регулярного выражения
			continue;
		}
		/**
		 * Если очередной символ начинает класс символов
		 */
		if(letter == '[') {
			// Выполняем установку флага нахождения внутри класса символов
			inClass = true;
			/**
			 * Если класс символов начинается со знака отрицания
			 */
			if(((i + 1) < size) && (this->_pattern.at(i + 1) == '^'))
				// Выполняем пропуск знака отрицания класса символов
				i++;
			/**
			 * Если класс символов начинается с закрывающей квадратной скобки
			 */
			if(((i + 1) < size) && (this->_pattern.at(i + 1) == ']'))
				// Выполняем пропуск закрывающей квадратной скобки
				i++;
			// Переходим к следующему символу регулярного выражения
			continue;
		}
		/**
		 * Если очередной символ не начинает группу
		 */
		if(letter != '(')
			// Переходим к следующему символу регулярного выражения
			continue;
		/**
		 * Если группа не содержит признака расширенного синтаксиса
		 */
		if(((i + 1) >= size) || (this->_pattern.at(i + 1) != '?')) {
			// Увеличиваем общее количество захватывающих групп
			this->_total++;
			// Переходим к следующему символу регулярного выражения
			continue;
		}
		/**
		 * Если признак расширенного синтаксиса не содержит продолжения
		 */
		if((i + 2) >= size)
			// Переходим к следующему символу регулярного выражения
			continue;
		// Получаем букву расширенного синтаксиса группы
		const char next = this->_pattern.at(i + 2);
		/**
		 * Если группа является именованной с именем в апострофах
		 */
		if(next == '\'')
			// Увеличиваем общее количество захватывающих групп
			this->_total++;
		/**
		 * Если группа является именованной с именем в угловых скобках
		 */
		else if((next == '<') && ((i + 3) < size) && (this->_pattern.at(i + 3) != '=') && (this->_pattern.at(i + 3) != '!'))
			// Увеличиваем общее количество захватывающих групп
			this->_total++;
		/**
		 * Если группа является именованной в синтаксисе языка Python
		 */
		else if((next == 'P') && ((i + 3) < size) && (this->_pattern.at(i + 3) == '<'))
			// Увеличиваем общее количество захватывающих групп
			this->_total++;
	}
	// Выводим результат выполнения предварительного прохода
	return true;
}
/**
 * @brief Метод проверки возможности пустого сопоставления узла
 *
 * @param id индекс узла в арене узлов
 * @return   результат проверки возможности пустого сопоставления
 *
 */
bool awh::regex::Parser::nullable(const node_id_t id) const noexcept {
	// Наименьшая длина сопоставляемой узлом последовательности
	uint32_t least = 0;
	// Наибольшая длина сопоставляемой узлом последовательности
	uint32_t most = 0;
	// Выполняем вычисление длины сопоставляемой узлом последовательности
	this->measureNode(id, least, most);
	// Выводим результат проверки возможности пустого сопоставления
	return (least == 0);
}
/**
 * @brief Метод вычисления длины сопоставляемой узлом последовательности
 *
 * @param id  индекс узла в арене узлов
 * @param min наименьшая длина сопоставляемой последовательности
 * @param max наибольшая длина сопоставляемой последовательности
 *
 */
void awh::regex::Parser::measureNode(const node_id_t id, uint32_t & min, uint32_t & max) const noexcept {
	// Выполняем сброс наименьшей длины последовательности
	min = 0;
	// Выполняем сброс наибольшей длины последовательности
	max = 0;
	/**
	 * Если индекс узла находится за пределами арены узлов
	 */
	if(id >= static_cast <node_id_t> (this->_nodes.size()))
		// Выходим из функции
		return;
	// Получаем узел синтаксического дерева
	const node_data_t & node = this->_nodes.at(id);
	/**
	 * Определяем тип узла синтаксического дерева
	 */
	switch(static_cast <uint8_t> (node.type)) {
		// Выполняем вычисление длины узла пустого выражения
		case static_cast <uint8_t> (node_t::EMPTY):
		// Выполняем вычисление длины узла привязки к позиции в тексте
		case static_cast <uint8_t> (node_t::ANCHOR):
		// Выполняем вычисление длины узла проверки окружения
		case static_cast <uint8_t> (node_t::LOOKAROUND): return;
		// Выполняем вычисление длины узла одиночного символа
		case static_cast <uint8_t> (node_t::LITERAL):
		// Выполняем вычисление длины узла класса символов
		case static_cast <uint8_t> (node_t::CLASS):
		// Выполняем вычисление длины узла одиночной единицы кодирования
		case static_cast <uint8_t> (node_t::CODEUNIT):
		// Выполняем вычисление длины узла любого символа
		case static_cast <uint8_t> (node_t::ANY): {
			// Выполняем установку наименьшей длины последовательности
			min = 1;
			// Выполняем установку наибольшей длины последовательности
			max = 1;
		} return;
		// Выполняем вычисление длины узла последовательности символов
		case static_cast <uint8_t> (node_t::STRING): {
			// Выполняем установку наименьшей длины последовательности
			min = node.string.length;
			// Выполняем установку наибольшей длины последовательности
			max = node.string.length;
		} return;
		// Выполняем вычисление длины узла последовательности элементов
		case static_cast <uint8_t> (node_t::CONCAT):
		// Выполняем вычисление длины узла группы
		case static_cast <uint8_t> (node_t::GROUP): {
			// Выполняем вычисление длины последовательности дочерних узлов
			this->measure(node.child, min, max);
		} return;
		// Выполняем вычисление длины узла выбора одной из ветвей
		case static_cast <uint8_t> (node_t::ALTERNATE):
		// Выполняем вычисление длины узла условного выражения
		case static_cast <uint8_t> (node_t::CONDITION): {
			// Флаг вычисления длины первой ветви выражения
			bool first = true;
			/**
			 * Выполняем обход ветвей выражения
			 */
			for(node_id_t branch = node.child; branch != INVALID_NODE; branch = this->_nodes.at(branch).next) {
				// Наименьшая длина последовательности ветви выражения
				uint32_t lower = 0;
				// Наибольшая длина последовательности ветви выражения
				uint32_t upper = 0;
				// Выполняем вычисление длины последовательности ветви выражения
				this->measureNode(branch, lower, upper);
				/**
				 * Если вычисляется длина первой ветви выражения
				 */
				if(first) {
					// Выполняем установку наименьшей длины последовательности
					min = lower;
					// Выполняем установку наибольшей длины последовательности
					max = upper;
					// Выполняем сброс флага вычисления первой ветви
					first = false;
					// Переходим к следующей ветви выражения
					continue;
				}
				/**
				 * Если наименьшая длина ветви меньше вычисленной
				 */
				if(lower < min)
					// Выполняем установку наименьшей длины последовательности
					min = lower;
				/**
				 * Если наибольшая длина ветви не ограничена либо превышает вычисленную
				 */
				if((upper == UNBOUNDED) || ((max != UNBOUNDED) && (upper > max)))
					// Выполняем установку наибольшей длины последовательности
					max = upper;
			}
		} return;
		// Выполняем вычисление длины узла повторения
		case static_cast <uint8_t> (node_t::REPEAT): {
			// Наименьшая длина последовательности повторяемого элемента
			uint32_t lower = 0;
			// Наибольшая длина последовательности повторяемого элемента
			uint32_t upper = 0;
			// Выполняем вычисление длины последовательности повторяемого элемента
			this->measureNode(node.child, lower, upper);
			/**
			 * Если наименьшая длина повторяемого элемента ограничена
			 */
			if(lower != UNBOUNDED)
				// Выполняем вычисление наименьшей длины последовательности
				min = (lower * node.repeat.min);
			// Выполняем установку отсутствия ограничения наименьшей длины
			else min = UNBOUNDED;
			/**
			 * Если повторяемый элемент не сопоставляет последовательности символов
			 */
			if(upper == 0)
				// Выполняем установку наибольшей длины последовательности
				max = 0;
			/**
			 * Если число повторений либо длина повторяемого элемента не ограничены
			 */
			else if((node.repeat.max == UNBOUNDED) || (upper == UNBOUNDED))
				// Выполняем установку отсутствия ограничения длины
				max = UNBOUNDED;
			// Выполняем вычисление наибольшей длины последовательности
			else max = (upper * node.repeat.max);
		} return;
	}
	/**
	 * Выполняем вычисление длины прочих узлов синтаксического дерева
	 *
	 * @details Ссылки на захваченные группы, рекурсивные вызовы и графемные кластеры
	 *          сопоставляют последовательность, длина которой на этапе разбора
	 *          неизвестна, поэтому она считается неограниченной.
	 *
	 */
	max = UNBOUNDED;
}
/**
 * @brief Метод вычисления длины сопоставляемой узлами последовательности
 *
 * @param id  индекс первого узла цепочки в арене узлов
 * @param min наименьшая длина сопоставляемой последовательности
 * @param max наибольшая длина сопоставляемой последовательности
 *
 */
void awh::regex::Parser::measure(const node_id_t id, uint32_t & min, uint32_t & max) const noexcept {
	// Выполняем сброс наименьшей длины последовательности
	min = 0;
	// Выполняем сброс наибольшей длины последовательности
	max = 0;
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = this->_nodes.at(index).next) {
		// Наименьшая длина последовательности, сопоставляемой узлом
		uint32_t least = 0;
		// Наибольшая длина последовательности, сопоставляемой узлом
		uint32_t most = 0;
		// Выполняем вычисление длины последовательности, сопоставляемой узлом
		this->measureNode(index, least, most);
		/**
		 * Если наименьшая длина последовательности узла либо накопленная не ограничены
		 */
		if((least == UNBOUNDED) || (min == UNBOUNDED))
			// Выполняем установку отсутствия ограничения наименьшей длины
			min = UNBOUNDED;
		// Выполняем добавление наименьшей длины последовательности узла
		else min += least;
		/**
		 * Если наибольшая длина последовательности узла либо накопленная не ограничены
		 */
		if((most == UNBOUNDED) || (max == UNBOUNDED))
			// Выполняем установку отсутствия ограничения наибольшей длины
			max = UNBOUNDED;
		// Выполняем добавление наибольшей длины последовательности узла
		else max += most;
	}
}
/**
 * @brief Метод разрешения отложенных ссылок на именованные группы
 *
 * @return результат разрешения отложенных ссылок
 *
 */
bool awh::regex::Parser::resolve() noexcept {
	/**
	 * Выполняем разрешение отложенных ссылок на именованные группы
	 */
	for(auto & deferred : this->_deferred) {
		// Выполняем поиск имени группы в соответствии имён группы их номерам
		auto i = this->_groups.find(this->name(deferred.name));
		/**
		 * Если группа с указанным именем не объявлена
		 */
		if(i == this->_groups.end()) {
			// Выполняем установку ошибки ссылки на несуществующую группу
			this->fail(error_t::BAD_BACKREFERENCE, deferred.offset);
			// Выводим результат разрешения отложенных ссылок
			return false;
		}
		/**
		 * Если разрешаемый узел является ссылкой на захваченную группу
		 */
		if(this->_nodes.at(deferred.node).type == node_t::BACKREF)
			// Выполняем установку номера группы ссылки
			this->_nodes.at(deferred.node).backref.number = i->second.front();
		/**
		 * Если разрешаемый узел является рекурсивным вызовом
		 */
		else if(this->_nodes.at(deferred.node).type == node_t::RECURSE)
			// Выполняем установку номера вызываемой группы
			this->_nodes.at(deferred.node).recurse.number = i->second.front();
		/**
		 * Если разрешаемый узел является условным выражением
		 */
		else if(this->_nodes.at(deferred.node).type == node_t::CONDITION)
			// Выполняем установку номера проверяемой группы
			this->_nodes.at(deferred.node).condition.number = i->second.front();
	}
	/**
	 * Выполняем проверку ссылок на захваченные группы по номеру
	 */
	for(auto & node : this->_nodes) {
		/**
		 * Если узел не является ссылкой на захваченную группу
		 */
		if(node.type != node_t::BACKREF)
			// Переходим к следующему узлу синтаксического дерева
			continue;
		/**
		 * Если ссылка указывает на несуществующую группу
		 */
		if((node.backref.number == 0) || (node.backref.number > this->_captures)) {
			// Выполняем установку ошибки ссылки на несуществующую группу
			this->fail(error_t::BAD_BACKREFERENCE, node.offset);
			// Выводим результат разрешения отложенных ссылок
			return false;
		}
	}
	/**
	 * Выполняем проверку рекурсивных вызовов групп по номеру
	 */
	for(auto & node : this->_nodes) {
		/**
		 * Если узел не является рекурсивным вызовом
		 */
		if(node.type != node_t::RECURSE)
			// Переходим к следующему узлу синтаксического дерева
			continue;
		/**
		 * Если вызов выполняется для выражения целиком
		 */
		if((node.recurse.number == 0) && (node.recurse.name == NO_NAME))
			// Переходим к следующему узлу синтаксического дерева
			continue;
		/**
		 * Если вызов выполняется для несуществующей группы
		 */
		if(node.recurse.number > this->_captures) {
			// Выполняем установку ошибки ссылки на несуществующую группу
			this->fail(error_t::BAD_BACKREFERENCE, node.offset);
			// Выводим результат разрешения отложенных ссылок
			return false;
		}
	}
	// Выводим результат разрешения отложенных ссылок
	return true;
}
/**
 * @brief Метод разбора регулярного выражения
 *
 * @param pattern текст регулярного выражения для разбора
 * @param flags   набор режимов компиляции регулярного выражения
 * @return        результат выполнения разбора
 *
 */
bool awh::regex::Parser::parse(string_view pattern, const uint32_t flags) noexcept {
	// Выполняем сброс результатов предыдущего разбора
	this->reset();
	// Выполняем установку текста разбираемого регулярного выражения
	this->_pattern = pattern;
	// Выполняем установку набора режимов компиляции
	this->_flags = flags;
	// Выполняем сохранение исходного набора режимов компиляции
	this->_options = flags;
	/**
	 * Если размер регулярного выражения превышает допустимый
	 */
	if(pattern.size() > 0x100000) {
		// Выполняем установку ошибки превышения размера регулярного выражения
		this->fail(error_t::PATTERN_TOO_LARGE, 0);
		// Выводим результат выполнения разбора
		return false;
	}
	/**
	 * Если предварительный проход по регулярному выражению не выполнен
	 */
	if(!this->prescan())
		// Выводим результат выполнения разбора
		return false;
	// Выполняем сброс позиции разбора
	this->_pos = 0;
	// Выполняем разбор регулярного выражения
	const node_id_t root = this->parseAlternate();
	/**
	 * Если разбор регулярного выражения не выполнен
	 */
	if(root == INVALID_NODE)
		// Выводим результат выполнения разбора
		return false;
	/**
	 * Если регулярное выражение разобрано не полностью
	 */
	if(this->_pos < this->_pattern.size()) {
		// Выполняем установку ошибки непарной круглой скобки
		this->fail(error_t::UNMATCHED_PAREN, this->_pos);
		// Выводим результат выполнения разбора
		return false;
	}
	/**
	 * Если разрешение отложенных ссылок не выполнено
	 */
	if(!this->resolve())
		// Выводим результат выполнения разбора
		return false;
	// Выполняем установку индекса корневого узла синтаксического дерева
	this->_root = root;
	// Выводим результат выполнения разбора
	return true;
}
