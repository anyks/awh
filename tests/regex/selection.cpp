/**
 * @file selection.cpp
 * @date 2026-10-02
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки выбора короткого поиска при подготовке JIT
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем поиск и окружение проверок
 */
#include <regex/engine.hpp>
#include <regex/codegen.hpp>
#include "../main.hpp"

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
 * @brief Проверка выбора и первой позиции короткого литерала
 *
 */
TEST(Regex, PrefilterSelection) {
	// Создаём предварительный отбор позиций
	regex::prefilter_t filter;
	/**
	 * Перебираем поддержанные длины и соседние значения
	 */
	for(size_t width = 0; width <= 12; width++){
		// Задаём ведущий двоичный литерал
		filter.leading.assign(width, '\0');
		/**
		 * Наполняем литерал разными байтами
		 */
		for(size_t i = 1; i < width; i++)
			// Получаем очередной двоичный байт
			filter.leading[i] = static_cast <char> ((i * 73) & 0xFF);
		// Отключаем отбор позиций
		filter.active = false;
		// Неактивный отбор не получает специального входа
		EXPECT_EQ(filter.select(), nullptr);
		// Разрешаем отбор позиций
		filter.active = true;
		// Выбираем подпрограмму единожды для этой длины
		const regex::prefilter_t::seeker_t selected = filter.select();
		#if (defined(__x86_64__) || defined(_M_X64)) && !defined(__e2k__) && !defined(AWH_REGEX_SCALAR)
			// Если длина не принадлежит специальному пути
			if((width < 4) || (width > 8)){
				// Сохраняем общий отбор
				EXPECT_EQ(selected, nullptr);
				// Переходим к следующей длине
				continue;
			}
			// Проверяем наличие специального входа
			ASSERT_NE(selected, nullptr);
		#else
			// На остальных наборах команд сохраняется общий отбор
			EXPECT_EQ(selected, nullptr);
			// Переходим к следующей длине
			continue;
		#endif
		// Отбрасываем текст, содержащий только неполный литерал
		const string truncated = filter.leading.substr(0, width - 1);
		EXPECT_EQ(selected(truncated.data(), truncated.size(), 0, &filter), truncated.size());
		// Нарушаем последний байт полного литерала
		string changed = filter.leading;
		changed.back() ^= 1;
		// Проверяем, что выбор меньшей длины не принимает этот префикс
		EXPECT_EQ(selected(changed.data(), changed.size(), 0, &filter), changed.size());
		/**
		 * Проверяем границы векторов, окна и длинного остатка
		 */
		for(const size_t size : {0, 3, 8, 31, 32, 33, 64, 100, 1023, 1024, 1025, 16384}){
			/**
			 * Перебираем все выравнивания начала текста
			 */
			for(size_t shift = 0; shift < 16; shift++){
				// Создаём участок с запасом перед текстом
				string storage(size + shift, 'x');
				// Получаем начало текста
				char * body = (storage.data() + shift);
				/**
				 * Проверяем отсутствие, близкую и дальнюю находки
				 */
				for(size_t mode = 0; mode < 4; mode++){
					// Восстанавливаем исходное наполнение
					::memset(body, 'x', size);
					/**
					 * Наполняем текст ложными кандидатами пары
					 */
					for(size_t at = 0; (at + width) <= size; at += 16){
						// Размещаем полный литерал
						::memcpy(body + at, filter.leading.data(), width);
						// Нарушаем сличение без изменения байтов пары
						body[at + 1] = 'x';
					}
					// Получаем позицию полного совпадения
					const size_t place = ((mode == 1) ? 0 : ((mode == 2) ? (size / 2) : ((size >= width) ? (size - width) : 0)));
					// Если полное совпадение предусмотрено и помещается
					if((mode != 0) && (size >= width) && (place <= (size - width)))
						// Размещаем полный литерал в тексте
						::memcpy(body + place, filter.leading.data(), width);
					// Получаем представление текста
					const string_view text(body, size);
					/**
					 * Проверяем начала до находки, на ней и за пределами текста
					 */
					for(const size_t pos : {static_cast <size_t> (0), static_cast <size_t> (1), place, place + 1, size, size + 1, string_view::npos}){
						// Получаем первую позицию независимым поиском
						const size_t expected = text.find(filter.leading, pos);
						// Проверяем точную позицию и обозначение отсутствия
						ASSERT_EQ(selected(body, size, pos, &filter), ((expected == string_view::npos) ? size : expected))
						 << "width=" << width << " size=" << size << " shift=" << shift << " mode=" << mode << " pos=" << pos;
					}
				}
			}
		}
	}
}

/**
 * @brief Проверка чтения текста у защищённой страницы
 *
 */
