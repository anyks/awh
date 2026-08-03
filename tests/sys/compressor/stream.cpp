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
 * @brief Тесты потоковой компрессии — проверка инкрементального сжатия и распаковки данных порциями,
 *        режимов сброса буфера и финализации потока
 *
 * @copyright: Copyright © 2026
 *
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
 *
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
 *
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
 * @brief Тест параметризованной проверки существа принудительного выдавливания
 *
 * @details Все прежние проверки сброса смотрели лишь на обратимость всего обмена целиком,
 *          а её обеспечивает финализация: кодер, обративший `flush_t::SYNC` в холостой ход,
 *          прошёл бы их все до единой. Между тем сброс затем и заведён, чтобы поданное
 *          дошло до принимающей стороны не дожидаясь конца потока — на том стоит всякий
 *          обмен сообщениями. Закрепляем именно это: подаём половину данных, выдавливаем
 *          накопленное и требуем, чтобы распаковка выдала эту половину целиком ещё до
 *          того, как поток будет завершён
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamFlushDeliversBeforeFinishTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Определяем размер первой половины данных
	const size_t half = (data.size() / 2);
	// Буфер выхода порции
	std::string part = "";
	// Результат компрессии данных
	std::string compressed = "";
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::ENCODE);
	// Проверяем что поток компрессии валиден
	ASSERT_TRUE(encoder.valid()) << "Method: " << this->_parameter.name;
	// Подаём первую половину данных в поток компрессии
	encoder.push(data.data(), half, part);
	// Дописываем полученный выход в результат компрессии
	compressed.append(part);
	// Принудительно выдавливаем накопленное движком
	encoder.flush(part);
	// Дописываем выдавленные данные в результат компрессии
	compressed.append(part);
	// Проверяем что выдавливание что-то выдало наружу
	ASSERT_FALSE(compressed.empty()) << "Method: " << this->_parameter.name;
	// Результат декомпрессии данных
	std::string restored = "";
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::DECODE);
	// Проверяем что поток декомпрессии валиден
	ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name;
	/**
	 * Подаём распаковке всё выдавленное и ничего сверх того: поток намеренно
	 * не завершается, ведь проверяется именно доставка до конца потока
	 */
	decoder.push(compressed.data(), compressed.size(), part);
	// Дописываем полученный выход в результат декомпрессии
	restored.append(part);
	/**
	 * BZip2 стоит здесь особняком, и это свойство самой библиотеки, а не модуля:
	 * сжатие по `BZ_FLUSH` выдавленное отдаёт, а вот `BZ2_bzDecompress` его целиком
	 * принимает и не выдаёт наружу ни октета, дожидаясь конца потока. Проверено
	 * отдельным опытом на голом libbz2, мимо модуля. Обходить это своими силами
	 * нечем, поэтому расхождение закрепляется как есть: изменись оно у библиотеки
	 * либо у модуля — проверка о том сообщит
	 */
	if(this->_parameter.method == awh::compressor::method_t::BZIP2){
		// Проверяем что BZip2 до конца потока наружу ничего не отдаёт
		ASSERT_TRUE(restored.empty()) << "Method: " << this->_parameter.name;
		// Проверяем что поток распаковки при этом остался живым
		ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name;
		// Дальнейшая проверка доставки к BZip2 неприменима
		return;
	}
	// Проверяем что первая половина данных дошла до принимающей стороны целиком
	ASSERT_EQ(data.substr(0, half), restored) << "Method: " << this->_parameter.name;
	// Проверяем что поток распаковки завершённым себя не объявил
	ASSERT_FALSE(decoder.done()) << "Method: " << this->_parameter.name;
	// Проверяем что поток распаковки остался живым
	ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name;
}

/**
 * @brief Тест параметризованной проверки уровня компрессии за границами допустимого
 *
 * @details Конструктор потоковой сессии открыт наружу, и уровень приходит к нему
 *          произвольным целым — ровно так же, как приходил размер скользящего окна.
 *          Окно сторожится самим конструктором, уровень же отдан на попечение кодеров,
 *          и обходятся они с ним по-разному: часть подменяет негодное значение своим
 *          умолчанием, часть отдаёт движку и тот отвечает отказом. Оба исхода законны,
 *          недопустим лишь третий — работа, принявшая негодный уровень и выдавшая
 *          необратимые данные либо упавшая. Его и закрепляем: сессия либо не заводится
 *          вовсе, либо ведёт полный оборот без потерь
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamLevelOutOfRangeTest){
	// Формируем тестовые данные
	const std::string data = makeStreamPayload();
	// Список уровней, лежащих вне допустимого промежутка у всех движков разом
	const int32_t levels[] = {INT32_MIN, -1000, -2, 100, 1000, INT32_MAX};
	/**
	 * Выполняем перебор негодных значений уровня компрессии
	 */
	for(auto & level : levels){
		// Формируем параметры инициализации потоковой сессии
		awh::compressor::params_t params;
		// Устанавливаем допустимый размер скользящего окна
		params.wbits = 15;
		// Устанавливаем негодный уровень компрессии
		params.level = level;
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder(this->_parameter.method, awh::compressor::event_t::ENCODE, params, nullptr);
		/**
		 * Отказ в заведении сессии — исход законный: движок негодное значение отверг
		 */
		if(!encoder.valid())
			// Переходим к следующему значению
			continue;
		// Буфер выхода порции
		std::string part = "";
		// Результат компрессии данных
		std::string compressed = "";
		// Выполняем подачу всех данных одной порцией с завершением кадра
		encoder.push(data.data(), data.size(), part, awh::compressor::flush_t::FINISH);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
		// Проверяем что сессия компрессии подачу пережила
		ASSERT_TRUE(encoder.valid()) << "Method: " << this->_parameter.name << ", level = " << level;
		// Проверяем что компрессия что-то выдала
		ASSERT_FALSE(compressed.empty()) << "Method: " << this->_parameter.name << ", level = " << level;
		// Создаём потоковую сессию декомпрессии на умолчаниях
		awh::compressor::stream_t decoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::DECODE);
		// Проверяем что поток декомпрессии валиден
		ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name << ", level = " << level;
		// Результат декомпрессии данных
		std::string restored = "";
		// Подаём весь сжатый буфер в поток декомпрессии
		decoder.push(compressed.data(), compressed.size(), part, awh::compressor::flush_t::FINISH);
		// Дописываем полученный выход в результат декомпрессии
		restored.append(part);
		// Проверяем что данные восстановлены без потерь
		ASSERT_EQ(data, restored) << "Method: " << this->_parameter.name << ", level = " << level;
	}
}

