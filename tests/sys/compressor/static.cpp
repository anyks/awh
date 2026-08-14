/**
 * @file static.cpp
 * @date 2026-01-21
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
 * @brief Статические тесты подсистемы компрессии — проверка создания и сброса объекта модуля,
 *        а также корректности блочного и потокового сжатия и распаковки данных всеми поддерживаемыми алгоритмами
 *
 * @copyright Copyright © 2026
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
		awh::compressor::method_t::LZ4,
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
	/**
	 * Выполняем перебор методов с совместимыми форматами режимов в обратную сторону
	 */
	for(auto & method : compatible){
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t stream = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(stream.valid()) << "method = " << static_cast <uint16_t> (method);
		// Буфер выхода порции
		std::string part;
		// Результат потоковой компрессии
		std::string compressed;
		// Подаём данные в поток компрессии
		stream.push <std::string> (text.data(), text.size(), part);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		// Финализируем поток компрессии
		stream.finish(part);
		// Дописываем хвост в результат компрессии
		compressed.append(part);
		// Проверяем что блочный режим разобрал потоковый результат
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, method)) << "method = " << static_cast <uint16_t> (method);
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
	/**
	 * DEFLATE вынесен из общего перебора: форматы режимов у него совместимы, а вот
	 * признак завершённости расходится. Блочное сообщение завершается Z_SYNC_FLUSH
	 * по RFC 7692 и конца потока не несёт вовсе, поэтому потоковая сессия данные
	 * разбирает, но завершённой себя не объявляет — и это верно, движку конца
	 * потока никто не показывал
	 */
	{
		// Выполняем блочную компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty());
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t stream = this->_compressor->stream(awh::compressor::method_t::DEFLATE, awh::compressor::event_t::DECODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(stream.valid());
		// Буфер результата декомпрессии
		std::string result;
		// Выполняем подачу блочно сжатых данных в потоковую сессию
		stream.push <std::string> (compressed.data(), compressed.size(), result, awh::compressor::flush_t::FINISH);
		// Проверяем что потоковая сессия разобрала блочный результат
		ASSERT_EQ(text, result);
		// Проверяем что сессия жива
		ASSERT_TRUE(stream.valid());
		// Проверяем что завершённой сессия себя не объявила: конца потока в кадре нет
		ASSERT_FALSE(stream.done());
	}
	/**
	 * В обратную сторону расхождения нет: потоковое сжатие доводит кадр до конца
	 * потока, а блочная распаковка DEFLATE конца потока не требует и им не смущается
	 */
	{
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t stream = this->_compressor->stream(awh::compressor::method_t::DEFLATE, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(stream.valid());
		// Буфер выхода порции
		std::string part;
		// Результат потоковой компрессии
		std::string compressed;
		// Подаём данные в поток компрессии
		stream.push <std::string> (text.data(), text.size(), part);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		// Финализируем поток компрессии
		stream.finish(part);
		// Дописываем хвост в результат компрессии
		compressed.append(part);
		// Проверяем что блочный режим разобрал потоковый результат
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE));
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
 * @brief Тест переиспользования контекста компрессии после отвергнутой пересборки
 *
 * @details Закрепляет восстановление после отвергнутого размера скользящего окна:
 *          прежнее значение сохраняется, контексты остаются работоспособными
 *
 */
TEST_F(CompressorFixture, TakeoverAfterFailedReinitTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework takeover reinit payload, Anyks Framework takeover reinit payload!!!!!!!!?";
	// Устанавливаем размер скользящего окна Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	// Включаем флаг переиспользования контекста компрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::ENCODE, true));
	// Включаем флаг переиспользования контекста декомпрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::DECODE, true));
	// Выполняем пересборку контекстов с заведомо некорректным размером скользящего окна
	ASSERT_FALSE(this->_compressor->wbitsDeflate(2));
	// Возвращаем корректный размер скользящего окна
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	// Выполняем компрессию данных
	const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE);
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Проверяем что результат декомпрессии совпадает с исходным текстом
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE));
}

/**
 * @brief Тест обработки усечённого потока данных
 *
 * @details Закрепляет отказ на кадре, потерявшем хвост: движки с самоописательным
 *          форматом обязаны требовать конца потока, а не довольствоваться исчерпанием входа
 *
 */
TEST_F(CompressorFixture, TruncatedStreamTest){
	// Формируем текст для компрессии
	const std::string text(512, 'A');
	// Список методов, формат которых несёт признак конца потока
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::LZMA
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_GT(compressed.size(), 4u) << "method = " << static_cast <uint16_t> (method);
		/**
		 * Выполняем перебор количества отрезанных от кадра октетов
		 */
		for(size_t cut = 1; cut <= 3; cut++){
			// Формируем усечённый кадр данных
			const std::vector <uint8_t> truncated(compressed.begin(), compressed.end() - cut);
			// Проверяем что декомпрессия усечённого кадра даёт пустой результат
			ASSERT_TRUE(this->_compressor->decompress <std::string> (truncated, method).empty()) << "method = " << static_cast <uint16_t> (method) << ", cut = " << cut;
		}
	}
}

/**
 * @brief Тест блочного цикла компрессии/декомпрессии на крупном буфере данных
 *
 * @details Закрепляет работу за пределами одной внутренней порции: буфер заведомо больше
 *          чанка, которым драйверы обмениваются с движками, поэтому проверяются наращивание
 *          выходного буфера и склейка порций
 *
 */
TEST_F(CompressorFixture, LargeBufferRoundTripTest){
	// Формируем крупный буфер данных
	std::string text;
	// Резервируем память под буфер данных
	text.reserve(512 * 1024);
	/**
	 * Наполняем буфер данными переменной повторяемости, чтобы он не сжимался вырожденно
	 */
	for(uint32_t i = 0; text.size() < (512 * 1024); i++){
		// Добавляем очередную порцию данных
		text.append("Anyks Framework large payload block ");
		// Добавляем переменную часть порции данных
		text.append(std::to_string(i * 2654435761u));
	}
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
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что результат декомпрессии совпадает с исходным текстом
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, method)) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест влияния уровня компрессии на потоковый режим
 *
 * @details Закрепляет направление уровня в потоковом режиме отдельно от блочного: у LZ4
 *          кадровый формат трактует уровень компрессией, а блочный — ускорением, и общее
 *          на оба режима значение переворачивало бы смысл максимальной производительности
 *
 */
