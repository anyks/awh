/**
 * @file: fmk.cpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация ядра фреймворка — базовые утилиты библиотеки: работа со строками и кодировками,
 *        смена регистра с учётом локали, форматирование, конвертация типов, проверка форматов данных и разбор чисел
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 *
	 * @note Подключается она прежде заголовков проекта и прочих заголовков MS Windows:
	 *       те самостоятельными не являются, а заголовок sys/os.hpp заводит макросом
	 *       имя u_char, какое системный _bsd_types.h объявляет типом через typedef
	 *
	 */
	#include <sys/win32.hpp>
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <locale>
#include <atomic>
#include <bitset>
#include <chrono>
#include <memory>
#include <random>
#include <limits>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <sys/types.h>

/**
 * Заголовочный файл для работы с быстрыми числами с плавающей точкой
 */
#include <num/lexical/lexical.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/global.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <encoding/ascii.hpp>
#include <encoding/charset/charset.hpp>
#include <encoding/unicode/utf8.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Сокращаем пространство имён проверок символов таблицы ASCII
 *
 * @details Протокольные данные определены стандартами в таблице ASCII и только
 *          в ней, а библиотечные проверки символов смотрят на текущую локаль,
 *          которую фреймворк устанавливает сам. Проверки ниже от локали не
 *          зависят и встраиванию поддаются. Обращение оставлено с указанием
 *          пространства имён намеренно: оно и отличает их от библиотечных.
 *
 */
namespace ascii = awh::ascii;

