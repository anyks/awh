/**
 * @file document.hpp
 * @date 2026-09-07
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
 * @brief Заголовочный файл сообщения системного журнала, удерживаемого целиком
 *
 * \~english
 * @brief Header file of a system log message held in full
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
#include "../abc/value.hpp"
#include "../../sys/fs.hpp"
#include "../../sys/chrono.hpp"
#include "../../sys/macro/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include "../../sys/macro/suppress.hpp"

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
		 * @brief Пространство имён контейнера SysLog
		 *
		 * \~english
		 * @brief SysLog container namespace
		 *
		 * \~
		 */
		namespace syslog {
			/**
			 * \~russian
			 * @brief Класс сообщения системного журнала, удерживаемого целиком
			 *
			 * @details Держит разобранную запись деревом контейнера ABC и выдаёт её
			 *          ходами по пути, общими у всех кодеков библиотеки. Устройство дерева
			 *          отвечает устройству самой записи:
			 *
			 * @li `/standard` - описание, каким запись прочтена, знаками `RFC3164`
			 *     либо `RFC5424`;
			 *
			 * @li `/priority` - приоритет записи целым числом, если он объявлен;
			 *
			 * @li `/facility` - краткое имя источника сообщения знаками;
			 *
			 * @li `/severity` - краткое имя степени важности сообщения знаками;
			 *
			 * @li `/header` - поля заголовка отображением с именами `version`,
			 *     `timestamp`, `hostname`, `application`, `process`, `messageId`;
			 *
			 * @li `/structures` - блоки структурированных данных отображением
			 *     отображений: `/structures/<опознаватель>/<имя поля>`;
			 *
			 * @li `/message` - свободный текст сообщения знаками
			 *
			 * @par Намеренные решения
			 *      Перечисленное ниже не является пробелом реализации: это очерченные границы
			 *      задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Своего владеющего значения (`value_t`) у кодека НЕТ, и это решение,
			 *     а не пробел.** Основанием записи служит дерево контейнера ABC: система
			 *     видов его вмещает виды записи системного журнала с запасом, а ходы `at`,
			 *     `place`, `contains`, `erase` даны им уже. Заводить поверх второй
			 *     владеющий вид значило бы держать один договор в двух местах и переводить
			 *     дерево в дерево на всяком обращении
			 *
			 * @li **Источник и важность держатся ИМЕНАМИ, а приоритет - числом.**
			 *     Приоритет есть источник, восьмикратно взятый, и важность: держать одно и
			 *     то же дважды числом значило бы завести два места, расходящиеся при
			 *     правке одного. Имена же держатся затем, что по ним пишутся правила
			 *     отбора у служб журналов, и разыскивать их в словаре при всяком обращении
			 *     было бы работой впустую. Закреплено
			 *     `CodecSysLogDocument.PriorityIsNotInvented` и `CodecSysLogReader.Priority`
			 *
			 * @li **Приставка приоритета необязательна.** Запись без неё разбирается
			 *     успешно, и поля `/priority`, `/facility`, `/severity` в дереве просто
			 *     отсутствуют. Отвергать такие записи значило бы терять события живых
			 *     устройств. Закреплено
			 *     `CodecSysLogReader.MissingPriority`
			 *
			 * @li **Повтор имени поля блока даёт перечень.** Описание дозволяет его
			 *     прямо - «An SD-PARAM MAY be repeated multiple times inside an SD-ELEMENT»
			 *     (RFC 5424, раздел 6.3.3), - и второе объявление того же имени обращает
			 *     значение в перечень, а запись выражает его повтором имени. Обход остаётся
			 *     замкнутым числовыми звеньями пути. Повтор ОПОЗНАВАТЕЛЯ блока описание, в
			 *     отличие от того, запрещает - «The same SD-ID MUST NOT exist more than once
			 *     in a message» (раздел 6.3.2), - и при вольном сличении запись берётся как
			 *     есть, оставляя поля последнего блока. Закреплено
			 *     `CodecSysLogDocument.DuplicateNames` и
			 *     `CodecSysLogWriter.RepeatedParamRoundTrip`
			 *
			 * @li **Глубина пути ограничена устройством записи.** Путь длиннее трёх
			 *     звеньев - `/structures/<опознаватель>/<имя поля>` - записи системного
			 *     журнала неведом. Это граница описания, а не недоделка обхода. Закреплено
			 *     `CodecSysLogDocument.PathDepthIsBoundedByTheFormat`
			 *
			 * @li **Дословного совпадения записи оборот не обещает - обещает
			 *     значение.** Дата выдаётся видом, описанию отвечающим, порядок блоков
			 *     берётся порядком дерева. Обратимость закрепляется сличением ДЕРЕВЬЕВ, а
			 *     не текстов. Закреплено
			 *     `CodecSysLogWriter.DateIsNeverRewritten` и `CodecSysLogDocument.Timestamps`
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
			 *     кодеком, а не системой. Закреплено `CodecSysLogDocument.SymlinkIsReadAsFile`
			 *     и `CodecSysLogDocument.SymlinkToDirectoryIsRefused`
			 *
			 * @li **Сохранение НЕДЕЛИМО: запись идёт во временный файл, подменяющий
			 *     цель переименованием.** Цель оттого либо остаётся прежней целиком, либо
			 *     становится новой целиком, а половины её не видно никогда. Порядок таков:
			 *     запись во временный файл рядом с целью, сброс его на носитель, подмена
			 *     ходом `fs_t::replaceAddress`; всякий отказ по дороге сносит временный файл
			 *     и цели не касается. Закреплено `CodecSysLogDocument.FailedSaveKeepsThePreviousRecord`
			 *     и `CodecSysLogDocument.SaveOverLongerFile`
			 *
			 * @li **Временный файл создаётся исключительно для текущего сохранения.**
			 *     Занятое имя пропускается без удаления файла; запись и сброс используют один
			 *     открытый объект. Под POSIX сохраняются биты доступа существующей цели,
			 *     новая цель получает прежние 0644 с учётом umask. Владение и ACL не переносятся.
			 *     Закреплено `CodecSysLogDocument.SavePreservesAccessBits`,
			 *     `CodecSysLogDocument.SaveKeepsUnrelatedSibling` и
			 *     `CodecSysLogDocument.TemporaryCollisionKeepsExistingFile`
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
			 * @li **Спрос у пустого дерева отвечает неопределённостью, а не гибелью.**
			 *     Описание отвечается `AUTO`, приоритет нулём, дата пустой строкой, число
			 *     блоков нулём. Потребитель волен спросить раньше чтения и обязан различить
			 *     «поля нет» по самому ответу. Закреплено
			 *     `CodecSysLogDocument.EmptyDocumentQueries`
			 *
			 * @li **Постановка значения вместилище звена ЗАВОДИТ, а снос - нет.** Путь
			 *     сквозь звено, картой не являющееся, постановке не помеха: дерево по пути
			 *     достраивается, а прежнее знаковое значение теряется. Сносу же такой путь
			 *     есть отказ отсутствия поля: снимать нечего. Закреплено
			 *     `CodecSysLogDocument.PathEdges`
			 *
			 * \~english
			 * @brief Class of a system log message held in full
			 *
			 * @details Holds a parsed record as a tree of the ABC container and issues it
			 *          by the methods by a path, common to all the codecs of the library.
			 *          The shape of the tree answers the shape of the record itself:
			 *
			 * @li `/standard` - the description by which the record was read, as characters
			 *     `RFC3164` or `RFC5424`;
			 *
			 * @li `/priority` - the priority of the record as an integer, if it is declared;
			 *
			 * @li `/facility` - the short name of the source of the message as characters;
			 *
			 * @li `/severity` - the short name of the degree of importance of the message as characters;
			 *
			 * @li `/header` - the fields of the header as a mapping with the names `version`,
			 *     `timestamp`, `hostname`, `application`, `process`, `messageId`;
			 *
			 * @li `/structures` - the blocks of structured data as a mapping of mappings:
			 *     `/structures/<identifier>/<field name>`;
			 *
			 * @li `/message` - the free text of the message as characters
			 *
			 * @par Deliberate decisions
			 *      The list below is not a gap of the implementation: these are the outlined
			 *      boundaries of the task, and each decision is fixed by a check
			 *
			 * @li **The codec has NO owning value (`value_t`) of its own, and that is a
			 *     decision, not a gap.** The record is built upon the tree of the ABC container:
			 *     its system of kinds covers the kinds of the system log record with margin, and
			 *     the methods `at`, `place`, `contains`, `erase` are already given by it. To layer
			 *     a second owning type on top would mean keeping one contract in two places and
			 *     translating tree into tree on every access
			 *
			 * @li **The source and the severity are held by NAMES, and the priority - by a number.**
			 *     The priority is the source taken eight times, and the severity: to hold the same
			 *     thing twice by a number would mean introducing two places that diverge on editing
			 *     one of them. The names are held because filtering rules at journal services are
			 *     written by them, and looking them up in the dictionary on every access would be
			 *     wasted work. Fixed by
			 *     `CodecSysLogDocument.PriorityIsNotInvented` and `CodecSysLogReader.Priority`
			 *
			 * @li **The priority prefix is optional.** A record without it is parsed successfully,
			 *     and the fields `/priority`, `/facility`, `/severity` are simply absent in the tree.
			 *     To refuse such records would mean losing events of live devices. Fixed by
			 *     `CodecSysLogReader.MissingPriority`
			 *
			 * @li **A repeating name of a field of a block gives an array.** The specification allows
			 *     it directly - "An SD-PARAM MAY be repeated multiple times inside an SD-ELEMENT"
			 *     (RFC 5424, section 6.3.3) - and the second declaration of the same name turns the
			 *     value into an array, while the record expresses it by repeating the name. The walk
			 *     remains closed by the numeric links of the path. A repeating IDENTIFIER of a block
			 *     the specification, unlike that, forbids - "The same SD-ID MUST NOT exist more than
			 *     once in a message" (section 6.3.2) - and under a lenient comparison the record is
			 *     taken as is, keeping the fields of the last block. Fixed by
			 *     `CodecSysLogDocument.DuplicateNames` and
			 *     `CodecSysLogWriter.RepeatedParamRoundTrip`
			 *
			 * @li **The depth of a path is bounded by the shape of the record.** A path longer than
			 *     three links - `/structures/<identifier>/<field name>` - is unknown to a system log
			 *     record. This is a boundary of the specification, not a missing feature of the walk.
			 *     Fixed by `CodecSysLogDocument.PathDepthIsBoundedByTheFormat`
			 *
			 * @li **The round trip does not promise the literal match of the record - it promises
			 *     the value.** The date is issued by the appearance matching the specification, the
			 *     order of the blocks is taken by the order of the tree. The reversibility is fixed
			 *     by the comparison of TREES, not of texts. Fixed by
			 *     `CodecSysLogWriter.DateIsNeverRewritten` and `CodecSysLogDocument.Timestamps`
			 *
			 * @li **A directory fed in place of a file is refused by the code `FILE_NOT_READ`,
			 *     not `FILE_NOT_OPENED`.** A directory on POSIX opens SUCCESSFULLY and is read with
			 *     the end-of-file markers - the same as an empty file answers - so before the guard
			 *     its feeding answered TRUE with an empty tree. The code is named after the substance
			 *     of the trouble: "failed to open" would send the consumer looking for access rights
			 *     or a missing path, when in fact a non-file was fed. The directory is recognised
			 *     BEFORE the read by `fs_t::type` - under MS Windows it does not open at all, and a
			 *     check after the open would be dead there. Fixed by
			 *     `CodecContract.DirectoryFedInsteadOfAFileIsRefused`, where the code is compared
			 *     numerically across eight codecs at once
			 *
			 * @li **A symbolic link is FOLLOWED, not distinguished from its target.** The query of
			 *     the kind of the object is conducted by `fs_t::type(addr, false)`: a link to a file
			 *     is read exactly as a file, and a link to a directory is refused exactly as a
			 *     directory. The query by default DISTINGUISHES the link, answering `LINK`, and the
			 *     first edition of the guard refused any link by the code `FILE_NOT_OPENED`, when the
			 *     file system tool could read it without a single complaint - the refusal was invented
			 *     by the codec, not by the system. Fixed by `CodecSysLogDocument.SymlinkIsReadAsFile`
			 *     and `CodecSysLogDocument.SymlinkToDirectoryIsRefused`
			 *
			 * @li **The save is INDIVISIBLE: the writing goes to a temporary file that replaces the
			 *     target by renaming.** The target therefore either stays the old one in full, or
			 *     becomes the new one in full, and a half of it is never seen by anyone. The order
			 *     is: write to a temporary file next to the target, flush it to the storage, replace
			 *     by `fs_t::replaceAddress`; any failure on the way removes the temporary file and
			 *     does not touch the target. Fixed by
			 *     `CodecSysLogDocument.FailedSaveKeepsThePreviousRecord` and
			 *     `CodecSysLogDocument.SaveOverLongerFile`
			 *
			 * @li **The temporary file is created solely for the current save.** A taken name is
			 *     skipped without removing the file; the write and the flush share one open object.
			 *     Under POSIX the access bits of the existing target are preserved, the new target
			 *     gets the prior 0644 with the umask taken into account. Ownership and ACL are not
			 *     transferred. Fixed by `CodecSysLogDocument.SavePreservesAccessBits`,
			 *     `CodecSysLogDocument.SaveKeepsUnrelatedSibling` and
			 *     `CodecSysLogDocument.TemporaryCollisionKeepsExistingFile`
			 *
			 * @warning EDITED 2026-09-16, and the prior decision was WRONG. It said: "the save
			 *          removes the previous file BEFORE the writing", because `fs_t::write` does not
			 *          truncate the file, but writes by offset, and without the removal the tail of
			 *          the previous save would stick out past the end of the new one. The argument
			 *          about the tail is still true - what was wrong was the CONCLUSION from it: the
			 *          removal solved the task of truncation at the cost of the consumer's data. A
			 *          failure of the writing after the removal left NOTHING. Proven by an experiment
			 *          on a separate volume of two megabytes: the volume is stuffed to the brim, the
			 *          previous file is 69 bytes, the new write is 200 000 bytes - the save honestly
			 *          answered with a falsehood and the code `FILE_NOT_OPENED`, while the file turned
			 *          into ZERO bytes. The temporary file solves both tasks at once: no truncation is
			 *          needed at all, because the file is created empty, and the target is whole until
			 *          the very last moment
			 *
			 * @note The temporary file is placed NEXT to the target, not in the directory of
			 *       temporaries: that one may sit on a different file system, and the rename would
			 *       turn into a copy with a removal, silently losing the indivisibility. The codecs
			 *       CSV, TOML, XML and ABC have been using this approach for a long time - here it
			 *       was simply not applied
			 *
			 * @li **The writing is followed by the flush to the storage, and the failure of that
			 *     flush does not cancel it.** A write that answered with success still sits in the
			 *     buffer of the kernel, and a power loss loses it; that is exactly what makes the
			 *     save of a journal event worth doing - it survives a crash of the machine. There is
			 *     no `fsync` of our own on purpose: under macOS it does NOT promise durability at all,
			 *     only `F_FULLFSYNC` reaches the platter, and the `fs_t::flush` method knows that. A
			 *     failure of the flush goes to the log as a warning, not as a failure of the save:
			 *     what was written stays in place and is read
			 *
			 * @li **A query to an empty tree answers with an undefinedness, not with a death.**
			 *     The description is answered `AUTO`, the priority zero, the date an empty string,
			 *     the number of blocks zero. The consumer is free to query before the read and is
			 *     obliged to distinguish "the field is absent" by the answer itself. Fixed by
			 *     `CodecSysLogDocument.EmptyDocumentQueries`
			 *
			 * @li **Setting a value CREATES the container of a link, and erasing does not.** A path
			 *     through a link that is not a mapping is not an obstacle to the setting: the tree is
			 *     built along the path, while the previous character value is lost. For the erasure,
			 *     such a path is a refusal of the absence of the field: there is nothing to remove.
			 *     Fixed by `CodecSysLogDocument.PathEdges`
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
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
					 * @details It persists as a FIELD rather than being embedded within the
					 *          execution flow; this is how other document supports—such as read,
					 *          write, and date—are structured, and other framework modules,
					 *          including "awh::args," operate in the same way.
					 *
					 * \~
					 */
					fs_t _fs;
				private:
					// Дерево разобранной записи контейнером ABC
					abc::value_t _root;
					/**
					 * \~russian
					 * @brief Кеш ссылки на вместилище блоков структурированных данных
					 *
					 * @details Заводится лениво при первом обращении к `"/" + STRUCTURES`,
					 *          обнуляется в начале каждого `parse()`. Ссылка валидна до конца
					 *          `parse()` - `reserve(2)` не даёт `_root._items` перераспределиться.
					 *
					 * \~english
					 * @brief Cached reference to the structured data blocks container
					 *
					 * @details Initialized lazily on the first access to `"/" + STRUCTURES`,
					 *          reset at the start of each `parse()` call. The reference remains
					 *          valid until the end of `parse()` - `reserve(2)` prevents
					 *          `_root._items` from being reallocated.
					 *
					 * \~
					 */
					abc::value_t * _structures;
				private:
					/**
					 * \~russian
					 * @brief Код ошибки последней операции
					 *
					 * @note Поле изменяемое: ход сохранения объявлен константным, а
					 *       причину отказа называть обязан - прежде он отвечал ложью
					 *       БЕЗ кода вовсе, и `error()` нёс что угодно от прежних
					 *       операций. Образец взят у кодека YAML
					 *
					 * \~english
					 * @brief Code of the refusal of the last operation
					 *
					 * @note The field is mutable: the save operation is declared `const`,
					 *       yet it is required to report the reason for failure — previously,
					 *       it would return a false result with *no* error code at all, and
					 *       `error()` would contain garbage left over from earlier operations.
					 *       This pattern was adopted from the YAML codec
					 *
					 * \~
					 */
					mutable error_t _error;
				private:
					// Объект потокового чтения записей
					reader_t _reader;
					// Объект записи событий
					writer_t _writer;
				private:
					// Объект работы с датой и временем
					mutable chrono_t _chrono;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора записи системного журнала
					 *
					 * @details Разбирается ПЕРВАЯ запись поданного текста: событие
					 *          удерживается по одному, а поток записей ведётся чтением.
					 *
					 * @param text разбираемый текст записи
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the parsing of a system log record
					 *
					 * @details The FIRST record of the fed text is parsed: the event is held
					 *          one at a time, while a stream of records is conducted by reading.
					 *
					 * @param text parsed text of the record
					 * @return     result of the operation
					 *
					 * \~
					 */
					bool parse(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения записи системного журнала из файла
					 *
					 * @param filename адрес файла записи системного журнала
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the reading of a system log record from a file
					 *
					 * @param filename address of the file of the system log record
					 * @return         result of the operation
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи события в файл
					 *
					 * @param filename адрес файла записи системного журнала
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the writing of an event into a file
					 *
					 * @param filename address of the file of the system log record
					 * @return         result of the operation
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сбора записи системного журнала из дерева события
					 *
					 * @return собранная запись системного журнала
					 *
					 * \~english
					 * @brief Method of the assembly of a system log record from a tree of an event
					 *
					 * @return assembled system log record
					 *
					 * \~
					 */
					string dump() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения значения дерева по пути
					 *
					 * @param path путь к значению
					 * @return     ссылка на значение либо ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a value of the tree by a path
					 *
					 * @param path path to the value
					 * @return     reference to the value or reference to an absent value
					 *
					 * \~
					 */
					const abc::value_t & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки значения дерева по пути
					 *
					 * @param path  путь к значению
					 * @param value значение, по пути ставимое
					 * @return      признак успешности постановки значения
					 *
					 * \~english
					 * @brief Method of the placing of a value of the tree by a path
					 *
					 * @param path  path to the value
					 * @param value value placed by the path
					 * @return      flag of the success of the placing of the value
					 *
					 * \~
					 */
					bool set(const string & path, const abc::value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса значения дерева по пути
					 *
					 * @details Сброс оставляет поле с пустым значением, а снос убирает
					 *          его из записи вовсе.
					 *
					 * @param path путь к сбрасываемому значению
					 * @return     признак успешности сброса значения
					 *
					 * \~english
					 * @brief Method of the reset of a value of the tree by a path
					 *
					 * @details A reset leaves the field with an empty value, while an erasure
					 *          removes it from the record entirely.
					 *
					 * @param path path to the reset value
					 * @return     flag of the success of the reset of the value
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
					 * @brief Method of the removal of a value of the tree by a path
					 *
					 * @param path path to the removed value
					 * @return     flag of the success of the removal of the value
					 *
					 * \~
					 */
					bool erase(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения звеньев пути, у значения объявленных
					 *
					 * @param path путь к значению
					 * @return     звенья пути, у значения объявленные
					 *
					 * \~english
					 * @brief Method of the extraction of the links of a path declared by a value
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
					 * @brief Method of the check of the presence of a value by a path
					 *
					 * @param path path to the value
					 * @return     flag of the presence of the value by the path
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
					 * @brief Method of the check of the presence of a nested value by a name
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
					 * @brief Метод получения количества блоков структурированных данных
					 *
					 * @return количество блоков структурированных данных
					 *
					 * \~english
					 * @brief Method of getting the number of the blocks of the structured data
					 *
					 * @return number of the blocks of the structured data
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
					 * @brief Метод получения описания, каким прочтена запись
					 *
					 * @return описание, каким прочтена запись
					 *
					 * \~english
					 * @brief Method of getting the description by which the record is read
					 *
					 * @return description by which the record is read
					 *
					 * \~
					 */
					standard_t standard() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения приоритета записи
					 *
					 * @details При отсутствии приставки приоритета выдаётся ноль, а
					 *          объявленность его берётся ходом `prioritized`.
					 *
					 * @return приоритет записи
					 *
					 * \~english
					 * @brief Method of getting the priority of the record
					 *
					 * @details In the absence of a priority prefix, zero is issued, while the
					 *          declaredness of it is retrieved by the `prioritized` method.
					 *
					 * @return priority of the record
					 *
					 * \~
					 */
					uint32_t priority() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения признака объявленности приоритета
					 *
					 * @return признак того, что приоритет записью объявлен
					 *
					 * \~english
					 * @brief Method of getting the flag of the declaredness of the priority
					 *
					 * @return flag that the priority is declared by the record
					 *
					 * \~
					 */
					bool prioritized() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения источника сообщения
					 *
					 * @return источник сообщения
					 *
					 * \~english
					 * @brief Method of getting the source of the message
					 *
					 * @return source of the message
					 *
					 * \~
					 */
					facility_t facility() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения степени важности сообщения
					 *
					 * @return степень важности сообщения
					 *
					 * \~english
					 * @brief Method of getting the degree of the importance of the message
					 *
					 * @return degree of the importance of the message
					 *
					 * \~
					 */
					severity_t severity() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения человеческого названия источника сообщения
					 *
					 * @return человеческое название источника сообщения
					 *
					 * \~english
					 * @brief Method of getting the human title of the source of the message
					 *
					 * @return human title of the source of the message
					 *
					 * \~
					 */
					string label() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения даты сообщения заданным видом
					 *
					 * @details Дата держится деревом тем видом, каким стояла в записи, а
					 *          потребителю нужен свой: ход этот разбирает её и выдаёт заданным
					 *          видом. Дата, разбору не поддавшаяся, выдаётся как есть.
					 *
					 * @param format вид, каким надлежит выдать дату сообщения
					 * @return       дата сообщения заданным видом
					 *
					 * \~english
					 * @brief Method of getting the date of a message in a given appearance
					 *
					 * @details The date is held by the tree in the appearance in which it stood
					 *          in the record, while the consumer needs their own: this method parses
					 *          it and issues it in the given appearance. A date that did not yield to
					 *          the parsing is issued as is.
					 *
					 * @param format appearance the date of the message is to be issued in
					 * @return       date of the message in the given appearance
					 *
					 * \~
					 */
					string timestamp(const string & format = string("%Y-%m-%dT%H:%M:%S")) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения времени сообщения числом
					 *
					 * @details Время выдаётся тысячными долями секунды от начала эпохи;
					 *          дата, разбору не поддавшаяся, выдаётся нулём.
					 *
					 * @return время сообщения тысячными долями секунды
					 *
					 * \~english
					 * @brief Method of getting the time of a message as a number
					 *
					 * @details The time is issued in the thousandths of a second from the beginning
					 *          of the epoch; a date that did not yield to the parsing is issued as zero.
					 *
					 * @return time of the message in the thousandths of a second
					 *
					 * \~
					 */
					uint64_t time() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения дерева разобранной записи
					 *
					 * @return дерево разобранной записи контейнером ABC
					 *
					 * \~english
					 * @brief Method of getting the tree of the parsed record
					 *
					 * @return tree of the parsed record as an ABC container
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
					 * @brief Метод получения положения обнаруженной ошибки
					 *
					 * @return положение обнаруженной ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of the discovered error
					 *
					 * @return position of the discovered error in the source text
					 *
					 * \~
					 */
					const pos_t & errorLocation() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек разбора записей
					 *
					 * @return настройки разбора записей
					 *
					 * \~english
					 * @brief Method of getting the settings of the parsing of the records
					 *
					 * @return settings of the parsing of the records
					 *
					 * \~
					 */
					const reader_t::settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора записей
					 *
					 * @details Настройка снятия отмены знаков у чтения СВОДИТСЯ с
					 *          настройкой постановки её у записи: порознь их задавать нельзя -
					 *          постановка отмены поверх неснятой наращивает косые при всяком
					 *          обороте, - оттого пару эту сводит документ, а не потребитель.
					 *
					 * @param settings настройки разбора записей
					 * @return         признак успешной установки настроек
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of the records
					 *
					 * @details The setting of the removal of the escape of characters at the
					 *          reading is MERGED with the setting of its imposition at the writing:
					 *          they cannot be given separately - imposing an escape on top of an
					 *          unremoved one multiplies the slashes on every turn - and therefore
					 *          this pair is merged by the document, not by the consumer.
					 *
					 * @param settings settings of the parsing of the records
					 * @return         flag of the successful setting of the settings
					 *
					 * \~
					 */
					bool settings(const reader_t::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи событий
					 *
					 * @param settings настройки записи событий
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the writing of the events
					 *
					 * @param settings settings of the writing of the events
					 *
					 * \~
					 */
					void settings(const writer_t::settings_t & settings) noexcept;
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

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"
