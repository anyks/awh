/**
 * @file fiber.cpp
 * @date 2026-08-26
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
 * @brief Файл реализации модуля волокон
 *
 * @details Подкладок три, и выбираются они по системе:
 *          - ucontext: macOS, FreeBSD, DragonFly, NetBSD, Solaris, illumos, Linux
 *          - родные волокна: MS Windows
 *          - свой переключатель стека: OpenBSD, где ucontext удалён из системы вовсе
 *
 * @copyright Copyright © 2026
 */

/**
 * Если операционной системой является macOS
 */
#if __APPLE__
	/**
	 * Если признак стека системой не заведён, обходимся без него
	 */
	#ifndef _XOPEN_SOURCE
		/**
		 * Отпираем ucontext: у macOS они помечены устаревшими и без этого не видны
		 */
		#define _XOPEN_SOURCE 700
	#endif
	/**
	 * Возвращаем расширения BSD
	 *
	 * @warning Отпирание строгого POSIX выше прячет MAP_ANON ВМЕСТЕ с MAP_ANONYMOUS,
	 *          и отображение безымянной памяти становится недоступно вовсе
	 */
	#ifndef _DARWIN_C_SOURCE
		/**
		 * Возвращаем расширения BSD
		 */
		#define _DARWIN_C_SOURCE 1
	#endif
#endif

/**
 * Стандартные модули
 */
#include <new>
#include <utility>

/**
 * Подключаем заголовочный файл
 */
#include <sys/fiber.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Если операционной системой является macOS
 *
 * @note Функции ucontext помечены там устаревшими с версии 10.6 и предупреждают о
 *       себе на каждом вызове. Замены им у системы нет: обещанной заменой считаются
 *       потоки, а нам нужно ровно обратное - переключение стека БЕЗ потоков.
 *       Предупреждение потому и снимается здесь целиком, а не правкой по месту
 */
#if __APPLE__
	/**
	 * Добавляем в стек предупреждений о вызове устаревшей функции
	 */
	#pragma clang diagnostic push
	/**
	 * Игнорируем предупреждение о вызове устаревшей функции
	 */
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/**
 * Выбираем подкладку переключения стека
 */
#if defined(AWH_FIBER_FORCE_ASM)
	/**
	 * Подкладка выбрана принудительно: своим переключателем стека
	 */
	#define AWH_FIBER_ASM 1
/**
 * Если операционной системой является MS Windows
 */
#elif _WIN32 || _WIN64
	// Подкладкой служат родные волокна системы
	#define AWH_FIBER_WINAPI 1
	/**
	 * @warning Подключать <windows.h> напрямую нельзя: rpcndr.h заводит собственный
	 *          typedef byte, и при уже подключённом <cstddef> обращение к byte
	 *          становится двусмысленным — сборка у MinGW рассыпается прямо в заголовках
	 *          системы. Посредник win32.hpp ставит WIN32_LEAN_AND_MEAN, NOMINMAX
	 *          и порядок winsock2.h → windows.h за нас
	 */
	#include <sys/win32.hpp>
/**
 * Если операционной системой является OpenBSD
 */
#elif __OpenBSD__
	/**
	 * У OpenBSD ucontext удалён из системы: заголовка нет вовсе,
	 * и переключать стек приходится самим
	 */
	#define AWH_FIBER_ASM 1
/**
 * Если операционной системой является Linux, FreeBSD, DragonFly, NetBSD, Solaris или illumos
 */
#else
	// Подкладкой служит ucontext
	#define AWH_FIBER_UCONTEXT 1
	/**
	 * Системный заголовочный файл
	 */
	#include <ucontext.h>
#endif

