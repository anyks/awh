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
	// Читаем размер страницы
	const size_t result = this->_granularity.load(std::memory_order_relaxed);
	// Если размер страницы уже определён
	if(result > 0)
		// Выводим размер страницы
		return result;
	// Определяем размер страницы у системы
	const size_t detected = this->detect();
	/**
	 * Записываем определённое, не сверяя с прежним
	 *
	 * Двое, определившие размер разом, получат ОДНО И ТО ЖЕ число: спрашивается оно у
	 * системы, а та своей страницы посреди работы не меняет. Оттого гонка здесь
	 * безобидна по существу - но не по договору языка, и вид поля неделимый
	 */
	const_cast <SystemSource *> (this)->_granularity.store(detected, std::memory_order_relaxed);
	// Выводим размер страницы
	return detected;
}
/**
 * @brief Метод задания просьбы о крупных страницах
 *
 * @param wanted признак просьбы о крупных страницах
 *
 */
void awh::alloc::SystemSource::superpages(const bool wanted) noexcept {
	// Запоминаем признак просьбы о крупных страницах
	this->_superpages.store(wanted, std::memory_order_relaxed);
}
/**
 * @brief Метод задания потолка взятого у системы
 *
 * @param limit потолок в байтах: нуль - без потолка
 *
 */
void awh::alloc::SystemSource::ceiling(const size_t limit) noexcept {
	// Запоминаем потолок взятого у системы
	this->_ceiling.store(limit, std::memory_order_relaxed);
}
/**
 * @brief Метод опроса взятого у системы
 *
 * @return взятое у системы в байтах
 *
 */
size_t awh::alloc::SystemSource::taken() const noexcept {
	// Выводим взятое у системы
	return this->_taken.load(std::memory_order_relaxed);
}
/**
 * @brief Метод получения числа областей, доставшихся крупными страницами
 *
 * @return число областей
 *
 */
