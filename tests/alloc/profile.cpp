/**
 * @file profile.cpp
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
 * @brief Тесты учёта мест выдачи памяти
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
 * @brief Тест накопления мест выдачи
 *
 * @note Учёт ведёт ЖИВЫЕ блоки: спроси его после нагрузки, где всё освобождено, он
 *       ответит пустотой - и проверка доложила бы о молчащем учёте там, где учитывать
 *       нечего. Оттого блоки держатся живыми на время опроса
 *
 */
TEST_F(AllocFixture, ProfileCollectsHoldings){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём учёту мест выдачи долю выборки
	options.profileRate = 64;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выданные блоки
	std::vector <void *> alive;
	/**
	 * Перебираем блоки нагрузки
	 */
	for(size_t i = 0; i < 4096; i++){
		// Выдаём блок изменчивого размера
		void * block = ::malloc(64 + (i % 4000));
		// Запоминаем выданный блок
		if(block != nullptr)
			// Добавляем блок в список живых
			alive.push_back(block);
	}
	// Число учтённых мест выдачи
	size_t seen = 0;
	// Число мест выдачи со снятым стеком
	size_t deep = 0;
	// Объём учтённого
	size_t bytes = 0;
	// Опрашиваем учёт мест выдачи
	awh::alloc::Allocator::holdings([&seen, &deep, &bytes](const awh::alloc::holding_t & holding) noexcept -> bool {
		// Считаем учтённое место выдачи
		seen++;
		// Копим объём учтённого
		bytes += holding.size;
		// Если стек вызовов снят
		if((holding.frames != nullptr) && (holding.depth > 0))
			// Считаем место выдачи со снятым стеком
			deep++;
		// Продолжаем обход
		return true;
	});
	// Учёт обязан что-то накопить
	EXPECT_GT(seen, static_cast <size_t> (0));
	// Объём учтённого обязан быть ненулевым
	EXPECT_GT(bytes, static_cast <size_t> (0));
	/**
	 * Записи без стека допускаются, но их обязано быть мало
	 *
	 * Раскрутка у части систем сама просит памяти (у OpenBSD это наблюдалось), и
	 * вложенный съём законно отдаёт пустоту: сторож возвратности его обрывает.
	 * Требовать стек у КАЖДОЙ записи значило бы объявить дефектом устройство сторожа
	 */
	EXPECT_GE(deep, ((seen * 9) / 10));
	/**
	 * Перебираем живые блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
}
/**
 * @brief Тест прекращения обхода мест выдачи
 *
 * @note Обход обязан слушаться отклика: отказ продолжать - способ забрать первые записи,
 *       не платя за обход всего учёта
 *
 */
TEST_F(AllocFixture, HoldingsWalkStopsOnRequest){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём учёту мест выдачи долю выборки
	options.profileRate = 1;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выданные блоки
	std::vector <void *> alive;
	/**
	 * Перебираем блоки нагрузки
	 */
	for(size_t i = 0; i < 256; i++){
		// Выдаём блок
		void * block = ::malloc(1024);
		// Запоминаем выданный блок
		if(block != nullptr)
			// Добавляем блок в список живых
			alive.push_back(block);
	}
	// Число обойдённых мест выдачи
	size_t seen = 0;
	// Опрашиваем учёт, прекращая обход на первой же записи
	awh::alloc::Allocator::holdings([&seen](const awh::alloc::holding_t &) noexcept -> bool {
		// Считаем обойдённое место выдачи
		seen++;
		// Прекращаем обход
		return false;
	});
	// Обход обязан прекратиться на первой записи, а не пройти учёт целиком
	EXPECT_LE(seen, static_cast <size_t> (1));
	/**
	 * Перебираем живые блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
}

#endif // !AWH_ALLOC_SANITIZED
