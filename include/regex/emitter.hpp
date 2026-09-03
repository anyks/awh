/**
 * @file emitter.hpp
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
 * @brief Заголовочный файл порождения машинного кода ARM64 — класс Emitter, собирающий
 *        последовательность команд процессора с отложенным разрешением переходов по меткам
 *
 * @section emitter_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Порождаемый код перемещаем: прямых адресов он не содержит.</b> Всё,
 *          что находится вне порождённого кода - таблицы принадлежности байтов,
 *          подпрограммы разбора, набор классов символов, - достигается смещением
 *          от единственного указателя, передаваемого при вызове в регистре
 *          обстановки. Решение принято до написания кодогенератора намеренно:
 *          задним числом оно не вставляется, а без него порождённый код нельзя
 *          ни сохранить на диск, ни загрузить обратно, тогда как надстройка Grok
 *          собирает тысячи выражений на запуске и сборка эта стоит секунд.
 *          Цена решения - одно лишнее обращение к памяти на вызов подпрограммы.
 *
 *          <b>Места кадра адресуются отдельным регистром, а не указателем
 *          стека.</b> Указателем стека они адресовались прежде, и порождению
 *          сопоставителя это было безразлично: смещения мест внутри кадра
 *          от того не менялись. Вызов вложенный и проход повторения, однако,
 *          требуют записи своей на каждый уровень, а число уровней при
 *          порождении не известно. Отдельный регистр позволяет сменять адрес
 *          записи, оставляя смещения мест прежними, - и потому ни одно
 *          из обращений порождения не меняется при заведении вложенности.
 *
 *          Набору x86-64 регистр этот отведён оберегаемый вызываемым: младших
 *          регистров там свободных нет вовсе - все девять заняты. Оттого вход
 *          сохраняет его наравне с прочими, а число сохранений становится
 *          чётным, и указатель стека к границе шестнадцати возвращает придача
 *          восьми байтов к кадру. Набору ARM64 отведён регистр младший:
 *          свободных там довольно, а сохранность его при вызовах подпрограмм
 *          обеспечивает сам сопоставитель, откладывая его в область сохранения.
 *
 *          <b>Переходы разрешаются после порождения, а не при нём.</b> Переход
 *          вперёд ссылается на код, ещё не порождённый, поэтому его смещение
 *          при порождении неизвестно. Команда размещается пустой, а её положение
 *          запоминается; разрешение проходит запомненные положения и вписывает
 *          смещения, когда все метки расставлены. Порождение в два прохода взамен
 *          этого потребовало бы порождать всё дважды.
 *
 *          <b>Выход смещения перехода за пределы поля команды - отказ, а не обрез.</b>
 *          Условный переход ARM64 несёт девятнадцать разрядов смещения, обычный -
 *          двадцать шесть, и выражение, порождающее код длиннее, кодогенерации
 *          не получает вовсе. Обрез смещения дал бы переход в середину чужой
 *          команды - отказ, неотличимый от порчи памяти.
 *
 * \~english
 * @brief Header file of the generation of ARM64 machine code — the Emitter class, which assembles
 *        a sequence of processor instructions with deferred resolution of the jumps by labels
 * @section emitter_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The generated code is relocatable: it holds no direct addresses.</b> Everything
 *          that lies outside the generated code — the byte belonging tables,
 *          the parsing subroutines, the set of character classes — is reached by an offset
 *          from a single pointer passed at the call in the context
 *          register. The decision was taken before writing the code generator deliberately:
 *          it cannot be inserted after the fact, and without it the generated code can neither
 *          be saved to disk nor loaded back, whereas the Grok superstructure
 *          builds thousands of expressions at startup and that building costs seconds.
 *          The price of the decision is one extra memory reference per subroutine call.
 *          <b>The jumps are resolved after the generation rather than during it.</b> A forward
 *          jump refers to code not yet generated, therefore its offset
 *          is unknown at generation time. The instruction is placed empty, and its position
 *          is remembered; the resolution walks the remembered positions and writes in the
 *          offsets when all the labels have been laid out. Generating in two passes instead of
 *          that would require generating everything twice.
 *          <b>A jump offset going beyond the bounds of the instruction field is a refusal rather than a truncation.</b>
 *          An ARM64 conditional jump carries nineteen bits of offset, an ordinary one
 *          twenty-six, and an expression generating longer code receives no code
 *          generation at all. Truncating the offset would give a jump into the middle of a foreign
 *          instruction — a fault indistinguishable from memory corruption.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_EMITTER__
#define __AWH_REGEX_EMITTER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы модулей
 */
