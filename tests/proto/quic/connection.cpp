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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"
#include "../../../include/proto/quic/params.hpp"
#include "../../../include/net/tls/coder.hpp"
#include "../../../include/proto/quic/frame.hpp"
#include "../../../include/proto/quic/packet.hpp"
#include "../../../include/proto/quic/crypto.hpp"
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
	 * @note Криптография задана шаблоном контекста кодера, из которого создано
	 *       соединение, поэтому подготовка сводится к транспортным параметрам
	 *
	 * @param connection объект соединения
	 * @param params     транспортные параметры эндпоинта
	 */
	static void configure(connection_t & connection, const params::params_t & params) noexcept {
		// Устанавливаем транспортные параметры
		connection.params(params);
	}
	/**
	 * @brief Функция подготовки соединения со стандартными настройками
	 *
	 * @param connection объект соединения
	 */
	static void setup(connection_t & connection) noexcept {
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
		::configure(connection, params);
	}
	/**
	 * @brief Функция передачи всех исходящих датаграмм одного эндпоинта другому
	 *
	 * @param from    эндпоинт-отправитель датаграмм
	 * @param to      эндпоинт-получатель датаграмм
	 * @param now     текущее время тестовых часов в миллисекундах
	 * @param history список переданных датаграмм (для повторов в тестах)
	 * @param ecn     маркировка ECN, с которой датаграммы доставляются получателю
	 * @return        количество переданных датаграмм
	 */
	static size_t transfer(connection_t & from, connection_t & to, uint64_t & now, std::vector <std::string> * history = nullptr, const awh::event::ecn_t ecn = awh::event::ecn_t::NOT_ECT) noexcept {
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
			if(to.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now, ecn) != status_t::OK)
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
	 * @param ecn     маркировка ECN, с которой датаграммы клиента доставляются серверу
	 * @return        результат установления соединения
	 */
	static bool establish(connection_t & client, connection_t & server, uint64_t & now, std::vector <std::string> * history = nullptr, const awh::event::ecn_t ecn = awh::event::ecn_t::NOT_ECT) noexcept {
		/**
		 * Выполняем обмен датаграммами (с запасом итераций)
		 */
		for(size_t i = 0; i < 10; i++){
			// Передаём датаграммы клиента серверу
			const size_t sent = ::transfer(client, server, now, nullptr, ecn);
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
	/**
	 * @brief Функция доставки произвольной нагрузки пакетом 1-RTT
	 *
	 * @note Собственный сборщик пакетов коалесценцией фреймов управляет сам,
	 *       поэтому нагрузку с заданным набором фреймов через него не получить.
	 *       Функция собирает и защищает пакет напрямую ключами эндпоинта-отправителя
	 *
	 * @param from    эндпоинт-отправитель пакета
	 * @param to      эндпоинт-получатель пакета
	 * @param pn      номер отправляемого пакета
	 * @param payload нагрузка пакета (фреймы)
	 * @param now     текущее время тестовых часов в миллисекундах
	 * @return        результат обработки нагрузки получателем
	 */
	static status_t inject(connection_t & from, connection_t & to, const uint64_t pn, const std::string & payload, const uint64_t now) noexcept {
		// Получаем ключи защиты исходящих пакетов уровня приложения отправителя
		const crypto::keys_t * keys = from.handshake().encryption(level_t::APPLICATION);
		// Если ключи защиты пакетов уровня приложения не выведены
		if(keys == nullptr)
			// Выводим отрицательный результат
			return status_t::ERROR;
		// Собираемый заголовок пакета
		std::string header = "";
		// Выполняем сборку короткого заголовка пакета 1-RTT с битом фазы ключей
		if(!packet::serialize::shortHeader(header, from.dcid(), pn, 4, from.phase(), false))
			// Выводим отрицательный результат
			return status_t::ERROR;
		// Собираемая датаграмма пакета
		std::string datagram = "";
		// Выполняем защиту пакета: AEAD-шифрование нагрузки и защита заголовка
		if(!crypto::seal(datagram, * keys, pn, header, payload))
			// Выводим отрицательный результат
			return status_t::ERROR;
		// Выводим результат обработки датаграммы получателем
		return to.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
	}
	/**
	 * @brief Функция сборки и защиты пакета с длинным заголовком
	 *
	 * @note Пакет дописывается в буфер датаграммы, поэтому повторные вызовы
	 *       собирают коалесцированную датаграмму (RFC 9000 §12.2)
	 *
	 * @param from    эндпоинт-отправитель пакета
	 * @param level   уровень шифрования пакета
	 * @param type    тип собираемого пакета
	 * @param pn      номер отправляемого пакета
	 * @param payload нагрузка пакета (фреймы)
	 * @param output  буфер собираемой датаграммы
	 * @return        результат сборки
	 */
	static bool build(connection_t & from, const level_t level, const packet_t type, const uint64_t pn, const std::string & payload, std::string & output) noexcept {
		// Получаем ключи защиты исходящих пакетов уровня отправителя
		const crypto::keys_t * keys = from.handshake().encryption(level);
		// Если ключи защиты пакетов уровня не выведены
		if(keys == nullptr)
			// Выводим отрицательный результат
			return false;
		// Собираемый заголовок пакета
		std::string header = "";
		// Вычисляем значение поля Length: номер пакета + нагрузка + тег AEAD
		const uint64_t length = (4 + payload.size() + crypto::AEAD_TAG_SIZE);
		// Выполняем сборку длинного заголовка пакета
		if(!packet::serialize::longHeader(header, type, proto::VERSION_1, from.dcid(), from.scid(), "", length, pn, 4))
			// Выводим отрицательный результат
			return false;
		// Выполняем защиту пакета: AEAD-шифрование нагрузки и защита заголовка
		return crypto::seal(output, * keys, pn, header, payload);
	}
	/**
	 * @brief Функция снятия защиты с пакета 1-RTT принятой датаграммы
	 *
	 * @note Собранная эндпоинтом нагрузка защищена, поэтому проверить набор
	 *       отправленных фреймов иначе как снятием защиты невозможно
	 *
	 * @param to       эндпоинт-получатель датаграммы
	 * @param datagram принятая датаграмма
	 * @param output   расшифрованная нагрузка пакета
	 * @return         результат снятия защиты
	 */
	static bool unseal(connection_t & to, const std::string & datagram, std::string & output) noexcept {
		// Получаем ключи снятия защиты входящих пакетов уровня приложения
		const crypto::keys_t * keys = to.handshake().decryption(level_t::APPLICATION);
		// Если ключи снятия защиты пакетов уровня приложения не выведены
		if(keys == nullptr)
			// Выводим отрицательный результат
			return false;
		// Копия датаграммы: снятие защиты выполняется на месте
		std::string buffer(datagram);
		// Разобранный заголовок пакета
		packet::header_t header;
		// Код ошибки транспорта
		error_t error = error_t::NO_ERROR;
		// Выполняем разбор заголовка пакета
		if(packet::parser::header(reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), connection_t::LOCAL_CID_SIZE, header, error) != status_t::OK)
			// Выводим отрицательный результат
			return false;
		// Номер принятого пакета
		uint64_t pn = 0;
		// Выполняем снятие защиты пакета: защита заголовка и AEAD-расшифровка
		return (crypto::open(reinterpret_cast <uint8_t *> (&buffer[0]), header.size, header.pnOffset, 0, * keys, pn, output, error) == status_t::OK);
	}
	/**
	 * @brief Функция поиска пакета заданного типа в датаграмме
	 *
	 * @note Датаграмма может содержать несколько коалесцированных пакетов, поэтому
	 *       выполняется обход всех. Биты типа первого октета защитой заголовка
	 *       не закрываются, и тип читается без снятия защиты (RFC 9001 §5.4.2)
	 *
	 * @param datagram датаграмма для обхода
	 * @param type     искомый тип пакета
	 * @return         результат поиска
	 */
	static bool contains(const std::string & datagram, const packet_t type) noexcept {
		// Смещение очередного пакета в датаграмме
		size_t offset = 0;
		/**
		 *  Перебираем коалесцированные пакеты датаграммы
		 */
		while(offset < datagram.size()){
			// Код ошибки разбора заголовка
			error_t error = error_t::NO_ERROR;
			// Заголовок очередного пакета
			packet::header_t header;
			// Если разбор заголовка очередного пакета не выполнен
			if(packet::parser::header(reinterpret_cast <const uint8_t *> (datagram.data()) + offset, datagram.size() - offset, connection_t::LOCAL_CID_SIZE, header, error) != status_t::OK)
				// Прекращаем обход - остаток датаграммы неразбираем
				break;
			// Если тип пакета совпадает с искомым
			if(header.type == type)
				// Выводим положительный результат
				return true;
			// Если размер пакета не определён - обход продолжить невозможно
			if(header.size == 0)
				// Прекращаем обход датаграммы
				break;
			// Переходим к следующему пакету датаграммы
			offset += header.size;
		}
		// Выводим отрицательный результат - пакет искомого типа не найден
		return false;
	}
};

/**
 * @brief Тест полного установления соединения через UDP-датаграммы
 *
 */
TEST_F(QuicFixture, ConnectionEstablishTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	ASSERT_EQ(client.alpn().protocol, "h3");
	ASSERT_EQ(server.alpn().protocol, "h3");
	// Проверяем согласованность идентификаторов соединения (RFC 9000 §7.2)
	ASSERT_TRUE(client.dcid() == server.scid());
	ASSERT_TRUE(server.dcid() == client.scid());
}

/**
 * @brief Тест дополнения первой датаграммы клиента до минимального размера (RFC 9000 §14.1)
 *
 */
TEST_F(QuicFixture, ConnectionInitialPaddingTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
 * @brief Тест прекращения разбора нагрузки после фрейма CONNECTION_CLOSE (RFC 9000 §10.2.2)
 *
 * @details Фреймы за фреймом завершения относятся к соединению, которое удалённый
 *          эндпоинт уже завершил. Ошибка разбора в этом остатке поставила бы в
 *          очередь собственное завершение, а отправлять его в состоянии завершения
 *          удалённым узлом запрещено
 */
TEST_F(QuicFixture, ConnectionCloseTrailingFrameTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Собираемая нагрузка пакета сервера
	std::string payload = "";
	// Выполняем сборку фрейма CONNECTION_CLOSE с кодом ошибки приложения
	frame::serialize::connectionClose(payload, 0x0100, 0, "goodbye", true);
	/**
	 * Дописываем в нагрузку фрейм неизвестного типа: разбор такого фрейма завершается
	 * ошибкой кодирования, и до исправления он переводил соединение из завершённого
	 * состояния обратно в состояние отправки собственного завершения
	 */
	payload.push_back(static_cast <char> (0x3F));
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1000, payload, now), status_t::OK);
	// Проверяем что клиент перешёл в завершённое состояние
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Проверяем что причиной завершения осталась ошибка приложения удалённого узла
	ASSERT_EQ(client.error(), error_t::APPLICATION_ERROR);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Проверяем что клиент в завершённом состоянии датаграмм не отправляет
	ASSERT_FALSE(client.write(datagram, now));
}

/**
 * @brief Тест прекращения разбора коалесцированных пакетов после CONNECTION_CLOSE (RFC 9000 §10.2.2)
 *
 * @details Датаграмма несёт несколько пакетов: первый завершает соединение, второй
 *          содержит недопустимый на своём уровне фрейм. Разбор второго вернул бы
 *          соединение из завершённого состояния в состояние отправки собственного
 *          завершения, отправлять которое уже запрещено
 */
TEST_F(QuicFixture, ConnectionCloseCoalescedPacketTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Передаём первый флайт клиента серверу
	ASSERT_GT(::transfer(client, server, now), static_cast <size_t> (0));
	// Передаём ответный флайт сервера клиенту
	ASSERT_GT(::transfer(server, client, now), static_cast <size_t> (0));
	/**
	 * Проверяем что ключи уровня Handshake выведены на обоих эндпоинтах: пакеты
	 * этого уровня несут поле длины и коалесцируются, в отличие от пакетов 1-RTT
	 */
	ASSERT_NE(server.handshake().encryption(level_t::HANDSHAKE), nullptr);
	ASSERT_NE(client.handshake().decryption(level_t::HANDSHAKE), nullptr);
	// Проверяем что соединение клиента до приёма датаграммы не завершено
	ASSERT_NE(client.state(), connection_t::state_t::DRAINING);
	// Нагрузка первого пакета датаграммы
	std::string closing = "";
	// Выполняем сборку фрейма CONNECTION_CLOSE с кодом ошибки транспорта
	frame::serialize::connectionClose(closing, static_cast <uint64_t> (error_t::NO_ERROR), 0, "goodbye", false);
	/**
	 * Нагрузка второго пакета датаграммы: фрейм неизвестного типа недопустим
	 * на уровне Handshake и завершается ошибкой разбора (RFC 9000 §12.4)
	 */
	const std::string broken(1, static_cast <char> (0x3F));
	// Собираемая коалесцированная датаграмма сервера
	std::string datagram = "";
	// Собираем первый пакет датаграммы с завершением соединения
	ASSERT_TRUE(::build(server, level_t::HANDSHAKE, packet_t::HANDSHAKE, 1000, closing, datagram));
	// Запоминаем размер датаграммы с одним пакетом
	const size_t single = datagram.size();
	// Собираем второй пакет датаграммы с недопустимым фреймом
	ASSERT_TRUE(::build(server, level_t::HANDSHAKE, packet_t::HANDSHAKE, 1001, broken, datagram));
	// Проверяем что датаграмма действительно содержит два пакета
	ASSERT_GT(datagram.size(), single);
	// Передаём коалесцированную датаграмму клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что клиент остался в завершённом удалённым эндпоинтом состоянии
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Проверяем что причиной завершения осталась причина удалённого эндпоинта
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	// Буфер исходящей датаграммы клиента
	std::string response = "";
	// Проверяем что клиент в завершённом состоянии датаграмм не отправляет
	ASSERT_FALSE(client.write(response, now));
}

/**
 * @brief Тест освобождения потока после запроса прекращения передачи (RFC 9000 §3.5)
 *
 * @details Запрос прекращения отбрасывает неотправленные данные потока. Курсор
 *          упакованных данных при этом обязан сброситься вместе с буфером: иначе
 *          завершённость отправки не наступает никогда, и поток остаётся в списке
 *          до конца соединения
 */