TEST_F(CompressorFixture, StreamLevelAffectsRatioTest){
	// Формируем текст для компрессии
	std::string text;
	// Наполняем текст повторяющимися данными, чтобы разница уровней была заметна
	for(uint16_t i = 0; i < 1024; i++)
		// Добавляем очередную порцию данных
		text.append("Anyks Framework stream compression level payload ");
	// Список методов, у которых уровень компрессии влияет на степень сжатия
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::LIZARD
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Размеры результатов компрессии на разных уровнях
		size_t size[2] = {0, 0};
		// Список проверяемых уровней компрессии
		const awh::compressor::level_t levels[2] = {awh::compressor::level_t::BEST, awh::compressor::level_t::SPEED};
		/**
		 * Выполняем перебор проверяемых уровней компрессии
		 */
		for(uint8_t i = 0; i < 2; i++){
			// Устанавливаем уровень компрессии
			this->_compressor->level(levels[i]);
			// Создаём потоковую сессию компрессии
			awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
			// Проверяем что потоковая сессия создана
			ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
			// Буфер выхода порции
			std::string part;
			// Подаём данные в поток компрессии
			encoder.push <std::string> (text.data(), text.size(), part);
			// Учитываем полученный выход
			size[i] += part.size();
			// Финализируем поток компрессии
			encoder.finish(part);
			// Учитываем хвост потока
			size[i] += part.size();
		}
		// Проверяем что результат компрессии получен
		ASSERT_GT(size[0], 0u) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что максимальный уровень компрессии не хуже уровня максимальной производительности
		ASSERT_LE(size[0], size[1]) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест обработки усечённого кадра на крупном слабосжимаемом буфере
 *
 * @details Дополняет TruncatedStreamTest: там кадр короткий и хорошо сжимаемый, и срез
 *          отрезает лишь хвост. Здесь кадр состоит из нескольких блоков, а срез приходится
 *          на его середину — движок при этом успевает выдать часть данных, и модуль обязан
 *          отличить это от успешного завершения
 *
 */
TEST_F(CompressorFixture, TruncatedLargeFrameTest){
	// Формируем крупный слабосжимаемый буфер данных
	std::string text;
	// Резервируем память под буфер данных
	text.reserve(256 * 1024);
	// Состояние генератора псевдослучайной последовательности
	uint64_t state = 0x9e3779b97f4a7c15ull;
	/**
	 * Наполняем буфер псевдослучайными октетами
	 */
	while(text.size() < (256 * 1024)){
		// Продвигаем состояние генератора
		state ^= (state << 13);
		state ^= (state >> 7);
		state ^= (state << 17);
		// Добавляем очередной октет
		text.push_back(static_cast <char> (state & 0xFF));
	}
	// Список методов, формат которых несёт признак конца потока
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::LZMA
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_GT(compressed.size(), 16u) << "method = " << static_cast <uint16_t> (method);
		// Доли кадра, на которых выполняется срез
		const size_t parts[] = {4, 2, 8};
		/**
		 * Выполняем перебор долей кадра
		 */
		for(auto & part : parts){
			// Формируем усечённый кадр данных
			const std::vector <uint8_t> truncated(compressed.begin(), compressed.begin() + ((compressed.size() * (part - 1)) / part));
			// Проверяем что декомпрессия усечённого кадра даёт пустой результат
			ASSERT_TRUE(this->_compressor->decompress <std::string> (truncated, method).empty()) << "method = " << static_cast <uint16_t> (method) << ", part = " << part;
		}
		// Проверяем что нетронутый кадр по-прежнему разбирается
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, method)) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест обработки входа короче служебной части формата
 *
 * @details Закрепляет отказ на буфере, который короче подвала контейнера LZMA:
 *          вычисление смещения подвала на таком входе уходило бы за границы буфера
 *
 */
TEST_F(CompressorFixture, ShortInputTest){
	/**
	 * Выполняем перебор размеров заведомо короткого входа
	 */
	for(size_t size = 1; size <= 16; size++){
		// Формируем заведомо короткий буфер данных
		const std::string data(size, '\x01');
		// Проверяем что декомпрессия короткого входа даёт пустой результат
		ASSERT_TRUE(this->_compressor->decompress <std::string> (data, awh::compressor::method_t::LZMA).empty()) << "size = " << size;
		// Проверяем что декомпрессия короткого входа даёт пустой результат
		ASSERT_TRUE(this->_compressor->decompress <std::string> (data, awh::compressor::method_t::GZIP).empty()) << "size = " << size;
		// Проверяем что декомпрессия короткого входа даёт пустой результат
		ASSERT_TRUE(this->_compressor->decompress <std::string> (data, awh::compressor::method_t::ZLIB).empty()) << "size = " << size;
		// Проверяем что декомпрессия короткого входа даёт пустой результат
		ASSERT_TRUE(this->_compressor->decompress <std::string> (data, awh::compressor::method_t::BZIP2).empty()) << "size = " << size;
		// Проверяем что декомпрессия короткого входа даёт пустой результат
		ASSERT_TRUE(this->_compressor->decompress <std::string> (data, awh::compressor::method_t::ZSTD).empty()) << "size = " << size;
	}
}

/**
 * @brief Тест применения уровня компрессии к живому переиспользуемому контексту
 *
 * @details Закрепляет согласование таблицы уровней с контекстом, заведённым заранее:
 *          уровень задаётся контексту при его создании, и смена таблицы обязана
 *          доходить до живого контекста, а не ждать его пересоздания
 *
 */
TEST_F(CompressorFixture, LevelAppliesToLiveTakeoverTest){
	// Формируем текст для компрессии
	std::string text;
	// Наполняем текст повторяющимися данными, чтобы разница уровней была заметна
	for(uint16_t i = 0; i < 512; i++)
		// Добавляем очередную порцию данных
		text.append("Anyks Framework takeover level payload ");
	// Устанавливаем размер скользящего окна Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	// Устанавливаем максимальный уровень компрессии
	this->_compressor->level(awh::compressor::level_t::BEST);
	// Включаем флаг переиспользования контекста компрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::ENCODE, true));
	// Выполняем компрессию данных на максимальном уровне компрессии
	const size_t best = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE).size();
	// Проверяем что результат компрессии получен
	ASSERT_GT(best, 0u);
	// Устанавливаем уровень максимальной производительности поверх живого контекста
	this->_compressor->level(awh::compressor::level_t::SPEED);
	// Выполняем компрессию данных на уровне максимальной производительности
	const size_t speed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE).size();
	// Проверяем что результат компрессии получен
	ASSERT_GT(speed, 0u);
	// Проверяем что смена уровня дошла до живого контекста
	ASSERT_GT(speed, best);
}

/**
 * @brief Тест очистки результата у перегрузок с выходным параметром
 *
 * @details Закрепляет договор пустого входа для перегрузок, отдающих результат
 *          через параметр: прежнее содержимое контейнера наружу уйти не должно
 *          ни на пустом входе, ни на значении метода вне перечисления
 *
 */
TEST_F(CompressorFixture, OutParameterIsClearedTest){
	// Заведомо постороннее содержимое выходного контейнера
	const std::string stale = "Anyks Framework stale payload";
	// Выходной контейнер результата
	std::string result;
	// Наполняем выходной контейнер посторонним содержимым
	result = stale;
	// Выполняем компрессию пустого буфера данных
	this->_compressor->compress(nullptr, 0, awh::compressor::method_t::GZIP, result);
	// Проверяем что прежнее содержимое не ушло наружу
	ASSERT_TRUE(result.empty());
	// Наполняем выходной контейнер посторонним содержимым
	result = stale;
	// Выполняем компрессию буфера нулевого размера
	this->_compressor->compress(stale.data(), 0, awh::compressor::method_t::GZIP, result);
	// Проверяем что прежнее содержимое не ушло наружу
	ASSERT_TRUE(result.empty());
	// Наполняем выходной контейнер посторонним содержимым
	result = stale;
	// Выполняем декомпрессию пустого буфера данных
	this->_compressor->decompress(nullptr, 0, awh::compressor::method_t::GZIP, result);
	// Проверяем что прежнее содержимое не ушло наружу
	ASSERT_TRUE(result.empty());
	// Наполняем выходной контейнер посторонним содержимым
	result = stale;
	// Выполняем компрессию с методом, лежащим вне перечисления
	this->_compressor->compress(stale.data(), stale.size(), static_cast <awh::compressor::method_t> (0xFE), result);
	// Проверяем что прежнее содержимое не ушло наружу
	ASSERT_TRUE(result.empty());
	// Наполняем выходной контейнер посторонним содержимым
	result = stale;
	// Выполняем декомпрессию с методом, лежащим вне перечисления
	this->_compressor->decompress(stale.data(), stale.size(), static_cast <awh::compressor::method_t> (0xFE), result);
	// Проверяем что прежнее содержимое не ушло наружу
	ASSERT_TRUE(result.empty());
}

/**
 * @brief Тест сквозного прохода при незаданном методе компрессии
 *
 * @details Закрепляет намеренное решение: незаданный метод отдаёт буфер как есть,
 *          а не считается отказом — это законное значение перечисления
 *
 */
TEST_F(CompressorFixture, NoneMethodPassesThroughTest){
	// Формируем текст для проверки
	const std::string text = "Anyks Framework pass-through payload";
	// Выполняем компрессию с незаданным методом
	ASSERT_EQ(text, this->_compressor->compress <std::string> (text, awh::compressor::method_t::NONE));
	// Выполняем декомпрессию с незаданным методом
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (text, awh::compressor::method_t::NONE));
}

