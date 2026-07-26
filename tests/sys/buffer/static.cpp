/**
 * @file: static.cpp
 * @date: 2025-12-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты бинарного смарт-буфера — проверка создания и сброса объекта модуля,
 *        а также корректности добавления и извлечения записей, обхода итераторами и транзакционной записи с откатом
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

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
	/**
	 * В цикле добавляем данные в буфер
	 */
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
	/**
	 * В цикле добавляем данные в буфер
	 */
	for(auto & item : data)
		// Добавляем данные в буфер
		buffer1.push(&item, sizeof(item));
	// Перемещаем первый буфер во второй
	buffer2 = std::move(buffer1);
	// Проверяем что первый буфер пустой
	ASSERT_EQ(buffer1.size(), 0);
	// Проверяем что во втором буфере корректные данные
	ASSERT_EQ(buffer2.count <size_t> (), data.size());
	/**
	 * В цикле сравниваем данные второго буфера с исходными данными
	 */
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
	/**
	 * В цикле добавляем данные в буфер
	 */
	for(auto & item : std::vector <size_t> ({888,9320,444,228,713,908,41134,77,24,1,66,39}))
		// Добавляем данные в буфер
		buffer1.push(item);
	// Проверяем количество элементов в буфере
	ASSERT_EQ(buffer1.count <size_t> (), 12);
	/**
	 * В цикле копируем данные из первого буфера во второй по индексам
	 */
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

/**
 * @brief Метод проверки zero-copy записи через prepare/commit
 *
 */
TEST_F(BufferFixture, ZeroCopyPrepareCommitBufferTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Данные для записи
	const std::string text = "Hello Zero-Copy World";
	// Резервируем место в хвосте буфера
	void * area = buffer.prepare(text.size());
	// Проверяем что место зарезервировано
	ASSERT_TRUE(area != nullptr);
	// Пишем данные напрямую в зарезервированную область
	::memcpy(area, text.data(), text.size());
	// Фиксируем записанные данные
	const size_t committed = buffer.commit(text.size());
	// Проверяем что зафиксировано столько же сколько записано
	ASSERT_EQ(committed, text.size());
	// Проверяем размер буфера
	ASSERT_EQ(buffer.size(), text.size());
	// Проверяем содержимое буфера
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), text);
	// Проверяем что commit ограничен доступным местом хвоста
	void * area2 = buffer.prepare(4);
	// Проверяем что место зарезервировано
	ASSERT_TRUE(area2 != nullptr);
	// Пытаемся зафиксировать больше чем зарезервировано
	const size_t committed2 = buffer.commit(1000);
	// Проверяем что фиксация ограничена доступным местом
	ASSERT_TRUE(committed2 <= buffer.capacity());
}

/**
 * @brief Метод проверки RAII-обёртки записи в буфер
 *
 */
TEST_F(BufferFixture, WriterGuardBufferTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Данные для записи
	const std::string text = "Framed payload";
	/**
	 * Записываем данные через RAII-обёртку с автоматической фиксацией
	 */
	{
		// Получаем объект записи
		awh::buffer_t::Writer writer = buffer.write(text.size());
		// Проверяем корректность резервирования
		ASSERT_TRUE(writer.valid());
		// Проверяем что зарезервировано достаточно места
		ASSERT_TRUE(writer.size() >= text.size());
		// Пишем данные напрямую в зарезервированную область
		::memcpy(writer.get(), text.data(), text.size());
		// Указываем количество записанных данных
		writer.commit(text.size());
	}
	// Проверяем что данные зафиксированы автоматически
	ASSERT_EQ(buffer.size(), text.size());
	// Проверяем содержимое буфера
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), text);
	/**
	 * Записываем данные но отменяем фиксацию
	 */
	{
		// Получаем объект записи
		awh::buffer_t::Writer writer = buffer.write(8);
		// Проверяем корректность резервирования
		ASSERT_TRUE(writer.valid());
		// Пишем данные напрямую в зарезервированную область
		::memcpy(writer.get(), "ZZZZ", 4);
		// Отменяем фиксацию данных
		writer.cancel();
	}
	// Проверяем что размер буфера не изменился
	ASSERT_EQ(buffer.size(), text.size());
	// Проверяем что содержимое буфера не изменилось
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), text);
}

/**
 * @brief Метод проверки потокового извлечения данных (сценарий Websocket)
 *
 */
TEST_F(BufferFixture, StreamConsumeBufferTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Добавляем полный фрейм и кусок следующего фрейма
	ASSERT_TRUE(buffer.push(std::string("FRAME1")));
	// Добавляем часть следующего фрейма
	ASSERT_TRUE(buffer.push(std::string("PARTIAL")));
	// Проверяем общий размер данных
	ASSERT_EQ(buffer.size(), 13u);
	// Извлекаем первый полный фрейм
	buffer.consume(6);
	// Проверяем что остался только недокопленный хвост
	ASSERT_EQ(buffer.size(), 7u);
	// Проверяем содержимое хвоста
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), "PARTIAL");
	// Дописываем остаток второго фрейма
	ASSERT_TRUE(buffer.push(std::string("END")));
	// Проверяем что хвост дополнился
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), "PARTIALEND");
	// Извлекаем оставшиеся данные
	buffer.consume(buffer.size());
	// Проверяем что буфер пустой
	ASSERT_TRUE(buffer.empty());
}

/**
 * @brief Метод проверки защиты по лимиту памяти без потери данных
 *
 */