/**
 * Если операционной системой является не MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системный заголовочный файл
	 */
	#include <sys/mman.h>
	/**
	 * Если признак стека системой не заведён, обходимся без него
	 *
	 * @warning У OpenBSD признак этот ОБЯЗАТЕЛЕН: память без него стеком быть не
	 *          вправе, и переход на неё валит процесс немедленно. Проверено опытом
	 *          на стенде: обычная память из кучи давала отказ сегментации на первом
	 *          же переходе в волокно
	 */
	#ifndef MAP_STACK
		/**
		 * Признак стека
		 */
		#define MAP_STACK 0
	#endif
	/**
	 * Если признак безымянного отображения назван по-иному
	 */
	#if !defined(MAP_ANON) && defined(MAP_ANONYMOUS)
		/**
		 * Признак безымянного отображения
		 */
		#define MAP_ANON MAP_ANONYMOUS
	#endif
#endif

/**
 * @brief Инкапсулируем статические функции в пространство имён awh
 *
 */
namespace awh {
	/**
	 * Если подкладкой служит свой переключатель стека
	 */
	#if AWH_FIBER_ASM
		/**
		 * @brief Функция входа волокна, зовётся переключателем по первому переходу
		 *
		 */
		extern "C" void __awh_fiber_start__(void) noexcept;
		/**
		 * @brief Функция переключения стека
		 *
		 * @details Сохраняются только те регистры, которые обязан сберечь вызываемый:
		 *          прочие вызывающая сторона сберегла сама, ещё до входа сюда. Возврат
		 *          в конце снимает со стека адрес, положенный туда при подготовке, - им
		 *          и делается первый переход в волокно.
		 *
		 * @param from куда сложить указатель стека покидаемой стороны
		 * @param to   указатель стека стороны, на которую выполняется переход
		 *
		 */
		extern "C" void __awh_fiber_swap__(void ** from, void * to);
		/**
		 * Если набором команд является x86-64
		 */
		#if __x86_64__
			/**
			 * Переключатель стека для x86-64
			 */
			__asm__(
				".text\n"
				".globl __awh_fiber_swap__\n"
				".globl ___awh_fiber_swap__\n"
				"__awh_fiber_swap__:\n"
				"___awh_fiber_swap__:\n"
				"	pushq %rbp\n"
				"	pushq %rbx\n"
				"	pushq %r12\n"
				"	pushq %r13\n"
				"	pushq %r14\n"
				"	pushq %r15\n"
				"	movq %rsp, (%rdi)\n"
				"	movq %rsi, %rsp\n"
				"	popq %r15\n"
				"	popq %r14\n"
				"	popq %r13\n"
				"	popq %r12\n"
				"	popq %rbx\n"
				"	popq %rbp\n"
				"	ret\n"
			);
		/**
		 * Если набором команд является ARM64
		 */
		#elif __aarch64__
			/**
			 * Переключатель стека для ARM64
			 */
			__asm__(
				".text\n"
				".globl __awh_fiber_swap__\n"
				".globl ___awh_fiber_swap__\n"
				"__awh_fiber_swap__:\n"
				"___awh_fiber_swap__:\n"
				"	sub sp, sp, #160\n"
				"	stp x19, x20, [sp, #0]\n"
				"	stp x21, x22, [sp, #16]\n"
				"	stp x23, x24, [sp, #32]\n"
				"	stp x25, x26, [sp, #48]\n"
				"	stp x27, x28, [sp, #64]\n"
				"	stp x29, x30, [sp, #80]\n"
				"	stp d8,  d9,  [sp, #96]\n"
				"	stp d10, d11, [sp, #112]\n"
				"	stp d12, d13, [sp, #128]\n"
				"	stp d14, d15, [sp, #144]\n"
				"	mov x2, sp\n"
				"	str x2, [x0]\n"
				"	mov sp, x1\n"
				"	ldp x19, x20, [sp, #0]\n"
				"	ldp x21, x22, [sp, #16]\n"
				"	ldp x23, x24, [sp, #32]\n"
				"	ldp x25, x26, [sp, #48]\n"
				"	ldp x27, x28, [sp, #64]\n"
				"	ldp x29, x30, [sp, #80]\n"
				"	ldp d8,  d9,  [sp, #96]\n"
				"	ldp d10, d11, [sp, #112]\n"
				"	ldp d12, d13, [sp, #128]\n"
				"	ldp d14, d15, [sp, #144]\n"
				"	add sp, sp, #160\n"
				"	ret\n"
			);
		#endif
	#endif

