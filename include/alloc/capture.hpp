/**
 * @file capture.hpp
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
 * \~russian
 * @brief Заголовочный файл захвата выделения памяти процесса — договор между
 *        распределителем и тем, каким способом он заслоняет собою выделение памяти
 *
 * @section capture_decisions Намеренные решения
 *
 * @details <b>Способов три, и делятся они по приёму, а не по системе.</b> Подмена
 *          именами при связывании работает одинаково у Linux, всех BSD и Solaris -
 *          формат ELF её позволяет, и делить приём на семейства значило бы трижды
 *          написать одно. Своё есть лишь у двух: macOS требует зарегистрировать зону
 *          распределителя, а MS Windows не даёт подмены именами вовсе, и там начало
 *          функции переписывается переходом на себя.
 *
 *          <b>Захват возвращает прежние функции, а не прячет их.</b> Часть памяти
 *          процесса выдана до захвата, и освобождать её обязан тот, кто выдавал.
 *          Оттого договор требует не только поставить свои функции, но и отдать
 *          прежние - без них захват означал бы порчу чужой памяти.
 *
 *          <b>Опознание чужого указателя - дело захвата, а не кучи.</b> Куча знает
 *          лишь свои области; сказать, что указатель принадлежит прежнему
 *          распределителю, а не потерян вовсе, может только тот, кто этот
 *          распределитель заслонил. У MS Windows это перебор куч процесса, у прочих -
 *          отсутствие адреса в наших областях.
 *
 *          <b>Отказ захвата - обычный исход, а не исключение.</b> Способ может
 *          оказаться неприменим: у Windows вход функции может не нести горячей
 *          заплатки, у macOS зона может быть уже занята другим подменщиком. Тогда
 *          захват отвечает отказом, а распределитель остаётся системным. Переписывать
 *          вслепую нельзя: порча кода библиотеки времени исполнения неисправима.
 *
 * \~english
 * @brief Header file of process memory allocation capture — the contract between the
 *        allocator and the way it shadows memory allocation
 *
 * @section capture_decisions Intentional decisions
 *
 * @details <b>There are three methods, divided by technique, not by system.</b> Symbol
 *          interposition at link time works the same on Linux, all BSDs, and Solaris -
 *          the ELF format allows it, and dividing the technique by families would mean
 *          writing the same thing three times. Only two are special: macOS requires
 *          registering an allocator zone, and MS Windows does not allow symbol
 *          interposition at all, and there the function entry is rewritten with a jump
 *          to itself.
 *
 *          <b>Capture returns the previous functions, not hides them.</b> Part of the
 *          process memory was issued before capture, and it must be freed by whoever
 *          issued it. Hence the contract requires not only installing our functions but
 *          also returning the previous ones - without them capture would mean corrupting
 *          foreign memory.
 *
 *          <b>Identifying a foreign pointer is the capture's job, not the heap's.</b> The
 *          heap knows only its own regions; saying that a pointer belongs to the previous
 *          allocator rather than being lost entirely can only be done by whoever shadowed
 *          that allocator. On MS Windows this is scanning the process heaps; on others -
 *          the address not being in our regions.
 *
 *          <b>Capture refusal is a normal outcome, not an exception.</b> The method may
 *          prove inapplicable: on Windows the function entry may not carry a hot patch,
 *          on macOS the zone may already be taken by another interposer. Then capture
 *          answers with refusal, and the allocator remains the system one. Blind
 *          rewriting is not allowed: corrupting the runtime library code is irreparable.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Пространство имён фреймворка
 *
 * \~english
 * @brief Framework namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Memory allocator namespace
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Набор функций выделения памяти
		 *
		 * @note Одним и тем же видом описываются и наши функции, ставимые на место
		 *       прежних, и прежние, отдаваемые захватом обратно: договор у них общий
		 *
		 * \~english
		 * @brief Set of memory allocation functions
		 *
		 * @note The same type describes both our functions replacing the previous
		 *       ones and the previous ones returned by capture: they share the same
		 *       contract
		 *
		 * \~
		 */
		typedef struct Functions {
			/**
			 * \~russian
			 * @brief Функция выделения памяти
			 *
			 * \~english
			 * @brief Function that allocates memory
			 *
			 * \~
			 */
			void * (* malloc)(size_t);
			/**
			 * \~russian
			 * @brief Функция освобождения выделенной памяти
			 *
			 * \~english
			 * @brief Function that frees allocated memory
			 *
			 * \~
			 */
			void (* free)(void *);
			/**
			 * \~russian
			 * @brief Функция выделения обнулённой памяти
			 *
			 * \~english
			 * @brief Function that allocates zero-initialised memory
			 *
			 * \~
			 */
			void * (* calloc)(size_t, size_t);
			/**
			 * \~russian
			 * @brief Функция изменения размера выделенной памяти
			 *
			 * \~english
			 * @brief Function that resizes an allocated block
			 *
			 * \~
			 */
			void * (* realloc)(void *, size_t);
			/**
			 * \~russian
			 * @brief Функция определения размера выделенного блока
			 *
			 * \~english
			 * @brief Function that reports the size of an allocated block
			 *
			 * \~
			 */
			size_t (* msize)(const void *);
			/**
			 * \~russian
			 * @brief Функция выделения памяти с требуемым выравниванием
			 *
			 * @details Нужно не всякому приёму захвата: у систем ELF выравнивающая выдача
			 *          библиотеки времени исполнения идёт своим путём и нашего malloc не зовёт.
			 *          У macOS же зона обязана его иметь: библиотека зовёт `zone->memalign`
			 *          БЕЗ проверки на пустоту, и пустое поле валит процесс обращением по нулю -
			 *          проверено съёмом стека, где `xpc_atfork_child` шёл в `posix_memalign`.
			 *
			 * \~english
			 * @brief Memory allocation function with the required alignment
			 *
			 * @details Not every capture technique needs it: on ELF systems the runtime
			 *          library allocates aligned memory on its own and never calls our malloc.
			 *          On macOS, however, the zone must provide it: the library calls
			 *          `zone->memalign` WITHOUT checking for null, and an empty field crashes the
			 *          process through a null dereference - verified by a stack sample where
			 *          `xpc_atfork_child` went into `posix_memalign`.
			 *
			 * \~
			 */
			void * (* memalign)(size_t, size_t);
			/**
			 * \~russian
			 * @brief Функция выделения памяти с требуемым выравниванием (размер, выравнивание)
			 *
			 * @details Прочим системам поля эти не нужны и остаются пустыми: у ELF и macOS
			 *          выровненная выдача идёт через `memalign` выше.
			 *
			 * @note У MSVCRT выровненная выдача идёт СВОИМИ именами, и нашего `malloc` они не
			 *       зовут: блок, выданный `_aligned_malloc`, шёл мимо распределителя целиком -
			 *       ни учёт мест выдачи, ни заслоны, ни отчёт его не считали. Оттого семья эта
			 *       перехватывается наравне с обычной выдачей
			 *
			 * @note Порядок доводов у `_aligned_malloc` ОБРАТЕН порядку `memalign`: размер
			 *       идёт первым, выравнивание вторым. Держать их одним полем оттого нельзя
			 *
			 * @note Освобождать выданное обязан `_aligned_free`, а не `free`: у MSVCRT это
			 *       разные пути, и перехватить один без другого значило бы отдать наш блок
			 *       чужому распределителю. Семья потому и перехватывается ЦЕЛИКОМ либо не
			 *       перехватывается вовсе
			 *
			 * \~english
			 * @brief Aligned allocation function (size, alignment)
			 *
			 * @details Other systems leave these fields empty: on ELF and macOS aligned
			 *          allocation goes through `memalign` above.
			 *
			 * @note MSVCRT allocates aligned memory under ITS OWN names, and they never call
			 *       our `malloc`: a block issued by `_aligned_malloc` bypassed the allocator
			 *       entirely - neither allocation-site accounting, nor guards, nor reports
			 *       counted it. That is why this family is intercepted alongside ordinary
			 *       allocation
			 *
			 * @note The argument order of `_aligned_malloc` is the REVERSE of `memalign`:
			 *       size comes first, alignment second. That is why they cannot share one field
			 *
			 * @note Memory issued by `_aligned_malloc` must be freed by `_aligned_free`, not by
			 *       `free`: the paths differ in MSVCRT, and intercepting one without the other
			 *       would hand our block to a foreign allocator. That is why the family is
			 *       intercepted WHOLLY or not at all
			 *
			 * \~
			 */
			void * (* alignedAlloc)(size_t, size_t);
			/**
			 * \~russian
			 * @brief Функция освобождения памяти, выданной с выравниванием
			 *
			 * \~english
			 * @brief Function that frees aligned memory
			 *
			 * \~
			 */
			void (* alignedFree)(void *);
			/**
			 * \~russian
			 * @brief Функция изменения размера памяти, выданной с выравниванием
			 *
			 * \~english
			 * @brief Function that resizes an aligned block
			 *
			 * \~
			 */
			void * (* alignedRealloc)(void *, size_t, size_t);
			/**
			 * \~russian
			 * @brief Функция определения размера памяти, выданной с выравниванием
			 *
			 * \~english
			 * @brief Function that reports the size of an aligned block
			 *
			 * \~
			 */
			size_t (* alignedMsize)(void *, size_t, size_t);
			/**
			 * \~russian
			 * @brief Функция освобождения памяти через исходную CRT
			 *
			 * \~english
			 * @brief Function that frees memory through the original CRT
			 *
			 * \~
			 */
			void (* freeFrom)(void *, const Functions &);
			/**
			 * \~russian
			 * @brief Функция изменения размера через исходную CRT
			 *
			 * \~english
			 * @brief Function that resizes memory through the original CRT
			 *
			 * \~
			 */
			void * (* reallocFrom)(void *, size_t, const Functions &);
			/**
			 * \~russian
			 * @brief Функция определения размера через исходную CRT
			 *
			 * \~english
			 * @brief Function that reports memory size through the original CRT
			 *
			 * \~
			 */
			size_t (* msizeFrom)(const void *, const Functions &);
			/**
			 * \~russian
			 * @brief Функция освобождения выровненной памяти через исходную CRT
			 *
			 * \~english
			 * @brief Function that frees aligned memory through the original CRT
			 *
			 * \~
			 */
			void (* alignedFreeFrom)(void *, const Functions &);
			/**
			 * \~russian
			 * @brief Функция изменения размера выровненной памяти через исходную CRT
			 *
			 * \~english
			 * @brief Function that resizes aligned memory through the original CRT
			 *
			 * \~
			 */
			void * (* alignedReallocFrom)(void *, size_t, size_t, const Functions &);
			/**
			 * \~russian
			 * @brief Функция определения размера выровненной памяти через исходную CRT
			 *
			 * \~english
			 * @brief Function that reports aligned memory size through the original CRT
			 *
			 * \~
			 */
			size_t (* alignedMsizeFrom)(void *, size_t, size_t, const Functions &);
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @note Помечен `constexpr` НЕ ради скорости, а ради порядка построения, и
			 *       снимать пометку нельзя. Предмет этого вида лежит переменной модуля
			 *       (`__awh_zone_hooks__` у захвата зоной, `originals` у самого
			 *       распределителя), а захват выдачи памяти случается ДО построения
			 *       переменных модуля: приложение вправе завести объект фреймворка
			 *       переменной уровня файла, и тогда его конструктор идёт первым же
			 *       обходом построения. Без `constexpr` собиратель заводит для
			 *       переменной обычное построение, и то ЗАТИРАЕТ НУЛЯМИ уже
			 *       расставленные захватом отклики - следующая же выдача памяти уходит
			 *       по нулевому адресу. С `constexpr` переменная заводится постоянным
			 *       значением на этапе сборки, построения ей не заводится вовсе, и
			 *       затирать её некому
			 *
			 * @note Доказано сторожевой точкой отладчика: запись захвата по адресу
			 *       переменной, а следом - запись из `__cxx_global_var_init` того же
			 *       файла. Падение приходило на обращение по нулю без стека вовсе
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @note constexpr is required for initialization order, not speed. Instances
			 *       live at module scope (__awh_zone_hooks__ in zone capture and originals
			 *       in the allocator), while capture may happen before dynamic module
			 *       initialization. An application may construct a framework object at
			 *       file scope during the first initialization pass. Without constexpr,
			 *       the compiler emits a dynamic constructor that clears callbacks already
			 *       installed by capture. The next allocation would call a null pointer.
			 *       Constant initialization prepares the instance at build time and emits
			 *       no dynamic constructor that could erase the installed callbacks.
			 *
			 * @note A debugger watchpoint confirmed the capture write followed by a write
			 *       from __cxx_global_var_init in the same file. The subsequent null call
			 *       crashed before a usable stack trace could be obtained.
			 *
			 * \~
			 */
			constexpr Functions() noexcept :
			 malloc(nullptr), free(nullptr), calloc(nullptr),
			 realloc(nullptr), msize(nullptr), memalign(nullptr),
			 alignedAlloc(nullptr), alignedFree(nullptr),
			 alignedRealloc(nullptr), alignedMsize(nullptr),
			 freeFrom(nullptr), reallocFrom(nullptr), msizeFrom(nullptr),
			 alignedFreeFrom(nullptr), alignedReallocFrom(nullptr), alignedMsizeFrom(nullptr) {}
		} functions_t;
		/**
		 * Сторож постоянного заведения набора функций
		 *
		 * Сличение это вычисляется на этапе сборки и требует, чтобы конструктор годился
		 * в постоянное выражение. Снимут `constexpr` - сборка встанет здесь, а не
		 * обернётся падением до входа в `main` у потребителя
		 */
		static_assert((Functions().malloc == nullptr), "набор функций обязан заводиться постоянным значением");

		/**
		 * \~russian
		 * @brief Класс захвата выделения памяти процесса
		 *
		 * \~english
		 * @brief Process memory allocation capture class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Capture {
			public:
				/**
				 * \~russian
				 * @brief Метод захвата выделения памяти процесса
				 *
				 * @note Звать до порождения потоков: переписывание правит начала функций
				 *       уже загруженной библиотеки времени исполнения
				 *
				 * @param hooks     наши функции, ставимые на место прежних
				 * @param originals прежние функции, отдаваемые захватом
				 * @return          признак состоявшегося захвата
				 *
				 * \~english
				 * @brief Method of capturing process memory allocation
				 *
				 * @note Call before spawning threads: rewriting patches the entry points of
				 *       an already loaded runtime library
				 *
				 * @param hooks     our functions installed in place of the previous ones
				 * @param originals previous functions returned by the capture
				 * @return          flag of the capture having taken place
				 *
				 * \~
				 */
				virtual bool acquire(const functions_t & hooks, functions_t & originals) noexcept = 0;
				/**
				 * \~russian
				 * @brief Метод снятия захвата
				 *
				 * @note Снятие возвращает прежние функции на место, но выданную нами
				 *       память годной не оставляет само по себе: освобождать её обязан
				 *       тот, кто выдавал, и распределитель обязан пережить снятие
				 *
				 * \~english
				 * @brief Method of releasing the capture
				 *
				 * @note Releasing returns the previous functions to their place, but does
				 *       not by itself make memory issued by us valid: it must be freed by
				 *       whoever issued it, and the allocator must survive the release
				 *
				 * \~
				 */
				virtual void release() noexcept = 0;
				/**
				 * \~russian
				 * @brief Метод определения состоявшегося захвата
				 *
				 * @return признак захвата
				 *
				 * \~english
				 * @brief Method of determining whether the capture has taken place
				 *
				 * @return capture flag
				 *
				 * \~
				 */
				virtual bool acquired() const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Метод опознания указателя, выданного прежним распределителем
				 *
				 * @note Опознание требуется затем, что часть памяти процесса выдана до
				 *       захвата, и освобождать её обязан прежний распределитель
				 *
				 * @param ptr разбираемый указатель
				 * @return    признак принадлежности прежнему распределителю
				 *
				 * \~english
				 * @brief Method of identifying a pointer issued by the previous allocator
				 *
				 * @note Identification is needed because part of the process memory was
				 *       issued before capture, and it must be freed by the previous
				 *       allocator
				 *
				 * @param ptr pointer being resolved
				 * @return    flag of belonging to the previous allocator
				 *
				 * \~
				 */
				virtual bool foreign(const void * ptr) const noexcept = 0;
				/**
				 * \~russian
				 * @brief Метод определения владельца чужого блока
				 *
				 * @details Переписывание входа держит несколько библиотек времени
				 *          исполнения, и блок, выданный до захвата, принадлежит одной из
				 *          них: освобождать его обязан её же набор функций, иной - порча
				 *          чужой кучи. Звавший без контекста библиотеки (оператор языка,
				 *          общий хук) спрашивает владельца здесь
				 *
				 * @note Пустой набор означает «владелец неизвестен»: блок отдаётся
				 *       никому и утекает молча - так безопаснее, чем унести чужой блок
				 *       не в ту кучу. Нуль означает, что способ захвата не ведает
				 *       несколькими владельцами вовсе (одна библиотека на всю систему),
				 *       и звавший разбирается прежним порядком
				 *
				 * @param ptr разбираемый указатель
				 * @return    набор прежних функций владельца, пустой набор при
				 *            неизвестном владельце, либо nullptr
				 *
				 * \~english
				 * @brief Method of determining the owner of a foreign block
				 *
				 * @details Entry rewriting holds several runtimes, and a block issued
				 *          before the capture belongs to one of them: it must be freed
				 *          by that library's own function set - any other one corrupts
				 *          a foreign heap. The caller without library context (a language
				 *          operator, the generic hook) asks for the owner here
				 *
				 * @note An empty set means the owner is unknown: the block goes to
				 *       nobody and leaks silently - safer than carrying a foreign block
				 *       to the wrong heap. Nullptr means the capture method knows no
				 *       multiple owners at all (one library per system) and the caller
				 *       resolves it the usual way
				 *
				 * @param ptr pointer being resolved
				 * @return    original function set of the owner, an empty set for an
				 *            unknown owner, or nullptr
				 *
				 * \~
				 */
				virtual const functions_t * owner(const void * ptr) const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Метод получения названия способа захвата
				 *
				 * @return название способа захвата
				 *
				 * \~english
				 * @brief Method of getting the capture method name
				 *
				 * @return capture method name
				 *
				 * \~
				 */
				virtual const char * name() const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				virtual ~Capture() noexcept {}
		} capture_t;
	};
};
