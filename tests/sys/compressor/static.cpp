/**
 * @file: static.cpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты подсистемы компрессии — проверка создания и сброса объекта модуля,
 *        а также корректности блочного и потокового сжатия и распаковки данных всеми поддерживаемыми алгоритмами
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "compressor.hpp"

/**
 * @brief Тест создания объекта компрессии
 *
 */
TEST_F(CompressorFixture, CreateCompressorTest){
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
	// Сбрасываем объект компрессии
	this->_compressor.reset();
	// Проверяем, что объект компрессии сброшен
	ASSERT_TRUE(this->_compressor == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта компрессии
 *
 */
TEST_F(CompressorFixture, ResetAndCreateCompressorTest){
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
	// Сбрасываем объект компрессии
	this->_compressor.reset();
	// Проверяем, что объект компрессии сброшен
	ASSERT_TRUE(this->_compressor == nullptr);
	// Создаём объект компрессии заново
	this->_compressor = std::make_unique <awh::compressor::block_t> (this->_log.get());
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
}

/**
 * @brief Тест повторного создания объекта компрессии
 *
 */
TEST_F(CompressorFixture, ReCreateCompressorTest){
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
	// Создаём объект компрессии заново
	this->_compressor = std::make_unique <awh::compressor::block_t> (this->_log.get());
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
}

/**
 * @brief Тест блочного цикла компрессии/декомпрессии всеми методами на всех уровнях компрессии
 *
 * @details Закрепляет таблицу уровней: ни одно сочетание метода и уровня не должно
 *          приводить к отказу движка на границах допустимых диапазонов параметров
 *
 */
TEST_F(CompressorFixture, RoundTripAllMethodsAllLevelsTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework compression round-trip payload, Anyks Framework compression round-trip payload!!!!!!!!?";
	// Список проверяемых методов компрессии
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::LIZARD,
		awh::compressor::method_t::SNAPPY,
		awh::compressor::method_t::DEFLATE,
		awh::compressor::method_t::DENSITY
	};
	// Список проверяемых уровней компрессии
	const awh::compressor::level_t levels[] = {
		awh::compressor::level_t::BEST,
		awh::compressor::level_t::SPEED,
		awh::compressor::level_t::NORMAL
	};
	/**
	 * Выполняем перебор всех уровней компрессии
	 */
	for(auto & level : levels){
		// Устанавливаем уровень компрессии
		this->_compressor->level(level);
		/**
		 * Выполняем перебор всех методов компрессии
		 */
		for(auto & method : methods){
			// Выполняем компрессию данных
			const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
			// Проверяем что результат компрессии не пустой
			ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method) << ", level = " << static_cast <uint16_t> (level);
			// Проверяем что результат декомпрессии совпадает с исходным текстом
			ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, method)) << "method = " << static_cast <uint16_t> (method) << ", level = " << static_cast <uint16_t> (level);
		}
	}
}

/**
 * @brief Тест влияния уровня компрессии на степень сжатия
 *
 * @details Закрепляет направление таблицы уровней: максимальный уровень не должен
 *          давать результат хуже, чем уровень максимальной производительности
 *
 */
TEST_F(CompressorFixture, LevelAffectsRatioTest){
	// Формируем текст для компрессии
	std::string text;
	// Наполняем текст повторяющимися данными, чтобы разница уровней была заметна
	for(uint16_t i = 0; i < 512; i++)
		// Добавляем очередную порцию данных
		text.append("Anyks Framework compression level payload ");
	// Список методов, у которых уровень компрессии влияет на степень сжатия
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::DEFLATE
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Устанавливаем максимальный уровень компрессии
		this->_compressor->level(awh::compressor::level_t::BEST);
		// Выполняем компрессию данных на максимальном уровне компрессии
		const size_t best = this->_compressor->compress <std::vector <uint8_t>> (text, method).size();
		// Устанавливаем уровень максимальной производительности
		this->_compressor->level(awh::compressor::level_t::SPEED);
		// Выполняем компрессию данных на уровне максимальной производительности
		const size_t speed = this->_compressor->compress <std::vector <uint8_t>> (text, method).size();
		// Проверяем что результат компрессии получен
		ASSERT_GT(best, 0u) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что максимальный уровень компрессии не хуже уровня максимальной производительности
		ASSERT_LE(best, speed) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест обработки пустого буфера данных
 *
 * @details Закрепляет намеренное решение: пустой вход не является ошибкой и даёт пустой результат
 *
 */
TEST_F(CompressorFixture, EmptyBufferTest){
	// Выполняем компрессию пустого буфера данных
	ASSERT_TRUE(this->_compressor->compress <std::vector <uint8_t>> (std::string_view(""), awh::compressor::method_t::GZIP).empty());
	// Выполняем декомпрессию пустого буфера данных
	ASSERT_TRUE(this->_compressor->decompress <std::string> (std::string_view(""), awh::compressor::method_t::GZIP).empty());
}

/**
 * @brief Тест обработки повреждённых данных
 *
 * @details Закрепляет отсутствие отказа и пустой результат на заведомо некорректном входе
 *
 */
TEST_F(CompressorFixture, CorruptedDataTest){
	// Формируем заведомо некорректный буфер данных
	const std::string corrupted(64, '\x7f');
	/**
	 * Список проверяемых методов компрессии. Движки с «сырым» самонепроверяемым форматом
	 * (LZ4, Lizard, Snappy, Density) сюда не входят: их формат не содержит ни заголовка,
	 * ни контрольной суммы, поэтому произвольный набор байт для них является допустимым входом
	 */
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods)
		// Проверяем что декомпрессия повреждённых данных даёт пустой результат
		ASSERT_TRUE(this->_compressor->decompress <std::string> (corrupted, method).empty()) << "method = " << static_cast <uint16_t> (method);
}

