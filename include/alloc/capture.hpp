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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_CAPTURE__
#define __AWH_ALLOC_CAPTURE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "../sys/global.hpp"

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
		 * @brief Набор функций выделения памяти
		 *
		 * @note Одним и тем же видом описываются и наши функции, ставимые на место
		 *       прежних, и прежние, отдаваемые захватом обратно: договор у них общий
		 *
		 * \~english
		 * @brief Set of memory allocation functions
		 *
		 */
		typedef struct Functions {
			// Выделение памяти
			void * (* malloc)(size_t);
			// Освобождение памяти
			void (* free)(void *);
			// Выделение обнулённой памяти
			void * (* calloc)(size_t, size_t);
			// Изменение размера выделенной памяти
			void * (* realloc)(void *, size_t);
			// Определение размера выделенного блока
			size_t (* msize)(const void *);
			/**
			 * Выделение памяти с требуемым выравниванием
			 *
			 * Нужно не всякому приёму захвата: у систем ELF выравнивающая выдача
			 * библиотеки времени исполнения идёт своим путём и нашего malloc не зовёт.
			 * У macOS же зона обязана его иметь: библиотека зовёт `zone->memalign`
			 * БЕЗ проверки на пустоту, и пустое поле валит процесс обращением по нулю -
			 * проверено съёмом стека, где `xpc_atfork_child` шёл в `posix_memalign`
			 */
			// Выделение памяти с требуемым выравниванием
			void * (* memalign)(size_t, size_t);
			/**
			 * Выровненная семья MS Windows
			 *
			 * У MSVCRT выровненная выдача идёт СВОИМИ именами, и нашего `malloc` они не
			 * зовут: блок, выданный `_aligned_malloc`, шёл мимо распределителя целиком -
			 * ни учёт мест выдачи, ни заслоны, ни отчёт его не считали. Оттого семья эта
			 * перехватывается наравне с обычной выдачей
			 *
			 * Порядок доводов у `_aligned_malloc` ОБРАТЕН порядку `memalign`: размер
			 * идёт первым, выравнивание вторым. Держать их одним полем оттого нельзя
			 *
			 * Освобождать выданное обязан `_aligned_free`, а не `free`: у MSVCRT это
			 * разные пути, и перехватить один без другого значило бы отдать наш блок
			 * чужому распределителю. Семья потому и перехватывается ЦЕЛИКОМ либо не
			 * перехватывается вовсе
			 *
			 * Прочим системам поля эти не нужны и остаются пустыми: у ELF и macOS
			 * выровненная выдача идёт через `memalign` выше
			 */
			// Выделение памяти с требуемым выравниванием (размер, выравнивание)
			void * (* alignedAlloc)(size_t, size_t);
			// Освобождение памяти, выданной с выравниванием
			void (* alignedFree)(void *);
			// Изменение размера памяти, выданной с выравниванием
			void * (* alignedRealloc)(void *, size_t, size_t);
			// Определение размера памяти, выданной с выравниванием
			size_t (* alignedMsize)(void *, size_t, size_t);
			/**
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
			 */
			constexpr Functions() noexcept :
			 malloc(nullptr), free(nullptr), calloc(nullptr),
			 realloc(nullptr), msize(nullptr), memalign(nullptr),
			 alignedAlloc(nullptr), alignedFree(nullptr),
			 alignedRealloc(nullptr), alignedMsize(nullptr) {}
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
				 * @param hooks     our functions installed in place of the previous ones
				 * @param originals previous functions returned by the capture
				 * @return          flag of the capture having taken place
				 *
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
				 * @param ptr pointer being resolved
				 * @return    flag of belonging to the previous allocator
				 *
				 */
				virtual bool foreign(const void * ptr) const noexcept = 0;
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
				 */
				virtual const char * name() const noexcept = 0;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Capture() noexcept {}
		} capture_t;
	};
};

#endif // __AWH_ALLOC_CAPTURE__
