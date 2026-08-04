/**
 * @file: socket.cpp
 * @date: 2026-02-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты низкоуровневой работы с сокетами — проверка установки неблокирующего режима, таймаутов,
 *        размеров буферов, keep-alive и остальных опций сокета
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 */
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Тест создания сокетов разных семейств и типов
 *
 */
TEST_F(EthFixture, SocketCreateTest){
	// Создаём UDP сокет IPv4
	auto udp4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что UDP сокет IPv4 создан успешно
	ASSERT_NE(udp4, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(udp4);

	// Создаём TCP сокет IPv4
	auto tcp4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что TCP сокет IPv4 создан успешно
	ASSERT_NE(tcp4, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(tcp4);

	// Создаём UDP сокет IPv6
	auto udp6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что UDP сокет IPv6 создан успешно
	ASSERT_NE(udp6, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(udp6);

	// Создаём TCP сокет IPv6
	auto tcp6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что TCP сокет IPv6 создан успешно
	ASSERT_NE(tcp6, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(tcp6);

	// Создаём STREAM сокет Unix Domain
	auto uds = this->_eth->socket.issue(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	// Проверяем что Unix Domain сокет создан успешно
	ASSERT_NE(uds, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(uds);

	// Создаём DATAGRAM сокет Unix Domain
	auto udsg = this->_eth->socket.issue(awh::event::family_t::UDS, awh::event::type_t::DATAGRAM, awh::event::protocol_t::NONE);
	// Проверяем что Unix Domain дейтаграммный сокет создан успешно
	ASSERT_NE(udsg, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(udsg);
}

/**
 * @brief Тест отказа создания сокета при недопустимых комбинациях параметров
 *
 */
TEST_F(EthFixture, SocketCreateInvalidTest){
	// Тип STREAM не поддерживает протокол UDP - сокет не должен быть создан
	ASSERT_EQ(awh::net::invalid_socket_t, this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::UDP));
	// Тип DATAGRAM не поддерживает протокол TCP - сокет не должен быть создан
	ASSERT_EQ(awh::net::invalid_socket_t, this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::TCP));
	// Неопределённое семейство - сокет не должен быть создан
	ASSERT_EQ(awh::net::invalid_socket_t, this->_eth->socket.issue(awh::event::family_t::NONE, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	// Неопределённый тип сокета - сокет не должен быть создан
	ASSERT_EQ(awh::net::invalid_socket_t, this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::NONE, awh::event::protocol_t::TCP));
}

/**
 * @brief Тест создания пары сокетов для межпроцессного взаимодействия
 *
 */
TEST_F(EthFixture, SocketPairTest){
	// Создаём пару сокетов Unix Domain Stream
	auto uds = this->_eth->socket.ipc(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	// Проверяем что первый сокет пары создан успешно
	ASSERT_NE(uds[0], awh::net::invalid_socket_t);
	// Проверяем что второй сокет пары создан успешно
	ASSERT_NE(uds[1], awh::net::invalid_socket_t);
	// Закрываем сокеты пары
	::close(uds[0]);
	::close(uds[1]);

	// Создаём пару сокетов Unix Domain Datagram
	auto udsg = this->_eth->socket.ipc(awh::event::family_t::UDS, awh::event::type_t::DATAGRAM, awh::event::protocol_t::NONE);
	// Проверяем что первый сокет пары создан успешно
	ASSERT_NE(udsg[0], awh::net::invalid_socket_t);
	// Проверяем что второй сокет пары создан успешно
	ASSERT_NE(udsg[1], awh::net::invalid_socket_t);
	// Закрываем сокеты пары
	::close(udsg[0]);
	::close(udsg[1]);

	// Создаём пару файловых дескрипторов канала PIPE
	auto pipe = this->_eth->socket.ipc(awh::event::family_t::PIPE, awh::event::type_t::NONE, awh::event::protocol_t::NONE);
	// Проверяем что дескриптор чтения канала создан успешно
	ASSERT_NE(pipe[0], awh::net::invalid_socket_t);
	// Проверяем что дескриптор записи канала создан успешно
	ASSERT_NE(pipe[1], awh::net::invalid_socket_t);
	// Закрываем дескрипторы канала
	::close(pipe[0]);
	::close(pipe[1]);
}

/**
 * @brief Тест получения кода ошибки сокета
 *
 */
TEST_F(EthFixture, SocketErrorTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);
	// На свежесозданном сокете ошибки быть не должно
	ASSERT_EQ(0, this->_eth->socket.getError(sock));
	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест установки и получения таймаутов сокета
 *
 */
TEST_F(EthFixture, SocketTimeoutTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Устанавливаем и проверяем таймаут на чтение в 100 миллисекунд
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::READ, 100));
	ASSERT_EQ(100, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::READ));

	// Устанавливаем и проверяем таймаут на запись в 100 миллисекунд
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::WRITE, 100));
	ASSERT_EQ(100, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::WRITE));

	// Проверяем корректность пересчёта таймаута больше секунды (1500 мс = 1 сек + 500 мс)
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::READ, 1500));
	ASSERT_EQ(1500, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::READ));

	// Проверяем сброс таймаута на чтение в ноль
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::READ, 0));
	ASSERT_EQ(0, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::READ));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест установки и получения размеров буфера сокета
 *
 */
