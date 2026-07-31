/**
 * @file: static.cpp
 * @date: 2025-12-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты модуля работы с сетевыми адресами — проверка создания и сброса объекта модуля,
 *        а также корректности разбора и форматирования IPv4-, IPv6- и MAC-адресов,
 *        вычисления масок и определения типа адреса
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "addr.hpp"

/**
 * Подключаем системный разборщик сетевых адресов для сличения
 */
#include <arpa/inet.h>

/**
 * @brief Метод инициализации тестовой среды
 *
 */
TEST_F(NetFixture, CreateNetTest){
	// Если объект сетевого адреса создан
	ASSERT_TRUE(this->_addr != nullptr);
	// Сбрасываем объект сетевого адреса
	this->_addr.reset();
	// Проверяем что объект сброшен
	ASSERT_TRUE(this->_addr == nullptr);
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
TEST_F(NetFixture, ResetAndCreateNetTest){
	// Если объект сетевого адреса создан
	ASSERT_TRUE(this->_addr != nullptr);
	// Сбрасываем объект сетевого адреса
	this->_addr.reset();
	// Проверяем что объект сброшен
	ASSERT_TRUE(this->_addr == nullptr);
	// Создаём объект сетевого адреса заново
	this->_addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_addr != nullptr);
}

/**
 * @brief Метод инициализации тестовой среды
 *
 */
TEST_F(NetFixture, ReCreateNetTest){
	// Если объект сетевого адреса создан
	ASSERT_TRUE(this->_addr != nullptr);
	// Создаём объект сетевого адреса заново
	this->_addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_addr != nullptr);
}

/**
 * @brief Метод парсинга сетевого адреса
 *
 */
TEST_F(NetFixture, ReCreateAndParseNetTest){
	// Если объект сетевого адреса создан
	ASSERT_TRUE(this->_addr != nullptr);
	// Создаём объект сетевого адреса заново
	this->_addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_addr != nullptr);
	// Парсим IP-адрес
	ASSERT_TRUE(this->_addr->parse("192.168.0.1"));
}

/**
 * @brief Метод парсинга и очистки сетевого адреса
 *
 */
TEST_F(NetFixture, ParseAndClearNetTest){
	// Парсим IP-адрес
	ASSERT_TRUE(this->_addr->parse("192.168.0.1"));
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(this->_addr->type(), awh::net_addr_t::type_t::IPV4);
	// Проверяем что адрес установлен правильно
	ASSERT_EQ("192.168.0.1", this->_addr->print());
	// Очищаем объект сетевого адреса
	this->_addr->clear();
	// Проверяем что тип адреса сброшен правильно
	ASSERT_EQ(this->_addr->type(), awh::net_addr_t::type_t::NONE);
	// Проверяем что адрес сброшен правильно
	ASSERT_EQ("", this->_addr->print());
}

/**
 * @brief Метод установки и получения IPv4 адреса
 *
 */
TEST_F(NetFixture, NetSetAndGetV4Test){
	// IP-адрес 156.12.48.10 в формате BIG-ENDIAN
	uint32_t ip = 2618044426;
	// Устанавливаем IP-адрес
	this->_addr->v4(ip, awh::net_addr_t::endian_t::BIG);
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(ip, this->_addr->v4(awh::net_addr_t::endian_t::BIG));
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->type());
	// Проверяем что адрес установлен правильно
	ASSERT_EQ("156.12.48.10", this->_addr->print());
}

/**
 * @brief Метод увеличения IPv4 адреса
 *
 */
TEST_F(NetFixture, IncreaseIpV4NetTest){
	// Парсим IP-адрес
	ASSERT_TRUE(this->_addr->parse("192.168.0.1"));
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(this->_addr->type(), awh::net_addr_t::type_t::IPV4);
	// Проверяем что адрес установлен правильно
	ASSERT_EQ("192.168.0.1", this->_addr->print());
	// Получаем IP-адрес в формате BIG-ENDIAN
	uint32_t ip = this->_addr->v4(awh::net_addr_t::endian_t::BIG);
	// Очищаем объект сетевого адреса
	this->_addr->clear();
	// Увеличиваем IP-адрес на единицу
	++ip;
	// Устанавливаем увеличенный IP-адрес
	this->_addr->v4(ip, awh::net_addr_t::endian_t::BIG);
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ("192.168.0.2", this->_addr->print());
}

/**
 * @brief Метод установки и получения IPv6 адреса
 *
 */
TEST_F(NetFixture, NetSetAndGetV6Test){
	// IP-адрес [2001:1234:abcd:5678:9877:3322:5541:aabb] в формате BIG-ENDIAN
	std::array <uint8_t, 16> ip = {
		0x20, 0x01, 0x12, 0x34,
		0xab, 0xcd, 0x56, 0x78,
		0x98, 0x77, 0x33, 0x22,
		0x55, 0x41, 0xaa, 0xbb
	};
	// Устанавливаем IP-адрес
	this->_addr->v6(ip, awh::net_addr_t::endian_t::LITTLE);
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(ip, this->_addr->v6(awh::net_addr_t::endian_t::LITTLE));
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(awh::net_addr_t::type_t::IPV6, this->_addr->type());
	// Проверяем что адрес установлен правильно
	ASSERT_EQ("2001:1234:ABCD:5678:9877:3322:5541:AABB", this->_addr->print());
}

/**
 * @brief Метод установки и получения MAC-адреса
 *
 */
TEST_F(NetFixture, NetSetAndGetMacTest){
	// MAC-адрес 02:42:9b:9e:f9:ae в формате BIG-ENDIAN
	std::array <uint8_t, 6> mac = {
		0x02, 0x42, 0x9B,
		0x9E, 0xF9, 0xAE
	};
	// Устанавливаем MAC-адрес
	this->_addr->mac(mac);
	// Проверяем что MAC-адрес установлен правильно
	ASSERT_EQ(mac, this->_addr->mac());
	// Проверяем что тип адреса установлен правильно
	ASSERT_EQ(awh::net_addr_t::type_t::MAC, this->_addr->type());
	// Проверяем что адрес установлен правильно
	ASSERT_EQ("02:42:9B:9E:F9:AE", this->_addr->print());
}

/**
 * @brief Метод преобразования IPv6 в IPv4 адрес
 *
 */
TEST_F(NetFixture, NetBroadcastIPv6ToIPv4SuccessTest){
	// Парсим IP-адрес
	ASSERT_TRUE(this->_addr->parse("::ffff:c0a8:6401"));
	// Проверяем что преобразование прошло успешно
	ASSERT_TRUE(this->_addr->broadcastIPv6ToIPv4());
}

/**
 * @brief Метод преобразования IPv6 в IPv4 адрес
 *
 */
TEST_F(NetFixture, NetBroadcastIPv6ToIPv4Failed1Test){
	// Парсим IP-адрес
	ASSERT_TRUE(this->_addr->parse("::0f0f:c0a8:6401"));
	// Проверяем что преобразование прошло неуспешно
	ASSERT_FALSE(this->_addr->broadcastIPv6ToIPv4());
}

/**
 * @brief Метод преобразования IPv6 в IPv4 адрес
 *
 */
