/**
 * @file encoding.hpp
 * @date 2026-08-01
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
 * @brief Заголовочный файл разрядов знаков и кодировок контейнера XML — проверки допустимости знаков разметки,
 *        работа с кодовыми значениями Юникода и класс Decoder, приводящий исходный текст к кодировке UTF-8
 *
 * \~english
 * @brief Header file of the character ranges and the encodings of the XML container — the checks of the admissibility of the markup characters,
 *        the work with the Unicode code values and the Decoder class, which converts the source text to the UTF-8 encoding
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_ENCODING__
#define __AWH_CODEC_XML_ENCODING__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

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
		 * @brief Пространство имён контейнера XML
		 *
		 *
		 * \~english
		 * @brief XML container namespace
		 *
		 * \~
		 */
		namespace xml {
			/**
			 * \~russian
			 * @brief Обозначение кодового значения, полученного ошибочно
			 *
			 * \~english
			 * @brief Designation of a code value obtained erroneously
			 *
			 * \~
			 */
			constexpr uint32_t INVALID_CODEPOINT = static_cast <uint32_t> (~0u);

			/**
			 * \~russian
			 * @brief Метод проверки знака на допустимость в разметке
			 *
			 * @details Договор допускает в тексте разметки лишь горизонтальную табуляцию,
			 * перевод строки и возврат каретки из числа управляющих знаков, а прочие
			 * управляющие знаки, суррогатные значения и два невыделенных значения запрещает
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for admissibility in the markup
			 * @details The specification admits in the text of a markup only the horizontal tabulation,
			 * the line feed and the carriage return out of the control characters, while the other
			 * control characters, the surrogate values and two unassigned values it prohibits
			 * @param code code value of the character being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isChar(const uint32_t code) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки знака на пробельность
			 *
			 * @details Разметка считает пробельными лишь четыре знака, и правила местности
			 * к этому перечню отношения не имеют
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for being a whitespace one
			 * @details A markup considers only four characters whitespace ones, and the rules of the locale
			 * have no relation to that list
			 * @param code code value of the character being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isSpace(const uint32_t code) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки знака на допустимость в начале имени
			 *
			 * @details Перечень допустимых знаков задан договором явными разрядами кодовых
			 * значений, а не разрядами Юникода: сличение ведётся по ним, и таблицы Юникода
			 * для этого не нужны
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for admissibility at the beginning of a name
			 * @details The list of the admissible characters is given by the specification as explicit ranges of the code
			 * values rather than as Unicode ranges: the comparison is conducted by them, and the Unicode tables
			 * are not needed for this
			 * @param code code value of the character being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isNameStart(const uint32_t code) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки знака на допустимость внутри имени
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for admissibility inside a name
			 * @param code code value of the character being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isName(const uint32_t code) noexcept;

			/**
			 * \~russian
			 * @brief Метод чтения кодового значения из текста в кодировке UTF-8
			 *
			 * @details Проверяется не только построение последовательности, но и её
			 * кратчайшесть: избыточно длинная запись кодового значения отвергается как
			 * ошибочная, поскольку служит обходом проверок содержимого
			 *
			 * @param buffer буфер исходного текста в кодировке UTF-8
			 * @param size   размер буфера исходного текста
			 * @param length длина прочитанной последовательности в байтах
			 * @return       прочитанное кодовое значение знака
			 *
			 * \~english
			 * @brief Method of reading a code value from a text in the UTF-8 encoding
			 * @details Not only the construction of the sequence is checked but also its
			 * shortestness: an overlong record of a code value is rejected as
			 * an erroneous one, since it serves as a bypass of the checks of the content
			 * @param buffer buffer of the source text in the UTF-8 encoding
			 * @param size   size of the buffer of the source text
			 * @param length length of the read sequence in bytes
			 * @return       read code value of the character
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint32_t decode(const char * buffer, const size_t size, size_t & length) noexcept;
			/**
			 * \~russian
			 * @brief Метод записи кодового значения в кодировке UTF-8
			 *
			 * @param code   записываемое кодовое значение знака
			 * @param result текст, к которому дописывается знак
			 * @return       результат выполнения операции
			 *
			 *
			 * \~english
			 * @brief Method of writing a code value in the UTF-8 encoding
			 * @param code   code value of the character being written
			 * @param result text to which the character is appended
			 * @return       result of performing the operation
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool encode(const uint32_t code, string & result) noexcept;

			/**
			 * \~russian
			 * @brief Класс приведения исходного текста к кодировке UTF-8
			 *
			 * @details Определяет кодировку исходного текста по метке порядка байтов и по
			 * объявлению разметки, после чего приводит текст к кодировке UTF-8, на которой
			 * работает разбор. Приведение ведётся по кускам: последовательность знака,
			 * разорванная границей куска, удерживается до прихода недостающих байтов
			 *
			 * @note Отдельный слой приведения избавляет разбор от знания о кодировках: он
			 * работает с единственной кодировкой, что и делает его пригодным для
			 * потокового чтения без оглядки на устройство исходного текста
			 *
			 * \~english
			 * @brief Class of the conversion of the source text to the UTF-8 encoding
			 * @details Determines the encoding of the source text by the byte order mark and by
			 * the declaration of the markup, after which it converts the text to the UTF-8 encoding on which
			 * the parsing works. The conversion is conducted by chunks: a character sequence
			 * torn by the boundary of a chunk is held until the missing bytes arrive
			 * @note A separate layer of the conversion relieves the parsing of the knowledge about the encodings: it
			 * works with a single encoding, which is what makes it suitable for
			 * a streaming reading without regard for the arrangement of the source text
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Decoder {
				private:
					// Определённая кодировка исходного текста
					encoding_t _encoding;
				private:
					// Код ошибки последней операции приведения
					error_t _error;
				private:
					// Признак того, что кодировка навязана извне
					bool _forced;
				private:
					// Признак того, что метка порядка байтов уже обработана
					bool _marked;
				private:
					// Флаг обнаружения метки порядка байтов в начале исходного текста
					bool _signed;
				private:
					// Признак того, что приведение текста уже началось
					bool _started;
				private:
					// Байты начала текста, удержанные до определения кодировки
					string _prolog;
				private:
					// Количество удержанных байтов незавершённой последовательности
					uint8_t _length;
				private:
					// Удержанные байты незавершённой последовательности знака
					char _pending[4];
				private:
					// Удержанная старшая половина суррогатной пары
					uint32_t _surrogate;
				public:
					/**
					 * \~russian
					 * @brief Метод получения определённой кодировки исходного текста
					 *
					 * @return определённая кодировка исходного текста
					 *
					 *
					 * \~english
					 * @brief Method of getting the determined encoding of the source text
					 * @return determined encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения признака обнаружения метки порядка байтов
					 *
					 * @details Метка порядка байтов задаёт кодировку исходного текста
					 * достовернее объявления разметки: объявление является частью текста и
					 * прочитано быть не может, пока кодировка неизвестна. Расхождение метки
					 * с объявлением означает подложное объявление, а не выбор между ними
					 *
					 * @return признак обнаружения метки порядка байтов
					 *
					 * \~english
					 * @brief Method of getting the flag of the detection of the byte order mark
					 * @details The byte order mark gives the encoding of the source text
					 * more reliably than the declaration of the markup: the declaration is a part of the text and
					 * cannot be read while the encoding is unknown. A divergence of the mark
					 * from the declaration means a forged declaration rather than a choice between them
					 * @return flag of the detection of the byte order mark
					 *
					 * \~
					 */
					bool signature() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки кодировки исходного текста
					 *
					 * @details Навязывает кодировку вопреки метке порядка байтов и объявлению
					 * разметки. Применяется там, где кодировка известна из внешнего источника
					 * - скажем, из поля «Content-Type» ответа по договору HTTP
					 *
					 * @warning Устанавливается лишь до приведения первого куска: сменить
					 * кодировку посреди текста нельзя, и такое указание отвергается
					 *
					 * @note Кодировка неопределённая снимает навязывание, возвращая приведение
					 * к определению кодировки по метке порядка байтов и объявлению разметки
					 *
					 * @param encoding устанавливаемая кодировка исходного текста
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the encoding of the source text
					 * @details Imposes the encoding contrary to the byte order mark and to the declaration
					 * of the markup. Applied where the encoding is known from an external source
					 * — say, from the «Content-Type» field of an answer over the HTTP protocol
					 * @warning It is set only before the conversion of the first chunk: the encoding cannot be
					 * changed in the middle of a text, and such an indication is rejected
					 * @note An undefined encoding removes the imposition, returning the conversion to the
					 * determination of the encoding by the byte order mark and by the declaration of the markup
					 * @param encoding encoding of the source text being set
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool encoding(const encoding_t encoding) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод приведения куска исходного текста к кодировке UTF-8
					 *
					 * @details Первый кусок определяет кодировку по метке порядка байтов,
					 * если она не навязана извне. Приведённый текст дописывается к
					 * переданному, а не замещает его
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @param result текст, к которому дописывается приведённый кусок
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of converting a chunk of the source text to the UTF-8 encoding
					 * @details The first chunk determines the encoding by the byte order mark,
					 * if it has not been imposed from the outside. The converted text is appended to
					 * what has been passed rather than replacing it
					 * @param buffer buffer of the next chunk of the source text
					 * @param size   size of the buffer of the next chunk of the source text
					 * @param end    flag of the chunk being the last one
					 * @param result text to which the converted chunk is appended
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool convert(const void * buffer, const size_t size, const bool end, string & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки приведения
					 *
					 * @return код ошибки последней операции приведения
					 *
					 * \~english
					 * @brief Method of getting the error code of the conversion
					 * @return error code of the last operation of the conversion
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса приведения в исходное состояние
					 *
					 * \~english
					 * @brief Method of resetting the conversion into the initial state
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
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
					Decoder() noexcept;
			} decoder_t;
		};
	};
};

#endif // __AWH_CODEC_XML_ENCODING__
