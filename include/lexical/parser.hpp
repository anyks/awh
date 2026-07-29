/**
 * @file: parser.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл лексического сканера чисел — посимвольный и векторизованный (SSE2, NEON) разбор знака,
 *        целой и дробной частей,
 *        экспоненты и специальных значений с формированием промежуточной структуры разобранного числа
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_PARSER__
#define __AWH_LEXICAL_PARSER__

/**
 * Стандартные заголовочные файлы
 */
#include <limits>
#include <cstdint>
#include <cstring>
#include <type_traits>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Если векторные инструкции доступны
 */
#ifdef AWH_LEXICAL_SSE2
	// Подключаем заголовочные файлы векторных инструкций
	#include <emmintrin.h>
#endif
/**
 * Если векторные инструкции NEON доступны, то подключаем заголовочные файлы
 */
#ifdef AWH_LEXICAL_NEON
	// Подключаем заголовочные файлы NEON
	#include <arm_neon.h>
#endif

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
		 * @brief Количество цифр, разбираемых за одну итерацию блочного разбора
		 *
		 */
		constexpr size_t DIGITS_PER_BLOCK = 8;

		/**
		 * @brief Наименьшее девятнадцатизначное десятичное число
		 *
		 */
		constexpr uint64_t MIN_NINETEEN_DIGITS = 1000000000000000000ULL;

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC>
		/**
		 * @brief Функция проверки доступности векторного разбора для типа символа
		 *
		 * @details Векторный путь применяется только для двухбайтовых символов:
		 *          для однобайтовых скалярный разбор блока быстрее векторного.
		 *
		 * @return результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool hasSimd() noexcept {
			/**
			 * Определяем доступность векторного разбора
			 */
			#ifdef AWH_LEXICAL_SIMD
				// Векторный разбор доступен для двухбайтовых символов
				return is_same <UC, char16_t>::value;
			/**
			 * Если векторные инструкции недоступны, то векторный разбор невозможен
			 */
			#else
				// Векторные инструкции недоступны
				return false;
			#endif
		}

		/**
		 * @brief Функция изменения порядка байт 64-битного значения
		 *
		 * @param value исходное значение
		 * @return      значение с обратным порядком байт
		 *
		 */
		AWH_ASCII_INLINE constexpr uint64_t swapBytes(const uint64_t value) noexcept {
			// Выполняем перестановку байт значения
			return (
				((value & 0xFF00000000000000ULL) >> 56) |
				((value & 0x00FF000000000000ULL) >> 40) |
				((value & 0x0000FF0000000000ULL) >> 24) |
				((value & 0x000000FF00000000ULL) >> 8)  |
				((value & 0x00000000FF000000ULL) << 8)  |
				((value & 0x0000000000FF0000ULL) << 24) |
				((value & 0x000000000000FF00ULL) << 40) |
				((value & 0x00000000000000FFULL) << 56)
			);
		}

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC>
		/**
		 * @brief Функция упаковки блока символов в 64-битное значение
		 *
		 * @details Символы шире одного байта усекаются до младшего байта.
		 *          Результат всегда формируется в порядке от младшего к старшему.
		 *
		 * @param chars указатель на начало блока символов
		 * @return      упакованное 64-битное значение
		 *
		 */
		AWH_ASCII_INLINE uint64_t readBlock(const UC * chars) noexcept {
			// Если символы исходной строки шире одного байта
			if(!is_same <UC, char>::value){
				// Аккумулятор упакованного значения
				uint64_t result = 0;
				/**
				 * Выполняем упаковку младших байт символов блока
				 */
				for(size_t i = 0; i < DIGITS_PER_BLOCK; ++i)
					// Записываем младший байт очередного символа
					result |= (static_cast <uint64_t> (static_cast <uint8_t> (chars[i])) << (i * 8));
				// Выводим упакованное значение
				return result;
			}
			// Аккумулятор упакованного значения
			uint64_t result = 0;
			// Выполняем чтение блока символов одной операцией
			::memcpy(&result, chars, sizeof(uint64_t));
			/**
			 * На платформах с обратным порядком байт приводим значение к прямому
			 */
			#if AWH_LEXICAL_BIG_ENDIAN
				// Выполняем перестановку байт значения
				result = swapBytes(result);
			#endif
			// Выводим упакованное значение
			return result;
		}

		/**
		 * Если векторные инструкции доступны, то определяем функции упаковки блока символов
		 */
		#ifdef AWH_LEXICAL_SSE2
			/**
			 * @brief Функция упаковки блока символов из векторного регистра
			 *
			 * @param data загруженный векторный регистр
			 * @return     упакованное 64-битное значение
			 *
			 */
			AWH_ASCII_INLINE uint64_t readBlockSimd(const __m128i data) noexcept {
				// Отключаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_DISABLE_WARNINGS
				// Выполняем упаковку 16-битных элементов в 8-битные
				const __m128i packed = _mm_packus_epi16(data, data);
				/**
				 * Извлекаем результат с учётом разрядности платформы
				 */
				#ifdef AWH_LEXICAL_64BIT
					// Выводим младшие 64 бита векторного регистра
					return static_cast <uint64_t> (_mm_cvtsi128_si64(packed));
				/**
				 * Если платформа 32-битная, то извлекаем младшие 32 бита и старшие 32 бита
				 */
				#else
					// Упакованное 64-битное значение
					uint64_t result = 0;
					// Выполняем выгрузку младших 64 бит векторного регистра
					_mm_storel_epi64(reinterpret_cast <__m128i *> (&result), packed);
					// Выводим упакованное значение
					return result;
				#endif
				// Восстанавливаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_RESTORE_WARNINGS
			}
			/**
			 * @brief Функция упаковки блока двухбайтовых символов векторными инструкциями
			 *
			 * @param chars указатель на начало блока символов
			 * @return      упакованное 64-битное значение
			 *
			 */
			AWH_ASCII_INLINE uint64_t readBlockSimd(const char16_t * chars) noexcept {
				// Отключаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_DISABLE_WARNINGS
				// Выполняем загрузку блока символов и его упаковку
				return readBlockSimd(_mm_loadu_si128(reinterpret_cast <const __m128i *> (chars)));
				// Восстанавливаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_RESTORE_WARNINGS
			}
		/**
		 * Если векторные инструкции NEON доступны, то определяем функции упаковки блока символов
		 */
		#elif AWH_LEXICAL_NEON
			/**
			 * @brief Функция упаковки блока символов из векторного регистра
			 *
			 * @param data загруженный векторный регистр
			 * @return     упакованное 64-битное значение
			 *
			 */
			AWH_ASCII_INLINE uint64_t readBlockSimd(const uint16x8_t data) noexcept {
				// Отключаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_DISABLE_WARNINGS
				// Выполняем сужение 16-битных элементов до 8-битных
				const uint8x8_t packed = vmovn_u16(data);
				// Выводим упакованное значение
				return vget_lane_u64(vreinterpret_u64_u8(packed), 0);
				// Восстанавливаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_RESTORE_WARNINGS
			}
			/**
			 * @brief Функция упаковки блока двухбайтовых символов векторными инструкциями
			 *
			 * @param chars указатель на начало блока символов
			 * @return      упакованное 64-битное значение
			 *
			 */
			AWH_ASCII_INLINE uint64_t readBlockSimd(const char16_t * chars) noexcept {
				// Отключаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_DISABLE_WARNINGS
				// Выполняем загрузку блока символов и его упаковку
				return readBlockSimd(vld1q_u16(reinterpret_cast <const uint16_t *> (chars)));
				// Восстанавливаем предупреждения о выравнивании указателей
				AWH_LEXICAL_SIMD_RESTORE_WARNINGS
			}
		#endif

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC, enableIf_t <!hasSimd <UC> ()> = 0>
		/**
		 * @brief Функция упаковки блока символов для типов без векторной поддержки
		 *
		 * @return всегда нулевое значение
		 *
		 */
		inline uint64_t readBlockSimd(const UC *) noexcept {
			// Векторный разбор для данного типа символа недоступен
			return 0;
		}

		/**
		 * @brief Функция разбора упакованного блока десятичных цифр
		 *
		 * @param value упакованные ASCII-коды восьми десятичных цифр
		 * @return      числовое значение блока цифр
		 *
		 */
		AWH_ASCII_INLINE constexpr uint32_t parseBlock(uint64_t value) noexcept {
			// Маска выделения байтовых пар результата
			constexpr uint64_t MASK = 0x000000FF000000FFULL;
			// Первый множитель свёртки разрядов
			constexpr uint64_t MUL1 = 0x000F424000000064ULL;
			// Второй множитель свёртки разрядов
			constexpr uint64_t MUL2 = 0x0000271000000001ULL;
			// Выполняем перевод ASCII-кодов в числовые значения цифр
			value -= 0x3030303030303030ULL;
			// Выполняем свёртку соседних разрядов
			value = ((value * 10) + (value >> 8));
			// Выполняем финальную свёртку в 32-битное значение
			value = ((((value & MASK) * MUL1) + (((value >> 16) & MASK) * MUL2)) >> 32);
			// Выводим числовое значение блока цифр
			return static_cast <uint32_t> (value);
		}

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC>
		/**
		 * @brief Функция разбора блока из восьми десятичных цифр
		 *
		 * @param chars указатель на начало блока цифр
		 * @return      числовое значение блока цифр
		 *
		 */
		AWH_ASCII_INLINE uint32_t parseBlock(const UC * chars) noexcept {
			// Если векторный разбор для типа символа недоступен
			if(!hasSimd <UC> ())
				// Выполняем скалярный разбор упакованного блока
				return parseBlock(readBlock(chars));
			// Выполняем векторный разбор упакованного блока
			return parseBlock(readBlockSimd(chars));
		}

		/**
		 * @brief Функция проверки упакованного блока на состав из десятичных цифр
		 *
		 * @param value упакованный блок символов
		 * @return      результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isDigitBlock(const uint64_t value) noexcept {
			// Выполняем одновременную проверку всех байт блока на диапазон цифр
			return !(((value + 0x4646464646464646ULL) | (value - 0x3030303030303030ULL)) & 0x8080808080808080ULL);
		}

		/**
		 * Если векторные инструкции доступны, то определяем функции проверки блока на состав из десятичных цифр
		 */
		#ifdef AWH_LEXICAL_SIMD
			/**
			 * @brief Функция векторного разбора блока двухбайтовых десятичных цифр
			 *
			 * @param chars указатель на начало блока символов
			 * @param value ссылка на аккумулятор числового значения
			 * @return      результат разбора блока
			 *
			 */
			AWH_ASCII_INLINE bool parseBlockSimd(const char16_t * chars, uint64_t & value) noexcept {
				/**
				 * Выполняем разбор блока доступным набором векторных инструкций
				 */
				#ifdef AWH_LEXICAL_SSE2
					// Отключаем предупреждения о выравнивании указателей
					AWH_LEXICAL_SIMD_DISABLE_WARNINGS
					// Выполняем загрузку блока символов
					const __m128i data = _mm_loadu_si128(reinterpret_cast <const __m128i *> (chars));
					// Выполняем смещение диапазона цифр к границе знакового переполнения
					const __m128i shifted = _mm_add_epi16(data, _mm_set1_epi16(32720));
					// Выполняем проверку выхода символов за диапазон десятичных цифр
					const __m128i invalid = _mm_cmpgt_epi16(shifted, _mm_set1_epi16(-32759));
					// Результат разбора блока
					bool result = false;
					// Если все символы блока являются десятичными цифрами
					if(_mm_movemask_epi8(invalid) == 0){
						// Выполняем накопление числового значения блока
						value = ((value * 100000000ULL) + parseBlock(readBlockSimd(data)));
						// Запоминаем, что блок успешно разобран
						result = true;
					}
					// Восстанавливаем предупреждения о выравнивании указателей
					AWH_LEXICAL_SIMD_RESTORE_WARNINGS
					// Выводим результат разбора блока
					return result;
				/**
				 * Если разбор блока не выполняется средствами SSE2, то используем NEON
				 */
				#else
					// Отключаем предупреждения о выравнивании указателей
					AWH_LEXICAL_SIMD_DISABLE_WARNINGS
					// Выполняем загрузку блока символов
					const uint16x8_t data = vld1q_u16(reinterpret_cast <const uint16_t *> (chars));
					// Выполняем смещение диапазона цифр к нулю
					const uint16x8_t shifted = vsubq_u16(data, vmovq_n_u16(u'0'));
					// Выполняем проверку попадания символов в диапазон десятичных цифр
					const uint16x8_t valid = vcltq_u16(shifted, vmovq_n_u16(static_cast <uint16_t> (u'9' - u'0' + 1)));
					// Результат разбора блока
					bool result = false;
					// Если все символы блока являются десятичными цифрами
					if(vminvq_u16(valid) == 0xFFFF){
						// Выполняем накопление числового значения блока
						value = ((value * 100000000ULL) + parseBlock(readBlockSimd(data)));
						// Запоминаем, что блок успешно разобран
						result = true;
					}
					// Восстанавливаем предупреждения о выравнивании указателей
					AWH_LEXICAL_SIMD_RESTORE_WARNINGS
					// Выводим результат разбора блока
					return result;
				#endif
			}
		#endif

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC, enableIf_t <!hasSimd <UC> ()> = 0>
		/**
		 * @brief Функция векторного разбора блока для типов без векторной поддержки
		 *
		 * @return всегда отрицательный результат
		 *
		 */
		inline bool parseBlockSimd(const UC *, uint64_t &) noexcept {
			// Векторный разбор для данного типа символа недоступен
			return false;
		}

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC, enableIf_t <!is_same <UC, char>::value> = 0>
		/**
		 * @brief Функция блочного разбора последовательности десятичных цифр
		 *
		 * @details Аккумулятор может переполниться: контроль переполнения выполняется
		 *          вызывающей стороной по количеству разобранных цифр.
		 *
		 * @param p     ссылка на текущую позицию в строке
		 * @param pend  конец строки
		 * @param value ссылка на аккумулятор числового значения
		 *
		 */
		AWH_ASCII_INLINE void parseBlocks(const UC * & p, const UC * const pend, uint64_t & value) noexcept {
			// Если векторный разбор для типа символа недоступен
			if(!hasSimd <UC> ())
				// Завершаем блочный разбор
				return;
			/**
			 * Выполняем разбор блоков цифр, пока это возможно
			 */
			while((static_cast <size_t> (pend - p) >= DIGITS_PER_BLOCK) && parseBlockSimd(p, value))
				// Выполняем смещение позиции на размер разобранного блока
				p += DIGITS_PER_BLOCK;
		}

		/**
		 * @brief Функция блочного разбора последовательности десятичных цифр
		 *
		 * @details Аккумулятор может переполниться: контроль переполнения выполняется
		 *          вызывающей стороной по количеству разобранных цифр.
		 *
		 * @param p     ссылка на текущую позицию в строке
		 * @param pend  конец строки
		 * @param value ссылка на аккумулятор числового значения
		 *
		 */
		AWH_ASCII_INLINE void parseBlocks(const char * & p, const char * const pend, uint64_t & value) noexcept {
			/**
			 * Выполняем разбор блоков цифр, пока это возможно
			 */
			while((static_cast <size_t> (pend - p) >= DIGITS_PER_BLOCK) && isDigitBlock(readBlock(p))){
				// Выполняем накопление числового значения блока
				value = ((value * 100000000ULL) + parseBlock(readBlock(p)));
				// Выполняем смещение позиции на размер разобранного блока
				p += DIGITS_PER_BLOCK;
			}
		}

		/**
		 * @brief Функция умножения с добавлением и контролем переполнения
		 *
		 * @param value ссылка на накапливаемое значение
		 * @param base  основание системы счисления
		 * @param digit добавляемая цифра
		 * @return      результат выполнения операции
		 *
		 */
		AWH_ASCII_INLINE bool mulAddChecked(uint64_t & value, const uint64_t base, const uint64_t digit) noexcept {
			/**
			 * Для компиляторов с поддержкой встроенной проверки используем её
			 */
			#if defined(AWH_LEXICAL_MUL_OVERFLOW) && defined(AWH_LEXICAL_ADD_OVERFLOW)
				// Если умножение накопленного значения переполняет разрядность
				if(__builtin_mul_overflow(value, base, &value))
					// Сообщаем о переполнении разрядности
					return false;
				// Если добавление цифры переполняет разрядность
				if(__builtin_add_overflow(value, digit, &value))
					// Сообщаем о переполнении разрядности
					return false;
			/**
			 * Для компиляторов без поддержки встроенной проверки выполняем контроль переполнения вручную
			 */
			#else
				// Если результат операции не помещается в разрядность
				if(value > ((numeric_limits <uint64_t>::max() - digit) / base))
					// Сообщаем о переполнении разрядности
					return false;
				// Выполняем накопление значения
				value = ((value * base) + digit);
			#endif
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC>
		/**
		 * @brief Структура результата разбора числовой строки
		 *
		 */
		struct parsedNumber_t {
			// Признак успешного разбора
			bool valid;
			// Признак отрицательного числа
			bool negative;
			// Признак превышения разрядности мантиссы
			bool tooManyDigits;
			// Код причины отказа при разборе
			error_t error;
			// Десятичный показатель степени
			int64_t exponent;
			// Значение мантиссы
			uint64_t mantissa;
			// Диапазон символов целой части
			span_t <UC> integer;
			// Диапазон символов дробной части
			span_t <UC> fraction;
			// Указатель на первый символ за разобранным числом
			const UC * lastmatch;
			/**
			 * @brief Конструктор
			 *
			 */
			parsedNumber_t() noexcept :
			 valid(false),
			 negative(false),
			 tooManyDigits(false),
			 error(error_t::NONE),
			 exponent(0), mantissa(0),
			 integer(), fraction(),
			 lastmatch(nullptr) {}
		};

		/**
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename UC>
		/**
		 * @brief Функция формирования результата разбора с ошибкой
		 *
		 * @param p     позиция обнаружения ошибки
		 * @param error код причины отказа при разборе
		 * @return      результат разбора числовой строки
		 *
		 */
		AWH_ASCII_INLINE parsedNumber_t <UC> reportError(const UC * p, const error_t error) noexcept {
			// Результат разбора числовой строки
			parsedNumber_t <UC> result;
			// Запоминаем, что разбор завершился отказом
			result.valid = false;
			// Запоминаем код причины отказа
			result.error = error;
			// Запоминаем позицию обнаружения ошибки
			result.lastmatch = p;
			// Выводим результат разбора числовой строки
			return result;
		}

		/**
		 * @brief Шаблон строгого режима разбора и типа символа исходной строки
		 *
		 * @tparam JSON включение строгих правил формата RFC 8259
		 * @tparam UC   тип символа исходной строки
		 *
		 */
		template <bool JSON, typename UC>
		/**
		 * @brief Функция разбора числовой строки числа с плавающей точкой
		 *
		 * @details Диапазон символов обязан быть непустым: проверка выполняется вызывающей стороной.
		 *
		 * @param p       начало разбираемой строки
		 * @param pend    конец разбираемой строки
		 * @param options опции разбора числовой строки
		 * @return        результат разбора числовой строки
		 *
		 */
		inline parsedNumber_t <UC> parseNumberString(const UC * p, const UC * const pend, const options_t <UC> options) noexcept {
			// Символ десятичной точки
			const UC decimalPoint = options.decimalPoint;
			// Результат разбора числовой строки
			parsedNumber_t <UC> result;
			// Определяем знак разбираемого числа
			result.negative = (* p == UC('-'));
			// Если строка начинается со знака числа
			if(result.negative || (!JSON && isFormat(options.format, format_t::ALLOW_LEADING_PLUS) && (* p == UC('+')))){
				// Выполняем пропуск символа знака
				++p;
				// Если после знака строка закончилась
				if(p == pend)
					// Сообщаем об отсутствии цифр после знака
					return reportError <UC> (p, error_t::MISSING_INTEGER_OR_DOT_AFTER_SIGN);
				/**
				 * Для строгого формата после знака обязательна цифра
				 */
				if(JSON){
					// Если после знака расположена не цифра
					if(!isDigit(* p))
						// Сообщаем об отсутствии цифры после знака
						return reportError <UC> (p, error_t::MISSING_INTEGER_AFTER_SIGN);
				// Для общего формата после знака допустима цифра или десятичная точка
				} else if(!isDigit(* p) && (* p != decimalPoint))
					// Сообщаем об отсутствии цифры или десятичной точки после знака
					return reportError <UC> (p, error_t::MISSING_INTEGER_OR_DOT_AFTER_SIGN);
			}
			// Запоминаем начало цифр целой части
			const UC * const startDigits = p;
			// Аккумулятор значения мантиссы
			uint64_t mantissa = 0;
			// Выполняем блочный разбор цифр целой части
			parseBlocks(p, pend, mantissa);
			/**
			 * Выполняем поразрядный разбор оставшихся цифр целой части
			 */
			while((p != pend) && isDigit(* p)){
				// Выполняем накопление очередной цифры
				mantissa = ((mantissa * 10ULL) + static_cast <uint64_t> (* p - UC('0')));
				// Переходим к следующему символу
				++p;
			}
			// Запоминаем конец цифр целой части
			const UC * const endOfIntegerPart = p;
			// Количество разобранных значащих цифр
			int64_t digitCount = static_cast <int64_t> (endOfIntegerPart - startDigits);
			// Сохраняем диапазон символов целой части
			result.integer = span_t <UC> (startDigits, static_cast <size_t> (digitCount));
			/**
			 * Для строгого формата выполняем дополнительные проверки целой части
			 */
			if(JSON){
				// Если целая часть не содержит цифр
				if(digitCount == 0)
					// Сообщаем об отсутствии цифр в целой части
					return reportError <UC> (p, error_t::NO_DIGITS_IN_INTEGER_PART);
				// Если целая часть содержит ведущие нули
				if((startDigits[0] == UC('0')) && (digitCount > 1))
					// Сообщаем о наличии ведущих нулей в целой части
					return reportError <UC> (startDigits, error_t::LEADING_ZEROS_IN_INTEGER_PART);
			}
			// Десятичный показатель степени разбираемого числа
			int64_t exponent = 0;
			// Определяем наличие десятичной точки
			const bool hasDecimalPoint = ((p != pend) && (* p == decimalPoint));
			// Если десятичная точка обнаружена
			if(hasDecimalPoint){
				// Выполняем пропуск символа десятичной точки
				++p;
				// Запоминаем начало цифр дробной части
				const UC * const startFraction = p;
				// Выполняем блочный разбор цифр дробной части
				parseBlocks(p, pend, mantissa);
				/**
				 * Выполняем поразрядный разбор оставшихся цифр дробной части
				 */
				while((p != pend) && isDigit(* p)){
					// Выполняем накопление очередной цифры
					mantissa = ((mantissa * 10ULL) + static_cast <uint64_t> (* p - UC('0')));
					// Переходим к следующему символу
					++p;
				}
				// Формируем отрицательный вклад дробной части в показатель степени
				exponent = static_cast <int64_t> (startFraction - p);
				// Сохраняем диапазон символов дробной части
				result.fraction = span_t <UC> (startFraction, static_cast <size_t> (p - startFraction));
				// Учитываем цифры дробной части в общем количестве
				digitCount -= exponent;
			}
			/**
			 * Выполняем проверку наличия цифр в мантиссе
			 */
			if(JSON){
				// Если дробная часть объявлена, но не содержит цифр
				if(hasDecimalPoint && (exponent == 0))
					// Сообщаем об отсутствии цифр в дробной части
					return reportError <UC> (p, error_t::NO_DIGITS_IN_FRACTIONAL_PART);
			// Если мантисса не содержит ни одной цифры
			} else if(digitCount == 0)
				// Сообщаем об отсутствии цифр в мантиссе
				return reportError <UC> (p, error_t::NO_DIGITS_IN_MANTISSA);
			// Значение явно указанной экспоненциальной части
			int64_t explicitExponent = 0;
			// Определяем наличие маркера экспоненциальной части
			const bool hasExponent = (p != pend) && (
				(isFormat(options.format, format_t::SCIENTIFIC) && ((* p == UC('e')) || (* p == UC('E')))) ||
				(isFormat(options.format, format_t::BASIC_FORTRAN) &&
				 ((* p == UC('+')) || (* p == UC('-')) || (* p == UC('d')) || (* p == UC('D'))))
			);
			// Если маркер экспоненциальной части обнаружен
			if(hasExponent){
				// Запоминаем позицию маркера на случай отката разбора
				const UC * const locationOfMarker = p;
				// Если текущий символ является маркером экспоненты
				if((* p == UC('e')) || (* p == UC('E')) || (* p == UC('d')) || (* p == UC('D')))
					// Выполняем пропуск маркера экспоненты
					++p;
				// Признак отрицательной экспоненциальной части
				bool negativeExponent = false;
				// Если экспоненциальная часть начинается со знака минус
				if((p != pend) && (* p == UC('-'))){
					// Запоминаем отрицательный знак экспоненциальной части
					negativeExponent = true;
					// Выполняем пропуск символа знака
					++p;
				// Если экспоненциальная часть начинается со знака плюс
				} else if((p != pend) && (* p == UC('+')))
					// Выполняем пропуск символа знака
					++p;
				// Если экспоненциальная часть не содержит цифр
				if((p == pend) || !isDigit(* p)){
					// Если запись с фиксированной точкой форматом не разрешена
					if(!isFormat(options.format, format_t::FIXED))
						// Сообщаем об отсутствии экспоненциальной части
						return reportError <UC> (p, error_t::MISSING_EXPONENTIAL_PART);
					// Выполняем откат разбора к позиции маркера
					p = locationOfMarker;
				// Если экспоненциальная часть содержит цифры
				} else {
					/**
					 * Выполняем разбор цифр экспоненциальной части
					 */
					while((p != pend) && isDigit(* p)){
						// Если накопленное значение ещё не достигло предела насыщения
						if(explicitExponent < 0x10000000)
							// Выполняем накопление очередной цифры
							explicitExponent = ((explicitExponent * 10) + static_cast <int64_t> (* p - UC('0')));
						// Переходим к следующему символу
						++p;
					}
					// Если экспоненциальная часть является отрицательной
					if(negativeExponent)
						// Изменяем знак накопленного значения
						explicitExponent = -explicitExponent;
					// Учитываем экспоненциальную часть в показателе степени
					exponent += explicitExponent;
				}
			// Если формат допускает только научную запись
			} else if(isFormat(options.format, format_t::SCIENTIFIC) && !isFormat(options.format, format_t::FIXED))
				// Сообщаем об отсутствии экспоненциальной части
				return reportError <UC> (p, error_t::MISSING_EXPONENTIAL_PART);
			// Запоминаем успешное завершение разбора
			result.valid = true;
			// Запоминаем позицию окончания разбора
			result.lastmatch = p;
			// Если количество разобранных цифр превышает разрядность мантиссы
			if(digitCount > 19){
				// Позиция перебора значащих цифр
				const UC * position = startDigits;
				/**
				 * Выполняем пропуск ведущих нулей и десятичной точки
				 */
				while((position != pend) && ((* position == UC('0')) || (* position == decimalPoint))){
					// Если пропускается ведущий нуль
					if(* position == UC('0'))
						// Уменьшаем количество значащих цифр
						--digitCount;
					// Переходим к следующему символу
					++position;
				}
				// Если количество значащих цифр всё ещё превышает разрядность мантиссы
				if(digitCount > 19){
					// Запоминаем факт усечения мантиссы
					result.tooManyDigits = true;
					// Сбрасываем накопленное значение мантиссы
					mantissa = 0;
					// Переходим к началу целой части
					position = result.integer.ptr;
					// Конец диапазона символов целой части
					const UC * const endInteger = (position + result.integer.len());
					/**
					 * Выполняем набор значащих цифр из целой части
					 */
					while((mantissa < MIN_NINETEEN_DIGITS) && (position != endInteger)){
						// Выполняем накопление очередной цифры
						mantissa = ((mantissa * 10ULL) + static_cast <uint64_t> (* position - UC('0')));
						// Переходим к следующему символу
						++position;
					}
					// Если значащих цифр целой части оказалось достаточно
					if(mantissa >= MIN_NINETEEN_DIGITS){
						// Формируем показатель степени по остатку целой части
						exponent = (static_cast <int64_t> (endOfIntegerPart - position) + explicitExponent);
					// Если требуется добор значащих цифр из дробной части
					} else {
						// Переходим к началу дробной части
						position = result.fraction.ptr;
						// Конец диапазона символов дробной части
						const UC * const endFraction = (position + result.fraction.len());
						/**
						 * Выполняем набор значащих цифр из дробной части
						 */
						while((mantissa < MIN_NINETEEN_DIGITS) && (position != endFraction)){
							// Выполняем накопление очередной цифры
							mantissa = ((mantissa * 10ULL) + static_cast <uint64_t> (* position - UC('0')));
							// Переходим к следующему символу
							++position;
						}
						// Формируем показатель степени по набранной дробной части
						exponent = (static_cast <int64_t> (result.fraction.ptr - position) + explicitExponent);
					}
				}
			}
			// Сохраняем итоговый показатель степени
			result.exponent = exponent;
			// Сохраняем итоговое значение мантиссы
			result.mantissa = mantissa;
			// Выводим результат разбора числовой строки
			return result;
		}

		/**
		 * @brief Шаблон типа целого результата и типа символа исходной строки
		 *
		 * @tparam T  тип целого результата разбора
		 * @tparam UC тип символа исходной строки
		 *
		 */
		template <typename T, typename UC>
		/**
		 * @brief Функция разбора числовой строки целого числа
		 *
		 * @details Диапазон символов обязан быть непустым,
		 *          а основание системы счисления проверенным: проверки выполняются вызывающей стороной.
		 *
		 * @param p       начало разбираемой строки
		 * @param pend    конец разбираемой строки
		 * @param value   ссылка на результат разбора
		 * @param options опции разбора числовой строки
		 * @return        результат разбора числовой строки
		 *
		 */
		inline result_t <UC> parseIntString(const UC * p, const UC * const pend, T & value, const options_t <UC> options) noexcept {
			// Основание системы счисления
			const int32_t base = options.base;
			// Запоминаем начало разбираемой строки
			const UC * const first = p;
			// Определяем знак разбираемого числа
			const bool negative = (* p == UC('-'));
			// Если для беззнакового типа результата получено отрицательное число
			if(!is_signed <T>::value && negative)
				// Сообщаем о недопустимости отрицательного числа
				return result_t <UC> (first, errc::invalid_argument, error_t::MISSING_INTEGER_AFTER_SIGN);
			// Если строка начинается со знака числа
			if(negative || (isFormat(options.format, format_t::ALLOW_LEADING_PLUS) && (* p == UC('+'))))
				// Выполняем пропуск символа знака
				++p;
			// Запоминаем начало числовой части
			const UC * const startNumber = p;
			/**
			 * Выполняем пропуск ведущих нулей
			 */
			while((p != pend) && (* p == UC('0')))
				// Переходим к следующему символу
				++p;
			// Определяем наличие ведущих нулей
			const bool hasLeadingZeros = (p > startNumber);
			// Запоминаем начало значащих цифр
			const UC * const startDigits = p;
			// Аккумулятор разобранного значения
			uint64_t result = 0;
			// Если разбор выполняется в десятичной системе счисления
			if(base == 10)
				// Выполняем блочный разбор значащих цифр
				parseBlocks(p, pend, result);
			/**
			 * Выполняем поразрядный разбор оставшихся значащих цифр
			 */
			while(p != pend){
				// Получаем значение очередной цифры
				const uint8_t digit = charToDigit(* p);
				// Если символ не является цифрой заданного основания
				if(digit >= base)
					// Завершаем разбор значащих цифр
					break;
				// Выполняем накопление очередной цифры
				result = ((result * static_cast <uint64_t> (base)) + digit);
				// Переходим к следующему символу
				++p;
			}
			// Количество разобранных значащих цифр
			const size_t digitCount = static_cast <size_t> (p - startDigits);
			// Если значащих цифр не обнаружено
			if(digitCount == 0){
				// Если строка состояла из ведущих нулей
				if(hasLeadingZeros){
					// Устанавливаем нулевой результат разбора
					value = static_cast <T> (0);
					// Выводим успешный результат разбора
					return result_t <UC> (p);
				}
				// Сообщаем о невозможности разобрать число
				return result_t <UC> (first, errc::invalid_argument, error_t::NO_DIGITS_IN_MANTISSA);
			}
			// Максимальное количество цифр, помещающихся в разрядность аккумулятора
			const size_t maxDigits = maxDigitsU64(base);
			// Если количество значащих цифр заведомо превышает разрядность аккумулятора
			if(digitCount > maxDigits)
				// Сообщаем о выходе значения за пределы диапазона
				return result_t <UC> (p, errc::result_out_of_range, error_t::OVERFLOW_RANGE);
			/**
			 * При предельном количестве цифр накопленное значение могло переполниться,
			 * поэтому выполняем повторный разбор с точным контролем переполнения
			 */
			if(digitCount == maxDigits){
				// Сбрасываем накопленное значение
				result = 0;
				/**
				 * Выполняем повторный разбор значащих цифр
				 */
				for(const UC * position = startDigits; position != p; ++position){
					// Выполняем накопление очередной цифры с контролем переполнения
					if(!mulAddChecked(result, static_cast <uint64_t> (base), charToDigit(* position)))
						// Сообщаем о выходе значения за пределы диапазона
						return result_t <UC> (p, errc::result_out_of_range, error_t::OVERFLOW_RANGE);
				}
			}
			// Если разобрано отрицательное число
			if(negative){
				// Предел абсолютного значения для отрицательных чисел целевого типа
				const uint64_t limit = (static_cast <uint64_t> (numeric_limits <T>::max()) + 1ULL);
				// Если абсолютное значение выходит за пределы целевого типа
				if(result > limit)
					// Сообщаем о выходе значения за пределы диапазона
					return result_t <UC> (p, errc::result_out_of_range, error_t::OVERFLOW_RANGE);
				// Если разобрано отрицательное нулевое значение
				if(result == 0)
					// Устанавливаем нулевой результат разбора
					value = static_cast <T> (0);
				// Записываем результат разбора без опоры на переполнение знакового типа
				else value = static_cast <T> (-static_cast <T> (result - 1ULL) - 1);
			// Если разобрано положительное число
			} else {
				// Если значение выходит за пределы целевого типа
				if(result > static_cast <uint64_t> (numeric_limits <T>::max()))
					// Сообщаем о выходе значения за пределы диапазона
					return result_t <UC> (p, errc::result_out_of_range, error_t::OVERFLOW_RANGE);
				// Записываем результат разбора
				value = static_cast <T> (result);
			}
			// Выводим успешный результат разбора
			return result_t <UC> (p);
		}
	};
};

#endif // __AWH_LEXICAL_PARSER__
