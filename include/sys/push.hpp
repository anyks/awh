/**
 * @file push.hpp
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
 * @brief Заголовочный файл временного снятия макросов, чьи имена заняты членами перечислений AWH —
 *        сохраняет прежние определения и снимает их на время объявлений, возвращает же их pop.hpp
 *
 * \~english
 * @brief Header file of the temporary removal of the macros whose names are taken by the members of the AWH enumerations —
 *        it keeps the former definitions and removes them for the duration of the declarations, while pop.hpp brings them back
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * \~russian
 * @brief Временное снятие макросов, занимающих имена членов перечислений AWH
 *
 * @details Системные заголовки MS Windows заводят макросами обычные английские слова —
 *          DELETE, ERROR, STRICT и прочие, — а препроцессор области видимости не разбирает
 *          и заменяет имя даже внутри enum class. Объявление `DELETE = 0x05` превращается
 *          в `(0x00010000) = 0x05`, и сборка отвечает отказом.
 *
 *          Снять макросы насовсем нельзя: заголовки эти открытые, и снятие протекло бы
 *          в единицу трансляции потребителя библиотеки, отняв у него имена, какими он
 *          вправе пользоваться. Поэтому имена снимаются лишь на время объявлений AWH,
 *          а прежние определения сохраняются и возвращаются заголовком pop.hpp.
 *
 *          Порядок применения — подключить push.hpp после всех прочих подключений
 *          заголовка, а pop.hpp поместить в самый конец, перед закрывающим #endif
 *          защиты от повторной инициализации.
 *
 * @par Пользующимся библиотекой
 *
 *      Пара эта защищает **объявления AWH**, но не обращения к ним. Возврат макросов
 *      следом сделан намеренно — им оставляют имена тому, кто пользуется ими по делу, —
 *      а платой за это идёт то, что запись `awh::event::error_t::INVALID_SOCKET` в
 *      **своём** коде под MS Windows столкнётся с возвращённым макросом ровно так же,
 *      как столкнулось бы само объявление.
 *
 *      Оттого правило одно: кто называет такие члены у себя, тот защищает свой файл той
 *      же парой — `#include <sys/push.hpp>` после всех подключений и
 *      `#include <sys/pop.hpp>` в конце. Иного пути нет: снять макросы насовсем
 *      значило бы отнять их у тех, кому они нужны, а переименовать члены перечислений —
 *      исказить понятия договоров в угоду одной системе.
 *
 *      Проверено опытом: `tests/net/io/static.cpp` и `parameterized.cpp` отвечали
 *      отказом «expected unqualified-id before '(' token» с указанием на строку самого
 *      возврата, пока защиту эту им не поставили.
 *
 * @note Охраны от повторной инициализации у заголовка нет намеренно: подключается он
 *       столько раз, сколько нужно, и каждому подключению отвечает своё снятие. Вложение
 *       безопасно — сохранённые определения складываются стопкой, и возврат идёт в
 *       обратном порядке
 *
 * @note Перечень шире того, что снимает sys/win32.hpp. Там снятие постоянное, и потому
 *       ограничено именами, сталкивающимися сейчас: макросы-функции FAILED(hr) и
 *       TEXT(quote) раскрываются лишь перед открывающей скобкой и в записи вида
 *       `FAILED = 0x02` безвредны. Здесь же снятие временное и ничего не стоит, поэтому
 *       имена эти взяты тоже — чтобы правка перечисления в будущем не вскрыла беду
 *
 * @warning Заголовок этот обязан идти в паре с pop.hpp. Снятие без возврата
 *          отнимает имена у потребителя библиотеки — ровно то, чего пара избегает
 *
 * \~english
 * @brief Temporary removal of the macros taking the names of the members of the AWH enumerations
 *
 * @details The system headers of MS Windows introduce ordinary English words as macros —
 *          DELETE, ERROR, STRICT and others — while the preprocessor does not parse scopes
 *          and replaces the name even inside an enum class. The declaration `DELETE = 0x05` turns
 *          into `(0x00010000) = 0x05`, and the build answers with a failure.
 *
 *          The macros cannot be removed for good: those headers are public, and the removal would leak
 *          into the translation unit of the consumer of the library, taking away from it the names it
 *          is entitled to use. Therefore the names are removed only for the duration of the AWH declarations,
 *          while the former definitions are kept and brought back by the pop.hpp header.
 *
 *          The order of use is to include push.hpp after all the other includes
 *          of the header, and to place pop.hpp at the very end, before the closing #endif
 *          of the guard against repeated initialisation.
 *
 * @par To those using the library
 *
 *      This pair protects the **AWH declarations**, but not the references to them. Bringing the macros back
 *      afterwards is done deliberately — it leaves the names to whoever uses them for a reason —
 *      and the price for that is that the record `awh::event::error_t::INVALID_SOCKET` in
 *      **your own** code under MS Windows will collide with the restored macro exactly as
 *      the declaration itself would have collided.
 *
 *      Hence the single rule: whoever names such members at home protects their own file by the
 *      same pair — `#include <sys/push.hpp>` after all the includes and
 *      `#include <sys/pop.hpp>` at the end. There is no other way: removing the macros for good
 *      would mean taking them away from those who need them, and renaming the members of the enumerations
 *      would distort the notions of the contracts for the sake of one system.
 *
 *      Checked by experience: `tests/net/io/static.cpp` and `parameterized.cpp` answered
 *      with the failure «expected unqualified-id before '(' token» pointing at the line of the
 *      restoration itself, until that protection was put in place for them.
 *
 * @note The header has no guard against repeated initialisation deliberately: it is included
 *       as many times as needed, and every include has its own removal. Nesting
 *       is safe — the kept definitions are stacked, and the restoration goes in the
 *       reverse order
 *
 * @note The list is wider than what sys/win32.hpp removes. There the removal is permanent, and therefore
 *       is limited to the names colliding right now: the function-like macros FAILED(hr) and
 *       TEXT(quote) expand only before an opening parenthesis and in a record of the
 *       `FAILED = 0x02` form are harmless. Here, on the other hand, the removal is temporary and costs nothing, therefore
 *       those names are taken as well — so that a future change of an enumeration does not uncover trouble
 *
 * @warning This header must go in a pair with pop.hpp. A removal without a restoration
 *          takes the names away from the consumer of the library — exactly what the pair avoids
 *
 * \~
 */
