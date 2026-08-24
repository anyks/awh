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
#include <thread>
#include <atomic>
#include <cstdlib>
#include <pthread.h>

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

/**
 * @brief Тест съёма стека при ключах с большими номерами
 *
 * @details Заводит ключи, съедающие первый уровень места под значения, и лишь затем
 *          включает учёт мест выдачи: ключ съёма достаётся тогда номер большой. У
 *          glibc первая отметка такого ключа заводит второй уровень ВЫЗОВОМ `calloc`,
 *          а тот приходит обратно в нашу выдачу - и если признак нахождения внутри
 *          съёма хранится тем же ключом, отметка не поспевает лечь, поток отмечается
 *          снова и срывает стек
 *
 * @note Проверка эта закрепляет НАМЕРЕННОЕ решение: признак хранится средствами
 *       собирателя всюду, где обращение к месту потока памяти не просит, и ключом
 *       системы лишь у macOS и OpenBSD, где просит наоборот. Прежний код валился здесь
 *       в SIGSEGV на Debian, glibc 2.36
 *
 */
TEST_F(AllocFixture, TraceSurvivesHighNumberedKeys){
	// Ключи, съедающие первый уровень места под значения
	pthread_key_t eaten[64];
	/**
	 * Перебираем заводимые ключи
	 */
	for(size_t i = 0; i < 64; i++)
		// Заводим ключ, съедающий место
		ASSERT_EQ(::pthread_key_create(&eaten[i], nullptr), 0) << "ключ " << i << " не заведён";
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём учёту мест выдачи долю выборки
	options.profileRate = 1;
	// Применяем настройки: ключ съёма заведётся ПОСЛЕ съеденных
	awh::alloc::Allocator::options(options);
	// Признак прохождения нагрузки свежим потоком
	std::atomic <bool> passed(false);
	/**
	 * Гоняем нагрузку СВЕЖИМ потоком: отметка съёма ставится там впервые
	 */
	std::thread worker([&passed]() noexcept -> void {
		/**
		 * Перебираем блоки нагрузки
		 */
		for(size_t i = 0; i < 4096; i++){
			// Выдаём блок
			void * block = ::malloc(64 + (i % 512));
			// Если блок выдан
			if(block != nullptr)
				// Освобождаем блок
				::free(block);
		}
		// Отмечаем нагрузку пройденной
		passed.store(true, std::memory_order_release);
	});
	// Дожидаемся потока нагрузки
	worker.join();
	// Нагрузка обязана пройти целиком, а не сорвать стек
	EXPECT_TRUE(passed.load(std::memory_order_acquire));
	// Выключаем учёт мест выдачи
	options.profileRate = 0;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	/**
	 * Перебираем заведённые ключи
	 */
	for(size_t i = 0; i < 64; i++)
		// Снимаем ключ, съедавший место
		::pthread_key_delete(eaten[i]);
}

#endif // !AWH_ALLOC_SANITIZED
