/**
 * @file: punycode.hpp
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
 * @brief Заголовочный файл кодировщика Punycode — представление набора кодовых
 *        значений записью из символов набора ASCII и обратный разбор такой записи
 *
 * \~english
 * @brief Header file of the Punycode encoder — representation of a set of code values by
 *        a record of characters of the ASCII set and the reverse parsing of such a record
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IDNA_PUNYCODE__
#define __AWH_IDNA_PUNYCODE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
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
	 * @brief Пространство имён кодировщика Punycode
	 *
	 * \~english
	 * @brief Punycode encoder namespace
	 *
	 * \~
	 */
	namespace punycode {
		/**
		 * \~russian
		 * @brief Функция представления набора кодовых значений записью Punycode
		 *
		 * @details Приставка, которой обозначается запись метки, к результату
		 *          не присоединяется.
		 *
		 * @param text   набор кодовых значений представляемого текста
		 * @param result получившаяся запись из символов набора ASCII
		 * @return       результат выполнения представления текста
		 *
		 * \~english
		 * @brief Function representing a set of code values by a Punycode record
		 *
		 * @details The prefix designating a label record is not appended to the result.
		 *
		 * @param text   set of code values of the text being represented
		 * @param result resulting record of characters of the ASCII set
		 * @return       result of performing the text representation
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool encode(const vector <uint32_t> & text, string & result) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора записи Punycode
		 *
		 * @details Приставка, которой обозначается запись метки, разбору
		 *          не подлежит и передаче не подлежит.
		 *
		 * @param text   разбираемая запись из символов набора ASCII
		 * @param result набор кодовых значений разобранного текста
		 * @return       результат выполнения разбора записи
		 *
		 * \~english
		 * @brief Punycode record parsing function
		 *
		 * @details The prefix designating a label record is subject neither to parsing nor to passing in.
		 *
		 * @param text   record of characters of the ASCII set being parsed
		 * @param result set of code values of the parsed text
		 * @return       result of performing the record parsing
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool decode(string_view text, vector <uint32_t> & result) noexcept;
	};
};

#endif // __AWH_IDNA_PUNYCODE__