size_t awh::alloc::SystemSource::superpaged() const noexcept {
	// Выводим число областей, доставшихся крупными страницами
	return this->_superpaged.load(std::memory_order_relaxed);
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
 * @note Совет заведён под тем же гейтом, что и его зватель: советуют системе через
 *       `madvise`, какого у MS Windows нет вовсе - там крупные страницы просятся
 *       признаком `MEM_LARGE_PAGES` при самом отведении. Прежде определение из гейта
 *       ВЫПАДАЛО, и у Windows выходила функция, не званная ниоткуда - clang отвечал на
 *       неё `-Wunused-function`. Мёртвым кодом она при этом не была: на POSIX путь
 *       замкнут - обычное отведение зовёт совет сразу за `mmap`, когда о крупных
 *       страницах просили
 *
 * @param addr адрес отображённой области
 * @param size размер области в байтах
 *
 */
#if !_WIN32 && !_WIN64
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
#endif
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
	/**
	 * Сверяем размер на переполнение ПРЕЖДЕ приведения к зернистости
	 *
	 * Приведение считает `size + grain - 1`, и у размеров близ предела счёт этот
	 * переполняется. Дефекта сегодня из этого не выходит, и это проверено перебором
	 * всего опасного окна: приведение вырождается в НУЛЬ, а нулевую длину отвергает уже
	 * ядро - ни `mmap`, ни `VirtualAlloc` её не берут. Держится это на том, что окно,
	 * пропускаемое звавшим, уже самой зернистости, а зависеть такому от отказа ядра
	 * незачем: смени кто страницу кучи, и переполнение выдало бы область КОРОЧЕ
	 * запрошенной, а звавший записал бы по ней запрошенное
	 *
	 * Сверка здесь холостая: обращение к системе идёт раз на кусок, а не на выдачу.
	 * Тот же довод записан при обоих таких же местах у слоя крупных выдач
	 */
	if(size > (static_cast <size_t> (-1) - (grain - 1)))
		// Отвечаем отказом: приведение к зернистости переполнилось бы
		return nullptr;
	/**
	 * Сверяем потолок взятого у системы ПРЕЖДЕ обращения к ней
	 *
	 * Сверяем приблизительно - по запрошенному, а не по округлённому: округление
	 * прибавит меньше страницы, а вторая сверка после выдачи стоила бы отдачи только
	 * что взятого. Потолок здесь и есть предел, заказанный приложением, а не точная
	 * граница до байта
	 */
	// Требуемое выравнивание не может быть меньше зернистости выдачи
	const size_t align = ((alignment > grain) ? alignment : grain);
	// Округляем требуемый размер до целого числа страниц
	const size_t rounded = (((size + (grain - 1)) / grain) * grain);
	{
		// Читаем потолок взятого у системы
		const size_t limit = this->_ceiling.load(std::memory_order_relaxed);
		/**
		 * Потолок сверяется с ОКРУГЛЁННЫМ размером, а не с затребованным
		 *
		 * У системы берётся целое число страниц, и учёт взятого ведётся по ним же:
		 * сверка по затребованному размеру пропускала запрос, какому до потолка не
		 * хватало округления, а прибавляла к взятому всю страницу. Потолок при этом
		 * перебирался молча - тем чаще, чем мельче запросы
		 */
		// Если потолок задан и запрос его перебирает
		if((limit > 0) && ((this->_taken.load(std::memory_order_relaxed) + rounded) > limit))
			// Отвечаем отказом: приложение само заказало этот предел
			return nullptr;
	}
	/**
	 * Пробуем крупные страницы, если о них просили
	 *
	 * Просим их лишь у выдач без особого выравнивания: крупная страница выровнена по
	 * себе самой и вольного выравнивания не держит. Отказ - обычный исход, и выдача
	 * идёт дальше обычным путём
	 */
	if(this->_superpages.load(std::memory_order_relaxed) && (align <= grain)){
		// Отводим область крупными страницами
		void * result = __awh_source_huge__(rounded);
		// Если область отведена
		if(result != nullptr){
			// Запоминаем действительно выданный размер
			actual = rounded;
			// Считаем взятое у системы
			this->_taken.fetch_add(rounded, std::memory_order_relaxed);
			// Считаем области, доставшиеся крупными страницами
			this->_superpaged.fetch_add(1, std::memory_order_relaxed);
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
			if(result != nullptr){
				// Запоминаем действительно выданный размер
				actual = rounded;
				// Считаем взятое у системы
				this->_taken.fetch_add(rounded, std::memory_order_relaxed);
			}
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
				// Считаем взятое у системы
				this->_taken.fetch_add(rounded, std::memory_order_relaxed);
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
		if(this->_superpages.load(std::memory_order_relaxed) && (probe != MAP_FAILED))
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
			// Считаем взятое у системы
			this->_taken.fetch_add(rounded, std::memory_order_relaxed);
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
		// Считаем взятое у системы
		this->_taken.fetch_add(rounded, std::memory_order_relaxed);
		// Выводим выровненный адрес области
		return reinterpret_cast <void *> (aligned);
	#endif
}
/**
 * @brief Метод запрета области уходить в подкачку
 *
 * @param addr   адрес области
 * @param size   размер области в байтах
 * @param wanted признак необходимости запрета
 * @return       признак выполнения операции
 *
 */
bool awh::alloc::SystemSource::wire(void * addr, const size_t size, const bool wanted) noexcept {
	// Если области нет
	if((addr == nullptr) || (size == 0))
		// Запрещать нечего
		return false;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Ставим либо снимаем запрет уходить в подкачку
		return (wanted ? (::VirtualLock(addr, size) != 0) : (::VirtualUnlock(addr, size) != 0));
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		/**
		 * Право на запрет ограничено пределом RLIMIT_MEMLOCK
		 *
		 * Отказ здесь - обычный исход, а не дефект: предел этот у обычной машины
		 * невелик, и просьба сверх него отвергается системой
		 */
		return ((wanted ? ::mlock(addr, size) : ::munlock(addr, size)) == 0);
	#endif
}
/**
 * @brief Метод отведения области, укрытой от снимков памяти
 *
 * @param size   требуемый размер в байтах
 * @param actual действительно выданный размер
 * @param hidden признак состоявшегося укрытия
 * @return       адрес выданной области либо nullptr
 *
 */
void * awh::alloc::SystemSource::conceal(const size_t size, size_t & actual, bool & hidden) noexcept {
	// Обнуляем действительно выданный размер
	actual = 0;
	// Укрытия пока не состоялось
	hidden = false;
	// Если размер не задан
	if(size == 0)
		// Выдавать нечего
		return nullptr;
	/**
	 * Укрытие от снимков памяти умеют не все системы
	 *
	 * У OpenBSD это `MAP_CONCEAL`: область не попадает в снимок памяти при падении. У
	 * FreeBSD тому же служит `MAP_NOCORE`. Прочим системам укрыть область при отведении
	 * нечем, и мы отвечаем честным «не укрыто», отводя её обычным путём: понизить
	 * обещание молча значило бы обмануть того, кто просил защиты
	 */
	#if defined(MAP_CONCEAL) || defined(MAP_NOCORE)
		// Получаем зернистость выдачи
		const size_t grain = this->granularity();
		// Если зернистость выдачи не определена
		if(grain == 0)
			// Выдавать нечего
			return nullptr;
		// Округляем требуемый размер до целого числа страниц
		const size_t rounded = (((size + (grain - 1)) / grain) * grain);
		/**
		 * Признак укрытия у своей системы
		 */
		#if defined(MAP_CONCEAL)
			// Область не попадает в снимок памяти при падении
			const int shelter = MAP_CONCEAL;
		#else
			// Область не попадает в снимок памяти при падении
			const int shelter = MAP_NOCORE;
		#endif
		// Отводим укрытую область у системы
		void * result = ::mmap(nullptr, rounded, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | shelter, -1, 0);
		// Если область не отведена
		if(result == MAP_FAILED)
			// Выдавать нечего
			return nullptr;
		// Запоминаем действительно выданный размер
		actual = rounded;
		// Считаем взятое у системы
		this->_taken.fetch_add(rounded, std::memory_order_relaxed);
		// Отмечаем укрытие состоявшимся
		hidden = true;
		// Выводим адрес отведённой области
		return result;
	#else
		// Отводим область обычным путём
		return this->alloc(size, 0, actual);
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
	 * Снимаем область со счёта взятого ПРЕЖДЕ отдачи
	 *
	 * Отдача у нас отказом не отвечает по существу - область наша, и система её примет,
	 * - а порядок этот держит счёт не выше истины: снятый после, он на короткий срок
	 * показывал бы взятым то, что уже отдано, и потолок отвергал бы выдачу на ровном месте
	 */
	{
		// Читаем взятое у системы
		size_t taken = this->_taken.load(std::memory_order_relaxed);
		/**
		 * Снимаем со счёта, не уводя его ниже нуля
		 *
		 * Область могла быть взята до задания потолка либо отдана частями: счёт этот
		 * приблизительный по устройству, и уход в минус превратил бы его в огромное
		 * число, отчего потолок закрыл бы выдачу насовсем
		 */
		while(!this->_taken.compare_exchange_weak(taken, ((taken > size) ? (taken - size) : 0),
		 std::memory_order_relaxed, std::memory_order_relaxed));
	}
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
