/**
 * @file: static.cpp
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
 * @brief Метод настройки тестового окружения
 *
 */
TEST_F(BufferFixture, CreateBufferTest){
	// Если объект буфера создан
	ASSERT_TRUE(this->_buffer != nullptr);
	// Выполняем сброс объекта буфера
	this->_buffer.reset();
	// Проверяем что объект буфера удалён
	ASSERT_TRUE(this->_buffer == nullptr);
}

/**
 * @brief Метод пересоздания тестового окружения
 *
 */
TEST_F(BufferFixture, ResetAndCreateBufferTest){
	// Если объект буфера создан
	ASSERT_TRUE(this->_buffer != nullptr);
	// Выполняем сброс объекта буфера
	this->_buffer.reset();
	// Проверяем что объект буфера удалён
	ASSERT_TRUE(this->_buffer == nullptr);
	// Создаём объект буфера заново
	this->_buffer = std::make_unique <awh::buffer_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект буфера создан
	ASSERT_TRUE(this->_buffer != nullptr);
}

/**
 * @brief Метод пересоздания тестового окружения
 *
 */
TEST_F(BufferFixture, ReCreateBufferTest){
	// Если объект буфера создан
	ASSERT_TRUE(this->_buffer != nullptr);
	// Создаём объект буфера заново
	this->_buffer = std::make_unique <awh::buffer_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект буфера создан
	ASSERT_TRUE(this->_buffer != nullptr);
}

/**
 * @brief Метод копирования буфера
 *
 */
TEST_F(BufferFixture, CopyBufferTest){
	// Создаём первый буфер данных
	awh::buffer_t buffer1(this->_fmk.get(), this->_log.get());
	// Создаём второй буфер данных
	awh::buffer_t buffer2(this->_fmk.get(), this->_log.get());
	// В цикле добавляем данные в буфер
	for(auto & item : std::vector <size_t> ({888,9320,444,228,713,908,41134,77,24,1,66,39}))
		// Добавляем данные в буфер
		buffer1.push(&item, sizeof(item));
	// Копируем первый буфер во второй
	buffer2 = buffer1;
	// Сравниваем размеры буферов
	ASSERT_EQ(buffer1.size(), buffer2.size());
	// Сравниваем содержимое буферов
	ASSERT_TRUE(buffer1 == buffer2);
}

/**
 * @brief Метод перемещения буфера
 *
 */
TEST_F(BufferFixture, MoveBufferTest){
	// Создаём первый буфер данных
	awh::buffer_t buffer1(this->_fmk.get(), this->_log.get());
	// Создаём второй буфер данных
	awh::buffer_t buffer2(this->_fmk.get(), this->_log.get());
	// Заполняем первый буфер данными
	std::vector <size_t> data = {888,9320,444,228,713,908,41134,77,24,1,66,39};
	// В цикле добавляем данные в буфер
	for(auto & item : data)
		// Добавляем данные в буфер
		buffer1.push(&item, sizeof(item));
	// Перемещаем первый буфер во второй
	buffer2 = std::move(buffer1);
	// Проверяем что первый буфер пустой
	ASSERT_EQ(buffer1.size(), 0);
	// Проверяем что во втором буфере корректные данные
	ASSERT_EQ(buffer2.count <size_t> (), data.size());
	// В цикле сравниваем данные второго буфера с исходными данными
	for(size_t i = 0; i < buffer2.count <size_t> (); i++)
		// Сравниваем данные
		ASSERT_EQ(buffer2.at <size_t> (i), data.at(i));
}

/**
 * @brief Метод индексации буфера
 *
 */
TEST_F(BufferFixture, IndexesBufferTest){
	// Создаём первый буфер данных
	awh::buffer_t buffer1(this->_fmk.get(), this->_log.get());
	// Создаём второй буфер данных
	awh::buffer_t buffer2(this->_fmk.get(), this->_log.get());
	// В цикле добавляем данные в буфер
	for(auto & item : std::vector <size_t> ({888,9320,444,228,713,908,41134,77,24,1,66,39}))
		// Добавляем данные в буфер
		buffer1.push(item);
	// Проверяем количество элементов в буфере
	ASSERT_EQ(buffer1.count <size_t> (), 12);
	// В цикле копируем данные из первого буфера во второй по индексам
	for(size_t i = 0; i < buffer1.size(); i++){
		// Извлекаем данные по индексу
		uint8_t data = buffer1.at <uint8_t> (i);
		// Добавляем данные во второй буфер
		buffer2.push(&data, sizeof(data));
	}
	// Сравниваем размеры буферов
	ASSERT_TRUE(buffer1 == buffer2);
	// Сравниваем первые элементы буфера
	ASSERT_EQ(buffer1.front <size_t> (), buffer2.front <size_t> ());
	// Сравниваем последние элементы буфера
	ASSERT_EQ(buffer1.back <size_t> (), buffer2.back <size_t> ());
	/**
	 * Перебираем буфер пока не достигнем конца
	 */
	while(!buffer1.empty())
		// Удаляем первый элемент буфера
		buffer1.pop <size_t> ();
	// Проверяем что буфер пустой
	ASSERT_TRUE(buffer1.empty());
	// Устанавливаем значение по индексу
	buffer2.set <size_t> (555, 3);
	// Проверяем что значение установлено корректно
	ASSERT_EQ(buffer2.at <size_t> (3), 555);
}
