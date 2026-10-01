/**
 * @file range.cpp
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
 * @brief Проверки просеивания начальных байтов по непрерывному диапазону
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
#include <utility>
#include <vector>

/**
 * Если доступны защищённые страницы POSIX
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <sys/mman.h>
	#include <unistd.h>
#endif

/**
 * Подключаем заголовочные файлы проекта и тестового окружения
 */
#include <regex/engine.hpp>
#include <regex/codegen.hpp>
#include "../main.hpp"

/**
 * Используем стандартное пространство имён и пространство имён проекта
 */
using namespace std;
using namespace awh;

/**
 * @brief Проверка границ диапазона, разрывов набора и записи порождённого кода
 *
 * @details Ожидаемые границы задаются расположением пяти одинаковых байтов,
 *          а не вторым исполнителем. Перед ними помещается приманка из двух
 *          допустимых байтов: после её отказа просеивание должно продолжиться.
 *          Каждый байт проверяется также с началом поиска внутри совпадения.
 *          Набор с разрывом запрещает заменять таблицу одним его участком.
 *          Границы 0 и 255 проверяют вычитание без учёта знака и ширину 255.
 *
 */
TEST(Regex, CodegenRangeSifting) {
	/**
	 * Если порождение машинного кода недоступно
	 */
	if(!regex::emitter_t::available() || !regex::assembly_t::available())
		// Пропускаем проверку недоступного исполнителя
		GTEST_SKIP() << "кодогенерация сборкой не поддерживается";
	/**
	 * @brief Набор выражений с известным множеством допустимых байтов
	 */
	static const struct {
		// Текст выражения
		const char * pattern;
		// Нижняя и верхняя границы первого участка включительно
		uint16_t lower, upper;
		// Дополнительный байт вне первого участка, 256 означает его отсутствие
		uint16_t extra;
		// Байт заполнения, выражению не принадлежащий
		uint8_t filler;
		// Признак сопоставления без учёта регистра
		bool caseless;
	} SAMPLES[] = {
		{"[0-9]{3,5}",                 48,  57, 256,  33, false},
		{"[\\x00-\\x0F]{3,5}",          0,  15, 256, 255, false},
		{"[\\xF0-\\xFF]{3,5}",        240, 255, 256,   0, false},
		{"[\\x00-\\xFE]{3,5}",          0, 254, 256, 255, false},
		{"[\\x01-\\xFF]{3,5}",          1, 255, 256,   0, false},
		{"[0-8A]{3,5}",                48,  56,  65,  33, false},
		{"[a-z]{3,5}",                 97, 122, 256,  33, false},
		{"[a-z]{3,5}",                 97, 122, 256,  33, true}
	};
	// Количество проверенных сочетаний текста и начала поиска
	size_t checked = 0;
	/**
	 * Выполняем обход выражений набора
	 */
	for(const auto & sample : SAMPLES){
		// Создаём движок и программу выражения
		regex::engine_t engine;
		regex::expression_t expression;
		// Получаем признаки сборки выражения
		const uint32_t flags = (sample.caseless ? static_cast <uint32_t> (regex::flag_t::CASELESS) : 0);
		// Проверяем успешную сборку программы
		ASSERT_TRUE(engine.build(sample.pattern, flags, expression)) << sample.pattern;
		// Создаём порождённый сопоставитель
		regex::codegen_t codegen;
		// Проверяем наличие машинного кода и выбора просеивания
		ASSERT_TRUE(codegen.compile(expression.forward)) << sample.pattern;
		ASSERT_EQ(codegen.filter(), regex::filter_t::SIFTING) << sample.pattern;
		// Создаём запись порождённого сопоставителя
		string record;
		// Проверяем сохранение порождённого кода
		ASSERT_TRUE(codegen.save(record)) << sample.pattern;
		// Создаём сопоставитель для восстановления записи
		regex::codegen_t restored;
		// Смещение чтения записи
		size_t offset = 0;
		// Проверяем восстановление всего сохранённого кода
		ASSERT_TRUE(restored.restore(record, offset, expression.forward)) << sample.pattern;
		ASSERT_EQ(offset, record.size());
		/**
		 * Выполняем обход удалений первой попытки сопоставления
		 */
		for(const size_t prefix : {0, 1, 15, 16, 31, 32, 43, 64}){
			/**
			 * Выполняем обход всех значений байта
			 */
			for(uint16_t value = 0; value < 256; value++){
				// Получаем ожидаемую принадлежность проверяемого байта
				const bool member = (((value >= sample.lower) && (value <= sample.upper)) ||
				 (value == sample.extra) || (sample.caseless && (value >= 'A') && (value <= 'Z')));
				// Размещаем участок без кандидатов
				string text(prefix, static_cast <char> (sample.filler));
				// Добавляем неполное совпадение и разделитель
				text.append(2, static_cast <char> (sample.lower)).append(1, static_cast <char> (sample.filler));
				// Запоминаем начало проверяемого совпадения
				const size_t begin = text.size();
				// Добавляем пять проверяемых байтов и завершающий разделитель
				text.append(5, static_cast <char> (value)).append(1, static_cast <char> (sample.filler));
				/**
				 * Выполняем обход начал поиска, включая конец текста
				 */
				for(const size_t start : {static_cast <size_t> (0), prefix, begin, begin + 1, begin + 2, begin + 3, text.size()}){
					// Получаем ожидаемый вердикт с учётом оставшейся длины совпадения
					const bool expected = (member && (start <= (begin + 2)));
					/**
					 * Проверяем исходный и восстановленный сопоставители
					 */
					for(const auto * matcher : {&codegen, &restored}){
						// Набор границ обнаруженного совпадения
						vector <pair <size_t, size_t>> bounds;
						// Проверяем вердикт сопоставления
						ASSERT_EQ(matcher->exec(text, start, bounds), expected)
						 << sample.pattern << " byte=" << value << " prefix=" << prefix << " start=" << start;
						/**
						 * Если совпадение ожидается
						 */
						if(expected){
							// Проверяем количество и точность границ совпадения
							ASSERT_EQ(bounds.size(), static_cast <size_t> (1));
							EXPECT_EQ(bounds.front().first, ((start > begin) ? start : begin));
							EXPECT_EQ(bounds.front().second, begin + 5);
						}
						// Увеличиваем количество проверенных сочетаний
						checked++;
					}
				}
			}
		}
	}
	// Проверяем полный охват набора
	ASSERT_EQ(checked, static_cast <size_t> (8 * 8 * 256 * 7 * 2));
}