	/**
	 * @brief Класс волокна
	 *
	 */
	class fiber::Context {
		public:
			// Размер стека волокна
			size_t size;
		public:
			// Функция, выполняемая волокном
			fiber::task_t task;
			// Состояние волокна
			fiber::state_t state;
		public:
			/**
			 * Если подкладкой служит ucontext
			 */
			#if defined(AWH_FIBER_UCONTEXT)
				// Обстановка стороны, разбудившей волокно
				::ucontext_t caller;
				// Обстановка волокна
				::ucontext_t context;
			/**
			 * Если подкладкой служат родные волокна системы
			 */
			#elif defined(AWH_FIBER_WINAPI)
				// Волокно системы
				LPVOID handle;
				// Волокно стороны, разбудившей это волокно
				LPVOID caller;
			/**
			 * Если подкладкой служит свой переключатель стека
			 */
			#elif defined(AWH_FIBER_ASM)
				// Указатель стека волокна
				void * handle;
				// Указатель стека стороны, разбудившей волокно
				void * caller;
			#endif
		public:
			// Стек волокна
			char * stack;
		public:
			// Объект работы с логами
			const log_t * log;
		public:
			/**
			 * Если подкладкой служит ucontext
			 */
			#if defined(AWH_FIBER_UCONTEXT)
				/**
				 * @brief Конструктор
				 *
				 */
				Context() noexcept :
				 size(0), task(nullptr),
				 state(fiber::state_t::NONE),
				 stack(nullptr), log(nullptr) {}
			/**
			 * Если подкладкой служит ucontext
			 */
			#elif defined(AWH_FIBER_WINAPI)
				/**
				 * @brief Конструктор
				 *
				 */
				Context() noexcept :
				 size(0), task(nullptr),
				 state(fiber::state_t::NONE),
				 handle(nullptr), caller(nullptr),
				 stack(nullptr), log(nullptr) {}
			/**
			 * Если подкладкой служит свой переключатель стека
			 */
			#elif defined(AWH_FIBER_ASM)
				/**
				 * @brief Конструктор
				 *
				 */
				Context() noexcept :
				 size(0), task(nullptr),
				 state(fiber::state_t::NONE),
				 handle(nullptr), caller(nullptr),
				 stack(nullptr), log(nullptr) {}
			#endif
	};

	/**
	 * Волокно, в котором идёт выполнение, своё у каждого потока
	 */
	static thread_local fiber::ctx_t * __awh_fiber_current__ = nullptr;

	/**
	 * Если подкладкой служат родные волокна системы
	 */
	#if AWH_FIBER_WINAPI
		/**
		 * @brief Функция обращения потока в волокно
		 *
		 * @details Родные волокна MS Windows переключаются только между волокнами: обычный поток
		 *          волокном не является, GetCurrentFiber() отдаёт для него мусор из области
		 *          потока, а SwitchToFiber() из него не определён вовсе. Замерено на стенде
		 *          Windows 10 x86-64: без обращения потока проверка падала с нарушением доступа
		 *          на первом же пробуждении.
		 *
		 * @warning Обратно поток не обращается намеренно: волокном он остаётся до своего конца.
		 *          Обращение туда и обратно на каждом пробуждении стоило бы двух переходов в
		 *          ядро, а вреда от того, что поток остался волокном, нет
		 *
		 */
		static void __awh_fiber_thread__() noexcept {
			// Если поток волокном ещё не является
			if(!::IsThreadAFiber())
				// Обращаем поток в волокно
				::ConvertThreadToFiber(nullptr);
		}
	#endif

	/**
	 * @brief Функция выполнения работы волокна
	 *
	 * @param fiber волокно, работу которого следует выполнить
	 *
	 */
	static void __awh_fiber_body__(fiber::ctx_t * fiber) noexcept {
		// Если работа волокна задана
		if(fiber->task != nullptr)
			// Выполняем работу волокна
			fiber->task();
		// Отмечаем волокно доработавшим
		fiber->state = fiber::state_t::FINISHED;
		// Снимаем отметку о текущем волокне
		__awh_fiber_current__ = nullptr;
	}

