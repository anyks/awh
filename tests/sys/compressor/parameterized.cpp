/**
 * @file: parameterized.cpp
 * @date: 2026-01-21
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
 * Подключаем заголовочный файлы проекта
 */
#include "compressor.hpp"

/**
 * @brief Параметры теста выполнения компрессии/декомпрессии
 *
 */
struct CompressorTestParameter {
	// Текст для компрессии
	std::string text = "";
	// Размер скользящего окна
	int16_t wbits = 0;
	// Уровень компрессии
	awh::compressor::level_t level = awh::compressor::level_t::NONE;
	// Метод компрессии
	awh::compressor::method_t method = awh::compressor::method_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с компрессией/декомпрессией
 *
 */
class CompressorTestParameterizedFixture : public CompressorFixture, public ::testing::WithParamInterface <CompressorTestParameter> {
	public:
		// Параметры теста
		CompressorTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного создания и проверки работы компрессии/декомпрессии
 *
 */
TEST_P(CompressorTestParameterizedFixture, CompressorTest){
	// Устанавливаем уровень компрессии
	this->_compressor->level(this->_parameter.level);
	// Устанавливаем размер скользящего окна GZip
	this->_compressor->wbitsGZip(this->_parameter.wbits);
	// Выполняем компрессию данных
	std::vector <uint8_t> compressed = std::move(this->_compressor->compress <std::vector <uint8_t>> (this->_parameter.text, this->_parameter.method));
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Проверяем что результат декомпрессии совпадает с исходным текстом
	ASSERT_EQ(this->_parameter.text, this->_compressor->decompress <std::string> (compressed, this->_parameter.method));
}

/**
 * @brief Инициализация параметров теста работы компрессии/декомпрессии
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, CompressorTestParameterizedFixture,
	::testing::Values(
		CompressorTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST,
			awh::compressor::method_t::LZ4
		}),
		CompressorTestParameter({
			"Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::SPEED,
			awh::compressor::method_t::LZMA
		}),
		CompressorTestParameter({
			"Compression and Decompression Test, Compression and Decompression Test, Compression and Decompression Test!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::NORMAL,
			awh::compressor::method_t::GZIP
		}),
		CompressorTestParameter({
			"Parameterized Unit Test for Compressor Class, Parameterized Unit Test for Compressor Class, Parameterized Unit Test for Compressor Class!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST,
			awh::compressor::method_t::ZSTD
		}),
		CompressorTestParameter({
			"Unit Testing with Google Test Framework, Unit Testing with Google Test Framework, Unit Testing with Google Test Framework!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::SPEED,
			awh::compressor::method_t::BROTLI
		}),
		CompressorTestParameter({
			"Data Compression Techniques in C++, Data Compression Techniques in C++, Data Compression Techniques in C++!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::NORMAL,
			awh::compressor::method_t::BZIP2
		}),
		CompressorTestParameter({
			"Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST,
			awh::compressor::method_t::DEFLATE
		}),
		CompressorTestParameter({
			"Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::SPEED,
			awh::compressor::method_t::LIZARD
		}),
		CompressorTestParameter({
			"Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::NORMAL,
			awh::compressor::method_t::SNAPPY
		}),
		CompressorTestParameter({
			"Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST,
			awh::compressor::method_t::DENSITY
		})
	)
);

/**
 * @brief Параметры теста выполнения компрессии/декомпрессии с переиспользованием контекста
 *
 */
struct CompressorTakeoverTestParameter {
	// Текст для компрессии
	std::string text = "";
	// Размер скользящего окна
	int16_t wbits = 0;
	// Уровень компрессии
	awh::compressor::level_t level = awh::compressor::level_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с компрессией/декомпрессией с переиспользованием контекста
 *
 */
class CompressorTakeoverTestParameterizedFixture : public CompressorFixture, public ::testing::WithParamInterface <CompressorTakeoverTestParameter> {
	public:
		// Параметры теста
		CompressorTakeoverTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного создания и проверки работы компрессии/декомпрессии с переиспользованием контекста
 *
 */
TEST_P(CompressorTakeoverTestParameterizedFixture, CompressorTakeoverTest){
	// Устанавливаем уровень компрессии
	this->_compressor->level(this->_parameter.level);
	// Устанавливаем размер скользящего окна GZip
	this->_compressor->wbitsGZip(this->_parameter.wbits);
	// Включаем флаг переиспользования контекста компрессии/декомпрессии
	this->_compressor->takeoverGZip(awh::compressor::event_t::ENCODE, true);
	this->_compressor->takeoverGZip(awh::compressor::event_t::DECODE, true);
	// Выполняем компрессию данных
	std::vector <uint8_t> compressed = std::move(this->_compressor->compress <std::vector <uint8_t>> (this->_parameter.text, awh::compressor::method_t::DEFLATE));
	// Проверяем что результат компрессии не пустой
	ASSERT_FALSE(compressed.empty());
	// Проверяем что результат декомпрессии совпадает с исходным текстом
	ASSERT_EQ(this->_parameter.text, this->_compressor->decompress <std::string> (compressed, awh::compressor::method_t::DEFLATE));
}

/**
 * @brief Инициализация параметров теста работы компрессии/декомпрессии с переиспользованием контекста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, CompressorTakeoverTestParameterizedFixture,
	::testing::Values(
		CompressorTakeoverTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST
		}),
		CompressorTakeoverTestParameter({
			"Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::SPEED
		}),
		CompressorTakeoverTestParameter({
			"Compression and Decompression Test, Compression and Decompression Test, Compression and Decompression Test!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::NORMAL
		}),
		CompressorTakeoverTestParameter({
			"Parameterized Unit Test for Compressor Class, Parameterized Unit Test for Compressor Class, Parameterized Unit Test for Compressor Class!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST
		}),
		CompressorTakeoverTestParameter({
			"Unit Testing with Google Test Framework, Unit Testing with Google Test Framework, Unit Testing with Google Test Framework!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::SPEED
		}),
		CompressorTakeoverTestParameter({
			"Data Compression Techniques in C++, Data Compression Techniques in C++, Data Compression Techniques in C++!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::NORMAL
		}),
		CompressorTakeoverTestParameter({
			"Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval, Efficient Data Storage and Retrieval!!!!!!!!!!!!!!!!?",
			15,
			awh::compressor::level_t::BEST
		})
	)
);
