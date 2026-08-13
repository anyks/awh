/**
 * @file: context.hpp
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
 * @brief Заголовочный файл правил окружения символов метки доменного имени —
 *        правила сочетания соединителей нулевой ширины по RFC 5892 и правила
 *        двунаправленного письма по RFC 5893
 *
 * @section context_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Правила окружения CONTEXTO не проверяются</b>. Стандартом задан их
 *          набор, ограничивающий размещение точки посередине, греческого нижнего
 *          знака, еврейских знаков гереш и гершаим, разделителя катаканы и цифр
 *          арабского письма. Приложение по обработке доменных имён их проверки
 *          не предписывает, оставляя её на усмотрение владельца зоны при внесении
 *          имени, а не на разрешение имени. Набор сверки соответствия приложению
 *          их не задаёт, и введение непроверяемых правил принесло бы больше вреда,
 *          чем пользы.
 *
 *          <b>Вид соединения символов берётся из состава эталонной реализации</b>
 *          приведения доменных имён, выгружаемый средством tools/encoding/idna/joining.cpp
 *          в файл tools/encoding/idna/joining.txt. Отдельным файлом базы данных символов
 *          Юникода в составе подмодулей это свойство не поставляется.
 *
 * \~english
 * @brief Header file of the context rules for the characters of a domain name label —
 *        the rules of combining zero-width joiners per RFC 5892 and the rules of
 *        bidirectional writing per RFC 5893
 *
 * @section context_decisions Deliberate decisions
 *
 * @details What is listed below looks like an incongruity, yet it was chosen deliberately and
 *          is not subject to correction. The section exists so that examination of the code does
 *          not start over and over with the very same conclusions.
 *
 *          <b>The CONTEXTO context rules are not checked</b>. The standard defines a set of them
 *          restricting the placement of the middle dot, the Greek lower numeral sign, the Hebrew
 *          geresh and gershayim signs, the katakana middle dot and the digits of Arabic script.
 *          The annex on domain name processing does not prescribe checking them, leaving it to
 *          the discretion of the zone owner at the time a name is entered rather than at name
 *          resolution. The conformance test set of the annex does not define them, and introducing
 *          unverifiable rules would do more harm than good.
 *
 *          <b>The character joining type is taken from the reference implementation</b> of domain
 *          name mapping, dumped by the tools/encoding/idna/joining.cpp tool into the file
 *          tools/encoding/idna/joining.txt. This property is not supplied as a separate file of
 *          the Unicode character database within the submodules.
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IDNA_CONTEXT__
#define __AWH_IDNA_CONTEXT__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "types.hpp"
#include "table.hpp"

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
	 * @brief Пространство имён модуля приведения доменных имён
	 *
	 * \~english
	 * @brief Namespace of the domain name mapping module
	 *
	 * \~
	 */
	namespace idna {
		/**
		 * \~russian
		 * @brief Функция извлечения вида соединения символа
		 *
		 * @param code кодовое значение символа
		 * @return     вид соединения символа
		 *
		 * \~english
		 * @brief Function extracting the joining type of a character
		 *
		 * @param code character code value
		 * @return     joining type of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ joining_t joining(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки правил сочетания соединителей нулевой ширины
		 *
		 * @details Соединитель нулевой ширины допускается лишь вслед за знаком
		 *          сочетания согласных, а разъединитель нулевой ширины — вслед
		 *          за ним же либо между соединяющимися символами письма.
		 *
		 * @param label набор кодовых значений проверяемой метки
		 * @return      результат проверки правил сочетания соединителей
		 *
		 * \~english
		 * @brief Function checking the rules of combining zero-width joiners
		 *
		 * @details A zero-width joiner is permitted only following a virama sign, and a zero-width
		 *          non-joiner — following that same sign or between joining characters of a script.
		 *
		 * @param label set of code values of the label being checked
		 * @return      result of checking the joiner combining rules
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool joiners(const vector <uint32_t> & label) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки принадлежности доменного имени двунаправленному письму
		 *
		 * @details Доменное имя признаётся записанным двунаправленным письмом, если
		 *          хотя бы одна его метка содержит символ письма справа налево.
		 *
		 * @param labels набор меток проверяемого доменного имени
		 * @return       результат проверки принадлежности двунаправленному письму
		 *
		 * \~english
		 * @brief Function checking whether a domain name belongs to bidirectional writing
		 *
		 * @details A domain name is recognized as written in bidirectional script if at least one
		 *          of its labels contains a right-to-left character.
		 *
		 * @param labels set of labels of the domain name being checked
		 * @return       result of checking the belonging to bidirectional writing
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool directional(const vector <vector <uint32_t>> & labels) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки правила двунаправленного письма
		 *
		 * @param label набор кодовых значений проверяемой метки
		 * @return      результат проверки правила двунаправленного письма
		 *
		 * \~english
		 * @brief Function checking the bidirectional writing rule
		 *
		 * @param label set of code values of the label being checked
		 * @return      result of checking the bidirectional writing rule
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool bidirectional(const vector <uint32_t> & label) noexcept;
	};
};

#endif // __AWH_IDNA_CONTEXT__