/**
 * @brief Тест параметризованной проверки подачи по одному октету
 *
 * @details Мельчайшая из возможных подач: у части кодеров рабочий буфер и признаки
 *          завершения считаются от размера принятого, и подача в один октет проходит
 *          по тем ветвям, куда порция покрупнее не заходит
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamSingleOctetChunkTest){
	// Формируем тестовые данные небольшого объёма
	const std::string data = makeStreamPayload().substr(0, 4096);
	// Буфер выхода порции
	std::string part = "";
	// Результат компрессии данных
	std::string compressed = "";
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::ENCODE);
	// Проверяем что поток компрессии валиден
	ASSERT_TRUE(encoder.valid()) << "Method: " << this->_parameter.name;
	/**
	 * Подаём данные по одному октету
	 */
	for(size_t i = 0; i < data.size(); i++){
		// Подаём очередной октет в поток компрессии
		encoder.push(data.data() + i, 1, part);
		// Дописываем полученный выход в результат компрессии
		compressed.append(part);
	}
	// Финализируем поток компрессии
	encoder.finish(part);
	// Дописываем хвост в результат компрессии
	compressed.append(part);
	// Проверяем что поток компрессии подачу пережил
	ASSERT_TRUE(encoder.valid()) << "Method: " << this->_parameter.name;
	// Результат декомпрессии данных
	std::string restored = "";
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::DECODE);
	// Проверяем что поток декомпрессии валиден
	ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name;
	/**
	 * Подаём сжатые данные распаковке тоже по одному октету
	 */
	for(size_t i = 0; i < compressed.size(); i++){
		// Подаём очередной октет в поток декомпрессии
		decoder.push(compressed.data() + i, 1, part);
		// Дописываем полученный выход в результат декомпрессии
		restored.append(part);
	}
	// Финализируем поток декомпрессии
	decoder.finish(part);
	// Дописываем остаток в результат декомпрессии
	restored.append(part);
	// Проверяем что восстановленные данные совпадают с исходными
	ASSERT_EQ(data, restored) << "Method: " << this->_parameter.name;
}

/**
 * @brief Тест параметризованной проверки подачи порцией крупнее рабочего буфера
 *
 * @details Рабочий буфер кодеров равен `AWH_COMPRESSOR_STREAM_CHUNK`, и подача крупнее
 *          его заставляет движок выдавать выход в несколько заходов — по той самой
 *          ветви, где выходной буфер заполняется целиком и цикл идёт на новый круг.
 *          Все прежние потоковые проверки подавали порциями заведомо мельче рабочего
 *          буфера, и эта ветвь у семи кодеров оставалась непройденной
 *
 */
TEST_P(CompressorStreamParameterizedFixture, StreamChunkLargerThanWorkBufferTest){
	// Формируем данные, заведомо крупнее рабочего буфера кодеров
	std::string data = "";
	// Резервируем память под буфер данных
	data.reserve(1024 * 1024);
	/**
	 * Наполняем буфер данными переменной повторяемости, чтобы выход не ужался
	 * до величины меньше рабочего буфера и ветвь была вправду пройдена
	 */
	for(uint32_t i = 0; data.size() < (1024 * 1024); i++){
		// Добавляем очередную порцию данных
		data.append("Anyks Framework large single push payload ");
		// Добавляем переменную часть порции данных
		data.append(std::to_string(i * 2654435761u));
	}
	// Буфер выхода порции
	std::string part = "";
	// Результат компрессии данных
	std::string compressed = "";
	// Создаём потоковую сессию компрессии
	awh::compressor::stream_t encoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::ENCODE);
	// Проверяем что поток компрессии валиден
	ASSERT_TRUE(encoder.valid()) << "Method: " << this->_parameter.name;
	// Подаём весь буфер одной порцией с завершением кадра
	encoder.push(data.data(), data.size(), part, awh::compressor::flush_t::FINISH);
	// Дописываем полученный выход в результат компрессии
	compressed.append(part);
	// Проверяем что компрессия что-то выдала
	ASSERT_FALSE(compressed.empty()) << "Method: " << this->_parameter.name;
	// Результат декомпрессии данных
	std::string restored = "";
	// Создаём потоковую сессию декомпрессии
	awh::compressor::stream_t decoder = this->_compressor->stream(this->_parameter.method, awh::compressor::event_t::DECODE);
	// Проверяем что поток декомпрессии валиден
	ASSERT_TRUE(decoder.valid()) << "Method: " << this->_parameter.name;
	// Подаём весь сжатый буфер одной порцией с завершением кадра
	decoder.push(compressed.data(), compressed.size(), part, awh::compressor::flush_t::FINISH);
	// Дописываем полученный выход в результат декомпрессии
	restored.append(part);
	// Проверяем что восстановленные данные совпадают с исходными
	ASSERT_EQ(data, restored) << "Method: " << this->_parameter.name;
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
