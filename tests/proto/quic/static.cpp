/**
 * @file: static.cpp
 * @date: 2026-07-21
 * @license: GPL-3.0
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
#include <cstring>
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
 * @brief Метод проверки эталонных значений кодека целых чисел переменной длины (RFC 9000 §A.1)
 *
 */
TEST_F(QuicFixture, VarintReferenceTest){
	// Прочитанное число
	uint64_t value = 0;
	// Эталон восьмиоктетного числа из RFC 9000 §16
	const std::string eight = this->unhex("c2197c5eff14e88c");
	// Читаем восьмиоктетное число
	ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (eight.data()), eight.size(), value), 8u);
	// Проверяем эталонное значение
	ASSERT_EQ(value, 151288809941952652ull);
	// Эталон четырёхоктетного числа из RFC 9000 §16
	const std::string four = this->unhex("9d7f3e7d");
	// Читаем четырёхоктетное число
	ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (four.data()), four.size(), value), 4u);
	// Проверяем эталонное значение
	ASSERT_EQ(value, 494878333ull);
	// Эталон двухоктетного числа из RFC 9000 §16
	const std::string two = this->unhex("7bbd");
	// Читаем двухоктетное число
	ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (two.data()), two.size(), value), 2u);
	// Проверяем эталонное значение
	ASSERT_EQ(value, 15293ull);
	// Эталон одноктетного числа из RFC 9000 §16
	const std::string one = this->unhex("25");
	// Читаем одноктетное число
	ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (one.data()), one.size(), value), 1u);
	// Проверяем эталонное значение
	ASSERT_EQ(value, 37ull);
	// Эталон того же числа в двухоктетном кодировании
	const std::string redundant = this->unhex("4025");
	// Читаем число в избыточном кодировании
	ASSERT_EQ(varint::read(reinterpret_cast <const uint8_t *> (redundant.data()), redundant.size(), value), 2u);
	// Проверяем эталонное значение
	ASSERT_EQ(value, 37ull);
}

/**
 * @brief Метод проверки записи чисел с минимальным кодированием
 *
 */
TEST_F(QuicFixture, VarintWriteTest){
	// Выходной буфер записи
	std::string output;
	// Записываем число 37
	ASSERT_EQ(varint::write(output, 37), 1u);
	// Проверяем минимальное кодирование
	ASSERT_EQ(this->hex(output), "25");
	// Очищаем выходной буфер
	output.clear();
	// Записываем число 15293
	ASSERT_EQ(varint::write(output, 15293), 2u);
	// Проверяем минимальное кодирование
	ASSERT_EQ(this->hex(output), "7bbd");
	// Очищаем выходной буфер
	output.clear();
	// Записываем число 494878333
	ASSERT_EQ(varint::write(output, 494878333), 4u);
	// Проверяем минимальное кодирование
	ASSERT_EQ(this->hex(output), "9d7f3e7d");
	// Очищаем выходной буфер
	output.clear();
	// Записываем число 151288809941952652
	ASSERT_EQ(varint::write(output, 151288809941952652ull), 8u);
	// Проверяем минимальное кодирование
	ASSERT_EQ(this->hex(output), "c2197c5eff14e88c");
	// Очищаем выходной буфер
	output.clear();
	// Записываем максимальное представимое число
	ASSERT_EQ(varint::write(output, proto::VARINT_MAX), 8u);
	// Записываем непредставимое число
	ASSERT_EQ(varint::write(output, proto::VARINT_MAX + 1), 0u);
}

/**
 * @brief Метод проверки восстановления полного номера пакета (RFC 9000 §A.3)
 *
 */
TEST_F(QuicFixture, DecodePacketNumberTest){
	// Эталон из RFC 9000 §A.3: largest 0xa82f30ea, усечённый 0x9b32 в двух октетах
	ASSERT_EQ(packet::decodePacketNumber(0xA82F30EA, 0x9B32, 2), 0xA82F9B32ull);
	// Первый пакет пространства номеров (largest отсутствует - ноль)
	ASSERT_EQ(packet::decodePacketNumber(0, 0, 1), 0ull);
	// Восстановление без перехода окна
	ASSERT_EQ(packet::decodePacketNumber(100, 101, 1), 101ull);
	// Восстановление с переходом в следующее окно
	ASSERT_EQ(packet::decodePacketNumber(255, 2, 1), 258ull);
}

/**
 * @brief Метод проверки выбора размера кодирования номера пакета (RFC 9000 §A.2)
 *
 */
