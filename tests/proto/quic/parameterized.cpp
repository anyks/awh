/**
 * @file parameterized.cpp
 * @date 2026-07-21
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
 * @brief Параметризованные тесты протокола QUIC — прогон подготовленных наборов входных данных через методы модуля с
 *        проверкой кодирования целых переменной длины, разбора пакетов и фреймов и вывода криптографических ключей
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic;

/**
 * @brief Структура параметров теста границ кодека целых чисел переменной длины
 *
 */
struct QuicVarintBoundaryParameter {
	// Кодируемое число
	uint64_t value;
	// Ожидаемый размер кодирования в октетах
	size_t size;
};

/**
 * @brief Класс фикстуры теста границ кодека целых чисел переменной длины
 *
 */
class QuicVarintBoundaryParameterizedFixture : public QuicFixture, public ::testing::WithParamInterface <QuicVarintBoundaryParameter> {
	public:
		// Параметры теста границ кодека
		QuicVarintBoundaryParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования полного цикла записи и чтения чисел на границах кодирования
 *
 */
TEST_P(QuicVarintBoundaryParameterizedFixture, RoundTrip){
	// Выходной буфер записи
	std::string output;
	// Записываем число
	ASSERT_EQ(varint::write(output, this->_parameter.value), this->_parameter.size);
	// Проверяем размер записанного буфера
	ASSERT_EQ(output.size(), this->_parameter.size);
	// Прочитанное число
	uint64_t value = 0;
	// Читаем записанное число
	ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (output.data()), output.size(), value), this->_parameter.size);
	// Проверяем совпадение значений
	ASSERT_EQ(value, this->_parameter.value);
	// Проверяем определение размера кодирования
	ASSERT_EQ(varint::size(this->_parameter.value), this->_parameter.size);
	// Проверяем что чтение усечённого буфера сообщает о нехватке данных
	if(this->_parameter.size > 1)
		// Читаем буфер без последнего октета
		ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (output.data()), output.size() - 1, value), 0u);
}

/**
 * Инстанцируем тесты границ кодека целых чисел переменной длины
 */
INSTANTIATE_TEST_SUITE_P(
	Quic,
	QuicVarintBoundaryParameterizedFixture,
	::testing::Values(
		// Нижняя граница одноктетного кодирования
		QuicVarintBoundaryParameter{0, 1},
		// Верхняя граница одноктетного кодирования
		QuicVarintBoundaryParameter{63, 1},
		// Нижняя граница двухоктетного кодирования
		QuicVarintBoundaryParameter{64, 2},
		// Верхняя граница двухоктетного кодирования
		QuicVarintBoundaryParameter{16383, 2},
		// Нижняя граница четырёхоктетного кодирования
		QuicVarintBoundaryParameter{16384, 4},
		// Верхняя граница четырёхоктетного кодирования
		QuicVarintBoundaryParameter{1073741823, 4},
		// Нижняя граница восьмиоктетного кодирования
		QuicVarintBoundaryParameter{1073741824, 8},
		// Верхняя граница восьмиоктетного кодирования (2^62 - 1)
		QuicVarintBoundaryParameter{0x3FFFFFFFFFFFFFFF, 8}
	)
);

/**
 * @brief Структура параметров теста вариантов фрейма STREAM
 *
 */
struct QuicStreamFrameParameter {
	// Идентификатор потока
	uint64_t streamId;
	// Смещение данных в потоке
	uint64_t offset;
	// Флаг завершения потока (FIN)
	bool fin;
	// Данные потока приложения
	const char * data;
};

/**
 * @brief Класс фикстуры теста вариантов фрейма STREAM
 *
 */
class QuicStreamFrameParameterizedFixture : public QuicFixture, public ::testing::WithParamInterface <QuicStreamFrameParameter> {
	public:
		// Параметры теста вариантов фрейма STREAM
		QuicStreamFrameParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования полного цикла сборки и разбора вариантов фрейма STREAM
 *
 */
TEST_P(QuicStreamFrameParameterizedFixture, RoundTrip){
	// Выходной буфер сборки
	std::string output;
	// Собираем фрейм STREAM согласно параметрам
	frame::serialize::stream(output, this->_parameter.streamId, this->_parameter.offset, this->_parameter.data, this->_parameter.fin);
	// Разобранный фрейм STREAM
	frame::stream_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	awh::quic::error_t error = awh::quic::error_t::NO_ERROR;
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::stream(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем что фрейм потреблён целиком
	ASSERT_EQ(consumed, output.size());
	// Проверяем идентификатор потока
	ASSERT_EQ(parsed.streamId, this->_parameter.streamId);
	// Проверяем смещение данных в потоке
	ASSERT_EQ(parsed.offset, this->_parameter.offset);
	// Проверяем флаг завершения потока
	ASSERT_EQ(parsed.fin, this->_parameter.fin);
	// Проверяем данные потока приложения
	ASSERT_EQ(parsed.data, this->_parameter.data);
}

/**
 * Инстанцируем тесты вариантов фрейма STREAM
 */
INSTANTIATE_TEST_SUITE_P(
	Quic,
	QuicStreamFrameParameterizedFixture,
	::testing::Values(
		// Клиентский двунаправленный поток без смещения
		QuicStreamFrameParameter{0, 0, false, "first-chunk"},
		// Поток со смещением данных
		QuicStreamFrameParameter{4, 1024, false, "middle-chunk"},
		// Завершающий фрагмент потока
		QuicStreamFrameParameter{8, 65536, true, "last-chunk"},
		// Пустой завершающий фрагмент (только FIN)
		QuicStreamFrameParameter{12, 2048, true, ""},
		// Большой идентификатор потока с большим смещением
		QuicStreamFrameParameter{0x3FFFFFFF, 0x3FFFFFFF, false, "big-ids"},
		// Серверный однонаправленный поток
		QuicStreamFrameParameter{3, 0, true, "server-uni"}
	)
);
