/**
 * @file: hash.hpp
 * @date: 2026-07-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля быстрого некриптографического хэширования — класс Hash, формирующий
 *        хэш произвольной разрядности как за один вызов, так и в потоковом режиме, с выводом результата
 *        во встроенные числовые типы и в длинные числа произвольной разрядности модуля BigNum
 *
 * \~english
 * @brief Header file of the module of fast non-cryptographic hashing — the Hash class forming
 *        a hash of arbitrary width both within a single call and in the streaming mode, with the output of the result
 *        into the built-in numeric types and into long numbers of arbitrary width of the BigNum module
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_HASH__
#define __AWH_HASH__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <vector>
#include <limits>
#include <cstring>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"
#include "../num/bignum.hpp"

/**
 * Если компилятор принадлежит к семейству Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_HASH_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_HASH_INLINE inline __attribute__((always_inline))
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
	 * @brief Пространство имён вычислительного движка хэширования
	 *
	 * @details Пространство имён содержит функции, выполняющие всю работу над сырыми
	 *          буферами байтов. Результат хэширования формируется как поток байтов
	 *          произвольной длины в порядке от младшего байта к старшему вне зависимости
	 *          от порядка байтов процессора, поэтому хэш заданной разрядности всегда
	 *          является префиксом хэша большей разрядности тех же самых данных.
	 *
	 * \~english
	 * @brief Namespace of the computational engine of hashing
	 *
	 * @details The namespace contains the functions performing all the work over raw
	 *          byte buffers. The result of hashing is formed as a stream of bytes
	 *          of arbitrary length in the order from the least significant byte to the most significant one regardless
	 *          of the byte order of the processor, therefore a hash of a given width is always
	 *          a prefix of a hash of a greater width of the very same data.
	 *
	 * \~
	 */
	namespace hashing {
		/**
		 * \~russian
		 * @brief Функция смешивания двух чисел
		 *
		 * @details Функция выполняет умножение двух чисел с получением полного
		 *          128-битного произведения и складывает его половины операцией
		 *          исключающего ИЛИ. Функция является основным строительным блоком
		 *          всех остальных функций пространства имён.
		 *
		 * @param value1 первое число для смешивания
		 * @param value2 второе число для смешивания
		 * @return       результат смешивания чисел
		 *
		 * \~english
		 * @brief Function mixing two numbers
		 *
		 * @details The function performs the multiplication of two numbers obtaining the full
		 *          128-bit product and adds its halves by the exclusive OR operation.
		 *          The function is the main building block of all the other functions of the namespace.
		 *
		 * @param value1 first number for the mixing
		 * @param value2 second number for the mixing
		 * @return       result of mixing the numbers
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint64_t mix(const uint64_t value1, const uint64_t value2) noexcept;
		/**
		 * \~russian
		 * @brief Функция окончательного перемешивания числа
		 *
		 * @details Функция размазывает влияние каждого бита входного числа на все
		 *          биты результата, обеспечивая лавинный эффект хэш-функции.
		 *
		 * @param value число для перемешивания
		 * @return      результат перемешивания числа
		 *
		 * \~english
		 * @brief Function of the final stirring of a number
		 *
		 * @details The function spreads the influence of every bit of the input number over all
		 *          the bits of the result, providing the avalanche effect of the hash function.
		 *
		 * @param value number for the stirring
		 * @return      result of stirring the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint64_t avalanche(const uint64_t value) noexcept;
		/**
		 * \~russian
		 * @brief Функция сведения пары начальных значений хэширования в одно
		 *
		 * @details Свести пару ключей перемешиванием нельзя: перемешивание
		 *          мультипликативно, и нулевой ключ обнуляет его целиком - пара
		 *          (999, 0) давала тот же результат, что и пара (7, 0), то есть
		 *          второй ключ терялся молча, а первый вместе с ним.
		 *
		 *          Здесь каждый ключ проходит окончательное перемешивание
		 *          самостоятельно, а сводятся они сложением по модулю два. Каждое из
		 *          двух перемешиваний взаимно однозначно, поэтому обнулить результат
		 *          одним лишь нулевым ключом невозможно, и оба ключа значимы при
		 *          любом своём значении
		 *
		 * @param seed1 первое начальное значение хэширования
		 * @param seed2 второе начальное значение хэширования
		 * @return      сведённое начальное значение хэширования
		 *
		 * \~english
		 * @brief Function of merging a pair of hashing seed values into one
		 *
		 * @details A pair of keys cannot be merged by stirring: the stirring is
		 *          multiplicative, and a zero key nullifies it entirely - the pair
		 *          (999, 0) gave the same result as the pair (7, 0), that is,
		 *          the second key was lost silently, and the first one along with it.
		 *
		 *          Here every key goes through the final stirring
		 *          on its own, and they are merged by addition modulo two. Each of
		 *          the two stirrings is one-to-one, therefore it is impossible to nullify the result
		 *          by a zero key alone, and both keys are significant at
		 *          any of their values
		 *
		 * @param seed1 first hashing seed value
		 * @param seed2 second hashing seed value
		 * @return      merged hashing seed value
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint64_t merge(const uint64_t seed1, const uint64_t seed2) noexcept;
		/**
		 * \~russian
		 * @brief Функция формирования хэша буфера данных
		 *
		 * @details Функция формирует хэш указанной длины за один вызов. Результат
		 *          полностью совпадает с результатом потокового хэширования тех же
		 *          самых данных с тем же самым начальным значением.
		 *
		 * @param buffer буфер данных для хэширования
		 * @param size   размер буфера данных для хэширования
		 * @param seed   начальное значение хэширования
		 * @param result буфер для записи результата хэширования
		 * @param length размер результата хэширования в байтах
		 *
		 * \~english
		 * @brief Function forming the hash of a data buffer
		 *
		 * @details The function forms a hash of the specified length within a single call. The result
		 *          fully coincides with the result of the streaming hashing of the very
		 *          same data with the very same seed value.
		 *
		 * @param buffer data buffer for the hashing
		 * @param size   size of the data buffer for the hashing
		 * @param seed   hashing seed value
		 * @param result buffer for writing the hashing result
		 * @param length size of the hashing result in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void generate(const void * buffer, const size_t size, const uint64_t seed, uint8_t * result, const size_t length) noexcept;
		/**
		 * \~russian
		 * @brief Функция формирования 64-битного хэша буфера данных
		 *
		 * @details Функция выводит первые восемь байтов результата хэширования в виде
		 *          числа, минуя формирование потока байтов результата, и предназначена
		 *          для наиболее частого случая хэширования ключей ассоциативных
		 *          контейнеров.
		 *
		 * @param buffer буфер данных для хэширования
		 * @param size   размер буфера данных для хэширования
		 * @param seed   начальное значение хэширования
		 * @return       результат хэширования
		 *
		 * \~english
		 * @brief Function forming the 64-bit hash of a data buffer
		 *
		 * @details The function outputs the first eight bytes of the hashing result as
		 *          a number, bypassing the forming of the byte stream of the result, and is intended
		 *          for the most frequent case of hashing the keys of associative
		 *          containers.
		 *
		 * @param buffer data buffer for the hashing
		 * @param size   size of the data buffer for the hashing
		 * @param seed   hashing seed value
		 * @return       hashing result
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint64_t generate(const void * buffer, const size_t size, const uint64_t seed) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения буфера длинного вещественного числа к конечному значению
		 *
		 * @details Произвольный набор байтов способен образовать бесконечность либо
		 *          значение не являющееся числом, а такое значение неравно самому себе
		 *          и в качестве ключа ассоциативного контейнера непригодно. Функция
		 *          гасит старший бит порядка числа, переводя значение в конечное.
		 *
		 * @param value буфер длинного вещественного числа для приведения
		 * @param size  размер буфера длинного вещественного числа в байтах
		 *
		 * \~english
		 * @brief Function bringing the buffer of a long real number to a finite value
		 *
		 * @details An arbitrary set of bytes is capable of forming an infinity or
		 *          a value that is not a number, and such a value is unequal to itself
		 *          and is unfit as a key of an associative container. The function
		 *          extinguishes the most significant bit of the exponent of the number, turning the value into a finite one.
		 *
		 * @param value buffer of the long real number for the bringing
		 * @param size  size of the buffer of the long real number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void finite(uint8_t * value, const size_t size) noexcept;
	};
	/**
	 * \~russian
	 * @brief Пространство имён вычислительного движка хэширования
	 *
	 * \~english
	 * @brief Namespace of the computational engine of hashing
	 *
	 * \~
	 */
	namespace hashing {
		/**
		 * \~russian
		 * @brief Шаблон типа результата хэширования
		 *
		 * @tparam T тип результата хэширования
		 *
		 * \~english
		 * @brief Template of the type of the hashing result
		 *
		 * @tparam T type of the hashing result
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Признак результата хэширования, выводимого числом напрямую
		 *
		 * @details Целое число, умещающееся в разрядность вычислительного движка,
		 *          снимается со свёртки напрямую и потока октетов не заводит. Знак
		 *          этому не помеха: приведение беззнакового числа к знаковому типу
		 *          той же разрядности отбрасывает старшие разряды по остатку - задано
		 *          это редакцией C++20, а прежде было определено самой сборкой и на
		 *          всех целевых сборках библиотеки делалось именно так, - то есть даёт
		 *          ровно то же значение, что и сборка знакового числа из потока
		 *          октетов сдвигами.
		 *
		 *          Признак логического типа сюда не входит: разрядность его равна
		 *          одному биту, и хэша он не несёт вовсе
		 *
		 * \~english
		 * @brief Sign of a hashing result output as a number directly
		 *
		 * @details An integer number fitting into the width of the computational engine
		 *          is taken off the convolution directly and does not start a stream of octets. The sign
		 *          is no hindrance to that: the conversion of an unsigned number to a signed type
		 *          of the same width discards the most significant bits by the remainder - this is
		 *          mandated by the C++20 edition, and before that it was defined by the build itself and on
		 *          all the target builds of the library it was done exactly that way, - that is, it gives
		 *          exactly the same value as the assembly of a signed number from a stream
		 *          of octets by shifts.
		 *
		 *          The sign of the boolean type is not included here: its width equals
		 *          one bit, and it carries no hash whatsoever
		 *
		 * \~
		 */
		constexpr bool numeric = (is_integral <T>::value && !is_same <T, bool>::value && (sizeof(T) <= sizeof(uint64_t)));
		/**
		 * \~russian
		 * @brief Шаблон типа результата хэширования
		 *
		 * @tparam T тип результата хэширования
		 *
		 * \~english
		 * @brief Template of the type of the hashing result
		 *
		 * @tparam T type of the hashing result
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция записи результата хэширования во встроенный целочисленный тип
		 *
		 * @details Число собирается из потока октетов в порядке от младшего октета
		 *          к старшему вне зависимости от порядка байтов процессора, поэтому
		 *          числовое значение результата на всех платформах одинаково и
		 *          совпадает с результатом быстрого пути функции формирования хэша.
		 *
		 * @param result результат хэширования для записи
		 * @param buffer буфер сформированного хэша
		 *
		 * \~english
		 * @brief Function writing the hashing result into a built-in integer type
		 *
		 * @details The number is assembled from the stream of octets in the order from the least significant octet
		 *          to the most significant one regardless of the byte order of the processor, therefore
		 *          the numeric value of the result is the same on all the platforms and
		 *          coincides with the result of the fast path of the hash forming function.
		 *
		 * @param result hashing result for the writing
		 * @param buffer buffer of the formed hash
		 *
		 * \~
		 */
		AWH_HASH_INLINE typename enable_if <is_integral <T>::value, void>::type assign(T & result, const uint8_t * buffer) noexcept {
			/**
			 * Создаём тип данных беззнакового представления результата хэширования
			 *
			 */
			using value_t = typename make_unsigned <T>::type;
			// Собираемое из потока октетов число
			value_t value = 0;
			/**
			 * Выполняем перебор всех октетов сформированного хэша
			 */
			for(size_t i = 0; i < sizeof(T); i++)
				// Добавляем очередной октет сформированного хэша в число
				value |= (static_cast <value_t> (buffer[i]) << (i * 8));
			// Записываем собранное число в результат хэширования
			result = static_cast <T> (value);
		}
		/**
		 * \~russian
		 * @brief Шаблон типа результата хэширования
		 *
		 * @tparam T тип результата хэширования
		 *
		 * \~english
		 * @brief Template of the type of the hashing result
		 *
		 * @tparam T type of the hashing result
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция записи результата хэширования во встроенный вещественный тип
		 *
		 * @details Произвольный набор октетов образует бесконечность либо значение
		 *          не являющееся числом примерно в одном случае из двухсот у числа
		 *          одинарной точности, а такое значение неравно самому себе и в
		 *          качестве ключа ассоциативного контейнера непригодно. Функция
		 *          гасит старший бит порядка числа, переводя значение в конечное.
		 *
		 * @param result результат хэширования для записи
		 * @param buffer буфер сформированного хэша
		 *
		 * \~english
		 * @brief Function writing the hashing result into a built-in real type
		 *
		 * @details An arbitrary set of octets forms an infinity or a value
		 *          that is not a number roughly in one case out of two hundred for a number
		 *          of single precision, and such a value is unequal to itself and is
		 *          unfit as a key of an associative container. The function
		 *          extinguishes the most significant bit of the exponent of the number, turning the value into a finite one.
		 *
		 * @param result hashing result for the writing
		 * @param buffer buffer of the formed hash
		 *
		 * \~
		 */
		AWH_HASH_INLINE typename enable_if <is_floating_point <T>::value, void>::type assign(T & result, const uint8_t * buffer) noexcept {
			/**
			 * Проверяем соответствие вещественного типа формату IEEE-754
			 */
			static_assert(numeric_limits <T>::is_iec559 && (sizeof(T) <= sizeof(uint64_t)),
			 "AWH hash: the floating point result must be an IEEE-754 binary32 or binary64 number, use bigreal_t for wider ones");
			/**
			 * Создаём тип данных битового представления результата хэширования
			 *
			 */
			using value_t = typename conditional <sizeof(T) == sizeof(uint32_t), uint32_t, uint64_t>::type;
			/**
			 * Определяем разрядность мантиссы вещественного числа
			 *
			 */
			constexpr uint8_t mantissa = static_cast <uint8_t> (numeric_limits <T>::digits - 1);
			/**
			 * Определяем маску порядка вещественного числа
			 *
			 */
			constexpr value_t exponent = ((static_cast <value_t> (1) << ((sizeof(T) * 8) - mantissa - 1)) - 1);
			// Собираемое из потока октетов число
			value_t value = 0;
			/**
			 * Выполняем перебор всех октетов сформированного хэша
			 */
			for(size_t i = 0; i < sizeof(T); i++)
				// Добавляем очередной октет сформированного хэша в число
				value |= (static_cast <value_t> (buffer[i]) << (i * 8));
			/**
			 * Если порядок числа заполнен единицами, что означает бесконечность
			 * либо значение не являющееся числом
			 */
			if(((value >> mantissa) & exponent) == exponent)
				// Выполняем сброс старшего бита порядка числа
				value &= ~(static_cast <value_t> (1) << ((sizeof(T) * 8) - 2));
			// Записываем битовое представление в результат хэширования
			::memcpy(&result, &value, sizeof(T));
		}
		/**
		 * \~russian
		 * @brief Шаблон размера массива байтов результата хэширования
		 *
		 * @tparam SIZE размер массива байтов результата хэширования
		 *
		 * \~english
		 * @brief Template of the size of the byte array of the hashing result
		 *
		 * @tparam SIZE size of the byte array of the hashing result
		 *
		 * \~
		 */
		template <size_t SIZE>
		/**
		 * \~russian
		 * @brief Функция записи результата хэширования в массив байтов
		 *
		 * @param result результат хэширования для записи
		 * @param buffer буфер сформированного хэша
		 *
		 * \~english
		 * @brief Function writing the hashing result into a byte array
		 *
		 * @param result hashing result for the writing
		 * @param buffer buffer of the formed hash
		 *
		 * \~
		 */
		AWH_HASH_INLINE void assign(array <uint8_t, SIZE> & result, const uint8_t * buffer) noexcept {
			// Копируем сформированный хэш в результат хэширования
			::memcpy(result.data(), buffer, SIZE);
		}
		/**
		 * \~russian
		 * @brief Шаблон разрядности и типа длинного числа результата хэширования
		 *
		 * @tparam BYTES размер длинного числа в байтах
		 * @tparam TYPE  тип хранимого длинного числа
		 *
		 * \~english
		 * @brief Template of the width and of the type of the long number of the hashing result
		 *
		 * @tparam BYTES size of the long number in bytes
		 * @tparam TYPE  type of the stored long number
		 *
		 * \~
		 */
		template <uint16_t BYTES, bignum::type_t TYPE>
		/**
		 * \~russian
		 * @brief Функция записи результата хэширования в длинное число
		 *
		 * @param result результат хэширования для записи
		 * @param buffer буфер сформированного хэша
		 *
		 * \~english
		 * @brief Function writing the hashing result into a long number
		 *
		 * @param result hashing result for the writing
		 * @param buffer buffer of the formed hash
		 *
		 * \~
		 */
		AWH_HASH_INLINE void assign(BigNum <BYTES, TYPE> & result, const uint8_t * buffer) noexcept {
			/**
			 * Проверяем совпадение размера длинного числа с размером его буфера
			 */
			static_assert(sizeof(BigNum <BYTES, TYPE>) == BYTES, "AWH hash: the big number layout must be a plain byte buffer");
			// Копируем сформированный хэш в результат хэширования
			::memcpy(result.data(), buffer, BYTES);
			/**
			 * Если результатом хэширования является вещественное длинное число
			 */
			if constexpr(TYPE == bignum::type_t::REAL)
				// Выполняем приведение результата хэширования к конечному значению
				hashing::finite(result.data(), BYTES);
		}
		/**
		 * \~russian
		 * @brief Шаблон типа результата хэширования
		 *
		 * @tparam T тип результата хэширования
		 *
		 * \~english
		 * @brief Template of the type of the hashing result
		 *
		 * @tparam T type of the hashing result
		 *
		 * \~
		 */
		template <typename T = uint64_t>
		/**
		 * \~russian
		 * @brief Функция формирования хэша буфера данных
		 *
		 * @details Функция выводит результат хэширования в любом типе данных,
		 *          разрядность которого известна на этапе компиляции: во встроенном
		 *          числовом типе, в массиве байтов либо в длинном числе модуля BigNum.
		 *
		 * @param buffer буфер данных для хэширования
		 * @param size   размер буфера данных для хэширования
		 * @param seed   начальное значение хэширования
		 * @return       результат хэширования
		 *
		 * \~english
		 * @brief Function forming the hash of a data buffer
		 *
		 * @details The function outputs the hashing result in any data type
		 *          whose width is known at the compilation stage: in a built-in
		 *          numeric type, in a byte array or in a long number of the BigNum module.
		 *
		 * @param buffer data buffer for the hashing
		 * @param size   size of the data buffer for the hashing
		 * @param seed   hashing seed value
		 * @return       hashing result
		 *
		 * \~
		 */
		AWH_HASH_INLINE T create(const void * buffer, const size_t size, const uint64_t seed = 0) noexcept {
			// Выполняем проверку типа результата хэширования на пригодность
			static_assert(!std::is_same_v <T, bool>, "AWH hash: the result of hashing does not fit in a boolean type, use an integer type of the required width");
			/**
			 * Если результатом хэширования является целое число, умещающееся
			 * в разрядность вычислительного движка хэширования
			 */
			if constexpr(hashing::numeric <T>)
				// Выводим младшие разряды сформированного хэша
				return static_cast <T> (hashing::generate(buffer, size, seed));
			/**
			 * Если результат хэширования формируется в буфере
			 */
			else {
				// Результат работы функции
				T result;
				// Буфер сформированного хэша
				uint8_t data[sizeof(T)];
				// Выполняем формирование хэша буфера данных
				hashing::generate(buffer, size, seed, data, sizeof(T));
				// Выполняем запись сформированного хэша в результат хэширования
				hashing::assign(result, data);
				// Выводим результат работы функции
				return result;
			}
		}
	};
	/**
	 * \~russian
	 * @brief Класс быстрого некриптографического хэширования
	 *
	 * @details Класс формирует хэш произвольной разрядности от буфера данных как за
	 *          один вызов, так и в потоковом режиме, когда данные поступают частями
	 *          и целиком в памяти не находятся. Результат потокового хэширования
	 *          совпадает с результатом хэширования тех же самых данных за один вызов
	 *          вне зависимости от того, какими частями данные поступали.
	 *
	 * @details Хэш формируется как поток байтов, поэтому хэш меньшей разрядности
	 *          является префиксом хэша большей разрядности: младшие 32 бита
	 *          128-битного хэша совпадают с 32-битным хэшем тех же самых данных.
	 *          Результат выводится во встроенные числовые типы, в массивы байтов и
	 *          в длинные числа модуля BigNum любой объявленной разрядности.
	 *
	 * @note    Хэш-функция криптографической стойкостью не обладает и для проверки
	 *          подлинности данных не предназначена. Для таких задач следует
	 *          использовать хэш-суммы и подписи модуля Crypto.
	 *
	 * \~english
	 * @brief Class of fast non-cryptographic hashing
	 *
	 * @details The class forms a hash of arbitrary width from a data buffer both within
	 *          a single call and in the streaming mode, when the data arrives in parts
	 *          and does not reside in memory in whole. The result of the streaming hashing
	 *          coincides with the result of hashing the very same data within a single call
	 *          regardless of what parts the data arrived in.
	 *
	 * @details The hash is formed as a stream of bytes, therefore a hash of a lesser width
	 *          is a prefix of a hash of a greater width: the least significant 32 bits
	 *          of a 128-bit hash coincide with the 32-bit hash of the very same data.
	 *          The result is output into the built-in numeric types, into byte arrays and
	 *          into long numbers of the BigNum module of any declared width.
	 *
	 * @note    The hash function possesses no cryptographic strength and is not intended
	 *          for verifying the authenticity of data. For such tasks the hash sums
	 *          and signatures of the Crypto module should be used.
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Hash {
		public:
			/**
			 * \~russian
			 * @brief Размер блока данных хэширования в байтах
			 *
			 * \~english
			 * @brief Size of the data block of the hashing in bytes
			 *
			 * \~
			 */
			static constexpr size_t BLOCK = 64;
		private:
			// Начальное значение хэширования
			uint64_t _seed;
			// Общий размер обработанных данных
			uint64_t _length;
			// Размер данных находящихся в буфере
			size_t _offset;
		private:
			// Состояние вычислительного движка хэширования
			array <uint64_t, 4> _state;
			// Буфер неполного блока данных
			array <uint8_t, BLOCK> _buffer;
		public:
			/**
			 * \~russian
			 * @brief Метод сброса состояния потокового хэширования
			 *
			 * @details Начальное значение хэширования метод не изменяет.
			 *
			 * \~english
			 * @brief Method resetting the state of the streaming hashing
			 *
			 * @details The method does not change the hashing seed value.
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения начального значения хэширования
			 *
			 * @return начальное значение хэширования
			 *
			 * \~english
			 * @brief Method extracting the hashing seed value
			 *
			 * @return hashing seed value
			 *
			 * \~
			 */
			uint64_t seed() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки начального значения хэширования
			 *
			 * @details Метод сбрасывает состояние потокового хэширования.
			 *
			 * @param seed начальное значение хэширования
			 *
			 * \~english
			 * @brief Method setting the hashing seed value
			 *
			 * @details The method resets the state of the streaming hashing.
			 *
			 * @param seed hashing seed value
			 *
			 * \~
			 */
			void seed(const uint64_t seed) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения размера обработанных данных
			 *
			 * @return размер обработанных данных в байтах
			 *
			 * \~english
			 * @brief Method extracting the size of the processed data
			 *
			 * @return size of the processed data in bytes
			 *
			 * \~
			 */
			uint64_t length() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод добавления данных в потоковое хэширование
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер буфера данных для хэширования
			 *
			 * \~english
			 * @brief Method adding data into the streaming hashing
			 *
			 * @param buffer data buffer for the hashing
			 * @param size   size of the data buffer for the hashing
			 *
			 * \~
			 */
			void update(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод формирования результата потокового хэширования
			 *
			 * @details Состояние потокового хэширования метод не изменяет, поэтому
			 *          добавление данных допускается продолжить и после формирования
			 *          промежуточного результата.
			 *
			 * @param result буфер для записи результата хэширования
			 * @param length размер результата хэширования в байтах
			 *
			 * \~english
			 * @brief Method forming the result of the streaming hashing
			 *
			 * @details The method does not change the state of the streaming hashing, therefore
			 *          the adding of data is allowed to be continued even after the forming
			 *          of an intermediate result.
			 *
			 * @param result buffer for writing the hashing result
			 * @param length size of the hashing result in bytes
			 *
			 * \~
			 */
			void digest(uint8_t * result, const size_t length) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования 64-битного результата потокового хэширования
			 *
			 * @details Метод выводит первые восемь октетов результата в виде числа,
			 *          минуя формирование потока октетов, и предназначен для наиболее
			 *          частого случая — хэширования ключей ассоциативных контейнеров.
			 *
			 * @return результат хэширования
			 *
			 * \~english
			 * @brief Method forming the 64-bit result of the streaming hashing
			 *
			 * @details The method outputs the first eight octets of the result as a number,
			 *          bypassing the forming of the stream of octets, and is intended for the most
			 *          frequent case — the hashing of the keys of associative containers.
			 *
			 * @return hashing result
			 *
			 * \~
			 */
			uint64_t digest() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод формирования хэша буфера данных
			 *
			 * @details Метод состояние потокового хэширования не затрагивает.
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер буфера данных для хэширования
			 * @param result буфер для записи результата хэширования
			 * @param length размер результата хэширования в байтах
			 *
			 * \~english
			 * @brief Method forming the hash of a data buffer
			 *
			 * @details The method does not affect the state of the streaming hashing.
			 *
			 * @param buffer data buffer for the hashing
			 * @param size   size of the data buffer for the hashing
			 * @param result buffer for writing the hashing result
			 * @param length size of the hashing result in bytes
			 *
			 * \~
			 */
			void hash(const void * buffer, const size_t size, uint8_t * result, const size_t length) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод формирования результата потокового хэширования
			 *
			 * @return результат хэширования
			 *
			 * \~english
			 * @brief Method forming the result of the streaming hashing
			 *
			 * @return hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T digest() const noexcept {
				// Выполняем проверку типа результата хэширования на пригодность
				static_assert(!std::is_same_v <T, bool>, "AWH hash: the result of hashing does not fit in a boolean type, use an integer type of the required width");
				/**
				 * Если результатом хэширования является целое число, умещающееся
				 * в разрядность вычислительного движка хэширования
				 */
				if constexpr(hashing::numeric <T>)
					// Выводим младшие разряды сформированного хэша
					return static_cast <T> (this->digest());
				/**
				 * Если результат хэширования формируется в буфере
				 */
				else {
					// Результат работы функции
					T result;
					// Буфер сформированного хэша
					uint8_t buffer[sizeof(T)];
					// Выполняем формирование результата потокового хэширования
					this->digest(buffer, sizeof(T));
					// Выполняем запись сформированного хэша в результат хэширования
					hashing::assign(result, buffer);
					// Выводим результат работы функции
					return result;
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод формирования хэша буфера данных
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер буфера данных для хэширования
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method forming the hash of a data buffer
			 *
			 * @param buffer data buffer for the hashing
			 * @param size   size of the data buffer for the hashing
			 * @return       hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hash(const void * buffer, const size_t size) const noexcept {
				// Выполняем формирование хэша буфера данных
				return hashing::create <T> (buffer, size, this->_seed);
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод формирования хэша текста
			 *
			 * @param text текст для хэширования
			 * @return     результат хэширования
			 *
			 * \~english
			 * @brief Method forming the hash of a text
			 *
			 * @param text text for the hashing
			 * @return     hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hash(string_view text) const noexcept {
				// Выполняем формирование хэша текста
				return this->hash <T> (text.data(), text.size());
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result and of the type of the data buffer
			 *
			 * @tparam A type of the hashing result
			 * @tparam B type of the data buffer for the hashing
			 *
			 * \~
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * \~russian
			 * @brief Метод формирования хэша буфера данных
			 *
			 * @details Отбором служит наличие у буфера непрерывного хранилища, а не
			 *          одного лишь типа хранимого значения: набор логических значений
			 *          стандартной библиотеки тип хранимого значения объявляет, но
			 *          хранит его упакованным по битам и хранилища не отдаёт, - и
			 *          отбор по типу значения приводил его сюда, обрывая сборку
			 *
			 * @param buffer буфер данных для хэширования
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method forming the hash of a data buffer
			 *
			 * @details The selection is served by the presence of contiguous storage in the buffer rather than
			 *          by the type of the stored value alone: the set of boolean values of
			 *          the standard library declares the type of the stored value, but
			 *          stores it packed by bits and does not give out the storage, - and
			 *          the selection by the type of the value brought it here, breaking the build
			 *
			 * @param buffer data buffer for the hashing
			 * @return       hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE A hash(const B & buffer) const noexcept {
				// Выполняем формирование хэша буфера данных
				return this->hash <A> (buffer.data(), buffer.size() * sizeof(typename B::value_type));
			}
		public:
			/**
			 * \~russian
			 * @brief Метод добавления текста в потоковое хэширование
			 *
			 * @param text текст для хэширования
			 *
			 * \~english
			 * @brief Method adding a text into the streaming hashing
			 *
			 * @param text text for the hashing
			 *
			 * \~
			 */
			AWH_HASH_INLINE void update(string_view text) noexcept {
				// Выполняем добавление текста в потоковое хэширование
				this->update(text.data(), text.size());
			}
			/**
			 * \~russian
			 * @brief Шаблон типа буфера данных для хэширования
			 *
			 * @tparam B тип буфера данных для хэширования
			 *
			 * \~english
			 * @brief Template of the type of the data buffer for the hashing
			 *
			 * @tparam B type of the data buffer for the hashing
			 *
			 * \~
			 */
			template <typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * \~russian
			 * @brief Метод добавления данных в потоковое хэширование
			 *
			 * @details Отбором служит наличие у буфера непрерывного хранилища —
			 *          по тому же доводу, что и у формирования хэша буфера данных
			 *
			 * @param buffer буфер данных для хэширования
			 *
			 * \~english
			 * @brief Method adding data into the streaming hashing
			 *
			 * @details The selection is served by the presence of contiguous storage in the buffer —
			 *          by the same argument as for the forming of the hash of a data buffer
			 *
			 * @param buffer data buffer for the hashing
			 *
			 * \~
			 */
			AWH_HASH_INLINE void update(const B & buffer) noexcept {
				// Выполняем добавление данных в потоковое хэширование
				this->update(buffer.data(), buffer.size() * sizeof(typename B::value_type));
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон разрядности и типа длинного числа для хэширования
			 *
			 * @tparam BYTES размер длинного числа в байтах
			 * @tparam TYPE  тип хранимого длинного числа
			 *
			 * \~english
			 * @brief Template of the width and of the type of the long number for the hashing
			 *
			 * @tparam BYTES size of the long number in bytes
			 * @tparam TYPE  type of the stored long number
			 *
			 * \~
			 */
			template <uint16_t BYTES, bignum::type_t TYPE>
			/**
			 * \~russian
			 * @brief Метод добавления длинного числа в потоковое хэширование
			 *
			 * @param num длинное число для хэширования
			 *
			 * \~english
			 * @brief Method adding a long number into the streaming hashing
			 *
			 * @param num long number for the hashing
			 *
			 * \~
			 */
			AWH_HASH_INLINE void update(const BigNum <BYTES, TYPE> & num) noexcept {
				// Выполняем добавление длинного числа в потоковое хэширование
				this->update(num.data(), BYTES);
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования, разрядности и типа длинного числа
			 *
			 * @tparam T     тип результата хэширования
			 * @tparam BYTES размер длинного числа в байтах
			 * @tparam TYPE  тип хранимого длинного числа
			 *
			 * \~english
			 * @brief Template of the type of the hashing result, of the width and of the type of the long number
			 *
			 * @tparam T     type of the hashing result
			 * @tparam BYTES size of the long number in bytes
			 * @tparam TYPE  type of the stored long number
			 *
			 * \~
			 */
			template <typename T = uint64_t, uint16_t BYTES, bignum::type_t TYPE>
			/**
			 * \~russian
			 * @brief Метод формирования хэша длинного числа
			 *
			 * @param num длинное число для хэширования
			 * @return    результат хэширования
			 *
			 * \~english
			 * @brief Method forming the hash of a long number
			 *
			 * @param num long number for the hashing
			 * @return    hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hash(const BigNum <BYTES, TYPE> & num) const noexcept {
				// Выполняем формирование хэша длинного числа
				return this->hash <T> (num.data(), BYTES);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор [()] формирования хэша текста
			 *
			 * @param text текст для хэширования
			 * @return     результат хэширования
			 *
			 * \~english
			 * @brief Operator [()] forming the hash of a text
			 *
			 * @param text text for the hashing
			 * @return     hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE uint64_t operator () (string_view text) const noexcept {
				// Выполняем формирование хэша текста
				return this->hash <uint64_t> (text.data(), text.size());
			}
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
			Hash() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param seed начальное значение хэширования
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param seed hashing seed value
			 *
			 * \~
			 */
			Hash(const uint64_t seed) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Hash() noexcept {}
	} hash_t;
};

/**
 * \~russian
 * @brief Пространство имён стандартной библиотеки
 *
 * \~english
 * @brief Namespace of the standard library
 *
 * \~
 */
namespace std {
	/**
	 * \~russian
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер длинного числа в байтах
	 * @tparam TYPE  тип хранимого длинного числа
	 *
	 * \~english
	 * @brief Template of the width and of the type of the long number
	 *
	 * @tparam BYTES size of the long number in bytes
	 * @tparam TYPE  type of the stored long number
	 *
	 * \~
	 */
	template <uint16_t BYTES, awh::bignum::type_t TYPE>
	/**
	 * \~russian
	 * @brief Специализация шаблона хэширования длинного числа
	 *
	 * @details Специализация позволяет использовать длинное число произвольной
	 *          разрядности в качестве ключа ассоциативных контейнеров стандартной
	 *          библиотеки, построенных на хэш-таблицах.
	 *
	 * \~english
	 * @brief Specialization of the template of the hashing of a long number
	 *
	 * @details The specialization makes it possible to use a long number of arbitrary
	 *          width as a key of the associative containers of the standard
	 *          library built upon hash tables.
	 *
	 * \~
	 */
	struct hash <awh::BigNum <BYTES, TYPE>> {
		/**
		 * \~russian
		 * @brief Оператор [()] формирования хэша длинного числа
		 *
		 * @param num длинное число для хэширования
		 * @return    результат хэширования
		 *
		 * \~english
		 * @brief Operator [()] forming the hash of a long number
		 *
		 * @param num long number for the hashing
		 * @return    hashing result
		 *
		 * \~
		 */
		size_t operator () (const awh::BigNum <BYTES, TYPE> & num) const noexcept {
			// Выполняем формирование хэша длинного числа
			return awh::hashing::create <size_t> (num.data(), BYTES);
		}
	};
};

#endif // __AWH_HASH__
