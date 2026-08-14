/**
 * @file parameterized.cpp
 * @date 2025-12-13
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
 * @brief Параметризованные тесты бинарной очереди —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой добавления,
 *        чтения и извлечения записей произвольного размера и контроля лимитов
 *
 * @copyright Copyright © 2025
 *
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
class QueueParameterizedFixture : public QueueFixture, public ::testing::WithParamInterface <QueueTestParameter> {
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
	// Устанавливаем максимальный размер потребляемой памяти
	this->_queue->setMaxMemory(1024);
	// Устанавливаем максимальное количество записей очереди
	this->_queue->setMaxRecords(1024);
	// Устанавливаем объект логов
	this->_queue->setLogger(this->_log.get());
	/**
	 * Переход по всем числам для добавления в очередь
	 */
	for(auto & number : this->_parameter.numbers)
		// Добавляем число в очередь
		this->_queue->push(&number, sizeof(number));
	// Проверяем что очередь не пустая
	ASSERT_FALSE(this->_queue->empty());
	// Проверяем что размер занимаемой памяти очередью больше нуля
	ASSERT_TRUE(this->_queue->capacity() > 0);
	// Проверяем что количество записей в очереди совпадает с добавленными числами
	ASSERT_EQ(this->_queue->count(), this->_parameter.numbers.size());
	// Индекс текущего числа
	size_t index = 0;
	/**
	 * Обходим очередь пока она не опустеет
	 */
	while(!this->_queue->empty()){
		// Извлекаем число из очереди
		size_t number = 0;
		// Копируем данные из очереди в число
		::memcpy(&number, this->_queue->data(), this->_queue->size());
		// Устанавливаем смещение чтения данных
		this->_queue->commit(this->_queue->size());
		// Удаляем запись из очереди
		this->_queue->pop();
		// Проверяем что извлечённое число совпадает с добавленным
		ASSERT_EQ(number, this->_parameter.numbers.at(index));
		// Увеличиваем индекс текущего числа
		index++;
	}
	// Инициализируем ещё одну очередь
	awh::queue_t queue(this->_fmk.get(), this->_log.get());
	// Переход по всем числам для добавления в очередь
	queue.swap(* this->_queue.get());
	// Выполняем сброс очереди
	this->_queue->reset();
	// Очищаем очередь
	this->_queue->clear();
	// Проверяем что очередь пустая
	ASSERT_TRUE(static_cast <size_t> (* this->_queue.get()) == 0);
	// Проверяем что размер занимаемой памяти очередью равен нулю
	ASSERT_TRUE(static_cast <const char *> (* this->_queue.get()) == nullptr);
	// Общий размер добавляемых данных
	size_t size = 0;
	// Создаём список записей для добавления в очередь
	std::vector <awh::queue_t::record_t> records;
	/**
	 * Переход по всем числам для добавления в очередь
	 */
	for(auto & number : this->_parameter.numbers){
		// Увеличиваем общий размер добавляемых данных
		size += sizeof(number);
		// Добавляем число в список записей
		records.emplace_back(reinterpret_cast <const void *> (&number), sizeof(number));
	}
	// Добавляем список записей в очередь
	this->_queue->push(records, size);
	/**
	 * Обходим очередь пока она не опустеет
	 */
	while(!this->_queue->empty()){
		// Создаём буфер данных для извлечения записи
		std::unique_ptr <uint8_t []> buffer(new uint8_t[this->_queue->size()]);
		// Копируем данные из очереди в число
		::memcpy(buffer.get(), this->_queue->data(), this->_queue->size());
		// Удаляем запись из очереди
		this->_queue->pop();
	}
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
