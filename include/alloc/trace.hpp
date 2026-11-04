/**
 * @file trace.hpp
 * @date 2026-08-21
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
 * @brief Заголовочный файл съёма стека вызовов — запоминание места выдачи памяти и
 *        обращение снятых адресов в имена
 *
 * @section trace_decisions Намеренные решения
 *
 * @details <b>Съём идёт раскруткой, а не `backtrace` из execinfo.h.</b> Заголовка того
 *          нет у musl вовсе, у FreeBSD и NetBSD он требует отдельной библиотеки
 *          (`-lexecinfo`), а у glibc `backtrace` при первом обращении подгружает
 *          libgcc_s через `dlopen` - то есть ВЫДЕЛЯЕТ ПАМЯТЬ, а мы стоим внутри
 *          выдачи памяти. Раскрутка же (`_Unwind_Backtrace`) есть у всех собирателей,
 *          какими мы собираемся, и связывается при загрузке.
 *
 *          <b>У MS Windows съём свой.</b> Раскрутки того же вида там нет, а
 *          `RtlCaptureStackBackTrace` снимает стек без памяти и без замков - это родной
 *          путь системы, и подражать ему чужим незачем.
 *
 *          <b>Прогрев обязателен.</b> Первое обращение к раскрутке строит указатели
 *          разделов раскрутки и у части систем выделяет память. Оттого съём
 *          прогревается ОДНАЖДЫ при заведении - вне всякого выделения, - и дальше
 *          памяти не просит. Проверяется это не рассуждением, а наблюдением: щуп
 *          считает обращения к нашему же malloc вокруг съёма.
 *
 *          <b>Возвратность разрывается признаком потока, а не запретом.</b> Раскрутка
 *          вольна обратиться за памятью, и обращение это пришло бы обратно в съём.
 *          Признак «мы внутри съёма» хранится ключом системы, а НЕ поток-локальным
 *          местом: ленивое заведение того само зовёт выделение памяти (проверено срывом
 *          стека на macOS внутри `_tlv_get_addr`).
 *
 *          <b>Имена добываются отдельно от съёма.</b> Обращение адреса в имя (`dladdr`,
 *          перебор образов у MS Windows) стоит дорого и памяти просит. Съём же обязан
 *          быть дёшев: он идёт на пути выдачи. Оттого снятое хранится адресами, а имена
 *          добываются потом - при докладе.
 *
 * \~english
 * @brief Header file of call stack capture — remembering the allocation site and
 *        resolving captured addresses into names
 *
 * @section trace_decisions Intentional decisions
 *
 * @details <b>Capture uses unwinding, not `backtrace` from execinfo.h.</b> That header
 *          does not exist on musl at all, on FreeBSD and NetBSD it requires a separate
 *          library (`-lexecinfo`), and on glibc `backtrace` on first use loads libgcc_s
 *          via `dlopen` - i.e. ALLOCATES MEMORY, while we are inside memory allocation.
 *          Unwinding (`_Unwind_Backtrace`) is available on all compilers we build with
 *          and links at load time.
 *
 *          <b>On MS Windows capture is native.</b> There is no unwinding of the same
 *          kind, and `RtlCaptureStackBackTrace` captures the stack without memory and
 *          without locks - it is the system's native path, and there is no reason to
 *          imitate it with something foreign.
 *
 *          <b>Warm-up is mandatory.</b> The first unwinding call builds unwind section
 *          pointers and on some systems allocates memory. Hence capture is warmed up
 *          ONCE at creation - outside any allocation - and thereafter does not request
 *          memory. This is verified not by reasoning but by observation: a probe counts
 *          calls to our own malloc around the capture.
 *
 *          <b>Recursion is broken by a thread flag, not by a prohibition.</b> Unwinding
 *          may request memory, and that request would come back into the capture. The
 *          "we are inside capture" flag is stored in a system key, NOT a thread-local
 *          slot: lazy creation of the latter itself calls memory allocation (verified by
 *          a stack overflow on macOS inside `_tlv_get_addr`).
 *
 *          <b>Names are extracted separately from capture.</b> Resolving an address to a
 *          name (`dladdr`, image scanning on MS Windows) is expensive and requests
 *          memory. Capture must be cheap: it is on the allocation path. Hence the
 *          captured data is stored as addresses, and names are extracted later - at
 *          reporting time.
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
		 * @brief Сведения об адресе стека вызовов
		 *
		 * @note Строки указывают на память образа и на память средств системы: годны
		 *       они до следующего обращения к разбору В ТОМ ЖЕ ПОТОКЕ, а копировать их
		 *       обязан тот, кому они нужны дольше. Оговорка о потоке не лишняя: у
		 *       MS Windows название образа хранится за нами, и буфер под него свой у
		 *       каждого потока - разбор в чужом потоке строку не трогает
		 *
		 * \~english
		 * @brief Information about a call stack address
		 *
		 * @note The strings point to image memory and system facility memory: they
		 *       are valid until the next resolution call IN THE SAME THREAD, and
		 *       whoever needs them longer must copy them. The thread caveat is not
		 *       superfluous: on MS Windows the image name is stored by us, and the
		 *       buffer for it is per-thread - resolution in a foreign thread does
		 *       not touch the string
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Symbol {
			// Название образа, которому принадлежит адрес
			const char * image;
			// Название функции, которой принадлежит адрес
			const char * name;
			// Адрес начала функции
			const void * begin;
			// Смещение разбираемого адреса от начала функции
			ptrdiff_t offset;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Symbol() noexcept;
		} symbol_t;
		/**
		 * \~russian
		 * @brief Класс съёма стека вызовов
		 *
		 * \~english
		 * @brief Call stack capture class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Trace {
			public:
				// Наибольшая снимаемая глубина стека вызовов
				static constexpr size_t DEPTH = 32;
			private:
				/**
				 * Признак прогретого съёма
				 *
				 * Вид НЕДЕЛИМЫЙ: пишет его настройка съёма, а читает сам съём - и читает
				 * на всяком потоке, идущем путём разбора мест выдачи. Обычное поле давало
				 * бы состязание по стандарту
				 */
				std::atomic <bool> _warmed;
				/**
				 * \~russian
				 * @brief Признак заведённого ключа - НЕДЕЛИМЫЙ
				 *
				 * @details Заведение съёма зовётся из настроек, а те приложение вправе менять из
				 *          любого потока. Два потока, вошедшие в заведение разом, оба видели
				 *          признак снятым и оба заводили ключ: второй затирал первый, тот утекал,
				 *          а поток, отметившийся внутри съёма ПЕРВЫМ ключом, читался вторым как
				 *          не отметившийся - и вложенный съём становился возможен, ровно тот, ради
				 *          запрета которого отметка и заведена.
				 *
				 * \~english
				 * @brief Sign of a started key - INDIVISIBLE
				 *
				 * @details A starting of a capture is called from the settings, and those an application has a right to change from
				 *          any thread. Two threads, entered the starting at once, both saw
				 *          a sign removed and both started a key: the second wiped the first, it leaked, and
				 *          a thread marked inside a capture BY THE FIRST key was read by the second as
				 *          unmarked - and a nested capture became possible, exactly the one for the sake
				 *          of a forbid of which a mark is started.
				 *
				 * \~
				 */
				std::atomic <bool> _keyed;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения съёма стека вызовов
				 *
				 * @note Звать вне выделения памяти: заведение прогревает раскрутку, а та
				 *       при первом обращении памяти просить вправе
				 *
				 * @return признак заведения съёма
				 *
				 * \~english
				 * @brief Method of initializing the call stack capture
				 *
				 * @note Call outside memory allocation: initialization warms up the
				 *       unwinding, and on first use it may request memory
				 *
				 * @return flag of capture initialization
				 *
				 * \~
				 */
				bool init() noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия съёма стека вызовов
				 *
				 * \~english
				 * @brief Method of shutting down the call stack capture
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод съёма стека вызовов
				 *
				 * @note Памяти метод не просит вовсе: снятое кладётся в переданный массив,
				 *       а раскрутка прогрета при заведении
				 *
				 * @param frames массив под адреса стека вызовов
				 * @param depth  длина массива в местах
				 * @param skip   число ближних уровней, какие пропустить
				 * @return       число снятых адресов
				 *
				 * \~english
				 * @brief Method of capturing the call stack
				 *
				 * @note The method does not request memory at all: captured data goes into
				 *       the passed array, and the unwinding is warmed up at initialization
				 *
				 * @param frames array for call stack addresses
				 * @param depth  array length in entries
				 * @param skip   number of nearest levels to skip
				 * @return       number of captured addresses
				 *
				 * \~
				 */
				size_t capture(const void ** frames, const size_t depth, const size_t skip) noexcept;
				/**
				 * \~russian
				 * @brief Метод обращения адреса стека в имя
				 *
				 * @note Метод дорог и памяти просит: звать его на пути выдачи нельзя, он
				 *       для доклада
				 *
				 * @param frame  разбираемый адрес
				 * @param symbol сведения о разобранном адресе
				 * @return       признак разбора адреса
				 *
				 * \~english
				 * @brief Method of resolving a stack address into a name
				 *
				 * @note The method is expensive and requests memory: it must not be called
				 *       on the allocation path, it is for reporting
				 *
				 * @param frame  address to resolve
				 * @param symbol information about the resolved address
				 * @return       flag of address resolution
				 *
				 * \~
				 */
				bool resolve(const void * frame, symbol_t & symbol) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения готовности съёма
				 *
				 * @return признак готовности съёма
				 *
				 * \~english
				 * @brief Method of determining whether the capture is ready
				 *
				 * @return flag of capture readiness
				 *
				 * \~
				 */
				bool ready() const noexcept;
				/**
				 * \~russian
				 * @brief Метод определения нахождения потока внутри съёма
				 *
				 * @note Пока поток снимает стек, всякая выдача памяти, какую просит сама
				 *       раскрутка (у OpenBSD она изредка просит), приходит в наш же
				 *       распределитель. Учитывать такую выдачу нельзя: она не выдача
				 *       вызвавшего, а издержка съёма, и стека под собой не имеет вовсе -
				 *       сторож возвратности обрывает вложенный съём. Учёт таких выдач
				 *       засорял бы таблицу мест выдачи записями без стека
				 *
				 * @return признак нахождения потока внутри съёма
				 *
				 * \~english
				 * @brief Method of determining whether the thread is inside a capture
				 *
				 * @note While a thread captures the stack, any memory allocation requested
				 *       by the unwinding itself (on OpenBSD it occasionally does) comes to
				 *       our allocator. Such allocation must not be counted: it is not the
				 *       caller's allocation but a capture overhead, and has no stack of its
				 *       own - the recursion guard breaks the nested capture. Counting such
				 *       allocations would pollute the allocation site table with stackless
				 *       entries
				 *
				 * @return flag of the thread being inside a capture
				 *
				 * \~
				 */
				bool capturing() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения названия способа съёма
				 *
				 * @return название способа съёма
				 *
				 * \~english
				 * @brief Method of getting the capture method name
				 *
				 * @return capture method name
				 *
				 * \~
				 */
				const char * name() const noexcept;
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
				explicit Trace() noexcept;
		} trace_t;
	};
};
