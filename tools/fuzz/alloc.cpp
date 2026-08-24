/**
 * @file alloc.cpp
 * @date 2026-08-23
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
 * @brief Ворошитель распределителя памяти — случайная нагрузка с проверкой договора
 *
 * @details Распределитель ворошится НАПРЯМУЮ, минуя захват выдачи памяти процесса.
 *          Сделано это ради санитайзеров: те подменяют `malloc` собою и уступить его
 *          нам не могут, - а связанные с нами обычным порядком, спорят с нами за одно
 *          и то же имя. Признак сборки `AWH_ALLOC_DISABLED` обращает наши входы в
 *          частные (`__awh_alloc_malloc__` и прочие), и ворошитель зовёт их сам,
 *          оставив `malloc` санитайзеру. Учёт же самого ворошителя идёт libc и нашего
 *          состояния не трогает вовсе
 *
 * @note Кэши потоков заводит установщик настроек `Allocator::options`, а не захват:
 *       без него ворошитель ходил бы вырожденным путём центральных списков и мерил
 *       не то, что нужно
 *
 * @warning Ворошитель ОБЯЗАН утверждать, а не просто отрабатывать: молчащий
 *          распределитель прошёл бы прогон, ничего не выдав. Всякий блок засевается
 *          образцом, и образец сличается при возврате
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
/**
 * Заголовок объявления размера выданного блока у системного распределителя
 */
/**
 * Заголовок объявления размера выданного блока
 *
 * Имя и заголовок свои у каждой системы, а у OpenBSD и NetBSD такого метода нет вовсе -
 * и заголовка `malloc.h` там тоже нет. Включать его «всем прочим» нельзя: у OpenBSD
 * сборка валится на первом же включении
 */
#if defined(__APPLE__) || defined(__MACH__)
	#include <malloc/malloc.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
	#include <malloc_np.h>
#elif defined(__linux__) || defined(_WIN32) || defined(_WIN64) || ((defined(__sun__) || defined(__sun) || defined(sun)) && (defined(__SVR4) || defined(__svr4__)))
	#include <malloc.h>
#endif
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>

/**
 * Подключаем заголовочные файлы проекта
 */
#if !defined(AWH_FUZZ_SYSTEM)
	#include <sys/log.hpp>
#endif

/**
 * Подставляем пустой журнал
 *
 * Библиотека целиком ворошителю не нужна, а распределитель зовёт журнал на редких
 * путях отказа: подставляем пустое тело вместо связывания со всем деревом
 */
#if !defined(AWH_FUZZ_SYSTEM)
	void awh::Logging::print(std::string_view, flag_t, ...) const noexcept {}
#endif

/**
 * Включаем файл кода распределителя в себя
 *
 * Иначе частные входы, заводимые признаком `AWH_ALLOC_DISABLED`, ворошителю не видны:
 * они статические
 */
#if !defined(AWH_FUZZ_SYSTEM)
	#include "../../src/alloc/alloc.cpp"
#endif

/**
 * Имена входов распределителя
 *
 * Ворошитель ходит двумя путями, и оба нужны. При сборке с `AWH_ALLOC_DISABLED` наши
 * входы частные, `malloc` остаётся за системой - так ворошителя можно вести под
 * санитайзерами. Без этого признака у систем ELF наши входы и ЕСТЬ `malloc`: то
 * настоящая поставляемая сборка, и её ворошить надо тоже - иначе проверялось бы не то,
 * что работает у потребителя
 */
#if defined(AWH_FUZZ_SYSTEM)
	/**
	 * Нагрузка идёт СИСТЕМНОМУ распределителю
	 *
	 * Ход этот нужен не ради замера, а ради разбора находок: нашедши расхождение, надо
	 * прежде всего узнать, чьё оно - модуля или самого ворошителя. Системный
	 * распределитель заведомо исправен, и расхождение,явившееся и на нём, принадлежит
	 * ворошителю. Разница между ходами ОДНА - чей распределитель обслуживает выдачу
	 */
	#define AWH_FUZZ_MALLOC   ::malloc
	#define AWH_FUZZ_CALLOC   ::calloc
	#define AWH_FUZZ_REALLOC  ::realloc
	#define AWH_FUZZ_FREE     ::free
	#if defined(__APPLE__) || defined(__MACH__)
		#define AWH_FUZZ_MSIZE ::malloc_size
	#elif defined(_WIN32) || defined(_WIN64)
		#define AWH_FUZZ_MSIZE ::_msize
	#else
		#define AWH_FUZZ_MSIZE ::malloc_usable_size
	#endif
	#define AWH_FUZZ_MEMALIGN(align, size) ::aligned_alloc(align, (((size) + (align) - 1) / (align)) * (align))