TEST_F(QuicFixture, PacketNumberSizeTest){
	// Эталон из RFC 9000 §A.2: отправка 0xac5c02 при подтверждённом 0xabe8b3 - два октета
	ASSERT_EQ(packet::packetNumberSize(0xAC5C02, 0xABE8B3), 2u);
	// Первый пакет пространства номеров - один октет
	ASSERT_EQ(packet::packetNumberSize(0, 0), 1u);
	// Большой разрыв неподтверждённых пакетов - четыре октета
	ASSERT_EQ(packet::packetNumberSize(0x10000000, 0), 4u);
}

/**
 * @brief Метод проверки разбора защищённого клиентского пакета Initial (RFC 9001 §A.2)
 *
 */
TEST_F(QuicFixture, ParseClientInitialTest){
	// Заголовок эталонного защищённого пакета Initial из RFC 9001 §A.2 (до поля Packet Number)
	std::string packet = this->unhex("c000000001088394c8f03e5157080000449e");
	// Дополняем пакет защищённой частью до полного размера датаграммы (4 октета PN + 1162 нагрузки + 16 тег AEAD)
	packet.append(1182, '\x42');
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем заголовок пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (packet.data()), packet.size(), 0, header, error), status_t::OK);
	// Проверяем тип пакета
	ASSERT_EQ(header.type, packet_t::INITIAL);
	// Проверяем версию протокола
	ASSERT_EQ(header.version, proto::VERSION_1);
	// Проверяем длину идентификатора соединения получателя
	ASSERT_EQ(header.dcid.size, 8u);
	// Проверяем данные идентификатора соединения получателя
	ASSERT_EQ(::memcmp(header.dcid.data, this->unhex("8394c8f03e515708").data(), 8), 0);
	// Проверяем что идентификатор соединения отправителя пуст
	ASSERT_EQ(header.scid.size, 0u);
	// Проверяем что токен пакета Initial пуст
	ASSERT_TRUE(header.token.empty());
	// Проверяем значение поля Length (4 октета PN + 1162 нагрузки + 16 тег AEAD)
	ASSERT_EQ(header.length, 1182u);
	// Проверяем смещение поля Packet Number (граница защиты заголовка)
	ASSERT_EQ(header.pnOffset, 18u);
	// Проверяем полный размер пакета (минимальная датаграмма Initial)
	ASSERT_EQ(header.size, proto::MIN_INITIAL_SIZE);
	// Проверяем размер защищённой нагрузки
	ASSERT_EQ(header.payload.size(), 1182u);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора длинного заголовка
 *
 */
TEST_F(QuicFixture, LongHeaderRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Идентификаторы соединения
	const cid_t dcid = this->makeCid(this->unhex("0102030405060708"));
	const cid_t scid = this->makeCid(this->unhex("aabbccdd"));
	// Токен пакета Initial
	const std::string token = "sample-token";
	// Собираем длинный заголовок пакета Initial (Length: 2 октета PN + 100 нагрузки)
	ASSERT_TRUE(packet::serialize::longHeader(output, packet_t::INITIAL, proto::VERSION_1, dcid, scid, token, 102, 0x1234, 2));
	// Дополняем пакет нагрузкой (Length за вычетом номера пакета)
	output.append(100, '\x00');
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный заголовок
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (output.data()), output.size(), 0, header, error), status_t::OK);
	// Проверяем тип пакета
	ASSERT_EQ(header.type, packet_t::INITIAL);
	// Проверяем версию протокола
	ASSERT_EQ(header.version, proto::VERSION_1);
	// Проверяем идентификаторы соединения
	ASSERT_TRUE(header.dcid == dcid);
	ASSERT_TRUE(header.scid == scid);
	// Проверяем токен пакета Initial
	ASSERT_EQ(header.token, token);
	// Проверяем значение поля Length
	ASSERT_EQ(header.length, 102u);
	// Проверяем биты размера номера пакета в первом октете
	ASSERT_EQ(static_cast <size_t> (header.first & 0x03) + 1, 2u);
	// Читаем усечённый номер пакета (защита заголовка в тесте не применяется)
	uint64_t pn = 0;
	// Проверяем чтение номера пакета
	ASSERT_TRUE(packet::parser::packetNumber(reinterpret_cast <const uint8_t *> (output.data()) + header.pnOffset, output.size() - header.pnOffset, 2, pn));
	// Проверяем значение номера пакета
	ASSERT_EQ(pn, 0x1234u);
	// Собираем заголовок пакета Handshake без токена
	std::string handshake;
	// Проверяем что токен для Handshake запрещён
	ASSERT_FALSE(packet::serialize::longHeader(handshake, packet_t::HANDSHAKE, proto::VERSION_1, dcid, scid, token, 102, 1, 1));
	// Проверяем что сборка без токена успешна
	ASSERT_TRUE(packet::serialize::longHeader(handshake, packet_t::HANDSHAKE, proto::VERSION_1, dcid, scid, "", 102, 1, 1));
}

