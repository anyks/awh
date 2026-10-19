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
#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	#include <unistd.h>
	#include <sys/mman.h>
#endif

/**
 * @brief Инкапсулируем резервирование общего бюджета системного источника
 *
 */
namespace {
	/**
	 * @brief Класс резерва памяти на время обращения к системе
	 *
	 * @note Учёт включает незавершённые выдачи. Нулевой результат возвращает резерв
	 *       автоматически на любом пути отказа; успешная выдача оставляет его занятым
	 *       до освобождения области. Замки вызывающих слоёв независимы
	 *
	 */
	typedef class Source_Reservation {
		private:
			// Общий счётчик выданной и зарезервированной памяти
			std::atomic <size_t> & _taken;
			// Размер состоявшейся выдачи
			const size_t & _actual;
			// Размер полученного резерва либо нуль при отказе
			size_t _size;
		public:
			/**
			 * @brief Метод проверки состоявшегося резервирования
			 *
			 * @return признак полученного резерва
			 *
			 */
			bool reserved() const noexcept {
				// Выводим признак полученного резерва
				return (this->_size > 0);
			}
		public:
			/**
			 * @brief Запрещаем копирование единственного владельца резерва
			 *
			 */
			Source_Reservation(const Source_Reservation &) = delete;
			/**
			 * @brief Запрещаем присваивание владельца резерва
			 *
			 */
			Source_Reservation & operator = (const Source_Reservation &) = delete;
			/**
			 * @brief Конструктор резерва системной выдачи
			 *
			 * @param taken   общий счётчик памяти
			 * @param ceiling потолок памяти
			 * @param size    размер новой выдачи
			 * @param actual  размер состоявшейся выдачи
			 *
			 */
			Source_Reservation(std::atomic <size_t> & taken, const std::atomic <size_t> & ceiling, const size_t size, const size_t & actual) noexcept :
			 _taken(taken), _actual(actual), _size(0) {
				// Читаем занятый бюджет
				size_t occupied = this->_taken.load(std::memory_order_relaxed);
				/**
				 * Повторяем проверку при изменении бюджета соседним потоком
				 */
				for(;;){
					// Читаем действующий потолок
					const size_t limit = ceiling.load(std::memory_order_relaxed);
					// Отсекаем переполнение и превышение потолка без опасного сложения
					if((size > (static_cast <size_t> (-1) - occupied)) || ((limit > 0) && ((occupied > limit) || (size > (limit - occupied)))))
						// Оставляем резерв пустым
						return;
					// Занимаем бюджет до обращения к системе
					if(this->_taken.compare_exchange_weak(occupied, (occupied + size), std::memory_order_relaxed, std::memory_order_relaxed)){
						// Запоминаем размер полученного резерва
						this->_size = size;
						// Резервирование завершено
						return;
					}
				}
			}
			/**
			 * @brief Деструктор резерва системной выдачи
			 *
			 */
			~Source_Reservation() noexcept {
				// Если выдача не состоялась, освобождаем полученный резерв
				if((this->_size > 0) && (this->_actual == 0))
					// Возвращаем бюджет без системного обращения
					this->_taken.fetch_sub(this->_size, std::memory_order_relaxed);
			}
	} source_reservation_t;
};

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
	#if defined(_WIN32) || defined(_WIN64)
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
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
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
	/**
	 * Для операционной системы Linux
	 */
	#elif defined(__linux__)
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
		/**
		 * Если отведение крупных страниц недоступно
		 */
		#else
			// Крупных страниц у этой системы нет
			static_cast <void> (size);
			// Отвечаем отсутствием крупных страниц
			return nullptr;
		#endif
	/**
	 * Для операционной системы FreeBSD
	 */
	#elif defined(__FreeBSD__)
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
		/**
		 * Если признак сверхстраниц недоступен
		 */
		#else
			// Размер не используется без поддержки сверхстраниц
			static_cast <void> (size);
			// Отвечаем отсутствием специальной области
			return nullptr;
		#endif
	/**
	 * Для систем без отдельного отведения крупных страниц
	 */
	#else
		/**
		 * Крупных страниц отдельным отображением эта система не даёт
		 *
		 * У macOS они просятся лишь через `mach_vm_allocate`, у NetBSD и OpenBSD их нет
		 * вовсе, а у систем Sun о них просят советом уже отображённой области - что и
		 * делается ниже, обычным путём
		 */
		static_cast <void> (size);
		// Отвечаем отсутствием специальной области
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
/**
 * Для систем с поддержкой совета отображённым страницам
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * @brief Функция совета системе собрать крупные страницы
	 *
	 * @param addr адрес отображённой области
	 * @param size размер отображённой области
	 *
	 */
	static void __awh_source_advise__(void * addr, const size_t size) noexcept {
		/**
		 * Если доступен совет о крупных страницах
		 */
		#if defined(MADV_HUGEPAGE)
			// Советуем системе собрать крупные страницы
			::madvise(addr, size, MADV_HUGEPAGE);
		/**
		 * Если доступно объединение страниц
		 */
		#elif defined(MADV_COLLAPSE)
			// Просим собрать область из крупных страниц
			::madvise(addr, size, MADV_COLLAPSE);
		/**
		 * Если советы о крупных страницах недоступны
		 */
		#else
			// Адрес не используется без поддержки совета
			static_cast <void> (addr);
			// Размер не используется без поддержки совета
			static_cast <void> (size);
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
	// Требуемое выравнивание не может быть меньше зернистости выдачи
	const size_t align = ((alignment > grain) ? alignment : grain);
	// Округляем требуемый размер до целого числа страниц
	const size_t rounded = (((size + (grain - 1)) / grain) * grain);
	// Отсекаем переполнение запаса под выравнивание на всех платформах
	if((align > grain) && (rounded > (static_cast <size_t> (-1) - align)))
		// Отвечаем отказом до обращения к системе
		return nullptr;
	// Резервируем округлённый размер окончательной области
	const source_reservation_t reservation(this->_taken, this->_ceiling, rounded, actual);
	// Если общий бюджет исчерпан
	if(!reservation.reserved())
		// Отвечаем отказом до обращения к системе
		return nullptr;
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
			// Считаем области, доставшиеся крупными страницами
			this->_superpaged.fetch_add(1, std::memory_order_relaxed);
			// Выводим адрес отведённой области
			return result;
		}
	}
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
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
	#if defined(_WIN32) || defined(_WIN64)
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
	 *
	 * У Linux укрытие даётся не отведением, а СОВЕТОМ следом за ним - `MADV_DONTDUMP`,
	 * с ядра 3.4. Оттого дорога его отдельная, ниже: просить `mmap` о том, чего тот не
	 * знает, бесполезно, а отказаться от укрытия вовсе значило бы оставить единственную
	 * систему без того, что она умеет
	 */
	#if defined(MAP_CONCEAL) || defined(MAP_NOCORE)
		// Получаем зернистость выдачи
		const size_t grain = this->granularity();
		// Если зернистость выдачи не определена
		if(grain == 0)
			// Выдавать нечего
			return nullptr;
		/**
		 * Отсекаем переполнение приведения к зернистости
		 *
		 * Обычная выдача (`alloc`) от него защищается тем же порядком: без сверки
		 * `size + grain - 1` завернулся бы за `SIZE_MAX`, `rounded` схлопнулся бы почти
		 * в ноль, `mmap` выдал бы крошечную область, а `actual` рапортовал бы её полной.
		 * Вход надуманный, но укрытая дорога держится наравне с обычной
		 */
		if(size > (static_cast <size_t> (-1) - (grain - 1)))
			// Отвечаем отказом: приведение к зернистости переполнилось бы
			return nullptr;
		// Округляем требуемый размер до целого числа страниц
		const size_t rounded = (((size + (grain - 1)) / grain) * grain);
		// Резервируем бюджет укрытой области наравне с обычной выдачей
		const source_reservation_t reservation(this->_taken, this->_ceiling, rounded, actual);
		// Если общего бюджета недостаточно
		if(!reservation.reserved())
			// Отвечаем отказом до обращения к системе
			return nullptr;
		/**
		 * Признак укрытия у своей системы
		 */
		#if defined(MAP_CONCEAL)
			// Область не попадает в снимок памяти при падении
			const int shelter = MAP_CONCEAL;
		/**
		 * Если укрытие задаётся признаком MAP_NOCORE
		 */
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
		// Отмечаем укрытие состоявшимся
		hidden = true;
		// Выводим адрес отведённой области
		return result;
	/**
	 * Если операционной системой является Linux
	 */
	#elif defined(MADV_DONTDUMP)
		// Отводим область обычным путём
		void * result = this->alloc(size, 0, actual);
		// Если область не отведена
		if(result == nullptr)
			// Выдавать нечего
			return nullptr;
		/**
		 * Укрываем область от снимка памяти советом системе
		 *
		 * Совет этот у Linux и есть всё укрытие: отведение о нём не знает вовсе. Отказ
		 * здесь беды не составляет - область отведена и годна, а обещание укрытия мы
		 * отдаём тем же честным признаком, каким отдаём его прочим системам
		 */
		hidden = (::madvise(result, ((actual > 0) ? actual : size), MADV_DONTDUMP) == 0);
		// Выводим адрес отведённой области
		return result;
	/**
	 * Если отдельное укрытие недоступно
	 */
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
	#if defined(_WIN32) || defined(_WIN64)
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
	/**
	 * Для операционной системы Linux
	 */
	#elif defined(__linux__)
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
	#if defined(_WIN32) || defined(_WIN64)
		// Отмечаем неиспользуемый параметр
		static_cast <void> (size);
		// Отдаём область системе
		const bool released = (::VirtualFree(addr, 0, MEM_RELEASE) != FALSE);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Отдаём область системе
		const bool released = (::munmap(addr, size) == 0);
	#endif
	// Не освобождаем бюджет области, оставшейся у системы
	if(!released)
		// Отвечаем отказом без изменения учёта
		return false;
	// Читаем занятый бюджет после успешной отдачи области
	size_t taken = this->_taken.load(std::memory_order_relaxed);
	/**
	 * Снимаем отданную область с учёта, сохраняя резервы соседних выдач
	 */
	while(!this->_taken.compare_exchange_weak(taken, ((taken > size) ? (taken - size) : 0), std::memory_order_relaxed, std::memory_order_relaxed));
	// Отвечаем успехом освобождения
	return true;
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
	#if defined(_WIN32) || defined(_WIN64)
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
