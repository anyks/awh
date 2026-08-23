/**
 * @file writer.hpp
 * @date 2026-08-12
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
 * @brief Заголовочный файл записи текста CSV — класс Writer, собирающий текст полем за
 *        полем и отдающий собранное как целиком, так и по мере накопления
 *
 * \~english
 * @brief Header file of the writing of a CSV text — the Writer class, which assembles the text field by
 *        field and gives away what has been assembled both in full and as it accumulates
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CSV_WRITER__
#define __AWH_CODEC_CSV_WRITER__

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
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений, применяемых ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
		 * @brief Пространство имён контейнера CSV
		 *
		 *
		 * \~english
		 * @brief CSV container namespace
		 *
		 * \~
		 */
		namespace csv {
			/**
			 * \~russian
			 * @brief Класс записи текста CSV
			 *
			 * @details Текст собирается полем за полем, а отдаётся либо целиком по
			 * окончании, либо кусками по мере накопления - смотря по тому, держит ли
			 * потребитель собранное в памяти или отправляет его дальше
			 *
			 * @par Порядок работы
			 *
			 * @par Потоковая запись
			 * @note Запись пределов не проверяет и ошибок не заводит: собираемый текст
			 * ошибочным не бывает, а поле, содержащее что угодно, всегда записывается
			 * так, чтобы разобраться обратно неизменным. Договор этот закреплён
			 * проверкой кругового прохода
			 *
			 *  @code{.cpp}
			 *  writer_t writer(log);
			 *
			 *  writer.field("имя");
			 *  writer.field("значение");
			 *  writer.record();
			 *
			 *  const string & text = writer.text();
			 *  @endcode
			 *
			 *  @code{.cpp}
			 *  while(!done){
			 *      writer.record(fields);
			 *
			 *      if(writer.size() >= 0x10000)
			 *          send(writer.take());
			 *  }
			 *
			 *  send(writer.take());
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the writing of a CSV text
			 * @details The text is assembled field by field, and it is given away either in full upon
			 * the ending or by chunks as it accumulates — depending on whether the consumer holds
			 * what has been assembled in the memory or sends it further on
			 * @par Order of the work
			 * @par Streaming writing
			 * @note The writing does not check the limits and does not record errors: the text being assembled is
			 * never erroneous, while a field containing anything at all is always written
			 * so as to be parsed back unchanged. This contract is fixed by
			 * a round-trip test
			 *
			 *  @code{.cpp}
			 *  writer_t writer(log);
			 *
			 *  writer.field("name");
			 *  writer.field("value");
			 *  writer.record();
			 *
			 *  const string & text = writer.text();
			 *  @endcode
			 *
			 *  @code{.cpp}
			 *  while(!done){
			 *      writer.record(fields);
			 *
			 *      if(writer.size() >= 0x10000)
			 *          send(writer.take());
			 *  }
			 *
			 *  send(writer.take());
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				private:
					/**
					 * \~russian
					 * Объект для работы с логами
					 *
					 * \~english
					 * Object for working with logs
					 *
					 * \~
					 */
					const Logging * _log;
				public:
					/**
					 * \~russian
					 * @brief Настройки записи текста
					 *
					 * \~english
					 * @brief Settings of the writing of a text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Знак-разделитель полей
						char separator;
						// Знак кавычек, обрамляющих поле
						char quote;
						/**
						 * \~russian
						 * Знак начала строки примечания, признаваемый разбором
						 *
						 * @note Знак этот записью не ставится, а лишь учитывается: поле,
						 * начинающее запись этим знаком, берётся в кавычки, иначе разбор
						 * прочтёт всю запись строкой примечания и отбросит её. Умолчанием
						 * знака нет вовсе - примечания договором не описаны
						 *
						 * \~english
						 * Character of the beginning of a comment line recognized by the parsing
						 * @note This character is not put by the writing but is only taken into account: a field
						 * beginning a record with this character is taken into quotes, otherwise the parsing
						 * will read the whole record as a comment line and discard it. By default
						 * there is no character at all — the comments are not described by the protocol
						 *
						 * \~
						 */
						char comment;
						// Правило заключения поля в кавычки
						quoting_t quoting;
						// Способ записи кавычки внутри поля, заключённого в кавычки
						escape_t escape;
						// Знак конца строки
						newline_t newline;
						/**
						 * \~russian
						 * Признак записи метки порядка байтов в начале текста
						 *
						 * @note Метка эта содержимым текста не является, но без неё
						 * табличные редакторы MS Windows читают UTF-8 как местную
						 * однобайтовую кодировку и портят всё, кроме US-ASCII
						 *
						 * \~english
						 * Flag of the writing of the byte order mark at the beginning of the text
						 * @note This mark is not the content of the text, but without it
						 * the spreadsheet editors of MS Windows read UTF-8 as the local
						 * single-byte encoding and spoil everything except US-ASCII
						 *
						 * \~
						 */
						bool signature;
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
						Settings() noexcept;
					} settings_t;
				private:
					// Настройки записи текста
					settings_t _settings;
				private:
					// Собранный текст
					string _text;
				private:
					// Признак того, что запись уже содержит поля
					bool _started;
					// Признак того, что метка порядка байтов уже записана
					bool _marked;
				private:
					/**
					 * \~russian
					 * @brief Метод записи метки порядка байтов
					 *
					 * @details Метка записывается однажды и лишь перед первым полем: текст,
					 * отданный по частям, метки посреди себя иметь не должен
					 *
					 * \~english
					 * @brief Method of writing the byte order mark
					 * @details The mark is written once and only before the first field: a text
					 * given away in parts must not have the mark in the middle of itself
					 *
					 * \~
					 */
					void mark() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи содержимого поля с обрамлением кавычками
					 *
					 * @param text содержимое записываемого поля
					 *
					 * \~english
					 * @brief Method of writing the content of a field with a framing by quotes
					 * @param text content of the field being written
					 *
					 * \~
					 */
					void quoted(const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи очередного поля записи
					 *
					 * @param text содержимое записываемого поля
					 *
					 * \~english
					 * @brief Method of writing the next field of a record
					 * @param text content of the field being written
					 *
					 * \~
					 */
					void field(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи числового поля записи
					 *
					 * @tparam T тип записываемого значения
					 * @param value записываемое значение
					 *
					 * \~english
					 * @brief Method of writing a numeric field of a record
					 * @tparam T type of the value being written
					 * @param value value being written
					 *
					 * \~
					 */
					template <typename T>
					void number(const T value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод завершения текущей записи
					 *
					 * @details Запись без полей даёт пустую строку, а разбор пустые строки
					 * пропускает: круговой проход такой записи не сохраняет, и это не
					 * упущение, а свойство самой записи CSV
					 *
					 * \~english
					 * @brief Method of completing the current record
					 * @details A record without fields gives an empty line, while the parsing skips the empty
					 * lines: a round trip does not preserve such a record, and this is not
					 * an omission but a property of the CSV record itself
					 *
					 * \~
					 */
					void record() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целой записи полем за полем
					 *
					 * @param fields поля записываемой записи
					 *
					 * \~english
					 * @brief Method of writing a whole record field by field
					 * @param fields fields of the record being written
					 *
					 * \~
					 */
					void record(const vector <string> & fields) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целой записи полем за полем
					 *
					 * @param fields поля записываемой записи
					 *
					 * \~english
					 * @brief Method of writing a whole record field by field
					 * @param fields fields of the record being written
					 *
					 * \~
					 */
					void record(const vector <string_view> & fields) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи текста целиком
					 *
					 * @details Собранное прежде не отбрасывается: записи дописываются к
					 * нему, что позволяет предпослать таблице заголовок, собранный
					 * отдельно
					 *
					 * @param records записываемые записи
					 *
					 * \~english
					 * @brief Method of writing a text in full
					 * @details What has been assembled before is not discarded: the records are appended to
					 * it, which makes it possible to prepend a header assembled separately to a table
					 * @param records records being written
					 *
					 * \~
					 */
					void write(const vector <vector <string>> & records) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения собранного текста
					 *
					 * @return собранный текст
					 *
					 * \~english
					 * @brief Method of getting the assembled text
					 * @return assembled text
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения размера собранного текста
					 *
					 * @return размер собранного текста в байтах
					 *
					 * \~english
					 * @brief Method of getting the size of the assembled text
					 * @return size of the assembled text in bytes
					 *
					 * \~
					 */
					size_t size() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод изъятия собранного текста
					 *
					 * @details Собранное изымается целиком, а сборщик остаётся готовым
					 * продолжать: тем и ведётся потоковая запись, когда держать текст в
					 * памяти целиком незачем
					 *
					 * @warning Изымать текст посреди записи нельзя: изъятое окончится
					 * полем без завершения записи, и склеенное обратно даст запись,
					 * разорванную надвое. Изъятие посреди записи потому отдаёт пустоту
					 *
					 * @return изъятый текст
					 *
					 * \~english
					 * @brief Method of withdrawing the assembled text
					 * @details What has been assembled is withdrawn in full, while the assembler remains ready
					 * to continue: that is how the streaming writing is conducted, when there is no point in holding the text in
					 * the memory in full
					 * @warning The text cannot be withdrawn in the middle of a record: what has been withdrawn would end
					 * with a field without the completion of the record, and glued back it would give a record
					 * torn in two. A withdrawal in the middle of a record therefore gives away emptiness
					 * @return withdrawn text
					 *
					 * \~
					 */
					string take() noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки собранного текста
					 *
					 * \~english
					 * @brief Method of clearing the assembled text
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек записи текста
					 *
					 * @return настройки записи текста
					 *
					 * \~english
					 * @brief Method of getting the settings of the writing of a text
					 * @return settings of the writing of a text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи текста
					 *
					 * @param settings настройки записи текста
					 *
					 * \~english
					 * @brief Method of setting the settings of the writing of a text
					 * @param settings settings of the writing of a text
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Writer(const Logging * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки записи текста
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the writing of a text
					 *
					 * \~
					 */
					Writer(const Logging * log, const settings_t & settings) noexcept;
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
					~Writer() noexcept {}
			} writer_t;
		}
	}
}

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_CSV_WRITER__
