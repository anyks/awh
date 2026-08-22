/**
 * @file purge.cpp
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
 * @brief Тесты возврата памяти системе и потолков занятого
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл главного модуля тестов
 */
#include "../main.hpp"

/**
 * Подключаем заголовочный файл набора
 */
#include "suite.hpp"

/**
 * Стандартные модули
 */
#include <vector>
#include <cstdlib>

/**
 * Под санитайзерами проверять здесь нечего: захват не состоится
 */
#if !AWH_ALLOC_SANITIZED

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод набора и освобождения памяти
	 *
	 * @note Берётся ЗАВЕДОМО много и крупными кусками: отдавать системе распределитель
	 *       вправе лишь целыми страницами, и мелочь россыпью повода к отдаче не даёт
	 *
	 * @param count число блоков
	 * @param size  размер блока
	 *
	 */
	static void churn(const size_t count, const size_t size) noexcept {
		// Выданные блоки
		std::vector <void *> alive;
		// Отводим место под список блоков
		alive.reserve(count);
		/**
		 * Перебираем блоки
		 */
		for(size_t i = 0; i < count; i++){
			// Выдаём блок
			void * block = ::malloc(size);
			// Если блок выдан
			if(block != nullptr){
				// Записываем начало блока
				::memset(block, 0x33, 64);
				// Запоминаем выданный блок
				alive.push_back(block);
			}
		}
		/**
		 * Перебираем выданные блоки
		 */
		for(void * block : alive)
			// Освобождаем блок
			::free(block);
	}
};

/**
 * @brief Тест возврата памяти системе по просьбе
 *
 */
TEST_F(AllocFixture, ManualPurgeReturnsMemory){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём отдачу лишь по просьбе
	options.purgeMode = awh::alloc::purge_t::MANUAL;
	// Снимаем отсрочку: удерживать освобождённое незачем
	options.purgeDelay = 0;
	// Снимаем порог куска
	options.purgeBlock = 0;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Набираем и освобождаем память
	::churn(4096, 8192);
	// Свободного в страничной куче обязано накопиться
	EXPECT_GT(awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE), static_cast <size_t> (0));
	// Запоминаем отданное системе до просьбы
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED);
	// Просим отдать свободное системе
	const size_t purged = awh::alloc::Allocator::purge();
	// Отдача обязана состояться: либо по ответу, либо по счётчику отданного
	EXPECT_TRUE((purged > 0) || (awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED) > before));
}
/**
 * @brief Тест полного запрета отдачи
 *
 * @note Запрет обязан быть СИЛЬНЕЕ просьбы: приложение, занявшее память навсегда,
 *       вправе рассчитывать, что случайный вызов `purge` её не отнимет
 *
 */
TEST_F(AllocFixture, NeverPurgeIsStrongerThanRequest){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём полный запрет отдачи
	options.purgeMode = awh::alloc::purge_t::NEVER;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Набираем и освобождаем память
	::churn(2048, 8192);
	// Запоминаем отданное системе до просьбы
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED);
	// Просьба обязана остаться без последствий
	EXPECT_EQ(awh::alloc::Allocator::purge(), static_cast <size_t> (0));
	// Отданное системе обязано остаться прежним
	EXPECT_EQ(awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED), before);
}
/**
 * @brief Тест потолка поток-локальных кэшей
 *
 */
TEST_F(AllocFixture, CacheCeilingHolds){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём потолок поток-локальных кэшей
	options.cacheLimit = (64u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Набираем и освобождаем память мелкими блоками: их и держат кэши
	::churn(20000, 64);
	/**
	 * Потолок сверяется с запасом
	 *
	 * Потолок этот - на кэш ПОТОКА, а не на процесс: потоков в наборе больше одного,
	 * и требовать от суммы всех кэшей потолка одного было бы неверно
	 */
	EXPECT_LE(awh::alloc::Allocator::property(awh::alloc::property_t::CACHED), (options.cacheLimit * 8));
}
/**
 * @brief Тест опроса состояния занятого
 *
 */
TEST_F(AllocFixture, PropertiesAreConsistent){
	// Выдаём заметный блок
	void * block = ::malloc(1u << 20);
	ASSERT_NE(block, nullptr);
	// Записываем начало блока
	::memset(block, 0x11, 64);
	// Занятое прикладным кодом обязано быть не меньше выданного
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED), static_cast <size_t> (1u << 20));
	// Наибольшее занятое обязано быть не меньше занятого прямо сейчас
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::PEAK), awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED));
	// Взятое у системы обязано покрывать занятое прикладным кодом
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::HEAP), awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED));
	// Освобождаем блок
	::free(block);
}

#endif // !AWH_ALLOC_SANITIZED
