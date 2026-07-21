/**
 * @file: connection.cpp
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
#include "../../../include/proto/quic/params.hpp"
#include "../../../include/proto/quic/connection.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic;

/**
 * @brief Внутренние вспомогательные функции тестов соединения
 *
 */
namespace {
	/**
	 * @brief Функция подготовки соединения с указанными транспортными параметрами
	 *
	 * @param connection  объект соединения
	 * @param certificate сертификат сервера в формате PEM
	 * @param privateKey  приватный ключ сервера в формате PEM
	 * @param endpoint    роль эндпоинта
	 * @param params      транспортные параметры эндпоинта
	 */
	static void configure(connection_t & connection, const std::string & certificate, const std::string & privateKey, const endpoint_t endpoint, const params::params_t & params) noexcept {
		// Устанавливаем список поддерживаемых ALPN-протоколов
		connection.alpn({"h3"});
		// Устанавливаем транспортные параметры
		connection.params(params);
		// Если эндпоинт является сервером
		if(endpoint == endpoint_t::SERVER)
			// Устанавливаем сертификат и приватный ключ сервера
			connection.certificate(certificate, privateKey);
		// Если эндпоинт является клиентом
		else
			// Устанавливаем доменное имя удалённого сервера
			connection.serverNameIndication("localhost");
	}
	/**
	 * @brief Функция подготовки соединения со стандартными настройками
	 *
	 * @param connection  объект соединения
	 * @param certificate сертификат сервера в формате PEM
	 * @param privateKey  приватный ключ сервера в формате PEM
	 * @param endpoint    роль эндпоинта
	 */
	static void setup(connection_t & connection, const std::string & certificate, const std::string & privateKey, const endpoint_t endpoint) noexcept {
		// Транспортные параметры эндпоинта
		params::params_t params;
		// Устанавливаем лимит данных соединения
		params.initialMaxData = 1048576;
		// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
		params.initialMaxStreamDataBidiLocal = 262144;
		// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
		params.initialMaxStreamDataBidiRemote = 262144;
		// Устанавливаем лимит данных однонаправленных потоков
		params.initialMaxStreamDataUni = 262144;
		// Устанавливаем лимит числа двунаправленных потоков
		params.initialMaxStreamsBidi = 100;
		// Устанавливаем лимит числа однонаправленных потоков
		params.initialMaxStreamsUni = 100;
		// Выполняем подготовку соединения
		::configure(connection, certificate, privateKey, endpoint, params);
	}
	/**
	 * @brief Функция передачи всех исходящих датаграмм одного эндпоинта другому
	 *
	 * @param from    эндпоинт-отправитель датаграмм
	 * @param to      эндпоинт-получатель датаграмм
	 * @param now     текущее время тестовых часов в миллисекундах
	 * @param history список переданных датаграмм (для повторов в тестах)
	 * @return        количество переданных датаграмм
	 */
	static size_t transfer(connection_t & from, connection_t & to, uint64_t & now, std::vector <std::string> * history = nullptr) noexcept {
		// Количество переданных датаграмм
		size_t result = 0;
		// Буфер исходящей датаграммы
		std::string datagram = "";
		/**
		 *  Извлекаем исходящие датаграммы отправителя (с запасом итераций)
		 */
		while((result < 16) && from.write(datagram, now)){
			// Продвигаем тестовые часы (имитация задержки сети)
			now += 5;
			// Передаём датаграмму получателю
			if(to.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now) != status_t::OK)
				// Прекращаем передачу датаграмм
				break;
			// Если ведётся список переданных датаграмм
			if(history != nullptr)
				// Сохраняем датаграмму в список
				history->push_back(datagram);
			// Считаем переданную датаграмму
			result++;
		}
		// Выводим количество переданных датаграмм
		return result;
	}
	/**
	 * @brief Функция обмена датаграммами до полного затишья
	 *
	 * @param client эндпоинт клиента
	 * @param server эндпоинт сервера
	 * @param now    текущее время тестовых часов в миллисекундах
	 */
	static void pump(connection_t & client, connection_t & server, uint64_t & now) noexcept {
		// Выполняем обмен датаграммами (с запасом итераций)
		for(size_t i = 0; i < 10; i++){
			// Передаём датаграммы клиента серверу
			const size_t sent = ::transfer(client, server, now);
			// Передаём датаграммы сервера клиенту
			const size_t received = ::transfer(server, client, now);
			// Если обмен датаграммами завершён
			if((sent == 0) && (received == 0))
				// Прекращаем обмен датаграммами
				break;
		}
	}
	/**
	 * @brief Функция выполнения полного установления соединения между клиентом и сервером
	 *
	 * @param client  эндпоинт клиента
	 * @param server  эндпоинт сервера
	 * @param now     текущее время тестовых часов в миллисекундах
	 * @param history список переданных датаграмм сервера (для повторов в тестах)
	 * @return        результат установления соединения
	 */
	static bool establish(connection_t & client, connection_t & server, uint64_t & now, std::vector <std::string> * history = nullptr) noexcept {
		/**
		 * Выполняем обмен датаграммами (с запасом итераций)
		 */
		for(size_t i = 0; i < 10; i++){
			// Передаём датаграммы клиента серверу
			const size_t sent = ::transfer(client, server, now);
			// Передаём датаграммы сервера клиенту
			const size_t received = ::transfer(server, client, now, history);
			// Если соединение установлено на обоих эндпоинтах и обмен завершён
			if((sent == 0) && (received == 0) &&
			   (client.state() == connection_t::state_t::CONNECTED) &&
			   (server.state() == connection_t::state_t::CONNECTED))
				// Выводим положительный результат
				return true;
		}
		// Выводим отрицательный результат - обмен не сошёлся
		return false;
	}
};

