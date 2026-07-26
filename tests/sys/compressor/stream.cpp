/**
 * @file: stream.cpp
 * @date: 2026-07-13
 * @license: LicenseRef-AWH-1.0
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <algorithm>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "compressor.hpp"

/**
 * @brief Функция формирования тестовых данных для потоковой компрессии
 *
 * @details Формирует смесь текстовой (хорошо сжимаемой) и бинарной (включая нулевые байты)
 *          нагрузки, чтобы проверить корректность работы потока на произвольных данных.
 *
 * @return тестовые данные
 */
static std::string makeStreamPayload() noexcept {
	// Результирующий буфер данных
	std::string data = "";
	/**
	 * Наполняем буфер повторяющейся текстовой нагрузкой (проверяем сжатие избыточных данных)
	 */
	for(size_t i = 0; i < 48; i++)
		// Дописываем очередной текстовый сегмент
		data.append("Anyks Framework streaming compression test payload segment. ");
	/**
	 * Наполняем буфер бинарной нагрузкой, включая нулевые байты (проверяем бинарную безопасность)
	 */
	for(size_t i = 0; i < 1024; i++)
		// Дописываем очередной байт полного диапазона значений
		data.push_back(static_cast <char> (i % 256));
	// Выводим сформированные тестовые данные
	return data;
}

/**
 * @brief Функция выполнения полного цикла потоковой компрессии/декомпрессии по порциям
 *
 * @param compressor объект-фабрика потоковых сессий
 * @param method     метод компрессии
 * @param data       исходные данные
 * @param chunk      размер порции обработки
 * @param flush      режим сброса данных при подаче порций
 * @return           восстановленные после декомпрессии данные
 */
static std::string streamRoundTrip(awh::compressor::block_t * compressor, const awh::compressor::method_t method, const std::string & data, const size_t chunk, const awh::compressor::flush_t flush = awh::compressor::flush_t::NONE) noexcept {
	// Буфер готового выхода очередной порции
	std::string part = "";
	// Результат компрессии данных
	std::string compressed = "";
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = compressor->stream(method, awh::compressor::event_t::ENCODE);
	// Если поток компрессии невалиден - возвращаем пустой результат
	if(!encoder.valid())
		// Возвращаем пустой результат
		return std::string();
	/**
	 * Выполняем компрессию данных порциями
	 */
	for(size_t i = 0; i < data.size(); i += chunk){
		// Вычисляем размер очередной порции
		const size_t n = std::min(chunk, data.size() - i);
		// Подаём порцию данных в поток компрессии
		encoder.push(data.data() + i, n, part, flush);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
	}
	// Финализируем поток компрессии
	encoder.finish(part);
	// Дописываем хвост в результат компрессии
	compressed.append(part);
	// Результат декомпрессии данных
	std::string restored = "";
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = compressor->stream(method, awh::compressor::event_t::DECODE);
	// Если поток декомпрессии невалиден - возвращаем пустой результат
	if(!decoder.valid())
		// Возвращаем пустой результат
		return std::string();
	/**
	 * Выполняем декомпрессию данных порциями
	 */
	for(size_t i = 0; i < compressed.size(); i += chunk){
		// Вычисляем размер очередной порции
		const size_t n = std::min(chunk, compressed.size() - i);
		// Подаём порцию данных в поток декомпрессии
		decoder.push(compressed.data() + i, n, part);
		// Дописываем полученный выход в результат декомпрессии
		restored.append(part);
	}
	// Финализируем поток декомпрессии
	decoder.finish(part);
	// Дописываем остаток в результат декомпрессии
	restored.append(part);
	// Выводим восстановленные данные
	return restored;
}

/**
 * @brief Тест проверки флагов поддержки потокового режима методами компрессии
 *
 */
