/**
 * @file: handshake.cpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
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
#include "../../../include/proto/quic/handshake.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic;

/**
 * @brief Внутренние вспомогательные функции тестов хендшейка
 *
 */
namespace {
	/**
	 * @brief Функция передачи исходящих CRYPTO-данных одного эндпоинта другому
	 *
	 * @param from эндпоинт-отправитель CRYPTO-данных
	 * @param to   эндпоинт-получатель CRYPTO-данных
	 * @return     результат передачи (false - ошибка обработки данных получателем)
	 */
	static bool transfer(handshake_t & from, handshake_t & to) noexcept {
		// Список уровней шифрования в порядке возрастания
		static const level_t levels[] = {level_t::INITIAL, level_t::EARLY_DATA, level_t::HANDSHAKE, level_t::APPLICATION};
		/**
		 * Перебираем список уровней шифрования
		 */
		for(auto & level : levels){
			// Если у отправителя есть исходящие CRYPTO-данные уровня
			if(from.pending(level)){
				// Извлекаем исходящие CRYPTO-данные уровня
				const std::string data = from.data(level);
				// Передаём CRYPTO-данные получателю
				if(to.crypto(level, reinterpret_cast <const uint8_t *> (data.data()), data.size()) != status_t::OK)
					// Выводим отрицательный результат
					return false;
			}
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция выполнения полного хендшейка между клиентом и сервером
	 *
	 * @param client эндпоинт клиента
	 * @param server эндпоинт сервера
	 * @return       результат выполнения хендшейка
	 */
	static bool complete(handshake_t & client, handshake_t & server) noexcept {
		/**
		 * Выполняем обмен данными хендшейка (с запасом итераций)
		 */
		for(size_t i = 0; i < 10; i++){
			// Передаём CRYPTO-данные клиента серверу
			if(!::transfer(client, server))
				// Выводим отрицательный результат
				return false;
			// Передаём CRYPTO-данные сервера клиенту
			if(!::transfer(server, client))
				// Выводим отрицательный результат
				return false;
			// Если хендшейк завершён на обоих эндпоинтах и данных для передачи нет
			if((client.state() == handshake_t::state_t::COMPLETED) &&
			   (server.state() == handshake_t::state_t::COMPLETED) &&
			   !client.pending(level_t::INITIAL) && !client.pending(level_t::HANDSHAKE) &&
			   !server.pending(level_t::INITIAL) && !server.pending(level_t::HANDSHAKE))
				// Выводим положительный результат
				return true;
		}
		// Выводим отрицательный результат - хендшейк не сошёлся
		return false;
	}
	/**
	 * @brief Функция подготовки эндпоинта хендшейка со стандартными настройками
	 *
	 * @note Криптография задана шаблоном контекста кодера, из которого создан
	 *       эндпоинт, поэтому подготовка сводится к транспортным параметрам
	 *
	 * @param handshake эндпоинт хендшейка
	 * @return          результат подготовки
	 */
	static bool setup(handshake_t & handshake) noexcept {
		// Транспортные параметры эндпоинта
		params::params_t params;
		// Устанавливаем лимит данных соединения
		params.initialMaxData = 1048576;
		// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
		params.initialMaxStreamDataBidiLocal = 262144;
		// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
		params.initialMaxStreamDataBidiRemote = 262144;
		// Устанавливаем лимит числа двунаправленных потоков
		params.initialMaxStreamsBidi = 100;
		// Устанавливаем транспортные параметры и выводим результат
		return handshake.params(params);
	}
};

/**
 * @brief Тест полного in-memory хендшейка клиента и сервера
 *
 */
TEST_F(QuicFixture, HandshakeCompleteTest){
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, this->_security->context(endpoint_t::CLIENT), this->_security->coder(), this->_log.get());
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Выполняем начало хендшейка на сервере
	ASSERT_EQ(server.start(), status_t::OK);
	// Выполняем начало хендшейка на клиенте
	ASSERT_EQ(client.start(), status_t::OK);
	// Проверяем что клиент сформировал ClientHello на уровне Initial
	ASSERT_TRUE(client.pending(level_t::INITIAL));
	// Выполняем полный хендшейк
	ASSERT_TRUE(::complete(client, server));
	// Проверяем состояние хендшейка клиента
	ASSERT_EQ(client.state(), handshake_t::state_t::COMPLETED);
	// Проверяем состояние хендшейка сервера
	ASSERT_EQ(server.state(), handshake_t::state_t::COMPLETED);
	// Проверяем отсутствие ошибки транспорта на клиенте
	ASSERT_EQ(client.error(), error_t::NO_ERROR);
	// Проверяем отсутствие ошибки транспорта на сервере
	ASSERT_EQ(server.error(), error_t::NO_ERROR);
}

/**
 * @brief Тест согласования ALPN-протокола
 *
 */
TEST_F(QuicFixture, HandshakeAlpnTest){
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, this->_security->context(endpoint_t::CLIENT), this->_security->coder(), this->_log.get());
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Выполняем начало хендшейка на обоих эндпоинтах
	ASSERT_EQ(server.start(), status_t::OK);
	ASSERT_EQ(client.start(), status_t::OK);
	// Выполняем полный хендшейк
	ASSERT_TRUE(::complete(client, server));
	// Проверяем согласованный ALPN-протокол на клиенте
	ASSERT_EQ(client.alpn().protocol, "h3");
	// Проверяем согласованный ALPN-протокол на сервере
	ASSERT_EQ(server.alpn().protocol, "h3");
}