#elif defined(AWH_ALLOC_DISABLED)
	#define AWH_FUZZ_MALLOC   __awh_alloc_malloc__
	#define AWH_FUZZ_CALLOC   __awh_alloc_calloc__
	#define AWH_FUZZ_REALLOC  __awh_alloc_realloc__
	#define AWH_FUZZ_FREE     __awh_alloc_free__
	#define AWH_FUZZ_MSIZE    __awh_alloc_msize__
	#define AWH_FUZZ_MEMALIGN __awh_alloc_memalign__
#else
	#define AWH_FUZZ_MALLOC   ::malloc
	#define AWH_FUZZ_CALLOC   ::calloc
	#define AWH_FUZZ_REALLOC  ::realloc
	#define AWH_FUZZ_FREE     ::free
	/**
	 * Имя размера блока своё у каждой системы
	 *
	 * У macOS и MS Windows входы наши частные, и размер спрашивается у частного же
	 * имени. У систем ELF подмена именами отдаёт нам системные имена, и спрашивать надо
	 * ИХ - тем самым проверяя заодно, что модуль их вправду выставил: `malloc_usable_size`
	 * у Linux и Solaris, `malloc_size` у FreeBSD. У OpenBSD и NetBSD такого имени нет
	 * вовсе, и размер там не проверяется - это свойство системы, а не пропуск
	 */
	#if defined(__APPLE__) || defined(__MACH__) || defined(_WIN32) || defined(_WIN64)
		#define AWH_FUZZ_MSIZE __awh_alloc_msize__
	#elif defined(__linux__) || ((defined(__sun__) || defined(__sun) || defined(sun)) && (defined(__SVR4) || defined(__svr4__)))
		#define AWH_FUZZ_MSIZE ::malloc_usable_size
	#elif defined(__FreeBSD__) || defined(__DragonFly__)
		#define AWH_FUZZ_MSIZE ::malloc_size
	#endif
	#if defined(__APPLE__) || defined(__MACH__) || defined(_WIN32) || defined(_WIN64)
		#define AWH_FUZZ_MEMALIGN __awh_alloc_memalign__
	#else
		#define AWH_FUZZ_MEMALIGN ::memalign
	#endif
#endif

/**
 * @brief Инкапсулируем работу ворошителя в собственное пространство имён
 *
 */