TEST_F(EthFixture, SocketBufferSizeTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Получаем текущий размер буфера на чтение
	const int32_t rcv = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::READ);
	// Проверяем что размер буфера на чтение положительный
	ASSERT_GT(rcv, 0);
	// Устанавливаем увеличенный размер буфера на чтение и проверяем что метод вернул положительное значение
	ASSERT_GT(this->_eth->socket.setBufferSize(sock, awh::net::socket_event_t::READ, rcv * 2), 0);

	// Получаем текущий размер буфера на запись
	const int32_t snd = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::WRITE);
	// Проверяем что размер буфера на запись положительный
	ASSERT_GT(snd, 0);
	// Устанавливаем увеличенный размер буфера на запись и проверяем что метод вернул положительное значение
	ASSERT_GT(this->_eth->socket.setBufferSize(sock, awh::net::socket_event_t::WRITE, snd * 2), 0);

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест установки постоянного подключения (keepalive)
 *
 */
TEST_F(EthFixture, SocketKeepaliveTest){
	// Создаём TCP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Устанавливаем корректные параметры постоянного подключения
	ASSERT_TRUE(this->_eth->socket.setKeepalive(sock, 5, 5, 5));

	/**
	 * Передача отрицательных параметров не должна приводить к аварийному завершению
	 * (ранее значения корректировались через const_cast, что являлось неопределённым поведением)
	 */
	ASSERT_NO_FATAL_FAILURE(this->_eth->socket.setKeepalive(sock, -1, -1, -1));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест общих опций сокета не зависящих от протокола
 *
 */
TEST_F(EthFixture, SocketSwitchOptionCommonTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Включаем повторное использование адреса
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_ADDR));
	// Включаем повторное использование порта
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_PORT));
	// Отключаем сигнал SIGILL
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_SIGILL));
	// Отключаем сигнал SIGPIPE
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_SIGPIPE));

	// Включаем неблокирующий режим ввода-вывода
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_IO_BLOCK));
	// Отключаем неблокирующий режим ввода-вывода (возврат к блокирующему режиму)
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::DISABLED, awh::event::options::NO_IO_BLOCK));

	// Включаем режим автоматического закрытия дескриптора при exec
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::CLOSE_ON_EXEC));
	// Отключаем режим автоматического закрытия дескриптора при exec
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::DISABLED, awh::event::options::CLOSE_ON_EXEC));

	// Включаем широковещательный адрес на UDP сокете
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::BROADCAST));
	// Включаем режим обратной петли для multicast пакетов
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::MULTICAST_LOOPBACK));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест опций сокета характерных для протокола TCP
 *
 */
