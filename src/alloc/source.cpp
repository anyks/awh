/**
 * @file source.cpp
 * @date 2026-08-20
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
 * @brief Реализация системного источника страниц распределителя памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/source.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	#include <windows.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	#include <unistd.h>
	#include <sys/mman.h>
#endif

/**
 * @brief Метод определения размера страницы у системы
 *
 * @return размер страницы в байтах
 *
 */
size_t awh::alloc::SystemSource::detect() const noexcept {
	/**
	 * Для операционной системы MS Windows
	 *
	 * Спрашивается именно зернистость выдачи (dwAllocationGranularity), а не размер
	 * страницы: VirtualAlloc выдаёт области, выровненные по ней, и обычно она равна
	 * 64 КБ при странице в 4 КБ. Приняв за зернистость размер страницы, куча просила
	 * бы выравнивания, какого система не даёт
	 */
	#if _WIN32 || _WIN64
		// Сведения о системе
		SYSTEM_INFO info;
		// Обнуляем сведения о системе
		::memset(&info, 0, sizeof(info));
		// Получаем сведения о системе
		::GetSystemInfo(&info);
		// Выводим зернистость выдачи областей
		return static_cast <size_t> (info.dwAllocationGranularity);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Получаем размер страницы у системы
		const long size = ::sysconf(_SC_PAGESIZE);
		// Выводим размер страницы, а при отказе - принятые повсеместно четыре килобайта
		return ((size > 0) ? static_cast <size_t> (size) : 4096);
	#endif
}
/**
 * @brief Метод получения размера страницы источника
 *
 * @return размер страницы в байтах
 *
 */
size_t awh::alloc::SystemSource::granularity() const noexcept {
	// Если размер страницы ещё не определён
	if(this->_granularity == 0)
		// Определяем размер страницы у системы
		const_cast <SystemSource *> (this)->_granularity = this->detect();
	// Выводим размер страницы
	return this->_granularity;
}
/**
 * @brief Метод выдачи области страниц
 *
 * @param size      требуемый размер в байтах
 * @param alignment требуемое выравнивание в байтах
 * @param actual    действительно выданный размер
 * @return          адрес выданной области либо nullptr
 *
 */
void awh::alloc::SystemSource::superpages(const bool wanted) noexcept {
	// Запоминаем признак просьбы о крупных страницах
	this->_superpages = wanted;
}
/**
 * @brief Метод получения числа областей, доставшихся крупными страницами
 *
 * @return число областей, доставшихся крупными страницами
 *
 */
size_t awh::alloc::SystemSource::superpaged() const noexcept {
	// Выводим число областей, доставшихся крупными страницами
	return this->_superpaged;
}
/**
 * @brief Метод отведения области крупными страницами
 *
 * @note Отказ здесь - обычный исход: крупные страницы у всех наших систем требуют либо
 *       заранее отведённого запаса, либо особого права. Отказавшись, выдача идёт
 *       обычным путём
 *
 * @param size выравненный по зерну размер в байтах
 * @return     адрес отведённой области либо nullptr
 *
 */
