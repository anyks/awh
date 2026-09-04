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
#ifndef __AWH_CODEC_CEF_READER__
#define __AWH_CODEC_CEF_READER__

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

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

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
			 * намеренно: потребитель, ведущий чтение по состоянию, иначе учил бы для
			 * всякого кодека свой перечень
			 *
			 * \~english
			 * @brief States of the streaming reading of the records
			 * @details The list is deliberately kept equal to the lists of the INI, TOML and YAML codecs
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
			 * целиком: приставку syslog, поля заголовка по счёту, пары расширения и
			 * знак окончания записи. Записи разделяются переводом строки, и в одном
			 * потоке их может быть сколько угодно
			 *
			 * @par Намеренные решения
			 *
			 * Перечисленное ниже не является пробелом реализации: это очерченные границы
			 * задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Разбор не зависит от нарезки на куски.** Кусок обрывается где угодно,
			 * в том числе посреди имени ключа, посреди отменяющей последовательности и
			 * посреди приставки syslog; события выдаются те же и в тех же местах, что и
			 * при подаче текста целиком
			 *
			 * @li **Пары расширения разбираются ходом `fmk_t::kv`.** Устройство его
			 * рассчитано на записи вида CEF, и заводить второй разбор того же значило бы
			 * держать один договор в двух местах: значение может нести разделитель
			 * записей и кончается перед разделителем ключа следующей записи, последняя
			 * запись занимает весь остаток, знак считается отменённым при нечётном числе
			 * предшествующих косых
			 *
			 * @li **Приставка syslog опознаётся по слову «CEF:», а не разбирается.**
			 * Всё, что стоит до первого неотменённого вхождения этого слова, выдаётся
			 * событием приставки одной последовательностью знаков. Разбирать её по
			 * RFC 3164 либо RFC 5424 в задачу кодека не входит: приставок этих
			 * несколько, они между собой несовместимы, и место им в своём модуле
			 *
			 * @li **Отмена знаков снимается порознь по областям.** В заголовке снимаются
			 * «\|» и «\\», в расширении - «\=», «\\», «\n» и «\r». Обратная косая перед
			 * иным знаком в расширении оставляется как есть вместе с самим знаком: живые
			 * журналы несут пути MS Windows без всякой отмены, и обращение «\d» в «d»
			 * молча портило бы значение
			 *
			 * \~english
			 * @brief Class of the streaming reading of the CEF records
			 * @details The reading issues the events as the text is parsed without holding it
			 * in full: the syslog prefix, the fields of the header by the count, the pairs of the extension and
			 * the sign of the end of a record
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
						// Кодировка исходного текста, извне навязанная
						encoding_t encoding;
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
						 mode(mode_t::NONE), empty(empty_t::STRING), encoding(encoding_t::NONE),
						 syslog(true), unescape(true), maxRecord(MAX_RECORD), maxExtensions(MAX_EXTENSIONS) {}
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
					pos_t _errorPosition;
					// Положение начала текущего события в исходном тексте
					pos_t _position;
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
					 * выдаётся не разом, а событиями порознь, и место остановки надлежит
					 * помнить
					 *
					 * \~english
					 * @brief Stages of the parsing of one record
					 * @details The stage is kept between the calls of the moving to an event
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
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект для работы с логами
					const log_t * _log;
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
					 * многострочное значение записывается последовательностью «\n», а не
					 * самим переводом строки
					 *
					 * @param length длина найденной записи без знака конца строки
					 * @param next   смещение начала следующей записи
					 * @return       признак того, что запись найдена целиком
					 *
					 * \~english
					 * @brief Method of the search for the end of the current record
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
					 * расширения, каковые затем выдаются событиями порознь. Разбор ведётся
					 * записью целиком, ибо конец значения расширения опознаётся лишь по
					 * началу следующей пары, а конец последней пары - по концу записи
					 *
					 * @param record текст записи целиком
					 * @return       признак успешности разбора записи
					 *
					 * \~english
					 * @brief Method of the parsing of the next record as a whole
					 * @details A record is parsed into the prefix, the fields of the header and the pairs
					 * of the extension, which are then issued as the events separately
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
					 * это может стоять и внутри приставки syslog как часть имени узла
					 *
					 * @param text   текст записи целиком
					 * @param result смещение найденного слова от начала записи
					 * @return       признак того, что слово найдено
					 *
					 * \~english
					 * @brief Method of the search for the word «CEF:» in a record
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
					 * считается отменённым при нечётном числе предшествующих обратных косых
					 *
					 * @param text   текст, начинающийся с поля заголовка
					 * @param result длина поля до разделяющей черты
					 * @return       признак того, что черта найдена
					 *
					 * \~english
					 * @brief Method of the search for the end of a field of the header
					 * @param text   text beginning with a field of the header
					 * @param result length of the field up to the separating bar
					 * @return       flag of the bar having been found
					 *
					 * \~
					 */
					bool bounds(const string_view text, size_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия отмены знаков со значения
					 *
					 * @details Правила отмены у областей записи РАЗНЫЕ, и область
					 * передаётся доводом именно поэтому: в заголовке снимаются «\|» и
					 * «\\», в расширении - «\=», «\\», «\n» и «\r»
					 *
					 * @note Обратная косая перед иным знаком оставляется вместе с самим
					 * знаком: живые журналы несут пути MS Windows без всякой отмены
					 *
					 * @param text   значение, отмену знаков несущее
					 * @param area   область записи, из которой значение взято
					 * @param result значение со снятой отменой знаков
					 *
					 * \~english
					 * @brief Method of the removal of the escaping of the characters from a value
					 * @details The rules of the escaping of the areas of a record are DIFFERENT, and the area
					 * is passed as an argument precisely for that reason
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
					 * посреди текста давала бы разбор одного текста двумя правилами
					 *
					 * @param settings настройки разбора записей
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing of the records
					 * @details The settings are accepted only before the beginning of the parsing
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
					 * подаваемого текста; настройки разбора при этом сохраняются
					 *
					 * \~english
					 * @brief Method of the resetting of the state of the reading
					 * @details The whole state of the parsing is reset together with the storage of the
					 * fed text; the settings of the parsing are preserved at that
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
					 * произвольно: обрыв допустим в любом месте, в том числе посреди имени
					 * ключа и посреди отменяющей последовательности
					 *
					 * @note Признак последнего куска обязателен: без него конец текста
					 * неотличим от его обрыва, и последняя запись без знака конца строки
					 * осталась бы неразобранной
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the next chunk of the source text
					 * @details The chunks are passed in the order of their succession in the text and are divided
					 * arbitrarily: a break is admissible in any place
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
					 * @details Разбирает переданный текст как единственный и последний кусок
					 *
					 * @param text исходный текст записей целиком
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the source text in full
					 * @details Parses the passed text as the single and last chunk
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
					 * либо исчерпал переданное и ждёт следующего куска, либо дошёл до конца
					 * текста, либо прекращён ошибкой. Что именно произошло, сообщает
					 * состояние чтения
					 *
					 * @return признак наличия очередного события разбора
					 *
					 * \~english
					 * @brief Method of moving to the next parsing event
					 * @details A negative result means that there are no more events
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
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const pos_t & errorPosition() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места начала текущего события
					 *
					 * @return положение начала текущего события в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the beginning of the current event
					 * @return position of the beginning of the current event in the source text
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
					 * выдаётся поле номера редакции записи
					 *
					 * @return поле заголовка, выдаваемое текущим событием
					 *
					 * \~english
					 * @brief Method of getting the field of the header of the current event
					 * @details Suitable only for the event of a field of the header
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
					 * имя пусто. Имя выдаётся СЫРЫМ, как оно в записи стоит: сведение
					 * ключа с меткой и с длинным именем словаря есть перевод, и ведётся он
					 * не чтением, а деревом
					 *
					 * @return имя ключа, выдаваемое текущим событием
					 *
					 * @warning Вид живёт ЛИШЬ до следующего события: содержимое его
					 * замещается разбором следующей пары. Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the name of the key of the current event
					 * @details Suitable only for the event of a pair of an extension
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
					 * расширения; для прочих событий значение пусто
					 *
					 * @return значение, выдаваемое текущим событием
					 *
					 * @warning Вид живёт ЛИШЬ до следующего события: содержимое его
					 * замещается разбором следующего. Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the value of the current event
					 * @details Suitable for the events of the syslog prefix, of a field of the header and of a pair
					 * of an extension
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
					 * @details Номер объявляется словом «CEF:» и держится до конца записи
					 *
					 * @return номер редакции записи
					 *
					 * \~english
					 * @brief Method of getting the number of the version of the current record
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
					 * конца записи; до разбора этого поля выдаётся ноль
					 *
					 * @return важность события записи
					 *
					 * \~english
					 * @brief Method of getting the severity of the event of the current record
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
					Reader(const fmk_t * fmk, const log_t * log) noexcept;
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

#endif // __AWH_CODEC_CEF_READER__
