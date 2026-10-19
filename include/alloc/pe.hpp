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
 * @details <b>Приём подмены свой у каждого набора команд.</b> У ARM64 вход всякой
 *          функции выделения памяти у «ucrtbase» начинается горячей заплаткой
 *          Microsoft - переходом через набивку, отведённым как раз под подмену. Цель
 *          того перехода и есть настоящее тело функции, и разбор сводится к опознанию
 *          одного-единственного вида команды. Вход, заплаткой не начинающийся, там
 *          отвергается: переписать начало, не разобрав его, значит испортить код
 *          библиотеки времени исполнения неисправимо.
 *
 *          <b>У x86-64 заплатки на входе нет, и разбиратель длин команд необходим.</b>
 *          Там начало входа занято настоящими командами: подмена ВЫТЕСНЯЕТ их, а
 *          вытесненные переносятся в переходник, откуда исполнение возвращается за
 *          место подмены. Перенести команду, не зная её длины, нельзя, а длину даёт
 *          лишь разбор. Разбиратель этот узкий - он отвечает на один вопрос, «сколько
 *          байт занимает команда», - и на неизвестной команде отказывает, а не гадает.
 *          Прежде здесь было записано, что разбирателя не будет вовсе; запись отстала
 *          от кода, и путь x86-64 без него не существовал бы.
 *
 *          <b>Подмена накладывается БЕЗ остановки прочих потоков.</b> Байты входа
 *          переписываются не одним действием, и поток, исполняющий в этот миг ровно
 *          эти байты, увидел бы половину подмены. Остановка потоков процесса ради этого
 *          не делается: захват идёт при заведении фреймворка, то есть до порождения
 *          рабочих потоков, а цена остановки - право отладчика и опасность
 *          взаимоблокировки. Кто зовёт захват из работающего многопоточного процесса,
 *          обязан знать об этом сам.
 *
 * \~english
 * @brief Header file of memory allocation capture by rewriting function entries —
 *        the method used on MS Windows, where the PE format does not allow name
 *        substitution
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)

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
				// Число обязательных подменяемых функций
				static constexpr size_t PATCH_COUNT = 5;
				/**
				 * Выровненная семья идёт группой НЕОБЯЗАТЕЛЬНОЙ
				 *
				 * Обязательная пятёрка перехватывается по правилу «всё или ничего»:
				 * не найдись хоть одно имя - захват отказывает, не тронув ничего.
				 * Держи мы выровненную семью в том же числе, редкая библиотека без
				 * одного из её имён лишала бы распределителя захвата ЦЕЛИКОМ - а он
				 * прекрасно работает и без выровненной выдачи
				 *
				 * Внутри же группы правило то же самое: `_aligned_malloc` без
				 * `_aligned_free` отдал бы наш блок чужому распределителю. Оттого
				 * семья берётся вся либо не берётся вовсе
				 */
				// Номер первой функции выровненной семьи
				static constexpr size_t ALIGNED_FIRST = 5;
				/**
				 * Ядро семьи - выдача, освобождение и смена размера
				 *
				 * Эти три обязаны лечь вместе: `_aligned_malloc` без `_aligned_free`
				 * отдал бы наш блок чужому распределителю
				 */
				// Число обязательных функций выровненной семьи
				static constexpr size_t ALIGNED_CORE = 3;
				/**
				 * Измерение идёт СВЕРХ ядра и обязательным не считается
				 *
				 * `_aligned_msize` экспортируется не всякой библиотекой: `msvcrt.dll`
				 * на x86-64 его не отдаёт вовсе - щуп получил нулевой адрес, - тогда
				 * как `ucrtbase.dll` отдаёт. Требуй мы его наравне с ядром, вся
				 * выровненная семья не бралась бы на x86-64 из-за имени, какого там
				 * нет: не экспортированного никто и не позовёт
				 */
				// Число функций выровненной семьи
				static constexpr size_t ALIGNED_COUNT = 4;
				// Общее число подменяемых функций
				static constexpr size_t PATCH_TOTAL = (PATCH_COUNT + ALIGNED_COUNT);
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
					/**
					 * Число байт, вытесненных подменой
					 *
					 * У ARM64 оно всегда равно области подмены: вход там - горячая
					 * заплатка, отведённая Microsoft под ровно эту надобность. У x86-64
					 * заплатки нет вовсе, и подмена вытесняет НАСТОЯЩИЕ команды входа -
					 * столько, сколько их укладывается в пять байт перехода
					 */
					// Число байт, вытесненных подменой
					size_t moved;
					// Признак наложенной подмены
					bool applied;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Patch() noexcept;
				} patch_t;
			private:
				// Число поддерживаемых библиотек времени исполнения
				static constexpr size_t LIBRARY_COUNT = 2;
				// Наложенные подмены каждой библиотеки
				patch_t _patches[LIBRARY_COUNT][PATCH_TOTAL];
				// Номер библиотеки, для которой строится переходник
				size_t _library;
				/**
				 * Область под переходники
				 *
				 * Нужна лишь x86-64: переход по относительному смещению достаёт не далее
				 * двух гигабайт, а наши функции лежат в образе программы - от библиотеки
				 * времени исполнения это семь гигабайт и более. Оттого вход прыгает на
				 * переходник ПОБЛИЗОСТИ, а тот уже - абсолютным переходом к нам
				 */
				// Область под переходники
				void * _arena[LIBRARY_COUNT];
				// Занято в области под переходники
				size_t _arenaUsed[LIBRARY_COUNT];
				// Размер области под переходники
				size_t _arenaSize[LIBRARY_COUNT];
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
				 * \~russian
				 * @brief Метод измерения длины команд входа функции
				 *
				 * @note Разбиратель узкий НАМЕРЕННО: он знает лишь те виды команд, какие
				 *       встречаются в начале функций, и на всяком незнакомом отвечает
				 *       отказом. Переписывать вслепую нельзя - порча кода библиотеки
				 *       времени исполнения неисправима
				 *
				 * @param entry   адрес входа функции
				 * @param need    требуемое число байт
				 * @param offsets массив под смещения начал команд, либо nullptr
				 * @param count   число разобранных команд, либо nullptr
				 * @return        длина целого числа команд не менее требуемой, либо нуль
				 *
				 * \~english
				 * @brief Method of measuring the length of the function entry instructions
				 *
				 */
				size_t prologue(const void * entry, const size_t need, size_t * offsets, size_t * count) const noexcept;
				/**
				 * \~russian
				 * @brief Метод определения пригодности входа функции к подмене
				 *
				 * @param entry адрес входа функции
				 * @return      признак пригодности входа
				 *
				 * \~english
				 * @brief Method of determining whether a function entry suits patching
				 *
				 */
				bool suits(void * entry) const noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи памяти под переходник вблизи образа
				 *
				 * @param anchor адрес, вблизи которого нужна память
				 * @param size   требуемый размер в байтах
				 * @return       адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating trampoline memory near an image
				 *
				 */
				void * nearby(const void * anchor, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод наложения подмены на вход функции
				 *
				 * @param patch     сведения о подмене
				 * @param entry     адрес входа подменяемой функции
				 * @param target    адрес подставляемой функции
				 * @param originals прежние функции исходной библиотеки
				 * @param index     номер подменяемой функции
				 * @return          признак успеха
				 *
				 * \~english
				 * @brief Method of patching a function entry
				 *
				 * @param patch     patch information
				 * @param entry     entry address of the original function
				 * @param target    replacement function address
				 * @param originals original functions of the source library
				 * @param index     index of the patched function
				 * @return          success flag
				 *
				 * \~
				 */
				bool apply(patch_t & patch, void * entry, const void * target, functions_t & originals, const size_t index) noexcept;
				/**
				 * @brief Метод снятия подмены со входа функции
				 *
				 * @param patch сведения о подмене
				 *
				 */
				void revert(patch_t & patch) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата выделения памяти загруженных CRT
				 *
				 * @note Обязательные функции всех найденных CRT захватываются совместно.
				 *       Отклики freeFrom, reallocFrom и msizeFrom обязательны: прежние блоки
				 *       возвращаются той библиотеке, через которую пришёл вызов.
				 *       Захват выполняется до запуска потоков, вызывающих функции CRT.
				 *
				 * @param hooks     наши функции, ставимые на место прежних
				 * @param originals прежние функции первой найденной CRT для прямых вызовов
				 * @return          признак состоявшегося захвата
				 *
				 * \~english
				 * @brief Method of capturing allocation in loaded CRTs
				 *
				 * @note Mandatory functions of all detected CRTs are captured together.
				 *       The freeFrom, reallocFrom and msizeFrom callbacks are required: legacy
				 *       blocks return to the library through which the call arrived.
				 *       Capture must run before starting threads that call CRT functions.
				 *
				 * @param hooks     our functions installed in place of the original ones
				 * @param originals original functions of the first detected CRT for direct calls
				 * @return          successful capture flag
				 *
				 * \~
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
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				PECapture() noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~PECapture() noexcept override;
		} pe_capture_t;
	};
};

#endif
