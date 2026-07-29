/**
 * @file: parameterized.cpp
 * @date: 2026-07-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты протокола SOCKS5 — прогон подготовленных наборов входных данных через методы модуля
 *        с проверкой обмена сообщениями приветствия и авторизации, разбора команд и формирования ответов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <climits>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "socks5.hpp"

/**
 * Подписываемся на пространство имён протоколов
 */
using namespace awh::proto;

/**
 * Псевдонимы вложенных типов протокола SOCKS5
 */
using state_t   = socks5_t::state_t;
using status_t  = socks5_t::status_t;
using command_t = socks5_t::command_t;

/**
 * @brief Структура параметров теста запроса CONNECT для разных типов адресов
 *
 */
struct ConnectAddressParameter {
	// Тип адреса хоста для подключения
	awh::net::type_t type;
	// Ожидаемый размер кадра запроса CONNECT
	size_t frameSize;
	// Ожидаемый код типа адреса в кадре
	uint8_t addressType;
};

/**
 * @brief Класс фикстуры теста запроса CONNECT для разных типов адресов
 *
 */
class ConnectAddressParameterizedFixture : public Socks5Fixture, public ::testing::WithParamInterface <ConnectAddressParameter> {
	public:
		// Параметры теста запроса CONNECT
		ConnectAddressParameter _parameter = GetParam();
	protected:
		/**
		 * @brief Метод создания адреса хоста согласно параметрам теста
		 *
		 * @return сформированный объект адреса хоста
		 *
		 */
		std::unique_ptr <awh::net::attr_t> makeHost() const noexcept {
			/**
			 * Определяем тип адреса хоста
			 */
			switch(static_cast <uint8_t> (this->_parameter.type)){
				// Если тип адреса соответствует FQDN
				case static_cast <uint8_t> (awh::net::type_t::FQDN): {
					// Создаём объект FQDN-адреса хоста
					std::unique_ptr <awh::net::attr_fqdn_t> host = std::make_unique <awh::net::attr_fqdn_t> ();
					// Устанавливаем доменное имя хоста
					host->domain = "anyks.com";
					// Устанавливаем порт хоста
					host->port = 443;
					// Возвращаем сформированный объект адреса хоста
					return host;
				}
				// Если тип адреса соответствует IPv4
				case static_cast <uint8_t> (awh::net::type_t::IPV4): {
					// Создаём объект IP-адреса хоста
					std::unique_ptr <awh::net::attr_net_t> host = std::make_unique <awh::net::attr_net_t> ();
					// Устанавливаем тип адреса IPv4
					host->type = awh::net::type_t::IPV4;
					// Устанавливаем порт хоста
					host->port = 8080;
					// Создаём объект IPv4-адреса хоста
					host->ip = std::make_unique <awh::net::addr_net_ipv4_t> ();
					// Устанавливаем IPv4-адрес хоста
					static_cast <awh::net::addr_net_ipv4_t *> (host->ip.get())->address = 0x0100007F;
					// Возвращаем сформированный объект адреса хоста
					return host;
				}
				// Если тип адреса соответствует IPv6
				case static_cast <uint8_t> (awh::net::type_t::IPV6): {
					// Создаём объект IP-адреса хоста
					std::unique_ptr <awh::net::attr_net_t> host = std::make_unique <awh::net::attr_net_t> ();
					// Устанавливаем тип адреса IPv6
					host->type = awh::net::type_t::IPV6;
					// Устанавливаем порт хоста
					host->port = 9090;
					// Создаём объект IPv6-адреса хоста
					host->ip = std::make_unique <awh::net::addr_net_ipv6_t> ();
					// Устанавливаем IPv6-адрес хоста (::1)
					static_cast <awh::net::addr_net_ipv6_t *> (host->ip.get())->address.back() = 0x01;
					// Возвращаем сформированный объект адреса хоста
					return host;
				}
			}
			// Возвращаем пустой адрес хоста
			return nullptr;
		}
};

/**
 * @brief Метод тестирования полного цикла запроса CONNECT для разных типов адресов
 *
 */
