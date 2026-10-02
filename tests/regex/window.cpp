/**
 * @file window.cpp
 * @date 2026-10-02
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки оконного поиска коротких литералов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

/**
 * Если доступны защищённые страницы POSIX
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <sys/mman.h>
	#include <unistd.h>
#endif

/**
 * Подключаем поиск и окружение проверок
 */
#include <regex/prefilter.hpp>
#include "../main.hpp"

/**
 * Используем стандартное пространство имён и пространство имён проекта
 */
using namespace std;
using namespace awh;

/**
 * @brief Проверка позиции первого совпадения на границах векторов и слов
 *
 * @details Литералы содержат двоичные байты, включая нуль. Длины от одного
 *          до двенадцати охватывают короткие литералы и соседние размеры.
 *          Эталон не использует отбор кандидатов: это поиск стандартной библиотеки.
 *
 */
TEST(Regex, PrefilterWindowedShort) {
	// Количество выполненных сличений
	size_t checked = 0;
	/**
	 * Перебираем длины искомого
	 */
	for(size_t width = 1; width <= 12; width++){
		// Создаём двоичный литерал
		string needle(width, '\0');
		/**
		 * Наполняем литерал разными байтами
		 */
		for(size_t i = 1; i < width; i++)
			// Получаем очередной байт, включая старшую половину диапазона
			needle[i] = static_cast <char> ((i * 73) & 0xFF);
		/**
		 * Перебираем длины текста
		 */
		for(size_t size = 0; size <= 100; size++){
			/**
			 * Перебираем выравнивание начала текста
			 */
			for(size_t shift = 0; shift < 16; shift++){
				// Создаём текст с запасом перед началом
				string storage(size + shift, static_cast <char> (0xA5));
				// Начало проверяемого участка
				char * body = (storage.data() + shift);
				/**
				 * Проверяем отсутствие, начало, середину и конец совпадения
				 */
				for(size_t mode = 0; mode < 4; mode++){
					// Возвращаем текст к исходному наполнению
					::memset(body, 0xA5, size);
					// Получаем положение вставки
					const size_t place = ((mode == 2) ? (size / 2) : (((mode == 3) && (size >= width)) ? (size - width) : 0));
					// Если вставка предусмотрена и помещается
					if((mode != 0) && (size >= width) && (place <= (size - width)))
						// Размещаем полный литерал
						::memcpy(body + place, needle.data(), width);
					// Получаем представление участка
					const string_view text(body, size);
					/**
					 * Проверяем все положения начала поиска
					 */
					for(size_t start = 0; start <= (size + 1); start++){
						// Счёт перед вызовом должен быть перезаписан
						size_t rejected = string_view::npos;
						// Получаем ожидаемую первую позицию
						const size_t expected = text.find(needle, start);
						// Проверяем точную позицию либо отсутствие совпадения
						ASSERT_EQ(regex::windowed(text, needle, start, rejected), expected)
						 << "width=" << width << " size=" << size << " shift=" << shift << " mode=" << mode << " start=" << start;
						// Увеличиваем счёт сличений
						checked++;
					}
				}
			}
		}
	}
	// Проверяем полный охват положений
	ASSERT_EQ(checked, static_cast <size_t> (12 * 16 * 4 * 5252));
}

/**
 * @brief Проверка сохранения счёта участков с кандидатами
 *
 * @details Ложный кандидат расположен в первом участке, полный — в третьем.
 *          Счёт включает участок найденного совпадения. Хвост не добавляет
 *          участков. Начало поиска проверяет отбрасывание прежних кандидатов.
 *
 */