/**
 * @brief Тест подачи данных в поток после его финализации
 *
 * @details Закрепляет единый договор кодеров: подача после finish ничего не делает
 *          и поток не рвёт — иначе лишний вызов выглядел бы обрывом сессии
 *
 */
TEST_F(CompressorFixture, PushAfterFinishTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework push after finish payload!!!!!!!!!!!!!!!!?";
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
		awh::compressor::method_t::DEFLATE
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Буфер выхода порции
		std::string part;
		// Подаём данные в поток компрессии
		encoder.push <std::string> (text.data(), text.size(), part);
		// Финализируем поток компрессии
		encoder.finish(part);
		// Проверяем что поток объявлен завершённым
		ASSERT_TRUE(encoder.done()) << "method = " << static_cast <uint16_t> (method);
		// Выполняем лишнюю подачу данных поверх завершённого потока
		encoder.push <std::string> (text.data(), text.size(), part);
		// Проверяем что поток остался валидным
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что лишняя подача ничего не выдала
		ASSERT_TRUE(part.empty()) << "method = " << static_cast <uint16_t> (method);
		// Выполняем лишнюю финализацию завершённого потока
		encoder.finish(part);
		// Проверяем что поток остался валидным
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест проверки размера скользящего окна установщиками
 *
 * @details Закрепляет отказ до записи: значение вне допустимого промежутка не
 *          принимается, прежнее сохраняется, и работоспособность не теряется
 *
 */
TEST_F(CompressorFixture, WindowBitsRangeTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework window bits payload!!!!!!!!!!!!!!!!?";
	// Устанавливаем допустимый размер скользящего окна GZip
	this->_compressor->wbitsGZip(15);
	// Устанавливаем допустимый размер скользящего окна Zlib
	this->_compressor->wbitsZlib(15);
	// Устанавливаем допустимый размер скользящего окна Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	/**
	 * Восьмёрка лежит здесь же: окно в восемь разрядов движок не заводит ни «сырым»
	 * потоком, ни потоком с заголовком gzip, а у формата zlib заводит сжатие, но
	 * поднимает окно до девяти молча — и разбор тем же значением получившийся поток
	 * уже не берёт. Работает она, словом, нигде
	 */
	// Список значений, лежащих вне допустимого промежутка
	const int16_t invalid[] = {-1, 0, 2, 7, 8, 16, 32, 255};
	/**
	 * Выполняем перебор значений вне допустимого промежутка
	 */
	for(auto & wbits : invalid){
		// Выполняем установку значения для GZip
		this->_compressor->wbitsGZip(wbits);
		// Выполняем установку значения для Zlib
		this->_compressor->wbitsZlib(wbits);
		// Проверяем что установщик Deflate отвергает значение
		ASSERT_FALSE(this->_compressor->wbitsDeflate(wbits)) << "wbits = " << wbits;
		// Проверяем что прежний размер окна GZip сохранён и компрессия работает
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::GZIP), awh::compressor::method_t::GZIP)) << "wbits = " << wbits;
		// Проверяем что прежний размер окна Zlib сохранён и компрессия работает
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::ZLIB), awh::compressor::method_t::ZLIB)) << "wbits = " << wbits;
		// Проверяем что прежний размер окна Deflate сохранён и компрессия работает
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE), awh::compressor::method_t::DEFLATE)) << "wbits = " << wbits;
	}
	/**
	 * Выполняем перебор значений внутри допустимого промежутка
	 */
	for(int16_t wbits = 9; wbits <= 15; wbits++)
		// Проверяем что установщик Deflate принимает значение
		ASSERT_TRUE(this->_compressor->wbitsDeflate(wbits)) << "wbits = " << wbits;

}

/**
 * @brief Тест восстановления переиспользуемого контекста после неудачного сообщения
 *
 * @details Закрепляет снятие недоведённого сообщения с живого контекста: после отказа
 *          на испорченных данных следующий полный цикл обязан пройти как обычно,
 *          а не продолжить брошенный поток с середины
 *
 */
TEST_F(CompressorFixture, TakeoverRecoversAfterFailedFrameTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework takeover recovery payload, Anyks Framework takeover recovery payload!!!!!!!!?";
	// Устанавливаем размер скользящего окна Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	// Включаем флаг переиспользования контекста декомпрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::DECODE, true));
	// Выполняем компрессию данных
	const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE);
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Формируем заведомо испорченный кадр данных
	const std::string corrupted(64, '\x7f');
	// Выполняем декомпрессию испорченного кадра поверх живого контекста
	ASSERT_TRUE(this->_compressor->decompress <std::string> (corrupted, awh::compressor::method_t::DEFLATE).empty());
	// Проверяем что следующий полный цикл проходит как обычно
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE));
	// Проверяем что и повторный цикл проходит как обычно
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE));
}

/**
 * @brief Тест проверки размера буфера на пригодность движку
 *
 * @details Закрепляет границы разрядности, общие для блочного и потокового режимов:
 *          буфер такого размера в проверке не завести, поэтому сторожится сама функция
 *
 */
TEST_F(CompressorFixture, SizeLimitsTest){
	// Предел движков, принимающих размер разрядностью 32 бита
	const uint64_t limit32 = static_cast <uint64_t> (UINT32_MAX);
	// Предел движков, принимающих размер знаковой разрядностью
	const uint64_t limit31 = static_cast <uint64_t> (INT32_MAX);
	// Список методов, принимающих размер разрядностью 32 бита
	const awh::compressor::method_t unsigned32[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::DEFLATE,
		awh::compressor::method_t::BZIP2
	};
	// Список методов, принимающих размер знаковой разрядностью
	const awh::compressor::method_t signed32[] = {
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LIZARD
	};
	// Список методов, работающих размером разрядностью платформы
	const awh::compressor::method_t native[] = {
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::SNAPPY,
		awh::compressor::method_t::DENSITY
	};
	/**
	 * На платформе с разрядностью размера в 32 бита предел движка недостижим
	 */
	if(sizeof(size_t) > 4){
		/**
		 * Выполняем перебор методов, принимающих размер разрядностью 32 бита
		 */
		for(auto & method : unsigned32){
			// Проверяем что размер на пределе принимается
			ASSERT_TRUE(awh::compressor::fits(static_cast <size_t> (limit32), method)) << "method = " << static_cast <uint16_t> (method);
			// Проверяем что размер за пределом отвергается
			ASSERT_FALSE(awh::compressor::fits(static_cast <size_t> (limit32 + 1), method)) << "method = " << static_cast <uint16_t> (method);
		}
		/**
		 * Выполняем перебор методов, принимающих размер знаковой разрядностью
		 */
		for(auto & method : signed32){
			// Проверяем что размер на пределе принимается
			ASSERT_TRUE(awh::compressor::fits(static_cast <size_t> (limit31), method)) << "method = " << static_cast <uint16_t> (method);
			// Проверяем что размер за пределом отвергается
			ASSERT_FALSE(awh::compressor::fits(static_cast <size_t> (limit31 + 1), method)) << "method = " << static_cast <uint16_t> (method);
		}
		/**
		 * Выполняем перебор методов, работающих размером разрядностью платформы
		 */
		for(auto & method : native)
			// Проверяем что размер за пределом 32 бит принимается
			ASSERT_TRUE(awh::compressor::fits(static_cast <size_t> (limit32 + 1), method)) << "method = " << static_cast <uint16_t> (method);
	}
	/**
	 * Выполняем перебор всех перечисленных методов на обычном размере
	 */
	for(auto & method : unsigned32)
		// Проверяем что обычный размер принимается
		ASSERT_TRUE(awh::compressor::fits(65536, method)) << "method = " << static_cast <uint16_t> (method);
	// Проверяем что обычный размер принимается незаданным методом
	ASSERT_TRUE(awh::compressor::fits(65536, awh::compressor::method_t::NONE));
}

