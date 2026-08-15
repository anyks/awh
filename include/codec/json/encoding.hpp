/**
 * @file encoding.hpp
 * @date 2026-08-14
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
 * @brief Заголовочный файл приведения исходного текста к кодировке UTF-8 —
 *        определение кодировки по метке порядка байтов, чтение и запись знаков Юникода,
 *        класс Decoder кусочного приведения текста
 *
 * \~english
 * @brief Header file of the conversion of the source text to the UTF-8 encoding —
 *        the determination of the encoding by the byte order mark, the reading and the writing of the Unicode characters,
 *        the Decoder class of the chunked conversion of the text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_JSON_ENCODING__
#define __AWH_CODEC_JSON_ENCODING__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>

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
		 * @brief Пространство имён контейнера JSON
		 *
		 * \~english
		 * @brief JSON container namespace
		 *
		 * \~
		 */
		namespace json {
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
			 * @brief Метод проверки знака на допустимость в тексте настроек
			 *
			 * @details Из управляющих знаков допускаются лишь горизонтальная табуляция,
			 * перевод строки и возврат каретки: прочие в файле настроек означают либо
			 * двоичный мусор, попавший туда по ошибке, либо попытку скрыть содержимое от
			 * читающего глазами
			 *
			 * @note Знак забоя и прочие управляющие знаки отвергаются даже внутри кавычек:
			 * записать их в значение можно управляющей последовательностью, и путь этот
			 * оставляет запись обозримой
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for admissibility in the text of the settings
			 * @details Of the control characters only the horizontal tabulation,
			 * the line feed and the carriage return are admitted: the rest of them in a settings file mean either
			 * binary rubbish that has got there by mistake or an attempt to hide the content from
			 * the one reading with the eyes
			 * @note The backspace character and the other control characters are rejected even inside the quotes:
			 * they can be written into a value by an escape sequence, and that path
			 * leaves the record surveyable
			 * @param code code value of the character being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isChar(const uint32_t code) noexcept;

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
			 * @details Определяет кодировку исходного текста по метке порядка байтов, после
			 * чего приводит текст к кодировке UTF-8, на которой работает разбор. Приведение
			 * ведётся по кускам: последовательность знака, разорванная границей куска,
			 * удерживается до прихода недостающих байтов
			 *
			 * @note Текст, в отличие от разметки XML, кодировку свою не объявляет:
			 * определить её можно лишь по метке порядка байтов, а при её отсутствии -
			 * принять UTF-8. Кодировку, известную из внешнего источника, следует навязать
			 * настройками разбора
			 *
			 * \~english
			 * @brief Class of the conversion of the source text to the UTF-8 encoding
			 * @details Determines the encoding of the source text by the byte order mark, after
			 * which it converts the text to the UTF-8 encoding on which the parsing works. The conversion
			 * is conducted by chunks: a character sequence torn by the boundary of a chunk
			 * is held until the missing bytes arrive
			 * @note A text, unlike an XML markup, does not announce its encoding:
			 * it can be determined only by the byte order mark, and in its absence —
			 * UTF-8 is assumed. An encoding known from an external source should be imposed
			 * by the settings of the parsing
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Decoder {
				public:
					/**
					 * \~russian
					 * @brief Кусок текста, проверенного на месте
					 *
					 * @details Проверка на месте текста, уже отвечающего кодировке UTF-8, ничего
					 * никуда не переносит, а лишь указывает на проверенное. Кусок состоит из двух
					 * частей: доведённой последовательности знака, разорванного границей
					 * предыдущего куска, и отрезка внутри поданного буфера
					 *
					 * @warning Указания живут лишь до следующего вызова проверки, а отрезок тела -
					 * лишь пока жив поданный буфер
					 *
					 * \~english
					 * @brief Chunk of a text verified in place
					 * @details The verification in place of a text already conforming to the UTF-8 encoding moves
					 * nothing anywhere, but merely points at what has been verified. The chunk consists of two
					 * parts: the completed sequence of a character torn by the boundary of the
					 * previous chunk, and a segment inside the passed buffer
					 * @warning The pointers live only until the next call of the verification, and the segment of the body -
					 * only while the passed buffer is alive
					 *
					 * \~
					 */
					typedef struct Chunk {
						// Буфер доведённой последовательности знака
						const char * head;
						// Размер доведённой последовательности знака
						size_t count;
						// Буфер отрезка проверенного текста
						const char * body;
						// Размер отрезка проверенного текста
						size_t length;
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
						Chunk() noexcept : head(nullptr), count(0), body(nullptr), length(0) {}
					} chunk_t;
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
					// Признак обнаружения метки порядка байтов в начале исходного текста
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
					/**
					 * Байты последовательности знака, доведённой очередным куском
					 *
					 * @note Буфер этот отдельный от удержанных байтов намеренно: доведённая
					 *       последовательность выдаётся указанием наружу, а проверка остатка
					 *       того же куска успевает удержать в буфере удержанных байтов новый
					 *       разорванный знак
					 */
					char _complete[4];
				private:
					// Удержанная старшая половина суррогатной пары
					uint32_t _surrogate;
				private:
					/**
					 * \~russian
					 * @brief Метод определения кодировки по метке порядка байтов
					 *
					 * @details Метка снимается с начала удержанных байтов, а при её отсутствии
					 * кодировкой принимается UTF-8
					 *
					 * @return признак того, что определение кодировки завершено
					 *
					 * \~english
					 * @brief Method of determining the encoding by the byte order mark
					 * @details The mark is taken from the beginning of the held bytes, and in its absence
					 * UTF-8 is assumed as the encoding
					 * @return flag of the determination of the encoding being completed
					 *
					 * \~
					 */
					bool sniff() noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения удержанных байтов к кодировке UTF-8
					 *
					 * @param buffer буфер приводимых байтов исходного текста
					 * @param size   размер буфера приводимых байтов исходного текста
					 * @param end    признак того, что приводимые байты являются последними
					 * @param result текст, к которому дописывается приведённое
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of converting the held bytes to the UTF-8 encoding
					 * @param buffer buffer of the bytes of the source text being converted
					 * @param size   size of the buffer of the bytes of the source text being converted
					 * @param end    flag of the bytes being converted being the last ones
					 * @param result text to which what has been converted is appended
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool process(const char * buffer, const size_t size, const bool end, string & result) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод проверки правильности построения знаков кодировки UTF-8
					 *
					 * @details Ничего никуда не переносит: продвигает положение по проверенному
					 * тексту. Последовательность знака, разорванная границей куска, удерживается
					 * до следующего куска, а положение остаётся концом проверенного
					 *
					 * @param buffer буфер проверяемых байтов исходного текста
					 * @param size   размер буфера проверяемых байтов исходного текста
					 * @param offset положение разбираемого байта в переданном буфере
					 * @param end    признак того, что проверяемые байты являются последними
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the verification of the correctness of the construction of the characters of the UTF-8 encoding
					 * @details Moves nothing anywhere: advances the position over the verified
					 * text. A sequence of a character torn by the boundary of a chunk is held
					 * until the next chunk, and the position remains the end of what has been verified
					 * @param buffer buffer of the bytes of the source text being verified
					 * @param size   size of the buffer of the bytes of the source text being verified
					 * @param offset position of the byte being parsed in the passed buffer
					 * @param end    flag of the bytes being verified being the last ones
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool scan(const char * buffer, const size_t size, size_t & offset, const bool end) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения определённой кодировки исходного текста
					 *
					 * @return определённая кодировка исходного текста
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
					 * @return признак обнаружения метки порядка байтов
					 *
					 * \~english
					 * @brief Method of getting the flag of the detection of the byte order mark
					 * @return flag of the detection of the byte order mark
					 *
					 * \~
					 */
					bool signature() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки кодировки исходного текста
					 *
					 * @details Навязывает кодировку вопреки метке порядка байтов. Применяется
					 * там, где кодировка известна из внешнего источника - скажем, из имени
					 * местности системы либо из поля ответа по договору HTTP
					 *
					 * @warning Устанавливается лишь до приведения первого куска: сменить
					 * кодировку посреди текста нельзя, и такое указание отвергается
					 *
					 * @param encoding устанавливаемая кодировка исходного текста
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the encoding of the source text
					 * @details Imposes the encoding contrary to the byte order mark. Applied
					 * where the encoding is known from an external source — say, from the name
					 * of the locale of the system or from a field of an answer over the HTTP protocol
					 * @warning It is set only before the conversion of the first chunk: the encoding cannot be
					 * changed in the middle of a text, and such an indication is rejected
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
					 * @brief Метод получения признака возможности проверки текста на месте
					 *
					 * @details Проверять текст на месте можно тогда, когда кодировка его уже
					 * определена и оказалась UTF-8: приводить в таком случае нечего, и остаётся
					 * лишь удостовериться в правильности построения знаков
					 *
					 * @return признак возможности проверки текста на месте
					 *
					 * \~english
					 * @brief Method of getting the flag of the possibility of the verification of the text in place
					 * @details The text can be verified in place when its encoding has already been
					 * determined and turned out to be UTF-8: there is nothing to convert in such a case, and there
					 * remains only to make sure of the correctness of the construction of the characters
					 * @return flag of the possibility of the verification of the text in place
					 *
					 * \~
					 */
					bool direct() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки куска исходного текста на месте
					 *
					 * @details Ничего никуда не переносит: проверяет правильность построения знаков
					 * и указывает на проверенное внутри поданного буфера. Последовательность знака,
					 * разорванная границей куска, удерживается до следующего куска, как и при приведении
					 *
					 * @warning Вызывается лишь тогда, когда проверка на месте возможна
					 *
					 * @note Кусок заполняется и при отказе: проверенное прежде места отказа обязано
					 * быть разобрано, иначе положение отказа зависело бы от нарезки текста на куски
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @param chunk  кусок, заполняемый указаниями на проверенное
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the verification of a chunk of the source text in place
					 * @details Moves nothing anywhere: verifies the correctness of the construction of the characters
					 * and points at what has been verified inside the passed buffer. A sequence of a character
					 * torn by the boundary of a chunk is held until the next chunk, as with the conversion
					 * @warning It is called only when the verification in place is possible
					 * @note The chunk is filled in on a failure as well: what has been verified before the place of the
					 * failure must be parsed, otherwise the position of the failure would depend on the cutting of the
					 * text into chunks
					 * @param buffer buffer of the next chunk of the source text
					 * @param size   size of the buffer of the next chunk of the source text
					 * @param end    flag of the chunk being the last one
					 * @param chunk  chunk filled in with the pointers at what has been verified
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool verify(const void * buffer, const size_t size, const bool end, chunk_t & chunk) noexcept;
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

#endif // __AWH_CODEC_JSON_ENCODING__