TEST(Regex, PrefilterWindowedCount) {
	/**
	 * Перебираем длины короткого пути
	 */
	for(size_t width = 4; width <= 8; width++){
		// Создаём литерал с различающимися байтами
		const string needle = string("abcdefgh").substr(0, width);
		// Создаём текст без случайных кандидатов
		string text(128, 'x');
		// Размещаем ложного кандидата
		text.replace(0, width, needle);
		// Разрываем совпадение, сохранив оба байта отбора
		text[1] = 'x';
		// Размещаем полное совпадение
		text.replace(64, width, needle);
		// Счёт участков с кандидатами
		size_t rejected = string_view::npos;
		// Проверяем оба участка
		ASSERT_EQ(regex::windowed(text, needle, 0, rejected), static_cast <size_t> (64));
		EXPECT_EQ(rejected, static_cast <size_t> (2));
		// Проверяем пропуск первого кандидата
		ASSERT_EQ(regex::windowed(text, needle, 1, rejected), static_cast <size_t> (64));
		EXPECT_EQ(rejected, static_cast <size_t> (1));
		// Проверяем сброс счёта после полного совпадения
		ASSERT_EQ(regex::windowed(text, needle, 65, rejected), string_view::npos);
		EXPECT_EQ(rejected, static_cast <size_t> (0));
		// Проверяем начало за пределами текста
		ASSERT_EQ(regex::windowed(text, needle, 129, rejected), string_view::npos);
		EXPECT_EQ(rejected, static_cast <size_t> (0));
		// Проверяем наибольшее возможное начало поиска
		ASSERT_EQ(regex::windowed(text, needle, string_view::npos, rejected), string_view::npos);
		EXPECT_EQ(rejected, static_cast <size_t> (0));
	}
}

/**
 * @brief Проверка границ чтения текста и искомого
 *
 * @details Каждая область окружена недоступными страницами. Проверяются
 *          обе стороны текста и литерала, длины от одного до двенадцати,
 *          все положения совпадения и поиск после него.
 *
 */
TEST(Regex, PrefilterWindowedGuardPages) {
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем размер страницы памяти
		const long page = ::sysconf(_SC_PAGESIZE);
		// Проверяем достаточность страницы
		ASSERT_GE(page, 128);
		// Отводим области текста и литерала с защитными страницами
		const size_t length = (static_cast <size_t> (page) * 6);
		void * allocated = ::mmap(nullptr, length, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
		// Проверяем успешность размещения
		ASSERT_NE(allocated, MAP_FAILED);
		/**
		 * @brief Функция освобождения страниц при любом выходе из проверки
		 *
		 * @param address начало участка памяти
		 *
		 */
		const auto release = [length](char * address) noexcept -> void {
			// Освобождаем весь участок
			::munmap(address, length);
		};
		// Закрепляем освобождение участка
		unique_ptr <char, decltype(release)> storage(static_cast <char *> (allocated), release);
		// Получаем начала доступных страниц
		char * haystack = (storage.get() + page), * literal = (storage.get() + (page * 4));
		// Разрешаем чтение и запись только в этих страницах
		ASSERT_EQ(::mprotect(haystack, static_cast <size_t> (page), PROT_READ | PROT_WRITE), 0);
		ASSERT_EQ(::mprotect(literal, static_cast <size_t> (page), PROT_READ | PROT_WRITE), 0);
		/**
		 * Перебираем длины литерала
		 */
		for(size_t width = 1; width <= 12; width++){
			/**
			 * Перебираем левую и правую границы литерала
			 */
			for(size_t edge = 0; edge < 2; edge++){
				// Получаем начало литерала у выбранной границы
				char * needle = ((edge == 0) ? literal : (literal + page - width));
				// Заполняем литерал одинаковыми байтами
				::memset(needle, 'a', width);
				/**
				 * Перебираем длины текста
				 */
				for(size_t size = 0; size <= 100; size++){
					/**
					 * Перебираем обе границы текста
					 */
					for(size_t side = 0; side < 2; side++){
						// Получаем начало текста у выбранной границы
						char * body = ((side == 0) ? haystack : (haystack + page - size));
						// Возвращаем текст к наполнению без совпадений
						::memset(body, 'x', size);
						// Счёт участков с кандидатами
						size_t rejected = string_view::npos;
						// Проверяем отсутствие совпадения
						ASSERT_EQ(regex::windowed(string_view(body, size), string_view(needle, width), 0, rejected), string_view::npos);
						/**
						 * Перебираем все положения полного совпадения
						 */
						for(size_t place = 0; (place + width) <= size; place++){
							// Размещаем единственное полное совпадение
							::memcpy(body + place, needle, width);
							// Проверяем найденную позицию
							ASSERT_EQ(regex::windowed(string_view(body, size), string_view(needle, width), 0, rejected), place);
							// Проверяем отсутствие полного совпадения после его начала
							ASSERT_EQ(regex::windowed(string_view(body, size), string_view(needle, width), place + 1, rejected), string_view::npos);
							// Восстанавливаем наполнение текста
							::memset(body + place, 'x', width);
						}
					}
				}
			}
		}
	#else
		// Пропускаем проверку при отсутствии защищённых страниц POSIX
		GTEST_SKIP() << "проверка требует защищённых страниц POSIX";
	#endif
}
