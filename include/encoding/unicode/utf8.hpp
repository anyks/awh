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
 * @brief Заголовочный файл кодировщика UTF-8 модуля Юникода — представление кодового значения
 *        символа последовательностью байтов, разбор последовательности байтов
 *        и проверка правильности записи текста
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён кодировщика UTF-8
	 *
	 */
	namespace utf8 {
		/**
		 * @brief Наибольшее кодовое значение символа Юникода
		 *
		 */
		constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

		/**
		 * @brief Наибольшая длина записи символа в кодировке UTF-8
		 *
		 */
		constexpr size_t MAX_LENGTH = 4;

		/**
		 * @brief Функция представления кодового значения символа записью UTF-8
		 *
		 * @details Кодовые значения суррогатных пар и значения, превышающие наибольшее
		 *          кодовое значение Юникода, записи не имеют.
		 *
		 * @param code   кодовое значение записываемого символа
		 * @param buffer буфер записи символа длиной не менее «MAX_LENGTH»
		 * @return       длина записи символа либо нулевое значение при отказе
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t encode(const uint32_t code, char * buffer) noexcept;
		/**
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
		 */
		__AWH_SHARED_EXPORT__ size_t decode(string_view text, const size_t pos, uint32_t & code) noexcept;
		/**
		 * @brief Функция проверки правильности записи текста в кодировке UTF-8
		 *
		 * @param text проверяемый текст
		 * @return     результат проверки правильности записи текста
		 *
		 */
		__AWH_SHARED_EXPORT__ bool valid(string_view text) noexcept;
		/**
		 * @brief Функция подсчёта количества символов текста в кодировке UTF-8
		 *
		 * @param text текст, записанный в кодировке UTF-8
		 * @return     количество символов текста либо нулевое значение при отказе
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t length(string_view text) noexcept;
	};
};

#endif // __AWH_UNICODE_UTF8__