/**
 * @brief Тест полного установления соединения через UDP-датаграммы
 *
 */
TEST_F(QuicFixture, ConnectionEstablishTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Проверяем состояние выполнения хендшейка
	ASSERT_EQ(client.state(), connection_t::state_t::HANDSHAKING);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем состояние соединения сервера
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на клиенте
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	// Проверяем отсутствие ошибки транспорта на сервере
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
	// Проверяем согласованный ALPN-протокол на обоих эндпоинтах
	ASSERT_EQ(client.alpn(), "h3");
	ASSERT_EQ(server.alpn(), "h3");
	// Проверяем согласованность идентификаторов соединения (RFC 9000 §7.2)
	ASSERT_TRUE(client.dcid() == server.scid());
	ASSERT_TRUE(server.dcid() == client.scid());
}

/**
 * @brief Тест дополнения первой датаграммы клиента до минимального размера (RFC 9000 §14.1)
 *
 */
TEST_F(QuicFixture, ConnectionInitialPaddingTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента
	ASSERT_TRUE(client.write(datagram, 1000));
	// Проверяем что датаграмма с пакетом Initial дополнена до минимального размера
	ASSERT_GE(datagram.size(), 1200);
	// Проверяем что датаграмма не превышает максимального размера
	ASSERT_LE(datagram.size(), connection_t::MAX_DATAGRAM_SIZE);
}

/**
 * @brief Тест обмена транспортными параметрами через соединение
 *
 */
TEST_F(QuicFixture, ConnectionPeerParamsTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Транспортные параметры удалённого узла
	params::params_t params;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Извлекаем транспортные параметры сервера на клиенте
	ASSERT_EQ(client.peer(params, error), status_t::OK);
	// Проверяем лимит данных соединения сервера
	ASSERT_EQ(params.initialMaxData, 1048576);
	// Проверяем что сервер прислал свой SCID (RFC 9000 §7.3)
	ASSERT_TRUE(params.hasInitialScid);
	ASSERT_TRUE(params.initialScid == client.dcid());
	// Проверяем что сервер прислал исходный DCID клиента (RFC 9000 §7.3)
	ASSERT_TRUE(params.hasOdcid);
	// Извлекаем транспортные параметры клиента на сервере
	ASSERT_EQ(server.peer(params, error), status_t::OK);
	// Проверяем что клиент прислал свой SCID (RFC 9000 §7.3)
	ASSERT_TRUE(params.hasInitialScid);
	ASSERT_TRUE(params.initialScid == server.dcid());
}