/**
 * @brief Метод проверки полного цикла сборки и разбора короткого заголовка
 *
 */
TEST_F(QuicFixture, ShortHeaderRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Идентификатор соединения получателя
	const cid_t dcid = this->makeCid(this->unhex("cafebabe01020304"));
	// Собираем короткий заголовок пакета 1-RTT
	ASSERT_TRUE(packet::serialize::shortHeader(output, dcid, 0xABCDEF, 3, true, false));
	// Дополняем пакет нагрузкой
	output.append(64, '\x00');
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный заголовок (длина идентификатора известна эндпоинту)
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (output.data()), output.size(), dcid.size, header, error), status_t::OK);
	// Проверяем тип пакета
	ASSERT_EQ(header.type, packet_t::ONE_RTT);
	// Проверяем идентификатор соединения получателя
	ASSERT_TRUE(header.dcid == dcid);
	// Проверяем смещение поля Packet Number
	ASSERT_EQ(header.pnOffset, 1u + dcid.size);
	// Проверяем бит фазы ключей в первом октете
	ASSERT_NE(header.first & 0x04, 0);
	// Проверяем биты размера номера пакета в первом октете
	ASSERT_EQ(static_cast <size_t> (header.first & 0x03) + 1, 3u);
	// Читаем усечённый номер пакета
	uint64_t pn = 0;
	// Проверяем чтение номера пакета
	ASSERT_TRUE(packet::parser::packetNumber(reinterpret_cast <const uint8_t *> (output.data()) + header.pnOffset, output.size() - header.pnOffset, 3, pn));
	// Проверяем значение номера пакета
	ASSERT_EQ(pn, 0xABCDEFu);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора пакета Version Negotiation
 *
 */
TEST_F(QuicFixture, VersionNegotiationRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Идентификаторы соединения
	const cid_t dcid = this->makeCid(this->unhex("0102030405060708"));
	const cid_t scid = this->makeCid(this->unhex("f0e0d0c0"));
	// Список поддерживаемых версий
	const uint32_t versions[] = {proto::VERSION_1, 0x709A50C4};
	// Собираем пакет Version Negotiation
	ASSERT_TRUE(packet::serialize::versionNegotiation(output, dcid, scid, versions, 2));
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный пакет
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (output.data()), output.size(), 0, header, error), status_t::OK);
	// Проверяем тип пакета
	ASSERT_EQ(header.type, packet_t::VERSION_NEGOTIATION);
	// Проверяем идентификаторы соединения
	ASSERT_TRUE(header.dcid == dcid);
	ASSERT_TRUE(header.scid == scid);
	// Список разобранных версий
	std::vector <uint32_t> parsed;
	// Разбираем список поддерживаемых версий
	ASSERT_EQ(packet::parser::versions(header, parsed, error), status_t::OK);
	// Проверяем количество версий
	ASSERT_EQ(parsed.size(), 2u);
	// Проверяем значения версий
	ASSERT_EQ(parsed[0], proto::VERSION_1);
	ASSERT_EQ(parsed[1], 0x709A50C4u);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора пакета Retry
 *
 */
TEST_F(QuicFixture, RetryRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Идентификаторы соединения
	const cid_t dcid = this->makeCid(this->unhex("8394c8f03e515708"));
	const cid_t scid = this->makeCid(this->unhex("f067a5502a4262b5"));
	// Токен для повторного пакета Initial
	const std::string token = "retry-token-data";
	// Тег целостности пакета Retry
	uint8_t tag[proto::RETRY_TAG_SIZE];
	// Заполняем тег целостности тестовыми данными
	::memset(tag, 0x5A, proto::RETRY_TAG_SIZE);
	// Собираем пакет Retry
	ASSERT_TRUE(packet::serialize::retry(output, proto::VERSION_1, dcid, scid, token, tag));
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный пакет
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (output.data()), output.size(), 0, header, error), status_t::OK);
	// Проверяем тип пакета
	ASSERT_EQ(header.type, packet_t::RETRY);
	// Разобранный токен и тег целостности
	std::string_view parsedToken;
	uint8_t parsedTag[proto::RETRY_TAG_SIZE];
	// Разбираем нагрузку пакета Retry
	ASSERT_EQ(packet::parser::retry(header, parsedToken, parsedTag, error), status_t::OK);
	// Проверяем токен для повторного пакета Initial
	ASSERT_EQ(parsedToken, token);
	// Проверяем тег целостности пакета Retry
	ASSERT_EQ(::memcmp(parsedTag, tag, proto::RETRY_TAG_SIZE), 0);
}