/**
 * @brief Проверка границ чтения порождённого поиска диапазона
 *
 * @details Текст примыкает к недоступной странице с каждой стороны.
 *          Проверяются все длины до шестнадцати векторов и все положения
 *          полного совпадения. Два допустимых байта в конце дают кандидата,
 *          но не совпадение: повторный вход в поиск обязан закончиться
 *          до чтения следующей страницы. Нулевая длина проверяется также
 *          с указателем на недоступную страницу.
 *
 */
TEST(Regex, CodegenRangeGuardPages) {
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если порождение машинного кода недоступно, проверка неприменима
		if(!regex::emitter_t::available() || !regex::assembly_t::available())
			// Пропускаем проверку недоступного исполнителя
			GTEST_SKIP() << "кодогенерация сборкой не поддерживается";
		// Получаем размер страницы памяти
		const long page = ::sysconf(_SC_PAGESIZE);
		// Проверяем возможность разместить текст в одной странице
		ASSERT_GE(page, 256);
		// Получаем размер трёх страниц, включая защитные
		const size_t length = (static_cast <size_t> (page) * 3);
		// Размещаем три недоступные страницы
		void * allocated = ::mmap(nullptr, length, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
		// Проверяем успешность выделения памяти
		ASSERT_NE(allocated, MAP_FAILED);
		/**
		 * @brief Функция освобождения страниц при любом выходе из проверки
		 *
		 * @param address начало участка памяти
		 *
		 */
		const auto release = [length](char * address) noexcept -> void {
			// Освобождаем все страницы участка
			::munmap(address, length);
		};
		// Закрепляем освобождение памяти, включая выход из ASSERT
		unique_ptr <char, decltype(release)> storage(static_cast <char *> (allocated), release);
		// Получаем начало средней страницы
		char * middle = (storage.get() + page);
		// Разрешаем чтение и запись только в средней странице
		ASSERT_EQ(::mprotect(middle, static_cast <size_t> (page), PROT_READ | PROT_WRITE), 0);
		/**
		 * @brief Набор диапазонов с границами в обеих половинах байта
		 */
		static const struct {
			// Текст выражения
			const char * pattern;
			// Допустимый байт и байт заполнения
			uint8_t letter, filler;
		} SAMPLES[] = {
			{"[0-9]{3,5}",          48,  33},
			{"[\\x00-\\x0F]{3,5}",   0, 255},
			{"[\\xF0-\\xFF]{3,5}", 255,   0},
			{"[\\x00-\\xFE]{3,5}", 254, 255},
			{"[\\x01-\\xFF]{3,5}", 255,   0}
		};
		/**
		 * Выполняем обход диапазонов набора
		 */
		for(const auto & sample : SAMPLES){
			// Создаём движок и программу выражения
			regex::engine_t engine;
			regex::expression_t expression;
			// Проверяем успешную сборку программы
			ASSERT_TRUE(engine.build(sample.pattern, 0, expression)) << sample.pattern;
			// Создаём порождённый сопоставитель
			regex::codegen_t codegen;
			// Проверяем наличие машинного кода
			ASSERT_TRUE(codegen.compile(expression.forward)) << sample.pattern;
			// Набор границ обнаруженного совпадения
			vector <pair <size_t, size_t>> bounds;
			/**
			 * Выполняем обход длин участка
			 */
			for(size_t size = 0; size <= 256; size++){
				/**
				 * Проверяем левую и правую границы участка
				 */
				for(size_t side = 0; side < 2; side++){
					// Получаем начало текста у выбранной границы
					char * text = ((side == 0) ? middle : (middle + page - size));
					// Заполняем участок недопустимым байтом
					::memset(text, sample.filler, size);
					// Проверяем отсутствие совпадения
					ASSERT_FALSE(codegen.exec(string_view(text, size), 0, bounds));
					/**
					 * Перебираем каждое положение трёх допустимых байтов
					 */
					for(size_t pos = 0; (pos + 3) <= size; pos++){
						// Размещаем совпадение минимальной длины
						::memset(text + pos, sample.letter, 3);
						// Проверяем обнаружение совпадения
						ASSERT_TRUE(codegen.exec(string_view(text, size), 0, bounds))
						 << sample.pattern << " size=" << size << " side=" << side << " pos=" << pos;
						// Проверяем точность установленных границ
						ASSERT_EQ(bounds.size(), static_cast <size_t> (1));
						ASSERT_EQ(bounds.front().first, pos);
						ASSERT_EQ(bounds.front().second, pos + 3);
						// Восстанавливаем исходное заполнение участка
						::memset(text + pos, sample.filler, 3);
					}
					/**
					 * Если участок вмещает неполное совпадение
					 */
					if(size >= 2){
						// Размещаем приманку вплотную к концу участка
						::memset(text + size - 2, sample.letter, 2);
						// Проверяем отказ без выхода за границу участка
						ASSERT_FALSE(codegen.exec(string_view(text, size), 0, bounds));
					}
				}
			}
		}
	#else
		// Защита страниц POSIX в этой системе недоступна
		GTEST_SKIP() << "POSIX guard pages are unavailable";
	#endif
}

/**
 * @brief Проверка точности отбора диапазона независимо от тела выражения
 *
 * @details Тело выражения отвергло бы лишнего кандидата и скрыло бы ошибку
 *          отбора. Здесь исполняется сам размещённый поиск: его ответ обязан
 *          называть первый допустимый байт либо отсутствие такого байта.
 *          Проверяются границы знакового сравнения, ширины вокруг 128,
 *          весь диапазон и начало поиска за концом участка.
 *
 */
TEST(Regex, EmitterRangeCandidates) {
	#if defined(__x86_64__) || defined(_M_X64)
		// Если исполняемая память недоступна, проверка неприменима
		if(!regex::assembly_t::available())
			// Пропускаем проверку недоступного исполнителя
			GTEST_SKIP() << "исполняемая память недоступна";
		// Тип регистра порождённого кода
		typedef regex::emitter_t::reg_t reg_t;
		// Договор вызова порождённого сопоставителя
		typedef bool (* matcher_t) (const char *, size_t, size_t, size_t *, const void *);
		// Количество проверенных диапазонов
		size_t checked = 0;
		/**
		 * Перебираем границы в обеих половинах байтового пространства
		 */
		for(const uint16_t lower : {0, 1, 47, 48, 127, 128, 239, 240, 255}){
			/**
			 * Перебираем ширины диапазона, включая полный
			 */
			for(const uint16_t width : {1, 8, 9, 10, 16, 127, 128, 129, 255, 256}){
				// Если диапазон выходит из байтового пространства, пропускаем его
				if((lower + width) > 256)
					// Переходим к следующей ширине
					continue;
				// Получаем верхнюю границу включительно
				const uint16_t upper = static_cast <uint16_t> (lower + width - 1);
				// Создаём порождатель машинных команд
				regex::emitter_t emitter;
				// Заводим метки двух результатов поиска
				const size_t ready = emitter.label(), empty = emitter.label();
				// Размещаем вход с сохранением регистров соглашения вызова
				emitter.prologue(0);
				// Устанавливаем начало поиска из переданного довода
				emitter.move(reg_t::KEEPER, reg_t::START);
				// Размещаем проверяемый поиск диапазона
				emitter.ranging(static_cast <uint8_t> (lower), static_cast <uint8_t> (upper), ready, empty);
				// Размещаем выдачу найденной позиции
				emitter.place(ready);
				emitter.store(reg_t::KEEPER, reg_t::BOUNDS, 0);
				emitter.move(reg_t::RESULT, 1);
				emitter.epilogue(0);
				emitter.ret();
				// Размещаем выдачу отсутствия кандидата
				emitter.place(empty);
				emitter.move(reg_t::RESULT, 0);
				emitter.epilogue(0);
				emitter.ret();
				// Проверяем разрешение всех переходов
				ASSERT_TRUE(emitter.resolve());
				ASSERT_FALSE(emitter.failed());
				// Размещаем и разрешаем исполнение полученного кода
				regex::assembly_t assembly;
				ASSERT_TRUE(assembly.allocate(emitter.length()));
				ASSERT_TRUE(assembly.fill(emitter.code().data(), emitter.length()));
				ASSERT_TRUE(assembly.commit());
				// Получаем адрес исполняемой функции
				const matcher_t matcher = reinterpret_cast <matcher_t> (const_cast <void *> (assembly.entry()));
				// Выбираем недопустимое заполнение, если такой байт существует
				const char filler = static_cast <char> ((lower > 0) ? (lower - 1) : ((upper + 1) & 0xFF));
				/**
				 * Проверяем каждую позицию трёх векторов
				 */
				for(size_t pos = 0; pos < 48; pos++){
					/**
					 * Проверяем все значения байта в выбранной позиции
					 */
					for(uint16_t value = 0; value < 256; value++){
						// Размещаем проверяемый байт среди заполнения
						string text(64, filler);
						text[pos] = static_cast <char> (value);
						/**
						 * Проверяем поиск до кандидата, после него и вне текста
						 */
						for(const size_t start : {static_cast <size_t> (0), pos, pos + 1, text.size(), text.size() + 1}){
							// Получаем ожидаемую позицию непосредственно из устройства текста
							const size_t expected = ((start >= text.size()) ? string::npos : ((width == 256) ? start :
							 (((value >= lower) && (value <= upper) && (start <= pos)) ? pos : string::npos)));
							// Начальное значение результата должно сохраниться при отсутствии кандидата
							size_t found = string::npos;
							// Проверяем вердикт и точную позицию без последующего сопоставления
							ASSERT_EQ(matcher(text.data(), text.size(), start, &found, nullptr), (expected != string::npos))
							 << "lower=" << lower << " width=" << width << " byte=" << value << " pos=" << pos << " start=" << start;
							ASSERT_EQ(found, expected);
						}
					}
				}
				// Увеличиваем количество проверенных диапазонов
				checked++;
			}
		}
		// Проверяем, что набор охватил более пятидесяти разных диапазонов
		ASSERT_GT(checked, static_cast <size_t> (50));
	#else
		// Этот способ порождения предназначен для набора команд x86-64
		GTEST_SKIP() << "поиск диапазона SSE2 недоступен";
	#endif
}