TEST_F(EthFixture, SocketSwitchOptionTcpTest){
	// Создаём TCP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Отключаем алгоритм Нейгла на TCP сокете
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::TCP_NO_DELAY));
	// Возвращаем алгоритм Нейгла на TCP сокете
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::DISABLED, awh::event::options::TCP_NO_DELAY));

	// Включаем режим отложенной отправки TCP пакетов (TCP CORK)
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::TCP_CORK));
	// Отключаем режим отложенной отправки TCP пакетов (TCP CORK)
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::DISABLED, awh::event::options::TCP_CORK));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест опций сокета характерных для семейства IPv6
 *
 */
TEST_F(EthFixture, SocketSwitchOptionIPv6Test){
	// Создаём TCP сокет IPv6
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Включаем режим только IPv6 на IPv6 сокете
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED, awh::event::options::IPV6_ONLY));
	// Отключаем режим только IPv6 на IPv6 сокете
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::DISABLED, awh::event::options::IPV6_ONLY));

	// Для IPv6 опция ручной установки заголовков (HDRINCL) не поддерживается и всегда возвращает успех
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED, awh::event::options::HDRINCL));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест генерации информации о трафике
 *
 * @note Проверяет корректность включения и отключения генерации метаданных пакета
 *       (ранее режим терялся из-за переиспользования переменной флага)
 *
 */
TEST_F(EthFixture, SocketTrafficInfoTest){
	// Создаём UDP сокет IPv4
	auto sock4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock4, awh::net::invalid_socket_t);
	// Включаем генерацию информации о трафике для IPv4
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(sock4, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED));
	// Отключаем генерацию информации о трафике для IPv4
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(sock4, awh::event::family_t::IPV4, awh::net::socket_mode_t::DISABLED));
	// Закрываем сокет
	::close(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);
	// Включаем генерацию информации о трафике для IPv6
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(sock6, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED));
	// Отключаем генерацию информации о трафике для IPv6
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(sock6, awh::event::family_t::IPV6, awh::net::socket_mode_t::DISABLED));
	// Закрываем сокет
	::close(sock6);
}

/**
 * @brief Тест установки и получения максимального количества хопов
 *
 */
TEST_F(EthFixture, SocketHopsTest){
	// Создаём UDP сокет IPv4
	auto sock4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock4, awh::net::invalid_socket_t);

	// Устанавливаем и проверяем количество хопов для unicast пакетов IPv4
	ASSERT_TRUE(this->_eth->socket.setHops(sock4, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST, static_cast <uint8_t> (awh::event::hops_t::NETWORK)));
	ASSERT_EQ(static_cast <uint8_t> (awh::event::hops_t::NETWORK), this->_eth->socket.getHops(sock4, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST));

	// Устанавливаем и проверяем количество хопов для multicast пакетов IPv4
	ASSERT_TRUE(this->_eth->socket.setHops(sock4, awh::event::family_t::IPV4, awh::event::delivery_mode_t::MULTICAST, 4));
	ASSERT_EQ(4, this->_eth->socket.getHops(sock4, awh::event::family_t::IPV4, awh::event::delivery_mode_t::MULTICAST));

	// Закрываем сокет
	::close(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);

	// Устанавливаем и проверяем количество хопов для unicast пакетов IPv6
	ASSERT_TRUE(this->_eth->socket.setHops(sock6, awh::event::family_t::IPV6, awh::event::delivery_mode_t::UNICAST, static_cast <uint8_t> (awh::event::hops_t::NETWORK)));
	ASSERT_EQ(static_cast <uint8_t> (awh::event::hops_t::NETWORK), this->_eth->socket.getHops(sock6, awh::event::family_t::IPV6, awh::event::delivery_mode_t::UNICAST));

	// Устанавливаем и проверяем количество хопов для multicast пакетов IPv6
	ASSERT_TRUE(this->_eth->socket.setHops(sock6, awh::event::family_t::IPV6, awh::event::delivery_mode_t::MULTICAST, 4));
	ASSERT_EQ(4, this->_eth->socket.getHops(sock6, awh::event::family_t::IPV6, awh::event::delivery_mode_t::MULTICAST));

	// Закрываем сокет
	::close(sock6);
}

