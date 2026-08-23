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
					 * @brief Конструктор
					 *
					 */
					Patch() noexcept :
					 entry(nullptr), body(nullptr), saved{0}, moved(0), applied(false) {}
				} patch_t;
			private:
				// Наложенные подмены
				patch_t _patches[PATCH_COUNT];
				/**
				 * Область под переходники
				 *
				 * Нужна лишь x86-64: переход по относительному смещению достаёт не далее
				 * двух гигабайт, а наши функции лежат в образе программы - от библиотеки
				 * времени исполнения это семь гигабайт и более. Оттого вход прыгает на
				 * переходник ПОБЛИЗОСТИ, а тот уже - абсолютным переходом к нам
				 */
				// Область под переходники
				void * _arena;
				// Занято в области под переходники
				size_t _arenaUsed;
				// Размер области под переходники
				size_t _arenaSize;
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
				 * @param size  требуемый размер в байтах
				 * @return      адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating trampoline memory near an image
				 *
				 */
				void * nearby(const void * anchor, const size_t size) noexcept;
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
				PECapture() noexcept :
				 _arena(nullptr), _arenaUsed(0), _arenaSize(0), _acquired(false) {}
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