	/**
	 * Если подкладкой служит ucontext
	 */
	#if AWH_FIBER_UCONTEXT
		/**
		 * @brief Функция входа волокна
		 *
		 */
		static void __awh_fiber_trampoline__() noexcept {
			// Получаем волокно, в котором идёт выполнение
			fiber::ctx_t * fiber = __awh_fiber_current__;
			// Выполняем работу волокна
			__awh_fiber_body__(fiber);
			// Возвращаем управление разбудившей стороне НАВСЕГДА
			::setcontext(&fiber->caller);
		}
	/**
	 * Если подкладкой служат родные волокна системы
	 */
	#elif AWH_FIBER_WINAPI
		/**
		 * @brief Функция входа волокна
		 *
		 * @param param волокно, работу которого следует выполнить
		 *
		 */
		static VOID CALLBACK __awh_fiber_trampoline__(LPVOID param){
			// Получаем волокно
			fiber::ctx_t * fiber = reinterpret_cast <fiber::ctx_t *> (param);
			// Выполняем работу волокна
			__awh_fiber_body__(fiber);
			// Возвращаем управление разбудившей стороне НАВСЕГДА
			::SwitchToFiber(fiber->caller);
		}
	/**
	 * Если подкладкой служит свой переключатель стека
	 */
	#elif AWH_FIBER_ASM
		/**
		 * @brief Функция входа волокна, зовётся переключателем по первому переходу
		 *
		 */
		extern "C" void __awh_fiber_start__(void) noexcept {
			// Получаем волокно, в котором идёт выполнение
			fiber::ctx_t * fiber = __awh_fiber_current__;
			// Выполняем работу волокна
			__awh_fiber_body__(fiber);
			// Место под указатель стека, который больше не понадобится
			void * finished = nullptr;
			// Возвращаем управление разбудившей стороне НАВСЕГДА
			__awh_fiber_swap__(&finished, fiber->caller);
		}
		/**
		 * @brief Функция подготовки стека волокна к первому переходу
		 *
		 * @details Переключатель заканчивается возвратом, и возврат этот обязан увести
		 *          управление во вход волокна. Значит на стек кладутся: адрес входа, а
		 *          под ним - место под сберегаемые регистры, которые переключатель снимет.
		 *
		 * @note Выравнивание: у x86-64 указатель стека при входе в функцию обязан быть
		 *       сравним с восемью по модулю шестнадцати - оттого над адресом входа
		 *       оставляется ещё восемь октетов пустоты
		 *
		 * @param top вершина стека волокна
		 * @return    указатель стека, готовый к первому переходу
		 *
		 */
		static void * __awh_fiber_prepare__(char * top) noexcept {
			/**
			 * Если набором команд является x86-64
			 */
			#if __x86_64__
				// Количество сберегаемых регистров: rbp, rbx, r12, r13, r14, r15
				constexpr size_t registers = 6;
				// Оставляем восемь октетов пустоты ради выравнивания
				top -= sizeof(void *);
			/**
			 * Если набором команд является ARM64
			 */
			#elif __aarch64__
				// Количество сберегаемых регистров: x19..x30 и d8..d15
				constexpr size_t registers = 20;
			/**
			 * Если набор команд не поддержан
			 */
			#else
				// Сберегать нечего: подкладка для этого набора команд не заведена
				constexpr size_t registers = 0;
			#endif
			// Указатель стека волокна
			void ** result = reinterpret_cast <void **> (top);
			/**
			 * Если набором команд является x86-64
			 */
			#if __x86_64__
				// Кладём адрес входа волокна: его снимет возврат переключателя
				*(--result) = reinterpret_cast <void *> (&__awh_fiber_start__);
			#endif
			/**
			 * Отводим место под сберегаемые регистры
			 */
			for(size_t i = 0; i < registers; i++)
				// Обнуляем очередной сберегаемый регистр
				*(--result) = nullptr;
			/**
			 * Если набором команд является ARM64
			 */
			#if __aarch64__
				// Кладём адрес входа волокна полем x30: возврат уводит управление по нему
				result[11] = reinterpret_cast <void *> (&__awh_fiber_start__);
			#endif
			// Выводим указатель стека, готовый к первому переходу
			return reinterpret_cast <void *> (result);
		}
	#endif
};

