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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_SPIN__
#define __AWH_ALLOC_SPIN__

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstdint>

/**
 * Наши модули
 */
#include "../sys/global.hpp"

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
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён распределителя памяти
	 *
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс замка распределителя памяти
		 *
		 * \~english
		 * @brief Memory allocator lock class
		 *
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
				 */
				AWH_SPIN_INLINE void acquire() noexcept {
					// Значение незахваченного замка
					uint32_t free = 0;
					// Пробуем захватить замок разом
					if(!this->_held.compare_exchange_weak(free, 1, std::memory_order_acquire, std::memory_order_relaxed))
						// Захватываем замок кружением
						this->wait();
				}
				/**
				 * \~russian
				 * @brief Метод освобождения замка
				 *
				 * \~english
				 * @brief Method of releasing the lock
				 *
				 */
				AWH_SPIN_INLINE void release() noexcept {
					// Освобождаем замок
					this->_held.store(0, std::memory_order_release);
				}
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
				 */
				void reset() noexcept;
			public:
				/**
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
		 */
		typedef class __AWH_SHARED_EXPORT__ Hold {
			private:
				// Удерживаемый замок
				spin_t & _spin;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param spin удерживаемый замок
				 *
				 */
				AWH_SPIN_INLINE Hold(spin_t & spin) noexcept : _spin(spin) {
					// Захватываем замок
					this->_spin.acquire();
				}
				/**
				 * @brief Деструктор
				 *
				 */
				AWH_SPIN_INLINE ~Hold() noexcept {
					// Освобождаем замок
					this->_spin.release();
				}
		} hold_t;
	};
};

#endif // __AWH_ALLOC_SPIN__
