/**
 * @file source.hpp
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
 * @brief Заголовочный файл источника страниц — договор между страничной кучей и тем,
 *        откуда берётся память: системой, заранее отведённой областью или крупными
 *        страницами
 *
 * @section source_decisions Намеренные решения
 *
 * @details <b>Источник подменяем потребителем.</b> Страничная куча не знает, откуда
 *          берутся страницы, и это позволяет положить фреймворк на область, отведённую
 *          приложением самостоятельно, - вплоть до устройства без операционной системы.
 *          Устройство взято у gperftools (SysAllocator): там оно выбрано верно.
 *
 *          <b>Возврат разделён на два действия.</b> Метод «purge» отдаёт содержимое
 *          страниц, оставляя за собою адреса, а «release» отдаёт и адреса. Разделение
 *          это не украшение: удержание адресов при отданном содержимом - основной
 *          способ отдать память системе, не теряя занятой области, и на нём стоит
 *          режим удержания арены.
 *
 *          <b>Отказ выдачи - обычное дело, а не исключение.</b> Источник отвечает
 *          пустым указателем и не бросает: он зовётся с взятыми замками распределителя,
 *          и раскрутка стека оттуда оставила бы кучу в разобранном виде.
 *
 * \~english
 * @brief Header file of the page source — the contract between the page heap and where
 *        memory comes from: the system, a preallocated region or huge pages
 *
 * @section source_decisions Deliberate decisions
 *
 * @details <b>The source is substitutable by the consumer.</b> The page heap does not
 *          know where pages come from, and this allows placing the framework on a
 *          region allocated by the application itself — up to a device without an
 *          operating system. The design is taken from gperftools (SysAllocator): it
 *          was chosen correctly there.
 *
 *          <b>Return is split into two actions.</b> The purge method returns page
 *          content while keeping the addresses, and release returns the addresses as
 *          well. This split is not decoration: holding addresses with content returned
 *          is the main way to give memory back to the system without losing the
 *          occupied region, and the arena retention mode is built on it.
 *
 *          <b>Allocation refusal is routine, not an exception.</b> The source answers
 *          with a null pointer and does not throw: it is called with the allocator
 *          locks held, and stack unwinding from there would leave the heap in a
 *          disassembled state.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Namespace of the memory allocator
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс источника страниц
		 *
		 * \~english
		 * @brief Page source class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Source {
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи области страниц
				 *
				 * @note Отказ выдачи отвечает пустым указателем, а не исключением: метод
				 *       зовётся с взятыми замками кучи
				 *
				 * @param size      требуемый размер в байтах
				 * @param alignment требуемое выравнивание в байтах
				 * @param actual    действительно выданный размер
				 * @return          адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a region of pages
				 *
				 * @note A refusal of an issue answers by an empty pointer, and not by an exception: a
				 *       method is called with the taken locks of the heaps
				 *
				 * @param size      required size in bytes
				 * @param alignment required alignment in bytes
				 * @param actual    actually allocated size
				 * @return          address of the allocated region or nullptr
				 *
				 * \~
				 */
				virtual void * alloc(const size_t size, const size_t alignment, size_t & actual) noexcept = 0;
				/**
				 * \~russian
				 * @brief Метод запрета области уходить в подкачку
				 *
				 * @note В отличие от укрытия, запрет этот навешивается на УЖЕ отведённую
				 *       область: он и есть отдельный вызов системы. Снимать его при
				 *       отдаче области незачем - система снимает его сама
				 *
				 * @note Право на запрет ограничено: у части систем оно требует поднятого
				 *       предела `RLIMIT_MEMLOCK`, и отказ здесь - обычный исход
				 *
				 * @param addr   адрес области
				 * @param size   размер области в байтах
				 * @param wanted признак необходимости запрета
				 * @return       признак выполнения операции
				 *
				 * \~english
				 * @brief Method of prohibiting a region from going to swap
				 *
				 * @note Unlike concealment, this prohibition is put on an ALREADY allocated
				 *       region: it is a separate system call. There is no need to lift it when
				 *       the region is returned - the system lifts it itself
				 *
				 * @note The right to prohibit is limited: on some systems it requires a raised
				 *       `RLIMIT_MEMLOCK` limit, and refusal here is a normal outcome
				 *
				 * @param addr   region address
				 * @param size   region size in bytes
				 * @param wanted sign of the prohibition being required
				 * @return       sign of the operation having been performed
				 *
				 * \~
				 */
				virtual bool wire(void * addr, const size_t size, const bool wanted) noexcept;
				/**
				 * \~russian
				 * @brief Метод отведения области, укрытой от снимков памяти
				 *
				 * @note Укрытие задаётся ПРИ ОТВЕДЕНИИ и задним числом не навешивается:
				 *       оттого это отдельный путь, а не признак у обычной выдачи
				 *
				 * @note Умеет его не всякая система. Отклик по умолчанию отводит область
				 *       обычным путём и отвечает признаком «не укрыто»: молчаливое
				 *       понижение обещания хуже честного отказа, и решает потребитель
				 *
				 * @param size   требуемый размер в байтах
				 * @param actual действительно выданный размер
				 * @param hidden признак состоявшегося укрытия
				 * @return       адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a region concealed from memory dumps
				 *
				 * @note Concealment is set AT ALLOCATION and cannot be applied after the
				 *       fact: hence this is a separate path, not a flag on ordinary
				 *       allocation
				 *
				 * @note Not every system can do it. The default response allocates the
				 *       region normally and answers with the not concealed flag: a silent
				 *       downgrade of the promise is worse than an honest refusal, and the
				 *       consumer decides
				 *
				 * @param size   required size in bytes
				 * @param actual actually allocated size
				 * @param hidden flag of the concealment having taken place
				 * @return       address of the allocated region or nullptr
				 *
				 * \~
				 */
				virtual void * conceal(const size_t size, size_t & actual, bool & hidden) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи содержимого страниц системе
				 *
				 * @note Адреса при этом остаются за нами: обращение по ним годно и
				 *       позднее. Это и есть способ отдать память, не теряя занятой области
				 *
				 * @note Содержимое после отдачи **не определено** и чтению не подлежит.
				 *       Нулевым его делает лишь MADV_DONTNEED у Linux; MADV_FREE у BSD и
				 *       macOS отдаёт страницы лениво и до нехватки памяти содержимое
				 *       сохраняет, а MEM_RESET у MS Windows не обещает ничего вовсе
				 *
				 * @param addr адрес отдаваемой области
				 * @param size размер отдаваемой области
				 * @return     признак выполнения операции
				 *
				 * \~english
				 * @brief Method of returning page contents to the system
				 *
				 * @note The addresses remain ours: access to them is valid later as well.
				 *       This is how memory is returned without losing the occupied region
				 *
				 * @note The content after returning is **undefined** and must not be read.
				 *       Only MADV_DONTNEED on Linux zeroes it; MADV_FREE on BSD and macOS
				 *       returns pages lazily and preserves content until memory pressure,
				 *       while MEM_RESET on MS Windows promises nothing at all
				 *
				 * @param addr address of the region being returned
				 * @param size size of the region being returned
				 * @return     flag of the operation having been performed
				 *
				 * \~
				 */
				virtual bool purge(void * addr, const size_t size) noexcept = 0;
				/**
				 * \~russian
				 * @brief Метод отдачи области системе целиком
				 *
				 * @param addr адрес отдаваемой области
				 * @param size размер отдаваемой области
				 * @return     признак выполнения операции
				 *
				 * \~english
				 * @brief Method of returning the region to the system entirely
				 *
				 * @param addr address of the region being returned
				 * @param size size of the region being returned
				 * @return     flag of the operation having been performed
				 *
				 * \~
				 */
				virtual bool release(void * addr, const size_t size) noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Метод смены доступности области
				 *
				 * @note Умение это необязательное: источник, страницами системы не
				 *       владеющий, отвечает отказом, и заслоны с ним выключаются. Отказ
				 *       здесь означает «не умею», а не «не вышло»
				 *
				 * @param addr   адрес области
				 * @param size   размер области
				 * @param opened признак открытой области: ложь закрывает её вовсе
				 * @return       признак выполнения операции
				 *
				 * \~english
				 * @brief Method of changing the accessibility of a region
				 *
				 * @note This ability is optional: a source not owning system pages answers
				 *       with refusal, and guards are disabled with it. Refusal here means
				 *       cannot do, not did not work out
				 *
				 * @param addr   region address
				 * @param size   region size
				 * @param opened open region flag: false closes it entirely
				 * @return       flag of the operation having been performed
				 *
				 * \~
				 */
				virtual bool protect(void * addr, const size_t size, const bool opened) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера страницы источника
				 *
				 * @return размер страницы в байтах
				 *
				 * \~english
				 * @brief Method of getting the source page size
				 *
				 * @return page size in bytes
				 *
				 * \~
				 */
				virtual size_t granularity() const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				virtual ~Source() noexcept;
		} source_t;
		/**
		 * \~russian
		 * @brief Класс системного источника страниц
		 *
		 * @note Обращение к системе идёт обычными вызовами libc, а не через syscall(2):
		 *       OpenBSD этот вызов убрала целиком, и именно на нём gperftools там и
		 *       спотыкается
		 *
		 * \~english
		 * @brief System page source class
		 *
		 * @note Access to the system goes through ordinary libc calls, not syscall(2):
		 *       OpenBSD removed that call entirely, and that is exactly where
		 *       gperftools stumbles there
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ SystemSource : public Source {
			private:
				/**
				 * Признак просьбы о крупных страницах
				 *
				 * Именно ПРОСЬБЫ: крупные страницы у всех наших систем требуют либо
				 * заранее отведённого запаса, либо особого права, и отказ в них - обычный
				 * исход, а не дефект. Не вышло - берём обычные, о чём договор и говорит
				 * словом «просить»
				 *
				 * Вид НЕДЕЛИМЫЙ: просьбу ставит поток приложения, а читает её путь выдачи
				 * всякого потока, и обычное поле давало бы состязание по стандарту - то же
				 * самое, ради чего неделимы и соседние поля источника
				 */
				atomic_bool _superpages;
				// Взято у системы в байтах
				atomic_size_t _taken;
				/**
				 * Потолок взятого у системы в байтах: нуль - без потолка
				 *
				 * Стоит он ЗДЕСЬ, а не у страничной кучи, потому что источник один на
				 * все слои: куча, крупные выдачи и заслоны берут память через него.
				 * Потолок, поставленный у одной лишь кучи, сторожил бы её часть, а
				 * приложение задаёт его по числу, какое видит у `property_t::HEAP`, -
				 * то есть по ВСЕМУ взятому. Проверено замером: при потолке в два
				 * мегабайта сверх занятого куча брала ещё двенадцать
				 */
				atomic_size_t _ceiling;
				/**
				 * Число областей, доставшихся крупными страницами
				 *
				 * Вид неделимый по той же причине: счётчик правится на пути выдачи, а
				 * путь этот у трёх слоёв идёт под разными замками
				 */
				atomic_size_t _superpaged;
				/**
				 * Размер страницы, узнаваемый у системы однажды
				 *
				 * Вид неделимый, хотя пишется он одним и тем же числом: источник зовут
				 * ТРИ слоя - страничная куча, крупные выдачи и заслоны, - и замки у них
				 * РАЗНЫЕ. Двое могут узнавать размер страницы разом, а обычное поле
				 * давало бы при этом состязание в глазах средств проверки
				 */
				atomic_size_t _granularity;
			private:
				/**
				 * \~russian
				 * @brief Метод определения размера страницы у системы
				 *
				 * @return размер страницы в байтах
				 *
				 * \~english
				 * @brief Method of detecting the system page size
				 *
				 * @return page size in bytes
				 *
				 * \~
				 */
				size_t detect() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи области страниц
				 *
				 * @param size      требуемый размер в байтах
				 * @param alignment требуемое выравнивание в байтах
				 * @param actual    действительно выданный размер
				 * @return          адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a page region
				 *
				 * @param size      required size in bytes
				 * @param alignment required alignment in bytes
				 * @param actual    actually allocated size
				 * @return          address of the allocated region or nullptr
				 *
				 * \~
				 */
				void * alloc(const size_t size, const size_t alignment, size_t & actual) noexcept override;
				/**
				 * \~russian
				 * @brief Метод отведения области, укрытой от снимков памяти
				 *
				 * @param size   требуемый размер в байтах
				 * @param actual действительно выданный размер
				 * @param hidden признак состоявшегося укрытия
				 * @return       адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a region concealed from memory dumps
				 *
				 * @param size   required size in bytes
				 * @param actual actually allocated size
				 * @param hidden flag of the concealment having taken place
				 * @return       address of the allocated region or nullptr
				 *
				 * \~
				 */
				void * conceal(const size_t size, size_t & actual, bool & hidden) noexcept override;
				/**
				 * \~russian
				 * @brief Метод запрета области уходить в подкачку
				 *
				 * @param addr   адрес области
				 * @param size   размер области в байтах
				 * @param wanted признак необходимости запрета
				 * @return       признак выполнения операции
				 *
				 * \~english
				 * @brief Method of preventing a region from being swapped out
				 *
				 * @param addr   region address
				 * @param size   region size in bytes
				 * @param wanted flag of the lock being required
				 * @return       flag of the operation having been performed
				 *
				 * \~
				 */
				bool wire(void * addr, const size_t size, const bool wanted) noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод включения просьбы о крупных страницах
				 *
				 * @note Действует на выдачи, идущие ПОСЛЕ него: уже отведённые области
				 *       крупными страницами задним числом не станут
				 *
				 * @param wanted признак просьбы о крупных страницах
				 *
				 * \~english
				 * @brief Method of enabling the request for huge pages
				 *
				 * @note Affects allocations that come AFTER it: already taken regions will
				 *       not become backed by huge pages retroactively
				 *
				 * @param wanted flag of the request for huge pages
				 *
				 * \~
				 */
				void superpages(const bool wanted) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа областей, доставшихся крупными страницами
				 *
				 * @note Нуль при включённой просьбе означает лишь отказ системы: у всех
				 *       наших систем крупные страницы требуют запаса либо права
				 *
				 * @return число областей, доставшихся крупными страницами
				 *
				 * \~english
				 * @brief Method of getting the number of spans backed by huge pages
				 *
				 * @note Zero with the request enabled means only a system refusal: on all our
				 *       systems huge pages require reserve or privilege
				 *
				 * @return number of spans backed by huge pages
				 *
				 * \~
				 */
				size_t superpaged() const noexcept;
				/**
				 * \~russian
				 * @brief Метод задания потолка взятого у системы
				 *
				 * @note Потолок сторожит ВСЁ взятое: и страничную кучу, и крупные выдачи,
				 *       и страницы заслонов. Упершись в него, источник отвечает отказом,
				 *       а слои отказ этот и без того обрабатывают - память у системы
				 *       кончается и без всякого потолка
				 *
				 * @param limit потолок в байтах: нуль - без потолка
				 *
				 * \~english
				 * @brief Method of setting the ceiling of memory taken from the system
				 *
				 * @note The ceiling applies to the page heap, huge allocations and guards.
				 *       Hitting it, the source answers with refusal, and the layers handle
				 *       that refusal anyway - memory from the system runs out with or
				 *       without any ceiling
				 *
				 * @param limit ceiling in bytes: zero means no ceiling
				 *
				 * \~
				 */
				void ceiling(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод опроса взятого у системы
				 *
				 * @return взятое у системы в байтах
				 *
				 * \~english
				 * @brief Method of querying the memory taken from the system
				 *
				 * @return memory taken from the system in bytes
				 *
				 * \~
				 */
				size_t taken() const noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи содержимого страниц системе
				 *
				 * @param addr адрес отдаваемой области
				 * @param size размер отдаваемой области
				 * @return     признак выполнения операции
				 *
				 * \~english
				 * @brief Method of returning page content to the system
				 *
				 * @param addr address of the region being returned
				 * @param size size of the region being returned
				 * @return     sign of successful operation
				 *
				 * \~
				 */
				bool purge(void * addr, const size_t size) noexcept override;
				/**
				 * \~russian
				 * @brief Метод отдачи области системе целиком
				 *
				 * @param addr адрес отдаваемой области
				 * @param size размер отдаваемой области
				 * @return     признак выполнения операции
				 *
				 * \~english
				 * @brief Method of returning a region to the system entirely
				 *
				 * @param addr address of the region being returned
				 * @param size size of the region being returned
				 * @return     sign of successful operation
				 *
				 * \~
				 */
				bool release(void * addr, const size_t size) noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод смены доступности области
				 *
				 * @param addr   адрес области
				 * @param size   размер области
				 * @param opened признак открытой области: ложь закрывает её вовсе
				 * @return       признак выполнения операции
				 *
				 * \~english
				 * @brief Method of changing region accessibility
				 *
				 * @param addr   region address
				 * @param size   region size
				 * @param opened open region flag: false closes it entirely
				 * @return       sign of successful operation
				 *
				 * \~
				 */
				bool protect(void * addr, const size_t size, const bool opened) noexcept override;

			public:
				/**
				 * \~russian
				 * @brief Метод получения размера страницы источника
				 *
				 * @return размер страницы в байтах
				 *
				 * \~english
				 * @brief Method of getting the source page size
				 *
				 * @return page size in bytes
				 *
				 * \~
				 */
				size_t granularity() const noexcept override;
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
				explicit SystemSource() noexcept;
		} system_source_t;
	};
};
