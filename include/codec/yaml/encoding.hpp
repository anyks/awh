/**
 * @file encoding.hpp
 * @date 2026-08-17
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
 * @brief Заголовочный файл приведения кодировок контейнера YAML — разбор
 *        последовательностей UTF-8, опознание кодировки по метке порядка байтов и
 *        потоковое приведение исходного текста к UTF-8
 *
 * \~english
 * @brief Header file of the conversion of the encodings of the YAML container — the parsing
 *        of the UTF-8 sequences, the recognition of the encoding by the byte order mark and
 *        the streaming conversion of the source text to UTF-8
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML_ENCODING__
#define __AWH_CODEC_YAML_ENCODING__

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
	 * Подписываемся на стандартное пространство имён
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
		 * @brief Пространство имён контейнера YAML
		 *
		 * \~english
		 * @brief YAML container namespace
		 *
		 * \~
		 */
		namespace yaml {
			/**
			 * \~russian
			 * @brief Наибольшее кодовое значение знака Юникода
			 *
			 * \~english
			 * @brief Largest code value of a Unicode character
			 *
			 * \~
			 */
			constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

			/**
			 * \~russian
			 * @brief Исходы разбора последовательности UTF-8
			 *
			 * @details Исходов три, а не два, и третий из них - главный для потока:
			 * последовательность, оборванная концом поданного куска, ошибочной **не
			 * является**, и разбор обязан дождаться продолжения. Смешение оборванной
			 * последовательности с ошибочной ставит исход разбора в зависимость от того,
			 * как текст нарезан на куски
			 *
			 * \~english
			 * @brief Outcomes of the parsing of a UTF-8 sequence
			 * @details There are three outcomes rather than two, and the third of them is the main one for a stream:
			 * a sequence cut off by the end of a fed chunk is **not** erroneous,
			 * and the parsing must wait for the continuation. A confusion of a cut-off sequence
			 * with an erroneous one puts the outcome of the parsing into a dependence on how
			 * the text is cut into the chunks
			 *
			 * \~
			 */
			enum class utf8_t : uint8_t {
				VALID     = 0x01, // Последовательность прочитана целиком и правила соблюдает
				BROKEN    = 0x02, // Последовательность построена ошибочно
				TRUNCATED = 0x03  // Последовательности не хватает байт до конца текста
			};

			/**
			 * \~russian
			 * @brief Функция получения длины последовательности UTF-8 по ведущему байту
			 *
			 * @param leading ведущий байт последовательности
			 * @return        количество байт последовательности, ноль - байт построен ошибочно
			 *
			 * \~english
			 * @brief Function of the obtaining of the length of a UTF-8 sequence by the leading byte
			 * @param leading leading byte of the sequence
			 * @return number of the bytes of the sequence, a zero — the byte is constructed erroneously
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t sequence(const uint8_t leading) noexcept;
			/**
			 * \~russian
			 * @brief Функция разбора очередной последовательности UTF-8
			 *
			 * @details Разбор ведётся одним телом на весь кодек: длина по ведущему байту,
			 * проверка продолжающих байтов, запрет записи длиннее необходимой и запрет
			 * суррогатов. Второе такое тело разошлось бы с первым при первой же правке
			 *
			 * @param text   последовательность знаков, из которой ведётся чтение
			 * @param offset положение начала знака в последовательности
			 * @param code   получаемый знак Юникода, при неудачном разборе не выставляется
			 * @param length количество байт, разбором пройденных, при нехватке байт - число байт налицо
			 * @return       исход разбора последовательности
			 *
			 * \~english
			 * @brief Function of the parsing of the next UTF-8 sequence
			 * @details The parsing is conducted by one body for the whole codec: the length by the leading byte,
			 * the check of the continuation bytes, the prohibition of a record longer than necessary and the prohibition
			 * of the surrogates. A second such body would diverge from the first one at the very first editing
			 * @param text sequence of the characters from which the reading is conducted
			 * @param offset position of the beginning of the character in the sequence
			 * @param code obtained Unicode character, at an unsuccessful parsing it is not set
			 * @param length number of the bytes passed by the parsing, at a shortage of the bytes — the number of the bytes at hand
			 * @return outcome of the parsing of the sequence
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ utf8_t inspect(const string_view text, const size_t offset, uint32_t & code, size_t & length) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения очередного знака Юникода последовательности UTF-8
			 *
			 * @param text   последовательность знаков, из которой ведётся чтение
			 * @param offset положение начала знака в последовательности
			 * @param code   получаемый знак Юникода
			 * @return       количество байт знака, ноль - знак битый либо оборванный
			 *
			 * \~english
			 * @brief Function of the obtaining of the next Unicode character of a UTF-8 sequence
			 * @param text sequence of the characters from which the reading is conducted
			 * @param offset position of the beginning of the character in the sequence
			 * @param code obtained Unicode character
			 * @return number of the bytes of the character, a zero — the character is broken or cut off
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t decode(const string_view text, const size_t offset, uint32_t & code) noexcept;
			/**
			 * \~russian
			 * @brief Функция записи знака Юникода последовательностью UTF-8
			 *
			 * @param code   записываемый знак Юникода
			 * @param result строка, куда дописывается последовательность
			 * @return       признак успешной записи знака
			 *
			 * \~english
			 * @brief Function of the writing of a Unicode character by a UTF-8 sequence
			 * @param code Unicode character being written
			 * @param result string to which the sequence is appended
			 * @return sign of the successful writing of the character
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool encode(const uint32_t code, string & result) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки знака на принадлежность к печатным
			 *
			 * @details Описание дозволяет в тексте лишь печатные знаки: горизонтальную
			 * подачу, перевод строки, возврат каретки, знаки от пробела и выше, а из
			 * управляющих второго набора - лишь знак смены строки. Прочие управляющие знаки
			 * записываются отменяющими последовательностями внутри двойной ограды
			 *
			 * @param code проверяемый знак Юникода
			 * @return     признак принадлежности знака к печатным
			 *
			 * \~english
			 * @brief Function of the checking of a character for the belonging to the printable ones
			 * @details The specification permits in a text only the printable characters: the horizontal
			 * tabulation, the line feed, the carriage return, the characters from the space and above, while of
			 * the controls of the second set — only the next line character. The other control characters
			 * are written by the escape sequences inside a double quoting
			 * @param code Unicode character being checked
			 * @return sign of the belonging of the character to the printable ones
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool printable(const uint32_t code) noexcept;

			/**
			 * \~russian
			 * @brief Приведение исходного текста к кодировке UTF-8
			 *
			 * @details Опознаёт кодировку по метке порядка байтов, а при её отсутствии - по
			 * расположению нулевых байтов в первых четырёх октетах, и приводит поданный текст
			 * к UTF-8 кусками, не требуя его целиком
			 *
			 * @note Текст в UTF-8 приведения не требует вовсе, и приводить его копированием
			 *       значило бы платить за него памятью и временем на всяком куске. Признак
			 *       `direct()` о том и сообщает: чтение вправе разбирать поданный кусок на
			 *       месте
			 *
			 * \~english
			 * @brief Conversion of the source text to the UTF-8 encoding
			 * @details It recognises the encoding by the byte order mark, while at its absence — by
			 * the arrangement of the zero bytes in the first four octets, and converts the fed text
			 * to UTF-8 by the chunks without demanding it in full
			 * @note A text in UTF-8 does not require a conversion at all, and to convert it by a copying
			 *       would mean to pay for it by the memory and by the time at every chunk. The sign
			 *       `direct()` reports about that: the reading may parse the fed chunk in
			 *       place
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Decoder {
				private:
					/**
					 * \~russian
					 * @brief Метод вывода сообщения об отказе в лог
					 *
					 * @details Код отказа остаётся доступен потребителю через error(): журнал
					 * его не заменяет, а лишь оповещает о случившемся
					 *
					 * \~english
					 * @brief Method of the output of the message about a refusal into the log
					 * @details The code of the refusal remains available to the consumer through error():
					 * the log does not replace it but merely notifies about what has happened
					 *
					 * \~
					 */
					void report() const noexcept;
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
				private:
					// Опознанная кодировка исходного текста
					encoding_t _encoding;
					// Код ошибки приведения кодировки
					error_t _error;
					// Признак того, что кодировка уже опознана
					bool _sniffed;
					// Признак того, что текст открывался меткой порядка байтов
					bool _signature;
					// Признак того, что кодировка навязана извне
					bool _forced;
					// Признак того, что начало текста ещё не пройдено
					bool _leading;
					// Старший суррогат, ожидающий пары своей
					uint32_t _surrogate;
					// Байты, оставшиеся от прошлого куска до полного знака
					string _tail;
				private:
					/**
					 * \~russian
					 * @brief Метод опознания кодировки по началу исходного текста
					 *
					 * @param buffer начало исходного текста
					 * @param size   размер начала исходного текста
					 * @param end    признак того, что текст окончен
					 * @return       признак того, что кодировка опознана
					 *
					 * \~english
					 * @brief Method of the recognition of the encoding by the beginning of the source text
					 * @param buffer beginning of the source text
					 * @param size size of the beginning of the source text
					 * @param end sign of the fact that the text is ended
					 * @return sign of the fact that the encoding is recognised
					 *
					 * \~
					 */
					bool sniff(const char * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки знака на дозволенность его в тексте
					 *
					 * @details Описание дозволяет тексту одни лишь печатные знаки, и знак иной
					 * есть отказ приведения, а не содержимое: пропустив его, приведение выдало
					 * бы разбору знак, которого в тексте YAML быть не может вовсе
					 *
					 * @note Проверка эта стоит именно здесь, а не в разборе: приведение есть
					 *       единственное место, через которое проходит всякий знак текста любой
					 *       кодировки, и стеречь в одном месте вернее, чем в четырёх
					 *
					 * @param code проверяемый знак Юникода
					 * @return     признак дозволенности знака в тексте
					 *
					 * \~english
					 * @brief Method of the checking of a character for the permissibility of it in a text
					 * @details The specification permits in a text only the printable characters, and another
					 * character is a refusal of the conversion rather than a content: having let it through, the conversion
					 * would issue to the parsing a character which cannot be in a YAML text at all
					 * @param code character of the Unicode being checked
					 * @return sign of the permissibility of the character in a text
					 *
					 * \~
					 */
					bool allowed(const uint32_t code) noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения куска текста, записанного парами байтов
					 *
					 * @param buffer приводимый кусок исходного текста
					 * @param size   размер приводимого куска
					 * @param end    признак того, что текст окончен
					 * @param result строка, куда дописывается приведённый текст
					 * @return       признак успешного приведения куска
					 *
					 * \~english
					 * @brief Method of the conversion of a chunk of a text written by the pairs of the bytes
					 * @param buffer chunk of the source text being converted
					 * @param size size of the chunk being converted
					 * @param end sign of the fact that the text is ended
					 * @param result string to which the converted text is appended
					 * @return sign of the successful conversion of the chunk
					 *
					 * \~
					 */
					bool doubled(const char * buffer, const size_t size, const bool end, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения куска текста, записанного четвёрками байтов
					 *
					 * @param buffer приводимый кусок исходного текста
					 * @param size   размер приводимого куска
					 * @param end    признак того, что текст окончен
					 * @param result строка, куда дописывается приведённый текст
					 * @return       признак успешного приведения куска
					 *
					 * \~english
					 * @brief Method of the conversion of a chunk of a text written by the quadruples of the bytes
					 * @param buffer chunk of the source text being converted
					 * @param size size of the chunk being converted
					 * @param end sign of the fact that the text is ended
					 * @param result string to which the converted text is appended
					 * @return sign of the successful conversion of the chunk
					 *
					 * \~
					 */
					bool quadrupled(const char * buffer, const size_t size, const bool end, string & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения опознанной кодировки исходного текста
					 *
					 * @return опознанная кодировка исходного текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the recognised encoding of the source text
					 * @return recognised encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод навязывания кодировки исходного текста
					 *
					 * @details Навязанная кодировка старше опознания: текст без метки порядка
					 * байтов опознаётся по расположению нулевых байтов, а опознание это
					 * ошибается на тексте, чей первый знак записан одним байтом, а второй -
					 * многими
					 *
					 * @param encoding навязываемая кодировка исходного текста
					 * @return         признак того, что кодировка принята
					 *
					 * \~english
					 * @brief Method of the imposing of the encoding of the source text
					 * @details An imposed encoding is senior to the recognition: a text without a byte order
					 * mark is recognised by the arrangement of the zero bytes, and this recognition
					 * errs on a text whose first character is written by one byte while the second one — by
					 * many
					 * @param encoding encoding of the source text being imposed
					 * @return sign of the fact that the encoding is accepted
					 *
					 * \~
					 */
					bool encoding(const encoding_t encoding) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения признака открытия текста меткой порядка байтов
					 *
					 * @return признак открытия текста меткой порядка байтов
					 *
					 * \~english
					 * @brief Method of the obtaining of the sign of the opening of a text by a byte order mark
					 * @return sign of the opening of the text by a byte order mark
					 *
					 * \~
					 */
					bool signature() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения признака ненадобности приведения
					 *
					 * @return признак того, что текст записан UTF-8 и приведения не требует
					 *
					 * \~english
					 * @brief Method of the obtaining of the sign of the needlessness of a conversion
					 * @return sign of the fact that the text is written in UTF-8 and does not require a conversion
					 *
					 * \~
					 */
					bool direct() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки приведения кодировки
					 *
					 * @return код ошибки приведения кодировки
					 *
					 * \~english
					 * @brief Method of the obtaining of the error code of the conversion of the encoding
					 * @return error code of the conversion of the encoding
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения очередного куска исходного текста к UTF-8
					 *
					 * @details Кусок, оборвавший знак посередине, приводится не целиком:
					 * недостающие байты удерживаются до прихода следующего куска. Оттого исход
					 * приведения не зависит от того, как текст нарезан
					 *
					 * @param buffer приводимый кусок исходного текста
					 * @param size   размер приводимого куска
					 * @param end    признак того, что текст окончен
					 * @param result строка, куда дописывается приведённый текст
					 * @return       признак успешного приведения куска
					 *
					 * \~english
					 * @brief Method of the conversion of the next chunk of the source text to UTF-8
					 * @details A chunk which has cut off a character in the middle is converted not in full:
					 * the missing bytes are held until the arrival of the next chunk. Whereby the outcome
					 * of the conversion does not depend on how the text is cut
					 * @param buffer chunk of the source text being converted
					 * @param size size of the chunk being converted
					 * @param end sign of the fact that the text is ended
					 * @param result string to which the converted text is appended
					 * @return sign of the successful conversion of the chunk
					 *
					 * \~
					 */
					bool convert(const void * buffer, const size_t size, const bool end, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния приведения кодировки
					 *
					 * \~english
					 * @brief Method of the reset of the state of the conversion of the encoding
					 *
					 * \~
					 */
					void reset() noexcept;
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
					Decoder(const Logging * log) noexcept;
			} decoder_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_YAML_ENCODING__