TEST_F(CompressorFixture, StreamableFlagsTest){
	// Проверяем что метод LZ4 поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::LZ4));
	// Проверяем что метод LZMA поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::LZMA));
	// Проверяем что метод ZSTD поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::ZSTD));
	// Проверяем что метод GZIP поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::GZIP));
	// Проверяем что метод ZLIB поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::ZLIB));
	// Проверяем что метод BZIP2 поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::BZIP2));
	// Проверяем что метод BROTLI поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::BROTLI));
	// Проверяем что метод LIZARD поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::LIZARD));
	// Проверяем что метод DEFLATE поддерживает потоковый режим
	ASSERT_TRUE(awh::compressor::block_t::streamable(awh::compressor::method_t::DEFLATE));
	// Проверяем что метод SNAPPY не поддерживает потоковый режим
	ASSERT_FALSE(awh::compressor::block_t::streamable(awh::compressor::method_t::SNAPPY));
	// Проверяем что метод DENSITY не поддерживает потоковый режим
	ASSERT_FALSE(awh::compressor::block_t::streamable(awh::compressor::method_t::DENSITY));
	// Проверяем что неустановленный метод не поддерживает потоковый режим
	ASSERT_FALSE(awh::compressor::block_t::streamable(awh::compressor::method_t::NONE));
}

/**
 * @brief Тест проверки состояния пустого (по умолчанию) потока
 *
 */
TEST_F(CompressorFixture, StreamDefaultInvalidTest){
	// Создаём пустой поток через конструктор по умолчанию
	awh::compressor::stream_t stream;
	// Проверяем что пустой поток невалиден
	ASSERT_FALSE(stream.valid());
	// Проверяем что направление операции пустого потока не установлено
	ASSERT_EQ(stream.event(), awh::compressor::event_t::NONE);
	// Проверяем что метод компрессии пустого потока не установлен
	ASSERT_EQ(stream.method(), awh::compressor::method_t::NONE);
}

/**
 * @brief Тест проверки создания невалидного потока для неподдерживаемого метода
 *
 */
TEST_F(CompressorFixture, StreamUnsupportedMethodTest){
	// Создаём потоковую сессию для метода, не поддерживающего потоковый режим
	awh::compressor::stream_t stream = this->_compressor->stream(awh::compressor::method_t::SNAPPY, awh::compressor::event_t::ENCODE);
	// Проверяем что поток невалиден
	ASSERT_FALSE(stream.valid());
}

/**
 * @brief Тест проверки корректности состояния валидного потока
 *
 */
TEST_F(CompressorFixture, StreamStateTest){
	// Создаём потоковую сессию компрессии GZIP
	awh::compressor::stream_t encoder = this->_compressor->stream(awh::compressor::method_t::GZIP, awh::compressor::event_t::ENCODE);
	// Проверяем что поток валиден
	ASSERT_TRUE(encoder.valid());
	// Проверяем что поток ещё не финализирован
	ASSERT_FALSE(encoder.done());
	// Проверяем что направление операции соответствует компрессии
	ASSERT_EQ(encoder.event(), awh::compressor::event_t::ENCODE);
	// Проверяем что метод компрессии соответствует GZIP
	ASSERT_EQ(encoder.method(), awh::compressor::method_t::GZIP);
	// Буфер выхода порции
	std::string part = "";
	// Подаём порцию данных в поток
	encoder.push("Hello streaming world", 21, part);
	// Финализируем поток компрессии
	encoder.finish(part);
	// Проверяем что поток финализирован
	ASSERT_TRUE(encoder.done());
}

/**
 * @brief Тест проверки семантики перемещения потока через конструктор
 *
 */
TEST_F(CompressorFixture, StreamMoveConstructTest){
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t source = this->_compressor->stream(awh::compressor::method_t::ZSTD, awh::compressor::event_t::ENCODE);
	// Проверяем что исходный поток валиден
	ASSERT_TRUE(source.valid());
	// Перемещаем поток через конструктор перемещения
	awh::compressor::stream_t moved(std::move(source));
	// Проверяем что перемещённый поток валиден
	ASSERT_TRUE(moved.valid());
	// Проверяем что исходный поток стал невалидным после перемещения
	ASSERT_FALSE(source.valid());
	// Проверяем что перемещённый поток сохранил метод компрессии
	ASSERT_EQ(moved.method(), awh::compressor::method_t::ZSTD);
	// Проверяем что перемещённый поток сохранил направление операции
	ASSERT_EQ(moved.event(), awh::compressor::event_t::ENCODE);
}

/**
 * @brief Тест проверки семантики перемещения потока через оператор присваивания
 *
 */