TEST_F(QuicFixture, ConnectionStopSendingCollectTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Количество открываемых потоков сверх порога сборки завершённых потоков
	static constexpr size_t COUNT = 80;
	// Список открытых клиентом потоков
	std::vector <uint64_t> streams;
	/**
	 * Открываем потоки и отправляем в каждый данные: часть данных успевает
	 * упаковаться в пакеты, продвигая курсор упакованных данных буфера
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Открываем двунаправленный поток на клиенте
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Ставим данные потока в очередь отправки без завершения потока
		ASSERT_EQ(client.send(sid, std::string(4096, 'x'), false), status_t::OK);
		// Запоминаем открытый поток
		streams.push_back(sid);
	}
	// Выполняем обмен датаграммами - часть данных потоков уходит серверу
	::pump(client, server, now);
	// Проверяем что потоки клиентом обслуживаются
	ASSERT_EQ(client.streams(), COUNT);
	/**
	 * Запрашиваем прекращение передачи каждого потока на сервере: клиент в ответ
	 * аварийно завершает отправку и отбрасывает неотправленные данные
	 */
	for(auto & sid : streams){
		// Запрашиваем прекращение передачи потока удалённым эндпоинтом
		server.stop(sid, 0x0100);
		// Прекращаем приём данных потока на сервере
		server.reset(sid, 0x0100);
	}
	// Выполняем обмен датаграммами до полного затишья
	::pump(client, server, now);
	// Выполняем ещё один обмен - освобождение выполняется при сборке датаграмм
	::pump(client, server, now);
	/**
	 * Проверяем что завершённые потоки освобождены: оставленный ненулевым курсор
	 * упакованных данных удерживал бы каждый из них до конца соединения
	 */
	ASSERT_LT(client.streams(), COUNT);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест сохранения счётчиков маркировок при смене пути (RFC 9000 §13.4.2)
 *
 * @details Удалённый эндпоинт ведёт счётчики маркировок нарастающим итогом по
 *          пространству номеров за всё соединение, а не по пути. Обнуление
 *          локального учёта при смене пути сделало бы первый же присланный им
 *          счётчик недостоверно большим, и маркировка отключилась бы навсегда
 */
TEST_F(QuicFixture, ConnectionEcnMigrationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Включаем маркировку исходящих датаграмм клиента
	client.ecn(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем установление соединения по пути, доставляющему маркировку
	ASSERT_TRUE(::establish(client, server, now, nullptr, awh::event::ecn_t::ECT0));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "marked path payload", true), status_t::OK);
	/**
	 * Выполняем обмен датаграммами по пути, доставляющему маркировку: счётчики
	 * маркировок удалённого эндпоинта нарастают
	 */
	for(size_t i = 0; i < 4; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу с маркировкой поддержки ECN
		::transfer(client, server, now, nullptr, awh::event::ecn_t::ECT0);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
	}
	// Проверяем что проверка пути пройдена и маркировка сохранена
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	// Выполняем миграцию соединения клиента на новый путь
	ASSERT_TRUE(client.migrate());
	// Проверяем что маркировка непосредственно после смены пути сохранена
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	/**
	 * Выполняем обмен датаграммами по новому пути: удалённый эндпоинт присылает
	 * счётчики, накопленные за всё соединение, и обнулённый локальный учёт
	 * признал бы их недостоверными
	 */
	for(size_t i = 0; i < 6; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу с маркировкой поддержки ECN
		::transfer(client, server, now, nullptr, awh::event::ecn_t::ECT0);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
	}
	/**
	 * Проверяем что маркировка пережила смену пути: её снятие означало бы, что
	 * проверка провалилась на достоверных счётчиках удалённого эндпоинта
	 */
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест изоляции контроля перегрузки при смене пути (RFC 9000 §9.4)
 *
 * @details Окно перегрузки характеризует конкретный путь. Отправленное прежним
 *          путём не занимает ёмкости нового, иначе окно нового пути оказалось бы
 *          исчерпанным ещё до первой отправки - включая проверку его достижимости
 */
TEST_F(QuicFixture, ConnectionMigrateCongestionResetTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим объёмные данные потока в очередь отправки
	ASSERT_EQ(client.send(sid, std::string(65536, 'x'), false), status_t::OK);
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	/**
	 * Передаём датаграммы клиента серверу, не возвращая подтверждений: отправленные
	 * пакеты остаются неподтверждёнными и занимают окно перегрузки
	 */
	while(client.write(datagram, now)){
		// Продвигаем тестовые часы
		now += 1;
		// Передаём датаграмму серверу
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	}
	/**
	 * Проверяем что неподтверждённых октетов накопилось сверх начального окна:
	 * иначе смена пути прошла бы по пустому окну и дефекта не выявила
	 */
	ASSERT_GT(client.inflight(), client.cwnd());
	// Выполняем миграцию соединения клиента на новый путь
	ASSERT_TRUE(client.migrate());
	/**
	 * Проверяем что октеты в полёте списаны: отправленное прежним путём ёмкость
	 * нового не занимает
	 */
	ASSERT_EQ(client.inflight(), static_cast <uint64_t> (0));
	// Запоминаем окно перегрузки нового пути
	const uint64_t window = client.cwnd();
	// Очищаем буфер датаграммы от предыдущей сборки
	datagram.clear();
	/**
	 * Проверяем что проверка достижимости нового пути отправляется сразу: она
	 * собирается только при неисчерпанном окне перегрузки, а датаграмма с фреймом
	 * проверки дополняется до минимального размера (RFC 9000 §8.2.1)
	 */
	ASSERT_TRUE(client.write(datagram, now));
	ASSERT_GE(datagram.size(), static_cast <size_t> (proto::MIN_INITIAL_SIZE));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём накопленные сервером подтверждения клиенту
	::transfer(server, client, now);
	/**
	 * Проверяем что подтверждения пакетов прежнего пути окно нового не нарастили:
	 * ёмкость нового пути они не характеризуют
	 */
	ASSERT_EQ(client.cwnd(), window);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест дедлайна отложенного подтверждения приёма (RFC 9000 §13.2.1)
 *
 * @details Подтверждение откладывается не долее анонсированной задержки, поэтому
 *          поставленное в очередь подтверждение обязано отражаться дедлайном
 *          таймера: иначе вызывающий код, не собирающий датаграммы после каждого
 *          приёма, не отправил бы его вовсе
 */
TEST_F(QuicFixture, ConnectionAckDelayTimeoutTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Транспортные параметры клиента
	params::params_t params;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Извлекаем транспортные параметры удалённого эндпоинта
	ASSERT_EQ(server.peer(params, error), status_t::OK);
	// Нагрузка пакета сервера с ack-eliciting фреймом
	std::string payload = "";
	// Выполняем сборку фрейма PING (RFC 9000 §19.2)
	frame::serialize::ping(payload);
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1000, payload, now), status_t::OK);
	// Получаем дедлайн ближайшего события таймера клиента
	const uint64_t deadline = client.timeout();
	// Проверяем что дедлайн взведён
	ASSERT_GT(deadline, static_cast <uint64_t> (0));
	/**
	 * Проверяем что дедлайн не выходит за анонсированную клиентом задержку
	 * подтверждения: до исправления ближайшим событием оставался таймер PTO,
	 * до которого подтверждение пролежало бы на порядок дольше дозволенного
	 */
	ASSERT_LE(deadline, (now + params.maxAckDelay));
	// Проверяем отсутствие ошибки транспорта на клиенте
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест ответа на каждую принятую проверку пути (RFC 9000 §8.2.2)
 *
 * @details На каждый принятый фрейм PATH_CHALLENGE отправляется свой фрейм
 *          PATH_RESPONSE с его данными. Одного слота хранения недостаточно:
 *          вторая проверка, принятая до сборки ответа, затёрла бы первую
 */
TEST_F(QuicFixture, ConnectionPathResponseQueueTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Данные первой проверки достижимости пути
	const std::string first(proto::PATH_DATA_SIZE, 'A');
	// Данные второй проверки достижимости пути
	const std::string second(proto::PATH_DATA_SIZE, 'B');
	// Нагрузка пакета сервера
	std::string payload = "";
	// Выполняем сборку фрейма первой проверки достижимости пути
	frame::serialize::path(payload, frame_t::PATH_CHALLENGE, reinterpret_cast <const uint8_t *> (first.data()));
	// Выполняем сборку фрейма второй проверки достижимости пути
	frame::serialize::path(payload, frame_t::PATH_CHALLENGE, reinterpret_cast <const uint8_t *> (second.data()));
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1000, payload, now), status_t::OK);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Извлекаем датаграмму клиента с ответами на проверки пути
	ASSERT_TRUE(client.write(datagram, now));
	// Расшифрованная нагрузка пакета клиента
	std::string plain = "";
	// Выполняем снятие защиты с пакета клиента
	ASSERT_TRUE(::unseal(server, datagram, plain));
	/**
	 * Проверяем что ответ содержит данные обеих принятых проверок: утрата любой
	 * из них оставила бы удалённый эндпоинт без подтверждения своего пути
	 */
	ASSERT_NE(plain.find(first), std::string::npos);
	ASSERT_NE(plain.find(second), std::string::npos);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест дополнения датаграмм проверки пути до минимального размера (RFC 9000 §8.2.1/§8.2.2)
 *
 * @details Проверка достижимости подтверждает не сам факт доставки, а пригодность
 *          пути к переносу датаграмм минимального размера, поэтому датаграммы
 *          с фреймами проверки дополняются до него
 */
TEST_F(QuicFixture, ConnectionPathValidationPaddingTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Данные принятой проверки достижимости пути
	const std::string probe(proto::PATH_DATA_SIZE, 'A');
	// Нагрузка пакета сервера
	std::string payload = "";
	// Выполняем сборку фрейма проверки достижимости пути
	frame::serialize::path(payload, frame_t::PATH_CHALLENGE, reinterpret_cast <const uint8_t *> (probe.data()));
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1000, payload, now), status_t::OK);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Извлекаем датаграмму клиента с ответом на проверку пути
	ASSERT_TRUE(client.write(datagram, now));
	/**
	 * Проверяем что датаграмма с фреймом PATH_RESPONSE дополнена до минимального
	 * размера: короткая датаграмма проверила бы пригодность пути не к тому размеру
	 */
	ASSERT_GE(datagram.size(), static_cast <size_t> (proto::MIN_INITIAL_SIZE));
	// Выполняем миграцию соединения клиента на новый путь
	ASSERT_TRUE(client.migrate());
	// Очищаем буфер датаграммы от предыдущей сборки
	datagram.clear();
	// Извлекаем датаграмму клиента с проверкой достижимости нового пути
	ASSERT_TRUE(client.write(datagram, now));
	/**
	 * Проверяем что датаграмма с фреймом PATH_CHALLENGE дополнена до минимального
	 * размера: путь, не пропускающий такую датаграмму, проверку пройти не должен
	 */
	ASSERT_GE(datagram.size(), static_cast <size_t> (proto::MIN_INITIAL_SIZE));
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест расхождения CRYPTO-данных по одному смещению (RFC 9000 §2.2)
 *
 * @details Поток криптографического хендшейка переписыванию не подлежит: данные
 *          по одному смещению одни и те же, а расхождение означает неисправный
 *          либо злонамеренный удалённый эндпоинт
 */
TEST_F(QuicFixture, ConnectionCryptoOverlapMismatchTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Передаём первый флайт клиента серверу
	ASSERT_GT(::transfer(client, server, now), static_cast <size_t> (0));
	// Передаём ответный флайт сервера клиенту
	ASSERT_GT(::transfer(server, client, now), static_cast <size_t> (0));
	// Проверяем что ключи уровня Handshake выведены на обоих эндпоинтах
	ASSERT_NE(server.handshake().encryption(level_t::HANDSHAKE), nullptr);
	ASSERT_NE(client.handshake().decryption(level_t::HANDSHAKE), nullptr);
	/**
	 * Смещение CRYPTO-данных заведомо впереди собранного: данные остаются
	 * в буфере сборки, не попадая в хендшейк-машину
	 */
	static constexpr uint64_t OFFSET = 16384;
	// Нагрузка первого пакета сервера
	std::string payload = "";
	// Выполняем сборку фрейма CRYPTO с разрывом впереди собранного смещения
	frame::serialize::crypto(payload, OFFSET, "original");
	// Собираемая датаграмма сервера
	std::string datagram = "";
	// Собираем пакет уровня Handshake с CRYPTO-данными
	ASSERT_TRUE(::build(server, level_t::HANDSHAKE, packet_t::HANDSHAKE, 2000, payload, datagram));
	// Передаём датаграмму клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что соединение не разорвано
	ASSERT_NE(client.state(), connection_t::state_t::CLOSING);
	// Очищаем нагрузку от предыдущей сборки
	payload.clear();
	// Выполняем сборку повтора того же фрагмента с теми же данными
	frame::serialize::crypto(payload, OFFSET, "original");
	// Очищаем датаграмму от предыдущей сборки
	datagram.clear();
	// Собираем пакет уровня Handshake с повтором CRYPTO-данных
	ASSERT_TRUE(::build(server, level_t::HANDSHAKE, packet_t::HANDSHAKE, 2001, payload, datagram));
	// Передаём датаграмму клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что совпадающий повтор соединение не затрагивает
	ASSERT_NE(client.state(), connection_t::state_t::CLOSING);
	// Очищаем нагрузку от предыдущей сборки
	payload.clear();
	// Выполняем сборку фрейма с иными данными по тому же смещению
	frame::serialize::crypto(payload, OFFSET, "tampered");
	// Очищаем датаграмму от предыдущей сборки
	datagram.clear();
	// Собираем пакет уровня Handshake с расходящимися CRYPTO-данными
	ASSERT_TRUE(::build(server, level_t::HANDSHAKE, packet_t::HANDSHAKE, 2002, payload, datagram));
	// Передаём датаграмму клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::ERROR);
	// Проверяем что соединение завершено нарушением протокола
	ASSERT_EQ(client.error(), error_t::PROTOCOL_VIOLATION);
}

/**
 * @brief Тест предела лимита числа потоков во фрейме MAX_STREAMS (RFC 9000 §19.11)
 *
 * @details Идентификаторы потоков кодируются varint, поэтому лимит сверх 2^60
 *          разрешал бы открытие потока с некодируемым идентификатором
 */
TEST_F(QuicFixture, ConnectionMaxStreamsBoundTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Предельный кодируемый лимит числа потоков
	static constexpr uint64_t LIMIT = (static_cast <uint64_t> (1) << 60);
	// Нагрузка пакета сервера
	std::string payload = "";
	// Выполняем сборку фрейма MAX_STREAMS с предельно допустимым лимитом
	frame::serialize::single(payload, frame_t::MAX_STREAMS_BIDI, LIMIT);
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1000, payload, now), status_t::OK);
	// Проверяем что предельно допустимый лимит соединение не разрывает
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Очищаем нагрузку от предыдущей сборки
	payload.clear();
	// Выполняем сборку фрейма MAX_STREAMS с лимитом сверх предела
	frame::serialize::single(payload, frame_t::MAX_STREAMS_BIDI, LIMIT + 1);
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1001, payload, now), status_t::ERROR);
	// Проверяем что соединение завершено ошибкой кодирования фрейма
	ASSERT_EQ(client.error(), error_t::FRAME_ENCODING_ERROR);
}

/**
 * @brief Тест расхождения данных потока по одному смещению (RFC 9000 §2.2)
 *
 * @details Октет потока по своему смещению один и тот же сколько бы раз он ни был
 *          прислан. Расхождение означает неисправный либо злонамеренный удалённый
 *          эндпоинт, а собранные данные без сверки зависели бы от порядка приёма
 */
TEST_F(QuicFixture, ConnectionStreamOverlapMismatchTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Идентификатор двунаправленного потока, инициируемого сервером
	static constexpr uint64_t SID = 0x01;
	// Нагрузка первого пакета сервера
	std::string payload = "";
	/**
	 * Выполняем сборку фрейма STREAM с разрывом в начале потока: нулевое смещение
	 * не заполнено, поэтому данные остаются в буфере сборки
	 */
	frame::serialize::stream(payload, SID, 16, "original", false);
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1000, payload, now), status_t::OK);
	// Проверяем что соединение не разорвано
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Очищаем нагрузку от предыдущей сборки
	payload.clear();
	// Выполняем сборку повтора того же фрагмента с теми же данными
	frame::serialize::stream(payload, SID, 16, "original", false);
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1001, payload, now), status_t::OK);
	// Проверяем что совпадающий повтор соединение не затрагивает
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Очищаем нагрузку от предыдущей сборки
	payload.clear();
	// Выполняем сборку фрейма с иными данными по тому же смещению
	frame::serialize::stream(payload, SID, 16, "tampered", false);
	// Доставляем нагрузку клиенту пакетом 1-RTT
	ASSERT_EQ(::inject(server, client, 1002, payload, now), status_t::ERROR);
	// Проверяем что соединение завершено нарушением протокола
	ASSERT_EQ(client.error(), error_t::PROTOCOL_VIOLATION);
	// Проверяем состояние завершения соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
}

/**
 * @brief Тест предела дробления данных потока при сборке (RFC 9000 §4.1)
 *
 * @details Лимит приёма ограничивает объём данных потока, но не их дробление:
 *          хранение каждого фрагмента обходится многократно дороже несомых им
 *          данных, поэтому число буферизируемых фрагментов ограничено отдельно
 */
TEST_F(QuicFixture, ConnectionStreamFragmentLimitTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Идентификатор двунаправленного потока, инициируемого сервером
	static constexpr uint64_t SID = 0x01;
	// Номер очередного отправляемого пакета
	uint64_t pn = 1000;
	// Смещение очередного однооктетного фрагмента данных потока
	uint64_t offset = 1;
	/**
	 * Доставляем умеренное дробление: нулевое смещение не заполняется, поэтому все
	 * фрагменты остаются в сборке. Такое количество разрывов образует и обычная
	 * перестановка пакетов в сети, поэтому соединение обязано его пережить
	 */
	for(size_t i = 0; i < 100; i++){
		// Собираемая нагрузка пакета сервера
		std::string payload = "";
		// Выполняем сборку фрейма STREAM с однооктетными данными
		frame::serialize::stream(payload, SID, offset, "x", false);
		// Продвигаем смещение через разрыв
		offset += 2;
		// Доставляем нагрузку клиенту пакетом 1-RTT
		ASSERT_EQ(::inject(server, client, pn++, payload, now), status_t::OK);
	}
	// Проверяем что соединение умеренным дроблением не разорвано
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на клиенте
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	// Результат обработки нагрузки клиентом
	status_t status = status_t::OK;
	/**
	 * Продолжаем дробление до срабатывания предела: без него удалённый эндпоинт
	 * занимал бы память многократно сверх анонсированного лимита приёма
	 */
	for(size_t i = 0; (i < 4096) && (status == status_t::OK); i++){
		// Собираемая нагрузка пакета сервера
		std::string payload = "";
		// Выполняем сборку фрейма STREAM с однооктетными данными
		frame::serialize::stream(payload, SID, offset, "x", false);
		// Продвигаем смещение через разрыв
		offset += 2;
		// Доставляем нагрузку клиенту пакетом 1-RTT
		status = ::inject(server, client, pn++, payload, now);
	}
	// Проверяем что предел дробления сработал
	ASSERT_EQ(status, status_t::ERROR);
	// Проверяем что соединение завершено ошибкой flow control
	ASSERT_EQ(client.error(), error_t::FLOW_CONTROL_ERROR);
	// Проверяем состояние завершения соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
}

/**
 * @brief Тест перезапуска отсчёта таймаута простоя отправкой (RFC 9000 §10.1)
 *
 * @details Отсчёт перезапускается не только приёмом пакета, но и отправкой
 *          ack-eliciting пакета после долгой паузы: иначе начатая перед самым
 *          истечением таймаута активность оборвалась бы, не дождавшись ответа
 */
