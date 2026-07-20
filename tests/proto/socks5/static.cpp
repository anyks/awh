/**
 * @file: static.cpp
 * @date: 2026-07-20
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
#include <memory>
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
 * @brief Функция создания FQDN-адреса хоста
 *
 * @param domain доменное имя хоста
 * @param port   порт хоста
 * @return       сформированный объект адреса хоста
 */
static std::unique_ptr <awh::net::attr_t> makeFQDN(const std::string & domain, const uint16_t port) noexcept {
	// Создаём объект FQDN-адреса хоста
	std::unique_ptr <awh::net::attr_fqdn_t> host = std::make_unique <awh::net::attr_fqdn_t> ();
	// Устанавливаем доменное имя хоста
	host->domain = domain;
	// Устанавливаем порт хоста
	host->port = port;
	// Возвращаем сформированный объект адреса хоста
	return host;
}

/**
 * @brief Метод проверки полного рукопожатия без аутентификации
 *
 */
TEST_F(Socks5Fixture, HandshakeNoAuthTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем приветствие клиента
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем что клиент перешёл в состояние ожидания выбора метода
	ASSERT_EQ(cctx.state, state_t::REQUEST);
	// Проверяем размер приветствия без аутентификации
	ASSERT_EQ(size, 3u);
	// Проверяем версию протокола в приветствии
	ASSERT_EQ(data[0], 0x05);
	// Проверяем количество методов аутентификации
	ASSERT_EQ(data[1], 0x01);
	// Проверяем что предложен метод "без аутентификации"
	ASSERT_EQ(data[2], 0x00);
	// Выполняем разбор приветствия клиента на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что сервер перешёл в состояние отправки выбранного метода
	ASSERT_EQ(sctx.state, state_t::AUTH);
	// Проверяем что сервер разрешил подключение без аутентификации
	ASSERT_EQ(sctx.status, status_t::SUCCESS);
	// Формируем ответ сервера с выбранным методом
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Проверяем размер ответа выбора метода
	ASSERT_EQ(size, 2u);
	// Проверяем что сервер выбрал метод "без аутентификации"
	ASSERT_EQ(data[1], 0x00);
	// Проверяем что сервер перешёл в состояние ожидания запроса подключения
	ASSERT_EQ(sctx.state, state_t::CONNECT);
	// Выполняем разбор выбранного метода на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что клиент перешёл в состояние отправки запроса подключения
	ASSERT_EQ(cctx.state, state_t::CONNECT);
	// Проверяем что установлена команда подключения
	ASSERT_EQ(cctx.command, command_t::CONNECT);
	// Устанавливаем хост конечного сервера для подключения
	cctx.host = ::makeFQDN("anyks.com", 443);
	// Формируем запрос CONNECT
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем что клиент перешёл в состояние ожидания разрешения на подключение
	ASSERT_EQ(cctx.state, state_t::SUCCESS);
	// Проверяем размер запроса CONNECT с FQDN-адресом (4 + 1 + 9 + 2)
	ASSERT_EQ(size, 16u);
	// Проверяем код команды подключения в запросе
	ASSERT_EQ(data[1], 0x01);
	// Проверяем тип адреса FQDN в запросе
	ASSERT_EQ(data[3], 0x03);
	// Проверяем размер доменного имени в запросе
	ASSERT_EQ(data[4], 0x09);
	// Выполняем разбор запроса CONNECT на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что сервер перешёл в состояние рукопожатия
	ASSERT_EQ(sctx.state, state_t::HANDSHAKE);
	// Проверяем что сервер разрешил подключение
	ASSERT_EQ(sctx.status, status_t::SUCCESS);
	// Проверяем что сервер получил запрошенную команду подключения
	ASSERT_EQ(sctx.command, command_t::CONNECT);
	// Проверяем что сервер получил адрес конечного сервера
	ASSERT_TRUE(sctx.host != nullptr);
	// Проверяем тип полученного адреса
	ASSERT_EQ(sctx.host->type, awh::net::type_t::FQDN);
	// Проверяем доменное имя полученного адреса
	ASSERT_EQ(static_cast <awh::net::attr_fqdn_t *> (sctx.host.get())->domain, "anyks.com");
	// Проверяем порт полученного адреса
	ASSERT_EQ(static_cast <awh::net::attr_fqdn_t *> (sctx.host.get())->port, 443);
	// Формируем ответ сервера на запрос CONNECT
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Выполняем разбор ответа на запрос CONNECT на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что рукопожатие на клиенте выполнено
	ASSERT_EQ(cctx.state, state_t::HANDSHAKE);
	// Проверяем что клиент получил адрес установленного соединения
	ASSERT_TRUE(cctx.host != nullptr);
}

