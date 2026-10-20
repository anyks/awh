/**
 * @file document.hpp
 * @date 2026-09-04
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
 * @brief Заголовочный файл события CEF, удерживаемого целиком
 *
 * \~english
 * @brief Header file of a CEF event held in full
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
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "dictionary.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../value.hpp"
#include "../abc/value.hpp"
#include "../../net/addr.hpp"
#include "../../sys/fs.hpp"
#include "../../sys/chrono.hpp"
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
	 * @brief Пространство имён контейнеров данных
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён контейнера CEF
		 *
		 * \~english
		 * @brief CEF container namespace
		 *
		 * \~
		 */
		namespace cef {
			/**
			 * \~russian
			 * @brief Класс события CEF, удерживаемого целиком
			 *
			 * @details Держит разобранное событие деревом контейнера ABC и выдаёт его
			 *          ходами по пути, общими у всех кодеков библиотеки. Устройство дерева
			 *          отвечает устройству самой записи:
			 *
			 * @li `/syslog` - приставка syslog, записи предшествующая, знаками;
			 *
			 * @li `/header` - поля заголовка отображением с именами `version`,
			 *     `vendor`, `product`, `release`, `signature`, `name`, `severity`;
			 *
			 * @li `/extension` - пары расширения отображением по СЫРЫМ ключам записи
			 *
			 * @par Намеренные решения
			 *      Перечисленное ниже не является пробелом реализации: это очерченные границы
			 *      задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Своего владеющего значения (`value_t`) у кодека НЕТ, и это решение,
			 *     а не пробел.** Основанием событию служит дерево контейнера ABC: система
			 *     видов его вмещает виды записи CEF с запасом, а ходы `at`, `place`,
			 *     `contains`, `erase` даны им уже. Заводить поверх второй владеющий вид
			 *     значило бы держать один договор в двух местах и переводить дерево в
			 *     дерево на всяком обращении
			 *
			 * @li **Глубина пути ограничена устройством записи.** Путь длиннее трёх
			 *     звеньев - `/extension/<ключ>/<номер>` у повторяющегося ключа - записи
			 *     CEF неведом. Это граница формата, а не недоделка обхода. Закреплено
			 *    `CodecCefDocument.PathDepthIsBoundedByTheFormat`
			 *
			 * @li **Обход по пути выдаёт СЫРЫЕ ключи.** Сведённое именование берётся
			 *     вторыми ходами: `field` разыскивает значение по полному имени словаря,
			 *    `label` выдаёт человеческое имя ключа - взятое из метки записи, а при
			 *     её отсутствии из словаря. Закреплено `CodecCefDocument.Naming` и
			 *     `CodecCefDocument.FieldByFullName`
			 *
			 * @li **Повтор ключа даёт перечень.** Второе объявление того же ключа
			 *    обращает значение в перечень и добавляет в его конец; обход остаётся
			 *    замкнутым числовыми звеньями пути - и на чтении, и на ЗАПИСИ. Счёт пар
			 *    ведётся по значениям, а не по ключам. Закреплено
			 *    `CodecCefDocument.SetThroughArrayIndex` и
			 *    `CodecCefDocument.SizeCountsPairsNotKeys`
			 *
			 * @li **Сброс значения и снос пары - РАЗНОЕ.** Сброс оставляет ключ с
			 *     пустым значением («cs3=»), снос убирает пару из записи вовсе. Различия
			 *     «пусто» и «нет вовсе» сама запись CEF не несёт, оттого сброшенное поле
			 *     от записанного пустым неотличимо - и это граница формата, названная
			 *     прямо. Закреплено `CodecCefDocument.ResetAgainstErase`
			 *
			 * @li **Каталог, поданный вместо файла, отвергается кодом `FILE_NOT_READ`,
			 *     а не `FILE_NOT_OPENED`.** Каталог на POSIX открывается УСПЕШНО и читается
			 *     признаками конца - теми же, какими отзывается пустой файл, - отчего до
			 *     заслона подача его отвечалась ИСТИНОЙ с пустым деревом. Код назван по
			 *     существу беды: «открыть не удалось» слало бы потребителя искать права
			 *     доступа либо отсутствующий путь, тогда как подан не файл вовсе.
			 *     Распознаётся каталог ДО чтения ходом `fs_t::type` - у MS Windows он не
			 *     открывается вовсе, и проверка после открытия была бы там мертва.
			 *     Закреплено `CodecContract.DirectoryFedInsteadOfAFileIsRefused`, где код
			 *     сличается числом у восьми кодеков разом
			 *
			 * @li **Символьная ссылка ПРОХОДИТСЯ, а не отличается от своей цели.** Спрос
			 *     вида объекта ведётся `fs_t::type(addr, false)`: ссылка на файл читается
			 *     ровно как файл, а ссылка на каталог отвергается ровно как каталог. Ход
			 *     этот по умолчанию ссылку ОТЛИЧАЕТ, отвечая `LINK`, и первая редакция
			 *     заслона отвергала всякую ссылку кодом `FILE_NOT_OPENED`, тогда как прочесть
			 *     её средство файловой системы давало без единой жалобы - отказ был выдуман
			 *     кодеком, а не системой. Закреплено `CodecCefDocument.SymlinkIsReadAsFile`
			 *     и `CodecCefDocument.SymlinkToDirectoryIsRefused`
			 *
			 * @li **Сохранение НЕДЕЛИМО: запись идёт во временный файл, подменяющий
			 *     цель переименованием.** Цель оттого либо остаётся прежней целиком, либо
			 *     становится новой целиком, а половины её не видно никогда. Порядок таков:
			 *     запись во временный файл рядом с целью, сброс его на носитель, подмена
			 *     ходом `fs_t::replaceAddress`; всякий отказ по дороге сносит временный файл
			 *     и цели не касается. Закреплено `CodecCefDocument.FailedSaveKeepsThePreviousRecord`
			 *     и `CodecCefDocument.SaveOverLongerFile`
			 *
			 * @li **Временный файл создаётся исключительно для текущего сохранения.**
			 *     Занятое имя пропускается без удаления файла; запись и сброс используют один
			 *     открытый объект. Под POSIX сохраняются биты доступа существующей цели,
			 *     новая цель получает прежние 0644 с учётом umask. Владение и ACL не переносятся.
			 *     Закреплено `CodecCefDocument.SavePreservesAccessBits`,
			 *     `CodecCefDocument.SaveKeepsUnrelatedSibling` и
			 *     `CodecCefDocument.TemporaryCollisionKeepsExistingFile`
			 *
			 * @warning ПРАВЛЕНО 16.09.2026, и прежнее решение было ОШИБОЧНЫМ. Гласило
			 *          оно: «сохранение сносит прежний файл ПЕРЕД записью», ибо `fs_t::write` файл
			 *          не усекает, а пишет по смещению, и без сноса хвост прежнего сохранения
			 *          торчал бы за концом нового. Довод о хвосте верен и посейчас - неверен был
			 *          ВЫВОД из него: снос решал задачу усечения ценою данных потребителя. Отказ
			 *          записи после сноса не оставлял НИЧЕГО. Доказано опытом на отдельном томе в
			 *          два мегабайта: том забит под завязку, прежний файл о 69 байтах, запись о
			 *          200 000 байтах - сохранение честно ответило ложью с кодом `FILE_NOT_OPENED`,
			 *          а файл обратился в НУЛЬ байтов. Временный файл решает обе задачи разом:
			 *          усечения не нужно вовсе, ибо файл заводится пустым, а цель до последнего
			 *          мига цела
			 *
			 * @note Временный файл кладётся РЯДОМ с целью, а не в каталог временных: тот
			 *       может лежать на иной файловой системе, и переименование обратилось бы в
			 *       копирование со сносом, потеряв неделимость молча. Способ этот кодеками CSV,
			 *       TOML, XML и ABC применялся давно - здесь он лишь не был применён
			 *
			 * @li **За записью следует сброс на носитель, и отказ его записи не
			 *     отменяет.** Запись, отвеченная успехом, лежит ещё во вместилище ядра, и
			 *     обрыв питания её теряет; сохранение события журнала тем и ценно, что
			 *     переживает падение машины. Своего `fsync` тут нет намеренно: под macOS он
			 *     долговечности НЕ обещает вовсе, доводит до пластины лишь `F_FULLFSYNC`, и
			 *     ход `fs_t::flush` это знает. Отказ сброса уходит предупреждением в журнал,
			 *     а не отказом сохранения: записанное на месте и читается
			 *
			 * \~english
			 * @brief Class of a CEF event held in full
			 *
			 * @details Holds a parsed event as a tree of the ABC container and issues it
			 *          by the methods by a path, common to all the codecs of the library.
			 *          The shape of the tree answers the shape of the record itself:
			 *
			 * @li `/syslog` - the syslog prefix preceding the record, as characters;
			 *
			 * @li `/header` - the fields of the header as a mapping with the names
			 *     `version`, `vendor`, `product`, `release`, `signature`, `name`, `severity`;
			 *
			 * @li `/extension` - the extension pairs as a mapping by the RAW keys of the record
			 *
			 * @par Deliberate decisions
			 *      The list below is not a gap of the implementation: these are the outlined
			 *      boundaries of the task, and each decision is fixed by a check
			 *
			 * @li **The codec has NO owning value (`value_t`) of its own, and that is a
			 *     decision, not a gap.** The event is built upon the tree of the ABC container:
			 *     its system of kinds covers the kinds of the CEF record with margin, and the
			 *     methods `at`, `place`, `contains`, `erase` are already given by it. To layer
			 *     a second owning type on top would mean keeping one contract in two places and
			 *     translating tree into tree on every access
			 *
			 * @li **The depth of a path is bounded by the shape of the record.** A path
			 *     longer than three links - `/extension/<key>/<number>` for a repeating key -
			 *     is unknown to a CEF record. This is a boundary of the format, not a missing
			 *     feature of the walk. Fixed by `CodecCefDocument.PathDepthIsBoundedByTheFormat`
			 *
			 * @li **The walk by a path issues the RAW keys.** The reduced naming is
			 *     retrieved by the second methods: `field` looks up a value by the full name
			 *     of the dictionary, `label` issues the human-readable name of a key - drawn
			 *     from the label of the record, and when absent, from the dictionary. Fixed by
			 *     `CodecCefDocument.Naming` and `CodecCefDocument.FieldByFullName`
			 *
			 * @li **A repeating key gives an array.** The second declaration of the same
			 *     key turns the value into an array and appends to its end; the walk remains
			 *     closed by the numeric links of the path - both on reading and on WRITING.
			 *     The count of pairs is kept by values, not by keys. Fixed by
			 *     `CodecCefDocument.SetThroughArrayIndex` and
			 *     `CodecCefDocument.SizeCountsPairsNotKeys`
			 *
			 * @li **Resetting a value and erasing a pair are DIFFERENT.** A reset leaves
			 *     the key with an empty value («cs3=»), an erasure removes the pair from the
			 *     record entirely. The distinction between "empty" and "absent" is not
			 *     carried by the CEF record itself, so a reset field is indistinguishable from
			 *     a written empty one - and that is a boundary of the format, named outright.
			 *     Fixed by `CodecCefDocument.ResetAgainstErase`
			 *
			 * @li **A directory fed in place of a file is refused by the code
			 *     `FILE_NOT_READ`, not `FILE_NOT_OPENED`.** A directory on POSIX opens
			 *     SUCCESSFULLY and is read with the end-of-file markers - the same as an empty
			 *     file answers - so before the guard its feeding answered TRUE with an empty
			 *     tree. The code is named after the substance of the trouble: "failed to open"
			 *     would send the consumer looking for access rights or a missing path, when in
			 *     fact a non-file was fed. The directory is recognised BEFORE the read by
			 *     `fs_t::type` - under MS Windows it does not open at all, and a check after
			 *     the open would be dead there. Fixed by
			 *     `CodecContract.DirectoryFedInsteadOfAFileIsRefused`, where the code is
			 *     compared numerically across eight codecs at once
			 *
			 * @li **A symbolic link is FOLLOWED, not distinguished from its target.** The
			 *     query of the kind of the object is conducted by `fs_t::type(addr, false)`:
			 *     a link to a file is read exactly as a file, and a link to a directory is
			 *     refused exactly as a directory. The query by default DISTINGUISHES the link,
			 *     answering `LINK`, and the first edition of the guard refused any link by the
			 *     code `FILE_NOT_OPENED`, when the file system tool could read it without a
			 *     single complaint - the refusal was invented by the codec, not by the system.
			 *     Fixed by `CodecCefDocument.SymlinkIsReadAsFile` and
			 *     `CodecCefDocument.SymlinkToDirectoryIsRefused`
			 *
			 * @li **The save is INDIVISIBLE: the writing goes to a temporary file that
			 *     replaces the target by renaming.** The target therefore either stays the
			 *     old one in full, or becomes the new one in full, and a half of it is never
			 *     seen by anyone. The order is: write to a temporary file next to the target,
			 *     flush it to the storage, replace by `fs_t::replaceAddress`; any failure on
			 *     the way removes the temporary file and does not touch the target. Fixed by
			 *     `CodecCefDocument.FailedSaveKeepsThePreviousRecord` and
			 *     `CodecCefDocument.SaveOverLongerFile`
			 *
			 * @li **The temporary file is created solely for the current save.** A taken
			 *     name is skipped without removing the file; the write and the flush share
			 *     one open object. Under POSIX the access bits of the existing target are
			 *     preserved, the new target gets the prior 0644 with the umask taken into
			 *     account. Ownership and ACL are not transferred. Fixed by
			 *     `CodecCefDocument.SavePreservesAccessBits`,
			 *     `CodecCefDocument.SaveKeepsUnrelatedSibling` and
			 *     `CodecCefDocument.TemporaryCollisionKeepsExistingFile`
			 *
			 * @warning EDITED 2026-09-16, and the prior decision was WRONG. It said:
			 *          "the save removes the previous file BEFORE the writing", because
			 *          `fs_t::write` does not truncate the file, but writes by offset, and
			 *          without the removal the tail of the previous save would stick out
			 *          past the end of the new one. The argument about the tail is still
			 *          true - what was wrong was the CONCLUSION from it: the removal solved
			 *          the task of truncation at the cost of the consumer's data. A
			 *          failure of the writing after the removal left NOTHING. Proven by an
			 *          experiment on a separate volume of two megabytes: the volume is
			 *          stuffed to the brim, the previous file is 69 bytes, the new write is
			 *          200 000 bytes - the save honestly answered with a falsehood and the
			 *          code `FILE_NOT_OPENED`, while the file turned into ZERO bytes. The
			 *          temporary file solves both tasks at once: no truncation is needed at
			 *          all, because the file is created empty, and the target is whole
			 *          until the very last moment
			 *
			 * @note The temporary file is placed NEXT to the target, not in the directory
			 *       of temporaries: that one may sit on a different file system, and the
			 *       rename would turn into a copy with a removal, silently losing the
			 *       indivisibility. The codecs CSV, TOML, XML and ABC have been using
			 *       this approach for a long time - here it was simply not applied
			 *
			 * @li **The writing is followed by the flush to the storage, and the failure
			 *     of that flush does not cancel it.** A write that answered with success still
			 *     sits in the buffer of the kernel, and a power loss loses it; that is exactly
			 *     what makes the save of a journal event worth doing - it survives a crash of
			 *     the machine. There is no `fsync` of our own on purpose: under macOS it does
			 *     NOT promise durability at all, only `F_FULLFSYNC` reaches the platter, and
			 *     the `fs_t::flush` method knows that. A failure of the flush goes to the
			 *     log as a warning, not as a failure of the save: what was written stays in
			 *     place and is read
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				public:
					/**
					 * \~russian
					 * @brief Структура настроек дерева разметки
					 *
					 * @details Настройки разбора и записи держатся раздельно: прочитать разметку
					 *          с одними пределами и записать её иным видом - обыкновенное дело, и
					 *          связывать их незачем.
					 *
					 * @note Построение это заведено ради согласия кодеков рамки: у всех у них
					 *       настройки дерева спрашиваются и ставятся одинаково, а вид их свой
					 *       у каждого кодека
					 *
					 * @warning Настройки разбора, поданные ходу `parse()` доводом, действуют
					 *          лишь на этот заход и хранимых настроек НЕ меняют
					 *
					 * \~english
					 * @brief Markup tree settings structure
					 *
					 * @details The settings of the parsing and of the writing are kept separately: to read
					 *          the markup with one limits and to write it in another form is an ordinary
					 *          affair, and there is no sense in binding them.
					 *
					 * @note This construction is made for the sake of the consent of the codecs of the framework:
					 *       at all of them the settings of a tree are asked and set in the same way, while their
					 *       form is own to each codec
					 *
					 * @warning The settings of the parsing fed to the `parse()` step as an argument act only on
					 *          this going and do NOT change the stored settings
					 *
					 * \~
					 */
					typedef struct Settings {
						// Настройки записи текста разметки
						writer_t::settings_t writer;
						// Настройки разбора текста разметки
						reader_t::settings_t reader;
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Объект работы с файловой системой
					 *
					 * @details Держится ПОЛЕМ, а не заводится в теле хода: так устроены и
					 *          прочие опоры документа - чтение, запись, дата, - и так же поступают
					 *          прочие модули рамки, «awh::args» в их числе.
					 *
					 * \~english
					 * @brief Filesystem object
					 *
					 * @details It persists as a FIELD rather than being embedded within
					 *          the execution flow; this is how other document supports—such
					 *          as read, write, and date—are structured, and other framework
					 *          modules, including "awh::args," operate in the same way.
					 *
					 * \~
					 */
					fs_t _fs;
				private:
					// Объект потокового чтения записей
					reader_t _reader;
					// Объект записи событий
					writer_t _writer;
				private:
					/**
					 * \~russian
					 * Код ошибки последней операции
					 *
					 * @note Поле изменяемое: ход сохранения объявлен константным, а
					 *       причину отказа называть обязан - прежде он отвечал ложью
					 *       БЕЗ кода вовсе, и `error()` нёс что угодно от прежних
					 *       операций. Образец взят у кодека YAML
					 *
					 * \~english
					 * Error code of the last operation
					 *
					 * @note The field is now mutable: the save operation is declared `const`,
					 *       yet it is required to report the reason for failure—previously,
					 *       it would return a false result with *no* error code at all,
					 *       and `error()` would contain garbage left over from earlier operations.
					 *       This pattern was adopted from the YAML codec
					 *
					 * \~
					 */
					mutable error_t _error;
				private:
					// Объект работы с адресами сети
					mutable net_addr_t _net;
					// Объект работы с датой и временем
					mutable chrono_t _chrono;
				private:
					// Дерево разобранного события контейнером ABC
					abc::value_t _root;
					/**
					 * \~russian
					 * @brief Кеш ссылки на вместилище пар расширения
					 *
					 * @details Заводится лениво при первом обращении к `"/" + EXTENSION`,
					 *          обнуляется в начале каждого `parse()`. Ссылка валидна до конца
					 *          `parse()` - `reserve(3)` не даёт `_root._items` перераспределиться.
					 *
					 * \~english
					 * @brief Cached reference to the extension pairs container
					 *
					 * @details Initialized lazily on the first access to `"/" + EXTENSION`,
					 *          reset at the start of each `parse()` call. The reference remains
					 *          valid until the end of `parse()` - `reserve(3)` prevents
					 *          `_root._items` from being reallocated.
					 *
					 * \~
					 */
					abc::value_t * _extension;
				private:
					/**
					 * \~russian
					 * @brief Метод укладки пары расширения в дерево события
					 *
					 * @details Повтор ключа обращает значение в перечень и добавляет в
					 *          его конец; вид значения берётся из словаря по строгости сличения.
					 *
					 * @param key   имя ключа пары расширения
					 * @param value значение пары расширения
					 * @return      признак успешности укладки пары
					 *
					 * \~english
					 * @brief Method of the placing of a pair of an extension into the tree of an event
					 *
					 * @details Repeating a key converts the value into a list and appends the
					 *          new item to the end; the value type is determined based on strict comparison rules.
					 *
					 * @param key   name of the key of the pair of the extension
					 * @param value value of the pair of the extension
					 * @return      flag of the success of the placing of the pair
					 *
					 * \~
					 */
					bool inject(const string & key, const string & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения значения расширения в значение дерева
					 *
					 * @details Вид берётся из словаря по ключу, а не угадывается по виду
					 *          знаков. Строгость сличения задаётся настройками разбора.
					 *
					 * @param entry  запись словаря расширений либо ничто
					 * @param value  значение пары расширения знаками
					 * @param result значение дерева контейнера ABC
					 * @return       признак успешности обращения значения
					 *
					 * \~english
					 * @brief Method of the conversion of a value of an extension into a value of the tree
					 *
					 * @details The type is retrieved from the dictionary using a key, rather than being
					 *          inferred from the appearance of the characters. The strictness of
					 *          the matching process is determined by the parsing settings.
					 *
					 * @param entry  record of the dictionary of the extensions or nothing
					 * @param value  value of the pair of the extension as characters
					 * @param result value of the tree of the ABC container
					 * @return       flag of the success of the conversion of the value
					 *
					 * \~
					 */
					bool convert(const entry_t * entry, const string & value, abc::value_t & result, const mode_t mode) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора записи CEF
					 *
					 * @details Разбирается ПЕРВАЯ запись поданного текста; записи, за нею
					 *          следующие, отбрасываются. Поток из многих записей разбирается
					 *          потоковым чтением, а не событием, удерживаемым целиком.
					 *
					 * @param text текст записи CEF
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the parsing of a CEF record
					 *
					 * @details The *first* record of the submitted text is parsed; subsequent
					 *          records are discarded. A stream containing multiple records is
					 *          processed via stream reading rather than as a single event held in its entirety.
					 *
					 * @param text text of a CEF record
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool parse(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения записи CEF из файла
					 *
					 * @param filename адрес файла записи CEF
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the reading of a CEF record from a file
					 *
					 * @param filename address of the file of the CEF record
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи события в файл
					 *
					 * @param filename адрес файла записи CEF
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the writing of an event into a file
					 *
					 * @param filename address of the file of the CEF record
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сбора записи CEF из дерева события
					 *
					 * @return собранная запись CEF
					 *
					 * \~english
					 * @brief Method of the assembly of a CEF record from the tree of an event
					 *
					 * @return assembled CEF record
					 *
					 * \~
					 */
					string dump() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения значения дерева по пути
					 *
					 * @details Звенья пути разделяются косой чертой; имя, косую черту
					 *          несущее, записывается отменяющей записью «~1» по RFC 6901.
					 *
					 * @param path путь к значению
					 * @return     ссылка на значение либо ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a value of the tree by a path
					 *
					 * @details Path segments are separated by a forward slash; the name
					 *          containing the forward slash is encoded using the escape
					 *          sequence "~1" as per RFC 6901.
					 *
					 * @param path path to the value
					 * @return     reference to the value or a reference to an absent value
					 *
					 * \~
					 */
					const abc::value_t & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки значения дерева по пути
					 *
					 * @details Недостающие звенья пути заводятся по дороге.
					 *
					 * @param path  путь к значению
					 * @param value значение, по пути ставимое
					 * @return      признак успешности постановки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value of the tree by a path
					 *
					 * @details The missing links of the track are installed as the work progresses.
					 *
					 * @param path  path to the value
					 * @param value value being set by the path
					 * @return      flag of the success of the setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const abc::value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса значения дерева по пути
					 *
					 * @details Узел остаётся на месте, а содержимое его замещается пустой
					 *          последовательностью знаков: запись «cs3=» есть законная запись
					 *          живых журналов. Снос же пары целиком совершается ходом `erase`.
					 *
					 * @param path путь к сбрасываемому значению
					 * @return     признак успешности сброса значения
					 *
					 * \~english
					 * @brief Method of the resetting of a value of the tree by a path
					 *
					 * @details The node remains in place, but its content is replaced by an empty
					 *          sequence of characters: the entry "cs3=" is a valid LiveJournal entry.
					 *          Removing the pair entirely is accomplished by the `erase` operation.
					 *
					 * @param path path to the value being reset
					 * @return     flag of the success of the resetting of the value
					 *
					 * \~
					 */
					bool reset(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод сноса значения дерева по пути
					 *
					 * @param path путь к сносимому значению
					 * @return     признак успешности сноса значения
					 *
					 * \~english
					 * @brief Method of the erasing of a value of the tree by a path
					 *
					 * @param path path to the value being erased
					 * @return     flag of the success of the erasing of the value
					 *
					 * \~
					 */
					bool erase(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения звеньев пути, у значения объявленных
					 *
					 * @details Выдаются ЗВЕНЬЯ ПУТИ, а не имена: обращение
					 *          `at(путь + "/" + звено)` ведёт к тому самому потомку. У перечня
					 *          звенья числовые. Отказом ход не отвечает никогда: и у листа, и у
					 *          пути несуществующего выдаётся пустой перечень, а различает их
					 *          ход `has`.
					 *
					 * @param path путь к значению
					 * @return     звенья пути, у значения объявленные
					 *
					 * \~english
					 * @brief Method of extracting the links of a path declared by a value
					 *
					 * @details The LINKS OF A PATH are issued rather than the names: the call
					 *          `at(path + "/" + link)` leads to that very child. For an array,
					 *          the links are numeric. The method never answers with a refusal:
					 *          both for a leaf and for a non-existent path, an empty list is
					 *          issued, while the distinction between them is made by `has`.
					 *
					 * @param path path to the value
					 * @return     links of the path declared by the value
					 *
					 * \~
					 */
					vector <string> keys(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия значения по пути
					 *
					 * @param path путь к значению
					 * @return     признак наличия значения по пути
					 *
					 * \~english
					 * @brief Method of the checking of the presence of a value by a path
					 *
					 * @param path path to the value
					 * @return     flag of the presence of a value by the path
					 *
					 * \~
					 */
					bool has(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия вложенного значения по имени
					 *
					 * @param path путь к значению
					 * @param name имя вложенного значения
					 * @return     признак наличия вложенного значения
					 *
					 * \~english
					 * @brief Method of the checking of the presence of a nested value by a name
					 *
					 * @param path path to the value
					 * @param name name of the nested value
					 * @return     flag of the presence of the nested value
					 *
					 * \~
					 */
					bool contains(const string & path, const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества пар расширения события
					 *
					 * @return количество пар расширения события
					 *
					 * \~english
					 * @brief Method of getting the number of the pairs of the extension of an event
					 *
					 * @return number of the pairs of the extension of the event
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки дерева события
					 *
					 * \~english
					 * @brief Method of the clearing of the tree of an event
					 *
					 * \~
					 */
					void clear() noexcept;
					/**
					 * \~russian
					 * @brief Псевдоним сброса состояния документа (см. clear())
					 *
					 * \~english
					 * @brief Alias of the reset of the state of a document (see clear())
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения значения расширения по ПОЛНОМУ имени ключа
					 *
					 * @details Ход этот и есть второй ход к сведённому именованию: обход
					 *          по пути выдаёт ключи сырыми, а полное имя словаря разыскивается им.
					 *
					 * @param name полное имя ключа расширения
					 * @return     ссылка на значение либо ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a value of an extension by the FULL name of a key
					 *
					 * @details This step constitutes the second approach to the fully qualified name:
					 *          the traversal yields the keys in their raw form, while the dictionary's
					 *          full name is resolved during the process.
					 *
					 * @param name full name of the key of the extension
					 * @return     reference to the value or a reference to an absent value
					 *
					 * \~
					 */
					const abc::value_t & field(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения человеческого имени ключа расширения
					 *
					 * @details Имя берётся из метки самой записи - пара «cs1Label=IDSClass»
					 *          задаёт имя ключу «cs1», - а при отсутствии метки из словаря
					 *          расширений. Если же ключ словарю неизвестен и метки не имеет, имя
					 *          выдаётся пустым: выдумывать его кодек не станет.
					 *
					 * @param key имя ключа расширения, в записи стоящее
					 * @return    человеческое имя ключа расширения
					 *
					 * \~english
					 * @brief Method of getting the human-readable name of an extension key
					 *
					 * @details The name is drawn from a label of the record itself - the pair
					 *          "cs1Label=IDSClass" assigns the name "cs1" to the key - and when
					 *          no label is present, from the dictionary of the extensions. If the
					 *          key is unknown to the dictionary and carries no label, the name
					 *          is issued as empty: the codec does not invent one.
					 *
					 * @param key name of the key of the extension standing in the record
					 * @return    human-readable name of the key of the extension
					 *
					 * \~
					 */
					string label(const string & key) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения метки времени записью заданного вида
					 *
					 * @details Метка времени держится деревом штампом в миллисекундах от
					 *          начала эпохи, а ходом этим выдаётся записью знаков любого вида,
					 *          какой примет модуль `chrono_t`. Ход этот и есть замена
					 *          установщику записи даты прежнего модуля: тот держал запись
					 *          настройкой объекта и менял выдачу ВСЕХ меток разом, здесь же
					 *          запись задаётся при выдаче, и два потребителя одного события
					 *          могут получить метку каждый в своей записи.
					 *
					 * @note Метка выдаётся в МЕСТНОЙ зоне машины, как её выдавал и
					 *       прежний модуль: зона, самой записью объявленная, при разборе уже
					 *       учтена, и штамп её не помнит. Кому нужна иная зона, тот берёт
					 *       штамп ходом `at` и зовёт `chrono_t::format` с зоною сам
					 *
					 * @param key    имя ключа расширения, метку времени несущего
					 * @param format запись даты, выдаваемой метке назначаемая
					 * @return       метка времени записью заданного вида
					 *
					 * \~english
					 * @brief Method of getting a timestamp in a notation of the given kind
					 *
					 * @details The timestamp is maintained internally as a millisecond value relative
					 *          to the epoch, while its output format is determined by the specific
					 *          representation accepted by the `chrono_t` module. This mechanism replaces
					 *          the date-formatting configuration of the previous module: the old approach
					 *          set the format via an object configuration—affecting the output of *all*
					 *          timestamps simultaneously—whereas here, the format is specified at the
					 *          point of output, allowing two consumers of the same event to receive the
					 *          timestamp in their own respective formats.
					 *
					 * @note The zone is assigned within the machine's *local* zone, just as the previous module did:
					 *       the zone explicitly declared in the record itself has already been accounted for during parsing,
					 *       and the timestamp does not store it. If a different zone is required, one retrieves the timestamp
					 *       via the `at` method and calls `chrono_t::format` with the desired zone
					 *
					 * @param key    name of the key of the extension carrying the timestamp
					 * @param format notation of the date appointed to the issued timestamp
					 * @return       timestamp in a notation of the given kind
					 *
					 * \~
					 */
					string timestamp(const string & key, const string & format = string(TIMESTAMP_FORMAT)) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида значения ключа расширения
					 *
					 * @param key имя ключа расширения, в записи стоящее
					 * @return    вид значения, словарём заданный
					 *
					 * \~english
					 * @brief Method of getting the kind of the value of a key of an extension
					 *
					 * @param key name of the key of the extension standing in the record
					 * @return    kind of the value given by the dictionary
					 *
					 * \~
					 */
					type_t type(const string & key) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения общего вида значения ключа расширения
					 *
					 * @param key имя ключа расширения, в записи стоящее
					 * @return    общий вид значения, словарём заданный
					 *
					 * \~english
					 * @brief Method for obtaining the general form of the extension key value
					 *
					 * @param key the general form of the meaning as defined by the dictionary
					 * @return    kind of the value given by the dictionary
					 *
					 * \~
					 */
					codec::kind_t kind(const string & key) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения дерева события целиком
					 *
					 * @return дерево разобранного события контейнером ABC
					 *
					 * \~english
					 * @brief Method of getting the tree of an event in full
					 *
					 * @return tree of the parsed event as an ABC container
					 *
					 * \~
					 */
					const abc::value_t & root() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки последней операции
					 *
					 * @return код ошибки последней операции
					 *
					 * \~english
					 * @brief Method of getting the error code of the last operation
					 *
					 * @return error code of the last operation
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения ошибки разбора
					 *
					 * @return положение обнаруженной ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error of the parsing
					 *
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const pos_t & errorLocation() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора записей
					 *
					 * @param settings настройки разбора записей
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing of the records
					 *
					 * @param settings settings of the parsing of the records
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Разрешаем оператор копирования
					 *
					 * \~english
					 * @brief Enable the copy operator
					 *
					 * \~
					 */
					Document & operator = (const Document &) = default;
					/**
					 * \~russian
					 * @brief Разрешаем оператор перемещения
					 *
					 * \~english
					 * @brief Enable the move operator
					 *
					 * \~
					 */
					Document & operator = (Document &&) noexcept = default;
				public:
					/**
					 * \~russian
					 * @brief Разрешаем конструктор копирования
					 *
					 * \~english
					 * @brief Enable the copy constructor
					 *
					 * \~
					 */
					Document(const Document &) = default;
					/**
					 * \~russian
					 * @brief Разрешаем конструктор перемещения
					 *
					 * \~english
					 * @brief Enable the move constructor
					 *
					 * \~
					 */
					Document(Document &&) noexcept = default;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Document() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки разбора документа
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param settings settings of the parsing of a document
					 *
					 * \~
					 */
					explicit Document(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Document() noexcept = default;
			} document_t;
		};
	};
};