/**
 * @brief Тест обмена транспортными параметрами (RFC 9000 §7.4)
 *
 */
TEST_F(QuicFixture, HandshakeTransportParamsTest){
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, this->_security->context(endpoint_t::CLIENT), this->_security->coder(), this->_log.get());
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Транспортные параметры удалённого узла
	params::params_t params;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Проверяем что до хендшейка параметры удалённого узла недоступны
	ASSERT_EQ(client.peer(params, error), status_t::INCOMPLETE);
	// Выполняем начало хендшейка на обоих эндпоинтах
	ASSERT_EQ(server.start(), status_t::OK);
	ASSERT_EQ(client.start(), status_t::OK);
	// Выполняем полный хендшейк
	ASSERT_TRUE(::complete(client, server));
	// Извлекаем транспортные параметры сервера на клиенте
	ASSERT_EQ(client.peer(params, error), status_t::OK);
	// Проверяем лимит данных соединения сервера
	ASSERT_EQ(params.initialMaxData, 1048576);
	// Проверяем лимит числа двунаправленных потоков сервера
	ASSERT_EQ(params.initialMaxStreamsBidi, 100);
	// Извлекаем транспортные параметры клиента на сервере
	ASSERT_EQ(server.peer(params, error), status_t::OK);
	// Проверяем лимит данных соединения клиента
	ASSERT_EQ(params.initialMaxData, 1048576);
	// Проверяем лимит данных удалённо инициируемых двунаправленных потоков клиента
	ASSERT_EQ(params.initialMaxStreamDataBidiRemote, 262144);
}

/**
 * @brief Тест вывода и согласованности ключей защиты пакетов по уровням
 *
 */