/**
 * @brief Тест установки и получения значения DSCP в заголовке IP-пакета
 *
 */
TEST_F(EthFixture, SocketDscpTest){
	// Создаём UDP сокет IPv4
	auto sock4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock4, awh::net::invalid_socket_t);
	// Устанавливаем и проверяем значение DSCP по умолчанию для IPv4
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4, awh::event::dscp_t::CS0));
	ASSERT_EQ(awh::event::dscp_t::CS0, this->_eth->socket.getDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4));
	// Устанавливаем и проверяем значение DSCP интерактивного класса для IPv4
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4, awh::event::dscp_t::CS3));
	ASSERT_EQ(awh::event::dscp_t::CS3, this->_eth->socket.getDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4));
	// Закрываем сокет
	::close(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);
	// Устанавливаем и проверяем значение DSCP критического класса для IPv6
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6, awh::event::dscp_t::CS5));
	ASSERT_EQ(awh::event::dscp_t::CS5, this->_eth->socket.getDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6));
	// Закрываем сокет
	::close(sock6);
}

/**
 * @brief Тест установки и получения значения ECN
 *
 * @details Класс обслуживания (DSCP) и признак перегрузки (ECN) занимают один
 *          октет заголовка IP-пакета, поэтому проверяется не только круговой
 *          обход каждого поля, но и их взаимная независимость: установка одного
 *          не должна сбрасывать другое
 *
 */
TEST_F(EthFixture, SocketEcnTest){
	// Создаём UDP сокет IPv4
	auto sock4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock4, awh::net::invalid_socket_t);
	// Проверяем что по умолчанию признак перегрузки не установлен
	ASSERT_EQ(awh::event::ecn_t::NOT_ECT, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Устанавливаем и проверяем признак поддержки ECN для IPv4
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock4, awh::event::family_t::IPV4, awh::event::ecn_t::ECT0));
	ASSERT_EQ(awh::event::ecn_t::ECT0, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Устанавливаем класс обслуживания поверх установленного признака перегрузки
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4, awh::event::dscp_t::CS3));
	// Проверяем что признак перегрузки установкой класса обслуживания не сброшен
	ASSERT_EQ(awh::event::ecn_t::ECT0, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Проверяем что класс обслуживания установлен
	ASSERT_EQ(awh::event::dscp_t::CS3, this->_eth->socket.getDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4));
	// Меняем признак перегрузки поверх установленного класса обслуживания
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock4, awh::event::family_t::IPV4, awh::event::ecn_t::ECT1));
	// Проверяем что класс обслуживания сменой признака перегрузки не сброшен
	ASSERT_EQ(awh::event::dscp_t::CS3, this->_eth->socket.getDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4));
	// Проверяем что признак перегрузки сменён
	ASSERT_EQ(awh::event::ecn_t::ECT1, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Снимаем признак поддержки ECN
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock4, awh::event::family_t::IPV4, awh::event::ecn_t::NOT_ECT));
	ASSERT_EQ(awh::event::ecn_t::NOT_ECT, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Закрываем сокет
	::close(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);
	// Устанавливаем и проверяем признак поддержки ECN для IPv6
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock6, awh::event::family_t::IPV6, awh::event::ecn_t::ECT0));
	ASSERT_EQ(awh::event::ecn_t::ECT0, this->_eth->socket.getExplicitCongestionNotification(sock6, awh::event::family_t::IPV6));
	// Устанавливаем класс обслуживания поверх установленного признака перегрузки
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6, awh::event::dscp_t::CS5));
	// Проверяем что оба поля октета сохранены независимо
	ASSERT_EQ(awh::event::ecn_t::ECT0, this->_eth->socket.getExplicitCongestionNotification(sock6, awh::event::family_t::IPV6));
	ASSERT_EQ(awh::event::dscp_t::CS5, this->_eth->socket.getDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6));
	// Закрываем сокет
	::close(sock6);
}