/**
 * @brief Тест подачи в поток без буфера данных
 *
 * @details Закрепляет отказ на подаче без буфера при ненулевом размере: движок пошёл бы
 *          читать по пустому указателю. Пустая подача при этом законна — ею работают
 *          выдавливание и финализация
 *
 */
TEST_F(CompressorFixture, StreamRejectsNullBufferTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework null buffer payload!!!!!!!!!!!!!!!!?";
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = this->_compressor->stream(awh::compressor::method_t::GZIP, awh::compressor::event_t::ENCODE);
	// Проверяем что потоковая сессия создана
	ASSERT_TRUE(encoder.valid());
	// Буфер выхода порции
	std::string part = "Anyks Framework stale payload";
	// Выполняем подачу без буфера при ненулевом размере
	encoder.push <std::string> (nullptr, 64, part);
	// Проверяем что прежнее содержимое не ушло наружу
	ASSERT_TRUE(part.empty());
	// Проверяем что поток остался валидным
	ASSERT_TRUE(encoder.valid());
	// Результат компрессии данных
	std::string compressed;
	// Подаём данные в поток компрессии
	encoder.push <std::string> (text.data(), text.size(), part);
	// Дописываем полученный выход в результат компрессии
	compressed.append(part);
	// Финализируем поток компрессии
	encoder.finish(part);
	// Дописываем хвост в результат компрессии
	compressed.append(part);
	// Проверяем что поток отработал как обычно
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::GZIP));
}

/**
 * @brief Тест блочного цикла на сильно сжимаемых данных
 *
 * @details Закрепляет работу драйверов там, где распакованное во много раз больше
 *          сжатого: выходной буфер при этом заполняется вплотную и наращивается
 *          многократно, а движок упирается в место как раз на исчерпании входа
 *
 */
TEST_F(CompressorFixture, HighRatioRoundTripTest){
	// Формируем сильно сжимаемый буфер данных
	const std::string text(1024 * 1024, 'A');
	// Список проверяемых методов компрессии
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::DEFLATE,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LIZARD
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что результат компрессии много меньше исходного
		ASSERT_LT(compressed.size(), (text.size() / 8)) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что результат декомпрессии совпадает с исходным текстом
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, method)) << "method = " << static_cast <uint16_t> (method);
	}
	// Устанавливаем размер скользящего окна Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	// Включаем флаг переиспользования контекста компрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::ENCODE, true));
	// Включаем флаг переиспользования контекста декомпрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::DECODE, true));
	/**
	 * Выполняем несколько сообщений подряд на переиспользуемом контексте
	 */
	for(uint8_t i = 0; i < 3; i++){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "message = " << static_cast <uint16_t> (i);
		// Проверяем что результат декомпрессии совпадает с исходным текстом
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE)) << "message = " << static_cast <uint16_t> (i);
	}
}

/**
 * @brief Тест устойчивости к порче отдельных октетов кадра
 *
 * @details Закрепляет отсутствие отказа и разбора за границами буфера на кадре
 *          с перевёрнутыми октетами: служебные поля форматов при этом уводят
 *          разбор по путям, до которых обычные данные не доходят
 *
 */
TEST_F(CompressorFixture, BitFlippedFrameTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework bit flipped frame payload, Anyks Framework bit flipped frame payload!!!!!!!!?";
	/**
	 * Список методов, формат которых несёт контрольную сумму: у них порча октета
	 * обязана быть замечена, и разбор либо отказывает, либо отдаёт исходные данные
	 */
	const awh::compressor::method_t checked[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::LZMA
	};
	/**
	 * Список методов, формат которых целостность не сверяет: испорченный кадр для них
	 * остаётся допустимым входом, и от разбора требуется лишь не выйти за границы буфера
	 */
	const awh::compressor::method_t unchecked[] = {
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LIZARD,
		awh::compressor::method_t::SNAPPY,
		awh::compressor::method_t::DENSITY
	};
	/**
	 * Выполняем перебор методов, сверяющих целостность
	 */
	for(auto & method : checked){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		/**
		 * Выполняем перебор позиций перевёрнутого октета
		 */
		for(size_t offset = 0; offset < compressed.size(); offset++){
			/**
			 * Выполняем перебор перевёрнутых разрядов
			 */
			for(uint8_t bit = 0; bit < 8; bit += 3){
				// Формируем испорченный кадр данных
				std::vector <uint8_t> damaged = compressed;
				// Переворачиваем очередной разряд
				damaged[offset] ^= static_cast <uint8_t> (1u << bit);
				// Выполняем декомпрессию испорченного кадра
				const std::string restored = this->_compressor->decompress <std::string> (damaged, method);
				// Проверяем что разбор либо отказал, либо восстановил исходные данные в точности
				ASSERT_TRUE(restored.empty() || (restored == text)) << "method = " << static_cast <uint16_t> (method) << ", offset = " << offset << ", bit = " << static_cast <uint16_t> (bit);
			}
		}
	}
	/**
	 * Выполняем перебор методов, целостность не сверяющих
	 */
	for(auto & method : unchecked){
		// Выполняем компрессию данных
		const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, method);
		// Проверяем что результат компрессии не пустой
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		/**
		 * Выполняем перебор позиций перевёрнутого октета
		 */
		for(size_t offset = 0; offset < compressed.size(); offset++){
			/**
			 * Выполняем перебор перевёрнутых разрядов
			 */
			for(uint8_t bit = 0; bit < 8; bit += 3){
				// Формируем испорченный кадр данных
				std::vector <uint8_t> damaged = compressed;
				// Переворачиваем очередной разряд
				damaged[offset] ^= static_cast <uint8_t> (1u << bit);
				/**
				 * Выполняем декомпрессию испорченного кадра. Проверяется сам факт возврата:
				 * разбор обязан завершиться, не выйдя за границы буфера и не отказав работе.
				 * Что именно он вернёт — свойство формата, а не модуля
				 */
				const std::string restored = this->_compressor->decompress <std::string> (damaged, method);
				// Проверяем что размер результата не превышает допустимого предела
				ASSERT_LE(restored.size(), static_cast <size_t> (1ULL << 30)) << "method = " << static_cast <uint16_t> (method) << ", offset = " << offset << ", bit = " << static_cast <uint16_t> (bit);
			}
		}
	}
}

/**
 * @brief Тест единого предела памяти распаковки LZMA у обоих режимов
 *
 * @details Закрепляет одинаковую политику блочного и потокового режимов: контейнер,
 *          принятый одним, обязан быть принят и другим — разъезд предела означал бы,
 *          что одни и те же данные разбираются не везде
 *
 */
TEST_F(CompressorFixture, LzmaMemoryLimitParityTest){
	// Формируем крупный буфер данных
	std::string text;
	// Резервируем память под буфер данных
	text.reserve(4 * 1024 * 1024);
	// Состояние генератора псевдослучайной последовательности
	uint64_t state = 0x9e3779b97f4a7c15ull;
	/**
	 * Наполняем буфер повторяющимися порциями с переменной частью
	 */
	while(text.size() < (4 * 1024 * 1024)){
		// Продвигаем состояние генератора
		state ^= (state << 13);
		state ^= (state >> 7);
		state ^= (state << 17);
		// Добавляем очередную порцию данных
		text.append("Anyks Framework lzma memory limit payload ");
		// Добавляем переменную часть порции данных
		text.append(std::to_string(state & 0xFFFF));
	}
	// Устанавливаем максимальный уровень компрессии — он берёт наибольший словарь
	this->_compressor->level(awh::compressor::level_t::BEST);
	// Выполняем блочную компрессию данных
	const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::LZMA);
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Проверяем что блочный режим разбирает свой же контейнер
	ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::LZMA));
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = this->_compressor->stream(awh::compressor::method_t::LZMA, awh::compressor::event_t::DECODE);
	// Проверяем что потоковая сессия создана
	ASSERT_TRUE(decoder.valid());
	// Буфер выхода порции
	std::string part;
	// Результат потоковой декомпрессии
	std::string restored;
	// Подаём блочно сжатые данные в потоковую сессию
	decoder.push <std::string> (compressed.data(), compressed.size(), part, awh::compressor::flush_t::FINISH);
	// Дописываем полученный выход в результат декомпрессии
	restored.append(part);
	// Проверяем что потоковый режим разобрал тот же контейнер
	ASSERT_EQ(text, restored);
	// Проверяем что потоковая сессия объявила поток завершённым
	ASSERT_TRUE(decoder.done());
}

