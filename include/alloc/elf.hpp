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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_ELF__
#define __AWH_ALLOC_ELF__

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
		 * @brief Класс захвата выделения памяти подменой имён ELF
		 *
		 * \~english
		 * @brief ELF symbol interposition capture class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ ELFCapture : public Capture {
			private:
				// Прежние функции выделения памяти
				functions_t _originals;
				// Признак состоявшегося захвата
				bool _acquired;
			private:
				/**
				 * @brief Метод сверки того, что имя заслонено нами
				 *
				 * @param name название сверяемого имени
				 * @param ours адрес нашей функции
				 * @return     признак того, что процесс видит наш адрес
				 *
				 */
				bool shadowed(const char * name, const void * ours) const noexcept;
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
				 * \~russian
				 * @brief Метод получения прежних функций выделения памяти
				 *
				 * @return прежние функции
				 *
				 * \~english
				 * @brief Method of getting the previous allocation functions
				 *
				 */
				const functions_t & originals() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				ELFCapture() noexcept : _originals(), _acquired(false) {}
				/**
				 * @brief Деструктор
				 *
				 */
				~ELFCapture() noexcept override;
		} elf_capture_t;
	};
};

#endif // !_WIN32 && !_WIN64 && !__APPLE__

#endif // __AWH_ALLOC_ELF__
