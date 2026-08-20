/**
 * @file pe.hpp
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
 * @brief Заголовочный файл захвата выделения памяти переписыванием входа функций —
 *        способ, применяемый у MS Windows, где формат PE подмены именами не даёт
 *
 * @section pe_decisions Намеренные решения
 *
 * @details <b>Разбирателя набора команд здесь нет и не будет.</b> gperftools переписывает
 *          вход разбирателем «ia32_*», и оттого на ARM64 отвечает отказом. Здесь разбор
 *          сведён к опознанию одного-единственного вида команды: вход всякой функции
 *          выделения памяти у «ucrtbase» начинается горячей заплаткой Microsoft -
 *          переходом через набивку, отведённым как раз под подмену. Цель того перехода
 *          и есть настоящее тело функции.
 *
 *          <b>Вход, заплаткой не начинающийся, отвергается.</b> Переписать начало,
 *          не разобрав его, значит испортить код библиотеки времени исполнения
 *          неисправимо. Оттого при несовпадении вида захват отвечает отказом, а
 *          распределитель остаётся системным.
 *
 * \~english
 * @brief Header file of memory allocation capture by rewriting function entries —
 *        the method used on MS Windows, where the PE format does not allow name
 *        substitution
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_PE__
#define __AWH_ALLOC_PE__

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "capture.hpp"

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
		 * @brief Класс захвата переписыванием входа функций
		 *
		 * \~english
		 * @brief Capture class by rewriting function entries
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ PECapture : public Capture {
			private:
				// Размер области подмены на входе функции
				static constexpr size_t PATCH_SIZE = 16;
				// Число подменяемых функций
				static constexpr size_t PATCH_COUNT = 5;
			private:
				/**
				 * @brief Структура одной наложенной подмены
				 *
				 */
				typedef struct Patch {
					// Адрес входа подменяемой функции
					void * entry;
					// Адрес настоящего тела, куда вёл переход входа
					void * body;
					// Прежнее содержимое области подмены
					uint8_t saved[PATCH_SIZE];
					// Признак наложенной подмены
					bool applied;
					/**
					 * @brief Конструктор
					 *
					 */
					Patch() noexcept :
					 entry(nullptr), body(nullptr), saved{0}, applied(false) {}
				} patch_t;
			private:
				// Наложенные подмены
				patch_t _patches[PATCH_COUNT];
				// Признак состоявшегося захвата
				bool _acquired;
			private:
				/**
				 * @brief Метод разбора цели перехода горячей заплатки
				 *
				 * @param entry адрес входа функции
				 * @return      адрес настоящего тела либо nullptr
				 *
				 */
				void * body(void * entry) const noexcept;
				/**
				 * @brief Метод наложения подмены на вход функции
				 *
				 * @param patch  сведения о подмене
				 * @param entry  адрес входа подменяемой функции
				 * @param target адрес подставляемой функции
				 * @return       признак успеха
				 *
				 */
				bool apply(patch_t & patch, void * entry, const void * target) noexcept;
				/**
				 * @brief Метод снятия подмены со входа функции
				 *
				 * @param patch сведения о подмене
				 *
				 */
				void revert(patch_t & patch) noexcept;
			public:
				/**
				 * @brief Метод захвата выделения памяти процесса
				 *
				 * @param hooks     наши функции, ставимые на место прежних
				 * @param originals прежние функции, отдаваемые захватом
				 * @return          признак состоявшегося захвата
				 *
				 */
				bool acquire(const functions_t & hooks, functions_t & originals) noexcept override;
				/**
				 * @brief Метод снятия захвата
				 *
				 */
				void release() noexcept override;
				/**
				 * @brief Метод определения состоявшегося захвата
				 *
				 * @return признак захвата
				 *
				 */
				bool acquired() const noexcept override;
			public:
				/**
				 * @brief Метод опознания указателя, выданного прежним распределителем
				 *
				 * @param ptr разбираемый указатель
				 * @return    признак принадлежности прежнему распределителю
				 *
				 */
				bool foreign(const void * ptr) const noexcept override;
			public:
				/**
				 * @brief Метод получения названия способа захвата
				 *
				 * @return название способа захвата
				 *
				 */
				const char * name() const noexcept override;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				PECapture() noexcept : _acquired(false) {}
				/**
				 * @brief Деструктор
				 *
				 */
				~PECapture() noexcept override;
		} pe_capture_t;
	};
};

#endif // _WIN32 || _WIN64

#endif // __AWH_ALLOC_PE__
