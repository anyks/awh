/**
 * @file replace.cpp
 * @date 2026-09-07
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
 * @brief Переносимая подмена целевого файла временным, общая всем кодекам
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Если операционная система является MS Windows
 *
 * @warning Заголовок этот приносит с собою макросы, чьи имена совпадают с именами
 *          перечислений кодеков - `ERROR`, `DELETE`, `TEXT` и прочие, - а препроцессор
 *          областей видимости не разбирает. Замер 07.09.2026: одно лишь включение
 *          `windows.h` в ЗАГОЛОВКЕ развалило сборку кодека JSON под MinGW, обратив
 *          `duplicate_t::ERROR` в число, тогда как разметка при этом собралась и прошла.
 *          Здесь включение заперто одной единицей трансляции и до кодеков не доходит, а
 *          ограда `suppress`/`restore` стоит сторожем на случай, если в этот файл
 *          добавят своё перечисление
 *
 * @note Работает ограда возвратом состояния, бывшего ДО включения, - оттого снимаются и
 *       те определения, что внёс сам `windows.h`
 */
#if defined(_WIN32) || defined(_WIN64)
	#include "../../include/sys/macro/suppress.hpp"
	#include <windows.h>
	#include "../../include/sys/macro/restore.hpp"
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/replace.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Для операционной системы, MS Windows не являющейся
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Инкапсулируем статические функции в пространство имён
	 *
	 */
	namespace {
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
			#if __cpp_lib_string_resize_and_overwrite
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
	};
#endif

/**
 * @brief Функция подмены целевого файла временным
 *
 * @param temporary адрес временного файла записи
 * @param filename  адрес целевого файла записи
 * @return          признак успешной подмены
 *
 */
bool awh::codec::replace(const string & temporary, const string & filename) noexcept {
	/**
	 * Для операционной системы, MS Windows не являющейся
	 */
	#if !_WIN32 && !_WIN64
		// Выполняем подмену целевого файла временным
		return (::rename(temporary.c_str(), filename.c_str()) == 0);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		/**
		 * Выполняем подмену целевого файла временным с заменою на месте
		 *
		 * @note Зовётся узкий вид, а не широкий: пути ходят здесь `std::string`
		 */
		return (::MoveFileExA(temporary.c_str(), filename.c_str(), (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) != 0);
	#endif
}
/**
 * @brief Функция подмены целевого файла временным
 *
 * @param temporary адрес временного файла записи
 * @param filename  адрес целевого файла записи
 * @return          признак успешной подмены
 *
 */
bool awh::codec::replace(const wstring & temporary, const wstring & filename) noexcept {
	/**
	 * Для операционной системы, MS Windows не являющейся
	 */
	#if !_WIN32 && !_WIN64
		// Конвертируем адрес временного файла записи
		const string & first = ::wideToUtf8(temporary.c_str(), temporary.size());
		// Конвертируем адрес целевого файла записи
		const string & second = ::wideToUtf8(filename.c_str(), filename.size());
		// Выполняем подмену целевого файла временным
		return (::rename(first.c_str(), second.c_str()) == 0);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		/**
		 * Выполняем подмену целевого файла временным с заменою на месте
		 *
		 * @note Зовётся узкий вид, а не широкий: пути ходят здесь `std::string`
		 */
		return (::MoveFileExW(temporary.c_str(), filename.c_str(), (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) != 0);
	#endif
}