/**
 * @brief Инкапсулируем статические функции в пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция перевода широкого символа в верхний регистр
	 *
	 * @param letter переводимый символ
	 * @return       символ, переведённый в верхний регистр
	 *
	 * @details У операционных систем POSIX перевод выполняет towupper, опирающийся на
	 *          локаль LC_CTYPE. У MS Windows опереться на неё нельзя: setlocale там
	 *          отвергает любое заданное имя локали, - проверено на MinGW всеми принятыми
	 *          написаниями, включая "en_US.UTF-8", ".UTF-8" и "English_United States.65001", -
	 *          и отвечает лишь на пустую строку, ставящую локаль системы. А значит, набор
	 *          букв, какие towupper там переведёт, определяется не приложением, а тем,
	 *          какой язык выбран у машины: на русской Windows кириллица переводится, на
	 *          английской - остаётся нетронутой
	 *
	 *          Поэтому у MS Windows перевод выполняется средствами системы: LCMapStringEx
	 *          с локалью LOCALE_NAME_INVARIANT знает таблицу Юникода целиком и от выбранного
	 *          у машины языка не зависит
	 *
	 * @note Быстрый путь для ASCII заведён затем, что перевод строк идёт посимвольно, а
	 *       обращение к средствам системы дороже сличения с границей диапазона
	 *
	 */
	wint_t wideUpper(const wint_t letter) noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если символ принадлежит набору ASCII - переводим его напрямую
			if(letter < 0x80)
				// Выводим переведённый символ
				return (((letter >= L'a') && (letter <= L'z')) ? (letter - 0x20) : letter);
			// Исходный символ в виде, пригодном для передачи системе
			const wchar_t source = static_cast <wchar_t> (letter);
			// Результат перевода символа
			wchar_t result = 0;
			// Если перевод символа средствами системы выполнить не удалось - оставляем символ нетронутым
			if(::LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, &source, 1, &result, 1, nullptr, nullptr, 0) != 1)
				// Выводим символ нетронутым
				return letter;
			// Выводим переведённый символ
			return static_cast <wint_t> (result);
		/**
		 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
		 */
		#else
			// Выполняем перевод символа в верхний регистр
			return ::towupper(letter);
		#endif
	}
	/**
	 * @brief Функция перевода широкого символа в нижний регистр
	 *
	 * @param letter переводимый символ
	 * @return       символ, переведённый в нижний регистр
	 *
	 * @details Пояснение к работе средства смотрите у wideUpper
	 *
	 */
	wint_t wideLower(const wint_t letter) noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если символ принадлежит набору ASCII - переводим его напрямую
			if(letter < 0x80)
				// Выводим переведённый символ
				return (((letter >= L'A') && (letter <= L'Z')) ? (letter + 0x20) : letter);
			// Исходный символ в виде, пригодном для передачи системе
			const wchar_t source = static_cast <wchar_t> (letter);
			// Результат перевода символа
			wchar_t result = 0;
			// Если перевод символа средствами системы выполнить не удалось - оставляем символ нетронутым
			if(::LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, &source, 1, &result, 1, nullptr, nullptr, 0) != 1)
				// Выводим символ нетронутым
				return letter;
			// Выводим переведённый символ
			return static_cast <wint_t> (result);
		/**
		 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
		 */
		#else
			// Выполняем перевод символа в нижний регистр
			return ::towlower(letter);
		#endif
	}
	/**
	 * @brief Функция декодирования буфера в кодировке UTF-8 в широкую строку
	 *
	 * @param data указатель на буфер данных в кодировке UTF-8
	 * @param size размер буфера данных в байтах
	 * @return     результирующая широкая строка (UTF-32 при 4-байтовом wchar_t, UTF-16 при 2-байтовом)
	 *
	 */
	wstring utf8ToWide(const char * data, const size_t size) noexcept {
		// Если данные не переданы
		if((data == nullptr) || (size == 0))
			// Возвращаем пустой результат
			return L"";
		/**
		 * @brief Ядро декодирования: пишет кодовые юниты в буфер out и возвращает их количество
		 *
		 * @details Размерная гарантия: число кодовых юнитов не превышает число входных байт (size),
		 *          поэтому буфер размером size всегда достаточен.
		 *
		 * @param out буфер для записи широких символов (вместимость не меньше size)
		 * @return    количество записанных кодовых юнитов
		 *
		 */
		const auto decode = [data, size](wchar_t * out) noexcept -> size_t {
			// Счётчик записанных символов
			size_t count = 0;
			// Начальный итератор указывает на первый байт данных
			const u_char * begin = reinterpret_cast <const u_char *> (data);
			// Конечный итератор указывает на байт за последним байтом данных
			const u_char * end = (begin + size);
			/**
			 * Выполняем перебор всех байт буфера
			 */
			while(begin < end){
				// Получаем ведущий байт последовательности
				const u_char lead = (* begin);
				/**
				 * Быстрый путь: копируем прогон ASCII-символов целиком
				 */
				if(lead < 0x80){
					// Находим конец ASCII-прогона
					const u_char * run = begin;
					/**
					 * Продвигаем итератор run, пока он указывает на ASCII-байт и не достигнет конца данных
					 */
					while((run < end) && ((* run) < 0x80))
						// Увеличиваем итератор run
						++run;
					/**
					 * Копируем прогон с расширением до wchar_t
					 */
					while(begin < run)
						// Копируем ASCII-байт в широкий символ
						out[count++] = static_cast <wchar_t> (* begin++);
					// Переходим к следующей итерации
					continue;
				}
				// Текущая кодовая точка
				uint32_t cp = 0;
				// Количество байт в последовательности
				uint8_t num = 0;
				// Если это последовательность из двух байт
				if((lead & 0xE0) == 0xC0){
					// Устанавливаем количество байт в последовательности
					num = 2;
					// Извлекаем первые пять бит кодовой точки из ведущего байта
					cp = (lead & 0x1F);
				// Если это последовательность из трёх байт
				} else if((lead & 0xF0) == 0xE0) {
					// Устанавливаем количество байт в последовательности
					num = 3;
					// Извлекаем первые четыре бит кодовой точки из ведущего байта
					cp = (lead & 0x0F);
				// Если это последовательность из четырёх байт
				} else if((lead & 0xF8) == 0xF0) {
					// Устанавливаем количество байт в последовательности
					num = 4;
					// Извлекаем первые три бит кодовой точки из ведущего байта
					cp = (lead & 0x07);
				// Если ведущий байт некорректен, прекращаем разбор
				} else break;
				// Если последовательность выходит за границу буфера
				if((begin + num) > end)
					// Прекращаем разбор
					break;
				// Смещаемся к байтам продолжения
				++begin;
				// Флаг корректности последовательности
				bool valid = true;
				/**
				 * Собираем кодовую точку из байт продолжения
				 */
				for(uint8_t i = 1; i < num; ++i, ++begin){
					// Если это не байт продолжения, последовательность некорректна
					if(((* begin) & 0xC0) != 0x80){
						// Устанавливаем флаг некорректности последовательности
						valid = false;
						// Прекращаем разбор
						break;
					}
					// Дописываем очередные шесть бит кодовой точки
					cp = ((cp << 6) | ((* begin) & 0x3F));
				}
				// Если последовательность некорректна
				if(!valid)
					// Прекращаем разбор
					break;
				/**
				 * Записываем кодовую точку в результат с учётом разрядности wchar_t
				 */
				if constexpr (sizeof(wchar_t) >= 4)
					// Для 4-байтового wchar_t (UTF-32) пишем одну кодовую точку
					out[count++] = static_cast <wchar_t> (cp);
				// Для 2-байтового wchar_t (UTF-16) символ из BMP пишем как один кодовый юнит
				else if(cp <= 0xFFFF)
					// Записываем кодовую точку
					out[count++] = static_cast <wchar_t> (cp);
				// Символ вне BMP кодируем суррогатной парой
				else {
					// Переводим кодовую точку в диапазон суррогатов
					cp -= 0x10000;
					// Записываем старший суррогат
					out[count++] = static_cast <wchar_t> (0xD800 + (cp >> 10));
					// Записываем младший суррогат
					out[count++] = static_cast <wchar_t> (0xDC00 + (cp & 0x3FF));
				}
			}
			// Возвращаем количество записанных символов
			return count;
		};
		/**
		 * Если стандартная библиотека поддерживает запись напрямую в буфер строки
		 */
		#if defined(__cpp_lib_string_resize_and_overwrite)
			// Результирующая широкая строка
			wstring result = L"";
			// Пишем результат сразу в буфер строки без обнуления и без промежуточного копирования (C++23)
			result.resize_and_overwrite(size, [&decode](wchar_t * out, size_t) noexcept -> size_t {
				// Выполняем декодирование напрямую в буфер результата
				return decode(out);
			});
			// Возвращаем результат
			return result;
		/**
		 * Иначе используем неинициализированный промежуточный буфер
		 */
		#else
			// Выделяем неинициализированный буфер (число кодовых юнитов не превышает число байт)
			unique_ptr <wchar_t []> scratch(new wchar_t[size]);
			// Декодируем данные в промежуточный буфер
			const size_t count = decode(scratch.get());
			// Конструируем результат точной длины (одна аллокация, без обнуления)
			return wstring(scratch.get(), scratch.get() + count);
		#endif
	}
	/**
	 * @brief Функция кодирования широкой строки в строку в кодировке UTF-8
	 *
	 * @param data буфер широких символов (UTF-32 при 4-байтовом wchar_t, UTF-16 при 2-байтовом)
	 * @param size размер буфера данных в символах
	 * @return     результирующая строка в кодировке UTF-8
	 *
	 */
	string wideToUtf8(const wchar_t * data, const size_t size) noexcept {
		// Если данные не переданы
		if((data == nullptr) || (size == 0))
			// Возвращаем пустой результат
			return "";
		/**
		 * @brief Ядро кодирования: пишет байты UTF-8 в буфер out и возвращает их количество
		 *
		 * @details Размерная гарантия: каждый входной кодовый юнит даёт не более 4 байт UTF-8,
		 *          поэтому буфер размером (size * 4) всегда достаточен.
		 *
		 * @param out буфер для записи байт UTF-8 (вместимость не меньше size * 4)
		 * @return    количество записанных байт
		 *
		 */
		const auto encode = [data, size](char * out) noexcept -> size_t {
			// Счётчик записанных байт
			size_t count = 0;
			// Начальный итератор указывает на первый символ данных
			const wchar_t * begin = data;
			// Конечный итератор указывает на символ за последним символом данных
			const wchar_t * end = (data + size);
			/**
			 * Выполняем перебор всех символов буфера
			 */
			while(begin < end){
				// Получаем текущую кодовую точку
				uint32_t cp = static_cast <uint32_t> (* begin++);
				/**
				 * Для 2-байтового wchar_t (UTF-16) обрабатываем суррогатные пары
				 */
				if constexpr (sizeof(wchar_t) < 4){
					// Если это старший суррогат
					if((cp >= 0xD800) && (cp <= 0xDBFF)){
						// Если следом идёт младший суррогат
						if((begin < end) && (static_cast <uint32_t> (* begin) >= 0xDC00) && (static_cast <uint32_t> (* begin) <= 0xDFFF))
							// Собираем полную кодовую точку из суррогатной пары
							cp = (0x10000 + ((cp - 0xD800) << 10) + (static_cast <uint32_t> (* begin++) - 0xDC00));
						// Одиночный суррогат некорректен, прекращаем разбор
						else break;
					}
				}
				/**
				 * Кодируем кодовую точку в последовательность UTF-8
				 */
				if(cp <= 0x7F)
					// Записываем одиночный байт (ASCII)
					out[count++] = static_cast <char> (cp);
				// Последовательность из двух байт
				else if(cp <= 0x7FF) {
					// Записываем ведущий байт
					out[count++] = static_cast <char> (0xC0 | (cp >> 6));
					// Записываем байт продолжения
					out[count++] = static_cast <char> (0x80 | (cp & 0x3F));
				// Последовательность из трёх байт
				} else if(cp <= 0xFFFF) {
					// Записываем ведущий байт
					out[count++] = static_cast <char> (0xE0 | (cp >> 12));
					// Записываем байты продолжения
					out[count++] = static_cast <char> (0x80 | ((cp >> 6) & 0x3F));
					out[count++] = static_cast <char> (0x80 | (cp & 0x3F));
				// Последовательность из четырёх байт
				} else if(cp <= 0x10FFFF) {
					// Записываем ведущий байт
					out[count++] = static_cast <char> (0xF0 | (cp >> 18));
					// Записываем байты продолжения
					out[count++] = static_cast <char> (0x80 | ((cp >> 12) & 0x3F));
					out[count++] = static_cast <char> (0x80 | ((cp >> 6) & 0x3F));
					out[count++] = static_cast <char> (0x80 | (cp & 0x3F));
				// Кодовая точка вне диапазона Unicode, прекращаем разбор
				} else break;
			}
			// Возвращаем количество записанных байт
			return count;
		};
		/**
		 * Если стандартная библиотека поддерживает запись напрямую в буфер строки
		 */
		#if defined(__cpp_lib_string_resize_and_overwrite)
			// Результирующая строка
			string result = "";
			// Пишем результат сразу в буфер строки без обнуления и без промежуточного копирования (C++23)
			result.resize_and_overwrite(size * 4, [&encode](char * out, size_t) noexcept -> size_t {
				// Выполняем кодирование напрямую в буфер результата
				return encode(out);
			});
			// Возвращаем результат
			return result;
		/**
		 * Иначе используем неинициализированный промежуточный буфер
		 */
		#else
			// Выделяем неинициализированный буфер (каждый кодовый юнит даёт не более 4 байт UTF-8)
			unique_ptr <char []> scratch(new char[size * 4]);
			// Кодируем данные в промежуточный буфер
			const size_t count = encode(scratch.get());
			// Конструируем результат точной длины (одна аллокация, без обнуления)
			return string(scratch.get(), scratch.get() + count);
		#endif
	}
	/**
	 * @brief Функция записи числа с плавающей точкой в безэкспоненциальной форме
	 *
	 * @details Разделителем дробной части в записи всегда служит точка, разделителей
	 *          разрядов запись не содержит, какой бы ни была установленная локаль.
	 *          Количество знаков после запятой либо задаётся явно, и тогда дробная
	 *          часть округляется, либо, если задано отрицательным, подбирается
	 *          наименьшим из тех, при котором запись читается обратно ровно тем же
	 *          числом. Подбор даёт краткую запись, не теряя при этом ни одного
	 *          значащего разряда: число 0.111 остаётся записью «0.111», а результат
	 *          деления 1536 на 1024 - записью «1.5»
	 *
	 * @details Запись выполняется средствами языка Си, а не функцией std::to_chars:
	 *          записывать числа с плавающей точкой та умеет далеко не везде, а
	 *          сообщить об этом нечем. Макрос __cpp_lib_to_chars отмечает готовность
	 *          возможности целиком, включая запись в общей форме, которой в libc++
	 *          нет до сих пор, поэтому и на свежих её выпусках макрос не объявлен.
	 *          Вдобавок в libc++ запись эта помечена доступной лишь начиная с macOS
	 *          13.3, и сборка под ранние выпуски системы прерывается ошибкой
	 *
	 * @param number    число для записи
	 * @param precision количество знаков после запятой, отрицательное для подбора
	 * @return          число в безэкспоненциальной форме
	 *
	 */
	string noexpFixed(const double number, const int32_t precision) noexcept {
		// Если число не является конечным, записывать нечего
		if(!::isfinite(number))
			// Выводим нулевой результат
			return string(1, '0');
		/**
		 * Ноль записывается сразу: отрицательный ноль от обычного в записи ничем
		 * отличаться не должен
		 */
		if(number == 0.)
			// Выводим нулевой результат
			return string(1, '0');
		// Количество знаков после запятой в записи числа
		int32_t fraction = precision;
		/**
		 * Если количество знаков после запятой требуется подобрать
		 */
		if(fraction < 0){
			// Буфер записи числа с показателем степени
			char probe[64];
			/**
			 * Количество значащих разрядов, при котором запись читается обратно тем же
			 * числом. Пятнадцати разрядов хватает подавляющему большинству чисел, а
			 * семнадцати - любому числу двойной точности без единого исключения
			 */
			int32_t digits = 17;
			/**
			 * Подбираем наименьшее количество значащих разрядов
			 */
			for(int32_t i = 15; i < 17; i++){
				// Выполняем запись числа с показателем степени
				::snprintf(probe, sizeof(probe), "%.*e", (i - 1), number);
				// Если запись читается обратно ровно тем же числом
				if(::strtod(probe, nullptr) == number){
					// Запоминаем подобранное количество значащих разрядов
					digits = i;
					// Выходим из цикла подбора
					break;
				}
			}
			/**
			 * Денормализованные числа отстоят друг от друга настолько далеко, что
			 * читаются обратно и по куда меньшему количеству значащих разрядов. Лишние
			 * разряды в их записи нулевыми не выходят и отброшены не будут, поэтому
			 * подбираем им количество разрядов убыванием. Числа эти в записи появляются
			 * до того редко, что на общей стоимости подбор этот не сказывается
			 */
			const bool subnormal = (::fabs(number) < numeric_limits <double>::min());
			// Если число оказалось денормализованным
			if(subnormal){
				/**
				 * Убавляем количество значащих разрядов, пока запись читается обратно
				 */
				while(digits > 1){
					// Выполняем запись числа на один значащий разряд короче
					::snprintf(probe, sizeof(probe), "%.*e", (digits - 2), number);
					// Если укороченная запись читается обратно уже иным числом
					if(::strtod(probe, nullptr) != number)
						// Выходим из цикла подбора
						break;
					// Принимаем укороченную запись
					digits--;
				}
			}
			/**
			 * Показатель степени берётся из самой записи, а не вычисляется логарифмом:
			 * логарифм на границах порядка ошибается в последнем разряде, и запись
			 * теряла бы значащий разряд
			 */
			if(subnormal || (digits == 17))
				/**
				 * Запись выполняется заново лишь тогда, когда в буфере осталась не она:
				 * подбор убыванием заканчивается заведомо укороченной записью, а подбор
				 * возрастанием при неудаче - записью на разряд короче требуемой
				 */
				::snprintf(probe, sizeof(probe), "%.*e", (digits - 1), number);
			// Выполняем поиск показателя степени в записи числа
			const char * pos = ::strchr(probe, 'e');
			// Получаем показатель степени записанного числа
			const int32_t exponent = ((pos != nullptr) ? ::atoi(pos + 1) : 0);
			/**
			 * Количество знаков после запятой равно количеству значащих разрядов за
			 * вычетом тех из них, что пришлись на целую часть числа
			 */
			fraction = ((digits - 1) - exponent);
			// Если все значащие разряды пришлись на целую часть числа
			if(fraction < 0)
				// Дробной части в записи не будет вовсе
				fraction = 0;
		}
		// Переменная результата
		string result = "";
		/**
		 * Буфер записи взят с запасом: наибольшую запись даёт денормализованное число,
		 * и она занимает менее четырёхсот разрядов
		 */
		char buffer[512];
		// Выполняем запись числа в буфер
		int32_t length = ::snprintf(buffer, sizeof(buffer), "%.*f", fraction, number);
		// Если запись выполнена
		if(length < 0)
			// Выводим нулевой результат
			return string(1, '0');
		// Если запись уместилась в буфер
		if(static_cast <size_t> (length) < sizeof(buffer))
			// Получаем выполненную запись числа
			result.assign(buffer, static_cast <size_t> (length));
		/**
		 * Иначе выполняем запись в буфер требуемого размера
		 */
		else {
			// Выделяем буфер размера, которого запись потребовала
			unique_ptr <char []> scratch(new char[static_cast <size_t> (length) + 1]);
			// Выполняем запись числа в выделенный буфер
			length = ::snprintf(scratch.get(), (static_cast <size_t> (length) + 1), "%.*f", fraction, number);
			// Если запись выполнить не удалось
			if(length < 0)
				// Выводим нулевой результат
				return string(1, '0');
			// Получаем выполненную запись числа
			result.assign(scratch.get(), static_cast <size_t> (length));
		}
		/**
		 * Разделителем дробной части в локалях многих стран служит запятая, а запись
		 * числа обязана оставаться одинаковой в любой из них
		 */
		const size_t point = result.find(',');
		// Если разделителем дробной части оказалась запятая
		if(point != string::npos)
			// Заменяем её точкой
			result[point] = '.';
		/**
		 * Если количество знаков после запятой подбиралось, лишние разряды записи
		 * оказались нулевыми и никакого смысла не несут
		 */
		if((precision < 0) && (result.find('.') != string::npos)){
			/**
			 * Удаляем хвостовые нули, а следом и разделитель дробной части
			 */
			while(!result.empty()){
				// Получаем последний символ записи
				const char letter = result.back();
				// Если последним символом записи оказался ноль
				if(letter == '0')
					// Удаляем последний символ записи
					result.pop_back();
				// Если последним символом записи оказался разделитель дробной части
				else if(letter == '.') {
					// Удаляем разделитель дробной части
					result.pop_back();
					// Выходим из цикла
					break;
				// В остальных случаях завершаем перебор
				} else break;
			}
		}
		// Если записывать оказалось нечего
		if(result.empty())
			// Выводим нулевой результат
			return string(1, '0');
		// Выводим полученную запись числа
		return result;
	}

	/**
	 * @brief Шаблон функции разделения строк на составляющие
	 *
	 * @tparam T тип контейнера в котором извлекается результат
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция разделения строк на составляющие
	 *
	 * @param str       строка для поиска
	 * @param delim     разделитель
	 * @param container контенер содержащий данные
	 * @param log       объект работы с логами
	 * @return          контенер содержащий данные
	 *
	 */
	T & split(string_view str, string_view delim, T & container, const awh::log_t * log) noexcept {
		/**
		 * @brief Функция удаления пробелов вначале и конце текста
		 *
		 * @param text текст для удаления пробелов
		 * @return     результат работы функции
		 *
		 */
		auto trimFn = [&](string & text) noexcept -> string & {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем удаление пробелов в начале текста
				text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](const char letter) noexcept -> bool {
					// Выполняем проверку символа на наличие пробела
					return ascii::isSpace(letter);
				}));
				// Выполняем удаление пробелов в конце текста
				text.erase(find_if_not(text.rbegin(), text.rend(), [](const char letter) noexcept -> bool {
					// Выполняем проверку символа на наличие пробела
					return ascii::isSpace(letter);
				}).base(), text.end());
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Если объект логирования установлен
				if(log != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str, delim, container.size()), awh::log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
					#endif
				// Если объект логирования не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
			// Возвращаем результат
			return text;
		};
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Очищаем словарь
			container.clear();
			// Переменная результата
			string result = "";
			// Получаем счётчики перебора
			size_t index = 0, pos = str.find(delim);
			/**
			 * Выполняем разбиение строк
			 */
			while(pos != string::npos){
				// Получаем полученный текст
				result = str.substr(index, pos - index);
				// Вставляем полученный результат в контейнер
				container.insert(container.end(), trimFn(result));
				// Выполняем смещение в тексте
				index = ++pos + (delim.size() - 1);
				// Выполняем поиск разделителя в тексте
				pos = str.find(delim, pos);
				// Если мы дошли до конца текста
				if(pos == string::npos){
					// Получаем полученный текст
					result = str.substr(index, str.size());
					// Вставляем полученный результат в контейнер
					container.insert(container.end(), trimFn(result));
				}
			}
			// Если слово передано а вектор пустой, тогда создаем вектори из 1-го элемента
			if(!str.empty() && container.empty()){
				// Получаем полученный текст
				result = str.substr(index, pos - index);
				// Вставляем полученный результат в контейнер
				container.insert(container.end(), trimFn(result));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str, delim, container.size()), awh::log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
		// Возвращаем результат
		return container;
	}
	/**
	 * @brief Шаблон функции разделения строк на составляющие
	 *
	 * @tparam T тип контейнера в котором извлекается результат
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция разделения строк на составляющие
	 *
	 * @param str       строка для поиска
	 * @param delim     разделитель
	 * @param container контенер содержащий данные
	 * @param log       объект работы с логами
	 * @return          контенер содержащий данные
	 *
	 */
	T & split(wstring_view str, wstring_view delim, T & container, const awh::log_t * log) noexcept {
		/**
		 * @brief Функция удаления пробелов вначале и конце текста
		 *
		 * @param text текст для удаления пробелов
		 * @return     результат работы функции
		 *
		 */
		auto trimFn = [&](wstring & text) noexcept -> wstring & {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем удаление пробелов в начале текста
				text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](const wchar_t letter) noexcept -> bool {
					// Выполняем проверку символа на наличие пробела
					return (static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
				}));
				// Выполняем удаление пробелов в конце текста
				text.erase(find_if_not(text.rbegin(), text.rend(), [](const wchar_t letter) noexcept -> bool {
					// Выполняем проверку символа на наличие пробела
					return (static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
				}).base(), text.end());
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Если объект логирования установлен
				if(log != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str.size(), delim.size(), container.size()), awh::log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
					#endif
				// Если объект логирования не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
			// Возвращаем результат
			return text;
		};
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Очищаем словарь
			container.clear();
			// Переменная результата
			wstring result = L"";
			// Получаем счётчики перебора
			size_t index = 0, pos = str.find(delim);
			/**
			 * Выполняем разбиение строк
			 */
			while(pos != wstring::npos){
				// Получаем полученный текст
				result = str.substr(index, pos - index);
				// Вставляем полученный результат в контейнер
				container.insert(container.end(), trimFn(result));
				// Выполняем смещение в тексте
				index = ++pos + (delim.size() - 1);
				// Выполняем поиск разделителя в тексте
				pos = str.find(delim, pos);
				// Если мы дошли до конца текста
				if(pos == wstring::npos){
					// Получаем полученный текст
					result = str.substr(index, str.size());
					// Вставляем полученный результат в контейнер
					container.insert(container.end(), trimFn(result));
				}
			}
			// Если слово передано а вектор пустой, тогда создаем вектори из 1-го элемента
			if(!str.empty() && container.empty()){
				// Получаем полученный текст
				result = str.substr(index, pos - index);
				// Вставляем полученный результат в контейнер
				container.insert(container.end(), trimFn(result));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str.size(), delim.size(), container.size()), awh::log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
		// Возвращаем результат
		return container;
	}

	/**
	 * @brief структура Римских чисел
	 *
	 */
	struct RomanNumerals {
		// Шаблоны римских форматов
		const wstring m[5]  = {L"", L"M", L"MM", L"MMM", L"MMMM"};
		const wstring i[10] = {L"", L"I", L"II", L"III", L"IV", L"V", L"VI", L"VII", L"VIII", L"IX"};
		const wstring x[10] = {L"", L"X", L"XX", L"XXX", L"XL", L"L", L"LX", L"LXX", L"LXXX", L"XC"};
		const wstring c[10] = {L"", L"C", L"CC", L"CCC", L"CD", L"D", L"DC", L"DCC", L"DCCC", L"CM"};
	} romanNumerals;
	/**
	 * @brief Класс основных символов
	 *
	 */
	class Symbols {
		private:
			// Контейнер римских чисел
			unordered_map <char, uint16_t> _romes;
			// Контейнер арабских чисел
			unordered_map <char, uint8_t> _arabics;
		private:
			// Контейнер латинских символов
			unordered_map <char, wchar_t> _letters;
			// Контейнер латинских символов для UTF-8
			unordered_map <wchar_t, char> _wideLetters;
		private:
			// Контейнер римских чисел для UTF-8
			unordered_map <wchar_t, uint16_t> _wideRomes;
			// Контейнер арабских чисел для UTF-8
			unordered_map <wchar_t, uint8_t> _wideArabics;
		public:
			/**
			 * @brief Метод проверки соответствия римской цифре
			 *
			 * @param num римская цифра для проверки
			 * @return    результат проверки
			 *
			 */
			bool isRome(const char num) const noexcept {
				// Выполняем проверку сущестования цифры
				return (this->_romes.find(ascii::toUpper(num)) != this->_romes.end());
			}
			/**
			 * @brief Метод проверки соответствия римской цифре
			 *
			 * @param num римская цифра для проверки
			 * @return    результат проверки
			 *
			 */
			bool isRome(const wchar_t num) const noexcept {
				// Выполняем проверку сущестования цифры
				return (this->_wideRomes.find(static_cast <wchar_t> (wideUpper(static_cast <wint_t> (num)))) != this->_wideRomes.end());
			}
		public:
			/**
			 * @brief Метод проверки соответствия арабской цифре
			 *
			 * @param num арабская цифра для проверки
			 * @return    результат проверки
			 *
			 */
			bool isArabic(const char num) const noexcept {
				// Выполняем проверку сущестования цифры
				return ascii::isDigit(num);
			}
			/**
			 * @brief Метод проверки соответствия арабской цифре
			 *
			 * @param num арабская цифра для проверки
			 * @return    результат проверки
			 *
			 */
			bool isArabic(const wchar_t num) const noexcept {
				// Выполняем проверку сущестования цифры
				return static_cast <bool> (::iswdigit(static_cast <wint_t> (num)));
			}
		public:
			/**
			 * @brief Метод проверки соответствия латинской букве
			 *
			 * @param letter латинская буква для проверки
			 * @return       результат проверки
			 *
			 */
			bool isLetter(const char letter) const noexcept {
				// Выполняем проверку сущестования латинской буквы
				return (this->_letters.find(ascii::toLower(letter)) != this->_letters.end());
			}
			/**
			 * @brief Метод проверки соответствия латинской букве
			 *
			 * @param letter латинская буква для проверки
			 * @return       результат проверки
			 *
			 */
			bool isLetter(const wchar_t letter) const noexcept {
				// Выполняем проверку сущестования латинской буквы
				return (this->_wideLetters.find(static_cast <wchar_t> (wideLower(static_cast <wint_t> (letter)))) != this->_wideLetters.end());
			}
		public:
			/**
			 * @brief Метод извлечения римской цифры
			 *
			 * @param num римская цифра для извлечения
			 * @return    арабская цифрва в виде числа
			 *
			 */
			uint16_t getRome(const char num) const noexcept {
				// Переменная результата
				uint16_t result = 0;
				// Выполняем поиск римского числа
				auto i = this->_romes.find(ascii::toUpper(num));
				// Если римское число найдено
				if(i != this->_romes.end())
					// Получаем римское число в чистом виде
					result = i->second;
				// Возвращаем результат
				return result;
			}
			/**
			 * @brief Метод извлечения римской цифры
			 *
			 * @param num римская цифра для извлечения
			 * @return    арабская цифрва в виде числа
			 *
			 */
			uint16_t getRome(const wchar_t num) const noexcept {
				// Переменная результата
				uint16_t result = 0;
				// Выполняем поиск римского числа
				auto i = this->_wideRomes.find(static_cast <wchar_t> (wideUpper(static_cast <wint_t> (num))));
				// Если римское число найдено
				if(i != this->_wideRomes.end())
					// Получаем римское число в чистом виде
					result = i->second;
				// Возвращаем результат
				return result;
			}
		public:
			/**
			 * @brief Метод извлечения арабской цифры
			 *
			 * @param num арабская цифра для извлечения
			 * @return    арабская цифрва в виде числа
			 *
			 */
			uint8_t getArabic(const char num) const noexcept {
				// Переменная результата
				uint8_t result = 0;
				// Выполняем поиск арабского числа
				auto i = this->_arabics.find(num);
				// Если арабское число найдено
				if(i != this->_arabics.end())
					// Получаем арабское число в чистом виде
					result = i->second;
				// Возвращаем результат
				return result;
			}
			/**
			 * @brief Метод извлечения арабской цифры
			 *
			 * @param num арабская цифра для извлечения
			 * @return    арабская цифрва в виде числа
			 *
			 */
			uint8_t getArabic(const wchar_t num) const noexcept {
				// Переменная результата
				uint8_t result = 0;
				// Выполняем поиск арабского числа
				auto i = this->_wideArabics.find(num);
				// Если арабское число найдено
				if(i != this->_wideArabics.end())
					// Получаем арабское число в чистом виде
					result = i->second;
				// Возвращаем результат
				return result;
			}
		public:
			/**
			 * @brief Метод извлечения латинской буквы
			 *
			 * @param letter латинская буква для извлечения
			 * @return       латинская буква в виде символа
			 *
			 */
			wchar_t getLetter(const char letter) const noexcept {
				// Переменная результата
				wchar_t result = 0;
				// Выполняем поиск латинской буквы
				auto i = this->_letters.find(ascii::toLower(letter));
				// Если латинская буква найдена
				if(i != this->_letters.end())
					// Получаем латинскую букву в чистом виде
					result = i->second;
				// Возвращаем результат
				return result;
			}
			/**
			 * @brief Метод извлечения латинской буквы
			 *
			 * @param letter латинская буква для извлечения
			 * @return       латинская буква в виде символа
			 *
			 */
			char getLetter(const wchar_t letter) const noexcept {
				// Переменная результата
				char result = 0;
				// Выполняем поиск латинской буквы
				auto i = this->_wideLetters.find(static_cast <wchar_t> (wideLower(static_cast <wint_t> (letter))));
				// Если латинская буква найдена
				if(i != this->_wideLetters.end())
					// Получаем латинскую букву в чистом виде
					result = i->second;
				// Возвращаем результат
				return result;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Symbols() noexcept {
				/**
				 * Выполняем заполнение арабских чисел
				 */
				this->_arabics = {
					{'0', 0}, {'1', 1},
					{'2', 2}, {'3', 3},
					{'4', 4}, {'5', 5},
					{'6', 6}, {'7', 7},
					{'8', 8}, {'9', 9}
				};
				/**
				 * Выполняем заполнение арабских чисел для UTF-8
				 */
				this->_wideArabics = {
					{L'0',0}, {L'1',1},
					{L'2',2}, {L'3',3},
					{L'4',4}, {L'5',5},
					{L'6',6}, {L'7',7},
					{L'8',8}, {L'9',9}
				};
				/**
				 * Выполняем заполнение римских чисел
				 */
				this->_romes = {
					{'I',1}, {'V',5},
					{'X',10}, {'L',50},
					{'C',100}, {'D',500},
					{'M',1000}
				};
				/**
				 * Выполняем заполнение римских чисел для UTF-8
				 */
				this->_wideRomes = {
					{L'I',1}, {L'V',5},
					{L'X',10}, {L'L',50},
					{L'C',100}, {L'D',500},
					{L'M',1000}
				};
				/**
				 * Выполняем заполнение латинских символов
				 */
				this->_letters = {
					{'a',L'a'}, {'b',L'b'},
					{'c',L'c'}, {'d',L'd'},
					{'e',L'e'}, {'f',L'f'},
					{'g',L'g'}, {'h',L'h'},
					{'i',L'i'}, {'j',L'j'},
					{'k',L'k'}, {'l',L'l'},
					{'m',L'm'}, {'n',L'n'},
					{'o',L'o'}, {'p',L'p'},
					{'q',L'q'}, {'r',L'r'},
					{'s',L's'}, {'t',L't'},
					{'u',L'u'}, {'v',L'v'},
					{'w',L'w'}, {'x',L'x'},
					{'y',L'y'}, {'z',L'z'}
				};
				/**
				 * Выполняем заполнение латинских символов для UTF-8
				 */
				this->_wideLetters = {
					{L'a','a'}, {L'b','b'},
					{L'c','c'}, {L'd','d'},
					{L'e','e'}, {L'f','f'},
					{L'g','g'}, {L'h','h'},
					{L'i','i'}, {L'j','j'},
					{L'k','k'}, {L'l','l'},
					{L'm','m'}, {L'n','n'},
					{L'o','o'}, {L'p','p'},
					{L'q','q'}, {L'r','r'},
					{L's','s'}, {L't','t'},
					{L'u','u'}, {L'v','v'},
					{L'w','w'}, {L'x','x'},
					{L'y','y'}, {L'z','z'}
				};
			}
	} symbols;
	/**
	 * @brief Шаблон функции потокового разбора текста на записи ключ-значение
	 *
	 * @tparam T тип символа обрабатываемого текста
	 * @tparam F тип функции обратного вызова
	 *
	 */
	template <typename T, typename F>
	/**
	 * @brief Функция потокового разбора текста на записи ключ-значение
	 *
	 * @details Значение записи может состоять из нескольких слов и содержать разделитель записей: концом значения
	 *          считается последний разделитель, встреченный перед разделителем ключа и значения следующей записи.
	 *          Разбор выполняется одним проходом вперёд, без возврата назад по уже просмотренному тексту.
	 *
	 * @param text      текст из которого извлекаются записи
	 * @param delim     разделитель записей
	 * @param separator разделитель ключа и значения
	 * @param escaping  символы экранирования
	 * @param callback  функция обратного вызова для каждой найденной записи
	 *
	 */
	void kvParse(const basic_string_view <T> text, const basic_string_view <T> delim, const basic_string_view <T> separator, const vector <basic_string <T>> & escaping, F && callback) noexcept {
		// Тип текстового представления обрабатываемых данных
		using view_t = basic_string_view <T>;
		// Получаем размер обрабатываемого текста
		const size_t length = text.size();
		// Символ экранирования отдельного символа
		constexpr T BACKSLASH = static_cast <T> ('\\');
		// Признак отсутствия позиции в тексте
		constexpr size_t NOPOS = view_t::npos;
		/**
		 * @brief Функция проверки соответствия текста в указанной позиции переданной подстроке
		 *
		 * @param pos позиция в тексте с которой начинается проверка
		 * @param str подстрока с которой производится сравнение
		 * @return    результат проверки
		 *
		 */
		auto startsWith = [&text, length](const size_t pos, const view_t str) noexcept -> bool {
			// Выполняем проверку соответствия текста переданной подстроке
			return (!str.empty() && ((pos + str.size()) <= length) && (text.compare(pos, str.size(), str) == 0));
		};
		/**
		 * @brief Функция проверки экранирования символа в указанной позиции
		 *
		 * @details Символ считается экранированным, если ему предшествует нечётное количество обратных слэшей
		 *
		 * @param pos позиция проверяемого символа в тексте
		 * @return    результат проверки
		 *
		 */
		auto shielded = [&text](const size_t pos) noexcept -> bool {
			// Количество обратных слэшей перед проверяемым символом
			size_t count = 0;
			/**
			 * Выполняем подсчёт обратных слэшей перед проверяемым символом
			 */
			while((count < pos) && (text[pos - (count + 1)] == BACKSLASH))
				// Увеличиваем количество найденных обратных слэшей
				count++;
			// Символ экранирован если количество обратных слэшей нечётное
			return ((count % 2) != 0);
		};
		// Позиция начала ключа текущей записи
		size_t keyBegin = 0;
		// Позиция разделителя ключа и значения следующей записи
		size_t pending = NOPOS;
		/**
		 * Выполняем пропуск разделителей записей в начале текста
		 */
		while(startsWith(keyBegin, delim))
			// Выполняем смещение позиции начала ключа
			keyBegin += delim.size();
		/**
		 * Выполняем разбор текста
		 */
		while(keyBegin < length){
			/**
			 * Если разделитель ключа и значения уже найден на предыдущем шаге, то используем его,
			 * иначе выполняем поиск разделителя ключа и значения текущей записи
			 */
			const size_t keyEnd = ((pending != NOPOS) ? pending : text.find(separator, keyBegin));
			// Сбрасываем позицию разделителя следующей записи
			pending = NOPOS;
			// Если разделитель ключа и значения не найден, выходим
			if(keyEnd == NOPOS)
				// Выходим из цикла
				break;
			// Запоминаем позицию начала ключа текущей записи
			const size_t current = keyBegin;
			// Позиция начала значения текущей записи
			size_t valueBegin = (keyEnd + separator.size());
			// Позиция конца значения текущей записи
			size_t valueEnd = length;
			// Выполняем поиск экранирования в начале значения
			const auto i = find_if(escaping.begin(), escaping.end(), [&startsWith, valueBegin](const basic_string <T> & esc) noexcept -> bool {
				// Выполняем проверку начала значения на символ экранирования
				return startsWith(valueBegin, view_t{esc});
			});
			/**
			 * Если значение записи является экранированным
			 */
			if(i != escaping.end()){
				// Получаем символ экранирования значения записи
				const view_t esc{* i};
				// Смещаем начало значения за символ экранирования
				valueBegin += esc.size();
				// Устанавливаем позицию поиска закрывающего символа экранирования
				valueEnd = valueBegin;
				/**
				 * Выполняем поиск закрывающего символа экранирования
				 */
				while((valueEnd = text.find(esc, valueEnd)) != NOPOS){
					// Если найденный символ экранирования не экранирован сам, то это конец значения
					if(!shielded(valueEnd))
						// Выходим из цикла
						break;
					// Продолжаем поиск за экранированным символом
					valueEnd += esc.size();
				}
				/**
				 * Если закрывающий символ экранирования не найден
				 */
				if(valueEnd == NOPOS){
					// Устанавливаем концом значения конец текста
					valueEnd = length;
					// Завершаем разбор текста
					keyBegin = length;
				// Устанавливаем начало следующей записи за закрывающим символом экранирования
				} else keyBegin = (valueEnd + esc.size());
			/**
			 * Если значение записи не является экранированным
			 */
			} else {
				// Текущая позиция просмотра текста
				size_t pos = valueBegin;
				// Позиция последнего встреченного разделителя записей
				size_t lastDelim = NOPOS;
				/**
				 * Выполняем поиск разделителя ключа и значения следующей записи
				 */
				while(pos < length){
					/**
					 * Если найден разделитель записей
					 */
					if(startsWith(pos, delim)){
						// Запоминаем позицию последнего разделителя записей
						lastDelim = pos;
						// Смещаем позицию просмотра за разделитель записей
						pos += delim.size();
						// Продолжаем просмотр текста
						continue;
					}
					/**
					 * Если найден неэкранированный разделитель ключа и значения и перед ним встречался разделитель
					 * записей, то это начало следующей записи, а иначе разделитель принадлежит значению текущей записи
					 */
					if((lastDelim != NOPOS) && startsWith(pos, separator) && !shielded(pos)){
						// Запоминаем позицию разделителя ключа и значения следующей записи
						pending = pos;
						// Выходим из цикла
						break;
					}
					// Смещаем позицию просмотра текста
					pos++;
				}
				/**
				 * Если начало следующей записи найдено
				 */
				if(pending != NOPOS){
					// Устанавливаем концом значения последний разделитель записей
					valueEnd = lastDelim;
					// Устанавливаем начало следующей записи за разделителем записей
					keyBegin = (lastDelim + delim.size());
				// Завершаем разбор текста так как значение занимает весь остаток текста
				} else keyBegin = length;
			}
			// Если запись корректна, выполняем передачу ключа и значения
			if(valueBegin <= valueEnd)
				// Выполняем передачу найденной записи
				callback(text.substr(current, keyEnd - current), text.substr(valueBegin, valueEnd - valueBegin));
			/**
			 * Выполняем пропуск разделителей записей перед началом следующей записи
			 */
			while(startsWith(keyBegin, delim))
				// Выполняем смещение позиции начала ключа
				keyBegin += delim.size();
		}
	}
}