TEST_F(NetFixture, NetBroadcastIPv6ToIPv4Failed2Test){
	// Парсим IP-адрес
	ASSERT_TRUE(this->_addr->parse("2001:1234:ABCD:5678:9877:ffff:c0a8:6401"));
	// Проверяем что преобразование прошло неуспешно
	ASSERT_FALSE(this->_addr->broadcastIPv6ToIPv4());
}

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(NetFixture, NetSuiteTest){
	// Выполняем парсинг IPv6 адреса
	(* this->_addr.get()) = "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("2001:DB8:11A3:9D7:1F34:8A2E:7A0:765D", static_cast <std::string> (* this->_addr.get()));
	ASSERT_EQ("2001:0DB8:11A3:09D7:1F34:8A2E:07A0:765D", this->_addr->print(awh::net_addr_t::format_size_t::LONG));
	ASSERT_EQ("2001:DB8:11A3:9D7:1F34:8A2E:7A0:765D", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE));
	ASSERT_EQ("2001:DB8:11A3:9D7:1F34:8A2E:7A0:765D", this->_addr->print(awh::net_addr_t::format_size_t::SHORT));
	ASSERT_EQ("8193:3512:4515:2519:7988:35374:1952:30301", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::DECIMAL));
	ASSERT_EQ("20001:6670:10643:4727:17464:105056:3640:73135", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL));
	ASSERT_EQ("2001:DB8:11A3:9D7:1F34:8A2E:7.160.118.93", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX_IPV4));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("2001:DB8::AE21:AD12", static_cast <std::string> (* this->_addr.get()));
	ASSERT_EQ("2001:DB8:0:0:0:0:174.33.173.18", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX_IPV4));
	ASSERT_EQ("2001:DB8:00:00:00:00:174.33.173.18", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_flag_t::HEX_IPV4));
	ASSERT_EQ("2001:0DB8:0000:0000:0000:0000:174.33.173.18", this->_addr->print(awh::net_addr_t::format_size_t::LONG, awh::net_addr_t::format_flag_t::HEX_IPV4));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:db8::ae21:ad12";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("2001:0DB8:0000:0000:0000:0000:AE21:AD12", this->_addr->print(awh::net_addr_t::format_size_t::LONG));
	ASSERT_EQ("2001:DB8:0:0:0:0:AE21:AD12", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "0000:0000:0000:0000:0000:0000:ae21:ad12";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("::AE21:AD12", this->_addr->print(awh::net_addr_t::format_size_t::SHORT));
	ASSERT_EQ("::174.33.173.18", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::ae21:ad12";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("0:0:0:0:0:0:AE21:AD12", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE));
	ASSERT_EQ("::174.33.173.18", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:0db8:11a3:09d7:1f34::";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("2001:0DB8:11A3:09D7:1F34:0000:0000:0000", this->_addr->print(awh::net_addr_t::format_size_t::LONG));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::ffff:192.0.2.1";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("::FFFF:C000:201", static_cast <std::string> (* this->_addr.get()));
	// Проверяем что адрес является вещанием IPv6 в IPv4
	ASSERT_TRUE(this->_addr->broadcastIPv6ToIPv4());
	// Проверяем форматы вывода адреса
	ASSERT_EQ("::FFFF:192.0.2.1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "[::1]";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("0000:0000:0000:0000:0000:0000:0000:0001", this->_addr->print(awh::net_addr_t::format_size_t::LONG));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "[::]";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("0000:0000:0000:0000:0000:0000:0000:0000", this->_addr->print(awh::net_addr_t::format_size_t::LONG));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "46.39.230.51";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("046.039.230.051", this->_addr->print(awh::net_addr_t::format_size_t::LONG));
	// Проверяем что адрес является вещанием IPv6 в IPv4
	ASSERT_FALSE(this->_addr->broadcastIPv6ToIPv4());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.16.0.1";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("192.016.000.001", this->_addr->print(awh::net_addr_t::format_size_t::LONG));
	ASSERT_EQ("192.16.0.1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT));
	ASSERT_EQ("192.16.00.01", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE));
	ASSERT_EQ("C0.10.0.1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX));
	ASSERT_EQ("300.20.0.1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Проверяем размер массива IPv6 адреса в формате LITTLE-ENDIAN
	ASSERT_EQ(16, this->_addr->v6().size());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "46.39.230.51";
	// Проверяем получение IPv4 адреса в формате LITTLE-ENDIAN
	ASSERT_EQ(870721326, this->_addr->v4());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	this->_addr->impose(53, awh::net_addr_t::addr_t::NETWORK);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:1234:ABCD:5000::", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	this->_addr->impose("FFFF:FFFF:FFFF:F800::", awh::net_addr_t::addr_t::NETWORK);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:1234:ABCD:5000::", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("C0.A8.3.C0", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX));
	ASSERT_EQ("300.250.3.300", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем префикс 9
	this->_addr->impose(9, awh::net_addr_t::addr_t::NETWORK);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("192.128.0.0", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем префикс 9
	this->_addr->impose("255.128.0.0", awh::net_addr_t::addr_t::NETWORK);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("192.128.0.0", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем префикс 9
	this->_addr->impose("255.255.255.0", awh::net_addr_t::addr_t::NETWORK);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("192.168.3.0", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	this->_addr->impose(53, awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("::678:9877:3322:5541:AABB", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс маску FFFF:FFFF:FFFF:F800::
	this->_addr->impose("FFFF:FFFF:FFFF:F800::", awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("::678:9877:3322:5541:AABB", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем префикс 9
	this->_addr->impose(9, awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("0.40.3.192", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем маску 255.128.0.0
	this->_addr->impose("255.128.0.0", awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("0.40.3.192", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем префикс 24
	this->_addr->impose(24, awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("0.0.0.192", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Накладываем маску 255.255.255.0
	this->_addr->impose("255.255.255.0", awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("0.0.0.192", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Выполняем проверки конвертации префикса и маски
	ASSERT_EQ("255.128.0.0", this->_addr->prefix2Mask(9));
	ASSERT_EQ(9, static_cast <uint16_t> (this->_addr->mask2Prefix("255.128.0.0")));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Выполняем проверки конвертации префикса и маски
	ASSERT_EQ("FFFF:FFFF:FFFF:F800::", this->_addr->prefix2Mask(53));
	ASSERT_EQ(53, static_cast <uint16_t> (this->_addr->mask2Prefix("FFFF:FFFF:FFFF:F800::")));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес маппится в сеть
	ASSERT_TRUE(this->_addr->mapping("192.168.0.0"));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Проверяем что адрес маппится в сеть
	ASSERT_TRUE(this->_addr->mapping("2001:1234:abcd:5678::"));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес маппится в сеть с префиксом 9
	ASSERT_TRUE(this->_addr->mapping("192.128.0.0", 9, awh::net_addr_t::addr_t::NETWORK));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Проверяем что адрес маппится в сеть с префиксом 53
	ASSERT_TRUE(this->_addr->mapping("2001:1234:abcd:5678::", 53, awh::net_addr_t::addr_t::NETWORK));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес маппится в сеть с маской 255.128.0.0
	ASSERT_TRUE(this->_addr->mapping("192.128.0.0", "255.128.0.0", awh::net_addr_t::addr_t::NETWORK));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Проверяем что адрес маппится в сеть с маской FFFF:FFFF:FFFF:F800::
	ASSERT_TRUE(this->_addr->mapping("2001:1234:abcd:5678::", "FFFF:FFFF:FFFF:F800::", awh::net_addr_t::addr_t::NETWORK));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес маппится в хост с префиксом 9
	ASSERT_TRUE(this->_addr->mapping("0.40.3.192", 9, awh::net_addr_t::addr_t::HOST));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Проверяем что адрес маппится в хост с префиксом 53
	ASSERT_TRUE(this->_addr->mapping("::678:9877:3322:5541:AABB", 53, awh::net_addr_t::addr_t::HOST));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес маппится в хост с маской 255.128.0.0
	ASSERT_TRUE(this->_addr->mapping("0.40.3.192", "255.128.0.0", awh::net_addr_t::addr_t::HOST));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Проверяем что адрес маппится в хост с маской FFFF:FFFF:FFFF:F800::
	ASSERT_TRUE(this->_addr->mapping("::678:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", awh::net_addr_t::addr_t::HOST));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес входит в диапазон
	ASSERT_TRUE(this->_addr->range("192.168.3.100", "192.168.3.200", 24));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "46.39.230.51";
	// Проверяем что тип адреса соответствует глобальному
	ASSERT_EQ(awh::net_addr_t::own_t::WAN, this->_addr->own());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.31.12";
	// Проверяем что тип адреса соответствует локальному
	ASSERT_EQ(awh::net_addr_t::own_t::LAN, this->_addr->own());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "0.0.0.0";
	// Проверяем что тип адреса соответствует зарезервированному
	ASSERT_EQ(awh::net_addr_t::own_t::SYS, this->_addr->own());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "[2a00:1450:4010:c0a::8b]";
	// Проверяем что тип адреса соответствует глобальному
	ASSERT_EQ(awh::net_addr_t::own_t::WAN, this->_addr->own());

	/**
	 * Петля выдана назначению, а не хостам, и потому считается служебной сетью, а
	 * не частной (RFC 6890). Прежде она относилась к частным - вместе с
	 * документационными и служебными блоками, - отчего проверка доступа,
	 * пускающая частные адреса, пускала заодно и их
	 */
	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::1";
	// Проверяем что тип адреса соответствует служебному
	ASSERT_EQ(awh::net_addr_t::own_t::SYS, this->_addr->own());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::";
	// Проверяем что тип адреса соответствует зарезервированному
	ASSERT_EQ(awh::net_addr_t::own_t::SYS, this->_addr->own());

	// Создаём IP-адрес
	std::string ip = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Присваиваем IP-адрес объекту сети
	ip = (* this->_addr.get()) = ip;
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:DB8::AE21:AD12", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678::";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:1234:ABCD:5678::", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "fe80:0000:0000:0000:1cff:84b4:8614:0000";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("FE80::1CFF:84B4:8614:0", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:DB8:11A3:9D7:1F34:8A2E:7A0:765D", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты MAC-адресов
	(* this->_addr.get()) = "73:0b:04:0d:db:79";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("73:0B:04:0D:DB:79", static_cast <std::string> (* this->_addr.get()));

	// Устанавливаем ARPA-адрес
	this->_addr->arpa("70.255.255.5.in-addr.arpa");
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("5.255.255.70", static_cast <std::string> (* this->_addr.get()));
	// Выполняем проверку формата вывода ARPA-адреса
	ASSERT_EQ("70.255.255.5.in-addr.arpa", this->_addr->arpa());

	// Устанавливаем ARPA-адрес
	this->_addr->arpa("b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa");
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:DB8::567:89AB", static_cast <std::string> (* this->_addr.get()));
	// Выполняем проверку формата вывода ARPA-адреса
	ASSERT_EQ("b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa", this->_addr->arpa());

	// Выполняем проверки определения типа адреса по строковому представлению
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->host("192.168.7.231"));
	ASSERT_EQ(awh::net_addr_t::type_t::IPV6, this->_addr->host("2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d"));
	ASSERT_EQ(awh::net_addr_t::type_t::NETV4, this->_addr->host("192.168.7.231/24"));
	ASSERT_EQ(awh::net_addr_t::type_t::NETV4, this->_addr->host("192.168.7.231/255.255.255.0"));
	ASSERT_EQ(awh::net_addr_t::type_t::NETV6, this->_addr->host("fe80::1cff:84b4:8614:0/112"));
	ASSERT_EQ(awh::net_addr_t::type_t::MAC, this->_addr->host("73:0b:04:0d:db:79"));
	ASSERT_EQ(awh::net_addr_t::type_t::URL, this->_addr->host("https://anyks.com"));
	ASSERT_EQ(awh::net_addr_t::type_t::FQDN, this->_addr->host("anyks.com"));
	ASSERT_EQ(awh::net_addr_t::type_t::FQDN, this->_addr->host("ns1.anyks.com"));
	ASSERT_EQ(awh::net_addr_t::type_t::FS, this->_addr->host("c:\\Program\\ Files"));
	ASSERT_EQ(awh::net_addr_t::type_t::FS, this->_addr->host("/opt/mc/bin/mc"));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.0.1";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("192.168.0.1", static_cast <std::string> (* this->_addr.get()));
	ASSERT_EQ("::FFFF:C0A8:1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX_IPV6));
	ASSERT_EQ("::FFFF:C0A8:01", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_flag_t::HEX_IPV6));
	ASSERT_EQ("::FFFF:C0A8:0001", this->_addr->print(awh::net_addr_t::format_size_t::LONG, awh::net_addr_t::format_flag_t::HEX_IPV6));
	ASSERT_EQ("::FFFF:192.168.0.1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX_IPV4));
	ASSERT_EQ("::FFFF:192.168.00.01", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_flag_t::HEX_IPV4));
	ASSERT_EQ("::FFFF:192.168.000.001", this->_addr->print(awh::net_addr_t::format_size_t::LONG, awh::net_addr_t::format_flag_t::HEX_IPV4));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "10.1";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("10.0.0.1", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "0xC0.0xA8.0.0x1";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("192.168.0.1", static_cast <std::string> (* this->_addr.get()));

	/**
	 * Запись из девяти хекстетов адресом IPv6 не является, и разбор её обязан
	 * оставить объект пустым. Прежде разновидность переживала неудачу и доставалась
	 * от прежнего разбора: негодная запись печаталась как "0.0.0.0" - то есть
	 * выглядела исправным адресом IPv4
	 */
	// Выполняем разбор заведомо негодной записи
	(* this->_addr.get()) = "2001:db8:0:0:0:8:800:200C:417A";
	// Разновидность адреса обязана быть сброшена
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
	// Запись пустого объекта обязана быть пустой
	ASSERT_EQ("", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:db8::8:800:200C:417A";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:DB8::8:800:200C:417A", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::ae21:ad12";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("::AE21:AD12", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::ffff:192.0.2.128";
	// Выполняем проверки форматов вывода адреса
	ASSERT_EQ("::FFFF:C000:280", static_cast <std::string> (* this->_addr.get()));
	ASSERT_EQ("::FFFF:C000:280", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX_IPV6));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "fe80::1%eth0";
	// Выполняем проверку извлечения зоны интерфейса
	ASSERT_EQ("eth0", this->_addr->zone());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "[fe80::1%25en0]";
	// Выполняем проверку извлечения зоны интерфейса
	ASSERT_EQ("en0", this->_addr->zone());

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("::", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты MAC-адресов
	(* this->_addr.get()) = "73:0b:04:0d:db:79";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("73-0B-04-0D-DB-79", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_flag_t::HEX, '-'));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.53.3";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("192.168.53.3", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::DECIMAL));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("20001-6670--127041-126422", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("20001-6670-10643-4727-17464-105056-3640-73135", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "2001:0db8:0000:0000:0000:0000:ae21:0";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("20001-6670--127041-0", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "::ffff:192.0.2.1";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("--177777-140000-1001", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "0000:0000:0000:0000:0000:0000:ae21:ad12";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("--127041-126422", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "FFFF:FFFF:FFFF:F800::";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("177777-177777-177777-174000--", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));

	// Создаём IP-адрес сети
	(* this->_addr.get()) = "2001:0db8:11a3:09d7:1f34::";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("20001-6670-10643-4727-17464--", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::OCTAL, '-'));
}

/**
 * @brief Тест соответствия определения разновидности адреса полному перебору
 *
 * @details Определение разновидности адреса устроено не полным перебором всех
 *          разновидностей, а сперва снятием примет строки: за один её обход
 *          выясняется, какими разновидностями она вообще способна оказаться, и
 *          проверяются только они. Приметы обязаны при этом давать надмножество:
 *          разновидность, проверка на которую могла бы пройти, обязана в набор
 *          попасть. Лишняя разновидность в наборе стоит одной проверки впустую,
 *          недостающая изменила бы ответ.
 *
 *          Свойство это на отдельных образцах не проверяется - его надо
 *          проверять сличением. Тест берёт набор образцов, покрывающий все
 *          разновидности и их пограничные случаи, и для каждого сличает ответ
 *          метода с ответом полного перебора тех же проверок в том же порядке.
 *          Расхождение означает, что примета отсекла разновидность, которая
 *          подошла бы
 *
 */
TEST_F(NetFixture, NetHostMatchesFullScanTest){
	// Порядок перебора разновидностей адреса
	static const awh::net_addr_t::type_t ORDER[] = {
		awh::net_addr_t::type_t::IPV4, awh::net_addr_t::type_t::IPV6,
		awh::net_addr_t::type_t::MAC, awh::net_addr_t::type_t::NETV4,
		awh::net_addr_t::type_t::NETV6, awh::net_addr_t::type_t::URL,
		awh::net_addr_t::type_t::FS, awh::net_addr_t::type_t::FQDN
	};
	// Набор образцов, покрывающий все разновидности и их пограничные случаи
	static const char * SAMPLES[] = {
		"", " ", "  ", "192.168.1.100", "8.8.8.8", "0.0.0.0", "255.255.255.255",
		"0x7f000001", "0177.0.0.1", "127.1", "127.0.1", "2130706433", "0xC0.0xA8.1.1",
		"2001:db8::1", "::", "::1", "fe80::1%lo0", "[::1]", "[fe80::1%25eth0]",
		"2001:0db8:0000:0000:0000:ff00:0042:8329", "::ffff:192.168.1.1", "::ffff:1.2.3.4",
		"02:42:9b:9e:f9:ae", "02-42-9b-9e-f9-ae", "02429b9ef9ae", "AABBCCDDEEFF",
		"192.168.0.0/24", "10.0.0.0/255.0.0.0", "2001:db8::/32", "::/0", "fe80::/10",
		"http://anyks.com", "https://anyks.com/path", "HTTP://ANYKS.COM", "https://",
		"/var/run/socket.sock", "/", "~/file", "./rel", "../rel", "\\\\server\\share",
		"C:\\path", "C:/path", "example.com", "localhost", "a.b", "sub.domain.example.com",
		"-bad.com", "bad-.com", "/24", "/dev/null", "1.2.3.4/", "1.2.3.4/24/8", "::1/128",
		"%", "%25", "a%b", "0123456789ab", "123456789012", "abcdef", "::ffff:0102:0304",
		" 1.2.3.4 ", " ::1 ", "h", "ht", "http", "http:/", "https:/x", "1:2:3:4:5:6:7:8",
		"1:2:3:4:5:6:1.2.3.4", "1:2:3:4:5:6:7:8:9", ":::", "1::2::3", "1:2:3:4:5:6:7",
		"[]", "[::]", "1.2.3.4%eth0", "xn--80ak6aa92e.com", "0:0:0:0:0:0:0:0"
	};
	/**
	 * Сличаем ответы в обоих режимах строгости разбора
	 */
	for(const bool strict : {false, true}){
		// Устанавливаем режим строгости разбора адресов
		this->_addr->strict(strict);
		/**
		 * Проходим по каждому образцу набора
		 */
		for(const char * sample : SAMPLES){
			// Разновидность, определённая по приметам строки
			const awh::net_addr_t::type_t actual = this->_addr->host(sample);
			// Разновидность, определённая полным перебором
			awh::net_addr_t::type_t expected = awh::net_addr_t::type_t::NONE;
			// Если образец не пустой
			if(*sample != '\0'){
				/**
				 * Перебираем все разновидности адреса в прежнем порядке
				 */
				for(const awh::net_addr_t::type_t type : ORDER){
					// Если проверка на очередную разновидность прошла
					if(this->_addr->check(sample, type)){
						// Запоминаем определённую разновидность адреса
						expected = type;
						// Прекращаем перебор разновидностей адреса
						break;
					}
				}
			}
			// Выполняем сличение ответов
			ASSERT_EQ(static_cast <uint16_t> (expected), static_cast <uint16_t> (actual))
				<< "образец [" << sample << "], строгий режим: " << (strict ? "да" : "нет");
		}
	}
	// Возвращаем режим строгости разбора адресов
	this->_addr->strict(false);
}

/**
 * @brief Тест независимости разбора адреса от системной локали
 *
 * @details Обрезка пробелов перед разбором адреса опиралась на библиотечную
 *          проверку пробельного символа, а та смотрит на LC_CTYPE. Фреймворк же
 *          в своём конструкторе локаль устанавливает, и по умолчанию она
 *          "en_US.UTF-8" против "C" под MS Windows. Выходило, что байт 0xA0 -
 *          неразрывный пробел кодировки Latin-1 - в одной локали считался
 *          пробелом и обрезался, а в другой нет: строка "1.2.3.4\xA0"
 *          разбиралась как годный IPv4-адрес на одной платформе и не
 *          разбиралась на другой.
 *
 *          Разбор адреса от настроек локали зависеть не должен, поэтому
 *          пробельным набором считается набор основной локали и только он:
 *          пробел и пятёрка управляющих символов от табуляции до возврата
 *          каретки. Тест закрепляет это: обычные пробелы обрезаются, байты
 *          старше 0x7F - нет
 *
 */
TEST_F(NetFixture, NetTrimIsLocaleIndependentTest){
	// Обычные пробельные символы обрезаются
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->host(" 1.2.3.4 "));
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->host("\t1.2.3.4\r\n"));
	ASSERT_EQ(awh::net_addr_t::type_t::IPV6, this->_addr->host(" ::1 "));
	// Неразрывный пробел Latin-1 пробелом не считается ни в какой локали
	ASSERT_NE(awh::net_addr_t::type_t::IPV4, this->_addr->host(std::string("1.2.3.4\xA0")));
	ASSERT_NE(awh::net_addr_t::type_t::IPV4, this->_addr->host(std::string("\xA0" "1.2.3.4")));
	ASSERT_NE(awh::net_addr_t::type_t::IPV6, this->_addr->host(std::string("::1\xA0")));
	// Прочие байты старше 0x7F пробелом не считаются тоже
	ASSERT_NE(awh::net_addr_t::type_t::IPV4, this->_addr->host(std::string("1.2.3.4\x85")));
	ASSERT_NE(awh::net_addr_t::type_t::IPV4, this->_addr->host(std::string("1.2.3.4\xFF")));
}

/**
 * @brief Тест умолчаний структур атрибутов подключения
 *
 * @details Атрибуты подключения объект самого адреса по умолчанию не заводят:
 *          у сетевых атрибутов разновидность не определена и IP-адрес пуст,
 *          у атрибутов UNIX-доменного сокета пуст путь. Заводит их тот, кто
 *          атрибуты наполняет, и обходится это на одно выделение памяти дешевле,
 *          когда наполнять их не приходится вовсе.
 *
 *          Прежде сетевые атрибуты создавались с разновидностью IPV4 и готовым
 *          объектом IPv4-адреса, и записи вида
 *
 *              attr->ip.get() ... ->address = value;
 *
 *          работали без создания объекта. Со сменой умолчания такая запись
 *          разыменовывает пустой указатель, а запись шестнадцати байт IPv6
 *          поверх готового четырёхбайтового IPv4 портила память и до неё.
 *          Тест закрепляет умолчания: если их вернуть обратно, он не пройдёт,
 *          и станет видно, что места наполнения атрибутов нужно проверить снова
 *
 */
TEST_F(NetFixture, NetAttributesHaveNoAddressByDefaultTest){
	// Сетевые атрибуты подключения разновидности не имеют
	awh::net::attr_net_t network;
	ASSERT_EQ(awh::net::type_t::NONE, network.type);
	// Сетевые атрибуты подключения объекта IP-адреса не имеют
	ASSERT_EQ(nullptr, network.ip);
	// Порт хоста обнулён
	ASSERT_EQ(0, network.port);
	// Атрибуты UNIX-доменного сокета объекта пути не имеют
	awh::net::attr_uds_t socket;
	ASSERT_EQ(awh::net::type_t::FS, socket.type);
	ASSERT_EQ(nullptr, socket.path);
	// Атрибуты доменного имени хранят имя строкой и заводить ничего не требуют
	awh::net::attr_fqdn_t domain;
	ASSERT_EQ(awh::net::type_t::FQDN, domain.type);
	ASSERT_EQ(0, domain.port);
	ASSERT_TRUE(domain.domain.empty());
}

/**
 * @brief Тест принадлежности зоны разобранному адресу
 *
 */
TEST_F(NetFixture, NetZoneBelongsToAddressTest){
	/**
	 * Зона обозначает область действия адреса локальной связи и есть только у
	 * IPv6: ни у IPv4-адреса, ни у MAC-адреса её быть не может. Очистка её,
	 * однако, стояла в одной лишь ветви разбора IPv6-адреса, а разбор IPv4
	 * до неё не доходил - и зона прежнего адреса доставалась новому: адрес
	 * "127.0.0.1", разобранный вслед за "fe80::1%eth0", печатался как
	 * "127.0.0.1%eth0"
	 */
	// Выполняем разбор адреса с зоной
	(* this->_addr.get()) = "fe80::1%eth0";
	// Проверяем зону, извлечённую из адреса
	ASSERT_EQ("eth0", this->_addr->zone());
	// Выполняем разбор адреса IPv4 тем же объектом
	(* this->_addr.get()) = "127.0.0.1";
	// Зона прежнего адреса новому достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
	// Проверяем запись адреса
	ASSERT_EQ("127.0.0.1", static_cast <std::string> (* this->_addr.get()));
	// Выполняем разбор адреса с зоной
	(* this->_addr.get()) = "fe80::1%en0";
	// Проверяем зону, извлечённую из адреса
	ASSERT_EQ("en0", this->_addr->zone());
	// Выполняем разбор MAC-адреса тем же объектом
	(* this->_addr.get()) = "73:0b:04:0d:db:79";
	// Зона прежнего адреса новому достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
	/**
	 * Разбор адреса одной лишь разновидностью очищать зону обязан тоже
	 */
	// Выполняем разбор адреса с зоной
	(* this->_addr.get()) = "fe80::1%eth1";
	// Проверяем зону, извлечённую из адреса
	ASSERT_EQ("eth1", this->_addr->zone());
	// Выполняем разбор адреса IPv4 разновидностью, заданной явно
	ASSERT_TRUE(this->_addr->parse("10.0.0.1", awh::net_addr_t::type_t::IPV4));
	// Зона прежнего адреса новому достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
	// Проверяем запись адреса
	ASSERT_EQ("10.0.0.1", static_cast <std::string> (* this->_addr.get()));
	/**
	 * Установка адреса в чистом виде зоны не несёт и прежнюю снимать обязана
	 */
	// Выполняем разбор адреса с зоной
	(* this->_addr.get()) = "fe80::1%eth2";
	// Снимаем адрес в чистом виде
	std::unique_ptr <awh::net::addr_t> source = this->_addr->source(awh::net_addr_t::endian_t::LITTLE);
	// Выполняем установку адреса в чистом виде
	this->_addr->source(source.get(), awh::net_addr_t::endian_t::LITTLE);
	// Зона прежнего адреса установленному достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
}

/**
 * @brief Тест наложения префикса на IPv6 в сетевом порядке байт
 *
 * @details Буфер держит адрес в сетевом порядке, где старший октет хекстета идёт
 *          первым. Прежде хекстет снимался копированием в целое, а на машине с
 *          обратным порядком байт октеты при этом менялись местами - маска ложилась
 *          не на те разряды. Адрес "2001:1234:abcd:5678::/53" давал сеть
 *          "2001:1234:abcd:78::" вместо "2001:1234:abcd:5000::": вместо старших
 *          пяти разрядов хекстета оставался его младший октет
 *
 */
TEST_F(NetFixture, NetImposeIPv6NetworkOrderTest){
	/**
	 * Набор образцов: префикс, ожидаемая сеть, ожидаемый хост
	 */
	const std::vector <std::tuple <uint8_t, std::string, std::string>> samples = {
		// Префикс, не кратный шестнадцати: хекстет 5678 режется по пятому разряду
		{53, "2001:1234:ABCD:5000::", "::678:9877:3322:5541:AABB"},
		// Префикс, отстоящий от границы хекстета на один разряд
		{63, "2001:1234:ABCD:5678::", "::9877:3322:5541:AABB"},
		// Префикс, кратный шестнадцати: хекстет остаётся нетронутым
		{48, "2001:1234:ABCD::", "::5678:9877:3322:5541:AABB"},
		// Префикс, режущий хекстет по первому разряду
		{49, "2001:1234:ABCD::", "::5678:9877:3322:5541:AABB"}
	};
	/**
	 * Перебираем все образцы
	 */
	for(auto & sample : samples){
		// Выполняем разбор исходного адреса
		(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
		// Накладываем префикс, оставляя сетевую часть
		this->_addr->impose(std::get <0> (sample), awh::net_addr_t::addr_t::NETWORK);
		// Проверяем полученную сеть
		ASSERT_EQ(std::get <1> (sample), static_cast <std::string> (* this->_addr.get()))
			<< "префикс: " << static_cast <uint16_t> (std::get <0> (sample));
		// Выполняем разбор исходного адреса заново
		(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
		// Накладываем префикс, оставляя хостовую часть
		this->_addr->impose(std::get <0> (sample), awh::net_addr_t::addr_t::HOST);
		// Проверяем полученный хост
		ASSERT_EQ(std::get <2> (sample), static_cast <std::string> (* this->_addr.get()))
			<< "префикс: " << static_cast <uint16_t> (std::get <0> (sample));
	}
}

/**
 * @brief Тест обратимости перевода префикса и маски для IPv6
 *
 * @details Перевод префикса в маску и обратный перевод обязаны сходиться. Прежде
 *          они расходились на всяком префиксе, не кратном шестнадцати: перевод
 *          префикса в маску шёл через наложение и переставлял октеты хекстета
 *          местами, а обратный перевод считал разряды правильно
 *
 */
TEST_F(NetFixture, NetPrefixMaskRoundTripIPv6Test){
	// Устанавливаем адрес IPv6, чтобы задать разновидность
	(* this->_addr.get()) = "2001:db8::1";
	/**
	 * Перебираем все допустимые длины префикса
	 */
	for(uint8_t prefix = 1; prefix <= 128; prefix++){
		// Переводим префикс в маску сети
		const std::string mask = this->_addr->prefix2Mask(prefix);
		// Обратный перевод обязан дать исходную длину префикса
		ASSERT_EQ(prefix, this->_addr->mask2Prefix(mask))
			<< "префикс: " << static_cast <uint16_t> (prefix) << ", маска: " << mask;
	}
}

/**
 * @brief Тест порядка адресов IPv6 при сравнении
 *
 * @details Адреса сравниваются побайтно прямо по буферу: он держит адрес в сетевом
 *          порядке, где старший октет идёт первым. Прежде адреса снимались с
 *          разворотом порядка байт, но разворачивался весь адрес целиком - это не
 *          сетевой порядок, а его полная противоположность, отчего "::1" оказывался
 *          больше "8000::"
 *
 */
TEST_F(NetFixture, NetCompareIPv6OrderTest){
	// Объект меньшего адреса
	awh::net_addr_t lower(this->_fmk.get(), this->_log.get());
	// Объект большего адреса
	awh::net_addr_t upper(this->_fmk.get(), this->_log.get());
	/**
	 * Набор образцов: меньший адрес и больший адрес
	 */
	const std::vector <std::pair <std::string, std::string>> samples = {
		// Различие в самом старшем разряде адреса
		{"::1", "8000::"},
		// Различие в старшем хекстете
		{"2001::", "fe80::"},
		// Различие в младшем хекстете при совпадающих старших
		{"2001:db8::1", "2001:db8::2"},
		// Границы диапазона адресов локальной связи
		{"fe80::", "febf:ffff:ffff:ffff:ffff:ffff:ffff:ffff"},
		// Нулевой адрес меньше любого другого
		{"::", "::1"}
	};
	/**
	 * Перебираем все образцы
	 */
	for(auto & sample : samples){
		// Выполняем разбор меньшего адреса
		ASSERT_TRUE(lower.parse(sample.first)) << "адрес: " << sample.first;
		// Выполняем разбор большего адреса
		ASSERT_TRUE(upper.parse(sample.second)) << "адрес: " << sample.second;
		// Меньший адрес обязан быть меньше большего
		ASSERT_TRUE(lower < upper) << sample.first << " < " << sample.second;
		// Больший адрес обязан быть больше меньшего
		ASSERT_TRUE(upper > lower) << sample.second << " > " << sample.first;
		// Нестрогие сравнения обязаны им отвечать
		ASSERT_TRUE(lower <= upper);
		// Нестрогое сравнение большего адреса
		ASSERT_TRUE(upper >= lower);
		// Обратные сравнения обязаны давать ложь
		ASSERT_FALSE(lower > upper);
		// Обратное сравнение большего адреса
		ASSERT_FALSE(upper < lower);
	}
}

/**
 * @brief Тест отказа на несплошной маске сети
 *
 * @details Маска сети обязана быть сплошной: единичные разряды идут подряд от
 *          старшего, а за первым нулевым разрядом стоят одни нули. Прежде разряды
 *          просто пересчитывались, и маска "255.0.255.0" давала префикс 16 - тот
 *          же, что и правильная маска "255.255.0.0". Для списков доступа и таблиц
 *          маршрутов такая подмена опасна: правило ложилось бы на чужую сеть
 *
 */
TEST_F(NetFixture, NetMaskContiguityTest){
	// Устанавливаем адрес IPv4, чтобы задать разновидность
	(* this->_addr.get()) = "192.168.0.1";
	/**
	 * Набор сплошных масок и отвечающих им длин префикса
	 */
	const std::vector <std::pair <std::string, uint8_t>> valid = {
		// Маска, режущая адрес по границе октета
		{"255.255.0.0", 16},
		// Маска, режущая адрес внутри октета
		{"255.255.255.128", 25},
		// Маска, оставляющая один старший разряд
		{"128.0.0.0", 1},
		// Маска, занимающая адрес целиком
		{"255.255.255.255", 32}
	};
	/**
	 * Перебираем все сплошные маски
	 */
	for(auto & sample : valid)
		// Сплошная маска обязана давать свою длину префикса
		ASSERT_EQ(sample.second, this->_addr->mask2Prefix(sample.first)) << "маска: " << sample.first;
	/**
	 * Набор несплошных масок: разряды в них идут вразнобой
	 */
	const std::vector <std::string> invalid = {
		// Нулевой октет между единичными
		"255.0.255.0",
		// Единичный разряд после нулевого внутри октета
		"255.255.255.129",
		// Единичный октет после нулевого
		"0.255.0.0",
		// Разряды вразнобой внутри старшего октета
		"170.0.0.0"
	};
	/**
	 * Перебираем все несплошные маски
	 */
	for(auto & sample : invalid)
		// Несплошная маска обязана быть отвергнута
		ASSERT_EQ(0, this->_addr->mask2Prefix(sample)) << "маска: " << sample;
	// Устанавливаем адрес IPv6, чтобы задать разновидность
	(* this->_addr.get()) = "2001:db8::1";
	// Сплошная маска IPv6 обязана давать свою длину префикса
	ASSERT_EQ(53, this->_addr->mask2Prefix("FFFF:FFFF:FFFF:F800::"));
	// Несплошная маска IPv6 обязана быть отвергнута
	ASSERT_EQ(0, this->_addr->mask2Prefix("FFFF:0:FFFF::"));
}

/**
 * @brief Тест согласия проверки и разбора аппаратного адреса
 *
 * @details Проверка принимает три записи аппаратного адреса: разделённую
 *          двоеточиями, разделённую дефисами и сплошную из двенадцати
 *          шестнадцатеричных цифр. Разбор же понимал одни лишь двоеточия, и запись,
 *          признанную проверкой годной, разобрать не мог - проверка обещала то,
 *          чего разбор не делал
 *
 */
TEST_F(NetFixture, NetMacCheckParseAgreementTest){
	/**
	 * Набор записей одного и того же аппаратного адреса
	 */
	const std::vector <std::string> valid = {
		// Запись, разделённая двоеточиями
		"AA:BB:CC:DD:EE:FF",
		// Запись, разделённая дефисами
		"AA-BB-CC-DD-EE-FF",
		// Сплошная запись без разделителей
		"AABBCCDDEEFF",
		// Запись строчными буквами
		"aa:bb:cc:dd:ee:ff"
	};
	/**
	 * Перебираем все записи аппаратного адреса
	 */
	for(auto & sample : valid){
		// Проверка обязана признать запись годной
		ASSERT_TRUE(this->_addr->check(sample, awh::net_addr_t::type_t::MAC)) << "запись: " << sample;
		// Разбор обязан принять всё, что признала годным проверка
		ASSERT_TRUE(this->_addr->parse(sample, awh::net_addr_t::type_t::MAC)) << "запись: " << sample;
		// Разбор с определением разновидности обязан дать аппаратный адрес
		ASSERT_TRUE(this->_addr->parse(sample)) << "запись: " << sample;
		// Разновидность разобранного адреса обязана быть аппаратной
		ASSERT_EQ(awh::net_addr_t::type_t::MAC, this->_addr->type()) << "запись: " << sample;
		// Все записи описывают один и тот же адрес
		ASSERT_EQ("AA:BB:CC:DD:EE:FF", static_cast <std::string> (* this->_addr.get())) << "запись: " << sample;
	}
	/**
	 * Набор заведомо негодных записей
	 */
	const std::vector <std::string> invalid = {
		// Запись с недостающим октетом
		"AA:BB:CC:DD:EE",
		// Запись с нешестнадцатеричным символом
		"AA:BB:CC:DD:EE:GG",
		// Сплошная запись неверной длины
		"AABBCCDDEEF",
		// Запись с негодным разделителем
		"AA.BB.CC.DD.EE.FF"
	};
	/**
	 * Перебираем все негодные записи
	 */
	for(auto & sample : invalid){
		// Проверка обязана отвергнуть негодную запись
		ASSERT_FALSE(this->_addr->check(sample, awh::net_addr_t::type_t::MAC)) << "запись: " << sample;
		// Разбор обязан отвергнуть то же самое
		ASSERT_FALSE(this->_addr->parse(sample, awh::net_addr_t::type_t::MAC)) << "запись: " << sample;
	}
}

/**
 * @brief Тест отказа на записи сети с пустым суффиксом
 *
 * @details Запись "10.0.0.0/" сетью не является: за косой чертой обязан стоять
 *          префикс либо маска. Прежде проверка на цифры давала на пустом промежутке
 *          истину, а перевод пустой записи в число - ноль, отчего такая запись
 *          принималась за сеть с нулевым префиксом, то есть за всю сеть целиком
 *
 */
TEST_F(NetFixture, NetEmptyNetworkSuffixTest){
	// Запись сети IPv4 с пустым суффиксом сетью не является
	ASSERT_FALSE(this->_addr->check("10.0.0.0/", awh::net_addr_t::type_t::NETV4));
	// Запись сети IPv6 с пустым суффиксом сетью не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/", awh::net_addr_t::type_t::NETV6));
	// Запись сети IPv4 с префиксом сетью является
	ASSERT_TRUE(this->_addr->check("10.0.0.0/8", awh::net_addr_t::type_t::NETV4));
	// Запись сети IPv4 с маской сетью является
	ASSERT_TRUE(this->_addr->check("10.0.0.0/255.0.0.0", awh::net_addr_t::type_t::NETV4));
	// Запись сети IPv6 с префиксом сетью является
	ASSERT_TRUE(this->_addr->check("2001:db8::/32", awh::net_addr_t::type_t::NETV6));
	// Определение разновидности записи с пустым суффиксом сетью её признать не должно
	ASSERT_NE(awh::net_addr_t::type_t::NETV4, this->_addr->host("10.0.0.0/"));
}

/**
 * @brief Тест проверки октетов записи ARPA
 *
 * @details Октеты перевёрнутой записи брались переводом в число без всяких
 *          проверок: метка "999" укладывалась в октет с потерей старших разрядов,
 *          а нецифровая метка давала ноль - негодная запись разбиралась в мусор
 *          молча, притом с положительным итогом
 *
 */
TEST_F(NetFixture, NetArpaOctetValidationTest){
	// Правильная запись обязана разбираться
	ASSERT_TRUE(this->_addr->arpa("1.0.168.192.in-addr.arpa"));
	// Проверяем восстановленный адрес
	ASSERT_EQ("192.168.0.1", static_cast <std::string> (* this->_addr.get()));
	/**
	 * Набор заведомо негодных записей
	 */
	const std::vector <std::string> invalid = {
		// Октет за пределами допустимого
		"999.0.168.192.in-addr.arpa",
		// Нецифровая метка
		"abc.0.168.192.in-addr.arpa",
		// Пустая метка
		"1.0..192.in-addr.arpa",
		// Метка длиннее трёх цифр
		"0001.0.168.192.in-addr.arpa",
		// Октет, выходящий за предел на единицу
		"256.0.168.192.in-addr.arpa"
	};
	/**
	 * Перебираем все негодные записи
	 */
	for(auto & sample : invalid)
		// Негодная запись обязана быть отвергнута
		ASSERT_FALSE(this->_addr->arpa(sample)) << "запись: " << sample;
}

/**
 * @brief Тест сброса разновидности адреса при неудачном разборе
 *
 * @details Неудачный разбор обязан оставлять объект пустым целиком: прежде
 *          очищался один лишь буфер, а разновидность доставалась от прежнего
 *          адреса - и type, own и print отвечали по виду, которого в объекте
 *          уже не было
 *
 */
TEST_F(NetFixture, NetParseFailureClearsTypeTest){
	// Выполняем разбор верного адреса
	ASSERT_TRUE(this->_addr->parse("192.168.0.1"));
	// Проверяем разновидность разобранного адреса
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->type());
	// Неудачный разбор обязан отвергнуть запись
	ASSERT_FALSE(this->_addr->parse("not-an-address"));
	// Разновидность обязана быть сброшена вместе с буфером
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
	// Печать пустого объекта обязана дать пустую запись
	ASSERT_EQ("", this->_addr->print());
	// Выполняем разбор верного адреса IPv6
	ASSERT_TRUE(this->_addr->parse("fe80::1"));
	// Неудачный разбор заданной разновидностью обязан сбросить объект тоже
	ASSERT_FALSE(this->_addr->parse("192.168.0.1", awh::net_addr_t::type_t::IPV6));
	// Разновидность обязана быть сброшена
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
}

/**
 * @brief Тест переноса зоны присваиванием и её сброса очисткой
 *
 * @details Зона принадлежит содержимому объекта: очистка снимает её вместе с
 *          адресом, а присваивание переносит вместе с ним. Прежде зона
 *          переживала и то, и другое - адрес локальной связи, перенесённый
 *          присваиванием, зону свою терял, а после очистки она доставалась
 *          следующему содержимому
 *
 */
TEST_F(NetFixture, NetZoneClearAndAssignTest){
	// Объект-получатель для присваивания
	awh::net_addr_t copy(this->_fmk.get(), this->_log.get());
	// Выполняем разбор адреса с зоной
	ASSERT_TRUE(this->_addr->parse("fe80::1%eth0"));
	// Проверяем зону, извлечённую из адреса
	ASSERT_EQ("eth0", this->_addr->zone());
	// Переносим адрес присваиванием
	copy = (* this->_addr.get());
	// Адрес обязан совпасть
	ASSERT_EQ("FE80::1%eth0", static_cast <std::string> (copy));
	// Зона обязана перенестись вместе с адресом
	ASSERT_EQ("eth0", copy.zone());
	// Очищаем исходный объект
	this->_addr->clear();
	// Разновидность обязана быть сброшена
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
	// Зона обязана быть сброшена вместе с адресом
	ASSERT_TRUE(this->_addr->zone().empty());
	// Выполняем разбор адреса IPv4 тем же объектом
	ASSERT_TRUE(this->_addr->parse("10.0.0.1"));
	// Зона прежнего адреса новому достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
	// Проверяем запись адреса
	ASSERT_EQ("10.0.0.1", static_cast <std::string> (* this->_addr.get()));
}

/**
 * @brief Тест очистки объекта при неудачном разборе записи ARPA
 *
 * @note Разбор ведётся прямо в буфере объекта, и отказ посреди него оставлял
 *       буфер новой разновидности при прежнем виде адреса: у бывшего IPv6
 *       выходил буфер о четырёх байтах
 *
 */
TEST_F(NetFixture, NetArpaFailureResetsAddressTest){
	/**
	 * Набор записей, отказ по которым приходится уже после переписи буфера
	 */
	const std::vector <std::string> invalid = {
		// Октет за пределами допустимого
		"999.0.168.192.in-addr.arpa",
		// Нецифровая метка
		"abc.0.168.192.in-addr.arpa",
		// Негодный разряд записи IPv6
		"z.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa"
	};
	/**
	 * Перебираем все негодные записи
	 */
	for(auto & sample : invalid){
		// Заводим заведомо годный адрес IPv6
		ASSERT_TRUE(this->_addr->parse("2001:db8::1"));
		// Негодная запись обязана быть отвергнута
		ASSERT_FALSE(this->_addr->arpa(sample)) << "запись: " << sample;
		// Вид адреса обязан быть сброшен вместе с буфером
		ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type()) << "запись: " << sample;
		// Запись отвергнутого адреса обязана быть пустой
		ASSERT_TRUE(static_cast <std::string> (* this->_addr.get()).empty()) << "запись: " << sample;
	}
}

/**
 * @brief Тест порядка сличения аппаратных адресов
 *
 * @note Адрес хранится от кода изготовителя к номеру устройства, и порядок
 *       адресов обязан идти по нему же: двоичное сличение чисел перебирает
 *       байты с последнего и давало порядок перевёрнутым
 *
 */
TEST_F(NetFixture, NetCompareMacOrderTest){
	// Заводим адрес, старший по коду изготовителя
	awh::net_addr_t first(this->_fmk.get(), this->_log.get());
	// Заводим адрес, старший по номеру устройства
	awh::net_addr_t second(this->_fmk.get(), this->_log.get());
	// Выполняем разбор старшего адреса
	ASSERT_TRUE(first.parse("ff:00:00:00:00:00", awh::net_addr_t::type_t::MAC));
	// Выполняем разбор младшего адреса
	ASSERT_TRUE(second.parse("00:00:00:00:00:ff", awh::net_addr_t::type_t::MAC));
	// Старшинство определяет код изготовителя, а не номер устройства
	ASSERT_TRUE(second < first);
	// Проверяем обратное сличение адресов
	ASSERT_TRUE(first > second);
	// Проверяем нестрогое сличение адресов
	ASSERT_TRUE(second <= first);
	// Проверяем обратное нестрогое сличение адресов
	ASSERT_TRUE(first >= second);
	// Совпадающие адреса нестрогому сличению обязаны отвечать оба раза
	ASSERT_TRUE(first <= first);
	// Проверяем обратное нестрогое сличение совпадающих адресов
	ASSERT_TRUE(first >= first);
	// Совпадающие адреса строгому сличению отвечать не обязаны
	ASSERT_FALSE(first < first);
	// Проверяем обратное строгое сличение совпадающих адресов
	ASSERT_FALSE(first > first);
}

/**
 * @brief Тест сверки разновидности самого адреса при проверке сети и диапазона
 *
 * @note Снятие значения ведётся по переданному виду, и адрес другой
 *       разновидности снимался чужим методом - выходил ноль, а с ним и
 *       ложное соответствие
 *
 */
TEST_F(NetFixture, NetMappingTypeMismatchTest){
	// Заводим начало диапазона адресов IPv4
	awh::net_addr_t begin(this->_fmk.get(), this->_log.get());
	// Заводим конец диапазона адресов IPv4
	awh::net_addr_t end(this->_fmk.get(), this->_log.get());
	// Выполняем разбор начала диапазона
	ASSERT_TRUE(begin.parse("10.0.0.0"));
	// Выполняем разбор конца диапазона
	ASSERT_TRUE(end.parse("10.255.255.255"));
	// Заводим адрес другой разновидности
	ASSERT_TRUE(this->_addr->parse("2001:db8::1"));
	// Сеть IPv4 адресу IPv6 отвечать не может
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", awh::net_addr_t::type_t::IPV4));
	// Проверяем сеть с явным префиксом
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", 8, awh::net_addr_t::addr_t::NETWORK, awh::net_addr_t::type_t::IPV4));
	// Диапазон IPv4 адресу IPv6 отвечать не может
	ASSERT_FALSE(this->_addr->range(begin, end, 8, awh::net_addr_t::type_t::IPV4));
	// Заводим адрес той же разновидности, в диапазон входящий
	ASSERT_TRUE(this->_addr->parse("10.1.2.3"));
	// Проверяем, что сверка вида годному адресу не мешает
	ASSERT_TRUE(this->_addr->mapping("10.0.0.0", awh::net_addr_t::type_t::IPV4));
	// Проверяем вхождение годного адреса в диапазон
	ASSERT_TRUE(this->_addr->range(begin, end, 8, awh::net_addr_t::type_t::IPV4));
}

/**
 * @brief Тест отнесения адресов связи к одному разряду у обеих разновидностей
 *
 * @note Сеть 169.254.0.0/16 и сеть fe80::/10 - двойники, и разряд у них обязан
 *       быть один: хост назначает такой адрес себе сам, оставшись без
 *       выдающего их узла
 *
 */
TEST_F(NetFixture, NetLinkLocalOwnAgreementTest){
	// Выполняем разбор адреса связи IPv4
	ASSERT_TRUE(this->_addr->parse("169.254.1.1"));
	// Адрес связи IPv4 относится к локальным
	ASSERT_EQ(awh::net_addr_t::own_t::LAN, this->_addr->own());
	// Выполняем разбор адреса связи IPv6
	ASSERT_TRUE(this->_addr->parse("fe80::1"));
	// Адрес связи IPv6 относится к локальным
	ASSERT_EQ(awh::net_addr_t::own_t::LAN, this->_addr->own());
}

/**
 * @brief Тест отказа от проверки сети при негодной маске
 *
 * @note Признак итога заводился истиной ещё до перевода маски в префикс, и
 *       остаться ей было негде: несплошная маска отдавала соответствие сети
 *       всякому адресу
 *
 */
TEST_F(NetFixture, NetMappingInvalidMaskTest){
	// Выполняем разбор адреса, к сети заведомо не принадлежащего
	ASSERT_TRUE(this->_addr->parse("192.168.1.1"));
	// Несплошная маска сети годной не является
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", "255.0.255.0", awh::net_addr_t::addr_t::NETWORK));
	// Нулевая маска сети годной не является
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", "0.0.0.0", awh::net_addr_t::addr_t::NETWORK));
	// Негодная запись маски сети годной не является
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", "нечто", awh::net_addr_t::addr_t::NETWORK));
	// Сплошная маска сети адресу чужой сети отвечать не должна
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", "255.0.0.0", awh::net_addr_t::addr_t::NETWORK));
	// Выполняем разбор адреса, сети принадлежащего
	ASSERT_TRUE(this->_addr->parse("10.1.2.3"));
	// Сплошная маска сети своему адресу отвечать обязана
	ASSERT_TRUE(this->_addr->mapping("10.0.0.0", "255.0.0.0", awh::net_addr_t::addr_t::NETWORK));
	// Несплошная маска годной не становится и при своём адресе
	ASSERT_FALSE(this->_addr->mapping("10.0.0.0", "255.0.255.0", awh::net_addr_t::addr_t::NETWORK));
}

/**
 * @brief Тест сверки разновидности адреса при проверке диапазона, заданного записями
 *
 * @note Проверка эта была выписана заново и вид самого адреса не сверяла: снятие
 *       четырёхбайтного значения с адреса IPv6 давало ноль, а хостовая часть
 *       нуля в диапазон попадала легко
 *
 */
TEST_F(NetFixture, NetRangeStringTypeMismatchTest){
	// Выполняем разбор адреса другой разновидности
	ASSERT_TRUE(this->_addr->parse("2001:db8::1"));
	// Диапазон IPv4 адресу IPv6 отвечать не может
	ASSERT_FALSE(this->_addr->range("10.0.0.0", "10.255.255.255", static_cast <uint8_t> (8), awh::net_addr_t::type_t::IPV4));
	// Выполняем разбор адреса, в диапазон входящего
	ASSERT_TRUE(this->_addr->parse("10.1.2.3"));
	// Проверяем, что сверка вида годному адресу не мешает
	ASSERT_TRUE(this->_addr->range("10.0.0.0", "10.255.255.255", static_cast <uint8_t> (8), awh::net_addr_t::type_t::IPV4));
}

/**
 * @brief Тест очистки зоны при опустошении объекта
 *
 * @note Очистка зоны стояла лишь в начале разбора, а разбор записи пустой до
 *       неё не доходил вовсе - зона переживала и пустую запись, и неудачу
 *
 */
TEST_F(NetFixture, NetZoneResetOnEmptyParseTest){
	// Выполняем разбор адреса связи с зоной
	ASSERT_TRUE(this->_addr->parse("fe80::1%en0"));
	// Проверяем, что зона адреса разобрана
	ASSERT_EQ("en0", this->_addr->zone());
	// Пустая запись адресом не является
	ASSERT_FALSE(this->_addr->parse(""));
	// Зона пустому объекту достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
	// Выполняем разбор адреса связи с зоной заново
	ASSERT_TRUE(this->_addr->parse("fe80::1%en0"));
	// Проверяем, что зона адреса разобрана
	ASSERT_EQ("en0", this->_addr->zone());
	// Негодная запись адресом не является
	ASSERT_FALSE(this->_addr->parse("нечто"));
	// Зона пустому объекту достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
	// Выполняем разбор адреса связи с зоной заново
	ASSERT_TRUE(this->_addr->parse("fe80::1%en0"));
	// Выполняем разбор записи ARPA, зоны не несущей
	ASSERT_TRUE(this->_addr->arpa("1.0.168.192.in-addr.arpa"));
	// Зона прежнего адреса разобранной записи достаться не должна
	ASSERT_TRUE(this->_addr->zone().empty());
}

/**
 * @brief Тест отказа от вырожденного сжатия нулей записи IPv6
 *
 * @note Двойное двоеточие означает одну или несколько групп нулей
 *       (RFC 4291 2.2), и записи о восьми словах сжимать нечего: прежде такая
 *       запись давала тот же адрес, что и правильная
 *
 */
TEST_F(NetFixture, NetIPv6DegenerateCompressionTest){
	// Запись о восьми словах со сжатием годной не является
	ASSERT_FALSE(this->_addr->parse("1:2:3:4:5:6:7::8"));
	// Проверяем отказ от такой записи и при явно заданной разновидности
	ASSERT_FALSE(this->_addr->parse("1:2:3:4:5:6:7:8::", awh::net_addr_t::type_t::IPV6));
	// Проверяем отказ от такой записи и у проверки
	ASSERT_FALSE(this->_addr->check("1:2:3:4:5:6:7::8", awh::net_addr_t::type_t::IPV6));
	// Сжатие одного слова годным остаётся
	ASSERT_TRUE(this->_addr->parse("1:2:3:4:5:6::8"));
	// Проверяем восстановленный адрес
	ASSERT_EQ("1:2:3:4:5:6:0:8", static_cast <std::string> (* this->_addr.get()));
	// Запись без сжатия годной остаётся
	ASSERT_TRUE(this->_addr->parse("1:2:3:4:5:6:7:8"));
	// Запись из одних нулей годной остаётся
	ASSERT_TRUE(this->_addr->parse("::"));
}

/**
 * @brief Тест сплошности маски в записи сети
 *
 * @note Проверка принимала за годную сеть запись с несплошной маской, а перевод
 *       той же маски в префикс отдавал отказ
 *
 */
TEST_F(NetFixture, NetCheckMaskContiguityTest){
	// Запись сети со сплошной маской годной является
	ASSERT_TRUE(this->_addr->check("10.0.0.0/255.0.0.0", awh::net_addr_t::type_t::NETV4));
	// Запись сети с несплошной маской годной не является
	ASSERT_FALSE(this->_addr->check("10.0.0.0/255.0.255.0", awh::net_addr_t::type_t::NETV4));
	// Запись сети с префиксом вместо маски годной остаётся
	ASSERT_TRUE(this->_addr->check("10.0.0.0/8", awh::net_addr_t::type_t::NETV4));
	// Запись сети IPv6 со сплошной маской годной является
	ASSERT_TRUE(this->_addr->check("2001:db8::/ffff:ffff::", awh::net_addr_t::type_t::NETV6));
	// Запись сети IPv6 с несплошной маской годной не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/ffff:0:ffff::", awh::net_addr_t::type_t::NETV6));
}

/**
 * @brief Тест сверки префикса с длиной буфера при наложении
 *
 * @note Предел префикса брался из переданного вида, а длина - из буфера, и вид
 *       этот с содержимым объекта сойтись не обязан: наложение префикса 128 на
 *       четырёхбайтный адрес писало за пределы буфера
 *
 */
TEST_F(NetFixture, NetImposePrefixBoundTest){
	// Выполняем разбор адреса IPv4
	ASSERT_TRUE(this->_addr->parse("192.168.1.42"));
	// Накладываем префикс, четырёхбайтному адресу не отвечающий
	this->_addr->impose(static_cast <uint8_t> (127), awh::net_addr_t::addr_t::NETWORK, awh::net_addr_t::type_t::IPV6);
	// Адрес обязан остаться нетронутым
	ASSERT_EQ("192.168.1.42", static_cast <std::string> (* this->_addr.get()));
	// Накладываем префикс, адресу отвечающий
	this->_addr->impose(static_cast <uint8_t> (24), awh::net_addr_t::addr_t::NETWORK, awh::net_addr_t::type_t::IPV4);
	// Проверяем полученный адрес сети
	ASSERT_EQ("192.168.1.0", static_cast <std::string> (* this->_addr.get()));
}

/**
 * @brief Тест отбраковки негодной длины префикса в записи сети
 *
 * @note Длина префикса читалась однобайтным числом, а перевод при переполнении
 *       отдаёт ноль: запись "10.0.0.0/256" выходила годной сетью с нулевым
 *       префиксом - тем самым, что означает "любой адрес"
 *
 */
TEST_F(NetFixture, NetCheckPrefixOverflowTest){
	/**
	 * Набор записей сети с негодной длиной префикса
	 */
	const std::vector <std::string> invalid = {
		// Длина префикса, переполняющая однобайтное число
		"10.0.0.0/256",
		// Длина префикса, заведомо выходящая за пределы
		"10.0.0.0/999",
		// Длина префикса на единицу сверх допустимой
		"10.0.0.0/33",
		// Длина префикса, переполняющая двухбайтное число
		"10.0.0.0/65536"
	};
	/**
	 * Перебираем все негодные записи сети
	 */
	for(auto & sample : invalid){
		// Негодная запись сети годной признаваться не должна
		ASSERT_FALSE(this->_addr->check(sample, awh::net_addr_t::type_t::NETV4)) << "запись: " << sample;
		// Определение разновидности негодной записи сети её выдавать не должно
		ASSERT_NE(awh::net_addr_t::type_t::NETV4, this->_addr->host(sample)) << "запись: " << sample;
	}
	// Годная запись сети годной остаётся
	ASSERT_TRUE(this->_addr->check("10.0.0.0/8", awh::net_addr_t::type_t::NETV4));
	// Наибольшая допустимая длина префикса годной остаётся
	ASSERT_TRUE(this->_addr->check("10.0.0.0/32", awh::net_addr_t::type_t::NETV4));
	// Длина префикса сети IPv6 сверх допустимой годной не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/129", awh::net_addr_t::type_t::NETV6));
	// Длина префикса сети IPv6, переполняющая однобайтное число, годной не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/999", awh::net_addr_t::type_t::NETV6));
	// Наибольшая допустимая длина префикса сети IPv6 годной остаётся
	ASSERT_TRUE(this->_addr->check("2001:db8::/128", awh::net_addr_t::type_t::NETV6));
}

/**
 * @brief Тест печати объекта, адреса не несущего
 *
 * @note Пустому буферу дописывался нулевой адрес по одной лишь пометке о виде, и
 *       объект, адреса не несущий, выглядел несущим нулевой
 *
 */
TEST_F(NetFixture, NetPrintEmptyBufferTest){
	// Выставляем вид адреса, самого адреса не задавая
	this->_addr->type(awh::net_addr_t::type_t::IPV4);
	// Запись объекта без адреса обязана быть пустой
	ASSERT_TRUE(this->_addr->print().empty());
	// Выставляем вид адреса IPv6
	this->_addr->type(awh::net_addr_t::type_t::IPV6);
	// Запись объекта без адреса обязана быть пустой
	ASSERT_TRUE(this->_addr->print().empty());
	// Выставляем вид аппаратного адреса
	this->_addr->type(awh::net_addr_t::type_t::MAC);
	// Запись объекта без адреса обязана быть пустой
	ASSERT_TRUE(this->_addr->print().empty());
	// Разобранный адрес выписывается как прежде
	ASSERT_TRUE(this->_addr->parse("192.168.1.42"));
	// Проверяем запись разобранного адреса
	ASSERT_EQ("192.168.1.42", this->_addr->print());
}

/**
 * @brief Тест обратной читаемости сплошной записи аппаратного адреса
 *
 * @note Короткий вид снимает ведущие нули, а без разделителя снятый нуль обратно
 *       не прочесть: запись "01:02:03:04:05:06" выходила как "123456"
 *
 */
TEST_F(NetFixture, NetMacSolidPrintRoundTripTest){
	// Выполняем разбор аппаратного адреса с ведущими нулями в разрядах
	ASSERT_TRUE(this->_addr->parse("01:02:03:04:05:06", awh::net_addr_t::type_t::MAC));
	// Снимаем сплошную запись адреса в коротком виде
	const std::string solid = this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX, 0);
	// Сплошная запись обязана нести все двенадцать разрядов
	ASSERT_EQ("010203040506", solid);
	// Разбор обязан принять снятую запись обратно
	ASSERT_TRUE(this->_addr->parse(solid, awh::net_addr_t::type_t::MAC));
	// Разобранный адрес обязан совпасть с исходным
	ASSERT_EQ("01:02:03:04:05:06", static_cast <std::string> (* this->_addr.get()));
}

/**
 * @brief Тест разбора области действия адреса связи
 *
 * @note Экранированный знак "%25" заведён для записи адреса внутри строки
 *       запроса (RFC 6874), а вне её означает сам себя: обозначение области
 *       числом читалось областью пустой
 *
 */
TEST_F(NetFixture, NetZoneEscapedDelimiterTest){
	// Область, обозначенная именем, разбирается как прежде
	ASSERT_TRUE(this->_addr->parse("fe80::1%en0"));
	// Проверяем разобранную область действия адреса
	ASSERT_EQ("en0", this->_addr->zone());
	// Область, обозначенная числом, областью пустой быть не должна
	ASSERT_TRUE(this->_addr->parse("fe80::1%25"));
	// Проверяем разобранную область действия адреса
	ASSERT_EQ("25", this->_addr->zone());
	// Запись в скобках экранированный знак разделителя снимает
	ASSERT_TRUE(this->_addr->parse("[fe80::1%25en0]"));
	// Проверяем разобранную область действия адреса
	ASSERT_EQ("en0", this->_addr->zone());
	// Разделитель области без самой области записью не является
	ASSERT_FALSE(this->_addr->parse("fe80::1%"));
	// Негодная запись адреса объекта за собой не оставляет
	ASSERT_TRUE(this->_addr->zone().empty());
	// Проверка обязана отвергнуть ту же запись
	ASSERT_FALSE(this->_addr->check("fe80::1%", awh::net_addr_t::type_t::IPV6));
}

/**
 * @brief Тест разбора вложенного адреса IPv4 при определении принадлежности
 *
 * @note Узел, слушающий по IPv6, получает подключения узлов IPv4 записью
 *       "::ffff:192.168.1.1": разряд такого адреса определяет вложенный адрес,
 *       а не оболочка - прежде он выходил внешним
 *
 */
TEST_F(NetFixture, NetOwnMappedIPv4Test){
	// Выполняем разбор вложенного частного адреса
	ASSERT_TRUE(this->_addr->parse("::ffff:192.168.1.1"));
	// Вложенный частный адрес относится к локальным
	ASSERT_EQ(awh::net_addr_t::own_t::LAN, this->_addr->own());
	// Выполняем разбор вложенной петли
	ASSERT_TRUE(this->_addr->parse("::ffff:127.0.0.1"));
	// Вложенная петля относится к служебным
	ASSERT_EQ(awh::net_addr_t::own_t::SYS, this->_addr->own());
	// Выполняем разбор вложенного внешнего адреса
	ASSERT_TRUE(this->_addr->parse("::ffff:8.8.8.8"));
	// Вложенный внешний адрес остаётся внешним
	ASSERT_EQ(awh::net_addr_t::own_t::WAN, this->_addr->own());
	// Выполняем разбор обычного адреса IPv6
	ASSERT_TRUE(this->_addr->parse("2001:4860:4860::8888"));
	// Обычный адрес IPv6 разбирается своей таблицей
	ASSERT_EQ(awh::net_addr_t::own_t::WAN, this->_addr->own());
	// Выполняем разбор частного адреса IPv6
	ASSERT_TRUE(this->_addr->parse("fc00::1"));
	// Частный адрес IPv6 относится к локальным
	ASSERT_EQ(awh::net_addr_t::own_t::LAN, this->_addr->own());
}

/**
 * @brief Тест закрепления намеренных краёв договора
 *
 * @details Свойства эти многократно принимались за дефекты сличением со
 *          стороны, и все они намеренны: закрепляются здесь, чтобы правка,
 *          меняющая их молча, была видна
 *
 */
TEST_F(NetFixture, NetDeliberateContractEdgesTest){
	/**
	 * Сеть из одних нулей значащих частей не имеет, и подходит под неё всякий
	 * адрес: правило хвостовых нулей доходит здесь до предела, отвечающего сети "/0"
	 */
	// Выполняем разбор заведомо внешнего адреса
	ASSERT_TRUE(this->_addr->parse("8.8.8.8"));
	// Сеть из одних нулей подходит всякому адресу
	ASSERT_TRUE(this->_addr->mapping("0.0.0.0"));
	// Выполняем разбор адреса IPv6
	ASSERT_TRUE(this->_addr->parse("2001:db8::1"));
	// Сеть IPv6 из одних нулей подходит всякому адресу
	ASSERT_TRUE(this->_addr->mapping("::"));
	/**
	 * Нулевая длина префикса означает и отказ, и настоящую сеть "/0": запись сети
	 * с нулевым префиксом годна, а рабочие методы при нуле ничего не делают
	 */
	// Запись сети с нулевым префиксом годной является
	ASSERT_TRUE(this->_addr->check("10.0.0.0/0", awh::net_addr_t::type_t::NETV4));
	// Нулевая маска сети даёт нулевую длину префикса
	ASSERT_EQ(0, this->_addr->mask2Prefix("0.0.0.0", awh::net_addr_t::type_t::IPV4));
	// Нулевая длина префикса даёт пустую маску сети
	ASSERT_TRUE(this->_addr->prefix2Mask(0, awh::net_addr_t::type_t::IPV4).empty());
	// Выполняем разбор адреса IPv4
	ASSERT_TRUE(this->_addr->parse("192.168.1.42"));
	// Наложение нулевого префикса адреса не трогает
	this->_addr->impose(static_cast <uint8_t> (0), awh::net_addr_t::addr_t::NETWORK);
	// Проверяем, что адрес остался нетронутым
	ASSERT_EQ("192.168.1.42", static_cast <std::string> (* this->_addr.get()));
	/**
	 * Вхождение в диапазон сличает остатки адресов за префиксом, а сеть не
	 * проверяет: адрес чужой сети с тем же остатком в диапазон проходит
	 */
	// Заводим начало диапазона адресов
	awh::net_addr_t begin(this->_fmk.get(), this->_log.get());
	// Заводим конец диапазона адресов
	awh::net_addr_t end(this->_fmk.get(), this->_log.get());
	// Выполняем разбор начала диапазона
	ASSERT_TRUE(begin.parse("192.168.3.100"));
	// Выполняем разбор конца диапазона
	ASSERT_TRUE(end.parse("192.168.3.200"));
	// Выполняем разбор адреса чужой сети с подходящим остатком
	ASSERT_TRUE(this->_addr->parse("10.0.0.150"));
	// Адрес чужой сети в диапазон проходит: сеть не сличается
	ASSERT_TRUE(this->_addr->range(begin, end, static_cast <uint8_t> (24)));
	/**
	 * Область действия в сличение адресов не входит: она обозначает не сам адрес,
	 * а сторону, с которой он достижим
	 */
	// Заводим адрес связи с одной областью действия
	awh::net_addr_t first(this->_fmk.get(), this->_log.get());
	// Заводим адрес связи с другой областью действия
	awh::net_addr_t second(this->_fmk.get(), this->_log.get());
	// Выполняем разбор адреса связи с одной областью
	ASSERT_TRUE(first.parse("fe80::1%en0"));
	// Выполняем разбор адреса связи с другой областью
	ASSERT_TRUE(second.parse("fe80::1%en1"));
	// Адреса с разными областями действия равны между собой
	ASSERT_TRUE(first == second);
	// Области действия при этом разные
	ASSERT_NE(first.zone(), second.zone());
	/**
	 * Разделители аппаратного адреса допускаются вперемешку - и проверкой, и
	 * разбором
	 */
	// Проверка обязана признать запись со смешанными разделителями годной
	ASSERT_TRUE(this->_addr->check("AA:BB-CC:DD-EE:FF", awh::net_addr_t::type_t::MAC));
	// Разбор обязан принять ту же запись
	ASSERT_TRUE(this->_addr->parse("AA:BB-CC:DD-EE:FF", awh::net_addr_t::type_t::MAC));
	// Проверяем разобранный аппаратный адрес
	ASSERT_EQ("AA:BB:CC:DD:EE:FF", static_cast <std::string> (* this->_addr.get()));
	/**
	 * Строгий режим разбора отменяет сокращённые записи IPv4 и записи не в
	 * десятичной системе, а нестрогий их принимает: по умолчанию режим нестрогий
	 */
	// Нестрогий разбор принимает восьмеричную запись
	ASSERT_TRUE(this->_addr->parse("010.1.1.1", awh::net_addr_t::type_t::IPV4));
	// Нестрогий разбор принимает сокращённую запись
	ASSERT_TRUE(this->_addr->parse("10.1", awh::net_addr_t::type_t::IPV4));
	// Выставляем строгий режим разбора
	this->_addr->strict(true);
	// Строгий разбор восьмеричную запись отвергает
	ASSERT_FALSE(this->_addr->parse("010.1.1.1", awh::net_addr_t::type_t::IPV4));
	// Строгий разбор сокращённую запись отвергает
	ASSERT_FALSE(this->_addr->parse("10.1", awh::net_addr_t::type_t::IPV4));
	// Строгий разбор обычную запись принимает
	ASSERT_TRUE(this->_addr->parse("10.1.1.1", awh::net_addr_t::type_t::IPV4));
	// Возвращаем нестрогий режим разбора
	this->_addr->strict(false);
}

/**
 * @brief Тест сплошной записи адреса IPv6
 *
 * @note Короткий вид сжимает нулевые разряды двойным разделителем, а разделителя
 *       у сплошной записи нет вовсе: сжатие выписывало на его месте нулевой
 *       символ, и запись обрывалась на первом же сжатии - строка держала полную
 *       длину, а читалась до первого нуля
 *
 */
TEST_F(NetFixture, NetIPv6SolidPrintTest){
	// Выполняем разбор адреса IPv6 со сжимаемой серединой
	ASSERT_TRUE(this->_addr->parse("2001:db8::1"));
	/**
	 * Перебираем все виды записи адреса
	 */
	for(auto size : {awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_size_t::LONG}){
		// Снимаем сплошную запись адреса
		const std::string solid = this->_addr->print(size, awh::net_addr_t::format_flag_t::HEX, 0);
		// Сплошная запись обязана нести все тридцать два разряда
		ASSERT_EQ(32, solid.size()) << "вид: " << static_cast <uint16_t> (size);
		// Запись обязана читаться целиком, а не до первого нулевого символа
		ASSERT_EQ(solid.size(), ::strlen(solid.c_str())) << "вид: " << static_cast <uint16_t> (size);
		// Проверяем саму запись адреса
		ASSERT_EQ("20010DB8000000000000000000000001", solid) << "вид: " << static_cast <uint16_t> (size);
	}
	// Запись с разделителем остаётся сжатой
	ASSERT_EQ("2001:DB8::1", this->_addr->print(awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_flag_t::HEX));
}

/**
 * @brief Тест отбраковки пустого шестнадцатеричного числа в записи IPv4
 *
 * @note Приставка системы счисления снималась без проверки на то, что за ней
 *       есть цифры: запись "0x" разбиралась молча в ноль
 *
 */
TEST_F(NetFixture, NetIPv4EmptyHexTokenTest){
	// Запись из одних приставок годной не является
	ASSERT_FALSE(this->_addr->parse("0x.0x.0x.0x", awh::net_addr_t::type_t::IPV4));
	// Запись с одной пустой приставкой годной не является
	ASSERT_FALSE(this->_addr->parse("0x1.0x.2.3", awh::net_addr_t::type_t::IPV4));
	// Проверка обязана отвергнуть ту же запись
	ASSERT_FALSE(this->_addr->check("0x.0x.0x.0x", awh::net_addr_t::type_t::IPV4));
	// Запись с непустыми приставками годной остаётся
	ASSERT_TRUE(this->_addr->parse("0xC0.0xA8.0x01.0x2A", awh::net_addr_t::type_t::IPV4));
	// Проверяем разобранный адрес
	ASSERT_EQ("192.168.1.42", static_cast <std::string> (* this->_addr.get()));
}

/**
 * @brief Тест записи ARPA у объекта, адреса не несущего
 *
 * @note Обход шёл по длине буфера у IPv4 и по шестнадцати байтам всегда у IPv6:
 *       у объекта с выставленным видом и пустым буфером запись собиралась из
 *       одного суффикса либо из байт, буфером уже не занятых
 *
 */
TEST_F(NetFixture, NetArpaEmptyBufferTest){
	// Выставляем вид адреса, самого адреса не задавая
	this->_addr->type(awh::net_addr_t::type_t::IPV4);
	// Запись объекта без адреса обязана быть пустой
	ASSERT_TRUE(this->_addr->arpa().empty());
	// Выставляем вид адреса IPv6
	this->_addr->type(awh::net_addr_t::type_t::IPV6);
	// Запись объекта без адреса обязана быть пустой
	ASSERT_TRUE(this->_addr->arpa().empty());
	// Разобранный адрес выписывается как прежде
	ASSERT_TRUE(this->_addr->parse("192.168.0.1"));
	// Проверяем запись разобранного адреса
	ASSERT_EQ("1.0.168.192.in-addr.arpa", this->_addr->arpa());
}

/**
 * @brief Тест печати адреса при несоответствии буфера выставленному виду
 *
 * @note Печать читала буфер по виду адреса, а вид выставляется и отдельно от
 *       содержимого: запись собиралась из байт, буфером уже не занятых
 *
 */
TEST_F(NetFixture, NetPrintTypeBufferMismatchTest){
	// Выполняем разбор адреса IPv4
	ASSERT_TRUE(this->_addr->parse("192.168.1.42"));
	// Выставляем вид, длине буфера не отвечающий
	this->_addr->type(awh::net_addr_t::type_t::IPV6);
	// Запись при несоответствии буфера виду обязана быть пустой
	ASSERT_TRUE(this->_addr->print().empty());
	// Выполняем разбор адреса IPv6
	ASSERT_TRUE(this->_addr->parse("2001:db8::1"));
	// Выставляем вид, длине буфера не отвечающий
	this->_addr->type(awh::net_addr_t::type_t::IPV4);
	// Запись при несоответствии буфера виду обязана быть пустой
	ASSERT_TRUE(this->_addr->print().empty());
	// Выставляем вид, длине буфера не отвечающий
	this->_addr->type(awh::net_addr_t::type_t::MAC);
	// Запись при несоответствии буфера виду обязана быть пустой
	ASSERT_TRUE(this->_addr->print().empty());
}

/**
 * @brief Тест записи аппаратного адреса при неприменимом виде записи
 *
 * @note Виды записи, вложенный адрес IPv4 касающиеся, аппаратному адресу смысла
 *       не имеют, но и записи его не отменяют: запрос с ними не подходил ни под
 *       одну ветвь, и адрес выписывался пустой строкой
 *
 */
TEST_F(NetFixture, NetMacPrintForeignFlagTest){
	// Выполняем разбор аппаратного адреса
	ASSERT_TRUE(this->_addr->parse("AA:BB:CC:DD:EE:FF", awh::net_addr_t::type_t::MAC));
	/**
	 * Перебираем все виды записи, аппаратному адресу неприменимые
	 */
	for(auto flag : {awh::net_addr_t::format_flag_t::HEX_IPV4, awh::net_addr_t::format_flag_t::HEX_IPV6}){
		/**
		 * Перебираем все размеры записи
		 */
		for(auto size : {awh::net_addr_t::format_size_t::SHORT, awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_size_t::LONG}){
			// Запись адреса пустой быть не должна
			ASSERT_FALSE(this->_addr->print(size, flag).empty())
				<< "вид: " << static_cast <uint16_t> (flag) << ", размер: " << static_cast <uint16_t> (size);
		}
	}
	// Средний вид записи выписывается разрядами полной ширины
	ASSERT_EQ("AA:BB:CC:DD:EE:FF", this->_addr->print(awh::net_addr_t::format_size_t::MIDDLE, awh::net_addr_t::format_flag_t::HEX_IPV4));
}

/**
 * @brief Тест решётки видов записи адреса
 *
 * @details Печать разбирается на три довода - подробность, систему счисления и
 *          разделитель, - и сочетаний у них выходит больше сотни на каждую
 *          разновидность адреса. Проверки покрывали из них единицы, и оба
 *          дефекта печати, найденные разбором - обрыв записи нулевым символом у
 *          сплошной записи IPv6 и пустая запись при неприменимом виде, - лежали
 *          именно в непройденных ветвях.
 *
 *          Сверяются здесь не сами записи, а свойства, которым обязана отвечать
 *          всякая: запись непуста, читается целиком, а длина её отвечает
 *          отведённому под неё месту. Ожидание записи для каждого сочетания
 *          пришлось бы выписывать вручную, и держалось бы оно на том же
 *          прочтении кода, что и сам код
 *
 */
TEST_F(NetFixture, NetPrintFormatMatrixTest){
	/**
	 * Набор образцов адресов всех разновидностей
	 */
	const std::vector <std::string> samples = {
		// Адрес IPv4 с нулями в разрядах
		"10.0.0.1",
		// Адрес IPv4 без нулей в разрядах
		"192.168.31.42",
		// Наибольший адрес IPv4
		"255.255.255.255",
		// Нулевой адрес IPv4
		"0.0.0.0",
		// Адрес IPv6 со сжимаемой серединой
		"2001:db8::1",
		// Адрес IPv6 без сжатия
		"2001:1234:abcd:5678:9877:3322:5541:aabb",
		// Адрес IPv6 со сжатием в начале
		"::1",
		// Нулевой адрес IPv6
		"::",
		// Адрес IPv6 со сжатием в конце
		"fe80::",
		// Адрес IPv6 с вложенным адресом IPv4
		"::ffff:192.168.1.1",
		// Аппаратный адрес с нулями в разрядах
		"01:02:03:04:05:06",
		// Аппаратный адрес без нулей в разрядах
		"AA:BB:CC:DD:EE:FF"
	};
	/**
	 * Набор всех подробностей записи
	 */
	const std::vector <awh::net_addr_t::format_size_t> sizes = {
		awh::net_addr_t::format_size_t::NONE,
		awh::net_addr_t::format_size_t::SHORT,
		awh::net_addr_t::format_size_t::MIDDLE,
		awh::net_addr_t::format_size_t::LONG
	};
	/**
	 * Набор всех систем счисления записи
	 */
	const std::vector <awh::net_addr_t::format_flag_t> flags = {
		awh::net_addr_t::format_flag_t::NONE,
		awh::net_addr_t::format_flag_t::HEX,
		awh::net_addr_t::format_flag_t::DECIMAL,
		awh::net_addr_t::format_flag_t::OCTAL,
		awh::net_addr_t::format_flag_t::HEX_IPV4,
		awh::net_addr_t::format_flag_t::HEX_IPV6
	};
	// Набор всех разделителей записи, включая заданный по умолчанию и снятый
	const std::vector <char> delims = {static_cast <char> (-1), 0, ':', '.', '-', '_'};
	// Количество пройденных сочетаний
	size_t count = 0;
	/**
	 * Перебираем все образцы адресов
	 */
	for(auto & sample : samples){
		// Выполняем разбор очередного образца
		ASSERT_TRUE(this->_addr->parse(sample)) << "адрес: " << sample;
		/**
		 * Перебираем все подробности записи
		 */
		for(auto size : sizes){
			/**
			 * Перебираем все системы счисления записи
			 */
			for(auto flag : flags){
				/**
				 * Перебираем все разделители записи
				 */
				for(auto delim : delims){
					// Снимаем запись адреса
					const std::string record = this->_addr->print(size, flag, delim);
					// Описание сочетания для сообщения об ошибке
					const std::string sign = (
						"адрес: " + sample +
						", подробность: " + std::to_string(static_cast <uint16_t> (size)) +
						", система: " + std::to_string(static_cast <uint16_t> (flag)) +
						", разделитель: " + std::to_string(static_cast <int16_t> (delim))
					);
					// Запись разобранного адреса пустой быть не может
					ASSERT_FALSE(record.empty()) << sign;
					// Запись обязана читаться целиком, а не до первого нулевого символа
					ASSERT_EQ(record.size(), ::strlen(record.c_str())) << sign;
					// Считаем пройденное сочетание
					count++;
				}
			}
		}
	}
	// Проверяем, что решётка пройдена целиком
	ASSERT_EQ(samples.size() * sizes.size() * flags.size() * delims.size(), count);
}

/**
 * @brief Метод сличения разбора и записи адреса IPv4 с системным разборщиком
 *
 */
TEST_F(NetFixture, NetSystemDifferentialIPv4Test){
	// Состояние порождателя псевдослучайных чисел, заданное намеренно для повторяемости прогона
	uint64_t state = 0x9E3779B97F4A7C15ULL;
	// Набор октетов очередного адреса
	uint8_t origin[4], result[4];
	// Запись адреса, полученная системным разборщиком
	char system[INET_ADDRSTRLEN];
	/**
	 * Перебираем набор случайных адресов
	 */
	for(uint32_t i = 0; i < 20000; i++){
		/**
		 * Набираем октеты очередного адреса
		 */
		for(uint8_t j = 0; j < 4; j++){
			// Продвигаем состояние порождателя
			state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
			// Извлекаем очередной октет из старших разрядов состояния
			origin[j] = static_cast <uint8_t> (state >> 56);
		}
		// Снимаем запись адреса системным разборщиком
		ASSERT_TRUE(::inet_ntop(AF_INET, origin, system, sizeof(system)) != nullptr);
		// Выполняем разбор системной записи адреса
		ASSERT_TRUE(this->_addr->parse(system)) << "запись: " << system;
		// Проверяем, что разновидность адреса определена верно
		ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->type()) << "запись: " << system;
		// Снимаем запись адреса разборщиком модуля
		const std::string record = this->_addr->print();
		// Выполняем разбор записи модуля системным разборщиком
		ASSERT_EQ(1, ::inet_pton(AF_INET, record.c_str(), result)) << "запись: " << record << ", система: " << system;
		// Проверяем, что адрес прошёл оба разборщика без изменений
		ASSERT_EQ(0, ::memcmp(origin, result, sizeof(origin))) << "запись: " << record << ", система: " << system;
	}
}

/**
 * @brief Метод сличения разбора и записи адреса IPv6 с системным разборщиком
 *
 */
TEST_F(NetFixture, NetSystemDifferentialIPv6Test){
	// Состояние порождателя псевдослучайных чисел, заданное намеренно для повторяемости прогона
	uint64_t state = 0xD1B54A32D192ED03ULL;
	// Набор октетов очередного адреса
	uint8_t origin[16], result[16];
	// Запись адреса, полученная системным разборщиком
	char system[INET6_ADDRSTRLEN];
	/**
	 * Перебираем набор случайных адресов
	 */
	for(uint32_t i = 0; i < 20000; i++){
		/**
		 * Набираем октеты очередного адреса
		 */
		for(uint8_t j = 0; j < 16; j++){
			// Продвигаем состояние порождателя
			state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
			/**
			 * Каждый четвёртый разряд обнуляем, чтобы в наборе оказались адреса
			 * с нулевыми группами — именно они проходят через сжатие записи
			 */
			origin[j] = (((state >> 48) & 3) == 0 ? 0 : static_cast <uint8_t> (state >> 56));
		}
		// Снимаем запись адреса системным разборщиком
		ASSERT_TRUE(::inet_ntop(AF_INET6, origin, system, sizeof(system)) != nullptr);
		// Выполняем разбор системной записи адреса
		ASSERT_TRUE(this->_addr->parse(system)) << "запись: " << system;
		// Проверяем, что разновидность адреса определена верно
		ASSERT_EQ(awh::net_addr_t::type_t::IPV6, this->_addr->type()) << "запись: " << system;
		// Снимаем запись адреса разборщиком модуля
		const std::string record = this->_addr->print();
		// Выполняем разбор записи модуля системным разборщиком
		ASSERT_EQ(1, ::inet_pton(AF_INET6, record.c_str(), result)) << "запись: " << record << ", система: " << system;
		// Проверяем, что адрес прошёл оба разборщика без изменений
		ASSERT_EQ(0, ::memcmp(origin, result, sizeof(origin))) << "запись: " << record << ", система: " << system;
	}
}

/**
 * @brief Метод проверки снятия и установки адреса в чистом виде всех разновидностей
 *
 */
TEST_F(NetFixture, NetSourceRoundTripTest){
	/**
	 * Набор образцов адресов всех разновидностей
	 */
	const std::vector <std::string> samples = {
		"192.168.31.7",
		"2001:db8:85a3::8a2e:370:7334",
		"00:1B:44:11:3A:B7"
	};
	/**
	 * Набор порядков следования байт
	 */
	const std::vector <awh::net_addr_t::endian_t> endians = {
		awh::net_addr_t::endian_t::LITTLE,
		awh::net_addr_t::endian_t::BIG
	};
	/**
	 * Перебираем все образцы адресов
	 */
	for(auto & sample : samples){
		/**
		 * Перебираем все порядки следования байт
		 */
		for(auto endian : endians){
			// Выполняем разбор очередного образца
			ASSERT_TRUE(this->_addr->parse(sample)) << "адрес: " << sample;
			// Запоминаем разновидность разобранного адреса
			const awh::net_addr_t::type_t type = this->_addr->type();
			// Снимаем адрес в чистом виде
			std::unique_ptr <awh::net::addr_t> source = this->_addr->source(endian);
			// Адрес в чистом виде выделен быть обязан
			ASSERT_TRUE(source != nullptr) << "адрес: " << sample;
			// Сбрасываем объект сетевого адреса
			this->_addr->clear();
			// Выполняем установку адреса в чистом виде
			this->_addr->source(source.get(), endian);
			// Проверяем, что разновидность адреса сохранена
			ASSERT_EQ(type, this->_addr->type()) << "адрес: " << sample;
			// Проверяем, что адрес прошёл снятие и установку без изменений
			ASSERT_TRUE(this->_addr->check(sample, type)) << "адрес: " << sample << ", запись: " << this->_addr->print();
		}
	}
}

/**
 * @brief Метод проверки установки адреса присвоением всех разновидностей
 *
 */
TEST_F(NetFixture, NetAssignmentOperatorsTest){
	// Набор октетов MAC-адреса
	const std::array <uint8_t, 6> mac = {0x00, 0x1B, 0x44, 0x11, 0x3A, 0xB7};
	// Набор октетов адреса IPv6
	const std::array <uint8_t, 16> ipv6 = {
		0x20, 0x01, 0x0D, 0xB8, 0x85, 0xA3, 0x00, 0x00,
		0x00, 0x00, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x34
	};
	// Выполняем установку MAC-адреса присвоением
	(* this->_addr.get()) = mac;
	// Проверяем разновидность установленного адреса
	ASSERT_EQ(awh::net_addr_t::type_t::MAC, this->_addr->type());
	// Проверяем, что адрес установлен без изменений
	ASSERT_EQ(mac, this->_addr->mac());
	// Выполняем установку адреса IPv6 присвоением
	(* this->_addr.get()) = ipv6;
	// Проверяем разновидность установленного адреса
	ASSERT_EQ(awh::net_addr_t::type_t::IPV6, this->_addr->type());
	// Проверяем, что адрес установлен без изменений
	ASSERT_EQ(ipv6, this->_addr->v6());
	// Выполняем установку адреса IPv4 присвоением
	(* this->_addr.get()) = static_cast <uint32_t> (0x0100A8C0);
	// Проверяем разновидность установленного адреса
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->type());
	// Проверяем, что адрес установлен без изменений
	ASSERT_EQ(static_cast <uint32_t> (0x0100A8C0), this->_addr->v4());
	// Выполняем установку разновидности адреса присвоением
	(* this->_addr.get()) = awh::net_addr_t::type_t::NONE;
	// Проверяем разновидность установленного адреса
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
	// Создаём второй объект сетевого адреса
	awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
	// Выполняем разбор адреса вторым объектом
	ASSERT_TRUE(addr.parse("10.20.30.40"));
	// Выполняем установку адреса присвоением объекта
	(* this->_addr.get()) = addr;
	// Проверяем, что адрес перенесён без изменений
	ASSERT_EQ("10.20.30.40", this->_addr->print());
}

/**
 * @brief Метод проверки сравнения адресов всех разновидностей
 *
 */
TEST_F(NetFixture, NetCompareOperatorsAllTypesTest){
	/**
	 * Набор пар адресов, где первый заведомо меньше второго
	 */
	const std::vector <std::pair <std::string, std::string>> samples = {
		{"192.168.31.7", "192.168.31.8"},
		{"2001:db8::1", "2001:db8::2"},
		{"00:1B:44:11:3A:B7", "00:1B:44:11:3A:B8"}
	};
	// Создаём второй объект сетевого адреса
	awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
	/**
	 * Перебираем все пары адресов
	 */
	for(auto & sample : samples){
		// Выполняем разбор меньшего адреса пары
		ASSERT_TRUE(this->_addr->parse(sample.first)) << "адрес: " << sample.first;
		// Выполняем разбор большего адреса пары
		ASSERT_TRUE(addr.parse(sample.second)) << "адрес: " << sample.second;
		// Описание пары для сообщения об ошибке
		const std::string sign = (sample.first + " и " + sample.second);
		// Проверяем сравнение адресов пары
		ASSERT_TRUE((* this->_addr.get()) < addr) << sign;
		ASSERT_TRUE((* this->_addr.get()) <= addr) << sign;
		ASSERT_FALSE((* this->_addr.get()) > addr) << sign;
		ASSERT_FALSE((* this->_addr.get()) >= addr) << sign;
		ASSERT_TRUE((* this->_addr.get()) != addr) << sign;
		ASSERT_FALSE((* this->_addr.get()) == addr) << sign;
		// Выполняем разбор большего адреса пары первым объектом
		ASSERT_TRUE(this->_addr->parse(sample.second)) << "адрес: " << sample.second;
		// Проверяем сравнение одинаковых адресов
		ASSERT_TRUE((* this->_addr.get()) == addr) << sign;
		ASSERT_FALSE((* this->_addr.get()) != addr) << sign;
		ASSERT_TRUE((* this->_addr.get()) <= addr) << sign;
		ASSERT_TRUE((* this->_addr.get()) >= addr) << sign;
		ASSERT_FALSE((* this->_addr.get()) < addr) << sign;
		ASSERT_FALSE((* this->_addr.get()) > addr) << sign;
	}
}

/**
 * @brief Метод проверки чтения и записи адреса потоком
 *
 */
TEST_F(NetFixture, NetStreamOperatorsTest){
	// Поток для чтения адреса
	std::istringstream input("192.168.31.7");
	// Выполняем чтение адреса из потока
	input >> (* this->_addr.get());
	// Проверяем разновидность прочитанного адреса
	ASSERT_EQ(awh::net_addr_t::type_t::IPV4, this->_addr->type());
	// Поток для записи адреса
	std::ostringstream output;
	// Выполняем запись адреса в поток
	output << (* this->_addr.get());
	// Проверяем запись адреса, выведенную в поток
	ASSERT_EQ("192.168.31.7", output.str());
	// Поток без записи адреса
	std::istringstream empty("");
	// Сбрасываем объект сетевого адреса
	this->_addr->clear();
	// Выполняем чтение адреса из пустого потока
	empty >> (* this->_addr.get());
	// Пустой поток адреса дать не может
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
}

/**
 * @brief Метод проверки разбора записи ARPA с необслуживаемым суффиксом
 *
 */
TEST_F(NetFixture, NetArpaUnknownSuffixTest){
	/**
	 * Набор записей, суффикс которых модулем не обслуживается
	 */
	const std::vector <std::string> samples = {
		// Запись вовсе без суффикса
		"1.0.168.192",
		// Запись с чужим суффиксом
		"1.0.168.192.in-addr.example",
		// Запись с суффиксом обратной зоны иного вида
		"1.0.168.192.ip4.arpa",
		// Запись пустая
		""
	};
	/**
	 * Перебираем все записи набора
	 */
	for(auto & sample : samples){
		// Выполняем разбор адреса IPv4 для заполнения объекта
		ASSERT_TRUE(this->_addr->parse("10.0.0.1"));
		// Разбор записи с необслуживаемым суффиксом удаться не может
		ASSERT_FALSE(this->_addr->arpa(sample)) << "запись: " << sample;
		// Неудачный разбор обязан оставить объект пустым целиком
		ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type()) << "запись: " << sample;
		// Запись пустого объекта пустой быть обязана
		ASSERT_TRUE(this->_addr->print().empty()) << "запись: " << sample;
	}
}

/**
 * @brief Метод проверки вхождения адреса в диапазон, заданный маской и разновидностью
 *
 */
TEST_F(NetFixture, NetRangeMaskWithTypeTest){
	// Выполняем разбор адреса IPv4
	ASSERT_TRUE(this->_addr->parse("192.168.31.7"));
	// Проверяем вхождение адреса в диапазон своей сети
	ASSERT_TRUE(this->_addr->range("192.168.31.1", "192.168.31.100", "255.255.255.0", awh::net_addr_t::type_t::IPV4));
	// Проверяем невхождение адреса в диапазон за его пределами
	ASSERT_FALSE(this->_addr->range("192.168.31.10", "192.168.31.100", "255.255.255.0", awh::net_addr_t::type_t::IPV4));
	// Пустая маска сети диапазона не задаёт
	ASSERT_FALSE(this->_addr->range("192.168.31.1", "192.168.31.100", "", awh::net_addr_t::type_t::IPV4));
	// Негодная маска сети диапазона не задаёт
	ASSERT_FALSE(this->_addr->range("192.168.31.1", "192.168.31.100", "255.0.255.0", awh::net_addr_t::type_t::IPV4));
	// Выполняем разбор адреса IPv6
	ASSERT_TRUE(this->_addr->parse("2001:db8::7"));
	// Проверяем вхождение адреса IPv6 в диапазон своей сети
	ASSERT_TRUE(this->_addr->range("2001:db8::1", "2001:db8::100", "ffff:ffff:ffff:ffff::", awh::net_addr_t::type_t::IPV6));
}

/**
 * @brief Метод проверки границ записи сети и доменного имени
 *
 */
TEST_F(NetFixture, NetCheckRecordBoundsTest){
	// Запись длины префикса длиннее трёх разрядов сетью не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/1280", awh::net_addr_t::type_t::NETV6));
	// Запись длины префикса больше допустимой сетью не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/129", awh::net_addr_t::type_t::NETV6));
	// Запись длины префикса в допустимых границах сетью является
	ASSERT_TRUE(this->_addr->check("2001:db8::/128", awh::net_addr_t::type_t::NETV6));
	// Пустой суффикс сети сетью не является
	ASSERT_FALSE(this->_addr->check("2001:db8::/", awh::net_addr_t::type_t::NETV6));
	// Доменное имя длиннее допустимого доменным именем не является
	ASSERT_FALSE(this->_addr->check(std::string(254, 'a'), awh::net_addr_t::type_t::FQDN));
}

/**
 * @brief Метод проверки вхождения адреса в диапазон, заданный объектами адресов
 *
 */
TEST_F(NetFixture, NetRangeObjectOverloadsTest){
	// Объект начала диапазона адресов
	awh::net_addr_t begin(this->_fmk.get(), this->_log.get());
	// Объект конца диапазона адресов
	awh::net_addr_t end(this->_fmk.get(), this->_log.get());
	// Выполняем разбор начала диапазона адресов
	ASSERT_TRUE(begin.parse("192.168.31.1"));
	// Выполняем разбор конца диапазона адресов
	ASSERT_TRUE(end.parse("192.168.31.100"));
	// Выполняем разбор проверяемого адреса
	ASSERT_TRUE(this->_addr->parse("192.168.31.7"));
	// Проверяем вхождение адреса в диапазон по маске сети без указания разновидности
	ASSERT_TRUE(this->_addr->range(begin, end, "255.255.255.0"));
	// Проверяем вхождение адреса в диапазон по маске сети с указанием разновидности
	ASSERT_TRUE(this->_addr->range(begin, end, "255.255.255.0", awh::net_addr_t::type_t::IPV4));
	// Пустая маска сети диапазона не задаёт
	ASSERT_FALSE(this->_addr->range(begin, end, "", awh::net_addr_t::type_t::IPV4));
	// Негодная маска сети диапазона не задаёт
	ASSERT_FALSE(this->_addr->range(begin, end, "255.0.255.0", awh::net_addr_t::type_t::IPV4));
	// Выполняем разбор адреса за пределами диапазона
	ASSERT_TRUE(this->_addr->parse("192.168.31.200"));
	// Проверяем невхождение адреса в диапазон
	ASSERT_FALSE(this->_addr->range(begin, end, "255.255.255.0"));
}

/**
 * @brief Метод проверки снятия и установки адреса IPv6 числом в обоих порядках байт
 *
 */
TEST_F(NetFixture, NetIPv6EndianRoundTripTest){
	// Выполняем разбор адреса IPv6
	ASSERT_TRUE(this->_addr->parse("2001:db8:85a3::8a2e:370:7334"));
	// Снимаем набор октетов адреса в обратном порядке байт
	const std::array <uint8_t, 16> reversed = this->_addr->v6(awh::net_addr_t::endian_t::BIG);
	// Снимаем набор октетов адреса в сетевом порядке байт
	const std::array <uint8_t, 16> network = this->_addr->v6(awh::net_addr_t::endian_t::LITTLE);
	/**
	 * Проверяем, что порядки байт друг другу обратны
	 */
	for(uint8_t i = 0; i < 16; i++)
		// Проверяем очередной октет обоих наборов
		ASSERT_EQ(network[i], reversed[15 - i]) << "октет: " << static_cast <uint16_t> (i);
	// Сбрасываем объект сетевого адреса
	this->_addr->clear();
	// Выполняем установку адреса набором октетов в обратном порядке байт
	this->_addr->v6(reversed, awh::net_addr_t::endian_t::BIG);
	// Проверяем, что адрес прошёл снятие и установку без изменений
	ASSERT_EQ("2001:DB8:85A3::8A2E:370:7334", this->_addr->print());
	// Проверяем состояние строгого режима разбора
	ASSERT_FALSE(this->_addr->strict());
	// Устанавливаем строгий режим разбора
	this->_addr->strict(true);
	// Проверяем состояние строгого режима разбора
	ASSERT_TRUE(this->_addr->strict());
}

/**
 * @brief Метод проверки разбора записей ARPA негодной длины
 *
 */
TEST_F(NetFixture, NetArpaLengthMismatchTest){
	/**
	 * Набор записей, длина которых обслуживаемому виду не отвечает
	 */
	const std::vector <std::string> samples = {
		// Запись обратной зоны IPv4 с недобором меток
		"0.168.192.in-addr.arpa",
		// Запись обратной зоны IPv4 с перебором меток
		"1.0.168.192.10.in-addr.arpa",
		// Запись обратной зоны IPv6 с недобором меток
		"1.2.3.ip6.arpa",
		// Запись обратной зоны IPv6 с перебором меток
		"0.b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa",
		// Запись обратной зоны IPv6 без разделителей меток
		"ba98765000000000000000008bd0102.ip6.arpa"
	};
	/**
	 * Перебираем все записи набора
	 */
	for(auto & sample : samples){
		// Выполняем разбор адреса IPv4 для заполнения объекта
		ASSERT_TRUE(this->_addr->parse("10.0.0.1"));
		// Разбор записи негодной длины удаться не может
		ASSERT_FALSE(this->_addr->arpa(sample)) << "запись: " << sample;
		// Неудачный разбор обязан оставить объект пустым целиком
		ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type()) << "запись: " << sample;
	}
}

/**
 * @brief Метод проверки присвоения объекта самому себе
 *
 */
TEST_F(NetFixture, NetSelfAssignmentTest){
	// Выполняем разбор адреса с зоной
	ASSERT_TRUE(this->_addr->parse("fe80::1%eth0"));
	// Выполняем присвоение объекта самому себе
	(* this->_addr.get()) = (* this->_addr.get());
	// Проверяем, что адрес присвоением не повреждён
	ASSERT_EQ(awh::net_addr_t::type_t::IPV6, this->_addr->type());
	// Проверяем запись адреса вместе с зоной
	ASSERT_EQ("FE80::1%eth0", this->_addr->print());
	// Проверяем зону адреса
	ASSERT_EQ("eth0", this->_addr->zone());
}

/**
 * @brief Метод проверки отказа разбора негодных записей адресов
 *
 */
TEST_F(NetFixture, NetParseRejectionTest){
	/**
	 * Набор записей, разбор которых удаться не может
	 */
	const std::vector <std::string> samples = {
		// Восьмеричная запись октета с разрядом вне восьмеричной системы
		"0377.0.0.09",
		// Восьмеричная запись октета сверх допустимого значения
		"0400.0.0.1",
		// Шестнадцатеричная запись октета с разрядом вне системы
		"0xZZ.0.0.1",
		// Запись октета сверх разрядности числа
		"99999999999999999999999.0.0.1",
		// Сокращённая запись из трёх частей со старшей частью сверх допустимой
		"256.0.65535",
		// Сокращённая запись из трёх частей с младшей частью сверх допустимой
		"10.0.65536",
		// Десятичная запись октета сверх допустимого значения
		"10.0.0.256",
		// Запись хекстета сверх допустимой разрядности
		"2001:db8::12345",
		// Запись со сжатием, где слов больше допустимого
		"1:2:3:4::5:6:7:8:9"
	};
	/**
	 * Перебираем все записи набора
	 */
	for(auto & sample : samples){
		// Разбор негодной записи удаться не может
		ASSERT_FALSE(this->_addr->parse(sample)) << "запись: " << sample;
		// Неудачный разбор обязан оставить объект пустым целиком
		ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type()) << "запись: " << sample;
	}
	// Установка пустого адреса в чистом виде объект менять не должна
	this->_addr->source(nullptr, awh::net_addr_t::endian_t::LITTLE);
	// Проверяем, что объект остался пустым
	ASSERT_EQ(awh::net_addr_t::type_t::NONE, this->_addr->type());
}

/**
 * @brief Метод сличения приёма записей адреса IPv4 с системным разборщиком
 *
 * @details Разбор ведётся по случайным записям, собранным из разрядов, точек и
 *          примет систем счисления: всё, что принимает модуль, обязан принимать и
 *          системный разборщик, причём с тем же самым адресом
 *
 * @note Обратное неверно намеренно: `inet_aton` в BSD принимает запись адреса
 *       одним числом шире тридцати двух разрядов, обрезая его, - Linux и POSIX
 *       такую запись отвергают, и модуль отвергает её вслед за ними
 *
 */
TEST_F(NetFixture, NetSystemDifferentialAcceptanceTest){
	// Состояние порождателя псевдослучайных чисел, заданное намеренно для повторяемости прогона
	uint64_t state = 0x1234567890ABCDEFULL;
	// Набор символов, из которых собирается запись адреса
	const char alphabet[] = "0123456789...0123456789abcdefx";
	// Запись очередного адреса
	std::string token;
	// Адрес, полученный системным разборщиком
	struct in_addr system;
	/**
	 * Перебираем набор случайных записей адреса
	 */
	for(uint32_t i = 0; i < 200000; i++){
		// Выполняем очистку записи адреса
		token.clear();
		// Продвигаем состояние порождателя
		state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
		// Определяем длину очередной записи адреса
		const uint8_t length = (1 + ((state >> 59) % 15));
		/**
		 * Набираем символы очередной записи адреса
		 */
		for(uint8_t j = 0; j < length; j++){
			// Продвигаем состояние порождателя
			state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
			// Добавляем очередной символ записи адреса
			token.push_back(alphabet[(state >> 33) % (sizeof(alphabet) - 1)]);
		}
		// Если разбор записи модулю не удался, сличать нечего
		if(!this->_addr->parse(token, awh::net_addr_t::type_t::IPV4))
			// Переходим к следующей записи адреса
			continue;
		// Принятую модулем запись обязан принять и системный разборщик
		ASSERT_EQ(1, ::inet_aton(token.c_str(), &system)) << "запись: " << token;
		// Снимаем адрес, полученный разбором модуля
		const uint32_t result = this->_addr->v4();
		// Оба разборщика обязаны дать один и тот же адрес
		ASSERT_EQ(0, ::memcmp(&result, &system.s_addr, sizeof(result))) << "запись: " << token << ", система: " << ::inet_ntoa(system) << ", модуль: " << this->_addr->print();
	}
}

/**
 * @brief Метод сличения приёма записей адреса IPv6 с системным разборщиком
 *
 * @details Разбор ведётся по случайным записям, собранным из разрядов, двоеточий и
 *          точек: всё, что принимает модуль, обязан принимать и системный
 *          разборщик, причём с тем же самым набором октетов
 *
 */
TEST_F(NetFixture, NetSystemDifferentialIPv6AcceptanceTest){
	// Состояние порождателя псевдослучайных чисел, заданное намеренно для повторяемости прогона
	uint64_t state = 0xFEEDFACECAFEBEEFULL;
	// Набор символов, из которых собирается запись адреса
	const char alphabet[] = "0123456789abcdef:::0123456789ABCDEF.";
	// Количество записей, принятых разборщиком модуля
	uint32_t accepted = 0;
	// Запись очередного адреса
	std::string token;
	// Набор октетов, полученный системным разборщиком
	uint8_t system[16];
	/**
	 * Перебираем набор случайных записей адреса
	 */
	for(uint32_t i = 0; i < 300000; i++){
		// Выполняем очистку записи адреса
		token.clear();
		// Продвигаем состояние порождателя
		state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
		// Определяем длину очередной записи адреса
		const uint8_t length = (2 + ((state >> 59) % 28));
		/**
		 * Набираем символы очередной записи адреса
		 */
		for(uint8_t j = 0; j < length; j++){
			// Продвигаем состояние порождателя
			state = ((state * 6364136223846793005ULL) + 1442695040888963407ULL);
			// Добавляем очередной символ записи адреса
			token.push_back(alphabet[(state >> 33) % (sizeof(alphabet) - 1)]);
		}
		// Если разбор записи модулю не удался, сличать нечего
		if(!this->_addr->parse(token, awh::net_addr_t::type_t::IPV6))
			// Переходим к следующей записи адреса
			continue;
		// Считаем принятую разборщиком модуля запись
		accepted++;
		// Принятую модулем запись обязан принять и системный разборщик
		ASSERT_EQ(1, ::inet_pton(AF_INET6, token.c_str(), system)) << "запись: " << token;
		// Снимаем набор октетов, полученный разбором модуля
		const std::array <uint8_t, 16> result = this->_addr->v6();
		// Оба разборщика обязаны дать один и тот же набор октетов
		ASSERT_EQ(0, ::memcmp(&result[0], system, sizeof(system))) << "запись: " << token << ", модуль: " << this->_addr->print();
	}
	// Проверка обязана иметь дело с принятыми записями, иначе она пуста
	ASSERT_GT(accepted, 0u);
}

/**
 * @brief Метод проверки регистра шестнадцатеричной записи адреса
 *
 * @details Запись выводится заглавными разрядами намеренно, вопреки RFC 5952 §4.3:
 *          шестнадцатеричная запись регистронезависима, разбор принимает любой
 *          регистр, а перевод в строчные обходится вызывающему в одну строку.
 *          Проверка эта закрепляет решение, а не описывает недочёт
 *
 */
TEST_F(NetFixture, NetUpperCaseRecordIsDeliberateTest){
	/**
	 * Набор записей одного и того же адреса в разных регистрах
	 */
	const std::vector <std::string> samples = {
		"2001:db8:85a3::8a2e:370:7334",
		"2001:DB8:85A3::8A2E:370:7334",
		"2001:Db8:85a3::8A2e:370:7334"
	};
	/**
	 * Перебираем все записи набора
	 */
	for(auto & sample : samples){
		// Разбор обязан принять запись в любом регистре
		ASSERT_TRUE(this->_addr->parse(sample)) << "запись: " << sample;
		// Запись адреса выводится заглавными разрядами
		ASSERT_EQ("2001:DB8:85A3::8A2E:370:7334", this->_addr->print()) << "запись: " << sample;
	}
	// Разбор обязан принять аппаратный адрес в строчном регистре
	ASSERT_TRUE(this->_addr->parse("00:1b:44:11:3a:b7"));
	// Запись аппаратного адреса выводится заглавными разрядами
	ASSERT_EQ("00:1B:44:11:3A:B7", this->_addr->print());
}
