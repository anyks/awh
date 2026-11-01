/**
 * @file link.hpp
 * @date 2026-08-22
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
 * @brief Заголовочный файл связи свободных блоков распределителя памяти
 *
 * @section link_decisions Намеренные решения
 *
 * @details <b>Указатель хранится перемешанным.</b> Свободный блок хранит указатель на
 *          следующий свободный блок в собственном теле, и тело это выдано наружу
 *          прежде освобождения. Переполнение буфера у соседа переписывает такой
 *          указатель и заставляет распределитель выдать любой заданный взломщиком
 *          адрес. Перемешивание рушит приём целиком: чтобы подделать связь, нужно
 *          знать зерно процесса, а зерно наружу не выдаётся вовсе.
 *
 *          <b>Место блока входит в перемешивание.</b> Одного зерна мало: значение,
 *          подсмотренное у одного блока, годилось бы подделкой всякому другому.
 *          Оттого к зерну примешивается собственный адрес блока, и всякая связь
 *          перемешана по-своему. Приём взят у glibc и tcmalloc.
 *
 *          <b>Адрес примешивается умножением, а не сдвигом.</b> У glibc маской
 *          служит адрес, сдвинутый на страницу, - и тогда все блоки одной страницы
 *          перемешаны ОДНОЙ маской, а соседи подделываются друг другом свободно.
 *          Проверено щупом: при сдвиге на двенадцать два соседних блока хранили одно
 *          и то же слово, при сдвиге на четыре - слова, различных в двух разрядах,
 *          то есть подделываемые угадыванием смещения. Умножение на зерно разносит
 *          соседей по всей ширине слова, и разница между их масками взломщику без
 *          зерна неизвестна. Стоит это одного умножения на связь.
 *
 *          <b>Пустой указатель остаётся пустым.</b> Перемешивание парное, а зерно с
 *          адресом входят в него дважды - при записи и при чтении, - оттого конец
 *          списка читается пустым указателем сам собой, без особой ветки.
 *
 *          <b>Зерно берётся один раз и не пересевается.</b> Пересев обессмыслил бы
 *          уже записанные связи: они читаются старым зерном. По той же причине
 *          зерно переживает ветвление процесса нетронутым - потомок наследует те же
 *          списки.
 *
 *          <b>Подстановка обязательна.</b> Связь читается и пишется на пути всякого
 *          выделения, и вызов сюда стоил бы дороже самого перемешивания. Оттого тело
 *          вынесено посредником с принудительной подстановкой.
 *
 * \~english
 * @brief Header file of the memory allocator free list linkage
 *
 * @section link_decisions Intentional decisions
 *
 * @details <b>The pointer is stored mangled.</b> A free block stores a pointer to the
 *          next free block in its own body, and this body was handed out before being
 *          freed. A buffer overflow by a neighbor rewrites such a pointer and forces
 *          the allocator to hand out any address chosen by the attacker. Mangling
 *          breaks the attack completely: to forge a link one must know the process
 *          cookie, and the cookie is never revealed.
 *
 *          <b>The block's address is mixed into the mangling.</b> A single cookie is
 *          not enough: a value peeked from one block could be used to forge any other.
 *          Hence the block's own address is mixed into the cookie, and every link is
 *          mangled differently. The technique is taken from glibc and tcmalloc.
 *
 *          <b>The address is mixed by multiplication, not shifting.</b> In glibc the
 *          mask is the address shifted by a page, - and then all blocks of one page
 *          are mangled with ONE mask, and neighbors forge each other freely. Verified
 *          by a probe: shifting by twelve made two adjacent blocks store the same
 *          word, shifting by four made words differing in two bits, i.e. forgeable by
 *          guessing the offset. Multiplication by the cookie spreads neighbors across
 *          the full width of the word, and the difference between their masks is
 *          unknown to an attacker without the cookie. It costs one multiplication per
 *          link.
 *
 *          <b>A null pointer stays null.</b> Mangling is pairwise, and the cookie with
 *          the address enter it twice - on write and on read, - hence the end of the
 *          list reads as a null pointer by itself, without a special branch.
 *
 *          <b>The cookie is taken once and never re-seeded.</b> Re-seeding would
 *          invalidate already written links: they are read with the old cookie. For
 *          the same reason the cookie survives process forking untouched - the child
 *          inherits the same lists.
 *
 *          <b>Inlining is mandatory.</b> The link is read and written on every
 *          allocation path, and a call here would cost more than the mangling itself.
 *          Hence the body is placed inline with forced inlining.
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
#include <cstring>
#include <cstdint>

/**
 * Наши модули
 */
#include "../sys/macro/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_LINK_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_LINK_INLINE inline __attribute__((always_inline))
#endif

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
		 * @brief Класс связи свободных блоков
		 *
		 * \~english
		 * @brief Free list linkage class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Link {
			private:
				// Зерно перемешивания указателей, одно на весь процесс
				static uintptr_t _cookie;
			private:
				/**
				 * \~russian
				 * @brief Метод перемешивания указателя
				 *
				 * @note Перемешивание парное: тот же вызов и мешает, и разбирает обратно
				 *
				 * @param block блок, хранящий указатель
				 * @param value перемешиваемый указатель
				 * @return      перемешанный указатель
				 *
				 * \~english
				 * @brief Method of mangling a pointer
				 *
				 * @note Mangling is pairwise: the same call both mangles and unmangles
				 *
				 * @param block block storing the pointer
				 * @param value pointer to mangle
				 * @return      mangled pointer
				 *
				 * \~
				 */
				static AWH_LINK_INLINE void * mangle(const void * block, void * value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод посева зерна перемешивания
				 *
				 * @note Зовётся при заведении центральных списков, прежде чем связан
				 *       хоть один блок; повторный вызов зерна не меняет
				 *
				 * \~english
				 * @brief Method of seeding the pointer mangling cookie
				 *
				 * @note Called when creating central lists, before any block is linked;
				 *       a repeated call does not change the cookie
				 *
				 * \~
				 */
				static void seed() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод чтения указателя на следующий свободный блок
				 *
				 * @param block читаемый свободный блок
				 * @return      следующий свободный блок
				 *
				 * \~english
				 * @brief Method of reading the pointer to the next free block
				 *
				 * @param block free block to read
				 * @return      next free block
				 *
				 * \~
				 */
				static AWH_LINK_INLINE void * next(void * block) noexcept;
				/**
				 * \~russian
				 * @brief Метод записи указателя на следующий свободный блок
				 *
				 * @param block      изменяемый свободный блок
				 * @param subsequent записываемый следующий свободный блок
				 *
				 * \~english
				 * @brief Method of writing the pointer to the next free block
				 *
				 * @param block      free block to modify
				 * @param subsequent next free block to write
				 *
				 * \~
				 */
				static AWH_LINK_INLINE void next(void * block, void * subsequent) noexcept;
		} link_t;
	};
};
