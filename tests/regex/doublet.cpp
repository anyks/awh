/**
 * @file doublet.cpp
 * @date 2026-10-02
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки поиска обязательного двухбайтового литерала
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем поиск и окружение проверок
 */
#include <regex/prefilter.hpp>
#include "../main.hpp"

/**
 * Стандартные заголовочные файлы
 */
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
 * Используем стандартное пространство имён и пространство имён проекта
 */
using namespace std;
using namespace awh;

/**
 * @brief Проверка первого вхождения и заданного начала поиска
 *
 * @details Проверяются двоичные пары, одинаковые байты и соседние длины.
 *          Текст содержит редкие и частые первые байты, два вхождения либо
 *          только ложных кандидатов. Эталон — стандартный поиск строки.
 *
 */
TEST(Regex, PrefilterLocateDoublet) {
	// Создаём предварительный отбор позиций
	regex::prefilter_t filter;
	// Число выполненных сличений
	size_t checked = 0;
	/**
	 * Перебираем литералы, включая соседние длины
	 */
	for(const string & needle : {string(), string("/"), string("/1"), string("aa"),
	 string("\0\0", 2), string("\0\xff", 2), string("\xff\0", 2), string("\xff\x80", 2), string("/12")}){
		// Задаём обязательный литерал
		filter.literal = needle;
		/**
		 * Перебираем длины, включающие границы векторов и хвоста
		 */
		for(size_t size = 0; size <= 80; size++){
			/**
			 * Перебираем выравнивание начала текста
			 */
			for(size_t shift = 0; shift < 16; shift++){
				/**
				 * Перебираем редкий и частый первый байт
				 */
				for(size_t frequent = 0; frequent < 2; frequent++){
					// Получаем заполнитель, не дающий случайного совпадения редкого байта
					const char filler = (needle.empty() ? 'x' : static_cast <char> (needle.front() ^ ((frequent == 0) ? 0x55 : 0)));
					// Создаём текст с запасом перед его началом
					string storage(size + shift, filler);
					char * body = (storage.data() + shift);
					/**
					 * Проверяем отсутствие, конец и два полных вхождения
					 */
					for(size_t mode = 0; mode < 3; mode++){
						// Восстанавливаем исходное наполнение
						::memset(body, static_cast <unsigned char> (filler), size);
						// Если вставка предусмотрена и помещается
						if((mode != 0) && !needle.empty() && (needle.size() <= size)){
							// Размещаем вхождение у конца текста
							::memcpy(body + size - needle.size(), needle.data(), needle.size());
							// Если предусмотрено ещё одно вхождение
							if(mode == 2)
								// Размещаем вхождение в начале текста
								::memcpy(body, needle.data(), needle.size());
						}
						// Получаем представление проверяемого участка
						const string_view text(body, size);
						/**
						 * Проверяем все начала поиска и ближайшую недопустимую позицию
						 */
						for(size_t start = 0; start <= (size + 1); start++){
							// Проверяем первую допустимую позицию совпадения
							ASSERT_EQ(filter.locate(text, start), text.find(needle, start))
							 << "width=" << needle.size() << " size=" << size << " shift=" << shift
							 << " frequent=" << frequent << " mode=" << mode << " start=" << start;
							// Учитываем выполненное сличение
							checked++;
						}
						// Проверяем наибольшее начало поиска без переполнения вычислений
						ASSERT_EQ(filter.locate(text, string_view::npos), string_view::npos);
					}
				}
			}
		}
	}
	// Проверяем полноту обхода сочетаний
	ASSERT_EQ(checked, static_cast <size_t> (9 * 16 * 2 * 3 * 3402));
}

/**
 * @brief Проверка чтения у обеих границ доступной страницы
 *
 */
TEST(Regex, PrefilterLocateDoubletGuardPages) {
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем размер страницы системы
		const long page = ::sysconf(_SC_PAGESIZE);
		ASSERT_GE(page, 128);
		// Отводим доступную страницу между двумя защищёнными
		const size_t length = (static_cast <size_t> (page) * 3);
		void * allocated = ::mmap(nullptr, length, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
		ASSERT_NE(allocated, MAP_FAILED);
		/**
		 * @brief Функция освобождения страниц при любом выходе из проверки
		 *
		 * @param address начало выделенной области
		 *
		 */
		const auto release = [length](char * address) noexcept -> void {
			// Освобождаем всю область
			::munmap(address, length);
		};
		// Закрепляем освобождение памяти и открываем только среднюю страницу
		unique_ptr <char, decltype(release)> storage(static_cast <char *> (allocated), release);
		char * region = (storage.get() + page);
		ASSERT_EQ(::mprotect(region, static_cast <size_t> (page), PROT_READ | PROT_WRITE), 0);
		// Создаём предварительный отбор по двоичной паре
		regex::prefilter_t filter;
		filter.literal.assign("\0\xff", 2);
		/**
		 * Перебираем длины текста и обе границы страницы
		 */
		for(size_t size = 0; size <= 100; size++){
			for(size_t side = 0; side < 2; side++){
				// Получаем участок у выбранной границы
				char * body = ((side == 0) ? region : (region + page - size));
				::memset(body, 0, size);
				const string_view text(body, size);
				// Проверяем текст из ложных кандидатов и недопустимое начало
				ASSERT_EQ(filter.locate(text, 0), string_view::npos);
				ASSERT_EQ(filter.locate(text, string_view::npos), string_view::npos);
				/**
				 * Проверяем каждую позицию полного вхождения
				 */
				for(size_t place = 0; (place + 2) <= size; place++){
					// Размещаем полную пару
					body[place + 1] = static_cast <char> (0xFF);
					// Проверяем первую позицию и поиск с этой позиции
					ASSERT_EQ(filter.locate(text, 0), place);
					ASSERT_EQ(filter.locate(text, place), place);
					// Проверяем остаток после найденной пары
					ASSERT_EQ(filter.locate(text, place + 1), string_view::npos);
					// Восстанавливаем ложного кандидата
					body[place + 1] = 0;
				}
			}
		}
	#else
		// Пропускаем проверку при отсутствии защищённых страниц POSIX
		GTEST_SKIP() << "проверка требует защищённых страниц POSIX";
	#endif
}