TEST_F(CompressorFixture, StreamMoveAssignTest){
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t source = this->_compressor->stream(awh::compressor::method_t::BROTLI, awh::compressor::event_t::ENCODE);
	// Проверяем что исходный поток валиден
	ASSERT_TRUE(source.valid());
	// Создаём пустой поток-приёмник
	awh::compressor::stream_t target;
	// Проверяем что поток-приёмник изначально невалиден
	ASSERT_FALSE(target.valid());
	// Перемещаем поток через оператор присваивания перемещением
	target = std::move(source);
	// Проверяем что поток-приёмник стал валидным
	ASSERT_TRUE(target.valid());
	// Проверяем что исходный поток стал невалидным после перемещения
	ASSERT_FALSE(source.valid());
	// Проверяем что поток-приёмник получил метод компрессии
	ASSERT_EQ(target.method(), awh::compressor::method_t::BROTLI);
	// Проверяем что исходный поток сбросил метод компрессии
	ASSERT_EQ(source.method(), awh::compressor::method_t::NONE);
	// Проверяем что исходный поток сбросил направление операции
	ASSERT_EQ(source.event(), awh::compressor::event_t::NONE);
}

/**
 * @brief Тест проверки восстановления данных при подаче целого буфера одной порцией
 *
 */
TEST_F(CompressorFixture, StreamSinglePushTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Выполняем полный цикл компрессии/декомпрессии с подачей всех данных одной порцией
	const std::string restored = streamRoundTrip(this->_compressor.get(), awh::compressor::method_t::GZIP, data, data.size());
	// Проверяем что восстановленные данные совпадают с исходными
	ASSERT_EQ(restored, data);
}

/**
 * @brief Тест проверки восстановления данных при обработке в контейнер std::vector
 *
 */
TEST_F(CompressorFixture, StreamVectorContainerTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Буфер выхода очередной порции в виде вектора байт
	std::vector <uint8_t> part;
	// Результат компрессии данных
	std::vector <uint8_t> compressed;
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = this->_compressor->stream(awh::compressor::method_t::ZSTD, awh::compressor::event_t::ENCODE);
	// Проверяем что поток компрессии валиден
	ASSERT_TRUE(encoder.valid());
	// Размер порции обработки
	const size_t chunk = 32;
	/**
	 * Выполняем компрессию данных порциями
	 */
	for(size_t i = 0; i < data.size(); i += chunk){
		// Вычисляем размер очередной порции
		const size_t n = std::min(chunk, data.size() - i);
		// Подаём порцию данных в поток компрессии
		encoder.push(data.data() + i, n, part);
		// Дописываем полученный выход в результат компрессии
		compressed.insert(compressed.end(), part.begin(), part.end());
	}
	// Финализируем поток компрессии
	encoder.finish(part);
	// Дописываем хвост в результат компрессии
	compressed.insert(compressed.end(), part.begin(), part.end());
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Результат декомпрессии данных
	std::vector <uint8_t> restored;
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = this->_compressor->stream(awh::compressor::method_t::ZSTD, awh::compressor::event_t::DECODE);
	// Проверяем что поток декомпрессии валиден
	ASSERT_TRUE(decoder.valid());
	/**
	 * Выполняем декомпрессию данных порциями
	 */
	for(size_t i = 0; i < compressed.size(); i += chunk){
		// Вычисляем размер очередной порции
		const size_t n = std::min(chunk, compressed.size() - i);
		// Подаём порцию данных в поток декомпрессии
		decoder.push(compressed.data() + i, n, part);
		// Дописываем полученный выход в результат декомпрессии
		restored.insert(restored.end(), part.begin(), part.end());
	}
	// Финализируем поток декомпрессии
	decoder.finish(part);
	// Дописываем остаток в результат декомпрессии
	restored.insert(restored.end(), part.begin(), part.end());
	// Проверяем что размер восстановленных данных совпадает с исходным
	ASSERT_EQ(restored.size(), data.size());
	// Проверяем что восстановленные данные совпадают с исходными
	ASSERT_TRUE(std::equal(restored.begin(), restored.end(), reinterpret_cast <const uint8_t *> (data.data())));
}

/**
 * @brief Параметры теста потоковой компрессии/декомпрессии
 *
 */
