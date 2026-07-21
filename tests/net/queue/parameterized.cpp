/**
 * @file: parameterized.cpp
 * @date: 2026-02-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "queue.hpp"

/**
 * @brief Структура параметров теста
 *
 */
struct QueueTestParameter {
	// Список чисел для добавления в очередь
	std::vector <size_t> numbers;
};

/**
 * @brief Класс параметризованной фикстуры для тестов очереди
 *
 */
class QueueParameterizedFixture : public NetworkQueueFixture, public ::testing::WithParamInterface <QueueTestParameter> {
	public:
		// Параметры теста
		QueueTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованной фикстуры очереди
 *
 */
TEST_P(QueueParameterizedFixture, QueueTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для записей фиксированного размера (например, UDP)
	this->_queue->type(awh::net_queue_t::type_t::UDP);
	/**
	 * Переход по всем числам для добавления в очередь
	 */
	for(auto & number : this->_parameter.numbers)
		// Добавляем число в очередь
		this->_queue->push(&number, sizeof(number));
	// Проверяем что очередь не пустая
	ASSERT_FALSE(this->_queue->empty());
	// Проверяем что количество записей в очереди совпадает с добавленными числами
	ASSERT_EQ(this->_queue->count(), this->_parameter.numbers.size());
	// Проверяем что размер данных в очереди совпадает с добавленными числами
	ASSERT_EQ(this->_queue->size(), this->_parameter.numbers.size() * sizeof(size_t));
	// Проверяем что место в очереди ещё есть
	ASSERT_GT(this->_queue->available(), 0);
	// Проверяем что тип очереди по умолчанию UDP
	ASSERT_EQ(awh::net_queue_t::type_t::UDP, this->_queue->type());
	// Индекс текущего числа
	size_t index = 0;
	/**
	 * Обходим очередь пока она не опустеет
	 */
	while(!this->_queue->empty()){
		// Извлекаем число из очереди
		size_t number = 0;
		// Размер данных для извлечения из очереди
		size_t size = 0;
		// Указатель на данные в очереди
		const void * ptr = nullptr;
		// Получаем указатель на данные в очереди и их размер
		this->_queue->front(&ptr, size);
		// Копируем данные из очереди в число
		::memcpy(&number, ptr, size);
		// Удаляем запись из очереди
		this->_queue->pop();
		// Проверяем что извлечённое число совпадает с добавленным
		ASSERT_EQ(number, this->_parameter.numbers.at(index));
		// Увеличиваем индекс текущего числа
		index++;
	}
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Добавляем все числа в очередь одним вызовом
	this->_queue->push(this->_parameter.numbers.data(), this->_parameter.numbers.size() * sizeof(size_t));
	// Проверяем что размер данных в очереди совпадает с добавленными числами
	ASSERT_EQ(this->_queue->count(), this->_queue->size());
	// Создаём вектор для хранения чисел из очереди
	std::vector <size_t> numbers(this->_parameter.numbers.size());
	// Индекс текущего числа
	index = 0;
	/**
	 * Обходим очередь пока она не опустеет
	 */
	while(!this->_queue->empty()){
		// Указатель на данные в очереди
		const void * ptr = nullptr;
		// Размер данных для извлечения из очереди
		size_t size = 0;
		// Получаем указатель на данные в очереди и их размер
		this->_queue->front(&ptr, size);
		// Копируем данные из очереди в вектор чисел
		::memcpy(reinterpret_cast <uint8_t *> (&numbers[0]) + index, ptr, sizeof(size_t));
		// Удаляем запись из очереди
		this->_queue->pop(sizeof(size_t));
		// Увеличиваем смещение
		index += sizeof(size_t);
	}
	// Проверяем что извлечённые числа совпадают с добавленными числами
	ASSERT_EQ(numbers, this->_parameter.numbers);
}

/**
 * @brief Инициализация параметров теста очереди
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, QueueParameterizedFixture,
	::testing::Values(
		QueueTestParameter({{888,9320,444,228,713,908,41134,77,24,1,66,39}}),
		QueueTestParameter({{79,74,245,11,89,800,78}}),
		QueueTestParameter({{780,274,71,34,79,90,12,444,135,1857}}),
		QueueTestParameter({{64895,7133,1923,0,8,73,14,56,279,14,74,11,442,12}})
	)
);
