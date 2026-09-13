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
#ifndef __AWH_CODEC_SYSLOG_DOCUMENT__
#define __AWH_CODEC_SYSLOG_DOCUMENT__

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
#include "../../sys/fs.hpp"
#include "../../sys/chrono.hpp"
#include "../abc/value.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include "../../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
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
			 * ходами по пути, общими у всех кодеков библиотеки. Устройство дерева
			 * отвечает устройству самой записи:
			 *
			 * @li `/standard` - описание, каким запись прочтена, знаками `RFC3164`
			 *     либо `RFC5424`;
			 * @li `/priority` - приоритет записи целым числом, если он объявлен;
			 * @li `/facility` - краткое имя источника сообщения знаками;
			 * @li `/severity` - краткое имя степени важности сообщения знаками;
			 * @li `/header` - поля заголовка отображением с именами `version`,
			 *     `timestamp`, `hostname`, `application`, `process`, `messageId`;
			 * @li `/structures` - блоки структурированных данных отображением
			 *     отображений: `/structures/<опознаватель>/<имя поля>`;
			 * @li `/message` - свободный текст сообщения знаками
			 *
			 * @par Намеренные решения
			 *
			 * Перечисленное ниже не является пробелом реализации: это очерченные границы
			 * задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Своего владеющего значения (`value_t`) у кодека НЕТ, и это решение,
			 * а не пробел.** Основанием записи служит дерево контейнера ABC: система
			 * видов его вмещает виды записи системного журнала с запасом, а ходы `at`,
			 * `place`, `contains`, `erase` даны им уже. Заводить поверх второй
			 * владеющий вид значило бы держать один договор в двух местах и переводить
			 * дерево в дерево на всяком обращении
			 *
			 * @li **Источник и важность держатся ИМЕНАМИ, а приоритет - числом.**
			 * Приоритет есть источник, восьмикратно взятый, и важность: держать одно и
			 * то же дважды числом значило бы завести два места, расходящиеся при
			 * правке одного. Имена же держатся затем, что по ним пишутся правила
			 * отбора у служб журналов, и разыскивать их в словаре при всяком обращении
			 * было бы работой впустую
			 *
			 * @li **Приставка приоритета необязательна.** Запись без неё разбирается
			 * успешно, и поля `/priority`, `/facility`, `/severity` в дереве просто
			 * отсутствуют. Отвергать такие записи значило бы терять события живых
			 * устройств
			 *
			 * @li **Глубина пути ограничена устройством записи.** Путь длиннее трёх
			 * звеньев - `/structures/<опознаватель>/<имя поля>` - записи системного
			 * журнала неведом. Это граница описания, а не недоделка обхода
			 *
			 * @li **Дословного совпадения записи оборот не обещает - обещает
			 * значение.** Дата выдаётся видом, описанию отвечающим, порядок блоков
			 * берётся порядком дерева. Обратимость закрепляется сличением ДЕРЕВЬЕВ, а
			 * не текстов
			 *
			 * @li **Каталог, поданный вместо файла, отвергается кодом `FILE_NOT_READ`,
			 * а не `FILE_NOT_OPENED`.** Каталог на POSIX открывается УСПЕШНО и читается
			 * признаками конца - теми же, какими отзывается пустой файл, - отчего до
			 * заслона подача его отвечалась ИСТИНОЙ с пустым деревом. Код назван по
			 * существу беды: «открыть не удалось» слало бы потребителя искать права
			 * доступа либо отсутствующий путь, тогда как подан не файл вовсе.
			 * Распознаётся каталог ДО чтения ходом `fs_t::type` - у MS Windows он не
			 * открывается вовсе, и проверка после открытия была бы там мертва.
			 * Закреплено `CodecContract.DirectoryFedInsteadOfAFileIsRefused`, где код
			 * сличается числом у восьми кодеков разом
			 *
			 * @li **Символьная ссылка ПРОХОДИТСЯ, а не отличается от своей цели.** Спрос
			 * вида объекта ведётся `fs_t::type(addr, false)`: ссылка на файл читается
			 * ровно как файл, а ссылка на каталог отвергается ровно как каталог. Ход
			 * этот по умолчанию ссылку ОТЛИЧАЕТ, отвечая `LINK`, и первая редакция
			 * заслона отвергала всякую ссылку кодом `FILE_NOT_OPENED`, тогда как прочесть
			 * её средство файловой системы давало без единой жалобы - отказ был выдуман
			 * кодеком, а не системой. Закреплено `CodecSysLogDocument.SymlinkIsReadAsFile`
			 * и `CodecSysLogDocument.SymlinkToDirectoryIsRefused`
			 *
			 * @li **Сохранение сносит прежний файл ПЕРЕД записью.** Ход записи
			 * `fs_t::write` файл не усекает, а пишет по смещению, и без сноса хвост
			 * прежнего сохранения торчал бы за концом нового: файл при этом читается, а
			 * несёт запись, какой никто не собирал. Связку `unlink` + `write` предписывает
			 * и записка по переходу `src/sys/FS-ADOPTION.md` - усечения средство не
			 * предлагает вовсе. Закреплено `CodecSysLogDocument.SaveOverLongerFile`, где
			 * сличается ВЕЛИЧИНА файла: у POSIX запись усекает сама, и сличение одних
			 * лишь деревьев прошло бы там и без сноса
			 *
			 * @li **За записью следует сброс на носитель, и отказ его записи не
			 * отменяет.** Запись, отвеченная успехом, лежит ещё во вместилище ядра, и
			 * обрыв питания её теряет; сохранение события журнала тем и ценно, что
			 * переживает падение машины. Своего `fsync` тут нет намеренно: под macOS он
			 * долговечности НЕ обещает вовсе, доводит до пластины лишь `F_FULLFSYNC`, и
			 * ход `fs_t::flush` это знает. Отказ сброса уходит предупреждением в журнал,
			 * а не отказом сохранения: записанное на месте и читается
			 *
			 * @li **Спрос у пустого дерева отвечает неопределённостью, а не гибелью.**
			 * Описание отвечается `AUTO`, приоритет нулём, дата пустой строкой, число
			 * блоков нулём. Потребитель волен спросить раньше чтения и обязан различить
			 * «поля нет» по самому ответу. Закреплено
			 * `CodecSysLogDocument.EmptyDocumentQueries`
			 *
			 * @li **Постановка значения вместилище звена ЗАВОДИТ, а снос - нет.** Путь
			 * сквозь звено, картой не являющееся, постановке не помеха: дерево по пути
			 * достраивается, а прежнее знаковое значение теряется. Сносу же такой путь
			 * есть отказ отсутствия поля: снимать нечего. Закреплено
			 * `CodecSysLogDocument.PathEdges`
			 *
			 * \~english
			 * @brief Class of a system log message held in full
			 * @details Holds a parsed record as a tree of the ABC container and issues it
			 * by the methods by a path, common to all the codecs of the library
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					// Дерево разобранной записи контейнером ABC
					abc::value_t _root;
				private:
					// Код ошибки последней операции
					error_t _error;
				private:
					// Объект потокового чтения записей
					reader_t _reader;
					// Объект записи событий
					writer_t _writer;
				private:
					// Объект работы с датой и временем
					mutable chrono_t _chrono;
				private:
					/**
					 * \~russian
					 * @brief Объект работы с файловой системой
					 *
					 * @details Держится ПОЛЕМ, а не заводится в теле хода: так устроены и
					 * прочие опоры документа - чтение, запись, дата, - и так же поступают
					 * прочие модули рамки, «awh::args» в их числе
					 *
					 * \~english
					 * @brief Filesystem object
					 *
					 * \~
					 */
					fs_t _fs;
				private:
					/**
					 * Объект для работы с логами
					 *
					 * @note Объекта фреймворка документ НЕ держит: разбор ведётся чтением,
					 *       сборка - записью, а даты - объектом времени, и всякий из них
					 *       принял фреймворк своим конструктором. Держать ссылку без
					 *       потребителя значило бы заводить поле, о каком собиратель
					 *       справедливо предупреждает
					 */
				public:
					/**
					 * \~russian
					 * @brief Метод разбора записи системного журнала
					 *
					 * @details Разбирается ПЕРВАЯ запись поданного текста: событие
					 * удерживается по одному, а поток записей ведётся чтением
					 *
					 * @param text разбираемый текст записи
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the parsing of a system log record
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
					 * его из записи вовсе
					 *
					 * @param path путь к сбрасываемому значению
					 * @return     признак успешности сброса значения
					 *
					 * \~english
					 * @brief Method of the reset of a value of the tree by a path
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
					 * @return number of the blocks of the structured data
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки дерева события
					 *
					 *
					 * \~english
					 * @brief Method of the clearing of the tree of an event
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения описания, каким прочтена запись
					 *
					 * @return описание, каким прочтена запись
					 *
					 * \~english
					 * @brief Method of getting the description by which the record is read
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
					 * объявленность его берётся ходом `prioritized`
					 *
					 * @return приоритет записи
					 *
					 * \~english
					 * @brief Method of getting the priority of the record
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
					 * потребителю нужен свой: ход этот разбирает её и выдаёт заданным
					 * видом. Дата, разбору не поддавшаяся, выдаётся как есть
					 *
					 * @param format вид, каким надлежит выдать дату сообщения
					 * @return       дата сообщения заданным видом
					 *
					 * \~english
					 * @brief Method of getting the date of a message in a given appearance
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
					 * дата, разбору не поддавшаяся, выдаётся нулём
					 *
					 * @return время сообщения тысячными долями секунды
					 *
					 * \~english
					 * @brief Method of getting the time of a message as a number
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
					 * @return position of the discovered error in the source text
					 *
					 * \~
					 */
					const pos_t & errorPosition() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек разбора записей
					 *
					 * @return настройки разбора записей
					 *
					 * \~english
					 * @brief Method of getting the settings of the parsing of the records
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
					 * настройкой постановки её у записи: порознь их задавать нельзя -
					 * постановка отмены поверх неснятой наращивает косые при всяком
					 * обороте, - оттого пару эту сводит документ, а не потребитель
					 *
					 * @param settings настройки разбора записей
					 * @return         признак успешной установки настроек
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of the records
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
					Document() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Document() noexcept {}
			} document_t;
		}
	}
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"
#include <sys/macro/global.hpp>

#endif // __AWH_CODEC_SYSLOG_DOCUMENT__