/**
 * @brief Функция усыпления текущего волокна
 *
 */
void awh::fiber::yield() noexcept {
	// Получаем волокно, в котором идёт выполнение
	ctx_t * fiber = __awh_fiber_current__;
	// Если выполнение идёт вне волокна, усыплять нечего
	if(fiber == nullptr)
		// Выходим, так как вызов сделан вне волокна
		return;
	// Отмечаем волокно спящим
	fiber->state = state_t::SUSPENDED;
	/**
	 * Если подкладкой служит ucontext
	 */
	#if AWH_FIBER_UCONTEXT
		// Возвращаем управление разбудившей стороне
		::swapcontext(&fiber->context, &fiber->caller);
	/**
	 * Если подкладкой служат родные волокна системы
	 */
	#elif AWH_FIBER_WINAPI
		// Возвращаем управление разбудившей стороне
		::SwitchToFiber(fiber->caller);
	/**
	 * Если подкладкой служит свой переключатель стека
	 */
	#elif AWH_FIBER_ASM
		// Возвращаем управление разбудившей стороне
		__awh_fiber_swap__(&fiber->handle, fiber->caller);
	#endif
	// Отмечаем волокно выполняющимся: сюда управление попадает уже после пробуждения
	fiber->state = state_t::RUNNING;
	// Возвращаем отметку о текущем волокне
	__awh_fiber_current__ = fiber;
}
/**
 * @brief Функция пробуждения волокна
 *
 * @param fiber волокно для пробуждения
 * @return      результат пробуждения
 *
 */
bool awh::fiber::resume(ctx_t * fiber) noexcept {
	// Если волокна нет, либо оно уже доработало, либо уже выполняется
	if((fiber == nullptr) || (fiber->state == state_t::FINISHED) || (fiber->state == state_t::RUNNING))
		// Сообщаем, что будить нечего
		return false;
	// Запоминаем волокно, из которого идёт пробуждение
	ctx_t * previous = __awh_fiber_current__;
	// Отмечаем волокно выполняющимся
	fiber->state = state_t::RUNNING;
	// Отмечаем волокно текущим
	__awh_fiber_current__ = fiber;
	/**
	 * Если подкладкой служит ucontext
	 */
	#if AWH_FIBER_UCONTEXT
		// Переходим в волокно, сложив обстановку разбудившей стороны
		::swapcontext(&fiber->caller, &fiber->context);
	/**
	 * Если подкладкой служат родные волокна системы
	 */
	#elif AWH_FIBER_WINAPI
		// Обращаем поток в волокно, иначе переходить не из чего
		__awh_fiber_thread__();
		// Запоминаем волокно разбудившей стороны
		fiber->caller = ::GetCurrentFiber();
		// Переходим в волокно
		::SwitchToFiber(fiber->handle);
	/**
	 * Если подкладкой служит свой переключатель стека
	 */
	#elif AWH_FIBER_ASM
		// Переходим в волокно, сложив указатель стека разбудившей стороны
		__awh_fiber_swap__(&fiber->caller, fiber->handle);
	#endif
	// Возвращаем прежнее текущее волокно
	__awh_fiber_current__ = previous;
	// Сообщаем, что пробуждение состоялось
	return true;
}
/**
 * @brief Функция уничтожения волокна
 *
 * @param fiber волокно для уничтожения
 * @return      результат уничтожения
 *
 */
