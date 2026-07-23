/**
 * @file: bigint.hpp
 * @date: 2026-07-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 *
 * Адаптировано из fast_float (https://github.com/fastfloat/fast_float)
 * Copyright 2021 The fast_float authors — MIT License.
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FLOAT_BIGINT__
#define __AWH_FLOAT_BIGINT__

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>

/**
 * Подключаем общие определения модуля
 */
#include "common.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён модуля чисел с плавающей точкой
	 *
	 */
	namespace floating {
		/**
		 * @brief Ширина limb для арифметики большого целого
		 *
		 * @details На 64-битных платформах (кроме SPARC) используем uint64_t,
		 *          чтобы эффективно получать старшую и младшую части произведения.
		 *          Иначе — uint32_t.
		 */
		#if defined(AWH_FLOAT_64BIT) && !defined(__sparc)
			#define AWH_FLOAT_64BIT_LIMB 1
			using limb = uint64_t;
			constexpr size_t limbBits = 64;
		#else
			#define AWH_FLOAT_32BIT_LIMB
			using limb = uint32_t;
			constexpr size_t limbBits = 32;
		#endif

		/**
		 * @brief Диапазон limb
		 *
		 */
		using limbSpan = span <limb>;

		/**
		 * @brief Максимальная битность bigint
		 *
		 * @details Нужно не меньше log2(10^(767 + 342)) ≈ 3600 бит; берём 4000.
		 */
		constexpr size_t bigintBits = 4000;

		/**
		 * @brief Максимальное число limb в bigint
		 *
		 */
		constexpr size_t bigintLimbs = bigintBits / limbBits;

		/**
		 * @brief Вектор limb фиксированной ёмкости на стеке
		 *
		 * @tparam size максимальное число элементов
		 */
		template <uint16_t size>
		struct stackvec {
			limb data[size];      // Буфер limb
			uint16_t length{0};   // Текущая длина (не больше size)

			/**
			 * @brief Конструктор по умолчанию
			 *
			 */
			stackvec() = default;

			/**
			 * @brief Копирование запрещено
			 *
			 */
			stackvec(const stackvec &) = delete;
			stackvec & operator = (const stackvec &) = delete;

			/**
			 * @brief Перемещение запрещено
			 *
			 */
			stackvec(stackvec &&) = delete;
			stackvec & operator = (stackvec &&) = delete;

			/**
			 * @brief Конструктор из диапазона limb
			 *
			 * @param s исходный диапазон
			 */
			AWH_FLOAT_CONSTEXPR20 stackvec(const limbSpan s) {
				// Копируем элементы диапазона
				AWH_FLOAT_ASSERT(tryExtend(s));
			}

			/**
			 * @brief Оператор доступа к элементу
			 *
			 * @param index индекс элемента
			 * @return      ссылка на limb
			 */
			constexpr limb & operator [] (const size_t index) noexcept {
				AWH_FLOAT_DEBUG_ASSERT(index < length);
				return data[index];
			}

			/**
			 * @brief Константный оператор доступа к элементу
			 *
			 * @param index индекс элемента
			 * @return      константная ссылка на limb
			 */
			constexpr const limb & operator [] (const size_t index) const noexcept {
				AWH_FLOAT_DEBUG_ASSERT(index < length);
				return data[index];
			}

			/**
			 * @brief Доступ к элементу от старшего конца
			 *
			 * @param index индекс от старшего limb (0 — самый старший)
			 * @return      константная ссылка на limb
			 */
			constexpr const limb & rindex(const size_t index) const noexcept {
				AWH_FLOAT_DEBUG_ASSERT(index < length);
				return data[length - index - 1];
			}

			/**
			 * @brief Устанавливает длину без проверки границ
			 *
			 * @param len новая длина
			 */
			constexpr void setLen(const size_t len) noexcept {
				length = uint16_t(len);
			}

			/**
			 * @brief Возвращает текущую длину
			 *
			 * @return длина вектора
			 */
			constexpr size_t len() const noexcept {
				return length;
			}

			/**
			 * @brief Проверяет, пуст ли вектор
			 *
			 * @return true, если элементов нет
			 */
			constexpr bool isEmpty() const noexcept {
				return (length == 0);
			}

			/**
			 * @brief Возвращает ёмкость вектора
			 *
			 * @return максимальное число элементов
			 */
			constexpr size_t capacity() const noexcept {
				return size;
			}

			/**
			 * @brief Добавляет элемент без проверки границ
			 *
			 * @param value добавляемое значение
			 */
			constexpr void pushUnchecked(const limb value) noexcept {
				data[length] = value;
				++length;
			}

			/**
			 * @brief Пытается добавить элемент
			 *
			 * @param value добавляемое значение
			 * @return       true, если элемент добавлен
			 */
			constexpr bool tryPush(const limb value) noexcept {
				if(len() < capacity()){
					pushUnchecked(value);
					return true;
				}
				return false;
			}

			/**
			 * @brief Добавляет диапазон без проверки границ
			 *
			 * @param s добавляемый диапазон
			 */
			AWH_FLOAT_CONSTEXPR20 void extendUnchecked(const limbSpan s) noexcept {
				limb * const ptr = data + length;
				copy_n(s.ptr, s.len(), ptr);
				setLen(len() + s.len());
			}

			/**
			 * @brief Пытается добавить диапазон
			 *
			 * @param s добавляемый диапазон
			 * @return  true, если диапазон добавлен
			 */
			AWH_FLOAT_CONSTEXPR20 bool tryExtend(const limbSpan s) noexcept {
				if((len() + s.len()) <= capacity()){
					extendUnchecked(s);
					return true;
				}
				return false;
			}

			/**
			 * @brief Меняет размер без проверки границ
			 *
			 * @param newLen новая длина
			 * @param value  значение для новых элементов
			 */
			AWH_FLOAT_CONSTEXPR20 void resizeUnchecked(const size_t newLen, const limb value) noexcept {
				if(newLen > len()){
					const size_t count = newLen - len();
					limb * const first = data + len();
					limb * const last = first + count;
					fill(first, last, value);
					setLen(newLen);
				} else {
					setLen(newLen);
				}
			}

			/**
			 * @brief Пытается изменить размер вектора
			 *
			 * @param newLen новая длина
			 * @param value  значение для новых элементов
			 * @return        true, если размер изменён
			 */
			AWH_FLOAT_CONSTEXPR20 bool tryResize(const size_t newLen, const limb value) noexcept {
				if(newLen > capacity())
					return false;
				resizeUnchecked(newLen, value);
				return true;
			}

			/**
			 * @brief Проверяет наличие ненулевых limb начиная с индекса от старшего конца
			 *
			 * @param index стартовый индекс от старшего limb
			 * @return      true, если найден ненулевой limb
			 */
			constexpr bool nonzero(size_t index) const noexcept {
				while(index < len()){
					if(rindex(index) != 0)
						return true;
					++index;
				}
				return false;
			}

			/**
			 * @brief Нормализует число: удаляет старшие нулевые limb
			 *
			 */
			constexpr void normalize() noexcept {
				while((len() > 0) && (rindex(0) == 0))
					--length;
			}
		};

		/**
		 * @brief Возвращает пустые старшие 64 бита
		 *
		 * @param truncated признак усечения младших бит
		 * @return          всегда 0
		 */
		AWH_FLOAT_INLINE constexpr uint64_t emptyHi64(bool & truncated) noexcept {
			truncated = false;
			return 0;
		}

		/**
		 * @brief Нормализует одно 64-битное слово к старшим 64 битам
		 *
		 * @param r0        исходное значение
		 * @param truncated признак усечения
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint64_t uint64Hi64(const uint64_t r0, bool & truncated) noexcept {
			truncated = false;
			const int shl = leadingZeroes(r0);
			return (r0 << shl);
		}

		/**
		 * @brief Собирает старшие 64 бита из двух 64-битных слов
		 *
		 * @param r0        старшее слово
		 * @param r1        младшее слово
		 * @param truncated признак усечения
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint64_t uint64Hi64(const uint64_t r0, const uint64_t r1, bool & truncated) noexcept {
			const int shl = leadingZeroes(r0);
			if(shl == 0){
				truncated = (r1 != 0);
				return r0;
			}
			const int shr = 64 - shl;
			truncated = ((r1 << shl) != 0);
			return ((r0 << shl) | (r1 >> shr));
		}

		/**
		 * @brief Нормализует одно 32-битное слово к старшим 64 битам
		 *
		 * @param r0        исходное значение
		 * @param truncated признак усечения
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint64_t uint32Hi64(const uint32_t r0, bool & truncated) noexcept {
			return uint64Hi64(r0, truncated);
		}

		/**
		 * @brief Собирает старшие 64 бита из двух 32-битных слов
		 *
		 * @param r0        старшее слово
		 * @param r1        младшее слово
		 * @param truncated признак усечения
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint64_t uint32Hi64(const uint32_t r0, const uint32_t r1, bool & truncated) noexcept {
			const uint64_t x0 = r0;
			const uint64_t x1 = r1;
			return uint64Hi64((x0 << 32) | x1, truncated);
		}

		/**
		 * @brief Собирает старшие 64 бита из трёх 32-битных слов
		 *
		 * @param r0        старшее слово
		 * @param r1        среднее слово
		 * @param r2        младшее слово
		 * @param truncated признак усечения
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint64_t uint32Hi64(const uint32_t r0, const uint32_t r1, const uint32_t r2, bool & truncated) noexcept {
			const uint64_t x0 = r0;
			const uint64_t x1 = r1;
			const uint64_t x2 = r2;
			return uint64Hi64(x0, (x1 << 32) | x2, truncated);
		}

		/**
		 * @brief Складывает два limb с контролем переполнения
		 *
		 * @param x         первое слагаемое
		 * @param y         второе слагаемое
		 * @param overflow  признак переполнения
		 * @return          сумма по модулю 2^limbBits
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 limb scalarAdd(const limb x, const limb y, bool & overflow) noexcept {
			limb z = 0;
			#if defined(__has_builtin)
				#if __has_builtin(__builtin_add_overflow)
					if(!cpp20AndInConstexpr()){
						overflow = __builtin_add_overflow(x, y, &z);
						return z;
					}
				#endif
			#endif
			// Универсальный путь (в т.ч. для MSVC)
			z = x + y;
			overflow = (z < x);
			return z;
		}

		/**
		 * @brief Умножает два limb с накоплением переноса
		 *
		 * @param x     множитель
		 * @param y     множитель
		 * @param carry входной/выходной перенос
		 * @return      младшая часть произведения
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 limb scalarMul(const limb x, const limb y, limb & carry) noexcept {
			#ifdef AWH_FLOAT_64BIT_LIMB
				#if defined(__SIZEOF_INT128__)
					const __uint128_t z = (__uint128_t(x) * __uint128_t(y)) + __uint128_t(carry);
					carry = limb(z >> limbBits);
					return limb(z);
				#else
					value128 z = fullMultiplication(x, y);
					bool overflow = false;
					z.low = scalarAdd(z.low, carry, overflow);
					z.high += uint64_t(overflow);
					carry = z.high;
					return z.low;
				#endif
			#else
				const uint64_t z = (uint64_t(x) * uint64_t(y)) + uint64_t(carry);
				carry = limb(z >> limbBits);
				return limb(z);
			#endif
		}

		/**
		 * @brief Добавляет скаляр к bigint начиная с заданного индекса
		 *
		 * @tparam size ёмкость stackvec
		 * @param vec   целевой вектор
		 * @param y     добавляемое значение
		 * @param start индекс начала
		 * @return      true при успехе
		 */
		template <uint16_t size>
		inline AWH_FLOAT_CONSTEXPR20 bool smallAddFrom(stackvec <size> & vec, const limb y, const size_t start) noexcept {
			size_t index = start;
			limb carry = y;
			bool overflow = false;
			while((carry != 0) && (index < vec.len())){
				vec[index] = scalarAdd(vec[index], carry, overflow);
				carry = limb(overflow);
				++index;
			}
			if(carry != 0)
				AWH_FLOAT_TRY(vec.tryPush(carry));
			return true;
		}

		/**
		 * @brief Добавляет скаляр к bigint
		 *
		 * @tparam size ёмкость stackvec
		 * @param vec   целевой вектор
		 * @param y     добавляемое значение
		 * @return      true при успехе
		 */
		template <uint16_t size>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 bool smallAdd(stackvec <size> & vec, const limb y) noexcept {
			return smallAddFrom(vec, y, 0);
		}

		/**
		 * @brief Умножает bigint на скаляр
		 *
		 * @tparam size ёмкость stackvec
		 * @param vec   целевой вектор
		 * @param y     множитель
		 * @return      true при успехе
		 */
		template <uint16_t size>
		inline AWH_FLOAT_CONSTEXPR20 bool smallMul(stackvec <size> & vec, const limb y) noexcept {
			limb carry = 0;
			for(size_t index = 0; index < vec.len(); ++index)
				vec[index] = scalarMul(vec[index], y, carry);
			if(carry != 0)
				AWH_FLOAT_TRY(vec.tryPush(carry));
			return true;
		}

		/**
		 * @brief Складывает bigint с диапазоном limb начиная с индекса
		 *
		 * @tparam size ёмкость stackvec
		 * @param x     целевой вектор
		 * @param y     добавляемый диапазон
		 * @param start индекс начала
		 * @return      true при успехе
		 */
		template <uint16_t size>
		AWH_FLOAT_CONSTEXPR20 bool largeAddFrom(stackvec <size> & x, const limbSpan y, const size_t start) noexcept {
			// При необходимости расширяем буфер
			if((x.len() < start) || (y.len() > (x.len() - start)))
				AWH_FLOAT_TRY(x.tryResize(y.len() + start, 0));

			bool carry = false;
			for(size_t index = 0; index < y.len(); ++index){
				limb xi = x[index + start];
				const limb yi = y[index];
				bool c1 = false;
				bool c2 = false;
				xi = scalarAdd(xi, yi, c1);
				if(carry)
					xi = scalarAdd(xi, 1, c2);
				x[index + start] = xi;
				carry = (c1 || c2);
			}

			// Обрабатываем финальный перенос
			if(carry)
				AWH_FLOAT_TRY(smallAddFrom(x, 1, y.len() + start));
			return true;
		}

		/**
		 * @brief Складывает bigint с диапазоном limb
		 *
		 * @tparam size ёмкость stackvec
		 * @param x     целевой вектор
		 * @param y     добавляемый диапазон
		 * @return      true при успехе
		 */
		template <uint16_t size>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 bool largeAddFrom(stackvec <size> & x, const limbSpan y) noexcept {
			return largeAddFrom(x, y, 0);
		}

		/**
		 * @brief Умножает bigint на диапазон limb школьным алгоритмом
		 *
		 * @tparam size ёмкость stackvec
		 * @param x     целевой вектор (множимое/результат)
		 * @param y     множитель
		 * @return      true при успехе
		 */
		template <uint16_t size>
		AWH_FLOAT_CONSTEXPR20 bool longMul(stackvec <size> & x, const limbSpan y) noexcept {
			const limbSpan xs = limbSpan(x.data, x.len());
			stackvec <size> z(xs);
			const limbSpan zs = limbSpan(z.data, z.len());

			if(y.len() != 0){
				const limb y0 = y[0];
				AWH_FLOAT_TRY(smallMul(x, y0));
				for(size_t index = 1; index < y.len(); ++index){
					const limb yi = y[index];
					if(yi != 0){
						stackvec <size> zi;
						zi.setLen(0);
						AWH_FLOAT_TRY(zi.tryExtend(zs));
						AWH_FLOAT_TRY(smallMul(zi, yi));
						const limbSpan zis = limbSpan(zi.data, zi.len());
						AWH_FLOAT_TRY(largeAddFrom(x, zis, index));
					}
				}
			}

			x.normalize();
			return true;
		}

		/**
		 * @brief Умножает bigint на диапазон limb
		 *
		 * @tparam size ёмкость stackvec
		 * @param x     целевой вектор
		 * @param y     множитель
		 * @return      true при успехе
		 */
		template <uint16_t size>
		AWH_FLOAT_CONSTEXPR20 bool largeMul(stackvec <size> & x, const limbSpan y) noexcept {
			if(y.len() == 1){
				AWH_FLOAT_TRY(smallMul(x, y[0]));
			} else {
				AWH_FLOAT_TRY(longMul(x, y));
			}
			return true;
		}

		/**
		 * @brief Таблицы степеней пяти для ускорения pow5
		 *
		 * @tparam unused фиктивный параметр для ODR constexpr
		 */
		template <typename = void>
		struct pow5Tables {
			static constexpr uint32_t largeStep = 135;
			static constexpr uint64_t smallPowerOf5[] = {
				1UL, 5UL, 25UL, 125UL, 625UL, 3125UL, 15625UL, 78125UL, 390625UL,
				1953125UL, 9765625UL, 48828125UL, 244140625UL, 1220703125UL,
				6103515625UL, 30517578125UL, 152587890625UL, 762939453125UL,
				3814697265625UL, 19073486328125UL, 95367431640625UL,
				476837158203125UL, 2384185791015625UL, 11920928955078125UL,
				59604644775390625UL, 298023223876953125UL, 1490116119384765625UL,
				7450580596923828125UL
			};
			#ifdef AWH_FLOAT_64BIT_LIMB
				constexpr static limb largePowerOf5[] = {
					1414648277510068013UL, 9180637584431281687UL, 4539964771860779200UL,
					10482974169319127550UL, 198276706040285095UL
				};
			#else
				constexpr static limb largePowerOf5[] = {
					4279965485U, 329373468U, 4020270615U, 2137533757U, 4287402176U,
					1057042919U, 1071430142U, 2440757623U, 381945767U, 46164893U
				};
			#endif
		};


		/**
		 * @brief Большое целое для точного округления float
		 *
		 * @details Используются простые алгоритмы: при малом числе limb
		 *          они быстрее асимптотически «быстрых» методов.
		 *          Все операции предполагают нормализованное представление.
		 */
		struct bigint : pow5Tables <> {
			stackvec <bigintLimbs> vec; // limb в порядке little-endian

			/**
			 * @brief Конструктор по умолчанию
			 *
			 */
			AWH_FLOAT_CONSTEXPR20 bigint() : vec() {}

			/**
			 * @brief Копирование запрещено
			 *
			 */
			bigint(const bigint &) = delete;
			bigint & operator = (const bigint &) = delete;

			/**
			 * @brief Перемещение запрещено
			 *
			 */
			bigint(bigint &&) = delete;
			bigint & operator = (bigint &&) = delete;

			/**
			 * @brief Конструктор из uint64_t
			 *
			 * @param value исходное значение
			 */
			AWH_FLOAT_CONSTEXPR20 bigint(const uint64_t value) : vec() {
				#ifdef AWH_FLOAT_64BIT_LIMB
					vec.pushUnchecked(value);
				#else
					vec.pushUnchecked(uint32_t(value));
					vec.pushUnchecked(uint32_t(value >> 32));
				#endif
				vec.normalize();
			}

			/**
			 * @brief Возвращает старшие 64 бита и признак усечения
			 *
			 * @param truncated признак наличия отброшенных младших бит
			 * @return          старшие 64 бита
			 */
			AWH_FLOAT_CONSTEXPR20 uint64_t hi64(bool & truncated) const noexcept {
				#ifdef AWH_FLOAT_64BIT_LIMB
					if(vec.len() == 0)
						return emptyHi64(truncated);
					if(vec.len() == 1)
						return uint64Hi64(vec.rindex(0), truncated);
					{
						const uint64_t result = uint64Hi64(vec.rindex(0), vec.rindex(1), truncated);
						truncated |= vec.nonzero(2);
						return result;
					}
				#else
					if(vec.len() == 0)
						return emptyHi64(truncated);
					if(vec.len() == 1)
						return uint32Hi64(vec.rindex(0), truncated);
					if(vec.len() == 2)
						return uint32Hi64(vec.rindex(0), vec.rindex(1), truncated);
					{
						const uint64_t result = uint32Hi64(vec.rindex(0), vec.rindex(1), vec.rindex(2), truncated);
						truncated |= vec.nonzero(3);
						return result;
					}
				#endif
			}

			/**
			 * @brief Сравнивает два нормализованных bigint
			 *
			 * @param other второе число
			 * @return      >0 если this больше, <0 если меньше, 0 если равны
			 */
			AWH_FLOAT_CONSTEXPR20 int compare(const bigint & other) const noexcept {
				if(vec.len() > other.vec.len())
					return 1;
				if(vec.len() < other.vec.len())
					return -1;
				for(size_t index = vec.len(); index > 0; --index){
					const limb xi = vec[index - 1];
					const limb yi = other.vec[index - 1];
					if(xi > yi)
						return 1;
					if(xi < yi)
						return -1;
				}
				return 0;
			}

			/**
			 * @brief Сдвигает каждый limb влево на n бит с переносом
			 *
			 * @param n величина сдвига в битах (1 .. limbBits-1)
			 * @return  true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool shlBits(const size_t n) noexcept {
				AWH_FLOAT_DEBUG_ASSERT(n != 0);
				AWH_FLOAT_DEBUG_ASSERT(n < (sizeof(limb) * 8));

				const size_t shl = n;
				const size_t shr = limbBits - shl;
				limb prev = 0;
				for(size_t index = 0; index < vec.len(); ++index){
					const limb xi = vec[index];
					vec[index] = (xi << shl) | (prev >> shr);
					prev = xi;
				}

				const limb carry = prev >> shr;
				if(carry != 0)
					return vec.tryPush(carry);
				return true;
			}

			/**
			 * @brief Сдвигает представление влево на n limb
			 *
			 * @param n число limb
			 * @return  true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool shlLimbs(const size_t n) noexcept {
				AWH_FLOAT_DEBUG_ASSERT(n != 0);
				if((n + vec.len()) > vec.capacity())
					return false;
				if(!vec.isEmpty()){
					limb * const dst = vec.data + n;
					const limb * const src = vec.data;
					copy_backward(src, src + vec.len(), dst + vec.len());
					limb * const first = vec.data;
					limb * const last = first + n;
					fill(first, last, 0);
					vec.setLen(n + vec.len());
				}
				return true;
			}

			/**
			 * @brief Сдвигает bigint влево на n бит
			 *
			 * @param n величина сдвига
			 * @return  true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool shl(const size_t n) noexcept {
				const size_t rem = n % limbBits;
				const size_t div = n / limbBits;
				if(rem != 0)
					AWH_FLOAT_TRY(shlBits(rem));
				if(div != 0)
					AWH_FLOAT_TRY(shlLimbs(div));
				return true;
			}

			/**
			 * @brief Возвращает число ведущих нулевых бит
			 *
			 * @return количество ведущих нулей
			 */
			AWH_FLOAT_CONSTEXPR20 int ctlz() const noexcept {
				if(vec.isEmpty())
					return 0;
				#ifdef AWH_FLOAT_64BIT_LIMB
					return leadingZeroes(vec.rindex(0));
				#else
					const uint64_t r0 = vec.rindex(0);
					return leadingZeroes(r0 << 32);
				#endif
			}

			/**
			 * @brief Возвращает битовую длину числа
			 *
			 * @return число значащих бит
			 */
			AWH_FLOAT_CONSTEXPR20 int bitLength() const noexcept {
				return int(limbBits * vec.len()) - ctlz();
			}

			/**
			 * @brief Умножает bigint на скаляр
			 *
			 * @param y множитель
			 * @return  true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool mul(const limb y) noexcept {
				return smallMul(vec, y);
			}

			/**
			 * @brief Добавляет скаляр к bigint
			 *
			 * @param y слагаемое
			 * @return  true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool add(const limb y) noexcept {
				return smallAdd(vec, y);
			}

			/**
			 * @brief Умножает на степень двойки
			 *
			 * @param exp показатель степени
			 * @return    true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool pow2(const uint32_t exp) noexcept {
				return shl(exp);
			}

			/**
			 * @brief Умножает на степень пяти
			 *
			 * @param exp показатель степени
			 * @return    true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool pow5(uint32_t exp) noexcept {
				const size_t largeLength = sizeof(largePowerOf5) / sizeof(limb);
				const limbSpan large = limbSpan(largePowerOf5, largeLength);

				/**
				 *  Крупные шаги по таблице largePowerOf5
				 */
				while(exp >= largeStep){
					AWH_FLOAT_TRY(largeMul(vec, large));
					exp -= largeStep;
				}

				#ifdef AWH_FLOAT_64BIT_LIMB
					constexpr uint32_t smallStep = 27;
					constexpr limb maxNative = 7450580596923828125UL;
				#else
					constexpr uint32_t smallStep = 13;
					constexpr limb maxNative = 1220703125U;
				#endif

				/**
				 *  Средние шаги нативным скаляром
				 */
				while(exp >= smallStep){
					AWH_FLOAT_TRY(smallMul(vec, maxNative));
					exp -= smallStep;
				}

				// Остаток из smallPowerOf5
				if(exp != 0){
					// Обход бага clang с индексированием constexpr-массива
					AWH_FLOAT_TRY(smallMul(vec, limb(((void) smallPowerOf5[0], smallPowerOf5[exp]))));
				}
				return true;
			}

			/**
			 * @brief Умножает на степень десяти
			 *
			 * @param exp показатель степени
			 * @return    true при успехе
			 */
			AWH_FLOAT_CONSTEXPR20 bool pow10(const uint32_t exp) noexcept {
				AWH_FLOAT_TRY(pow5(exp));
				return pow2(exp);
			}
		};
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_BIGINT__
