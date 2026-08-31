/**
 * @file fiber.hpp
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
 * @brief Заголовочный файл модуля волокон —
 *        сопрограммы со своим стеком, дающие синхронный вид коду, который на деле асинхронен
 *
 * @details Задача, ради которой модуль заведён: цикл опроса событий крутится в
 *          основном потоке, и остановить его нельзя - ни ожиданием на promise, ни
 *          чем бы то ни было ещё: остановив поток, останавливают и сам цикл, и
 *          ответ, которого ждут, не придёт никогда. Корутин в C++17 нет.
 *
 *          Волокно решает это переключением стека: вызов засыпает на СВОЁМ стеке,
 *          управление возвращается циклу, цикл работает как ни в чём не бывало, а
 *          отклик, получив ответ, будит волокно - и вызов продолжается со
 *          следующей строки, будто ожидание было обычным.
 *
 *          Потоков при этом не заводится ни одного, и цикл опроса не вызывается
 *          вложенно - то есть повторной входимости в самом горячем коде движка не
 *          возникает
 *
 * @note Правило одного направления: волокно вправе засыпать, а отклик, который его
 *       будит, идёт на стеке цикла и засыпать НЕ ВПРАВЕ
 *
 * \~english
 * @brief Header file of the fiber module —
 *        stackful coroutines giving synchronous shape to asynchronous code
 *
 * @details The task the module is made for: the event polling loop spins in the
 *          main thread, and it cannot be stopped - neither by waiting on a promise,
 *          nor by anything else: stopping the thread stops the loop itself, and the
 *          awaited answer never arrives. There are no coroutines in C++17.
 *
 *          A fiber solves this by switching the stack: the call falls asleep on ITS
 *          OWN stack, control returns to the loop, the loop works as if nothing had
 *          happened, and the callback, having received the answer, wakes the fiber
 *          up - and the call continues from the next line, as though the waiting had
 *          been an ordinary one.
 *
 *          No threads are made for this, and the polling loop is not called nested -
 *          that is, no re-entrancy arises in the hottest code of the engine
 *
 * @note The rule of a single direction: a fiber is allowed to fall asleep, while the
 *       callback that wakes it up runs on the stack of the loop and is NOT ALLOWED
 *       to fall asleep
 *
 * \~
 *
 * @copyright Copyright © 2026
 */

#ifndef __AWH_FIBER__
#define __AWH_FIBER__

/**
 * Стандартные модули
 */
#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * Подключаем наши модули
 */