/**
 * @brief Тест завершения соединения приложением (RFC 9000 §10.2)
 *
 */
TEST_F(QuicFixture, ConnectionCloseTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Выполняем завершение соединения приложением на клиенте
	client.close(0x0100, "goodbye");
	// Проверяем состояние завершения соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
	// Буфер датаграммы завершения соединения
	std::string datagram = "";
	// Извлекаем датаграмму завершения соединения
	ASSERT_TRUE(client.write(datagram, now));
	// Проверяем что фрейм CONNECTION_CLOSE отправляется однократно
	std::string empty = "";
	ASSERT_FALSE(client.write(empty, now));
	// Передаём датаграмму завершения серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем состояние завершения соединения удалённым эндпоинтом
	ASSERT_EQ(server.state(), connection_t::state_t::DRAINING);
	// Проверяем код ошибки приложения на сервере
	ASSERT_EQ(server.error(), error_t::APPLICATION_ERROR);
	// Проверяем что сервер в состоянии DRAINING не отправляет датаграмм
	ASSERT_FALSE(server.write(datagram, now));
}

/**
 * @brief Тест повторного приёма датаграммы (защита от дубликатов)
 *
 */
TEST_F(QuicFixture, ConnectionDuplicateTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Список переданных датаграмм сервера
	std::vector <std::string> history;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now, &history));
	// Проверяем что датаграммы сервера были переданы
	ASSERT_FALSE(history.empty());
	/**
	 * Перебираем список переданных датаграмм сервера
	 */
	for(auto & datagram : history)
		// Повторно передаём датаграмму клиенту - дубликаты отбрасываются
		ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что состояние соединения клиента не изменилось
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на клиенте
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест устойчивости к мусорным датаграммам
 *
 */
TEST_F(QuicFixture, ConnectionGarbageTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Формируем мусорную датаграмму
	std::string garbage(64, '\xAA');
	// Передаём мусорную датаграмму клиенту - датаграмма отбрасывается
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (garbage.data()), garbage.size(), now), status_t::OK);
	// Проверяем что состояние соединения клиента не изменилось
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Формируем датаграмму из нулевых октетов
	std::string zeros(64, '\x00');
	// Передаём датаграмму из нулевых октетов серверу - датаграмма отбрасывается
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (zeros.data()), zeros.size(), now), status_t::OK);
	// Проверяем что состояние соединения сервера не изменилось
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
}

/**
 * @brief Тест недопустимых операций соединения
 *
 */
TEST_F(QuicFixture, ConnectionMisuseTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Проверяем что сервер не может начать соединение методом connect()
	ASSERT_EQ(server.connect(), status_t::ERROR);
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Тестовые данные датаграммы
	const uint8_t data[4] = {0xC0, 0x00, 0x00, 0x00};
	// Проверяем что обработка датаграмм до начала соединения невозможна
	ASSERT_EQ(client.read(data, sizeof(data), 1000), status_t::ERROR);
	// Буфер исходящей датаграммы
	std::string datagram = "";
	// Проверяем что до начала соединения датаграммы не собираются
	ASSERT_FALSE(client.write(datagram, 1000));
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Проверяем что повторное начало соединения невозможно
	ASSERT_EQ(client.connect(), status_t::ERROR);
}

/**
 * @brief Тест ретрансмиссии первой датаграммы клиента по таймеру PTO (RFC 9002 §6.2)
 *
 */
TEST_F(QuicFixture, ConnectionLossFirstFlightTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента и теряем её (не передаём серверу)
	ASSERT_TRUE(client.write(datagram, now));
	// Проверяем что таймер PTO взведён после отправки
	const uint64_t deadline = client.timeout();
	ASSERT_GT(deadline, now);
	// Проверяем что до истечения таймера повторных датаграмм нет
	ASSERT_FALSE(client.write(datagram, now));
	// Продвигаем тестовые часы до дедлайна таймера PTO
	now = deadline;
	// Выполняем обработку просроченного таймера PTO
	client.tick(now);
	// Извлекаем ретрансмиссию первой датаграммы клиента
	ASSERT_TRUE(client.write(datagram, now));
	// Проверяем что ретрансмиссия дополнена до минимального размера (RFC 9000 §14.1)
	ASSERT_GE(datagram.size(), 1200);
	// Передаём ретрансмиссию серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем состояние соединения сервера
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
}