#pragma push_macro("TEXT")
#pragma push_macro("CALLBACK")
#pragma push_macro("ERROR")
#pragma push_macro("DELETE")
#pragma push_macro("FAILED")
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#pragma push_macro("STRICT")
#pragma push_macro("NO_ERROR")
#pragma push_macro("ALTERNATE")
#pragma push_macro("TRANSPARENT")
#pragma push_macro("INVALID_SOCKET")
#pragma push_macro("FS")
#pragma push_macro("ES")
#pragma push_macro("CS")
#pragma push_macro("SS")
#pragma push_macro("DS")
#pragma push_macro("GS")
#pragma push_macro("CS5")
#pragma push_macro("CS6")
#pragma push_macro("CS7")
#pragma push_macro("CS8")
#pragma push_macro("PRIVATE")
#pragma push_macro("SHARED")
#pragma push_macro("SUSPENDED")

#undef TEXT
#undef CALLBACK
#undef ERROR
#undef DELETE
#undef FAILED
#undef TRUE
#undef FALSE
#undef STRICT
#undef NO_ERROR
#undef ALTERNATE
#undef TRANSPARENT
#undef INVALID_SOCKET

/**
 * \~russian
 * Имена указателей сегментов набора команд x86
 *
 * @details Заголовок sys/regset.h у Sun Solaris и illumos заводит макросами имена
 *          указателей сегментов - FS, ES, CS, SS, DS, GS, - раскрывая их в порядковые
 *          числа состояния процесса. Приходит он не напрямую, а через заголовки работы
 *          с процессами, и оттого столкновение всплывает не везде: перечисление
 *          address_t с членом FS собиралось на Solaris и отказывало на OpenIndiana,
 *          где набор подключаемых заголовков сложился иначе.
 *
 * @note Сняты все шесть, а не один лишь столкнувшийся FS: имена эти короткие и общие,
 *       под члены перечислений годятся все, и снятие их ничего не стоит - ровно тот же
 *       довод, по какому выше взяты FAILED и TEXT
 *
 * \~english
 * Names of the segment pointers of the x86 instruction set
 *
 * @details The sys/regset.h header of Sun Solaris and illumos introduces as macros the names
 *          of the segment pointers — FS, ES, CS, SS, DS, GS — expanding them into the ordinal
 *          numbers of the process state. It comes not directly but through the headers for working
 *          with processes, and that is why the collision surfaces not everywhere: the enumeration
 *          address_t with the FS member built on Solaris and failed on OpenIndiana,
 *          where the set of included headers turned out differently.
 *
 * @note All six are removed rather than the single colliding FS: those names are short and common,
 *       all of them are fit for the members of enumerations, and removing them costs nothing — exactly the same
 *       argument by which FAILED and TEXT are taken above
 *
 * \~
 */
