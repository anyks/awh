/**
 * @file capture.cpp
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
 * @brief Захват выдачи памяти процесса для набора тестов распределителя
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
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	// Признак состоявшегося захвата
	static bool __awh_alloc_seized__ = false;
	/**
	 * @brief Класс захвата выдачи памяти на весь набор
	 *
	 * @note Захват берётся ОДИН на процесс, и настройки его живут в переменных
	 *       процесса. Оттого настройки здесь взяты по умолчанию: тест, какому нужны
	 *       иные, обязан порождать собственный процесс, а не править общие
	 *
	 */
	class Capture : public ::testing::Environment {
		public:
			/**
			 * @brief Метод подготовки окружения набора
			 *
			 */
			void SetUp() override {
				// Настройки распределителя памяти
				awh::alloc::options_t options;
				// Захватываем выдачу памяти процесса
				__awh_alloc_seized__ = awh::alloc::Allocator::capture(options, nullptr);
			}
	};
	/**
	 * Заводим окружение набора до входа в главную функцию
	 *
	 * Захват обязан состояться прежде первого теста: у macOS выдача памяти до захвата
	 * идёт мимо нас - зоной системы, - и тест мерил бы чужой распределитель
	 */
	static const bool __awh_alloc_environment__ = (::testing::AddGlobalTestEnvironment(new Capture()), true);
};

/**
 * @brief Метод получения признака состоявшегося захвата
 *
 * @return признак состоявшегося захвата выдачи памяти процесса
 *
 */
bool __awh_alloc_captured__() noexcept {
	// Выводим признак состоявшегося захвата
	return __awh_alloc_seized__;
}

/**
 * @brief Тест состоявшегося захвата выдачи памяти
 *
 * @note Тест этот держит весь набор: без захвата прочие тесты мерили бы распределитель
 *       системы и проходили бы, ничего о нашем не сказав
 *
 */
TEST(AllocCaptureTest, ProcessAllocationIsCaptured){
	/**
	 * Под санитайзерами захват НЕ состоится
	 *
	 * Они подменяют выдачу памяти собою, и уступить её нам не могут. Ответом здесь
	 * обязан быть отказ, а не порча памяти, - это тоже утверждение, а не пропуск
	 */
	#if AWH_ALLOC_SANITIZED
		EXPECT_FALSE(::__awh_alloc_captured__());
	#else
		EXPECT_TRUE(::__awh_alloc_captured__());
	#endif
}
/**
 * @brief Тест принадлежности выданной памяти нашему распределителю
 *
 * @note Захват, состоявшийся лишь на словах, ничего не значит: выдача обязана идти
 *       ЧЕРЕЗ нас, и доказывается это разбором выданного адреса
 *
 */
TEST(AllocCaptureTest, AllocationGoesThroughUs){
	// Выдаём память
	void * block = ::malloc(1000);
	ASSERT_NE(block, nullptr);
	/**
	 * Выданное обязано принадлежать нашему распределителю
	 *
	 * Под санитайзером всё наоборот: выдача идёт мимо нас, и разбор адреса обязан
	 * отвечать нулём - иначе распределитель врёт о принадлежности чужой памяти
	 */
	#if AWH_ALLOC_SANITIZED
		EXPECT_EQ(awh::alloc::Allocator::resolve(block).size, static_cast <size_t> (0));
	#else
		EXPECT_GE(awh::alloc::Allocator::resolve(block).size, static_cast <size_t> (1000));
	#endif
	// Освобождаем выданное
	::free(block);
}
/**
 * Проверка эта своя у macOS: захват там идёт зоной выдачи
 */
#if (defined(__APPLE__) || defined(__MACH__)) && !AWH_ALLOC_SANITIZED

/**
 * Заголовочный файл зон выдачи памяти
 */
#include <malloc/malloc.h>

/**
 * @brief Тест вопроса о размере НЕ нашего блока
 *
 * @note Размер блока система спрашивает ОБХОДОМ зон: она идёт по зонам, покуда одна не
 *       ответит ненулём. Ответь наша зона о чужом блоке не нулём, а вопросом к прежнему
 *       распределителю - и тот спросил бы тем же обходом вновь: возвратность срывала
 *       стек, а падение выходило вдали от места, в слое страниц
 *
 * @warning Проверка эта на исправленном коде проходит мгновенно, а на порченом ВАЛИТ
 *          процесс: иного исхода у сорванного стека нет
 *
 */
TEST(AllocCaptureTest, ForeignBlockSizeDoesNotRecurse){
	// Адрес на стеке: нашему распределителю он заведомо чужой
	volatile int local = 0;
	// Спрашиваем размер чужого адреса обходом зон
	const size_t size = ::malloc_size(const_cast <const int *> (&local));
	// Чужой адрес обязан получить нулевой размер, а не сорвать стек
	EXPECT_EQ(size, static_cast <size_t> (0));
	// Спрашиваем размер нулевого адреса
	EXPECT_EQ(::malloc_size(nullptr), static_cast <size_t> (0));
}

#endif // (__APPLE__ || __MACH__) && !AWH_ALLOC_SANITIZED
