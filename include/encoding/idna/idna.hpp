/**
 * @file idna.hpp
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
 * @brief Заголовочный файл модуля приведения доменных имён — приведение имени
 *        к записи из символов набора ASCII и обратное приведение к записи Юникода
 *        по приложению по обработке доменных имён стандарта Юникода
 *
 * @section idna_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Переходный режим преобразования символов по умолчанию не применяется</b>.
 *          Приложение по обработке доменных имён объявило его устаревшим: он приводит
 *          немецкую букву эсцет и греческую конечную сигму к иным символам, отчего
 *          одно и то же имя разрешается по-разному. Режим оставлен настройкой, так как
 *          обращение к зонам, заведённым по прежним правилам, без него невозможно.
 *
 *          <b>Правила записи имён узлов по умолчанию не применяются</b>. Приложение
 *          оставляет их применение на усмотрение потребителя, а Framework разрешает
 *          имена не только для протокола DNS, где эти правила обязательны.
 *
 *          <b>Приведение к записи Юникода не отвергает имя целиком</b> при ошибке
 *          разбора отдельной метки, а оставляет её как есть. Так предписывает
 *          приложение: обратное приведение служит показу имени человеку, и показ
 *          имени с неразобранной меткой полезнее отказа.
 *
 *          <b>Издание стандарта таблицы преобразований отстаёт от издания прочих
 *          таблиц модуля Юникода</b>: таблица берётся из состава подмодуля эталонной
 *          реализации приведения доменных имён, а прочие таблицы — из состава подмодуля
 *          эталонной реализации регулярных выражений. Сверка с эталоном тем самым
 *          остаётся осмысленной, а расхождение изданий сказывается лишь на символах,
 *          добавленных позднее и в доменных именах пока не встречающихся.
 *
 * \~english
 * @brief Header file of the domain name mapping module — bringing a name to a record of
 *        characters of the ASCII set and the reverse bringing to a Unicode record per the
 *        annex on domain name processing of the Unicode standard
 *
 * @section idna_decisions Deliberate decisions
 *
 * @details What is listed below looks like an incongruity, yet it was chosen deliberately and
 *          is not subject to correction. The section exists so that examination of the code does
 *          not start over and over with the very same conclusions.
 *
 *          <b>The transitional character transformation mode is not applied by default</b>. The
 *          annex on domain name processing declared it obsolete: it maps the German sharp s and
 *          the Greek final sigma to other characters, whereby one and the same name resolves
 *          differently. The mode is left as a setting, since addressing zones set up under the
 *          former rules is impossible without it.
 *
 *          <b>The host name record rules are not applied by default</b>. The annex leaves their
 *          application to the discretion of the consumer, and the Framework resolves names not
 *          only for the DNS protocol, where these rules are mandatory.
 *
 *          <b>Bringing to a Unicode record does not reject the name as a whole</b> upon a parsing
 *          error of an individual label, but leaves it as it is. That is what the annex prescribes:
 *          the reverse bringing serves to display the name to a human, and displaying a name with
 *          an unparsed label is more useful than a refusal.
 *
 *          <b>The standard edition of the transformations table lags behind the edition of the
 *          other tables of the Unicode module</b>: the table is taken from the submodule of the
 *          reference implementation of domain name mapping, and the other tables — from the
 *          submodule of the reference implementation of regular expressions. Verification against
 *          the reference thereby remains meaningful, and the divergence of editions affects only
 *          characters added later and not yet encountered in domain names.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IDNA__
#define __AWH_IDNA__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "types.hpp"
#include "table.hpp"
#include "context.hpp"
#include "punycode.hpp"

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
		 * @brief Функция извлечения состояния символа в таблице преобразований
		 *
		 * @param code   кодовое значение символа
		 * @param result набор кодовых значений преобразования символа
		 * @return       состояние символа в таблице преобразований
		 *
		 * \~english
		 * @brief Function extracting the status of a character in the transformations table
		 *
		 * @param code   character code value
		 * @param result set of code values of the character transformation
		 * @return       status of the character in the transformations table
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ status_t status(const uint32_t code, vector <uint32_t> & result) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения описания ошибки приведения доменного имени
		 *
		 * @param error код ошибки приведения доменного имени
		 * @return      описание ошибки приведения доменного имени
		 *
		 * \~english
		 * @brief Function extracting the description of a domain name mapping error
		 *
		 * @param error code of the domain name mapping error
		 * @return      description of the domain name mapping error
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view message(const error_t error) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения доменного имени к записи из символов набора ASCII
		 *
		 * @param domain приводимое доменное имя, записанное в кодировке UTF-8
		 * @param result получившаяся запись доменного имени
		 * @param error  код ошибки приведения доменного имени
		 * @param mode   набор режимов приведения доменного имени
		 * @return       результат выполнения приведения доменного имени
		 *
		 * \~english
		 * @brief Function bringing a domain name to a record of characters of the ASCII set
		 *
		 * @param domain domain name being brought, recorded in the UTF-8 encoding
		 * @param result resulting record of the domain name
		 * @param error  code of the domain name mapping error
		 * @param mode   set of domain name mapping modes
		 * @return       result of performing the domain name mapping
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool toAscii(string_view domain, string & result, error_t & error, const uint16_t mode = DEFAULT_MODE) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения доменного имени к записи из символов набора ASCII
		 *
		 * @details Отказ приведения выводится пустой записью. Приведение, причину
		 *          отказа которого требуется получить, выполняется одноимённой функцией,
		 *          выводящей код ошибки отдельно.
		 *
		 * @param domain приводимое доменное имя, записанное в кодировке UTF-8
		 * @param mode   набор режимов приведения доменного имени
		 * @return       получившаяся запись доменного имени
		 *
		 * \~english
		 * @brief Function bringing a domain name to a record of characters of the ASCII set
		 *
		 * @details A failure of the bringing is output as an empty record. A bringing whose reason of
		 *          failure is required is performed by the function of the same name, which outputs the
		 *          error code separately.
		 *
		 * @param domain domain name being brought, recorded in the UTF-8 encoding
		 * @param mode   set of domain name mapping modes
		 * @return       resulting record of the domain name
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string toAscii(string_view domain, const uint16_t mode = DEFAULT_MODE) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения доменного имени к записи Юникода
		 *
		 * @param domain приводимое доменное имя
		 * @param result получившаяся запись доменного имени в кодировке UTF-8
		 * @param error  код ошибки приведения доменного имени
		 * @param mode   набор режимов приведения доменного имени
		 * @return       результат выполнения приведения доменного имени
		 *
		 * \~english
		 * @brief Function bringing a domain name to a Unicode record
		 *
		 * @param domain domain name being brought
		 * @param result resulting record of the domain name in the UTF-8 encoding
		 * @param error  code of the domain name mapping error
		 * @param mode   set of domain name mapping modes
		 * @return       result of performing the domain name mapping
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool toUnicode(string_view domain, string & result, error_t & error, const uint16_t mode = DEFAULT_MODE) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения доменного имени к записи Юникода
		 *
		 * @details Метки, разобрать которые не вышло, выводятся без изменений,
		 *          что предписано приложением по обработке доменных имён.
		 *
		 * @param domain приводимое доменное имя
		 * @param mode   набор режимов приведения доменного имени
		 * @return       получившаяся запись доменного имени в кодировке UTF-8
		 *
		 * \~english
		 * @brief Function bringing a domain name to a Unicode record
		 *
		 * @details Labels that could not be parsed are output unchanged, which is prescribed by the
		 *          annex on domain name processing.
		 *
		 * @param domain domain name being brought
		 * @param mode   set of domain name mapping modes
		 * @return       resulting record of the domain name in the UTF-8 encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string toUnicode(string_view domain, const uint16_t mode = DEFAULT_MODE) noexcept;
	};
};

#endif // __AWH_IDNA__