bool awh::fiber::destroy(ctx_t * fiber) noexcept {
	// Если уничтожать нечего
	if(fiber == nullptr)
		// Сообщаем, что уничтожение не выполнено
		return false;
	/**
	 * Если волокно ещё спит, уничтожать его нельзя
	 *
	 * @warning Кадры спящего волокна не раскручены, и всё, что они держат, осталось
	 *          бы неосвобождённым: от объектов на его стеке до захваченных им ресурсов
	 */
	if(fiber->state == state_t::SUSPENDED){
		// Если объект логирования передан
		if(fiber->log != nullptr)
			// Записываем ошибку в лог
			fiber->log->print("Suspended fiber cannot be destroyed: its frames are not unwound", log_t::flag_t::WARNING);
		// Сообщаем, что уничтожение не выполнено
		return false;
	}
	/**
	 * Если волокно выполняется прямо сейчас, уничтожать его нельзя
	 *
	 * @warning Уничтожение снимает отображение стека волокна, а на этом самом стеке
	 *          лежит кадр текущего вызова: возврат пошёл бы по снятому отображению.
	 *          Отметка о текущем волокне указывала бы при этом на снесённый объект
	 *
	 * @note Достижимо это стало вместе с выводом наружу `current()`: пока указателя
	 *       на выполняющееся волокно взять было неоткуда, состояние это в уничтожение
	 *       не попадало вовсе, и проверки не требовалось
	 *
	 */
	if(fiber->state == state_t::RUNNING){
		// Если объект логирования передан
		if(fiber->log != nullptr)
			// Записываем ошибку в лог
			fiber->log->print("Running fiber cannot be destroyed: its stack is under the caller frame", log_t::flag_t::WARNING);
		// Сообщаем, что уничтожение не выполнено
		return false;
	}
	/**
	 * Если подкладкой служат родные волокна системы
	 */
	#if AWH_FIBER_WINAPI
		// Если волокно системы заведено
		if(fiber->handle != nullptr)
			// Удаляем волокно системы
			::DeleteFiber(fiber->handle);
	/**
	 * Если операционной системой является не MS Windows
	 */
	#else
		// Если стек волокна отведён
		if(fiber->stack != nullptr)
			// Снимаем отображение стека волокна
			::munmap(fiber->stack, fiber->size);
	#endif
	// Освобождаем волокно
	delete fiber;
	// Сообщаем, что уничтожение выполнено
	return true;
}
/**
 * @brief Функция получения состояния волокна
 *
 * @param fiber волокно для проверки
 * @return      состояние волокна
 *
 */
awh::fiber::state_t awh::fiber::state(const ctx_t * fiber) noexcept {
	// Выводим состояние волокна, считая несуществующее доработавшим
	return ((fiber != nullptr) ? fiber->state : state_t::FINISHED);
}
/**
 * @brief Функция получения волокна, в котором идёт выполнение
 *
 * @return волокно, либо nullptr, если выполнение идёт вне волокна
 *
 */
awh::fiber::Context * awh::fiber::current() noexcept {
	// Выводим волокно, в котором идёт выполнение
	return __awh_fiber_current__;
}
/**
 * @brief Функция заведения волокна
 *
 * @details Волокно заводится СПЯЩИМ: работа его начнётся первым пробуждением.
 *
 * @param task функция, выполняемая волокном
 * @param log  объект работы с логами
 * @return     заведённое волокно, либо nullptr при отказе
 *
 */
awh::fiber::Context * awh::fiber::spawn(task_t task, const log_t * log) noexcept {
	// Выводим заведённое волокно
	return awh::fiber::spawn(task, STACK_SIZE, log);
}
/**
 * @brief Функция заведения волокна
 *
 * @details Волокно заводится СПЯЩИМ: работа его начнётся первым пробуждением.
 *
 * @param task функция, выполняемая волокном
 * @param size размер стека волокна в октетах
 * @return     заведённое волокно, либо nullptr при отказе
 *
 */
awh::fiber::Context * awh::fiber::spawn(task_t task, const size_t size) noexcept {
	// Выводим заведённое волокно
	return awh::fiber::spawn(task, size, nullptr);
}
/**
 * @brief Функция заведения волокна
 *
 * @param task функция, выполняемая волокном
 * @param size размер стека волокна в октетах
 * @param log  объект работы с логами
 * @return     заведённое волокно, либо nullptr при отказе
 *
 */
