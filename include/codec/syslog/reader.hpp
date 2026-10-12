/**
 * @file reader.hpp
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
 * @brief Заголовочный файл потокового чтения сообщений системного журнала — подачи текста кусками
 *        произвольного размера, выдачи полей заголовка, структурированных данных и текста сообщения
 *        событиями порознь, без удержания разобранного в памяти
 *
 * \~english
 * @brief Header file of the streaming reading of the system log messages
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
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "common.hpp"
#include "../../sys/chrono.hpp"

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
			 * @brief Состояния потокового чтения записей
			 *
			 * @details Перечень намеренно держится равным перечням кодеков INI, TOML, YAML
			 *          и CEF: потребитель, ведущий чтение по состоянию, иначе учил бы для всякого
			 *          кодека свой перечень.
			 *
			 * \~english
			 * @brief States of the streaming reading of the records
			 *
			 * @details The list is deliberately kept equal to the lists of the INI, TOML, YAML and CEF codecs:
			 *          a consumer conducting the reading by a state would otherwise learn its own list for every
			 *          codec.
			 *
			 * \~
			 */
			enum class state_t : uint8_t {
				READY    = 0x00, // Событие получено и доступно для чтения
				HUNGRY   = 0x01, // Для продолжения разбора требуется следующий кусок текста
				FINISHED = 0x02, // Текст разобран до конца
				FAILED   = 0x03  // Разбор прекращён ошибкой
			};

			/**
			 * \~russian
			 * @brief Класс потокового чтения сообщений системного журнала
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его
			 *          целиком: поля заголовка по счёту, опознаватели блоков структурированных
			 *          данных, поля этих блоков и текст сообщения. Записи разделяются переводом
			 *          строки, и в одном потоке их может быть сколько угодно.
			 *
			 * @par Намеренные решения
			 *
			 * @li **Координаты отсчитываются от начала исходного потока.** Удаление
			 *     разобранной части хранилища сохраняет смещение, строку и столбец его
			 *     нового начала. Ход `reset()` начинает отсчёт заново. Закреплено
			 *     `CodecSysLogReader.PositionsSurviveRepeatedCompaction`,
			 *     `CodecSysLogReader.ErrorPositionsSurviveRepeatedCompaction` и
			 *     `CodecSysLogReader.ResetRestoresPositionsAfterCompaction`
			 *
			 * @li **Описание записи, определённое самоопределением, ЗАПОМИНАЕТСЯ.** Спрос
			 *     `standard()` после разбора отвечает опознанным описанием, а не настройкой
			 *     `AUTO`, — то есть ровно так же, как если бы описание задали вручную.
			 *     Потребитель, ведущий разбор потока от многих устройств, обязан знать, чем
			 *     прочтена ИМЕННО ЭТА запись, а не чем велено читать поток
			 *
			 * @li **Приставка приоритета необязательна.** Описание требует её, живые же
			 *     устройства шлют записи и без неё, а сборщики журналов такие записи
			 *     принимают. Отвергать их значило бы терять события, оттого отсутствие
			 *     приставки отказом НЕ является, а приоритет остаётся неопределённым
			 *
			 * @li **Знак «-» есть отсутствие значения ЛИШЬ у записей RFC 5424.**
			 *     Описание RFC 3164 его не знает вовсе: там это законное имя узла и законная
			 *     метка приложения, и настройка обращения с отсутствующим значением к таким
			 *     записям не применяется
			 *
			 * @li **Метка приложения ограничена набором знаков.** Описание зовёт её
			 *     алфавитно-цифровой, живые устройства пишут в ней ещё путь к программе и
			 *     знаки «-», «_», «.». Знак, набору чуждый, означает, что метки нет вовсе, а
			 *     есть уже текст сообщения. Ограничение это есть условие ОБРАТИМОСТИ: без
			 *     него текст, двоеточие несущий, становился меткой при повторном разборе
			 *
			 * @li **Текст сообщения выдаётся ОДНИМ событием и не разбирается.** Что бы в
			 *     нём ни лежало - пары «ключ=значение», JSON, разметка, - разбирать его
			 *     значило бы гадать о договоре, какого запись не объявляет
			 *
			 * \~english
			 * @brief Class of the streaming reading of the system log messages
			 *
			 * @details The reading issues the events as the text is parsed, without holding the text in full:
			 *          the fields of the header by count, the identifiers of the blocks of the structured data,
			 *          the fields of those blocks and the text of a message. The records are separated by a line
			 *          feed, and one stream may carry as many of them as need be.
			 *
			 * @par Deliberate decisions
			 *
			 * @li **The coordinates are counted from the beginning of the source stream.** The removal of the
			 *     parsed part of the storage preserves the offset, the line and the column of its new beginning.
			 *     The method `reset()` starts the counting anew. Fixed by
			 *     `CodecSysLogReader.PositionsSurviveRepeatedCompaction`,
			 *     `CodecSysLogReader.ErrorPositionsSurviveRepeatedCompaction` and
			 *     `CodecSysLogReader.ResetRestoresPositionsAfterCompaction`
			 *
			 * @li **The description of a record, determined by a self-determination, is REMEMBERED.** The query
			 *     `standard()` after the parsing answers with the recognised description, and not with the
			 *     `AUTO` setting — that is, exactly as if the description had been set by hand. A consumer
			 *     conducting the parsing of a stream from many devices is bound to know by what THIS VERY
			 *     record has been read, and not by what it was ordered to read the stream
			 *
			 * @li **The prefix of the priority is optional.** The description requires it, but the living devices
			 *     send the records without it as well, and the collectors of journals accept such records. To
			 *     refuse them would mean to lose events, so the absence of a prefix is NOT a refusal, and the
			 *     priority remains undefined
			 *
			 * @li **The sign «-» is an absence of a value ONLY in the records of RFC 5424.** The description of
			 *     RFC 3164 does not know it at all: there it is a lawful name of a host and a lawful label of an
			 *     application, and the setting of the handling of an absent value does not apply to such records
			 *
			 * @li **The label of an application is limited by a set of signs.** The description calls it
			 *     alphanumeric, while the living devices write there a path to a program and the signs «-», «_»,
			 *     «.» as well. A sign alien to that set means that there is no label at all, and that the text of
			 *     a message has already begun. This limitation is a condition of the REVERSIBILITY: without it a
			 *     text carrying a colon would turn into a label at a repeated parsing
			 *
			 * @li **The text of a message is issued by ONE event and is not parsed.** Whatever lies in it — the
			 *     pairs «key=value», JSON, a markup — to parse it would mean to guess about a contract which the
			 *     record does not declare
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора сообщений системного журнала
					 *
					 *
					 * \~english
					 * @brief Settings of the parsing of the system log messages
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Описание, каким надлежит читать записи
						standard_t standard;
						// Строгость сличения разбираемой записи с описанием
						mode_t mode;
						// Обращение с отсутствующим значением поля
						nil_t nil;
						// Признак снятия метки порядка байтов с текста сообщения
						bool bom;
						// Признак снятия отмены знаков со значений структурированных данных
						bool unescape;
						// Наибольшая допустимая длина одной записи в байтах
						uint32_t maxRecord;
						// Наибольшее допустимое количество блоков структурированных данных
						uint32_t maxStructures;
						// Наибольшее допустимое количество полей одного блока данных
						uint32_t maxParams;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						Settings() noexcept :
						 standard(standard_t::AUTO), mode(mode_t::NONE), nil(nil_t::OMIT),
						 bom(true), unescape(true), maxRecord(MAX_RECORD),
						 maxStructures(MAX_STRUCTURES), maxParams(MAX_PARAMS) {}
					} settings_t;
				private:
					// Настройки разбора записей
					settings_t _settings;
				private:
					// Текущее состояние чтения
					state_t _state;
					// Вид текущего события разбора
					event_t _event;
					// Код ошибки последней операции разбора
					error_t _error;
				private:
					// Положение обнаруженной ошибки в исходном тексте
					pos_t _errorLocation;
					// Положение начала текущего события в исходном тексте
					pos_t _position;
					// Положение начала хранилища в исходном тексте
					pos_t _origin;
				private:
					// Хранилище подаваемого текста
					string _buffer;
					// Смещение разбора в хранилище
					size_t _offset;
					// Смещение начала неразобранного остатка записи
					size_t _record;
				private:
					// Признак того, что подан последний кусок текста
					bool _end;
				private:
					/**
					 * \~russian
					 * @brief Этапы разбора одной записи
					 *
					 * @details Этап держится между вызовами перехода к событию: запись
					 *          выдаётся не разом, а событиями порознь, и место остановки надлежит
					 *          помнить.
					 *
					 * \~english
					 * @brief Stages of the parsing of one record
					 *
					 * @details The stage is kept between the calls of the moving to an event: a record is issued not at
					 *          once but by the events separately, and the place of the stop has to be remembered.
					 *
					 * \~
					 */
					enum class stage_t : uint8_t {
						RECORD    = 0x00, // Отыскание очередной записи в хранилище
						HEADER    = 0x01, // Выдача полей заголовка событиями порознь
						STRUCTURE = 0x02, // Выдача структурированных данных событиями порознь
						MESSAGE   = 0x03, // Выдача текста сообщения событием
						FINISH    = 0x04  // Выдача знака окончания записи событием
					};
				private:
					// Этап разбора текущей записи
					stage_t _stage;
					// Номер поля заголовка, выдаваемого текущим событием
					field_t _field;
					// Указатель выдачи полей заголовка либо полей данных
					size_t _index;
					// Указатель выдачи блоков структурированных данных
					size_t _block;
				private:
					// Поля заголовка текущей записи
					vector <pair <field_t, string>> _fields;
					/**
					 * \~russian
					 * @brief Блоки структурированных данных текущей записи
					 *
					 * @note Блок несёт опознаватель и порядок своих полей: порядок этот
					 *       описанием значим - поля одного имени внутри блока запрещены, а
					 *       порядок блоков есть порядок их объявления
					 *
					 * \~english
					 * @brief The blocks of the structured data of the current record
					 *
					 * @note A block carries an identifier and the order of its own fields: that order is
					 *       meaningful by the description - the fields of one name inside a block are forbidden,
					 *       and the order of the blocks is the order of their declaration
					 *
					 * \~
					 */
					vector <pair <string, vector <pair <string, string>>>> _structures;
				private:
					// Текст сообщения текущей записи
					string _message;
					// Признак того, что текст сообщения записью объявлен
					bool _messaged;
				private:
					// Описание, каким прочтена текущая запись
					standard_t _standard;
					// Приоритет текущей записи
					uint32_t _priority;
					// Признак того, что приоритет записью объявлен
					bool _prioritized;
					// Номер описания записи, объявленный самой записью
					uint32_t _version;
				private:
					// Содержимое текущего события: имя ключа либо опознаватель блока
					string _key;
					// Содержимое текущего события: значение
					string _value;
				private:
					// Объект работы с датой и временем
					mutable chrono_t _chrono;
				private:
					/**
					 * \~russian
					 * @brief Объект для работы с логами
					 *
					 * @note Объекта фреймворка читатель НЕ держит: разбор ведётся своими
					 *       ходами, а даты - объектом времени, каким фреймворк принят
					 *       конструктором. Держать ссылку без потребителя значило бы
					 *       заводить поле, о каком собиратель справедливо предупреждает
					 *
					 * \~english
					 * @brief An object for the work with the logs
					 *
					 * @note The reader does NOT hold an object of the framework: the parsing is conducted by its own
					 *       methods, and the dates by the object of the time, as the framework has been accepted by the
					 *       constructor. To hold a link without a consumer would mean to raise a field about which the
					 *       collector fairly warns
					 *
					 * \~
					 */
				private:
					/**
					 * \~russian
					 * @brief Метод прекращения разбора ошибкой
					 *
					 * @param error  код ошибки разбора
					 * @param offset смещение места ошибки в хранилище разбора
					 * @return       признак наличия очередного события разбора
					 *
					 * \~english
					 * @brief Method of the termination of the parsing by an error
					 *
					 * @param error  error code of the parsing
					 * @param offset offset of the place of the error in the storage of the parsing
					 * @return       flag of the presence of the next parsing event
					 *
					 * \~
					 */
					bool fail(const error_t error, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод определения положения смещения в исходном тексте
					 *
					 * @param offset смещение в хранилище разбора
					 * @param result положение, вычисляемое по смещению
					 *
					 * \~english
					 * @brief Method of the determination of the position of an offset in the source text
					 *
					 * @param offset offset in the storage of the parsing
					 * @param result position computed by the offset
					 *
					 * \~
					 */
					void place(const size_t offset, pos_t & result) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод отыскания конца текущей записи
					 *
					 * @details Концом записи служит перевод строки: описание позволяет
					 *          тексту сообщения нести любые октеты, кроме него самого.
					 *
					 * @param length длина найденной записи без знака конца строки
					 * @param next   смещение начала следующей записи
					 * @return       признак того, что запись найдена целиком
					 *
					 * \~english
					 * @brief Method of the search for the end of the current record
					 *
					 * @details The end of a record is served by a line feed: the description allows the text of a
					 *          message to carry any octets except that one itself.
					 *
					 * @param length length of the found record without the character of the end of the line
					 * @param next   offset of the beginning of the next record
					 * @return       flag that the record is found in full
					 *
					 * \~
					 */
					bool measure(size_t & length, size_t & next) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора очередной записи целиком
					 *
					 * @details Запись разбирается разом, а событиями выдаётся порознь:
					 *          разбор по частям требовал бы держать полусостояние всякого поля, а
					 *          запись системного журнала коротка и целиком помещается в памяти.
					 *
					 * @param record текст очередной записи без знака конца строки
					 * @return       признак успешного разбора записи
					 *
					 * \~english
					 * @brief Method of the parsing of the next record as a whole
					 *
					 * @details A record is parsed at once, while the events are issued separately: a parsing by parts
					 *          would require keeping a half-state of every field, and a system log record is short and
					 *          fits in the memory in full.
					 *
					 * @param record text of the next record without the character of the end of the line
					 * @return       flag of the successful parsing of the record
					 *
					 * \~
					 */
					bool prepare(const string_view record) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора приставки приоритета
					 *
					 * @details Приставка есть число, взятое в угловые скобки. Отсутствие её
					 *          отказом не является: приоритет остаётся неопределённым.
					 *
					 * @param record разбираемая запись
					 * @param offset смещение, за приставкой следующее
					 * @return       признак успешного разбора приставки
					 *
					 * \~english
					 * @brief Method of the parsing of the prefix of the priority
					 *
					 * @details The prefix is a number taken into the angle brackets. Its absence is not a refusal:
					 *          the priority remains undefined.
					 *
					 * @param record parsed record
					 * @param offset offset following the prefix
					 * @return       flag of the successful parsing of the prefix
					 *
					 * \~
					 */
					bool priority(const string_view record, size_t & offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод определения описания записи по её виду
					 *
					 * @details Признак назначен самим RFC 5424: номер описания там
					 *          обязателен и стоит первым за приставкой приоритета. Число, за каким
					 *          следует пробел, означает RFC 5424, всё прочее - RFC 3164.
					 *
					 * @param record разбираемая запись
					 * @param offset смещение, за приставкой приоритета следующее
					 * @return       определённое описание записи
					 *
					 * \~english
					 * @brief Method of the determination of the description of a record by its appearance
					 *
					 * @details The sign is appointed by RFC 5424 itself: there the number of the description is
					 *          mandatory and stands first after the prefix of the priority. A number followed by a
					 *          space means RFC 5424, everything else — RFC 3164.
					 *
					 * @param record parsed record
					 * @param offset offset following the prefix of the priority
					 * @return       determined description of the record
					 *
					 * \~
					 */
					standard_t detect(const string_view record, const size_t offset) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора записи описания RFC 3164
					 *
					 * @param record разбираемая запись
					 * @param offset смещение, за приставкой приоритета следующее
					 * @return       признак успешного разбора записи
					 *
					 * \~english
					 * @brief Method of the parsing of a record of the RFC 3164 description
					 *
					 * @param record parsed record
					 * @param offset offset following the prefix of the priority
					 * @return       flag of the successful parsing of the record
					 *
					 * \~
					 */
					bool legacy(const string_view record, size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора записи описания RFC 5424
					 *
					 * @param record разбираемая запись
					 * @param offset смещение, за приставкой приоритета следующее
					 * @return       признак успешного разбора записи
					 *
					 * \~english
					 * @brief Method of the parsing of a record of the RFC 5424 description
					 *
					 * @param record parsed record
					 * @param offset offset following the prefix of the priority
					 * @return       flag of the successful parsing of the record
					 *
					 * \~
					 */
					bool modern(const string_view record, size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора блоков структурированных данных
					 *
					 * @details Блоки идут подряд, без разделителя между ними, и всякий взят
					 *          в квадратные скобки. Отмена знаков внутри значения своя: отменяются
					 *          лишь кавычка, закрывающая скобка и сама обратная косая.
					 *
					 * @param record разбираемая запись
					 * @param offset смещение начала первого блока
					 * @return       признак успешного разбора блоков
					 *
					 * \~english
					 * @brief Method of the parsing of the blocks of the structured data
					 *
					 * @details The blocks go in a row, without a separator between them, and every one is taken into the
					 *          square brackets. The escaping of the signs inside a value is its own: only a quotation mark,
					 *          a closing bracket and the backslash itself are escaped.
					 *
					 * @param record parsed record
					 * @param offset offset of the beginning of the first block
					 * @return       flag of the successful parsing of the blocks
					 *
					 * \~
					 */
					bool structured(const string_view record, size_t & offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки поля заголовка
					 *
					 * @details Ход этот и решает судьбу отсутствующего значения: поле со
					 *          знаком «-» кладётся по настройке, а не всегда одинаково.
					 *
					 * @param field поле заголовка, укладываемое в запись
					 * @param text  значение поля, записью объявленное
					 * @return      признак успешной укладки поля
					 *
					 * \~english
					 * @brief Method of the laying of a field of the header
					 *
					 * @details This method decides the fate of an absent value: a field carrying the sign «-» is laid by
					 *          a setting, and not always in the same way.
					 *
					 * @param field field of the header laid into the record
					 * @param text  value of the field declared by the record
					 * @return      flag of the successful laying of the field
					 *
					 * \~
					 */
					bool lay(const field_t field, const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия отмены знаков со значения структурированных данных
					 *
					 * @param text   значение с отменёнными знаками
					 * @param result значение со снятой отменой знаков
					 *
					 * \~english
					 * @brief Method of the removal of the escaping of the characters from a value of the structured data
					 *
					 * @param text   value with the escaped characters
					 * @param result value with the escaping of the characters removed
					 *
					 * \~
					 */
					void unescape(const string_view text, string & result) const noexcept;
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
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора записей
					 *
					 * @details Настройки принимаются лишь до начала разбора: смена правил
					 *          посреди потока дала бы записи, прочтённые по разным правилам, и
					 *          потребитель не имел бы способа узнать, какая чем прочтена.
					 *
					 * @param settings настройки разбора записей
					 * @return         признак успешной установки настроек
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of the records
					 *
					 * @details The settings are accepted only before the beginning of the parsing: a change of the rules
					 *          in the middle of a stream would give records read by different rules, and a consumer would
					 *          have no way to learn which one was read by what.
					 *
					 * @param settings settings of the parsing of the records
					 * @return         flag of the successful setting of the settings
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния чтения
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the reading
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи очередного куска разбираемого текста
					 *
					 * @param buffer буфер подаваемого текста
					 * @param size   размер подаваемого текста в байтах
					 * @param end    признак того, что подан последний кусок текста
					 * @return       признак успешной подачи текста
					 *
					 * \~english
					 * @brief Method of the feeding of the next piece of the parsed text
					 *
					 * @param buffer buffer of the fed text
					 * @param size   size of the fed text in bytes
					 * @param end    flag that the last piece of the text is fed
					 * @return       flag of the successful feeding of the text
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи разбираемого текста целиком
					 *
					 * @param text подаваемый текст целиком
					 * @return     признак успешной подачи текста
					 *
					 * \~english
					 * @brief Method of the feeding of the parsed text as a whole
					 *
					 * @param text fed text as a whole
					 * @return     flag of the successful feeding of the text
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к очередному событию разбора
					 *
					 * @return признак наличия очередного события разбора
					 *
					 * \~english
					 * @brief Method of the moving to the next event of the parsing
					 *
					 * @return flag of the presence of the next event of the parsing
					 *
					 * \~
					 */
					bool next() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущего состояния чтения
					 *
					 * @return текущее состояние чтения
					 *
					 * \~english
					 * @brief Method of getting the current state of the reading
					 *
					 * @return current state of the reading
					 *
					 * \~
					 */
					state_t state() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида текущего события разбора
					 *
					 * @return вид текущего события разбора
					 *
					 * \~english
					 * @brief Method of getting the kind of the current event of the parsing
					 *
					 * @return kind of the current event of the parsing
					 *
					 * \~
					 */
					event_t event() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки последней операции разбора
					 *
					 * @return код ошибки последней операции разбора
					 *
					 * \~english
					 * @brief Method of getting the error code of the last operation of the parsing
					 *
					 * @return error code of the last operation of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
				public:
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
					/**
					 * \~russian
					 * @brief Метод получения положения начала текущей ЗАПИСИ
					 *
					 * @details Место отсчитывается по ЗАПИСИ, а не по отдельному событию:
					 *          события одной записи - поля заголовка, пары расширения, текст - все
					 *          отвечают местом её начала. Потребителю оно и нужно: событие без
					 *          записи, его породившей, ни к чему не отнести.
					 *
					 * @note Прежде описание обещало «положение начала текущего события», и
					 *       обещание это делу не отвечало. Уточнено по делу 17.09.2026, а не
					 *       дело подогнано под слово: место на КАЖДОЕ событие требовало бы
					 *       отсчёта в горячем ходе разбора, а цена его рядом с ним уже
					 *       мерена - 43 % пропускной способности за один лишний путь
					 *
					 * @return положение начала текущей записи в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of the beginning of the current record
					 *
					 * @details The place is counted by the RECORD, and not by a single event: the events of one record —
					 *          the fields of the header, the pairs of an extension, the text — all answer with the place
					 *          of its beginning. That is exactly what a consumer needs: an event without the record that
					 *          gave it birth cannot be attributed to anything.
					 *
					 * @note Before, the description promised «the position of the beginning of the current event», and
					 *       that promise did not answer the deed. It was clarified by the deed on 2026-09-17, and not the
					 *       deed fitted to a word: a place at EVERY event would require a counting in the hot step of the
					 *       parsing, and its price has already been measured next to it - 43 % of the throughput for one
					 *       extra path
					 *
					 * @return position of the beginning of the current record in the source text
					 *
					 * \~
					 */
					const pos_t & position() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения поля заголовка, текущим событием выданного
					 *
					 * @return поле заголовка, текущим событием выданное
					 *
					 * \~english
					 * @brief Method of getting the field of the header issued by the current event
					 *
					 * @return field of the header issued by the current event
					 *
					 * \~
					 */
					field_t field() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения имени ключа текущего события
					 *
					 * @details Событием STRUCTURE выдаётся опознаватель блока, событием
					 *          PARAM - имя поля внутри блока.
					 *
					 * @return имя ключа текущего события
					 *
					 * \~english
					 * @brief Method of getting the name of the key of the current event
					 *
					 * @details By the event STRUCTURE the identifier of a block is issued, by the event PARAM — the
					 *          name of a field inside a block.
					 *
					 * @return name of the key of the current event
					 *
					 * \~
					 */
					const string & key() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения текущего события
					 *
					 * @return значение текущего события
					 *
					 * \~english
					 * @brief Method of getting the value of the current event
					 *
					 * @return value of the current event
					 *
					 * \~
					 */
					const string & value() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения описания, каким прочтена текущая запись
					 *
					 * @details При самоопределении выдаётся ОПОЗНАННОЕ описание, а не
					 *          настройка `AUTO`: спрос этот отвечает так же, как если бы описание
					 *          задали вручную.
					 *
					 * @return описание, каким прочтена текущая запись
					 *
					 * \~english
					 * @brief Method of getting the description by which the current record is read
					 *
					 * @details Under the self-determination the RECOGNIZED description is issued rather than the `AUTO` setting.
					 *
					 * @return description by which the current record is read
					 *
					 * \~
					 */
					standard_t standard() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения приоритета текущей записи
					 *
					 * @details При отсутствии приставки приоритета выдаётся ноль, а
					 *          объявленность его берётся ходом `prioritized`
					 *
					 * @return приоритет текущей записи
					 *
					 * \~english
					 * @brief Method of getting the priority of the current record
					 *
					 * @details When the prefix of the priority is absent, a zero is issued, and the fact of its
					 *          declaration is taken by the method `prioritized`
					 *
					 * @return priority of the current record
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
					 * @details Источник есть частное от деления приоритета на восемь и
					 *          потому вычисляется, а не хранится.
					 *
					 * @return источник сообщения
					 *
					 * \~english
					 * @brief Method of getting the source of the message
					 *
					 * @details The source is the quotient of the division of the priority by eight, and therefore it
					 *          is computed rather than stored.
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
					 * @details Важность есть остаток от деления приоритета на восемь и
					 *          потому вычисляется, а не хранится.
					 *
					 * @return степень важности сообщения
					 *
					 * \~english
					 * @brief Method of getting the degree of the importance of the message
					 *
					 * @details The severity is the remainder of the division of the priority by eight, and therefore
					 *          it is computed rather than stored.
					 *
					 * @return degree of the importance of the message
					 *
					 * \~
					 */
					severity_t severity() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения номера описания записи
					 *
					 * @details Номер объявляется лишь записями RFC 5424; у записей RFC 3164
					 *          выдаётся ноль.
					 *
					 * @return номер описания записи
					 *
					 * \~english
					 * @brief Method of getting the number of the description of the record
					 *
					 * @details The number is declared only by the records of RFC 5424; for the records of RFC 3164
					 *          a zero is issued.
					 *
					 * @return number of the description of the record
					 *
					 * \~
					 */
					uint32_t version() const noexcept;
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
					Reader() noexcept;
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
					~Reader() noexcept {}
			} reader_t;
		}
	}
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"
#include <sys/macro/global.hpp>
