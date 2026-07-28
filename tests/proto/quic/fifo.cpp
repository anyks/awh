/**
 * @file: fifo.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты сегментированного FIFO-буфера собранных данных потока
 *        (awh::quic::chunked_fifo_t) — дозапись через границы блоков, слив и
 *        отбрасывание с возвратом блоков в пул, переиспользование блоков между
 *        буферами и сверка содержимого с эталонной непрерывной строкой
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/quic/connection.hpp"

/**
 * @brief Внутренние средства теста
 *
 */
namespace {
	/**
	 * @brief Функция генерации детерминированной строки заданной длины
	 *
	 * @param count размер строки в октетах
	 * @param base  базовый символ узора
	 * @return      сгенерированная строка
	 *
	 */
	std::string pattern(const size_t count, const char base) noexcept {
		// Результирующая строка
		std::string result;
		// Резервируем ёмкость под узор
		result.reserve(count);
		// Заполняем строку циклическим узором
		for(size_t i = 0; i < count; i++)
			// Дописываем очередной символ узора
			result.push_back(static_cast <char> (base + static_cast <char> (i % 26)));
		// Выводим сгенерированную строку
		return result;
	}
};

/**
 * @brief Тест исходного состояния: пустой буфер
 *
 */
TEST(QuicChunkedFifo, EmptyState){
	// Пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Создаём буфер собранных данных потока
	awh::quic::chunked_fifo_t fifo;
	// Проверяем исходную пустоту буфера
	EXPECT_TRUE(fifo.empty());
	EXPECT_EQ(fifo.size(), 0u);
	// Слив пустого буфера сохраняет содержимое выходного буфера
	std::string output = "prefix";
	fifo.drain(pool, output);
	EXPECT_EQ(output, "prefix");
	EXPECT_TRUE(fifo.empty());
}

/**
 * @brief Тест дозаписи в пределах одного блока и слива
 *
 */
TEST(QuicChunkedFifo, AppendWithinBlock){
	// Пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Создаём буфер собранных данных потока
	awh::quic::chunked_fifo_t fifo;
	// Эталонная строка
	const std::string data = pattern(100, 'A');
	// Дописываем данные в буфер
	fifo.append(pool, data);
	// Проверяем размер накопленных данных
	EXPECT_FALSE(fifo.empty());
	EXPECT_EQ(fifo.size(), data.size());
	// Сливаем данные в выходной буфер
	std::string output;
	fifo.drain(pool, output);
	// Слитые данные совпадают с эталоном
	EXPECT_EQ(output, data);
	EXPECT_TRUE(fifo.empty());
}

/**
 * @brief Тест дозаписи через границы блоков (несколько блоков)
 *
 */
TEST(QuicChunkedFifo, AppendAcrossBlocks){
	// Пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Создаём буфер собранных данных потока
	awh::quic::chunked_fifo_t fifo;
	// Эталонная строка размером в 3.5 блока (форсирует разбиение по границам)
	const std::string data = pattern(awh::quic::chunked_fifo_t::BLOCK * 3 + 777, 'a');
	// Дописываем данные несколькими кусками, пересекающими границы блоков
	std::string_view view = data;
	while(!view.empty()){
		// Размер очередного куска (не кратен размеру блока)
		const size_t count = ((view.size() < 5000) ? view.size() : 5000);
		// Дописываем очередной кусок
		fifo.append(pool, view.substr(0, count));
		// Отбрасываем записанную часть
		view.remove_prefix(count);
	}
	// Размер совпадает с эталоном
	EXPECT_EQ(fifo.size(), data.size());
	// Сливаем данные в выходной буфер
	std::string output;
	fifo.drain(pool, output);
	// Слитые данные посимвольно совпадают с эталоном
	EXPECT_EQ(output, data);
}

/**
 * @brief Тест возврата блоков в пул и их переиспользования
 *
 */