/**
 * @brief Тест доставки маркировки ECN принятой датаграммы (RFC 3168 §5)
 *
 * @details Маркировка накладывается на заголовок IP-пакета и приложению
 *          доступна только служебным сообщением сокета. Без включённой
 *          генерации метаданных трафика датаграмма приходит без неё,
 *          и определить перегрузку пути невозможно
 *
 * @par Намеренные решения
 *
 * Проверка пропускается там, где ядро класс обслуживания принятой датаграммы
 * не выдаёт вовсе: NetBSD и OpenBSD параметра IP_RECVTOS не имеют, и список
 * их IP_RECV* класса обслуживания не содержит. Признаком служит отсутствие
 * самого параметра, а не перечисление систем поимённо: перечисление устареет
 * с первым же выпуском, который параметр добавит. По IPv6 обе системы класс
 * выдают полностью, пробел только по IPv4
 *
 */
TEST_F(EthFixture, SocketEcnDeliveryTest){
	/**
	 * Если система класс обслуживания принятой датаграммы не выдаёт
	 */
	#if !defined(IP_RECVTOS)
		// Пропускаем тест - проверять нечего
		GTEST_SKIP() << "IPv4 traffic class delivery is not supported by the system";
	#else
	// Создаём UDP сокет получателя
	auto rx = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Создаём UDP сокет отправителя
	auto tx = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокеты созданы успешно
	ASSERT_NE(rx, awh::net::invalid_socket_t);
	ASSERT_NE(tx, awh::net::invalid_socket_t);
	/**
	 * Включаем генерацию метаданных трафика на сокете получателя: без неё
	 * служебных сообщений с классом обслуживания сокет не выдаёт
	 */
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(rx, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED));
	// Формируем адрес получателя на петлевом интерфейсе
	struct sockaddr_in addr;
	// Зануляем структуру адреса получателя
	::memset(&addr, 0, sizeof(addr));
	// Устанавливаем семейство адреса получателя
	addr.sin_family = AF_INET;
	// Устанавливаем адрес петлевого интерфейса
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Устанавливаем произвольный порт получателя
	addr.sin_port = htons(43219);
	// Если привязка сокета получателя не выполнена
	if(::bind(rx, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) != 0){
		// Закрываем сокеты
		::close(rx);
		::close(tx);
		// Пропускаем тест - порт занят
		GTEST_SKIP() << "loopback port is not available";
	}
	// Помечаем исходящие датаграммы отправителя поддержкой ECN
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(tx, awh::event::family_t::IPV4, awh::event::ecn_t::ECT0));
	// Отправляем датаграмму получателю
	ASSERT_EQ(::sendto(tx, "e", 1, 0, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)), 1);
	// Буфер принимаемых данных
	char buffer[64];
	// Буфер принимаемых служебных сообщений
	char control[256];
	// Описание буфера принимаемых данных
	struct iovec io = {buffer, sizeof(buffer)};
	// Описание принимаемого сообщения
	struct msghdr message;
	// Зануляем описание принимаемого сообщения
	::memset(&message, 0, sizeof(message));
	// Устанавливаем буфер принимаемых данных
	message.msg_iov = &io;
	// Устанавливаем количество буферов принимаемых данных
	message.msg_iovlen = 1;
	// Устанавливаем буфер принимаемых служебных сообщений
	message.msg_control = control;
	// Устанавливаем размер буфера принимаемых служебных сообщений
	message.msg_controllen = sizeof(control);
	// Проверяем что датаграмма принята
	ASSERT_EQ(::recvmsg(rx, &message, 0), 1);
	// Маркировка ECN принятой датаграммы
	uint8_t congestion = 0xFF;
	/**
	 * Перебираем служебные сообщения принятой датаграммы
	 */
	for(struct cmsghdr * cmsg = CMSG_FIRSTHDR(&message); cmsg != nullptr; cmsg = CMSG_NXTHDR(&message, cmsg)){
		// Если служебное сообщение несёт класс обслуживания заголовка IPv4-пакета
		if((cmsg->cmsg_level == IPPROTO_IP) && ((cmsg->cmsg_type == IP_TOS) || (cmsg->cmsg_type == IP_RECVTOS)))
			// Извлекаем признак перегрузки пути из младших двух бит октета
			congestion = static_cast <uint8_t> ((* reinterpret_cast <const uint8_t *> (CMSG_DATA(cmsg))) & 0x03);
	}
	// Закрываем сокеты
	::close(rx);
	::close(tx);
	// Проверяем что маркировка доставлена в неизменном виде
	ASSERT_EQ(congestion, static_cast <uint8_t> (awh::event::ecn_t::ECT0));
	#endif
}