/**
 * @brief Тест потокового цикла на сильно сжимаемых данных одной порцией
 *
 * @details Закрепляет дожатие накопленного движком: кадр здесь во много раз меньше
 *          распакованного, поэтому вход исчерпывается задолго до того, как движок
 *          выдаст всё, — и разбор, идущий только по наличию входа, терял бы хвост
 *
 */
TEST_F(CompressorFixture, StreamHighRatioSinglePushTest){
	// Формируем сильно сжимаемый буфер данных
	const std::string text(1024 * 1024, 'A');
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
		awh::compressor::method_t::DEFLATE
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Буфер выхода порции
		std::string part;
		// Результат потоковой компрессии
		std::string compressed;
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Подаём все данные одной порцией
		encoder.push <std::string> (text.data(), text.size(), part);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		// Финализируем поток компрессии
		encoder.finish(part);
		// Дописываем хвост в результат компрессии
		compressed.append(part);
		// Проверяем что кадр во много раз меньше исходных данных
		ASSERT_LT(compressed.size(), (text.size() / 64)) << "method = " << static_cast <uint16_t> (method);
		// Результат потоковой декомпрессии
		std::string restored;
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t decoder = this->_compressor->stream(method, awh::compressor::event_t::DECODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(decoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Подаём весь кадр одной порцией
		decoder.push <std::string> (compressed.data(), compressed.size(), part);
		// Дописываем полученный выход в результат декомпрессии
		restored.append(part);
		// Финализируем поток декомпрессии
		decoder.finish(part);
		// Дописываем хвост в результат декомпрессии
		restored.append(part);
		// Проверяем что поток остался валидным
		ASSERT_TRUE(decoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что восстановленные данные совпадают с исходными
		ASSERT_EQ(text, restored) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Тест потоковой компрессии мелкими порциями с финализацией
 *
 * @details Закрепляет доведение сброса до конца: движок вправе ответить обычным успехом,
 *          ещё не выписав эпилог кадра, и разбор, выходящий по одному лишь исчерпанию
 *          входа, оставил бы кадр незакрытым
 *
 */
TEST_F(CompressorFixture, StreamSmallChunkFinishTest){
	// Формируем крупный буфер данных
	std::string text;
	// Резервируем память под буфер данных
	text.reserve(512 * 1024);
	/**
	 * Наполняем буфер данными переменной повторяемости
	 */
	for(uint32_t i = 0; text.size() < (512 * 1024); i++){
		// Добавляем очередную порцию данных
		text.append("Anyks Framework small chunk finish payload ");
		// Добавляем переменную часть порции данных
		text.append(std::to_string(i * 2654435761u));
	}
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
		awh::compressor::method_t::DEFLATE
	};
	// Набор размеров порций подачи
	const size_t chunks[] = {1024, 2048, 4096};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		/**
		 * Выполняем перебор размеров порций подачи
		 */
		for(auto & chunk : chunks){
			// Буфер выхода порции
			std::string part;
			// Результат потоковой компрессии
			std::string compressed;
			// Создаём потоковую сессию компрессии
			awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
			// Проверяем что потоковая сессия создана
			ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", chunk = " << chunk;
			/**
			 * Подаём данные мелкими порциями
			 */
			for(size_t offset = 0; offset < text.size(); offset += chunk){
				// Определяем размер очередной порции
				const size_t actual = ((text.size() - offset) > chunk ? chunk : (text.size() - offset));
				// Подаём очередную порцию в поток компрессии
				encoder.push <std::string> (text.data() + offset, actual, part);
				// Дописываем полученный выход в результат компрессии
				compressed.append(part);
			}
			// Финализируем поток компрессии
			encoder.finish(part);
			// Дописываем хвост в результат компрессии
			compressed.append(part);
			// Проверяем что поток объявлен завершённым
			ASSERT_TRUE(encoder.done()) << "method = " << static_cast <uint16_t> (method) << ", chunk = " << chunk;
			// Результат потоковой декомпрессии
			std::string restored;
			// Создаём потоковую сессию декомпрессии
			awh::compressor::stream_t decoder = this->_compressor->stream(method, awh::compressor::event_t::DECODE);
			// Проверяем что потоковая сессия создана
			ASSERT_TRUE(decoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", chunk = " << chunk;
			/**
			 * Подаём кадр мелкими порциями
			 */
			for(size_t offset = 0; offset < compressed.size(); offset += chunk){
				// Определяем размер очередной порции
				const size_t actual = ((compressed.size() - offset) > chunk ? chunk : (compressed.size() - offset));
				// Подаём очередную порцию в поток декомпрессии
				decoder.push <std::string> (compressed.data() + offset, actual, part);
				// Дописываем полученный выход в результат декомпрессии
				restored.append(part);
			}
			// Финализируем поток декомпрессии
			decoder.finish(part);
			// Дописываем хвост в результат декомпрессии
			restored.append(part);
			// Проверяем что восстановленные данные совпадают с исходными
			ASSERT_EQ(text, restored) << "method = " << static_cast <uint16_t> (method) << ", chunk = " << chunk;
		}
	}
}

/**
 * @brief Проверка разбора кадра с хвостом посторонних октетов
 *
 * @details Кадр, дошедший до конца, разбор прекращает: движок, объявленный
 *          завершённым, вход больше не берёт, и хвост, оставшийся за кадром,
 *          крутил бы цикл вечно — выхода он не производит, входа не убавляет.
 *          Тест сторожит именно незавершаемость подачи, а не судьбу хвоста:
 *          распорядиться им — дело вызывающей стороны, разбирающей кадры
 *
 */
TEST_F(CompressorFixture, StreamTrailingBytesTest){
	// Формируем буфер исходных данных
	const std::string text = "Anyks Framework trailing bytes payload for the streaming decoder";
	// Список проверяемых методов компрессии
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::LIZARD
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Буфер выхода порции
		std::string part;
		// Результат потоковой компрессии
		std::string compressed;
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Подаём данные в поток компрессии
		encoder.push <std::string> (text.data(), text.size(), part);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		// Финализируем поток компрессии
		encoder.finish(part);
		// Дописываем хвост в результат компрессии
		compressed.append(part);
		// Дописываем к готовому кадру хвост посторонних октетов
		compressed.append(64, '\x5a');
		// Результат потоковой декомпрессии
		std::string restored;
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t decoder = this->_compressor->stream(method, awh::compressor::event_t::DECODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(decoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Подаём кадр с хвостом одной порцией — работа обязана завершиться
		decoder.push <std::string> (compressed.data(), compressed.size(), restored);
		// Буфер остатка потока декомпрессии
		std::string tail;
		// Финализируем поток декомпрессии
		decoder.finish(tail);
		// Дописываем остаток в результат декомпрессии
		restored.append(tail);
		/**
		 * Хвост движок либо отбрасывает, либо считает порчей и рвёт сессию: и то,
		 * и другое законно. Середины между ними нет — выдать часть данных работа
		 * не вправе, поэтому у живой сессии сличается весь результат
		 */
		// Если поток отказом не завершился
		if(decoder.valid())
			// Проверяем что исходные данные разобраны целиком
			ASSERT_EQ(text, restored) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Проверка отказа Snappy на кадре с завышенным распакованным размером
 *
 * @details Распакованный размер записан в самом кадре, и движок отводит по нему
 *          память прежде, чем разбирать данные. Проверка сторожит сам отказ;
 *          отведения памяти, которого правка избегает, она увидеть не может —
 *          итог у обоих случаев один
 *
 */
TEST_F(CompressorFixture, SnappyOversizedLengthTest){
	// Собираем подвал кадра с распакованным размером в два гигабайта
	std::string frame;
	// Записываем размер переменной длины по устройству формата
	for(uint64_t length = (2ull * 1024ull * 1024ull * 1024ull); length > 0; length >>= 7)
		// Добавляем очередной октет размера
		frame.push_back(static_cast <char> ((length & 0x7f) | ((length >> 7) > 0 ? 0x80 : 0x00)));
	// Дописываем к подвалу немного данных
	frame.append(16, '\x00');
	// Результат декомпрессии
	std::string result = "Anyks";
	// Выполняем декомпрессию подделанного кадра
	this->_compressor->decompress(frame.data(), frame.size(), awh::compressor::method_t::SNAPPY, result);
	// Проверяем что кадр отвергнут
	ASSERT_TRUE(result.empty());
}

/**
 * @brief Проверка распаковки, заполняющей рабочий буфер ровно по краю
 *
 * @details Рабочий буфер распаковки Deflate начинается с 255 октетов и растёт
 *          удвоением, у GZip и Zlib он постоянен и равен размеру порции. Данные,
 *          чья длина ложится на этот край ровно, уводят работу на ещё один заход
 *          с уже съеденным входом: движок отвечает Z_BUF_ERROR, и принять этот
 *          код за отказ значит отвергнуть законный кадр.
 *
 *          Края у движков разные, поэтому в переборе и те длины, и другие: без
 *          длин, кратных размеру порции, строки GZip и Zlib проверяли бы обычный
 *          разбор, а не край, ради которого проверка заведена
 *
 */
TEST_F(CompressorFixture, ExactBufferFillTest){
	// Список проверяемых методов компрессии
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::DEFLATE
	};
	// Список проверяемых длин, ложащихся на края рабочих буферов ровно
	const size_t sizes[] = {255, 510, 1020, 2040, 4080, 16384, 32768};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		/**
		 * Выполняем перебор проверяемых длин
		 */
		for(auto & size : sizes){
			/**
			 * Выполняем перебор наполнений буфера: нули, возрастающая
			 * последовательность и слабо сжимаемая псевдослучайная величина
			 */
			for(uint16_t kind = 0; kind < 3; kind++){
				// Буфер исходных данных
				std::string text(size, '\0');
				/**
				 * Наполняем буфер данными выбранного вида
				 */
				for(size_t i = 0; i < size; i++){
					// Определяем вид наполнения буфера
					switch(kind){
						// Возрастающая последовательность
						case 1: text[i] = static_cast <char> (i & 0xff); break;
						// Псевдослучайная величина
						case 2: text[i] = static_cast <char> ((i * 2654435761u) >> 13); break;
					}
				}
				// Результат компрессии
				std::string compressed;
				// Выполняем компрессию исходных данных
				this->_compressor->compress(text.data(), text.size(), method, compressed);
				// Проверяем что компрессия выполнена
				ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method) << ", size = " << size << ", kind = " << kind;
				// Результат декомпрессии
				std::string restored;
				// Выполняем декомпрессию полученного кадра
				this->_compressor->decompress(compressed.data(), compressed.size(), method, restored);
				// Проверяем что восстановленные данные совпадают с исходными
				ASSERT_EQ(text, restored) << "method = " << static_cast <uint16_t> (method) << ", size = " << size << ", kind = " << kind;
			}
		}
	}
}

/**
 * @brief Проверка смены размера скользящего окна при живом переиспользуемом контексте
 *
 * @details У живого контекста размер окна сменить нельзя, поэтому установщик
 *          пересобирает обе половины разом. Проверка закрепляет, что после
 *          пересборки переиспользование остаётся включённым и обмен сообщениями
 *          продолжается, а не рвётся посередине
 *
 */
TEST_F(CompressorFixture, WbitsChangeOnLiveTakeoverTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework live window resize payload, Anyks Framework live window resize payload!!!!!!!!?";
	// Устанавливаем размер скользящего окна Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
	// Включаем переиспользование контекста компрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::ENCODE, true));
	// Включаем переиспользование контекста декомпрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::DECODE, true));
	/**
	 * Выполняем перебор размеров скользящего окна на живом контексте
	 */
	for(int16_t wbits = 15; wbits >= 9; wbits--){
		// Выполняем пересборку контекстов под новый размер окна
		ASSERT_TRUE(this->_compressor->wbitsDeflate(wbits)) << "wbits = " << wbits;
		/**
		 * Обмениваемся парой сообщений: контекст переиспользуется, и второе
		 * сообщение опирается на словарь, набранный первым
		 */
		for(uint16_t i = 0; i < 2; i++){
			// Выполняем компрессию данных
			const std::vector <uint8_t> compressed = this->_compressor->compress <std::vector <uint8_t>> (text, awh::compressor::method_t::DEFLATE);
			// Проверяем что результат компрессии не пустой
			ASSERT_FALSE(compressed.empty()) << "wbits = " << wbits << ", message = " << i;
			// Проверяем что результат декомпрессии совпадает с исходным текстом
			ASSERT_EQ(text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE)) << "wbits = " << wbits << ", message = " << i;
		}
	}
}

/**
 * @brief Проверка разбора блочного кадра с хвостом посторонних октетов
 *
 * @details Блочная работа получает длину буфера от вызывающей стороны, и хвост
 *          за концом кадра для неё — те же поданные данные. Требуется одно:
 *          работа завершается и либо отвергает кадр, либо выдаёт исходные данные,
 *          а не крутится на неразбираемом остатке
 *
 */
TEST_F(CompressorFixture, BlockTrailingBytesTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework block trailing bytes payload for the one-shot codec";
	// Список проверяемых методов компрессии
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Выполняем компрессию исходных данных
		std::string compressed;
		// Выполняем компрессию данных
		this->_compressor->compress(text.data(), text.size(), method, compressed);
		// Проверяем что компрессия выполнена
		ASSERT_FALSE(compressed.empty()) << "method = " << static_cast <uint16_t> (method);
		// Дописываем к готовому кадру хвост посторонних октетов
		compressed.append(64, '\x5a');
		// Результат декомпрессии
		std::string restored;
		// Выполняем декомпрессию кадра с хвостом — работа обязана завершиться
		this->_compressor->decompress(compressed.data(), compressed.size(), method, restored);
		/**
		 * Хвост движок либо отбрасывает, либо считает порчей и отвергает кадр:
		 * и то, и другое законно. Середины между ними нет — выдать часть данных
		 * работа не вправе, поэтому сличается весь результат, а не его начало
		 */
		// Если кадр не отвергнут
		if(!restored.empty())
			// Проверяем что исходные данные разобраны целиком
			ASSERT_EQ(text, restored) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Проверка живучести сессии на подаче, работы не несущей
 *
 * @details Пустая подача без сброса и повторный сброс без накопленного — вызовы
 *          законные и работы движку не дают. Сессию они рвать не должны: отказ
 *          здесь означал бы, что вызывающая сторона обязана считать за движок,
 *          осталось ли ему что выдавать
 *
 */
TEST_F(CompressorFixture, IdlePushKeepsSessionTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework idle push payload, Anyks Framework idle push payload!!!!!!!!?";
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
		awh::compressor::method_t::DEFLATE
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Буфер выхода порции
		std::string part;
		// Результат потоковой компрессии
		std::string compressed;
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия создана
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Выполняем пустую подачу без сброса на свежей сессии
		encoder.push <std::string> (text.data(), 0, part);
		// Проверяем что сессия жива
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", empty push on fresh stream";
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		/**
		 * Выполняем два выдавливания подряд на свежей сессии: выдавливать нечего
		 * ни первому, ни второму, и оба обязаны выйти успехом
		 */
		for(uint16_t i = 0; i < 2; i++){
			// Выполняем принудительное выдавливание накопленного
			encoder.flush(part);
			// Проверяем что сессия жива
			ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", flush on fresh stream = " << i;
			// Дописываем полученный выход в результат компрессии
			compressed.append(part);
		}
		// Подаём данные в поток компрессии
		encoder.push <std::string> (text.data(), text.size(), part);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		// Выполняем пустую подачу без сброса после данных
		encoder.push <std::string> (text.data(), 0, part);
		// Проверяем что сессия жива
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", empty push after data";
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		/**
		 * Выполняем два выдавливания подряд: второму выдавливать уже нечего
		 */
		for(uint16_t i = 0; i < 2; i++){
			// Выполняем принудительное выдавливание накопленного
			encoder.flush(part);
			// Проверяем что сессия жива
			ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", flush = " << i;
			// Дописываем полученный выход в результат компрессии
			compressed.append(part);
		}
		// Финализируем поток компрессии
		encoder.finish(part);
		// Проверяем что поток объявлен завершённым
		ASSERT_TRUE(encoder.done()) << "method = " << static_cast <uint16_t> (method);
		// Дописываем хвост в результат компрессии
		compressed.append(part);
		// Результат потоковой декомпрессии
		std::string restored;
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t decoder = this->_compressor->stream(method, awh::compressor::event_t::DECODE);
		// Подаём кадр в поток декомпрессии
		decoder.push <std::string> (compressed.data(), compressed.size(), restored);
		// Буфер остатка потока декомпрессии
		std::string tail;
		// Финализируем поток декомпрессии
		decoder.finish(tail);
		// Дописываем остаток в результат декомпрессии
		restored.append(tail);
		// Проверяем что восстановленные данные совпадают с исходными
		ASSERT_EQ(text, restored) << "method = " << static_cast <uint16_t> (method);
	}
}

/**
 * @brief Проверка отказа блочного режима на подаче без буфера
 *
 * @details Подача без буфера при ненулевом размере — ошибка вызывающей стороны,
 *          а не пустой вход: работа получила указание разобрать данные, которых
 *          ей не дали. Договор здесь общий с потоковым режимом
 *
 */
TEST_F(CompressorFixture, BlockRejectsNullBufferTest){
	// Список проверяемых методов компрессии
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::LZ4,
		awh::compressor::method_t::LZMA,
		awh::compressor::method_t::ZSTD,
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::BZIP2,
		awh::compressor::method_t::BROTLI,
		awh::compressor::method_t::SNAPPY,
		awh::compressor::method_t::DENSITY,
		awh::compressor::method_t::LIZARD,
		awh::compressor::method_t::DEFLATE
	};
	/**
	 * Выполняем перебор всех методов компрессии
	 */
	for(auto & method : methods){
		// Результат работы
		std::string result = "Anyks";
		// Выполняем компрессию несуществующего буфера
		this->_compressor->compress(nullptr, 64, method, result);
		// Проверяем что подача отвергнута
		ASSERT_TRUE(result.empty()) << "method = " << static_cast <uint16_t> (method) << ", compress";
		// Восстанавливаем содержимое результата
		result = "Anyks";
		// Выполняем декомпрессию несуществующего буфера
		this->_compressor->decompress(nullptr, 64, method, result);
		// Проверяем что подача отвергнута
		ASSERT_TRUE(result.empty()) << "method = " << static_cast <uint16_t> (method) << ", decompress";
		/**
		 * Пустая подача при этом отказом не является: сжимать нечего — это законное
		 * положение дел, и работа выдаёт пустой результат молча
		 */
		// Восстанавливаем содержимое результата
		result = "Anyks";
		// Выполняем компрессию пустого буфера
		this->_compressor->compress(nullptr, 0, method, result);
		// Проверяем что результат пуст
		ASSERT_TRUE(result.empty()) << "method = " << static_cast <uint16_t> (method) << ", empty";
	}
}

/**
 * @brief Проверка размера скользящего окна у напрямую заведённой сессии
 *
 * @details Установщики `Block` промежуток сторожат, а конструктор `Stream` открыт
 *          наружу и параметры принимает как есть. Окно ниже девяти разрядов
 *          негодно и здесь: сессия обязана выйти невалидной, а не завести
 *          сжатие, разобрать которое тем же значением нельзя
 *
 */
TEST_F(CompressorFixture, StreamWindowBitsRangeTest){
	// Формируем параметры инициализации сессии
	awh::compressor::params_t params;
	// Список методов семейства Zlib
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::DEFLATE
	};
	// Список значений, лежащих вне допустимого промежутка
	const int16_t invalid[] = {-1, 0, 2, 7, 8, 16, 32, 255};
	/**
	 * Выполняем перебор методов семейства Zlib
	 */
	for(auto & method : methods){
		/**
		 * Выполняем перебор значений вне допустимого промежутка
		 */
		for(auto & wbits : invalid){
			// Устанавливаем размер скользящего окна
			params.wbits = wbits;
			// Создаём потоковую сессию компрессии
			awh::compressor::stream_t encoder(method, awh::compressor::event_t::ENCODE, params, this->_log.get());
			// Проверяем что сессия не заведена
			ASSERT_FALSE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", wbits = " << wbits << ", encode";
			// Создаём потоковую сессию декомпрессии
			awh::compressor::stream_t decoder(method, awh::compressor::event_t::DECODE, params, this->_log.get());
			// Проверяем что сессия не заведена
			ASSERT_FALSE(decoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", wbits = " << wbits << ", decode";
		}
		/**
		 * Выполняем перебор значений внутри допустимого промежутка
		 */
		for(int16_t wbits = 9; wbits <= 15; wbits++){
			// Устанавливаем размер скользящего окна
			params.wbits = wbits;
			// Создаём потоковую сессию компрессии
			awh::compressor::stream_t encoder(method, awh::compressor::event_t::ENCODE, params, this->_log.get());
			// Проверяем что сессия заведена
			ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method) << ", wbits = " << wbits;
		}
	}
}

