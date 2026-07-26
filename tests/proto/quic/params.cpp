/**
 * @file: params.cpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты транспортных параметров QUIC — проверка разбора и сборки параметров соединения,
 *        значений по умолчанию и контроля ролевых ограничений
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"
#include "../../../include/proto/quic/params.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic;

/**
 * @brief Метод проверки полного цикла сборки и разбора параметров клиента
 *
 */
TEST_F(QuicFixture, ParamsClientRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Параметры транспорта клиента
	params::params_t source;
	// Устанавливаем SCID первого пакета клиента
	source.hasInitialScid = true;
	source.initialScid    = this->makeCid(this->unhex("aabbccddeeff0011"));
	// Устанавливаем таймаут простоя соединения
	source.maxIdleTimeout = 30000;
	// Устанавливаем лимиты данных
	source.initialMaxData                 = 1048576;
	source.initialMaxStreamDataBidiLocal  = 65536;
	source.initialMaxStreamDataBidiRemote = 32768;
	source.initialMaxStreamDataUni        = 16384;
	// Устанавливаем лимиты числа потоков
	source.initialMaxStreamsBidi = 100;
	source.initialMaxStreamsUni  = 3;
	// Устанавливаем запрет активной миграции
	source.disableActiveMigration = true;
	// Устанавливаем показатель степени задержки подтверждения
	source.ackDelayExponent = 10;
	// Устанавливаем максимальную задержку подтверждения
	source.maxAckDelay = 50;
	// Устанавливаем лимит активных идентификаторов соединения
	source.activeConnectionIdLimit = 8;
	// Собираем параметры транспорта клиента
	ASSERT_TRUE(params::serialize::encode(output, source, endpoint_t::CLIENT));
	// Разобранные параметры транспорта
	params::params_t parsed;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранные параметры
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (output.data()), output.size(), endpoint_t::CLIENT, parsed, error), status_t::OK);
	// Проверяем SCID первого пакета клиента
	ASSERT_TRUE(parsed.hasInitialScid);
	ASSERT_TRUE(parsed.initialScid == source.initialScid);
	// Проверяем таймаут простоя соединения
	ASSERT_EQ(parsed.maxIdleTimeout, 30000u);
	// Проверяем лимиты данных
	ASSERT_EQ(parsed.initialMaxData, 1048576u);
	ASSERT_EQ(parsed.initialMaxStreamDataBidiLocal, 65536u);
	ASSERT_EQ(parsed.initialMaxStreamDataBidiRemote, 32768u);
	ASSERT_EQ(parsed.initialMaxStreamDataUni, 16384u);
	// Проверяем лимиты числа потоков
	ASSERT_EQ(parsed.initialMaxStreamsBidi, 100u);
	ASSERT_EQ(parsed.initialMaxStreamsUni, 3u);
	// Проверяем запрет активной миграции
	ASSERT_TRUE(parsed.disableActiveMigration);
	// Проверяем показатель степени задержки подтверждения
	ASSERT_EQ(parsed.ackDelayExponent, 10u);
	// Проверяем максимальную задержку подтверждения
	ASSERT_EQ(parsed.maxAckDelay, 50u);
	// Проверяем лимит активных идентификаторов соединения
	ASSERT_EQ(parsed.activeConnectionIdLimit, 8u);
	// Проверяем отсутствие параметров только для сервера
	ASSERT_FALSE(parsed.hasOdcid);
	ASSERT_FALSE(parsed.hasResetToken);
	ASSERT_FALSE(parsed.hasRetryScid);
	ASSERT_FALSE(parsed.hasPreferredAddress);
}

/**
 * @brief Метод проверки полного цикла сборки и разбора параметров сервера
 *
 */
