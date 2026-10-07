/**
 * @file utf8.hpp
 * @date 2026-08-03
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
 * @brief Заголовочный файл кодировщика UTF-8 модуля Юникода — представление кодового значения
 *        символа последовательностью байтов, разбор последовательности байтов
 *        и проверка правильности записи текста
 *
 * \~english
 * @brief Header file of the UTF-8 encoder of the Unicode module — representation of a character
 *        code value by a sequence of bytes, parsing of a sequence of bytes
 *        and verification of the correctness of a text record
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
#include <cstdint>
#include <string>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Принудительная подстановка самой горячей службы кодировщика
 *
 * @details Отсев ведущего октета стоит в переборе всякого знака текста, и работа в нём
 *          одна лишь арифметика: оставленный в единице трансляции, он обращается вызовом
 *          через границу единиц и стоит дороже самого дела
 *
 * @note Приём этот есть признанное в AWH исключение из правила о чистых заголовочных
 *       файлах: реализация живёт в `.cpp`, а для горячих тривиальных служб заводят
 *       подстановочные посредники с `always_inline`. См. `include/codec/abc/common.hpp`
 *       и `include/codec/json/common.hpp`. Касается исключение ОДНОГО `sequence()`:
 *       разбор и сборка последовательности в `.cpp` и остаются там. Обособленного
 *       объявления при определении не держат - по образцу ABC и JSON одно определение
 *       с `AWH_UTF8_INLINE`, иначе объявление без `inline` рядом с подстановочным
 *       определением ломает правило однозначности
 *
 * @note Мера исключения снята 07.10.2026 на кластере рядом форм
 *       (`2026-10-07/cluster-shapes.tsv`
 *       (пять кругов, две сборки прежнего дерева против двух сводных форм): с телом в
 *       единице трансляции читатели многострочных и кавычных полей CSV держатся 0.92..0.98
 *       прежней пропускной способности, с подстановочным телом - 1.20..1.23. Размещение тем
 *       же рядом исключено: две сборки одного исходника расходятся в среднем на 0.02
 *
 * \~english
 * @brief Forced inlining of the hottest service of the encoder
 *
 * \~
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_UTF8_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_UTF8_INLINE inline __attribute__((always_inline))
#endif

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён кодировщика UTF-8
	 *
	 * \~english
	 * @brief UTF-8 encoder namespace
	 *
	 * \~
	 */
	namespace utf8 {
		/**
		 * \~russian
		 * @brief Наибольшая длина записи символа в кодировке UTF-8
		 *
		 * \~english
		 * @brief Largest length of a character record in the UTF-8 encoding
		 *
		 * \~
		 */
		constexpr size_t MAX_LENGTH = 4;

		/**
		 * \~russian
		 * @brief Наибольшее кодовое значение символа Юникода
		 *
		 * \~english
		 * @brief Largest code value of a Unicode character
		 *
		 * \~
		 */
		constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

		/**
		 * \~russian
		 * @brief Обозначение кодового значения, знаком не являющегося
		 *
		 * @details Его выводит побайтовый разбор и при негодном ведущем октете, и при
		 *          нарушенном продолжающем, и при оборванной записи: различает эти случаи
		 *          не значение, а отмеренная длина наибольшей годной части.
		 *
		 * \~english
		 * @brief Marker of a code value that is not a character
		 *
		 * @details Byte-by-byte parsing identifies this condition whether the leading octet
		 *          is invalid, a continuation octet is malformed, or the sequence is truncated;
		 *          the distinction between these cases is based not on the value itself,
		 *          but on the measured length of the longest valid portion.
		 *
		 * \~
		 */
		constexpr uint32_t INVALID_CODEPOINT = static_cast <uint32_t> (~0u);

		/**
		 * \~russian
		 * @brief Перечень исходов побайтового разбора последовательности знака
		 *
		 * @details Исход нужен потоковым потребителям: оборванную на границе куска запись
		 *          они обязаны дочитать на следующем куске, а не отвергнуть, и различить
		 *          эти два решения по одному кодовому значению нельзя - оно при обоих
		 *          одно и то же. Вид этот заведён в общем модуле затем, чтобы девять
		 *          кодеков разводили исходы одним перечнем, а не каждый своим.
		 *
		 * \~english
		 * @brief Outcome of the byte-level parsing of a character sequence
		 *
		 * @details Streaming consumers need this outcome: they must finish reading a record
		 *          that was cut off at a chunk boundary from the next chunk rather than
		 *          discarding it, yet these two scenarios cannot be distinguished by a single
		 *          code value—the value is the same for both. This type was defined in a
		 *          shared module so that the nine codecs could handle these outcomes using
		 *          a unified list, rather than each maintaining its own.
		 *
		 * \~
		 */
		enum class utf8_t : uint8_t {
			VALID     = 0x01, // Последовательность прочитана целиком и правила соблюдает
			BROKEN    = 0x02, // Последовательность построена ошибочно
			TRUNCATED = 0x03  // Последовательности не хватает октетов до конца подачи
		};

		/**
		 * \~russian
		 * @brief Функция проверки правильности записи текста в кодировке UTF-8
		 *
		 * @param text проверяемый текст
		 * @return     результат проверки правильности записи текста
		 *
		 * \~english
		 * @brief Function verifying the correctness of a text record in the UTF-8 encoding
		 *
		 * @param text text being verified
		 * @return     result of verifying the correctness of the text record
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool valid(string_view text) noexcept;
		/**
		 * \~russian
		 * @brief Функция подсчёта количества символов текста в кодировке UTF-8
		 *
		 * @param text текст, записанный в кодировке UTF-8
		 * @return     количество символов текста либо нулевое значение при отказе
		 *
		 * \~english
		 * @brief Function counting the number of characters of a text in the UTF-8 encoding
		 *
		 * @param text text recorded in the UTF-8 encoding
		 * @return     number of characters of the text or zero on failure
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t length(string_view text) noexcept;
		/**
		 * \~russian
		 * @brief Функция определения длины побайтовой последовательности по ведущему октету
		 *
		 * @details Ведущими не бывают октеты `C0` и `C1`: открывали бы запись длиннее
		 *          необходимой, а октеты свыше `F4` - точку свыше U+10FFFF. Ни того, ни
		 *          другого кодировка не допускает при любом продолжении.
		 *
		 * @note Замет о происхождении правила, перенесённый из тел кодеков при сведении
		 *       копий в одну: отсев `C0`, `C1` и октетов свыше `F4` стоял не всюду - он
		 *       водился лишь у наречия YAML, а побайтовое тело разбора UTF-8 лежало у трёх
		 *       кодеков тремя списками. Выправлен был один список из трёх, и два прочих
		 *       остались в прежнем виде; матрица сличений 06.10.2026 показывает, что и
		 *       ныне копии CSV, JSON и XML ведут себя иначе на 579 768 подачах из
		 *       17 101 312. Прежде такие ведущие принимались, и негодность обнаруживалась
		 *       лишь по прочтении продолжения - граница негодной подачи при том уплывала
		 *
		 * @note О размещении тела: подстановку в заголовке против тела в единице
		 *       трансляции меряли парой на кластере, пять кругов по двадцати четырём
		 *       сценариям INI; медиана пропускной способности вышла 1.0093 при контроле
		 *       1.0061 - то есть при сличении двух сборок одного и того же кода разброс
		 *       тот же самый, и ни один сценарий за полосу контроля не вышел. Выигрыша
		 *       нет, и тело держат в единице трансляции по общему правилу. Замет этот
		 *       заведён затем, чтобы исключение не заводили снова по одному лишь
		 *       рассуждению о горячем пути: рассуждение было, мера ему противит
		 *
		 * @param leading ведущий октет последовательности
		 * @return        длина последовательности либо нуль при ошибочном ведущем октете
		 *
		 * \~english
		 * @brief Function of the obtaining of the length of a byte sequence by the leading octet
		 *
		 * @details The `C0` and `C1` octets never appear as leading bytes—as they would initiate
		 *          a sequence longer than necessary—nor do octets above `F4`, which would
		 *          correspond to a code point beyond U+10FFFF. The encoding permits neither case,
		 *          regardless of the continuation bytes.
		 *
		 * @note Regarding the origin of the rule carried over from the codec implementations
		 *       during the consolidation of copies: the filtering of `C0`, `C1`, and octets
		 *       above `F4` was not applied universally—it existed only in the YAML dialect,
		 *       while the byte-level UTF-8 parsing logic was distributed across three separate
		 *       lists for the three codecs. Only one of the three lists was corrected, leaving
		 *       the other two unchanged; a comparison matrix from October 6, 2026, reveals that
		 *       the CSV, JSON, and XML implementations still behave differently on 579,768 out
		 *       of 17,101,312 inputs. Previously, such inputs were accepted, and their invalidity
		 *       was detected only upon reading the subsequent data—causing the boundary
		 *       of the invalid input to shift
		 *
		 * @note On the placement of the body: header inlining versus a body held in the
		 *       translation unit was measured as a cluster pair—five rounds over
		 *       twenty-four INI scenarios. The throughput median came out 1.0093 against a
		 *       control of 1.0061, that is, the same spread as two builds of identical
		 *       code, and no scenario left the control band. There is no gain, so the body
		 *       stays in the translation unit under the general rule. This note exists so
		 *       that the exception is not reintroduced on the strength of reasoning about a
		 *       hot path alone: the reasoning was made, and the measurement contradicts it
		 *
		 * @param leading leading octet of the sequence
		 * @return        length of the sequence or zero at an erroneous leading octet
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ AWH_UTF8_INLINE size_t sequence(const uint8_t leading) noexcept {
			/**
			 * Если знак записан одним октетом
			 */
			if(leading < 0x80)
				// Выводим длину последовательности знака
				return 1;
			/**
			 * Если ведущий октет построен ошибочно
			 */
			if((leading == 0xC0) || (leading == 0xC1) || (leading > 0xF4))
				// Выводим признак ошибочного построения последовательности
				return 0;
			/**
			 * Если знак записан двумя октетами
			 */
			if((leading & 0xE0) == 0xC0)
				// Выводим длину последовательности знака
				return 2;
			/**
			 * Если знак записан тремя октетами
			 */
			if((leading & 0xF0) == 0xE0)
				// Выводим длину последовательности знака
				return 3;
			/**
			 * Если знак записан четырьмя октетами
			 */
			if((leading & 0xF8) == 0xF0)
				// Выводим длину последовательности знака
				return 4;
			// Выводим признак ошибочного построения последовательности
			return 0;
		}
		/**
		 * \~russian
		 * @brief Функция дописывания кодового значения знака к тексту
		 *
		 * @details Тело общее, а не местное: побайтовая сборка записи не должна жить в
		 *          нескольких местах, ибо починка одного места в прочие не приезжает.
		 *
		 * @param code   записываемое кодовое значение знака
		 * @param result текст, к которому дописывается знак
		 * @return       признак успешной записи: у суррогата и у точки свыше U+10FFFF записи нет
		 *
		 * \~english
		 * @brief Function of appending the code value of a character to a text
		 *
		 * @details The logic is global, not local: the byte-by-byte assembly of the record
		 *          must not exist in multiple places, because a fix applied in one location
		 *          does not propagate to the others.
		 *
		 * @param code   code value of the character being recorded
		 * @param result text to which the character is appended
		 * @return       record result: a surrogate and a point above U+10FFFF have no record
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool encode(const uint32_t code, string & result) noexcept;
		/**
		 * \~russian
		 * @brief Функция представления кодового значения символа записью UTF-8
		 *
		 * @details Кодовые значения суррогатных пар и значения, превышающие наибольшее
		 *          кодовое значение Юникода, записи не имеют.
		 *
		 * @param code   кодовое значение записываемого символа
		 * @param buffer буфер записи символа длиной не менее «MAX_LENGTH»
		 * @return       длина записи символа либо нулевое значение при отказе
		 *
		 * \~english
		 * @brief Function representing a character code value by a UTF-8 record
		 *
		 * @details Code values of surrogate pairs and values exceeding the largest Unicode code
		 *          value have no record.
		 *
		 * @param code   code value of the character being recorded
		 * @param buffer buffer of the character record no shorter than "MAX_LENGTH"
		 * @return       length of the character record or zero on failure
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t encode(const uint32_t code, char * buffer) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора записи символа в кодировке UTF-8
		 *
		 * @details Записи, длина которых превышает необходимую для кодового значения,
		 *          записи суррогатных пар и записи, оборванные концом текста, отклоняются.
		 *
		 * @param text текст, записанный в кодировке UTF-8
		 * @param pos  положение начала записи символа в тексте
		 * @param code кодовое значение разобранного символа
		 * @return     длина разобранной записи либо нулевое значение при отказе
		 *
		 * \~english
		 * @brief Function parsing a character record in the UTF-8 encoding
		 *
		 * @details Records whose length exceeds the one necessary for the code value, records of
		 *          surrogate pairs and records cut short by the end of the text are rejected.
		 *
		 * @param text text recorded in the UTF-8 encoding
		 * @param pos  position of the beginning of the character record within the text
		 * @param code code value of the parsed character
		 * @return     length of the parsed record or zero on failure
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t decode(string_view text, const size_t pos, uint32_t & code) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора побайтовой последовательности знака
		 *
		 * @details Разбор ведётся по правилу наибольшей годной части (Юникод, раздел 3.9),
		 *          и в этом его отличие от `decode(string_view, ...)`: негодной подаче
		 *          отмеряется длина уже годной её части, а не ноль, дабы перебор, идущий по
		 *          этой длине, не встал на месте навек. Негодными признаются ведущие `C0`, `C1`
		 *          и всякий октет свыше `F4`, равно как нарушенный продолжающий: предел своего
		 *          первый продолжающий байт берёт у ведущего (`E0` требует `A0..BF`, `ED` - не
		 *          выше `9F`, `F0` требует `90..BF`, `F4` - не выше `8F`), чем запись длиннее
		 *          необходимой, суррогат да точка свыше U+10FFFF отвергаются разом.
		 *
		 * @note Оборванный на конце текста заход различим с негодным по сумме: при нехватке
		 *       октетов отмеряется их наличное число, и потребителю, подающему текст кусками,
		 *       этого довольно, чтобы ждать остаток, а не отвергать знак
		 *
		 * @param buffer буфер разбираемого текста
		 * @param size   количество октетов в буфере
		 * @param length отмеренная длина последовательности; при пустой подаче - нуль
		 * @return       кодовое значение знака либо обозначение негодного
		 *
		 * \~english
		 * @brief Function of the parsing of the byte sequence of a character
		 *
		 * @details Parsing follows the "longest valid prefix" rule (Unicode, Section 3.9),
		 *          which distinguishes it from `decode(string_view, ...)`: for an invalid
		 *          sequence, the length of the valid portion is reported rather than zero,
		 *          ensuring that the iteration process advances instead of stalling indefinitely.
		 *          Invalid sequences include leading `C0` and `C1` bytes, any octet greater than
		 *          `F4`, and malformed continuation bytes; the first continuation byte
		 *          is constrained by the leading byte (e.g., `E0` requires `A0..BF`, `ED`
		 *          requires no higher than `9F`, `F0` requires `90..BF`, and `F4` requires
		 *          no higher than `8F`), while overlong encodings, surrogates, and code points
		 *          exceeding U+10FFFF are all rejected.
		 *
		 * @note A transmission cut off at the end of the text is distinguishable from a checksum error:
		 *       if octets are missing, the available ones are measured, and for a consumer receiving
		 *       the text in chunks, this suffices to await the remainder rather than rejecting the signal.
		 *
		 * @param buffer buffer of the parsed text
		 * @param size   number of the octets of the buffer
		 * @param length measured length of the sequence; zero at an empty request
		 * @return       code value of the character or the marker of an unsuitable one
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t decode(const char * buffer, const size_t size, size_t & length) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора побайтовой последовательности знака с исходом разбора
		 *
		 * @details Разбор ведёт правило наибольшей годной части: негодной подаче отмеряется
		 *          длина её годной начала части, а не ноль, дабы перебор, идущий по
		 *          отмеренной длине, не встал на месте навек. Исход при том назван отдельно,
		 *          ибо потоковому потребителю надлежит различать оборванную на границе куска
		 *          запись, дочитаемую следующим куском, от записи построенной ошибочно,
		 *          отвергаемой навеки.
		 *
		 * @note Пустая подача (буфер не передан либо в нём нет октетов) исхода не имеет:
		 *       выводятся BROKEN с длиной ноль, и решение о переходе остаётся за зовущим -
		 *       двигать перебор по нулю нечем
		 *
		 * @param buffer буфер разбираемого текста
		 * @param size   количество октетов в буфере
		 * @param code   прочитанное кодовое значение либо обозначение негодного
		 * @param length отмеренная длина последовательности
		 * @return       исход разбора
		 *
		 * \~english
		 * @brief Function of the parsing of a byte sequence with the outcome of the parsing
		 *
		 * @details The parsing process is governed by the "largest valid part" rule:
		 *          a defective input segment is assigned a length corresponding to its
		 *          valid initial portion—rather than zero—to ensure that the scan advancing
		 *          across this measured length does not stall indefinitely. The outcome
		 *          is classified separately because a stream consumer must distinguish between
		 *          a record truncated at a chunk boundary (to be completed by the subsequent chunk)
		 *          and a malformed record that is permanently rejected.
		 *
		 * @note An empty submission (where no buffer is passed or the buffer contains no octets)
		 *       yields no result: a BROKEN status with a length of zero is returned, and the decision
		 *       on how to proceed rests with the caller—there is nothing to advance the search by
		 *
		 * @param buffer buffer of the text being parsed
		 * @param size   number of octets in the buffer
		 * @param code   read code value or the marker of an invalid one
		 * @param length measured length of the sequence
		 * @return       outcome of the parsing
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ utf8_t inspect(const char * buffer, const size_t size, uint32_t & code, size_t & length) noexcept;
	};
};