/**
 * @brief Метод проверки успешной аутентификации USER/PASS (RFC 1929)
 *
 */
TEST_F(Socks5Fixture, AuthSuccessTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Устанавливаем параметры авторизации клиента
	client->setUser("forman", "12345");
	// Устанавливаем функцию проверки авторизации на сервере
	server->on([](const std::string & username, const std::string & password) noexcept -> bool {
		// Подтверждаем корректность только для известной пары
		return ((username == "forman") && (password == "12345"));
	});
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем приветствие клиента
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем что предложено два метода аутентификации
	ASSERT_EQ(size, 4u);
	// Проверяем количество методов аутентификации
	ASSERT_EQ(data[1], 0x02);
	// Выполняем разбор приветствия клиента на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что сервер требует аутентификацию по паролю
	ASSERT_EQ(sctx.status, status_t::FORBIDDEN);
	// Формируем ответ сервера с требованием аутентификации
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Проверяем что сервер выбрал метод аутентификации по паролю
	ASSERT_EQ(data[1], 0x02);
	// Выполняем разбор выбранного метода на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что клиент перешёл в состояние отправки пакета авторизации
	ASSERT_EQ(cctx.state, state_t::AUTH);
	// Формируем пакет авторизации клиента
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем что клиент перешёл в состояние ожидания ответа авторизации
	ASSERT_EQ(cctx.state, state_t::RESPONSE);
	// Проверяем размер пакета авторизации (1 + 1 + 6 + 1 + 5)
	ASSERT_EQ(size, 14u);
	// Проверяем версию соглашения авторизации
	ASSERT_EQ(data[0], 0x01);
	// Проверяем длину логина пользователя
	ASSERT_EQ(data[1], 0x06);
	// Выполняем разбор пакета авторизации на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что авторизация на сервере прошла успешно
	ASSERT_EQ(sctx.status, status_t::SUCCESS);
	// Формируем ответ сервера с результатом авторизации
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Проверяем что сервер перешёл в состояние ожидания запроса подключения
	ASSERT_EQ(sctx.state, state_t::CONNECT);
	// Выполняем разбор ответа авторизации на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что клиент перешёл в состояние отправки запроса подключения
	ASSERT_EQ(cctx.state, state_t::CONNECT);
}

/**
 * @brief Метод проверки отказа в авторизации при неверном пароле
 *
 */
TEST_F(Socks5Fixture, AuthWrongPasswordTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Устанавливаем неверные параметры авторизации клиента
	client->setUser("forman", "wrong-password");
	// Устанавливаем функцию проверки авторизации на сервере
	server->on([](const std::string & username, const std::string & password) noexcept -> bool {
		// Подтверждаем корректность только для известной пары
		return ((username == "forman") && (password == "12345"));
	});
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем приветствие клиента
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Выполняем разбор приветствия клиента на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Формируем ответ сервера с требованием аутентификации
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Выполняем разбор выбранного метода на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Формируем пакет авторизации клиента
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Выполняем разбор пакета авторизации на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что авторизация на сервере отклонена
	ASSERT_EQ(sctx.status, status_t::FORBIDDEN);
	// Формируем ответ сервера с отказом в авторизации
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Выполняем разбор ответа авторизации на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что клиент перешёл в состояние ошибки
	ASSERT_EQ(cctx.state, state_t::BROKEN);
	// Проверяем код ошибки полученного ответа
	ASSERT_EQ(cctx.status, status_t::FORBIDDEN);
}

