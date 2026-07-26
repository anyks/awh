/**
 * @file: api.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
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
#ifndef __AWH_LEXICAL_API__
#define __AWH_LEXICAL_API__

/**
 * Стандартные заголовочные файлы
 */
#include <cfloat>
#include <limits>
#include <type_traits>
#include <system_error>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "parser.hpp"
#include "decimal.hpp"
#include "digits.hpp"

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
		 * @brief Метод проверки режима округления на округление к ближайшему
		 *
		 * @details Проверка эквивалентна сравнению fegetround() с FE_TONEAREST,
		 *          но выполняется существенно быстрее.
		 *
		 * @see https://lemire.me/blog/2022/11/16/a-fast-function-to-check-your-floating-point-rounding-mode/
		 *
		 * @return результат проверки
		 */
		AWH_LEXICAL_INLINE bool roundsToNearest() noexcept {
			/**
			 * При расширенной точности промежуточных вычислений проверка неприменима
			 */
			#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
				// Сообщаем, что режим округления к ближайшему не гарантирован
				return false;
			/**
			 * Проверка выполняется только для одинарной точности, так как
			 * для двойной точности и расширенной точности результат всегда верен
			 */
			#else
				/**
				 * Наименьшее нормализованное положительное число одинарной точности,
				 * объявленное изменяемым для запрета вычисления на этапе компиляции
				 */
				static volatile float minimum = numeric_limits <float>::min();
				// Выполняем однократное чтение значения
				const float value = minimum;
				/**
				 * Отключаем предупреждение о точном сравнении чисел с плавающей точкой
				 */
				#if AWH_LEXICAL_VISUAL_STUDIO
					// Отключаем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma warning(push)
				/**
				 * Компилятор Clang не поддерживает pragma warning, поэтому используем pragma diagnostic
				 */
				#elif __clang__
					// Отключаем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma clang diagnostic push
					// Игнорируем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma clang diagnostic ignored "-Wfloat-equal"
				/**
				 * Компилятор GCC не поддерживает pragma warning, поэтому используем pragma diagnostic
				 */
				#elif __GNUC__
					// Отключаем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma GCC diagnostic push
					// Игнорируем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma GCC diagnostic ignored "-Wfloat-equal"
				#endif
				// Равенство выполняется только при округлении к ближайшему
				const bool result = ((value + 1.0f) == (1.0f - value));
				/**
				 * Восстанавливаем предупреждение о точном сравнении
				 */
				#if AWH_LEXICAL_VISUAL_STUDIO
					// Восстанавливаем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma warning(pop)
				/**
				 * Компилятор Clang не поддерживает pragma warning, поэтому используем pragma diagnostic
				 */
				#elif __clang__
					// Восстанавливаем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma clang diagnostic pop
				/**
				 * Компилятор GCC не поддерживает pragma warning, поэтому используем pragma diagnostic
				 */
				#elif __GNUC__
					// Восстанавливаем предупреждение о точном сравнении чисел с плавающей точкой
					#pragma GCC diagnostic pop
				#endif
				// Выводим результат проверки режима округления
				return result;
			#endif
		}

		/**
		 * @brief Шаблон типа числа с плавающей точкой
		 *
		 * @tparam T тип числа с плавающей точкой
		 */
		template <typename T>
		/**
		 * @brief Метод быстрого преобразования мантиссы и показателя степени
		 *
		 * @details Метод реализует быстрый путь Клингера и даёт корректно округлённый
		 *          результат независимо от установленного режима округления потока.
		 *
		 * @param mantissa значение мантиссы
		 * @param exponent десятичный показатель степени
		 * @param negative признак отрицательного числа
		 * @param value    ссылка на результат преобразования
		 * @return         результат выполнения быстрого пути
		 */
		AWH_LEXICAL_INLINE bool clingerFastPath(const uint64_t mantissa, const int64_t exponent, const bool negative, T & value) noexcept {
			// Если показатель степени выходит за пределы быстрого пути
			if((exponent < static_cast <int64_t> (binary_t <T>::minExponentFastPath())) ||
			   (exponent > static_cast <int64_t> (binary_t <T>::maxExponentFastPath())))
				// Сообщаем, что быстрый путь неприменим
				return false;
			// Если установлен режим округления к ближайшему
			if(roundsToNearest()){
				// Если мантисса выходит за пределы точного представления
				if(mantissa > binary_t <T>::maxMantissaFastPath())
					// Сообщаем, что быстрый путь неприменим
					return false;
				// Выполняем преобразование мантиссы к типу результата
				value = static_cast <T> (mantissa);
				// Если показатель степени является отрицательным
				if(exponent < 0)
					// Выполняем деление на точную степень десяти
					value = (value / binary_t <T>::exactPowerOfTen(-exponent));
				// Выполняем умножение на точную степень десяти
				else value = (value * binary_t <T>::exactPowerOfTen(exponent));
				// Если разбираемое число является отрицательным
				if(negative)
					// Изменяем знак результата
					value = -value;
				// Сообщаем, что быстрый путь выполнен успешно
				return true;
			}
			/**
			 * Если показатель степени является отрицательным или мантисса
			 * выходит за пределы точного представления для этой степени
			 */
			if((exponent < 0) || (mantissa > binary_t <T>::maxMantissaFastPath(exponent))){
				// Сообщаем, что быстрый путь неприменим
				return false;
			}
			/**
			 * Отдельные компиляторы отображают нулевое значение в отрицательный
			 * нуль при округлении вниз, поэтому обрабатываем нуль явно
			 */
			#if __clang__ || AWH_LEXICAL_32BIT
				// Если мантисса является нулевой
				if(mantissa == 0){
					// Устанавливаем нулевой результат с корректным знаком
					value = (negative ? T(-0.0) : T(0.0));
					// Сообщаем, что быстрый путь выполнен успешно
					return true;
				}
			#endif
			// Выполняем умножение мантиссы на точную степень десяти
			value = (static_cast <T> (mantissa) * binary_t <T>::exactPowerOfTen(exponent));
			// Если разбираемое число является отрицательным
			if(negative)
				// Изменяем знак результата
				value = -value;
			// Сообщаем, что быстрый путь выполнен успешно
			return true;
		}

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC>
		/**
		 * @brief Метод разбора специальных значений бесконечности и нечисловых значений
		 *
		 * @details Диапазон символов обязан быть непустым: проверка выполняется вызывающей стороной.
		 *
		 * @param first   начало разбираемой строки
		 * @param last    конец разбираемой строки
		 * @param value   ссылка на результат разбора
		 * @param options опции разбора числовой строки
		 * @return        результат разбора числовой строки
		 */
		inline result_t <UC> parseInfNan(const UC * first, const UC * const last, T & value, const options_t <UC> options) noexcept {
			// Определяем знак разбираемого значения
			const bool negative = (* first == UC('-'));
			// Если строка начинается со знака значения
			if(negative || (isFormat(options.format, format_t::ALLOW_LEADING_PLUS) && (* first == UC('+'))))
				// Выполняем пропуск символа знака
				++first;
			// Если оставшейся длины строки достаточно для специального значения
			if((last - first) >= 3){
				// Если строка начинается с обозначения нечислового значения
				if(compareIgnoreCase(first, constNan <UC> (), 3)){
					// Выполняем пропуск обозначения нечислового значения
					first += 3;
					// Устанавливаем нечисловой результат с корректным знаком
					value = (negative ? -numeric_limits <T>::quiet_NaN() : numeric_limits <T>::quiet_NaN());
					// Позиция окончания разбора специального значения
					const UC * lastmatch = first;
					// Если за обозначением следует уточняющая последовательность
					if((first != last) && (* first == UC('('))){
						/**
						 * Выполняем поиск закрывающей скобки уточняющей последовательности
						 */
						for(const UC * position = (first + 1); position != last; ++position){
							// Если обнаружена закрывающая скобка
							if(* position == UC(')')){
								// Запоминаем позицию за закрывающей скобкой
								lastmatch = (position + 1);
								// Завершаем поиск закрывающей скобки
								break;
							}
							// Если обнаружен символ вне допустимого набора
							if(!(((UC('a') <= * position) && (* position <= UC('z'))) ||
								 ((UC('A') <= * position) && (* position <= UC('Z'))) ||
								 ((UC('0') <= * position) && (* position <= UC('9'))) ||
								 (* position == UC('_'))))
								// Завершаем поиск закрывающей скобки
								break;
						}
					}
					// Выводим успешный результат разбора
					return result_t <UC> (lastmatch);
				}
				// Если строка начинается с обозначения бесконечности
				if(compareIgnoreCase(first, constInf <UC> (), 3)){
					// Устанавливаем бесконечный результат с корректным знаком
					value = (negative ? -numeric_limits <T>::infinity() : numeric_limits <T>::infinity());
					// Если строка содержит полное обозначение бесконечности
					if(((last - first) >= 8) && compareIgnoreCase(first + 3, constInf <UC> () + 3, 5))
						// Выводим успешный результат разбора полного обозначения
						return result_t <UC> (first + 8);
					// Выводим успешный результат разбора сокращённого обозначения
					return result_t <UC> (first + 3);
				}
			}
			// Сообщаем о невозможности разобрать специальное значение
			return result_t <UC> (first, errc::invalid_argument, error_t::NO_DIGITS_IN_MANTISSA);
		}

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC>
		/**
		 * @brief Метод преобразования разобранной числовой строки в число
		 *
		 * @param number разобранная числовая строка
		 * @param value  ссылка на результат преобразования
		 * @return       результат преобразования
		 */
		inline result_t <UC> fromParsedNumber(const parsedNumber_t <UC> & number, T & value) noexcept {
			// Выполняем проверку поддержки типа результата
			static_assert(is_supported_float <T>::value, "AWH lexical: only some floating-point types are supported");
			// Выполняем проверку поддержки типа символа исходной строки
			static_assert(is_supported_char <UC>::value, "AWH lexical: only char, wchar_t, char16_t and char32_t are supported");
			// Если мантисса не усечена и быстрый путь применим
			if(!number.tooManyDigits && clingerFastPath(number.mantissa, number.exponent, number.negative, value))
				// Выводим успешный результат преобразования
				return result_t <UC> (number.lastmatch);
			// Выполняем вычисление двоичного представления числа
			mantissa_t result = computeFloat <binary_t <T>> (number.exponent, number.mantissa);
			// Если мантисса усечена, а вычисление дало однозначный результат
			if(number.tooManyDigits && (result.power2 >= 0)){
				// Если соседнее значение мантиссы даёт иной результат
				if(result != computeFloat <binary_t <T>> (number.exponent, number.mantissa + 1))
					// Выполняем повторное вычисление с пометкой необходимости уточнения
					result = computeError <binary_t <T>> (number.exponent, number.mantissa);
			}
			// Если однозначное округление невозможно
			if(result.power2 < 0)
				// Выполняем точное округление длинной арифметикой
				result = digitComp <T, UC> (number, result);
			// Выполняем сборку числа с плавающей точкой
			toFloat(number.negative, result, value);
			// Если получено антипереполнение либо переполнение результата
			if(((number.mantissa != 0) && (result.mantissa == 0) && (result.power2 == 0)) ||
			   (result.power2 == binary_t <T>::infinitePower()))
				// Сообщаем о выходе значения за пределы диапазона
				return result_t <UC> (number.lastmatch, errc::result_out_of_range, error_t::OVERFLOW_RANGE);
			// Выводим успешный результат преобразования
			return result_t <UC> (number.lastmatch);
		}

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC>
		/**
		 * @brief Метод разбора числа с плавающей точкой из строки
		 *
		 * @param first   начало разбираемой строки
		 * @param last    конец разбираемой строки
		 * @param value   ссылка на результат разбора
		 * @param options опции разбора числовой строки
		 * @return        результат разбора числовой строки
		 */
		inline result_t <UC> fromCharsFloat(const UC * first, const UC * const last, T & value, const options_t <UC> options) noexcept {
			// Выполняем проверку поддержки типа результата
			static_assert(is_supported_float <T>::value, "AWH lexical: only some floating-point types are supported");
			// Выполняем проверку поддержки типа символа исходной строки
			static_assert(is_supported_char <UC>::value, "AWH lexical: only char, wchar_t, char16_t and char32_t are supported");
			// Если разрешён пропуск ведущих пробельных символов
			if(isFormat(options.format, format_t::SKIP_WHITE_SPACE)){
				/**
				 * Выполняем пропуск ведущих пробельных символов
				 */
				while((first != last) && isSpace(* first))
					// Переходим к следующему символу
					++first;
			}
			// Если разбираемая строка оказалась пустой
			if(first == last)
				// Сообщаем о невозможности разобрать число
				return result_t <UC> (first, errc::invalid_argument, error_t::EMPTY_INPUT);
			// Выполняем разбор числовой строки в строгом или общем режиме
			const parsedNumber_t <UC> number = (isFormat(options.format, format_t::BASIC_JSON)
				? parseNumberString <true, UC> (first, last, options)
				: parseNumberString <false, UC> (first, last, options));
			// Если разбор числовой строки завершился отказом
			if(!number.valid){
				// Если специальные значения форматом запрещены
				if(isFormat(options.format, format_t::NO_INFNAN))
					// Сообщаем о невозможности разобрать число
					return result_t <UC> (first, errc::invalid_argument, number.error);
				// Выполняем разбор специального значения
				return parseInfNan(first, last, value, options);
			}
			// Выполняем преобразование разобранной числовой строки в число
			return fromParsedNumber(number, value);
		}

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип целого числа
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC>
		/**
		 * @brief Метод разбора целого числа из строки
		 *
		 * @param first   начало разбираемой строки
		 * @param last    конец разбираемой строки
		 * @param value   ссылка на результат разбора
		 * @param options опции разбора числовой строки
		 * @return        результат разбора числовой строки
		 */
		inline result_t <UC> fromCharsInt(const UC * first, const UC * const last, T & value, const options_t <UC> options) noexcept {
			// Выполняем проверку поддержки типа результата
			static_assert(is_supported_integer <T>::value, "AWH lexical: only integer types are supported");
			// Выполняем проверку поддержки типа символа исходной строки
			static_assert(is_supported_char <UC>::value, "AWH lexical: only char, wchar_t, char16_t and char32_t are supported");
			// Если разрешён пропуск ведущих пробельных символов
			if(isFormat(options.format, format_t::SKIP_WHITE_SPACE)){
				/**
				 * Выполняем пропуск ведущих пробельных символов
				 */
				while((first != last) && isSpace(* first))
					// Переходим к следующему символу
					++first;
			}
			// Если разбираемая строка оказалась пустой
			if(first == last)
				// Сообщаем о невозможности разобрать число
				return result_t <UC> (first, errc::invalid_argument, error_t::EMPTY_INPUT);
			// Если основание системы счисления выходит за допустимые пределы
			if((options.base < 2) || (options.base > 36))
				// Сообщаем о недопустимом основании системы счисления
				return result_t <UC> (first, errc::invalid_argument, error_t::INVALID_BASE);
			// Выполняем разбор числовой строки целого числа
			return parseIntString(first, last, value, options);
		}

		/**
		 * @brief Шаблон индекса категории типа результата
		 *
		 * @tparam INDEX индекс категории типа результата
		 */
		template <size_t INDEX>
		/**
		 * @brief Структура диспетчера разбора по категории типа результата
		 *
		 */
		struct dispatch_t {
			// Выполняем проверку поддержки типа результата
			static_assert(INDEX > 0, "AWH lexical: unsupported result type");
		};

		/**
		 * @brief Структура диспетчера разбора для чисел с плавающей точкой
		 *
		 */
		template <> struct dispatch_t <1> {
			/**
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип числа с плавающей точкой
			 * @tparam UC тип символа исходной строки
			 */
			template <typename T, typename UC>
			/**
			 * @brief Метод разбора числа с плавающей точкой из строки
			 *
			 * @param first   начало разбираемой строки
			 * @param last    конец разбираемой строки
			 * @param value   ссылка на результат разбора
			 * @param options опции разбора числовой строки
			 * @return        результат разбора числовой строки
			 */
			static result_t <UC> call(const UC * first, const UC * const last, T & value, const options_t <UC> options) noexcept {
				// Выполняем разбор числа с плавающей точкой
				return fromCharsFloat(first, last, value, options);
			}
		};

		/**
		 * @brief Структура диспетчера разбора для целых чисел
		 *
		 */
		template <> struct dispatch_t <2> {
			/**
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип целого числа
			 * @tparam UC тип символа исходной строки
			 */
			template <typename T, typename UC>
			/**
			 * @brief Метод разбора целого числа из строки
			 *
			 * @param first   начало разбираемой строки
			 * @param last    конец разбираемой строки
			 * @param value   ссылка на результат разбора
			 * @param options опции разбора числовой строки
			 * @return        результат разбора числовой строки
			 */
			static result_t <UC> call(const UC * first, const UC * const last, T & value, const options_t <UC> options) noexcept {
				// Выполняем разбор целого числа
				return fromCharsInt(first, last, value, options);
			}
		};

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип результата разбора
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC = char>
		/**
		 * @brief Метод разбора числа из строки с расширенными опциями
		 *
		 * @details Метод поддерживает как числа с плавающей точкой, так и целые.
		 *
		 * @param first   начало разбираемой строки
		 * @param last    конец разбираемой строки
		 * @param value   ссылка на результат разбора
		 * @param options опции разбора числовой строки
		 * @return        результат разбора числовой строки
		 */
		inline result_t <UC> fromCharsAdvanced(const UC * first, const UC * const last, T & value, const options_t <UC> options) noexcept {
			// Выполняем разбор числа диспетчером по категории типа результата
			return dispatch_t <
				static_cast <size_t> (is_supported_float <T>::value) +
				(2 * static_cast <size_t> (is_supported_integer <T>::value))
			>::call(first, last, value, options);
		}

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC = char, enableIf_t <is_supported_float <T>::value> = 0>
		/**
		 * @brief Метод разбора числа с плавающей точкой из строки
		 *
		 * @details Формат записи соответствует функции strtod в локали «C».
		 *          Результатом является ближайшее представимое значение,
		 *          округлённое по правилу IEEE 754 к ближайшему чётному.
		 *          Метод не выделяет память и не выбрасывает исключений.
		 *
		 * @param first  начало разбираемой строки
		 * @param last   конец разбираемой строки
		 * @param value  ссылка на результат разбора
		 * @param format допустимый формат записи числа
		 * @return       результат разбора числовой строки
		 */
		inline result_t <UC> fromChars(const UC * first, const UC * const last, T & value, const format_t format = format_t::GENERAL) noexcept {
			// Выполняем разбор числа с плавающей точкой
			return fromCharsFloat(first, last, value, options_t <UC> (format));
		}

		/**
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип целого числа
		 * @tparam UC тип символа исходной строки
		 */
		template <typename T, typename UC = char, enableIf_t <is_supported_integer <T>::value> = 0>
		/**
		 * @brief Метод разбора целого числа из строки
		 *
		 * @details Метод не выделяет память и не выбрасывает исключений.
		 *
		 * @param first начало разбираемой строки
		 * @param last  конец разбираемой строки
		 * @param value ссылка на результат разбора
		 * @param base  основание системы счисления в диапазоне от 2 до 36
		 * @return      результат разбора числовой строки
		 */
		inline result_t <UC> fromChars(const UC * first, const UC * const last, T & value, const int32_t base = 10) noexcept {
			// Выполняем разбор целого числа
			return fromCharsInt(first, last, value, options_t <UC> (format_t::GENERAL, UC('.'), base));
		}

		/**
		 * @brief Шаблон типа результата
		 *
		 * @tparam T тип числа с плавающей точкой
		 */
		template <typename T, enableIf_t <is_supported_float <T>::value> = 0>
		/**
		 * @brief Метод умножения беззнакового целого на степень десяти
		 *
		 * @details При переполнении результатом является бесконечность,
		 *          при антипереполнении — нуль. Метод не выделяет память
		 *          и не выбрасывает исключений.
		 *
		 * @param mantissa значение мантиссы
		 * @param exponent десятичный показатель степени
		 * @return         результат умножения
		 */
		inline T integerTimesPow10(const uint64_t mantissa, const int32_t exponent) noexcept {
			// Результат умножения
			T value = T(0);
			// Если быстрый путь преобразования применим
			if(clingerFastPath(mantissa, exponent, false, value))
				// Выводим результат быстрого пути
				return value;
			// Выполняем сборку результата из вычисленного двоичного представления
			toFloat(false, computeFloat <binary_t <T>> (exponent, mantissa), value);
			// Выводим результат умножения
			return value;
		}

		/**
		 * @brief Шаблон типа результата
		 *
		 * @tparam T тип числа с плавающей точкой
		 */
		template <typename T, enableIf_t <is_supported_float <T>::value> = 0>
		/**
		 * @brief Метод умножения знакового целого на степень десяти
		 *
		 * @details При переполнении результатом является бесконечность,
		 *          при антипереполнении — нуль. Метод не выделяет память
		 *          и не выбрасывает исключений.
		 *
		 * @param mantissa значение мантиссы
		 * @param exponent десятичный показатель степени
		 * @return         результат умножения
		 */
		inline T integerTimesPow10(const int64_t mantissa, const int32_t exponent) noexcept {
			// Определяем знак значения мантиссы
			const bool negative = (mantissa < 0);
			// Приводим значение мантиссы к абсолютному представлению
			const uint64_t absolute = (negative
				? (static_cast <uint64_t> (-(mantissa + 1)) + 1ULL)
				: static_cast <uint64_t> (mantissa));
			// Результат умножения
			T value = T(0);
			// Если быстрый путь преобразования применим
			if(clingerFastPath(absolute, exponent, negative, value))
				// Выводим результат быстрого пути
				return value;
			// Выполняем сборку результата из вычисленного двоичного представления
			toFloat(negative, computeFloat <binary_t <T>> (exponent, absolute), value);
			// Выводим результат умножения
			return value;
		}

		/**
		 * @brief Шаблон типа результата и типа исходного целого
		 *
		 * @tparam T   тип числа с плавающей точкой
		 * @tparam INT тип исходного целого числа
		 */
		template <typename T = double, typename INT, enableIf_t <is_supported_float <T>::value && is_integral <INT>::value> = 0>
		/**
		 * @brief Метод умножения целого произвольного типа на степень десяти
		 *
		 * @param mantissa значение мантиссы
		 * @param exponent десятичный показатель степени
		 * @return         результат умножения
		 */
		inline T integerTimesPow10(const INT mantissa, const int32_t exponent) noexcept {
			/**
			 * Выполняем приведение значения мантиссы к разрядности вычисления
			 */
			if constexpr (is_signed <INT>::value)
				// Выполняем умножение знакового значения мантиссы
				return integerTimesPow10 <T> (static_cast <int64_t> (mantissa), exponent);
			// Выполняем умножение беззнакового значения мантиссы
			else return integerTimesPow10 <T> (static_cast <uint64_t> (mantissa), exponent);
		}
	};
};

#endif // __AWH_LEXICAL_API__