/**
 * @brief Проверка завершающих октетов блочного сообщения Deflate
 *
 * @details Сообщение завершается Z_SYNC_FLUSH, а не Z_FINISH, и потому оканчивается
 *          четырьмя октетами 00 00 FF FF (RFC 7692). Договор этот несут заголовок
 *          класса и README, а сторожа у него не было: смена сброса на Z_FINISH
 *          прошла бы мимо обратимости — блочная распаковка конца потока не требует, —
 *          и разошлась бы лишь с чужим узлом
 *
 */
TEST_F(CompressorFixture, DeflateSyncFlushTailTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework permessage-deflate tail payload, Anyks Framework permessage-deflate tail payload!!!!?";
	/**
	 * Выполняем перебор обоих способов работы: на своём контексте и на переиспользуемом
	 */
	for(uint16_t takeover = 0; takeover < 2; takeover++){
		// Включаем либо отключаем переиспользование контекста компрессии
		ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::ENCODE, takeover > 0));
		/**
		 * Отправляем пару сообщений: у переиспользуемого контекста второе сообщение
		 * опирается на словарь первого, и хвост обязан стоять у обоих
		 */
		for(uint16_t i = 0; i < 2; i++){
			// Выполняем компрессию данных
			const std::string compressed = this->_compressor->compress <std::string> (text, awh::compressor::method_t::DEFLATE);
			// Проверяем что результат компрессии не пустой
			ASSERT_GT(compressed.size(), 4u) << "takeover = " << takeover << ", message = " << i;
			// Проверяем что сообщение оканчивается четырьмя октетами 00 00 FF FF
			ASSERT_EQ(std::string("\x00\x00\xFF\xFF", 4), compressed.substr(compressed.size() - 4)) << "takeover = " << takeover << ", message = " << i;
		}
	}
	// Снимаем переиспользование контекста компрессии
	ASSERT_TRUE(this->_compressor->takeoverDeflate(awh::compressor::event_t::ENCODE, false));
}