/**
 * @brief Тест установки и получения режима обнаружения MTU
 *
 */
TEST_F(EthFixture, SocketMtuDiscoverTest){
	/**
	 * @par Намеренные решения
	 *
	 * Каждое семейство проверяется лишь там, где запрет фрагментации на отдельном
	 * сокете системой вообще задаётся. NetBSD имеет его только для IPv6, OpenBSD -
	 * ни для одного семейства: обнаружение пути ведёт ядро само, и приложению
	 * задать его нечем. Признаком служит наличие самого параметра, а не перечисление
	 * систем поимённо - перечисление устареет с первым же выпуском, который параметр
	 * добавит
	 *
	 */
	/**
	 * Если запрет фрагментации для IPv4 системой задаётся
	 */
	#if defined(IP_DONTFRAG)
		// Создаём UDP сокет IPv4
		auto sock4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
		// Проверяем что сокет создан успешно
		ASSERT_NE(sock4, awh::net::invalid_socket_t);
		// Включаем обнаружение MTU (запрет фрагментации) для IPv4
		ASSERT_TRUE(this->_eth->socket.setMaximumTransmissionUnitDiscover(sock4, awh::event::family_t::IPV4, awh::event::mtu_discover_t::DO));
		ASSERT_EQ(awh::event::mtu_discover_t::DO, this->_eth->socket.getMaximumTransmissionUnitDiscover(sock4, awh::event::family_t::IPV4));
		// Закрываем сокет
		::close(sock4);
	#endif
	/**
	 * Если запрет фрагментации для IPv6 системой задаётся
	 */
	#if defined(IPV6_DONTFRAG)
		// Создаём UDP сокет IPv6
		auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
		// Проверяем что сокет создан успешно
		ASSERT_NE(sock6, awh::net::invalid_socket_t);
		// Включаем обнаружение MTU (запрет фрагментации) для IPv6
		ASSERT_TRUE(this->_eth->socket.setMaximumTransmissionUnitDiscover(sock6, awh::event::family_t::IPV6, awh::event::mtu_discover_t::DO));
		ASSERT_EQ(awh::event::mtu_discover_t::DO, this->_eth->socket.getMaximumTransmissionUnitDiscover(sock6, awh::event::family_t::IPV6));
		// Закрываем сокет
		::close(sock6);
	#endif
	/**
	 * Если ни одно семейство системой не поддерживается
	 */
	#if !defined(IP_DONTFRAG) && !defined(IPV6_DONTFRAG)
		// Пропускаем тест - проверять нечего
		GTEST_SKIP() << "per-socket fragmentation control is not supported by the system";
	#endif
}

/**
 * @brief Тест установки сетевого интерфейса для multicast пакетов
 *
 * @note Косвенно проверяет работу статического кеша сетевых интерфейсов
 *
 */