TEST_F(QuicFixture, ParamsServerRoundTripTest){
	// Выходной буфер сборки
	std::string output;
	// Параметры транспорта сервера
	params::params_t source;
	// Устанавливаем исходный DCID первого пакета Initial клиента
	source.hasOdcid = true;
	source.odcid    = this->makeCid(this->unhex("8394c8f03e515708"));
	// Устанавливаем SCID первого пакета сервера
	source.hasInitialScid = true;
	source.initialScid    = this->makeCid(this->unhex("f067a5502a4262b5"));
	// Устанавливаем SCID пакета Retry
	source.hasRetryScid = true;
	source.retryScid    = this->makeCid(this->unhex("0102030405"));
	// Устанавливаем токен сброса без сохранения состояния
	source.hasResetToken = true;
	// Заполняем токен сброса тестовыми данными
	::memset(source.resetToken, 0x3C, proto::RESET_TOKEN_SIZE);
	// Устанавливаем предпочтительный адрес сервера
	source.hasPreferredAddress = true;
	// Заполняем IPv4-адрес сервера (192.168.1.10:4433)
	source.preferredAddress.ipv4[0] = 192;
	source.preferredAddress.ipv4[1] = 168;
	source.preferredAddress.ipv4[2] = 1;
	source.preferredAddress.ipv4[3] = 10;
	source.preferredAddress.ipv4Port = 4433;
	// Заполняем IPv6-адрес сервера (::1:8443)
	source.preferredAddress.ipv6[15] = 1;
	source.preferredAddress.ipv6Port = 8443;
	// Устанавливаем идентификатор соединения предпочтительного адреса
	source.preferredAddress.cid = this->makeCid(this->unhex("cafebabe"));
	// Заполняем токен сброса предпочтительного адреса
	::memset(source.preferredAddress.resetToken, 0x9E, proto::RESET_TOKEN_SIZE);
	// Собираем параметры транспорта сервера
	ASSERT_TRUE(params::serialize::encode(output, source, endpoint_t::SERVER));
	// Разобранные параметры транспорта
	params::params_t parsed;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем собранные параметры
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (output.data()), output.size(), endpoint_t::SERVER, parsed, error), status_t::OK);
	// Проверяем исходный DCID первого пакета Initial клиента
	ASSERT_TRUE(parsed.hasOdcid);
	ASSERT_TRUE(parsed.odcid == source.odcid);
	// Проверяем SCID первого пакета сервера
	ASSERT_TRUE(parsed.hasInitialScid);
	ASSERT_TRUE(parsed.initialScid == source.initialScid);
	// Проверяем SCID пакета Retry
	ASSERT_TRUE(parsed.hasRetryScid);
	ASSERT_TRUE(parsed.retryScid == source.retryScid);
	// Проверяем токен сброса без сохранения состояния
	ASSERT_TRUE(parsed.hasResetToken);
	ASSERT_EQ(::memcmp(parsed.resetToken, source.resetToken, proto::RESET_TOKEN_SIZE), 0);
	// Проверяем предпочтительный адрес сервера
	ASSERT_TRUE(parsed.hasPreferredAddress);
	// Проверяем IPv4-адрес и порт сервера
	ASSERT_EQ(::memcmp(parsed.preferredAddress.ipv4, source.preferredAddress.ipv4, 4), 0);
	ASSERT_EQ(parsed.preferredAddress.ipv4Port, 4433u);
	// Проверяем IPv6-адрес и порт сервера
	ASSERT_EQ(::memcmp(parsed.preferredAddress.ipv6, source.preferredAddress.ipv6, 16), 0);
	ASSERT_EQ(parsed.preferredAddress.ipv6Port, 8443u);
	// Проверяем идентификатор соединения предпочтительного адреса
	ASSERT_TRUE(parsed.preferredAddress.cid == source.preferredAddress.cid);
	// Проверяем токен сброса предпочтительного адреса
	ASSERT_EQ(::memcmp(parsed.preferredAddress.resetToken, source.preferredAddress.resetToken, proto::RESET_TOKEN_SIZE), 0);
}

/**
 * @brief Метод проверки значений по умолчанию при пустом наборе параметров
 *
 */
TEST_F(QuicFixture, ParamsDefaultsTest){
	// Разобранные параметры транспорта
	params::params_t parsed;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем пустой набор параметров
	ASSERT_EQ(params::parser::decode(nullptr, 0, endpoint_t::CLIENT, parsed, error), status_t::OK);
	// Проверяем значения по умолчанию из RFC 9000 §18.2
	ASSERT_EQ(parsed.maxIdleTimeout, 0u);
	ASSERT_EQ(parsed.maxUdpPayloadSize, 65527u);
	ASSERT_EQ(parsed.ackDelayExponent, 3u);
	ASSERT_EQ(parsed.maxAckDelay, 25u);
	ASSERT_EQ(parsed.activeConnectionIdLimit, 2u);
	// Проверяем отсутствие необязательных параметров
	ASSERT_FALSE(parsed.hasInitialScid);
	ASSERT_FALSE(parsed.disableActiveMigration);
}

/**
 * @brief Метод проверки разбора эталонной записи параметра
 *
 */