TEST_F(QuicFixture, HandshakeKeysTest){
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, this->_security->context(endpoint_t::CLIENT), this->_security->coder(), this->_log.get());
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Формируем DCID первого пакета Initial клиента (RFC 9001 §A)
	const cid_t dcid = this->makeCid(this->unhex("8394c8f03e515708"));
	// Выполняем вывод ключей уровня Initial на клиенте
	ASSERT_TRUE(client.initial(dcid));
	// Выполняем вывод ключей уровня Initial на сервере
	ASSERT_TRUE(server.initial(dcid));
	// Извлекаем ключи защиты исходящих пакетов Initial клиента
	const crypto::keys_t * write = client.encryption(level_t::INITIAL);
	// Извлекаем ключи снятия защиты входящих пакетов Initial сервера
	const crypto::keys_t * read = server.decryption(level_t::INITIAL);
	// Проверяем наличие ключей Initial обоих эндпоинтов
	ASSERT_NE(write, nullptr);
	ASSERT_NE(read, nullptr);
	// Проверяем совпадение секретов направления клиент-сервер (RFC 9001 §A.1)
	ASSERT_EQ(this->hex(write->secret), "c00cf151ca5be075ed0ebfb5c80323c42d6b7db67881289af4008f1f6c357aea");
	ASSERT_EQ(write->secret, read->secret);
	ASSERT_EQ(write->key, read->key);
	ASSERT_EQ(write->iv, read->iv);
	ASSERT_EQ(write->hp, read->hp);
	// Выполняем начало хендшейка на обоих эндпоинтах
	ASSERT_EQ(server.start(), status_t::OK);
	ASSERT_EQ(client.start(), status_t::OK);
	// Проверяем что до хендшейка ключи уровня Handshake отсутствуют
	ASSERT_EQ(client.encryption(level_t::HANDSHAKE), nullptr);
	// Выполняем полный хендшейк
	ASSERT_TRUE(::complete(client, server));
	// Проверяем наличие ключей уровня Handshake обоих направлений
	ASSERT_NE(client.encryption(level_t::HANDSHAKE), nullptr);
	ASSERT_NE(client.decryption(level_t::HANDSHAKE), nullptr);
	ASSERT_NE(server.encryption(level_t::HANDSHAKE), nullptr);
	ASSERT_NE(server.decryption(level_t::HANDSHAKE), nullptr);
	// Проверяем наличие ключей уровня приложения обоих направлений
	ASSERT_NE(client.encryption(level_t::APPLICATION), nullptr);
	ASSERT_NE(client.decryption(level_t::APPLICATION), nullptr);
	ASSERT_NE(server.encryption(level_t::APPLICATION), nullptr);
	ASSERT_NE(server.decryption(level_t::APPLICATION), nullptr);
	// Проверяем согласованность ключей уровня Handshake между эндпоинтами
	ASSERT_EQ(client.encryption(level_t::HANDSHAKE)->secret, server.decryption(level_t::HANDSHAKE)->secret);
	ASSERT_EQ(client.decryption(level_t::HANDSHAKE)->secret, server.encryption(level_t::HANDSHAKE)->secret);
	// Проверяем согласованность ключей уровня приложения между эндпоинтами
	ASSERT_EQ(client.encryption(level_t::APPLICATION)->key, server.decryption(level_t::APPLICATION)->key);
	ASSERT_EQ(client.decryption(level_t::APPLICATION)->key, server.encryption(level_t::APPLICATION)->key);
	// Проверяем что секреты направлений различаются
	ASSERT_NE(client.encryption(level_t::APPLICATION)->secret, client.decryption(level_t::APPLICATION)->secret);
	// Выполняем сброс ключей уровня Initial (RFC 9001 §4.9)
	client.discard(level_t::INITIAL);
	// Проверяем что ключи уровня Initial сброшены
	ASSERT_EQ(client.encryption(level_t::INITIAL), nullptr);
	ASSERT_EQ(client.decryption(level_t::INITIAL), nullptr);
}

/**
 * @brief Тест ошибки согласования ALPN-протокола (RFC 9001 §8.1)
 *
 */
TEST_F(QuicFixture, HandshakeAlpnMismatchTest){
	// Создаём шаблон контекста клиента с несовместимым списком ALPN-протоколов
	const awh::tls::Coder::id_t context = this->_security->make(endpoint_t::CLIENT, {awh::tls::Coder::alpn_t{0, "hq-interop"}});
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, context, this->_security->coder(), this->_log.get());
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Выполняем начало хендшейка на обоих эндпоинтах
	ASSERT_EQ(server.start(), status_t::OK);
	ASSERT_EQ(client.start(), status_t::OK);
	// Извлекаем ClientHello клиента
	const std::string data = client.data(level_t::INITIAL);
	// Передаём ClientHello серверу - хендшейк должен завершиться ошибкой
	ASSERT_EQ(server.crypto(level_t::INITIAL, reinterpret_cast <const uint8_t *> (data.data()), data.size()), status_t::ERROR);
	// Проверяем состояние ошибки хендшейка сервера
	ASSERT_EQ(server.state(), handshake_t::state_t::FAILED);
	// Проверяем что код ошибки транспорта находится в диапазоне CRYPTO_ERROR (RFC 9001 §4.8)
	ASSERT_GE(static_cast <uint64_t> (server.error()), static_cast <uint64_t> (error_t::CRYPTO_ERROR));
	ASSERT_LE(static_cast <uint64_t> (server.error()), (static_cast <uint64_t> (error_t::CRYPTO_ERROR) + 0xFF));
	// Удаляем созданный шаблон контекста безопасности
	ASSERT_TRUE(this->_security->coder().destroy(context));
}