TEST_F(QuicFixture, ConnectionIdleRestartOnSendTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Транспортные параметры эндпоинтов
	params::params_t params;
	// Устанавливаем таймаут простоя соединения в миллисекундах
	params.maxIdleTimeout = 30000;
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
	// Выполняем подготовку соединения клиента
	::configure(client, params);
	// Выполняем подготовку соединения сервера
	::configure(server, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Запоминаем дедлайн таймаута простоя, отсчитанный от последнего приёма
	const uint64_t deadline = (now + params.maxIdleTimeout);
	// Продвигаем тестовые часы к самому концу периода простоя
	now = (deadline - 100);
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные потока в очередь отправки
	ASSERT_EQ(client.send(sid, "late activity", true), status_t::OK);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Извлекаем датаграмму клиента с данными потока
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы за прежний дедлайн таймаута простоя
	now = (deadline + 100);
	// Выполняем обработку просроченных таймеров клиента
	client.tick(now);
	/**
	 * Проверяем что соединение живо: отсчёт перезапущен отправкой, и ответ
	 * удалённого эндпоинта ещё имеет шанс прийти
	 */
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Продвигаем тестовые часы за дедлайн, отсчитанный от отправки
	now = (deadline + params.maxIdleTimeout);
	// Выполняем обработку просроченных таймеров клиента
	client.tick(now);
	/**
	 * Проверяем что соединение всё же завершено по простою: перезапуск отправкой
	 * однократен, и молчащий удалённый эндпоинт соединение не удерживает
	 */
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
}

/**
 * @brief Тест выдержки периода завершения соединения (RFC 9000 §10.2)
 *
 */
TEST_F(QuicFixture, ConnectionClosingPeriodTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Выполняем завершение соединения приложением на клиенте
	client.close(0x0100, "goodbye");
	// Буфер датаграммы завершения соединения
	std::string datagram = "";
	// Извлекаем датаграмму завершения соединения
	ASSERT_TRUE(client.write(datagram, now));
	// Проверяем состояние выдержки периода завершения соединения
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
	/**
	 * Проверяем что дедлайн периода завершения взведён: без него вызывающий код
	 * не узнает, когда состояние соединения подлежит освобождению, и удерживал бы
	 * его до конца работы процесса
	 */
	const uint64_t deadline = client.timeout();
	// Проверяем что дедлайн периода завершения наступает позже текущего времени
	ASSERT_GT(deadline, now);
	// Продвигаем часы до момента перед истечением периода завершения
	now = (deadline - 1);
	// Выполняем обработку просроченных таймеров соединения
	client.tick(now);
	// Проверяем что период завершения соединения ещё выдерживается
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
	// Продвигаем часы за момент истечения периода завершения
	now = deadline;
	// Выполняем обработку просроченных таймеров соединения
	client.tick(now);
	// Проверяем что соединение перешло в завершённое состояние
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Проверяем что завершённое соединение таймера более не требует
	ASSERT_EQ(client.timeout(), static_cast <uint64_t> (0));
	// Проверяем что завершённое соединение датаграмм не отправляет
	ASSERT_FALSE(client.write(datagram, now));
}

/**
 * @brief Тест отмены поставленного в очередь завершения приёмом сброса (RFC 9000 §10.3)
 *
 * @details Сброс без сохранения состояния приходит между постановкой завершения
 *          соединения в очередь и его отправкой: отправлять что-либо после приёма
 *          сброса эндпоинт не вправе, поэтому фрейм CONNECTION_CLOSE из очереди
 *          отправлен быть уже не может
 */
TEST_F(QuicFixture, ConnectionCloseAbortedByResetTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Транспортные параметры эндпоинтов
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
	// Выполняем подготовку соединения клиента
	::configure(client, params);
	// Устанавливаем наличие токена сброса без сохранения состояния (RFC 9000 §18.2)
	params.hasResetToken = true;
	/**
	 * Заполняем токен сброса известным значением: клиент получит его транспортным
	 * параметром сервера и обязан опознать по нему сброс
	 */
	for(size_t i = 0; i < proto::RESET_TOKEN_SIZE; i++)
		// Записываем очередной октет токена сброса
		params.resetToken[i] = static_cast <uint8_t> (0xA0 + i);
	// Выполняем подготовку соединения сервера
	::configure(server, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Выполняем завершение соединения приложением на клиенте
	client.close(0x0100, "goodbye");
	// Проверяем состояние выдержки периода завершения соединения
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
	/**
	 * Формируем датаграмму сброса: короткий заголовок со случайным содержимым
	 * и токен сброса в последних 16 октетах (RFC 9000 §10.3)
	 */
	std::string reset(40, '\0');
	// Устанавливаем первый октет короткого заголовка с обязательным fixed-битом
	reset[0] = static_cast <char> (0x40);
	/**
	 * Заполняем непрозрачную часть датаграммы произвольными данными
	 */
	for(size_t i = 1; i < (reset.size() - proto::RESET_TOKEN_SIZE); i++)
		// Записываем очередной октет непрозрачной части
		reset[i] = static_cast <char> (0x5A + i);
	/**
	 * Дописываем токен сброса в хвост датаграммы
	 */
	for(size_t i = 0; i < proto::RESET_TOKEN_SIZE; i++)
		// Записываем очередной октет токена сброса
		reset[reset.size() - proto::RESET_TOKEN_SIZE + i] = static_cast <char> (0xA0 + i);
	// Передаём датаграмму сброса клиенту до отправки им фрейма CONNECTION_CLOSE
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (reset.data()), reset.size(), now), status_t::OK);
	// Проверяем что клиент перешёл в завершённое состояние
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Проверяем что поставленное в очередь завершение соединения не отправляется
	ASSERT_FALSE(client.write(datagram, now));
}

/**
 * @brief Тест повторного приёма датаграммы (защита от дубликатов)
 *
 */
