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
	ASSERT_EQ("2001:1234:ABCD:78::", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	this->_addr->impose("FFFF:FFFF:FFFF:F8::", awh::net_addr_t::addr_t::NETWORK);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("2001:1234:ABCD:78::", static_cast <std::string> (* this->_addr.get()));

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
	ASSERT_EQ("::5600:9877:3322:5541:AABB", static_cast <std::string> (* this->_addr.get()));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс маску FFFF:FFFF:FFFF:F800::
	this->_addr->impose("FFFF:FFFF:FFFF:F800::", awh::net_addr_t::addr_t::HOST);
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("::5600:9877:3322:5541:AABB", static_cast <std::string> (* this->_addr.get()));

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
	ASSERT_EQ("FFFF:FFFF:FFFF:F8::", this->_addr->prefix2Mask(53));
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
	ASSERT_TRUE(this->_addr->mapping("::5600:9877:3322:5541:AABB", 53, awh::net_addr_t::addr_t::HOST));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "192.168.3.192";
	// Проверяем что адрес маппится в хост с маской 255.128.0.0
	ASSERT_TRUE(this->_addr->mapping("0.40.3.192", "255.128.0.0", awh::net_addr_t::addr_t::HOST));

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Проверяем что адрес маппится в хост с маской FFFF:FFFF:FFFF:F800::
	ASSERT_TRUE(this->_addr->mapping("::5600:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", awh::net_addr_t::addr_t::HOST));

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

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "::1";
	// Проверяем что тип адреса соответствует локальному
	ASSERT_EQ(awh::net_addr_t::own_t::LAN, this->_addr->own());

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

	// Выполняем тесты IP-адресов
	(* this->_addr.get()) = "2001:db8:0:0:0:8:800:200C:417A";
	// Выполняем проверку формата вывода адреса
	ASSERT_EQ("0.0.0.0", static_cast <std::string> (* this->_addr.get()));

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
