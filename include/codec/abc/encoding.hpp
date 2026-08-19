/**
 * @file encoding.hpp
 * @date 2026-08-18
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
 * @brief Заголовочный файл проволочной укладки бинарного контейнера ABC — снятие и
 *        укладка элементарных единиц записи
 *
 * \~english
 * @brief Header file of the wire laying of the ABC binary container — the taking and
 *        the laying of the elementary units of the record
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_ENCODING__
#define __AWH_CODEC_ABC_ENCODING__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <cstddef>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
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
		 * @brief Пространство имён бинарного контейнера ABC
		 *
		 * \~english
		 * @brief ABC binary container namespace
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Ширина записи отметки времени в октетах
			 *
			 * \~english
			 * @brief Width of the record of a time stamp in octets
			 *
			 * \~
			 */
			constexpr size_t TIME_WIDTH = 8;

			/**
			 * \~russian
			 * @brief Ширина записи опознавателя в октетах
			 *
			 * \~english
			 * @brief Width of the record of an identifier in octets
			 *
			 * \~
			 */
			constexpr size_t UUID_WIDTH = 16;

			/**
			 * \~russian
			 * @brief Снятая единица проволочной записи
			 *
			 * @details Единица есть ведущий октет вместе с ведомой им записью. Значение
			 * толкуется крупным видом: у целого оно есть само число, у строки и двоичных
			 * данных - длина в октетах, у вместимого - количество вложенных значений, а у
			 * одиночного значения и расширения - разновидность
			 *
			 * \~english
			 * @brief Taken unit of the wire record
			 * @details A unit is the leading octet together with the record led by it. The value
			 * is interpreted by the group kind: for an integer it is the number itself, for a string and binary
			 * data — the length in octets, for a container — the number of the nested values, and for
			 * a singleton value and an extension — the variety
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Item {
				// Крупный вид проволочной записи
				group_t group;
				// Подробность ведущего октета
				uint8_t detail;
				// Значение, ведомое меткой
				uint64_t value;
				// Признак неопределённой длины вместимого
				bool indefinite;
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
				Item() noexcept : group(group_t::UNSIGNED), detail(0), value(0), indefinite(false) {}
			} item_t;

			/**
			 * \~russian
			 * @brief Функция укладки целого числа установленной ширины
			 *
			 * @details Октеты укладываются от младшего к старшему. Порядок этот совпадает с
			 * родным порядком у всех целевых машин, и перестановка на них не стоит ничего
			 *
			 * @param result буфер, куда следует уложить запись
			 * @param value  укладываемое значение
			 * @param width  ширина записи в октетах
			 *
			 * \~english
			 * @brief Function of the laying of an integer of a set width
			 * @details The octets are laid from the low one to the high one. This order coincides with
			 * the native order on all the target machines, and a rearrangement on them costs nothing
			 * @param result buffer the record should be laid into
			 * @param value value being laid
			 * @param width width of the record in octets
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void fixed(vector <uint8_t> & result, const uint64_t value, const uint8_t width) noexcept;
			/**
			 * \~russian
			 * @brief Функция укладки метки вместе с ведомым значением
			 *
			 * @details Значение укладывается наименьшей записью, какая его вмещает: до
			 * `INLINE_LIMIT` оно умещается в самую метку, выше - ведёт запись установленной
			 * ширины
			 *
			 * @param result буфер, куда следует уложить запись
			 * @param group  крупный вид проволочной записи
			 * @param value  укладываемое значение
			 *
			 * \~english
			 * @brief Function of the laying of a tag together with the led value
			 * @details The value is laid by the smallest record which contains it: up to
			 * `INLINE_LIMIT` it fits into the tag itself, above it leads a record of a set
			 * width
			 * @param result buffer the record should be laid into
			 * @param group group kind of the wire record
			 * @param value value being laid
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void put(vector <uint8_t> & result, const group_t group, const uint64_t value) noexcept;
			/**
			 * \~russian
			 * @brief Функция укладки метки с заданной подробностью
			 *
			 * @details Служит укладке того, чья подробность значением не является: начала
			 * вместимого неопределённой длины, конца его и одиночных значений
			 *
			 * @param result буфер, куда следует уложить запись
			 * @param group  крупный вид проволочной записи
			 * @param detail подробность ведущего октета
			 *
			 * \~english
			 * @brief Function of the laying of a tag with a set detail
			 * @details Serves the laying of that whose detail is not a value: the beginning
			 * of a container of an indefinite length, its end and the singleton values
			 * @param result buffer the record should be laid into
			 * @param group group kind of the wire record
			 * @param detail detail of the leading octet
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void mark(vector <uint8_t> & result, const group_t group, const uint8_t detail) noexcept;
			/**
			 * \~russian
			 * @brief Функция укладки целого числа со знаком
			 *
			 * @details Число, меньшее нуля, укладывается крупным видом `NEGATIVE` дополнением
			 * до −1: запись хранит `−1 − value`, отчего наименьшее по величине отрицательное
			 * число получает наименьшую запись, а `INT64_MIN` укладывается без переполнения
			 *
			 * @param result буфер, куда следует уложить запись
			 * @param value  укладываемое значение
			 *
			 * \~english
			 * @brief Function of the laying of an integer with a sign
			 * @details A number less than zero is laid by the group kind `NEGATIVE` by a complement
			 * to −1: the record holds `−1 − value`, whereby the smallest in magnitude negative
			 * number receives the smallest record, and `INT64_MIN` is laid without an overflow
			 * @param result buffer the record should be laid into
			 * @param value value being laid
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void integer(vector <uint8_t> & result, const int64_t value) noexcept;
			/**
			 * \~russian
			 * @brief Функция обращения записи дополнения до −1 в число со знаком
			 *
			 * @param value значение, снятое с записи крупного вида `NEGATIVE`
			 * @param result обращённое число со знаком
			 * @return       признак представимости числа видом `int64_t`
			 *
			 * \~english
			 * @brief Function of the turning of a record of a complement to −1 into a number with a sign
			 * @param value value taken from a record of the group kind `NEGATIVE`
			 * @param result turned number with a sign
			 * @return sign of the representability of the number by the kind `int64_t`
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool negative(const uint64_t value, int64_t & result) noexcept;
			/**
			 * \~russian
			 * @brief Функция укладки дробного числа одинарной точности
			 *
			 * @param result буфер, куда следует уложить запись
			 * @param value  укладываемое значение
			 *
			 * \~english
			 * @brief Function of the laying of a fractional number of a single precision
			 * @param result buffer the record should be laid into
			 * @param value value being laid
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void real(vector <uint8_t> & result, const float value) noexcept;
			/**
			 * \~russian
			 * @brief Функция укладки дробного числа двойной точности
			 *
			 * @param result буфер, куда следует уложить запись
			 * @param value  укладываемое значение
			 *
			 * \~english
			 * @brief Function of the laying of a fractional number of a double precision
			 * @param result buffer the record should be laid into
			 * @param value value being laid
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void real(vector <uint8_t> & result, const double value) noexcept;
			/**
			 * \~russian
			 * @brief Функция снятия единицы проволочной записи
			 *
			 * @details Работа снимает ведущий октет вместе с ведомой им записью и сдвигает
			 * смещение. При недостатке октетов смещение остаётся нетронутым: единица не
			 * снята наполовину, и подача следующего куска продолжит разбор с того же места
			 *
			 * @note У одиночного значения и расширения работа снимает **одну лишь метку**:
			 *       ведомые ими данные снимает потребитель. Ширина их задана разновидностью,
			 *       а у опознавателя она к тому же не вмещается в значение единицы вовсе.
			 *       Прочие крупные виды снимаются вместе с ведомой записью
			 *
			 * @param buffer буфер поданной записи
			 * @param size   размер поданной записи в октетах
			 * @param offset смещение, с какого следует снимать единицу
			 * @param item   снятая единица проволочной записи
			 * @param error  код отказа, если снять единицу не удалось
			 * @return       признак успешно снятой единицы
			 *
			 * \~english
			 * @brief Function of the taking of a unit of the wire record
			 * @details The work takes the leading octet together with the record led by it and shifts
			 * the offset. At a shortage of the octets the offset remains untouched: the unit is
			 * not taken by half, and the submission of the next chunk will continue the parsing from the same place
			 * @note For a singleton value and an extension the work takes **the tag alone**:
			 *       the data led by them are taken by the consumer. Their width is set by the variety,
			 *       while for an identifier it moreover does not fit into the value of the unit at all.
			 *       The other group kinds are taken together with the led record
			 * @param buffer buffer of the submitted record
			 * @param size size of the submitted record in octets
			 * @param offset offset the unit should be taken from
			 * @param item taken unit of the wire record
			 * @param error error code if the unit could not be taken
			 * @return sign of a successfully taken unit
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool take(const uint8_t * buffer, const size_t size, size_t & offset, item_t & item, error_t & error) noexcept;
			/**
			 * \~russian
			 * @brief Функция снятия целого числа установленной ширины
			 *
			 * @param buffer буфер поданной записи
			 * @param width  ширина записи в октетах
			 * @return       снятое значение
			 *
			 * \~english
			 * @brief Function of the taking of an integer of a set width
			 * @param buffer buffer of the submitted record
			 * @param width width of the record in octets
			 * @return taken value
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint64_t gather(const uint8_t * buffer, const uint8_t width) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки строки на соответствие кодировке UTF-8
			 *
			 * @details Негодными считаются: ведущий октет, ведущим не являющийся; недостача
			 * продолжающих октетов; продолжающий октет вне отведённого ведущему предела;
			 * запись кодовой точки длиннее необходимого; суррогат; кодовая точка свыше U+10FFFF
			 *
			 * @param buffer   буфер проверяемой строки
			 * @param size     размер проверяемой строки в октетах
			 * @param position смещение первой негодной последовательности
			 * @return         признак соответствия строки кодировке
			 *
			 * \~english
			 * @brief Function of the checking of a string for the conformity to the UTF-8 encoding
			 * @details The following are considered malformed: a leading octet which is not a leading one; a shortage
			 * of the continuing octets; a continuing octet outside the limit allotted to the leading one;
			 * a record of a code point longer than necessary; a surrogate; a code point above U+10FFFF
			 * @param buffer buffer of the string being checked
			 * @param size size of the string being checked in octets
			 * @param position offset of the first malformed sequence
			 * @return sign of the conformity of the string to the encoding
			 *
			 * \~
			 */
			[[nodiscard]] __AWH_SHARED_EXPORT__ bool utf8(const uint8_t * buffer, const size_t size, size_t & position) noexcept;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_ENCODING__
