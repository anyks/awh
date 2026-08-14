/**
 * @file conformance.hpp
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
 * @brief Записанный образец сличения переносимого стенда — описание блоков порождаемых
 *        образцов вместе с суммами по итогам их сопоставления
 *
 * @details Суммы получены на платформе, где поведение модуля сличено с эталонной
 *          реализацией PCRE2 тестами «Regex.Reference*». Пересчитывать их следует
 *          лишь при намеренном изменении поведения модуля либо набора порождаемых
 *          образцов, и всякий раз - на платформе, где эталонная реализация доступна.
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_TOOLS_REGEX_CONFORMANCE__
#define __AWH_TOOLS_REGEX_CONFORMANCE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * @brief Описание блока порождаемых образцов сличения
 *
 */
typedef struct Block {
	// Название блока образцов сличения
	const char * name;
	// Начальное значение источника псевдослучайных значений
	uint32_t seed;
	// Количество порождаемых образцов блока
	size_t samples;
	// Наибольшая длина текста сопоставления в символах
	size_t length;
	// Флаг порождения образцов в режиме разбора UTF-8
	bool utf;
	// Сумма по итогам сопоставления образцов блока
	uint64_t checksum;
} block_t;

/**
 * @brief Набор блоков образцов сличения
 *
 */
static const block_t BLOCKS[] = {
	{"короткие тексты",      1,  20000,   24, false, 0x3f9055af4d5700e5ull},
	{"длинные тексты",       7,   5000,  400, false, 0x75b46b23f5459608ull},
	{"сетевые сообщения",   99,   2000, 4000, false, 0xe1d193fc39913518ull},
	{"UTF-8 короткие",      11,  20000,   24, true,  0x58b4dcf0f12d097cull},
	{"UTF-8 длинные",       77,   5000,  300, true,  0xa4c3d67d050a46beull},
	{"UTF-8 сообщения",    404,   1000, 3000, true,  0x99bcc089764d6adaull}
};

#endif // __AWH_TOOLS_REGEX_CONFORMANCE__
