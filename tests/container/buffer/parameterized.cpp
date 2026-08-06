/**
 * @file: parameterized.cpp
 * @date: 2025-12-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты бинарного смарт-буфера — прогон подготовленных наборов входных данных через методы
 *        модуля с проверкой добавления и извлечения записей, обхода итераторами и транзакционной записи с откатом
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
	/**
	 * Переход по всем данным для добавления в буфер
	 */
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
	std::vector <uint8_t> tempBuffer1, tempBuffer2;
	/**
	 * Переход по всем данным для проверки индексов буфера
	 */
	for(auto & item : this->_buffer->as <char>())
		// Добавляем данные во временный буфер
		tempBuffer1.push_back(item);
	/**
	 * Переход по всем данным переданным для добавления в буфер
	 */
	for(auto & item : this->_parameter.items)
		/**
		 * Переход по всем символам строки
		 */
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
	ASSERT_EQ(static_cast <const uint8_t *> (* this->_buffer.get()), static_cast <const std::vector <uint8_t> &> (* this->_buffer.get()).data() + static_cast <size_t> (* this->_buffer.get()));
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

/**
 * @brief Структура параметров теста буфера с фреймами разного размера
 *
 */
struct FrameTestParameter {
	// Список размеров фреймов
	std::vector <size_t> frames;
};

/**
 * @brief Класс фикстуры теста буфера с фреймами разного размера
 *
 */
class FrameBufferParameterizedFixture : public BufferFixture, public ::testing::WithParamInterface <FrameTestParameter> {
	public:
		// Параметры теста буфера
		FrameTestParameter _parameter = GetParam();
	public:
		/**
		 * @brief Метод генерации детерминированных данных заданного размера
		 *
		 * @param size размер генерируемых данных
		 * @param seed начальное значение для генерации
		 * @return     сгенерированные данные
		 *
		 */
		std::string generate(const size_t size, const size_t seed) const noexcept {
			// Результирующие данные
			std::string result(size, '\0');
			/**
			 * Заполняем данные детерминированными значениями
			 */
			for(size_t i = 0; i < size; i++)
				// Устанавливаем значение байта
				result[i] = static_cast <char> ((seed + i) & 0xFF);
			// Возвращаем сгенерированные данные
			return result;
		}
};

/**
 * @brief Метод тестирования потокового извлечения фреймов разного размера (сценарий Websocket)
 *
 */
TEST_P(FrameBufferParameterizedFixture, StreamFramesTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Список ожидаемых фреймов
	std::vector <std::string> expected;
	/**
	 * Накапливаем все фреймы в буфере
	 */
	for(size_t i = 0; i < this->_parameter.frames.size(); i++){
		// Получаем размер текущего фрейма
		const size_t size = this->_parameter.frames.at(i);
		// Если фрейм пустой
		if(size == 0)
			// Пропускаем пустой фрейм
			continue;
		// Генерируем данные фрейма
		const std::string frame = this->generate(size, i);
		// Добавляем фрейм в буфер
		ASSERT_TRUE(buffer.push(frame));
		// Запоминаем ожидаемый фрейм
		expected.push_back(frame);
	}
	/**
	 * Извлекаем фреймы в порядке поступления (FIFO) с сохранением хвоста
	 */
	for(auto & frame : expected){
		// Проверяем что в буфере достаточно данных
		ASSERT_TRUE(buffer.size() >= frame.size());
		// Проверяем что начало буфера содержит ожидаемый фрейм
		ASSERT_EQ(::memcmp(buffer.data(), frame.data(), frame.size()), 0);
		// Извлекаем обработанный фрейм
		buffer.consume(frame.size());
	}
	// Проверяем что буфер опустел
	ASSERT_TRUE(buffer.empty());
}

/**
 * @brief Метод тестирования zero-copy записи фреймов разного размера через RAII-обёртку
 *
 */
TEST_P(FrameBufferParameterizedFixture, ZeroCopyWriteFramesTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Ожидаемое итоговое содержимое буфера
	std::string expected;
	/**
	 * Записываем все фреймы напрямую через RAII-обёртку
	 */
	for(size_t i = 0; i < this->_parameter.frames.size(); i++){
		// Получаем размер текущего фрейма
		const size_t size = this->_parameter.frames.at(i);
		// Если фрейм пустой
		if(size == 0)
			// Пропускаем пустой фрейм
			continue;
		// Генерируем данные фрейма
		const std::string frame = this->generate(size, i);
		// Получаем объект записи
		awh::buffer_t::Writer writer = buffer.write(size);
		// Проверяем корректность резервирования
		ASSERT_TRUE(writer.valid());
		// Проверяем что зарезервировано достаточно места
		ASSERT_TRUE(writer.size() >= size);
		// Пишем данные напрямую в зарезервированную область
		::memcpy(writer.get(), frame.data(), size);
		// Фиксируем записанные данные
		ASSERT_EQ(writer.commit(size), size);
		// Дополняем ожидаемое содержимое
		expected.append(frame);
	}
	// Проверяем итоговый размер буфера
	ASSERT_EQ(buffer.size(), expected.size());
	// Если в буфере есть данные
	if(!expected.empty())
		// Проверяем итоговое содержимое буфера
		ASSERT_EQ(::memcmp(buffer.data(), expected.data(), expected.size()), 0);
}

/**
 * @brief Метод тестирования накопления фреймов из произвольных кусков (имитация чтения из сокета)
 *
 */
TEST_P(FrameBufferParameterizedFixture, ChunkedPrepareCommitTest){
	// Создаём буфер данных
	awh::buffer_t buffer(this->_fmk.get(), this->_log.get());
	// Собираем все данные в одну строку
	std::string source;
	/**
	 * Формируем исходные данные из всех фреймов
	 */
	for(size_t i = 0; i < this->_parameter.frames.size(); i++)
		// Дополняем исходные данные данными фрейма
		source.append(this->generate(this->_parameter.frames.at(i), i));
	// Если исходных данных нет
	if(source.empty())
		// Завершаем тест
		return;
	// Текущее смещение в исходных данных
	size_t offset = 0;
	// Размер куска для чтения
	const size_t chunk = 7;
	/**
	 * Имитируем чтение исходных данных кусками произвольного размера
	 */
	while(offset < source.size()){
		// Определяем размер текущего куска
		const size_t size = (((source.size() - offset) < chunk) ? (source.size() - offset) : chunk);
		// Резервируем место в хвосте буфера
		void * area = buffer.prepare(size);
		// Проверяем что место зарезервировано
		ASSERT_TRUE(area != nullptr);
		// Пишем кусок данных напрямую в буфер
		::memcpy(area, source.data() + offset, size);
		// Фиксируем записанные данные
		ASSERT_EQ(buffer.commit(size), size);
		// Смещаем позицию в исходных данных
		offset += size;
	}
	// Проверяем что накоплены все данные
	ASSERT_EQ(buffer.size(), source.size());
	// Проверяем что накопленные данные совпадают с исходными
	ASSERT_EQ(::memcmp(buffer.data(), source.data(), source.size()), 0);
}

/**
 * @brief Инициализация параметров теста буфера с фреймами разного размера
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FrameBufferParameterizedFixture,
	::testing::Values(
		FrameTestParameter({{1, 2, 3, 4, 5}}),
		FrameTestParameter({{10, 100, 1000}}),
		FrameTestParameter({{4096}}),
		FrameTestParameter({{1, 4095, 1, 8192, 13}}),
		FrameTestParameter({{512, 512, 512, 512}}),
		FrameTestParameter({{65536, 1, 65536}})
	)
);
