/**
 * @file: handshake.cpp
 * @date: 2026-07-22
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

/**
 * Подключаем заголовочный файл тестов кодера
 */
#include "tls.hpp"

/**
 * Подписываемся на пространство имён проекта
 */
using namespace awh;

/**
 * @brief Внутренние вспомогательные средства тестов рукопожатия
 *
 */
namespace {
	/**
	 * @brief Структура эндпоинта тестового обмена
	 *
	 * @details Кодер отдаёт исходящий шифртекст функцией обратного вызова чтения
	 *          с событием шифрования, а входящий принимает методом расшифровки.
	 *          Эндпоинт накапливает исходящий шифртекст и выданный приложению
	 *          открытый текст, что позволяет прогнать рукопожатие без сокетов
	 */
	typedef struct Endpoint {
		// Идентификатор шаблона контекста безопасности
		tls::Coder::id_t context;
		// Идентификатор транспортного уровня передачи
		tls::Coder::id_t transport;
		// Накопленный исходящий шифртекст
		std::string outgoing;
		// Накопленный принятый открытый текст
		std::string incoming;
		// Флаг выполненного рукопожатия
		bool handshaked;
		// Флаг отказа рукопожатия
		bool failed;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Endpoint() noexcept :
		 context(0), transport(0), handshaked(false), failed(false) {}
	} endpoint_t;