static void * __awh_source_huge__(const size_t size) noexcept {
	#if _WIN32 || _WIN64
		/**
		 * Просим у системы крупные страницы
		 *
		 * Требуют они права SeLockMemoryPrivilege, какого у обычного пользователя нет:
		 * отказ здесь - правило, а не исключение. Размер обязан быть кратен размеру
		 * крупной страницы, иначе система отвечает отказом сама
		 */
		static SIZE_T (WINAPI * minimum)() = reinterpret_cast <SIZE_T (WINAPI *)()> (
			reinterpret_cast <void *> (::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "GetLargePageMinimum"))
		);
		// Если средства узнать размер крупной страницы нет
		if(minimum == nullptr)
			// Крупных страниц у этой системы нет
			return nullptr;
		// Узнаём размер крупной страницы
		const SIZE_T grain = minimum();
		// Если крупных страниц система не даёт
		if(grain == 0)
			// Отвечаем отказом
			return nullptr;
		// Если размер не кратен крупной странице
		if((size % static_cast <size_t> (grain)) != 0)
			// Отвечаем отказом
			return nullptr;
		// Отводим область крупными страницами
		return ::VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
	#elif __linux__
		/**
		 * Просим у системы крупные страницы
		 *
		 * Требуют они заранее отведённого запаса (`vm.nr_hugepages`), какого у обычной
		 * машины нет: отказ здесь - правило
		 */
		#if defined(MAP_HUGETLB)
			// Отображаем область крупными страницами
			void * result = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
			// Выводим отображённую область
			return ((result != MAP_FAILED) ? result : nullptr);
		#else
			// Крупных страниц у этой системы нет
			(void) size;
			return nullptr;
		#endif
	#elif __FreeBSD__
		/**
		 * Просим у системы сверхстраницы
		 *
		 * FreeBSD не отводит их особым отображением, а СОБИРАЕТ из обычных, когда
		 * область выровнена по их размеру: `MAP_ALIGNED_SUPER` о том и просит
		 */
		#if defined(MAP_ALIGNED_SUPER)
			// Отображаем область, выровненную под сверхстраницу
			void * result = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED_SUPER, -1, 0);
			// Выводим отображённую область
			return ((result != MAP_FAILED) ? result : nullptr);
		#else
			(void) size;
			return nullptr;
		#endif
	#else
		/**
		 * Крупных страниц отдельным отображением эта система не даёт
		 *
		 * У macOS они просятся лишь через `mach_vm_allocate`, у NetBSD и OpenBSD их нет
		 * вовсе, а у систем Sun о них просят советом уже отображённой области - что и
		 * делается ниже, обычным путём
		 */
		(void) size;
		return nullptr;
	#endif
}
/**
 * @brief Метод совета системе держать область крупными страницами
 *
 * @note Совет этот не отводит крупных страниц, а просит собрать их из обычных: систему
 *       он ни к чему не обязывает, и ответ его нам безразличен
 *
 * @param addr адрес отображённой области
 * @param size размер области в байтах
 *
 */
static void __awh_source_advise__(void * addr, const size_t size) noexcept {
	#if defined(MADV_HUGEPAGE)
		// Советуем системе собрать крупные страницы
		::madvise(addr, size, MADV_HUGEPAGE);
	#elif defined(MADV_COLLAPSE)
		::madvise(addr, size, MADV_COLLAPSE);
	#else
		(void) addr; (void) size;
	#endif
}
/**
 * @brief Метод отведения области у системы
 *
 * @param size      требуемый размер в байтах
 * @param alignment требуемое выравнивание в байтах
 * @param actual    действительно выданный размер в байтах
 * @return          адрес отведённой области либо nullptr
 *
 */
