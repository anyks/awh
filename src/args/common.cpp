/**
 * @file common.cpp
 * @date 2026-09-02
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
 * \~russian
 * @brief Исходный файл общих определений модуля параметров запуска
 *
 * \~english
 * @brief Source file of the common definitions of the module of the parameters of the launch
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <args/common.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён параметров запуска приложения
 */
using namespace awh::args;

/**
 * @brief Метод извлечения описания кода ошибки разбора
 *
 * @param error код ошибки разбора
 * @return      описание кода ошибки разбора
 *
 */
const char * awh::args::message(const error_t error) noexcept {
	// Определяем код ошибки разбора
	switch(static_cast <uint8_t> (error)){
		// Если отказа не произошло
		case static_cast <uint8_t> (error_t::NONE):
			// Выводим описание кода ошибки разбора
			return "no error";
		// Если имя параметра пусто вовсе
		case static_cast <uint8_t> (error_t::EMPTY_KEY):
			// Выводим описание кода ошибки разбора
			return "empty parameter name";
		// Если путь укладки пуст либо содержит пустое звено
		case static_cast <uint8_t> (error_t::EMPTY_PATH):
			// Выводим описание кода ошибки разбора
			return "empty path segment";
		// Если глубина пути укладки превысила предел разбора
		case static_cast <uint8_t> (error_t::DEEP_PATH):
			// Выводим описание кода ошибки разбора
			return "path depth limit exceeded";
		// Если длина имени параметра превысила предел разбора
		case static_cast <uint8_t> (error_t::LONG_KEY):
			// Выводим описание кода ошибки разбора
			return "parameter name length limit exceeded";
		// Если длина значения параметра превысила предел разбора
		case static_cast <uint8_t> (error_t::LONG_VALUE):
			// Выводим описание кода ошибки разбора
			return "parameter value length limit exceeded";
		// Если число лексем превысило предел разбора
		case static_cast <uint8_t> (error_t::MANY_TOKENS):
			// Выводим описание кода ошибки разбора
			return "token count limit exceeded";
		// Если кавычка в поданном тексте не закрыта
		case static_cast <uint8_t> (error_t::UNPAIRED):
			// Выводим описание кода ошибки разбора
			return "unterminated quote";
		// Если обратная косая стоит последним знаком текста
		case static_cast <uint8_t> (error_t::DANGLING):
			// Выводим описание кода ошибки разбора
			return "dangling escape character";
		// Если параметру потребно значение, а его нет вовсе
		case static_cast <uint8_t> (error_t::NO_VALUE):
			// Выводим описание кода ошибки разбора
			return "missing parameter value";
		// Если параметру значение не потребно, а оно подано
		case static_cast <uint8_t> (error_t::ODD_VALUE):
			// Выводим описание кода ошибки разбора
			return "unexpected parameter value";
		// Если имя параметра описанию ожидаемых неизвестно
		case static_cast <uint8_t> (error_t::UNKNOWN):
			// Выводим описание кода ошибки разбора
			return "unknown parameter name";
		// Если параметр подан повторно, а кратность его одиночна
		case static_cast <uint8_t> (error_t::DUPLICATE):
			// Выводим описание кода ошибки разбора
			return "duplicate parameter";
		// Если разбор настроек кодеком окончился отказом
		case static_cast <uint8_t> (error_t::CODEC):
			// Выводим описание кода ошибки разбора
			return "codec parsing failure";
		// Если кодек выдачи вместить поданное дерево не может
		case static_cast <uint8_t> (error_t::UNSUPPORTED):
			// Выводим описание кода ошибки разбора
			return "value unsupported by the codec";
		// Если файла настроек нет вовсе либо чтение его отказало
		case static_cast <uint8_t> (error_t::FILESYSTEM):
			// Выводим описание кода ошибки разбора
			return "settings file is unreadable";
	}
	// Выводим общее описание кода ошибки, отведённого не имеющего
	return "unknown error";
}
