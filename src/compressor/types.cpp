/**
 * @file types.cpp
 * @date 2026-07-13
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
 * @brief Реализация общих типов подсистемы компрессии —
 *        конструирование и инициализация структуры параметров потоковой сессии значениями по умолчанию
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <compressor/types.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция проверки размера буфера данных на пригодность движку компрессии
 *
 * @param size   размер буфера данных
 * @param method метод компрессии
 * @return       результат проверки
 *
 */
bool awh::compressor::fits(const size_t size, const method_t method) noexcept {
	/**
	 * Определяем метод компрессии
	 */
	switch(static_cast <uint8_t> (method)){
		// Движки семейства zlib принимают размер разрядностью uInt
		case static_cast <uint8_t> (method_t::GZIP):
		case static_cast <uint8_t> (method_t::ZLIB):
		case static_cast <uint8_t> (method_t::DEFLATE):
		// BZip2 принимает размер разрядностью uint32_t
		case static_cast <uint8_t> (method_t::BZIP2):
			// Выводим результат проверки
			return (static_cast <uint64_t> (size) <= static_cast <uint64_t> (UINT32_MAX));
		// LZ4 и Lizard принимают размер знаковой разрядностью
		case static_cast <uint8_t> (method_t::LZ4):
		case static_cast <uint8_t> (method_t::LIZARD):
			// Выводим результат проверки
			return (static_cast <uint64_t> (size) <= static_cast <uint64_t> (INT32_MAX));
	}
	// Остальные движки работают размером разрядностью платформы
	return true;
}
/**
 * @brief Конструктор
 *
 */
awh::compressor::Params::Params() noexcept : wbits(15), level(-1) {}