namespace {
	// Признак найденного расхождения
	std::atomic <bool> broken(false);
	// Зерно прогона, печатаемое при расхождении
	uint64_t seed = 0;
	// Число выданных блоков за прогон
	std::atomic <uint64_t> served(0);
	// Число возвращённых блоков за прогон
	std::atomic <uint64_t> returned(0);
	// Число блоков, отданных на возврат соседнему потоку
	std::atomic <uint64_t> handed(0);
	/**
	 * @brief Описание выданного блока
	 *
	 */
	struct block_t {
		// Адрес выданного блока
		void * addr;
		// Затребованный размер блока в байтах
		size_t size;
		// Образец, которым засеян блок
		uint8_t mark;
		// Затребованное выравнивание, либо нуль
		size_t align;
	};
	/**
	 * Признак хода передачи блоков соседним потокам
	 *
	 * Отключается доводом запуска ради РАЗБОРА находок: расхождение, пропадающее с
	 * отключением передачи, принадлежит пути освобождения чужим потоком, а не выдаче
	 */
	bool crossing = true;
	/**
	 * Признак хода отдачи памяти системе посреди нагрузки
	 *
	 * Отключается доводом запуска ради разбора находок: отдача идёт по всей куче и
	 * может задевать то, чем прямо сейчас пользуются соседние потоки
	 */
	bool purging = true;
	/**
	 * Признак хода перевыдачи блоков
	 *
	 * Отключается доводом запуска ради разбора находок: перевыдача сама и выдаёт, и
	 * возвращает, и потому подозревается первой
	 */
	bool regrowing = true;
	/**
	 * Признак хода выдачи блоков ЛИШЬ в пределах разрядов
	 *
	 * Слои устройства разные, и находку надо приписать одному из них
	 */
	bool ranked = false;
	/**
	 * Признак строгого учёта выданных блоков
	 *
	 * Ход разбора, а не проверки: всякая выдача сличается со ВСЕМИ живыми блоками на
	 * перекрытие. Стоит это дорого и нагрузку искажает, зато ловит саму двойную выдачу
	 * в тот миг, когда она случилась, - и называет ОБА блока, а не оставляет обвал
	 * памяти где-то поодаль
	 */
	bool strict = false;
	// Живые блоки всех потоков при строгом учёте
	std::vector <block_t> census;
	// Замок учёта живых блоков
	std::mutex censusLock;
	/**
	 * @brief Метод постановки блока на строгий учёт
	 *
	 * @param block описание выданного блока
	 *
	 */
	static void enlist(const block_t & block) noexcept;
	/**
	 * @brief Метод снятия блока со строгого учёта
	 *
	 * @param addr адрес возвращаемого блока
	 *
	 */
	static void delist(const void * addr) noexcept;
	// Блоки, отданные на возврат соседним потокам
	std::vector <block_t> orphans;
	// Замок перечня блоков, отданных соседям
	std::mutex orphanLock;
	/**
	 * @brief Метод получения очередного случайного числа
	 *
	 * @note Свой источник на поток, а не общий: общий сам стал бы местом состязания и
	 *       менял бы то самое, что ворошитель проверяет
	 *
	 * @param state состояние источника
	 * @return      очередное случайное число
	 *
	 */
	static uint64_t next(uint64_t & state) noexcept {
		// Ворошим состояние источника
		state ^= (state << 13);
		state ^= (state >> 7);
		state ^= (state << 17);
		// Выводим очередное случайное число
		return state;
	}
	/**
	 * @brief Метод доклада о найденном расхождении
	 *
	 * @param what о чём доклад
	 * @param addr адрес разбираемого блока
	 * @param size размер разбираемого блока
	 *
	 */
	static void complain(const char * what, const void * addr, const size_t size) noexcept {
		// Отмечаем расхождение найденным
		broken.store(true, std::memory_order_relaxed);
		// Печатаем доклад о расхождении
		::fprintf(stderr, "РАСХОЖДЕНИЕ: %s | блок %p | размер %zu | зерно %llu\n",
		 what, addr, size, static_cast <unsigned long long> (seed));
	}
	/**
	 * @brief Метод постановки блока на строгий учёт
	 *
	 * @param block описание выданного блока
	 *
	 */
	static void enlist(const block_t & block) noexcept {
		// Если строгий учёт не ведётся
		if(!strict)
			// Учитывать нечего
			return;
		// Начало и конец ставимого на учёт блока
		const uintptr_t begin = reinterpret_cast <uintptr_t> (block.addr);
		const uintptr_t end = (begin + block.size);
		// Захватываем замок учёта
		std::lock_guard <std::mutex> lock(censusLock);
		/**
		 * Перебираем живые блоки всех потоков
		 */
		for(const block_t & other : census){
			// Начало и конец разбираемого живого блока
			const uintptr_t at = reinterpret_cast <uintptr_t> (other.addr);
			const uintptr_t to = (at + other.size);
			// Если блоки не перекрываются
			if((end <= at) || (to <= begin))
				// Разбираем следующий блок
				continue;
			// Печатаем оба перекрывшихся блока
			::fprintf(stderr, "ДВОЙНАЯ ВЫДАЧА: выдан [%p .. %p) размером %zu, "
			 "а он перекрывает живой [%p .. %p) размером %zu\n",
			 reinterpret_cast <void *> (begin), reinterpret_cast <void *> (end), block.size,
			 reinterpret_cast <void *> (at), reinterpret_cast <void *> (to), other.size);
			// Докладываем о двойной выдаче
			complain("один участок памяти выдан дважды", block.addr, block.size);
			// Разбирать больше нечего
			break;
		}
		// Ставим блок на учёт
		census.push_back(block);
	}
	/**
	 * @brief Метод снятия блока со строгого учёта
	 *
	 * @param addr адрес возвращаемого блока
	 *
	 */
	static void delist(const void * addr) noexcept {
		// Если строгий учёт не ведётся
		if(!strict)
			// Снимать нечего
			return;
		// Захватываем замок учёта
		std::lock_guard <std::mutex> lock(censusLock);
		/**
		 * Перебираем живые блоки всех потоков
		 */
		for(size_t i = 0; i < census.size(); i++){
			// Если разбираемый блок не тот
			if(census[i].addr != addr)
				// Разбираем следующий блок
				continue;
			// Переносим последний блок на место снимаемого
			census[i] = census.back();
			// Снимаем последний блок с учёта
			census.pop_back();
			// Снимать больше нечего
			return;
		}
		// Докладываем о возврате блока, на учёте не стоявшего
		complain("возвращён блок, на учёте не стоявший", addr, 0);
	}
	/**
	 * @brief Метод засева блока образцом
	 *
	 * @param block описание засеваемого блока
	 *
	 */
	static void sow(const block_t & block) noexcept {
		// Засеваем блок образцом целиком
		::memset(block.addr, block.mark, block.size);
	}
	/**
	 * @brief Метод сличения содержимого блока с образцом
	 *
	 * @param block описание сличаемого блока
	 * @return      признак совпадения содержимого с образцом
	 *
	 */
	static bool reaped(const block_t & block) noexcept {
		// Получаем содержимое блока побайтно
		const uint8_t * bytes = reinterpret_cast <const uint8_t *> (block.addr);
		/**
		 * Перебираем содержимое блока
		 */
		for(size_t i = 0; i < block.size; i++){
			// Если содержимое разошлось с образцом
			if(bytes[i] != block.mark){
				// Докладываем о расхождении содержимого
				complain("содержимое блока разошлось с образцом", block.addr, block.size);
				// Выводим признак расхождения
				return false;
			}
		}
		// Содержимое совпало с образцом
		return true;
	}
	/**
	 * @brief Метод проверки договора выданного блока
	 *
	 * @param block описание проверяемого блока
	 *
	 */
	static void examine(const block_t & block) noexcept {
		// Если блок не выдан
		if(block.addr == nullptr){
			// Докладываем об отказе выдачи
			complain("выдача ответила пустотой", nullptr, block.size);
			// Проверять больше нечего
			return;
		}
		/**
		 * Размер блока обязан покрывать затребованный
		 *
		 * Меньший означал бы, что прикладной код вправе писать за пределы выданного, а
		 * это порча кучи, какую не поймать ничем
		 */
		#if defined(AWH_FUZZ_MSIZE)
			const size_t measured = AWH_FUZZ_MSIZE(block.addr);
			// Если объявленный размер меньше затребованного
			if(measured < block.size)
				// Докладываем о недостаточном размере
				complain("объявленный размер меньше затребованного", block.addr, block.size);
		#endif
		/**
		 * Выравнивание обязано соблюдаться дословно
		 */
		if((block.align > 0) && ((reinterpret_cast <uintptr_t> (block.addr) % block.align) != 0))
			// Докладываем о нарушенном выравнивании
			complain("выравнивание блока нарушено", block.addr, block.size);
		/**
		 * Распределитель обязан признавать свой блок своим
		 *
		 * Не признай он его - освобождение ушло бы к системному распределителю с нашим
		 * указателем, а это гибель процесса
		 */
		#if !defined(AWH_FUZZ_SYSTEM)
			const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block.addr);
			// Если распределитель своего блока не признал
			if(region.begin == nullptr)
				// Докладываем о непризнанном блоке
				complain("распределитель не признал свой блок", block.addr, block.size);
		#endif
	}
	/**
	 * @brief Метод прогона нагрузки одним потоком
	 *
	 * @param state состояние источника случайных чисел
	 * @param steps число шагов нагрузки
	 *
	 */
	static void toil(uint64_t state, const size_t steps) noexcept {
		// Блоки, удерживаемые этим потоком
		std::vector <block_t> live;
		// Отводим место под удерживаемые блоки заранее
		live.reserve(512);
		/**
		 * Перебираем шаги нагрузки
		 */
		for(size_t step = 0; (step < steps) && !broken.load(std::memory_order_relaxed); step++){
			/**
			 * Забираем случайное число ОТДЕЛЬНЫМ действием
			 *
			 * Два забора в одном выражении отдали бы порядок вычисления собирателю, и
			 * прогон по одному зерну расходился бы между системами
			 */
			const uint64_t draw = next(state);
			// Определяем разновидность шага
			const size_t kind = static_cast <size_t> (draw % 100);
			/**
			 * Возвращаем случайный удерживаемый блок
			 */
			if((kind < 35) && !live.empty()){
				// Забираем случайное число под выбор блока
				const uint64_t pick = next(state);
				// Определяем место возвращаемого блока
				const size_t place = static_cast <size_t> (pick % live.size());
				// Получаем описание возвращаемого блока
				const block_t block = live[place];
				// Сличаем содержимое блока с образцом
				static_cast <void> (reaped(block));
				// Снимаем блок со строгого учёта
				delist(block.addr);
				// Возвращаем блок распределителю
				AWH_FUZZ_FREE(block.addr);
				// Считаем возвращённый блок
				returned.fetch_add(1, std::memory_order_relaxed);
				// Переносим последний блок на место возвращённого
				live[place] = live.back();
				// Снимаем последний блок с учёта
				live.pop_back();
				// Шаг отработан
				continue;
			}
			/**
			 * Отдаём случайный блок на возврат соседнему потоку
			 *
			 * Освобождение чужим потоком - отдельный путь распределителя, и без него
			 * ворошитель проверял бы лишь половину устройства
			 */
			if((kind < 45) && crossing && !live.empty()){
				// Забираем случайное число под выбор блока
				const uint64_t pick = next(state);
				// Определяем место отдаваемого блока
				const size_t place = static_cast <size_t> (pick % live.size());
				// Получаем описание отдаваемого блока
				const block_t block = live[place];
				/**
				 * Ставим блок в общий перечень под замком
				 */
				{
					// Захватываем замок общего перечня
					std::lock_guard <std::mutex> lock(orphanLock);
					// Ставим блок в общий перечень
					orphans.push_back(block);
				}
				// Считаем отданный соседу блок
				handed.fetch_add(1, std::memory_order_relaxed);
				// Переносим последний блок на место отданного
				live[place] = live.back();
				// Снимаем последний блок с учёта
				live.pop_back();
				// Шаг отработан
				continue;
			}
			/**
			 * Возвращаем блок, отданный соседним потоком
			 */
			if((kind < 55) && crossing){
				// Описание возвращаемого блока
				block_t block = {nullptr, 0, 0, 0};
				/**
				 * Забираем блок из общего перечня под замком
				 */
				{
					// Захватываем замок общего перечня
					std::lock_guard <std::mutex> lock(orphanLock);
					// Если в перечне есть блоки
					if(!orphans.empty()){
						// Забираем последний блок перечня
						block = orphans.back();
						// Снимаем забранный блок с перечня
						orphans.pop_back();
					}
				}
				// Если блок из перечня забран
				if(block.addr != nullptr){
					// Сличаем содержимое блока с образцом
					static_cast <void> (reaped(block));
					// Снимаем блок со строгого учёта
					delist(block.addr);
					// Возвращаем блок распределителю
					AWH_FUZZ_FREE(block.addr);
					// Считаем возвращённый блок
					returned.fetch_add(1, std::memory_order_relaxed);
				}
				// Шаг отработан
				continue;
			}
			/**
			 * Перевыдаём случайный удерживаемый блок
			 *
			 * Содержимое обязано пережить перевыдачу в пределах МЕНЬШЕГО из размеров
			 */
			if((kind < 65) && regrowing && !live.empty()){
				// Забираем случайное число под выбор блока
				const uint64_t pick = next(state);
				// Определяем место перевыдаваемого блока
				const size_t place = static_cast <size_t> (pick % live.size());
				// Получаем описание перевыдаваемого блока
				const block_t block = live[place];
				// Забираем случайное число под новый размер
				const uint64_t grown = next(state);
				// Определяем новый размер блока
				const size_t size = (static_cast <size_t> (grown % 8192) + 1);
				// Снимаем прежний блок со строгого учёта: перевыдача его хоронит
				delist(block.addr);
				// Перевыдаём блок под новый размер
				void * moved = AWH_FUZZ_REALLOC(block.addr, size);
				// Если перевыдача не состоялась
				if(moved == nullptr){
					// Докладываем об отказе перевыдачи
					complain("перевыдача ответила пустотой", block.addr, size);
					// Шаг отработан
					continue;
				}
				// Определяем переживающую перевыдачу часть содержимого
				const size_t kept = ((size < block.size) ? size : block.size);
				// Получаем содержимое блока побайтно
				const uint8_t * bytes = reinterpret_cast <const uint8_t *> (moved);
				/**
				 * Перебираем переживающую перевыдачу часть содержимого
				 */
				for(size_t i = 0; i < kept; i++){
					// Если содержимое разошлось с образцом
					if(bytes[i] != block.mark){
						// Докладываем о расхождении содержимого
						complain("перевыдача не сохранила содержимое", moved, size);
						// Разбирать больше нечего
						break;
					}
				}
				// Описываем перевыданный блок
				block_t fresh = {moved, size, block.mark, 0};
				// Засеваем перевыданный блок образцом заново
				sow(fresh);
				// Проверяем договор перевыданного блока
				examine(fresh);
				// Ставим перевыданный блок на строгий учёт
				enlist(fresh);
				// Ставим перевыданный блок на учёт
				live[place] = fresh;
				// Шаг отработан
				continue;
			}
			/**
			 * Отдаём системе свободную память
			 *
			 * Отдача идёт ПОСРЕДИ нагрузки, а не после неё: отдающая куча обязана
			 * оставлять выданные блоки нетронутыми, и проверить это можно лишь так
			 */
			if((kind < 66) && purging){
				#if !defined(AWH_FUZZ_SYSTEM)
					// Отдаём системе свободную память
					static_cast <void> (awh::alloc::Allocator::purge());
				#endif
				// Шаг отработан
				continue;
			}
			/**
			 * Выдаём очередной блок
			 */
			// Забираем случайное число под разновидность выдачи
			const uint64_t shape = next(state);
			// Забираем случайное число под размер блока
			const uint64_t bulk = next(state);
			// Затребованное выравнивание блока
			size_t align = 0;
			// Затребованный размер блока
			size_t size = 0;
			/**
			 * Размеры берём по всем слоям устройства: разряды, сверх разрядов и
			 * крупные выдачи, - иначе целые слои остались бы невороченными
			 */
			switch(::ranked ? size_t(0) : static_cast <size_t> (shape % 10)){
				// Выдаём блок в пределах разрядов
				case 0: case 1: case 2: case 3: case 4:
					size = (static_cast <size_t> (bulk % 1024) + 1);
				break;
				// Выдаём блок сверх разрядов
				case 5: case 6:
					size = (static_cast <size_t> (bulk % 262144) + 1);
				break;
				// Выдаём крупный блок
				case 7:
					size = (static_cast <size_t> (bulk % 4194304) + 1);
				break;
				// Выдаём обнулённый блок
				case 8:
					size = (static_cast <size_t> (bulk % 4096) + 1);
				break;
				// Выдаём блок с выравниванием
				default: {
					// Забираем случайное число под выравнивание
					const uint64_t edge = next(state);
					// Определяем затребованное выравнивание
					align = (static_cast <size_t> (1) << (3 + static_cast <size_t> (edge % 10)));
					// Определяем затребованный размер
					size = (static_cast <size_t> (bulk % 4096) + 1);
				}
			}
			// Адрес выданного блока
			void * addr = nullptr;
			// Если затребовано выравнивание
			if(align > 0)
				// Выдаём блок с затребованным выравниванием
				addr = AWH_FUZZ_MEMALIGN(align, size);
			// Если затребован обнулённый блок
			else if((shape % 10) == 8) {
				// Выдаём обнулённый блок
				addr = AWH_FUZZ_CALLOC(1, size);
				// Если блок выдан
				if(addr != nullptr){
					// Получаем содержимое блока побайтно
					const uint8_t * bytes = reinterpret_cast <const uint8_t *> (addr);
					/**
					 * Перебираем содержимое выданного блока
					 */
					for(size_t i = 0; i < size; i++){
						// Если содержимое блока не обнулено
						if(bytes[i] != 0){
							// Докладываем о необнулённом блоке
							complain("выдача обнулённого блока не обнулила его", addr, size);
							// Разбирать больше нечего
							break;
						}
					}
				}
			// Выдаём обычный блок
			} else addr = AWH_FUZZ_MALLOC(size);
			// Забираем случайное число под образец засева
			const uint64_t stamp = next(state);
			// Описываем выданный блок
			block_t block = {addr, size, static_cast <uint8_t> ((stamp % 255) + 1), align};
			// Проверяем договор выданного блока
			examine(block);
			// Если блок выдан
			if(addr != nullptr){
				// Ставим выданный блок на строгий учёт ПРЕЖДЕ засева
				enlist(block);
				// Засеваем выданный блок образцом
				sow(block);
				// Считаем выданный блок
				served.fetch_add(1, std::memory_order_relaxed);
				// Ставим выданный блок на учёт
				live.push_back(block);
			}
		}
		/**
		 * Возвращаем всё удерживаемое потоком
		 */
		for(const block_t & block : live){
			// Сличаем содержимое блока с образцом
			static_cast <void> (reaped(block));
			// Снимаем блок со строгого учёта
			delist(block.addr);
			// Возвращаем блок распределителю
			AWH_FUZZ_FREE(block.addr);
			// Считаем возвращённый блок
			returned.fetch_add(1, std::memory_order_relaxed);
		}
	}
};

