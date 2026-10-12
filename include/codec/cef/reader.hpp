/**
 * @file reader.hpp
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
 * @brief Заголовочный файл потокового чтения записей CEF
 *
 * \~english
 * @brief Header file of the streaming reading of the CEF records
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
#include <sys/fmk.hpp>
#include <sys/macro/global.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */

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
			 * @brief Состояния потокового чтения записей
			 *
			 * @details Перечень держится равным перечням кодеков INI, TOML и YAML
			 *          намеренно: потребитель, ведущий чтение по состоянию, иначе учил бы для
			 *          всякого кодека свой перечень.
			 *
			 * \~english
			 * @brief States of the streaming reading of the records
			 *
			 * @details The list is deliberately kept equal to the lists of the INI, TOML and YAML codecs:
			 *          a consumer conducting the reading by a state would otherwise learn its own list for
			 *          every codec.
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
			 * @brief Класс потокового чтения записей CEF
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его
			 *          целиком: приставку syslog, поля заголовка по счёту, пары расширения и
			 *          знак окончания записи. Записи разделяются переводом строки, и в одном
			 *          потоке их может быть сколько угодно.
			 *
			 * @par Намеренные решения
			 *
			 * Перечисленное ниже не является пробелом реализации: это очерченные границы
			 * задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Разбор не зависит от нарезки на куски.** Кусок обрывается где угодно,
			 *     в том числе посреди имени ключа, посреди отменяющей последовательности и
			 *     посреди приставки syslog; события выдаются те же и в тех же местах, что и
			 *     при подаче текста целиком
			 *
			 * @li **Координаты отсчитываются от начала исходного потока.** Удаление
			 *     разобранной части хранилища сохраняет смещение, строку и столбец его
			 *     нового начала. Ход `reset()` начинает отсчёт заново. Закреплено
			 *     `CodecCefReader.PositionsSurviveRepeatedCompaction`,
			 *     `CodecCefReader.ErrorPositionsSurviveRepeatedCompaction` и
			 *     `CodecCefReader.ResetRestoresPositionsAfterCompaction`
			 *
			 * @li **Пары расширения разбираются собственным ходом `Reader::extensions`.**
			 *     Устройство его рассчитано на записи вида CEF: значение может нести
			 *     разделитель записей и кончается перед разделителем ключа следующей записи,
			 *     последняя запись занимает весь остаток, знак считается отменённым при
			 *     нечётном числе предшествующих косых. Общий разбор `awh::fmk::kv` стоял здесь
			 *     прежде и держал 1612 нс на оборот против 163 нс своего хода (замер
			 *     06.10.2026); договор при замене не изменился — пары сверены с общим ходом
			 *     на 1 200 000 случайных текстов расхождением в ноль
			 *
			 * @li **Приставка syslog опознаётся по слову «CEF:», а не разбирается.**
			 *     Всё, что стоит до первого неотменённого вхождения этого слова, выдаётся
			 *     событием приставки одной последовательностью знаков. Разбирать её по
			 *     RFC 3164 либо RFC 5424 в задачу кодека не входит: приставок этих
			 *     несколько, они между собой несовместимы, и место им в своём модуле
			 *
			 * @li **Выключенное снятие отмены знаков обратимости НЕ обещает.** Разбор
			 *     пар ведётся собственным ходом, а он снимает кавычки-ограду значения ВСЕГДА,
			 *     настройке кодека не подчиняясь: значение «"a b"» выдаётся как
			 *     «a b» и при выключенном снятии отмены. Записанное же обратно без ограды,
			 *     оно разбирается иначе, коли несёт знак равенства. Обратимость оттого
			 *     обещана при умолчании - снятии отмены включённом, - где знак равенства
			 *     ограждается отменою и разреза не даёт. Ставить кавычки писателем было бы
			 *     отсебятиной: описание ArcSight ограды кавычками не знает вовсе
			 *
			 * @li **Строгое сличение обратимости ТЕКСТ-В-ТЕКСТ не обещает.** Сличение
			 *     со словарём обращает значения в виды, словарём заданные: число становится
			 *     числом, адрес - адресом, метка времени - штампом в миллисекундах. Обратная
			 *     сборка пишет их формою КАНОНИЧЕСКОЙ, а не исходною: «rt=Feb 17 2023
			 *     23:30:15.734 YEKT» возвращается как «rt=1676658615734». Форма эта описанием
			 *     ArcSight дозволена наравне с текстовой, и запись остаётся годной, но
			 *     совпадения знак в знак ждать от неё нельзя. Обратимость текстовая обещана
			 *     при сличении выключенном, где значения кладутся в дерево как есть; при
			 *     сличении же обещано совпадение ДЕРЕВЬЕВ, а не текстов
			 *
			 * @li **Отмена знаков снимается порознь по областям.** В заголовке снимаются
			 *     «\|» и «\\», в расширении - «\=», «\\», «\n» и «\r». Обратная косая перед
			 *     иным знаком в расширении оставляется как есть вместе с самим знаком: живые
			 *     журналы несут пути MS Windows без всякой отмены, и обращение «\d» в «d»
			 *     молча портило бы значение
			 *
			 * \~english
			 * @brief Class of the streaming reading of the CEF records
			 *
			 * @details The reading issues the events as the text is parsed without holding it
			 *          in full: the syslog prefix, the fields of the header by the count, the pairs of the extension and
			 *          the sign of the end of a record.
			 *
			 * @par Deliberate decisions
			 *      What is listed below is not a gap of the implementation: these are the outlined
			 *      boundaries of the task, and each of the decisions is fixed by a verifying test
			 *
			 * @li **The parsing does not depend on the cutting into pieces.** A piece is cut anywhere,
			 *     among other things in the middle of a key name, in the middle of an escaping sequence
			 *     and in the middle of a syslog prefix; the events are issued the same and at the same
			 *     places as at the feeding of the text in full
			 *
			 * @li The coordinates are counted from the beginning of the source stream. A removal of the
			 *     parsed part of the storage keeps the offset, the line and the column of its new
			 *     beginning. The `reset()` step starts the counting anew. Fixed by
			 *     `CodecCefReader.PositionsSurviveRepeatedCompaction`,
			 *     `CodecCefReader.ErrorPositionsSurviveRepeatedCompaction` and
			 *     `CodecCefReader.ResetRestoresPositionsAfterCompaction`
			 *
			 * @li **The pairs of the extension are parsed by the own step `Reader::extensions`.** Its
			 *     arrangement is reckoned on the records of the CEF form: a value may carry a separator of
			 *     the records and ends before a separator of a key of the next record, the last record
			 *     takes the whole remainder, a sign is counted escaped at an odd number of the preceding
			 *     backslashes. The common parsing `awh::fmk::kv` stood here before and held 1612 ns per a
			 *     turn against 163 ns of the own step (a measurement of 2026-10-06); the contract by the
			 *     replacement did not change - the pairs were compared with the common step over
			 *     1 200 000 random texts with a zero divergence
			 *
			 * @li **The syslog prefix is recognized by the word "CEF:", and not parsed.** Everything
			 *     standing before the first unescaped occurrence of that word is issued as a prefix event
			 *     by a single sequence of characters. To parse it by RFC 3164 or RFC 5424 is not in the
			 *     task of the codec: there are several of these prefixes, they are incompatible with each
			 *     other, and their place is in their own module
			 *
			 * @li **A turned-off removal of the escaping does NOT promise a round trip.** The parsing of
			 *     the pairs is conducted by the own step, and it removes the quotes-enclosure of a value
			 *     ALWAYS, not obeying the setting of the codec: a value «"a b"» is issued as «a b» even
			 *     at a turned-off removal of the escaping. A written back without an enclosure, it is
			 *     parsed differently when it carries an equals sign. A round trip is therefore promised at
			 *     the default - a removal of the escaping turned on, - where an equals sign is enclosed by
			 *     an escaping and gives no cut. To put quotes by the writer would be an invention: the
			 *     ArcSight description knows no quotes as an enclosure at all
			 *
			 * @li **A strict round trip comparison TEXT-TO-TEXT does not promise.** A comparison with the
			 *     dictionary turns the values into the kinds given by the dictionary: a number becomes a
			 *     number, an address - an address, a timestamp - a stamp in milliseconds. A reverse
			 *     assembling writes them by a CANONICAL form, and not by the source one: «rt=Feb 17 2023
			 *     23:30:15.734 YEKT» returns as «rt=1676658615734». That form is allowed by the ArcSight
			 *     description on a par with a textual one, and the record stays fit, but a character by
			 *     character coincidence from it is not to be waited. A textual round trip is promised at a
			 *     comparison turned off, where the values are laid into the tree as they are; at a
			 *     comparison, on the contrary, a coincidence of the TREES is promised, and not of the
			 *     texts
			 *
			 * @li **An escaping of the signs is removed separately by the regions.** In the header
			 *     `\|` and `\\` are removed, in the extension - `\=`, `\\`, `\n` and `\r`. A
			 *     backslash before another sign in the extension is left as it is, together with the sign
			 *     itself: the live journals carry the paths of MS Windows without any escaping, and
			 *     turning an escaped `\d` into a bare `d` would silently spoil a value
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора записей CEF
					 *
					 *
					 * \~english
					 * @brief Settings of the parsing of the CEF records
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Строгость сличения ключей расширения со словарём
						mode_t mode;
						// Обращение с пустым значением расширения
						empty_t empty;
						// Запись даты, меткам времени назначенная
						string timestamp;
						// Запись даты, меткам времени с долей секунды назначенная
						string fraction;
						// Признак признания приставки syslog перед словом «CEF:»
						bool syslog;
						// Признак снятия отмены знаков со значений
						bool unescape;
						// Наибольшая допустимая длина одной записи в байтах
						uint32_t maxRecord;
						// Наибольшее допустимое количество пар расширения у одной записи
						uint32_t maxExtensions;
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
						 mode(mode_t::NONE), empty(empty_t::STRING),
						 timestamp(TIMESTAMP_FORMAT), fraction(TIMESTAMP_FRACTION_FORMAT),
						 syslog(true), unescape(true),
						 maxRecord(MAX_RECORD), maxExtensions(MAX_EXTENSIONS) {}
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
						SYSLOG    = 0x01, // Выдача приставки syslog событием
						HEADER    = 0x02, // Выдача полей заголовка событиями порознь
						EXTENSION = 0x03, // Выдача пар расширения событиями порознь
						FINISH    = 0x04  // Выдача знака окончания записи событием
					};
				private:
					// Этап разбора текущей записи
					stage_t _stage;
					// Номер поля заголовка, выдаваемого текущим событием
					field_t _field;
					// Указатель выдачи полей заголовка либо пар расширения
					size_t _index;
				private:
					// Поля заголовка текущей записи со снятой отменой знаков
					vector <string> _fields;
					// Пары расширения текущей записи со снятой отменой знаков
					vector <pair <string, string>> _pairs;
				private:
					// Номер редакции записи, словом «CEF:» объявленный
					uint32_t _version;
					// Важность события, заголовком объявленная
					uint32_t _severity;
				private:
					// Содержимое текущего события: имя ключа расширения
					string _key;
					// Содержимое текущего события: значение
					string _value;
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
					 * @details Концом записи служит перевод строки, отмене не подлежащий:
					 *          многострочное значение записывается последовательностью «\n», а не
					 *          самим переводом строки.
					 *
					 * @param length длина найденной записи без знака конца строки
					 * @param next   смещение начала следующей записи
					 * @return       признак того, что запись найдена целиком
					 *
					 * \~english
					 * @brief Method of the search for the end of the current record
					 *
					 * @details The end of a record serves a line feed not subject to an escaping: a multiline
					 *          value is written by a sequence of an escaped line feed, and not by the line feed
					 *          itself.
					 *
					 * @param length length of the found record without the end-of-line character
					 * @param next   offset of the beginning of the next record
					 * @return       flag of the record having been found in full
					 *
					 * \~
					 */
					bool measure(size_t & length, size_t & next) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора очередной записи целиком
					 *
					 * @details Запись разбирается на приставку, поля заголовка и пары
					 *          расширения, каковые затем выдаются событиями порознь. Разбор ведётся
					 *          записью целиком, ибо конец значения расширения опознаётся лишь по
					 *          началу следующей пары, а конец последней пары - по концу записи.
					 *
					 * @param record текст записи целиком
					 * @return       признак успешности разбора записи
					 *
					 * \~english
					 * @brief Method of the parsing of the next record as a whole
					 *
					 * @details A record is parsed into the prefix, the fields of the header and the pairs
					 *          of the extension, which are then issued as the events separately. The parsing is
					 *          conducted by a record as a whole, because the end of the value of an extension is
					 *          recognised only by the beginning of the next pair, and the end of the last pair —
					 *          by the end of the record.
					 *
					 * @param record text of the record as a whole
					 * @return       flag of the success of the parsing of the record
					 *
					 * \~
					 */
					bool prepare(const string_view record) noexcept;
					/**
					 * \~russian
					 * @brief Метод отыскания слова «CEF:» в записи
					 *
					 * @details Разыскивается первое вхождение, отмене не подлежащее: слово
					 *          это может стоять и внутри приставки syslog как часть имени узла.
					 *
					 * @param text   текст записи целиком
					 * @param result смещение найденного слова от начала записи
					 * @return       признак того, что слово найдено
					 *
					 * \~english
					 * @brief Method of the search for the word "CEF:" in a record
					 *
					 * @details The first occurrence not subject to an escaping is looked for: that word may
					 *          stand inside a syslog prefix as a part of a device name.
					 *
					 * @param text   text of the record in full
					 * @param result offset of the found word from the beginning of the record
					 * @return       flag of the word having been found
					 *
					 * \~
					 */
					bool signature(const string_view text, size_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод отыскания конца поля заголовка
					 *
					 * @details Концом поля служит прямая черта, отмене не подлежащая: знак
					 *          считается отменённым при нечётном числе предшествующих обратных косых.
					 *
					 * @param text   текст, начинающийся с поля заголовка
					 * @param result длина поля до разделяющей черты
					 * @return       признак того, что черта найдена
					 *
					 * \~english
					 * @brief Method of the search for the end of a field of the header
					 *
					 * @details The end of a field serves a pipe character not subject to an escaping: a sign is
					 *          counted escaped at an odd number of the preceding backslashes.
					 *
					 * @param text   text beginning with a field of the header
					 * @param result length of the field up to the separating bar
					 * @return       flag of the bar having been found
					 *
					 * \~
					 */
					bool bounds(const string_view text, size_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора пар расширения записи
					 *
					 * @details Пары разделяются прямым пробелом, а имя ключа от значения
					 *          отделяется прямым знаком «=». Значение бывает многословным: концом его
					 *          служит последний пробел, встреченный перед знаком «=» следующей пары, и
					 *          потому разбор идёт ОДНИМ проходом вперёд, без возврата по прочитанному.
					 *          Знак, ограждённый нечётным числом предшествующих обратных косых,
					 *          разделителем не является, а значение в кавычках кончается закрывающей
					 *          кавычкой, а не пробелом.
					 *
					 * @note Ход этот заменяет общий разбор awh::fmk::kv, и платой у того был
					 *       сам перебор знаков: на всяком шаге общий ход сличает текст с
					 *       подстрокою-разделителем, а не со знаком. Замер 06.10.2026 на
					 *       2 000 000 оборотов: разбор расширения одной записи стоит 163 нс
					 *       ходом собственным против 1612 нс ходом общим; стирание вида
					 *       обратного вызова в том же ходе добавляет лишь 9 %, а поиск
					 *       подстрокой вместо знака — лишь 6 %. Пары сверены с общим ходом на
					 *       1 200 000 случайных текстов расхождением в ноль, прежде чем тот
					 *       был снят
					 *
					 * @param text текст расширения записи
					 *
					 * \~english
					 * @brief Method of the parsing of the pairs of the extension of a record
					 *
					 * @details Pairs are separated by a plain space, and the name of a key is
					 *          separated from its value by a plain «=». A value may consist of several
					 *          words: its end is the last space met before the «=» of the next pair, and
					 *          for that reason the parsing goes as a SINGLE forward pass, without backing
					 *          up. A character shielded by an odd number of the preceding backslashes is
					 *          not a separator, and a quoted value ends with the closing quote, not with
					 *          the space.
					 *
					 * @note This step replaces the common parsing of `awh::fmk::kv`, and its price was the
					 *       traversal of the signs itself: at every turn the common step compares a text with a
					 *       substring-separator, and not with a sign. A measurement of 2026-10-06 over
					 *       2 000 000 turns: the parsing of an extension of one record costs 163 ns by the own
					 *       step against 1612 ns by the common one; erasing the kind of a callback at the same
					 *       step adds only 9 %, and a search by a substring instead of a sign - only 6 %. The
					 *       pairs were compared with the common step over 1 200 000 random texts with a zero
					 *       divergence before it was removed
					 *
					 * @param text text of the extension of the record
					 *
					 * \~
					 */
					void extensions(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия отмены знаков со значения
					 *
					 * @details Правила отмены у областей записи РАЗНЫЕ, и область
					 *          передаётся доводом именно поэтому: в заголовке снимаются «\|» и
					 *          «\\», в расширении - «\=», «\\», «\n» и «\r»
					 *
					 * @note Обратная косая перед иным знаком оставляется вместе с самим
					 *       знаком: живые журналы несут пути MS Windows без всякой отмены
					 *
					 * @param text   значение, отмену знаков несущее
					 * @param area   область записи, из которой значение взято
					 * @param result значение со снятой отменой знаков
					 *
					 * \~english
					 * @brief Method of the removal of the escaping of the characters from a value
					 *
					 * @details The rules of the escaping of the areas of a record are DIFFERENT, and the area
					 *          is passed as an argument precisely for that reason: in the header «\|» and «\\»
					 *          are removed, in the extension — «\=», «\\», «\n» and «\r»
					 *
					 * @note A backslash before another sign is left together with the sign itself: the live
					 *       journals carry the paths of MS Windows without any escaping
					 *
					 * @param text   value carrying the escaping of the characters
					 * @param area   area of the record the value is taken from
					 * @param result value with the escaping of the characters removed
					 *
					 * \~
					 */
					void unescape(const string_view text, const area_t area, string & result) const noexcept;
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
					 * @details Настройки принимаются лишь до начала разбора: смена их
					 *          посреди текста давала бы разбор одного текста двумя правилами.
					 *
					 * @param settings настройки разбора записей
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing of the records
					 *
					 * @details The settings are accepted only before the beginning of the parsing: a change of them in the
					 *          middle of a text would give the parsing of one text by two rules.
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
					 * @brief Метод сброса состояния чтения
					 *
					 * @details Сбрасывается всё состояние разбора вместе с хранилищем
					 *          подаваемого текста; настройки разбора при этом сохраняются.
					 *
					 * \~english
					 * @brief Method of the resetting of the state of the reading
					 *
					 * @details The whole state of the parsing is reset together with the storage of the
					 *          fed text; the settings of the parsing are preserved at that.
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод передачи очередного куска исходного текста
					 *
					 * @details Куски подаются в порядке их следования в тексте и делятся
					 *          произвольно: обрыв допустим в любом месте, в том числе посреди имени
					 *          ключа и посреди отменяющей последовательности.
					 *
					 * @note Признак последнего куска обязателен: без него конец текста
					 *       неотличим от его обрыва, и последняя запись без знака конца строки
					 *       осталась бы неразобранной
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the next chunk of the source text
					 *
					 * @details The chunks are passed in the order of their succession in the text and are divided
					 *          arbitrarily: a break is admissible in any place, including the middle of a name of
					 *          a key and the middle of an escaping sequence.
					 *
					 * @note A flag of the last chunk is mandatory: without it an end of the text is
					 *       indistinguishable from its break, and a last record without a sign of the end of a
					 *       line would stay unparsed
					 *
					 * @param buffer buffer of the next chunk of the source text
					 * @param size   size of the buffer of the next chunk of the source text
					 * @param end    flag of the chunk being the last one
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод передачи исходного текста целиком
					 *
					 * @details Разбирает переданный текст как единственный и последний кусок.
					 *
					 * @param text исходный текст записей целиком
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the source text in full
					 *
					 * @details Parses the passed text as the single and last chunk.
					 *
					 * @param text source text of the records in full
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к следующему событию разбора
					 *
					 * @details Отрицательный итог означает, что событий больше нет: разбор
					 *          либо исчерпал переданное и ждёт следующего куска, либо дошёл до конца
					 *          текста, либо прекращён ошибкой. Что именно произошло, сообщает
					 *          состояние чтения.
					 *
					 * @return признак наличия очередного события разбора
					 *
					 * \~english
					 * @brief Method of moving to the next parsing event
					 *
					 * @details A negative result means that there are no more events: the parsing has either exhausted
					 *          what was fed and waits for the next piece, or has reached the end of the text, or has been
					 *          stopped by a failure. What exactly has happened is reported by the state of the reading.
					 *
					 * @return flag of the presence of the next parsing event
					 *
					 * \~
					 */
					bool next() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущего состояния чтения
					 *
					 * @return текущее состояние чтения записей
					 *
					 * \~english
					 * @brief Method of getting the current state of the reading
					 *
					 * @return current state of the reading of the records
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
					 * @brief Method of getting the kind of the current parsing event
					 *
					 * @return kind of the current parsing event
					 *
					 * \~
					 */
					event_t event() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки последней операции разбора
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
					 *
					 * @return error code of the last operation of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения ошибки
					 *
					 * @return положение обнаруженной ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error
					 *
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const pos_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места начала текущей ЗАПИСИ
					 *
					 * @details Место отсчитывается по ЗАПИСИ, а не по отдельному событию:
					 *          события одной записи - поля заголовка, пары расширения - все отвечают
					 *          местом её начала. Потребителю оно и нужно: событие без записи, его
					 *          породившей, ни к чему не отнести.
					 *
					 * @note Прежде описание обещало «место начала текущего события», и
					 *       обещание это делу не отвечало. Уточнено по делу 17.09.2026, а не
					 *       дело подогнано под слово: место на КАЖДОЕ событие требовало бы
					 *       отсчёта в горячем ходе разбора, а цена его рядом с ним мерена -
					 *       43 % пропускной способности за один лишний путь
					 *
					 * @return положение начала текущей записи в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of the beginning of the current record
					 *
					 * @details The place is counted by the RECORD, and not by a single event: the events of one record —
					 *          the fields of the header, the pairs of an extension — all answer with the place of its
					 *          beginning. That is exactly what a consumer needs: an event without the record that gave it
					 *          birth cannot be attributed to anything.
					 *
					 * @note Before, the description promised "a place of the beginning of the current event",
					 *       and that promise did not answer the deed. It was clarified by the deed on
					 *       2026-09-17, and not the deed fitted to a word: a place at EVERY event would ask for
					 *       a counting in the hot step of the parsing, and its price was measured next to it -
					 *       43 % of the throughput for one extra path
					 *
					 * @return position of the beginning of the current record in the source text
					 *
					 * \~
					 */
					const pos_t & position() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения поля заголовка текущего события
					 *
					 * @details Пригодно лишь для события поля заголовка; для прочих событий
					 *          выдаётся поле номера редакции записи.
					 *
					 * @return поле заголовка, выдаваемое текущим событием
					 *
					 * \~english
					 * @brief Method of getting the field of the header of the current event
					 *
					 * @details Suitable only for the event of a field of the header; for the other events the field of the
					 *          number of the version of the record is issued.
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
					 * @details Пригодно лишь для события пары расширения; для прочих событий
					 *          имя пусто. Имя выдаётся СЫРЫМ, как оно в записи стоит: сведение
					 *          ключа с меткой и с длинным именем словаря есть перевод, и ведётся он
					 *          не чтением, а деревом.
					 *
					 * @return имя ключа, выдаваемое текущим событием
					 *
					 * @warning Вид живёт ЛИШЬ до следующего события: содержимое его
					 *          замещается разбором следующей пары. Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the name of the key of the current event
					 *
					 * @details Suitable only for the event of a pair of an extension; for the other events the name is
					 *          empty. The name is issued RAW, as it stands in the record: the reduction of a key with its
					 *          label and with the long name of the dictionary is a translation, and it is conducted not by
					 *          the reading but by the tree.
					 *
					 * @warning The view lives ONLY up to the next event: its content is replaced by the parsing
					 *          of the next pair. A copy is to be kept, and not a view
					 *
					 * @return name of the key issued by the current event
					 *
					 * \~
					 */
					const string & key() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения текущего события
					 *
					 * @details Пригодно для событий приставки syslog, поля заголовка и пары
					 *          расширения; для прочих событий значение пусто.
					 *
					 * @return значение, выдаваемое текущим событием
					 *
					 * @warning Вид живёт ЛИШЬ до следующего события: содержимое его
					 *          замещается разбором следующего. Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the value of the current event
					 *
					 * @details Suitable for the events of the syslog prefix, of a field of the header and of a pair
					 *          of an extension.
					 *
					 * @warning The view lives ONLY up to the next event: its content is replaced by the next
					 *          parsing. A copy is to be kept, and not a view
					 *
					 * @return value issued by the current event
					 *
					 * \~
					 */
					const string & value() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения номера редакции текущей записи
					 *
					 * @details Номер объявляется словом «CEF:» и держится до конца записи.
					 *
					 * @return номер редакции записи
					 *
					 * \~english
					 * @brief Method of getting the number of the version of the current record
					 *
					 * @details A number is declared by the word "CEF:" and is held to the end of the record.
					 *
					 * @return number of the version of the record
					 *
					 * \~
					 */
					uint32_t version() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения важности события текущей записи
					 *
					 * @details Важность объявляется последним полем заголовка и держится до
					 *          конца записи; до разбора этого поля выдаётся ноль.
					 *
					 * @return важность события записи
					 *
					 * \~english
					 * @brief Method of getting the severity of the event of the current record
					 *
					 * @details A severity is declared by the last field of the header and is held to the end of
					 *          the record; before the parsing of that field a zero is issued.
					 *
					 * @return severity of the event of the record
					 *
					 * \~
					 */
					uint32_t severity() const noexcept;
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
