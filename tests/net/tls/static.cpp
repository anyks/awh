/**
 * @file: static.cpp
 * @date: 2026-07-22
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
 * Подключаем заголовочный файл тестов кодера
 */
#include "tls.hpp"

/**
 * Подписываемся на пространство имён кодера транспортной безопасности
 */
using namespace awh;

/**
 * @brief Тест создания шаблонов контекста безопасности
 *
 * @details Шаблон контекста - самостоятельный объект с собственным
 *          идентификатором: на нём выполняется вся настройка криптографии,
 *          а транспортные уровни создаются из него отдельно
 */
TEST_F(TlsFixture, ContextCreateTest){
	// Создаём шаблон контекста безопасности клиента поверх TCP
	const tls::Coder::id_t client = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Проверяем что шаблон контекста клиента создан
	ASSERT_NE(client, 0u);
	// Создаём шаблон контекста безопасности сервера поверх TCP
	const tls::Coder::id_t server = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблон контекста сервера создан
	ASSERT_NE(server, 0u);
	// Проверяем что идентификаторы шаблонов различаются
	ASSERT_NE(client, server);
	// Создаём шаблон контекста безопасности клиента поверх UDP
	const tls::Coder::id_t datagram = this->_coder->context(event::node_t::CLIENT, event::protocol_t::UDP);
	// Проверяем что шаблон контекста поверх UDP создан
	ASSERT_NE(datagram, 0u);
	// Выполняем удаление созданных шаблонов контекста
	ASSERT_TRUE(this->_coder->destroy(client));
	ASSERT_TRUE(this->_coder->destroy(server));
	ASSERT_TRUE(this->_coder->destroy(datagram));
}

/**
 * @brief Тест создания транспортных уровней из шаблона контекста
 *
 * @details Из одного шаблона контекста создаётся произвольное количество
 *          транспортных уровней: настройка выполняется однократно и
 *          наследуется всеми соединениями
 */
TEST_F(TlsFixture, TransportCreateTest){
	// Создаём шаблон контекста безопасности клиента
	const tls::Coder::id_t context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Создаём первый транспортный уровень из шаблона контекста
	const tls::Coder::id_t first = this->_coder->transport(context);
	// Проверяем что транспортный уровень создан
	ASSERT_NE(first, 0u);
	// Создаём второй транспортный уровень из того же шаблона контекста
	const tls::Coder::id_t second = this->_coder->transport(context);
	// Проверяем что второй транспортный уровень создан
	ASSERT_NE(second, 0u);
	// Проверяем что транспортные уровни различаются
	ASSERT_NE(first, second);
	// Проверяем что транспортный уровень отличается от шаблона контекста
	ASSERT_NE(first, context);
	// Выполняем удаление созданных объектов
	ASSERT_TRUE(this->_coder->destroy(first));
	ASSERT_TRUE(this->_coder->destroy(second));
	ASSERT_TRUE(this->_coder->destroy(context));
}

/**
 * @brief Тест получения нативного контекста криптографической библиотеки
 *
 * @details Нативный контекст выдаётся протоколам, которые ведут собственный
 *          обмен данными поверх настроенного контекста и транспортным уровнем
 *          кодера не пользуются
 */
TEST_F(TlsFixture, NativeContextTest){
	// Проверяем что нативный контекст несуществующего идентификатора не выдаётся
	ASSERT_EQ(this->_coder->native(0), nullptr);
	// Создаём шаблон контекста безопасности клиента
	const tls::Coder::id_t context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Проверяем что нативный контекст шаблона выдаётся
	ASSERT_NE(this->_coder->native(context), nullptr);
	// Создаём транспортный уровень из шаблона контекста
	const tls::Coder::id_t transport = this->_coder->transport(context);
	// Проверяем что транспортный уровень создан
	ASSERT_NE(transport, 0u);
	// Проверяем что нативный контекст транспортного уровня совпадает с контекстом шаблона
	ASSERT_EQ(this->_coder->native(transport), this->_coder->native(context));
	// Выполняем удаление созданных объектов
	ASSERT_TRUE(this->_coder->destroy(transport));
	ASSERT_TRUE(this->_coder->destroy(context));
}

/**
 * @brief Тест настройки сертификата и приватного ключа
 *
 * @details Настройка выполняется на шаблоне контекста до создания транспортных
 *          уровней. Некорректные пути обрабатываются без аварийного завершения
 */
