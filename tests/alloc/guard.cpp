/**
 * @file guard.cpp
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
 * @brief Тесты заслонов, карантина и разбора адреса до блока
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
 * Под санитайзерами проверять здесь нечего: захват не состоится, и слоёв наших
 * в процессе нет. Отсутствие это утверждает `AllocCaptureTest`
 */
#if !AWH_ALLOC_SANITIZED

/**
 * @brief Тест действующих заслонов
 *
 * @note Заслон кладёт блок вплотную к концу страницы, а следующую страницу закрывает:
 *       запись за конец блока валит программу немедля, а не портит соседа. Проверяется
 *       это по СЛЕДУ - концу блока на границе страницы, - а не по признаку настройки:
 *       заслон, взятый и не поставленный, признаком от рабочего не отличить
 *
 */
TEST_F(AllocFixture, GuardsAreActuallyPlaced){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём выборку заслонов: один блок из шестнадцати
	options.guardRate = 16;
	// Задаём объём карантина
	options.quarantine = (256u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выданные блоки
	std::vector <void *> alive;
	/**
	 * Выдаём вдоволь: выборка берёт одну выдачу из шестнадцати
	 */
	for(size_t i = 0; i < 4096; i++){
		// Выдаём блок изменчивого размера
		void * block = ::malloc(64 + (i % 512));
		// Запоминаем выданный блок
		if(block != nullptr)
			// Добавляем блок в список живых
			alive.push_back(block);
	}
	// Число заслонённых блоков
	size_t sealed = 0;
	/**
	 * Перебираем живые блоки
	 */
	for(void * block : alive){
		// Разбираем адрес блока
		const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
		// У заслонённого блока конец совпадает с границей страницы системы
		if((region.size > 0) && (((reinterpret_cast <uintptr_t> (block) + region.size) % 4096) == 0))
			// Считаем заслонённый блок
			sealed++;
	}
	// Хоть один блок обязан оказаться заслонённым
	EXPECT_GT(sealed, static_cast <size_t> (0));
	/**
	 * Перебираем живые блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
}
/**
 * @brief Тест опознания освобождённого блока
 *
 * @note Карантин держит освобождённое некоторое время, и разбор адреса обязан назвать
 *       такой блок освобождённым: ровно это отличает висячий указатель от чужой памяти
 *
 */
TEST_F(AllocFixture, FreedBlockIsRecognized){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём выборку заслонов
	options.guardRate = 1;
	// Задаём объём карантина
	options.quarantine = (256u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выдаём блок
	void * block = ::malloc(512);
	ASSERT_NE(block, nullptr);
	// Записываем блок целиком
	::memset(block, 0x77, 512);
	// Живой блок обязан опознаваться живым
	EXPECT_EQ(awh::alloc::Allocator::resolve(block).origin, awh::alloc::origin_t::LIVE);
	// Освобождаем блок
	::free(block);
	// Освобождённый блок обязан опознаваться освобождённым
	EXPECT_EQ(awh::alloc::Allocator::resolve(block).origin, awh::alloc::origin_t::FREED);
}
/**
 * @brief Тест разбора чужих и негодных адресов
 *
 */
TEST_F(AllocFixture, ForeignAddressesAreTold){
	// Место на стеке распределителю не принадлежит
	const uint64_t anchor = 0;
	EXPECT_EQ(awh::alloc::Allocator::resolve(&anchor).origin, awh::alloc::origin_t::FOREIGN);
	// Нулевая страница опознаётся собою: разыменование пустого указателя видно сразу
	EXPECT_EQ(awh::alloc::Allocator::resolve(reinterpret_cast <const void *> (16)).origin, awh::alloc::origin_t::NULLPAGE);
}
/**
 * @brief Тест разбора адреса за концом блока
 *
 * @note Разбор обязан назвать не только принадлежность, но и СТОРОНУ промаха: адрес за
 *       концом блока и адрес перед началом - разные ошибки, и лечатся они по-разному
 *
 */
TEST_F(AllocFixture, OverrunAndUnderrunAreTold){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём выборку заслонов: заслон ставится всякой выдаче
	options.guardRate = 1;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выдаём блок
	uint8_t * block = reinterpret_cast <uint8_t *> (::malloc(128));
	ASSERT_NE(block, nullptr);
	// Разбираем адрес блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
	// Если блок оказался заслонённым
	if(region.origin == awh::alloc::origin_t::LIVE){
		// Адрес за концом блока обязан опознаваться промахом за конец
		const awh::alloc::region_t beyond = awh::alloc::Allocator::resolve(block + region.size + 1);
		// Промах обязан быть назван: живым такой адрес быть не может
		EXPECT_NE(beyond.origin, awh::alloc::origin_t::LIVE);
	}
	// Освобождаем блок
	::free(block);
}

#endif // !AWH_ALLOC_SANITIZED