/**
 * @brief Метод проверки отказа сервера при отсутствии применимых методов аутентификации
 *
 */
TEST_F(Socks5Fixture, NoAcceptableMethodTest){
	// Создаём объект клиента SOCKS5 без учётных данных
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5 с обязательной аутентификацией
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Устанавливаем функцию проверки авторизации на сервере
	server->on([](const std::string &, const std::string &) noexcept -> bool {
		// Подтверждаем любую пару учётных данных
		return true;
	});
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем приветствие клиента (только метод "без аутентификации")
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Выполняем разбор приветствия клиента на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что сервер не нашёл применимых методов
	ASSERT_EQ(sctx.status, status_t::DENIED);
	// Формируем ответ сервера с отказом в подключении
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Проверяем что сервер выбрал метод "нет применимых методов"
	ASSERT_EQ(data[1], 0xFF);
	// Выполняем разбор ответа сервера на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что клиент перешёл в состояние ошибки
	ASSERT_EQ(cctx.state, state_t::BROKEN);
	// Проверяем код ошибки полученного ответа
	ASSERT_EQ(cctx.status, status_t::NOSUPPORT);
}

/**
 * @brief Метод проверки отказа клиента при требовании аутентификации без учётных данных
 *
 */
TEST_F(Socks5Fixture, ClientMissingCredentialsTest){
	// Создаём объект клиента SOCKS5 без учётных данных
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Устанавливаем состояние ожидания выбора метода
	cctx.state = state_t::REQUEST;
	// Формируем ответ сервера с требованием аутентификации по паролю
	const uint8_t frame[] = {0x05, 0x02};
	// Выполняем разбор ответа сервера на клиенте
	ASSERT_TRUE(client->parse(frame, sizeof(frame), cctx));
	// Проверяем что клиент без учётных данных перешёл в состояние ошибки
	ASSERT_EQ(cctx.state, state_t::BROKEN);
	// Проверяем код ошибки полученного ответа
	ASSERT_EQ(cctx.status, status_t::FORBIDDEN);
}

/**
 * @brief Метод проверки отклонения приветствия с неверной версией протокола
 *
 */
TEST_F(Socks5Fixture, GreetingWrongVersionTest){
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём контекст обмена данными сервера
	socks5_t::ctx_t sctx;
	// Формируем приветствие с версией протокола SOCKS4
	const uint8_t frame[] = {0x04, 0x01, 0x00};
	// Выполняем разбор приветствия на сервере
	ASSERT_FALSE(server->parse(frame, sizeof(frame), sctx));
	// Проверяем что сервер перешёл в состояние ошибки
	ASSERT_EQ(sctx.state, state_t::BROKEN);
	// Проверяем код ошибки протокола
	ASSERT_EQ(sctx.status, status_t::SOCKSERR);
}

/**
 * @brief Метод проверки отклонения ответа с неверной версией протокола
 *
 */
TEST_F(Socks5Fixture, ResponseWrongVersionTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Устанавливаем состояние ожидания выбора метода
	cctx.state = state_t::REQUEST;
	// Формируем ответ с версией протокола SOCKS4
	const uint8_t frame[] = {0x04, 0x00};
	// Выполняем разбор ответа на клиенте
	ASSERT_TRUE(client->parse(frame, sizeof(frame), cctx));
	// Проверяем что клиент перешёл в состояние ошибки
	ASSERT_EQ(cctx.state, state_t::BROKEN);
	// Проверяем код ошибки полученного ответа
	ASSERT_EQ(cctx.status, status_t::FORBIDDEN);
}

/**
 * @brief Метод проверки отклонения пустого буфера данных
 *
 */
TEST_F(Socks5Fixture, EmptyBufferTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Устанавливаем состояние ожидания выбора метода
	cctx.state = state_t::REQUEST;
	// Проверяем что разбор пустого буфера на клиенте неуспешен
	ASSERT_FALSE(client->parse(nullptr, 0, cctx));
	// Проверяем что разбор пустого буфера на сервере неуспешен
	ASSERT_FALSE(server->parse(nullptr, 0, sctx));
	// Формируем непустой буфер данных
	const uint8_t frame[] = {0x05};
	// Проверяем что разбор буфера нулевого размера на клиенте неуспешен
	ASSERT_FALSE(client->parse(frame, 0, cctx));
	// Проверяем что разбор буфера нулевого размера на сервере неуспешен
	ASSERT_FALSE(server->parse(frame, 0, sctx));
}