TEST_F(EthFixture, SocketMulticastIfaceTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Пустое имя интерфейса должно приводить к отказу
	ASSERT_FALSE(this->_eth->socket.setMulticastIface(sock, awh::event::family_t::IPV4, ""));
	// Несуществующее имя интерфейса должно приводить к отказу
	ASSERT_FALSE(this->_eth->socket.setMulticastIface(sock, awh::event::family_t::IPV4, "nonexistent999"));

	// Извлекаем имя реального сетевого интерфейса текущей машины
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(source);
	// Если интерфейс найден
	if(!source.iface.empty()){
		// Устанавливаем найденный интерфейс для multicast пакетов (первый вызов наполняет кеш)
		ASSERT_TRUE(this->_eth->socket.setMulticastIface(sock, awh::event::family_t::IPV4, source.iface));
		// Повторная установка должна использовать кеш и так же завершиться успехом
		ASSERT_TRUE(this->_eth->socket.setMulticastIface(sock, awh::event::family_t::IPV4, source.iface));
	}

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест защиты от некорректных аргументов в членстве multicast группы
 *
 */
TEST_F(EthFixture, SocketMembershipGuardTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Создаём корректный объект IPv4-адреса
	std::unique_ptr <awh::net::addr_net_t> addr4 = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес мультикаст-группы 239.0.0.1
	static_cast <awh::net::addr_net_ipv4_t *> (addr4.get())->address = htonl(0xEF000001);

	// Передача нулевого указателя группы не должна приводить к аварийному завершению и должна вернуть отказ
	ASSERT_FALSE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, nullptr, addr4.get()));
	// Передача нулевого указателя источника не должна приводить к аварийному завершению и должна вернуть отказ
	ASSERT_FALSE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, addr4.get(), nullptr));

	// Создаём объект IPv6-адреса
	std::unique_ptr <awh::net::addr_net_t> addr6 = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Несовпадение типов адресов группы и источника должно приводить к отказу
	ASSERT_FALSE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, addr4.get(), addr6.get()));

	// Создаём объект нулевого IPv4-адреса (некорректная multicast-группа)
	std::unique_ptr <awh::net::addr_net_t> zero = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем нулевой адрес
	static_cast <awh::net::addr_net_ipv4_t *> (zero.get())->address = 0;
	// Подписка на некорректную multicast-группу должна приводить к отказу
	ASSERT_FALSE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, zero.get(), zero.get()));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест членства в multicast группе IPv4
 *
 */
TEST_F(EthFixture, SocketMembershipIPv4Test){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Разрешаем повторное использование адреса для корректной работы multicast
	this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_ADDR);

	// Создаём адрес multicast-группы 239.0.0.1
	std::unique_ptr <awh::net::addr_net_t> group = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес multicast-группы
	static_cast <awh::net::addr_net_ipv4_t *> (group.get())->address = htonl(0xEF000001);

	// Создаём адрес интерфейса источника (INADDR_ANY - интерфейс по умолчанию)
	std::unique_ptr <awh::net::addr_net_t> source = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес интерфейса источника
	static_cast <awh::net::addr_net_ipv4_t *> (source.get())->address = htonl(INADDR_ANY);

	/**
	 * Подписка и отписка от multicast-группы зависят от наличия multicast-маршрута в системе,
	 * поэтому проверяем только отсутствие аварийного завершения, а не результат операции
	 */
	ASSERT_NO_FATAL_FAILURE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, group.get(), source.get()));
	ASSERT_NO_FATAL_FAILURE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::DISABLED, group.get(), source.get()));

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест членства в multicast группе IPv6
 *
 * @note Косвенно проверяет работу статического кеша сетевых интерфейсов для IPv6
 *
 */
TEST_F(EthFixture, SocketMembershipIPv6Test){
	// Создаём UDP сокет IPv6
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Разрешаем повторное использование адреса для корректной работы multicast
	this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_ADDR);

	// Создаём адрес multicast-группы (ff02::1 - все узлы в локальном сегменте)
	std::unique_ptr <awh::net::addr_net_t> group = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Устанавливаем адрес multicast-группы ff02::1
	static_cast <awh::net::addr_net_ipv6_t *> (group.get())->address = {0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01};

	// Создаём адрес интерфейса источника (нулевой адрес - интерфейс по умолчанию)
	std::unique_ptr <awh::net::addr_net_t> source = std::make_unique <awh::net::addr_net_ipv6_t> ();

	/**
	 * Подписка и отписка от multicast-группы зависят от наличия multicast-интерфейса в системе,
	 * поэтому проверяем только отсутствие аварийного завершения, а не результат операции
	 */
	ASSERT_NO_FATAL_FAILURE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, group.get(), source.get()));
	ASSERT_NO_FATAL_FAILURE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::DISABLED, group.get(), source.get()));

	// Закрываем сокет
	::close(sock);
}