/**
 * @brief Тест раздельности размера скользящего окна у движков семейства Zlib
 *
 * @details Закрепляет то, ради чего настройки семейства и были разведены по именам:
 *          у GZip, Zlib и Deflate свой размер окна, и каждый движок обязан читать
 *          именно свой. Все прежние проверки задавали трём установщикам одно и то
 *          же значение, поэтому подмена поля одного движка полем другого прошла бы
 *          мимо них незамеченной. Наблюдаемым признаком служит размер сжатого:
 *          повтор стоит дальше, чем достаёт малое окно, и сжатие с ним выходит
 *          крупнее. Меняем окно одному движку, двум другим оставляем большое —
 *          и размер обязан вырасти у того движка, которому его меняли
 *
 */
TEST_F(CompressorFixture, WindowBitsAreSeparatePerEngineTest){
	// Формируем блок данных, повтор которого малому окну не достать
	std::string block;
	/**
	 * Наполняем блок неповторяющимся содержимым, чтобы сжатие опиралось
	 * на повтор блока целиком, а не на совпадения внутри него самого
	 */
	for(uint16_t i = 0; i < 2048; i++)
		// Добавляем очередной октет блока
		block.append(1, static_cast <char> ((i * 37) % 251));
	// Формируем текст для компрессии из двух одинаковых блоков
	const std::string text = (block + block);
	// Список методов семейства с их установщиками размера окна
	const awh::compressor::method_t methods[] = {
		awh::compressor::method_t::GZIP,
		awh::compressor::method_t::ZLIB,
		awh::compressor::method_t::DEFLATE
	};
	/**
	 * Выполняем перебор движков семейства
	 */
	for(auto & method : methods){
		// Устанавливаем большое окно всем трём движкам
		this->_compressor->wbitsGZip(15);
		// Устанавливаем большое окно движку Zlib
		this->_compressor->wbitsZlib(15);
		// Устанавливаем большое окно движку Deflate
		ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
		// Выполняем компрессию данных на большом окне
		const std::string wide = this->_compressor->compress <std::string> (text, method);
		// Проверяем что компрессия на большом окне выполнена
		ASSERT_FALSE(wide.empty()) << "method = " << static_cast <uint16_t> (method);
		/**
		 * Обратимость большого окна проверяется до его уменьшения: разбор ведётся
		 * тем же размером окна, каким шло сжатие, и малым окном крупный поток не берётся
		 */
		// Проверяем что данные на большом окне обратимы
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (wide, method)) << "method = " << static_cast <uint16_t> (method);
		/**
		 * Уменьшаем окно одному лишь разбираемому движку: два других остаются
		 * с большим окном, и подмена поля тут же выдала бы себя неизменным размером
		 */
		switch(static_cast <uint8_t> (method)){
			// Уменьшаем окно движку GZip
			case static_cast <uint8_t> (awh::compressor::method_t::GZIP):
				// Выполняем установку малого окна
				this->_compressor->wbitsGZip(9);
			break;
			// Уменьшаем окно движку Zlib
			case static_cast <uint8_t> (awh::compressor::method_t::ZLIB):
				// Выполняем установку малого окна
				this->_compressor->wbitsZlib(9);
			break;
			// Уменьшаем окно движку Deflate
			case static_cast <uint8_t> (awh::compressor::method_t::DEFLATE):
				// Выполняем установку малого окна
				ASSERT_TRUE(this->_compressor->wbitsDeflate(9));
			break;
		}
		// Выполняем компрессию данных на малом окне
		const std::string narrow = this->_compressor->compress <std::string> (text, method);
		// Проверяем что компрессия на малом окне выполнена
		ASSERT_FALSE(narrow.empty()) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что уменьшение окна сказалось именно на этом движке
		ASSERT_GT(narrow.size(), wide.size()) << "method = " << static_cast <uint16_t> (method);
		// Проверяем что данные на малом окне обратимы
		ASSERT_EQ(text, this->_compressor->decompress <std::string> (narrow, method)) << "method = " << static_cast <uint16_t> (method);
		/**
		 * Потоковая сессия разбирает настройки семейства своим отдельным местом,
		 * и подмена поля там прошла бы мимо проверки блочного режима: сессия,
		 * заведённая при малом окне, обязана сжимать так же крупно
		 */
		// Буфер выхода очередной порции потоковой сессии
		std::string part;
		// Результат компрессии данных потоковой сессией
		std::string streamed;
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder = this->_compressor->stream(method, awh::compressor::event_t::ENCODE);
		// Проверяем что потоковая сессия заведена
		ASSERT_TRUE(encoder.valid()) << "method = " << static_cast <uint16_t> (method);
		// Выполняем подачу всех данных одной порцией с завершением кадра
		encoder.push(text.data(), text.size(), part, awh::compressor::flush_t::FINISH);
		// Добавляем полученную порцию к результату
		streamed.append(part);
		// Проверяем что сессия при малом окне сжала так же крупно, как блочный режим
		ASSERT_GT(streamed.size(), wide.size()) << "method = " << static_cast <uint16_t> (method);
	}
	// Возвращаем большое окно движку GZip
	this->_compressor->wbitsGZip(15);
	// Возвращаем большое окно движку Zlib
	this->_compressor->wbitsZlib(15);
	// Возвращаем большое окно движку Deflate
	ASSERT_TRUE(this->_compressor->wbitsDeflate(15));
}