/**
 * @brief Метод проверки отклонения неподдерживаемой команды BIND
 *
 */
TEST_F(Socks5Fixture, BindNotSupportedTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём контексты обмена данными клиента и сервера
	socks5_t::ctx_t cctx, sctx;
	// Устанавливаем состояние отправки запроса подключения на клиенте
	cctx.state = state_t::CONNECT;
	// Устанавливаем команду обратного подключения
	cctx.command = command_t::BIND;
	// Устанавливаем хост конечного сервера для подключения
	cctx.host = ::makeFQDN("anyks.com", 443);
	// Устанавливаем состояние ожидания запроса подключения на сервере
	sctx.state = state_t::CONNECT;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем запрос BIND
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем код команды обратного подключения в запросе
	ASSERT_EQ(data[1], 0x02);
	// Выполняем разбор запроса BIND на сервере
	ASSERT_TRUE(server->parse(data, size, sctx));
	// Проверяем что сервер отклонил неподдерживаемую команду
	ASSERT_EQ(sctx.status, status_t::NOCOMMAND);
	// Формируем ответ сервера с отказом
	ASSERT_TRUE(server->buffer(&data, size, sctx));
	// Выполняем разбор ответа на клиенте
	ASSERT_TRUE(client->parse(data, size, cctx));
	// Проверяем что клиент перешёл в состояние ошибки
	ASSERT_EQ(cctx.state, state_t::BROKEN);
	// Проверяем код ошибки полученного ответа
	ASSERT_EQ(cctx.status, status_t::NOCOMMAND);
}

/**
 * @brief Метод проверки усечения слишком длинных учётных данных
 *
 */
TEST_F(Socks5Fixture, UserCredentialsTruncationTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Устанавливаем учётные данные превышающие максимальную длину (255 байт)
	client->setUser(std::string(300, 'u'), std::string(300, 'p'));
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Устанавливаем состояние отправки пакета авторизации
	cctx.state = state_t::AUTH;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Формируем пакет авторизации клиента
	ASSERT_TRUE(client->buffer(&data, size, cctx));
	// Проверяем что длина логина усечена до максимальной
	ASSERT_EQ(data[1], 0xFF);
	// Проверяем что длина пароля усечена до максимальной
	ASSERT_EQ(data[2 + 0xFF], 0xFF);
	// Проверяем полный размер пакета авторизации (1 + 1 + 255 + 1 + 255)
	ASSERT_EQ(size, 513u);
}

/**
 * @brief Метод проверки инкапсуляции UDP-датаграммы (UDP ASSOCIATE)
 *
 */