/**
 * @brief Метод проверки отклонения пакетов с некорректным заголовком
 *
 */
TEST_F(QuicFixture, MalformedPacketTest){
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Пакет со сброшенным fixed-битом длинного заголовка
	const std::string badFixed = this->unhex("8000000001088394c8f03e51570800000a");
	// Проверяем отклонение пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (badFixed.data()), badFixed.size(), 0, header, error), status_t::ERROR);
	// Проверяем код ошибки транспорта
	ASSERT_EQ(error, error_t::PROTOCOL_VIOLATION);
	// Пакет с длиной идентификатора соединения больше лимита QUIC v1
	const std::string badCid = this->unhex("c000000001ff");
	// Проверяем отклонение пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (badCid.data()), badCid.size(), 0, header, error), status_t::ERROR);
	// Пакет Initial с полем Length больше остатка датаграммы
	const std::string badLength = this->unhex("c000000001088394c8f03e51570800004064ffff");
	// Проверяем отклонение пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (badLength.data()), badLength.size(), 0, header, error), status_t::ERROR);
	// Усечённый пакет (не хватает данных версии)
	const std::string truncated = this->unhex("c0000000");
	// Проверяем неполноту данных
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (truncated.data()), truncated.size(), 0, header, error), status_t::INCOMPLETE);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора фрейма ACK
 *
 */
