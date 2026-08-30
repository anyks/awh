/**
 * @file mangle.cpp
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
 * @brief Тесты перемешивания указателей в списках свободных блоков распределителя памяти
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
#include <cstdlib>

/**
 * Под санитайзерами проверять здесь нечего
 *
 * Захват там не состоится вовсе, выдача идёт мимо нас, и списков свободных блоков
 * наших в процессе попросту нет. Отсутствие это не замалчивается: его утверждает
 * `AllocCaptureTest.ProcessAllocationIsCaptured`
 */
#if !AWH_ALLOC_SANITIZED

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод чтения первого слова блока
	 *
	 * @note Побайтовым копированием: выравнивания указателя выданная память не обещает
	 *
	 * @param block читаемый блок
	 * @return      хранимое в блоке слово
	 *
	 */
	static uintptr_t word(const void * block) noexcept {
		// Хранимое слово
		uintptr_t result = 0;
		// Читаем слово побайтовым копированием
		::memcpy(&result, block, sizeof(result));
		// Выводим хранимое слово
		return result;
	}
	/**
	 * @brief Метод подсчёта различающихся разрядов
	 *
	 * @param first  первое число
	 * @param second второе число
	 * @return       число различающихся разрядов
	 *
	 */
	static size_t distance(const uintptr_t first, const uintptr_t second) noexcept {
		// Число различающихся разрядов
		size_t result = 0;
		/**
		 * Перебираем разряды различия
		 */
		for(uintptr_t value = (first ^ second); value != 0; value >>= 1)
			// Считаем разряд различия
			result += static_cast <size_t> (value & 1);
		// Выводим число различающихся разрядов
		return result;
	}
};

/**
 * @brief Тест хранения связи свободных блоков в перемешанном виде
 *
 * @note Свободный блок хранит связь на следующий свободный в собственном теле - в той
 *       самой памяти, что была выдана наружу. Переполнение буфера у соседа переписывает
 *       эту связь и заставляет распределитель выдать заданный взломщиком адрес. Связь
 *       обязана храниться перемешанной, и проверяется это на настоящих числах: молча
 *       выключенное перемешивание признаком сборки от рабочего не отличить
 *
 */
TEST(AllocMangleTest, LinkIsNotStoredInPlain){
	/**
	 * Проверка эта спрашивает перемешивание, а оно снимается сборкой
	 *
	 * Признак AWH_ALLOC_NO_MANGLE кладёт указатель в блок как есть намеренно, и
	 * утверждать о его перемешанности в такой сборке нельзя
	 */
	#if defined(AWH_ALLOC_NO_MANGLE)
		// Пропускаем проверку: заслон списка снят сборкой
		GTEST_SKIP() << "заслон списка снят сборкой: AWH_ALLOC_NO_MANGLE";
	#endif
	// Выдаём два блока одного разряда
	void * first = ::malloc(64);
	void * second = ::malloc(64);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	ASSERT_NE(first, second);
	/**
	 * Освобождаем блоки по порядку
	 *
	 * Список свободных ведётся с головы: освободив первый, затем второй, связь второго
	 * ведёт на первый. Внутрь распределителя тест при этом не заглядывает вовсе
	 */
	::free(first);
	::free(second);
	// Читаем слово, хранимое в освобождённом блоке
	const uintptr_t stored = ::word(second);
	// Забираем блоки обратно, подтверждая порядок списка
	void * again = ::malloc(64);
	void * later = ::malloc(64);
	// Порядок обязан подтвердиться: иначе связь ведёт не туда, куда думает тест
	EXPECT_EQ(again, second);
	EXPECT_EQ(later, first);
	// Связь обязана храниться не в открытую
	EXPECT_NE(stored, reinterpret_cast <uintptr_t> (first));
	// Освобождаем выданное
	::free(again);
	::free(later);
}
/**
 * @brief Тест участия места блока в перемешивании
 *
 * @note Одного зерна мало: значение, подсмотренное у одного блока, годилось бы
 *       подделкой всякому другому. Два блока, чья связь ведёт на ОДИН и тот же адрес,
 *       обязаны хранить РАЗНЫЕ слова
 *
 */
