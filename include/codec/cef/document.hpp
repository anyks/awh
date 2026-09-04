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
#ifndef __AWH_CODEC_CEF_DOCUMENT__
#define __AWH_CODEC_CEF_DOCUMENT__

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
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../net/addr.hpp"
#include "../../sys/chrono.hpp"
#include "../abc/value.hpp"

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
		 * @brief Пространство имён контейнера CEF
		 *
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
			 * ходами по пути, общими у всех кодеков библиотеки. Устройство дерева
			 * отвечает устройству самой записи:
			 *
			 * @li `/syslog` - приставка syslog, записи предшествующая, знаками;
			 * @li `/header` - поля заголовка отображением с именами `version`,
			 *     `vendor`, `product`, `release`, `signature`, `name`, `severity`;
			 * @li `/extension` - пары расширения отображением по СЫРЫМ ключам записи
			 *
			 * @par Намеренные решения
			 *
			 * Перечисленное ниже не является пробелом реализации: это очерченные границы
			 * задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Своего владеющего значения (`value_t`) у кодека НЕТ, и это решение,
			 * а не пробел.** Основанием событию служит дерево контейнера ABC: система
			 * видов его вмещает виды записи CEF с запасом, а ходы `at`, `place`,
			 * `contains`, `erase` даны им уже. Заводить поверх второй владеющий вид
			 * значило бы держать один договор в двух местах и переводить дерево в
			 * дерево на всяком обращении
			 *
			 * @li **Глубина пути ограничена устройством записи.** Путь длиннее трёх
			 * звеньев - `/extension/<ключ>/<номер>` у повторяющегося ключа - записи
			 * CEF неведом. Это граница формата, а не недоделка обхода
			 *
			 * @li **Обход по пути выдаёт СЫРЫЕ ключи.** Сведённое именование берётся
			 * вторыми ходами: `field` разыскивает значение по полному имени словаря,
			 * `label` выдаёт человеческое имя ключа - взятое из метки записи, а при
			 * её отсутствии из словаря
			 *
			 * @li **Повтор ключа даёт перечень.** Второе объявление того же ключа
			 * обращает значение в перечень и добавляет в его конец; обход остаётся
			 * замкнутым числовыми звеньями пути
			 *
			 * @li **Сброс значения и снос пары - РАЗНОЕ.** Сброс оставляет ключ с
			 * пустым значением («cs3=»), снос убирает пару из записи вовсе. Различия
			 * «пусто» и «нет вовсе» сама запись CEF не несёт, оттого сброшенное поле
			 * от записанного пустым неотличимо - и это граница формата, названная
			 * прямо
			 *
			 * \~english
			 * @brief Class of a CEF event held in full
			 * @details Holds a parsed event as a tree of the ABC container and issues it
			 * by the methods by a path, common to all the codecs of the library
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					// Дерево разобранного события контейнером ABC
					abc::value_t _root;
				private:
					// Объект потокового чтения записей
					reader_t _reader;
					// Объект записи событий
					writer_t _writer;
				private:
					// Объект работы с адресами сети
					mutable net_addr_t _net;
					// Объект работы с датой и временем
					mutable chrono_t _chrono;
				private:
					// Код ошибки последней операции
					error_t _error;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект для работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод укладки пары расширения в дерево события
					 *
					 * @details Повтор ключа обращает значение в перечень и добавляет в
					 * его конец; вид значения берётся из словаря по строгости сличения
					 *
					 * @param key   имя ключа пары расширения
					 * @param value значение пары расширения
					 * @return      признак успешности укладки пары
					 *
					 * \~english
					 * @brief Method of the placing of a pair of an extension into the tree of an event
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
					 * знаков. Строгость сличения задаётся настройками разбора
					 *
					 * @param entry запись словаря расширений либо ничто
					 * @param value значение пары расширения знаками
					 * @param result значение дерева контейнера ABC
					 * @return      признак успешности обращения значения
					 *
					 * \~english
					 * @brief Method of the conversion of a value of an extension into a value of the tree
					 * @param entry  record of the dictionary of the extensions or nothing
					 * @param value  value of the pair of the extension as characters
					 * @param result value of the tree of the ABC container
					 * @return       flag of the success of the conversion of the value
					 *
					 * \~
					 */
					bool convert(const entry_t * entry, const string & value, abc::value_t & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора записи CEF
					 *
					 * @details Разбирается ПЕРВАЯ запись поданного текста; записи, за нею
					 * следующие, отбрасываются. Поток из многих записей разбирается
					 * потоковым чтением, а не событием, удерживаемым целиком
					 *
					 * @param text текст записи CEF
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the parsing of a CEF record
					 * @details The FIRST record of the fed text is parsed
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
					 * несущее, записывается отменяющей записью «~1» по RFC 6901
					 *
					 * @param path путь к значению
					 * @return     ссылка на значение либо ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a value of the tree by a path
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
					 * @details Недостающие звенья пути заводятся по дороге
					 *
					 * @param path  путь к значению
					 * @param value значение, по пути ставимое
					 * @return      признак успешности постановки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value of the tree by a path
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
					 * последовательностью знаков: запись «cs3=» есть законная запись
					 * живых журналов. Снос же пары целиком совершается ходом `erase`
					 *
					 * @param path путь к сбрасываемому значению
					 * @return     признак успешности сброса значения
					 *
					 * \~english
					 * @brief Method of the resetting of a value of the tree by a path
					 * @details The node stays in its place, while its content is replaced by an empty
					 * sequence of characters
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
					 * `at(путь + "/" + звено)` ведёт к тому самому потомку. У перечня
					 * звенья числовые. Отказом ход не отвечает никогда: и у листа, и у
					 * пути несуществующего выдаётся пустой перечень, а различает их
					 * ход `has`
					 *
					 * @param path путь к значению
					 * @return     звенья пути, у значения объявленные
					 *
					 * \~english
					 * @brief Method of the extraction of the links of a path declared by a value
					 * @details The LINKS OF A PATH are issued rather than the names
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
					 * @return number of the pairs of the extension of the event
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
					 * @brief Метод извлечения значения расширения по ПОЛНОМУ имени ключа
					 *
					 * @details Ход этот и есть второй ход к сведённому именованию: обход
					 * по пути выдаёт ключи сырыми, а полное имя словаря разыскивается им
					 *
					 * @param name полное имя ключа расширения
					 * @return     ссылка на значение либо ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a value of an extension by the FULL name of a key
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
					 * задаёт имя ключу «cs1», - а при отсутствии метки из словаря
					 * расширений. Если же ключ словарю неизвестен и метки не имеет, имя
					 * выдаётся пустым: выдумывать его кодек не станет
					 *
					 * @param key имя ключа расширения, в записи стоящее
					 * @return    человеческое имя ключа расширения
					 *
					 * \~english
					 * @brief Method of getting the human-readable name of a key of an extension
					 * @details The name is taken from a label of the record itself, and in the absence of a label
					 * from the dictionary of the extensions
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
					 * начала эпохи, а ходом этим выдаётся записью знаков любого вида,
					 * какой примет модуль `chrono_t`. Ход этот и есть замена
					 * установщику записи даты прежнего модуля: тот держал запись
					 * настройкой объекта и менял выдачу ВСЕХ меток разом, здесь же
					 * запись задаётся при выдаче, и два потребителя одного события
					 * могут получить метку каждый в своей записи
					 *
					 * @note Метка выдаётся в МЕСТНОЙ зоне машины, как её выдавал и
					 * прежний модуль: зона, самой записью объявленная, при разборе уже
					 * учтена, и штамп её не помнит. Кому нужна иная зона, тот берёт
					 * штамп ходом `at` и зовёт `chrono_t::format` с зоною сам
					 *
					 * @param key    имя ключа расширения, метку времени несущего
					 * @param format запись даты, выдаваемой метке назначаемая
					 * @return       метка времени записью заданного вида
					 *
					 * \~english
					 * @brief Method of getting a timestamp in a notation of the given kind
					 * @details A timestamp is held by the tree as a stamp in the milliseconds from
					 * the beginning of the epoch, while by this method it is issued as a record of the characters
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
					 * @param key name of the key of the extension standing in the record
					 * @return    kind of the value given by the dictionary
					 *
					 * \~
					 */
					type_t type(const string & key) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения дерева события целиком
					 *
					 * @return дерево разобранного события контейнером ABC
					 *
					 * \~english
					 * @brief Method of getting the tree of an event in full
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
					 * @return position of the detected error in the source text
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
					 * @param settings настройки разбора записей
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing of the records
					 * @param settings settings of the parsing of the records
					 * @return         result of performing the operation
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
					 * @brief Method of setting the settings of the writing of the events
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
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param fmk framework object
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Document(const fmk_t * fmk, const log_t * log) noexcept;
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

#endif // __AWH_CODEC_CEF_DOCUMENT__