#include "log.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён волокон
	 *
	 * \~english
	 * @brief Namespace of the fibers
	 *
	 * \~
	 */
	namespace fiber {
		/**
		 * \~russian
		 * @brief Прототип класса волокна
		 *
		 * \~english
		 * @brief Prototype of the fiber class
		 *
		 * \~
		 */
		class Context;
		/**
		 * \~russian
		 * @brief Тип контекста волокна
		 *
		 * \~english
		 * @brief Type of the fiber context
		 *
		 * \~
		 */
		using ctx_t = Context;
		/**
		 * \~russian
		 * @brief Тип функции, выполняемой волокном
		 *
		 * \~english
		 * @brief Type of the function performed by the fiber
		 *
		 * \~
		 */
		using task_t = function <void (void)>;
		/**
		 * \~russian
		 * @brief Состояние волокна
		 *
		 * \~english
		 * @brief State of the fiber
		 *
		 * \~
		 */
		enum class state_t : uint8_t {
			NONE      = 0x00, // Волокно заведено, но ещё не запускалось
			RUNNING   = 0x01, // Волокно выполняется прямо сейчас
			SUSPENDED = 0x02, // Волокно спит на своём стеке
			FINISHED  = 0x03  // Волокно доработало и подлежит уничтожению
		};
		/**
		 * \~russian
		 * @brief Размер стека волокна по умолчанию в октетах
		 *
		 * @note Шестидесяти четырёх килобайт хватает обычному обмену с запасом.
		 *       Считать этот размер стоит заранее: тысяча одновременных обменов
		 *       это шестьдесят четыре мегабайта отображённой памяти
		 *
		 * \~english
		 * @brief Default size of the fiber stack in octets
		 *
		 * @note Sixty four kilobytes are enough for an ordinary exchange with a
		 *       reserve. This size is worth counting beforehand: a thousand of the
		 *       simultaneous exchanges are sixty four megabytes of the mapped memory
		 *
		 * \~
		 */
		static constexpr size_t STACK_SIZE = 0x10000;
		/**
		 * \~russian
		 * @brief Функция усыпления текущего волокна
		 *
		 * @details Управление возвращается той стороне, которая волокно разбудила.
		 *          Кадр вызова со всеми его переменными остаётся жив на стеке волокна
		 *
		 * @note Зовётся ИЗ волокна. Вызов вне волокна не делает ничего
		 *
		 * \~english
		 * @brief Function of putting the current fiber to sleep
		 *
		 * @details Control returns to the side which has woken the fiber up. The call
		 *          frame with all of its variables stays alive on the fiber stack
		 *
		 * @note Is called FROM a fiber. A call outside of a fiber does nothing
		 *
		 * \~
		 */
		void yield() noexcept;
		/**
		 * \~russian
		 * @brief Функция пробуждения волокна
		 *
		 * @details Управление уходит в волокно и возвращается сюда, когда волокно
		 *          уснёт снова либо доработает
		 *
		 * @param fiber волокно для пробуждения
		 * @return      результат пробуждения
		 *
		 * \~english
		 * @brief Function of waking the fiber up
		 *
		 * @details Control goes into the fiber and returns here when the fiber falls
		 *          asleep again or finishes its work
		 *
		 * @param fiber fiber to wake up
		 * @return      result of the wake-up
		 *
		 * \~
		 */
		bool resume(ctx_t * fiber) noexcept;
		/**
		 * \~russian
		 * @brief Функция уничтожения волокна
		 *
		 * @warning Уничтожать волокно, которое ещё спит, НЕЛЬЗЯ: его кадры не
		 *          раскручены, и всё, что они держат, останется неосвобождённым
		 *
		 * @warning Уничтожать волокно, которое выполняется прямо сейчас, тоже НЕЛЬЗЯ:
		 *          уничтожение снимает отображение его стека, а на этом самом стеке
		 *          лежит кадр вызывающего - возврат пошёл бы по снятому отображению
		 *
		 * @param fiber волокно для уничтожения
		 * @return      результат уничтожения
		 *
		 * \~english
		 * @brief Function of destroying the fiber
		 *
		 * @warning Destroying a fiber which is still sleeping is NOT ALLOWED: its
		 *          frames are not unwound, and everything they hold would stay unfreed
		 *
		 * @warning Destroying a fiber which is running right now is NOT ALLOWED either:
		 *          the destruction unmaps its stack, while the frame of the caller lies
		 *          on that very stack - the return would go over the unmapped memory
		 *
		 * @param fiber fiber to destroy
		 * @return      result of the destruction
		 *
		 * \~
		 */
		bool destroy(ctx_t * fiber) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения состояния волокна
		 *
		 * @param fiber волокно для проверки
		 * @return      состояние волокна
		 *
		 * \~english
		 * @brief Function of getting the state of the fiber
		 *
		 * @param fiber fiber to check
		 * @return      state of the fiber
		 *
		 * \~
		 */
		state_t state(const ctx_t * fiber) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения волокна, в котором идёт выполнение
		 *
		 * @return волокно, либо nullptr, если выполнение идёт вне волокна
		 *
		 * \~english
		 * @brief Function of getting the fiber the execution runs in
		 *
		 * @return fiber, or nullptr if the execution runs outside of a fiber
		 *
		 * \~
		 */
		ctx_t * current() noexcept;
		/**
		 * \~russian
		 * @brief Функция заведения волокна
		 *
		 * @details Волокно заводится СПЯЩИМ: работа его начнётся первым пробуждением
		 *
		 * @param task функция, выполняемая волокном
		 * @param size размер стека волокна в октетах
		 * @return     заведённое волокно, либо nullptr при отказе
		 *
		 * \~english
		 * @brief Function of making the fiber
		 *
		 * @details The fiber is made SLEEPING: its work begins with the first wake-up
		 *
		 * @param task function performed by the fiber
		 * @param size size of the fiber stack in octets
		 * @return     made fiber, or nullptr on failure
		 *
		 * \~
		 */
		ctx_t * spawn(task_t task, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция заведения волокна
		 *
		 * @details Волокно заводится СПЯЩИМ: работа его начнётся первым пробуждением
		 *
		 * @param task функция, выполняемая волокном
		 * @param log  объект работы с логами
		 * @return     заведённое волокно, либо nullptr при отказе
		 *
		 * \~english
		 * @brief Function of making the fiber
		 *
		 * @details The fiber is made SLEEPING: its work begins with the first wake-up
		 *
		 * @param task function performed by the fiber
		 * @param log  object of the working with logs
		 * @return     made fiber, or nullptr on failure
		 *
		 * \~
		 */
		ctx_t * spawn(task_t task, const log_t * log) noexcept;
		/**
		 * \~russian
		 * @brief Функция заведения волокна
		 *
		 * @details Волокно заводится СПЯЩИМ: работа его начнётся первым пробуждением
		 *
		 * @param task функция, выполняемая волокном
		 * @param size размер стека волокна в октетах
		 * @param log  объект работы с логами
		 * @return     заведённое волокно, либо nullptr при отказе
		 *
		 * \~english
		 * @brief Function of making the fiber
		 *
		 * @details The fiber is made SLEEPING: its work begins with the first wake-up
		 *
		 * @param task function performed by the fiber
		 * @param size size of the fiber stack in octets
		 * @param log  object of the working with logs
		 * @return     made fiber, or nullptr on failure
		 *
		 * \~
		 */
		ctx_t * spawn(task_t task, const size_t size = STACK_SIZE, const log_t * log = nullptr) noexcept;
	};
};

#endif // __AWH_FIBER__
