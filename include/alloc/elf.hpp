/**
 * @file elf.hpp
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
 * @brief Заголовочный файл захвата выделения памяти подменой имён ELF
 *
 * @section elf_decisions Намеренные решения
 *
 * @details <b>Подмена происходит при связывании, а захват её лишь удостоверяет.</b>
 *          Формат ELF устроен так, что имя `malloc`, определённое в самой программе,
 *          заслоняет собою одноимённое из библиотеки времени исполнения у ВСЕХ
 *          обращений процесса - переписывать при этом нечего. Оттого `acquire` здесь
 *          не правит ни байта: он сверяет, что подмена действительно состоялась, и
 *          добывает прежние функции. Отчитаться успехом, не сверив, значило бы
 *          обещать захват там, где его нет.
 *
 *          <b>Сверка идёт сличением адресов, а не наличием имени.</b> Имя `malloc`
 *          разрешается всегда - вопрос лишь в том, чьё оно. Оттого захват спрашивает
 *          у связывателя, какой адрес видит процесс, и сличает его с адресом нашей
 *          функции. Расхождение означает, что заслонить не вышло: наш файл кода
 *          выброшен связывателем как неупотребляемый, либо библиотека связана прежде
 *          нас.
 *
 *          <b>Прежние функции добываются `RTLD_NEXT`, а не по имени библиотеки.</b>
 *          Библиотека времени исполнения зовётся `libc.so.6` у glibc, `libc.so.7` у
 *          FreeBSD, `libc.so` у musl и Solaris - перечислять их значило бы гадать.
 *          `RTLD_NEXT` же спрашивает «следующее определение за нашим», и ответ верен
 *          на всех системах ELF.
 *
 *          <b>Своего у систем ELF лишь имя метода измерения блока.</b> `malloc_usable_size`
 *          у Linux и Solaris, `malloc_size` у FreeBSD и macOS, а у OpenBSD и NetBSD
 *          такого метода нет вовсе - и там размер блока берётся у самой кучи.
 *
 * \~english
 * @brief Header file of memory allocation capture by ELF symbol interposition
 *
 * @section elf_decisions Intentional decisions
 *
 * @details <b>Interposition happens at link time, and capture merely verifies it.</b>
 *          The ELF format is designed so that the name `malloc`, defined in the program
 *          itself, shadows the same-named one from the runtime library at ALL process
 *          references - nothing needs to be rewritten. Hence `acquire` here does not
 *          change a single byte: it verifies that interposition actually took place and
 *          extracts the previous functions. Reporting success without verifying would
 *          promise capture where there is none.
 *
 *          <b>Verification compares addresses, not name presence.</b> The name `malloc`
 *          always resolves - the question is whose it is. Hence capture asks the linker
 *          what address the process sees and compares it with the address of our
 *          function. A mismatch means shadowing failed: our code file was discarded by
 *          the linker as unused, or the library was linked before us.
 *
 *          <b>Previous functions are extracted by `RTLD_NEXT`, not by library name.</b>
 *          The runtime library is called `libc.so.6` on glibc, `libc.so.7` on FreeBSD,
 *          `libc.so` on musl and Solaris - enumerating them would be guessing.
 *          `RTLD_NEXT` asks for "the next definition after ours", and the answer is
 *          correct on all ELF systems.
 *
 *          <b>The only ELF-specific thing is the block size measurement method name.</b>
 *          `malloc_usable_size` on Linux and Solaris, `malloc_size` on FreeBSD and macOS,
 *          while OpenBSD and NetBSD have no such method at all - and there the block size
 *          is taken from the heap itself.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Если операционной системой не является MS Windows и не macOS
 */
#if !defined(_WIN32) && !defined(_WIN64) && !defined(__APPLE__)

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "capture.hpp"
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
		 * @brief Класс захвата выделения памяти подменой имён ELF
		 *
		 * \~english
		 * @brief ELF symbol interposition capture class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ ELFCapture : public Capture {
			private:
				// Прежние функции выделения памяти
				functions_t _originals;
				// Признак состоявшегося захвата
				bool _acquired;
			private:
				/**
				 * \~russian
				 * @brief Метод сверки того, что имя заслонено нами
				 *
				 * @param name название сверяемого имени
				 * @param ours адрес нашей функции
				 * @return     признак того, что процесс видит наш адрес
				 *
				 * \~english
				 * @brief Method of verifying that a name is shadowed by us
				 *
				 * @param name name to verify
				 * @param ours address of our function
				 * @return     flag that the process sees our address
				 *
				 * \~
				 */
				bool shadowed(const char * name, const void * ours) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата выделения памяти процесса
				 *
				 * @param hooks     наши функции, ставимые на место прежних
				 * @param originals прежние функции, отдаваемые захватом
				 * @return          признак состоявшегося захвата
				 *
				 * \~english
				 * @brief Method of capturing process memory allocation
				 *
				 * @param hooks     our functions replacing the previous ones
				 * @param originals previous functions returned by the capture
				 * @return          flag of the capture having taken place
				 *
				 * \~
				 */
				bool acquire(const functions_t & hooks, functions_t & originals) noexcept override;
				/**
				 * \~russian
				 * @brief Метод снятия захвата
				 *
				 * \~english
				 * @brief Method of releasing the capture
				 *
				 * \~
				 */
				void release() noexcept override;
				/**
				 * \~russian
				 * @brief Метод определения состоявшегося захвата
				 *
				 * @return признак захвата
				 *
				 * \~english
				 * @brief Method of determining whether capture has taken place
				 *
				 * @return capture flag
				 *
				 * \~
				 */
				bool acquired() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод опознания указателя, выданного прежним распределителем
				 *
				 * @param ptr разбираемый указатель
				 * @return    признак принадлежности прежнему распределителю
				 *
				 * \~english
				 * @brief Method of identifying a pointer issued by the previous allocator
				 *
				 * @param ptr pointer to examine
				 * @return    flag of belonging to the previous allocator
				 *
				 * \~
				 */
				bool foreign(const void * ptr) const noexcept override;
				/**
				 * \~russian
				 * @brief Метод определения владельца чужого блока
				 *
				 * @param ptr разбираемый указатель
				 * @return    nullptr: владельцев несколько не бывает
				 *
				 * \~english
				 * @brief Method of determining the owner of a foreign block
				 *
				 * @param ptr pointer being resolved
				 * @return    nullptr: there are never multiple owners
				 *
				 * \~
				 */
				const functions_t * owner(const void * ptr) const noexcept override;
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
				 * \~
				 */
				const char * name() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод получения прежних функций выделения памяти
				 *
				 * @return прежние функции
				 *
				 * \~english
				 * @brief Method of getting the previous allocation functions
				 *
				 * @return previous functions
				 *
				 * \~
				 */
				const functions_t & originals() const noexcept;
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
				explicit ELFCapture() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~ELFCapture() noexcept override;
		} elf_capture_t;
	};
};

#endif