/**
 * @brief Тест ретрансмиссии потерянного флайта сервера по таймеру PTO (RFC 9002 §6.2)
 *
 */
TEST_F(QuicFixture, ConnectionLossServerFlightTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Передаём первую датаграмму клиента серверу
	ASSERT_EQ(::transfer(client, server, now), 1);
	// Буфер исходящей датаграммы
	std::string datagram = "";
	// Количество потерянных датаграмм сервера
	size_t dropped = 0;
	/**
	 *  Извлекаем и теряем весь флайт сервера (не передаём клиенту)
	 */
	while(server.write(datagram, now))
		// Считаем потерянную датаграмму
		dropped++;
	// Проверяем что флайт сервера был собран
	ASSERT_GT(dropped, 0);
	// Проверяем что таймер PTO сервера взведён
	ASSERT_GT(server.timeout(), now);
	/**
	 * Выполняем обмен датаграммами с обработкой таймеров (с запасом итераций)
	 */
	for(size_t i = 0; i < 20; i++){
		// Передаём датаграммы клиента серверу
		::transfer(client, server, now);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
		// Если соединение установлено на обоих эндпоинтах
		if((client.state() == connection_t::state_t::CONNECTED) &&
		   (server.state() == connection_t::state_t::CONNECTED))
			// Прекращаем обмен датаграммами
			break;
		// Получаем дедлайны таймеров обоих эндпоинтов
		const uint64_t clientTime = client.timeout();
		const uint64_t serverTime = server.timeout();
		// Вычисляем ближайший ненулевой дедлайн таймеров
		uint64_t nearest = 0;
		// Если таймер клиента взведён
		if(clientTime > 0)
			// Устанавливаем дедлайн таймера клиента
			nearest = clientTime;
		// Если таймер сервера взведён и является ближайшим
		if((serverTime > 0) && ((nearest == 0) || (serverTime < nearest)))
			// Устанавливаем дедлайн таймера сервера
			nearest = serverTime;
		// Если таймеры не взведены - обмен не сойдётся
		if(nearest == 0)
			// Прекращаем обмен датаграммами
			break;
		// Продвигаем тестовые часы до ближайшего дедлайна
		now = ((nearest > now) ? nearest : now);
		// Выполняем обработку просроченных таймеров клиента
		client.tick(now);
		// Выполняем обработку просроченных таймеров сервера
		server.tick(now);
	}
	// Проверяем состояние соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем состояние соединения сервера
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на клиенте
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	// Проверяем отсутствие ошибки транспорта на сервере
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест разоружения таймеров после подтверждения всех пакетов
 *
 */
TEST_F(QuicFixture, ConnectionTimerIdleTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Проверяем что до начала соединения таймеры не взведены
	ASSERT_EQ(client.timeout(), 0);
	ASSERT_EQ(server.timeout(), 0);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем что после подтверждения всех пакетов таймер клиента разоружён
	ASSERT_EQ(client.timeout(), 0);
	// Проверяем что после подтверждения всех пакетов таймер сервера разоружён
	ASSERT_EQ(server.timeout(), 0);
	// Проверяем что обработка таймеров в покое не порождает датаграмм
	client.tick(now + 10000);
	server.tick(now + 10000);
	// Буфер исходящей датаграммы
	std::string datagram = "";
	ASSERT_FALSE(client.write(datagram, now + 10000));
	ASSERT_FALSE(server.write(datagram, now + 10000));
}

/**
 * @brief Тест обмена данными по двунаправленному потоку (эхо)
 *
 */
TEST_F(QuicFixture, StreamEchoTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем идентификатор первого двунаправленного потока клиента (RFC 9000 §2.1)
	ASSERT_EQ(sid, 0);
	// Ставим данные запроса в очередь отправки с завершением потока
	ASSERT_EQ(client.send(sid, "hello quic streams", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Проверяем список потоков с данными на сервере
	const std::vector <uint64_t> readable = server.readable();
	ASSERT_EQ(readable.size(), 1);
	ASSERT_EQ(readable.front(), sid);
	// Принятые данные запроса
	std::string request = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём собранные данные потока на сервере
	ASSERT_EQ(server.receive(sid, request, fin), status_t::OK);
	// Проверяем данные и завершение потока
	ASSERT_EQ(request, "hello quic streams");
	ASSERT_TRUE(fin);
	// Ставим данные ответа в очередь отправки с завершением потока
	ASSERT_EQ(server.send(sid, "echo: hello quic streams", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Принятые данные ответа
	std::string response = "";
	// Сбрасываем флаг завершения потока
	fin = false;
	// Выдаём собранные данные потока на клиенте
	ASSERT_EQ(client.receive(sid, response, fin), status_t::OK);
	// Проверяем данные и завершение потока
	ASSERT_EQ(response, "echo: hello quic streams");
	ASSERT_TRUE(fin);
	// Проверяем отсутствие ошибок транспорта
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест приёма фрейма STREAM в датаграмме с завершением хендшейка (RFC 9000 §12.2)
 *
 */
TEST_F(QuicFixture, StreamCoalescedHandshakeTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Передаём первую датаграмму клиента серверу
	ASSERT_EQ(::transfer(client, server, now), 1);
	// Передаём флайт сервера клиенту - хендшейк клиента завершается
	ASSERT_GT(::transfer(server, client, now), 0);
	// Проверяем что соединение клиента установлено
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Открываем двунаправленный поток на клиенте до отправки завершения хендшейка
	const uint64_t sid = client.open(false);
	// Проверяем идентификатор первого двунаправленного потока клиента
	ASSERT_EQ(sid, 0);
	// Ставим данные запроса в очередь отправки с завершением потока
	ASSERT_EQ(client.send(sid, "coalesced stream data", true), status_t::OK);
	/**
	 * Передаём датаграммы клиента серверу - завершение хендшейка (Handshake)
	 * и данные потока (1-RTT) коалесцируются в одну датаграмму
	 */
	ASSERT_GT(::transfer(client, server, now), 0);
	// Проверяем что соединение сервера установлено
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на сервере
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
	// Проверяем список потоков с данными на сервере
	const std::vector <uint64_t> readable = server.readable();
	ASSERT_EQ(readable.size(), 1);
	ASSERT_EQ(readable.front(), sid);
	// Принятые данные запроса
	std::string request = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём собранные данные потока на сервере
	ASSERT_EQ(server.receive(sid, request, fin), status_t::OK);
	// Проверяем данные и завершение потока
	ASSERT_EQ(request, "coalesced stream data");
	ASSERT_TRUE(fin);
}

/**
 * @brief Тест передачи большого объёма данных несколькими датаграммами
 *
 */
TEST_F(QuicFixture, StreamLargeTransferTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Формируем большой блок данных (несколько датаграмм)
	std::string payload = "";
	// Заполняем блок данных проверяемым шаблоном
	for(size_t i = 0; i < 10000; i++)
		// Дописываем октет шаблона
		payload.push_back(static_cast <char> ('A' + (i % 26)));
	// Ставим данные в очередь отправки с завершением потока
	ASSERT_EQ(client.send(sid, payload, true), status_t::OK);
	// Принятые данные потока
	std::string received = "";
	// Флаг завершения потока
	bool fin = false;
	/**
	 * Выполняем обмен датаграммами до полного приёма данных
	 */
	for(size_t i = 0; (i < 50) && !fin; i++){
		// Выполняем обмен датаграммами
		::pump(client, server, now);
		// Выдаём собранные данные потока на сервере
		ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	}
	// Проверяем полноту и целостность принятых данных
	ASSERT_TRUE(fin);
	ASSERT_EQ(received, payload);
}

/**
 * @brief Тест однонаправленных потоков в обе стороны
 *
 */
TEST_F(QuicFixture, StreamUnidirectionalTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем однонаправленный поток на клиенте
	const uint64_t clientSid = client.open(true);
	// Проверяем идентификатор первого однонаправленного потока клиента (RFC 9000 §2.1)
	ASSERT_EQ(clientSid, 2);
	// Открываем однонаправленный поток на сервере
	const uint64_t serverSid = server.open(true);
	// Проверяем идентификатор первого однонаправленного потока сервера (RFC 9000 §2.1)
	ASSERT_EQ(serverSid, 3);
	// Ставим данные в очереди отправки с завершением потоков
	ASSERT_EQ(client.send(clientSid, "client to server", true), status_t::OK);
	ASSERT_EQ(server.send(serverSid, "server to client", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Принятые данные потоков
	std::string data = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём собранные данные однонаправленного потока клиента на сервере
	ASSERT_EQ(server.receive(clientSid, data, fin), status_t::OK);
	ASSERT_EQ(data, "client to server");
	ASSERT_TRUE(fin);
	// Выдаём собранные данные однонаправленного потока сервера на клиенте
	data.clear();
	fin = false;
	ASSERT_EQ(client.receive(serverSid, data, fin), status_t::OK);
	ASSERT_EQ(data, "server to client");
	ASSERT_TRUE(fin);
	// Проверяем что отправка в чужой однонаправленный поток недопустима (RFC 9000 §2.1)
	ASSERT_EQ(server.send(clientSid, "reverse", false), status_t::ERROR);
	ASSERT_EQ(client.send(serverSid, "reverse", false), status_t::ERROR);
}

/**
 * @brief Тест лимита количества потоков и его продвижения (RFC 9000 §4.6)
 *
 */
TEST_F(QuicFixture, StreamLimitTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Транспортные параметры эндпоинтов
	params::params_t params;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимиты данных потоков
	params.initialMaxStreamDataBidiLocal = 262144;
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит в один двунаправленный поток
	params.initialMaxStreamsBidi = 1;
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединений с ограниченными параметрами
	::configure(client, certificate, privateKey, endpoint_t::CLIENT, params);
	::configure(server, certificate, privateKey, endpoint_t::SERVER, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем первый двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	ASSERT_EQ(sid, 0);
	// Проверяем что лимит потоков удалённого эндпоинта исчерпан
	ASSERT_EQ(client.open(false), connection_t::INVALID_STREAM);
	// Завершаем поток без данных
	ASSERT_EQ(client.send(sid, "", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Принятые данные потока
	std::string data = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём завершение потока на сервере (лимит потоков продвигается)
	ASSERT_EQ(server.receive(sid, data, fin), status_t::OK);
	ASSERT_TRUE(fin);
	// Выполняем обмен датаграммами (сервер отправляет MAX_STREAMS)
	::pump(client, server, now);
	// Проверяем что открытие нового потока стало доступно
	ASSERT_EQ(client.open(false), 4);
}

/**
 * @brief Тест flow control потока с продвижением окна (RFC 9000 §4.1)
 *
 */
TEST_F(QuicFixture, StreamFlowControlTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Транспортные параметры эндпоинтов
	params::params_t params;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем маленькие лимиты данных потоков (окно 64 октета)
	params.initialMaxStreamDataBidiLocal = 64;
	params.initialMaxStreamDataBidiRemote = 64;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initialMaxStreamsBidi = 100;
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединений с ограниченными параметрами
	::configure(client, certificate, privateKey, endpoint_t::CLIENT, params);
	::configure(server, certificate, privateKey, endpoint_t::SERVER, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Формируем блок данных больше окна flow control
	std::string payload(256, 'X');
	// Ставим данные в очередь отправки с завершением потока
	ASSERT_EQ(client.send(sid, payload, true), status_t::OK);
	// Выполняем обмен датаграммами (первая порция ограничена окном)
	::pump(client, server, now);
	// Принятые данные потока
	std::string received = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём первую порцию данных на сервере
	ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	// Проверяем что первая порция ограничена окном flow control
	ASSERT_EQ(received.size(), 64);
	ASSERT_FALSE(fin);
	/**
	 * Выполняем обмен датаграммами до полного приёма данных
	 */
	for(size_t i = 0; (i < 50) && !fin; i++){
		// Выполняем обмен датаграммами (окно продвигается фреймами MAX_STREAM_DATA)
		::pump(client, server, now);
		// Выдаём собранные данные потока на сервере
		ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	}
	// Проверяем полноту принятых данных
	ASSERT_TRUE(fin);
	ASSERT_EQ(received, payload);
	// Проверяем отсутствие ошибок транспорта
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест аварийного завершения потока отправителем (RFC 9000 §19.4)
 *
 */
TEST_F(QuicFixture, StreamResetTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "partial data", false), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Выполняем аварийное завершение потока на клиенте
	client.reset(sid, 0x0101);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Код ошибки приложения принятого фрейма RESET_STREAM
	uint64_t code = 0;
	// Проверяем что сервер принял аварийное завершение потока
	ASSERT_TRUE(server.aborted(sid, code));
	ASSERT_EQ(code, 0x0101);
	// Принятые данные потока
	std::string data = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что выдача данных сброшенного потока недоступна
	ASSERT_EQ(server.receive(sid, data, fin), status_t::ERROR);
	// Проверяем что отправка в сброшенный поток недопустима
	ASSERT_EQ(client.send(sid, "more", false), status_t::ERROR);
}

/**
 * @brief Тест запроса прекращения передачи получателем (RFC 9000 §19.5)
 *
 */
TEST_F(QuicFixture, StreamStopSendingTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "unwanted data", false), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Выполняем запрос прекращения передачи на сервере
	server.stop(sid, 0x0202);
	// Выполняем обмен датаграммами (STOP_SENDING - RESET_STREAM)
	::pump(client, server, now);
	// Код ошибки приложения принятого фрейма RESET_STREAM
	uint64_t code = 0;
	// Проверяем что клиент ответил аварийным завершением потока (RFC 9000 §3.5)
	ASSERT_TRUE(server.aborted(sid, code));
	ASSERT_EQ(code, 0x0202);
	// Проверяем что отправка в прекращённый поток недопустима
	ASSERT_EQ(client.send(sid, "more", false), status_t::ERROR);
	// Проверяем отсутствие ошибок транспорта
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест ретрансмиссии потерянных данных потока (RFC 9002 §6.3)
 *
 */
TEST_F(QuicFixture, StreamLossTest){
	// Сертификат сервера в формате PEM
	std::string certificate = "";
	// Приватный ключ сервера в формате PEM
	std::string privateKey = "";
	// Выполняем генерацию самоподписанного сертификата
	ASSERT_TRUE(this->makeCertificate(certificate, privateKey));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER);
	// Выполняем подготовку соединения клиента
	::setup(client, certificate, privateKey, endpoint_t::CLIENT);
	// Выполняем подготовку соединения сервера
	::setup(server, certificate, privateKey, endpoint_t::SERVER);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Ставим данные в очередь отправки с завершением потока
	ASSERT_EQ(client.send(sid, "lost stream data", true), status_t::OK);
	// Буфер исходящей датаграммы
	std::string datagram = "";
	// Извлекаем датаграмму с данными потока и теряем её (не передаём серверу)
	ASSERT_TRUE(client.write(datagram, now));
	// Проверяем что таймер PTO взведён после отправки
	const uint64_t deadline = client.timeout();
	ASSERT_GT(deadline, now);
	// Продвигаем тестовые часы до дедлайна таймера PTO
	now = deadline;
	// Выполняем обработку просроченного таймера PTO
	client.tick(now);
	// Выполняем обмен датаграммами (ретрансмиссия данных потока)
	::pump(client, server, now);
	// Принятые данные потока
	std::string received = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём собранные данные потока на сервере
	ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	// Проверяем данные и завершение потока
	ASSERT_EQ(received, "lost stream data");
	ASSERT_TRUE(fin);
}
