/**
 * @file numeric.hpp
 * @date 2026-09-01
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
 * @brief Заголовочный файл общего для всех кодеков рамки извлечения числа из записи —
 *        разбор записи, опознание её вида и приведение к затребованному виду числа
 *
 * \~english
 * @brief Header file of the extraction of a number from a text common for all the codecs of the framework —
 *        the parsing of the text, the recognition of its shape and the conversion to the requested type of a number
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_NUMERIC__
#define __AWH_CODEC_NUMERIC__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"

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
	 * Подключаем пространство имён стандартной библиотеки
	 */
	using namespace std;
	/**
	 * \~russian
	 * @brief Пространство имён кодеков
	 *
	 * \~english
	 * @brief Namespace of the codecs
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Шаблон вида числа, извлекаемого из записи
		 * @tparam T вид числа, извлекаемого из записи
		 *
		 * \~english
		 * @brief Template of the type of a number being extracted from a text
		 * @tparam T type of a number being extracted from a text
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Метод извлечения числа из записи затребованным видом
		 *
		 * @details Место это общее у всех кодеков рамки, и заведено оно решением владельца
		 * от 01.09.2026: договор извлечения числа един, а написан разбор был у каждого
		 * кодека свой, и написания расходились между собою. Правила извлечения таковы:
		 *
		 * 1. Пробельная обвязка записи отбрасывается.
		 * 2. Записи `nan` и `inf` числом не считаются.
		 * 3. Вид разбора решается ЗАПИСЬЮ, а не затребованным видом: `1e2` и `300.5`
		 *    извлекаются и целым, округляясь с уводом половины от нуля.
		 * 4. Целое, в затребованный вид не вмещающееся, заворачивается по кругу правилами
		 *    языка, а дробное с настоящей дробной частью выдаётся пределом вида.
		 * 5. Разобрана обязана быть вся запись целиком: остаток за числом есть отказ.
		 *
		 * @note Отказ следует лишь тогда, когда запись числом не является вовсе: запись,
		 *       в разрядность не вмещающаяся, числом быть не перестаёт
		 *
		 * @note Переменная-приёмник при отказе не трогается вовсе и сохраняет то, что
		 *       несла до вызова
		 *
		 * @param text   разбираемая запись числа
		 * @param result ссылка на результат извлечения
		 * @return       признак успешного извлечения
		 *
		 * \~english
		 * @brief Method of the extraction of a number from a text by the requested type
		 * @details This place is common for all the codecs of the framework. The parsing of the text
		 * discards the whitespace padding, does not reckon `nan` and `inf` among the numbers, decides the shape
		 * of the parsing by the TEXT rather than by the requested type and wraps around an integer that does not fit
		 * into the requested type. A refusal follows only when the text is not a number at all
		 * @param text   text of a number being parsed
		 * @param result reference to the result of the extraction
		 * @return       flag of a successful extraction
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool numeric(const string_view text, T & result) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон вида числа, к какому ведётся приведение
		 * @tparam T вид числа, к какому ведётся приведение
		 *
		 * \~english
		 * @brief Template of the type of a number to which the conversion is conducted
		 * @tparam T type of a number to which the conversion is conducted
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Метод приведения дробного числа к затребованному виду
		 *
		 * @details Место это общее у всех кодеков рамки: у текстовых оно завершает разбор
		 * записи, а у кодеков, число при чтении разбирающих, приведением извлечение и
		 * исчерпывается. Приведение отвечает языку ВЕЗДЕ, где у языка ответ определён:
		 * целое, в затребованный вид не помещающееся, заворачивается по кругу, и туда же
		 * отнесено дробное с нулевой дробной частью. Разница с `static_cast` одна - дробное,
		 * у которого дробная часть есть на деле, а целая лежит за пределами затребованного
		 * вида, выдаётся пределом этого вида, ибо у языка там поведение неопределено
		 *
		 * @note Дробная часть ОКРУГЛЯЕТСЯ с уводом половины от нуля, а не отбрасывается
		 *       усечением к нулю
		 *
		 * @param value приводимое дробное число
		 * @return      приведённое число
		 *
		 * \~english
		 * @brief Method of the conversion of a floating-point number to the requested type
		 * @details The conversion answers the language EVERYWHERE where the answer of the language is defined.
		 * The fractional part is ROUNDED with the half away from zero
		 * @param value floating-point number being converted
		 * @return      converted number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T convert(const double value) noexcept;
	};
};

#endif // __AWH_CODEC_NUMERIC__