TEST_P(ConnectAddressParameterizedFixture, ConnectAddressRoundTripTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Устанавливаем состояние отправки запроса подключения на клиенте
	cctx.state = state_t::CONNECT;
	// Устанавливаем команду подключения
	cctx.command = command_t::CONNECT;
	// Устанавливаем хост конечного сервера согласно параметрам теста
	cctx.host = this->makeHost();
	// Устанавливаем состояние ожидания запроса подключения на сервере
	sctx.state = state_t::CONNECT;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем запрос CONNECT
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем ожидаемый размер кадра запроса
	ASSERT_EQ(size, this->_parameter.frameSize);
	// Проверяем ожидаемый код типа адреса в кадре
	ASSERT_EQ(data[3], this->_parameter.addressType);
	// Проверяем что определение размера кадра соответствует фактическому размеру
	ASSERT_EQ(socks5_t::frameSize(state_t::CONNECT, data, size), size);
	// Выполняем разбор запроса CONNECT на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что сервер разрешил подключение
	ASSERT_EQ(sctx.status, status_t::SUCCESS);
	// Проверяем что сервер получил адрес конечного сервера
	ASSERT_TRUE(sctx.host != nullptr);
	// Проверяем что тип полученного адреса соответствует отправленному
	ASSERT_EQ(sctx.host->type, this->_parameter.type);
	/**
	 * Определяем тип адреса хоста для сверки полученных данных
	 */
	switch(static_cast <uint8_t> (this->_parameter.type)){
		// Если тип адреса соответствует FQDN
		case static_cast <uint8_t> (awh::net::type_t::FQDN): {
			// Проверяем доменное имя полученного адреса
			ASSERT_EQ(static_cast <awh::net::attr_fqdn_t *> (sctx.host.get())->domain, "anyks.com");
			// Проверяем порт полученного адреса
			ASSERT_EQ(static_cast <awh::net::attr_fqdn_t *> (sctx.host.get())->port, 443);
		} break;
		// Если тип адреса соответствует IPv4
		case static_cast <uint8_t> (awh::net::type_t::IPV4): {
			// Проверяем IPv4-адрес полученного адреса
			ASSERT_EQ(static_cast <awh::net::addr_net_ipv4_t *> (static_cast <awh::net::attr_net_t *> (sctx.host.get())->ip.get())->address, 0x0100007Fu);
			// Проверяем порт полученного адреса
			ASSERT_EQ(static_cast <awh::net::attr_net_t *> (sctx.host.get())->port, 8080);
		} break;
		// Если тип адреса соответствует IPv6
		case static_cast <uint8_t> (awh::net::type_t::IPV6): {
			// Проверяем последний октет IPv6-адреса полученного адреса
			ASSERT_EQ(static_cast <awh::net::addr_net_ipv6_t *> (static_cast <awh::net::attr_net_t *> (sctx.host.get())->ip.get())->address.back(), 0x01);
			// Проверяем порт полученного адреса
			ASSERT_EQ(static_cast <awh::net::attr_net_t *> (sctx.host.get())->port, 9090);
		} break;
	}
	// Формируем ответ сервера на запрос CONNECT
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Выполняем разбор ответа на запрос CONNECT на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что рукопожатие на клиенте выполнено
	ASSERT_EQ(cctx.state, state_t::HANDSHAKE);
	// Проверяем что клиент получил адрес установленного соединения
	ASSERT_TRUE(cctx.host != nullptr);
	// Проверяем что тип полученного адреса соответствует отправленному
	ASSERT_EQ(cctx.host->type, this->_parameter.type);
}

/**
 * @brief Инициализация параметров теста запроса CONNECT для разных типов адресов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ConnectAddressParameterizedFixture,
	::testing::Values(
		// FQDN-адрес: 4 (заголовок) + 1 (длина) + 9 (домен) + 2 (порт)
		ConnectAddressParameter({awh::net::type_t::FQDN, 16, 0x03}),
		// IPv4-адрес: 4 (заголовок) + 4 (адрес) + 2 (порт)
		ConnectAddressParameter({awh::net::type_t::IPV4, 10, 0x01}),
		// IPv6-адрес: 4 (заголовок) + 16 (адрес) + 2 (порт)
		ConnectAddressParameter({awh::net::type_t::IPV6, 22, 0x04})
	)
);

/**
 * @brief Структура параметров теста определения полного размера SOCKS5-кадра
 *
 */
