/**
 * @file: static.cpp
 * @date: 2025-12-15
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * @brief Генерация случайного порта в диапазоне 49152-65535
 *
 * @return случайный порт
 */
static uint16_t port() noexcept {
	/**
	 * Инициализация генератора случайных чисел
	 */
	::srandom(::time(nullptr) ^ ::getpid());
	/**
	 * Возвращаем случайный порт из диапазона 49152-65535
	 */
	return (49152 + (::random() % (65535 - 49152 + 1)));
}

/**
 * @brief Тест создания объекта работы со списком параметров URL
 *
 */
TEST_F(IoFixture, CreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Сбрасываем объект асинхронного движка ввода-вывода
	this->_io.reset();
	// Проверяем, что объект асинхронного движка ввода-вывода сброшен
	ASSERT_TRUE(this->_io == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта асинхронного движка ввода-вывода
 *
 */
TEST_F(IoFixture, ResetAndCreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Сбрасываем объект асинхронного движка ввода-вывода
	this->_io.reset();
	// Проверяем, что объект асинхронного движка ввода-вывода сброшен
	ASSERT_TRUE(this->_io == nullptr);
	// Создаём объект асинхронного движка ввода-вывода заново
	this->_io = std::make_unique <awh::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @brief Тест повторного создания объекта асинхронного движка ввода-вывода
 *
 */
TEST_F(IoFixture, ReCreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Создаём объект асинхронного движка ввода-вывода заново
	this->_io = std::make_unique <awh::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(IoFixture, IoSuiteTest){
	// Устанавливаем логгер
	this->_fmk->setLogger(this->_log.get());
	// Создаём объект работы с Ethernet
	awh::eth_t eth(this->_fmk.get(), this->_log.get());
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	eth.fillsource(source);
	// Проверяем, что название сетевого интерфейса получено
	ASSERT_FALSE(source.iface.empty());
	// Если сетевой интерфейс не принадлежит к VPN
	if(::memcmp("ut", source.iface.c_str(), 2) != 0){
		// MAC-адрес и IP-адрес сетевого интерфейса
		std::string mac = "", ip = "";
		/**
		 * IPv4 событие
		 */
		{
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid1 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid1, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->iface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid1));

			/**
			 * Для операционной системы FreeBSD
			 */
			#if __FreeBSD__
				// Извлекаем информационные метаданные SCTP сообщения
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid1);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Устанавливаем информационные метаданные SCTP сообщения
				this->_sctp->messageInfo(eid1, minfo);
				// Извлекаем статус SCTP событий SCTP
				const awh::net::sctp::status_t & status = this->_sctp->status(eid1);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;

				// Текст инициализационных сообщений SCTP
				awh::net::sctp::initmsg_t initmsg;
				// Устанавливаем количество попыток подключения SCTP
				initmsg.attempts = 4;
				// Устанавливаем количество исходящих потоков SCTP
				initmsg.ostreams = 5;
				// Устанавливаем количество входящих потоков SCTP
				initmsg.istreams = 5;
				// Инициализируем сообщения SCTP
				this->_sctp->initMessages(eid1, initmsg);
				// Типы SCTP событий для подписки
				awh::net::sctp::event_types_t types = {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR,
					awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
				};
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid1, types);

				// Проверяем что типы SCTP событий совпадают с установленными ранее
				ASSERT_EQ(types, this->_sctp->eventsSubscribed(eid1));

				// Устанавливаем таймаут INIT SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::INIT, 3000));
				// Проверяем что таймаут INIT SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::INIT));

				// Устанавливаем таймаут DATA SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::DATA, 3000));
				// Проверяем что таймаут DATA SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::DATA));

				// Устанавливаем таймаут SACK SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::SACK, 3000));
				// Проверяем что таймаут SACK SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::SACK));

				// Устанавливаем таймаут COOKIE SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::COOKIE, 3000));
				// Проверяем что таймаут COOKIE SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::COOKIE));

				// Устанавливаем таймаут SHUTDOWN SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::SHUTDOWN, 3000));
				// Проверяем что таймаут SHUTDOWN SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::SHUTDOWN));

				// Устанавливаем таймаут HEARTBEAT SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
				// Проверяем что таймаут HEARTBEAT SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::HEARTBEAT));

				// Устанавливаем таймаут SHUTDOWNACK SCTP события
				ASSERT_TRUE(this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::SHUTDOWNACK, 3000));
				// Проверяем что таймаут SHUTDOWNACK SCTP события получен
				ASSERT_EQ(3000, this->_sctp->timeout(eid1, awh::net::sctp::timeout_t::SHUTDOWNACK));

				// Устанавливаем ключ аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateKey(eid1, 1, "0123456789abcdef0123456789abcdef"));
				// Устанавливаем режим использования ключа аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateKey(eid1, awh::event::mode_t::ENABLED, 1));
				// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(eid1, {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
				// Устанавливаем чанки аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(eid1, {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));

				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(eid1, awh::event::origin_t::LOCAL, chunks));
				// Проверяем что чанки аутентификации SCTP-сокета получены
				ASSERT_FALSE(chunks.empty());
				// Перебираем все извлечённые чанки
				for(auto & chunk : chunks)
					// Выводим информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			#endif

			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->address(eid1, awh::event::address_t::IPV4);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->address(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid2, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::IPV4, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid3, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::NETWORK, ip + "/255.255.255.0"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid4).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid4));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid4, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid4, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid5 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid5, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->port(eid5, 8080));
			// Устанавливаем UDS-адрес события
			ASSERT_TRUE(this->_io->address(eid5, awh::event::address_t::UDS, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->iface(eid5).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->address(eid5, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->address(eid5, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid4).empty());
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->address(eid5, awh::event::address_t::UDS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid6 = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid6, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->port(eid6, 8080));
			// Устанавливаем сетевой адрес события
			ASSERT_TRUE(this->_io->address(eid6, awh::event::address_t::FS, "/tmp/awh.txt"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->iface(eid6).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->address(eid6, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->address(eid6, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid6).empty());
			// Проверяем, что адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.txt", this->_io->address(eid6, awh::event::address_t::FS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->target(eid7, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid7).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid7));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid7, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid7, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->target(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid7, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid8 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid8, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->port(eid8, 8080));
			// Устанавливаем UDS-адрес назначения для события
			ASSERT_TRUE(this->_io->target(eid8, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->iface(eid8).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->address(eid8, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->address(eid8, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_EQ("/tmp/awh.sock", this->_io->target(eid8));
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->address(eid8, awh::event::address_t::UDS));
		}
		/**
		 * IPv6 событие
		 */
		{
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid1 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid1, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->iface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid1));
			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->address(eid1, awh::event::address_t::IPV6);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->address(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid2, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::IPV6, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid3, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::NETWORK, ip + "/64"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid4).empty());
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->address(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->target(eid7, ip));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->address(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->target(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid7, awh::event::address_t::UDS).empty());
		}
	}
}

/**
 * @brief Тест проверки работы TCP-соединения
 *
 */
TEST_F(IoFixture, IoTCPTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-событий асинхронного ввода-вывода
 *
 */
TEST_F(IoFixture, IoUDPTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::IPV4).c_str(), this->_io->port(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Количество прочитанных сообщений
		uint8_t count = 0;
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&count, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			// Останавливаем тест
			stop = (++count >= 10);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
		// Текст исходящего сообщения
		std::string message("Hello from async client message!");
		// Отправляем данные обратно клиенту
		ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.size()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-соединения
 *
 */
TEST_F(IoFixture, IoUDPConnectTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::IPV4).c_str(), this->_io->port(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDS-соединения
 *
 */
TEST_F(IoFixture, IoUDSTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие клиента TCP
	awh::event::id_t cid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM);
	// Добавляем новое событие клиента TCP
	awh::event::id_t sid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::STREAM);
	// Проверяем корректность создания событий
	ASSERT_GT(cid, 0);
	ASSERT_GT(sid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->options(sid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->target(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->address(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(sid, static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s", awh::log_t::flag_t::INFO, sid, cid, this->_io->address(cid, awh::event::address_t::UDS).c_str());
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(sid, awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(sid));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(sid, 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(sid));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid, true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(cid));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP UDS-соединения
 *
 */
TEST_F(IoFixture, IoUDPUDSTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие клиента TCP
	awh::event::id_t cid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::DATAGRAM);
	// Добавляем новое событие клиента TCP
	awh::event::id_t sid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::DATAGRAM);
	// Проверяем корректность создания событий
	ASSERT_GT(cid, 0);
	ASSERT_GT(sid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->options(sid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->target(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->address(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(sid, static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::UDS).c_str());
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(sid, awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(sid));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(sid));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid, true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(cid));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы широковещательного события
 *
 */
TEST_F(IoFixture, IoBroadcastTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::IPV4).c_str(), this->_io->port(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::BROADCAST));
		// Создаём объект работы с Ethernet
		awh::eth_t eth(this->_fmk.get(), this->_log.get());
		// Временный объект для извлечения сетевого интерфейса
		awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
		// Выполняем извлечение сетевых параметров
		eth.fillsource(source);
		// Если сетевой интерфейс не принадлежит к VPN
		if(::memcmp("ut", source.iface.c_str(), 2) != 0){
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->iface(events[0], source.iface));
			// Создаём объект сетевого адреса
			awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
			// Извлекаем IP-адрес сетевого интерфейса
			addr = std::move(this->_io->address(events[0], awh::event::address_t::IPV4));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(source.iface.empty());
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Формируем адрес Broadcast
			addr.v4((addr.v4(awh::net_addr_t::endian_t::BIG) & 0xFFFFFF00U) | 0x000000FFU, awh::net_addr_t::endian_t::BIG);
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->target(events[0], static_cast <std::string> (addr)));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
			// Текст исходящего сообщения
			const std::string message("Hello from async client!");
			// Отправляем данные обратно клиенту
			ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.size()));
		// Если сетевой интерфейс принадлежит к VPN
		} else {
			// Выводим сообщение о пропуске теста для VPN-интерфейса
			this->_log->print("Пропуск теста для VPN-интерфейса: %s", awh::log_t::flag_t::WARNING, source.iface.c_str());
			// Устанавливаем флаг остановки теста
			stop = true;
		}
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-соединения с перехватом данных из файла
 *
 */
TEST_F(IoFixture, IoUDPSpliceConnectTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие отслеживания файла
	awh::event::id_t fid = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем адрес текстового файла для чтения
	ASSERT_TRUE(this->_io->address(fid, awh::event::address_t::FS, "../README.md"));
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::IPV4).c_str(), this->_io->port(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Выполняем объединение двух сокетов
		ASSERT_TRUE(this->_io->splice(fid, events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([fid, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Выполняем фиксацию события файла
				ASSERT_TRUE(this->_io->commit(fid));
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(fid, awh::event::options::KEEPALIVE));
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы файловой системой I/O событий
 *
 */
TEST_F(IoFixture, IoFsTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие отслеживания каталога
	awh::event::id_t did = this->_io->event(awh::event::node_t::DIR, awh::event::family_t::FSYS);
	// Добавляем новое событие отслеживания файла
	awh::event::id_t fid = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
	// Проверяем идентификаторы созданных событий
	ASSERT_GT(did, 0);
	ASSERT_GT(fid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Событие каталога
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на изменение события
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action, const awh::event::vnode_t vnode, const std::string & path) noexcept -> void {
			/**
			 * Обрабатываем тип узла события
			 */
			switch(static_cast <uint8_t> (vnode)){
				// Если тип узла не определён
				case static_cast <uint8_t> (awh::event::vnode_t::NONE):
					// Выводим сообщение о типе узла события
					this->_log->print("Тип узла события: Не определён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
				break;
				case static_cast <uint8_t> (awh::event::vnode_t::CHR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Символьный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Символьный узел устройства удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				case static_cast <uint8_t> (awh::event::vnode_t::BLK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Блочный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Блочный узел устройства удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каналом FIFO
				case static_cast <uint8_t> (awh::event::vnode_t::FIFO): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Канал FIFO добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Канал FIFO удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является сокетом
				case static_cast <uint8_t> (awh::event::vnode_t::SOCK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Сокет добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Сокет удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является файлом
				case static_cast <uint8_t> (awh::event::vnode_t::FILE): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Файл добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Файл удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каталогом
				case static_cast <uint8_t> (awh::event::vnode_t::DIR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Каталог добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение о типе узла события
							this->_log->print("Тип узла события: Каталог удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является символической ссылкой
				case static_cast <uint8_t> (awh::event::vnode_t::LINK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Символическая ссылка добавлена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение о типе узла события
							this->_log->print("Тип узла события: Символическая ссылка удалена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому каталогу
		ASSERT_TRUE(this->_io->address(did, awh::event::address_t::FS, "./"));
		// Выполняем фиксацию настроек события каталога
		ASSERT_TRUE(this->_io->commit(did));
		// Запускаем событие каталога
		ASSERT_TRUE(this->_io->launch(did));
	}
	/**
	 * Событие файла
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(fid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING): {
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					// Устанавливаем смещение в файле
					// this->_io->seek(eid, 1024);
					// Отправляем тестовое сообщение в файл
					this->_io->send(eid, "Hello World!!!", 14);
				} break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(fid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(fid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Удаляем файл
				::remove("tmp.txt");
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Удаляем файл
				::unlink("./tmp.txt");
			#endif
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(fid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому файлу
		ASSERT_TRUE(this->_io->address(fid, awh::event::address_t::FS, "./tmp.txt"));
		// Выполняем фиксацию настроек события файла
		ASSERT_TRUE(this->_io->commit(fid));
		// Устанавливаем опции события
		ASSERT_TRUE(this->_io->options(fid, awh::event::options::KEEPALIVE));
		// Запускаем событие файла
		ASSERT_TRUE(this->_io->launch(fid));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы пользовательских событий
 *
 */
TEST_F(IoFixture, IoEventsTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие отслеживания каталога
	awh::event::id_t eid = this->_io->event(awh::event::node_t::NOTIFY, awh::event::family_t::USER);
	// Проверяем инициализацию
	ASSERT_GT(eid, 0);
	// Проверяем, что идентификатор события больше нуля
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (status)){
			// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
		}
	});
	// Устанавливаем функцию обратного вызова на запись в событие
	this->_io->on(eid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
		// Выводим сообщение о переподключении события
		this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
	}));
	// Устанавливаем функцию обратного вызова на чтение из события
	this->_io->on(eid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const std::string message(reinterpret_cast <const char *> (data), size);
		// Выводим сообщение о переподключении события
		this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
		// Останавливаем тест
		stop = true;
	});
	// Устанавливаем функцию обратного вызова на ошибку события
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (error)){
			// Если ошибка неизвестного события
			case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
				// Выводим сообщение об ошибке неизвестного события
				this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недопустимой операции
			case static_cast <uint8_t> (awh::event::error_t::INVALID):
				// Выводим сообщение об ошибке недопустимой операции
				this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа запрещёния
			case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
				// Выводим сообщение об ошибке доступа запрещёния
				this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка уже существующего объекта
			case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
				// Выводим сообщение об ошибке уже существующего объекта
				this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа к сокету
			case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
				// Выводим сообщение об ошибке доступа к сокету
				this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка некорректного адреса
			case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
				// Выводим сообщение об ошибке некорректного адреса
				this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка ошибки подключения
			case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
				// Выводим сообщение об ошибке подключения
				this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недостаточно ресурсов
			case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
				// Выводим сообщение об ошибке недостаточно ресурсов
				this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка события
			case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
				// Выводим сообщение об ошибке события
				this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если объект не найден
			case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
				// Выводим сообщение об ошибке события
				this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
		}
	});
	// Устанавливаем функцию обратного вызова на общее событие
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
		/**
		 * Обрабатываем действие события
		 */
		switch(static_cast <uint8_t> (action)){
			// Если действие является чтением
			case static_cast <uint8_t> (awh::event::action_t::READ):
				// Выводим сообщение о чтении события
				this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является записью
			case static_cast <uint8_t> (awh::event::action_t::WRITE):
				// Выводим сообщение о записи события
				this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является подключением
			case static_cast <uint8_t> (awh::event::action_t::CONNECT):
				// Выводим сообщение о подключении события
				this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отключением
			case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
				// Выводим сообщение об отключении события
				this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переподключением
			case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
				// Выводим сообщение о переподключении события
				this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является закрытием
			case static_cast <uint8_t> (awh::event::action_t::CLOSE):
				// Выводим сообщение о закрытии события
				this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением
			case static_cast <uint8_t> (awh::event::action_t::CHANGE):
				// Выводим сообщение об изменении события
				this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является удалением
			case static_cast <uint8_t> (awh::event::action_t::DELETE):
				// Выводим сообщение об удалении события
				this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переименованием
			case static_cast <uint8_t> (awh::event::action_t::RENAME):
				// Выводим сообщение о переименовании события
				this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением атрибутов
			case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
				// Выводим сообщение об изменении атрибутов события
				this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отзывом доступа
			case static_cast <uint8_t> (awh::event::action_t::REVOKE):
				// Выводим сообщение об отзыве доступа события
				this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением счётчика жёстких ссылок
			case static_cast <uint8_t> (awh::event::action_t::HDLINK):
				// Выводим сообщение о изменении счётчика жёстких ссылок события
				this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
		}
	});
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	// Запускаем событие пользователя
	ASSERT_TRUE(this->_io->launch(eid));
	// Запускаем дочерний поток для уведомления события
	std::thread([this](const awh::event::id_t eid) noexcept -> void {
		// Текст сообщения
		const std::string message = "Hello AWH IO Event!";
		// Уведомляем событие
		this->_io->send(eid, reinterpret_cast <const char *> (message.c_str()), message.length());
	}, eid).detach();
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы многоадресной передачи UDP
 *
 */
TEST_F(IoFixture, IoMulticast1Test){
	/**
	 * 1. Peer-to-peer discovery (mDNS, SSDP)
	 * Клиент отправляет запрос в мультикаст (224.0.0.251:5353 для mDNS),
	 * Серверы слушают этот адрес и отвечают либо в мультикаст, либо напрямую клиенту.
	 */
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->delivery(events[1], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->hops(events[1], awh::event::family_t::IPV4, awh::event::hops_t::NETWORK));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[1], awh::event::mode_t::ENABLED, "239.255.1.1", "0.0.0.0"));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::IPV4).c_str(), this->_io->port(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Флаг отправки сообщений
			static bool sending = false;
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [eid, this](const awh::event::id_t cid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, cid, size, message.c_str());
				// Если сообщение ещё не отправлено
				if(!sending){
					// Устанавливаем флаг отправки сообщения
					sending = !sending;
					// Помечаем сообщение
					message.append("1");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.length()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено в группу: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.length());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки в группу: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					// Помечаем сообщение
					message.append("2");
					// Отправляем данные обратно клиенту
					if(this->_io->send(cid, message.c_str(), message.length()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено клиенту: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, message.length());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки клиенту: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск события
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->delivery(events[0], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем количество хопов события
		ASSERT_TRUE(this->_io->hops(events[0], awh::event::family_t::IPV4, awh::event::hops_t::NETWORK));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "239.255.1.1"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[0], awh::event::mode_t::ENABLED, "239.255.1.1", "0.0.0.0"));
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск события
		ASSERT_TRUE(this->_io->launch(events[0]));
		// Формируем отправляемое сообщение
		const std::string & message = "Hello World!!!";
		// Отправляем данные обратно клиенту
		ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.length()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы многоадресной передачи UDP
 *
 */
TEST_F(IoFixture, IoMulticast3Test){
	/**
	 * 3. Сервер-обнаружение
	 * Клиенты слушают 239.255.1.2,
	 * Сервер периодически рассылает "я здесь",
	 * Клиенты отвечают прямо серверу (unicast), чтобы установить соединение.
	 */
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Добавляем новое событие интервала
		awh::event::id_t tid = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(tid, 0);
		// Добавляем новое событие интервала
		this->_io->timeout(tid, awh::event::action_t::NONE, 5000);
		// Выполняем фиксацию настроек события интервала
		ASSERT_TRUE(this->_io->commit(tid));
		// Устанавливаем функцию обратного вызова на событие интервала
		this->_io->on(tid, [&events, this](const awh::event::id_t tid, const awh::event::status_t status) noexcept -> void {
			// Количество отправленных сообщений
			static uint8_t counter = 0;
			// Формируем отправляемое сообщение
			const std::string & message = this->_fmk->format("Message #%d", ++counter);
			// Выполняем отправку сообщения в мультикастовую группу
			if(this->_io->send(events[1], message.c_str(), message.size()))
				// Выводим сообщение об ошибке отправки сообщения
				this->_log->print("Сообщение отправлено: ID=%u, %s", awh::log_t::flag_t::INFO, events[1], message.c_str());
		});
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->delivery(events[1], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->hops(events[1], awh::event::family_t::IPV4, awh::event::hops_t::NETWORK));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->address(cid, awh::event::address_t::IPV4).c_str(), this->_io->port(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск работы таймера
		ASSERT_TRUE(this->_io->launch(tid));
		// Выполняем запуск работы сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->delivery(events[0], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[0], awh::event::mode_t::ENABLED, "239.255.1.1", "0.0.0.0", port));
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Количество прочитанных сообщений
		uint8_t count = 0;
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&count, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			// Останавливаем тест
			stop = (++count >= 3);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск события
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы TLS-соединения
 *
 */
TEST_F(IoFixture, IoTLSTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объект транспортного уровня безопасности
		awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_tls->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_tls->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_tls->validateHostname(cts, false);
		// Устанавливаем клиентский сертификат TLS
		this->_tls->certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_tls->privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(cts, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls_t::id_t ctl = this->_tls->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки TLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение TLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке TLS
						this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка TLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке TLS
						this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
						// Выводим сообщение об ошибке транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
						// Выводим сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
						// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_tls->hostname(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_tls->alpn(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
						// Выводим сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
						// Выводим сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_tls->peer(ctl, this->_io->address(cid, awh::event::address_t::IPV4), this->_io->port(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_tls->on(ctl, [cid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Выводим сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_tls->encrypt(id, response.c_str(), response.size()))
							// Выводим сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_tls->decrypt(ctl, data, size))
					// Выводим сообщение об успешном дешифровании данных TLS
					this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Если рукопожатие TLS успешно
			if(this->_tls->handshake(ctl))
				// Выводим сообщение о начале рукопожатия TLS
				this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
			// Если рукопожатие TLS не выполнено
			else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Регистрируем объект транспортного уровня безопасности
		awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_tls->alpn(cts, {{0,"http/1.1"}});
		// this->_tls->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_tls->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_tls->validateHostname(cts, false);
		// Устанавливаем имя хоста TLS
		this->_tls->hostname(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_tls->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_tls->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls_t::id_t ctl = this->_tls->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
					// Выводим сообщение об ошибке транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
					// Выводим сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
					// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Выводим данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
					// Выводим информацию о TLS соединении
					std::cout << this->_tls->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_tls->encrypt(id, request.c_str(), request.size()))
						// Выводим сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
					// Выводим сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
					// Выводим сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_tls->on(ctl, [&events, &stop, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Выводим сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_tls->decrypt(ctl, data, size))
				// Выводим сообщение об успешном дешифровании данных TLS
				this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_tls->handshake(ctl))
					// Выводим сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы мульти TLS-соединения
 *
 */
TEST_F(IoFixture, IoMultiTLSTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объекты транспортного уровня безопасности
		awh::tls_t::id_t cts1 = this->_tls->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		awh::tls_t::id_t cts2 = this->_tls->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts1, 0);
		ASSERT_GT(cts2, 0);
		// Устанавливаем режим работы TLS
		this->_tls->mode(cts1, awh::tls_t::mode_t::MULTICERT);
		this->_tls->mode(cts2, awh::tls_t::mode_t::MULTICERT);
		// Включаем проверку имени хоста TLS
		this->_tls->validateHostname(cts1, false);
		this->_tls->validateHostname(cts2, false);
		// Устанавливаем ALPN протоколы TLS
		this->_tls->alpn(cts1, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		this->_tls->alpn(cts2, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_tls->ca(cts1, "../sh/certificates", "ca.pem");
		this->_tls->ca(cts2, "../sh/certificates", "ca.pem");
		// Устанавливаем клиентский сертификат TLS
		this->_tls->certificate(cts1, "../sh/certificates/example/cert.pem");
		this->_tls->certificate(cts2, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_tls->privateKey(cts1, "../sh/certificates/example/key.pem");
		this->_tls->privateKey(cts2, "../sh/certificates/server/key.pem");
		// Устанавливаем имя хоста TLS (Указывать нужно после установки режима работы мультисертификатного TLS!!!!!!!)
		this->_tls->hostname(cts2, "anyks.com");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(cts1, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(cts2, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([cts1, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls_t::id_t ctl = this->_tls->transport(cts1);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки TLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение TLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке TLS
						this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка TLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке TLS
						this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
						// Выводим сообщение об ошибке транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
						// Выводим сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
						// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_tls->hostname(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_tls->alpn(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
						// Выводим сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
						// Выводим сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_tls->peer(ctl, this->_io->address(cid, awh::event::address_t::IPV4), this->_io->port(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_tls->on(ctl, [cid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Выводим сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_tls->encrypt(id, response.c_str(), response.size()))
							// Выводим сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_tls->decrypt(ctl, data, size))
					// Выводим сообщение об успешном дешифровании данных TLS
					this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Если рукопожатие TLS успешно
			if(this->_tls->handshake(ctl))
				// Выводим сообщение о начале рукопожатия TLS
				this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
			// Если рукопожатие TLS не выполнено
			else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Регистрируем объект транспортного уровня безопасности
		awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_tls->alpn(cts, {{0,"http/1.1"}});
		// this->_tls->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_tls->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_tls->validateHostname(cts, false);
		// Устанавливаем имя хоста TLS
		this->_tls->hostname(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_tls->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_tls->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls_t::id_t ctl = this->_tls->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
					// Выводим сообщение об ошибке транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
					// Выводим сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
					// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Выводим данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
					// Выводим информацию о TLS соединении
					std::cout << this->_tls->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_tls->encrypt(id, request.c_str(), request.size()))
						// Выводим сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
					// Выводим сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
					// Выводим сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_tls->on(ctl, [&events, &stop, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Выводим сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_tls->decrypt(ctl, data, size))
				// Выводим сообщение об успешном дешифровании данных TLS
				this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_tls->handshake(ctl))
					// Выводим сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы TLS-соединения
 *
 */
TEST_F(IoFixture, IoDTLSTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объект транспортного уровня безопасности
		awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::SERVER, awh::event::protocol_t::UDP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_tls->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_tls->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_tls->validateHostname(cts, false);
		// Устанавливаем клиентский сертификат TLS
		this->_tls->certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_tls->privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(cts, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls_t::id_t ctl = this->_tls->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки TLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение TLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке TLS
						this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка TLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке TLS
						this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
						// Выводим сообщение об ошибке транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
						// Выводим сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
						// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_tls->hostname(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_tls->alpn(id));
						// Выводим информацию о DTLS соединении
						std::cout << this->_tls->peerInfo(id) << std::endl;
						// Выполняем повторную передачу данных TLS
						ASSERT_TRUE(this->_tls->retransmit(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
						// Выводим сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
						// Выводим сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_tls->peer(ctl, this->_io->address(cid, awh::event::address_t::IPV4), this->_io->port(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_tls->on(ctl, [cid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Выводим сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_tls->encrypt(id, response.c_str(), response.size()))
							// Выводим сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_tls->decrypt(ctl, data, size)){
					// Выводим сообщение об успешном дешифровании данных TLS
					this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
					// Если рукопожатие DTLS успешно
					if(this->_tls->handshake(ctl))
						// Выводим сообщение о начале рукопожатия DTLS
						this->_log->print("Начинаем процесс рукопожатия: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, ctl);
					// Если рукопожатие DTLS не выполнено
					else this->_log->print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
				// Если данные не отправлены
				} else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Регистрируем объект транспортного уровня безопасности
		awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::CLIENT, awh::event::protocol_t::UDP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_tls->alpn(cts, {{0,"http/1.1"}});
		// this->_tls->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_tls->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_tls->validateHostname(cts, false);
		// Устанавливаем имя хоста TLS
		this->_tls->hostname(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_tls->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_tls->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls_t::id_t ctl = this->_tls->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
					// Выводим сообщение об ошибке транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
					// Выводим сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
					// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Выводим данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
					// Выводим информацию о TLS соединении
					std::cout << this->_tls->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_tls->encrypt(id, request.c_str(), request.size()))
						// Выводим сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
					// Выводим сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
					// Выводим сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_tls->on(ctl, [&events, &stop, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Выводим сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Выводим сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_tls->decrypt(ctl, data, size))
				// Выводим сообщение об успешном дешифровании данных TLS
				this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_tls->handshake(ctl))
					// Выводим сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * Для операционной системы Linux или FreeBSD
 */
#if __linux__ || __FreeBSD__
	/**
	 * @brief Тест проверки работы протокола SCTP Stream
	 *
	 */
	TEST_F(IoFixture, IoSCTPStreamTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP Stream
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++){
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(events[i], port));
			// Проверяем что порт получен
			ASSERT_EQ(port, this->_io->port(events[i]));
		}
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Выводим сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Выводим информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Выводим сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Выводим сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Выводим сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Выводим сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Выводим сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Выводим сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Выводим сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Выводим сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Выводим сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Выводим сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Выводим сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Выводим сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Выводим сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Выводим сообщение о переподключении события
					this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Выводим сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Выводим сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на возрождение события
			this->_io->on(events[0], [this](const awh::event::id_t eid) noexcept -> void {
				// Выводим сообщение об возрождении события
				this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid, {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR
				});
			});
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Выводим информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Выводим сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Выводим сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Выводим сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Выводим сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Выводим сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Выводим сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Выводим сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Выводим сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Выводим сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Выводим сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Выводим сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Выводим сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Выводим сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Текст исходящего сообщения
					const std::string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0], true));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP SEQPACKET
	 *
	 */
	TEST_F(IoFixture, IoSCTPSeqPacketTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP SEQPACKET
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::SEQPACKET, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++){
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(events[i], port));
			// Проверяем что порт получен
			ASSERT_EQ(port, this->_io->port(events[i]));
		}
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Выводим сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Выводим информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Выводим сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Выводим сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Выводим сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Выводим сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Выводим сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Выводим сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Выводим сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Выводим сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Выводим сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Выводим сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Выводим сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Выводим сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Выводим сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Выводим сообщение о переподключении события
					this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Выводим сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Выводим сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Текст инициализационных сообщений SCTP
			awh::net::sctp::initmsg_t initmsg;
			// Устанавливаем количество попыток подключения SCTP
			initmsg.attempts = 4;
			// Устанавливаем количество исходящих потоков SCTP
			initmsg.ostreams = 5;
			// Устанавливаем количество входящих потоков SCTP
			initmsg.istreams = 5;
			// Инициализируем сообщения SCTP
			this->_sctp->initMessages(events[1], initmsg);
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на возрождение события
			this->_io->on(events[0], [this](const awh::event::id_t eid) noexcept -> void {
				// Выводим сообщение об возрождении события
				this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid, {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR
				});
			});
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Выводим информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Выводим сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Выводим сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Выводим сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Выводим сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Выводим сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Выводим сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Выводим сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Выводим сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Выводим сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Выводим сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Выводим сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Выводим сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Выводим сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Текст исходящего сообщения
					const std::string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0], true));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP SEQPACKET с аутентификацией
	 *
	 */
	TEST_F(IoFixture, IoSCTPSeqPacketAuthTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP SEQPACKET
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::SEQPACKET, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++){
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(events[i], port));
			// Проверяем что порт получен
			ASSERT_EQ(port, this->_io->port(events[i]));
		}
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Устанавливаем ключ аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[1], 1, "0123456789abcdef0123456789abcdef"));
			// Устанавливаем режим использования ключа аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[1], awh::event::mode_t::ENABLED, 1));
			// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(events[1], {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
			// Устанавливаем чанки аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[1], {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR,
				awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
			});
			// Извлекаем чанки аутентификации SCTP-сокета
			std::vector <awh::net::sctp::auth_chunk_t> chunks;
			// Выполняем извлечение чанков аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[1], awh::event::origin_t::LOCAL, chunks));
			// Перебираем все извлечённые чанки
			for(auto & chunk : chunks)
				// Выводим информацию о чанках аутентификации SCTP-сокета
				std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			// Устанавливаем таймаут heartbeat SCTP-сокета
			ASSERT_TRUE(this->_sctp->timeout(events[1], awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			// Выводим heartbeat timeout SCTP-сокета
			ASSERT_EQ(3000, this->_sctp->timeout(events[1], awh::net::sctp::timeout_t::HEARTBEAT));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(cid, awh::event::origin_t::REMOTE, chunks));
				// Перебираем все извлечённые чанки
				for(auto & chunk : chunks)
					// Выводим информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Устанавливаем таймаут heartbeat SCTP-сокета
				ASSERT_TRUE(this->_sctp->timeout(cid, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
				// Выводим heartbeat timeout SCTP-сокета
				ASSERT_EQ(3000, this->_sctp->timeout(cid, awh::net::sctp::timeout_t::HEARTBEAT));
				ASSERT_EQ(3000, this->_sctp->timeout(sid, awh::net::sctp::timeout_t::HEARTBEAT));
				// Выводим сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Выводим информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Выводим сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Выводим сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Выводим сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Выводим сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Выводим сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Выводим сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Выводим сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Выводим сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Выводим сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Выводим сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Выводим сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Выводим сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Выводим сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Выводим сообщение о переподключении события
					this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Выводим сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Выводим сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Текст инициализационных сообщений SCTP
			awh::net::sctp::initmsg_t initmsg;
			// Устанавливаем количество попыток подключения SCTP
			initmsg.attempts = 4;
			// Устанавливаем количество исходящих потоков SCTP
			initmsg.ostreams = 5;
			// Устанавливаем количество входящих потоков SCTP
			initmsg.istreams = 5;
			// Инициализируем сообщения SCTP
			this->_sctp->initMessages(events[1], initmsg);
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Устанавливаем ключ аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[0], 1, "0123456789abcdef0123456789abcdef"));
			// Устанавливаем режим использования ключа аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[0], awh::event::mode_t::ENABLED, 1));
			// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(events[0], {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
			// Устанавливаем чанки аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[0], {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR,
				awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
			});
			// Извлекаем чанки аутентификации SCTP-сокета
			std::vector <awh::net::sctp::auth_chunk_t> chunks;
			// Выполняем извлечение чанков аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[0], awh::event::origin_t::LOCAL, chunks));
			// Перебираем все извлечённые чанки
			for(auto & chunk : chunks)
				// Выводим информацию о чанках аутентификации SCTP-сокета
				std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			// Устанавливаем таймаут heartbeat SCTP-сокета
			ASSERT_TRUE(this->_sctp->timeout(events[0], awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на возрождение события
			this->_io->on(events[0], [this](const awh::event::id_t eid) noexcept -> void {
				// Выводим сообщение об возрождении события
				this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
				// Устанавливаем ключ аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateKey(eid, 1, "0123456789abcdef0123456789abcdef"));
				// Устанавливаем режим использования ключа аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateKey(eid, awh::event::mode_t::ENABLED, 1));
				// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(eid, {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
				// Устанавливаем чанки аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(eid, {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid, {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR,
					awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
				});
				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(eid, awh::event::origin_t::LOCAL, chunks));
				// Перебираем все извлечённые чанки
				for(auto & chunk : chunks)
					// Выводим информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Устанавливаем таймаут heartbeat SCTP-сокета
				ASSERT_TRUE(this->_sctp->timeout(eid, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			});
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Выводим информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Выводим сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Выводим сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Выводим сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Выводим сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Выводим сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Выводим сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Выводим сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Выводим сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Выводим сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Выводим сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Выводим сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Выводим сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(eid, awh::event::origin_t::REMOTE, chunks));
				// Перебираем все извлечённые чанки
				for(auto & chunk : chunks)
					// Выводим информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Выводим heartbeat timeout SCTP-сокета
				ASSERT_EQ(3000, this->_sctp->timeout(eid, awh::net::sctp::timeout_t::HEARTBEAT));
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Выводим сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Текст исходящего сообщения
					const std::string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0], true));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP SEQPACKET DTLS
	 *
	 */
	TEST_F(IoFixture, IoSCTPSeqPacketDTLSTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP SEQPACKET
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::SEQPACKET, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++){
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(events[i], port));
			// Проверяем что порт получен
			ASSERT_EQ(port, this->_io->port(events[i]));
		}
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Регистрируем объект транспортного уровня безопасности
			awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::SERVER, awh::event::protocol_t::SCTP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_tls->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_tls->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_tls->validateHostname(cts, false);
			// Устанавливаем клиентский сертификат DTLS
			this->_tls->certificate(cts, "../sh/certificates/server/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_tls->privateKey(cts, "../sh/certificates/server/key.pem");
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_tls->on(cts, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки DTLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Предупреждение DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Выводим сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Создаём идентификатор транспортного уровня DTLS
				awh::tls_t::id_t ctl = this->_tls->transport(cts);
				// Проверяем, что идентификатор транспортного уровня больше нуля
				ASSERT_GT(ctl, 0);
				// Устанавливаем клиента DTLS для события
				this->_tls->peer(ctl, this->_io->address(cid, awh::event::address_t::IPV4), this->_io->port(cid));
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
					/**
					 * Обрабатываем входящие ошибки DTLS
					 */
					switch(static_cast <uint8_t> (error)){
						// Если получено предупреждение DTLS
						case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
							// Выводим сообщение о предупреждающей ошибке DTLS
							this->_log->print("Предупреждение DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
						break;
						// Если получена критическая ошибка DTLS
						case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
							// Выводим сообщение о предупреждающей ошибке DTLS
							this->_log->print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
						break;
					}
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
							// Выводим сообщение о записи зашифрованных данных DTLS
							this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
							// Выводим сообщение о записи дешифрованных данных DTLS
							this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
							// Выводим сообщение об ошибке транспортного уровня TLS
							this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
							// Выводим сообщение об успешном удалении контекста TLS
							this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
							// Выводим сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_tls->hostname(id) << std::endl << std::endl;
							std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
							std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
							std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
							std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
							std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
							// Выводим данные сертификата DTLS
							std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
							// Выводим сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							this->_log->print("Рукопожатие DTLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_tls->alpn(id));
							// Выводим информацию о DTLS соединении
							std::cout << this->_tls->peerInfo(id) << std::endl;
							// Выполняем повторную передачу данных TLS
							ASSERT_TRUE(this->_tls->retransmit(id));
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Выводим информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Выводим сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Выводим сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Выводим сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Выводим сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Выводим сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Выводим сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Выводим сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Выводим сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Выводим сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Выводим сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Выводим сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Выводим сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				this->_tls->on(ctl, [cid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const std::string response(reinterpret_cast <const char *> (buffer), size);
							// Выводим сообщение полученных данных с сервера
							this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(this->_tls->encrypt(id, response.c_str(), response.size()))
								// Выводим сообщение об успешном шифровании данных DTLS
								this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Выводим сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы DTLS
					if(this->_tls->decrypt(ctl, data, size)){
						// Выводим сообщение об успешном дешифровании данных DTLS
						this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
					// Если данные не отправлены
					} else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					// Если рукопожатие DTLS успешно
					if(this->_tls->handshake(ctl))
						// Выводим сообщение о начале рукопожатия DTLS
						this->_log->print("Начинаем процесс рукопожатия: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, ctl);
					// Если рукопожатие DTLS не выполнено
					else this->_log->print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Выводим сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Выводим сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Текст инициализационных сообщений SCTP
			awh::net::sctp::initmsg_t initmsg;
			// Устанавливаем количество попыток подключения SCTP
			initmsg.attempts = 4;
			// Устанавливаем количество исходящих потоков SCTP
			initmsg.ostreams = 5;
			// Устанавливаем количество входящих потоков SCTP
			initmsg.istreams = 5;
			// Инициализируем сообщения SCTP
			this->_sctp->initMessages(events[1], initmsg);
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Регистрируем объект транспортного уровня безопасности
			awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::CLIENT, awh::event::protocol_t::SCTP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_tls->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_tls->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_tls->validateHostname(cts, false);
			// Устанавливаем имя хоста DTLS
			this->_tls->hostname(cts, "server.anyks.com");
			// Устанавливаем клиентский сертификат DTLS
			this->_tls->certificate(cts, "../sh/certificates/client/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_tls->privateKey(cts, "../sh/certificates/client/key.pem");
			// Создаём идентификатор транспортного уровня DTLS
			awh::tls_t::id_t ctl = this->_tls->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния DTLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
						// Выводим сообщение об ошибке транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
						// Выводим сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
						// Выводим сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Выводим данные сертификата DTLS
						std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
						// Выводим информацию о DTLS соединении
						std::cout << this->_tls->peerInfo(id) << std::endl;
						// Текст запроса к серверу
						const std::string request =
							"GET / HTTP/1.1\r\n"
							"Host: www.google.com\r\n"
							"Connection: close\r\n"
							"User-Agent: iouring-openssl-sample/1.0\r\n"
							"\r\n";
						// Если данные успешно зашифрованы DTLS
						if(this->_tls->encrypt(id, request.c_str(), request.size()))
							// Выводим сообщение об успешном шифровании данных DTLS
							this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки DTLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Предупреждение DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных DTLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
						// Выводим сообщение о записи зашифрованных данных DTLS
						this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
						// Выводим сообщение о записи дешифрованных данных DTLS
						this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Регистрируем функцию обратного вызова на чтение данных DTLS
			this->_tls->on(ctl, [&stop, eid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(eid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					} break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Выводим сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Останавливаем тест
						stop = true;
					} break;
				}
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на возрождение события
			this->_io->on(events[0], [this](const awh::event::id_t eid) noexcept -> void {
				// Выводим сообщение об возрождении события
				this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid, {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR
				});
			});
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Выводим информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Выводим сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Выводим сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Выводим сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Выводим сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Выводим сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Выводим сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Выводим сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Выводим сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Выводим сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Выводим сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Выводим сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Выводим сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Если данные успешно дешифрованы DTLS
				if(this->_tls->decrypt(ctl, data, size))
					// Выводим сообщение об успешном дешифровании данных DTLS
					this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Выводим сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Если рукопожатие DTLS успешно
					if(this->_tls->handshake(ctl))
						// Выводим сообщение о начале рукопожатия DTLS
						this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
					// Если рукопожатие DTLS не выполнено
					else this->_log->print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0], true));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP STREAM TLS
	 *
	 */
	TEST_F(IoFixture, IoSCTPStreamTLSTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP STREAM
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++){
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(events[i], port));
			// Проверяем что порт получен
			ASSERT_EQ(port, this->_io->port(events[i]));
		}
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Регистрируем объект транспортного уровня безопасности
			awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_tls->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_tls->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_tls->validateHostname(cts, false);
			// Устанавливаем клиентский сертификат DTLS
			this->_tls->certificate(cts, "../sh/certificates/server/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_tls->privateKey(cts, "../sh/certificates/server/key.pem");
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_tls->on(cts, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки DTLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Предупреждение DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Выводим сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Создаём идентификатор транспортного уровня DTLS
				awh::tls_t::id_t ctl = this->_tls->transport(cts);
				// Проверяем, что идентификатор транспортного уровня больше нуля
				ASSERT_GT(ctl, 0);
				// Устанавливаем клиента DTLS для события
				this->_tls->peer(ctl, this->_io->address(cid, awh::event::address_t::IPV4), this->_io->port(cid));
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
					/**
					 * Обрабатываем входящие ошибки DTLS
					 */
					switch(static_cast <uint8_t> (error)){
						// Если получено предупреждение DTLS
						case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
							// Выводим сообщение о предупреждающей ошибке DTLS
							this->_log->print("Предупреждение DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
						break;
						// Если получена критическая ошибка DTLS
						case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
							// Выводим сообщение о предупреждающей ошибке DTLS
							this->_log->print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
						break;
					}
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
							// Выводим сообщение о записи зашифрованных данных DTLS
							this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
							// Выводим сообщение о записи дешифрованных данных DTLS
							this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
							// Выводим сообщение об ошибке транспортного уровня TLS
							this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
							// Выводим сообщение об успешном удалении контекста TLS
							this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
							// Выводим сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_tls->hostname(id) << std::endl << std::endl;
							std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
							std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
							std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
							std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
							std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
							// Выводим данные сертификата TLS
							std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
							// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
							this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_tls->alpn(id));
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Выводим информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Выводим сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Выводим сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Выводим сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Выводим сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Выводим сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Выводим сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Выводим сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Выводим сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Выводим сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Выводим сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Выводим сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Выводим сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				this->_tls->on(ctl, [cid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const std::string response(reinterpret_cast <const char *> (buffer), size);
							// Выводим сообщение полученных данных с сервера
							this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(this->_tls->encrypt(id, response.c_str(), response.size()))
								// Выводим сообщение об успешном шифровании данных DTLS
								this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Выводим сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы TLS
					if(this->_tls->decrypt(ctl, data, size)){
						// Выводим сообщение об успешном дешифровании данных TLS
						this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
					// Если данные не отправлены
					} else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Выводим сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Выводим сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Если рукопожатие TLS успешно
				if(this->_tls->handshake(ctl))
					// Выводим сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Регистрируем объект транспортного уровня безопасности
			awh::tls_t::id_t cts = this->_tls->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_tls->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_tls->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_tls->validateHostname(cts, false);
			// Устанавливаем имя хоста DTLS
			this->_tls->hostname(cts, "server.anyks.com");
			// Устанавливаем клиентский сертификат TLS
			this->_tls->certificate(cts, "../sh/certificates/client/cert.pem");
			// Устанавливаем приватный ключ TLS
			this->_tls->privateKey(cts, "../sh/certificates/client/key.pem");
			// Создаём идентификатор транспортного уровня TLS
			awh::tls_t::id_t ctl = this->_tls->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::FAILED):
						// Выводим сообщение об ошибке транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls_t::state_t::DESTROYED):
						// Выводим сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls_t::state_t::HANDSHAKED): {
						// Выводим сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_tls->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_tls->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_tls->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_tls->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_tls->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_tls->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_tls->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Выводим данные сертификата DTLS
						std::cout << "Certificate data:\n" << this->_tls->certificateExtract(id) << std::endl << std::endl;
						// Выводим информацию о DTLS соединении
						std::cout << this->_tls->peerInfo(id) << std::endl;
						// Текст запроса к серверу
						const std::string request =
							"GET / HTTP/1.1\r\n"
							"Host: www.google.com\r\n"
							"Connection: close\r\n"
							"User-Agent: iouring-openssl-sample/1.0\r\n"
							"\r\n";
						// Если данные успешно зашифрованы DTLS
						if(this->_tls->encrypt(id, request.c_str(), request.size()))
							// Выводим сообщение об успешном шифровании данных DTLS
							this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::error_t error, const std::string & message) noexcept -> void {
				/**
				 * Обрабатываем входящие ошибки DTLS
				 */
				switch(static_cast <uint8_t> (error)){
					// Если получено предупреждение DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::WARNING):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Предупреждение DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::WARNING, id, message.c_str());
					break;
					// Если получена критическая ошибка DTLS
					case static_cast <uint8_t> (awh::tls_t::error_t::CRITICAL):
						// Выводим сообщение о предупреждающей ошибке DTLS
						this->_log->print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, message.c_str());
					break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных DTLS
			this->_tls->on(ctl, [this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION):
						// Выводим сообщение о записи зашифрованных данных DTLS
						this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION):
						// Выводим сообщение о записи дешифрованных данных DTLS
						this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Регистрируем функцию обратного вызова на чтение данных DTLS
			this->_tls->on(ctl, [&stop, eid, this](const awh::tls_t::id_t id, const awh::tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(eid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					} break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Выводим сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Останавливаем тест
						stop = true;
					} break;
				}
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на возрождение события
			this->_io->on(events[0], [this](const awh::event::id_t eid) noexcept -> void {
				// Выводим сообщение об возрождении события
				this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid, {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR
				});
			});
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Выводим сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Выводим сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Выводим сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Выводим сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Выводим сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Выводим сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Выводим сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Выводим сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Выводим сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Выводим сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Выводим сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Выводим сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Выводим сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::net::sctp::callback::info_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Выводим информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Выводим сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Выводим сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Выводим сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Выводим сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Выводим сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Выводим сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Выводим сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Выводим сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Выводим сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Выводим сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Выводим сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Выводим сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Выводим информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Выводим статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Если данные успешно дешифрованы DTLS
				if(this->_tls->decrypt(ctl, data, size))
					// Выводим сообщение об успешном дешифровании данных DTLS
					this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Выводим сообщение об ошибке неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Выводим сообщение об ошибке недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Выводим сообщение об ошибке доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Выводим сообщение об ошибке уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Выводим сообщение об ошибке доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Выводим сообщение об ошибке некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Выводим сообщение об ошибке подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Выводим сообщение об ошибке недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Выводим сообщение об ошибке события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Выводим сообщение об ошибке события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Выводим сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Если рукопожатие DTLS успешно
					if(this->_tls->handshake(ctl))
						// Выводим сообщение о начале рукопожатия DTLS
						this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
					// Если рукопожатие DTLS не выполнено
					else this->_log->print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0], true));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}
#endif
