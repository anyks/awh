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
