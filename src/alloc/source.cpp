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