TEST_F(Socks5Fixture, UDPRoundTripTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Создаём объект UDP заголовка исходящей датаграммы
	socks5_t::udp_head_t udpOut;
	// Устанавливаем хост конечного получателя датаграммы
	udpOut.host = ::makeFQDN("anyks.com", 53);
	// Буфер и размер сформированного UDP заголовка
	uint8_t * header = nullptr;
	size_t size      = 0;
	// Формируем UDP заголовок датаграммы
	ASSERT_TRUE(client->buffer(&header, size, udpOut));
	// Проверяем размер UDP заголовка с FQDN-адресом (2 + 1 + 1 + 1 + 9 + 2)
	ASSERT_EQ(size, 16u);
	// Собираем полную датаграмму из UDP заголовка и полезной нагрузки
	std::vector <uint8_t> datagram(header, header + size);
	// Формируем полезную нагрузку датаграммы
	const std::string payload = "DNS-QUERY";
	// Добавляем полезную нагрузку в датаграмму
	datagram.insert(datagram.end(), payload.begin(), payload.end());
	// Создаём объект UDP заголовка входящей датаграммы
	socks5_t::udp_head_t udpIn;
	// Выполняем разбор UDP заголовка датаграммы на сервере
	ASSERT_TRUE(server->parse(datagram.data(), datagram.size(), udpIn));
	// Проверяем что фрагментация отсутствует
	ASSERT_EQ(udpIn.frag, 0x00);
	// Проверяем размер разобранного UDP заголовка
	ASSERT_EQ(udpIn.size, 16u);
	// Проверяем что адрес конечного получателя получен
	ASSERT_TRUE(udpIn.host != nullptr);
	// Проверяем тип полученного адреса
	ASSERT_EQ(udpIn.host->type, awh::net::type_t::FQDN);
	// Проверяем доменное имя полученного адреса
	ASSERT_EQ(static_cast <awh::net::attr_fqdn_t *> (udpIn.host.get())->domain, "anyks.com");
	// Проверяем порт полученного адреса
	ASSERT_EQ(static_cast <awh::net::attr_fqdn_t *> (udpIn.host.get())->port, 53);
	// Проверяем что полезная нагрузка следует сразу за заголовком
	ASSERT_EQ(std::string(datagram.begin() + udpIn.size, datagram.end()), payload);
}

/**
 * @brief Метод проверки отклонения фрагментированной UDP-датаграммы
 *
 */
TEST_F(Socks5Fixture, UDPFragmentRejectedTest){
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Формируем датаграмму с признаком фрагментации (FRAG = 0x01)
	const uint8_t datagram[] = {
		0x00, 0x00, 0x01, 0x01,
		0x7F, 0x00, 0x00, 0x01, 0x00, 0x35,
		'd', 'a', 't', 'a'
	};
	// Создаём объект UDP заголовка входящей датаграммы
	socks5_t::udp_head_t udp;
	// Проверяем что фрагментированная датаграмма отклонена
	ASSERT_FALSE(server->parse(datagram, sizeof(datagram), udp));
}

/**
 * @brief Метод проверки отклонения усечённой UDP-датаграммы
 *
 */
TEST_F(Socks5Fixture, UDPTruncatedTest){
	// Создаём объект сервера SOCKS5
	std::unique_ptr <server_socks5_t> server = this->makeServer();
	// Формируем усечённую датаграмму с IPv4-адресом (не хватает порта)
	const uint8_t datagram[] = {0x00, 0x00, 0x00, 0x01, 0x7F, 0x00, 0x00, 0x01};
	// Создаём объект UDP заголовка входящей датаграммы
	socks5_t::udp_head_t udp;
	// Проверяем что усечённая датаграмма отклонена
	ASSERT_FALSE(server->parse(datagram, sizeof(datagram), udp));
}

/**
 * @brief Метод проверки отклонения слишком длинного доменного имени при формировании запроса
 *
 */
TEST_F(Socks5Fixture, ConnectDomainTooLongTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Устанавливаем состояние отправки запроса подключения
	cctx.state = state_t::CONNECT;
	// Устанавливаем команду подключения
	cctx.command = command_t::CONNECT;
	// Устанавливаем хост с доменным именем превышающим максимальную длину (255 байт)
	cctx.host = ::makeFQDN(std::string(300, 'x'), 443);
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Проверяем что формирование запроса с длинным доменным именем неуспешно
	ASSERT_FALSE(client->buffer(&data, size, cctx));
}

/**
 * @brief Метод проверки отклонения запроса CONNECT без установленного хоста
 *
 */
TEST_F(Socks5Fixture, ConnectWithoutHostTest){
	// Создаём объект клиента SOCKS5
	std::unique_ptr <client_socks5_t> client = this->makeClient();
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Устанавливаем состояние отправки запроса подключения
	cctx.state = state_t::CONNECT;
	// Устанавливаем команду подключения
	cctx.command = command_t::CONNECT;
	// Буфер и размер сформированного кадра
	uint8_t * data = nullptr;
	size_t size    = 0;
	// Проверяем что формирование запроса без хоста неуспешно
	ASSERT_FALSE(client->buffer(&data, size, cctx));
}
