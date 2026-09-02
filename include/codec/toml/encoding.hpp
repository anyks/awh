/**
 * @file encoding.hpp
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
 * @brief Заголовочный файл приведения исходного текста настроек TOML к кодировке UTF-8 —
 *        класс Decoder, определяющий кодировку по метке порядка байтов и приводящий текст
 *        кусками произвольного размера
 *
 * \~english
 * @brief Header file of the conversion of the source text of the TOML settings to the UTF-8 encoding —
 *        the Decoder class, which determines the encoding by the byte order mark and converts the text
 *        by chunks of an arbitrary size
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_TOML_ENCODING__
#define __AWH_CODEC_TOML_ENCODING__

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
		 * @brief Пространство имён контейнера TOML
		 *
		 *
		 * \~english
		 * @brief TOML container namespace
		 *
		 * \~
		 */
		namespace toml {
			/**
			 * \~russian
			 * @brief Метод проверки знака на допустимость в тексте настроек
			 *
			 * @details Описание отвергает управляющие знаки области C0 сырыми, дозволяя
			 * из них лишь горизонтальную табуляцию, перевод строки и возврат каретки, и
			 * отвергает знак забоя. Область C1 при этом дозволена: описание её к
			 * управляющим не относит, и отвергать её значило бы отвергать законный текст
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for admissibility in the text of the settings
			 * @details The specification rejects the control characters of the C0 area in a raw form, permitting
			 * of them only the horizontal tabulation, the line feed and the carriage return, and
			 * it rejects the backspace character. The C1 area is permitted thereby: the specification does not consider it
			 * a control one, and to reject it would mean to reject a lawful text
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
			 * @param buffer буфер исходного текста в кодировке UTF-8
			 * @param size   размер буфера исходного текста
			 * @param length длина прочитанной последовательности в байтах
			 * @return       прочитанное кодовое значение знака
			 *
			 * \~english
			 * @brief Method of reading a code value from a text in the UTF-8 encoding
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
			 * @brief Класс приведения исходного текста настроек к кодировке UTF-8
			 *
			 * @details Кодировка определяется меткой порядка байтов в начале текста, а при
			 * её отсутствии текстом считается UTF-8: описание отводит записи TOML именно
			 * его. Приведение принимает текст кусками произвольного размера и удерживает
			 * последовательность знака, разорванную границей куска
			 *
			 * @warning Выдача приведения не зависит от того, как исходный текст нарезан на
			 * куски: отказ откладывается до исчерпания уже приведённого начала, чтобы
			 * подача кусками теряла не больше событий, чем подача целиком
			 *
			 * \~english
			 * @brief Class of the conversion of the source text of the settings to the UTF-8 encoding
			 * @details The encoding is determined by the byte order mark at the beginning of the text, and in
			 * its absence the text is considered UTF-8: the specification allots exactly it to a TOML
			 * record. The conversion accepts the text by chunks of an arbitrary size and holds
			 * a character sequence torn by the boundary of a chunk
			 * @warning The output of the conversion does not depend on how the source text is cut into
			 * chunks: a refusal is postponed until the already converted beginning is exhausted, so that
			 * a feeding by chunks loses no more events than a feeding in full
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
					const log_t * _log;
				private:
					// Определённая кодировка исходного текста
					encoding_t _encoding;
				private:
					// Код ошибки последней операции приведения
					error_t _error;
				private:
					// Признак навязанной извне кодировки
					bool _forced;
					// Признак обработки метки порядка байтов
					bool _marked;
					// Признак обнаружения метки порядка байтов
					bool _signed;
					// Признак начала приведения текста
					bool _started;
				private:
					// Удержанное начало текста до определения кодировки
					string _prolog;
				private:
					// Количество удержанных байтов незавершённой последовательности
					size_t _length;
					// Удержанные байты незавершённой последовательности знака
					char _pending[4];
				private:
					// Удержанная старшая половина суррогатной пары
					uint16_t _surrogate;
				private:
					/**
					 * \~russian
					 * @brief Метод определения кодировки по метке порядка байтов
					 *
					 * @return признак того, что определение кодировки завершено
					 *
					 * \~english
					 * @brief Method of determining the encoding by the byte order mark
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
					 * @param encoding устанавливаемая кодировка исходного текста
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the encoding of the source text
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
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @param result текст, к которому дописывается приведённый кусок
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of converting a chunk of the source text to the UTF-8 encoding
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
				public:
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
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Decoder(const log_t * log) noexcept;
			} decoder_t;
		};
	};
};

#endif // __AWH_CODEC_TOML_ENCODING__