struct CompressorStreamParameter {
	// Название метода компрессии (для читаемости вывода тестов)
	std::string name = "";
	// Метод компрессии
	awh::compressor::method_t method = awh::compressor::method_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для потоковой компрессии/декомпрессии
 *
 */
class CompressorStreamParameterizedFixture : public CompressorFixture, public ::testing::WithParamInterface <CompressorStreamParameter> {
	public:
		// Параметры теста
		CompressorStreamParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного полного цикла потоковой компрессии/декомпрессии по порциям
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamRoundTripTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Набор размеров порций для проверки устойчивости к произвольным границам порций
	const std::vector <size_t> chunks = {1, 3, 16, 64, 512, data.size()};
	/**
	 * Перебираем размеры порций обработки
	 */
	for(const size_t chunk : chunks){
		// Выполняем полный цикл компрессии/декомпрессии для текущего размера порции
		const std::string restored = streamRoundTrip(this->_compressor.get(), this->_parameter.method, data, chunk);
		// Проверяем что восстановленные данные совпадают с исходными для текущего размера порции
		ASSERT_EQ(restored, data) << "Method: " << this->_parameter.name << ", chunk: " << chunk;
	}
}

/**
 * @brief Тест параметризованного полного цикла потоковой компрессии/декомпрессии с промежуточным сбросом (SYNC-flush)
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamSyncFlushRoundTripTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Выполняем полный цикл компрессии/декомпрессии с принудительным сбросом на границах порций
	const std::string restored = streamRoundTrip(this->_compressor.get(), this->_parameter.method, data, 24, awh::compressor::flush_t::SYNC);
	// Проверяем что восстановленные данные совпадают с исходными
	ASSERT_EQ(restored, data) << "Method: " << this->_parameter.name;
}

/**
 * @brief Тест параметризованной проверки принудительного выдавливания накопленного через метод flush
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamExplicitFlushTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Буфер выхода порции
	std::string part = "";
	// Результат компрессии данных
	std::string compressed = "";
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::ENCODE);
	// Проверяем что поток компрессии валиден
	ASSERT_TRUE(encoder.valid()) << "Method: " << this->_parameter.name;
	// Подаём первую половину данных в поток компрессии
	encoder.push(data.data(), (data.size() / 2), part);
	// Дописываем полученный выход в результат компрессии
	compressed.append(part);
	// Принудительно выдавливаем накопленные данные через метод flush
	encoder.flush(part);
	// Дописываем выдавленные данные в результат компрессии
	compressed.append(part);
	// Подаём вторую половину данных в поток компрессии
	encoder.push(data.data() + (data.size() / 2), (data.size() - (data.size() / 2)), part);
	// Дописываем полученный выход в результат компрессии
	compressed.append(part);
	// Финализируем поток компрессии
	encoder.finish(part);
	// Дописываем хвост в результат компрессии
	compressed.append(part);
	// Результат декомпрессии данных
	std::string restored = "";
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::DECODE);
	// Проверяем что поток декомпрессии валиден
	ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name;
	// Подаём весь сжатый буфер в поток декомпрессии
	decoder.push(compressed.data(), compressed.size(), part);
	// Дописываем полученный выход в результат декомпрессии
	restored.append(part);
	// Финализируем поток декомпрессии
	decoder.finish(part);
	// Дописываем остаток в результат декомпрессии
	restored.append(part);
	// Проверяем что восстановленные данные совпадают с исходными
	ASSERT_EQ(restored, data) << "Method: " << this->_parameter.name;
}

/**
 * @brief Инициализация параметров теста потоковой компрессии/декомпрессии
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, CompressorStreamParameterizedFixture,
	::testing::Values(
		CompressorStreamParameter({"GZIP", awh::compressor::method_t::GZIP}),
		CompressorStreamParameter({"DEFLATE", awh::compressor::method_t::DEFLATE}),
		CompressorStreamParameter({"ZLIB", awh::compressor::method_t::ZLIB}),
		CompressorStreamParameter({"ZSTD", awh::compressor::method_t::ZSTD}),
		CompressorStreamParameter({"BROTLI", awh::compressor::method_t::BROTLI}),
		CompressorStreamParameter({"LZMA", awh::compressor::method_t::LZMA}),
		CompressorStreamParameter({"BZIP2", awh::compressor::method_t::BZIP2}),
		CompressorStreamParameter({"LZ4", awh::compressor::method_t::LZ4}),
		CompressorStreamParameter({"LIZARD", awh::compressor::method_t::LIZARD})
	)
);
