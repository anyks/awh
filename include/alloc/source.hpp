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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_SOURCE__
#define __AWH_ALLOC_SOURCE__

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
		 * @brief Класс источника страниц
		 *
		 * \~english
		 * @brief Page source class
		 *
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
				 * @param size      required size in bytes
				 * @param alignment required alignment in bytes
				 * @param actual    actually allocated size
				 * @return          address of the allocated region or nullptr
				 *
				 */
				virtual void * alloc(const size_t size, const size_t alignment, size_t & actual) noexcept = 0;
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
				 * @param addr address of the region being returned
				 * @param size size of the region being returned
				 * @return     flag of the operation having been performed
				 *
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
				 * @param addr    адрес области
				 * @param size    размер области
				 * @param opened  признак открытой области: ложь закрывает её вовсе
				 * @return        признак выполнения операции
				 *
				 * \~english
				 * @brief Method of changing the accessibility of a region
				 *
				 * @param addr   region address
				 * @param size   region size
				 * @param opened open region flag: false closes it entirely
				 * @return       flag of the operation having been performed
				 *
				 */
				virtual bool protect(void * addr, const size_t size, const bool opened) noexcept {
					// Область не наша, менять доступность нечему
					(void) addr; (void) size; (void) opened;
					// Отвечаем отказом: источник страницами системы не владеет
					return false;
				}

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
				 */
				virtual size_t granularity() const noexcept = 0;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Source() noexcept {}
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
		 */
		typedef class __AWH_SHARED_EXPORT__ SystemSource : public Source {
			private:
				// Размер страницы, узнаваемый у системы однажды
				size_t _granularity;
				/**
				 * Признак просьбы о крупных страницах
				 *
				 * Именно ПРОСЬБЫ: крупные страницы у всех наших систем требуют либо
				 * заранее отведённого запаса, либо особого права, и отказ в них - обычный
				 * исход, а не дефект. Не вышло - берём обычные, о чём договор и говорит
				 * словом «просить»
				 */
				// Признак просьбы о крупных страницах
				bool _superpages;
				// Число областей, доставшихся крупными страницами
				size_t _superpaged;
			private:
				/**
				 * @brief Метод определения размера страницы у системы
				 *
				 * @return размер страницы в байтах
				 *
				 */
				size_t detect() const noexcept;
			public:
				/**
				 * @brief Метод выдачи области страниц
				 *
				 * @param size      требуемый размер в байтах
				 * @param alignment требуемое выравнивание в байтах
				 * @param actual    действительно выданный размер
				 * @return          адрес выданной области либо nullptr
				 *
				 */
				void * alloc(const size_t size, const size_t alignment, size_t & actual) noexcept override;
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
				 * @param wanted huge pages request flag
				 *
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
				 * @return number of spans backed by huge pages
				 *
				 */
				size_t superpaged() const noexcept;
				/**
				 * @brief Метод отдачи содержимого страниц системе
				 *
				 * @param addr адрес отдаваемой области
				 * @param size размер отдаваемой области
				 * @return     признак выполнения операции
				 *
				 */
				bool purge(void * addr, const size_t size) noexcept override;
				/**
				 * @brief Метод отдачи области системе целиком
				 *
				 * @param addr адрес отдаваемой области
				 * @param size размер отдаваемой области
				 * @return     признак выполнения операции
				 *
				 */
				bool release(void * addr, const size_t size) noexcept override;
			public:
				/**
				 * @brief Метод смены доступности области
				 *
				 * @param addr   адрес области
				 * @param size   размер области
				 * @param opened признак открытой области: ложь закрывает её вовсе
				 * @return       признак выполнения операции
				 *
				 */
				bool protect(void * addr, const size_t size, const bool opened) noexcept override;

			public:
				/**
				 * @brief Метод получения размера страницы источника
				 *
				 * @return размер страницы в байтах
				 *
				 */
				size_t granularity() const noexcept override;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				SystemSource() noexcept : _granularity(0), _superpages(false), _superpaged(0) {}
		} system_source_t;
	};
};

#endif // __AWH_ALLOC_SOURCE__