/**
 * @brief Метод генерации уникального идентификатора
 *
 * @return уникальный идентификатор
 *
 */
uint32_t awh::Framework::identifier() const noexcept {
	// Начинаем с 1 (0 можно оставить как "invalid")
	static std::atomic_uint32_t id{1};
	// Получаем следующий идентификатор
	return id.fetch_add(1, std::memory_order_relaxed);
}
/**
 * @brief Метод проверки текста на соответствие флагу
 *
 * @param letter текст для проверки
 * @param flag   флаг проверки
 * @return       результат проверки
 *
 */
bool awh::Framework::is(const char letter, const check_t flag) const noexcept {
	// Переменная результата
	bool result = false;
	// Если буква передана
	if(letter > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Выполняем определение флага проверки
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT):
					// Выполняем проверку символа
					result = ascii::isPrint(letter);
				break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <int32_t> (letter) == ascii::toUpper(letter));
				break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <int32_t> (letter) == ascii::toLower(letter));
				break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE):
					// Выполняем проверку, является ли символ пробелом
					result = ascii::isSpace(letter);
				break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN):
					// Если символ принадлежит к латинскому алфавиту
					result = symbols.isLetter(letter);
				break;
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER):
					// Если символ принадлежит к цифрам
					result = symbols.isArabic(letter);
				break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8):
					// Выполняем проверку симаола на соответствие UTF-8
					result = this->is(string(1, letter), flag);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(letter, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки текста на соответствие флагу
 *
 * @param letter текст для проверки
 * @param flag   флаг проверки
 * @return       результат проверки
 *
 */
bool awh::Framework::is(const wchar_t letter, const check_t flag) const noexcept {
	// Переменная результата
	bool result = false;
	// Если буква передана
	if(letter > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Выполняем определение флага проверки
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT):
					// Выполняем проверку символа
					result = static_cast <bool> (::iswprint(static_cast <wint_t> (letter)));
				break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <wint_t> (letter) == wideUpper(static_cast <wint_t> (letter)));
				break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <wint_t> (letter) == wideLower(static_cast <wint_t> (letter)));
				break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE):
					// Выполняем проверку, является ли символ пробелом
					result = static_cast <bool> (::iswspace(static_cast <wint_t> (letter)));
				break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN):
					// Если символ принадлежит к латинскому алфавиту
					result = symbols.isLetter(letter);
				break;
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER):
					// Если символ принадлежит к цифрам
					result = symbols.isArabic(letter);
				break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8):
					// Выполняем проверку симаола на соответствие UTF-8
					result = this->is(wstring(1, letter), flag);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(letter, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки текста на соответствие флагу
 *
 * @param text текст для проверки
 * @param flag флаг проверки
 * @return     результат проверки
 *
 */
bool awh::Framework::is(string_view text, const check_t flag) const noexcept {
	// Переменная результата
	bool result = false;
	// Выполняем удаление пробелов вокруг представления текста (без копирования)
	{
		// Функция проверки символа на пробельность
		auto isSpace = [](const char letter) noexcept -> bool {
			// Выполняем проверку символа на наличие пробела
			return ascii::isSpace(letter);
		};
		// Получаем границы представления текста
		size_t begin = 0, end = text.size();
		/**
		 * Смещаем начало представления за пробелы
		 */
		while((begin < end) && isSpace(text[begin]))
			// Выполняем смещение начала представления за пробелы
			++begin;
		/**
		 * Смещаем конец представления за пробелы
		 */
		while((end > begin) && isSpace(text[end - 1]))
			// Выполняем смещение конца представления за пробелы
			--end;
		// Обрезаем представление текста
		text = text.substr(begin, end - begin);
	}
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Выполняем определение флага проверки
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг роверки на URL адреса
				case static_cast <uint8_t> (check_t::URL): {
					// Выполняем парсинг nwt адреса
					const auto & url = this->_nwt.parse(text);
					// Если ссылка найдена
					result = (url.type != nwt_t::types_t::NONE);
				} break;
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT): {
					/**
					 * Выполняем перебор всех символов строки
					 */
					for(char letter : text){
						// Выполняем проверку символа
						result = ascii::isPrint(letter);
						// Если символ не печатаемый
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER): {
					/**
					 * Выполняем перебор всего слова
					 */
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <int32_t> (letter) == ascii::toUpper(letter));
						// Если символы не совпадают
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER): {
					/**
					 * Выполняем перебор всего слова
					 */
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <int32_t> (letter) == ascii::toLower(letter));
						// Если символы не совпадают
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE): {
					/**
					 * Выполняем поиск пробела в слове
					 */
					for(auto & letter : text){
						// Выполняем проверку, является ли символ пробелом
						result = ascii::isSpace(letter);
						// Если пробел найден
						if(result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						/**
						 * @brief Функция проверки на валидность символа
						 *
						 * @param text  текст для проверки
						 * @param index индекс буквы в слове
						 * @return      результат проверки
						 *
						 */
						auto checkFn = [this](string_view text, const size_t index) noexcept -> bool {
							// Переменная результата
							bool result = false;
							// Получаем текущую букву
							const char letter = text[index];
							// Если буква не первая и не последняя
							if((index > 0) && (index < (text.size() - 1))){
								// Получаем предыдущую букву
								const char first = text[index - 1];
								// Получаем следующую букву
								const char second = text[index + 1];
								// Если проверка не пройдена, проверяем на апостроф
								if(!(result = (((letter == '-') && (first != '-') && (second != '-')) || ascii::isSpace(letter)))){
									// Выполняем проверку на апостроф
									result = (
										(letter == '\'') && (((first != '\'') && (second != '\'')) ||
										(symbols.isLetter(first) && symbols.isLetter(second)))
									);
								}
								// Если идентификатор обнулился после переполнения счётчика
								if(!result)
									// Печатаем результат проверки
									result = symbols.isLetter(letter);
							// Печатаем результат проверки
							} else result = symbols.isLetter(letter);
							// Возвращаем результат
							return result;
						};
						// Определяем конец текста
						const uint8_t end = ((text.back() == '!') || (text.back() == '?') ? 2 : 1);
						/**
						 * Переходим по всем буквам слова
						 */
						for(size_t i = 0, j = (text.size() - end); j > ((text.size() / 2) - end); i++, j--){
							// Проверяем является ли слово латинским
							result = (i == j ? checkFn(text, i) : checkFn(text, i) && checkFn(text, j));
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ принадлежит к латинскому алфавиту
					} else result = symbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8):
					// Выводим результат проверки правильности записи текста в кодировке UTF-8
					return utf8::valid(text);
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == '-') || (text.front() == '+') ? 1 : 0);
						/**
						 * Переходим по всем буквам слова
						 */
						for(size_t i = static_cast <size_t> (pos), j = (text.size() - 1); j > ((text.size() / 2) - 1); i++, j--){
							// Проверяем является ли слово арабским числом
							result = !(
								(i == j) ?
								!symbols.isArabic(text[i]) :
								!symbols.isArabic(text[i]) ||
								!symbols.isArabic(text[j])
							);
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = symbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки на число с плавающей точкой
				case static_cast <uint8_t> (check_t::DECIMAL): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						// Текущая буква
						char letter = 0;
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == '-') || (text.front() == '+') ? 1 : 0);
						/**
						 * Переходим по всем символам слова
						 */
						for(size_t i = static_cast <size_t> (pos); i < text.size(); i++){
							// Если позиция не первая
							if(i > static_cast <size_t> (pos)){
								// Получаем текущую букву
								letter = text[i];
								// Если плавающая точка найдена
								if((letter == '.') || (letter == ',') || (letter == 'e')){
									// Проверяем правые и левую части
									result = (
										this->is(text.substr(pos, i - pos), check_t::NUMBER) &&
										this->is(text.substr(i + 1), check_t::NUMBER)
									);
									// Выходим из цикла
									break;
								}
							}
						}
					// Если символ всего один, проверяем его так
					} else result = symbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки наличия латинских символов в строке
				case static_cast <uint8_t> (check_t::PRESENCE_LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						/**
						 * Переходим по всем буквам слова
						 */
						for(size_t i = 0, j = (text.size() - 1); j > ((text.size() / 2) - 1); i++, j--){
							// Проверяем является ли слово латинским
							result = (
								(i == j) ?
								symbols.isLetter(text[i]) :
								symbols.isLetter(text[i]) ||
								symbols.isLetter(text[j])
							);
							// Если найдена хотя бы одна латинская буква тогда выходим
							if(result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = symbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на псевдо-число
				case static_cast <uint8_t> (check_t::PSEUDO_NUMBER): {
					// Если не является то проверяем дальше
					if(!(result = this->is(text, check_t::NUMBER))){
						// Проверяем являются ли первая и последняя буква слова, числом
						result = (symbols.isArabic(text.front()) || symbols.isArabic(text.back()));
						// Если оба варианта не сработали
						if(!result && (text.size() > 2)){
							/**
							 * Переходим по всему списку
							 */
							for(size_t i = 1, j = (text.size() - 2); j > ((text.size() / 2) - 1); i++, j--){
								// Проверяем является ли слово арабским числом
								result = (
									(i == j) ?
									symbols.isArabic(text[i]) :
									symbols.isArabic(text[i]) ||
									symbols.isArabic(text[j])
								);
								// Если хоть один символ является числом, выходим
								if(result)
									// Выполняем выход из цикла
									break;
							}
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки текста на соответствие флагу
 *
 * @param text текст для проверки
 * @param flag флаг проверки
 * @return     результат проверки
 *
 */
bool awh::Framework::is(wstring_view text, const check_t flag) const noexcept {
	// Переменная результата
	bool result = false;
	// Выполняем удаление пробелов вокруг представления текста (без копирования)
	{
		// Функция проверки символа на пробельность
		auto isSpace = [](const wchar_t letter) noexcept -> bool {
			// Выполняем проверку символа на наличие пробела
			return (static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
		};
		// Получаем границы представления текста
		size_t begin = 0, end = text.size();
		/**
		 * Смещаем начало представления за пробелы
		 */
		while((begin < end) && isSpace(text[begin]))
			// Выполняем смещение начала представления за пробелы
			++begin;
		/**
		 * Смещаем конец представления за пробелы
		 */
		while((end > begin) && isSpace(text[end - 1]))
			// Выполняем смещение конца представления за пробелы
			--end;
		// Обрезаем представление текста
		text = text.substr(begin, end - begin);
	}
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Выполняем определение флага проверки
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг роверки на URL адреса
				case static_cast <uint8_t> (check_t::URL): {
					// Выполняем парсинг nwt адреса
					const auto & url = this->_nwt.parse(this->convert(text));
					// Если ссылка найдена
					result = (url.type != nwt_t::types_t::NONE);
				} break;
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT): {
					/**
					 * Выполняем перебор всех символов строки
					 */
					for(wchar_t letter : text){
						// Выполняем проверку символа
						result = static_cast <bool> (::iswprint(static_cast <wint_t> (letter)));
						// Если символ не печатаемый
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER): {
					/**
					 * Выполняем перебор всего слова
					 */
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <wint_t> (letter) == wideUpper(static_cast <wint_t> (letter)));
						// Если символы не совпадают
						if(!result)
							// Выполняем выход из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER): {
					/**
					 * Выполняем перебор всего слова
					 */
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <wint_t> (letter) == wideLower(static_cast <wint_t> (letter)));
						// Если символы не совпадают
						if(!result)
							// Выполняем выход из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE): {
					/**
					 * Выполняем поиск пробела в слове
					 */
					for(auto & letter : text){
						// Выполняем проверку, является ли символ пробелом
						result = (static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
						// Если пробел найден
						if(result)
							// Выполняем выход из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						/**
						 * @brief Функция проверки на валидность символа
						 *
						 * @param text  текст для проверки
						 * @param index индекс буквы в слове
						 * @return      результат проверки
						 *
						 */
						auto checkFn = [this](wstring_view text, const size_t index) noexcept -> bool {
							// Переменная результата
							bool result = false;
							// Получаем текущую букву
							const wchar_t letter = text[index];
							// Если буква не первая и не последняя
							if((index > 0) && (index < (text.size() - 1))){
								// Получаем предыдущую букву
								const wchar_t first = text[index - 1];
								// Получаем следующую букву
								const wchar_t second = text[index + 1];
								// Если проверка не пройдена, проверяем на апостроф
								if(!(result = (((letter == L'-') && (first != L'-') && (second != L'-')) || static_cast <bool> (::iswspace(static_cast <wint_t> (letter)))))){
									// Выполняем проверку на апостроф
									result = (
										(letter == L'\'') && (((first != L'\'') && (second != L'\'')) ||
										(symbols.isLetter(first) && symbols.isLetter(second)))
									);
								}
								// Если идентификатор обнулился после переполнения счётчика
								if(!result)
									// Печатаем результат проверки
									result = symbols.isLetter(letter);
							// Печатаем результат проверки
							} else result = symbols.isLetter(letter);
							// Возвращаем результат
							return result;
						};
						// Определяем конец текста
						const uint8_t end = ((text.back() == L'!') || (text.back() == L'?') ? 2 : 1);
						/**
						 * Переходим по всем буквам слова
						 */
						for(size_t i = 0, j = (text.size() - end); j > ((text.size() / 2) - end); i++, j--){
							// Проверяем является ли слово латинским
							result = (i == j ? checkFn(text, i) : checkFn(text, i) && checkFn(text, j));
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ принадлежит к латинскому алфавиту
					} else result = symbols.isLetter(text.front());
				} break;
				/**
				 * Если установлен флаг проверки на соответствие кодировки UTF-8
				 *
				 * @details Текст широких символов проверяется по кодовым значениям его
				 *          символов, взятым байтами записи: так проверка выполнялась и
				 *          прежде. Разрядность широкого символа при этом роли не играет,
				 *          тогда как прежняя проверка читала кодовые значения как байты
				 *          и на разной разрядности вела себя по-разному.
				 *
				 */
				case static_cast <uint8_t> (check_t::UTF8): {
					// Запись текста байтами кодовых значений его символов
					string bytes = "";
					// Выполняем предварительное выделение памяти под запись текста
					bytes.reserve(text.size());
					/**
					 * Выполняем обход символов проверяемого текста
					 */
					for(auto & letter : text) {
						// Получаем кодовое значение очередного символа текста
						const uint32_t code = static_cast <uint32_t> (letter);
						/**
						 * Если кодовое значение символа за пределы байта выходит
						 */
						if(code > 0xFF)
							// Выводим результат проверки правильности записи текста
							return false;
						// Выполняем добавление байта кодового значения символа
						bytes.append(1, static_cast <char> (code));
					}
					// Выводим результат проверки правильности записи текста в кодировке UTF-8
					return utf8::valid(bytes);
				}
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == L'-') || (text.front() == L'+') ? 1 : 0);
						/**
						 * Переходим по всем буквам слова
						 */
						for(size_t i = static_cast <size_t> (pos), j = (text.size() - 1); j > ((text.size() / 2) - 1); i++, j--){
							// Проверяем является ли слово арабским числом
							result = !(
								(i == j) ?
								!symbols.isArabic(text[i]) :
								!symbols.isArabic(text[i]) ||
								!symbols.isArabic(text[j])
							);
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = symbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки на число с плавающей точкой
				case static_cast <uint8_t> (check_t::DECIMAL): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						// Текущая буква
						wchar_t letter = 0;
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == L'-') || (text.front() == L'+') ? 1 : 0);
						/**
						 * Переходим по всем символам слова
						 */
						for(size_t i = static_cast <size_t> (pos); i < text.size(); i++){
							// Если позиция не первая
							if(i > static_cast <size_t> (pos)){
								// Получаем текущую букву
								letter = text[i];
								// Если плавающая точка найдена
								if((letter == L'.') || (letter == L',') || (letter == L'e')){
									// Проверяем правые и левую части
									result = (
										this->is(text.substr(pos, i - pos), check_t::NUMBER) &&
										this->is(text.substr(i + 1), check_t::NUMBER)
									);
									// Выходим из цикла
									break;
								}
							}
						}
					// Если символ всего один, проверяем его так
					} else result = symbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки наличия латинских символов в строке
				case static_cast <uint8_t> (check_t::PRESENCE_LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.size() > 1){
						/**
						 * Переходим по всем буквам слова
						 */
						for(size_t i = 0, j = (text.size() - 1); j > ((text.size() / 2) - 1); i++, j--){
							// Проверяем является ли слово латинским
							result = (
								(i == j) ?
								symbols.isLetter(text[i]) :
								symbols.isLetter(text[i]) ||
								symbols.isLetter(text[j])
							);
							// Если найдена хотя бы одна латинская буква тогда выходим
							if(result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = symbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на псевдо-число
				case static_cast <uint8_t> (check_t::PSEUDO_NUMBER): {
					// Если не является то проверяем дальше
					if(!(result = this->is(text, check_t::NUMBER))){
						// Проверяем являются ли первая и последняя буква слова, числом
						result = (symbols.isArabic(text.front()) || symbols.isArabic(text.back()));
						// Если оба варианта не сработали
						if(!result && (text.size() > 2)){
							/**
							 * Переходим по всему списку
							 */
							for(size_t i = 1, j = (text.size() - 2); j > ((text.size() / 2) - 1); i++, j--){
								// Проверяем является ли слово арабским числом
								result = (
									(i == j) ?
									symbols.isArabic(text[i]) :
									symbols.isArabic(text[i]) ||
									symbols.isArabic(text[j])
								);
								// Если хоть один символ является числом
								if(result)
									// Выполняем выход из цикла
									break;
							}
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(text), static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод сравнения двух строк без учёта регистра
 *
 * @param first  первое слово
 * @param second второе слово
 * @return       результат сравнения
 *
 */
bool awh::Framework::compare(string_view first, string_view second) const noexcept {
	// Если строки пришли не пустыми
	if(!first.empty() && !second.empty()){
		// Если длины строк не совпадают, сравнивать их незачем
		if(first.size() != second.size())
			// Возвращаем результат сравнения
			return false;
		/**
		 * Выполняем перебор обоих строк
		 */
		for(size_t i = 0; i < first.size(); ++i){
			// Выполняем сравнение очередного символа без учёта регистра
			if(!ascii::equals(first[i], second[i]))
				// Возвращаем результат сравнения
				return false;
		}
		// Возвращаем результат сравнения
		return true;
	}
	// Возвращаем значение по умолчанию
	return (first.size() == second.size());
}
/**
 * @brief Метод сравнения двух строк без учёта регистра
 *
 * @param first  первое слово
 * @param second второе слово
 * @return       результат сравнения
 *
 */
bool awh::Framework::compare(const char * first, const char * second) const noexcept {
	// Если данные для сравнения не пришли пустыми
	if((first != nullptr) && ((* first) != '\0') && (second != nullptr) && ((* second) != '\0'))
		// Выполняем перебор обоих строк (через string_view - без выделения памяти под копии строк)
		return this->compare(string_view{first}, string_view{second});
	// Возвращаем значение по умолчанию
	return (first == second);
}
/**
 * @brief Метод сравнения двух строк без учёта регистра
 *
 * @param first  первое слово
 * @param second второе слово
 * @return       результат сравнения
 *
 */
bool awh::Framework::compare(wstring_view first, wstring_view second) const noexcept {
	// Если строки пришли не пустыми
	if(!first.empty() && !second.empty()){
		// Выполняем перебор обоих строк
		return ((first.size() == second.size()) ? std::equal(first.begin(), first.end(), second.begin(), second.end(), [](const wchar_t first, const wchar_t second) noexcept -> bool {
			// Выполняем сравнение каждого символа (при полном совпадении символов приведение регистра не требуется)
			return ((first == second) || (wideLower(static_cast <wint_t> (first)) == wideLower(static_cast <wint_t> (second))));
		}) : false);
	}
	// Возвращаем значение по умолчанию
	return (first.size() == second.size());
}
/**
 * @brief Метод сравнения двух строк без учёта регистра
 *
 * @param first  первое слово
 * @param second второе слово
 * @return       результат сравнения
 *
 */
bool awh::Framework::compare(const wchar_t * first, const wchar_t * second) const noexcept {
	// Если данные для сравшнения не пришли пустыми
	if((first != nullptr) && ((* first) != L'\0') && (second != nullptr) && ((* second) != L'\0'))
		// Выполняем перебор обоих строк (через wstring_view - без выделения памяти под копии строк)
		return this->compare(wstring_view{first}, wstring_view{second});
	// Возвращаем значение по умолчанию
	return (first == second);
}
/**
 * @brief Метод получения штампа времени в указанных единицах измерения
 *
 * @param buffer буфер бинарных данных для установки штампа времени
 * @param size   размер бинарных данных штампа времени
 * @param type   тип формируемого штампа времени
 * @param text   флаг извлечения данных в текстовом виде
 *
 */
void awh::Framework::timestamp(void * buffer, const size_t size, const chrono_t type, const bool text) const noexcept {
	// Если буфер данных передан правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если данные извлекаются в текстовом виде
			if(text){
				/**
				 * Определяем единицы измерения штампа времени
				 */
				switch(static_cast <uint8_t> (type)){
					// Если единицы измерения штампа времени требуется получить в годы
					case static_cast <uint8_t> (chrono_t::YEAR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (hours.count() / static_cast <double> (8760)));
					} break;
					// Если единицы измерения штампа времени требуется получить в месяцах
					case static_cast <uint8_t> (chrono_t::MONTH): {
						// Получаем штамп времени в часы
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (seconds.count() / static_cast <double> (2629746)));
					} break;
					// Если единицы измерения штампа времени требуется получить в неделях
					case static_cast <uint8_t> (chrono_t::WEEK): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (hours.count() / static_cast <double> (168)));
					} break;
					// Если единицы измерения штампа времени требуется получить в днях
					case static_cast <uint8_t> (chrono_t::DAY): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (hours.count() / static_cast <double> (24)));
					} break;
					// Если единицы измерения штампа времени требуется получить в часах
					case static_cast <uint8_t> (chrono_t::HOUR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(hours.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в минутах
					case static_cast <uint8_t> (chrono_t::MINUTES): {
						// Получаем штамп времени в минуты
						chrono::minutes minutes = chrono::duration_cast <chrono::minutes> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(minutes.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в секундах
					case static_cast <uint8_t> (chrono_t::SECONDS): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(seconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в миллисекундах
					case static_cast <uint8_t> (chrono_t::MILLISECONDS): {
						// Получаем штамп времени в миллисекундах
						chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(milliseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в микросекундах
					case static_cast <uint8_t> (chrono_t::MICROSECONDS): {
						// Получаем штамп времени в микросекунды
						chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(microseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в наносекундах
					case static_cast <uint8_t> (chrono_t::NANOSECONDS): {
						// Получаем штамп времени в наносекундах
						chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(nanoseconds.count());
					} break;
				}
			// Если данные извлекаются в виде числа
			} else {
				// Переменная результата
				uint64_t result = 0;
				/**
				 * Определяем единицы измерения штампа времени
				 */
				switch(static_cast <uint8_t> (type)){
					// Если единицы измерения штампа времени требуется получить в годы
					case static_cast <uint8_t> (chrono_t::YEAR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / static_cast <double> (8760));
					} break;
					// Если единицы измерения штампа времени требуется получить в месяцах
					case static_cast <uint8_t> (chrono_t::MONTH): {
						// Получаем штамп времени в часы
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count() / static_cast <double> (2629746));
					} break;
					// Если единицы измерения штампа времени требуется получить в неделях
					case static_cast <uint8_t> (chrono_t::WEEK): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / static_cast <double> (168));
					} break;
					// Если единицы измерения штампа времени требуется получить в днях
					case static_cast <uint8_t> (chrono_t::DAY): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / static_cast <double> (24));
					} break;
					// Если единицы измерения штампа времени требуется получить в часах
					case static_cast <uint8_t> (chrono_t::HOUR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в минутах
					case static_cast <uint8_t> (chrono_t::MINUTES): {
						// Получаем штамп времени в минуты
						chrono::minutes minutes = chrono::duration_cast <chrono::minutes> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (minutes.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в секундах
					case static_cast <uint8_t> (chrono_t::SECONDS): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в миллисекундах
					case static_cast <uint8_t> (chrono_t::MILLISECONDS): {
						// Получаем штамп времени в миллисекундах
						chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (milliseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в микросекундах
					case static_cast <uint8_t> (chrono_t::MICROSECONDS): {
						// Получаем штамп времени в микросекунды
						chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (microseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в наносекундах
					case static_cast <uint8_t> (chrono_t::NANOSECONDS): {
						// Получаем штамп времени в наносекундах
						chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (nanoseconds.count());
					} break;
				}
				/**
				 * Определяем размер буфера данных
				 */
				switch(size){
					// Если размер данных 1 байт
					case 1: {
						// Получаем максимальное число которое содержит буфер
						const uint8_t length = numeric_limits <uint8_t>::max();
						// Если полученный результат помещается в буфер
						if(result <= static_cast <uint64_t> (length))
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &result, size);
						// Если результат не помещается в буфер данных
						else {
							// Получаем размер множителя
							const uint64_t rate = static_cast <uint64_t> (
								::pow(10, ::floor(::log10(static_cast <double> (result)))) /
								::pow(10, ::floor(::log10(static_cast <double> (length))))
							);
							// Получаем итоговый результат для вывода
							const uint8_t data = static_cast <uint8_t> ((result - (result % rate)) / rate);
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &data, size);
						}
					} break;
					// Если размер данных 2 байта
					case 2: {
						// Получаем максимальное число которое содержит буфер
						const uint16_t length = numeric_limits <uint16_t>::max();
						// Если полученный результат помещается в буфер
						if(result <= static_cast <uint64_t> (length))
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &result, size);
						// Если результат не помещается в буфер данных
						else {
							// Получаем размер множителя
							const uint64_t rate = static_cast <uint64_t> (
								::pow(10, ::floor(::log10(static_cast <double> (result)))) /
								::pow(10, ::floor(::log10(static_cast <double> (length))))
							);
							// Получаем итоговый результат для вывода
							const uint16_t data = static_cast <uint16_t> ((result - (result % rate)) / rate);
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &data, size);
						}
					} break;
					// Если размер данных 4 байта
					case 4: {
						// Получаем максимальное число которое содержит буфер
						const uint32_t length = numeric_limits <uint32_t>::max();
						// Если полученный результат помещается в буфер
						if(result <= static_cast <uint64_t> (length))
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &result, size);
						// Если результат не помещается в буфер данных
						else {
							// Получаем размер множителя
							const uint64_t rate = static_cast <uint64_t> (
								::pow(10, ::floor(::log10(static_cast <double> (result)))) /
								::pow(10, ::floor(::log10(static_cast <double> (length))))
							);
							// Получаем итоговый результат для вывода
							const uint32_t data = static_cast <uint32_t> ((result - (result % rate)) / rate);
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &data, size);
						}
					} break;
					// Если размер данных 8 байт
					case 8:
						// Выполняем копирование результата в буфер данных
						::memcpy(buffer, &result, size);
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (type), text), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Шаблон метода получения штампа времени в указанных единицах измерения
 *
 * @tparam T тип данных в котором извлекаются данные
 *
 */
template <typename T>
/**
 * @brief Метод получения штампа времени в указанных единицах измерения
 *
 * @param type тип формируемого штампа времени
 * @return     сгенерированный штамп времени
 *
 */
T awh::Framework::timestamp(const chrono_t type) const noexcept {
	/**
	 * Если штамп времени требуется извлечь дробным числом
	 * @note Извлечение выполняется целым числом с последующим приведением, так как
	 *       разбор по размеру буфера ниже писан для целых видов и записал бы в
	 *       дробное двоичное представление целого, а не само число
	 */
	if constexpr(is_floating_point_v <T>) {
		// Буфер для извлечения штампа времени целым числом
		uint64_t stamp = 0;
		// Выполняем извлечение штампа времени целым числом
		this->timestamp(&stamp, sizeof(stamp), type, false);
		// Выводим штамп времени, приведённый к дробному виду
		return static_cast <T> (stamp);
	}
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Выполняем извлечение данных
	this->timestamp(&result, sizeof(result), type, is_class_v <T>);
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для извлечения значений времени
 */
template int8_t awh::Framework::timestamp <int8_t> (const chrono_t) const noexcept;
template uint8_t awh::Framework::timestamp <uint8_t> (const chrono_t) const noexcept;
template int16_t awh::Framework::timestamp <int16_t> (const chrono_t) const noexcept;
template uint16_t awh::Framework::timestamp <uint16_t> (const chrono_t) const noexcept;
template int32_t awh::Framework::timestamp <int32_t> (const chrono_t) const noexcept;
template uint32_t awh::Framework::timestamp <uint32_t> (const chrono_t) const noexcept;
template int64_t awh::Framework::timestamp <int64_t> (const chrono_t) const noexcept;
template uint64_t awh::Framework::timestamp <uint64_t> (const chrono_t) const noexcept;
template float awh::Framework::timestamp <float> (const chrono_t) const noexcept;
template double awh::Framework::timestamp <double> (const chrono_t) const noexcept;
template string awh::Framework::timestamp <string> (const chrono_t) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template size_t awh::Framework::timestamp <size_t> (const chrono_t) const noexcept;
	template ssize_t awh::Framework::timestamp <ssize_t> (const chrono_t) const noexcept;
#endif
/**
 * @brief Метод конвертирования строки из одной кодировки в другую
 *
 * @param text    текст для конвертирования
 * @param from    кодировка, в которой записан текст
 * @param to      кодировка, в которую требуется сконвертировать текст
 * @param replace порядок обращения с символами, кодировке не представимыми
 * @return        сконвертированный текст в требуемой кодировке
 *
 */
string awh::Framework::transcode(string_view text, const codepage_t from, const codepage_t to, const replace_t replace) const noexcept {
	// Переменная результата
	string result = "";
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Если выполнить конвертирование текста не вышло
			 */
			if(!charset::transcode(text, from, to, result, replace)){
				// Выполняем очистку результата конвертирования
				result.clear();
				// Выполняем формирование текста ошибки
				string message = "Unable to convert text from ";
				// Выполняем добавление имени кодировки, в которой записан текст
				message.append(charset::label(from));
				// Выполняем добавление разделителя имён кодировок
				message.append(" to ");
				// Выполняем добавление имени кодировки, в которую выполнялось конвертирование
				message.append(charset::label(to));
				// Выполняем генерацию ошибки
				throw ::logic_error(message);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, static_cast <uint16_t> (from), static_cast <uint16_t> (to)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод разбора имени кодировки
 *
 * @param name имя кодировки, заданное заголовком протокола
 * @return     обозначение кодировки либо признак нераспознанного имени
 *
 */
awh::Framework::codepage_t awh::Framework::codepage(string_view name) const noexcept {
	// Выводим результат разбора имени кодировки
	return charset::encoding(name);
}
/**
 * @brief Метод извлечения имени кодировки по её обозначению
 *
 * @param codepage обозначение кодировки текста
 * @return         каноническое имя кодировки
 *
 */
string awh::Framework::codepage(const codepage_t codepage) const noexcept {
	// Выводим каноническое имя кодировки
	return string{charset::label(codepage)};
}
/**
 * @brief Метод определения кодировки текста
 *
 * @param text     текст, кодировку которого требуется определить
 * @param fallback кодировка, предполагаемая для текста, записью UTF-8 не являющегося
 * @return         обозначение определённой кодировки текста
 *
 */
awh::Framework::codepage_t awh::Framework::detect(string_view text, const codepage_t fallback) const noexcept {
	// Выводим обозначение определённой кодировки текста
	return charset::detect(text, fallback);
}
/**
 * @brief Метод трансформации одного символа
 *
 * @param letter символ для трансформации
 * @param flag   флаг трансформации
 * @return       трансформированный символ
 *
 */
char awh::Framework::transform(const char letter, const transform_t flag) const noexcept {
	/**
	 * Определяем алгоритм трансформации
	 */
	switch(static_cast <uint8_t> (flag)){
		// Если передан флаг перевода строки в верхний регистр
		case static_cast <uint8_t> (transform_t::UPPER_CASE):
			// Выполняем перевод символа в верхний регистр
			return ascii::toUpper(letter);
		// Если передан флаг перевода строки в нижний регистр
		case static_cast <uint8_t> (transform_t::LOWER_CASE):
			// Выполняем перевод символа в нижний регистр
			return ascii::toLower(letter);
	}
	// Возвращаем результат
	return letter;
}
/**
 * @brief Метод трансформации одного символа
 *
 * @param letter символ для трансформации
 * @param flag   флаг трансформации
 * @return       трансформированный символ
 *
 */
wchar_t awh::Framework::transform(const wchar_t letter, const transform_t flag) const noexcept {
	/**
	 * Определяем алгоритм трансформации
	 */
	switch(static_cast <uint8_t> (flag)){
		// Если передан флаг перевода строки в верхний регистр
		case static_cast <uint8_t> (transform_t::UPPER_CASE):
			// Выполняем перевод символа в верхний регистр
			return static_cast <wchar_t> (wideUpper(static_cast <wint_t> (letter)));
		// Если передан флаг перевода строки в нижний регистр
		case static_cast <uint8_t> (transform_t::LOWER_CASE):
			// Выполняем перевод символа в нижний регистр
			return static_cast <wchar_t> (wideLower(static_cast <wint_t> (letter)));
	}
	// Возвращаем результат
	return letter;
}
/**
 * @brief Метод трансформации строки
 *
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 *
 */
string & awh::Framework::transform(string & text, const transform_t flag) const noexcept {
	// Если текст для обработки передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем алгоритм трансформации
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если передан флаг удаления пробелов
				case static_cast <uint8_t> (transform_t::TRIM): {
					// Выполняем удаление пробелов в начале текста
					text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](const char letter) -> bool {
						// Выполняем проверку символа на наличие пробела
						return ascii::isSpace(letter);
					}));
					// Выполняем удаление пробелов в конце текста
					text.erase(find_if_not(text.rbegin(), text.rend(), [](const char letter) -> bool {
						// Выполняем проверку символа на наличие пробела
						return ascii::isSpace(letter);
					}).base(), text.end());
				} break;
				// Если передан флаг перевода строки в верхний регистр
				case static_cast <uint8_t> (transform_t::UPPER_CASE): {
					// Выполняем приведение к верхнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](const char letter) -> char {
						// Приводим к верхнему регистру каждую букву
						return ascii::toUpper(letter);
					});
				} break;
				// Если передан флаг перевода строки в нижний регистр
				case static_cast <uint8_t> (transform_t::LOWER_CASE): {
					// Выполняем приведение к нижнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](const char letter) -> char {
						// Приводим к нижнему регистру каждую букву
						return ascii::toLower(letter);
					});
				} break;
				// Если передан флаг умного перевода начальных символов в верхний регистр
				case static_cast <uint8_t> (transform_t::SMART_CASE): {
					// Символ с которым ведётся работа в данный момент
					char letter = 0;
					// Флаг детекции символа
					bool mode = true;
					/**
					 * Переходим по всем буквам слова и формируем новую строку
					 */
					for(size_t i = 0; i < text.length(); i++){
						// Получаем символ с которым ведётся работа в данный момент
						letter = text[i];
						// Если флаг перевода в верхний регистр активирован
						if(mode)
							// Переводим символ в верхний режим
							text[i] = ascii::toUpper(letter);
						// Переводим остальные символы в нижний регистр
						else text[i] = ascii::toLower(letter);
						// Если найден спецсимвол, устанавливаем флаг детекции
						mode = ((letter == '-') || (letter == '_') || ascii::isSpace(letter));
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return text;
}
/**
 * @brief Метод трансформации строки
 *
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 *
 */
wstring & awh::Framework::transform(wstring & text, const transform_t flag) const noexcept {
	// Если текст для обработки передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем алгоритм трансформации
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если передан флаг удаления пробелов
				case static_cast <uint8_t> (transform_t::TRIM): {
					// Выполняем удаление пробелов в начале текста
					text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](const wchar_t letter) -> bool {
						// Выполняем проверку символа на наличие пробела
						return (static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
					}));
					// Выполняем удаление пробелов в конце текста
					text.erase(find_if_not(text.rbegin(), text.rend(), [](const wchar_t letter) -> bool {
						// Выполняем проверку символа на наличие пробела
						return (static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
					}).base(), text.end());
				} break;
				// Если передан флаг перевода строки в верхний регистр
				case static_cast <uint8_t> (transform_t::UPPER_CASE): {
					// Выполняем приведение к верхнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](const wchar_t letter) -> wchar_t {
						// Приводим к верхнему регистру каждую букву
						return static_cast <wchar_t> (wideUpper(static_cast <wint_t> (letter)));
					});
				} break;
				// Если передан флаг перевода строки в нижний регистр
				case static_cast <uint8_t> (transform_t::LOWER_CASE): {
					// Выполняем приведение к нижнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](const wchar_t letter) -> wchar_t {
						// Приводим к нижнему регистру каждую букву
						return static_cast <wchar_t> (wideLower(static_cast <wint_t> (letter)));
					});
				} break;
				// Если передан флаг умного перевода начальных символов в верхний регистр
				case static_cast <uint8_t> (transform_t::SMART_CASE): {
					// Флаг детекции символа
					bool mode = true;
					// Символ с которым ведётся работа в данный момент
					wchar_t letter = 0;
					/**
					 * Переходим по всем буквам слова и формируем новую строку
					 */
					for(size_t i = 0; i < text.length(); i++){
						// Получаем символ с которым ведётся работа в данный момент
						letter = text[i];
						// Если флаг перевода в верхний регистр активирован
						if(mode)
							// Переводим символ в верхний режим
							text[i] = static_cast <wchar_t> (wideUpper(static_cast <wint_t> (letter)));
						// Переводим остальные символы в нижний регистр
						else text[i] = static_cast <wchar_t> (wideLower(static_cast <wint_t> (letter)));
						// Если найден спецсимвол, устанавливаем флаг детекции
						mode = ((letter == L'-') || (letter == L'_') || static_cast <bool> (::iswspace(static_cast <wint_t> (letter))) || (letter == 160) || (letter == 173));
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(text), static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return text;
}
/**
 * @brief Метод трансформации строки
 *
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 *
 */
const string & awh::Framework::transform(const string & text, const transform_t flag) const noexcept {
	// Выполняем трансформацию текста
	return this->transform(* const_cast <string *> (&text), flag);
}
/**
 * @brief Метод трансформации строки
 *
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 *
 */
const wstring & awh::Framework::transform(const wstring & text, const transform_t flag) const noexcept {
	// Выполняем трансформацию текста
	return this->transform(* const_cast <wstring *> (&text), flag);
}
/**
 * @brief Метод трансформации строки
 *
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 *
 */
string awh::Framework::transform(string_view text, const transform_t flag) const noexcept {
	// Выполняем трансформацию текста
	return this->transform(string{text}, flag);
}
/**
 * @brief Метод трансформации строки
 *
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 *
 */
wstring awh::Framework::transform(wstring_view text, const transform_t flag) const noexcept {
	// Выполняем трансформацию текста
	return this->transform(wstring{text}, flag);
}
/**
 * @brief Метод объединения списка строк в одну строку
 *
 * @param items список строк которые необходимо объединить
 * @param delim разделитель
 * @return      строка полученная после объединения
 *
 */
string awh::Framework::join(const vector <string> & items, string_view delim) const noexcept {
	// Переменная результата
	string result = "";
	// Если список строк которые необходимо объединить переданы
	if(!items.empty()){
		/**
		 * Выполняем перебор всего списка строк
		 */
		for(auto & item : items){
			// Если результат ещё не сформирован
			if(!result.empty())
				// Выполняем добавление разделителя
				result.append(delim.data(), delim.size());
			// Выполняем добавление текущей строки
			result.append(item);
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод объединения списка строк в одну строку
 *
 * @param items список строк которые необходимо объединить
 * @param delim разделитель
 * @return      строка полученная после объединения
 *
 */
wstring awh::Framework::join(const vector <wstring> & items, wstring_view delim) const noexcept {
	// Переменная результата
	wstring result = L"";
	// Если список строк которые необходимо объединить переданы
	if(!items.empty()){
		/**
		 * Выполняем перебор всего списка строк
		 */
		for(auto & item : items){
			// Если результат ещё не сформирован
			if(!result.empty())
				// Выполняем добавление разделителя
				result.append(delim.data(), delim.size());
			// Выполняем добавление текущей строки
			result.append(item);
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод разделения строк на токены
 *
 * @param text      строка для парсинга
 * @param delim     разделитель
 * @param container результирующий вектор
 *
 */
vector <string> & awh::Framework::split(string_view text, string_view delim, vector <string> & container) const noexcept {
	// Выполняем сплит текста
	return ::split(text, delim, container, this->_log);
}
/**
 * @brief Метод разделения строк на токены
 *
 * @param text      строка для парсинга
 * @param delim     разделитель
 * @param container результирующий вектор
 *
 */
vector <wstring> & awh::Framework::split(wstring_view text, wstring_view delim, vector <wstring> & container) const noexcept {
	// Выполняем сплит текста
	return ::split(text, delim, container, this->_log);
}
/**
 * @brief Метод конвертирования строки в строку utf-8
 *
 * @param str строка для конвертирования
 * @return    строка в utf-8
 *
 */
wstring awh::Framework::convert(string_view str) const noexcept {
	// Переменная результата
	wstring result = L"";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <wchar_t> (str.data(), str.data() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Выполняем конвертирование строки UTF-8 в широкую строку
				result = ::utf8ToWide(str.data(), str.size());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод конвертирования строки utf-8 в строку
 *
 * @param str строка utf-8 для конвертирования
 * @return    обычная строка
 *
 */
string awh::Framework::convert(wstring_view str) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <char> (str.data(), str.data() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Выполняем конвертирование широкой строки в строку UTF-8
				result = ::wideToUtf8(str.data(), str.size());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод конвертирования строки в строку utf-8
 *
 * @param str строка для конвертирования
 * @return    строка в utf-8
 *
 */
wstring awh::Framework::convert(const char * str) const noexcept {
	// Переменная результата
	wstring result = L"";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if((str != nullptr) && ((* str) != '\0')){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <wchar_t> (str, str + ::strlen(str));
			// Если нужно использовать стандартную библиотеку
			#else
				// Выполняем конвертирование строки UTF-8 в широкую строку
				result = ::utf8ToWide(str, ::strlen(str));
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод конвертирования строки utf-8 в строку
 *
 * @param str строка utf-8 для конвертирования
 * @return    обычная строка
 *
 */
string awh::Framework::convert(const wchar_t * str) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if((str != nullptr) && ((* str) != L'\0')){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <char> (str, str + ::wcslen(str));
			// Если нужно использовать стандартную библиотеку
			#else
				// Выполняем конвертирование широкой строки в строку UTF-8
				result = ::wideToUtf8(str, ::wcslen(str));
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод конвертирования строки в строку utf-8
 *
 * @param str строка для конвертирования
 * @return    строка в utf-8
 *
 */
wstring awh::Framework::convert(const string & str) const noexcept {
	// Переменная результата
	wstring result = L"";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <wchar_t> (str.c_str(), str.c_str() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Выполняем конвертирование строки UTF-8 в широкую строку
				result = ::utf8ToWide(str.data(), str.size());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод конвертирования строки utf-8 в строку
 *
 * @param str строка utf-8 для конвертирования
 * @return    обычная строка
 *
 */
string awh::Framework::convert(const wstring & str) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <char> (str.c_str(), str.c_str() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Выполняем конвертирование широкой строки в строку UTF-8
				result = ::wideToUtf8(str.data(), str.size());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief функции определения точного размера, сколько занимает число байт
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод определения точного размера, сколько занимает число байт
 *
 * @param num число для проверки
 * @return    фактический размер занимаемым числом байт
 *
 */
size_t awh::Framework::size(const T num) const noexcept {
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value)
		// Выполняем подсчёт занимаемых числом данных
		return this->size(&num, sizeof(num));
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * Объявляем прототипы для извлечения точного размера числа байт
 */
template size_t awh::Framework::size <int8_t> (const int8_t) const noexcept;
template size_t awh::Framework::size <uint8_t> (const uint8_t) const noexcept;
template size_t awh::Framework::size <int16_t> (const int16_t) const noexcept;
template size_t awh::Framework::size <uint16_t> (const uint16_t) const noexcept;
template size_t awh::Framework::size <int32_t> (const int32_t) const noexcept;
template size_t awh::Framework::size <uint32_t> (const uint32_t) const noexcept;
template size_t awh::Framework::size <int64_t> (const int64_t) const noexcept;
template size_t awh::Framework::size <uint64_t> (const uint64_t) const noexcept;
template size_t awh::Framework::size <float> (const float) const noexcept;
template size_t awh::Framework::size <double> (const double) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template size_t awh::Framework::size <size_t> (const size_t) const noexcept;
	template size_t awh::Framework::size <ssize_t> (const ssize_t) const noexcept;
#endif
/**
 * @brief Метод определения точного размера, сколько занимают данные (в байтах) в буфере
 *
 * @param value значение бинарного буфера для проверки
 * @param size  общий размер бинарного буфера
 * @return      фактический размер буфера занимаемый данными
 *
 */
size_t awh::Framework::size(const void * value, const size_t size) const noexcept {
	// Переменная результата
	size_t result = 0;
	// Если значение бинарного буфера передано верное
	if((value != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Значение байта с которым будем работать
			uint8_t byte = 0;
			// Получаем общее количество байт буфера
			size_t index = size;
			/**
			 * Выполняем перебор всех байт буфера
			 */
			while(index--){
				// Выполняем получениетекущего байта
				byte = reinterpret_cast <const uint8_t *> (value)[index];
				// Если байты нулевые
				if(byte == 0)
					// Увеличиваем значение результата
					result++;
				// Если байты не нулевые, выходим из цикла
				else break;
			}
			// Формируем окончательный результат
			result = (size - result);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Шаблон функции проверки больше первое число второго или нет (бинарным методом)
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод проверки больше первое число второго или нет (бинарным методом)
 *
 * @param num1 значение первого числа в бинарном виде
 * @param num2 значение второго числа в бинарном виде
 * @return     результат проверки
 *
 */
bool awh::Framework::isGreater(const T num1, const T num2) const noexcept {
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value)
		// Выполняем проверку
		return this->isGreater(&num1, &num2, sizeof(num1));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * Объявляем прототипы для сравнения больших чисел без ограничения
 */
template bool awh::Framework::isGreater <int8_t> (const int8_t, const int8_t) const noexcept;
template bool awh::Framework::isGreater <uint8_t> (const uint8_t, const uint8_t) const noexcept;
template bool awh::Framework::isGreater <int16_t> (const int16_t, const int16_t) const noexcept;
template bool awh::Framework::isGreater <uint16_t> (const uint16_t, const uint16_t) const noexcept;
template bool awh::Framework::isGreater <int32_t> (const int32_t, const int32_t) const noexcept;
template bool awh::Framework::isGreater <uint32_t> (const uint32_t, const uint32_t) const noexcept;
template bool awh::Framework::isGreater <int64_t> (const int64_t, const int64_t) const noexcept;
template bool awh::Framework::isGreater <uint64_t> (const uint64_t, const uint64_t) const noexcept;
template bool awh::Framework::isGreater <float> (const float, const float) const noexcept;
template bool awh::Framework::isGreater <double> (const double, const double) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template bool awh::Framework::isGreater <size_t> (const size_t, const size_t) const noexcept;
	template bool awh::Framework::isGreater <ssize_t> (const ssize_t, const ssize_t) const noexcept;
#endif
/**
 * @brief Метод проверки больше первое число второго или нет (бинарным методом)
 *
 * @param value1 значение первого числа в бинарном виде
 * @param value2 значение второго числа в бинарном виде
 * @param size   размер бинарного буфера числа
 * @return       результат проверки
 *
 */
bool awh::Framework::isGreater(const void * value1, const void * value2, const size_t size) const noexcept {
	// Переменная результата
	bool result = false;
	// Если данные переданы правильно
	if((value1 != nullptr) && (value2 != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Значений чисел для сравнения
			std::bitset <8> num1(0), num2(0);
			// Индекс перебора всех бит числа
			size_t count = 0, index = size;
			/**
			 * Выполняем перебор всех байт буфера
			 */
			while(index--){
				// Получаем значение числа в виде первого байта
				num1 = reinterpret_cast <const uint8_t *> (value1)[index];
				// Получаем значение числа в виде второго байта
				num2 = reinterpret_cast <const uint8_t *> (value2)[index];
				// Получаем первоначальное значение индексов
				count = num1.size();
				/**
				 * Выполняем перебор всей строки
				 */
				while(count--){
					// Если первый байт больше второго
					if((result = (num1.test(count) && !num2.test(count))) || (!num1.test(count) && num2.test(count)))
						// Выходим из функции
						return result;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value1, value2, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Шаблон функции конвертации чисел в указанную систему счисления
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод конвертации чисел в указанную систему счисления
 *
 * @param value число для конвертации
 * @param radix система счисления
 * @return      полученная строка в указанной системе счисления
 *
 */
string awh::Framework::itoa(const T value, const uint8_t radix) const noexcept {
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value)
		// Выполняем конвертацию чисел в указанную систему счисления
		return this->itoa(&value, sizeof(value), radix);
	// Возвращаем пустое значение
	return "";
}
/**
 * Объявляем прототипы для метода конвертации чисел в указанную систему счисления
 */
template string awh::Framework::itoa <int8_t> (const int8_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <uint8_t> (const uint8_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <int16_t> (const int16_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <uint16_t> (const uint16_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <int32_t> (const int32_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <uint32_t> (const uint32_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <int64_t> (const int64_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <uint64_t> (const uint64_t, const uint8_t) const noexcept;
template string awh::Framework::itoa <float> (const float, const uint8_t) const noexcept;
template string awh::Framework::itoa <double> (const double, const uint8_t) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template string awh::Framework::itoa <size_t> (const size_t, const uint8_t) const noexcept;
	template string awh::Framework::itoa <ssize_t> (const ssize_t, const uint8_t) const noexcept;
#endif
/**
 * @brief Метод конвертации чисел в указанную систему счисления
 *
 * @param value бинарный буфер числа для конвертации
 * @param size  размер бинарного буфера
 * @param radix система счисления
 * @return      полученная строка в указанной системе счисления
 *
 */
string awh::Framework::itoa(const void * value, const size_t size, const uint8_t radix) const noexcept {
	// Переменная результата
	string result = "";
	// Если данные переданы
	if((value != nullptr) && (size > 0) && (radix > 1) && (radix < 37)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем числовые обозначения
			const string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			// Если запись в бинарном виде
			if(radix == 2){
				// Результат с которым будем работать
				std::bitset <8> byte(0);
				/**
				 * Выполняем перебор всего буфера данных
				 */
				for(size_t i = 0; i < size; i++){
					// Получаем байт
					byte = reinterpret_cast <const uint8_t *> (value)[i];
					/**
					 * Переходим по всем байтам полученного бита
					 */
					for(size_t j = 0; j < byte.size(); j++){
						// Если бит установлен
						if(byte.test(j))
							// Выполняем добавление первого символа символа
							result.push_back(digits[1]);
						// Иначе добавлям нулевой символ
						else result.push_back(digits[0]);
					}
				}
			// Если это другая система счисления
			} else {
				/**
				 * Определяем размер данных для конвертации
				 */
				switch(size){
					// Если это один байт
					case 1: {
						// Число с которым будем работать
						uint8_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.push_back(digits[0]);
						/**
						 * Раскладываем число на цифры (младшими разрядами вперёд)
						 */
						while(num != 0){
							// Добавляем идентификатор числа
							result.push_back(digits[num % radix]);
							// Выполняем финальное деление
							num /= radix;
						}
					} break;
					// Если это два байта
					case 2: {
						// Число с которым будем работать
						uint16_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.push_back(digits[0]);
						/**
						 * Раскладываем число на цифры (младшими разрядами вперёд)
						 */
						while(num != 0){
							// Добавляем идентификатор числа
							result.push_back(digits[num % static_cast <uint16_t> (radix)]);
							// Выполняем финальное деление
							num /= static_cast <uint16_t> (radix);
						}
					} break;
					// Если это четыре байта
					case 4: {
						// Число с которым будем работать
						uint32_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.push_back(digits[0]);
						/**
						 * Раскладываем число на цифры (младшими разрядами вперёд)
						 */
						while(num != 0){
							// Добавляем идентификатор числа
							result.push_back(digits[num % static_cast <uint32_t> (radix)]);
							// Выполняем финальное деление
							num /= static_cast <uint32_t> (radix);
						}
					} break;
					// Если это восемь байт
					case 8: {
						// Число с которым будем работать
						uint64_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.push_back(digits[0]);
						/**
						 * Раскладываем число на цифры (младшими разрядами вперёд)
						 */
						while(num != 0){
							// Добавляем идентификатор числа
							result.push_back(digits[num % static_cast <uint64_t> (radix)]);
							// Выполняем финальное деление
							num /= static_cast <uint64_t> (radix);
						}
					} break;
					// Для всех остальных размеров
					default: {
						// Сбрасываем полученный результат
						result.clear();
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "Binary data buffer cannot be cast to a number");
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							::fprintf(stderr, "ERROR! %s\n\n", "Binary data buffer cannot be cast to a number");
						#endif
					}
				}
			}
			// Цифры формировались младшими разрядами вперёд, поэтому разворачиваем результат в правильный порядок
			std::reverse(result.begin(), result.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			result.clear();
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value, size, static_cast <uint16_t> (radix)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод конвертации строковых чисел в десятичную систему счисления
 *
 * @param value строковое представление числа
 * @return      числовое значение в десятичной системе счисления
 *
 */
T awh::Framework::atoi(string_view value) const noexcept {
	// Переменная результата
	T result = T();
	// Если мы получили на вход перечисление
	if constexpr (is_enum_v <T>){
		// Если строка для конвертации не пуста
		if(!value.empty()){
			// Результат конвертации в базовом типе перечисления
			underlying_type_t <T> number = 0;
			// Вызываем метод конвертации
			auto answer = lexical_t::fromChars(value.data(), value.data() + value.size(), number);
			// Если ошибок нет и строка разобрана полностью (в конце не осталось мусора)
			if((answer.ec == std::errc()) && (answer.ptr == (value.data() + value.size())))
				// Выполняем приведение результата к типу перечисления
				result = static_cast <T> (number);
		}
	// Если мы получили на вход число
	} else if constexpr (is_arithmetic_v <T>){
		// Возвращаем значение по умолчанию
		result = static_cast <T> (0);
		// Если строка для конвертации не пуста
		if(!value.empty()){
			// Вызываем метод конвертации
			auto answer = lexical_t::fromChars(value.data(), value.data() + value.size(), result);
			// Если мы получили ошибку или строка разобрана не полностью (в конце остался мусор)
			if((answer.ec != std::errc()) || (answer.ptr != (value.data() + value.size())))
				// Возвращаем значение по умолчанию
				result = static_cast <T> (0);
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода конвертации строковых чисел в десятичную систему счисления
 */
template int8_t awh::Framework::atoi <int8_t> (string_view) const noexcept;
template uint8_t awh::Framework::atoi <uint8_t> (string_view) const noexcept;
template int16_t awh::Framework::atoi <int16_t> (string_view) const noexcept;
template uint16_t awh::Framework::atoi <uint16_t> (string_view) const noexcept;
template int32_t awh::Framework::atoi <int32_t> (string_view) const noexcept;
template uint32_t awh::Framework::atoi <uint32_t> (string_view) const noexcept;
template int64_t awh::Framework::atoi <int64_t> (string_view) const noexcept;
template uint64_t awh::Framework::atoi <uint64_t> (string_view) const noexcept;
template float awh::Framework::atoi <float> (string_view) const noexcept;
template double awh::Framework::atoi <double> (string_view) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template size_t awh::Framework::atoi <size_t> (string_view) const noexcept;
	template ssize_t awh::Framework::atoi <ssize_t> (string_view) const noexcept;
#endif
/**
 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод конвертации строковых чисел в десятичную систему счисления
 *
 * @param value число в бинарном виде для конвертации в 10-ю систему
 * @param radix система счисления
 * @return      полученное значение в десятичной системе счисления
 *
 */
T awh::Framework::atoi(string_view value, const uint8_t radix) const noexcept {
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Выполняем извлечение данных
	this->atoi(value, radix, &result, sizeof(result));
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода конвертации строковых чисел в десятичную систему счисления
 */
template int8_t awh::Framework::atoi <int8_t> (string_view, const uint8_t) const noexcept;
template uint8_t awh::Framework::atoi <uint8_t> (string_view, const uint8_t) const noexcept;
template int16_t awh::Framework::atoi <int16_t> (string_view, const uint8_t) const noexcept;
template uint16_t awh::Framework::atoi <uint16_t> (string_view, const uint8_t) const noexcept;
template int32_t awh::Framework::atoi <int32_t> (string_view, const uint8_t) const noexcept;
template uint32_t awh::Framework::atoi <uint32_t> (string_view, const uint8_t) const noexcept;
template int64_t awh::Framework::atoi <int64_t> (string_view, const uint8_t) const noexcept;
template uint64_t awh::Framework::atoi <uint64_t> (string_view, const uint8_t) const noexcept;
template float awh::Framework::atoi <float> (string_view, const uint8_t) const noexcept;
template double awh::Framework::atoi <double> (string_view, const uint8_t) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template size_t awh::Framework::atoi <size_t> (string_view, const uint8_t) const noexcept;
	template ssize_t awh::Framework::atoi <ssize_t> (string_view, const uint8_t) const noexcept;
#endif
/**
 * @brief Метод конвертации строковых чисел в десятичную систему счисления
 *
 * @param value  число в бинарном виде для конвертации в 10-ю систему
 * @param radix  система счисления
 * @param buffer бинарный буфер куда следует положить результат
 * @param size   размер бинарного буфера куда следует положить результат
 *
 */
void awh::Framework::atoi(string_view value, const uint8_t radix, void * buffer, const size_t size) const noexcept {
	// Если данные для конвертации переданы
	if(!value.empty() && (radix > 1) && (radix < 37) && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перевод в верхний регистр
			string number(value);
			// Позиция в строке алфавита
			size_t pos = string::npos;
			// Устанавливаем числовые обозначения
			const string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			// Если запись в 16-м виде
			if(radix == 16){
				// Если первые два значения числа являются префиксом
				if(number.compare(0, 2, "0x") == 0)
					// Удаляем первые два символа
					number.erase(0, 2);
			}
			// Выполняем перевод число в верхний регистр
			this->transform(number, transform_t::UPPER_CASE);
			// Количество перебираемых элементов
			const uint8_t count = static_cast <uint8_t> (number.length());
			/**
			 * Определяем размер данных для конвертации
			 */
			switch(size){
				// Если это один байт
				case 1: {
					// Результат с которым будем работать
					uint8_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint8_t> (result * radix + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это два байта
				case 2: {
					// Результат с которым будем работать
					uint16_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint16_t> (result * static_cast <uint16_t> (radix) + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это четыре байта
				case 4: {
					// Результат с которым будем работать
					uint32_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint32_t> (result * static_cast <uint32_t> (radix) + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это восемь байт
				case 8: {
					// Результат с которым будем работать
					uint64_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint64_t> (result * static_cast <uint64_t> (radix) + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Для всех остальных размеров
				default: {
					// Если запись в бинарном виде
					if(radix == 2){
						// Значение байта для установки
						uint8_t byte = 0;
						// Результат с которым будем работать
						std::bitset <8> result(0);
						// Получаем первоначальное значение индексов
						size_t i = value.size(), j = 0, offset = 0;
						/**
						 * Выполняем перебор всей строки
						 */
						while(i--){
							// Если бит положительный
							if(value[i] == '1')
								// Устанавливаем бит результата
								result.set(j);
							// Если бит отрицательный, снимаем его
							else result.reset(j);
							// Увеличиваем смещение бит
							j++;
							// Если мы заполнили байт целиком
							if((j % 8) == 0){
								// Сбрасываем значение счётчика
								j = 0;
								// Выполняем получение числа
								byte = static_cast <uint8_t> (result.to_ulong());
								// Выполняем добавление байта в буфер
								::memcpy(reinterpret_cast <uint8_t *> (buffer) + (offset / 8), &byte, sizeof(byte));
								// Увеличиваем смещение в буфере
								offset += 8;
								// Сбрасываем результат
								result.reset();
							}
						}
					// Записываем ошибку в лог
					} else {
						// Сбрасываем полученный результат
						::memset(buffer, 0, size);
						// Если объект логирования установлен
						if(this->_log != nullptr){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value, static_cast <uint16_t> (radix), buffer, size), log_t::flag_t::CRITICAL, "Only binary number can be converted to binary buffer");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, "Only binary number can be converted to binary buffer");
							#endif
						// Если объект логирования не установлен
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "Only binary number can be converted to binary buffer");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								::fprintf(stderr, "ERROR! %s\n\n", "Only binary number can be converted to binary buffer");
							#endif
						}
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			::memset(buffer, 0, size);
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value, static_cast <uint16_t> (radix), buffer, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод конвертации строковых чисел в десятичную систему счисления
 *
 * @param value строковое представление числа
 * @return      числовое значение в десятичной системе счисления
 *
 */
T awh::Framework::atoi(wstring_view value) const noexcept {
	// Переменная результата
	T result = T();
	// Если мы получили на вход перечисление
	if constexpr (is_enum_v <T>){
		// Если строка для конвертации не пуста
		if(!value.empty()){
			// Результат конвертации в базовом типе перечисления
			underlying_type_t <T> number = 0;
			// Вызываем метод конвертации
			auto answer = lexical_t::fromChars(value.data(), value.data() + value.size(), number);
			// Если ошибок нет и строка разобрана полностью (в конце не осталось мусора)
			if((answer.ec == std::errc()) && (answer.ptr == (value.data() + value.size())))
				// Выполняем приведение результата к типу перечисления
				result = static_cast <T> (number);
		}
	// Если мы получили на вход число
	} else if constexpr (is_arithmetic_v <T>){
		// Возвращаем значение по умолчанию
		result = static_cast <T> (0);
		// Если строка для конвертации не пуста
		if(!value.empty()){
			// Вызываем метод конвертации
			auto answer = lexical_t::fromChars(value.data(), value.data() + value.size(), result);
			// Если мы получили ошибку или строка разобрана не полностью (в конце остался мусор)
			if((answer.ec != std::errc()) || (answer.ptr != (value.data() + value.size())))
				// Возвращаем значение по умолчанию
				result = static_cast <T> (0);
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода конвертации строковых чисел в десятичную систему счисления
 */
template int8_t awh::Framework::atoi <int8_t> (wstring_view) const noexcept;
template uint8_t awh::Framework::atoi <uint8_t> (wstring_view) const noexcept;
template int16_t awh::Framework::atoi <int16_t> (wstring_view) const noexcept;
template uint16_t awh::Framework::atoi <uint16_t> (wstring_view) const noexcept;
template int32_t awh::Framework::atoi <int32_t> (wstring_view) const noexcept;
template uint32_t awh::Framework::atoi <uint32_t> (wstring_view) const noexcept;
template int64_t awh::Framework::atoi <int64_t> (wstring_view) const noexcept;
template uint64_t awh::Framework::atoi <uint64_t> (wstring_view) const noexcept;
template float awh::Framework::atoi <float> (wstring_view) const noexcept;
template double awh::Framework::atoi <double> (wstring_view) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template size_t awh::Framework::atoi <size_t> (wstring_view) const noexcept;
	template ssize_t awh::Framework::atoi <ssize_t> (wstring_view) const noexcept;
#endif
/**
 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод конвертации строковых чисел в десятичную систему счисления
 *
 * @param value число в бинарном виде для конвертации в 10-ю систему
 * @param radix система счисления
 * @return      полученное значение в десятичной системе счисления
 *
 */
T awh::Framework::atoi(wstring_view value, const uint8_t radix) const noexcept {
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Выполняем извлечение данных
	this->atoi(value, radix, &result, sizeof(result));
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода конвертации строковых чисел в десятичную систему счисления
 */
template int8_t awh::Framework::atoi <int8_t> (wstring_view, const uint8_t) const noexcept;
template uint8_t awh::Framework::atoi <uint8_t> (wstring_view, const uint8_t) const noexcept;
template int16_t awh::Framework::atoi <int16_t> (wstring_view, const uint8_t) const noexcept;
template uint16_t awh::Framework::atoi <uint16_t> (wstring_view, const uint8_t) const noexcept;
template int32_t awh::Framework::atoi <int32_t> (wstring_view, const uint8_t) const noexcept;
template uint32_t awh::Framework::atoi <uint32_t> (wstring_view, const uint8_t) const noexcept;
template int64_t awh::Framework::atoi <int64_t> (wstring_view, const uint8_t) const noexcept;
template uint64_t awh::Framework::atoi <uint64_t> (wstring_view, const uint8_t) const noexcept;
template float awh::Framework::atoi <float> (wstring_view, const uint8_t) const noexcept;
template double awh::Framework::atoi <double> (wstring_view, const uint8_t) const noexcept;
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template size_t awh::Framework::atoi <size_t> (wstring_view, const uint8_t) const noexcept;
	template ssize_t awh::Framework::atoi <ssize_t> (wstring_view, const uint8_t) const noexcept;
#endif
/**
 * @brief Метод конвертации строковых чисел в десятичную систему счисления
 *
 * @param value  число в бинарном виде для конвертации в 10-ю систему
 * @param radix  система счисления
 * @param buffer бинарный буфер куда следует положить результат
 * @param size   размер бинарного буфера куда следует положить результат
 *
 */
void awh::Framework::atoi(wstring_view value, const uint8_t radix, void * buffer, const size_t size) const noexcept {
	// Если данные для конвертации переданы
	if(!value.empty() && (radix > 1) && (radix < 37) && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перевод в верхний регистр
			wstring number(value);
			// Позиция в строке алфавита
			size_t pos = wstring::npos;
			// Устанавливаем числовые обозначения
			const wstring digits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			// Если запись в 16-м виде
			if(radix == 16){
				// Если первые два значения числа являются префиксом
				if(number.compare(0, 2, L"0x") == 0)
					// Удаляем первые два символа
					number.erase(0, 2);
			}
			// Выполняем перевод число в верхний регистр
			this->transform(number, transform_t::UPPER_CASE);
			// Количество перебираемых элементов
			const uint8_t count = static_cast <uint8_t> (number.length());
			/**
			 * Определяем размер данных для конвертации
			 */
			switch(size){
				// Если это один байт
				case 1: {
					// Результат с которым будем работать
					uint8_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != wstring::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint8_t> (result * radix + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это два байта
				case 2: {
					// Результат с которым будем работать
					uint16_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != wstring::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint16_t> (result * static_cast <uint16_t> (radix) + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это четыре байта
				case 4: {
					// Результат с которым будем работать
					uint32_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != wstring::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint32_t> (result * static_cast <uint32_t> (radix) + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это восемь байт
				case 8: {
					// Результат с которым будем работать
					uint64_t result = 0;
					/**
					 * Выполняем перебор всех чисел
					 */
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number[i])) != wstring::npos)
							// Выполняем перевод в 10-ю систему счисления
							result = static_cast <uint64_t> (result * static_cast <uint64_t> (radix) + pos);
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Для всех остальных размеров
				default: {
					// Если запись в бинарном виде
					if(radix == 2){
						// Значение байта для установки
						uint8_t byte = 0;
						// Результат с которым будем работать
						std::bitset <8> result(0);
						// Получаем первоначальное значение индексов
						size_t i = value.size(), j = 0, offset = 0;
						/**
						 * Выполняем перебор всей строки
						 */
						while(i--){
							// Если бит положительный
							if(value[i] == '1')
								// Устанавливаем бит результата
								result.set(j);
							// Если бит отрицательный, снимаем его
							else result.reset(j);
							// Увеличиваем смещение бит
							j++;
							// Если мы заполнили байт целиком
							if((j % 8) == 0){
								// Сбрасываем значение счётчика
								j = 0;
								// Выполняем получение числа
								byte = static_cast <uint8_t> (result.to_ulong());
								// Выполняем добавление байта в буфер
								::memcpy(reinterpret_cast <uint8_t *> (buffer) + (offset / 8), &byte, sizeof(byte));
								// Увеличиваем смещение в буфере
								offset += 8;
								// Сбрасываем результат
								result.reset();
							}
						}
					// Записываем ошибку в лог
					} else {
						// Сбрасываем полученный результат
						::memset(buffer, 0, size);
						// Если объект логирования установлен
						if(this->_log != nullptr){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(value), static_cast <uint16_t> (radix), buffer, size), log_t::flag_t::CRITICAL, "Only binary number can be converted to binary buffer");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, "Only binary number can be converted to binary buffer");
							#endif
						// Если объект логирования не установлен
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "Only binary number can be converted to binary buffer");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								::fprintf(stderr, "ERROR! %s\n\n", "Only binary number can be converted to binary buffer");
							#endif
						}
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			::memset(buffer, 0, size);
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(value), static_cast <uint16_t> (radix), buffer, size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод перевода числа в безэкспоненциальную форму
 *
 * @param number число для перевода
 * @param step   размер шага после запятой
 * @return       число в безэкспоненциальной форме
 *
 */
string awh::Framework::noexp(const double number, const uint8_t step) const noexcept {
	// Переменная результата
	string result = "";
	// Если размер шага после запятой передан
	if(step > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Целая часть числа
			double intpart = 0.;
			/**
			 * Целое число записывается без дробной части вовсе, а дробное - с
			 * количеством знаков после запятой, заданным размером шага
			 */
			result = ::noexpFixed(number, ((::modf(number, &intpart) != 0.) ? static_cast <int32_t> (step) : 0));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			result.clear();
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(number, static_cast <uint8_t> (step)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Если запись числа выполнить не удалось
	if(result.empty())
		// Выводим нулевой результат
		result = "0";
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода числа в безэкспоненциальную форму
 *
 * @param number  число для перевода
 * @param onlyNum выводить только числа
 * @return        число в безэкспоненциальной форме
 *
 */
string awh::Framework::noexp(const double number, const bool onlyNum) const noexcept {
	/**
	 * Запись выполняется без обращения к локали и посторонних символов не содержит,
	 * поэтому отбор одних лишь разрядов ничего в ней не меняет. Довод сохранён ради
	 * совместимости вызовов
	 */
	(void) onlyNum;
	// Переменная результата
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем запись числа наименьшей точной записью
		result = ::noexpFixed(number, -1);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Сбрасываем полученный результат
		result.clear();
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(number, onlyNum), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Если запись числа выполнить не удалось
	if(result.empty())
		// Выводим нулевой результат
		result = "0";
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод порверки на сколько процентов (A > B) или (A < B)
 *
 * @param a первое число
 * @param b второе число
 * @return  результат расчёта
 *
 */
float awh::Framework::rate(const float a, const float b) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если второе число равно нулю, относительный процент не определён
		if(b == 0.f)
			// Возвращаем нулевой результат
			return 0.f;
		// Возвращаем разницу в процентах
		return ((a > b ? ((a - b) / b * 100.f) : ((b - a) / b * 100.f) * -1.f));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(a, b), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
		// Возвращаем пустой результат
		return .0f;
	}
}
/**
 * @brief Метод приведения количества символов после запятой к указанному количества
 *
 * @param x число для приведения
 * @param n количество символов после запятой
 * @return  сформированное число
 *
 */
double awh::Framework::floor(const double x, const uint8_t n) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем получение разрядности числа
		const double range = ::pow(10., static_cast <int32_t> (n));
		// Выполняем приведение числа к указанной разрядности
		return (::floor(x * range) / range);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(x, static_cast <uint8_t> (n)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем пустой результат
	return .0;
}
/**
 * @brief Метод перевода римских цифр в арабские
 *
 * @param word римское число
 * @return     арабское число
 *
 */
uint16_t awh::Framework::rome2arabic(string_view word) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем длину слова
			const size_t length = word.size();
			// Если слово состоит всего из одной буквы
			if((length == 0) || ((length == 1) && !symbols.isRome(word.front())))
				// Возвращаем нулевой результат
				return result;
			// Если слово длиннее одной буквы
			else {
				/**
				 * Переходим по всем буквам слова
				 */
				for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
					// Проверяем является ли слово римским числом
					if(!((i == j) ?
						symbols.isRome(word[i]) :
						symbols.isRome(word[i]) &&
						symbols.isRome(word[j])
					)) return result;
				}
			}
			// Символ поиска
			char c = 0, o = 0;
			// Вспомогательные переменные
			uint32_t i = 0, v = 0, n = 0;
			/**
			 * Преобразовываем цифру M
			 */
			if(static_cast <char> (ascii::toLower(word.front())) == 'm'){
				/**
				 * Преобразовываем буквы M в начале слова
				 */
				for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'm'); n++, i++);
				// Если букв больше четырёх
				if(n > 4)
					// Возвращаем нулевой результат
					return result;
				// Добавляем значение к результату
				v += (n * 1000);
			}
			/**
			 * Преобразовываем букву D и C
			 */
			if((i < length) && (((o = ascii::toLower(word[i])) == 'd') || (o == 'c'))){
				// Если буква D
				if(o == 'd'){
					// Увеличиваем позицию
					i++;
					// Добавляем значение к результату
					v += 500;
				}
				/**
				 * Проверяем наличие следующего символа для комбинаций типа "CM", "CD"
				 */
				if((i + 1 < length) && (o = ascii::toLower(word[i])) == 'c'){
					// Запоминаем найденный символ
					char next = ascii::toLower(word[i + 1]);
					// Если это комбинация CM
					if(next == 'm'){
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 900;
					// Если это комбинация CD
					} else if(next == 'd') {
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 400;
					// Иначе это просто буква C
					} else {
						/**
						 * Преобразовываем буквы C
						 */
						for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'c'); n++, i++);
						// Если букв больше четырёх
						if(n > 4)
							// Возвращаем нулевой результат
							return result;
						// Добавляем значение к результату
						v += (n * 100);
					}
				// Иначе это просто буква C
				} else if(i < length && ascii::toLower(word[i]) == 'c') {
					/**
					 * Преобразовываем буквы C
					 */
					for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'c'); n++, i++);
					// Если букв больше четырёх
					if(n > 4)
						// Возвращаем нулевой результат
						return result;
					// Добавляем значение к результату
					v += (n * 100);
				}
			}
			/**
			 * Преобразовываем букву L и X
			 */
			if((i < length) && (((o = ascii::toLower(word[i])) == 'l') || (o == 'x'))){
				// Если буква L
				if(o == 'l'){
					// Увеличиваем позицию
					i++;
					// Добавляем значение к результату
					v += 50;
				}
				/**
				 * Проверяем наличие следующего символа для комбинаций типа "XC", "XL"
				 */
				if((i + 1 < length) && (o = ascii::toLower(word[i])) == 'x'){
					// Запоминаем найденный символ
					char next = ascii::toLower(word[i + 1]);
					// Если это комбинация XC
					if(next == 'c'){
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 90;
					// Если это комбинация XL
					} else if(next == 'l') {
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 40;
					// Иначе это просто буква X
					} else {
						/**
						 * Преобразовываем буквы X
						 */
						for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'x'); n++, i++);
						// Если букв больше четырёх
						if(n > 4)
							// Возвращаем нулевой результат
							return result;
						// Добавляем значение к результату
						v += (n * 10);
					}
				// Иначе это просто буква X
				} else if(i < length && ascii::toLower(word[i]) == 'x') {
					/**
					 * Преобразовываем буквы X
					 */
					for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'x'); n++, i++);
					// Если букв больше четырёх
					if(n > 4)
						// Возвращаем нулевой результат
						return result;
					// Добавляем значение к результату
					v += (n * 10);
				}
			}
			/**
			 * Преобразовываем букву V и I
			 */
			if((i < length) && (((o = ascii::toLower(word[i])) == 'v') || (o == 'i'))){
				// Если буква V
				if(o == 'v'){
					// Увеличиваем позицию
					i++;
					// Добавляем значение к результату
					v += 5;
				}
				/**
				 * Проверяем наличие следующего символа для комбинаций типа "IX", "IV"
				 */
				if((i + 1 < length) && (o = ascii::toLower(word[i])) == 'i'){
					// Запоминаем найденный символ
					char next = ascii::toLower(word[i + 1]);
					// Если это комбинация IX
					if(next == 'x'){
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 9;
					// Если это комбинация IV
					} else if(next == 'v') {
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 4;
					// Иначе это просто буква I
					} else {
						/**
						 * Преобразовываем буквы I
						 */
						for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'i'); n++, i++);
						// Если букв больше четырёх
						if(n > 4)
							// Возвращаем нулевой результат
							return result;
						// Добавляем значение к результату
						v += n;
					}
				// Иначе это просто буква I
				} else if(i < length && ascii::toLower(word[i]) == 'i') {
					/**
					 * Преобразовываем буквы I
					 */
					for(n = 0; (i < length) && (ascii::toLower(word[i]) == 'i'); n++, i++);
					// Если букв больше четырёх
					if(n > 4)
						// Возвращаем нулевой результат
						return result;
					// Добавляем значение к результату
					v += n;
				}
			}
			// Формируем результат
			result = (((i == length) && (v >= 1) && (v <= 4999)) ? v : 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(word), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода римских цифр в арабские
 *
 * @param word римское число
 * @return     арабское число
 *
 */
uint16_t awh::Framework::rome2arabic(wstring_view word) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем длину слова
			const size_t length = word.size();
			// Если слово состоит всего из одной буквы
			if((length == 0) || ((length == 1) && !symbols.isRome(word.front())))
				// Возвращаем нулевой результат
				return result;
			// Если слово длиннее одной буквы
			else {
				/**
				 * Переходим по всем буквам слова
				 */
				for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
					// Проверяем является ли слово римским числом
					if(!((i == j) ?
						symbols.isRome(word[i]) :
						symbols.isRome(word[i]) &&
						symbols.isRome(word[j])
					)) return result;
				}
			}
			// Символ поиска
			wchar_t c = 0, o = 0;
			// Вспомогательные переменные
			uint32_t i = 0, v = 0, n = 0;
			/**
			 * Преобразовываем цифру M
			 */
			if(static_cast <wchar_t> (wideLower(static_cast <wint_t> (word.front()))) == L'm'){
				/**
				 * Преобразовываем буквы M в начале слова
				 */
				for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'm'); n++, i++);
				// Если букв больше четырёх
				if(n > 4)
					// Возвращаем нулевой результат
					return result;
				// Добавляем значение к результату
				v += (n * 1000);
			}
			/**
			 * Преобразовываем букву D и C
			 */
			if((i < length) && (((o = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i])))) == L'd') || (o == L'c'))){
				// Если буква D
				if(o == L'd'){
					// Увеличиваем позицию
					i++;
					// Добавляем значение к результату
					v += 500;
				}
				/**
				 * Проверяем наличие следующего символа для комбинаций типа "CM", "CD"
				 */
				if((i + 1 < length) && (o = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i])))) == L'c'){
					// Запоминаем найденный символ
					wchar_t next = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i + 1])));
					// Если это комбинация CM
					if(next == L'm'){
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 900;
					// Если это комбинация CD
					} else if(next == L'd') {
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 400;
					// Иначе это просто буква C
					} else {
						/**
						 * Преобразовываем буквы C
						 */
						for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'c'); n++, i++);
						// Если букв больше четырёх
						if(n > 4)
							// Возвращаем нулевой результат
							return result;
						// Добавляем значение к результату
						v += (n * 100);
					}
				// Иначе это просто буква C
				} else if(i < length && static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'c') {
					/**
					 * Преобразовываем буквы C
					 */
					for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'c'); n++, i++);
					// Если букв больше четырёх
					if(n > 4)
						// Возвращаем нулевой результат
						return result;
					// Добавляем значение к результату
					v += (n * 100);
				}
			}
			/**
			 * Преобразовываем букву L и X
			 */
			if((i < length) && (((o = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i])))) == L'l') || (o == L'x'))){
				// Если буква L
				if(o == L'l'){
					// Увеличиваем позицию
					i++;
					// Добавляем значение к результату
					v += 50;
				}
				/**
				 * Проверяем наличие следующего символа для комбинаций типа "XC", "XL"
				 */
				if((i + 1 < length) && (o = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i])))) == L'x'){
					// Запоминаем найденный символ
					wchar_t next = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i + 1])));
					// Если это комбинация XC
					if(next == L'c'){
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 90;
					// Если это комбинация XL
					} else if(next == L'l') {
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 40;
					// Иначе это просто буква X
					} else {
						/**
						 * Преобразовываем буквы X
						 */
						for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'x'); n++, i++);
						// Если букв больше четырёх
						if(n > 4)
							// Возвращаем нулевой результат
							return result;
						// Добавляем значение к результату
						v += (n * 10);
					}
				// Иначе это просто буква X
				} else if(i < length && static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'x') {
					/**
					 * Преобразовываем буквы X
					 */
					for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'x'); n++, i++);
					// Если букв больше четырёх
					if(n > 4)
						// Возвращаем нулевой результат
						return result;
					// Добавляем значение к результату
					v += (n * 10);
				}
			}
			/**
			 * Преобразовываем букву V и I
			 */
			if((i < length) && (((o = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i])))) == L'v') || (o == L'i'))){
				// Если буква V
				if(o == L'v'){
					// Увеличиваем позицию
					i++;
					// Добавляем значение к результату
					v += 5;
				}
				/**
				 * Проверяем наличие следующего символа для комбинаций типа "IX", "IV"
				 */
				if((i + 1 < length) && (o = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i])))) == L'i'){
					// Запоминаем найденный символ
					wchar_t next = static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i + 1])));
					// Если это комбинация IX
					if(next == L'x'){
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 9;
					// Если это комбинация IV
					} else if(next == L'v') {
						// Увеличиваем позицию
						i += 2;
						// Добавляем значение к результату
						v += 4;
					// Иначе это просто буква I
					} else {
						/**
						 * Преобразовываем буквы I
						 */
						for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'i'); n++, i++);
						// Если букв больше четырёх
						if(n > 4)
							// Возвращаем нулевой результат
							return result;
						// Добавляем значение к результату
						v += n;
					}
				// Иначе это просто буква I
				} else if(i < length && static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'i') {
					/**
					 * Преобразовываем буквы I
					 */
					for(n = 0; (i < length) && (static_cast <wchar_t> (wideLower(static_cast <wint_t> (word[i]))) == L'i'); n++, i++);
					// Если букв больше четырёх
					if(n > 4)
						// Возвращаем нулевой результат
						return result;
					// Добавляем значение к результату
					v += n;
				}
			}
			// Формируем результат
			result = (((i == length) && (v >= 1) && (v <= 4999)) ? v : 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(word)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода арабских чисел в римские
 *
 * @param number арабское число от 1 до 4999
 * @return       римское число
 *
 */
wstring awh::Framework::arabic2rome(const uint32_t number) const noexcept {
	// Переменная результата
	wstring result = L"";
	// Если число передано верное
	if((number >= 1) && (number <= 4999)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Копируем полученное число
			uint32_t n = number;
			// Вычисляем до тысяч
			result.append(romanNumerals.m[static_cast <uint8_t> (::floor(n / 1000.))]);
			// Уменьшаем диапазон
			n %= 1000;
			// Вычисляем до сотен
			result.append(romanNumerals.c[static_cast <uint8_t> (::floor(n / 100.))]);
			// Вычисляем до сотен
			n %= 100;
			// Вычисляем до десятых
			result.append(romanNumerals.x[static_cast <uint8_t> (::floor(n / 10.))]);
			// Вычисляем до сотен
			n %= 10;
			// Формируем окончательный результат
			result.append(romanNumerals.i[n]);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(number), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода арабских чисел в римские
 *
 * @param word арабское число от 1 до 4999
 * @return     римское число
 *
 */
string awh::Framework::arabic2rome(string_view word) const noexcept {
	// Переменная результата
	string result = "";
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Преобразуем слово в число
			const uint32_t number = this->atoi <uint32_t> (word);
			// Выполняем расчет
			result = ::move(this->convert(this->arabic2rome(number)));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(word), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода арабских чисел в римские
 *
 * @param word арабское число от 1 до 4999
 * @return     римское число
 *
 */
wstring awh::Framework::arabic2rome(wstring_view word) const noexcept {
	// Переменная результата
	wstring result = L"";
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Преобразуем слово в число
			const uint32_t number = ::stoi(wstring{word});
			// Выполняем расчет
			result = this->arabic2rome(number);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(word)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод подсчёта количества указанной буквы в слове
 *
 * @param word   слово в котором нужно подсчитать букву
 * @param letter букву которую нужно подсчитать
 * @return       результат подсчёта
 *
 */
size_t awh::Framework::countLetter(string_view word, const wchar_t letter) const noexcept {
	// Переменная результата
	size_t result = 0;
	// Если слово и буква переданы
	if(!word.empty() && (letter > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ищем нашу букву
			size_t pos = 0;
			// Если искомый символ принадлежит таблице ASCII
			if(letter < 0x80){
				// Получаем искомый символ в однобайтовом виде
				const char target = static_cast <char> (letter);
				/**
				 * Выполняем подсчёт количества указанных букв в слове
				 */
				while((pos = word.find(target, pos)) != string::npos){
					// Считаем количество букв
					result++;
					// Увеличиваем позицию
					pos++;
				}
			// Если искомый символ является многобайтовым (UTF-8)
			} else {
				// Получаем искомый символ в виде UTF-8 последовательности
				const string target = this->convert(wstring(1, letter));
				// Если последовательность получена
				if(!target.empty()){
					/**
					 * Выполняем подсчёт количества указанных букв в слове
					 */
					while((pos = word.find(target, pos)) != string::npos){
						// Считаем количество букв
						result++;
						// Смещаем позицию на длину последовательности
						pos += target.size();
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(word, letter), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод подсчёта количества указанной буквы в слове
 *
 * @param word   слово в котором нужно подсчитать букву
 * @param letter букву которую нужно подсчитать
 * @return       результат подсчёта
 *
 */
size_t awh::Framework::countLetter(wstring_view word, const wchar_t letter) const noexcept {
	// Переменная результата
	size_t result = 0;
	// Если слово и буква переданы
	if(!word.empty() && (letter > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ищем нашу букву
			size_t pos = 0;
			/**
			 * Выполняем подсчёт количества указанных букв в слове
			 */
			while((pos = word.find(letter, pos)) != wstring::npos){
				// Считаем количество букв
				result++;
				// Увеличиваем позицию
				pos++;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(word), letter), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Шаблон функции проверки установлен ли бит в указанной позиции
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод проверки установлен ли бит в указанной позиции
 *
 * @param pos позиция для проверки
 * @param num число в бинарном виде для проверки бита
 * @return    результат проверки
 *
 */
bool awh::Framework::isBit(const T pos, const T num) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем размер типа входящих данных
		 */
		switch(sizeof(result)){
			// Если число принадлежит к типу uint8_t
			case 1: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 8)
					// Устанавливаем бит регистра по указанной позиции
					return ((num & (static_cast <T> (1) << pos)) != 0);
			} break;
			// Если число принадлежит к типу uint16_t
			case 2: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 16)
					// Устанавливаем бит регистра по указанной позиции
					return ((num & (static_cast <T> (1) << pos)) != 0);
			} break;
			// Если число принадлежит к типу uint32_t
			case 4: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 32)
					// Устанавливаем бит регистра по указанной позиции
					return ((num & (static_cast <T> (1) << pos)) != 0);
			} break;
			// Если число принадлежит к типу uint64_t
			case 8: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 64)
					// Устанавливаем бит регистра по указанной позиции
					return ((num & (static_cast <T> (1) << pos)) != 0);
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pos, num), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода проверки установлен ли бит в указанной позиции
 */
template bool awh::Framework::isBit <uint8_t> (const uint8_t, const uint8_t) const noexcept;
template bool awh::Framework::isBit <uint16_t> (const uint16_t, const uint16_t) const noexcept;
template bool awh::Framework::isBit <uint32_t> (const uint32_t, const uint32_t) const noexcept;
template bool awh::Framework::isBit <uint64_t> (const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Шаблон функции инверсии бита в указанной позиции
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод инверсии бита в указанной позиции
 *
 * @param pos позиция для инверсии
 * @param num число в бинарном виде для инверсии бита
 * @return    итоговое значение числа после инверсии
 *
 */
T awh::Framework::flipBit(const T pos, const T num) const noexcept {
	// Результат работы функции
	T result = num;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем размер типа входящих данных
		 */
		switch(sizeof(result)){
			// Если число принадлежит к типу uint8_t
			case 1: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 8)
					// Инвертируем бит регистра по указанной позиции
					result ^= (static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint16_t
			case 2: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 16)
					// Инвертируем бит регистра по указанной позиции
					result ^= (static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint32_t
			case 4: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 32)
					// Инвертируем бит регистра по указанной позиции
					result ^= (static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint64_t
			case 8: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 64)
					// Инвертируем бит регистра по указанной позиции
					result ^= (static_cast <T> (1) << pos);
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pos, num), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода инверсии бита в указанной позиции
 */
template uint8_t awh::Framework::flipBit <uint8_t> (const uint8_t, const uint8_t) const noexcept;
template uint16_t awh::Framework::flipBit <uint16_t> (const uint16_t, const uint16_t) const noexcept;
template uint32_t awh::Framework::flipBit <uint32_t> (const uint32_t, const uint32_t) const noexcept;
template uint64_t awh::Framework::flipBit <uint64_t> (const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Шаблон функции сброса бита в указанной позиции
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод сброса бита в указанной позиции
 *
 * @param pos позиция для сброса
 * @param num число в бинарном виде для сброса бита
 * @return    итоговое значение числа после сброса бита
 *
 */
T awh::Framework::resetBit(const T pos, const T num) const noexcept {
	// Результат работы функции
	T result = num;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем размер типа входящих данных
		 */
		switch(sizeof(result)){
			// Если число принадлежит к типу uint8_t
			case 1: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 8)
					// Сбрасываем бит регистра по указанной позиции
					result &= ~(static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint16_t
			case 2: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 16)
					// Сбрасываем бит регистра по указанной позиции
					result &= ~(static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint32_t
			case 4: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 32)
					// Сбрасываем бит регистра по указанной позиции
					result &= ~(static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint64_t
			case 8: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 64)
					// Сбрасываем бит регистра по указанной позиции
					result &= ~(static_cast <T> (1) << pos);
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pos, num), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода сброса бита в указанной позиции
 */
template uint8_t awh::Framework::resetBit <uint8_t> (const uint8_t, const uint8_t) const noexcept;
template uint16_t awh::Framework::resetBit <uint16_t> (const uint16_t, const uint16_t) const noexcept;
template uint32_t awh::Framework::resetBit <uint32_t> (const uint32_t, const uint32_t) const noexcept;
template uint64_t awh::Framework::resetBit <uint64_t> (const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Шаблон функции устанвки бита в указанную позицию
 *
 * @tparam T тип данных с которым работает функция
 *
 */
template <typename T>
/**
 * @brief Метод устанвки бита в указанную позицию
 *
 * @param pos позиция для установки бита
 * @param num начальное значение бита
 * @return    итоговое значение числа после установки бита
 *
 */
T awh::Framework::setBit(const T pos, const T num) const noexcept {
	// Переменная результата
	T result = num;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем размер типа входящих данных
		 */
		switch(sizeof(result)){
			// Если число принадлежит к типу uint8_t
			case 1: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 8)
					// Устанавливаем бит регистра по указанной позиции
					result += (static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint16_t
			case 2: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 16)
					// Устанавливаем бит регистра по указанной позиции
					result += (static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint32_t
			case 4: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 32)
					// Устанавливаем бит регистра по указанной позиции
					result += (static_cast <T> (1) << pos);
			} break;
			// Если число принадлежит к типу uint64_t
			case 8: {
				// Если позиция в пределах разрядности счётчика
				if(pos < 64)
					// Устанавливаем бит регистра по указанной позиции
					result += (static_cast <T> (1) << pos);
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pos, num), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода установки бита в указанную позицию
 */
template uint8_t awh::Framework::setBit <uint8_t> (const uint8_t, const uint8_t) const noexcept;
template uint16_t awh::Framework::setBit <uint16_t> (const uint16_t, const uint16_t) const noexcept;
template uint32_t awh::Framework::setBit <uint32_t> (const uint32_t, const uint32_t) const noexcept;
template uint64_t awh::Framework::setBit <uint64_t> (const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Метод реализации функции формирования форматированной строки
 *
 * @param format формат строки вывода
 * @param args   передаваемые аргументы
 * @return       сформированная строка
 *
 */
string awh::Framework::formatted(const char * format, ...) const noexcept {
	// Переменная результата
	string result = "";
	// Если формат передан
	if((format != nullptr) && (format[0] != '\0')){
		// Создаем список аргументов
		va_list args;
		// Запускаем инициализацию списка аргументов
		va_start(args, format);
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер полученной строки
			size_t length = 0;
			// Создаем буфер данных
			result.resize(1024);
			/**
			 * Выполняем перебор всех аргументов
			 */
			while(true){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				length = ::vsnprintf(result.data(), result.size(), format, args2);
				// Если результат получен
				if((length >= 0) && (length < result.size())){
					// Завершаем список аргументов
					va_end(args);
					// Завершаем список локальных аргументов
					va_end(args2);
					// Если идентификатор обнулился после переполнения счётчика
					if(length == 0){
						// Выполняем сброс результата
						result.clear();
						// Выходим из функции
						return result;
					// Возвращаем результат
					} else return result.assign(result.begin(), result.begin() + length);
				}
				// Размер буфера данных
				size_t size = 0;
				// Если данные не получены, увеличиваем буфер в два раза
				if(length < 0)
					// Увеличиваем размер буфера в два раза
					size = (result.size() * 2);
				// Увеличиваем размер буфера на один байт
				else size = (length + 1);
				// Очищаем буфер данных
				result.clear();
				// Выделяем память для буфера
				result.resize(size);
				// Завершаем список локальных аргументов
				va_end(args2);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(format), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
		// Завершаем список аргументов
		va_end(args);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод реализации функции формирования форматированной строки
 *
 * @param format формат строки вывода
 * @param args   передаваемые аргументы
 * @return       сформированная строка
 *
 */
wstring awh::Framework::formatted(const wchar_t * format, ...) const noexcept {
	// Переменная результата
	wstring result = L"";
	// Если формат передан
	if((format != nullptr) && (format[0] != L'\0')){
		// Создаем список аргументов
		va_list args;
		// Запускаем инициализацию списка аргументов
		va_start(args, format);
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер полученной строки
			size_t length = 0;
			// Создаем буфер данных
			result.resize(1024);
			/**
			 * Выполняем перебор всех аргументов
			 */
			while(true){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				length = ::vswprintf(result.data(), result.size(), format, args2);
				// Если результат получен
				if((length >= 0) && (length < result.size())){
					// Завершаем список аргументов
					va_end(args);
					// Завершаем список локальных аргументов
					va_end(args2);
					// Если идентификатор обнулился после переполнения счётчика
					if(length == 0){
						// Выполняем сброс результата
						result.clear();
						// Выходим из функции
						return result;
					// Возвращаем результат
					} else return result.assign(result.begin(), result.begin() + length);
				}
				// Размер буфера данных
				size_t size = 0;
				// Если данные не получены, увеличиваем буфер в два раза
				if(length < 0)
					// Увеличиваем размер буфера в два раза
					size = (result.size() * 2);
				// Увеличиваем размер буфера на один байт
				else size = (length + 1);
				// Очищаем буфер данных
				result.clear();
				// Выделяем память для буфера
				result.resize(size);
				// Завершаем список локальных аргументов
				va_end(args2);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(format)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
		// Завершаем список аргументов
		va_end(args);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод реализации функции формирования форматированной строки
 *
 * @param format формат строки вывода
 * @param items  список аргументов строки
 * @return       сформированная строка
 *
 */
string awh::Framework::format(string_view format, const vector <string> & items) const noexcept {
	// Переменная результата
	string result(format);
	// Если данные переданы
	if(!format.empty() && !items.empty()){
		/**
		 * @brief Функция заменты подстроки в строке
		 *
		 * @param str  строка в которой нужно произвести замену
		 * @param from строка которую нужно заменить
		 * @param to   строка на которую нужно заменить
		 *
		 */
		auto replaceFn = [&format, &items, this](string & str, const string & from, const string & to) noexcept {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Если строка пустая, выходим
				if(from.empty() || to.empty())
					// Выходим из функции
					return;
				// Позиция подстроки в строке
				size_t pos = 0;
				/**
				 * Выполняем поиск подстроки в стркое
				 */
				while((pos = str.find(from, pos)) != string::npos){
					// Заменяем подстроку в строке
					str.replace(pos, from.length(), to);
					// Увеличиваем позицию для поиска в строке
					pos += to.length();
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Если объект логирования установлен
				if(this->_log != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(format, items.size()), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				// Если объект логирования не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		};
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Индекс в массиве
			uint16_t index = 1;
			// Исправляем возврат каретки
			replaceFn(result, "\\r", "\r");
			// Исправляем перенос строки
			replaceFn(result, "\\n", "\n");
			// Исправляем табуляцию
			replaceFn(result, "\\t", "\t");
			/**
			 * Перебираем весь список аргументов
			 */
			for(auto & item : items)
				// Выполняем замену индекса аргумента на указанный аргумент
				replaceFn(result, "$" + std::to_string(index++), item);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(format, items.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод реализации функции формирования форматированной строки
 *
 * @param format формат строки вывода
 * @param items  список аргументов строки
 * @return       сформированная строка
 *
 */
wstring awh::Framework::format(wstring_view format, const vector <wstring> & items) const noexcept {
	// Переменная результата
	wstring result(format);
	// Если данные переданы
	if(!format.empty() && !items.empty()){
		/**
		 * @brief Функция заменты подстроки в строке
		 *
		 * @param str  строка в которой нужно произвести замену
		 * @param from строка которую нужно заменить
		 * @param to   строка на которую нужно заменить
		 *
		 */
		auto replaceFn = [&format, &items, this](wstring & str, const wstring & from, const wstring & to) noexcept {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Если строка пустая, выходим
				if(from.empty() || to.empty())
					// Выходим из функции
					return;
				// Позиция подстроки в строке
				size_t pos = 0;
				/**
				 * Выполняем поиск подстроки в стркое
				 */
				while((pos = str.find(from, pos)) != string::npos){
					// Заменяем подстроку в строке
					str.replace(pos, from.length(), to);
					// Увеличиваем позицию для поиска в строке
					pos += to.length();
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Если объект логирования установлен
				if(this->_log != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(format), items.size()), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				// Если объект логирования не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		};
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Индекс в массиве
			uint16_t index = 1;
			// Исправляем возврат каретки
			replaceFn(result, L"\\r", L"\r");
			// Исправляем перенос строки
			replaceFn(result, L"\\n", L"\n");
			// Исправляем табуляцию
			replaceFn(result, L"\\t", L"\t");
			/**
			 * Перебираем весь список аргументов
			 */
			for(auto & item : items)
				// Выполняем замену индекса аргумента на указанный аргумент
				replaceFn(result, L"$" + std::to_wstring(index++), item);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(format), items.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки существования слова в тексте
 *
 * @param word слово для проверки
 * @param text текст в котором выполнения проверка
 * @return     результат выполнения проверки
 *
 */
bool awh::Framework::exists(string_view word, string_view text) const noexcept {
	// Если данные переданы верные
	if(!word.empty() && !text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем регистронезависимый поиск слова в тексте
			const auto i = std::search(text.begin(), text.end(), word.begin(), word.end(), [](const char first, const char second) noexcept -> bool {
				// Выполняем сравнение символов без учёта регистра
				return (ascii::toLower(first) == ascii::toLower(second));
			});
			// Возвращаем результат проверки
			return (i != text.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(word, text), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат проверки по умолчанию
	return false;
}
/**
 * @brief Метод проверки существования слова в тексте
 *
 * @param word слово для проверки
 * @param text текст в котором выполнения проверка
 * @return     результат выполнения проверки
 *
 */
bool awh::Framework::exists(wstring_view word, wstring_view text) const noexcept {
	// Если данные переданы верные
	if(!word.empty() && !text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем регистронезависимый поиск слова в тексте
			const auto i = std::search(text.begin(), text.end(), word.begin(), word.end(), [](const wchar_t first, const wchar_t second) noexcept -> bool {
				// Выполняем сравнение символов без учёта регистра
				return (wideLower(static_cast <wint_t> (first)) == wideLower(static_cast <wint_t> (second)));
			});
			// Возвращаем результат проверки
			return (i != text.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(word), this->convert(text)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат проверки по умолчанию
	return false;
}
/**
 * @brief Метод замены в тексте слово на другое слово
 *
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 *
 */
string & awh::Framework::replace(string & text, const string & word, const string & alt) const noexcept {
	// Если текст передан и искомое слово не равно слову для замены
	if(!text.empty() && !word.empty() && !this->compare(word, alt)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция искомого текста
			size_t pos = 0;
			// Определяем текст на который нужно произвести замену
			const string & alternative = (!alt.empty() ? alt : "");
			/**
			 * Выполняем поиск всех слов
			 */
			while((pos = text.find(word, pos)) != string::npos){
				// Выполняем замену текста
				text.replace(pos, word.length(), alternative);
				// Смещаем позицию на единицу
				pos++;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, word, alt), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return text;
}
/**
 * @brief Метод замены в тексте слово на другое слово
 *
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 *
 */
wstring & awh::Framework::replace(wstring & text, const wstring & word, const wstring & alt) const noexcept {
	// Если текст передан и искомое слово не равно слову для замены
	if(!text.empty() && !word.empty() && !this->compare(word, alt)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция искомого текста
			size_t pos = 0;
			// Определяем текст на который нужно произвести замену
			const wstring & alternative = (!alt.empty() ? alt : L"");
			/**
			 * Выполняем поиск всех слов
			 */
			while((pos = text.find(word, pos)) != wstring::npos){
				// Выполняем замену текста
				text.replace(pos, word.length(), alternative);
				// Смещаем позицию на единицу
				pos++;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(text), this->convert(word), this->convert(alt)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return text;
}
/**
 * @brief Метод замены в тексте слово на другое слово
 *
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 *
 */
const string & awh::Framework::replace(const string & text, const string & word, const string & alt) const noexcept {
	// Выполняем замену в тексте слово на другое слово
	return this->replace(* const_cast <string *> (&text), word, alt);
}
/**
 * @brief Метод замены в тексте слово на другое слово
 *
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 *
 */
const wstring & awh::Framework::replace(const wstring & text, const wstring & word, const wstring & alt) const noexcept {
	// Выполняем замену в тексте слово на другое слово
	return this->replace(* const_cast <wstring *> (&text), word, alt);
}
/**
 * @brief Метод извлечения списка символов экранирования по умолчанию
 *
 * @return список символов экранирования по умолчанию
 *
 */
const vector <string> & awh::Framework::escapingText() noexcept {
	// Список символов экранирования по умолчанию
	static const vector <string> result = {string{"\""}};
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения списка символов экранирования по умолчанию
 *
 * @return список символов экранирования по умолчанию
 *
 */
const vector <wstring> & awh::Framework::escapingWide() noexcept {
	// Список символов экранирования по умолчанию
	static const vector <wstring> result = {wstring{L"\""}};
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения ключей и значений из текста
 *
 * @param text      текст из которого извлекаются записи
 * @param delim     разделитель записей
 * @param separator разделитель ключа и значения
 * @param escaping  символы экранирования
 * @return          список найденных элементов
 *
 */
unordered_multimap <string, string> awh::Framework::kv(string_view text, string_view delim, string_view separator, const vector <string> & escaping) const noexcept {
	// Переменная результата
	unordered_multimap <string, string> result;
	// Если данные для обработки текста передан
	if(!text.empty() && !delim.empty() && !separator.empty() && !escaping.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем разбор текста на записи ключ-значение
			kvParse <char> (text, delim, separator, escaping, [&result](const string_view key, const string_view value) noexcept -> void {
				// Выполняем формирование записи результата
				result.emplace(key, value);
			});
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text, delim, separator, escaping.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения ключей и значений из текста
 *
 * @param text      текст из которого извлекаются записи
 * @param delim     разделитель записей
 * @param separator разделитель ключа и значения
 * @param escaping  символы экранирования
 * @return          список найденных элементов
 *
 */
unordered_multimap <wstring, wstring> awh::Framework::kv(wstring_view text, wstring_view delim, wstring_view separator, const vector <wstring> & escaping) const noexcept {
	// Переменная результата
	unordered_multimap <wstring, wstring> result;
	// Если данные для обработки текста передан
	if(!text.empty() && !delim.empty() && !separator.empty() && !escaping.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем разбор текста на записи ключ-значение
			kvParse <wchar_t> (text, delim, separator, escaping, [&result](const wstring_view key, const wstring_view value) noexcept -> void {
				// Выполняем формирование записи результата
				result.emplace(key, value);
			});
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->convert(text), this->convert(delim), this->convert(separator), escaping.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод потокового извлечения ключей и значений из текста
 *
 * @param sid       идентификатор потока разбора
 * @param text      текст из которого извлекаются записи
 * @param delim     разделитель записей
 * @param callback  функция обратного вызова для каждой найденной записи
 * @param separator разделитель ключа и значения
 * @param escaping  символы экранирования
 *
 */
void awh::Framework::kv(const uint64_t sid, string_view text, string_view delim, function <void (const uint64_t, const string_view, const string_view)> callback, string_view separator, const vector <string> & escaping) const noexcept {
	// Если данные для обработки текста передан
	if((callback != nullptr) && !text.empty() && !delim.empty() && !separator.empty() && !escaping.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем разбор текста на записи ключ-значение
			kvParse <char> (text, delim, separator, escaping, [sid, &callback](const string_view key, const string_view value) noexcept -> void {
				// Выполняем передачу найденной записи
				callback(sid, key, value);
			});
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, text, delim, separator, escaping.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод потокового извлечения ключей и значений из текста
 *
 * @param sid       идентификатор потока разбора
 * @param text      текст из которого извлекаются записи
 * @param delim     разделитель записей
 * @param callback  функция обратного вызова для каждой найденной записи
 * @param separator разделитель ключа и значения
 * @param escaping  символы экранирования
 *
 */
void awh::Framework::kv(const uint64_t sid, wstring_view text, wstring_view delim, function <void (const uint64_t, const wstring_view, const wstring_view)> callback, wstring_view separator, const vector <wstring> & escaping) const noexcept {
	// Если данные для обработки текста передан
	if((callback != nullptr) && !text.empty() && !delim.empty() && !separator.empty() && !escaping.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем разбор текста на записи ключ-значение
			kvParse <wchar_t> (text, delim, separator, escaping, [sid, &callback](const wstring_view key, const wstring_view value) noexcept -> void {
				// Выполняем передачу найденной записи
				callback(sid, key, value);
			});
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, this->convert(text), this->convert(delim), this->convert(separator), escaping.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод установки пользовательской зоны
 *
 * @param zone пользовательская зона
 *
 */
void awh::Framework::domainZone(string_view zone) noexcept {
	// Если зона передана, устанавливаем её
	if(!zone.empty())
		// Устанавливаем пользовательскую зону
		this->_nwt.zone(zone);
}
/**
 * @brief Метод установки списка пользовательских зон
 *
 * @param zones список доменных зон интернета
 *
 */
void awh::Framework::domainZones(const unordered_set <string> & zones) noexcept {
	// Устанавливаем список доменных зон
	if(!zones.empty())
		// Устанавливаем список пользовательских зон
		this->_nwt.zones(zones);
}
/**
 * @brief Метод извлечения списка пользовательских зон интернета
 *
 * @return список доменных зон
 *
 */
const unordered_set <string> & awh::Framework::domainZones() const noexcept {
	// Возвращаем список доменных зон интернета
	return this->_nwt.zones();
}
/**
 * @brief Метод установки системной локали
 *
 * @param locale локализация приложения
 *
 */
void awh::Framework::setLocale(const string & locale) noexcept {
	// Устанавливаем локаль
	if(!locale.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливапм локализацию приложения
			::setlocale(LC_ALL, locale.c_str());
			::setlocale(LC_CTYPE, locale.c_str());
			::setlocale(LC_COLLATE, locale.c_str());
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Параметры устанавливаемого шрифта
				CONSOLE_FONT_INFOEX fontInfo = {0};
				// Устанавливаем размер буфера шрифта
				fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
				// Формируем параметры шрифта
				fontInfo.nFont = 1;
				fontInfo.dwFontSize.X = 7;
				fontInfo.dwFontSize.Y = 12;
				fontInfo.FontWeight = 500;
				fontInfo.FontFamily = FF_DONTCARE;
				// Выполняем установку шрифта Lucida Console
				::lstrcpyW(fontInfo.FaceName, L"Lucida Console");
				// Применяем шрифт
				::SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &fontInfo);
				// Устанавливаем кодировку ввода текстовых данных в консоле 65001
				::SetConsoleCP(CP_UTF8);
				// Устанавливаем кодировку вывода текстовых данных из консоли
				::SetConsoleOutputCP(CP_UTF8);
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если локаль не дефолтная
			if(locale.compare("C") != 0)
				// Устанавливаем локаль повторно
				this->setLocale("C");
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(locale), log_t::flag_t::WARNING, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "WARNING! Called function:\n%s\n\nMessage:\n%s (%s)\n\n", __PRETTY_FUNCTION__, error.what(), locale.data());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "WARNING! %s (%s)\n\n", error.what(), locale.data());
				#endif
			}
		}
	}
}
/**
 * @brief Метод извлечения координат url адресов в строке
 *
 * @param text текст для извлечения url адресов
 * @return     список координат с url адресами
 *
 */
unordered_map <size_t, size_t> awh::Framework::urls(string_view text) const noexcept {
	// Переменная результата
	unordered_map <size_t, size_t> result;
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция найденного nwt адреса
			size_t pos = 0;
			/**
			 * Выполням поиск ссылок в тексте
			 */
			while(pos < text.size()){
				// Выполняем парсинг nwt адреса
				auto resUri = this->_nwt.parse(text.substr(pos));
				// Если ссылка найдена
				if(resUri.type != nwt_t::types_t::NONE){
					// Получаем данные слова
					const string & word = resUri.uri;
					// Если позиция найдена
					if((pos = text.find(word, pos)) != string::npos){
						// Если в списке результатов найдены пустные значения, очищаем список
						if(result.count(string::npos) > 0)
							// Выполняем очистку результата
							result.clear();
						// Добавляем в список нашу ссылку
						result.insert({pos, pos + word.length()});
					// Если ссылка не найдена в тексте, выходим
					} else break;
					// Сдвигаем значение позиции
					pos += word.length();
				// Если uri адрес больше не найден то выходим
				} else break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения иконки
 *
 * @param end флаг завершения работы
 * @return    иконка напутствия работы
 *
 */
string awh::Framework::icon(const bool end) const noexcept {
	// Список иконок для начала работы
	static const vector <string> iconBegin = {
		"🎲","🎰","🏓","🎱","🥚","⚽️",
		"🏀","🏈","⚾️","🥎","🏐","🪙",
		"🎾","🏑","🧲","🏹","🧱","🏋‍♀️",
		"⛹‍♀️","🤽‍♀️","🥁","🕯","🎳","🎮",
		"🙏","🤪","🙄","😏","😊","☺️",
		"😉","🤔","😋","😤","🤥","🧐",
		"🤓","😇","🙃","🤫","🤭","🙂",
		"🤗","🤩","😌","😎","🤡","🤠",
		"🌟","🧠","👀","👁","🏦","🛸",
		"🎬","❤️","📈","🛒","🛎","🤹‍♀️",
		"☝️","🎈","🧚","🕊","✨","⚡️",
		"🌏", "🔥","🪁","🎻","🎲","🎪",
		"🚦","🇷🇺","📺","🏸","🚀","⏳",
		"⏳","♨️","📉","💤","📊","🏳️"
	};
	// Список иконок для конца работы
	static const vector <string> iconEnd = {
		"🍾","🎉","🎊","🎈","🎁","🥳",
		"🤩","😍","🥰","🤝","🙌","👐",
		"👌","✌️","🤟","🐝","🎖","🥇",
		"🥈","🥉","🏅","💳","🧨","🚬",
		"🏆","🎯","💎","🔮","🎗","🏵",
		"💪","👍","🪄","💍","⏰","🧮",
		"👸","🤴","🥷","💖","💘","🛍",
		"💝","🧸","💸","🧟‍♂️","💞","👩‍💻",
		"🎀","👅","💋","🚨","🦾","🦠",
		"💩","👾","👼","💥","💫","🌞",
		"🍫","🎂","💯","📰","❤️‍🔥","🎣",
		"🏁","🧾","💶","💷","💴","💵"
	};
	// Потокобезопасный генератор случайных чисел (инициализируется один раз на поток)
	static thread_local std::mt19937_64 engine(static_cast <uint64_t> (this->timestamp <uint64_t> (chrono_t::NANOSECONDS)) ^ static_cast <uint64_t> (reinterpret_cast <uintptr_t> (&engine)));
	// Получаем список иконок в зависимости от флага завершения работы
	const vector <string> & icons = (!end ? iconBegin : iconEnd);
	// Создаём равномерное распределение по индексам списка
	std::uniform_int_distribution <size_t> distribution(0, icons.size() - 1);
	// Получаем иконку
	return icons[distribution(engine)];
}
/**
 * @brief Метод получения размера в байтах из строки
 *
 * @param str строка обозначения размерности (b, Kb, Mb, Gb, Tb)
 * @return    размер в байтах
 *
 */
double awh::Framework::bytes(const string_view str) const noexcept {
	// Размер количество байт
	double result = 0.;
	// Если строка передана и начинается с цифры
	if(!str.empty() && ascii::isDigit(str[0])){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Начало и конец позиции значения в строке
			size_t start = 0, stop = 0;
			// Признак обнаружения разделителя дробной части числа
			bool fraction = false;
			/**
			 * Выполняем парсинг строки
			 */
			for(size_t i = 0; i < str.size(); i++){
				// Если текущий символ является пробельным
				if(ascii::isSpace(str[i])){
					// Если позиция конца значения не установлена
					if(stop == 0)
						// Устанавливаем позицию конца значения
						stop = i;
					// Устанавливаем позицию начала значения
					else start = (i + 1);
				// Если текущий символ не является цифрой
				} else if(!ascii::isDigit(str[i])) {
					/**
					 * Если очередной символ является разделителем дробной части числа
					 *
					 * @details Разделитель принимается за часть числа, а не за начало
					 *          обозначения размерности: запись вида «1.5 Mb» задаёт
					 *          полтора мегабайта, а не один.
					 *
					 */
					if((str[i] == '.') && !fraction && (stop == 0) &&
					 ((i + 1) < str.size()) && ascii::isDigit(str[i + 1])) {
						// Запоминаем обнаружение разделителя дробной части числа
						fraction = true;
						// Переходим к следующему символу записи
						continue;
					}
					// Если установлена позиция конца значения
					if(stop > 0)
						// Получаем значение рзамерности данных
						result = this->atoi <double> (str.substr(0, stop));
					// Если позиция конца значения не установлена, извлекаем значение рзамерности данных до текущей позиции
					else result = this->atoi <double> (str.substr(0, i));
					// Обозначение рзамерности данных
					string_view handle = "";
					// Если позиция начала значения установлена
					if(start > 0)
						// Извлекаем обозначение рзамерности данных от позиции начала значения до конца строки
						handle = str.substr(start);
					// Если позиция начала значения не установлена, извлекаем обозначение рзамерности данных от текущей позиции до конца строки
					else handle = str.substr(i);
					// Размерность объема данных
					double dimension = 1.;
					// Если это размерность в килобайтах
					if(this->compare("Kb", handle))
						// Выполняем установку множителя
						dimension = 1024.;
					// Если это размерность в мегабайтах
					else if(this->compare("Mb", handle))
						// Выполняем установку множителя
						dimension = 1048576.;
					// Если это размерность в гигабайтах
					else if(this->compare("Gb", handle))
						// Выполняем установку множителя
						dimension = 1073741824.;
					// Если это размерность в терабайтах
					else if(this->compare("Tb", handle))
						// Выполняем установку множителя
						dimension = 1099511627776.;
					// Если это байты
					else if(this->compare("b", handle) || this->compare("bytes", handle))
						// Выполняем установку множителя
						dimension = 1.;
					// Применяем множитель размерности к полученному значению
					result *= dimension;
					// Выходим из цикла
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод конвертации байт в строку
 *
 * @param value   количество байт
 * @param onlyNum выводить только числа
 * @return        полученная строка
 *
 */
string awh::Framework::bytes(const double value, const bool onlyNum) const noexcept {
	// Переменная результата
	string result = "0 bytes";
	// Если количество байт передано
	if(value > 0.){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Шаблон киллобайта
			const double kb = 1024.;
			// Шаблон мегабайта
			const double mb = 1048576.;
			// Шаблон гигабайта
			const double gb = 1073741824.;
			// Шаблон терабайта
			const double tb = 1099511627776.;
			// Если переданное значение соответствует терабайту
			if(value >= tb){
				// Выполняем копирование терабайта
				result = this->noexp(value / tb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Tb");
			// Если переданное значение соответствует гигабайту
			} else if((value >= gb) && (value < tb)) {
				// Выполняем копирование гигабайта
				result = this->noexp(value / gb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Gb");
			// Если переданное значение соответствует мегабайту
			} else if((value >= mb) && (value < gb)) {
				// Выполняем копирование мегабайта
				result = this->noexp(value / mb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Mb");
			// Если переданное значение соответствует киллобайту
			} else if((value >= kb) && (value < mb)) {
				// Выполняем копирование килобайта
				result = this->noexp(value / kb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Kb");
			// Если переданное значение соответствует байту
			} else {
				// Выполняем копирование байтов
				result = this->noexp(value, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" bytes");
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value, onlyNum), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения количества байт в секунду из строки
 *
 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
 * @return    количество байт в секунду
 *
 */
size_t awh::Framework::bpsSize(const string_view str) const noexcept {
	// Переменная результата
	size_t result = 0;
	// Если строка передана и начинается с цифры
	if(!str.empty() && ascii::isDigit(str[0])){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Начало и конец позиции значения в строке
			size_t start = 0, stop = 0;
			// Признак обнаружения разделителя дробной части числа
			bool fraction = false;
			/**
			 * Выполняем парсинг строки
			 */
			for(size_t i = 0; i < str.size(); i++){
				// Если текущий символ является пробельным
				if(ascii::isSpace(str[i])){
					// Если позиция конца значения не установлена
					if(stop == 0)
						// Устанавливаем позицию конца значения
						stop = i;
					// Устанавливаем позицию начала значения
					else start = (i + 1);
				// Если текущий символ не является цифрой
				} else if(!ascii::isDigit(str[i])) {
					/**
					 * Если очередной символ является разделителем дробной части числа
					 *
					 * @details Разделитель принимается за часть числа, а не за начало
					 *          обозначения размерности: запись вида «1.5 Mb» задаёт
					 *          полтора мегабайта, а не один.
					 *
					 */
					if((str[i] == '.') && !fraction && (stop == 0) &&
					 ((i + 1) < str.size()) && ascii::isDigit(str[i + 1])) {
						// Запоминаем обнаружение разделителя дробной части числа
						fraction = true;
						// Переходим к следующему символу записи
						continue;
					}
					// Значение скорости
					float speed = .0f;
					// Если установлена позиция конца значения
					if(stop > 0)
						// Получаем значение скорости
						speed = this->atoi <float> (str.substr(0, stop));
					// Если позиция конца значения не установлена, извлекаем значение скорости до текущей позиции
					else speed = this->atoi <float> (str.substr(0, i));
					// Обозначение размерности скорости
					string_view handle = "";
					// Если позиция начала значения установлена
					if(start > 0)
						// Извлекаем обозначение размерности скорости от позиции начала значения до конца строки
						handle = str.substr(start);
					// Если позиция начала значения не установлена, извлекаем обозначение размерности скорости от текущей позиции до конца строки
					else handle = str.substr(i);
					// Размерность скорости
					float dimension = .0f;
					// Если это биты
					if(this->compare("bps", handle))
						// Выполняем установку множителя
						dimension = 1.f;
					// Если это размерность в киллобитах
					else if(this->compare("kbps", handle))
						// Выполняем установку множителя
						dimension = 1000.f;
					// Если это размерность в мегабитах
					else if(this->compare("Mbps", handle))
						// Выполняем установку множителя
						dimension = 1000000.f;
					// Если это размерность в гигабитах
					else if(this->compare("Gbps", handle))
						// Выполняем установку множителя
						dimension = 1000000000.f;
					// Выполняем получение размера в байтах
					result = static_cast <size_t> ((speed * dimension) / 8.f);
					// Выходим из цикла
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения размера буфера в байтах
 *
 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
 * @return    размер буфера в байтах
 *
 */
size_t awh::Framework::bpsBuffer(const string_view str) const noexcept {
	/**
	 * Readme - http://www.securitylab.ru/analytics/243414.php
	 *
	 * Example: 17520 Байт / .04 секунды = .44 МБ/сек = 3.5 Мб/сек
	 * Description: Пропускная способность = размер буфера / задержка
	 *
	 * 1. Количество байт в киллобайте: 1024
	 * 2. Количество байт в мегабайте: 1024000
	 * 3. Количество байт в гигабайте: 1024000000
	 *
	 * Размер буфера: 65536
	 * Задержка сети: .04
	 * Количество бит в байте: 8
	 *
	 * 65536 / .04 / 1024000 = 1.6 (МБ/сек) * 8 = 13 Мб/сек
	 *
	 * Получение размера буфера
	 * (13 / 8) * (1024000 * .04) = 66560
	 *
	 */
	// Переменная результата
	size_t result = 0;
	// Если строка передана и начинается с цифры
	if(!str.empty() && ascii::isDigit(str[0])){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Начало и конец позиции значения в строке
			size_t start = 0, stop = 0;
			// Признак обнаружения разделителя дробной части числа
			bool fraction = false;
			/**
			 * Выполняем парсинг строки
			 */
			for(size_t i = 0; i < str.size(); i++){
				// Если текущий символ является пробельным
				if(ascii::isSpace(str[i])){
					// Если позиция конца значения не установлена
					if(stop == 0)
						// Устанавливаем позицию конца значения
						stop = i;
					// Устанавливаем позицию начала значения
					else start = (i + 1);
				// Если текущий символ не является цифрой
				} else if(!ascii::isDigit(str[i])) {
					/**
					 * Если очередной символ является разделителем дробной части числа
					 *
					 * @details Разделитель принимается за часть числа, а не за начало
					 *          обозначения размерности: запись вида «1.5 Mb» задаёт
					 *          полтора мегабайта, а не один.
					 *
					 */
					if((str[i] == '.') && !fraction && (stop == 0) &&
					 ((i + 1) < str.size()) && ascii::isDigit(str[i + 1])) {
						// Запоминаем обнаружение разделителя дробной части числа
						fraction = true;
						// Переходим к следующему символу записи
						continue;
					}
					// Значение скорости
					float speed = .0f;
					// Если установлена позиция конца значения
					if(stop > 0)
						// Получаем значение скорости
						speed = this->atoi <float> (str.substr(0, stop));
					// Если позиция конца значения не установлена, извлекаем значение скорости до текущей позиции
					else speed = this->atoi <float> (str.substr(0, i));
					// Обозначение размерности скорости
					string_view handle = "";
					// Если позиция начала значения установлена
					if(start > 0)
						// Извлекаем обозначение размерности скорости от позиции начала значения до конца строки
						handle = str.substr(start);
					// Если позиция начала значения не установлена, извлекаем обозначение размерности скорости от текущей позиции до конца строки
					else handle = str.substr(i);
					// Размерность скорости
					float dimension = .0f;
					// Проверяем являются ли переданные данные байтами (8, 16, 32, 64, 128, 256, 512, 1024 ...)
					const bool bytes = !::fmod(speed / 8.f, 2.f);
					// Если это биты
					if(this->compare("bps", handle))
						// Выполняем установку множителя
						dimension = 1.f;
					// Если это размерность в киллобитах
					else if(this->compare("kbps", handle))
						// Выполняем установку множителя
						dimension = (bytes ? 1000.f : 1024.f);
					// Если это размерность в мегабитах
					else if(this->compare("Mbps", handle))
						// Выполняем установку множителя
						dimension = (bytes ? 1000000.f : 1024000.f);
					// Если это размерность в гигабитах
					else if(this->compare("Gbps", handle))
						// Выполняем установку множителя
						dimension = (bytes ? 1000000000.f : 1024000000.f);
					// Выполняем получение размера в байтах
					result = static_cast <size_t> ((speed / 8.f) * (dimension * .04f));
					// Выходим из цикла
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(str), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 *
 */
void awh::Framework::setLogger(const log_t * log) noexcept {
	// Выполняем установку объекта логирования
	this->_log = log;
	// Выполняем установку логирования в nwt модуль
	this->_nwt.setLogger(log);
}
/**
 * @brief Конструктор
 *
 */
awh::Framework::Framework() noexcept : _nwt(nullptr), _log(nullptr) {
	// Устанавливаем локализацию системы
	this->setLocale();
}
/**
 * @brief Конструктор
 *
 * @param locale локализация приложения
 *
 */
awh::Framework::Framework(const string & locale) noexcept : _nwt(nullptr), _log(nullptr) {
	// Устанавливаем локализацию системы
	this->setLocale(locale);
}