TEST_F(BufferFixture, OverflowLimitBufferTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Устанавливаем максимальный размер потребления памяти
	buffer.setMaxMemory(16);
	// Добавляем данные в пределах лимита
	ASSERT_TRUE(buffer.push(std::string("0123456789")));
	// Проверяем размер буфера
	ASSERT_EQ(buffer.size(), 10u);
	// Пытаемся добавить данные сверх лимита (10 + 10 > 16)
	ASSERT_FALSE(buffer.push(std::string("ABCDEFGHIJ")));
	// Проверяем что данные не потеряны
	ASSERT_EQ(buffer.size(), 10u);
	// Проверяем что содержимое не повреждено
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), "0123456789");
	// Извлекаем часть данных
	buffer.consume(4);
	// Проверяем граничный случай (6 + 10 == 16)
	ASSERT_TRUE(buffer.push(std::string("ABCDEFGHIJ")));
	// Проверяем итоговый размер буфера
	ASSERT_EQ(buffer.size(), 16u);
	// Проверяем итоговое содержимое буфера
	ASSERT_EQ(std::string(static_cast <const char *> (buffer.data()), buffer.size()), "456789ABCDEFGHIJ");
}

/**
 * @brief Метод проверки переиспользования памяти при потоковой работе
 *
 */
TEST_F(BufferFixture, CompactionReuseBufferTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Устанавливаем максимальный размер потребления памяти
	buffer.setMaxMemory(64);
	/**
	 * Многократно добавляем и извлекаем данные
	 */
	for(size_t i = 0; i < 1000; i++){
		// Добавляем порцию данных
		ASSERT_TRUE(buffer.push(std::string("0123456789")));
		// Извлекаем добавленные данные
		buffer.consume(10);
		// Проверяем что буфер опустел
		ASSERT_TRUE(buffer.empty());
	}
	// Проверяем что объём выделенной памяти не вырос безгранично
	ASSERT_TRUE(buffer.capacity() <= 64u);
}

/**
 * @brief Метод проверки корректности front/back после извлечения данных
 *
 */
TEST_F(BufferFixture, BackFrontAfterEraseBufferTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	/**
	 * Добавляем элементы в буфер
	 */
	for(uint16_t value : std::vector <uint16_t> ({10, 20, 30, 40}))
		// Добавляем элемент в буфер
		ASSERT_TRUE(buffer.push(value));
	// Проверяем количество элементов
	ASSERT_EQ(buffer.count <uint16_t> (), 4u);
	// Проверяем первый и последний элементы
	ASSERT_EQ(buffer.front <uint16_t> (), 10);
	// Проверяем последний элемент
	ASSERT_EQ(buffer.back <uint16_t> (), 40);
	// Удаляем первый элемент (смещаем начало буфера)
	buffer.pop <uint16_t> ();
	// Проверяем количество элементов
	ASSERT_EQ(buffer.count <uint16_t> (), 3u);
	// Проверяем что первый элемент сместился
	ASSERT_EQ(buffer.front <uint16_t> (), 20);
	// Проверяем что последний элемент корректен при смещённом начале буфера
	ASSERT_EQ(buffer.back <uint16_t> (), 40);
	// Удаляем ещё один элемент
	buffer.pop <uint16_t> ();
	// Проверяем что первый элемент сместился
	ASSERT_EQ(buffer.front <uint16_t> (), 30);
	// Проверяем что последний элемент по-прежнему корректен
	ASSERT_EQ(buffer.back <uint16_t> (), 40);
}

/**
 * @brief Метод проверки обмена содержимым буферов
 *
 */
TEST_F(BufferFixture, SwapContentBufferTest){
	// Создаём первый буфер данных
	awh::buffer_t buffer1(this->_fmk.get(), this->_log.get());
	// Создаём второй буфер данных
	awh::buffer_t buffer2(this->_fmk.get(), this->_log.get());
	// Заполняем первый буфер данными
	ASSERT_TRUE(buffer1.push(std::string("AAA")));
	// Заполняем второй буфер данными
	ASSERT_TRUE(buffer2.push(std::string("BBBBB")));
	// Выполняем обмен содержимым буферов
	buffer1.swap(buffer2);
	// Проверяем содержимое первого буфера
	ASSERT_EQ(std::string(static_cast <const char *> (buffer1.data()), buffer1.size()), "BBBBB");
	// Проверяем содержимое второго буфера
	ASSERT_EQ(std::string(static_cast <const char *> (buffer2.data()), buffer2.size()), "AAA");
}

/**
 * @brief Метод проверки добавления буфера через перемещение
 *
 */
TEST_F(BufferFixture, PushMoveAppendBufferTest){
	// Создаём первый буфер данных
	awh::buffer_t buffer1(this->_fmk.get(), this->_log.get());
	// Создаём второй буфер данных
	awh::buffer_t buffer2(this->_fmk.get(), this->_log.get());
	// Заполняем первый буфер данными
	ASSERT_TRUE(buffer1.push(std::string("HEAD")));
	// Заполняем второй буфер данными
	ASSERT_TRUE(buffer2.push(std::string("TAIL")));
	// Добавляем второй буфер в первый через перемещение
	ASSERT_TRUE(buffer1.push(std::move(buffer2)));
	// Проверяем что данные были дописаны, а не заменены
	ASSERT_EQ(std::string(static_cast <const char *> (buffer1.data()), buffer1.size()), "HEADTAIL");
}
