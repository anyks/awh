/**
 * @file macro_pop.hpp
 * @date 2026-08-05
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
 * @brief Заголовочный файл возврата макросов, снятых заголовком macro_push.hpp —
 *        восстанавливает прежние определения потребителя библиотеки
 *
 * \~english
 * @brief Header file of the restoration of the macros removed by the macro_push.hpp header —
 *        it restores the former definitions of the consumer of the library
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * \~russian
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
 * \~english
 * @brief Restoration of the macros removed by the macro_push.hpp header
 * @details It brings the names back in the form they had before the removal. If a macro did not
 *          exist at all — and on the platforms other than MS Windows that is the case —
 *          the name stays non-existent, and the pair leaves no trace behind.
 *          Checked by experience on MinGW64
 *          The restoration goes in the order reverse to the removal
 * @note The header has no guard against repeated initialisation deliberately — for the same reason
 *       as macro_push.hpp: there are as many includes as removals
 * @warning This header should be included only after macro_push.hpp. A restoration without
 *          a preceding removal leaves the behaviour undefined
 *
 * \~
 */
#pragma pop_macro("GS")
#pragma pop_macro("DS")
#pragma pop_macro("SS")
#pragma pop_macro("CS")
#pragma pop_macro("ES")
#pragma pop_macro("FS")
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