awh::fiber::Context * awh::fiber::spawn(task_t task, const size_t size, const log_t * log) noexcept {
	// Если работа волокна не задана, заводить нечего
	if(task == nullptr){
		// Если объект логирования передан
		if(log != nullptr)
			// Записываем ошибку в лог
			log->print("Fiber cannot be spawned without a task", log_t::flag_t::WARNING);
		// Выводим пустой результат
		return nullptr;
	}
	// Заводим волокно
	ctx_t * result = new (std::nothrow) ctx_t();
	// Если волокно завести не удалось
	if(result == nullptr){
		// Если объект логирования передан
		if(log != nullptr)
			// Записываем ошибку в лог
			log->print("Fiber object cannot be allocated", log_t::flag_t::CRITICAL);
		// Выводим пустой результат
		return nullptr;
	}
	// Запоминаем объект работы с логами
	result->log = log;
	// Запоминаем размер стека волокна
	result->size = size;
	// Запоминаем работу волокна
	result->task = ::move(task);
	/**
	 * Если операционной системой является MS Windows
	 */
	#if _WIN32 || _WIN64
		// Стек волокну отводит сама система
		result->stack = nullptr;
	/**
	 * Если операционной системой является не MS Windows
	 */
	#else
		/**
		 * Отводим память под стек волокна отображением
		 *
		 * @warning Обычная память из кучи стеком быть не вправе у OpenBSD: там
		 *          отображение обязано нести признак MAP_STACK
		 */
		void * memory = ::mmap(nullptr, size, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANON | MAP_STACK), -1, 0);
		// Если память под стек отвести не удалось
		if(memory == MAP_FAILED){
			// Если объект логирования передан
			if(log != nullptr)
				// Записываем ошибку в лог
				log->print("Fiber stack of %zu bytes cannot be mapped", log_t::flag_t::CRITICAL, size);
			// Освобождаем волокно
			delete result;
			// Выводим пустой результат
			return nullptr;
		}
		// Запоминаем стек волокна
		result->stack = reinterpret_cast <char *> (memory);
	#endif
	/**
	 * Если подкладкой служит ucontext
	 */
	#if AWH_FIBER_UCONTEXT
		// Снимаем обстановку волокна
		::getcontext(&result->context);
		// Возврата по завершении нет: вход волокна уводит управление сам
		result->context.uc_link = nullptr;
		// Устанавливаем размер стека волокна
		result->context.uc_stack.ss_size = size;
		// Устанавливаем стек волокна
		result->context.uc_stack.ss_sp = result->stack;
		// Устанавливаем вход волокна
		::makecontext(&result->context, reinterpret_cast <void (*)(void)> (&__awh_fiber_trampoline__), 0);
	/**
	 * Если подкладкой служат родные волокна системы
	 */
	#elif AWH_FIBER_WINAPI
		// Заводим волокно системы
		result->handle = ::CreateFiber(size, &__awh_fiber_trampoline__, result);
		// Если волокно системы завести не удалось
		if(result->handle == nullptr){
			// Если объект логирования передан
			if(log != nullptr)
				// Записываем ошибку в лог
				log->print("System fiber cannot be created", log_t::flag_t::CRITICAL);
			// Освобождаем волокно
			delete result;
			// Выводим пустой результат
			return nullptr;
		}
	/**
	 * Если подкладкой служит свой переключатель стека
	 */
	#elif AWH_FIBER_ASM
		// Получаем вершину стека волокна
		char * top = (result->stack + size);
		// Выравниваем вершину стека по шестнадцати октетам
		top = reinterpret_cast <char *> (reinterpret_cast <uintptr_t> (top) & ~static_cast <uintptr_t> (15));
		// Готовим стек волокна к первому переходу
		result->handle = __awh_fiber_prepare__(top);
	#endif
	// Отмечаем волокно спящим: работа его начнётся первым пробуждением
	result->state = state_t::SUSPENDED;
	// Выводим заведённое волокно
	return result;
}

/**
 * Если операционной системой является macOS, возвращаем разбор предупреждений
 */
#if __APPLE__
	/**
	 * Возвращаем разбор предупреждений о вызове устаревшей функции
	 */
	#pragma clang diagnostic pop
#endif