void * awh::alloc::SystemSource::alloc(const size_t size, const size_t alignment, size_t & actual) noexcept {
	// Обнуляем действительно выданный размер
	actual = 0;
	// Если размер не задан
	if(size == 0)
		// Выдавать нечего
		return nullptr;
	/**
	 * Проверяем запрошенное выравнивание прежде приведения
	 *
	 * Проверять приведённое нельзя: запрос вида 3000 при зернистости 16384 привёлся бы
	 * к зернистости, та степень двойки, проверка прошла бы, - а выданный адрес кратен
	 * 16384 и числу 3000 не кратен вовсе. Обещание выравнивания нарушилось бы молча
	 */
	if((alignment != 0) && ((alignment & (alignment - 1)) != 0))
		// Отвечаем отказом: выравнивание обязано быть степенью двойки
		return nullptr;
	// Получаем зернистость выдачи
	const size_t grain = this->granularity();
	// Требуемое выравнивание не может быть меньше зернистости выдачи
	const size_t align = ((alignment > grain) ? alignment : grain);
	// Округляем требуемый размер до целого числа страниц
	const size_t rounded = (((size + (grain - 1)) / grain) * grain);
	/**
	 * Пробуем крупные страницы, если о них просили
	 *
	 * Просим их лишь у выдач без особого выравнивания: крупная страница выровнена по
	 * себе самой и вольного выравнивания не держит. Отказ - обычный исход, и выдача
	 * идёт дальше обычным путём
	 */
	if(this->_superpages && (align <= grain)){
		// Отводим область крупными страницами
		void * result = __awh_source_huge__(rounded);
		// Если область отведена
		if(result != nullptr){
			// Запоминаем действительно выданный размер
			actual = rounded;
			// Считаем области, доставшиеся крупными страницами
			this->_superpaged++;
			// Выводим адрес отведённой области
			return result;
		}
	}
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Выдача идёт с запасом под выравнивание, а затем повторяется по выровненному
		 * адресу
		 *
		 * Причина в том, что MS Windows не даёт освободить часть отведённой области:
		 * MEM_RELEASE принимает лишь адрес, полученный от VirtualAlloc, и освобождает
		 * область целиком. Отрезать голову и хвост, как это делается у POSIX, нечем.
		 * Оттого область отводится с запасом, тут же отдаётся целиком, и по выровненному
		 * адресу внутри неё делается вторая попытка
		 *
		 * Между отдачей и второй попыткой область на короткий срок свободна, и другой
		 * поток может её занять. Оттого попыток делается несколько, и при выравнивании,
		 * равном зернистости выдачи, обход этот не нужен вовсе - система и так выдаёт
		 * выровненное
		 */
		// Если требуемое выравнивание не превышает зернистости выдачи
		if(align <= grain){
			// Отводим область у системы
			void * result = ::VirtualAlloc(nullptr, rounded, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			// Если область отведена
			if(result != nullptr)
				// Запоминаем действительно выданный размер
				actual = rounded;
			// Выводим адрес отведённой области
			return result;
		}
		/**
		 * Перебираем попытки отвести выровненную область
		 */
		for(size_t attempt = 0; attempt < 8; attempt++){
			// Отводим область с запасом под выравнивание
			void * probe = ::VirtualAlloc(nullptr, (rounded + align), MEM_RESERVE, PAGE_NOACCESS);
			// Если область с запасом не отведена
			if(probe == nullptr)
				// Отвечаем отказом
				return nullptr;
			// Определяем выровненный адрес внутри отведённой области
			const uintptr_t aligned = ((reinterpret_cast <uintptr_t> (probe) + (align - 1)) & ~(static_cast <uintptr_t> (align) - 1));
			// Отдаём область с запасом обратно системе
			::VirtualFree(probe, 0, MEM_RELEASE);
			// Отводим область по выровненному адресу
			void * result = ::VirtualAlloc(reinterpret_cast <void *> (aligned), rounded, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			// Если область отведена
			if(result != nullptr){
				// Запоминаем действительно выданный размер
				actual = rounded;
				// Выводим адрес отведённой области
				return result;
			}
		}
		// Отвечаем отказом: выровненную область занять не удалось
		return nullptr;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Размер отображения с запасом под выравнивание
		const size_t span = ((align > grain) ? (rounded + align) : rounded);
		// Отображаем область у системы
		void * probe = ::mmap(nullptr, span, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		/**
		 * Советуем системе собрать крупные страницы, если о них просили
		 *
		 * Совет этот - второй заход после отказа в отведении: часть систем крупных
		 * страниц отдельным отображением не даёт вовсе, но собирает их из обычных по
		 * совету. Ни к чему систему он не обязывает, и ответ его нам безразличен
		 */
		if(this->_superpages && (probe != MAP_FAILED))
			// Советуем системе собрать крупные страницы
			__awh_source_advise__(probe, span);
		// Если область не отображена
		if(probe == MAP_FAILED)
			// Отвечаем отказом
			return nullptr;
		// Если запас под выравнивание не отводился
		if(align <= grain){
			// Запоминаем действительно выданный размер
			actual = rounded;
			// Выводим адрес отображённой области
			return probe;
		}
		/**
		 * Отрезаем от отображения голову и хвост
		 *
		 * У POSIX это возможно: munmap принимает любой участок отображения, а не только
		 * отображение целиком, и запас под выравнивание возвращается системе сразу
		 */
		// Определяем выровненный адрес внутри отображённой области
		const uintptr_t aligned = ((reinterpret_cast <uintptr_t> (probe) + (align - 1)) & ~(static_cast <uintptr_t> (align) - 1));
		// Определяем размер отрезаемой головы
		const size_t head = (aligned - reinterpret_cast <uintptr_t> (probe));
		// Если голова не пуста
		if(head > 0)
			// Отдаём голову системе
			::munmap(probe, head);
		// Определяем размер отрезаемого хвоста
		const size_t tail = (span - head - rounded);
		// Если хвост не пуст
		if(tail > 0)
			// Отдаём хвост системе
			::munmap(reinterpret_cast <void *> (aligned + rounded), tail);
		// Запоминаем действительно выданный размер
		actual = rounded;
		// Выводим выровненный адрес области
		return reinterpret_cast <void *> (aligned);
	#endif
}
/**
 * @brief Метод отдачи содержимого страниц системе
 *
 * @param addr адрес отдаваемой области
 * @param size размер отдаваемой области
 * @return     признак выполнения операции
 *
 */
bool awh::alloc::SystemSource::purge(void * addr, const size_t size) noexcept {
	// Если область не задана
	if((addr == nullptr) || (size == 0))
		// Отдавать нечего
		return false;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Средство отдачи выбирается по наличию: DiscardVirtualMemory появилось в
		 * Windows 8.1 и отдаёт страницы сразу, а MEM_RESET лишь помечает содержимое
		 * ненужным, оставляя страницы за процессом. Первое предпочтительнее, второе
		 * есть всюду
		 */
		// Берём средство отдачи страниц у библиотеки ядра
		static DWORD (WINAPI * discard)(PVOID, SIZE_T) = reinterpret_cast <DWORD (WINAPI *)(PVOID, SIZE_T)> (
			reinterpret_cast <void *> (::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "DiscardVirtualMemory"))
		);
		// Если средство отдачи страниц у системы есть
		if(discard != nullptr)
			// Отдаём страницы системе
			return (discard(addr, static_cast <SIZE_T> (size)) == ERROR_SUCCESS);
		// Помечаем содержимое страниц ненужным
		return (::VirtualAlloc(addr, size, MEM_RESET, PAGE_READWRITE) != nullptr);
	/**
	 * Для операционной системы Linux
	 *
	 * MADV_DONTNEED отдаёт страницы немедленно и обнуляет их при следующем обращении.
	 * MADV_FREE там тоже есть, но отдаёт лениво, и расход памяти по отчётам системы
	 * при этом не убывает - для нашей задачи это негодно
	 */
	#elif __linux__
		// Отдаём страницы системе
		return (::madvise(addr, size, MADV_DONTNEED) == 0);
	/**
	 * Для прочих систем семейства POSIX
	 *
	 * У BSD, macOS и систем Sun отдачей ведает MADV_FREE, а MADV_DONTNEED там лишь
	 * подсказка о порядке обращения и страниц не освобождает вовсе
	 */
	#else
		/**
		 * Если средство отложенной отдачи страниц у системы есть
		 */
		#if defined(MADV_FREE)
			// Отдаём страницы системе
			return (::madvise(addr, size, MADV_FREE) == 0);
		/**
		 * Если средства отложенной отдачи страниц у системы нет
		 */
		#else
			// Отдаём страницы системе способом, какой есть
			return (::madvise(addr, size, MADV_DONTNEED) == 0);
		#endif
	#endif
}
/**
 * @brief Метод отдачи области системе целиком
 *
 * @param addr адрес отдаваемой области
 * @param size размер отдаваемой области
 * @return     признак выполнения операции
 *
 */
bool awh::alloc::SystemSource::release(void * addr, const size_t size) noexcept {
	// Если область не задана
	if((addr == nullptr) || (size == 0))
		// Отдавать нечего
		return false;
	/**
	 * Для операционной системы MS Windows
	 *
	 * MEM_RELEASE требует нулевого размера и адреса, полученного от VirtualAlloc:
	 * область отдаётся целиком, частями отдать её система не даёт
	 */
	#if _WIN32 || _WIN64
		// Отмечаем неиспользуемый параметр
		static_cast <void> (size);
		// Отдаём область системе
		return (::VirtualFree(addr, 0, MEM_RELEASE) != FALSE);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Отдаём область системе
		return (::munmap(addr, size) == 0);
	#endif
}
/**
 * @brief Метод смены доступности области
 *
 * @param addr   адрес области
 * @param size   размер области
 * @param opened признак открытой области: ложь закрывает её вовсе
 * @return       признак выполнения операции
 *
 */
bool awh::alloc::SystemSource::protect(void * addr, const size_t size, const bool opened) noexcept {
	// Если область не задана
	if((addr == nullptr) || (size == 0))
		// Менять доступность нечему
		return false;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Прежняя доступность области
		DWORD previous = 0;
		// Меняем доступность области
		return (::VirtualProtect(addr, size, (opened ? PAGE_READWRITE : PAGE_NOACCESS), &previous) != FALSE);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Меняем доступность области
		return (::mprotect(addr, size, (opened ? (PROT_READ | PROT_WRITE) : PROT_NONE)) == 0);
	#endif
}