/**
 * @brief Точка входа ворошителя
 *
 * @param argc число доводов запуска
 * @param argv доводы запуска
 * @return     нуль при отсутствии расхождений
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Определяем зерно прогона
	::seed = ((argc > 1) ? ::strtoull(argv[1], nullptr, 10) : 1);
	// Определяем число шагов нагрузки на поток
	const size_t steps = ((argc > 2) ? ::strtoul(argv[2], nullptr, 10) : 200000);
	// Определяем число потоков нагрузки
	const size_t threads = ((argc > 3) ? ::strtoul(argv[3], nullptr, 10) : 4);
	// Определяем ход передачи блоков соседним потокам
	::crossing = ((argc > 4) ? (::strtoul(argv[4], nullptr, 10) != 0) : true);
	// Определяем ход отдачи памяти системе
	::purging = ((argc > 5) ? (::strtoul(argv[5], nullptr, 10) != 0) : true);
	// Определяем ход перевыдачи блоков
	::regrowing = ((argc > 6) ? (::strtoul(argv[6], nullptr, 10) != 0) : true);
	// Определяем ход выдачи лишь в пределах разрядов
	::ranked = ((argc > 7) ? (::strtoul(argv[7], nullptr, 10) != 0) : false);
	// Определяем ход строгого учёта выданных блоков
	::strict = ((argc > 8) ? (::strtoul(argv[8], nullptr, 10) != 0) : false);
	// Если зерно прогона не задано
	if(::seed == 0)
		// Заводим зерно прогона: источник от нуля не расходится вовсе
		::seed = 1;
	#if defined(AWH_FUZZ_SYSTEM)
		// Распределитель здесь системный: заводить и настраивать нечего
		const bool seized = false;
	#else
		// Требуемые настройки распределителя
		awh::alloc::options_t options;
		/**
		 * Просим захват выдачи памяти процесса
		 *
		 * Отказ его расхождением НЕ является: под санитайзерами выдача памяти принадлежит
		 * им, и уступить её нам они не могут. Ворошителю захват и не нужен - он зовёт наши
		 * входы сам, - а нужен лишь установщик настроек, заводящий кэши потоков
		 */
		const bool seized = awh::alloc::Allocator::capture(options, nullptr);
		/**
		 * Заводим распределитель ПЕРВОЙ выдачей, прежде задания настроек
		 *
		 * Порядок этот обязателен, и вот отчего. При сборке с `AWH_ALLOC_DISABLED` захват
		 * выходит отказом СРАЗУ, не заводя ничего; а установщик настроек заводит кэши
		 * потоков лишь у ЗАВЕДЁННОГО распределителя - незаведённому он свою работу
		 * пропускает. Задай мы настройки первыми - кэшей не было бы вовсе, и ворошитель
		 * ходил бы вырожденным путём центральных списков, ворочая не то устройство,
		 * какое проверяет
		 *
		 * Заодно снимается ложное расхождение: пока распределитель заводится, выдача идёт
		 * из области первоначальной выдачи, а её `resolve` знать не обязан - и соседний
		 * поток видел бы «распределитель не признал свой блок» на пустом месте
		 */
		void * kindle = AWH_FUZZ_MALLOC(64);
		// Возвращаем блок заведения
		AWH_FUZZ_FREE(kindle);
		// Заводим кэши потоков установщиком настроек
		awh::alloc::Allocator::options(options);
		/**
		 * Утверждаем, что кэши потоков вправду заведены
		 *
		 * Иначе ворошитель молча проверял бы не тот путь: это уже случалось, и след
		 * остался в записях
		 */
		{
			// Блоки, оседающие в кэше потока при возврате
			void * probe[16];
			// Выдаём блоки одного разряда
			for(size_t i = 0; i < 16; i++)
				// Выдаём очередной блок
				probe[i] = AWH_FUZZ_MALLOC(64);
			// Возвращаем блоки: они обязаны осесть в кэше потока
			for(size_t i = 0; i < 16; i++)
				// Возвращаем очередной блок
				AWH_FUZZ_FREE(probe[i]);
			// Читаем объём свободного в кэшах потоков
			const size_t cached = awh::alloc::Allocator::property(awh::alloc::property_t::CACHED);
			// Если кэши потоков не заведены
			if(cached == 0){
				// Печатаем доклад о незаведённых кэшах
				::fprintf(stderr, "РАСХОЖДЕНИЕ: кэши потоков не заведены - ворошить нечего\n");
				// Отвечаем отказом
				return 1;
			}
		}
	#endif
	// Печатаем условия прогона
	::printf("ворошитель распределителя: зерно %llu, шагов %zu, потоков %zu, захват %s, передача соседям %s\n",
	 static_cast <unsigned long long> (::seed), steps, threads,
	 (seized ? "состоялся" : "не состоялся"), (::crossing ? "есть" : "нет"));
	// Печатаем ход отдачи памяти системе
	::printf("отдача памяти системе: %s\n", (::purging ? "есть" : "нет"));
	/**
	 * Поток-наблюдатель, опрашивающий состояние распределителя на ходу
	 *
	 * Нужен он не ради самих чисел, а ради ПУТИ: состояние кэшей потоков пишет хозяин
	 * кэша, а читает его чужой поток - опрос расхода да установщик настроек. Без такого
	 * потока ворошитель трогает состояние кэша ОДНИМ ЛИШЬ хозяином, и гонки на нём не
	 * бывает по устройству прогона
	 *
	 * Доказано опытом: неделимость сняли с объёма лежащего в кэшах намеренно, прогнали
	 * ворошителя под ThreadSanitizer'ом на восьми потоках - и он смолчал. Не оттого, что
	 * гонки нет, а оттого, что читателя не было. Молчание при неспособности молчать
	 * неотличимо от молчания при исправности
	 *
	 * Наблюдатель правит и предел кэшей: установщик настроек обходит все заведённые кэши
	 * и пишет каждому, тогда как хозяева читают предел без замка. Две гонки этого рода
	 * названы ThreadSanitizer'ом и закрыты
	 */
	// Признак остановки наблюдателя
	std::atomic <bool> watching(true);
	// Поток-наблюдатель
	std::thread watcher([&watching, &options](){
		// Оборот опроса
		size_t round = 0;
		/**
		 * Опрашиваем состояние, пока идёт нагрузка
		 */
		while(watching.load(std::memory_order_relaxed)){
			// Читаем объём свободного в кэшах потоков
			static_cast <void> (awh::alloc::Allocator::property(awh::alloc::property_t::CACHED));
			// Читаем занятое прикладным кодом прямо сейчас
			static_cast <void> (awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED));
			// Читаем взятое у системы всего
			static_cast <void> (awh::alloc::Allocator::property(awh::alloc::property_t::HEAP));
			/**
			 * Правим предел кэшей потоков через оборот
			 *
			 * Через оборот, а не на каждом: правка обходит все кэши под замком, и делать
			 * её без передышки значило бы держать замок занятым, а нагрузку - в ожидании
			 */
			if((round++ & 1u) == 0){
				// Настройки с иным пределом кэша
				awh::alloc::options_t amended = options;
				// Задаём предел кэша, чередуя два значения
				amended.cacheLimit = (((round & 2u) != 0) ? (64u * 1024u) : (1u * 1024u * 1024u));
				// Задаём настройки распределителю
				awh::alloc::Allocator::options(amended);
			}
		}
	});
	// Потоки нагрузки
	std::vector <std::thread> workers;
	// Отводим место под потоки нагрузки заранее
	workers.reserve(threads);
	/**
	 * Заводим потоки нагрузки
	 */
	for(size_t i = 0; i < threads; i++)
		// Заводим очередной поток нагрузки, дав ему своё зерно
		workers.emplace_back(&::toil, (::seed + (i * 0x9E3779B97F4A7C15ull) + 1), steps);
	/**
	 * Дожидаемся всех потоков нагрузки
	 */
	for(std::thread & worker : workers)
		// Дожидаемся очередного потока нагрузки
		worker.join();
	// Останавливаем наблюдателя
	watching.store(false, std::memory_order_relaxed);
	// Дожидаемся наблюдателя
	watcher.join();
	// Возвращаем распределителю первоначальные настройки
	awh::alloc::Allocator::options(options);
	/**
	 * Возвращаем всё, что осталось отданным соседям
	 */
	for(const auto & block : ::orphans){
		// Возвращаем блок распределителю
		AWH_FUZZ_FREE(block.addr);
		// Считаем возвращённый блок
		::returned.fetch_add(1, std::memory_order_relaxed);
	}
	// Печатаем итог прогона
	::printf("выдано %llu, возвращено %llu, отдано соседям %llu\n",
	 static_cast <unsigned long long> (::served.load()),
	 static_cast <unsigned long long> (::returned.load()),
	 static_cast <unsigned long long> (::handed.load()));
	/**
	 * Утверждаем, что прогон вправду шёл
	 *
	 * Молчащий распределитель, не выдавший ничего, прошёл бы прогон без единого
	 * расхождения - и отчитался бы успехом
	 */
	if(::served.load() == 0){
		// Печатаем доклад о пустом прогоне
		::fprintf(stderr, "РАСХОЖДЕНИЕ: за прогон не выдано ни одного блока\n");
		// Отвечаем отказом
		return 1;
	}
	// Если расхождения найдены
	if(::broken.load()){
		// Печатаем доклад о найденных расхождениях
		::fprintf(stderr, "прогон завершён С РАСХОЖДЕНИЯМИ, зерно %llu\n",
		 static_cast <unsigned long long> (::seed));
		// Отвечаем отказом
		return 1;
	}
	// Печатаем доклад об отсутствии расхождений
	::printf("расхождений нет\n");
	// Отвечаем успехом
	return 0;
}
