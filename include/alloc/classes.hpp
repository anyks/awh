/**
 * @file classes.hpp
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
 * @brief Заголовочный файл классов размеров — разбиение мелких запросов на разряды,
 *        внутри которых блоки выдаются одинакового размера
 *
 * @section classes_decisions Намеренные решения
 *
 * @details <b>Поиск разряда, размер блока и число разрядов вынесены посредниками с
 *          принудительной подстановкой.</b> Зовут их на пути КАЖДОЙ выдачи и каждого
 *          освобождения, а работы там - таблица да три сравнения: вызов между
 *          единицами трансляции стоил бы дороже самой работы. Замерено на стенде
 *          `benchmark/alloc`: 19,59 → 17,10 наносекунды на действие, то есть 13 %.
 *          Прочее тело разрядов лежит в файле кода, как и положено.
 *
 *          <b>Разряды растут геометрически, а не равномерно.</b> Равномерный шаг даёт
 *          либо непомерное число разрядов, либо непомерную потерю на округлении у
 *          мелких запросов: шаг в 64 байта на запросе в 129 байт теряет треть. Шаг же,
 *          равный восьмой доле размера, держит потерю ниже 12,5 % при семи десятках
 *          разрядов.
 *
 *          <b>Выравнивание шестнадцать байт у всех разрядов.</b> Язык требует, чтобы
 *          выданная память годилась под любой вид с основным выравниванием, а таковое
 *          на всех наших системах равно шестнадцати. Восьмибайтовое выравнивание
 *          мелких разрядов сберегло бы немного памяти ценою нарушения этого правила.
 *
 *          <b>Таблицы строятся при заведении, а не пишутся числами.</b> Записанные
 *          числами разряды расходятся с расчётом при первой же правке размера страницы
 *          или порога мелкого запроса, и расхождение это ничем не выявляется. Здесь
 *          таблица строится тем же расчётом, что и проверяется.
 *
 *          <b>Число страниц на разряд подбирается по потере в хвосте.</b> Область,
 *          нарезаемая на блоки, редко делится на них нацело, и остаток пропадает.
 *          Оттого для каждого разряда берётся наименьшее число страниц, при каком
 *          остаток не превышает восьмой доли области.
 *
 * \~english
 * @brief Header file of size classes — splitting small requests into classes within
 *        which blocks are issued of the same size
 *
 * @section classes_decisions Intentional decisions
 *
 * @details <b>Class lookup, block size, and class count are placed inline with forced
 *          inlining.</b> They are called on EVERY allocation and free path, and the work
 *          there is a table lookup plus three comparisons: a cross-translation-unit call
 *          would cost more than the work itself. Measured on the `benchmark/alloc`
 *          stand: 19.59 → 17.10 nanoseconds per action, i.e. 13 %. The rest of the
 *          class body lives in the code file, as it should.
 *
 *          <b>Classes grow geometrically, not uniformly.</b> A uniform step gives either
 *          an excessive number of classes or excessive rounding loss on small requests:
 *          a 64-byte step on a 129-byte request loses a third. A step equal to one
 *          eighth of the size keeps the loss below 12.5 % with seven dozen classes.
 *
 *          <b>Alignment is sixteen bytes for all classes.</b> The language requires
 *          allocated memory to be suitable for any type with fundamental alignment,
 *          and on all our systems that equals sixteen. Eight-byte alignment of small
 *          classes would save some memory at the cost of violating this rule.
 *
 *          <b>Tables are built at creation, not written as numbers.</b> Classes written
 *          as numbers diverge from the calculation at the first edit of the page size
 *          or the small-request threshold, and the divergence is not detectable. Here
 *          the table is built by the same calculation that is checked.
 *
 *          <b>Pages per class are chosen by tail loss.</b> A region carved into blocks
 *          rarely divides evenly, and the remainder is lost. Hence for each class the
 *          smallest page count is taken such that the remainder does not exceed one
 *          eighth of the region.
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
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "pages.hpp"
#include "../sys/macro/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_CLASSES_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_CLASSES_INLINE inline __attribute__((always_inline))
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
		 * @brief Класс разрядов размеров
		 *
		 * \~english
		 * @brief Size classes class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Classes {
			public:
				// Выравнивание выдаваемых блоков в байтах
				static constexpr size_t ALIGN = 16;
				/**
				 * Наибольший размер, какой разряды СПОСОБНЫ обслужить, в байтах
				 *
				 * Это ёмкость таблиц поиска, а не действующая граница: та задаётся при
				 * построении и по умолчанию много ниже. Ёмкость стоит лишних 768 октет
				 * статической памяти на таблицу крупных запросов, и больше ничего
				 */
				// Наибольший размер, какой разряды способны обслужить, в байтах
				static constexpr size_t MAXIMUM = (128u * 1024u);
				/**
				 * Действующая граница разрядов по умолчанию, в байтах
				 *
				 * Граница эта - РАЗМЕН, и обе его стороны отмерены. Выше неё блок
				 * обслуживается областью страниц, ниже - разрядом с кэшем потока.
				 *
				 * Поднять границу до 128 КБ значит выиграть на выдаче крупных блоков
				 * втрое: 22.70-23.61 против 9.56-9.72 нс/оп на замере, - и это лучше
				 * tcmalloc (8.58). Выиграть и на удержании множества блоков: 15.22-15.92
				 * против 9.63-10.20.
				 *
				 * И проиграть ВЧЕТВЕРО на росте перевыдачей: 64.57-66.70 против
				 * 268.50-271.63. Причина в том, что область страниц умеет расти НА МЕСТЕ,
				 * поглощая свободных соседей, а блок разряда - не умеет и не может:
				 * размер его определён разрядом. Всякая перевыдача с ростом в этом
				 * поясе обращается в новую выдачу с полным копированием.
				 *
				 * Соперники этой возможности не имеют вовсе: на том же замере tcmalloc
				 * даёт 259.23 нс/оп, jemalloc - 81.67, системный - 91.20, а мы 67.43.
				 * Поднятая граница обратила бы наше первое место в последнее.
				 *
				 * Оттого по умолчанию граница низкая: рост перевыдачей встречается в
				 * прикладном коде чаще, чем выдача крупных блоков постоянного размера, а
				 * терять втрое там, где мы впереди всех, ради выигрыша там, где мы вторые,
				 * - размен невыгодный. Приложению, у какого нагрузка иная, граница
				 * задаётся настройкой `classLimit`
				 */
				// Действующая граница разрядов по умолчанию, в байтах
				static constexpr size_t DEFAULT = (32u * 1024u);
				// Наибольшее число разрядов
				static constexpr size_t LIMIT = 96;
				// Порог, до которого поиск разряда идёт мелкой таблицей
				static constexpr size_t SMALL = 1024;
				// Шаг мелкой таблицы поиска в байтах
				static constexpr size_t STEP_SMALL = ALIGN;
				// Шаг крупной таблицы поиска в байтах
				static constexpr size_t STEP_LARGE = 128;
			private:
				// Число заведённых разрядов
				size_t _count;
				/**
				 * Действующая граница разрядов в байтах
				 *
				 * Читается на КАЖДОЙ выдаче, оттого лежит рядом с числом разрядов: оба
				 * поля берутся из одной строки памяти, и второе чтение ничего не стоит
				 */
				// Действующая граница разрядов в байтах
				size_t _maximum;
				// Размер блока каждого разряда в байтах
				size_t _size[LIMIT];
				// Число страниц кучи на область каждого разряда
				size_t _pages[LIMIT];
				// Число блоков в области каждого разряда
				size_t _blocks[LIMIT];
				/**
				 * Таблицы поиска разряда
				 *
				 * Длина берётся с запасом в два места, а не в одно: место ищется делением
				 * с округлением вверх, и при размере, равном порогу, оно выходит на
				 * единицу больше частного. Взяв длину ровно по частному, поиск читал бы
				 * за концом массива - и читал бы мусор, а не отказывал
				 */
				// Таблица поиска разряда для мелких запросов
				uint8_t _small[(SMALL / STEP_SMALL) + 2];
				// Таблица поиска разряда для крупных запросов
				uint8_t _large[(MAXIMUM / STEP_LARGE) + 2];
				/**
				 * Разрядность таблиц поиска связана с числом разрядов
				 *
				 * Таблицы держат НОМЕР разряда в одном октете: номеров у нас девяносто
				 * шесть, и октета им с запасом, а таблица о двух тысячах ячеек лежит в
				 * кэше целиком. Подними кто предел разрядов выше двухсот пятидесяти
				 * пяти - и номера начали бы усекаться молча, отдавая на всякий крупный
				 * запрос чужой разряд
				 */
				static_assert((LIMIT <= 255), "номер разряда обязан помещаться в октет таблиц поиска");
			public:
				/**
				 * \~russian
				 * @brief Метод построения таблиц разрядов
				 *
				 * @return число заведённых разрядов
				 *
				 * \~english
				 * @brief Method of building the class tables
				 *
				 * @return number of classes created
				 *
				 * \~
				 */
				size_t init(const size_t limit = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения действующей границы разрядов
				 *
				 * @note Величина эта - НЕ `MAXIMUM`: та означает ёмкость таблиц поиска, а
				 *       действующая граница задаётся построением и по умолчанию много ниже
				 *
				 * @return действующая граница разрядов в байтах
				 *
				 * \~english
				 * @brief Method of getting the effective size class boundary
				 *
				 * @note This value is NOT `MAXIMUM`: that means the lookup table capacity,
				 *       while the effective boundary is set at build time and by default is
				 *       much lower
				 *
				 * @return effective size class boundary in bytes
				 *
				 * \~
				 */
				AWH_CLASSES_INLINE size_t maximum() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения разряда по требуемому размеру
				 *
				 * @param size требуемый размер в байтах
				 * @return     номер разряда, либо LIMIT если размер разрядами не обслуживается
				 *
				 * \~english
				 * @brief Method of determining the class by the required size
				 *
				 * @param size required size in bytes
				 * @return     class number, or LIMIT if the size is not served by classes
				 *
				 * \~
				 */
				AWH_CLASSES_INLINE size_t index(const size_t size) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения размера блока разряда
				 *
				 * @param index номер разряда
				 * @return      размер блока в байтах
				 *
				 * \~english
				 * @brief Method of getting the class block size
				 *
				 * @param index class number
				 * @return      block size in bytes
				 *
				 * \~
				 */
				AWH_CLASSES_INLINE size_t size(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения размера блока разряда БЕЗ проверки номера
				 *
				 * @warning Номер разряда обязан быть годным: звавший ручается за него
				 *          сам. Негодный номер читает память за таблицей
				 *
				 * @note Заведён ради горячих путей. Те спрашивают годность разряда сами -
				 *       без неё им не решить, идти ли быстрой дорогой вовсе, - и
				 *       спрашивали её ВТОРИЧНО на каждой выдаче и каждом возврате: счётчик
				 *       инструкций отдавал одному этому сличению пять команд на действие
				 *       при сорока трёх на весь путь у соперника
				 *
				 * @param index номер разряда
				 * @return      размер блока в байтах
				 *
				 * \~english
				 * @brief Method of getting the class block size without checking the index
				 *
				 * @warning The class number must be valid: the caller vouches for it
				 *          themselves. An invalid number reads memory past the table
				 *
				 * @note Created for hot paths. Those check the class validity themselves -
				 *       without it they cannot decide whether to take the fast path at all -
				 *       and used to check it SECONDARILY on every allocation and every
				 *       return: the instruction counter charged this single comparison five
				 *       commands per action against forty-three for the entire path at the
				 *       competitor
				 *
				 * @param index class number
				 * @return      block size in bytes
				 *
				 * \~
				 */
				AWH_CLASSES_INLINE size_t extent(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа страниц на область разряда
				 *
				 * @param index номер разряда
				 * @return      число страниц кучи
				 *
				 * \~english
				 * @brief Method of getting the number of pages per class region
				 *
				 * @param index class number
				 * @return      number of heap pages
				 *
				 * \~
				 */
				size_t pages(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа блоков в области разряда
				 *
				 * @param index номер разряда
				 * @return      число блоков
				 *
				 * \~english
				 * @brief Method of getting the number of blocks per class region
				 *
				 * @param index class number
				 * @return      number of blocks
				 *
				 * \~
				 */
				size_t blocks(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа заведённых разрядов
				 *
				 * @return число разрядов
				 *
				 * \~english
				 * @brief Method of getting the number of classes created
				 *
				 * @return number of classes
				 *
				 * \~
				 */
				AWH_CLASSES_INLINE size_t count() const noexcept;
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
				Classes() noexcept;
		} classes_t;
	};
};
