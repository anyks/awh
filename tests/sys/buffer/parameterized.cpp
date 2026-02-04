/**
 * @file: parameterized.cpp
 * @date: 2025-12-13
 * @license: GPL-3.0
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
#include "buffer.hpp"

/**
 * @brief Структура параметров теста буфера
 *
 */
struct BufferTestParameter {
	// Список данных для буфера
	std::vector <std::string> items;
};

/**
 * @brief Конструктор структуры параметров теста буфера
 *
 */
class BufferParameterizedFixture : public BufferFixture, public ::testing::WithParamInterface <BufferTestParameter> {
	public:
		// Параметры теста буфера
		BufferTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования буфера с параметрами
 *
 */
TEST_P(BufferParameterizedFixture, BufferTest){
	// Очищаем буфер перед тестированием
	this->_buffer->clear();
	// Выполняем сброс буфера
	this->_buffer->reset();
	// Резервируем память для буфера
	this->_buffer->reserve(1024);
	// Устанавливаем максимальный размер потребляемой памяти
	this->_buffer->setMaxMemory(1024);
	// Устанавливаем объект логов
	this->_buffer->setLogger(this->_log.get());
	// Переход по всем данным для добавления в буфер
	for(auto & item : this->_parameter.items)
		// Добавляем данные в буфер
		this->_buffer->push(item);
	// Проверяем что буфер не пустой
	ASSERT_FALSE(this->_buffer->empty());
	// Проверяем что сырые данные буфера не пустые
	ASSERT_FALSE(this->_buffer->raw().empty());
	// Проверяем что размер буфера больше нуля
	ASSERT_TRUE(this->_buffer->size() > 0);
	// Проверяем что размер занимаемой памяти буфером больше нуля
	ASSERT_TRUE(this->_buffer->capacity() > 0);
	// Проверяем совпадание последних элементов буфера
	ASSERT_EQ(this->_parameter.items.back().back(), this->_buffer->back <char> ());
	// Проверяем совпадание первых элементов буфера
	ASSERT_EQ(this->_parameter.items.front().front(), this->_buffer->front <char> ());
	// Создаём временный буфер для проверки индексов
	std::vector <char> tempBuffer1, tempBuffer2;
	// Переход по всем данным для проверки индексов буфера
	for(auto & item : this->_buffer->as <char>())
		// Добавляем данные во временный буфер
		tempBuffer1.push_back(item);
	// Переход по всем данным переданным для добавления в буфер
	for(auto & item : this->_parameter.items)
		// Переход по всем символам строки
		for(auto & ch : item)
			// Добавляем символ во временный буфер
			tempBuffer2.push_back(ch);
	// Проверяем что данные по индексам совпадают с исходными данными
	ASSERT_EQ(::memcmp(tempBuffer1.data(), tempBuffer2.data(), tempBuffer1.size()), 0);
	/**
	 * Обходим буфер пока он не опустеет
	 */
	while(!this->_buffer->empty()){
		// Получаем размер первых данных
		const size_t size = this->_parameter.items.front().size();
		// Проверяем что извлечённые данные совпадают с исходными данными
		ASSERT_EQ(this->_parameter.items.front(), std::string(reinterpret_cast <const char *> (this->_buffer->data()), size));
		// Удаляем первые данные из исходных данных
		this->_parameter.items.erase(this->_parameter.items.begin());
		// Удаляем первые данные из буфера
		this->_buffer->erase(size);
	}
	// Если в параметрах теста остались данные
	if(!this->_parameter.items.empty())
		// Удаляем первые данные из буфера
		this->_buffer->erase(this->_parameter.items.front().size());
	// Проверяем что первые данные буфера совпадают с исходными данными
	ASSERT_EQ(static_cast <const char *> (* this->_buffer.get()), static_cast <const std::vector <char> &> (* this->_buffer.get()).data() + static_cast <size_t> (* this->_buffer.get()));
	// Инициализируем ещё одну копию буфера
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Переход по всем числам для добавления в буфер
	buffer.swap(* this->_buffer.get());
	// Выполняем сброс буфера
	this->_buffer->reset();
	// Очищаем буфер
	this->_buffer->clear();
}

/**
 * @brief Инициализация параметров теста буфера
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, BufferParameterizedFixture,
	::testing::Values(
		BufferTestParameter({{"Hello World", "Data", "Test", "Buffer", "AWK Framework"}}),
		BufferTestParameter({{"Goga", "Best", "Friend", "In", "The", "World"}}),
		BufferTestParameter({{"Another", "Set", "Of", "Strings", "For", "Testing"}}),
		BufferTestParameter({{"More", "Data", "To", "Test", "Buffer", "Functionality"}})
	)
);