#undef FS
#undef ES
#undef CS
#undef SS
#undef DS
#undef GS

/**
 * \~russian
 * Имена размеров знака у настроек последовательной связи
 *
 * @details Заголовок <termios.h> заводит макросами имена CS5, CS6, CS7 и CS8, задающие
 *          число разрядов в знаке у последовательного порта. Имена эти совпадают с
 *          членами перечисления dscp_t, где CS означает совсем иное - класс обслуживания
 *          пакета по RFC 2474.
 *
 * @note Столкновение это скрытое: дерево собирается лишь потому, что <termios.h> в него
 *       никто не включает. У потребителя же библиотеки, работающего с последовательным
 *       портом, объявление `CS5 = 0x28` обратилось бы в `(0000000) = 0x28` - ровно тот же
 *       отказ, каким встречает MS Windows своё DELETE
 *
 * @note Снят и CS8, какого у перечисления сейчас нет: по доводу выше снятие временное и
 *       ничего не стоит, а имя из того же ряда
 *
 * \~english
 * Names of the character sizes of the settings of the serial link
 *
 * @details The <termios.h> header introduces as macros the names CS5, CS6, CS7 and CS8, setting
 *          the number of the bits in a character of a serial port. Those names coincide with
 *          the members of the dscp_t enumeration, where CS means something entirely different — the class
 *          of the service of a packet by RFC 2474.
 *
 * @note This collision is hidden: the tree builds only because nobody includes <termios.h>
 *       into it. At the consumer of the library working with a serial
 *       port, the declaration `CS5 = 0x28` would turn into `(0000000) = 0x28` — exactly the same
 *       failure with which MS Windows meets its own DELETE
 *
 * @note CS8 is removed as well, the one the enumeration does not have right now: by the argument above the removal is temporary and
 *       costs nothing, while the name is from the same row
 *
 * \~
 */
#undef CS5
#undef CS6
#undef CS7
#undef CS8

/**
 * \~russian
 * Имена признаков отображения и состояния у Sun Solaris и illumos
 *
 * @details Заголовок <sys/mman.h> у Sun Solaris и illumos заводит макросами PRIVATE и
 *          SHARED - собственные признаки отображения памяти поверх обычных MAP_PRIVATE и
 *          MAP_SHARED, - раскрывая их в 0x20 и 0x10. Имя SUSPENDED приходит из набора
 *          заголовков работы с процессами и нитями и всплывает лишь при их совместном
 *          включении. Все три совпадают с членами перечислений AWH: PRIVATE - с
 *          cryptography::key_t (вид ключа), SHARED - с locker_t::mode_t (разделённая
 *          блокировка), SUSPENDED - с fiber::state_t (волокно спит на своём стеке).
 *
 * @note Столкновение это скрытое и системно-зависимое: PRIVATE и SHARED отказывали и на
 *       Solaris, и на OpenIndiana, а SUSPENDED - лишь на OpenIndiana, где набор
 *       подключаемых заголовков сложился шире. Снятие временное и ничего не стоит - по
 *       тому же доводу, что и ряды выше
 *
 * \~english
 * Names of the mapping flags and of the state of Sun Solaris and illumos
 *
 * @details The <sys/mman.h> header of Sun Solaris and illumos introduces as macros PRIVATE and
 *          SHARED — its own memory-mapping flags on top of the ordinary MAP_PRIVATE and
 *          MAP_SHARED — expanding them into 0x20 and 0x10. The name SUSPENDED comes from the set
 *          of the headers for working with processes and threads and surfaces only upon their joint
 *          inclusion. All three coincide with the members of the AWH enumerations: PRIVATE — with
 *          cryptography::key_t (a kind of a key), SHARED — with locker_t::mode_t (a shared
 *          lock), SUSPENDED — with fiber::state_t (the fiber sleeps on its own stack).
 *
 * @note This collision is hidden and system-dependent: PRIVATE and SHARED failed on both
 *       Solaris and OpenIndiana, while SUSPENDED — only on OpenIndiana, where the set of
 *       the included headers turned out wider. The removal is temporary and costs nothing — by
 *       the same argument as the rows above
 *
 * \~
 */
#undef PRIVATE
#undef SHARED
#undef SUSPENDED