TEST_F(QuicFixture, AckFrameRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Фрейм подтверждения приёма пакетов
	frame::ack_t source;
	// Устанавливаем задержку подтверждения
	source.delay = 100;
	// Диапазон подтверждаемых пакетов
	frame::range_t range;
	// Первый диапазон - пакеты 95-100
	range.low  = 95;
	range.high = 100;
	// Дописываем первый диапазон
	source.ranges.push_back(range);
	// Второй диапазон - пакеты 80-90
	range.low  = 80;
	range.high = 90;
	// Дописываем второй диапазон
	source.ranges.push_back(range);
	// Третий диапазон - пакет 50
	range.low  = 50;
	range.high = 50;
	// Дописываем третий диапазон
	source.ranges.push_back(range);
	// Собираем фрейм ACK
	ASSERT_TRUE(frame::serialize::ack(output, source));
	// Разобранный фрейм ACK
	frame::ack_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::ack(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем что фрейм потреблён целиком
	ASSERT_EQ(consumed, output.size());
	// Проверяем отсутствие счётчиков ECN
	ASSERT_FALSE(parsed.hasEcn);
	// Проверяем задержку подтверждения
	ASSERT_EQ(parsed.delay, 100u);
	// Проверяем количество диапазонов
	ASSERT_EQ(parsed.ranges.size(), 3u);
	/**
	 * Проверяем границы всех диапазонов
	 */
	for(size_t i = 0; i < source.ranges.size(); i++){
		// Проверяем наименьший номер диапазона
		ASSERT_EQ(parsed.ranges[i].low, source.ranges[i].low);
		// Проверяем наибольший номер диапазона
		ASSERT_EQ(parsed.ranges[i].high, source.ranges[i].high);
	}
}

/**
 * @brief Метод проверки полного цикла сборки и разбора фрейма ACK со счётчиками ECN
 *
 */
TEST_F(QuicFixture, AckEcnFrameRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Фрейм подтверждения приёма пакетов
	frame::ack_t source;
	// Устанавливаем флаг наличия счётчиков ECN
	source.hasEcn = true;
	// Устанавливаем задержку подтверждения
	source.delay = 42;
	// Устанавливаем счётчики ECN
	source.ect0 = 10;
	source.ect1 = 20;
	source.ce   = 30;
	// Диапазон подтверждаемых пакетов
	frame::range_t range;
	// Единственный диапазон - пакеты 0-1000
	range.low  = 0;
	range.high = 1000;
	// Дописываем диапазон
	source.ranges.push_back(range);
	// Собираем фрейм ACK_ECN
	ASSERT_TRUE(frame::serialize::ack(output, source));
	// Разобранный фрейм ACK
	frame::ack_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::ack(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем наличие счётчиков ECN
	ASSERT_TRUE(parsed.hasEcn);
	// Проверяем счётчики ECN
	ASSERT_EQ(parsed.ect0, 10u);
	ASSERT_EQ(parsed.ect1, 20u);
	ASSERT_EQ(parsed.ce, 30u);
}

/**
 * @brief Метод проверки отклонения фрейма ACK с некорректными диапазонами
 *
 */
TEST_F(QuicFixture, AckFrameMalformedTest){
	// Фрейм ACK с первым диапазоном ниже нулевого номера пакета (largest 5, first range 10)
	const std::string underflow = this->unhex("0205000a0a");
	// Разобранный фрейм ACK
	frame::ack_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Проверяем отклонение фрейма
	ASSERT_EQ(frame::parser::ack(reinterpret_cast <const uint8_t *> (underflow.data()), underflow.size(), parsed, consumed, error), status_t::ERROR);
	// Проверяем код ошибки транспорта
	ASSERT_EQ(error, error_t::FRAME_ENCODING_ERROR);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора фрейма CRYPTO
 *
 */
TEST_F(QuicFixture, CryptoFrameRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Данные криптографического хендшейка
	const std::string data = "client-hello-handshake-data";
	// Собираем фрейм CRYPTO
	frame::serialize::crypto(output, 1024, data);
	// Разобранный фрейм CRYPTO
	frame::crypto_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::crypto(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем что фрейм потреблён целиком
	ASSERT_EQ(consumed, output.size());
	// Проверяем смещение данных в потоке хендшейка
	ASSERT_EQ(parsed.offset, 1024u);
	// Проверяем данные криптографического хендшейка
	ASSERT_EQ(parsed.data, data);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора фрейма NEW_CONNECTION_ID
 *
 */
TEST_F(QuicFixture, NewConnectionIdFrameRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Фрейм анонса нового идентификатора соединения
	frame::new_connection_id_t source;
	// Устанавливаем порядковые номера
	source.seq           = 7;
	source.retirePriorTo = 3;
	// Устанавливаем новый идентификатор соединения
	source.cid = this->makeCid(this->unhex("deadbeefcafe"));
	// Заполняем токен сброса тестовыми данными
	::memset(source.resetToken, 0x77, proto::RESET_TOKEN_SIZE);
	// Собираем фрейм NEW_CONNECTION_ID
	ASSERT_TRUE(frame::serialize::newConnectionId(output, source));
	// Разобранный фрейм NEW_CONNECTION_ID
	frame::new_connection_id_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::newConnectionId(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем что фрейм потреблён целиком
	ASSERT_EQ(consumed, output.size());
	// Проверяем порядковые номера
	ASSERT_EQ(parsed.seq, 7u);
	ASSERT_EQ(parsed.retirePriorTo, 3u);
	// Проверяем новый идентификатор соединения
	ASSERT_TRUE(parsed.cid == source.cid);
	// Проверяем токен сброса без сохранения состояния
	ASSERT_EQ(::memcmp(parsed.resetToken, source.resetToken, proto::RESET_TOKEN_SIZE), 0);
	// Проверяем отклонение фрейма с retirePriorTo больше seq
	frame::new_connection_id_t invalid = source;
	// Устанавливаем некорректный порядковый номер вывода
	invalid.retirePriorTo = 100;
	// Очищаем выходной буфер
	output.clear();
	// Проверяем что сборка отклонена
	ASSERT_FALSE(frame::serialize::newConnectionId(output, invalid));
}

/**
 * @brief Метод проверки полного цикла сборки и разбора фрейма CONNECTION_CLOSE
 *
 */
TEST_F(QuicFixture, ConnectionCloseFrameRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Причина завершения соединения
	const std::string reason = "protocol violation detected";
	// Собираем фрейм CONNECTION_CLOSE с ошибкой транспорта
	frame::serialize::connectionClose(output, static_cast <uint64_t> (error_t::PROTOCOL_VIOLATION), static_cast <uint64_t> (frame_t::CRYPTO), reason, false);
	// Разобранный фрейм CONNECTION_CLOSE
	frame::connection_close_t parsed;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::connectionClose(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем что фрейм содержит ошибку транспорта
	ASSERT_FALSE(parsed.app);
	// Проверяем код ошибки
	ASSERT_EQ(parsed.code, static_cast <uint64_t> (error_t::PROTOCOL_VIOLATION));
	// Проверяем тип фрейма, вызвавшего ошибку
	ASSERT_EQ(parsed.frameType, static_cast <uint64_t> (frame_t::CRYPTO));
	// Проверяем причину завершения
	ASSERT_EQ(parsed.reason, reason);
	// Очищаем выходной буфер
	output.clear();
	// Собираем фрейм CONNECTION_CLOSE с ошибкой приложения
	frame::serialize::connectionClose(output, 42, 0, "app-close", true);
	// Разбираем собранный фрейм
	ASSERT_EQ(frame::parser::connectionClose(reinterpret_cast <const uint8_t *> (output.data()), output.size(), parsed, consumed, error), status_t::OK);
	// Проверяем что фрейм содержит ошибку приложения
	ASSERT_TRUE(parsed.app);
	// Проверяем код ошибки приложения
	ASSERT_EQ(parsed.code, 42u);
}

/**
 * @brief Метод проверки разбора последовательности фреймов в нагрузке пакета
 *
 */
TEST_F(QuicFixture, FrameSequenceTest){
	// Выходной буфер сборки нагрузки пакета
	std::string output;
	// Собираем фрейм PING
	frame::serialize::ping(output);
	// Собираем фрейм STREAM с данными
	frame::serialize::stream(output, 4, 0, "hello", true);
	// Собираем фрейм MAX_DATA
	frame::serialize::single(output, frame_t::MAX_DATA, 1048576);
	// Собираем заполнение PADDING
	frame::serialize::padding(output, 10);
	// Буфер разбора нагрузки
	const uint8_t * data = reinterpret_cast <const uint8_t *> (output.data());
	// Размер нагрузки
	const size_t size = output.size();
	// Смещение разбора нагрузки
	size_t offset = 0;
	// Количество потреблённых октетов
	size_t consumed = 0;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Определённый тип фрейма
	frame_t type = frame_t::UNKNOWN;
	// Определяем тип первого фрейма
	ASSERT_TRUE(frame::parser::type(data, size, type));
	// Проверяем тип первого фрейма
	ASSERT_EQ(type, frame_t::PING);
	// Разбираем фрейм PING
	ASSERT_EQ(frame::parser::ping(data, size, consumed, error), status_t::OK);
	// Сдвигаем смещение разбора
	offset += consumed;
	// Определяем тип второго фрейма
	ASSERT_TRUE(frame::parser::type(data + offset, size - offset, type));
	// Проверяем тип второго фрейма
	ASSERT_EQ(type, frame_t::STREAM);
	// Разобранный фрейм STREAM
	frame::stream_t stream;
	// Разбираем фрейм STREAM
	ASSERT_EQ(frame::parser::stream(data + offset, size - offset, stream, consumed, error), status_t::OK);
	// Проверяем идентификатор потока
	ASSERT_EQ(stream.streamId, 4u);
	// Проверяем флаг завершения потока
	ASSERT_TRUE(stream.fin);
	// Проверяем данные потока
	ASSERT_EQ(stream.data, "hello");
	// Сдвигаем смещение разбора
	offset += consumed;
	// Определяем тип третьего фрейма
	ASSERT_TRUE(frame::parser::type(data + offset, size - offset, type));
	// Проверяем тип третьего фрейма
	ASSERT_EQ(type, frame_t::MAX_DATA);
	// Значение лимита данных соединения
	uint64_t value = 0;
	// Разбираем фрейм MAX_DATA
	ASSERT_EQ(frame::parser::single(data + offset, size - offset, frame_t::MAX_DATA, value, consumed, error), status_t::OK);
	// Проверяем значение лимита
	ASSERT_EQ(value, 1048576u);
	// Сдвигаем смещение разбора
	offset += consumed;
	// Определяем тип четвёртого фрейма
	ASSERT_TRUE(frame::parser::type(data + offset, size - offset, type));
	// Проверяем тип четвёртого фрейма
	ASSERT_EQ(type, frame_t::PADDING);
	// Разбираем заполнение PADDING
	ASSERT_EQ(frame::parser::padding(data + offset, size - offset, consumed, error), status_t::OK);
	// Проверяем длину серии заполнения
	ASSERT_EQ(consumed, 10u);
	// Сдвигаем смещение разбора
	offset += consumed;
	// Проверяем что нагрузка разобрана целиком
	ASSERT_EQ(offset, size);
}
