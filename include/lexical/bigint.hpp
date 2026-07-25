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
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_BIGINT__
#define __AWH_LEXICAL_BIGINT__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * Подключаем заголовочные файлы модуля
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
	 * @brief Пространство имён модуля разбора чисел
	 *
	 */
	namespace lexical {
		/**
		 * Определяем разрядность разряда длинной арифметики
		 *
		 * @details На 64-битных платформах, кроме SPARC, используется 64-битный
		 *          разряд, что позволяет эффективно получать старшую и младшую
		 *          части произведения. На остальных платформах используется
		 *          32-битный разряд.
		 */
		#if defined(AWH_LEXICAL_64BIT) && !defined(__sparc)
			/**
			 * Разряд длинной арифметики является 64-битным
			 */
			#define AWH_LEXICAL_64BIT_LIMB 1
			/**
			 * @brief Создаём тип данных разряда длинной арифметики
			 *
			 */
			using limb_t = uint64_t;
			/**
			 * @brief Разрядность одного разряда длинной арифметики
			 *
			 */
			constexpr size_t LIMB_BITS = 64;
		#else
			/**
			 * Разряд длинной арифметики является 32-битным
			 */
			#define AWH_LEXICAL_32BIT_LIMB 1
			/**
			 * @brief Создаём тип данных разряда длинной арифметики
			 *
			 */
			using limb_t = uint32_t;
			/**
			 * @brief Разрядность одного разряда длинной арифметики
			 *
			 */
			constexpr size_t LIMB_BITS = 32;
		#endif

		/**
		 * @brief Создаём тип данных диапазона разрядов длинной арифметики
		 *
		 */
		using limbSpan_t = span_t <limb_t>;

		/**
		 * @brief Максимальная разрядность значения длинной арифметики
		 *
		 * @details Точное округление требует не менее log2(10^(769 + 342)) ≈ 3700 бит,
		 *          округляем требование вверх до 4000 бит.
		 */
		constexpr size_t BIGINT_BITS = 4000;

		/**
		 * @brief Максимальное количество разрядов значения длинной арифметики
		 *
		 */
		constexpr size_t BIGINT_LIMBS = (BIGINT_BITS / LIMB_BITS);

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Структура вектора разрядов фиксированной ёмкости на стеке
		 *
		 * @details Буфер разрядов намеренно не инициализируется: значимыми
		 *          считаются только первые length элементов.
		 */
		struct limbs_t {
			// Буфер разрядов длинной арифметики
			limb_t data[SIZE];
			// Текущее количество значимых разрядов
			uint16_t length;
			/**
			 * @brief Конструктор
			 *
			 */
			limbs_t() noexcept : length(0) {}
			/**
			 * @brief Конструктор копирования
			 *
			 */
			limbs_t(const limbs_t &) = delete;
			/**
			 * @brief Конструктор перемещения
			 *
			 */
			limbs_t(limbs_t &&) = delete;
			/**
			 * @brief Оператор присваивания копированием
			 *
			 */
			limbs_t & operator = (const limbs_t &) = delete;
			/**
			 * @brief Оператор присваивания перемещением
			 *
			 */
			limbs_t & operator = (limbs_t &&) = delete;
			/**
			 * @brief Метод получения количества значимых разрядов
			 *
			 * @return количество значимых разрядов
			 */
			size_t len() const noexcept {
				// Выводим количество значимых разрядов
				return this->length;
			}
			/**
			 * @brief Метод получения ёмкости вектора разрядов
			 *
			 * @return максимальное количество разрядов
			 */
			size_t capacity() const noexcept {
				// Выводим ёмкость вектора разрядов
				return SIZE;
			}
			/**
			 * @brief Метод проверки вектора разрядов на пустоту
			 *
			 * @return результат проверки
			 */
			bool isEmpty() const noexcept {
				// Вектор пуст, если значимых разрядов нет
				return (this->length == 0);
			}
			/**
			 * @brief Метод установки количества значимых разрядов
			 *
			 * @param count устанавливаемое количество значимых разрядов
			 */
			void setLen(const size_t count) noexcept {
				// Выполняем проверку соблюдения ёмкости вектора
				AWH_LEXICAL_ASSERT(count <= SIZE);
				// Выполняем установку количества значимых разрядов
				this->length = static_cast <uint16_t> (count);
			}
			/**
			 * @brief Оператор доступа к разряду по индексу
			 *
			 * @param index индекс запрашиваемого разряда
			 * @return      ссылка на запрашиваемый разряд
			 */
			limb_t & operator [] (const size_t index) noexcept {
				// Выполняем проверку соблюдения границ вектора
				AWH_LEXICAL_ASSERT(index < this->length);
				// Выводим ссылку на запрашиваемый разряд
				return this->data[index];
			}
			/**
			 * @brief Оператор доступа к разряду по индексу
			 *
			 * @param index индекс запрашиваемого разряда
			 * @return      константная ссылка на запрашиваемый разряд
			 */
			const limb_t & operator [] (const size_t index) const noexcept {
				// Выполняем проверку соблюдения границ вектора
				AWH_LEXICAL_ASSERT(index < this->length);
				// Выводим константную ссылку на запрашиваемый разряд
				return this->data[index];
			}
			/**
			 * @brief Метод доступа к разряду по индексу от старшего конца
			 *
			 * @param index индекс запрашиваемого разряда от старшего конца
			 * @return      константная ссылка на запрашиваемый разряд
			 */
			const limb_t & rindex(const size_t index) const noexcept {
				// Выполняем проверку соблюдения границ вектора
				AWH_LEXICAL_ASSERT(index < this->length);
				// Выводим константную ссылку на запрашиваемый разряд
				return this->data[this->length - index - 1];
			}
			/**
			 * @brief Метод добавления разряда в конец вектора
			 *
			 * @param value добавляемый разряд
			 * @return      результат выполнения операции
			 */
			bool push(const limb_t value) noexcept {
				// Если ёмкость вектора исчерпана
				if(this->len() >= this->capacity())
					// Сообщаем, что разряд добавить не удалось
					return false;
				// Выполняем добавление разряда в конец вектора
				this->data[this->length] = value;
				// Увеличиваем количество значимых разрядов
				++this->length;
				// Сообщаем, что разряд успешно добавлен
				return true;
			}
			/**
			 * @brief Метод добавления диапазона разрядов в конец вектора
			 *
			 * @param values добавляемый диапазон разрядов
			 * @return       результат выполнения операции
			 */
			bool extend(const limbSpan_t values) noexcept {
				// Если ёмкости вектора недостаточно
				if((this->len() + values.len()) > this->capacity())
					// Сообщаем, что диапазон добавить не удалось
					return false;
				// Выполняем копирование диапазона в конец вектора
				std::copy_n(values.ptr, values.len(), this->data + this->length);
				// Увеличиваем количество значимых разрядов
				this->setLen(this->len() + values.len());
				// Сообщаем, что диапазон успешно добавлен
				return true;
			}
			/**
			 * @brief Метод изменения количества значимых разрядов
			 *
			 * @param count устанавливаемое количество значимых разрядов
			 * @param value значение для заполнения добавляемых разрядов
			 * @return      результат выполнения операции
			 */
			bool resize(const size_t count, const limb_t value) noexcept {
				// Если ёмкости вектора недостаточно
				if(count > this->capacity())
					// Сообщаем, что размер изменить не удалось
					return false;
				// Если вектор увеличивается
				if(count > this->len())
					// Выполняем заполнение добавляемых разрядов
					std::fill(this->data + this->len(), this->data + count, value);
				// Выполняем установку количества значимых разрядов
				this->setLen(count);
				// Сообщаем, что размер успешно изменён
				return true;
			}
			/**
			 * @brief Метод проверки наличия ненулевых разрядов от старшего конца
			 *
			 * @param index индекс начала проверки от старшего конца
			 * @return      результат проверки
			 */
			bool nonzero(size_t index) const noexcept {
				/**
				 * Выполняем перебор разрядов от указанной позиции
				 */
				while(index < this->len()){
					// Если очередной разряд не является нулевым
					if(this->rindex(index) != 0)
						// Сообщаем, что ненулевые разряды найдены
						return true;
					// Переходим к следующему разряду
					++index;
				}
				// Сообщаем, что ненулевых разрядов не найдено
				return false;
			}
			/**
			 * @brief Метод удаления старших нулевых разрядов
			 *
			 */
			void normalize() noexcept {
				/**
				 * Выполняем удаление старших нулевых разрядов
				 */
				while((this->length > 0) && (this->rindex(0) == 0))
					// Уменьшаем количество значимых разрядов
					--this->length;
			}
		};

		/**
		 * @brief Метод нормализации одного 64-битного слова к старшим 64 битам
		 *
		 * @param value     нормализуемое значение
		 * @param truncated ссылка на признак отброшенных значащих бит
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_LEXICAL_INLINE uint64_t toHigh64(const uint64_t value, bool & truncated) noexcept {
			// Отбрасывать при нормализации одного слова нечего
			truncated = false;
			// Если значение является нулевым
			if(value == 0)
				// Выводим нулевой результат
				return 0;
			// Выводим значение, сдвинутое до старшего бита
			return (value << leadingZeros(value));
		}

		/**
		 * @brief Метод сборки старших 64 бит из двух 64-битных слов
		 *
		 * @param high      старшее слово
		 * @param low       младшее слово
		 * @param truncated ссылка на признак отброшенных значащих бит
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_LEXICAL_INLINE uint64_t toHigh64(const uint64_t high, const uint64_t low, bool & truncated) noexcept {
			// Если старшее слово является нулевым
			if(high == 0){
				// Выводим результат нормализации младшего слова
				return toHigh64(low, truncated);
			}
			// Количество ведущих нулевых бит старшего слова
			const int32_t shl = leadingZeros(high);
			// Если старший бит слова уже установлен
			if(shl == 0){
				// Значащие биты отброшены, если младшее слово не нулевое
				truncated = (low != 0);
				// Выводим старшее слово без изменений
				return high;
			}
			// Величина обратного сдвига младшего слова
			const int32_t shr = (64 - shl);
			// Значащие биты отброшены, если младшее слово теряет установленные биты
			truncated = ((low << shl) != 0);
			// Выводим собранные старшие 64 бита
			return ((high << shl) | (low >> shr));
		}

		/**
		 * @brief Метод сборки старших 64 бит из двух 32-битных слов
		 *
		 * @param high      старшее слово
		 * @param low       младшее слово
		 * @param truncated ссылка на признак отброшенных значащих бит
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_LEXICAL_INLINE uint64_t toHigh64(const uint32_t high, const uint32_t low, bool & truncated) noexcept {
			// Выводим результат нормализации объединённого значения
			return toHigh64((static_cast <uint64_t> (high) << 32) | static_cast <uint64_t> (low), truncated);
		}

		/**
		 * @brief Метод сборки старших 64 бит из трёх 32-битных слов
		 *
		 * @param high      старшее слово
		 * @param middle    среднее слово
		 * @param low       младшее слово
		 * @param truncated ссылка на признак отброшенных значащих бит
		 * @return          нормализованные старшие 64 бита
		 */
		AWH_LEXICAL_INLINE uint64_t toHigh64(const uint32_t high, const uint32_t middle, const uint32_t low, bool & truncated) noexcept {
			// Выводим результат сборки старшего слова и объединённых младших
			return toHigh64(
				static_cast <uint64_t> (high),
				(static_cast <uint64_t> (middle) << 32) | static_cast <uint64_t> (low),
				truncated
			);
		}

		/**
		 * @brief Метод сложения двух разрядов с контролем переполнения
		 *
		 * @param x        первое слагаемое
		 * @param y        второе слагаемое
		 * @param overflow ссылка на признак переполнения разряда
		 * @return         сумма по модулю разрядности
		 */
		AWH_LEXICAL_INLINE limb_t scalarAdd(const limb_t x, const limb_t y, bool & overflow) noexcept {
			// Результат сложения разрядов
			limb_t result = 0;
			/**
			 * Для компиляторов с поддержкой встроенной проверки используем её
			 */
			#ifdef AWH_LEXICAL_ADD_OVERFLOW
				// Выполняем сложение с контролем переполнения
				overflow = __builtin_add_overflow(x, y, &result);
			/**
			 * Для остальных компиляторов используем стандартное сложение
			 */
			#else
				// Выполняем сложение разрядов
				result = static_cast <limb_t> (x + y);
				// Переполнение произошло, если сумма меньше слагаемого
				overflow = (result < x);
			#endif
			// Выводим результат сложения
			return result;
		}

		/**
		 * @brief Метод умножения двух разрядов с накоплением переноса
		 *
		 * @param x     первый множитель
		 * @param y     второй множитель
		 * @param carry ссылка на входящий и исходящий перенос
		 * @return      младшая часть произведения
		 */
		AWH_LEXICAL_INLINE limb_t scalarMul(const limb_t x, const limb_t y, limb_t & carry) noexcept {
			/**
			 * Для 64-битного разряда используем расширенное умножение
			 */
			#ifdef AWH_LEXICAL_64BIT_LIMB
				/**
				 * Для компиляторов с поддержкой встроенного расширенного умножения используем его
				 */
				#if __SIZEOF_INT128__
					// Выполняем умножение с расширением разрядности и учётом переноса
					const __uint128_t result = (
						(static_cast <__uint128_t> (x) * static_cast <__uint128_t> (y)) +
						static_cast <__uint128_t> (carry)
					);
					// Сохраняем исходящий перенос
					carry = static_cast <limb_t> (result >> LIMB_BITS);
					// Выводим младшую часть произведения
					return static_cast <limb_t> (result);
				/**
				 * Для остальных компиляторов используем 128-битное умножение разрядов
				 */
				#else
					// Выполняем 128-битное умножение разрядов
					value128_t result = multiply128(x, y);
					// Признак переполнения при добавлении переноса
					bool overflow = false;
					// Выполняем добавление входящего переноса
					result.low = scalarAdd(result.low, carry, overflow);
					// Учитываем переполнение в старшей части произведения
					result.high += static_cast <uint64_t> (overflow);
					// Сохраняем исходящий перенос
					carry = result.high;
					// Выводим младшую часть произведения
					return result.low;
				#endif
			/**
			 * Для 32-битного разряда используем расширенное умножение
			 */
			#else
				// Выполняем умножение с расширением разрядности и учётом переноса
				const uint64_t result = (
					(static_cast <uint64_t> (x) * static_cast <uint64_t> (y)) +
					static_cast <uint64_t> (carry)
				);
				// Сохраняем исходящий перенос
				carry = static_cast <limb_t> (result >> LIMB_BITS);
				// Выводим младшую часть произведения
				return static_cast <limb_t> (result);
			#endif
		}

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Метод добавления скаляра к значению длинной арифметики со смещением
		 *
		 * @param limbs целевой вектор разрядов
		 * @param value добавляемое значение
		 * @param start индекс разряда, с которого начинается добавление
		 * @return      результат выполнения операции
		 */
		inline bool smallAddFrom(limbs_t <SIZE> & limbs, const limb_t value, const size_t start) noexcept {
			// Текущий индекс обрабатываемого разряда
			size_t index = start;
			// Текущее значение переноса
			limb_t carry = value;
			// Признак переполнения разряда
			bool overflow = false;
			/**
			 * Выполняем распространение переноса по значимым разрядам
			 */
			while((carry != 0) && (index < limbs.len())){
				// Выполняем добавление переноса к очередному разряду
				limbs[index] = scalarAdd(limbs[index], carry, overflow);
				// Формируем перенос для следующего разряда
				carry = static_cast <limb_t> (overflow);
				// Переходим к следующему разряду
				++index;
			}
			// Если перенос не поглощён значимыми разрядами
			if(carry != 0)
				// Выполняем добавление переноса новым разрядом
				return limbs.push(carry);
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Метод добавления скаляра к значению длинной арифметики
		 *
		 * @param limbs целевой вектор разрядов
		 * @param value добавляемое значение
		 * @return      результат выполнения операции
		 */
		AWH_LEXICAL_INLINE bool smallAdd(limbs_t <SIZE> & limbs, const limb_t value) noexcept {
			// Выполняем добавление скаляра с нулевого разряда
			return smallAddFrom(limbs, value, 0);
		}

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Метод умножения значения длинной арифметики на скаляр
		 *
		 * @param limbs целевой вектор разрядов
		 * @param value множитель
		 * @return      результат выполнения операции
		 */
		inline bool smallMul(limbs_t <SIZE> & limbs, const limb_t value) noexcept {
			// Текущее значение переноса
			limb_t carry = 0;
			/**
			 * Выполняем умножение каждого значимого разряда
			 */
			for(size_t index = 0; index < limbs.len(); ++index)
				// Выполняем умножение очередного разряда с накоплением переноса
				limbs[index] = scalarMul(limbs[index], value, carry);
			// Если перенос не поглощён значимыми разрядами
			if(carry != 0)
				// Выполняем добавление переноса новым разрядом
				return limbs.push(carry);
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Метод добавления диапазона разрядов со смещением
		 *
		 * @param limbs  целевой вектор разрядов
		 * @param values добавляемый диапазон разрядов
		 * @param start  индекс разряда, с которого начинается добавление
		 * @return       результат выполнения операции
		 */
		inline bool largeAddFrom(limbs_t <SIZE> & limbs, const limbSpan_t values, const size_t start) noexcept {
			// Если текущей длины вектора недостаточно для добавления диапазона
			if((limbs.len() < start) || (values.len() > (limbs.len() - start))){
				// Выполняем расширение вектора нулевыми разрядами
				if(!limbs.resize(values.len() + start, 0))
					// Сообщаем, что операция не выполнена
					return false;
			}
			// Признак переноса между разрядами
			bool carry = false;
			/**
			 * Выполняем поразрядное сложение добавляемого диапазона
			 */
			for(size_t index = 0; index < values.len(); ++index){
				// Текущее значение целевого разряда
				limb_t value = limbs[index + start];
				// Признак переполнения при сложении разрядов
				bool overflowValue = false;
				// Признак переполнения при добавлении переноса
				bool overflowCarry = false;
				// Выполняем сложение разрядов
				value = scalarAdd(value, values[index], overflowValue);
				// Если имеется перенос из младшего разряда
				if(carry)
					// Выполняем добавление переноса
					value = scalarAdd(value, 1, overflowCarry);
				// Сохраняем результат сложения
				limbs[index + start] = value;
				// Формируем перенос для следующего разряда
				carry = (overflowValue || overflowCarry);
			}
			// Если перенос не поглощён обработанными разрядами
			if(carry)
				// Выполняем распространение переноса по старшим разрядам
				return smallAddFrom(limbs, 1, values.len() + start);
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Метод умножения значения длинной арифметики на диапазон разрядов
		 *
		 * @details Используется школьный алгоритм умножения: при небольшом количестве
		 *          разрядов он быстрее асимптотически более эффективных методов.
		 *
		 * @param limbs  целевой вектор разрядов
		 * @param values множитель
		 * @return       результат выполнения операции
		 */
		inline bool longMul(limbs_t <SIZE> & limbs, const limbSpan_t values) noexcept {
			// Копия исходного значения для формирования частичных произведений
			limbs_t <SIZE> source;
			// Выполняем сохранение исходного значения
			if(!source.extend(limbSpan_t(limbs.data, limbs.len())))
				// Сообщаем, что операция не выполнена
				return false;
			// Диапазон разрядов сохранённого исходного значения
			const limbSpan_t range = limbSpan_t(source.data, source.len());
			// Если множитель содержит значимые разряды
			if(values.len() != 0){
				// Выполняем умножение на младший разряд множителя
				if(!smallMul(limbs, values[0]))
					// Сообщаем, что операция не выполнена
					return false;
				/**
				 * Выполняем перебор оставшихся разрядов множителя
				 */
				for(size_t index = 1; index < values.len(); ++index){
					// Если очередной разряд множителя не является нулевым
					if(values[index] != 0){
						// Частичное произведение для текущего разряда множителя
						limbs_t <SIZE> partial;
						// Выполняем копирование исходного значения
						if(!partial.extend(range))
							// Сообщаем, что операция не выполнена
							return false;
						// Выполняем умножение на очередной разряд множителя
						if(!smallMul(partial, values[index]))
							// Сообщаем, что операция не выполнена
							return false;
						// Выполняем добавление частичного произведения со смещением
						if(!largeAddFrom(limbs, limbSpan_t(partial.data, partial.len()), index))
							// Сообщаем, что операция не выполнена
							return false;
					}
				}
			}
			// Выполняем удаление старших нулевых разрядов
			limbs.normalize();
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * @brief Шаблон ёмкости вектора разрядов
		 *
		 * @tparam SIZE максимальное количество разрядов
		 */
		template <uint16_t SIZE>
		/**
		 * @brief Метод умножения значения длинной арифметики на диапазон разрядов
		 *
		 * @param limbs  целевой вектор разрядов
		 * @param values множитель
		 * @return       результат выполнения операции
		 */
		inline bool largeMul(limbs_t <SIZE> & limbs, const limbSpan_t values) noexcept {
			// Если множитель состоит из одного разряда
			if(values.len() == 1)
				// Выполняем умножение на скаляр
				return smallMul(limbs, values[0]);
			// Выполняем умножение школьным алгоритмом
			return longMul(limbs, values);
		}

		/**
		 * @brief Шаблон фиктивного типа подстановки
		 *
		 * @tparam U фиктивный параметр для подстановки при специализации
		 */
		template <typename U = void>
		/**
		 * @brief Структура таблиц степеней пятёрки для длинной арифметики
		 *
		 */
		struct pow5_t {
			/**
			 * @brief Показатель степени, покрываемый таблицей крупного шага
			 *
			 */
			static constexpr uint32_t LARGE_STEP = 135;
			/**
			 * @brief Таблица степеней пятёрки, помещающихся в 64-битное слово
			 *
			 */
			static constexpr uint64_t SMALL[] = {
				1ULL, 5ULL, 25ULL, 125ULL, 625ULL, 3125ULL, 15625ULL,
				78125ULL, 390625ULL, 1953125ULL, 9765625ULL, 48828125ULL,
				244140625ULL, 1220703125ULL, 6103515625ULL, 30517578125ULL,
				152587890625ULL, 762939453125ULL, 3814697265625ULL,
				19073486328125ULL, 95367431640625ULL, 476837158203125ULL,
				2384185791015625ULL, 11920928955078125ULL, 59604644775390625ULL,
				298023223876953125ULL, 1490116119384765625ULL, 7450580596923828125ULL
			};
			/**
			 * Если разряд длинной арифметики является 64-битным, то таблица крупного шага
			 * содержит значения пятёрки в степени крупного шага, помещающиеся в 64-битное слово.
			 * В противном случае таблица крупного шага содержит значения пятёрки в степени крупного шага, помещающиеся в 32-битное слово.
			 */
			#ifdef AWH_LEXICAL_64BIT_LIMB
				/**
				 * @brief Таблица разрядов значения пятёрки в степени крупного шага
				 *
				 */
				static constexpr limb_t LARGE[] = {
					1414648277510068013ULL, 9180637584431281687ULL, 4539964771860779200ULL,
					10482974169319127550ULL, 198276706040285095ULL
				};
			/**
			 * Если разряд длинной арифметики является 32-битным, то таблица крупного шага
			 * содержит значения пятёрки в степени крупного шага, помещающиеся в 32-битное слово.
			 */
			#else
				/**
				 * @brief Таблица разрядов значения пятёрки в степени крупного шага
				 *
				 */
				static constexpr limb_t LARGE[] = {
					4279965485UL, 329373468UL, 4020270615UL, 2137533757UL, 4287402176UL,
					1057042919UL, 1071430142UL, 2440757623UL, 381945767UL, 46164893UL
				};
			#endif
		};

		/**
		 * @brief Структура значения длинной арифметики для точного округления
		 *
		 * @details Разряды хранятся в порядке от младшего к старшему. Все операции
		 *          возвращают признак успеха: отказ означает исчерпание ёмкости
		 *          буфера и обязан быть обработан вызывающей стороной.
		 */
		typedef struct BigInt : pow5_t <> {
			// Вектор разрядов значения длинной арифметики
			limbs_t <BIGINT_LIMBS> vec;
			/**
			 * @brief Конструктор
			 *
			 */
			BigInt() noexcept : vec() {}
			/**
			 * @brief Конструктор копирования
			 *
			 */
			BigInt(const BigInt &) = delete;
			/**
			 * @brief Конструктор перемещения
			 *
			 */
			BigInt(BigInt &&) = delete;
			/**
			 * @brief Оператор присваивания копированием
			 *
			 */
			BigInt & operator = (const BigInt &) = delete;
			/**
			 * @brief Оператор присваивания перемещением
			 *
			 */
			BigInt & operator = (BigInt &&) = delete;
			/**
			 * @brief Конструктор
			 *
			 * @param value исходное 64-битное значение
			 */
			explicit BigInt(const uint64_t value) noexcept : vec() {
				/**
				 * Для 64-битного разряда значение помещается в один разряд
				 */
				#ifdef AWH_LEXICAL_64BIT_LIMB
					// Выполняем добавление значения одним разрядом
					this->vec.push(value);
				/**
				 * Для 32-битного разряда значение помещается в два разряда
				 */
				#else
					// Выполняем добавление младшей части значения
					this->vec.push(static_cast <limb_t> (value));
					// Выполняем добавление старшей части значения
					this->vec.push(static_cast <limb_t> (value >> 32));
				#endif
				// Выполняем удаление старших нулевых разрядов
				this->vec.normalize();
			}
			/**
			 * @brief Метод получения старших 64 бит значения
			 *
			 * @param truncated ссылка на признак отброшенных значащих бит
			 * @return          старшие 64 бита значения
			 */
			uint64_t hi64(bool & truncated) const noexcept {
				// Сбрасываем признак отброшенных значащих бит
				truncated = false;
				/**
				 * Для 64-битного разряда достаточно двух старших разрядов
				 */
				#ifdef AWH_LEXICAL_64BIT_LIMB
					// Если значение является нулевым
					if(this->vec.isEmpty())
						// Выводим нулевой результат
						return 0;
					// Если значение состоит из одного разряда
					if(this->vec.len() == 1)
						// Выводим результат нормализации единственного разряда
						return toHigh64(this->vec.rindex(0), truncated);
					// Выполняем сборку старших 64 бит из двух старших разрядов
					const uint64_t result = toHigh64(this->vec.rindex(0), this->vec.rindex(1), truncated);
					// Учитываем значащие биты оставшихся младших разрядов
					truncated = (truncated || this->vec.nonzero(2));
					// Выводим старшие 64 бита значения
					return result;
				/**
				 * Для 32-битного разряда достаточно трёх старших разрядов
				 */
				#else
					// Если значение является нулевым
					if(this->vec.isEmpty())
						// Выводим нулевой результат
						return 0;
					// Если значение состоит из одного разряда
					if(this->vec.len() == 1)
						// Выводим результат нормализации единственного разряда
						return toHigh64(static_cast <uint64_t> (this->vec.rindex(0)), truncated);
					// Если значение состоит из двух разрядов
					if(this->vec.len() == 2)
						// Выводим результат сборки двух разрядов
						return toHigh64(this->vec.rindex(0), this->vec.rindex(1), truncated);
					// Выполняем сборку старших 64 бит из трёх старших разрядов
					const uint64_t result = toHigh64(this->vec.rindex(0), this->vec.rindex(1), this->vec.rindex(2), truncated);
					// Учитываем значащие биты оставшихся младших разрядов
					truncated = (truncated || this->vec.nonzero(3));
					// Выводим старшие 64 бита значения
					return result;
				#endif
			}
			/**
			 * @brief Метод сравнения с другим значением длинной арифметики
			 *
			 * @details Оба значения обязаны быть нормализованными.
			 *
			 * @param other сравниваемое значение длинной арифметики
			 * @return      результат сравнения
			 */
			int32_t compare(const BigInt & other) const noexcept {
				// Если текущее значение содержит больше разрядов
				if(this->vec.len() > other.vec.len())
					// Сообщаем, что текущее значение больше
					return 1;
				// Если текущее значение содержит меньше разрядов
				if(this->vec.len() < other.vec.len())
					// Сообщаем, что текущее значение меньше
					return -1;
				/**
				 * Выполняем поразрядное сравнение от старшего разряда к младшему
				 */
				for(size_t index = this->vec.len(); index > 0; --index){
					// Если разряд текущего значения больше
					if(this->vec[index - 1] > other.vec[index - 1])
						// Сообщаем, что текущее значение больше
						return 1;
					// Если разряд текущего значения меньше
					if(this->vec[index - 1] < other.vec[index - 1])
						// Сообщаем, что текущее значение меньше
						return -1;
				}
				// Сообщаем, что значения равны
				return 0;
			}
			/**
			 * @brief Метод получения количества ведущих нулевых бит старшего разряда
			 *
			 * @return количество ведущих нулевых бит
			 */
			int32_t ctlz() const noexcept {
				// Если значение содержит значимые разряды
				if(!this->vec.isEmpty()){
					/**
					 * Приводим старший разряд к разрядности подсчёта
					 */
					#ifdef AWH_LEXICAL_64BIT_LIMB
						// Выводим количество ведущих нулевых бит старшего разряда
						return leadingZeros(this->vec.rindex(0));
					/**
					 * Приводим старший разряд к разрядности подсчёта 64-битного слова
					 */
					#else
						// Выводим количество ведущих нулевых бит старшего разряда
						return leadingZeros(static_cast <uint64_t> (this->vec.rindex(0)) << 32);
					#endif
				}
				// Ведущих нулевых бит нет
				return 0;
			}
			/**
			 * @brief Метод получения количества значащих бит значения
			 *
			 * @return количество значащих бит
			 */
			int32_t bitLength() const noexcept {
				// Выводим количество значащих бит значения
				return (static_cast <int32_t> (LIMB_BITS * this->vec.len()) - this->ctlz());
			}
			/**
			 * @brief Метод умножения значения на скаляр
			 *
			 * @param value множитель
			 * @return      результат выполнения операции
			 */
			bool mul(const limb_t value) noexcept {
				// Выполняем умножение вектора разрядов на скаляр
				return smallMul(this->vec, value);
			}
			/**
			 * @brief Метод добавления скаляра к значению
			 *
			 * @param value слагаемое
			 * @return      результат выполнения операции
			 */
			bool add(const limb_t value) noexcept {
				// Выполняем добавление скаляра к вектору разрядов
				return smallAdd(this->vec, value);
			}
			/**
			 * @brief Метод сдвига значения влево на количество бит внутри разряда
			 *
			 * @param count величина сдвига в диапазоне от 1 до LIMB_BITS - 1
			 * @return      результат выполнения операции
			 */
			bool shlBits(const size_t count) noexcept {
				// Выполняем проверку допустимости величины сдвига
				AWH_LEXICAL_ASSERT((count != 0) && (count < LIMB_BITS));
				// Величина обратного сдвига для переноса старших бит
				const size_t shr = (LIMB_BITS - count);
				// Значение предыдущего обработанного разряда
				limb_t previous = 0;
				/**
				 * Выполняем сдвиг каждого значимого разряда с переносом
				 */
				for(size_t index = 0; index < this->vec.len(); ++index){
					// Сохраняем текущее значение разряда
					const limb_t value = this->vec[index];
					// Выполняем сдвиг разряда с добавлением переноса
					this->vec[index] = static_cast <limb_t> ((value << count) | (previous >> shr));
					// Сохраняем разряд для формирования переноса
					previous = value;
				}
				// Формируем перенос из старшего обработанного разряда
				const limb_t carry = static_cast <limb_t> (previous >> shr);
				// Если перенос содержит значащие биты
				if(carry != 0)
					// Выполняем добавление переноса новым разрядом
					return this->vec.push(carry);
				// Сообщаем, что операция выполнена успешно
				return true;
			}
			/**
			 * @brief Метод сдвига значения влево на количество разрядов
			 *
			 * @param count количество разрядов сдвига
			 * @return      результат выполнения операции
			 */
			bool shlLimbs(const size_t count) noexcept {
				// Выполняем проверку допустимости величины сдвига
				AWH_LEXICAL_ASSERT(count != 0);
				// Если ёмкости вектора недостаточно для сдвига
				if((count + this->vec.len()) > this->vec.capacity())
					// Сообщаем, что операция не выполнена
					return false;
				// Если значение содержит значимые разряды
				if(!this->vec.isEmpty()){
					// Выполняем сдвиг разрядов в сторону старших позиций
					std::copy_backward(this->vec.data, this->vec.data + this->vec.len(), this->vec.data + this->vec.len() + count);
					// Выполняем обнуление освободившихся младших разрядов
					std::fill(this->vec.data, this->vec.data + count, limb_t(0));
					// Выполняем установку нового количества значимых разрядов
					this->vec.setLen(this->vec.len() + count);
				}
				// Сообщаем, что операция выполнена успешно
				return true;
			}
			/**
			 * @brief Метод сдвига значения влево на количество бит
			 *
			 * @param count величина сдвига в битах
			 * @return      результат выполнения операции
			 */
			bool shl(const size_t count) noexcept {
				// Величина сдвига внутри разряда
				const size_t bits = (count % LIMB_BITS);
				// Количество разрядов сдвига
				const size_t limbs = (count / LIMB_BITS);
				// Если требуется сдвиг внутри разряда
				if(bits != 0){
					// Выполняем сдвиг внутри разряда
					if(!this->shlBits(bits))
						// Сообщаем, что операция не выполнена
						return false;
				}
				// Если требуется сдвиг на целые разряды
				if(limbs != 0)
					// Выполняем сдвиг на целые разряды
					return this->shlLimbs(limbs);
				// Сообщаем, что операция выполнена успешно
				return true;
			}
			/**
			 * @brief Метод умножения значения на степень двойки
			 *
			 * @param exponent показатель степени двойки
			 * @return         результат выполнения операции
			 */
			bool pow2(const uint32_t exponent) noexcept {
				// Выполняем сдвиг значения влево на показатель степени
				return this->shl(exponent);
			}
			/**
			 * @brief Метод умножения значения на степень пятёрки
			 *
			 * @param exponent показатель степени пятёрки
			 * @return         результат выполнения операции
			 */
			bool pow5(uint32_t exponent) noexcept {
				// Диапазон разрядов значения пятёрки в степени крупного шага
				const limbSpan_t large = limbSpan_t(LARGE, sizeof(LARGE) / sizeof(limb_t));
				/**
				 * Выполняем умножение крупными шагами по таблице разрядов
				 */
				while(exponent >= LARGE_STEP){
					// Выполняем умножение на значение крупного шага
					if(!largeMul(this->vec, large))
						// Сообщаем, что операция не выполнена
						return false;
					// Уменьшаем оставшийся показатель степени
					exponent -= LARGE_STEP;
				}
				/**
				 * Определяем параметры среднего шага по разрядности
				 */
				#ifdef AWH_LEXICAL_64BIT_LIMB
					// Показатель степени, помещающийся в один разряд
					constexpr uint32_t SMALL_STEP = 27;
					// Наибольшая степень пятёрки, помещающаяся в один разряд
					constexpr limb_t MAX_NATIVE = 7450580596923828125ULL;
				/**
				 * Определяем параметры среднего шага для 32-битного разряда
				 */
				#else
					// Показатель степени, помещающийся в один разряд
					constexpr uint32_t SMALL_STEP = 13;
					// Наибольшая степень пятёрки, помещающаяся в один разряд
					constexpr limb_t MAX_NATIVE = 1220703125UL;
				#endif
				/**
				 * Выполняем умножение средними шагами нативным скаляром
				 */
				while(exponent >= SMALL_STEP){
					// Выполняем умножение на наибольшую нативную степень
					if(!smallMul(this->vec, MAX_NATIVE))
						// Сообщаем, что операция не выполнена
						return false;
					// Уменьшаем оставшийся показатель степени
					exponent -= SMALL_STEP;
				}
				// Если остался необработанный показатель степени
				if(exponent != 0)
					// Выполняем умножение на оставшуюся степень пятёрки
					return smallMul(this->vec, static_cast <limb_t> (((void) SMALL[0], SMALL[exponent])));
				// Сообщаем, что операция выполнена успешно
				return true;
			}
			/**
			 * @brief Метод умножения значения на степень десятки
			 *
			 * @param exponent показатель степени десятки
			 * @return         результат выполнения операции
			 */
			bool pow10(const uint32_t exponent) noexcept {
				// Выполняем умножение на степень пятёрки
				if(!this->pow5(exponent))
					// Сообщаем, что операция не выполнена
					return false;
				// Выполняем умножение на степень двойки
				return this->pow2(exponent);
			}
		} bigint_t;
	};
};

#endif // __AWH_LEXICAL_BIGINT__