struct FrameSizeParameter {
	// Текущее состояние протокола
	state_t state;
	// Буфер входящих данных
	std::vector <uint8_t> frame;
	// Ожидаемый результат определения размера кадра
	size_t expected;
};

/**
 * @brief Класс фикстуры теста определения полного размера SOCKS5-кадра
 *
 */
class FrameSizeParameterizedFixture : public Socks5Fixture, public ::testing::WithParamInterface <FrameSizeParameter> {
	public:
		// Параметры теста определения полного размера SOCKS5-кадра
		FrameSizeParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования определения полного размера SOCKS5-кадра
 *
 */
TEST_P(FrameSizeParameterizedFixture, FrameSizeTest){
	// Проверяем что определение размера кадра соответствует ожидаемому результату
	ASSERT_EQ(socks5_t::frameSize(this->_parameter.state, this->_parameter.frame.data(), this->_parameter.frame.size()), this->_parameter.expected);
}

/**
 * @brief Инициализация параметров теста определения полного размера SOCKS5-кадра
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FrameSizeParameterizedFixture,
	::testing::Values(
		// Пустой буфер данных — кадр неполный
		FrameSizeParameter({state_t::NONE, {}, 0}),
		// Приветствие клиента без списка методов — кадр неполный
		FrameSizeParameter({state_t::NONE, {0x05}, 0}),
		// Приветствие клиента с неполным списком методов — кадр неполный
		FrameSizeParameter({state_t::NONE, {0x05, 0x02, 0x00}, 0}),
		// Полное приветствие клиента с двумя методами — размер кадра 4
		FrameSizeParameter({state_t::NONE, {0x05, 0x02, 0x00, 0x02}, 4}),
		// Полное приветствие клиента с одним методом — размер кадра 3
		FrameSizeParameter({state_t::NONE, {0x05, 0x01, 0x00}, 3}),
		// Пакет авторизации с неверной версией соглашения — кадр некорректный
		FrameSizeParameter({state_t::AUTH, {0x05, 0x01}, SIZE_MAX}),
		// Пакет авторизации без пароля — кадр неполный
		FrameSizeParameter({state_t::AUTH, {0x01, 0x02, 'a', 'b'}, 0}),
		// Полный пакет авторизации — размер кадра 7 (1 + 1 + 2 + 1 + 2)
		FrameSizeParameter({state_t::AUTH, {0x01, 0x02, 'a', 'b', 0x02, 'c', 'd'}, 7}),
		// Запрос CONNECT с неполным заголовком — кадр неполный
		FrameSizeParameter({state_t::CONNECT, {0x05, 0x01, 0x00}, 0}),
		// Запрос CONNECT с неподдерживаемым типом адреса — кадр некорректный
		FrameSizeParameter({state_t::CONNECT, {0x05, 0x01, 0x00, 0x02, 0x00, 0x00}, SIZE_MAX}),
		// Полный запрос CONNECT с IPv4-адресом — размер кадра 10
		FrameSizeParameter({state_t::CONNECT, {0x05, 0x01, 0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x01, 0xBB}, 10}),
		// Неполный запрос CONNECT с IPv6-адресом — кадр неполный
		FrameSizeParameter({state_t::CONNECT, {0x05, 0x01, 0x00, 0x04, 0x00, 0x00}, 0}),
		// Неполный ответ выбора метода — кадр неполный
		FrameSizeParameter({state_t::REQUEST, {0x05}, 0}),
		// Полный ответ выбора метода — размер кадра 2
		FrameSizeParameter({state_t::REQUEST, {0x05, 0x00}, 2}),
		// Неполный ответ авторизации — кадр неполный
		FrameSizeParameter({state_t::RESPONSE, {0x01}, 0}),
		// Полный ответ авторизации — размер кадра 2
		FrameSizeParameter({state_t::RESPONSE, {0x01, 0x00}, 2}),
		// Неполный ответ CONNECT с FQDN-адресом без длины домена — кадр неполный
		FrameSizeParameter({state_t::SUCCESS, {0x05, 0x00, 0x00, 0x03}, 0}),
		// Полный ответ CONNECT с IPv4-адресом — размер кадра 10
		FrameSizeParameter({state_t::SUCCESS, {0x05, 0x00, 0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0xCF, 0xDB}, 10})
	)
);

/**
 * @brief Структура параметров теста текстовых сообщений кодов статусов
 *
 */
struct StatusMessageParameter {
	// Код статуса ответа сервера
	status_t status;
	// Ожидаемое текстовое сообщение кода статуса
	std::string message;
};

/**
 * @brief Класс фикстуры теста текстовых сообщений кодов статусов
 *
 */
class StatusMessageParameterizedFixture : public Socks5Fixture, public ::testing::WithParamInterface <StatusMessageParameter> {
	public:
		// Параметры теста текстовых сообщений кодов статусов
		StatusMessageParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования текстовых сообщений кодов статусов
 *
 */
TEST_P(StatusMessageParameterizedFixture, StatusMessageTest){
	// Проверяем что текстовое сообщение кода статуса соответствует ожидаемому
	ASSERT_EQ(socks5_t::statusMessage(this->_parameter.status), this->_parameter.message);
}

/**
 * @brief Инициализация параметров теста текстовых сообщений кодов статусов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, StatusMessageParameterizedFixture,
	::testing::Values(
		StatusMessageParameter({status_t::SUCCESS, "Successful completion"}),
		StatusMessageParameter({status_t::SOCKSERR, "SOCKS server error"}),
		StatusMessageParameter({status_t::FORBIDDEN, "Connection forbidden by ruleset"}),
		StatusMessageParameter({status_t::UNAVNET, "Network unreachable"}),
		StatusMessageParameter({status_t::UNAVHOST, "Host unreachable"}),
		StatusMessageParameter({status_t::DENIED, "Connection denied"}),
		StatusMessageParameter({status_t::TIMETTL, "Connection timed out"}),
		StatusMessageParameter({status_t::NOCOMMAND, "Command not supported"}),
		StatusMessageParameter({status_t::NOADDR, "Address type not supported"}),
		StatusMessageParameter({status_t::NOSUPPORT, "General SOCKS server failure"}),
		StatusMessageParameter({status_t::NOSTATUS, "Unknown status"})
	)
);

/**
 * @brief Структура параметров теста инкапсуляции UDP-датаграмм для разных типов адресов
 *
 */
struct UDPAddressParameter {
	// Тип адреса конечного получателя
	awh::net::type_t type;
	// Ожидаемый размер UDP заголовка
	size_t headerSize;
};

/**
 * @brief Класс фикстуры теста инкапсуляции UDP-датаграмм для разных типов адресов
 *
 */
class UDPAddressParameterizedFixture : public Socks5Fixture, public ::testing::WithParamInterface <UDPAddressParameter> {
	public:
		// Параметры теста инкапсуляции UDP-датаграмм
		UDPAddressParameter _parameter = GetParam();
	protected:
		/**
		 * @brief Метод создания адреса конечного получателя согласно параметрам теста
		 *
		 * @return сформированный объект адреса конечного получателя
		 *
		 */
		std::unique_ptr <awh::net::attr_t> makeHost() const noexcept {
			/**
			 * Определяем тип адреса конечного получателя
			 */
			switch(static_cast <uint8_t> (this->_parameter.type)){
				// Если тип адреса соответствует FQDN
				case static_cast <uint8_t> (awh::net::type_t::FQDN): {
					// Создаём объект FQDN-адреса конечного получателя
					std::unique_ptr <awh::net::attr_fqdn_t> host = std::make_unique <awh::net::attr_fqdn_t> ();
					// Устанавливаем доменное имя конечного получателя
					host->domain = "anyks.com";
					// Устанавливаем порт конечного получателя
					host->port = 53;
					// Возвращаем сформированный объект адреса конечного получателя
					return host;
				}
				// Если тип адреса соответствует IPv4
				case static_cast <uint8_t> (awh::net::type_t::IPV4): {
					// Создаём объект IP-адреса конечного получателя
					std::unique_ptr <awh::net::attr_net_t> host = std::make_unique <awh::net::attr_net_t> ();
					// Устанавливаем тип адреса IPv4
					host->type = awh::net::type_t::IPV4;
					// Устанавливаем порт конечного получателя
					host->port = 53;
					// Создаём объект IPv4-адреса конечного получателя
					host->ip = std::make_unique <awh::net::addr_net_ipv4_t> ();
					// Устанавливаем IPv4-адрес конечного получателя
					static_cast <awh::net::addr_net_ipv4_t *> (host->ip.get())->address = 0x08080808;
					// Возвращаем сформированный объект адреса конечного получателя
					return host;
				}
				// Если тип адреса соответствует IPv6
				case static_cast <uint8_t> (awh::net::type_t::IPV6): {
					// Создаём объект IP-адреса конечного получателя
					std::unique_ptr <awh::net::attr_net_t> host = std::make_unique <awh::net::attr_net_t> ();
					// Устанавливаем тип адреса IPv6
					host->type = awh::net::type_t::IPV6;
					// Устанавливаем порт конечного получателя
					host->port = 53;
					// Создаём объект IPv6-адреса конечного получателя
					host->ip = std::make_unique <awh::net::addr_net_ipv6_t> ();
					// Устанавливаем IPv6-адрес конечного получателя (::1)
					static_cast <awh::net::addr_net_ipv6_t *> (host->ip.get())->address.back() = 0x01;
					// Возвращаем сформированный объект адреса конечного получателя
					return host;
				}
			}
			// Возвращаем пустой адрес конечного получателя
			return nullptr;
		}
};

/**
 * @brief Метод тестирования инкапсуляции UDP-датаграмм для разных типов адресов
 *
 */
TEST_P(UDPAddressParameterizedFixture, UDPAddressRoundTripTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём объект UDP заголовка исходящей датаграммы
	socks5_t::udp_head_t udpOut;
	// Устанавливаем хост конечного получателя согласно параметрам теста
	udpOut.host = this->makeHost();
	// Буфер и размер сформированного UDP заголовка
	uint8_t * header = nullptr;
	size_t size      = 0;
	// Формируем UDP заголовок датаграммы
	ASSERT_TRUE(client->buffer(&header, size, udpOut));
	// Проверяем ожидаемый размер UDP заголовка
	ASSERT_EQ(size, this->_parameter.headerSize);
	// Собираем полную датаграмму из UDP заголовка и полезной нагрузки
	std::vector <uint8_t> datagram(header, header + size);
	// Формируем полезную нагрузку датаграммы
	const std::string payload = "PAYLOAD";
	// Добавляем полезную нагрузку в датаграмму
	datagram.insert(datagram.end(), payload.begin(), payload.end());
	// Создаём объект UDP заголовка входящей датаграммы
	socks5_t::udp_head_t udpIn;
	// Выполняем разбор UDP заголовка датаграммы на сервере
	ASSERT_TRUE(server->parse(datagram.data(), datagram.size(), udpIn));
	// Проверяем размер разобранного UDP заголовка
	ASSERT_EQ(udpIn.size, this->_parameter.headerSize);
	// Проверяем что адрес конечного получателя получен
	ASSERT_TRUE(udpIn.host != nullptr);
	// Проверяем что тип полученного адреса соответствует отправленному
	ASSERT_EQ(udpIn.host->type, this->_parameter.type);
	// Проверяем что полезная нагрузка следует сразу за заголовком
	ASSERT_EQ(std::string(datagram.begin() + udpIn.size, datagram.end()), payload);
}

/**
 * @brief Инициализация параметров теста инкапсуляции UDP-датаграмм
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, UDPAddressParameterizedFixture,
	::testing::Values(
		// FQDN-адрес: 4 (заголовок) + 1 (длина) + 9 (домен) + 2 (порт)
		UDPAddressParameter({awh::net::type_t::FQDN, 16}),
		// IPv4-адрес: 4 (заголовок) + 4 (адрес) + 2 (порт)
		UDPAddressParameter({awh::net::type_t::IPV4, 10}),
		// IPv6-адрес: 4 (заголовок) + 16 (адрес) + 2 (порт)
		UDPAddressParameter({awh::net::type_t::IPV6, 22})
	)
);
