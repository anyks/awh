/**
 * @file: parameterized.cpp
 * @date: 2025-12-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "threadpool.hpp"

/**
 * @brief Структура параметров теста
 *
 */
struct ThreadPoolTestParameter {
	// Список элементов для обработки
	std::vector <uint32_t> items;
};

/**
 * @brief Класс параметризованной фикстуры для тестов пула потоков
 *
 */
class ThreadPoolParameterizedFixture : public ThreadPoolFixture, public ::testing::WithParamInterface <ThreadPoolTestParameter> {
	public:
		// Мьютекс для синхронизации потоков
		awh::lock_state_t <std::mutex> _mtx;
		// Параметры теста
		ThreadPoolTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования пула потоков с параметрами
 *
 */
TEST_P(ThreadPoolParameterizedFixture, ThreadPoolTest){
	// Если пул потоков инициализирован
	if(this->_thr->initialized())
		// Очищаем пул потоков
		this->_thr->clean();
	// Инициализируем пул потоков
	this->_thr->init(static_cast <uint16_t> (this->_parameter.items.size()));
	// Проверяем что пул потоков инициализирован
	ASSERT_TRUE(this->_thr->initialized());
	// Счётчики для проверки результатов
	uint32_t count1 = 0, count2 = 0;
	/**
	 * Перебираем элементы для обработки
	 */
	for(auto & item : this->_parameter.items){
		// Увеличиваем первый счётчик
		count1 += item;
		// Добавляем задачу в пул потоков
		this->_thr->push([&count2, this](const uint32_t num) noexcept -> void {
			// Выполняем блокировку уникальным мютексом
			const awh::locker_t <std::mutex> lock(this->_mtx);
			// Увеличиваем второй счётчик
			count2 += num;
		}, item);
	}
	// Проверяем размер очереди задач
	ASSERT_GE(this->_thr->getTaskQueueSize(), 0);
	// Ожидаем завершения всех задач
	this->_thr->wait();
	// Сравниваем результаты
	ASSERT_EQ(count1, count2);
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ThreadPoolParameterizedFixture,
	::testing::Values(
		ThreadPoolTestParameter({{3,5,9,18,22,44,15,49,21}}),
		ThreadPoolTestParameter({{15,39,41,17,81,12,1,99,120,0,33}}),
		ThreadPoolTestParameter({{67,72,1,0,99,14,74,324,13,99,13,47,254}}),
		ThreadPoolTestParameter({{874,732,124}})
	)
);