/**
 * @brief Тест работы потоковой сессии с пустым объектом журнала
 *
 * @details Конструктор потока открыт наружу и пустой объект работы с логами принимает —
 *          проверка размера скользящего окна в нём самом и проверка предела выхода
 *          у кодеров его на пустоту стерегут. Подача же обращалась к нему без проверки,
 *          и работа падала ровно там, где собиралась сообщить об ошибке вызывающей
 *          стороны: подача пустого указателя при ненулевом размере. Закрепляем то,
 *          что сессия с пустым журналом жива и на верной подаче, и на ошибочной
 *
 */
TEST_F(CompressorFixture, StreamNullLogTest){
	// Формируем текст для компрессии
	const std::string text = "Anyks Framework null log payload, Anyks Framework null log payload!!!!!!!!!!!!!!?";
	// Формируем параметры инициализации потоковой сессии
	awh::compressor::params_t params;
	// Устанавливаем размер скользящего окна
	params.wbits = 15;
	// Создаём потоковую сессию компрессии с пустым объектом работы с логами
	awh::compressor::stream_t encoder(awh::compressor::method_t::DEFLATE, awh::compressor::event_t::ENCODE, params, nullptr);
	// Проверяем что потоковая сессия заведена
	ASSERT_TRUE(encoder.valid());
	// Буфер выхода очередной порции
	std::string part;
	/**
	 * Подача пустого указателя при ненулевом размере: работа обязана отвергнуть
	 * её признаком, а не обращением по пустому объекту журнала
	 */
	encoder.push(nullptr, text.size(), part, awh::compressor::flush_t::NONE);
	// Проверяем что ошибочная подача выхода не дала
	ASSERT_TRUE(part.empty());
	// Проверяем что сессия ошибочной подачей не сломана
	ASSERT_TRUE(encoder.valid());
	// Результат компрессии данных
	std::string compressed;
	// Выполняем подачу всех данных одной порцией с завершением кадра
	encoder.push(text.data(), text.size(), part, awh::compressor::flush_t::FINISH);
	// Добавляем полученную порцию к результату
	compressed.append(part);
	// Проверяем что компрессия выполнена
	ASSERT_FALSE(compressed.empty());
	// Создаём потоковую сессию декомпрессии с пустым объектом работы с логами
	awh::compressor::stream_t decoder(awh::compressor::method_t::DEFLATE, awh::compressor::event_t::DECODE, params, nullptr);
	// Проверяем что потоковая сессия заведена
	ASSERT_TRUE(decoder.valid());
	// Результат декомпрессии данных
	std::string restored;
	// Выполняем подачу всех данных одной порцией с завершением кадра
	decoder.push(compressed.data(), compressed.size(), part, awh::compressor::flush_t::FINISH);
	// Добавляем полученную порцию к результату
	restored.append(part);
	// Проверяем что данные восстановлены
	ASSERT_EQ(text, restored);
}
