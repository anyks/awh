/**
 * @file utf8.hpp
 * @date 2026-08-03
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
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/macro/global.hpp"

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
		 * @brief Обозначение кодового значения, знаком не являющегося
		 *
		 * @details Его выводит побайтовый разбор и при негодном ведущем октете, и при
		 *          нарушенном продолжающем, и при оборванной записи: различает эти случаи
		 *          не значение, а отмеренная длина наибольшей годной части
		 *
		 * \~english
		 * @brief Marker of a code value that is not a character
		 *
		 * \~
		 */
		constexpr uint32_t INVALID_CODEPOINT = static_cast <uint32_t> (~0u);

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
		/**
		 * \~russian
		 * @brief Функция определения длины побайтовой последовательности по ведущему октету
		 *
		 * @details Ведущими не бывают октеты `C0` и `C1`: открывали бы запись длиннее
		 *          необходимой, а октеты свыше `F4` - точку свыше U+10FFFF. Ни того, ни
		 *          другого кодировка не допускает при любом продолжении
		 *
		 * @param leading ведущий октет последовательности
		 * @return        длина последовательности либо нуль при ошибочном ведущем октете
		 *
		 * \~english
		 * @brief Function of the obtaining of the length of a byte sequence by the leading octet
		 * @param leading leading octet of the sequence
		 * @return length of the sequence or zero at an erroneous leading octet
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t sequence(const uint8_t leading) noexcept;
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
		 * @brief Функция дописывания кодового значения знака к тексту
		 *
		 * @details Тело общее, а не местное: побайтовая сборка записи не должна жить в
		 *          нескольких местах, ибо починка одного места в прочие не приезжает
		 *
		 * @param code   записываемое кодовое значение знака
		 * @param result текст, к которому дописывается знак
		 * @return       признак успешной записи: у суррогата и у точки свыше U+10FFFF записи нет
		 *
		 * \~english
		 * @brief Function of appending the code value of a character to a text
		 * @param code code value of the character being recorded
		 * @param result text to which the character is appended
		 * @return record result: a surrogate and a point above U+10FFFF have no record
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool encode(const uint32_t code, string & result) noexcept;
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
		 * @brief Функция разбора побайтовой последовательности знака
		 *
		 * @details Разбор ведётся по правилу наибольшей годной части (Юникод, раздел 3.9),
		 *          и в этом его отличие от `decode(string_view, ...)`: негодной подаче
		 *          отмеряется длина уже годной её части, а не ноль, дабы перебор, идущий по
		 *          этой длине, не встал на месте навек. Негодными признаются ведущие `C0`, `C1`
		 *          и всякий октет свыше `F4`, равно как нарушенный продолжающий: предел своего
		 *          первый продолжающий байт берёт у ведущего (`E0` требует `A0..BF`, `ED` - не
		 *          выше `9F`, `F0` требует `90..BF`, `F4` - не выше `8F`), чем запись длиннее
		 *          необходимой, суррогат да точка свыше U+10FFFF отвергаются разом
		 *
		 * @note Оборванный на конце текста заход различим с негодным по сумме: при нехватке
		 *       октетов отмеряется их наличное число, и потребителю, подающему текст кусками,
		 *       этого довольно, чтобы ждать остаток, а не отвергать знак
		 *
		 * @param buffer буфер разбираемого текста
		 * @param size   количество октетов в буфере
		 * @param length отмеренная длина последовательности; при пустой подаче - нуль
		 * @return       кодовое значение знака либо обозначение негодного
		 *
		 * \~english
		 * @brief Function of the parsing of the byte sequence of a character
		 * @param buffer buffer of the parsed text
		 * @param size number of the octets of the buffer
		 * @param length measured length of the sequence; zero at an empty request
		 * @return code value of the character or the marker of an unsuitable one
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t decode(const char * buffer, const size_t size, size_t & length) noexcept;
	};
};