#include "../sys/log.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../sys/macro/suppress.hpp"

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
		 * @brief Значение метки, перехода к какой не размещено
		 *
		 * \~english
		 * @brief Value of a label no jump to which has been placed
		 *
		 * \~
		 */
		constexpr size_t INVALID_LABEL = static_cast <size_t> (~0ull);

		/**
		 * \~russian
		 * @brief Класс порождения машинного кода ARM64
		 *
		 * @details Класс собирает последовательность команд процессора, откладывая
		 *          разрешение переходов до расстановки всех меток. Порождённый код
		 *          перемещаем: обращения за его пределы выполняются смещением
		 *          от регистра обстановки.
		 *
		 * \~english
		 * @brief Class of the generation of ARM64 machine code
		 * @details The class assembles a sequence of processor instructions, deferring
		 *          the resolution of the jumps until all the labels have been laid out. The generated code
		 *          is relocatable: references beyond its bounds are performed by an offset
		 *          from the context register.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Emitter {
			public:
				/**
				 * \~russian
				 * @brief Номера регистров соглашения о вызове порождённого кода
				 *
				 * @details Порождённый сопоставитель вызывается как функция вида
				 *          «bool (const char * text, size_t size, size_t start,
				 *          size_t * bounds, const void * context)», отчего первые
				 *          пять её доводов приходят в регистрах с нулевого по четвёртый.
				 *          Регистр обстановки удерживается на всём протяжении
				 *          сопоставления: через него достигается всё внешнее.
				 *
				 * \~english
				 * @brief Register numbers of the calling convention of the generated code
				 * @details The generated matcher is called as a function of the form
				 *          «bool (const char * text, size_t size, size_t start,
				 *          size_t * bounds, const void * context)», which is why its first
				 *          five arguments arrive in the registers from the zeroth to the fourth.
				 *          The context register is held for the whole duration of the
				 *          matching: everything external is reached through it.
				 *
				 * \~
				 */
				enum class reg_t : uint8_t {
					#if (defined(__x86_64__) || defined(_M_X64)) && defined(_WIN32)
						/**
						 * \~russian
						 * Номера регистров набора команд x86-64 под Windows
						 *
						 * @details Windows держится соглашения своего, а не
						 *          System V: доводы вызова приходят в регистрах
						 *          rcx, rdx, r8 и r9, довод пятый передаётся
						 *          кадром вызова, а оберегаемыми вызываемым
						 *          числятся rbx, rbp, rsi, rdi и r12-r15.
						 *          Отображение сложено так, чтобы четыре первых
						 *          довода легли в те же назначения, что и
						 *          в System V: подпрограммы обстановки принимают
						 *          ровно четыре довода, отчего порождение вызова
						 *          их остаётся общим обоим соглашениям.
						 *
						 *          Адрес таблицы обстановки, довод пятый, входом
						 *          вычитывается из кадра вызова в регистр rdi:
						 *          он оберегаем вызываемым, отчего переживает
						 *          вызовы подпрограмм без сохранения.
						 *
						 * \~english
						 * Register numbers of the x86-64 instruction set under Windows
						 * @details Windows keeps to a convention of its own rather than
						 *          System V: the call arguments arrive in the registers
						 *          rcx, rdx, r8 and r9, the fifth argument is passed
						 *          by the call frame, and the callee-saved ones
						 *          are rbx, rbp, rsi, rdi and r12-r15.
						 *          The mapping is laid out so that the first four
						 *          arguments fall into the same assignments as
						 *          in System V: the context subroutines take
						 *          exactly four arguments, which is why generating a call to
						 *          them stays common to both conventions.
						 *          The address of the context table, the fifth argument, is read out
						 *          of the call frame by the entry into the rdi register:
						 *          it is callee-saved, which is why it survives
						 *          subroutine calls without being saved.
						 *
						 * \~
						 */
						TEXT    = 0x01, // Адрес начала текста сопоставления, регистр rcx
						SIZE    = 0x02, // Размер текста сопоставления в байтах, регистр rdx
						START   = 0x08, // Позиция начала попытки сопоставления, регистр r8
						BOUNDS  = 0x09, // Адрес набора границ обнаруженного совпадения, регистр r9
						CONTEXT = 0x07, // Адрес таблицы адресов обстановки исполнения, регистр rdi
						RESULT  = 0x00, // Результат сопоставления, выдаваемый вызывающей стороне, регистр rax
						CURSOR  = 0x0A, // Позиция сопоставления в тексте, регистр r10
						BEGIN   = 0x0B, // Позиция начала обнаруженного совпадения, регистр r11
						RECORD  = 0x0E, // Адрес записи кадра исполняемого вызова, регистр r14
						/**
						 * \~russian
						 * Адрес вершины области записей, регистр r13
						 *
						 * @details Отведение записи ведётся вершиной, а обращение
						 *          к местам её - регистром записи: снятие уровня
						 *          вершины не двигает, покуда отступление в него
						 *          возможно. Эталон PCRE2 держит вершину области
						 *          в регистре тем же порядком.
						 *
						 * \~english
						 * Address of the summit of the area of records, register r13
						 *
						 * \~
						 */
						SUMMIT  = 0x0D, // Адрес вершины области записей
						LETTER  = 0x06, // Значение сопоставляемого байта текста, регистр rsi
						SCRATCH = 0x00, // Промежуточное значение вычисления, регистр rax
						SPARE   = 0x03, // Второе промежуточное значение вычисления, регистр rbx
						KEEPER  = 0x0C, // Значение, сохраняемое между шагами сопоставления, регистр r12
					#elif defined(__x86_64__) || defined(_M_X64)
						/**
						 * \~russian
						 * Номера регистров набора команд x86-64
						 *
						 * @details Доводы вызова приходят в регистрах rdi, rsi, rdx,
						 *          rcx и r8, а итог выдаётся в rax - таково соглашение
						 *          System V. Промежуточное значение отведено тому же
						 *          rax, что и итог: они не бывают нужны одновременно,
						 *          а регистров, вызовом не затираемых, всего девять,
						 *          и одиннадцати назначениям их не достаёт.
						 *
						 * \~english
						 * Register numbers of the x86-64 instruction set
						 * @details The call arguments arrive in the registers rdi, rsi, rdx,
						 *          rcx and r8, and the result is yielded in rax — such is the System V
						 *          convention. The intermediate value is allotted the same
						 *          rax as the result: they are never needed at the same time,
						 *          and there are only nine registers not clobbered by a call,
						 *          which is not enough for eleven assignments.
						 *
						 * \~
						 */
						TEXT    = 0x07, // Адрес начала текста сопоставления, регистр rdi
						SIZE    = 0x06, // Размер текста сопоставления в байтах, регистр rsi
						START   = 0x02, // Позиция начала попытки сопоставления, регистр rdx
						BOUNDS  = 0x01, // Адрес набора границ обнаруженного совпадения, регистр rcx
						CONTEXT = 0x08, // Адрес таблицы адресов обстановки исполнения, регистр r8
						RESULT  = 0x00, // Результат сопоставления, выдаваемый вызывающей стороне, регистр rax
						CURSOR  = 0x09, // Позиция сопоставления в тексте, регистр r9
						BEGIN   = 0x0A, // Позиция начала обнаруженного совпадения, регистр r10
						RECORD  = 0x0E, // Адрес записи кадра исполняемого вызова, регистр r14
						/**
						 * \~russian
						 * Адрес вершины области записей, регистр r13
						 *
						 * @details Отведение записи ведётся вершиной, а обращение
						 *          к местам её - регистром записи: снятие уровня
						 *          вершины не двигает, покуда отступление в него
						 *          возможно. Эталон PCRE2 держит вершину области
						 *          в регистре тем же порядком.
						 *
						 * \~english
						 * Address of the summit of the area of records, register r13
						 *
						 * \~
						 */
						SUMMIT  = 0x0D, // Адрес вершины области записей
						LETTER  = 0x0B, // Значение сопоставляемого байта текста, регистр r11
						SCRATCH = 0x00, // Промежуточное значение вычисления, регистр rax
						SPARE   = 0x03, // Второе промежуточное значение вычисления, регистр rbx
						KEEPER  = 0x0C, // Значение, сохраняемое между шагами сопоставления, регистр r12
					#else
						TEXT    = 0x00, // Адрес начала текста сопоставления
						SIZE    = 0x01, // Размер текста сопоставления в байтах
						START   = 0x02, // Позиция начала попытки сопоставления
						BOUNDS  = 0x03, // Адрес набора границ обнаруженного совпадения
						CONTEXT = 0x04, // Адрес таблицы адресов обстановки исполнения
						RESULT  = 0x00, // Результат сопоставления, выдаваемый вызывающей стороне
						CURSOR  = 0x05, // Позиция сопоставления в тексте
						BEGIN   = 0x06, // Позиция начала обнаруженного совпадения
						RECORD  = 0x0B, // Адрес записи кадра исполняемого вызова
						/**
						 * \~russian
						 * Адрес вершины области записей, регистр x12
						 *
						 * @details Отведение записи ведётся вершиной, а обращение
						 *          к местам её - регистром записи: снятие уровня
						 *          вершины не двигает, покуда отступление в него
						 *          возможно. Эталон PCRE2 держит вершину области
						 *          в регистре тем же порядком.
						 *
						 * \~english
						 * Address of the summit of the area of records, register x12
						 *
						 * \~
						 */
						SUMMIT  = 0x0C, // Адрес вершины области записей
						LETTER  = 0x07, // Значение сопоставляемого байта текста
						SCRATCH = 0x08, // Промежуточное значение вычисления
						SPARE   = 0x09, // Второе промежуточное значение вычисления
						KEEPER  = 0x0A, // Значение, сохраняемое между шагами сопоставления
					#endif
					/**
					 * \~russian
					 * Регистр адреса возврата из вызова подпрограммы
					 *
					 * @details Команда вызова подпрограммы записывает в этот регистр
					 *          адрес возврата, отчего вызов из порождённого кода обязан
					 *          сохранять его прежде вызова и восстанавливать после:
					 *          иначе завершение сопоставителя передавало бы исполнение
					 *          внутрь самого сопоставителя.
					 *
					 * \~english
					 * Register of the return address from a subroutine call
					 * @details The subroutine call instruction writes the return address into that register,
					 *          which is why a call from the generated code is obliged
					 *          to save it before the call and restore it afterwards:
					 *          otherwise the completion of the matcher would pass execution
					 *          inside the matcher itself.
					 *
					 * \~
					 */
					#if defined(__x86_64__) || defined(_M_X64)
						LINK    = 0x0D,
					#else
						LINK    = 0x1E,
					#endif
					/**
					 * \~russian
					 * Указатель стека вызова
					 *
					 * @details Набор команд ARM64 обозначает указатель стека тем же
					 *          номером, что и нулевой регистр, а различает их сама
					 *          команда: сложение и вычитание с числом, а равно чтение
					 *          и запись по смещению обращаются к указателю стека.
					 *
					 * \~english
					 * Call stack pointer
					 * @details The ARM64 instruction set denotes the stack pointer by the same
					 *          number as the zero register, and it is the instruction itself that tells them apart:
					 *          addition and subtraction with a number, as well as reading
					 *          and writing by an offset, refer to the stack pointer.
					 *
					 * \~
					 */
					#if defined(__x86_64__) || defined(_M_X64)
						STACK   = 0x04
					#else
						STACK   = 0x1F
					#endif
				};
				/**
				 * \~russian
				 * @brief Условие выполнения перехода
				 *
				 * \~english
				 * @brief Condition for taking a jump
				 *
				 * \~
				 */
				enum class cond_t : uint8_t {
					#if defined(__x86_64__) || defined(_M_X64)
						EQUAL    = 0x04, // Значения равны
						NOTEQUAL = 0x05, // Значения не равны
						ABOVE    = 0x03, // Значение не меньше без учёта знака
						BELOW    = 0x02, // Значение меньше без учёта знака
						GREATER  = 0x07, // Значение больше без учёта знака
						LESS     = 0x06  // Значение не больше без учёта знака
					#else
						EQUAL    = 0x00, // Значения равны
						NOTEQUAL = 0x01, // Значения не равны
						ABOVE    = 0x02, // Значение не меньше без учёта знака
						BELOW    = 0x03, // Значение меньше без учёта знака
						GREATER  = 0x08, // Значение больше без учёта знака
						LESS     = 0x09  // Значение не больше без учёта знака
					#endif
				};
			private:
				/**
				 * \~russian
				 * Порождаемая последовательность команд процессора
				 *
				 * @details Последовательность ведётся байтами, а не словами: набор
				 *          команд ARM64 несёт команды шириною постоянной, а x86-64 -
				 *          переменной, и байтовый вид пригоден обоим.
				 *
				 * \~english
				 * Generated sequence of processor instructions
				 * @details The sequence is kept in bytes rather than in words: the ARM64 instruction
				 *          set carries instructions of a constant width, and x86-64 of a
				 *          variable one, and the byte form is fit for both.
				 *
				 * \~
				 */
				vector <uint8_t> _code;
			private:
				/**
				 * \~russian
				 * @brief Запись значения регистра в память, только что размещённая
				 *
				 * \~english
				 * @brief A write of a register value to memory, just placed
				 *
				 * \~
				 */
				typedef struct Stored {
					// Регистр записанного значения
					reg_t source;
					// Регистр адреса начала области записи
					reg_t base;
					// Номер записанного значения в области
					uint32_t index;
					/**
					 * @brief Конструктор записи
					 *
					 * @param source регистр записанного значения
					 * @param base   регистр адреса начала области записи
					 * @param index  номер записанного значения в области
					 *
					 */
					Stored(const reg_t source, const reg_t base, const uint32_t index) noexcept :
					 source(source), base(base), index(index) {}
				} stored_t;
			private:
				/**
				 * \~russian
				 * Ряд записей в память, подряд размещённых
				 *
				 * @details Ряд ведётся ради снятия чтения избыточного: порождение
				 *          кладёт значение в место кадра и тут же читает его оттуда
				 *          обратно, тогда как оно ещё цело в регистре. Опыт назвал
				 *          цену такого чтения прямо - оно стоит пересылки из буфера
				 *          записи, а не обращения к памяти, и стоит её на всяком
				 *          проходе повторения.
				 *
				 *          Ряд ведётся лишь покуда идут записи подряд: запись
				 *          регистров не меняет, отчего значение, в ряду записанное,
				 *          заведомо цело. Всякая иная команда ряд обрывает -
				 *          обрыв опознаётся несовпадением длины кода с меткой ряда,
				 *          отчего править всякий метод размещения не требуется.
				 *
				 * \~english
				 * A row of writes to memory placed one after another
				 *
				 * \~
				 */
				vector <stored_t> _stored;
			private:
				/**
				 * \~russian
				 * Длина кода на миг завершения ряда записей
				 *
				 * @details Значение `SIZE_MAX` означает ряд оборванный: его кладут
				 *          расстановка метки и размещение цели перехода по адресу
				 *          в регистре - через них проходит управление со стороны,
				 *          и содержимое регистров ряду более не известно.
				 *
				 * \~english
				 * The length of the code at the moment the row of writes ended
				 *
				 * \~
				 */
				size_t _stamp;
			private:
				/**
				 * \~russian
				 * Количество мест кадра вызова, входом отведённых
				 *
				 * @details Место кадра адресуется номером, а размер кадра
				 *          вычисляется порождением по числу мест, каким оно
				 *          пользуется. Связи между ними не было никакой:
				 *          номер, размер кадра превышающий, писался бы за
				 *          кадр молча, портя стек вызывающего. Ограждение
				 *          обращает такую запись отказом порождения.
				 *          Нулевое количество мест ограждения не выполняет -
				 *          так стоит до размещения входа.
				 *
				 * \~english
				 * The number of the slots of the call frame allocated by the prologue
				 * @details A slot of the frame is addressed by number, and the size of
				 *          the frame is computed by the generation from the number of slots
				 *          it uses. There was no tie between them: a number exceeding
				 *          the size of the frame would be written past the frame silently,
				 *          corrupting the stack of the caller.
				 *
				 * \~
				 */
				size_t _seats;
			private:
				// Положения меток в порождаемой последовательности команд
				vector <size_t> _labels;
			private:
				/**
				 * \~russian
				 * @brief Отложенный переход, подлежащий разрешению
				 *
				 * \~english
				 * @brief Deferred jump subject to resolution
				 *
				 * \~
				 */
				typedef struct Fixup {
					/**
					 * \~russian
					 * @brief Вид команды, смещение к метке несущей
					 *
					 * \~english
					 * @brief Kind of the instruction carrying the offset to a label
					 *
					 * \~
					 */
					enum class kind_t : uint8_t {
						JUMP    = 0x00, // Безусловный переход
						BRANCH  = 0x01, // Переход по условию
						ADDRESS = 0x02  // Вычисление адреса метки
					};
					// Положение команды в последовательности
					size_t position;
					// Номер метки, к какой выполняется переход
					size_t label;
					// Вид команды, смещение к метке несущей
					kind_t kind;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Fixup() noexcept : position(0), label(0), kind(kind_t::JUMP) {}
				} fixup_t;
			private:
				// Набор отложенных переходов, подлежащих разрешению
				vector <fixup_t> _fixups;
			private:
				// Флаг отказа порождения машинного кода
				bool _failed;
			private:
				/**
				 * \~russian
				 * Объект журнала событий
				 *
				 * @details Журналом сообщается лишь ОДНА беда - переход к метке,
				 *          заведению не подвергшейся. Она означает несогласованность
				 *          самого порождения, а не свойство выражения, и молчать
				 *          о ней нельзя: наружу она выходит неотличимой от отказа
				 *          законного - от числа, в поле команды не помещающегося, -
				 *          и потребитель получает «кодогенерация неприменима» там,
				 *          где на деле изъян
				 *
				 * \~english
				 * The event log object
				 * @details Only ONE trouble is reported through the log - a jump to
				 *          a label that has not been declared. It means an inconsistency
				 *          of the generation itself rather than a property of the expression
				 *
				 * \~
				 */
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки поддержки порождения машинного кода сборкой
				 *
				 * @details Порождение выполнено для набора команд ARM64, поэтому
				 *          сборки для прочих наборов команд его не получают.
				 *          Отсутствие поддержки изъяном не является: сопоставление
				 *          выполняется исполнением программы, как и прежде.
				 *
				 * @return результат проверки поддержки порождения машинного кода
				 *
				 * \~english
				 * @brief Method of checking the support of machine code generation by the build
				 * @details The generation is implemented for the ARM64 instruction set, therefore
				 *          builds for the other instruction sets do not receive it.
				 *          The absence of the support is not a defect: the matching
				 *          is performed by executing the program, as before.
				 * @return result of checking the support of machine code generation
				 *
				 * \~
				 */
				static bool available() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки сохранности регистра при вызове подпрограммы
				 *
				 * @details Соглашение о вызове делит регистры надвое: одни вызываемый
				 *          обязан вернуть нетронутыми, другие волен затирать. Порождение
				 *          вызова сохраняет в кадре лишь затираемые: сохранение
				 *          оберегаемых было бы работою впустую, а всякий вызов
				 *          подпрограммы обстановки лежит в цикле попыток и платится
				 *          на каждую позицию начала.
				 *
				 *          Набору x86-64 оберегаемыми отведены три регистра из девяти
				 *          сохраняемых, набору же ARM64 - ни одного: там взяты младшие
				 *          намеренно, и сохранность их обеспечивает сам сопоставитель.
				 *
				 * @param reg проверяемый регистр соглашения о вызове
				 * @return    результат проверки сохранности регистра при вызове
				 *
				 * \~english
				 * @brief Method of checking the register preservation across a subroutine call
				 * @param reg checked register of the calling convention
				 * @return result of checking the register preservation across a call
				 *
				 * \~
				 */
				static bool preserved(const reg_t reg) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки порождаемой последовательности команд
				 *
				 * \~english
				 * @brief Method of clearing the generated instruction sequence
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения метки перехода
				 *
				 * @details Заведённая метка положения не имеет: положение она получает
				 *          расстановкой, а до неё лишь принимает переходы.
				 *
				 * @return номер заведённой метки перехода
				 *
				 * \~english
				 * @brief Method of introducing a jump label
				 * @details An introduced label has no position: it gets its position
				 *          by being laid out, and before that it only takes jumps.
				 * @return number of the introduced jump label
				 *
				 * \~
				 */
				size_t label() noexcept;
				/**
				 * \~russian
				 * @brief Метод расстановки метки перехода
				 *
				 * @param label номер расставляемой метки перехода
				 *
				 * \~english
				 * @brief Method of laying out a jump label
				 * @param label number of the jump label being laid out
				 *
				 * \~
				 */
				void place(const size_t label) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения входа в порождаемый сопоставитель
				 *
				 * @param frame размер отводимого кадра вызова в байтах
				 *
				 * @details Вход сохраняет регистры, соглашением о вызове вызываемой
				 *          стороне доверенные, и отводит кадр вызова. Состав
				 *          сохраняемого набором команд определяется: ARM64 отводит
				 *          порождённому коду регистры, сохранения не требующие, а
				 *          x86-64 их столько не имеет, и часть назначений
				 *          приходится на регистры сохраняемые.
				 *
				 * \~english
				 * @brief Method of placing the entry into the generated matcher
				 * @param frame size of the allotted call frame in bytes
				 * @details The entry saves the registers entrusted by the calling convention to the callee
				 *          side and allots the call frame. What is saved
				 *          is determined by the instruction set: ARM64 allots
				 *          to the generated code registers that require no saving, whereas
				 *          x86-64 does not have that many of them, and part of the assignments
				 *          falls on the saved registers.
				 *
				 * \~
				 */
				void prologue(const uint32_t frame) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения выхода из порождаемого сопоставителя
				 *
				 * @param frame размер освобождаемого кадра вызова в байтах
				 *
				 * @details Выход освобождает кадр вызова и восстанавливает регистры,
				 *          входом сохранённые. Размещается он перед всяким
				 *          завершением вызова, а не единожды.
				 *
				 * \~english
				 * @brief Method of placing the exit from the generated matcher
				 * @param frame size of the released call frame in bytes
				 * @details The exit releases the call frame and restores the registers
				 *          saved by the entry. It is placed before every
				 *          completion of the call rather than once.
				 *
				 * \~
				 */
				void epilogue(const uint32_t frame) noexcept;
				/**
				 * \~russian
				 * @brief Метод отведения записи вложенного уровня
				 *
				 * @param frame размер отводимой записи в байтах
				 *
				 * @details Места кадра адресуются регистром записи, отчего
				 *          продвижение его на размер кадра даёт обращениям,
				 *          порождением размещённым, свежий набор тех же самых
				 *          мест: ряд, положение своё хранящий, получает его
				 *          отдельным на каждый уровень вложенности, а сами
				 *          обращения не меняются ни единым.
				 *
				 *          Область записей отводится вызовом сопоставления
				 *          и стека машины не расходует: проход повторения над
				 *          областью требует записи своей на каждый проход,
				 *          и число их доходит до длины текста.
				 *
				 * \~english
				 * @brief Method of allotting the record of a nested level
				 * @param frame size of the allotted record in bytes
				 * @details The places of the frame are addressed by the record register, and so
				 *          advancing it by the size of the frame gives the accesses placed by
				 *          the generation a fresh set of the very same places.
				 *
				 * \~
				 */
				void enter(const uint32_t frame) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия записи вложенного уровня
				 *
				 * @param frame размер снимаемой записи в байтах
				 *
				 * @details Размер снимаемой записи обязан совпадать с размером,
				 *          отведённым при входе в уровень: регистр записи
				 *          отступает на него, возвращая обращения к местам
				 *          записи уровня вызывающего.
				 *
				 * \~english
				 * @brief Method of removing the record of a nested level
				 * @param frame size of the removed record in bytes
				 *
				 * \~
				 */
				void leave(const uint32_t frame) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения перехода к метке
				 *
				 * @param label номер метки, к какой выполняется переход
				 *
				 * \~english
				 * @brief Method of placing a jump to a label
				 * @param label number of the label the jump is taken to
				 *
				 * \~
				 */
				void jump(const size_t label) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения перехода к метке по условию
				 *
				 * @param cond  условие выполнения перехода
				 * @param label номер метки, к какой выполняется переход
				 *
				 * \~english
				 * @brief Method of placing a conditional jump to a label
				 * @param cond  condition for taking the jump
				 * @param label number of the label the jump is taken to
				 *
				 * \~
				 */
				void branch(const cond_t cond, const size_t label) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения сравнения значений регистров
				 *
				 * @param first  регистр уменьшаемого значения
				 * @param second регистр вычитаемого значения
				 *
				 * \~english
				 * @brief Method of placing a comparison of the register values
				 * @param first  register of the minuend value
				 * @param second register of the subtrahend value
				 *
				 * \~
				 */
				void compare(const reg_t first, const reg_t second) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения сравнения значения регистра с числом
				 *
				 * @details Число размещается в самой команде и потому ограничено
				 *          двенадцатью разрядами.
				 *
				 * @param reg   регистр сравниваемого значения
				 * @param value сравниваемое число
				 *
				 * \~english
				 * @brief Method of placing a comparison of a register value with a number
				 * @details The number is placed in the instruction itself and is therefore bounded
				 *          by twelve bits.
				 * @param reg   register of the compared value
				 * @param value compared number
				 *
				 * \~
				 */
				void compare(const reg_t reg, const uint32_t value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения сложения значения регистра с числом
				 *
				 * @param target регистр итога сложения
				 * @param source регистр слагаемого значения
				 * @param value  прибавляемое число
				 *
				 * \~english
				 * @brief Method of placing an addition of a register value with a number
				 * @param target register of the result of the addition
				 * @param source register of the addend value
				 * @param value  added number
				 *
				 * \~
				 */
				void add(const reg_t target, const reg_t source, const uint32_t value) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения вычитания числа из значения регистра
				 *
				 * @param target регистр итога вычитания
				 * @param source регистр уменьшаемого значения
				 * @param value  вычитаемое число
				 *
				 * \~english
				 * @brief Method of placing a subtraction of a number from a register value
				 * @param target register of the result of the subtraction
				 * @param source register of the minuend value
				 * @param value  subtracted number
				 *
				 * \~
				 */
				void sub(const reg_t target, const reg_t source, const uint32_t value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения переноса значения регистра
				 *
				 * @param target регистр назначения переноса
				 * @param source регистр источника переноса
				 *
				 * \~english
				 * @brief Method of placing a move of a register value
				 * @param target destination register of the move
				 * @param source source register of the move
				 *
				 * \~
				 */
				void move(const reg_t target, const reg_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения записи числа в регистр
				 *
				 * @param target регистр назначения записи
				 * @param value  записываемое число
				 *
				 * \~english
				 * @brief Method of placing a write of a number into a register
				 * @param target destination register of the write
				 * @param value  written number
				 *
				 * \~
				 */
				void move(const reg_t target, const uint64_t value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения чтения байта текста
				 *
				 * @details Байт читается по сумме адреса начала текста и позиции
				 *          сопоставления, что отвечает обращению к тексту по указателю
				 *          без проверки границ: положение проверено сравнением выше.
				 *
				 * @param target регистр прочитанного значения байта
				 * @param base   регистр адреса начала области чтения
				 * @param offset регистр смещения читаемого байта
				 *
				 * \~english
				 * @brief Method of placing a read of a byte of the text
				 * @details The byte is read at the sum of the address of the beginning of the text and the matching
				 *          position, which corresponds to referring to the text by a pointer
				 *          without bounds checking: the position was checked by the comparison above.
				 * @param target register of the read byte value
				 * @param base   register of the address of the beginning of the read area
				 * @param offset register of the offset of the read byte
				 *
				 * \~
				 */
				void load(const reg_t target, const reg_t base, const reg_t offset) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения чтения значения обстановки исполнения
				 *
				 * @details Обращение выполняется смещением от регистра обстановки,
				 *          чем и обеспечивается перемещаемость порождённого кода.
				 *
				 * @param target регистр прочитанного значения
				 * @param index  номер значения в таблице адресов обстановки
				 *
				 * \~english
				 * @brief Method of placing a read of a value of the execution context
				 * @details The reference is performed by an offset from the context register,
				 *          which is what ensures the relocatability of the generated code.
				 * @param target register of the read value
				 * @param index  number of the value in the address table of the context
				 *
				 * \~
				 */
				void context(const reg_t target, const uint32_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения чтения значения из памяти
				 *
				 * @param target регистр прочитанного значения
				 * @param base   регистр адреса начала области чтения
				 * @param index  номер читаемого значения в области
				 *
				 * \~english
				 * @brief Method of placing a read of a value from memory
				 * @param target register of the read value
				 * @param base   register of the address of the beginning of the read area
				 * @param index  number of the read value in the area
				 *
				 * \~
				 */
				void fetch(const reg_t target, const reg_t base, const uint32_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения записи значения регистра в память
				 *
				 * @param source регистр записываемого значения
				 * @param base   регистр адреса начала области записи
				 * @param index  номер записываемого значения в области
				 *
				 * \~english
				 * @brief Method of placing a write of a register value into memory
				 * @param source register of the written value
				 * @param base   register of the address of the beginning of the written area
				 * @param index  number of the written value in the area
				 *
				 * \~
				 */
				void store(const reg_t source, const reg_t base, const uint32_t index) noexcept;

				/**
				 * \~russian
				 * @brief Метод размещения чтения значения из памяти по адресу в регистре
				 *
				 * @details Смещение задаётся регистром, а не полем команды, отчего
				 *          читаемое место известно лишь при исполнении. Нужно это
				 *          набору, глубина какого зависит от данных.
				 *
				 * @note Порождением сопоставителя обращение это пока не звучит:
				 *       заведено оно заделом под рекурсию и оставлено намеренно.
				 *       Непроверенным кодом оно, однако, не остаётся - сличение
				 *       ведёт проверка «Regex.EmitterIndexed», порождающая образец
				 *       записи по одному смещению и чтения по другому и прогоняющая
				 *       его по всем их сочетаниям
				 *
				 * @param target регистр прочитанного значения
				 * @param base   регистр адреса начала области чтения
				 * @param offset регистр номера читаемого значения в области
				 *
				 * \~english
				 * @brief Method of placing a read of a value from memory by an address in a register
				 * @details The offset is given by a register rather than by a field of the instruction,
				 *          which is why the place being read is known only at execution time. This is needed by
				 *          a set whose depth depends on the data: by the return stack
				 *          of recursive calls.
				 * @param target register of the read value
				 * @param base   register of the address of the beginning of the read area
				 * @param offset register of the number of the read value in the area
				 *
				 * \~
				 */
				void fetch(const reg_t target, const reg_t base, const reg_t offset) noexcept;

				/**
				 * \~russian
				 * @brief Метод размещения записи значения регистра в память по адресу в регистре
				 *
				 * @param source регистр записываемого значения
				 * @param base   регистр адреса начала области записи
				 * @param offset регистр номера записываемого значения в области
				 *
				 * \~english
				 * @brief Method of placing a write of a register value into memory by an address in a register
				 * @param source register of the written value
				 * @param base   register of the address of the beginning of the write area
				 * @param offset register of the number of the written value in the area
				 *
				 * \~
				 */
				void store(const reg_t source, const reg_t base, const reg_t offset) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения вызова подпрограммы по адресу в регистре
				 *
				 * @details Вызов выполняется по адресу, прочитанному из обстановки
				 *          исполнения, чем и сохраняется перемещаемость порождённого
				 *          кода. Значения регистров, вызовом затираемые, сохраняет
				 *          сторона вызывающая: соглашение о вызове подпрограмм
				 *          сохранности младших регистров не обещает.
				 *
				 * @param reg регистр адреса вызываемой подпрограммы
				 *
				 * \~english
				 * @brief Method of placing a subroutine call by an address in a register
				 * @details The call is performed by the address read from the execution
				 *          context, which is what keeps the relocatability of the generated
				 *          code. The register values clobbered by the call are saved by
				 *          the calling side: the subroutine calling convention
				 *          promises no preservation of the lower registers.
				 * @param reg register of the address of the called subroutine
				 *
				 * \~
				 */
				void call(const reg_t reg) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения вычисления адреса метки
				 *
				 * @details Адрес вычисляется прибавлением смещения к положению самой
				 *          команды, отчего перемещаемость порождённого кода сохраняется:
				 *          запомненный адрес перемещается вместе с кодом.
				 *
				 * @param target регистр вычисленного адреса
				 * @param label  номер метки, адрес какой вычисляется
				 *
				 * \~english
				 * @brief Method of placing a computation of the address of a label
				 * @details The address is computed by adding the offset to the position of the
				 *          instruction itself, which is why the relocatability of the generated code is kept:
				 *          the remembered address moves together with the code.
				 * @param target register of the computed address
				 * @param label  number of the label whose address is computed
				 *
				 * \~
				 */
				void address(const reg_t target, const size_t label) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения перехода по адресу в регистре
				 *
				 * @param reg регистр адреса выполняемого перехода
				 *
				 * \~english
				 * @brief Method of placing a jump by an address in a register
				 * @param reg register of the address of the taken jump
				 *
				 * \~
				 */
				void proceed(const reg_t reg) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения метки цели перехода по адресу в регистре
				 *
				 * @details Размещается сразу за меткой, куда приходит переход методом
				 *          «proceed». Наборы команд, требования такого не ставящие,
				 *          не размещают ничего.
				 *
				 * \~english
				 * @brief Method of placing the target marker of a jump by an address in a register
				 * @details It is placed right after the label the jump arrives at by the
				 *          «proceed» method. The instruction sets that make no such requirement
				 *          place nothing.
				 *
				 * \~
				 */
				void landing() noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения завершения вызова
				 *
				 * \~english
				 * @brief Method of placing the completion of the call
				 *
				 * \~
				 */
				void ret() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разрешения отложенных переходов
				 *
				 * @details Метод вписывает смещения переходов в размещённые команды
				 *          и обязан вызываться после расстановки всех меток.
				 *
				 * @return результат разрешения отложенных переходов
				 *
				 * \~english
				 * @brief Method of resolving the deferred jumps
				 * @details The method writes the jump offsets into the placed instructions
				 *          and is obliged to be called after all the labels have been laid out.
				 * @return result of resolving the deferred jumps
				 *
				 * \~
				 */
				bool resolve() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения порождённой последовательности команд
				 *
				 * @return порождённая последовательность команд процессора
				 *
				 * \~english
				 * @brief Method of getting the generated instruction sequence
				 * @return generated sequence of processor instructions
				 *
				 * \~
				 */
				const vector <uint8_t> & code() const noexcept;
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
				 * @brief Метод проверки отказа порождения машинного кода
				 *
				 * @details Отказ вызывается доводом, не помещающимся в поле команды,
				 *          обращением к незаведённой метке либо смещением перехода,
				 *          выходящим за пределы поля. Порождение после отказа
				 *          продолжается, но код его к исполнению непригоден.
				 *
				 * @return результат проверки отказа порождения машинного кода
				 *
				 * \~english
				 * @brief Method of checking a failure of the machine code generation
				 * @details A failure is caused by an argument not fitting into the instruction field,
				 *          by a reference to a label that was not introduced or by a jump offset
				 *          going beyond the bounds of the field. The generation continues after a failure,
				 *          but its code is unfit for execution.
				 * @return result of checking a failure of the machine code generation
				 *
				 * \~
				 */
				bool failed() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Emitter(const log_t * log) noexcept;
		} emitter_t;
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../sys/macro/restore.hpp"

#endif // __AWH_REGEX_EMITTER__