TEST(AllocMangleTest, LinkDependsOnBlockAddress){
	/**
	 * Проверка эта спрашивает перемешивание, а оно снимается сборкой
	 *
	 * Признак AWH_ALLOC_NO_MANGLE кладёт указатель в блок как есть намеренно, и
	 * утверждать о его перемешанности в такой сборке нельзя
	 */
	#if defined(AWH_ALLOC_NO_MANGLE)
		// Пропускаем проверку: заслон списка снят сборкой
		GTEST_SKIP() << "заслон списка снят сборкой: AWH_ALLOC_NO_MANGLE";
	#endif
	// Выдаём три блока одного разряда
	void * anchor = ::malloc(96);
	void * left = ::malloc(96);
	void * right = ::malloc(96);
	ASSERT_NE(anchor, nullptr);
	ASSERT_NE(left, nullptr);
	ASSERT_NE(right, nullptr);
	// Освобождаем якорь, затем левый: связь левого ведёт на якорь
	::free(anchor);
	::free(left);
	// Читаем слово левого блока
	const uintptr_t sample = ::word(left);
	// Забираем левый обратно, чтобы головой списка снова стал якорь
	void * taken = ::malloc(96);
	EXPECT_EQ(taken, left);
	// Освобождаем правый: его связь ведёт на тот же якорь
	::free(right);
	// Читаем слово правого блока
	const uintptr_t twin = ::word(right);
	// Одна связь у разных блоков обязана храниться по-разному
	EXPECT_NE(sample, twin);
	/**
	 * Различие обязано быть не в одном разряде
	 *
	 * Перемешивание сдвигом адреса даёт соседям различие в считанные разряды, а такое
	 * подделывается угадыванием смещения. Умножение на зерно разносит соседей по всей
	 * ширине слова
	 */
	EXPECT_GE(::distance(sample, twin), static_cast <size_t> (16));
	// Освобождаем выданное
	::free(taken);
}
/**
 * @brief Тест исправности списков под оборотом
 *
 * @note Перемешивание, разобранное неверно, вывело бы выдачу за пределы кучи. Блоки
 *       берутся во множестве, записываются целиком и сверяются на совпадение
 *
 */
TEST(AllocMangleTest, ListsSurviveChurn){
	// Число блоков в круге
	constexpr size_t COUNT = 4096;
	// Выданные блоки
	static void * blocks[COUNT];
	/**
	 * Перебираем блоки круга
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Выдаём блок
		blocks[i] = ::malloc(64);
		ASSERT_NE(blocks[i], nullptr);
		// Записываем блок целиком
		::memset(blocks[i], static_cast <int> (i & 0xFF), 64);
	}
	/**
	 * Перебираем блоки круга
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Получаем содержимое блока
		const uint8_t * bytes = reinterpret_cast <const uint8_t *> (blocks[i]);
		// Содержимое обязано уцелеть: пересечение блоков испортило бы его
		ASSERT_EQ(bytes[0], static_cast <uint8_t> (i & 0xFF));
		ASSERT_EQ(bytes[63], static_cast <uint8_t> (i & 0xFF));
		// Блоки обязаны быть разными
		if(i > 0)
			ASSERT_NE(blocks[i], blocks[i - 1]);
	}
	/**
	 * Перебираем блоки круга
	 */
	for(size_t i = 0; i < COUNT; i++)
		// Освобождаем блок
		::free(blocks[i]);
	/**
	 * Гоняем оборот через центральные списки
	 *
	 * Кэш потока переполняется и сдаёт блоки центру: связь при этом переписывается уже
	 * другим слоем, и перемешивание обоих слоёв обязано быть одним
	 */
	for(size_t round = 0; round < 8; round++){
		/**
		 * Перебираем блоки круга
		 */
		for(size_t i = 0; i < COUNT; i++){
			// Выдаём блок изменчивого разряда
			blocks[i] = ::malloc(64 + ((i % 3) * 64));
			ASSERT_NE(blocks[i], nullptr);
			// Записываем блок
			::memset(blocks[i], 0x5A, 64);
		}
		/**
		 * Перебираем блоки круга
		 */
		for(size_t i = 0; i < COUNT; i++)
			// Освобождаем блок
			::free(blocks[i]);
	}
}

#endif // !AWH_ALLOC_SANITIZED
