/**
 * @file spin.hpp
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
 * @brief Заголовочный файл замка распределителя памяти
 *
 * @section spin_decisions Намеренные решения
 *
 * @details <b>Замок свой, а не std::mutex.</b> Замок этот берётся из-под перехваченного
 *          malloc, а стандартный мьютекс волен на первом же захвате обратиться к
 *          выделению памяти - и обратиться к тому самому malloc, из-под которого его
 *          и взяли. Возвратность эту ничем не разорвать, оттого замок обходится одним
 *          знаком в памяти и ничего не выделяет вовсе.
 *
 *          <b>Подстановка обязательна.</b> Захват замка стоит на пути всякого
 *          выделения, и вызов сюда стоил бы дороже самого захвата. Оттого тело
 *          вынесено посредником с принудительной подстановкой, а не в файл кода.
 *
 *          <b>Кружение с уступкой, а не одно кружение.</b> Голое кружение на машине с
 *          одним ядром стоит целого кванта времени: удерживающий замок поток не
 *          получит его, пока кружащий не будет снят силой. Оттого после недолгого
 *          кружения поток уступает время сам.
 *
 * \~english
 * @brief Header file of the memory allocator lock
 *
 * @section spin_decisions Intentional decisions
 *
 * @details <b>The lock is ours, not std::mutex.</b> This lock is taken from under
 *          intercepted malloc, and a standard mutex may on its very first acquisition
 *          call memory allocation - and call that very malloc it was taken from under.
 *          This recursion cannot be broken, hence the lock is a single word in memory
 *          and allocates nothing at all.
 *
 *          <b>Inlining is mandatory.</b> Lock acquisition is on every allocation path,
 *          and a call here would cost more than the acquisition itself. Hence the body
 *          is placed inline with forced inlining, not in a code file.
 *
 *          <b>Spinning with yielding, not bare spinning.</b> Bare spinning on a
 *          single-core machine costs a whole time quantum: the thread holding the lock
 *          will not get it until the spinning thread is forcibly descheduled. Hence
 *          after brief spinning the thread yields time itself.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 * \~
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/macro/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_SPIN_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_SPIN_INLINE inline __attribute__((always_inline))
#endif

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
		 * @brief Класс замка распределителя памяти
		 *
		 * \~english
		 * @brief Memory allocator lock class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Spin {
			public:
				// Число оборотов кружения прежде уступки времени
				static constexpr uint32_t ROUNDS = 64;
				// Число уступок времени, после какого замок считается заклинившим
				static constexpr uint32_t STUCK = 1024;
			private:
				// Признак захваченности замка
				std::atomic <uint32_t> _held;
			private:
				/**
				 * \~russian
				 * @brief Метод захвата замка кружением
				 *
				 * @note Вынесен в файл кода: зовётся лишь при занятом замке
				 *
				 * \~english
				 * @brief Method of acquiring a contended lock
				 *
				 * @note Moved to a code file: called only when the lock is held
				 *
				 * \~
				 */
				void wait() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка
				 *
				 * \~english
				 * @brief Method of acquiring the lock
				 *
				 * \~
				 */
				AWH_SPIN_INLINE void acquire() noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения замка
				 *
				 * \~english
				 * @brief Method of releasing the lock
				 *
				 * \~
				 */
				AWH_SPIN_INLINE void release() noexcept;
				/**
				 * \~russian
				 * @brief Метод задания отклика на заклинивший замок
				 *
				 * @note Отклик зовётся, когда замок не отпускается тысячу уступок времени
				 *       подряд, - это уже не состязание потоков, а неотпущенный замок.
				 *       Распределитель вешает сюда разбор ветвления процесса: `fork`,
				 *       позванный в обход обёртки библиотеки времени исполнения, откликов
				 *       `pthread_atfork` не зовёт вовсе, и замок, захваченный не пережившим
				 *       ветвление потоком, иначе остаётся захваченным навсегда
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback for a stuck lock
				 *
				 * @note The callback is called when the lock is not released for a
				 *       thousand time yields in a row, - this is no longer thread
				 *       contention but an unreleased lock. The allocator hangs process
				 *       forking analysis here: `fork`, called bypassing the runtime
				 *       library wrapper, does not call `pthread_atfork` callbacks at
				 *       all, and a lock acquired by a thread that did not survive the
				 *       fork would otherwise remain acquired forever
				 *
				 * @param callback callback function
				 *
				 * \~
				 */
				static void onStuck(void (* callback)()) noexcept;
				/**
				 * \~russian
				 * @brief Метод принудительного освобождения замка
				 *
				 * @note Нужен потомку после ветвления процесса: замок, захваченный не
				 *       пережившим ветвление потоком, иначе остаётся захваченным навсегда
				 *
				 * \~english
				 * @brief Method of forcibly releasing the lock
				 *
				 * @note Needed by the child after process forking: a lock acquired by a
				 *       thread that did not survive the fork would otherwise remain
				 *       acquired forever
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @note Стоит он ЗДЕСЬ, а не в файле кода, вопреки общему укладу модуля,
				 *       и стоит намеренно: пометка `constexpr` требует тела на месте
				 *       объявления. Замки эти лежат переменными модуля, а захват выдачи
				 *       памяти случается ДО построения переменных модуля - приложение
				 *       вправе завести объект фреймворка переменной уровня файла. Без
				 *       `constexpr` собиратель заводит для переменной обычное построение,
				 *       и то ОБНУЛЯЕТ замок: захваченный к тому времени замок оказался бы
				 *       свободным, а держащий его поток - не держащим ничего
				 *
				 * @note Довод общий с набором функций захвата (`capture.hpp`); там же
				 *       записано, чем этот вид дефекта доказан
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @note It is HERE, not in a code file, contrary to the module's general
				 *       layout, and deliberately: the `constexpr` marker requires a body
				 *       at the declaration. These locks lie as module variables, and
				 *       memory allocation capture happens BEFORE module variables are
				 *       constructed - the application may create a framework object as
				 *       a file-level variable. Without `constexpr` the compiler creates
				 *       ordinary construction for the variable, and that ZEROES the lock:
				 *       a lock acquired by that time would appear free, and the thread
				 *       holding it would hold nothing
				 *
				 * @note The reasoning is shared with the capture function set
				 *       (`capture.hpp`); there it is also recorded how this kind of defect
				 *       was proven
				 *
				 * \~
				 */
				constexpr Spin() noexcept : _held(0) {}
		} spin_t;
		/**
		 * Сторож постоянного заведения замка
		 *
		 * Сличение это вычисляется на этапе сборки и требует, чтобы конструктор годился
		 * в постоянное выражение. Снимут `constexpr` - сборка встанет здесь
		 */
		static_assert((sizeof(spin_t) > 0) && [](){ constexpr Spin probe; static_cast <void> (probe); return true; }(), "замок обязан заводиться постоянным значением");
		/**
		 * \~russian
		 * @brief Класс удержания замка на время жизни
		 *
		 * \~english
		 * @brief Scoped lock holder class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Hold {
			private:
				// Удерживаемый замок
				spin_t & _spin;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param spin удерживаемый замок
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param spin lock to hold
				 *
				 * \~
				 */
				AWH_SPIN_INLINE Hold(spin_t & spin) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				AWH_SPIN_INLINE ~Hold() noexcept;
		} hold_t;
	};
};