TEST(QuicChunkedFifo, DrainReturnsBlocksToPool){
	// Пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Создаём буфер собранных данных потока
	awh::quic::chunked_fifo_t fifo;
	// Первый цикл: наполняем на несколько блоков и сливаем - блоки уходят в пул
	fifo.append(pool, pattern(awh::quic::chunked_fifo_t::BLOCK * 2 + 10, 'x'));
	std::string first;
	fifo.drain(pool, first);
	// После слива пул удерживает освобождённые блоки
	const size_t retained = pool.size();
	EXPECT_GT(retained, 0u);
	// Второй цикл того же объёма: должен переиспользовать блоки, не увеличивая пул сверх удержанного
	fifo.append(pool, pattern(awh::quic::chunked_fifo_t::BLOCK * 2 + 10, 'y'));
	// Пока данные в буфере - взятые блоки изъяты из пула
	EXPECT_LT(pool.size(), retained);
	std::string second;
	fifo.drain(pool, second);
	// Пул вернулся к прежнему числу удержанных блоков (переиспользование, без роста)
	EXPECT_EQ(pool.size(), retained);
}

/**
 * @brief Тест отбрасывания данных с возвратом блоков в пул
 *
 */
TEST(QuicChunkedFifo, ClearDiscardsAndReuses){
	// Пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Создаём буфер собранных данных потока
	awh::quic::chunked_fifo_t fifo;
	// Наполняем буфер и отбрасываем данные без выдачи
	fifo.append(pool, pattern(awh::quic::chunked_fifo_t::BLOCK + 5, 'q'));
	fifo.clear(pool);
	// После отбрасывания буфер пуст
	EXPECT_TRUE(fifo.empty());
	EXPECT_EQ(fifo.size(), 0u);
	// Повторное использование того же объекта даёт корректный результат
	const std::string data = pattern(300, 'r');
	fifo.append(pool, data);
	std::string output;
	fifo.drain(pool, output);
	EXPECT_EQ(output, data);
}

/**
 * @brief Тест общего пула для множества одновременно живых буферов
 *
 */
TEST(QuicChunkedFifo, SharedPoolManyBuffers){
	// Общий пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Множество одновременно живущих буферов (конкурентная буферизация)
	std::vector <awh::quic::chunked_fifo_t> buffers(40);
	// Эталонные строки по каждому буферу
	std::vector <std::string> oracles(40);
	// Несколько раундов дозаписи во все буферы
	for(int round = 0; round < 20; round++){
		// Дозаписываем в каждый буфер
		for(size_t i = 0; i < buffers.size(); i++){
			// Очередной кусок разного размера, пересекающий границы блоков
			const std::string piece = pattern((i * 37 + static_cast <size_t> (round) * 101) % 9000, static_cast <char> ('a' + static_cast <int> (i % 20)));
			// Дозаписываем кусок в буфер
			buffers[i].append(pool, piece);
			// Дозаписываем кусок в эталон
			oracles[i].append(piece);
		}
	}
	// Сливаем все буферы и сверяем с эталонами
	for(size_t i = 0; i < buffers.size(); i++){
		// Проверяем размер до слива
		EXPECT_EQ(buffers[i].size(), oracles[i].size());
		// Сливаем данные буфера
		std::string output;
		buffers[i].drain(pool, output);
		// Слитые данные совпадают с эталоном
		EXPECT_EQ(output, oracles[i]);
	}
}

/**
 * @brief Тест сверки с эталоном на псевдослучайной последовательности дозаписей
 *
 */
TEST(QuicChunkedFifo, OracleRandomizedAppendDrain){
	// Пул переиспользуемых блоков
	std::vector <std::string> pool;
	// Множество прогонов с разным числом и размером кусков
	for(int trial = 0; trial < 300; trial++){
		// Буфер собранных данных потока
		awh::quic::chunked_fifo_t fifo;
		// Эталонная строка
		std::string oracle;
		// Число кусков прогона
		const size_t chunks = static_cast <size_t> (1 + (trial % 50));
		// Дозаписываем куски
		for(size_t c = 0; c < chunks; c++){
			// Размер куска 0..20099, пересекает размер блока
			const size_t length = (c * 131 + static_cast <size_t> (trial) * 977) % 20100;
			// Генерируем кусок
			const std::string piece = pattern(length, static_cast <char> ('A' + static_cast <int> (c % 8)));
			// Дозаписываем кусок в буфер и эталон
			fifo.append(pool, piece);
			oracle.append(piece);
			// Размер синхронен с эталоном на каждом шаге
			EXPECT_EQ(fifo.size(), oracle.size());
		}
		// Сливаем и сверяем
		std::string output;
		fifo.drain(pool, output);
		EXPECT_EQ(output, oracle);
	}
}
