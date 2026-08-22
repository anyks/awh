/**
 * @file contract.cpp
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
 * @brief Тесты договора выдачи памяти: края, какие обязан соблюдать всякий распределитель
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл главного модуля тестов
 */
#include "../main.hpp"

/**
 * Подключаем наши модули
 */
#include <alloc/alloc.hpp>

/**
 * Подключаем заголовочный файл набора
 */
#include "suite.hpp"

/**
 * Стандартные модули
 */
#include <new>
#include <cstdlib>

/**
 * @brief Тест выдачи нулевого размера
 *
 * @note Договор допускает и пустой указатель, и годный к освобождению адрес; чего он
 *       не допускает - так это выдачи, какую нельзя освободить
 *
 */
TEST(AllocContractTest, ZeroSizeIsFreeable){
	// Выдаём память нулевого размера
	void * block = ::malloc(0);
	// Освобождаем выданное: договор обязан пережить это в любом случае
	::free(block);
	// Пустой указатель освобождается по договору без последствий
	::free(nullptr);
	SUCCEED();
}
/**
 * @brief Тест перевыдачи памяти
 *
 */
TEST(AllocContractTest, ReallocKeepsContent){
	// Перевыдача пустого указателя равна выдаче
	void * block = ::realloc(nullptr, 1000);
	ASSERT_NE(block, nullptr);
	// Записываем содержимое
	::memset(block, 0x3C, 1000);
	// Растим блок
	block = ::realloc(block, 8192);
	ASSERT_NE(block, nullptr);
	// Признак сохранности содержимого
	bool kept = true;
	/**
	 * Перебираем записанное содержимое
	 */
	for(size_t i = 0; i < 1000; i++)
		// Проверяем сохранность байта
		kept = (kept && (reinterpret_cast <const uint8_t *> (block)[i] == 0x3C));
	// Содержимое обязано уцелеть при росте
	EXPECT_TRUE(kept);
	// Усекаем блок
	block = ::realloc(block, 32);
	ASSERT_NE(block, nullptr);
	// Признак сохранности содержимого
	bool trimmed = true;
	/**
	 * Перебираем уцелевшее содержимое
	 */
	for(size_t i = 0; i < 32; i++)
		// Проверяем сохранность байта
		trimmed = (trimmed && (reinterpret_cast <const uint8_t *> (block)[i] == 0x3C));
	// Содержимое обязано уцелеть при усечении
	EXPECT_TRUE(trimmed);
	// Освобождаем выданное
	::free(block);
}
/**
 * @brief Тест выдачи обнулённого
 *
 */
TEST(AllocContractTest, CallocZeroesAndRejectsOverflow){
	// Выдаём обнулённую память
	void * block = ::calloc(1000, 4);
	ASSERT_NE(block, nullptr);
	// Признак обнулённости выданного
	bool zeroed = true;
	/**
	 * Перебираем выданное содержимое
	 */
	for(size_t i = 0; i < (1000 * 4); i++)
		// Проверяем обнулённость байта
		zeroed = (zeroed && (reinterpret_cast <const uint8_t *> (block)[i] == 0));
	// Выданное обязано быть обнулено
	EXPECT_TRUE(zeroed);
	// Освобождаем выданное
	::free(block);
	/**
	 * Переполнение произведения обязано быть отвергнуто
	 *
	 * Иначе выдача выходит меньше запрошенного, а звавший пишет за её конец - это
	 * целый класс приёмов взлома, закрытый одной проверкой.
	 *
	 * Под санитайзером края не проверяются: он валит процесс на переполнении сам, по
	 * своей политике, и проверялся бы тогда ОН, а не мы
	 */
	#if !AWH_ALLOC_SANITIZED
		EXPECT_EQ(::calloc((std::numeric_limits <size_t>::max() / 2) + 2, 4), nullptr);
	#endif
}
/**
 * @brief Тест отказа при неподъёмном размере
 *
 * @note Отказ обязан быть ОТВЕТОМ, а не падением: звавший вправе разобрать нехватку
 *       памяти сам
 *
 */
TEST(AllocContractTest, HugeSizeRefusesWithoutCrash){
	/**
	 * Под санитайзером край не проверяется
	 *
	 * Неподъёмный размер он числит ошибкой и валит процесс, а не отвечает пустотой:
	 * проверялась бы его политика, а не наш договор
	 */
	#if !AWH_ALLOC_SANITIZED
		// Неподъёмный размер обязан быть отвергнут ответом
		EXPECT_EQ(::malloc(std::numeric_limits <size_t>::max()), nullptr);
		// То же и для размера чуть меньше предельного
		EXPECT_EQ(::malloc(std::numeric_limits <size_t>::max() - 4096), nullptr);
	#endif
}
/**
 * @brief Тест выдачи с требуемым выравниванием
 *
 */
TEST(AllocContractTest, AlignedAllocationIsAligned){
	/**
	 * Выдаём память с выравниванием средствами ЯЗЫКА, а не POSIX
	 *
	 * `posix_memalign` есть не везде: у MinGW его нет вовсе, и проверка, написанная на
	 * нём, закрывала себе Windows целиком. Выровненная выдача языка ведёт к тому же
	 * нашему пути, но пишется одинаково для всех систем
	 */
	void * block = ::operator new (8192, std::align_val_t(4096));
	ASSERT_NE(block, nullptr);
	// Выравнивание обязано быть соблюдено
	EXPECT_EQ((reinterpret_cast <uintptr_t> (block) % 4096), static_cast <uintptr_t> (0));
	// Блок обязан быть пригоден к записи целиком
	::memset(block, 0x77, 8192);
	EXPECT_EQ(reinterpret_cast <const uint8_t *> (block)[8191], static_cast <uint8_t> (0x77));
	// Освобождаем выданное
	::operator delete (block, std::align_val_t(4096));
	/**
	 * Негодное выравнивание обязано быть отвергнуто кодом, а не падением
	 *
	 * Проверяется лишь у систем POSIX: у MinGW `posix_memalign` нет вовсе. Под
	 * санитайзером край не проверяется и там: он валит процесс на негодном
	 * выравнивании сам, по своей политике
	 */
	#if !defined(_WIN32) && !defined(_WIN64) && !AWH_ALLOC_SANITIZED
		void * refused = nullptr;
		EXPECT_NE(::posix_memalign(&refused, 3, 128), 0);
	#endif
}
/**
 * @brief Тест разбора выданного адреса
 *
 */
TEST(AllocContractTest, ResolveKnowsOwnBlocks){
	// Выдаём память
	void * block = ::malloc(1000);
	ASSERT_NE(block, nullptr);
	/**
	 * Разбор своего блока обязан дать размер не меньше запрошенного
	 *
	 * Под санитайзером всё наоборот: выдача идёт мимо нас, и разбор обязан отвечать
	 * нулём - иначе распределитель врёт о принадлежности чужой памяти
	 */
	#if AWH_ALLOC_SANITIZED
		EXPECT_EQ(awh::alloc::Allocator::resolve(block).size, static_cast <size_t> (0));
	#else
		EXPECT_GE(awh::alloc::Allocator::resolve(block).size, static_cast <size_t> (1000));
	#endif
	// Освобождаем выданное
	::free(block);
	// Место на стеке распределителю не принадлежит
	const uint64_t anchor = 0;
	EXPECT_EQ(awh::alloc::Allocator::resolve(&anchor).size, static_cast <size_t> (0));
	// Пустой указатель разбирается нулём
	EXPECT_EQ(awh::alloc::Allocator::resolve(nullptr).size, static_cast <size_t> (0));
}