TEST_F(TlsFixture, CertificateSetupTest){
	// Проверяем что сертификат тестового узла сгенерирован
	ASSERT_FALSE(this->_certificate.empty());
	ASSERT_FALSE(this->_privateKey.empty());
	// Создаём шаблон контекста безопасности сервера
	const tls::Coder::id_t context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Устанавливаем сертификат тестового узла
	this->_coder->certificate(context, this->_certificate);
	// Устанавливаем приватный ключ тестового узла
	this->_coder->privateKey(context, this->_privateKey);
	// Устанавливаем несуществующий сертификат - обработка обязана быть безопасной
	this->_coder->certificate(context, "/nonexistent/path/to/certificate.pem");
	// Устанавливаем несуществующий приватный ключ
	this->_coder->privateKey(context, "/nonexistent/path/to/private.key");
	// Устанавливаем пустой путь к сертификату
	this->_coder->certificate(context, "");
	// Проверяем что шаблон контекста остался работоспособным
	ASSERT_NE(this->_coder->native(context), nullptr);
	// Выполняем удаление шаблона контекста
	ASSERT_TRUE(this->_coder->destroy(context));
}

/**
 * @brief Тест настройки доверенных центров сертификации
 *
 * @details Проверяются обе формы вызова и устойчивость к несуществующим путям
 */
TEST_F(TlsFixture, CertificateAuthoritySetupTest){
	// Создаём шаблон контекста безопасности клиента
	const tls::Coder::id_t context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Устанавливаем доверенный центр сертификации файлом
	this->_coder->ca(context, this->_certificate);
	// Устанавливаем доверенный центр сертификации каталогом и файлом
	this->_coder->ca(context, "", this->_certificate);
	// Устанавливаем несуществующий доверенный центр сертификации
	this->_coder->ca(context, "/nonexistent/path/to/ca.pem");
	// Устанавливаем пустой путь к доверенному центру сертификации
	this->_coder->ca(context, "");
	// Проверяем что шаблон контекста остался работоспособным
	ASSERT_NE(this->_coder->native(context), nullptr);
	// Выполняем удаление шаблона контекста
	ASSERT_TRUE(this->_coder->destroy(context));
}

/**
 * @brief Тест настройки согласования протокола приложения и доменного имени
 *
 * @details ALPN и SNI настраиваются на шаблоне контекста и применяются
 *          ко всем созданным из него транспортным уровням
 */
TEST_F(TlsFixture, NegotiationSetupTest){
	// Создаём шаблон контекста безопасности клиента
	const tls::Coder::id_t context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Проверяем что шаблон контекста создан
	ASSERT_NE(context, 0u);
	// Устанавливаем список протоколов приложения
	this->_coder->alpn(context, {tls::Coder::alpn_t{0, "h2"}, tls::Coder::alpn_t{0, "http/1.1"}});
	// Устанавливаем доменное имя удалённого узла
	this->_coder->serverNameIndication(context, "example.com");
	// Проверяем что доменное имя сохранено
	ASSERT_EQ(this->_coder->serverNameIndication(context), "example.com");
	// Устанавливаем режим единственного сертификата
	this->_coder->mode(context, tls::Coder::mode_t::UNICERT);
	// Устанавливаем проверку доменного имени удалённого узла
	this->_coder->validateServerNameIndication(context, true);
	// Проверяем что шаблон контекста остался работоспособным
	ASSERT_NE(this->_coder->native(context), nullptr);
	// Выполняем удаление шаблона контекста
	ASSERT_TRUE(this->_coder->destroy(context));
}

/**
 * @brief Тест устойчивости методов настройки к недопустимым идентификаторам
 *
 * @details Методы настройки и управления жизненным циклом закрепляют объект
 *          в реестре и обязаны отвергать несуществующий идентификатор.
 *
 *          Методы горячего пути - handshake(), encrypt() и decrypt() - под эту
 *          проверку намеренно не подпадают: поиск идентификатора в реестре на
 *          каждой итерации обмена стоит дороже, чем даёт, а вызов с мёртвым
 *          контекстом означает нарушение контракта вызывающим кодом и
 *          неработоспособность приложения в целом. Тест это учитывает и таких
 *          вызовов не делает
 */
TEST_F(TlsFixture, InvalidIdentifierTest){
	// Недопустимый идентификатор объекта
	const tls::Coder::id_t invalid = 0;
	// Проверяем отказ создания транспортного уровня по недопустимому идентификатору
	ASSERT_EQ(this->_coder->transport(invalid), 0u);
	// Проверяем отказ получения нативного контекста
	ASSERT_EQ(this->_coder->native(invalid), nullptr);
	// Проверяем отказ удаления несуществующего объекта
	ASSERT_FALSE(this->_coder->destroy(invalid));
	// Выполняем настройку несуществующего объекта - обработка обязана быть безопасной
	this->_coder->ca(invalid, this->_certificate);
	// Выполняем установку сертификата несуществующего объекта
	this->_coder->certificate(invalid, this->_certificate);
	// Выполняем установку доменного имени несуществующего объекта
	this->_coder->serverNameIndication(invalid, "example.com");
	// Проверяем что доменное имя несуществующего объекта пустое
	ASSERT_TRUE(this->_coder->serverNameIndication(invalid).empty());
}
