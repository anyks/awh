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
#ifndef __AWH_CODEC_CSV_ENCODING__
#define __AWH_CODEC_CSV_ENCODING__

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
		 * @brief Пространство имён контейнера CSV
		 *
		 * \~english
		 * @brief CSV container namespace
		 *
		 * \~
		 */
		namespace csv {
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
			 * @brief Принудительная подстановка суждения о знаке
			 *
			 * @details Суждение о годности знака и чтение его кодового значения зовутся на
			 * КАЖДЫЙ байт всякого поля - и при разборе, и при записи. Оставленные вызовами
			 * через границу единиц трансляции, они стоят дороже самой работы: замер
			 * 05.09.2026 показал 394 МБ/с записи против 768 при снятой проверке вовсе, а
			 * встраивание разбора вернуло 581
			 *
			 * @note Приём этот - принятое в AWH исключение из правила о чистых заголовочных
			 *       файлах, тот же, каким пользуется кодек JSON: реализация живёт в `.cpp`,
			 *       а для встраивания заводятся посредники с `always_inline`
			 *
			 * @warning Определение суждения ОДНО: открытые `isChar()` и `decode()` сведены
			 *          к этим же посредникам и своих тел не несут. Два тела разошлись бы со
			 *          временем, и запись стала бы судить иначе, чем разбор
			 *
			 * @warning Звать посредников ВНУТРИ `encoding.cpp` не надо: там суждение и так
			 *          лежит в одной единице трансляции, и собиратель встраивает его сам,
			 *          по своему разумению. Принудительная же подстановка там ВРЕДИТ -
			 *          замер 05.09.2026 дал 131 МБ/с чтения крупной таблицы против 153, и
			 *          проверено повтором. Посредники эти - для ЧУЖИХ единиц трансляции,
			 *          где вызов иначе уходит через границу
			 *
			 * \~english
			 * @brief Forced inlining of the judgement about a character
			 * @note This device is an accepted exception in AWH from the rule about clean header files
			 *
			 * \~
			 */
			#if defined(_MSC_VER)
				#define AWH_CSV_INLINE __forceinline
			#else
				#define AWH_CSV_INLINE inline __attribute__((always_inline))
			#endif

			/**
			 * \~russian
			 * @brief Встраиваемый посредник получения длины последовательности знака
			 *
			 * @param letter первый байт последовательности знака
			 * @return       длина последовательности знака в байтах, нулевая при ошибке
			 *
			 * \~english
			 * @brief Inline mediator of the getting of the length of the sequence of a character
			 * @param letter first byte of the sequence of the character
			 * @return       length of the sequence of the character in bytes, zero on an error
			 *
			 * \~
			 */
			AWH_CSV_INLINE size_t sequence(const uint8_t letter) noexcept {
				// Если знак записан одним байтом
				if(letter < 0x80)
					// Выводим длину последовательности знака
					return 1;
				// Если знак записан двумя байтами
				if((letter & 0xE0) == 0xC0)
					// Выводим длину последовательности знака
					return 2;
				// Если знак записан тремя байтами
				if((letter & 0xF0) == 0xE0)
					// Выводим длину последовательности знака
					return 3;
				// Если знак записан четырьмя байтами
				if((letter & 0xF8) == 0xF0)
					// Выводим длину последовательности знака
					return 4;
				// Выводим признак ошибочно построенного первого байта
				return 0;
			}

			/**
			 * \~russian
			 * @brief Встраиваемый посредник суждения о допустимости знака
			 *
			 * @param code кодовое значение проверяемого знака
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Inline mediator of the judgement about the admissibility of a character
			 * @param code code value of the character being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			AWH_CSV_INLINE bool suitableChar(const uint32_t code) noexcept {
				// Если знаком является табуляция, перевод строки либо возврат каретки
				if((code == 0x09) || (code == 0x0A) || (code == 0x0D))
					// Выводим положительный результат проверки знака
					return true;
				// Если знак принадлежит управляющим знакам области C0
				if(code < 0x20)
					// Выводим отрицательный результат проверки знака
					return false;
				// Если знаком является забой либо знак области C1
				if((code >= 0x7F) && (code <= 0x9F))
					// Выводим отрицательный результат проверки знака
					return false;
				// Если значение суррогатное либо выходит за пределы Юникода
				if(((code >= 0xD800) && (code <= 0xDFFF)) || (code > MAX_CODEPOINT))
					// Выводим отрицательный результат проверки знака
					return false;
				// Выводим положительный результат проверки знака
				return true;
			}

			/**
			 * \~russian
			 * @brief Встраиваемый посредник чтения кодового значения из текста UTF-8
			 *
			 * @param buffer буфер исходного текста в кодировке UTF-8
			 * @param size   размер буфера исходного текста
			 * @param length длина прочитанной последовательности в байтах
			 * @return       прочитанное кодовое значение знака
			 *
			 * \~english
			 * @brief Inline mediator of the reading of a code value from a UTF-8 text
			 * @param buffer buffer of the source text in the UTF-8 encoding
			 * @param size   size of the buffer of the source text
			 * @param length length of the read sequence in bytes
			 * @return       read code value of the character
			 *
			 * \~
			 */
			AWH_CSV_INLINE uint32_t readCode(const char * buffer, const size_t size, size_t & length) noexcept {
				// Выполняем сброс длины прочитанной последовательности
				length = 0;
				// Если исходный текст не передан
				if((buffer == nullptr) || (size == 0))
					// Выводим обозначение ошибочного кодового значения
					return INVALID_CODEPOINT;
				// Получаем исходный текст в беззнаковом виде
				const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
				// Если знак записан одним байтом
				if(data[0] < 0x80){
					// Запоминаем длину прочитанной последовательности
					length = 1;
					// Выводим прочитанное кодовое значение знака
					return data[0];
				}
				/**
				 * Если знак записан двумя байтами, читаем его ОТДЕЛЬНОЙ дорогой
				 *
				 * @details Дорога эта - та же самая, лишь развёрнутая: два байта не требуют
				 * ни разбора длины таблицей, ни цикла по продолжающим байтам. Двумя байтами
				 * записана вся кириллица, и на ней дорога эта решает: замер 05.09.2026 дал
				 * 581 МБ/с записи против 508 общим путём
				 *
				 * @note Избыточно длинная запись отвергается здесь тем же порогом `0x80`,
				 *       каким её отвергает общая дорога ниже; суррогатных значений двумя
				 *       байтами не записать вовсе, и порога Юникода тут не нужно
				 */
				if((data[0] & 0xE0) == 0xC0){
					// Если переданного текста для чтения последовательности недостаточно
					if(size < 2)
						// Выводим обозначение ошибочного кодового значения
						return INVALID_CODEPOINT;
					// Если продолжающий байт построен ошибочно
					if((data[1] & 0xC0) != 0x80){
						// Запоминаем длину прочитанной последовательности
						length = 1;
						// Выводим обозначение ошибочного кодового значения
						return INVALID_CODEPOINT;
					}
					// Собираем кодовое значение знака из двух байтов
					const uint32_t value = ((static_cast <uint32_t> (data[0] & 0x1F) << 6) | (data[1] & 0x3F));
					// Запоминаем длину прочитанной последовательности
					length = 2;
					// Выводим прочитанное значение либо признак избыточно длинной записи
					return ((value < 0x80) ? INVALID_CODEPOINT : value);
				}
				// Получаем количество байтов последовательности знака
				const size_t count = sequence(data[0]);
				// Если первый байт последовательности построен ошибочно
				if(count == 0){
					// Запоминаем длину прочитанной последовательности
					length = 1;
					// Выводим обозначение ошибочного кодового значения
					return INVALID_CODEPOINT;
				}
				// Если переданного текста для чтения последовательности недостаточно
				if(size < count)
					// Выводим обозначение ошибочного кодового значения
					return INVALID_CODEPOINT;
				// Кодовое значение прочитанного знака
				uint32_t code = 0;
				// Определяем количество байтов последовательности знака
				switch(count){
					// Если знак записан двумя байтами
					case 2: code = (data[0] & 0x1F); break;
					// Если знак записан тремя байтами
					case 3: code = (data[0] & 0x0F); break;
					// Если знак записан четырьмя байтами
					case 4: code = (data[0] & 0x07); break;
				}
				// Выполняем чтение продолжающих байтов последовательности
				for(size_t i = 1; i < count; i++){
					// Если продолжающий байт построен ошибочно
					if((data[i] & 0xC0) != 0x80){
						// Запоминаем длину прочитанной последовательности
						length = i;
						// Выводим обозначение ошибочного кодового значения
						return INVALID_CODEPOINT;
					}
					// Добавляем значащие разряды продолжающего байта
					code = ((code << 6) | (data[i] & 0x3F));
				}
				// Запоминаем длину прочитанной последовательности
				length = count;
				/**
				 * Если запись кодового значения избыточно длинна
				 *
				 * @note Одно и то же кодовое значение допускает лишь единственную запись:
				 *       избыточно длинные записи служат обходом проверок содержимого
				 */
				if(((count == 2) && (code < 0x80)) || ((count == 3) && (code < 0x800)) ||
				   ((count == 4) && (code < 0x10000)))
					// Выводим обозначение ошибочного кодового значения
					return INVALID_CODEPOINT;
				// Если значение суррогатное либо выходит за пределы Юникода
				if(((code >= 0xD800) && (code <= 0xDFFF)) || (code > MAX_CODEPOINT))
					// Выводим обозначение ошибочного кодового значения
					return INVALID_CODEPOINT;
				// Выводим прочитанное кодовое значение знака
				return code;
			}

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

#endif // __AWH_CODEC_CSV_ENCODING__
