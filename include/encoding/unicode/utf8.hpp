/**
 * @file: utf8.hpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл кодировщика UTF-8 модуля Юникода — представление кодового значения
 *        символа последовательностью байтов, разбор последовательности байтов
 *        и проверка правильности записи текста
 *
 * \~english
 * @brief Header file of the UTF-8 encoder of the Unicode module — representation of a character
 *        code value by a sequence of bytes, parsing of a sequence of bytes
 *        and verification of the correctness of a text record
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNICODE_UTF8__
#define __AWH_UNICODE_UTF8__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён кодировщика UTF-8
	 *
	 * \~english
	 * @brief UTF-8 encoder namespace
	 *
	 * \~
	 */
	namespace utf8 {
		/**
		 * \~russian
		 * @brief Наибольшее кодовое значение символа Юникода
		 *
		 * \~english
		 * @brief Largest code value of a Unicode character
		 *
		 * \~
		 */
		constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

		/**
		 * \~russian
		 * @brief Наибольшая длина записи символа в кодировке UTF-8
		 *
		 * \~english
		 * @brief Largest length of a character record in the UTF-8 encoding
		 *
		 * \~
		 */
		constexpr size_t MAX_LENGTH = 4;

		/**
		 * \~russian
		 * @brief Функция представления кодового значения символа записью UTF-8
		 *
		 * @details Кодовые значения суррогатных пар и значения, превышающие наибольшее
		 *          кодовое значение Юникода, записи не имеют.
		 *
		 * @param code   кодовое значение записываемого символа
		 * @param buffer буфер записи символа длиной не менее «MAX_LENGTH»
		 * @return       длина записи символа либо нулевое значение при отказе
		 *
		 * \~english
		 * @brief Function representing a character code value by a UTF-8 record
		 *
		 * @details Code values of surrogate pairs and values exceeding the largest Unicode code
		 *          value have no record.
		 *
		 * @param code   code value of the character being recorded
		 * @param buffer buffer of the character record no shorter than "MAX_LENGTH"
		 * @return       length of the character record or zero on failure
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t encode(const uint32_t code, char * buffer) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора записи символа в кодировке UTF-8
		 *
		 * @details Записи, длина которых превышает необходимую для кодового значения,
		 *          записи суррогатных пар и записи, оборванные концом текста, отклоняются.
		 *
		 * @param text текст, записанный в кодировке UTF-8
		 * @param pos  положение начала записи символа в тексте
		 * @param code кодовое значение разобранного символа
		 * @return     длина разобранной записи либо нулевое значение при отказе
		 *
		 * \~english
		 * @brief Function parsing a character record in the UTF-8 encoding
		 *
		 * @details Records whose length exceeds the one necessary for the code value, records of
		 *          surrogate pairs and records cut short by the end of the text are rejected.
		 *
		 * @param text text recorded in the UTF-8 encoding
		 * @param pos  position of the beginning of the character record within the text
		 * @param code code value of the parsed character
		 * @return     length of the parsed record or zero on failure
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t decode(string_view text, const size_t pos, uint32_t & code) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки правильности записи текста в кодировке UTF-8
		 *
		 * @param text проверяемый текст
		 * @return     результат проверки правильности записи текста
		 *
		 * \~english
		 * @brief Function verifying the correctness of a text record in the UTF-8 encoding
		 *
		 * @param text text being verified
		 * @return     result of verifying the correctness of the text record
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool valid(string_view text) noexcept;
		/**
		 * \~russian
		 * @brief Функция подсчёта количества символов текста в кодировке UTF-8
		 *
		 * @param text текст, записанный в кодировке UTF-8
		 * @return     количество символов текста либо нулевое значение при отказе
		 *
		 * \~english
		 * @brief Function counting the number of characters of a text in the UTF-8 encoding
		 *
		 * @param text text recorded in the UTF-8 encoding
		 * @return     number of characters of the text or zero on failure
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t length(string_view text) noexcept;
	};
};

#endif // __AWH_UNICODE_UTF8__