/**
 * @brief Тест ошибки проверки самоподписанного сертификата клиентом
 *
 */
TEST_F(QuicFixture, HandshakeVerifyFailedTest){
	// Получаем объект кодера транспортной безопасности
	awh::tls::Coder & coder = this->_security->coder();
	/**
	 * Создаём шаблон контекста клиента вручную: общий шаблон окружения содержит
	 * тестовый сертификат в качестве доверенного якоря, а здесь проверяется
	 * отказ проверки самоподписанного сертификата без него
	 */
	const awh::tls::Coder::id_t context = coder.context(awh::event::node_t::CLIENT, awh::event::protocol_t::QUIC);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Устанавливаем список поддерживаемых ALPN-протоколов
	coder.alpn(context, {awh::tls::Coder::alpn_t{0, "h3"}});
	// Устанавливаем доменное имя удалённого сервера
	coder.serverNameIndication(context, "localhost");
	// Включаем проверку сертификата удалённого узла без доверенных центров
	coder.validateServerNameIndication(context, true);
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, context, coder, this->_log.get());
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), coder, this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Выполняем начало хендшейка на обоих эндпоинтах
	ASSERT_EQ(server.start(), status_t::OK);
	ASSERT_EQ(client.start(), status_t::OK);
	// Выполняем обмен данными хендшейка - хендшейк должен завершиться ошибкой
	ASSERT_FALSE(::complete(client, server));
	// Проверяем состояние ошибки хендшейка клиента
	ASSERT_EQ(client.state(), handshake_t::state_t::FAILED);
	// Проверяем что клиент зарегистрировал ошибку транспорта
	ASSERT_NE(client.error(), error_t::NO_ERROR);
	// Удаляем созданный шаблон контекста безопасности
	ASSERT_TRUE(coder.destroy(context));
}

/**
 * @brief Тест недопустимых операций хендшейка
 *
 */
TEST_F(QuicFixture, HandshakeMisuseTest){
	// Создаём эндпоинт клиента без транспортных параметров
	handshake_t empty(endpoint_t::CLIENT, this->_security->context(endpoint_t::CLIENT), this->_security->coder(), this->_log.get());
	// Проверяем что начало хендшейка без транспортных параметров невозможно
	ASSERT_EQ(empty.start(), status_t::ERROR);
	// Создаём эндпоинт клиента
	handshake_t client(endpoint_t::CLIENT, this->_security->context(endpoint_t::CLIENT), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта клиента
	ASSERT_TRUE(::setup(client));
	// Тестовые данные CRYPTO-фрейма (несуществующий тип сообщения хендшейка TLS)
	const uint8_t data[4] = {0xFF, 0x00, 0x00, 0x00};
	// Проверяем что обработка CRYPTO-данных до начала хендшейка невозможна
	ASSERT_EQ(client.crypto(level_t::INITIAL, data, sizeof(data)), status_t::ERROR);
	// Выполняем начало хендшейка на клиенте
	ASSERT_EQ(client.start(), status_t::OK);
	// Проверяем что повторное начало хендшейка невозможно
	ASSERT_EQ(client.start(), status_t::ERROR);
	// Создаём эндпоинт сервера
	handshake_t server(endpoint_t::SERVER, this->_security->context(endpoint_t::SERVER), this->_security->coder(), this->_log.get());
	// Выполняем подготовку эндпоинта сервера
	ASSERT_TRUE(::setup(server));
	// Выполняем начало хендшейка на сервере
	ASSERT_EQ(server.start(), status_t::OK);
	// Передаём серверу мусорные данные вместо ClientHello
	ASSERT_EQ(server.crypto(level_t::INITIAL, data, sizeof(data)), status_t::ERROR);
	// Проверяем состояние ошибки хендшейка сервера
	ASSERT_EQ(server.state(), handshake_t::state_t::FAILED);
}
