/**
 * @file: macro_pop.hpp
 * @date: 2026-08-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл возврата макросов, снятых заголовком macro_push.hpp —
 *        восстанавливает прежние определения потребителя библиотеки
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * @brief Возврат макросов, снятых заголовком macro_push.hpp
 *
 * @details Возвращает имена в том виде, в каком они были до снятия. Если макроса не
 *          существовало вовсе — а на платформах, отличных от MS Windows, так и есть, —
 *          имя остаётся несуществующим, и никакого следа пара после себя не оставляет.
 *          Проверено опытом на MinGW64
 *
 *          Возврат идёт в порядке, обратном снятию
 *
 * @note Охраны от повторной инициализации у заголовка нет намеренно — по той же причине,
 *       что и у macro_push.hpp: подключений столько же, сколько снятий
 *
 * @warning Подключать заголовок этот следует только вслед за macro_push.hpp. Возврат без
 *          предшествующего снятия поведения не определяет
 *
 */
#pragma pop_macro("INVALID_SOCKET")
#pragma pop_macro("TRANSPARENT")
#pragma pop_macro("ALTERNATE")
#pragma pop_macro("NO_ERROR")
#pragma pop_macro("STRICT")
#pragma pop_macro("FAILED")
#pragma pop_macro("DELETE")
#pragma pop_macro("ERROR")
#pragma pop_macro("CALLBACK")
#pragma pop_macro("TEXT")
