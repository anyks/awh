/**
 * @file assembly.hpp
 * @date 2026-08-02
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
 * @brief Заголовочный файл исполняемой памяти кодогенерации — класс Assembly, размещающий
 *        участок памяти, допускающий исполнение, наполняющий его порождённым машинным
 *        кодом и передающий ему управление
 *
 * @section assembly_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Участок памяти не бывает одновременно записываемым и исполняемым.</b>
 *          Разрешение записи и исполнения разом делает участок пригодным для подмены
 *          кода посторонним. Наполнение выполняется при разрешённой записи, передача
 *          управления - при разрешённом исполнении, и переход между этими
 *          состояниями возможен лишь целиком.
 *
 *          На платформах Apple с процессорами ARM64 права участка при размещении
 *          запрашиваются разом на запись и исполнение, и выглядит это отступлением
 *          от сказанного. Отступления нет: участок, размещённый с признаком
 *          «MAP_JIT», доступен потоку либо для записи, либо для исполнения,
 *          и состояние это принадлежит потоку, а не участку. Запрос прав
 *          при размещении лишь объявляет намерение, а действует переключение.
 *          Размещение без права исполнения пробовалось и даёт отказ шины
 *          при передаче управления участку.
 *
 *          <b>Кэш команд процессора сбрасывается явно.</b> Порождённый код
 *          записывается через кэш данных, тогда как выборка команд идёт через
 *          отдельный кэш, и на процессорах ARM64 согласованность их не обеспечивается
 *          самим оборудованием. Пропуск сброса даёт исполнение случайного содержимого
 *          памяти - отказ, воспроизводимый не всегда и потому особенно дорогой в разборе.
 *
 *          <b>Размер участка кратен размеру страницы памяти.</b> Права доступа
 *          устанавливаются страницами целиком, поэтому участок меньше страницы
 *          её всё равно занимает, а участок, страницу пересекающий, изменил бы
 *          права соседнего участка.
 *
 * \~english
 * @brief Header file of the executable memory of code generation — the Assembly class, which allocates
 *        a stretch of memory that admits execution, fills it with generated machine
 *        code and passes control to it
 * @section assembly_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>A stretch of memory is never writable and executable at the same time.</b>
 *          Allowing writing and execution at once makes the stretch fit for substituting
 *          the code by an outsider. Filling is performed while writing is allowed, passing
 *          control while execution is allowed, and the transition between those
 *          states is possible only as a whole.
 *          On Apple platforms with ARM64 processors the rights of the stretch are requested at allocation
 *          for writing and execution at once, and that looks like a departure
 *          from what was said. There is no departure: a stretch allocated with the
 *          «MAP_JIT» indication is available to the thread either for writing or for execution,
 *          and that state belongs to the thread rather than to the stretch. Requesting the rights
 *          at allocation only declares the intention, while it is the switching that takes effect.
 *          Allocation without the execution right was tried and yields a bus fault
 *          when control is passed to the stretch.
 *          <b>The instruction cache of the processor is flushed explicitly.</b> The generated code
 *          is written through the data cache, whereas instruction fetch goes through
 *          a separate cache, and on ARM64 processors their coherency is not ensured
 *          by the hardware itself. Skipping the flush yields execution of random content of
 *          memory — a fault that is not always reproducible and therefore especially expensive to investigate.
 *          <b>The size of the stretch is a multiple of the memory page size.</b> The access rights
 *          are set for whole pages, therefore a stretch smaller than a page
 *          occupies it all the same, and a stretch crossing a page would change
 *          the rights of the neighbouring stretch.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_ASSEMBLY__
#define __AWH_REGEX_ASSEMBLY__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "../sys/log.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;


	/**
	 * \~russian
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 * \~english
	 * @brief Namespace of the regular expression module
	 *
	 * \~
	 */
	namespace regex {
		/**
		 * \~russian
		 * @brief Класс исполняемой памяти кодогенерации
		 *
		 * @details Класс размещает участок памяти, допускающий исполнение, наполняет
		 *          его порождённым машинным кодом и переводит в состояние, пригодное
		 *          для передачи управления. Владение участком единоличное: копирование
		 *          запрещено, перемещение передаёт владение целиком.
		 *
		 * \~english
		 * @brief Class of the executable memory of code generation
		 * @details The class allocates a stretch of memory that admits execution, fills
		 *          it with generated machine code and moves it into a state fit
		 *          for passing control. Ownership of the stretch is exclusive: copying
		 *          is forbidden, moving hands over the ownership as a whole.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Assembly {
			private:
				// Адрес размещённого участка исполняемой памяти
				void * _address;
			private:
				// Размер размещённого участка исполняемой памяти в байтах
				size_t _size;
			private:
				// Количество байт, занятых порождённым машинным кодом
				size_t _length;
			private:
				// Флаг разрешения исполнения размещённого участка памяти
				bool _executable;
			private:
				// Объект журнала событий
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки поддержки исполняемой памяти сборкой
				 *
				 * @details Проверка касается размещения памяти, допускающей исполнение,
				 *          и от набора команд выполняющего процессора не зависит:
				 *          порождение машинного кода проверяется отдельно. Разделение
				 *          намеренное - иначе средства размещения памяти оставались бы
				 *          непроверенными на всех платформах, кодогенерации не получивших,
				 *          а именно они и понадобятся ей первыми.
				 *
				 * @return результат проверки поддержки исполняемой памяти сборкой
				 *
				 * \~english
				 * @brief Method of checking the support of executable memory by the build
				 * @details The check concerns the allocation of memory that admits execution
				 *          and does not depend on the instruction set of the executing processor:
				 *          the generation of machine code is checked separately. The separation
				 *          is deliberate — otherwise the means of memory allocation would remain
				 *          unchecked on all the platforms that have received no code generation,
				 *          and it is exactly those that it will need first.
				 * @return result of checking the support of executable memory by the build
				 *
				 * \~
				 */
				static bool available() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения участка исполняемой памяти
				 *
				 * @details Размещённый участок доступен для записи, но не для исполнения.
				 *          Повторное размещение освобождает участок, размещённый прежде.
				 *
				 * @param size требуемый размер участка памяти в байтах
				 * @return     результат размещения участка исполняемой памяти
				 *
				 * \~english
				 * @brief Method of allocating a stretch of executable memory
				 * @details The allocated stretch is available for writing but not for execution.
				 *          A repeated allocation frees the stretch allocated before.
				 * @param size required size of the stretch of memory in bytes
				 * @return     result of allocating the stretch of executable memory
				 *
				 * \~
				 */
				bool allocate(const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения участка исполняемой памяти
				 *
				 * \~english
				 * @brief Method of freeing the stretch of executable memory
				 *
				 * \~
				 */
				void release() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод наполнения участка памяти порождённым машинным кодом
				 *
				 * @details Наполнение допустимо лишь до разрешения исполнения. Код
				 *          записывается с начала участка, замещая записанный прежде.
				 *
				 * @param code   адрес порождённого машинного кода
				 * @param length размер порождённого машинного кода в байтах
				 * @return       результат наполнения участка памяти
				 *
				 * \~english
				 * @brief Method of filling the stretch of memory with generated machine code
				 * @details Filling is admissible only before execution is allowed. The code
				 *          is written from the beginning of the stretch, replacing what was written before.
				 * @param code   address of the generated machine code
				 * @param length size of the generated machine code in bytes
				 * @return       result of filling the stretch of memory
				 *
				 * \~
				 */
				bool fill(const void * code, const size_t length) noexcept;
				/**
				 * \~russian
				 * @brief Метод разрешения исполнения участка памяти
				 *
				 * @details Метод запрещает запись, разрешает исполнение и сбрасывает
				 *          кэш команд процессора, после чего участку допустимо
				 *          передавать управление.
				 *
				 * @return результат разрешения исполнения участка памяти
				 *
				 * \~english
				 * @brief Method of allowing execution of the stretch of memory
				 * @details The method forbids writing, allows execution and flushes
				 *          the instruction cache of the processor, after which control may be
				 *          passed to the stretch.
				 * @return result of allowing execution of the stretch of memory
				 *
				 * \~
				 */
				bool commit() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения адреса порождённого машинного кода
				 *
				 * @return адрес порождённого машинного кода либо нулевой указатель
				 *
				 * \~english
				 * @brief Method of getting the address of the generated machine code
				 * @return address of the generated machine code or a null pointer
				 *
				 * \~
				 */
				const void * entry() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения размера порождённого машинного кода
				 *
				 * @return размер порождённого машинного кода в байтах
				 *
				 * \~english
				 * @brief Method of getting the size of the generated machine code
				 * @return size of the generated machine code in bytes
				 *
				 * \~
				 */
				size_t length() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки разрешения исполнения участка памяти
				 *
				 * @return результат проверки разрешения исполнения участка памяти
				 *
				 * \~english
				 * @brief Method of checking whether execution of the stretch of memory is allowed
				 * @return result of checking whether execution of the stretch of memory is allowed
				 *
				 * \~
				 */
				bool executable() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор перемещения
				 *
				 * @param assembly перемещаемый объект исполняемой памяти
				 * @return         текущий объект исполняемой памяти
				 *
				 * \~english
				 * @brief Move operator
				 * @param assembly executable memory object to move
				 * @return         the current executable memory object
				 *
				 * \~
				 */
				Assembly & operator = (Assembly && assembly) noexcept;
				/**
				 * \~russian
				 * @brief Оператор присванивания
				 *
				 * \~english
				 * @brief Assignment operator
				 *
				 * \~
				 */
				Assembly & operator = (const Assembly &) noexcept = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор перемещения
				 *
				 * @param assembly перемещаемый объект исполняемой памяти
				 *
				 * \~english
				 * @brief Move constructor
				 * @param assembly executable memory object to move
				 *
				 * \~
				 */
				Assembly(Assembly && assembly) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 *
				 * \~english
				 * @brief Copy constructor
				 *
				 * \~
				 */
				Assembly(const Assembly &) noexcept = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param log объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Assembly(const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Assembly() noexcept;
		} assembly_t;
	};
};

#endif // __AWH_REGEX_ASSEMBLY__