TEST_F(QuicFixture, ParamsReferenceTest){
	// Эталонная запись параметра initial_max_data со значением 1000 (id 0x04, длина 2, значение 0x43e8)
	const std::string reference = this->unhex("040243e8");
	// Разобранные параметры транспорта
	params::params_t parsed;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем эталонную запись
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (reference.data()), reference.size(), endpoint_t::CLIENT, parsed, error), status_t::OK);
	// Проверяем эталонное значение параметра
	ASSERT_EQ(parsed.initialMaxData, 1000u);
	// Проверяем обратную сборку эталонной записи
	std::string output;
	// Параметры транспорта с одним значением
	params::params_t source;
	// Устанавливаем начальный лимит данных
	source.initialMaxData = 1000;
	// Собираем параметры транспорта
	ASSERT_TRUE(params::serialize::encode(output, source, endpoint_t::CLIENT));
	// Проверяем совпадение с эталонной записью
	ASSERT_EQ(this->hex(output), "040243e8");
}

/**
 * @brief Метод проверки игнорирования неизвестных параметров (RFC 9000 §7.4.2)
 *
 */
TEST_F(QuicFixture, ParamsUnknownIgnoredTest){
	// Набор параметров: неизвестный GREASE-параметр (id 0x4271) + initial_max_data 1000
	const std::string input = this->unhex("42710401020304040243e8");
	// Разобранные параметры транспорта
	params::params_t parsed;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем набор параметров
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (input.data()), input.size(), endpoint_t::CLIENT, parsed, error), status_t::OK);
	// Проверяем что известный параметр разобран
	ASSERT_EQ(parsed.initialMaxData, 1000u);
}

/**
 * @brief Метод проверки отклонения некорректных наборов параметров
 *
 */
TEST_F(QuicFixture, ParamsMalformedTest){
	// Разобранные параметры транспорта
	params::params_t parsed;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Дубликат параметра initial_max_data (RFC 9000 §7.4)
	const std::string duplicate = this->unhex("040243e8040243e8");
	// Проверяем отклонение дубликата
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (duplicate.data()), duplicate.size(), endpoint_t::CLIENT, parsed, error), status_t::ERROR);
	// Проверяем код ошибки транспорта
	ASSERT_EQ(error, error_t::TRANSPORT_PARAMETER_ERROR);
	// Параметр только для сервера в параметрах клиента (original_destination_connection_id)
	const std::string serverOnly = this->unhex("00088394c8f03e515708");
	// Проверяем отклонение параметров клиента
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (serverOnly.data()), serverOnly.size(), endpoint_t::CLIENT, parsed, error), status_t::ERROR);
	// Проверяем что те же параметры от сервера принимаются
	params::params_t fromServer;
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (serverOnly.data()), serverOnly.size(), endpoint_t::SERVER, fromServer, error), status_t::OK);
	// Показатель степени задержки подтверждения больше 20 (id 0x0a, значение 21)
	const std::string badExponent = this->unhex("0a0115");
	// Проверяем отклонение некорректного показателя
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (badExponent.data()), badExponent.size(), endpoint_t::CLIENT, parsed, error), status_t::ERROR);
	// Максимальный размер UDP-нагрузки меньше 1200 (id 0x03, значение 1199)
	const std::string badUdpSize = this->unhex("030244af");
	// Проверяем отклонение некорректного размера
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (badUdpSize.data()), badUdpSize.size(), endpoint_t::CLIENT, parsed, error), status_t::ERROR);
	// Лимит активных идентификаторов меньше 2 (id 0x0e, значение 1)
	const std::string badCidLimit = this->unhex("0e0101");
	// Проверяем отклонение некорректного лимита
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (badCidLimit.data()), badCidLimit.size(), endpoint_t::CLIENT, parsed, error), status_t::ERROR);
	// Значение короче объявленной длины (id 0x04, длина 3, значение из 2 октетов)
	const std::string badLength = this->unhex("040343e8");
	// Проверяем отклонение некорректной длины
	ASSERT_EQ(params::parser::decode(reinterpret_cast <const uint8_t *> (badLength.data()), badLength.size(), endpoint_t::CLIENT, parsed, error), status_t::ERROR);
	// Проверяем отклонение сборки параметров сервера у клиента
	std::string output;
	// Параметры транспорта с параметром только для сервера
	params::params_t invalid;
	// Устанавливаем исходный DCID (параметр только для сервера)
	invalid.hasOdcid = true;
	invalid.odcid    = this->makeCid(this->unhex("01020304"));
	// Проверяем что сборка отклонена
	ASSERT_FALSE(params::serialize::encode(output, invalid, endpoint_t::CLIENT));
}