TEST(Regex, PrefilterSelectionGuardPages) {
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем размер страницы памяти
		const long page = ::sysconf(_SC_PAGESIZE);
		// Проверяем достаточность страницы
		ASSERT_GE(page, 1024);
		// Отводим одну доступную страницу между недоступными
		const size_t length = (static_cast <size_t> (page) * 3);
		void * allocated = ::mmap(nullptr, length, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
		// Проверяем размещение участка
		ASSERT_NE(allocated, MAP_FAILED);
		/**
		 * @brief Функция освобождения страниц при выходе из проверки
		 *
		 * @param address начало участка памяти
		 *
		 */
		const auto release = [length](char * address) noexcept -> void {
			// Освобождаем весь участок
			::munmap(address, length);
		};
		// Закрепляем освобождение памяти при любом выходе
		unique_ptr <char, decltype(release)> storage(static_cast <char *> (allocated), release);
		// Получаем начало доступной страницы
		char * middle = (storage.get() + page);
		// Разрешаем чтение и запись средней страницы
		ASSERT_EQ(::mprotect(middle, static_cast <size_t> (page), PROT_READ | PROT_WRITE), 0);
		/**
		 * Перебираем длины короткого литерала
		 */
		for(size_t width = 4; width <= 8; width++){
			// Создаём активный отбор по ведущему литералу
			regex::prefilter_t filter;
			filter.active = true;
			filter.leading.assign(width, 'a');
			// Выбираем подпрограмму поиска
			const regex::prefilter_t::seeker_t selected = filter.select();
			// Если специальный проход недоступен на этом наборе команд
			if(selected == nullptr)
				// Завершаем проверку защищённых страниц
				GTEST_SKIP() << "специальный проход недоступен";
			/**
			 * Перебираем длины текста и обе границы страницы
			 */
			for(size_t size = 0; size <= 100; size++){
				for(size_t side = 0; side < 2; side++){
					// Помещаем текст у выбранной границы
					char * body = ((side == 0) ? middle : (middle + page - size));
					// Заполняем текст без совпадений
					::memset(body, 'x', size);
					// Проверяем отсутствие совпадения
					ASSERT_EQ(selected(body, size, 0, &filter), size);
					/**
					 * Проверяем каждое положение полного совпадения
					 */
					for(size_t place = 0; (place + width) <= size; place++){
						// Размещаем полное совпадение
						::memcpy(body + place, filter.leading.data(), width);
						// Проверяем первое совпадение и поиск после него
						ASSERT_EQ(selected(body, size, 0, &filter), place);
						ASSERT_EQ(selected(body, size, place + 1, &filter), size);
						// Убираем полное совпадение
						::memset(body + place, 'x', width);
					}
				}
			}
		}
	#else
		// Пропускаем проверку без защищённых страниц POSIX
		GTEST_SKIP() << "проверка требует защищённых страниц POSIX";
	#endif
}

/**
 * @brief Проверка выбранного поиска при создании и восстановлении JIT
 *
 */
TEST(Regex, CodegenSelectionRestore) {
	// Если порождение машинного кода недоступно
	if(!regex::emitter_t::available() || !regex::assembly_t::available())
		// Пропускаем проверку JIT
		GTEST_SKIP() << "порождение машинного кода недоступно";
	// Создаём движок регулярных выражений
	regex::engine_t engine;
	/**
	 * Перебираем все длины специального прохода
	 */
	for(size_t width = 4; width <= 8; width++){
		// Задаём ведущий литерал без метасимволов
		const string needle = string("abcdefgh").substr(0, width);
		// Создаём выражение с захватом после ведущего литерала
		regex::expression_t expression;
		ASSERT_TRUE(engine.build(needle + "([0-9]+)", static_cast <uint32_t> (regex::flag_t::JIT), expression));
		// Проверяем наличие JIT и точность ведущего литерала
		ASSERT_NE(expression.machine, nullptr);
		ASSERT_EQ(expression.forward.prefilter.leading, needle);
		// Сохраняем порождённый сопоставитель
		string record;
		ASSERT_TRUE(expression.machine->save(record));
		// Восстанавливаем сопоставитель с новой таблицей адресов
		regex::codegen_t restored;
		size_t offset = 0;
		ASSERT_TRUE(restored.restore(record, offset, expression.forward));
		/**
		 * Проверяем близкую и дальнюю находки, включая остаток за окном
		 */
		for(const size_t place : {0, 31, 32, 64, 1023, 1024, 2048}){
			// Создаём текст с единственным полным совпадением
			const string text = string(place, 'x') + needle + "123!";
			/**
			 * Сличаем исходный и восстановленный сопоставители
			 */
			for(const regex::codegen_t * machine : {expression.machine.get(), &restored}){
				// Набор границ совпадения и захваченной группы
				vector <pair <size_t, size_t>> captures;
				// Проверяем сопоставление непосредственно порождённым кодом
				ASSERT_TRUE(machine->exec(text, 0, captures));
				ASSERT_EQ(captures.size(), static_cast <size_t> (2));
				EXPECT_EQ(captures[0].first, place);
				EXPECT_EQ(captures[0].second, place + width + 3);
				EXPECT_EQ(captures[1].first, place + width);
				EXPECT_EQ(captures[1].second, place + width + 3);
				// Проверяем отсутствие другого совпадения за первой позицией
				EXPECT_FALSE(machine->exec(text, place + 1, captures));
			}
		}
	}
}