/**
 * @brief Тест контракта форматов блочного и потокового режимов
 *
 * @details Закрепляет намеренное решение: блочный режим LZ4 и Lizard выдаёт «сырой» блок,
 *          а потоковый — кадр, поэтому данные одного режима не разбираются другим.
 *          Для остальных движков форматы обоих режимов совместимы
 *
 */
TEST_F(CompressorFixture, BlockAndStreamFormatContractTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework block and stream format contract payload!!!!!!!!!!!!!!!!?";
	// Список методов, у которых форматы режимов совместимы
	const awh::compressor::method_t compatible[] = {
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::LZMA
	};
	/**
	 * Выполняем перебор методов с совместимыми форматами режимов
	 */
	for(auto & method : compatible){
		// Выполняем блочную компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t stream = this->_compressor->stream(method, awh::compressor::event_t::DECODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(stream.valid()) << "method = " << static_cast <uint16_t> (method);
		// Буфер результата декомпрессии
		std::string result;
		// Выполняем подачу блочно сжатых данных в потоковую сессию
		stream.push <std::string> (compressed.data(), compressed.size(), result, awh::compressor::flush_t::FINISH);
		// Проверяем что потоковая сессия разобрала блочный результат
		ASSERT_EQ(text, result) << "method = " << static_cast <uint16_t> (method);
	}
	// Список методов, у которых форматы режимов несовместимы
	const awh::compressor::method_t incompatible[] = {
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LIZARD
	};
	/**
	 * Выполняем перебор методов с несовместимыми форматами режимов
	 */
	for(auto & method : incompatible){
		// Выполняем блочную компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t stream = this->_compressor->stream(method, awh::compressor::event_t::DECODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(stream.valid()) << "method = " << static_cast <uint16_t> (method);
		// Буфер результата декомпрессии
		std::string result;
		// Выполняем подачу блочно сжатых данных в потоковую сессию
		stream.push <std::string> (compressed.data(), compressed.size(), result, awh::compressor::flush_t::FINISH);
		// Проверяем что потоковая сессия не разобрала блочный результат
		ASSERT_NE(text, result) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест перемещения потоковой сессии
 *
 * @details Закрепляет согласованность конструктора и оператора перемещения: источник
 *          после перемещения становится невалидным и теряет метод с направлением операции
 *
 */
TEST_F(CompressorFixture, StreamMoveResetsSourceTest){
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t source = this->_compressor->stream(awh::compressor::method_t::GZIP, awh::compressor::event_t::ENCODE);
	// Проверяем что потоковая сессия создана
	ASSERT_TRUE(source.valid());
	// Выполняем перемещение потоковой сессии конструктором перемещения
	awh::compressor::stream_t target(std::move(source));
	// Проверяем что перемещённая потоковая сессия валидна
	ASSERT_TRUE(target.valid());
	// Проверяем что метод компрессии перенесён
	ASSERT_EQ(awh::compressor::method_t::GZIP, target.method());
	// Проверяем что направление операции перенесено
	ASSERT_EQ(awh::compressor::event_t::ENCODE, target.event());
	// Проверяем что источник стал невалидным
	ASSERT_FALSE(source.valid());
	// Проверяем что метод компрессии у источника сброшен
	ASSERT_EQ(awh::compressor::method_t::NONE, source.method());
	// Проверяем что направление операции у источника сброшено
	ASSERT_EQ(awh::compressor::event_t::NONE, source.event());
}

/**
 * @brief Тест переиспользования контекста компрессии после неудачной пересборки
 *
 * @details Закрепляет восстановление после отказа инициализации: флаг переиспользования
 *          контекста сбрасывается вместе с разрушенным потоком, поэтому после возврата
 *          корректного размера скользящего окна компрессор снова работоспособен
 *
 */
TEST_F(CompressorFixture, TakeoverAfterFailedReinitTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework takeover reinit payload, Anyks Framework takeover reinit payload!!!!!!!!?";
	// Устанавливаем размер скользящего окна GZip
	ASSERT_TRUE(this->_compressor->wbitsGZip(15));
	// Включаем флаг переиспользования контекста компрессии
	ASSERT_TRUE(this->_compressor->takeoverGZip(awh::compressor::event_t::ENCODE, true));
	// Включаем флаг переиспользования контекста декомпрессии
	ASSERT_TRUE(this->_compressor->takeoverGZip(awh::compressor::event_t::DECODE, true));
	// Выполняем пересборку контекстов с заведомо некорректным размером скользящего окна
	ASSERT_FALSE(this->_compressor->wbitsGZip(2));
	// Возвращаем корректный размер скользящего окна
	ASSERT_TRUE(this->_compressor->wbitsGZip(15));
	// Выполняем компрессию данных
	const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE);
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Проверяем что результат декомпрессии совпадает с исходным текстом
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE));
}
