/**
 * @file byte.cpp
 * @date 2026-10-01
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
 * @brief Проверки поиска байта и границ чтения памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

/**
 * Если система поддерживает защиту страниц памяти средствами POSIX
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <sys/mman.h>
	#include <unistd.h>
#endif

/**
 * Подключаем заголовочные файлы проекта и тестового окружения
 */
#include <regex/prefilter.hpp>
#include "../main.hpp"

/**
 * Используем стандартное пространство имён и пространство имён проекта
 */
using namespace std;
using namespace awh;

/**
 * @brief Проверка поиска байта на разных смещениях и значениях целого довода
 *
 */
TEST(Regex, ByteSearchValues) {
	// Создаём воспроизводимый источник байтов
	mt19937 random(0x53A175);
	// Создаём участок, включающий место для смещений начала поиска
	vector <uint8_t> storage(4096 + 32);
	/**
	 * Перебираем значения, включая отрицательные и превышающие один байт
	 */
	for(int value = -257; value <= 511; value++){
		/**
		 * Перебираем смещения начала относительно границы вектора
		 */
		for(size_t offset = 0; offset < 32; offset++){
			// Получаем длину очередного участка
			const size_t size = (random() % 4097);
			// Получаем начало очередного участка
			uint8_t * text = (storage.data() + offset);
			/**
			 * Заполняем участок воспроизводимой последовательностью байтов
			 */
			for(size_t pos = 0; pos < size; pos++)
				// Выполняем установку очередного байта
				text[pos] = static_cast <uint8_t> (random());
			// Сличаем адрес первого совпадения с библиотечным поиском
			ASSERT_EQ(regex::findByte(text, value, size), ::memchr(text, value, size))
				<< "value=" << value << " offset=" << offset << " size=" << size;
		}
	}
}

/**
 * @brief Проверка каждой позиции вектора и хвоста у защищённых страниц
 *
 * @details Ожидаемый адрес задаётся положением вставленного байта, без вызова
 *          библиотечного поиска. Левая граница запрещает чтение до участка,
 *          правая — после него. Нулевая длина проверяется и на недоступной
 *          странице; повторное вхождение не должно скрывать первое.
 *
 */
TEST(Regex, ByteSearchGuardPages) {
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем размер страницы памяти
		const long page = ::sysconf(_SC_PAGESIZE);
		// Проверяем возможность разместить проверяемый участок в одной странице
		ASSERT_GE(page, 256);
		// Получаем размер трёх страниц, включая две защитные
		const size_t length = (static_cast <size_t> (page) * 3);
		// Резервируем участок из трёх недоступных страниц
		void * allocated = ::mmap(nullptr, length, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
		// Проверяем успешность выделения страниц
		ASSERT_NE(allocated, MAP_FAILED);
		/**
		 * @brief Функция освобождения страниц при любом выходе из проверки
		 *
		 * @param address начало выделенного участка
		 *
		 */
		const auto release = [length](uint8_t * address) noexcept -> void {
			// Выполняем освобождение всех страниц участка
			::munmap(address, length);
		};
		// Закрепляем освобождение страниц, включая выход из ASSERT
		unique_ptr <uint8_t, decltype(release)> storage(static_cast <uint8_t *> (allocated), release);
		// Получаем начало средней страницы
		uint8_t * middle = (storage.get() + page);
		// Разрешаем чтение и запись только в средней странице
		ASSERT_EQ(::mprotect(middle, static_cast <size_t> (page), PROT_READ | PROT_WRITE), 0);
		/**
		 * Перебираем обычные, старшие и приведённые значения байта
		 */
		for(const int value : {0, 0x80, 0xFF, 0x101, -1}){
			// Получаем байтовое представление искомого значения
			const uint8_t letter = static_cast <uint8_t> (value);
			// Получаем заведомо отличающееся значение заполнения
			const uint8_t filler = static_cast <uint8_t> (letter ^ 0xFF);
			/**
			 * Перебираем все длины в пределах шестнадцати векторов
			 */
			for(size_t size = 0; size <= 256; size++){
				/**
				 * Проверяем примыкание участка к левой и правой защитным страницам
				 */
				for(size_t side = 0; side < 2; side++){
					// Получаем начало участка у выбранной границы
					uint8_t * text = ((side == 0) ? middle : (middle + page - size));
					// Заполняем участок значением, отличающимся от искомого
					::memset(text, filler, size);
					// Проверяем отсутствие искомого байта, включая пустой участок
					ASSERT_EQ(regex::findByte(text, value, size), nullptr);
					/**
					 * Перебираем каждое возможное положение единственного совпадения
					 */
					for(size_t pos = 0; pos < size; pos++){
						// Помещаем искомый байт в проверяемую позицию
						text[pos] = letter;
						// Проверяем точность адреса найденного байта
						ASSERT_EQ(regex::findByte(text, value, size), text + pos)
							<< "value=" << value << " size=" << size << " side=" << side << " pos=" << pos;
						// Восстанавливаем исходное заполнение участка
						text[pos] = filler;
					}
					/**
					 * Если участок вмещает два совпадения
					 */
					if(size > 1){
						// Помещаем совпадение в начало и конец участка
						text[0] = text[size - 1] = letter;
						// Проверяем выбор первого совпадения
						ASSERT_EQ(regex::findByte(text, value, size), text);
					}
				}
			}
		}
	#else
		// Защита страниц POSIX в этой системе недоступна
		GTEST_SKIP() << "POSIX guard pages are unavailable";
	#endif
}