TEST_F(QuicFixture, ConnectionDuplicateTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Проверяем что сервер не может начать соединение методом connect()
	ASSERT_EQ(server.connect(), status_t::ERROR);
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Список потоков с данными на сервере
	std::vector <uint64_t> readable;
	// Получаем список потоков с данными на сервере
	server.readable(readable);
	// Проверяем список потоков с данными на сервере
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	std::vector <uint64_t> readable;
	// Получаем список потоков с данными на сервере
	server.readable(readable);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединений с ограниченными параметрами
	::configure(client, params);
	::configure(server, params);
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
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединений с ограниченными параметрами
	::configure(client, params);
	::configure(server, params);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
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

/**
 * @brief Тест завершения соединения по таймауту простоя (RFC 9000 §10.1)
 *
 */
TEST_F(QuicFixture, ConnectionIdleTimeoutTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Транспортные параметры эндпоинтов
	params::params_t params;
	// Устанавливаем таймаут простоя соединения в миллисекундах
	params.maxIdleTimeout = 5000;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initialMaxStreamsBidi = 100;
	// Выполняем подготовку соединения клиента
	::configure(client, params);
	// Выполняем подготовку соединения сервера
	::configure(server, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Получаем дедлайн ближайшего события таймера сервера
	const uint64_t deadline = server.timeout();
	// Проверяем что таймаут простоя взведён
	ASSERT_GT(deadline, now);
	// Проверяем что дедлайн не превышает согласованный таймаут простоя
	ASSERT_LE(deadline, now + 5000);
	// Обрабатываем таймеры сервера до наступления дедлайна
	server.tick(deadline - 1);
	// Проверяем что соединение сервера ещё активно
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Обрабатываем таймеры сервера после наступления дедлайна
	server.tick(deadline);
	// Проверяем что соединение сервера завершено молча (RFC 9000 §10.1)
	ASSERT_EQ(server.state(), connection_t::state_t::DRAINING);
	// Обрабатываем таймеры клиента после наступления его дедлайна
	client.tick(client.timeout());
	// Проверяем что соединение клиента завершено молча
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
}

/**
 * @brief Тест повторной отправки CONNECTION_CLOSE при потере (RFC 9000 §10.2.1)
 *
 */
TEST_F(QuicFixture, ConnectionCloseRetransmitTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Выполняем завершение соединения клиентом
	client.close(0x00, "goodbye");
	// Буфер исходящей датаграммы
	std::string datagram = "";
	// Извлекаем датаграмму с фреймом CONNECTION_CLOSE и теряем её (не передаём серверу)
	ASSERT_TRUE(client.write(datagram, now));
	// Проверяем что повторная датаграмма не собирается без принятых пакетов
	ASSERT_FALSE(client.write(datagram, now));
	// Открываем двунаправленный поток на сервере (сервер не знает о завершении)
	const uint64_t sid = server.open(false);
	// Ставим данные сервера в очередь отправки
	ASSERT_EQ(server.send(sid, "server data", false), status_t::OK);
	// Извлекаем датаграмму сервера с данными потока
	ASSERT_TRUE(server.write(datagram, now));
	// Передаём датаграмму сервера завершающемуся клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Извлекаем повторную датаграмму с фреймом CONNECTION_CLOSE (RFC 9000 §10.2.1)
	ASSERT_TRUE(client.write(datagram, now));
	// Передаём повторную датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что сервер принял завершение соединения
	ASSERT_EQ(server.state(), connection_t::state_t::DRAINING);
}

/**
 * @brief Тест согласования версии протокола (RFC 9000 §6)
 *
 */
TEST_F(QuicFixture, VersionNegotiationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial
	ASSERT_TRUE(client.write(datagram, now));
	// Подменяем версию пакета на неподдерживаемую (октеты 1-4 длинного заголовка)
	datagram[1] = static_cast <char> (0x0A);
	datagram[2] = static_cast <char> (0x0A);
	datagram[3] = static_cast <char> (0x0A);
	datagram[4] = static_cast <char> (0x0A);
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Буфер датаграммы сервера
	std::string response = "";
	// Извлекаем датаграмму сервера с пакетом Version Negotiation
	ASSERT_TRUE(server.write(response, now));
	// Проверяем что версия пакета сервера - Version Negotiation (октеты 1-4 нулевые)
	ASSERT_EQ(static_cast <uint8_t> (response[1]), 0x00);
	ASSERT_EQ(static_cast <uint8_t> (response[2]), 0x00);
	ASSERT_EQ(static_cast <uint8_t> (response[3]), 0x00);
	ASSERT_EQ(static_cast <uint8_t> (response[4]), 0x00);
	// Проверяем что установлен бит длинного заголовка
	ASSERT_NE(static_cast <uint8_t> (response[0]) & 0x80, 0);
	/**
	 * Формируем синтетический пакет Version Negotiation без поддерживаемых клиентом версий:
	 * идентификаторы соединения соответствуют отправленным клиентом (RFC 9000 §6.2)
	 */
	std::string forged = "";
	// Дописываем первый октет (длинный заголовок с произвольными младшими битами)
	forged.push_back(static_cast <char> (0xC0));
	// Дописываем версию Version Negotiation (нулевую)
	forged.append(4, '\0');
	// Дописываем длину идентификатора соединения получателя (SCID клиента)
	forged.push_back(static_cast <char> (client.scid().size));
	// Дописываем данные идентификатора соединения получателя
	forged.append(reinterpret_cast <const char *> (client.scid().data), client.scid().size);
	// Дописываем длину идентификатора соединения отправителя (DCID клиента)
	forged.push_back(static_cast <char> (client.dcid().size));
	// Дописываем данные идентификатора соединения отправителя
	forged.append(reinterpret_cast <const char *> (client.dcid().data), client.dcid().size);
	// Дописываем единственную неподдерживаемую версию (RFC 9000 §6.2)
	forged.append("\x0A\x0A\x0A\x0A", 4);
	// Продвигаем тестовые часы
	now += 5;
	// Передаём синтетический пакет Version Negotiation клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (forged.data()), forged.size(), now), status_t::OK);
	// Проверяем что клиент завершил соединение из-за отсутствия общей версии (RFC 9000 §6.2)
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Проверяем код ошибки согласования версии
	ASSERT_EQ(client.error(), error_t::VERSION_NEGOTIATION_ERROR);
}

/**
 * @brief Тест проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
 *
 */
TEST_F(QuicFixture, RetryTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Включаем проверку адреса клиента через пакет Retry
	server.retry(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial без токена
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что сервер не начал соединение (ожидает токен)
	ASSERT_EQ(server.state(), connection_t::state_t::NONE);
	// Буфер датаграммы сервера
	std::string retry = "";
	// Извлекаем датаграмму сервера с пакетом Retry
	ASSERT_TRUE(server.write(retry, now));
	// Проверяем что тип пакета сервера - Retry (биты 4-5 первого октета)
	ASSERT_EQ((static_cast <uint8_t> (retry[0]) & 0x30) >> 4, 0x03);
	// Продвигаем тестовые часы
	now += 5;
	// Передаём пакет Retry клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (retry.data()), retry.size(), now), status_t::OK);
	// Выполняем полное установление соединения после обработки Retry
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения обоих эндпоинтов
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест обновления ключей уровня приложения (RFC 9001 §6)
 *
 */
TEST_F(QuicFixture, KeyUpdateTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Ставим первые данные в очередь отправки
	ASSERT_EQ(client.send(sid, "phase zero", false), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Запоминаем бит фазы ключей клиента до обновления
	const bool phase = client.phase();
	// Выполняем обновление ключей клиентом (RFC 9001 §6)
	ASSERT_EQ(client.rekey(), status_t::OK);
	// Проверяем что бит фазы ключей переключился
	ASSERT_NE(client.phase(), phase);
	// Ставим вторые данные в очередь отправки в новой фазе ключей
	ASSERT_EQ(client.send(sid, " phase one", false), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Проверяем что сервер переключился на новую фазу ключей
	ASSERT_EQ(server.phase(), client.phase());
	// Принятые данные потока
	std::string received = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём собранные данные потока на сервере
	ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	// Проверяем данные обеих фаз ключей
	ASSERT_EQ(received, "phase zero phase one");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест ротации идентификатора соединения (RFC 9000 §5.1.1)
 *
 */
TEST_F(QuicFixture, ConnectionIdRotationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Запоминаем идентификатор соединения удалённого эндпоинта до ротации
	const cid_t before = client.dcid();
	// Выполняем ротацию идентификатора соединения удалённого эндпоинта
	ASSERT_TRUE(client.rotate());
	// Запоминаем идентификатор соединения удалённого эндпоинта после ротации
	const cid_t after = client.dcid();
	// Проверяем что идентификатор соединения изменился
	ASSERT_FALSE(before == after);
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Ставим данные в очередь отправки с новым идентификатором соединения
	ASSERT_EQ(client.send(sid, "rotated cid", false), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Принятые данные потока
	std::string received = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём собранные данные потока на сервере
	ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	// Проверяем данные потока после ротации идентификатора
	ASSERT_EQ(received, "rotated cid");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест инициализации окна перегрузки congestion control (RFC 9002 §7.2)
 *
 */
TEST_F(QuicFixture, CongestionControlTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Проверяем начальное окно перегрузки до отправки (RFC 9002 §7.2)
	ASSERT_EQ(client.cwnd(), 12000);
	// Проверяем отсутствие октетов в полёте до отправки
	ASSERT_EQ(client.inflight(), 0);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "congestion window data", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Проиграем тестовые часы для подтверждения всех пакетов
	::pump(client, server, now);
	// Проверяем что подтверждённые данные списаны из полёта
	ASSERT_EQ(client.inflight(), 0);
	// Проверяем что окно перегрузки выросло в замедленном старте (RFC 9002 §7.3.1)
	ASSERT_GT(client.cwnd(), 12000);
}

/**
 * @brief Тест соблюдения лимита анти-амплификации сервером (RFC 9000 §8.1)
 *
 * @details До подтверждения адреса клиента сервер не вправе отправить более
 *          трёхкратного объёма принятых от него октетов. Проверка выполняется
 *          на повторных срабатываниях таймера PTO без ответа клиента
 */
TEST_F(QuicFixture, AntiAmplificationLimitTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial
	ASSERT_TRUE(client.write(datagram, now));
	// Запоминаем объём принятых сервером октетов
	const size_t received = datagram.size();
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Суммарный объём отправленных сервером октетов
	size_t sent = 0;
	// Буфер исходящей датаграммы сервера
	std::string outgoing = "";
	/**
	 * Вычерпываем сервер до упора, не отвечая ему: ответа клиента нет, поэтому
	 * адрес не подтверждается и лимит анти-амплификации остаётся в силе
	 */
	for(size_t i = 0; i < 64; i++){
		/**
		 * Извлекаем все готовые датаграммы сервера
		 */
		while(server.write(outgoing, now)){
			// Проверяем что датаграмма не превышает предельный размер пути
			ASSERT_LE(outgoing.size(), connection_t::MAX_DATAGRAM_SIZE);
			// Суммируем объём отправленных октетов
			sent += outgoing.size();
			// Проверяем соблюдение трёхкратного лимита на каждом шаге (RFC 9000 §8.1)
			ASSERT_LE(sent, received * 3);
		}
		// Продвигаем тестовые часы за дедлайн ближайшего таймера
		now += 1000;
		// Обрабатываем просроченные таймеры сервера (срабатывание PTO)
		server.tick(now);
	}
	// Проверяем что сервер вообще отправлял данные
	ASSERT_GT(sent, 0u);
	// Проверяем что лимит анти-амплификации действительно был достигнут
	ASSERT_GT(sent, received);
}

/**
 * @brief Тест снятия лимита анти-амплификации по данным от клиента (RFC 9000 §8.1)
 *
 * @details Исчерпав лимит, сервер обязан замолчать до прихода новых октетов
 *          от клиента и возобновить отправку после их получения
 */
TEST_F(QuicFixture, AntiAmplificationResumeTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Буфер исходящей датаграммы сервера
	std::string outgoing = "";
	/**
	 * Вычерпываем сервер до исчерпания лимита анти-амплификации
	 */
	for(size_t i = 0; i < 64; i++){
		// Извлекаем очередные датаграммы сервера без ответа клиенту
		while(server.write(outgoing, now));
		// Продвигаем тестовые часы за дедлайн ближайшего таймера
		now += 1000;
		// Обрабатываем просроченные таймеры сервера
		server.tick(now);
	}
	// Проверяем что сервер замолчал по исчерпанию лимита
	ASSERT_FALSE(server.write(outgoing, now));
	// Передаём серверу повторно первую датаграмму клиента - объём принятого растёт
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что отправка возобновилась после роста объёма принятых октетов
	ASSERT_TRUE(server.write(outgoing, now));
}

/**
 * @brief Тест соблюдения предельного размера датаграммы на всём обмене (RFC 9000 §14.1)
 *
 * @details Проверяются оба пути расчёта бюджета: сборка нагрузки уровня и
 *          дополнение датаграммы с пакетом Initial. Режим Retry включён, чтобы
 *          заголовок пакета Initial содержал токен проверки адреса
 */
TEST_F(QuicFixture, DatagramSizeBudgetTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Включаем проверку адреса клиента через пакет Retry
	server.retry(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	/**
	 * Выполняем обмен датаграммами вручную с проверкой размера каждой
	 */
	for(size_t i = 0; i < 32; i++){
		// Количество переданных на шаге датаграмм
		size_t moved = 0;
		/**
		 * Передаём датаграммы клиента серверу
		 */
		while(client.write(datagram, now)){
			/**
			 * Проверяем что датаграмма не превышает предельный размер зонда пути:
			 * зондирование наращивает размер сверх обязательного минимума, а сам
			 * зонд заведомо больше подтверждённого размера
			 */
			ASSERT_LE(datagram.size(), connection_t::MAX_PROBE_SIZE);
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграмму серверу
			ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			// Считаем переданную датаграмму
			moved++;
		}
		/**
		 * Передаём датаграммы сервера клиенту
		 */
		while(server.write(datagram, now)){
			// Проверяем что датаграмма не превышает предельный размер зонда пути
			ASSERT_LE(datagram.size(), connection_t::MAX_PROBE_SIZE);
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграмму клиенту
			ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			// Считаем переданную датаграмму
			moved++;
		}
		// Если соединение установлено и обмен завершён
		if((moved == 0) && (client.state() == connection_t::state_t::CONNECTED))
			// Прекращаем обмен датаграммами
			break;
	}
	// Проверяем что соединение установлено через пакет Retry
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим крупный блок данных в очередь отправки для наполнения датаграмм
	ASSERT_EQ(client.send(sid, std::string(200000, 'x'), true), status_t::OK);
	// Буфер принятых сервером данных потока
	std::string received = "";
	// Флаг принятого завершения потока
	bool finished = false;
	/**
	 * Выполняем передачу данных с проверкой размера каждой датаграммы
	 */
	for(size_t i = 0; i < 200; i++){
		// Количество переданных на шаге датаграмм
		size_t moved = 0;
		/**
		 * Передаём датаграммы клиента серверу
		 */
		while(client.write(datagram, now)){
			// Проверяем что датаграмма не превышает предельный размер зонда пути
			ASSERT_LE(datagram.size(), connection_t::MAX_PROBE_SIZE);
			// Продвигаем тестовые часы
			now += 1;
			// Передаём датаграмму серверу
			ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			// Считаем переданную датаграмму
			moved++;
		}
		// Флаг завершения потока на текущем шаге
		bool fin = false;
		// Выдаём принятые данные приложению для продвижения лимитов приёма
		server.receive(sid, received, fin);
		// Накапливаем признак принятого завершения потока
		finished = (finished || fin);
		/**
		 * Передаём датаграммы сервера клиенту
		 */
		while(server.write(datagram, now)){
			// Проверяем что датаграмма не превышает предельный размер зонда пути
			ASSERT_LE(datagram.size(), connection_t::MAX_PROBE_SIZE);
			// Продвигаем тестовые часы
			now += 1;
			// Передаём датаграмму клиенту
			ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			// Считаем переданную датаграмму
			moved++;
		}
		// Если обмен датаграммами завершён
		if(moved == 0)
			// Прекращаем передачу данных
			break;
	}
	// Проверяем что данные потока переданы полностью и без искажений
	ASSERT_EQ(received.size(), 200000u);
	ASSERT_EQ(received, std::string(200000, 'x'));
	// Проверяем что завершение потока доставлено приложению
	ASSERT_TRUE(finished);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест возврата кредита лимита потоков при прекращении приёма до завершения потока
 *
 * @details Приложение прекращает приём через stop(), после чего приходит завершение
 *          потока. Данные приложению не выдаются, поэтому кредит MAX_STREAMS обязан
 *          вернуться на самом приёме завершения. Ответный фрейм RESET_STREAM пира
 *          в этом тесте намеренно не доставляется: он вернул бы кредит по другому
 *          пути и замаскировал дефект, а по RFC 9000 §3.5 пир в состоянии Data Recvd
 *          отвечать на STOP_SENDING не обязан
 */
TEST_F(QuicFixture, StreamStopSendingBeforeFinCreditTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Транспортные параметры с лимитом в один двунаправленный поток
	params::params_t params;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит в один двунаправленный поток
	params.initialMaxStreamsBidi = 1;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initialMaxStreamsUni = 1;
	// Выполняем подготовку соединения клиента
	::configure(client, params);
	// Выполняем подготовку соединения сервера
	::configure(server, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем единственный разрешённый лимитом поток
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Проверяем что лимит потоков исчерпан
	ASSERT_EQ(client.open(false), connection_t::INVALID_STREAM);
	// Ставим первую часть данных без завершения потока
	ASSERT_EQ(client.send(sid, "first part", false), status_t::OK);
	// Передаём данные серверу
	ASSERT_GT(::transfer(client, server, now), 0u);
	// Прекращаем приём потока на сервере до прихода завершения потока
	server.stop(sid, 0x01);
	// Ставим вторую часть данных с завершением потока
	ASSERT_EQ(client.send(sid, "second part", true), status_t::OK);
	/**
	 * Передаём завершение потока серверу, не доставляя клиенту встречных датаграмм:
	 * клиент не узнаёт о прекращении приёма и ответный RESET_STREAM не отправляет
	 */
	ASSERT_GT(::transfer(client, server, now), 0u);
	// Доставляем клиенту датаграммы сервера с обновлённым лимитом MAX_STREAMS
	::transfer(server, client, now);
	/**
	 * Кредит завершённого потока возвращён на приёме завершения, поэтому клиент
	 * вправе открыть следующий поток сверх начального лимита в один поток
	 */
	ASSERT_NE(client.open(false), connection_t::INVALID_STREAM);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест возврата кредита лимита потоков при прекращении приёма после завершения потока
 *
 * @details Завершение потока принято, но данные приложению не выданы, после чего
 *          приложение прекращает приём через stop(). Кредит MAX_STREAMS обязан
 *          вернуться на самом вызове stop(), не дожидаясь выдачи данных
 */
TEST_F(QuicFixture, StreamStopSendingAfterFinCreditTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Транспортные параметры с лимитом в один двунаправленный поток
	params::params_t params;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит в один двунаправленный поток
	params.initialMaxStreamsBidi = 1;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initialMaxStreamsUni = 1;
	// Выполняем подготовку соединения клиента
	::configure(client, params);
	// Выполняем подготовку соединения сервера
	::configure(server, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем единственный разрешённый лимитом поток
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные с завершением потока в очередь отправки
	ASSERT_EQ(client.send(sid, "payload with fin", true), status_t::OK);
	/**
	 * Передаём данные серверу, не доставляя клиенту встречных датаграмм:
	 * ответный RESET_STREAM клиента не должен маскировать возврат кредита
	 */
	ASSERT_GT(::transfer(client, server, now), 0u);
	// Прекращаем приём потока на сервере без выдачи данных приложению
	server.stop(sid, 0x01);
	// Доставляем клиенту датаграммы сервера с обновлённым лимитом MAX_STREAMS
	::transfer(server, client, now);
	/**
	 * Кредит завершённого потока возвращён на вызове stop(), поэтому клиент
	 * вправе открыть следующий поток сверх начального лимита в один поток
	 */
	ASSERT_NE(client.open(false), connection_t::INVALID_STREAM);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест кругового обслуживания потоков при упаковке данных (RFC 9000 §2.3)
 *
 * @details Все потоки с данными обязаны получать эфир. Обход списка потоков
 *          с начала отдавал бы датаграммы потоку с наименьшим идентификатором,
 *          а остальные простаивали бы до полной его передачи
 */
TEST_F(QuicFixture, StreamRoundRobinTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Список идентификаторов открытых потоков
	std::vector <uint64_t> streams;
	/**
	 * Открываем несколько потоков и наполняем каждый крупным блоком данных
	 */
	for(size_t i = 0; i < 4; i++){
		// Открываем двунаправленный поток на клиенте
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Ставим крупный блок данных в очередь отправки
		ASSERT_EQ(client.send(sid, std::string(40000, 'a'), false), status_t::OK);
		// Сохраняем идентификатор потока
		streams.push_back(sid);
	}
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	/**
	 * Передаём ограниченное количество датаграмм: полной передачи всех потоков
	 * не происходит, поэтому виден порядок их обслуживания
	 */
	for(size_t i = 0; i < 8; i++){
		// Если исходящих датаграмм у клиента не осталось
		if(!client.write(datagram, now))
			// Прекращаем передачу
			break;
		// Продвигаем тестовые часы
		now += 1;
		// Передаём датаграмму серверу
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	}
	// Количество потоков, получивших данные
	size_t served = 0;
	/**
	 * Перебираем список открытых потоков
	 */
	for(auto & sid : streams){
		// Буфер принятых сервером данных
		std::string payload = "";
		// Флаг завершения потока
		bool fin = false;
		// Выдаём принятые данные приложению
		server.receive(sid, payload, fin);
		// Если поток получил эфир
		if(!payload.empty())
			// Считаем обслуженный поток
			served++;
	}
	// Проверяем что эфир получил не только поток с наименьшим идентификатором
	ASSERT_GT(served, 1u);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест сборки завершённых потоков приложения
 *
 * @details Сборка запускается при превышении порога числа потоков и обязана
 *          удалять только потоки, на которые не ссылаются очередь ретрансмиссии
 *          и учётные записи неподтверждённых пакетов
 */
TEST_F(QuicFixture, StreamCollectTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Полностью проводим через соединение количество потоков выше порога сборки
	 */
	for(size_t i = 0; i < 80; i++){
		// Открываем двунаправленный поток на клиенте
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Ставим данные с завершением потока в очередь отправки
		ASSERT_EQ(client.send(sid, "stream payload", true), status_t::OK);
		// Выполняем обмен датаграммами
		::pump(client, server, now);
		// Буфер принятых сервером данных
		std::string payload = "";
		// Флаг завершения потока
		bool fin = false;
		// Выдаём принятые данные приложению - приём потока завершается
		ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
		// Проверяем содержимое принятых данных
		ASSERT_EQ(payload, "stream payload");
		// Проверяем принятое завершение потока
		ASSERT_TRUE(fin);
		// Завершаем отправку встречного направления потока на сервере
		ASSERT_EQ(server.send(sid, "reply payload", true), status_t::OK);
		// Выполняем обмен датаграммами
		::pump(client, server, now);
		// Буфер принятых клиентом данных
		std::string reply = "";
		// Флаг завершения встречного направления потока
		bool replyFin = false;
		// Выдаём принятые данные приложению
		ASSERT_EQ(client.receive(sid, reply, replyFin), status_t::OK);
		// Проверяем содержимое принятых данных
		ASSERT_EQ(reply, "reply payload");
	}
	// Выполняем обмен датаграммами для подтверждения последних пакетов
	::pump(client, server, now);
	/**
	 * Соединение обязано остаться работоспособным после сборки завершённых потоков
	 */
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки нового потока
	ASSERT_EQ(client.send(sid, "after collect", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные нового потока приняты сервером
	ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
	// Проверяем содержимое принятых данных
	ASSERT_EQ(payload, "after collect");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест защиты от повторной обработки принятых пакетов (RFC 9000 §12.3)
 *
 * @details Повторно доставленные датаграммы обязаны отбрасываться без выдачи
 *          дублирующих данных приложению и без нарушения flow control. Проверка
 *          выполняется на длинной серии пакетов, выходящей за пределы окна защиты
 */
TEST_F(QuicFixture, ConnectionReplayWindowTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Список переданных серверу датаграмм клиента
	std::vector <std::string> history;
	// Эталонное содержимое переданных данных потока
	std::string expected = "";
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	/**
	 * Передаём серию отдельных блоков данных, каждый в своей датаграмме:
	 * серия заведомо длиннее окна защиты от повторов в 64 пакета
	 */
	for(size_t i = 0; i < 100; i++){
		// Формируем очередной блок данных потока
		const std::string chunk = ("chunk-" + std::to_string(i) + ";");
		// Ставим блок данных в очередь отправки
		ASSERT_EQ(client.send(sid, chunk, false), status_t::OK);
		// Дописываем блок в эталонное содержимое
		expected.append(chunk);
		/**
		 * Передаём все готовые датаграммы клиента серверу
		 */
		while(client.write(datagram, now)){
			// Продвигаем тестовые часы
			now += 1;
			// Передаём датаграмму серверу
			ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			// Сохраняем датаграмму для последующего повтора
			history.push_back(datagram);
		}
		// Передаём датаграммы сервера клиенту для продвижения подтверждений
		::transfer(server, client, now);
	}
	// Буфер принятых сервером данных
	std::string received = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём принятые данные приложению
	ASSERT_EQ(server.receive(sid, received, fin), status_t::OK);
	// Проверяем что данные приняты полностью и без искажений
	ASSERT_EQ(received, expected);
	// Проверяем что переданных датаграмм больше размера окна защиты от повторов
	ASSERT_GT(history.size(), 64u);
	/**
	 * Повторно доставляем серверу всю историю датаграмм: и свежие, попадающие
	 * в окно защиты, и давно устаревшие, вышедшие за его пределы
	 */
	for(auto & replay : history)
		// Повторно передаём датаграмму серверу - дубликат обязан быть отброшен
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (replay.data()), replay.size(), now), status_t::OK);
	// Буфер данных, выданных приложению после повтора
	std::string duplicated = "";
	// Флаг завершения потока после повтора
	bool duplicatedFin = false;
	// Выдаём принятые данные приложению повторно
	ASSERT_EQ(server.receive(sid, duplicated, duplicatedFin), status_t::OK);
	// Проверяем что повтор не породил ни одного дублирующего октета
	ASSERT_TRUE(duplicated.empty());
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	// Проверяем что состояние соединения не изменилось
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
}

/**
 * @brief Тест схлопывания окна перегрузки при устойчивой перегрузке (RFC 9002 §7.6)
 *
 * @details Длительный обрыв связи, при котором подряд теряются пакеты за период
 *          дольше порогового, обязан вернуть окно перегрузки к минимальному:
 *          иначе на восстановлении отправитель бьёт полным окном в перегруженный путь
 */
TEST_F(QuicFixture, CongestionPersistentTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Запоминаем окно перегрузки до обрыва связи
	const uint64_t before = client.cwnd();
	// Проверяем что окно перегрузки выше минимального
	ASSERT_GT(before, 2400u);
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	/**
	 * Имитируем обрыв связи: клиент отправляет данные длительное время, но ни одна
	 * датаграмма до сервера не доходит. Между отправками часы двигаются далеко за
	 * порог периода устойчивой перегрузки
	 */
	for(size_t i = 0; i < 8; i++){
		// Ставим очередной блок данных в очередь отправки
		ASSERT_EQ(client.send(sid, "blackout payload", false), status_t::OK);
		// Извлекаем датаграмму клиента и отбрасываем её
		client.write(datagram, now);
		// Продвигаем тестовые часы далеко вперёд
		now += 400;
		// Обрабатываем просроченные таймеры клиента
		client.tick(now);
	}
	// Наименьшее окно перегрузки, наблюдавшееся после восстановления связи
	uint64_t minimal = client.cwnd();
	/**
	 * Связь восстановлена: доводим обмен до конца, чтобы сервер подтвердил
	 * дошедшие пакеты и клиент признал потерянными все отправленные в обрыв.
	 * Окно перегрузки отслеживается на каждом шаге: после схлопывания оно снова
	 * растёт в замедленном старте, поэтому по итоговому значению судить нельзя
	 */
	for(size_t i = 0; i < 16; i++){
		/**
		 * Ставим новые данные в очередь отправки: подтверждение дошедших пакетов
		 * и признание потерянными отправленных в обрыв возможны только на встречном
		 * трафике - собственных поводов к отправке у клиента уже нет
		 */
		ASSERT_EQ(client.send(sid, "recovery payload", false), status_t::OK);
		// Продвигаем тестовые часы
		now += 20;
		// Обрабатываем просроченные таймеры клиента
		client.tick(now);
		/**
		 * Передаём датаграммы клиента серверу
		 */
		while(client.write(datagram, now)){
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграмму серверу
			ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
		}
		/**
		 * Передаём датаграммы сервера клиенту по одной: окно перегрузки замеряется
		 * после каждой, иначе схлопывание успевает смениться ростом в замедленном
		 * старте между замерами
		 */
		while(server.write(datagram, now)){
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграмму клиенту
			ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			// Запоминаем наименьшее наблюдавшееся окно перегрузки
			minimal = std::min(minimal, client.cwnd());
		}
	}
	// Проверяем что окно перегрузки схлопывалось до минимального (RFC 9002 §7.6.2)
	ASSERT_EQ(minimal, 2400u);
	// Проверяем что окно перегрузки было заметно выше до обрыва связи
	ASSERT_GT(before, minimal);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест исключения задержки подтверждения из оценки задержки приёма-передачи (RFC 9002 §5.3)
 *
 * @details Удалённый эндпоинт сообщает во фрейме ACK собственную задержку
 *          подтверждения. Без её вычитания оценка задержки приёма-передачи
 *          завышается на всё время ожидания, а вслед за ней завышается таймер PTO
 */
TEST_F(QuicFixture, ConnectionAckDelayTest){
	// Дедлайны таймера клиента для немедленного и отложенного подтверждения
	uint64_t deadlines[2] = {0, 0};
	/**
	 * Прогоняем один сценарий дважды: с немедленным ответом сервера и с задержкой
	 * подтверждения. Учтённая задержка обязана сделать дедлайны сопоставимыми
	 */
	for(size_t pass = 0; pass < 2; pass++){
		// Задержка подтверждения сервером в миллисекундах
		const uint64_t delay = ((pass == 0) ? 0 : 90);
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Транспортные параметры с расширенной максимальной задержкой подтверждения
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
		/**
		 * Расширяем максимальную задержку подтверждения: оценка задержки ограничивает
		 * вычитаемое значение анонсированным максимумом пира (RFC 9002 §5.3)
		 */
		params.maxAckDelay = 100;
		// Выполняем подготовку соединения клиента
		::configure(client, params);
		// Выполняем подготовку соединения сервера
		::configure(server, params);
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		// Открываем двунаправленный поток на клиенте
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Буфер передаваемой датаграммы
		std::string datagram = "";
		/**
		 * Выполняем серию обменов: оценка задержки сглаженная, поэтому одного
		 * измерения для расхождения дедлайнов недостаточно
		 */
		for(size_t i = 0; i < 12; i++){
			// Ставим очередной блок данных в очередь отправки
			ASSERT_EQ(client.send(sid, "round trip payload", false), status_t::OK);
			/**
			 * Передаём датаграммы клиента серверу
			 */
			while(client.write(datagram, now)){
				// Продвигаем тестовые часы на время передачи по сети
				now += 5;
				// Передаём датаграмму серверу
				ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			}
			// Выдерживаем задержку подтверждения на стороне сервера
			now += delay;
			/**
			 * Передаём датаграммы сервера клиенту
			 */
			while(server.write(datagram, now)){
				// Продвигаем тестовые часы на время передачи по сети
				now += 5;
				// Передаём датаграмму клиенту
				ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
			}
			// Буфер принятых сервером данных
			std::string payload = "";
			// Флаг завершения потока
			bool fin = false;
			// Выдаём принятые данные приложению
			server.receive(sid, payload, fin);
		}
		// Ставим данные в очередь отправки для взведения таймера
		ASSERT_EQ(client.send(sid, "pending payload", false), status_t::OK);
		// Извлекаем датаграмму клиента без доставки серверу
		ASSERT_TRUE(client.write(datagram, now));
		// Получаем дедлайн ближайшего события таймера клиента
		const uint64_t deadline = client.timeout();
		// Проверяем что таймер взведён
		ASSERT_GT(deadline, now);
		// Запоминаем интервал до срабатывания таймера
		deadlines[pass] = (deadline - now);
	}
	/**
	 * Задержка подтверждения исключена из оценки, поэтому интервалы обоих прогонов
	 * сопоставимы. Без её учёта отложенный прогон дал бы завышение почти на всю
	 * задержку в 90 миллисекунд
	 */
	const uint64_t difference = ((deadlines[1] > deadlines[0]) ? (deadlines[1] - deadlines[0]) : (deadlines[0] - deadlines[1]));
	// Проверяем что расхождение интервалов существенно меньше выдержанной задержки
	ASSERT_LT(difference, 45u);
}

/**
 * @brief Тест отказа от обработки фреймов в состоянии завершения соединения (RFC 9000 §10.2.1)
 *
 * @details Эндпоинт в состоянии завершения отвечает на принятые пакеты только
 *          повторным фреймом CONNECTION_CLOSE и никаких иных фреймов не обрабатывает
 */
TEST_F(QuicFixture, ConnectionClosingIgnoresFramesTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на сервере
	const uint64_t sid = server.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Завершаем соединение на клиенте по инициативе приложения
	client.close(0x00, "application shutdown");
	// Проверяем состояние завершения соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	// Извлекаем датаграмму клиента с фреймом CONNECTION_CLOSE
	ASSERT_TRUE(client.write(datagram, now));
	// Ставим данные потока в очередь отправки на сервере
	ASSERT_EQ(server.send(sid, "data after close", true), status_t::OK);
	/**
	 * Передаём датаграммы сервера клиенту: клиент находится в состоянии завершения
	 * и обязан их отбросить, не разбирая нагрузку
	 */
	while(server.write(datagram, now)){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграмму клиенту
		ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	}
	// Список потоков с собранными данными
	std::vector <uint64_t> streams;
	// Получаем список потоков с собранными данными
	client.readable(streams);
	// Проверяем что поток на клиенте не создан - фреймы STREAM не обрабатывались
	ASSERT_TRUE(streams.empty());
	// Буфер принятых клиентом данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные потока клиенту недоступны
	ASSERT_EQ(client.receive(sid, payload, fin), status_t::ERROR);
	// Проверяем что состояние завершения соединения сохранено
	ASSERT_EQ(client.state(), connection_t::state_t::CLOSING);
	// Проверяем что клиент продолжает отвечать повторным фреймом CONNECTION_CLOSE
	ASSERT_TRUE(client.write(datagram, now));
}

/**
 * @brief Тест обнаружения сброса без сохранения состояния (RFC 9000 §10.3)
 *
 * @details Удалённый эндпоинт, утративший состояние соединения, отвечает
 *          датаграммой с токеном сброса. Опознав её, локальный эндпоинт обязан
 *          молча перейти в состояние завершения и прекратить любую отправку
 */
TEST_F(QuicFixture, ConnectionStatelessResetTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Транспортные параметры эндпоинтов
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
	// Выполняем подготовку соединения клиента
	::configure(client, params);
	/**
	 * Токен сброса без сохранения состояния - параметр только для сервера:
	 * клиент закодировать его не вправе (RFC 9000 §18.2)
	 */
	params.hasResetToken = true;
	/**
	 * Заполняем токен сброса известным значением: клиент получит его транспортным
	 * параметром сервера и обязан опознать по нему сброс
	 */
	for(size_t i = 0; i < proto::RESET_TOKEN_SIZE; i++)
		// Записываем очередной октет токена сброса
		params.resetToken[i] = static_cast <uint8_t> (0xA0 + i);
	// Выполняем подготовку соединения сервера
	::configure(server, params);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние установленного соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	/**
	 * Формируем датаграмму сброса: короткий заголовок со случайным содержимым
	 * и токен сброса в последних 16 октетах (RFC 9000 §10.3)
	 */
	std::string reset(40, '\0');
	// Устанавливаем первый октет короткого заголовка с обязательным fixed-битом
	reset[0] = static_cast <char> (0x40);
	/**
	 * Заполняем непрозрачную часть датаграммы произвольными данными
	 */
	for(size_t i = 1; i < (reset.size() - proto::RESET_TOKEN_SIZE); i++)
		// Записываем очередной октет непрозрачной части
		reset[i] = static_cast <char> (0x5A + i);
	/**
	 * Дописываем токен сброса в хвост датаграммы
	 */
	for(size_t i = 0; i < proto::RESET_TOKEN_SIZE; i++)
		// Записываем очередной октет токена сброса
		reset[reset.size() - proto::RESET_TOKEN_SIZE + i] = static_cast <char> (0xA0 + i);
	// Передаём датаграмму сброса клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (reset.data()), reset.size(), now), status_t::OK);
	// Проверяем что клиент перешёл в состояние завершения соединения
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Проверяем что отправка после сброса прекращена (RFC 9000 §10.3)
	ASSERT_FALSE(client.write(datagram, now));
}

/**
 * @brief Тест устойчивости к датаграммам с чужим токеном сброса (RFC 9000 §10.3.1)
 *
 * @details Датаграмма, хвост которой не совпадает ни с одним известным токеном,
 *          сбросом не является и соединение затрагивать не должна
 */
TEST_F(QuicFixture, ConnectionForeignResetTokenTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Формируем датаграмму с непринадлежащим соединению токеном в хвосте
	std::string foreign(40, '\0');
	// Устанавливаем первый октет короткого заголовка с обязательным fixed-битом
	foreign[0] = static_cast <char> (0x40);
	/**
	 * Заполняем датаграмму произвольными данными, включая хвост: совпадение
	 * с токеном сброса исключено
	 */
	for(size_t i = 1; i < foreign.size(); i++)
		// Записываем очередной октет датаграммы
		foreign[i] = static_cast <char> (0x11 + i);
	// Передаём датаграмму клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (foreign.data()), foreign.size(), now), status_t::OK);
	// Проверяем что состояние соединения не изменилось
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест привязки токена проверки адреса к адресу клиента (RFC 9000 §8.1.4)
 *
 * @details Токен пакета Retry заверен кодом аутентичности от адреса клиента.
 *          Повтор токена с другого адреса обязан быть отвергнут, иначе проверка
 *          адреса теряет смысл: перехваченный токен работал бы откуда угодно
 */
TEST_F(QuicFixture, RetryTokenAddressBindingTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Устанавливаем адрес клиента на серверном соединении
	server.address("198.51.100.17:44301");
	// Включаем проверку адреса клиента через пакет Retry
	server.retry(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial без токена
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Буфер датаграммы сервера с пакетом Retry
	std::string retry = "";
	// Извлекаем датаграмму сервера с пакетом Retry
	ASSERT_TRUE(server.write(retry, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём пакет Retry клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (retry.data()), retry.size(), now), status_t::OK);
	// Извлекаем повторный пакет Initial клиента с токеном проверки адреса
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	/**
	 * Создаём соединение стороннего сервера с тем же режимом проверки адреса,
	 * но с другим адресом клиента: перехваченный токен обязан быть отвергнут
	 */
	connection_t foreign(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения стороннего сервера
	::setup(foreign);
	// Устанавливаем другой адрес клиента на стороннем соединении
	foreign.address("203.0.113.9:51520");
	// Включаем проверку адреса клиента через пакет Retry
	foreign.retry(true);
	// Передаём повторный пакет Initial стороннему серверу
	ASSERT_EQ(foreign.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что сторонний сервер соединение не начал - токен выдан другому адресу
	ASSERT_EQ(foreign.state(), connection_t::state_t::NONE);
	/**
	 * Тот же пакет для сервера с совпадающим адресом клиента: токен принимается
	 * без сохранения состояния выдачи
	 */
	connection_t accepting(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку принимающего соединения
	::setup(accepting);
	// Устанавливаем адрес клиента, которому выдавался токен
	accepting.address("198.51.100.17:44301");
	// Включаем проверку адреса клиента через пакет Retry
	accepting.retry(true);
	// Передаём повторный пакет Initial принимающему серверу
	ASSERT_EQ(accepting.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что соединение начато по корректному токену
	ASSERT_EQ(accepting.state(), connection_t::state_t::HANDSHAKING);
}

/**
 * @brief Тест истечения срока годности токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @details Токен несёт отметку времени выдачи и после истечения срока годности
 *          обязан отвергаться: иначе перехваченный токен пригоден бессрочно
 */
TEST_F(QuicFixture, RetryTokenExpiryTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Устанавливаем адрес клиента на серверном соединении
	server.address("198.51.100.17:44301");
	// Включаем проверку адреса клиента через пакет Retry
	server.retry(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial без токена
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Буфер датаграммы сервера с пакетом Retry
	std::string retry = "";
	// Извлекаем датаграмму сервера с пакетом Retry
	ASSERT_TRUE(server.write(retry, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём пакет Retry клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (retry.data()), retry.size(), now), status_t::OK);
	// Извлекаем повторный пакет Initial клиента с токеном проверки адреса
	ASSERT_TRUE(client.write(datagram, now));
	/**
	 * Создаём принимающее соединение и продвигаем его часы далеко за срок
	 * годности токена
	 */
	connection_t expired(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку принимающего соединения
	::setup(expired);
	// Устанавливаем адрес клиента, которому выдавался токен
	expired.address("198.51.100.17:44301");
	// Включаем проверку адреса клиента через пакет Retry
	expired.retry(true);
	// Передаём повторный пакет Initial спустя срок годности токена
	ASSERT_EQ(expired.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now + 60000), status_t::OK);
	// Проверяем что просроченный токен отвергнут
	ASSERT_EQ(expired.state(), connection_t::state_t::NONE);
}

/**
 * @brief Тест проверки сертификата сервера по заданному доверенному центру
 *
 * @details Проверка сертификата без указания доверенных центров опирается на
 *          системное хранилище, в котором самоподписанного сертификата нет.
 *          Указание файла делает его доверенным якорем, и хендшейк проходит
 */
TEST_F(QuicFixture, ConnectionVerifyWithCaTest){
	// Создаём шаблон контекста клиента с включённой проверкой сертификата
	const awh::tls::Coder::id_t context = ::security().make(endpoint_t::CLIENT, {awh::tls::Coder::alpn_t{0, "h3"}}, true);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, context, ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения обоих эндпоинтов
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
	// Удаляем созданный шаблон контекста безопасности
	ASSERT_TRUE(::security().coder().destroy(context));
}

/**
 * @brief Тест отказа проверки сертификата без доверенного центра
 *
 * @details Тот же сертификат без указания доверенного центра доверия не
 *          заслуживает: хендшейк обязан завершиться ошибкой криптографического
 *          уровня, а не пройти молча
 */
TEST_F(QuicFixture, ConnectionVerifyWithoutCaTest){
	// Получаем объект кодера транспортной безопасности
	awh::tls::Coder & coder = ::security().coder();
	/**
	 * Создаём шаблон контекста клиента вручную: общий шаблон окружения содержит
	 * тестовый сертификат в качестве доверенного якоря, а здесь проверяется
	 * поведение именно без него
	 */
	const awh::tls::Coder::id_t context = coder.context(awh::event::node_t::CLIENT, awh::event::protocol_t::QUIC);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Устанавливаем список поддерживаемых ALPN-протоколов
	coder.alpn(context, {awh::tls::Coder::alpn_t{0, "h3"}});
	// Устанавливаем доменное имя удалённого узла
	coder.serverNameIndication(context, "localhost");
	// Включаем проверку сертификата удалённого узла без доверенных центров
	coder.validateServerNameIndication(context, true);
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, context, coder, &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), coder, &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем обмен датаграммами - соединение установиться не должно
	ASSERT_FALSE(::establish(client, server, now));
	// Проверяем что соединение клиента не установлено
	ASSERT_NE(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем что хендшейк клиента завершился ошибкой
	ASSERT_EQ(client.handshake().state(), handshake_t::state_t::FAILED);
	// Проверяем что клиент сообщил об ошибке криптографического уровня (RFC 9001 §4.8)
	ASSERT_GE(static_cast <uint64_t> (client.error()), static_cast <uint64_t> (error_t::CRYPTO_ERROR));
	// Удаляем созданный шаблон контекста безопасности
	ASSERT_TRUE(coder.destroy(context));
}

/**
 * @brief Тест хендшейка на отдельном экземпляре кодера транспортной безопасности
 *
 * @details Настройка криптографии выполняется средствами кодера, а слой
 *          соединения QUIC ведёт на его шаблоне контекста собственный хендшейк:
 *          слой записей TLS в QUIC не используется, поэтому транспортный
 *          уровень кодера не задействован.
 *
 *          Тест подтверждает контракт: шаблон контекста безопасности допускает
 *          создание объектов TLS сторонним модулем, и функции обратного вызова
 *          уровня контекста это переносят
 */
TEST_F(QuicFixture, ConnectionExternalContextTest){
	// Объект фреймворка
	awh::fmk_t fmk;
	// Объект логирования
	awh::log_t log(&fmk);
	// Отдельное окружение транспортной безопасности с собственным кодером
	QuicSecurity security(&fmk, &log);
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, security.context(endpoint_t::CLIENT), security.coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, security.context(endpoint_t::SERVER), security.coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения обоих эндпоинтов
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "payload over external context", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные приняты сервером
	ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
	// Проверяем содержимое принятых данных
	ASSERT_EQ(payload, "payload over external context");
}

/**
 * @brief Тест проверки достижимости пути (RFC 9000 §8.2)
 *
 * @details Локальный эндпоинт отправляет фрейм PATH_CHALLENGE со случайными
 *          данными, удалённый обязан вернуть их фреймом PATH_RESPONSE.
 *          До получения ответа путь подтверждённым не считается
 */
TEST_F(QuicFixture, ConnectionPathValidationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем что путь ещё не подтверждён
	ASSERT_FALSE(client.validated());
	// Инициируем проверку достижимости пути
	ASSERT_TRUE(client.probe());
	// Проверяем что повторная проверка до получения ответа отвергается
	ASSERT_FALSE(client.probe());
	// Проверяем что путь до обмена всё ещё не подтверждён
	ASSERT_FALSE(client.validated());
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Проверяем что путь подтверждён ответом удалённого эндпоинта
	ASSERT_TRUE(client.validated());
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
	// Проверяем что после подтверждения возможна новая проверка пути
	ASSERT_TRUE(client.probe());
	// Проверяем что новая проверка сбросила подтверждение
	ASSERT_FALSE(client.validated());
}

/**
 * @brief Тест обнаружения миграции соединения по смене адреса (RFC 9000 §9)
 *
 * @details Слой соединения адресов не знает - их сообщает вызывающий код.
 *          Смена адреса удалённого эндпоинта при установленном соединении
 *          означает новый сетевой путь: прежние оценки ёмкости и задержки
 *          к нему неприменимы, а достижимость требует подтверждения
 */
TEST_F(QuicFixture, ConnectionMigrationDetectTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Устанавливаем исходный адрес клиента на сервере
	server.address("198.51.100.17:44301");
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки для роста окна перегрузки
	ASSERT_EQ(client.send(sid, std::string(60000, 'x'), false), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Выдаём принятые данные приложению
	server.receive(sid, payload, fin);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Запоминаем окно перегрузки сервера до миграции
	const uint64_t before = server.cwnd();
	// Проверяем что смен пути ещё не выполнялось
	ASSERT_EQ(server.migrations(), 0u);
	// Сообщаем серверу новый адрес клиента - имитация смены сети
	server.address("203.0.113.9:51520");
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	// Ставим данные в очередь отправки клиента
	ASSERT_EQ(client.send(sid, "payload after migration", false), status_t::OK);
	// Извлекаем датаграмму клиента
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу с нового адреса
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что смена пути обнаружена
	ASSERT_EQ(server.migrations(), 1u);
	// Проверяем что достижимость нового пути ещё не подтверждена
	ASSERT_FALSE(server.validated());
	// Проверяем что окно перегрузки возвращено к начальному значению (RFC 9000 §9.4)
	ASSERT_EQ(server.cwnd(), 12000u);
	// Проверяем что окно перегрузки до миграции было выше начального
	ASSERT_GT(before, server.cwnd());
	// Выполняем обмен датаграммами для проверки достижимости нового пути
	::pump(client, server, now);
	// Проверяем что достижимость нового пути подтверждена
	ASSERT_TRUE(server.validated());
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест инициативной миграции соединения (RFC 9000 §9.5)
 *
 * @details Локальный эндпоинт переключается на новый путь сам: берёт
 *          неиспользованный идентификатор удалённого эндпоинта, сбрасывает
 *          состояние пути и начинает проверку достижимости
 */
TEST_F(QuicFixture, ConnectionMigrateTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Выполняем обмен для доставки анонсов дополнительных идентификаторов
	 * соединения: без неиспользованного идентификатора миграция невозможна
	 */
	::pump(client, server, now);
	// Открываем двунаправленный поток для наращивания окна перегрузки
	const uint64_t warmup = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(warmup, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(warmup, std::string(60000, 'x'), true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string warmupPayload = "";
	// Флаг завершения потока
	bool warmupFin = false;
	// Выдаём принятые данные приложению
	server.receive(warmup, warmupPayload, warmupFin);
	// Выполняем обмен датаграммами для подтверждения отправленного
	::pump(client, server, now);
	/**
	 * Проверяем что окно перегрузки выросло выше начального: без этого проверка
	 * его сброса при миграции ничего бы не значила
	 */
	ASSERT_GT(client.cwnd(), 12000u);
	// Запоминаем идентификатор соединения удалённого эндпоинта до миграции
	const cid_t before = client.dcid();
	// Выполняем инициативную миграцию соединения
	ASSERT_TRUE(client.migrate());
	// Проверяем что смена пути учтена
	ASSERT_EQ(client.migrations(), 1u);
	// Проверяем что идентификатор удалённого эндпоинта сменился (RFC 9000 §9.5)
	ASSERT_FALSE(client.dcid() == before);
	// Проверяем что достижимость нового пути ещё не подтверждена
	ASSERT_FALSE(client.validated());
	// Проверяем что окно перегрузки возвращено к начальному значению
	ASSERT_EQ(client.cwnd(), 12000u);
	// Выполняем обмен датаграммами для проверки достижимости нового пути
	::pump(client, server, now);
	// Проверяем что достижимость нового пути подтверждена
	ASSERT_TRUE(client.validated());
	// Открываем двунаправленный поток на клиенте после миграции
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "payload over migrated path", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные приняты сервером по новому пути
	ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
	// Проверяем содержимое принятых данных
	ASSERT_EQ(payload, "payload over migrated path");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест возврата счётчиков маркировок ECN эхом (RFC 9000 §13.4.1)
 *
 * @details Принимающий эндпоинт считает маркировки заголовка IP-пакета и
 *          возвращает счётчики пиру во фрейме ACK_ECN. Без маркировок фрейм
 *          подтверждения счётчиков не несёт
 */
TEST_F(QuicFixture, ConnectionEcnEchoTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	// Извлекаем первый пакет Initial клиента
	ASSERT_TRUE(client.write(datagram, now));
	/**
	 * Передаём датаграмму серверу с маркировкой поддержки ECN: счётчик маркировок
	 * сервера обязан вырасти, а отправленный им ответ - нести их эхом
	 */
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now, awh::event::ecn_t::ECT0), status_t::OK);
	// Продвигаем тестовые часы
	now += 5;
	// Буфер ответной датаграммы сервера
	std::string answer = "";
	// Извлекаем ответную датаграмму сервера
	ASSERT_TRUE(server.write(answer, now));
	// Проверяем что ответная датаграмма не пустая
	ASSERT_FALSE(answer.empty());
	/**
	 * Проверяем что ответ несёт фрейм подтверждения со счётчиками маркировок:
	 * пакет Initial сервера защищён ключами, выведенными из DCID клиента,
	 * поэтому проверка выполняется приёмом на стороне клиента
	 */
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (answer.data()), answer.size(), now), status_t::OK);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест сокращения окна перегрузки по сигналу ECN (RFC 9002 §7.7)
 *
 * @details Маршрутизатор на пути отмечает пакеты маркировкой перегрузки, не
 *          отбрасывая их. Прирост счётчика такой маркировки в подтверждении
 *          обязан сократить окно перегрузки отправителя ровно как потеря.
 *
 *          Сравниваются два одинаковых прогона - с маркировкой и без неё:
 *          абсолютная величина окна зависит от числа подтверждений, поэтому
 *          значима именно разница между прогонами
 */
TEST_F(QuicFixture, ConnectionEcnCongestionTest){
	/**
	 * @brief Функция прогона передачи с заданной маркировкой ECN
	 *
	 * @param ecn маркировка ECN датаграмм отправителя
	 * @return    окно перегрузки отправителя по завершении прогона
	 */
	auto run = [](const awh::event::ecn_t ecn) noexcept -> uint64_t {
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Устанавливаем маркировку исходящих датаграмм клиента
		client.ecn(ecn != awh::event::ecn_t::NOT_ECT);
		// Выполняем начало соединения клиентом
		if(client.connect() != status_t::OK)
			// Выводим отрицательный результат
			return 0;
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Выполняем полное установление соединения с заданной маркировкой
		if(!::establish(client, server, now, nullptr, ecn))
			// Выводим отрицательный результат
			return 0;
		// Открываем двунаправленный поток на клиенте
		const uint64_t sid = client.open(false);
		// Если поток не открыт
		if(sid == connection_t::INVALID_STREAM)
			// Выводим отрицательный результат
			return 0;
		// Формируем полезную нагрузку одного шага передачи
		const std::string payload(8192, 'e');
		/**
		 * Выполняем передачу порциями: маркировка перегрузки сообщается пиру
		 * подтверждением, поэтому окно замеряется по завершении всех шагов
		 */
		for(size_t i = 0; i < 24; i++){
			// Ставим очередную порцию данных в очередь отправки
			client.send(sid, payload, false);
			// Продвигаем тестовые часы
			now += 5;
			// Буфер передаваемой датаграммы
			std::string datagram = "";
			/**
			 * Передаём датаграммы клиента серверу с заданной маркировкой
			 */
			while(client.write(datagram, now))
				// Передаём датаграмму серверу с заданной маркировкой
				server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now, ecn);
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграммы сервера клиенту
			::transfer(server, client, now);
		}
		// Выводим окно перегрузки отправителя по завершении прогона
		return client.cwnd();
	};
	// Выполняем прогон передачи с маркировкой поддержки ECN
	const uint64_t supported = run(awh::event::ecn_t::ECT0);
	// Выполняем прогон передачи с маркировкой перегрузки пути
	const uint64_t congested = run(awh::event::ecn_t::CE);
	// Проверяем что прогон без перегрузки состоялся
	ASSERT_GT(supported, 12000u);
	/**
	 * Проверяем что маркировка перегрузки сократила окно: оба прогона одинаковы
	 * во всём, кроме маркировки, поэтому разница между ними и есть её действие
	 */
	ASSERT_LT(congested, supported);
}

/**
 * @brief Тест возобновления сессии соединения (RFC 9001 §4.6)
 *
 * @details Сервер присылает билет возобновления после установления соединения.
 *          Сохранённая по нему сессия позволяет клиенту возобновить соединение
 *          с тем же сервером, не выполняя полного хендшейка
 */
TEST_F(QuicFixture, ConnectionSessionResumeTest){
	// Сериализованная сессия возобновления, полученная от сервера
	std::string ticket = "";
	/**
	 * Выполняем первое соединение: по его завершении сервер присылает билет
	 * возобновления, из которого и извлекается сессия
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Проверяем что до соединения сессия недоступна
		ASSERT_TRUE(client.session().empty());
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		/**
		 * Прогоняем обмен после установления соединения: билет возобновления
		 * приходит отдельным сообщением уже по защищённому каналу
		 */
		for(size_t i = 0; i < 8; i++){
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграммы сервера клиенту
			::transfer(server, client, now);
			// Передаём датаграммы клиента серверу
			::transfer(client, server, now);
		}
		// Извлекаем сессию возобновления соединения
		ticket = client.session();
		// Проверяем что билет возобновления получен от сервера
		ASSERT_FALSE(ticket.empty());
	}
	/**
	 * Выполняем второе соединение с возобновлением сессии: полный хендшейк
	 * не выполняется, а согласование завершается по сохранённому билету
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Устанавливаем сохранённую сессию возобновления
		ASSERT_TRUE(client.session(ticket));
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		/**
		 * Проверяем что ключи защиты ранних данных выданы: возобновление сессии
		 * позволяет отправлять данные, не дожидаясь завершения хендшейка,
		 * и криптографическая библиотека выдаёт ключи сразу (RFC 9001 §4.6)
		 */
		ASSERT_NE(client.handshake().encryption(level_t::EARLY_DATA), nullptr);
		/**
		 * Проверяем что хендшейк завершённым не считается: криптографическая
		 * библиотека возвращает успех шага, приостановив хендшейк для отправки
		 * ранних данных, и принять это за завершение нельзя
		 */
		ASSERT_EQ(client.handshake().state(), handshake_t::state_t::PROCESS);
		// Проверяем что установка сессии после начала соединения отвергается
		ASSERT_FALSE(client.session(ticket));
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		// Проверяем состояние соединения обоих эндпоинтов
		ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
		ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
		// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
		ASSERT_EQ(client.error(), error_t::NO_ERROR);
		ASSERT_EQ(server.error(), error_t::NO_ERROR);
		// Открываем двунаправленный поток на возобновлённом соединении
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Ставим данные в очередь отправки
		ASSERT_EQ(client.send(sid, "resumed session data", true), status_t::OK);
		// Выполняем обмен датаграммами
		::pump(client, server, now);
		// Буфер принятых сервером данных
		std::string payload = "";
		// Флаг завершения потока
		bool fin = false;
		// Проверяем что данные приняты сервером
		ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
		// Проверяем содержимое принятых данных
		ASSERT_EQ(payload, "resumed session data");
	}
}

/**
 * @brief Тест выдачи изменений набора идентификаторов соединения (RFC 9000 §5.1.1)
 *
 * @details Соединение адресуется набором идентификаторов, который меняется по
 *          ходу работы. Изменения выдаются однократно и сбрасываются: вызывающий
 *          код синхронизирует по ним маршрутизацию, и повторная выдача привела бы
 *          к попытке привязать уже привязанное
 */
TEST_F(QuicFixture, ConnectionIssuedCidsTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Идентификаторы, введённые в обращение
	std::vector <cid_t> added;
	// Идентификаторы, выведенные из обращения
	std::vector <cid_t> removed;
	// Извлекаем изменения набора идентификаторов до начала соединения
	client.issued(added, removed);
	// Проверяем что до начала соединения идентификаторы не выдавались
	ASSERT_TRUE(added.empty());
	ASSERT_TRUE(removed.empty());
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Извлекаем изменения набора идентификаторов после начала соединения
	client.issued(added, removed);
	/**
	 * Проверяем что идентификатор локального эндпоинта введён в обращение:
	 * по нему удалённый узел и адресует ответные датаграммы
	 */
	ASSERT_EQ(added.size(), 1u);
	// Проверяем что введённый в обращение идентификатор совпадает с идентификатором эндпоинта
	ASSERT_TRUE(added.front() == client.scid());
	// Проверяем что из обращения ничего не выведено
	ASSERT_TRUE(removed.empty());
	/**
	 * Извлекаем изменения повторно: накопленные изменения выдаются однократно,
	 * поэтому второй вызов обязан вернуть пустые списки
	 */
	client.issued(added, removed);
	// Проверяем что повторная выдача изменений пуста
	ASSERT_TRUE(added.empty());
	ASSERT_TRUE(removed.empty());
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Прогоняем обмен после установления соединения: дополнительные идентификаторы
	 * анонсируются фреймами NEW_CONNECTION_ID уже по защищённому каналу
	 */
	for(size_t i = 0; i < 4; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
		// Передаём датаграммы клиента серверу
		::transfer(client, server, now);
	}
	// Извлекаем изменения набора идентификаторов сервера
	server.issued(added, removed);
	/**
	 * Проверяем что сервер ввёл в обращение дополнительные идентификаторы:
	 * без них клиенту не на что переключаться при миграции (RFC 9000 §9.5)
	 */
	ASSERT_FALSE(added.empty());
	/**
	 * Перебираем введённые в обращение идентификаторы
	 */
	for(auto & cid : added)
		// Проверяем что длина идентификатора соответствует локальной политике
		ASSERT_EQ(cid.size, connection_t::LOCAL_CID_SIZE);
}

/**
 * @brief Тест отправки ранних данных до завершения хендшейка (RFC 9001 §4.6)
 *
 * @details Возобновление сессии позволяет клиенту открыть поток и отправить
 *          данные сразу, не дожидаясь хендшейка: ключи защиты ранних данных
 *          выданы, а лимиты потоков взяты из прошлого соединения
 */
TEST_F(QuicFixture, ConnectionEarlyDataTest){
	// Сериализованный билет возобновления, полученный от сервера
	std::string ticket = "";
	/**
	 * Выполняем первое соединение ради билета возобновления
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		/**
		 * Прогоняем обмен после установления соединения ради билета возобновления
		 */
		for(size_t i = 0; i < 8; i++){
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграммы сервера клиенту
			::transfer(server, client, now);
			// Передаём датаграммы клиента серверу
			::transfer(client, server, now);
		}
		// Извлекаем билет возобновления соединения
		ticket = client.session();
		// Проверяем что билет возобновления получен
		ASSERT_FALSE(ticket.empty());
	}
	/**
	 * Выполняем второе соединение с отправкой ранних данных
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Проверяем что до установки билета поток открыть невозможно
		ASSERT_EQ(client.open(false), connection_t::INVALID_STREAM);
		// Устанавливаем сохранённую сессию возобновления
		ASSERT_TRUE(client.session(ticket));
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Проверяем что соединение ещё не установлено
		ASSERT_EQ(client.state(), connection_t::state_t::HANDSHAKING);
		/**
		 * Открываем поток до завершения хендшейка: лимиты взяты из запомненных
		 * транспортных параметров прошлого соединения (RFC 9001 §4.6.1)
		 */
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт до завершения хендшейка
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Ставим ранние данные в очередь отправки до завершения хендшейка
		ASSERT_EQ(client.send(sid, "early application data", true), status_t::OK);
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Буфер первой исходящей датаграммы клиента
		std::string datagram = "";
		// Проверяем что клиент собрал первую датаграмму
		ASSERT_TRUE(client.write(datagram, now));
		// Проверяем что первый флайт содержит пакет Initial с ClientHello
		ASSERT_TRUE(::contains(datagram, packet_t::INITIAL));
		/**
		 * Проверяем что ранние данные ушли пакетом 0-RTT того же флайта: ради этого
		 * возобновление и выполняется - данные приходят удалённому узлу вместе
		 * с ClientHello, не дожидаясь хендшейка (RFC 9000 §17.2.3)
		 */
		ASSERT_TRUE(::contains(datagram, packet_t::ZERO_RTT));
		// Передаём первую датаграмму серверу
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		// Проверяем что удалённый узел ранние данные принял (RFC 9001 §4.6.2)
		ASSERT_TRUE(client.early());
		// Проверяем состояние соединения обоих эндпоинтов
		ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
		ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
		// Выполняем обмен датаграммами
		::pump(client, server, now);
		// Буфер принятых сервером данных
		std::string payload = "";
		// Флаг завершения потока
		bool fin = false;
		// Проверяем что ранние данные приняты сервером
		ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
		// Проверяем содержимое принятых ранних данных
		ASSERT_EQ(payload, "early application data");
		// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
		ASSERT_EQ(client.error(), error_t::NO_ERROR);
		ASSERT_EQ(server.error(), error_t::NO_ERROR);
	}
}

/**
 * @brief Тест отказа удалённого узла в ранних данных (RFC 9001 §4.6.2)
 *
 * @details Ранние данные принимаются только под теми лимитами, что были анонсированы
 *          при выдаче билета: изменение лимитов делает билет неприменимым, и удалённый
 *          узел в ранних данных отказывает. Отказ отказом хендшейка не является -
 *          отправленные ранние данные возвращаются в очереди отправки и уходят
 *          повторно защитой уровня приложения
 */
TEST_F(QuicFixture, ConnectionEarlyDataRejectTest){
	// Сериализованный билет возобновления, полученный от сервера
	std::string ticket = "";
	/**
	 * Выполняем первое соединение ради билета возобновления
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		/**
		 * Прогоняем обмен после установления соединения ради билета возобновления
		 */
		for(size_t i = 0; i < 8; i++){
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграммы сервера клиенту
			::transfer(server, client, now);
			// Передаём датаграммы клиента серверу
			::transfer(client, server, now);
		}
		// Извлекаем билет возобновления соединения
		ticket = client.session();
		// Проверяем что билет возобновления получен
		ASSERT_FALSE(ticket.empty());
	}
	/**
	 * Выполняем второе соединение с изменёнными лимитами сервера
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Транспортные параметры сервера с изменёнными лимитами
		params::params_t params;
		// Устанавливаем лимит данных соединения, отличный от анонсированного при выдаче билета
		params.initialMaxData = 2097152;
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
		// Выполняем подготовку соединения сервера с изменёнными лимитами
		::configure(server, params);
		// Устанавливаем сохранённую сессию возобновления
		ASSERT_TRUE(client.session(ticket));
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Открываем поток до завершения хендшейка
		const uint64_t sid = client.open(false);
		// Проверяем что поток открыт до завершения хендшейка
		ASSERT_NE(sid, connection_t::INVALID_STREAM);
		// Ставим ранние данные в очередь отправки до завершения хендшейка
		ASSERT_EQ(client.send(sid, "rejected early data", true), status_t::OK);
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Буфер первой исходящей датаграммы клиента
		std::string datagram = "";
		// Проверяем что клиент собрал первую датаграмму
		ASSERT_TRUE(client.write(datagram, now));
		// Проверяем что ранние данные ушли пакетом 0-RTT первого флайта
		ASSERT_TRUE(::contains(datagram, packet_t::ZERO_RTT));
		// Передаём первую датаграмму серверу
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		// Проверяем что удалённый узел в ранних данных отказал
		ASSERT_FALSE(client.early());
		/**
		 * Проверяем что отказ хендшейк не сорвал: соединение установлено на обоих
		 * эндпоинтах и ошибки транспорта нет (RFC 9001 §4.6.2)
		 */
		ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
		ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
		// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
		ASSERT_EQ(client.error(), error_t::NO_ERROR);
		ASSERT_EQ(server.error(), error_t::NO_ERROR);
		// Выполняем обмен датаграммами
		::pump(client, server, now);
		// Буфер принятых сервером данных
		std::string payload = "";
		// Флаг завершения потока
		bool fin = false;
		/**
		 * Проверяем что отвергнутые ранние данные приняты сервером: они возвращены
		 * в очереди отправки и отправлены повторно защитой уровня приложения
		 */
		ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
		// Проверяем содержимое принятых данных
		ASSERT_EQ(payload, "rejected early data");
		// Проверяем что принято завершение потока
		ASSERT_TRUE(fin);
	}
}

/**
 * @brief Тест проверки пути на поддержку ECN (RFC 9000 §13.4.2)
 *
 * @details Маркировка накладывается на заголовок IP-пакета, и промежуточный узел
 *          пути вправе её стереть. Отправитель выявляет это по счётчикам маркировок
 *          в подтверждениях: путь, не вернувший ни одной маркировки, проверку
 *          не проходит, и маркировать его датаграммы далее бессмысленно
 */
TEST_F(QuicFixture, ConnectionEcnValidationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Проверяем что до включения маркировки датаграммы не помечаются
	ASSERT_EQ(client.marking(), awh::event::ecn_t::NOT_ECT);
	// Включаем маркировку исходящих датаграмм клиента
	client.ecn(true);
	// Проверяем что маркировка исходящих датаграмм включена
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	/**
	 * Выполняем установление соединения по пути, стирающему маркировку: датаграммы
	 * клиента доставляются серверу без неё, поэтому счётчиков маркировок
	 * в подтверждениях сервера не будет
	 */
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Проверяем что проверка пути не пройдена и маркировка снята: счётчики
	 * маркировок не вернулись ни на один помеченный пакет
	 */
	ASSERT_EQ(client.marking(), awh::event::ecn_t::NOT_ECT);
	// Проверяем что непройденная проверка пути соединение не разорвала
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест сохранения маркировки на пути с поддержкой ECN (RFC 9000 §13.4.2)
 *
 * @details Путь, доставляющий маркировку в сохранности, проверку проходит:
 *          счётчики маркировок возвращаются подтверждениями и растут ровно
 *          на число помеченных пакетов, поэтому маркировка сохраняется
 */
TEST_F(QuicFixture, ConnectionEcnValidationPassTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Включаем маркировку исходящих датаграмм клиента
	client.ecn(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем установление соединения по пути, доставляющему маркировку
	ASSERT_TRUE(::establish(client, server, now, nullptr, awh::event::ecn_t::ECT0));
	// Проверяем что проверка пути пройдена и маркировка сохранена
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "marked path payload", true), status_t::OK);
	/**
	 * Выполняем обмен датаграммами по пути, доставляющему маркировку
	 */
	for(size_t i = 0; i < 4; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу с маркировкой поддержки ECN
		::transfer(client, server, now, nullptr, awh::event::ecn_t::ECT0);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
	}
	// Проверяем что маркировка сохранена по завершении обмена
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные приняты сервером
	ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
	// Проверяем содержимое принятых данных
	ASSERT_EQ(payload, "marked path payload");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест выявления частичной потери маркировки на пути (RFC 9000 §13.4.2.1)
 *
 * @details Промежуточный узел вправе стирать маркировку не на всех датаграммах.
 *          Счётчики при этом возвращаются, но растут медленнее числа подтверждённых
 *          помеченных пакетов - этого достаточно, чтобы проверку не пройти
 */
TEST_F(QuicFixture, ConnectionEcnValidationPartialTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Включаем маркировку исходящих датаграмм клиента
	client.ecn(true);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем установление соединения по пути, доставляющему маркировку
	ASSERT_TRUE(::establish(client, server, now, nullptr, awh::event::ecn_t::ECT0));
	/**
	 * Проверяем что проверка пути пройдена: счётчики маркировок возвращены
	 * и выросли ровно на число помеченных пакетов хендшейка
	 */
	ASSERT_EQ(client.marking(), awh::event::ecn_t::ECT0);
	// Открываем двунаправленный поток на клиенте
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	/**
	 * Выполняем обмен датаграммами по пути, перестающему доставлять маркировку:
	 * счётчики сервера возвращаются прежними, а помеченные пакеты подтверждаются.
	 * Обмен ведётся несколькими шагами: по первому подтверждению накопленные
	 * счётчики принимаются за исходные, и расхождение проявляется со второго
	 */
	for(size_t i = 0; i < 4; i++){
		// Ставим очередную порцию данных в очередь отправки
		ASSERT_EQ(client.send(sid, "partially stripped path", false), status_t::OK);
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу без маркировки
		::transfer(client, server, now);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
	}
	// Проверяем что проверка пути не пройдена и маркировка снята
	ASSERT_EQ(client.marking(), awh::event::ecn_t::NOT_ECT);
	// Проверяем что непройденная проверка пути соединение не разорвала
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест выдачи и предъявления токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @details Сервер с включённой проверкой адреса выдаёт установленному соединению
 *          токен фреймом NEW_TOKEN. Предъявление токена в первом пакете следующего
 *          соединения подтверждает адрес клиента сразу, поэтому обмен пакетом
 *          Retry не выполняется и круг задержки экономится
 */
TEST_F(QuicFixture, ConnectionNewTokenTest){
	// Токен проверки адреса, выданный сервером
	std::string token = "";
	/**
	 * Выполняем первое соединение через пакет Retry ради токена
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Включаем проверку адреса клиента через пакет Retry
		server.retry(true);
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		// Проверяем что до соединения токен недоступен
		ASSERT_TRUE(client.token().empty());
		// Тестовые часы в миллисекундах
		uint64_t now = 1000;
		// Буфер первой датаграммы клиента
		std::string datagram = "";
		// Извлекаем первую датаграмму клиента с пакетом Initial без токена
		ASSERT_TRUE(client.write(datagram, now));
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграмму серверу
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
		// Буфер датаграммы сервера
		std::string retry = "";
		// Извлекаем датаграмму сервера с пакетом Retry
		ASSERT_TRUE(server.write(retry, now));
		// Проверяем что тип пакета сервера - Retry
		ASSERT_EQ((static_cast <uint8_t> (retry[0]) & 0x30) >> 4, 0x03);
		// Продвигаем тестовые часы
		now += 5;
		// Передаём пакет Retry клиенту
		ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (retry.data()), retry.size(), now), status_t::OK);
		// Выполняем полное установление соединения после обработки Retry
		ASSERT_TRUE(::establish(client, server, now));
		/**
		 * Прогоняем обмен после установления соединения: токен приходит уже
		 * по защищённому каналу отдельным фреймом
		 */
		for(size_t i = 0; i < 4; i++){
			// Продвигаем тестовые часы
			now += 5;
			// Передаём датаграммы сервера клиенту
			::transfer(server, client, now);
			// Передаём датаграммы клиента серверу
			::transfer(client, server, now);
		}
		// Извлекаем выданный сервером токен проверки адреса
		token = client.token();
		// Проверяем что токен проверки адреса получен от сервера
		ASSERT_FALSE(token.empty());
		// Проверяем что ошибки транспорта на обоих эндпоинтах нет
		ASSERT_EQ(client.error(), error_t::NO_ERROR);
		ASSERT_EQ(server.error(), error_t::NO_ERROR);
	}
	/**
	 * Выполняем второе соединение с предъявлением выданного токена
	 */
	{
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
		// Выполняем подготовку соединения клиента
		::setup(client);
		// Выполняем подготовку соединения сервера
		::setup(server);
		// Включаем проверку адреса клиента через пакет Retry
		server.retry(true);
		// Устанавливаем выданный сервером токен проверки адреса
		client.token(token);
		// Выполняем начало соединения клиентом
		ASSERT_EQ(client.connect(), status_t::OK);
		/**
		 * Тестовые часы второго соединения: отсчёт продолжается за первым соединением,
		 * поскольку токен несёт отметку времени выдачи и в прошлом выдан быть не мог
		 */
		uint64_t now = 5000;
		// Буфер первой датаграммы клиента
		std::string datagram = "";
		// Извлекаем первую датаграмму клиента с пакетом Initial и токеном
		ASSERT_TRUE(client.write(datagram, now));
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграмму серверу
		ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
		/**
		 * Проверяем что сервер начал хендшейк сразу: предъявленный токен подтвердил
		 * адрес клиента, и выдавать пакет Retry не потребовалось
		 */
		ASSERT_EQ(server.state(), connection_t::state_t::HANDSHAKING);
		// Буфер ответной датаграммы сервера
		std::string answer = "";
		// Извлекаем ответную датаграмму сервера
		ASSERT_TRUE(server.write(answer, now));
		// Проверяем что ответ сервера пакетом Retry не является
		ASSERT_NE((static_cast <uint8_t> (answer[0]) & 0x30) >> 4, 0x03);
		// Продвигаем тестовые часы
		now += 5;
		// Передаём ответную датаграмму клиенту
		ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (answer.data()), answer.size(), now), status_t::OK);
		// Выполняем полное установление соединения
		ASSERT_TRUE(::establish(client, server, now));
		// Проверяем состояние соединения обоих эндпоинтов
		ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
		ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
		// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
		ASSERT_EQ(client.error(), error_t::NO_ERROR);
		ASSERT_EQ(server.error(), error_t::NO_ERROR);
	}
}

/**
 * @brief Тест отказа в предъявленном токене проверки адреса (RFC 9000 §8.1.3)
 *
 * @details Токен, не прошедший проверку, адрес клиента не подтверждает: соединение
 *          продолжается обычным порядком через выдачу пакета Retry, а не
 *          отбрасыванием датаграммы
 */
TEST_F(QuicFixture, ConnectionNewTokenRejectTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Включаем проверку адреса клиента через пакет Retry
	server.retry(true);
	/**
	 * Устанавливаем поддельный токен проверки адреса: метка формата соответствует
	 * токену фрейма NEW_TOKEN, а код аутентичности подделан
	 */
	client.token(std::string(1, static_cast <char> (0x02)) + std::string(33, '\0'));
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Буфер первой датаграммы клиента
	std::string datagram = "";
	// Извлекаем первую датаграмму клиента с пакетом Initial и поддельным токеном
	ASSERT_TRUE(client.write(datagram, now));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём датаграмму серверу
	ASSERT_EQ(server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now), status_t::OK);
	// Проверяем что сервер соединение не начал - адрес клиента не подтверждён
	ASSERT_EQ(server.state(), connection_t::state_t::NONE);
	// Буфер датаграммы сервера
	std::string retry = "";
	// Извлекаем датаграмму сервера
	ASSERT_TRUE(server.write(retry, now));
	// Проверяем что сервер ответил пакетом Retry
	ASSERT_EQ((static_cast <uint8_t> (retry[0]) & 0x30) >> 4, 0x03);
	// Продвигаем тестовые часы
	now += 5;
	// Передаём пакет Retry клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (retry.data()), retry.size(), now), status_t::OK);
	// Выполняем полное установление соединения после обработки Retry
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения обоих эндпоинтов
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест переезда соединения на предпочтительный адрес сервера (RFC 9000 §9.6)
 *
 * @details Сервер вправе анонсировать транспортным параметром адрес, на который
 *          клиенту следует переехать после хендшейка. Переезд выполняется на
 *          выданный вместе с адресом идентификатор соединения, а достижимость
 *          нового пути подтверждается проверкой
 */
TEST_F(QuicFixture, ConnectionPreferredAddressTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Транспортные параметры сервера с предпочтительным адресом
	params::params_t options;
	// Устанавливаем лимит данных соединения
	options.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	options.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	options.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	options.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит числа двунаправленных потоков
	options.initialMaxStreamsBidi = 100;
	// Устанавливаем лимит числа однонаправленных потоков
	options.initialMaxStreamsUni = 100;
	// Устанавливаем флаг наличия предпочтительного адреса сервера
	options.hasPreferredAddress = true;
	// Устанавливаем предпочтительный IPv4-адрес сервера 192.0.2.10
	options.preferredAddress.ipv4[0] = 192;
	options.preferredAddress.ipv4[1] = 0;
	options.preferredAddress.ipv4[2] = 2;
	options.preferredAddress.ipv4[3] = 10;
	// Устанавливаем порт предпочтительного IPv4-адреса сервера
	options.preferredAddress.ipv4Port = 4433;
	// Устанавливаем длину идентификатора соединения предпочтительного адреса
	options.preferredAddress.cid.size = connection_t::LOCAL_CID_SIZE;
	/**
	 * Заполняем идентификатор соединения предпочтительного адреса
	 */
	for(uint8_t i = 0; i < connection_t::LOCAL_CID_SIZE; i++)
		// Устанавливаем очередной октет идентификатора соединения
		options.preferredAddress.cid.data[i] = static_cast <uint8_t> (0xA0 + i);
	/**
	 * Заполняем токен сброса без сохранения состояния предпочтительного адреса
	 */
	for(uint8_t i = 0; i < awh::quic::proto::RESET_TOKEN_SIZE; i++)
		// Устанавливаем очередной октет токена сброса
		options.preferredAddress.resetToken[i] = static_cast <uint8_t> (0xB0 + i);
	// Выполняем подготовку соединения сервера с предпочтительным адресом
	::configure(server, options);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Проверяем что до установления соединения переезд невозможен
	ASSERT_FALSE(client.relocatable());
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем что переезд на предпочтительный адрес возможен
	ASSERT_TRUE(client.relocatable());
	// Проверяем что сервер переезжать не вправе - адрес анонсирует он сам
	ASSERT_FALSE(server.relocatable());
	// Извлечённый предпочтительный адрес сервера
	std::string ip = "";
	// Извлечённый порт предпочтительного адреса сервера
	uint16_t port = 0;
	// Проверяем что адрес семейства IPv6 не анонсирован
	ASSERT_FALSE(client.preferred(true, ip, port));
	// Проверяем что адрес семейства IPv4 анонсирован
	ASSERT_TRUE(client.preferred(false, ip, port));
	// Проверяем длину извлечённого адреса
	ASSERT_EQ(ip.size(), 4u);
	// Проверяем содержимое извлечённого адреса
	ASSERT_EQ(static_cast <uint8_t> (ip[0]), 192);
	ASSERT_EQ(static_cast <uint8_t> (ip[1]), 0);
	ASSERT_EQ(static_cast <uint8_t> (ip[2]), 2);
	ASSERT_EQ(static_cast <uint8_t> (ip[3]), 10);
	// Проверяем извлечённый порт предпочтительного адреса
	ASSERT_EQ(port, 4433);
	// Запоминаем идентификатор соединения удалённого эндпоинта до переезда
	const cid_t before = client.dcid();
	// Количество выполненных смен пути до переезда
	const uint64_t migrations = client.migrations();
	// Выполняем переезд на предпочтительный адрес сервера
	ASSERT_TRUE(client.relocate());
	// Проверяем что идентификатор соединения удалённого эндпоинта сменён
	ASSERT_FALSE(client.dcid() == before);
	/**
	 * Проверяем что переезд выполнен на идентификатор предпочтительного адреса:
	 * именно его сервер выдал вместе с адресом (RFC 9000 §5.1.1)
	 */
	ASSERT_TRUE(client.dcid() == options.preferredAddress.cid);
	// Проверяем что смена пути учтена
	ASSERT_EQ(client.migrations(), migrations + 1);
	// Проверяем что повторный переезд невозможен
	ASSERT_FALSE(client.relocatable());
	ASSERT_FALSE(client.relocate());
	// Выполняем обмен датаграммами для подтверждения достижимости нового пути
	::pump(client, server, now);
	// Проверяем что достижимость нового пути подтверждена сервером
	ASSERT_TRUE(client.validated());
	// Открываем двунаправленный поток на переехавшем соединении
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "relocated connection", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные приняты сервером после переезда
	ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
	// Проверяем содержимое принятых данных
	ASSERT_EQ(payload, "relocated connection");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест ненадёжной доставки датаграмм приложения (RFC 9221)
 *
 * @details Датаграммы доставляются вне потоков: flow control им не подчиняется,
 *          потерянные повторно не отправляются. Отправка возможна только когда
 *          удалённый узел анонсировал приём транспортным параметром
 */
TEST_F(QuicFixture, ConnectionDatagramTest){
	/**
	 * @brief Функция подготовки транспортных параметров с поддержкой датаграмм
	 *
	 * @param limit предельный размер принимаемого фрейма DATAGRAM
	 * @return      транспортные параметры эндпоинта
	 */
	auto options = [](const uint64_t limit) noexcept -> params::params_t {
		// Транспортные параметры эндпоинта
		params::params_t result;
		// Устанавливаем лимит данных соединения
		result.initialMaxData = 1048576;
		// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
		result.initialMaxStreamDataBidiLocal = 262144;
		// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
		result.initialMaxStreamDataBidiRemote = 262144;
		// Устанавливаем лимит данных однонаправленных потоков
		result.initialMaxStreamDataUni = 262144;
		// Устанавливаем лимит числа двунаправленных потоков
		result.initialMaxStreamsBidi = 100;
		// Устанавливаем лимит числа однонаправленных потоков
		result.initialMaxStreamsUni = 100;
		// Устанавливаем предельный размер принимаемого фрейма DATAGRAM
		result.maxDatagramFrameSize = limit;
		// Выводим подготовленные транспортные параметры
		return result;
	};
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента с поддержкой датаграмм
	::configure(client, options(1200));
	// Выполняем подготовку соединения сервера с поддержкой датаграмм
	::configure(server, options(1200));
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Проверяем что до завершения хендшейка предел отправки неизвестен
	ASSERT_EQ(client.datagrams(), 0u);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем что предел отправки датаграмм анонсирован удалённым узлом
	ASSERT_GT(client.datagrams(), 0u);
	ASSERT_GT(server.datagrams(), 0u);
	// Буфер принятой датаграммы приложения
	std::string payload = "";
	// Проверяем что принятых датаграмм до обмена нет
	ASSERT_FALSE(server.datagram(payload));
	// Ставим датаграмму приложения в очередь отправки клиента
	ASSERT_EQ(client.datagram("unreliable payload"), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Проверяем что датаграмма приложения принята сервером
	ASSERT_TRUE(server.datagram(payload));
	// Проверяем содержимое принятой датаграммы приложения
	ASSERT_EQ(payload, "unreliable payload");
	// Проверяем что очередь принятых датаграмм опустела
	ASSERT_FALSE(server.datagram(payload));
	/**
	 * Выполняем встречную отправку: датаграммы передаются в обе стороны
	 * независимо от потоков
	 */
	ASSERT_EQ(server.datagram("server side payload"), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Проверяем что датаграмма приложения принята клиентом
	ASSERT_TRUE(client.datagram(payload));
	// Проверяем содержимое принятой датаграммы приложения
	ASSERT_EQ(payload, "server side payload");
	// Проверяем что датаграмма сверх анонсированного предела не принимается
	ASSERT_EQ(client.datagram(std::string(client.datagrams() + 1, 'x')), status_t::ERROR);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест отказа в датаграммах без анонса приёма (RFC 9221 §3)
 *
 * @details Эндпоинт, не анонсировавший приём датаграмм, их не принимает:
 *          отправка ему запрещена, а фрейм DATAGRAM в его адрес является
 *          нарушением протокола
 */
TEST_F(QuicFixture, ConnectionDatagramUnsupportedTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента без поддержки датаграмм
	::setup(client);
	// Выполняем подготовку соединения сервера без поддержки датаграмм
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем что предел отправки датаграмм нулевой на обоих эндпоинтах
	ASSERT_EQ(client.datagrams(), 0u);
	ASSERT_EQ(server.datagrams(), 0u);
	// Проверяем что постановка датаграммы в очередь отправки отвергается
	ASSERT_EQ(client.datagram("unsupported"), status_t::ERROR);
	ASSERT_EQ(server.datagram("unsupported"), status_t::ERROR);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест сброса без сохранения состояния на общем ключе (RFC 9000 §10.3.2)
 *
 * @details Токен сброса выводится из идентификатора соединения на общем ключе,
 *          поэтому воспроизводится и после утраты состояния соединения: сервер,
 *          забывший о соединении, сбрасывает его немедленно, а не оставляет
 *          удалённый узел ждать таймаута простоя
 */
TEST_F(QuicFixture, ConnectionStatelessResetKeyTest){
	// Общий ключ вывода токенов сброса без сохранения состояния
	std::string key = "";
	// Проверяем что генерация общего ключа выполнена
	ASSERT_TRUE(awh::quic::resetKey(key));
	// Проверяем что общий ключ не пустой
	ASSERT_FALSE(key.empty());
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Устанавливаем общий ключ вывода токенов сброса соединению сервера
	server.resetKey(key);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Проверяем состояние соединения клиента
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	/**
	 * Запоминаем идентификатор соединения сервера: именно ему клиент адресует
	 * датаграммы, и именно на него сервер отправит сброс
	 */
	const cid_t target = client.dcid();
	// Буфер пакета сброса без сохранения состояния
	std::string reset = "";
	/**
	 * Собираем пакет сброса вне соединения: сервер о соединении уже ничего
	 * не помнит и располагает только идентификатором из принятой датаграммы
	 */
	ASSERT_TRUE(awh::quic::reset(reset, key, target, connection_t::MAX_DATAGRAM_SIZE));
	// Проверяем что пакет сброса меньше вызвавшей его датаграммы (RFC 9000 §10.3.3)
	ASSERT_LT(reset.size(), connection_t::MAX_DATAGRAM_SIZE);
	// Проверяем что первый октет пакета соответствует короткому заголовку
	ASSERT_EQ(static_cast <uint8_t> (reset[0]) & 0xC0, 0x40);
	// Продвигаем тестовые часы
	now += 5;
	// Передаём пакет сброса клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (reset.data()), reset.size(), now), status_t::OK);
	/**
	 * Проверяем что клиент распознал сброс и перешёл к завершению молча: отправка
	 * любых пакетов далее запрещена (RFC 9000 §10.3)
	 */
	ASSERT_EQ(client.state(), connection_t::state_t::DRAINING);
	// Буфер исходящей датаграммы клиента
	std::string datagram = "";
	// Проверяем что клиент отправку прекратил
	ASSERT_FALSE(client.write(datagram, now));
}

/**
 * @brief Тест непригодности чужого токена сброса (RFC 9000 §10.3.2)
 *
 * @details Токен выводится на общем ключе, поэтому собранный на другом ключе
 *          сброс соединение не разрывает: иначе разорвать чужое соединение
 *          мог бы любой, знающий идентификатор
 */
TEST_F(QuicFixture, ConnectionStatelessResetForeignTest){
	// Общий ключ вывода токенов сброса соединения сервера
	std::string key = "";
	// Чужой ключ вывода токенов сброса
	std::string foreign = "";
	// Проверяем что генерация обоих ключей выполнена
	ASSERT_TRUE(awh::quic::resetKey(key));
	ASSERT_TRUE(awh::quic::resetKey(foreign));
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Устанавливаем общий ключ вывода токенов сброса соединению сервера
	server.resetKey(key);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	// Буфер пакета сброса без сохранения состояния
	std::string reset = "";
	// Собираем пакет сброса на чужом ключе
	ASSERT_TRUE(awh::quic::reset(reset, foreign, client.dcid(), connection_t::MAX_DATAGRAM_SIZE));
	// Продвигаем тестовые часы
	now += 5;
	// Передаём пакет сброса клиенту
	ASSERT_EQ(client.read(reinterpret_cast <const uint8_t *> (reset.data()), reset.size(), now), status_t::OK);
	// Проверяем что соединение чужим сбросом не разорвано
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест ограничения размера сброса без сохранения состояния (RFC 9000 §10.3.3)
 *
 * @details Сброс обязан быть меньше вызвавшей его датаграммы: иначе два эндпоинта,
 *          утративших состояние, отвечали бы друг другу сбросами неограниченно долго
 */
TEST_F(QuicFixture, ConnectionStatelessResetBoundsTest){
	// Общий ключ вывода токенов сброса
	std::string key = "";
	// Проверяем что генерация общего ключа выполнена
	ASSERT_TRUE(awh::quic::resetKey(key));
	// Идентификатор соединения получателя
	cid_t cid;
	// Устанавливаем длину идентификатора соединения
	cid.size = connection_t::LOCAL_CID_SIZE;
	/**
	 * Заполняем идентификатор соединения получателя
	 */
	for(uint8_t i = 0; i < connection_t::LOCAL_CID_SIZE; i++)
		// Устанавливаем очередной октет идентификатора соединения
		cid.data[i] = static_cast <uint8_t> (0x10 + i);
	// Буфер пакета сброса без сохранения состояния
	std::string reset = "";
	// Проверяем что на слишком малую датаграмму сброс не собирается
	ASSERT_FALSE(awh::quic::reset(reset, key, cid, 21));
	// Проверяем что на датаграмму чуть большего размера сброс собирается
	ASSERT_TRUE(awh::quic::reset(reset, key, cid, 32));
	// Проверяем что собранный сброс меньше вызвавшей его датаграммы
	ASSERT_LT(reset.size(), 32u);
	// Проверяем что сброс не короче минимума для неотличимости от пакета 1-RTT
	ASSERT_GE(reset.size(), 21u);
	// Проверяем что без общего ключа сброс не собирается
	ASSERT_FALSE(awh::quic::reset(reset, "", cid, 1200));
	// Выведенный токен сброса без сохранения состояния
	uint8_t token[awh::quic::proto::RESET_TOKEN_SIZE];
	// Проверяем что вывод токена на общем ключе выполнен
	ASSERT_TRUE(awh::quic::resetToken(key, cid, token));
	/**
	 * Проверяем что токен воспроизводится: на этом и держится сброс без сохранения
	 * состояния - сервер выводит его заново, ничего не помня о соединении
	 */
	uint8_t repeat[awh::quic::proto::RESET_TOKEN_SIZE];
	// Выполняем повторный вывод токена сброса
	ASSERT_TRUE(awh::quic::resetToken(key, cid, repeat));
	// Проверяем что повторно выведенный токен совпадает
	ASSERT_EQ(::memcmp(token, repeat, awh::quic::proto::RESET_TOKEN_SIZE), 0);
	// Проверяем что без общего ключа вывод токена невозможен
	ASSERT_FALSE(awh::quic::resetToken("", cid, token));
}

/**
 * @brief Тест запрета активной миграции соединения (RFC 9000 §18.2)
 *
 * @details Эндпоинт, анонсировавший запрет, не поддерживает смену локального
 *          адреса удалённым узлом: активная миграция ему запрещена. На переезд
 *          по анонсированному предпочтительному адресу запрет не распространяется
 */
TEST_F(QuicFixture, ConnectionDisableActiveMigrationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Транспортные параметры сервера с запретом активной миграции
	params::params_t options;
	// Устанавливаем лимит данных соединения
	options.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	options.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	options.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	options.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит числа двунаправленных потоков
	options.initialMaxStreamsBidi = 100;
	// Устанавливаем лимит числа однонаправленных потоков
	options.initialMaxStreamsUni = 100;
	// Устанавливаем запрет активной миграции соединения
	options.disableActiveMigration = true;
	// Выполняем подготовку соединения сервера с запретом активной миграции
	::configure(server, options);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Прогоняем обмен для доставки анонсов дополнительных идентификаторов:
	 * без неиспользованного идентификатора миграция невозможна и по другой причине
	 */
	::pump(client, server, now);
	// Количество выполненных смен пути до попытки миграции
	const uint64_t migrations = client.migrations();
	// Запоминаем идентификатор соединения удалённого эндпоинта до попытки миграции
	const cid_t before = client.dcid();
	// Проверяем что активная миграция отвергнута запретом удалённого узла
	ASSERT_FALSE(client.migrate());
	// Проверяем что идентификатор соединения удалённого эндпоинта не сменён
	ASSERT_TRUE(client.dcid() == before);
	// Проверяем что смена пути не выполнена
	ASSERT_EQ(client.migrations(), migrations);
	/**
	 * Проверяем что ротация идентификатора запретом не затронута: она к смене
	 * пути отношения не имеет и служит защитой от связывания путей
	 */
	ASSERT_TRUE(client.rotate());
	// Проверяем что соединение работоспособно после отвергнутой миграции
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Открываем двунаправленный поток на соединении
	const uint64_t sid = client.open(false);
	// Проверяем что поток открыт
	ASSERT_NE(sid, connection_t::INVALID_STREAM);
	// Ставим данные в очередь отправки
	ASSERT_EQ(client.send(sid, "no migration allowed", true), status_t::OK);
	// Выполняем обмен датаграммами
	::pump(client, server, now);
	// Буфер принятых сервером данных
	std::string payload = "";
	// Флаг завершения потока
	bool fin = false;
	// Проверяем что данные приняты сервером
	ASSERT_EQ(server.receive(sid, payload, fin), status_t::OK);
	// Проверяем содержимое принятых данных
	ASSERT_EQ(payload, "no migration allowed");
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест поиска размера пути зондированием (RFC 8899)
 *
 * @details Соединение начинает с размера датаграммы, который обязан пропускать
 *          любой путь, и наращивает его зондами. Подтверждённый зонд поднимает
 *          подтверждённый размер, потерянный - опускает верхнюю границу поиска
 */
TEST_F(QuicFixture, ConnectionPmtuDiscoveryTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Проверяем что до соединения размер пути равен размеру, пропускаемому любым путём
	ASSERT_EQ(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Прогоняем обмен: путь тестового транспорта датаграммы не ограничивает,
	 * поэтому зонды проходят и подтверждённый размер растёт
	 */
	for(size_t i = 0; i < 12; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу
		::transfer(client, server, now);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
	}
	/**
	 * Проверяем что подтверждённый размер пути вырос: путь пропускает датаграммы
	 * больше обязательного минимума, и зондирование это установило
	 */
	ASSERT_GT(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	// Проверяем что подтверждённый размер пути не превышает предела поиска
	ASSERT_LE(client.pmtu(), connection_t::MAX_PROBE_SIZE);
	// Запоминаем найденный размер пути до миграции
	const size_t before = client.pmtu();
	// Проверяем что найденный размер пути превышает обязательный минимум
	ASSERT_GT(before, connection_t::MAX_DATAGRAM_SIZE);
	// Выполняем миграцию соединения на новый путь
	ASSERT_TRUE(client.migrate());
	/**
	 * Проверяем что размер пути сброшен к обязательному минимуму: найденный размер
	 * относился к прежнему пути, и на новом он неприменим (RFC 8899 §5.4)
	 */
	ASSERT_EQ(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест сохранения заданной границы поиска размера пути при миграции (RFC 8899 §5.1)
 *
 * @details Смена пути начинает поиск размера заново, но заданное вызывающим кодом
 *          ограничение относится не к пути, а к самому соединению, поэтому
 *          обязано применяться и на новом пути
 */
TEST_F(QuicFixture, ConnectionPmtuLimitMigrationTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Заданная вызывающим кодом верхняя граница поиска размера пути
	static constexpr size_t LIMIT = 1280;
	// Устанавливаем верхнюю границу поиска размера пути
	client.pmtu(LIMIT);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Выполняем полное установление соединения
	ASSERT_TRUE(::establish(client, server, now));
	/**
	 * Прогоняем обмен: путь тестового транспорта датаграммы не ограничивает,
	 * поэтому поиск упирается в заданную границу, а не в размер пути
	 */
	for(size_t i = 0; i < 12; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу
		::transfer(client, server, now);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
	}
	// Проверяем что подтверждённый размер пути заданную границу не превышает
	ASSERT_LE(client.pmtu(), LIMIT);
	// Проверяем что поиск до заданной границы всё же дошёл
	ASSERT_GT(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	// Выполняем миграцию соединения на новый путь
	ASSERT_TRUE(client.migrate());
	// Проверяем что размер пути сброшен к обязательному минимуму
	ASSERT_EQ(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	/**
	 * Прогоняем обмен на новом пути: поиск начат заново и обязан упереться
	 * в ту же заданную вызывающим кодом границу
	 */
	for(size_t i = 0; i < 24; i++){
		// Продвигаем тестовые часы
		now += 5;
		// Передаём датаграммы клиента серверу
		::transfer(client, server, now);
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
		// Выполняем обработку просроченных таймеров клиента
		client.tick(now);
		// Выполняем обработку просроченных таймеров сервера
		server.tick(now);
	}
	// Проверяем что поиск на новом пути возобновился
	ASSERT_GT(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	/**
	 * Проверяем что заданная граница пережила смену пути: её утрата позволила бы
	 * поиску подняться до предельного размера зонда
	 */
	ASSERT_LE(client.pmtu(), LIMIT);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест схождения поиска размера пути на узком пути (RFC 8899 §5.3)
 *
 * @details Путь, не пропускающий датаграммы сверх своего размера, обнаруживается
 *          по потере зондов: верхняя граница поиска опускается, и подтверждённый
 *          размер не превышает пропускаемого путём
 */
TEST_F(QuicFixture, ConnectionPmtuNarrowPathTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Размер датаграммы, пропускаемой узким путём
	static constexpr size_t NARROW = 1300;
	/**
	 * Прогоняем обмен через узкий путь с самого начала, включая установление
	 * соединения: датаграммы клиента сверх размера пути до сервера не доходят,
	 * поэтому зонды такого размера теряются. Пакеты хендшейка ограничены
	 * размером, который обязан пропускать любой путь, и проходят
	 */
	for(size_t i = 0; i < 40; i++){
		// Продвигаем тестовые часы
		now += 25;
		// Буфер передаваемой датаграммы
		std::string datagram = "";
		/**
		 * Передаём датаграммы клиента серверу через узкий путь
		 */
		while(client.write(datagram, now)){
			// Если датаграмма помещается в размер узкого пути
			if(datagram.size() <= NARROW)
				// Передаём датаграмму серверу
				server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
		}
		// Продвигаем тестовые часы
		now += 25;
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
		// Выполняем обработку просроченных таймеров клиента
		client.tick(now);
		// Выполняем обработку просроченных таймеров сервера
		server.tick(now);
	}
	// Проверяем что соединение через узкий путь установлено
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	ASSERT_EQ(server.state(), connection_t::state_t::CONNECTED);
	/**
	 * Проверяем что подтверждённый размер пути узкий путь не превышает: зонды
	 * большего размера терялись, и верхняя граница поиска опускалась
	 */
	ASSERT_LE(client.pmtu(), NARROW);
	/**
	 * Проверяем что поиск всё же нашёл размер больше обязательного минимума: узкий
	 * путь шире минимума, и остановка на минимуме означала бы, что поиск после
	 * потери зонда не продолжился
	 */
	ASSERT_GT(client.pmtu(), connection_t::MAX_DATAGRAM_SIZE);
	// Проверяем что соединение узким путём не разорвано
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест независимости окна перегрузки от потери зондов размера пути (RFC 9000 §14.4)
 *
 * @details Зонд теряется потому, что не помещается в путь, а не из-за затора,
 *          поэтому его потеря событием перегрузки не является и окно отправителя
 *          сокращать не вправе
 */
TEST_F(QuicFixture, ConnectionPmtuCongestionTest){
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, ::security().context(endpoint_t::CLIENT), ::security().coder(), &::logger());
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, ::security().context(endpoint_t::SERVER), ::security().coder(), &::logger());
	// Выполняем подготовку соединения клиента
	::setup(client);
	// Выполняем подготовку соединения сервера
	::setup(server);
	// Выполняем начало соединения клиентом
	ASSERT_EQ(client.connect(), status_t::OK);
	// Тестовые часы в миллисекундах
	uint64_t now = 1000;
	// Размер датаграммы, пропускаемой узким путём
	static constexpr size_t NARROW = 1250;
	/**
	 * Прогоняем обмен через узкий путь: зонды размера пути теряются, а обычные
	 * датаграммы доходят - потерь, вызванных затором, на пути нет
	 */
	for(size_t i = 0; i < 30; i++){
		// Продвигаем тестовые часы
		now += 25;
		// Буфер передаваемой датаграммы
		std::string datagram = "";
		/**
		 * Передаём датаграммы клиента серверу через узкий путь
		 */
		while(client.write(datagram, now)){
			// Если датаграмма помещается в размер узкого пути
			if(datagram.size() <= NARROW)
				// Передаём датаграмму серверу
				server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
		}
		// Продвигаем тестовые часы
		now += 25;
		// Передаём датаграммы сервера клиенту
		::transfer(server, client, now);
		// Выполняем обработку просроченных таймеров клиента
		client.tick(now);
		// Выполняем обработку просроченных таймеров сервера
		server.tick(now);
	}
	// Проверяем что соединение через узкий путь установлено
	ASSERT_EQ(client.state(), connection_t::state_t::CONNECTED);
	/**
	 * Проверяем что окно перегрузки не схлопнуто: единственные потери на пути -
	 * это зонды размера пути, а они событием перегрузки не являются
	 */
	ASSERT_GE(client.cwnd(), 12000u);
	// Проверяем отсутствие ошибки транспорта на обоих эндпоинтах
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}
