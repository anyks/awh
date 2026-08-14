/**
 * @file bignum.hpp
 * @date 2026-07-26
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
 * @brief Заголовочный файл тестовой фикстуры модуля работы с длинными числами — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BIGNUM_TESTS__
#define __AWH_BIGNUM_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/num/bignum.hpp"

/**
 * @brief Класс фикстуры для тестов длинных чисел
 *
 * @details Фикстура создаёт по одному представителю каждого из трёх видов длинных чисел —
 *          знакового целого, беззнакового целого и вещественного, а также предоставляет
 *          помощники битового сравнения длинного числа с эталонным нативным типом.
 *
 */
class BigNumFixture : public testing::Test {
	protected:
		// Объект знакового длинного целого числа
		std::unique_ptr <awh::int128_t> _integer;
		// Объект беззнакового длинного целого числа
		std::unique_ptr <awh::uint128_t> _natural;
		// Объект вещественного длинного числа
		std::unique_ptr <awh::real64_t> _real;
	public:
		/**
		 * @brief Метод битового сравнения длинного числа с эталонным нативным значением
		 *
		 * @details Длинное число хранит данные в том же формате, что и одноимённый нативный тип,
		 *          поэтому побайтовое совпадение буферов означает полное совпадение значений
		 *          вместе со знаком нуля и битовым образом денормализованных значений.
		 *
		 * @tparam BYTES размер длинного числа в байтах
		 * @tparam TYPE  тип хранимого длинного числа
		 * @tparam T     тип эталонного нативного значения
		 * @param  num   длинное число для сравнения
		 * @param  value эталонное нативное значение
		 * @return       результат сравнения битовых образов
		 *
		 */
		template <uint16_t BYTES, awh::bignum::type_t TYPE, typename T>
		static bool identical(const awh::BigNum <BYTES, TYPE> & num, const T value) noexcept {
			// Выполняем проверку совпадения разрядности сравниваемых значений
			static_assert(sizeof(T) == BYTES, "AWH bignum tests: the reference type width must match the number width");
			// Выполняем побайтовое сравнение буферов
			return (::memcmp(num.data(), &value, BYTES) == 0);
		}
		/**
		 * @brief Метод создания длинного числа из битового образа нативного значения
		 *
		 * @details Прямая запись битового образа позволяет получить длинное число
		 *          без промежуточного преобразования через операторы присваивания,
		 *          что необходимо для проверки произвольных, в том числе
		 *          денормализованных и специальных, битовых комбинаций.
		 *
		 * @tparam T     тип создаваемого длинного числа
		 * @tparam V     тип эталонного нативного значения
		 * @param  value эталонное нативное значение
		 * @return       созданное длинное число
		 *
		 */
		template <typename T, typename V>
		static T unpack(const V value) noexcept {
			// Выполняем проверку совпадения разрядности переносимых значений
			static_assert(sizeof(V) == T::size(), "AWH bignum tests: the reference type width must match the number width");
			// Создаём длинное число
			T result;
			// Копируем битовый образ эталонного значения в буфер длинного числа
			::memcpy(result.data(), &value, sizeof(V));
			// Выводим результат
			return result;
		}
		/**
		 * @brief Метод извлечения битового образа длинного числа в нативное значение
		 *
		 * @tparam V     тип извлекаемого нативного значения
		 * @tparam BYTES размер длинного числа в байтах
		 * @tparam TYPE  тип хранимого длинного числа
		 * @param  num   длинное число для извлечения
		 * @return       извлечённое нативное значение
		 *
		 */
		template <typename V, uint16_t BYTES, awh::bignum::type_t TYPE>
		static V pack(const awh::BigNum <BYTES, TYPE> & num) noexcept {
			// Выполняем проверку совпадения разрядности переносимых значений
			static_assert(sizeof(V) == BYTES, "AWH bignum tests: the reference type width must match the number width");
			// Создаём нативное значение
			V result;
			// Копируем битовый образ длинного числа в нативное значение
			::memcpy(&result, num.data(), BYTES);
			// Выводим результат
			return result;
		}
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
};

#endif // __AWH_BIGNUM_TESTS__