	/**
	 * @brief Функция подключения функций обратного вызова эндпоинта
	 *
	 * @param coder    объект кодера транспортной безопасности
	 * @param endpoint эндпоинт тестового обмена
	 */
	static void subscribe(tls::Coder & coder, endpoint_t & endpoint) noexcept {
		// Устанавливаем функцию обратного вызова чтения
		coder.on(endpoint.transport, [&endpoint](const tls::Coder::id_t, const tls::Coder::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Определяем тип события кодера
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие является шифрованием - данные готовы к отправке пиру
				case static_cast <uint8_t> (tls::Coder::event_t::ENCRYPTION):
					// Накапливаем исходящий шифртекст
					endpoint.outgoing.append(reinterpret_cast <const char *> (buffer), size);
				break;
				// Если событие является расшифровкой - данные предназначены приложению
				case static_cast <uint8_t> (tls::Coder::event_t::DECRYPTION):
					// Накапливаем принятый открытый текст
					endpoint.incoming.append(reinterpret_cast <const char *> (buffer), size);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова изменения состояния
		coder.on(endpoint.transport, [&endpoint](const tls::Coder::id_t, const tls::Coder::state_t state) noexcept -> void {
			/**
			 * Определяем состояние кодера
			 */
			switch(static_cast <uint8_t> (state)){
				// Если рукопожатие выполнено
				case static_cast <uint8_t> (tls::Coder::state_t::HANDSHAKED):
					// Устанавливаем флаг выполненного рукопожатия
					endpoint.handshaked = true;
				break;
				// Если рукопожатие завершилось отказом
				case static_cast <uint8_t> (tls::Coder::state_t::HANDSHAKE_FAILED):
				// Если работа завершилась ошибкой
				case static_cast <uint8_t> (tls::Coder::state_t::FAILED):
					// Устанавливаем флаг отказа рукопожатия
					endpoint.failed = true;
				break;
			}
		});
	}
	/**
	 * @brief Функция передачи накопленного шифртекста между эндпоинтами
	 *
	 * @param coder объект кодера транспортной безопасности
	 * @param from  эндпоинт-отправитель
	 * @param to    эндпоинт-получатель
	 * @return      количество переданных октетов
	 */
	static size_t transfer(tls::Coder & coder, endpoint_t & from, endpoint_t & to) noexcept {
		// Если исходящих данных нет
		if(from.outgoing.empty())
			// Выводим нулевое количество переданных октетов
			return 0;
		// Забираем накопленный исходящий шифртекст
		const std::string buffer = from.outgoing;
		// Очищаем буфер исходящего шифртекста
		from.outgoing.clear();
		// Передаём шифртекст получателю на расшифровку
		coder.decrypt(to.transport, buffer.data(), buffer.size());
		// Выводим количество переданных октетов
		return buffer.size();
	}
	/**
	 * @brief Функция выполнения рукопожатия между эндпоинтами
	 *
	 * @param coder  объект кодера транспортной безопасности
	 * @param client эндпоинт клиента
	 * @param server эндпоинт сервера
	 * @return       результат выполнения рукопожатия
	 */
	static bool establish(tls::Coder & coder, endpoint_t & client, endpoint_t & server) noexcept {
		/**
		 * Инициируем рукопожатие на обоих эндпоинтах однократно: далее оно
		 * продвигается подачей принятого шифртекста на расшифровку, как это
		 * устроено в рабочих примерах модуля
		 */
		coder.handshake(client.transport);
		// Инициируем рукопожатие сервера
		coder.handshake(server.transport);
		/**
		 * Выполняем обмен до завершения рукопожатия на обоих эндпоинтах
		 */
		for(size_t i = 0; i < 32; i++){
			// Передаём исходящий шифртекст клиента серверу
			const size_t sent = ::transfer(coder, client, server);
			// Передаём исходящий шифртекст сервера клиенту
			const size_t received = ::transfer(coder, server, client);
			// Если рукопожатие выполнено на обоих эндпоинтах
			if(client.handshaked && server.handshaked)
				// Выводим положительный результат
				return true;
			// Если рукопожатие завершилось отказом
			if(client.failed || server.failed)
				// Выводим отрицательный результат
				return false;
			// Если обмен данными прекратился
			if((sent == 0) && (received == 0))
				// Выводим отрицательный результат - обмен не сошёлся
				return false;
		}
		// Выводим отрицательный результат - обмен не сошёлся
		return false;
	}
};

/**
 * @brief Тест полного рукопожатия между клиентом и сервером
 *
 * @details Прогон без сокетов: шифртекст переносится между эндпоинтами вручную.
 *          Тест задействует весь тракт кодера - создание контекстов, настройку
 *          сертификата, функции обратного вызова уровня контекста и передачу
 *          прикладных данных после рукопожатия
 */
TEST_F(TlsFixture, HandshakeLoopbackTest){
	// Проверяем что сертификат тестового узла сгенерирован
	ASSERT_FALSE(this->_certificate.empty());
	// Эндпоинт клиента
	::endpoint_t client;
	// Эндпоинт сервера
	::endpoint_t server;
	// Создаём шаблон контекста безопасности клиента
	client.context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Создаём шаблон контекста безопасности сервера
	server.context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблоны контекста созданы
	ASSERT_NE(client.context, 0u);
	ASSERT_NE(server.context, 0u);
	// Устанавливаем сертификат сервера
	this->_coder->certificate(server.context, this->_certificate);
	// Устанавливаем приватный ключ сервера
	this->_coder->privateKey(server.context, this->_privateKey);
	// Устанавливаем доверенный центр сертификации клиента
	this->_coder->ca(client.context, this->_certificate);
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder->serverNameIndication(client.context, "localhost");
	/**
	 * Снимаем проверку сертификата на сервере: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию. Тесты
	 * проверяют односторонний TLS, поэтому требование снимается явно
	 */
	this->_coder->validateServerNameIndication(server.context, false);
	// Создаём транспортный уровень клиента
	client.transport = this->_coder->transport(client.context);
	// Создаём транспортный уровень сервера
	server.transport = this->_coder->transport(server.context);
	// Проверяем что транспортные уровни созданы
	ASSERT_NE(client.transport, 0u);
	ASSERT_NE(server.transport, 0u);
	// Подключаем функции обратного вызова клиента
	::subscribe(* this->_coder, client);
	// Подключаем функции обратного вызова сервера
	::subscribe(* this->_coder, server);
	// Выполняем рукопожатие между эндпоинтами
	ASSERT_TRUE(::establish(* this->_coder, client, server));
	// Проверяем что рукопожатие выполнено на обоих эндпоинтах
	ASSERT_TRUE(client.handshaked);
	ASSERT_TRUE(server.handshaked);
	// Проверяем что согласованный шифр доступен
	ASSERT_FALSE(this->_coder->cipherInfo(client.transport).empty());
	// Передаваемое прикладное сообщение
	const std::string message = "прикладные данные поверх установленного соединения";
	// Шифруем прикладное сообщение на клиенте
	ASSERT_TRUE(this->_coder->encrypt(client.transport, message.data(), message.size()));
	// Передаём шифртекст серверу
	ASSERT_GT(::transfer(* this->_coder, client, server), 0u);
	// Проверяем что сервер принял сообщение без искажений
	ASSERT_EQ(server.incoming, message);
	// Выполняем удаление созданных объектов
	ASSERT_TRUE(this->_coder->destroy(client.transport));
	ASSERT_TRUE(this->_coder->destroy(server.transport));
	ASSERT_TRUE(this->_coder->destroy(client.context));
	ASSERT_TRUE(this->_coder->destroy(server.context));
}

/**
 * @brief Тест согласования протокола приложения при рукопожатии
 *
 * @details Задействует функцию обратного вызова выбора протокола на сервере -
 *          одну из устанавливаемых на шаблон контекста безопасности
 */
TEST_F(TlsFixture, HandshakeAlpnTest){
	// Эндпоинт клиента
	::endpoint_t client;
	// Эндпоинт сервера
	::endpoint_t server;
	// Создаём шаблоны контекста безопасности
	client.context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	server.context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблоны контекста созданы
	ASSERT_NE(client.context, 0u);
	ASSERT_NE(server.context, 0u);
	// Устанавливаем сертификат и приватный ключ сервера
	this->_coder->certificate(server.context, this->_certificate);
	this->_coder->privateKey(server.context, this->_privateKey);
	// Устанавливаем доверенный центр сертификации клиента
	this->_coder->ca(client.context, this->_certificate);
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder->serverNameIndication(client.context, "localhost");
	// Устанавливаем список протоколов приложения клиента
	this->_coder->alpn(client.context, {tls::Coder::alpn_t{0, "h2"}, tls::Coder::alpn_t{0, "http/1.1"}});
	// Устанавливаем список протоколов приложения сервера
	this->_coder->alpn(server.context, {tls::Coder::alpn_t{0, "h2"}});
	/**
	 * Снимаем проверку сертификата на сервере: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию. Тесты
	 * проверяют односторонний TLS, поэтому требование снимается явно
	 */
	this->_coder->validateServerNameIndication(server.context, false);
	// Создаём транспортные уровни
	client.transport = this->_coder->transport(client.context);
	server.transport = this->_coder->transport(server.context);
	// Проверяем что транспортные уровни созданы
	ASSERT_NE(client.transport, 0u);
	ASSERT_NE(server.transport, 0u);
	// Подключаем функции обратного вызова эндпоинтов
	::subscribe(* this->_coder, client);
	::subscribe(* this->_coder, server);
	// Выполняем рукопожатие между эндпоинтами
	ASSERT_TRUE(::establish(* this->_coder, client, server));
	// Проверяем что рукопожатие выполнено на обоих эндпоинтах
	ASSERT_TRUE(client.handshaked);
	ASSERT_TRUE(server.handshaked);
	// Выполняем удаление созданных объектов
	ASSERT_TRUE(this->_coder->destroy(client.transport));
	ASSERT_TRUE(this->_coder->destroy(server.transport));
	ASSERT_TRUE(this->_coder->destroy(client.context));
	ASSERT_TRUE(this->_coder->destroy(server.context));
}

/**
 * @brief Тест отказа рукопожатия при недоверенном сертификате
 *
 * @details Клиент без указанного доверенного центра сертификации не должен
 *          принимать самоподписанный сертификат сервера
 */
TEST_F(TlsFixture, HandshakeUntrustedCertificateTest){
	// Эндпоинт клиента
	::endpoint_t client;
	// Эндпоинт сервера
	::endpoint_t server;
	// Создаём шаблоны контекста безопасности
	client.context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	server.context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблоны контекста созданы
	ASSERT_NE(client.context, 0u);
	ASSERT_NE(server.context, 0u);
	// Устанавливаем сертификат и приватный ключ сервера
	this->_coder->certificate(server.context, this->_certificate);
	this->_coder->privateKey(server.context, this->_privateKey);
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder->serverNameIndication(client.context, "localhost");
	/**
	 * Снимаем проверку сертификата на сервере: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию. Тест проверяет
	 * односторонний TLS, поэтому требование снимается явно
	 */
	this->_coder->validateServerNameIndication(server.context, false);
	/**
	 * Доверенный центр сертификации клиенту намеренно не задаётся: сертификат
	 * сервера самоподписанный и в системном хранилище отсутствует
	 */
	client.transport = this->_coder->transport(client.context);
	// Создаём транспортный уровень сервера
	server.transport = this->_coder->transport(server.context);
	// Проверяем что транспортные уровни созданы
	ASSERT_NE(client.transport, 0u);
	ASSERT_NE(server.transport, 0u);
	// Подключаем функции обратного вызова эндпоинтов
	::subscribe(* this->_coder, client);
	::subscribe(* this->_coder, server);
	// Проверяем что рукопожатие не выполнено
	ASSERT_FALSE(::establish(* this->_coder, client, server));
	// Проверяем что рукопожатие клиента не завершилось успехом
	ASSERT_FALSE(client.handshaked);
	/**
	 * Транспортные уровни после неудачного рукопожатия удалению не подлежат:
	 * кодер помечает их на удаление самостоятельно, поэтому повторный вызов
	 * отвергается. Удаляются только шаблоны контекста
	 */
	ASSERT_TRUE(this->_coder->destroy(client.context));
	ASSERT_TRUE(this->_coder->destroy(server.context));
}
