/**
 * @file: static.cpp
 * @date: 2025-12-15
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты асинхронного движка ввода-вывода — проверка создания и сброса объекта модуля,
 *        а также корректности регистрации событий, управления подписками,
 *        работы таймеров и корректной остановки цикла событий
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем стандартные модули
 */
#include <cinttypes>

/**
 * Подключаем стандартные модули
 */
#include <arpa/inet.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * Системные заголовочные файлы
 */
#include <netinet/in.h>
#include <sys/socket.h>

/**
 * @brief Генерация случайного порта в диапазоне 49152-65535
 *
 * @return случайный порт
 *
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
	this->_io = std::make_unique <awh::engine::io_t> (this->_fmk.get(), this->_log.get());
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
	this->_io = std::make_unique <awh::engine::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @note TODO: Полноценный юнит-тест сетевого движка ещё предстоит написать отдельной
 *       задачей (сейчас движок покрыт лишь частично: create/reset, ping, таймеры).
 *       Ниже добавлено точечное покрытие метода rebuild() - пересоздания нижележащего
 *       дескриптора события с сохранением самого события (используется дочерними
 *       процессами кластера для получения собственного сокета вместо унаследованного
 *       от мастера, где SO_REUSEPORT требует отдельного сокета на процесс).
 *
 */

/**
 * @brief Тест перестройки серверного события до фиксации (сценарий дочернего процесса кластера)
 *
 */
TEST_F(IoFixture, RebuildServerBeforeCommitTest){
	// Генерируем случайный порт привязки
	const uint16_t listenPort = port();
	// Создаём серверное событие TCP
	awh::event::id_t eid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(eid, 0);
	// Устанавливаем адрес привязки
	ASSERT_TRUE(this->_io->setAddress(eid, awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем порт привязки
	ASSERT_TRUE(this->_io->setSourcePort(eid, listenPort));
	// Событие ещё не зафиксировано
	ASSERT_EQ(awh::event::status_t::NONE, this->_io->status(eid));
	// Перестраиваем дескриптор до фиксации - должно пройти успешно, статус остаётся неопределённым
	ASSERT_TRUE(this->_io->rebuild(eid));
	// Проверяем, что статус события не изменился
	ASSERT_EQ(awh::event::status_t::NONE, this->_io->status(eid));
	// Фиксируем событие (привязка нового дескриптора)
	ASSERT_TRUE(this->_io->commit(eid));
	// Проверяем, что порт привязки сохранился
	ASSERT_EQ(listenPort, this->_io->getSourcePort(eid));
	// Уничтожаем событие
	this->_io->destroy(eid);
}

/**
 * @brief Тест перестройки серверного события в режиме прослушивания (полный цикл commit -> listen -> launch)
 *
 */
TEST_F(IoFixture, RebuildServerListeningTest){
	// Генерируем случайный порт привязки
	const uint16_t listenPort = port();
	// Создаём серверное событие TCP
	awh::event::id_t eid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(eid, 0);
	// Устанавливаем адрес привязки
	ASSERT_TRUE(this->_io->setAddress(eid, awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем порт привязки
	ASSERT_TRUE(this->_io->setSourcePort(eid, listenPort));
	// Устанавливаем опции переиспользования адреса и порта
	ASSERT_TRUE(this->_io->setOptions(eid, static_cast <uint16_t> (awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT)));
	// Фиксируем событие (привязка дескриптора)
	ASSERT_TRUE(this->_io->commit(eid));
	// Переводим событие в режим прослушивания
	// (стадию launch, включающую фильтр чтения, не выполняем: ей нужен запущенный
	//  цикл событий - поведение уровня цикла проверяется прогоном кластера, не здесь)
	ASSERT_TRUE(this->_io->listen(eid, 128));
	// Проверяем, что событие переведено в режим прослушивания
	ASSERT_EQ(awh::event::status_t::SUCCESS, this->_io->status(eid));
	// Запоминаем привязанный порт
	const uint16_t boundPort = this->_io->getSourcePort(eid);
	// Перестраиваем дескриптор - статус и порт должны сохраниться
	ASSERT_TRUE(this->_io->rebuild(eid));
	// Проверяем, что событие вновь в режиме прослушивания
	ASSERT_EQ(awh::event::status_t::SUCCESS, this->_io->status(eid));
	// Проверяем, что привязанный порт сохранился
	ASSERT_EQ(boundPort, this->_io->getSourcePort(eid));
	// Проверяем, что опция переиспользования порта сохранилась
	ASSERT_TRUE(this->_io->getOptions(eid) & static_cast <uint16_t> (awh::event::options::REUSE_PORT));
	// Уничтожаем событие
	this->_io->destroy(eid);
}

/**
 * @brief Тест перестройки пары IPC целиком по одному идентификатору
 *
 */
TEST_F(IoFixture, RebuildIpcPairTest){
	// Создаём пару IPC (UNIX-доменный socketpair)
	auto ids = this->_io->events(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	// Проверяем, что оба идентификатора пары созданы
	ASSERT_GT(ids[0], 0);
	ASSERT_GT(ids[1], 0);
	// Перестраиваем пару целиком по одному из идентификаторов
	ASSERT_TRUE(this->_io->rebuild(ids[0]));
	// Проверяем, что оба узла пары остаются событиями типа IPC
	ASSERT_EQ(awh::event::node_t::IPC, this->_io->node(ids[0]));
	ASSERT_EQ(awh::event::node_t::IPC, this->_io->node(ids[1]));
	// Уничтожаем оба события пары
	this->_io->destroy(ids[0]);
	this->_io->destroy(ids[1]);
}

/**
 * @brief Тест того, что перестройка неприменима к событию без дескриптора (таймер)
 *
 */
TEST_F(IoFixture, RebuildUnsupportedTypeTest){
	// Создаём событие интервального таймера (без сетевого дескриптора)
	awh::event::id_t eid = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(eid, 0);
	// Перестройка дескриптора неприменима - метод возвращает ложь
	ASSERT_FALSE(this->_io->rebuild(eid));
	// Уничтожаем событие
	this->_io->destroy(eid);
}

/**
 * @brief Тест того, что перестройка несуществующего события возвращает ложь
 *
 */
TEST_F(IoFixture, RebuildUnknownIdTest){
	// Перестройка несуществующего идентификатора события - метод возвращает ложь
	ASSERT_FALSE(this->_io->rebuild(static_cast <awh::event::id_t> (999999999)));
}

/**
 * @brief Тест допустимых комбинаций объединения данных (splice) между узлами
 *
 * @note Проверяется только матрица допустимости комбинаций типов узлов и установка
 *       приёмника: сам перенос данных проверяется прогоном соединения (см.
 *       IoUDPSpliceConnectTest) и прогоном QUIC-прокси (sample server-quic-proxy).
 *       Объединение опирается лишь на наличие узлов в реестре, поэтому фиксация
 *       (commit) и сокеты здесь не требуются
 *
 */
TEST_F(IoFixture, SpliceValidCombinationsTest){
	// Создаём событие клиента TCP (узел-источник)
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём второе событие клиента TCP (узел-приёмник)
	awh::event::id_t client2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём серверное событие TCP (узел-приёмник)
	awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что все идентификаторы событий созданы
	ASSERT_GT(client, 0);
	ASSERT_GT(client2, 0);
	ASSERT_GT(server, 0);
	// Объединение клиент -> сервер допустимо
	ASSERT_TRUE(this->_io->splice(client, server));
	// Объединение клиент -> клиент допустимо
	ASSERT_TRUE(this->_io->splice(client, client2));
	// Уничтожаем созданные события
	this->_io->destroy(client);
	this->_io->destroy(client2);
	this->_io->destroy(server);
}

/**
 * @brief Тест того, что серверный узел не может быть источником объединения данных
 *
 */
TEST_F(IoFixture, SpliceServerSourceRejectedTest){
	// Создаём серверное событие TCP (недопустимый узел-источник)
	awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём событие клиента TCP (узел-приёмник)
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификаторы событий созданы
	ASSERT_GT(server, 0);
	ASSERT_GT(client, 0);
	// Серверный узел не является допустимым источником - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(server, client));
	// Уничтожаем созданные события
	this->_io->destroy(server);
	this->_io->destroy(client);
}

/**
 * @brief Тест того, что объединение с узлом таймера отклоняется
 *
 */
TEST_F(IoFixture, SpliceTimerRejectedTest){
	// Создаём событие клиента TCP (узел-источник)
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём событие интервального таймера (недопустимый узел-приёмник)
	awh::event::id_t timer = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Проверяем, что идентификаторы событий созданы
	ASSERT_GT(client, 0);
	ASSERT_GT(timer, 0);
	// Узел таймера не может быть приёмником объединения - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(client, timer));
	// Уничтожаем созданные события
	this->_io->destroy(client);
	this->_io->destroy(timer);
}

/**
 * @brief Тест того, что объединение с несуществующим событием возвращает ложь
 *
 */
TEST_F(IoFixture, SpliceUnknownIdTest){
	// Создаём событие клиента TCP
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(client, 0);
	// Несуществующий узел-источник - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(static_cast <awh::event::id_t> (999999999), client));
	// Несуществующий узел-приёмник - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(client, static_cast <awh::event::id_t> (999999999)));
	// Уничтожаем событие
	this->_io->destroy(client);
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
	eth.addr.fillSource(source);
	// Проверяем, что название сетевого интерфейса получено
	ASSERT_FALSE(source.iface.empty());
	// Устанавливаем тип таймера для событий сетевого движка
	this->_io->setInternalTimer(awh::event::timer_t::DIFFICULT);
	// Проверяем, что тип таймера для событий сетевого движка установлен
	ASSERT_EQ(awh::event::timer_t::DIFFICULT, this->_io->getInternalTimer());
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
			ASSERT_TRUE(this->_io->setTargetPort(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid1));
			// Устанавливаем MTU события
			// ASSERT_TRUE(this->_io->setMaximumTransmissionUnit(eid1, 1500));
			// Проверяем что MTU получен
			ASSERT_EQ(1500, this->_io->getMaximumTransmissionUnit(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->setIface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid1));

			/**
			 * Для операционной системы FreeBSD
			 */
			#if __FreeBSD__
				// Извлекаем информационные метаданные SCTP сообщения
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid1);
				// Записываем в лог информацию о сообщении SCTP-сокета
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
				// Возвращаем статус SCTP-сокета
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

				// Проверяем что типы SCTP событий совпадают с установленными ранее (так-как событие не принадлежит к SCTP)
				ASSERT_NE(types, this->_sctp->eventsSubscribed(eid1));

				// Устанавливаем таймаут INIT SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::INIT, 3000));
				// Проверяем что таймаут INIT SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::INIT));

				// Устанавливаем таймаут DATA SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::DATA, 3000));
				// Проверяем что таймаут DATA SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::DATA));

				// Устанавливаем таймаут SACK SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::SACK, 3000));
				// Проверяем что таймаут SACK SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::SACK));

				// Устанавливаем таймаут COOKIE SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::COOKIE, 3000));
				// Проверяем что таймаут COOKIE SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::COOKIE));

				// Устанавливаем таймаут SHUTDOWN SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWN, 3000));
				// Проверяем что таймаут SHUTDOWN SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWN));

				// Устанавливаем таймаут HEARTBEAT SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
				// Проверяем что таймаут HEARTBEAT SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::HEARTBEAT));

				// Устанавливаем таймаут SHUTDOWNACK SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWNACK, 3000));
				// Проверяем что таймаут SHUTDOWNACK SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWNACK));

				// Устанавливаем ключ аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateKey(eid1, 1, "0123456789abcdef0123456789abcdef"));
				// Устанавливаем режим использования ключа аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateKey(eid1, awh::event::mode_t::ENABLED, 1));
				// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateSupportAlgorithms(eid1, {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
				// Устанавливаем чанки аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateChunks(eid1, {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));

				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateChunks(eid1, awh::event::origin_t::LOCAL, chunks));
				// Проверяем что чанки аутентификации SCTP-сокета получены
				ASSERT_TRUE(chunks.empty());
				/**
				 * Перебираем все извлечённые чанки
				 */
				for(auto & chunk : chunks)
					// Записываем в лог информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			#endif

			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->getAddress(eid1, awh::event::address_t::IPV4);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->getAddress(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid2));
			// Устанавливаем MTU события
			// ASSERT_TRUE(this->_io->setMaximumTransmissionUnit(eid2, 1500));
			// Проверяем что MTU получен
			ASSERT_EQ(1500, this->_io->getMaximumTransmissionUnit(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid2, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid3, awh::event::address_t::IPV4, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid3, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid4, awh::event::address_t::NETWORK, ip + "/255.255.255.0"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid4).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid4));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid4, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid4, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid5 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid5, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->setTargetPort(eid5, 8080));
			// Устанавливаем UDS-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid5, awh::event::address_t::UDS, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->getIface(eid5).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->getAddress(eid5, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->getAddress(eid5, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid4).empty());
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->getAddress(eid5, awh::event::address_t::UDS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid6 = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid6, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->setTargetPort(eid6, 8080));
			// Устанавливаем сетевой адрес события
			ASSERT_TRUE(this->_io->setAddress(eid6, awh::event::address_t::FS, "/tmp/awh.txt"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->getIface(eid6).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->getAddress(eid6, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->getAddress(eid6, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid6).empty());
			// Проверяем, что адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.txt", this->_io->getAddress(eid6, awh::event::address_t::FS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->setTarget(eid7, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid7).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid7));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid7, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid7, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->getTarget(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid7, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid8 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid8, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->setTargetPort(eid8, 8080));
			// Устанавливаем UDS-адрес назначения для события
			ASSERT_TRUE(this->_io->setTarget(eid8, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->getIface(eid8).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->getAddress(eid8, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->getAddress(eid8, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_EQ("/tmp/awh.sock", this->_io->getTarget(eid8));
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->getAddress(eid8, awh::event::address_t::UDS));
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
			ASSERT_TRUE(this->_io->setTargetPort(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->setIface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid1));
			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->getAddress(eid1, awh::event::address_t::IPV6);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->getAddress(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid2, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid3, awh::event::address_t::IPV6, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid3, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid4, awh::event::address_t::NETWORK, ip + "/64"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid4).empty());
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->getAddress(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->setTarget(eid7, ip));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->getAddress(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->getTarget(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid7, awh::event::address_t::UDS).empty());
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Проверяем, что таймаут события на чтение установлен правильно
		ASSERT_EQ(3000, this->_io->getTimeout(events[1], awh::event::action_t::READ));
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Проверяем, что таймаут события на запись установлен правильно
		ASSERT_EQ(3000, this->_io->getTimeout(events[1], awh::event::action_t::WRITE));
		// Устанавливаем режим многократного использования таймаута события на чтение
		this->_io->setUsageReadTimeout(events[1], awh::event::usage_t::REUSABLE);
		// Проверяем, что режим многократного использования таймаута события на чтение установлен правильно
		ASSERT_EQ(awh::event::usage_t::REUSABLE, this->_io->getUsageReadTimeout(events[1]));
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
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
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
	/**
	 * Количество прочитанных сообщений. Счётчик объявлен в области видимости теста:
	 * функция обратного вызова захватывает его по ссылке и вызывается уже из цикла
	 * событий, то есть переживает блок настройки события
	 */
	uint8_t count = 0;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&count, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
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
	ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->setOptions(sid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setTarget(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setAddress(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(sid, static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s", awh::log_t::flag_t::INFO, sid, cid, this->_io->getAddress(cid, awh::event::address_t::UDS).c_str());
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(sid, awh::event::action_t::WRITE, 3000);
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid));
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
	ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->setOptions(sid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setTarget(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setAddress(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(sid, static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::UDS).c_str());
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(sid, awh::event::action_t::WRITE, 3000);
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid));
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::BROADCAST));
		// Создаём объект работы с Ethernet
		awh::eth_t eth(this->_fmk.get(), this->_log.get());
		// Временный объект для извлечения сетевого интерфейса
		awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
		// Выполняем извлечение сетевых параметров
		eth.addr.fillSource(source);
		// Если сетевой интерфейс не принадлежит к VPN
		if(::memcmp("ut", source.iface.c_str(), 2) != 0){
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->setIface(events[0], source.iface));
			// Создаём объект сетевого адреса
			awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
			// Извлекаем IP-адрес сетевого интерфейса
			addr = std::move(this->_io->getAddress(events[0], awh::event::address_t::IPV4));
			// Если IP-адрес не принадлежит к LAN
			if(addr.own() != awh::net_addr_t::own_t::LAN){
				// Уничтожаем все события после получения ответа
				ASSERT_TRUE(this->_io->deinitialize());
				// Выходиим из функции, так как IP-адрес не подходит для тестирования
				return;
			}
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(source.iface.empty());
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Формируем адрес Broadcast
			addr.v4((addr.v4(awh::net_addr_t::endian_t::BIG) & 0xFFFFFF00U) | 0x000000FFU, awh::net_addr_t::endian_t::BIG);
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], static_cast <std::string> (addr)));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
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
			// Записываем в лог сообщение о пропуске теста для VPN-интерфейса
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
		// Устанавливаем размер буфера для чтения и записи
		this->_io->setBufferSize(events[i], awh::event::action_t::READ, 10240);
		this->_io->setBufferSize(events[i], awh::event::action_t::WRITE, 10240);
	}
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем адрес текстового файла для чтения
	ASSERT_TRUE(this->_io->setAddress(fid, awh::event::address_t::FS, "../README.md"));
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([fid, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Выполняем фиксацию события файла
				ASSERT_TRUE(this->_io->commit(fid));
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(fid, awh::event::options::KEEPALIVE));
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о типе узла события
					this->_log->print("Тип узла события: Не определён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
				break;
				case static_cast <uint8_t> (awh::event::vnode_t::CHR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Символьный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
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
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Блочный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
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
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Канал FIFO добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
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
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Сокет добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
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
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Файл добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
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
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Каталог добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение о типе узла события
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
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Символическая ссылка добавлена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение о типе узла события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому каталогу
		ASSERT_TRUE(this->_io->setAddress(did, awh::event::address_t::FS, "./"));
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
		this->_io->on(fid, [&stop, this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING): {
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					// Устанавливаем смещение в файле
					// this->_io->getSeek(eid, 1024);
					// Отправляем тестовое сообщение в файл
					this->_io->send(eid, "Hello World!!!", 14);
					// Останавливаем тест
					stop = true;
				} break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(fid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(fid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому файлу
		ASSERT_TRUE(this->_io->setAddress(fid, awh::event::address_t::FS, "./tmp.txt"));
		// Выполняем фиксацию настроек события файла
		ASSERT_TRUE(this->_io->commit(fid));
		// Устанавливаем опции события
		ASSERT_TRUE(this->_io->setOptions(fid, awh::event::options::KEEPALIVE));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
		}
	});
	// Устанавливаем функцию обратного вызова на запись в событие
	this->_io->on(eid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
		// Записываем в лог сообщение о переподключении события
		this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
	}));
	// Устанавливаем функцию обратного вызова на чтение из события
	this->_io->on(eid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const std::string message(reinterpret_cast <const char *> (data), size);
		// Записываем в лог сообщение о переподключении события
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
				// Записываем ошибку в лог неизвестного события
				this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недопустимой операции
			case static_cast <uint8_t> (awh::event::error_t::INVALID):
				// Записываем ошибку в лог недопустимой операции
				this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа запрещёния
			case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
				// Записываем ошибку в лог доступа запрещёния
				this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка уже существующего объекта
			case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
				// Записываем ошибку в лог уже существующего объекта
				this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа к сокету
			case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
				// Записываем ошибку в лог доступа к сокету
				this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка некорректного адреса
			case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
				// Записываем ошибку в лог некорректного адреса
				this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка ошибки подключения
			case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
				// Записываем ошибку в лог подключения
				this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недостаточно ресурсов
			case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
				// Записываем ошибку в лог недостаточно ресурсов
				this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка события
			case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
				// Записываем ошибку в лог события
				this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если объект не найден
			case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
				// Записываем ошибку в лог события
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
				// Записываем в лог сообщение о чтении события
				this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является записью
			case static_cast <uint8_t> (awh::event::action_t::WRITE):
				// Записываем в лог сообщение о записи события
				this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является подключением
			case static_cast <uint8_t> (awh::event::action_t::CONNECT):
				// Записываем в лог сообщение о подключении события
				this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отключением
			case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
				// Записываем в лог сообщение об отключении события
				this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переподключением
			case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является закрытием
			case static_cast <uint8_t> (awh::event::action_t::CLOSE):
				// Записываем в лог сообщение о закрытии события
				this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением
			case static_cast <uint8_t> (awh::event::action_t::CHANGE):
				// Записываем в лог сообщение об изменении события
				this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является удалением
			case static_cast <uint8_t> (awh::event::action_t::DELETE):
				// Записываем в лог сообщение об удалении события
				this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переименованием
			case static_cast <uint8_t> (awh::event::action_t::RENAME):
				// Записываем в лог сообщение о переименовании события
				this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением атрибутов
			case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
				// Записываем в лог сообщение об изменении атрибутов события
				this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отзывом доступа
			case static_cast <uint8_t> (awh::event::action_t::REVOKE):
				// Записываем в лог сообщение об отзыве доступа события
				this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением счётчика жёстких ссылок
			case static_cast <uint8_t> (awh::event::action_t::HDLINK):
				// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
 * @note Тест отключён: многоадресная передача требует, чтобы её пропускал
 *       сетевой интерфейс, а через VPN она не проходит вовсе, поэтому
 *       результат прогона зависит от того, где он выполняется, а не от кода
 */
TEST_F(IoFixture, DISABLED_IoMulticast1Test){
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->setHops(events[1], awh::event::hops_t::NETWORK));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[1], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Флаг отправки сообщений
			static bool sending = false;
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [eid, this](const awh::event::id_t cid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
		// Устанавливаем количество хопов события
		ASSERT_TRUE(this->_io->setHops(events[0], awh::event::hops_t::NETWORK));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[0], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "239.255.1.1"));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
 * @note Тест отключён: многоадресная передача требует, чтобы её пропускал
 *       сетевой интерфейс, а через VPN она не проходит вовсе, поэтому
 *       результат прогона зависит от того, где он выполняется, а не от кода
 */
TEST_F(IoFixture, DISABLED_IoMulticast3Test){
	/**
	 * 3. Сервер-обнаружение
	 * Клиенты слушают 239.255.1.2,
	 * Сервер периодически рассылает "я здесь",
	 * Клиенты отвечают прямо серверу (unicast), чтобы установить соединение.
	 */
	// Флаг остановки теста
	bool stop = false;
	/**
	 * Количество прочитанных сообщений. Счётчик объявлен в области видимости теста:
	 * функция обратного вызова захватывает его по ссылке и вызывается уже из цикла
	 * событий, то есть переживает блок настройки события
	 */
	uint8_t count = 0;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
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
		this->_io->setTimeout(tid, awh::event::action_t::NONE, 5000);
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
				// Записываем ошибку в лог отправки сообщения
				this->_log->print("Сообщение отправлено: ID=%u, %s", awh::log_t::flag_t::INFO, events[1], message.c_str());
		});
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->setHops(events[1], awh::event::hops_t::NETWORK));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[1], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
		ASSERT_TRUE(this->_io->setDelivery(events[0], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
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
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&count, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_coder->encrypt(id, response.c_str(), response.size()))
							// Записываем в лог сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных TLS
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"http/1.1"}});
		// this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем имя хоста TLS
		this->_coder->serverNameIndication(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
					// Записываем ошибку в лог транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
					// Записываем в лог сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
					// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Возвращаем данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
					// Записываем в лог информацию о TLS соединении
					std::cout << this->_coder->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_coder->encrypt(id, request.c_str(), request.size()))
						// Записываем в лог сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
					// Записываем в лог сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
					// Записываем в лог сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_coder->on(ctl, [&events, &stop, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Записываем в лог сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_coder->decrypt(ctl, data, size))
				// Записываем в лог сообщение об успешном дешифровании данных TLS
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_coder->handshake(ctl))
					// Записываем в лог сообщение о начале рукопожатия TLS
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объекты транспортного уровня безопасности
		awh::tls::coder_t::id_t cts1 = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		awh::tls::coder_t::id_t cts2 = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts1, 0);
		ASSERT_GT(cts2, 0);
		// Устанавливаем режим работы TLS
		this->_coder->mode(cts1, awh::tls::coder_t::mode_t::MULTICERT);
		this->_coder->mode(cts2, awh::tls::coder_t::mode_t::MULTICERT);
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts1, false);
		this->_coder->validateServerNameIndication(cts2, false);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts1, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		this->_coder->alpn(cts2, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts1, "../sh/certificates", "ca.pem");
		this->_coder->ca(cts2, "../sh/certificates", "ca.pem");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts1, "../sh/certificates/example/cert.pem");
		this->_coder->certificate(cts2, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts1, "../sh/certificates/example/key.pem");
		this->_coder->privateKey(cts2, "../sh/certificates/server/key.pem");
		// Устанавливаем имя хоста TLS (Указывать нужно после установки режима работы мультисертификатного TLS!!!!!!!)
		this->_coder->serverNameIndication(cts2, "anyks.com");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts1, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts2, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts1, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts1);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_coder->encrypt(id, response.c_str(), response.size()))
							// Записываем в лог сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных TLS
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"http/1.1"}});
		// this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем имя хоста TLS
		this->_coder->serverNameIndication(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
					// Записываем ошибку в лог транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
					// Записываем в лог сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
					// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Возвращаем данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
					// Записываем в лог информацию о TLS соединении
					std::cout << this->_coder->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_coder->encrypt(id, request.c_str(), request.size()))
						// Записываем в лог сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
					// Записываем в лог сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
					// Записываем в лог сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_coder->on(ctl, [&events, &stop, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Записываем в лог сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_coder->decrypt(ctl, data, size))
				// Записываем в лог сообщение об успешном дешифровании данных TLS
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_coder->handshake(ctl))
					// Записываем в лог сообщение о начале рукопожатия TLS
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
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
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::UDP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
						// Записываем в лог информацию о DTLS соединении
						std::cout << this->_coder->peerInfo(id) << std::endl;
						// Выполняем повторную передачу данных TLS
						ASSERT_TRUE(this->_coder->retransmit(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_coder->encrypt(id, response.c_str(), response.size()))
							// Записываем в лог сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных TLS
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::UDP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"http/1.1"}});
		// this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем имя хоста TLS
		this->_coder->serverNameIndication(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
					// Записываем ошибку в лог транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
					// Записываем в лог сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
					// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Возвращаем данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
					// Записываем в лог информацию о TLS соединении
					std::cout << this->_coder->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_coder->encrypt(id, request.c_str(), request.size()))
						// Записываем в лог сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
					// Записываем в лог сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
					// Записываем в лог сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_coder->on(ctl, [&events, &stop, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Записываем в лог сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_coder->decrypt(ctl, data, size))
				// Записываем в лог сообщение об успешном дешифровании данных TLS
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
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_coder->handshake(ctl))
					// Записываем в лог сообщение о начале рукопожатия TLS
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
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
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
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о переподключении события
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
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
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
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
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
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о переподключении события
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
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
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
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
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
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
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
			/**
			 * Перебираем все извлечённые чанки
			 */
			for(auto & chunk : chunks)
				// Записываем в лог информацию о чанках аутентификации SCTP-сокета
				std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			// Устанавливаем таймаут heartbeat SCTP-сокета
			ASSERT_TRUE(this->_sctp->setTimeout(events[1], awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			// Возвращаем heartbeat timeout SCTP-сокета
			ASSERT_EQ(3000, this->_sctp->getTimeout(events[1], awh::net::sctp::timeout_t::HEARTBEAT));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
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
				/**
				 * Перебираем все извлечённые чанки
				 */
				for(auto & chunk : chunks)
					// Записываем в лог информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Устанавливаем таймаут heartbeat SCTP-сокета
				ASSERT_TRUE(this->_sctp->setTimeout(cid, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
				// Возвращаем heartbeat timeout SCTP-сокета
				ASSERT_EQ(3000, this->_sctp->getTimeout(cid, awh::net::sctp::timeout_t::HEARTBEAT));
				ASSERT_EQ(3000, this->_sctp->getTimeout(sid, awh::net::sctp::timeout_t::HEARTBEAT));
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о переподключении события
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
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
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
			/**
			 * Перебираем все извлечённые чанки
			 */
			for(auto & chunk : chunks)
				// Записываем в лог информацию о чанках аутентификации SCTP-сокета
				std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			// Устанавливаем таймаут heartbeat SCTP-сокета
			ASSERT_TRUE(this->_sctp->setTimeout(events[0], awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
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
						/**
						 * Перебираем все извлечённые чанки
						 */
						for(auto & chunk : chunks)
							// Записываем в лог информацию о чанках аутентификации SCTP-сокета
							std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
						// Устанавливаем таймаут heartbeat SCTP-сокета
						ASSERT_TRUE(this->_sctp->setTimeout(eid, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
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
				/**
				 * Перебираем все извлечённые чанки
				 */
				for(auto & chunk : chunks)
					// Записываем в лог информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Возвращаем heartbeat timeout SCTP-сокета
				ASSERT_EQ(3000, this->_sctp->getTimeout(eid, awh::net::sctp::timeout_t::HEARTBEAT));
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
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
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::SCTP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем клиентский сертификат DTLS
			this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Создаём идентификатор транспортного уровня DTLS
				awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
				// Проверяем, что идентификатор транспортного уровня больше нуля
				ASSERT_GT(ctl, 0);
				// Устанавливаем клиента DTLS для события
				this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
					// Записываем в лог сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
							// Записываем в лог сообщение о записи зашифрованных данных DTLS
							this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
							// Записываем в лог сообщение о записи дешифрованных данных DTLS
							this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
							// Записываем ошибку в лог транспортного уровня TLS
							this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
							// Записываем в лог сообщение об успешном удалении контекста TLS
							this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
							std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
							std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
							std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
							std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
							std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
							// Возвращаем данные сертификата DTLS
							std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							this->_log->print("Рукопожатие DTLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
							// Записываем в лог информацию о DTLS соединении
							std::cout << this->_coder->peerInfo(id) << std::endl;
							// Выполняем повторную передачу данных TLS
							ASSERT_TRUE(this->_coder->retransmit(id));
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const std::string response(reinterpret_cast <const char *> (buffer), size);
							// Записываем в лог сообщение полученных данных с сервера
							this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(this->_coder->encrypt(id, response.c_str(), response.size()))
								// Записываем в лог сообщение об успешном шифровании данных DTLS
								this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы DTLS
					if(this->_coder->decrypt(ctl, data, size))
						// Записываем в лог сообщение об успешном дешифровании данных DTLS
						this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
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
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::SCTP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем имя хоста DTLS
			this->_coder->serverNameIndication(cts, "server.anyks.com");
			// Устанавливаем клиентский сертификат DTLS
			this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
			// Создаём идентификатор транспортного уровня DTLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния DTLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Возвращаем данные сертификата DTLS
						std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
						// Записываем в лог информацию о DTLS соединении
						std::cout << this->_coder->peerInfo(id) << std::endl;
						// Текст запроса к серверу
						const std::string request =
							"GET / HTTP/1.1\r\n"
							"Host: www.google.com\r\n"
							"Connection: close\r\n"
							"User-Agent: iouring-openssl-sample/1.0\r\n"
							"\r\n";
						// Если данные успешно зашифрованы DTLS
						if(this->_coder->encrypt(id, request.c_str(), request.size()))
							// Записываем в лог сообщение об успешном шифровании данных DTLS
							this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на запись данных DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных DTLS
						this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных DTLS
						this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Регистрируем функцию обратного вызова на чтение данных DTLS
			this->_coder->on(ctl, [&stop, &events, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
					} break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Останавливаем тест
						stop = true;
					} break;
				}
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
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
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных DTLS
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Если рукопожатие DTLS успешно
					if(this->_coder->handshake(ctl))
						// Записываем в лог сообщение о начале рукопожатия DTLS
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
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
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем клиентский сертификат DTLS
			this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Создаём идентификатор транспортного уровня DTLS
				awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
				// Проверяем, что идентификатор транспортного уровня больше нуля
				ASSERT_GT(ctl, 0);
				// Устанавливаем клиента DTLS для события
				this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
					// Записываем в лог сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
							// Записываем в лог сообщение о записи зашифрованных данных DTLS
							this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
							// Записываем в лог сообщение о записи дешифрованных данных DTLS
							this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
							// Записываем ошибку в лог транспортного уровня TLS
							this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
							// Записываем в лог сообщение об успешном удалении контекста TLS
							this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
							std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
							std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
							std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
							std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
							std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
							// Возвращаем данные сертификата TLS
							std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
							// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
							this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::KEEPALIVE));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const std::string response(reinterpret_cast <const char *> (buffer), size);
							// Записываем в лог сообщение полученных данных с сервера
							this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(this->_coder->encrypt(id, response.c_str(), response.size()))
								// Записываем в лог сообщение об успешном шифровании данных DTLS
								this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы TLS
					if(this->_coder->decrypt(ctl, data, size)){
						// Записываем в лог сообщение об успешном дешифровании данных TLS
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
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
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
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем имя хоста DTLS
			this->_coder->serverNameIndication(cts, "server.anyks.com");
			// Устанавливаем клиентский сертификат TLS
			this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
			// Устанавливаем приватный ключ TLS
			this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Возвращаем данные сертификата DTLS
						std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
						// Записываем в лог информацию о DTLS соединении
						std::cout << this->_coder->peerInfo(id) << std::endl;
						// Текст запроса к серверу
						const std::string request =
							"GET / HTTP/1.1\r\n"
							"Host: www.google.com\r\n"
							"Connection: close\r\n"
							"User-Agent: iouring-openssl-sample/1.0\r\n"
							"\r\n";
						// Если данные успешно зашифрованы DTLS
						if(this->_coder->encrypt(id, request.c_str(), request.size()))
							// Записываем в лог сообщение об успешном шифровании данных DTLS
							this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на запись данных DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных DTLS
						this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных DTLS
						this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Регистрируем функцию обратного вызова на чтение данных DTLS
			this->_coder->on(ctl, [&stop, &events, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
					} break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Останавливаем тест
						stop = true;
					} break;
				}
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
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
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных DTLS
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
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Если рукопожатие DTLS успешно
					if(this->_coder->handshake(ctl))
						// Записываем в лог сообщение о начале рукопожатия DTLS
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
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
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

/**
 * @brief Тест маршрутизации дейтаграмм по ключу приложения
 *
 * @details Протоколы с собственной адресацией сессий внутри датаграммы
 *          маршрутизируются не четвёркой сокета, а ключом. Тест проверяет три
 *          свойства такой маршрутизации: две четвёрки с одним ключом попадают
 *          в одну сессию, ответ уходит на адрес последней принятой датаграммы,
 *          а датаграмма без ключа отбрасывается без создания сессии
 *
 */
TEST_F(IoFixture, IoOriginKeyedRoutingTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий
	size_t sessions = 0;
	// Количество отброшенных датаграмм
	size_t dropped = 0;
	// Идентификатор события созданной сессии
	awh::event::id_t session = 0;
	// Буфер принятых сессией датаграмм
	std::vector <std::string> received;
	// Буфер ответов с идентификаторами принявших их клиентов
	std::vector <std::pair <awh::event::id_t, std::string>> answers;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем событие сервера UDP
	const awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что событие сервера создано
	ASSERT_GT(server, 0u);
	// Добавляем первое событие клиента UDP
	const awh::event::id_t first = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем второе событие клиента UDP
	const awh::event::id_t second = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события клиентов и таймера созданы
	ASSERT_GT(first, 0u);
	ASSERT_GT(second, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порт события сервера
	ASSERT_TRUE(this->_io->setSourcePort(server, port));
	// Устанавливаем порт назначения событий клиентов
	ASSERT_TRUE(this->_io->setTargetPort(first, port));
	ASSERT_TRUE(this->_io->setTargetPort(second, port));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции события сервера
	ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем адрес события сервера
	ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
	/**
	 * Устанавливаем функцию обратного вызова определения сессии: ключом служат
	 * первые четыре октета датаграммы, а датаграмма короче ключа объявляется
	 * посторонней
	 */
	this->_io->on(server, static_cast <awh::engine::callback::origin_t> ([&dropped](
		[[maybe_unused]] const awh::event::id_t eid, const uint8_t * data, const size_t size, awh::net::origin_key_t & key
	) noexcept -> bool {
		// Если датаграмма короче ключа сессии
		if(size < 4){
			// Увеличиваем количество отброшенных датаграмм
			dropped++;
			// Выводим отрицательный результат - датаграмма посторонняя
			return false;
		}
		// Формируем ключ сессии из первых октетов датаграммы
		key = awh::net::origin_key_t(data, 4);
		// Выводим положительный результат
		return true;
	}));
	// Устанавливаем функцию обратного вызова на создание сессии
	this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&](
		[[maybe_unused]] const awh::event::id_t eid, const awh::event::id_t oid
	) noexcept -> void {
		// Увеличиваем количество созданных сессий
		sessions++;
		// Запоминаем идентификатор события созданной сессии
		session = oid;
		// Устанавливаем функцию обратного вызова на чтение из сессии
		this->_io->on(oid, [&](const awh::event::id_t oid, const uint8_t * data, const size_t size) noexcept -> void {
			// Накапливаем принятую сессией датаграмму
			received.emplace_back(reinterpret_cast <const char *> (data), size);
			// Отправляем ответ по адресу последней принятой датаграммы
			this->_io->send(oid, data, size);
		});
	}));
	/**
	 * Устанавливаем функции обратного вызова на чтение ответов клиентами
	 */
	for(auto & client : {first, second}){
		/**
		 * Устанавливаем функцию обратного вызова на подключение события клиента:
		 * до завершения подключения событие к отправке не готово
		 */
		this->_io->on(client, static_cast <awh::engine::callback::connect_t> ([&, client](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Если подключение не выполнено
			if(!ok)
				// Выходим из функции обработки
				return;
			// Если подключено первое событие клиента
			if(client == first){
				// Отправляем посторонную датаграмму короче ключа сессии
				this->_io->send(eid, "xx", 2);
				// Отправляем датаграмму с ключом сессии первым клиентом
				this->_io->send(eid, "KEY0first", 9);
			// Отправляем датаграмму с тем же ключом сессии вторым клиентом
			} else this->_io->send(eid, "KEY0second", 10);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события клиента
		this->_io->on(client, [&](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Накапливаем принятый клиентом ответ вместе с идентификатором клиента
			answers.emplace_back(eid, std::string(reinterpret_cast <const char *> (data), size));
			// Останавливаем тест после ответа второму клиенту
			stop = (answers.size() >= 2);
		});
		// Устанавливаем опции события клиента
		ASSERT_TRUE(this->_io->setOptions(client, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
		// Устанавливаем локальный адрес события клиента
		ASSERT_TRUE(this->_io->setAddress(client, awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес назначения события клиента
		ASSERT_TRUE(this->_io->setTarget(client, "127.0.0.1"));
	}
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 3000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(server));
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(server));
	ASSERT_TRUE(this->_io->launch(timer));
	/**
	 * Выполняем подготовку и запуск событий клиентов
	 */
	for(auto & client : {first, second}){
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(client));
		// Выполняем подключение события клиента к серверу
		ASSERT_TRUE(this->_io->connect(client));
		// Выполняем запуск события клиента
		ASSERT_TRUE(this->_io->launch(client));
	}
	/**
	 *  Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Проверяем что посторонняя датаграмма отброшена
	ASSERT_EQ(dropped, 1u);
	/**
	 * Проверяем что две четвёрки с одним ключом попали в одну сессию: посторонняя
	 * датаграмма сессии не создала, а вторая четвёрка не создала новой
	 */
	ASSERT_EQ(sessions, 1u);
	// Проверяем что идентификатор события сессии получен
	ASSERT_GT(session, 0u);
	// Проверяем что сессия приняла обе датаграммы
	ASSERT_EQ(received.size(), 2u);
	// Проверяем содержимое принятых сессией датаграмм
	ASSERT_EQ(received[0], "KEY0first");
	ASSERT_EQ(received[1], "KEY0second");
	// Проверяем что оба ответа доставлены
	ASSERT_EQ(answers.size(), 2u);
	/**
	 * Проверяем что ответы ушли по адресам отправителей: сессия следует за
	 * последней принятой датаграммой, поэтому второй ответ получил второй
	 * клиент, а не первый, от которого сессия была заведена
	 */
	ASSERT_EQ(answers[0].first, first);
	ASSERT_EQ(answers[0].second, "KEY0first");
	ASSERT_EQ(answers[1].first, second);
	ASSERT_EQ(answers[1].second, "KEY0second");
}

/**
 * @brief Тест предела количества сессий дейтаграммного события
 *
 * @details Предел защищает от исчерпания памяти потоком датаграмм с разными
 *          ключами: по его достижении новые сессии не создаются
 *
 */
TEST_F(IoFixture, IoOriginLimitTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий
	size_t sessions = 0;
	// Количество отказов в создании сессии
	size_t cancelled = 0;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем событие сервера UDP
	const awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие клиента UDP
	const awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(server, 0u);
	ASSERT_GT(client, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порт события сервера
	ASSERT_TRUE(this->_io->setSourcePort(server, port));
	// Устанавливаем порт назначения события клиента
	ASSERT_TRUE(this->_io->setTargetPort(client, port));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции события сервера
	ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем адрес события сервера
	ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
	// Проверяем предельное количество сессий по умолчанию
	ASSERT_EQ(this->_io->getMaxConnections(server), 100u);
	// Устанавливаем предельное количество сессий события
	ASSERT_TRUE(this->_io->setMaxConnections(server, 1));
	// Проверяем что предельное количество сессий установлено
	ASSERT_EQ(this->_io->getMaxConnections(server), 1u);
	// Проверяем отказ установки нулевого предела
	ASSERT_FALSE(this->_io->setMaxConnections(server, 0));
	// Устанавливаем функцию обратного вызова определения сессии
	this->_io->on(server, static_cast <awh::engine::callback::origin_t> ([](
		[[maybe_unused]] const awh::event::id_t eid, const uint8_t * data, const size_t size, awh::net::origin_key_t & key
	) noexcept -> bool {
		// Если датаграмма короче ключа сессии
		if(size < 4)
			// Выводим отрицательный результат - датаграмма посторонняя
			return false;
		// Формируем ключ сессии из первых октетов датаграммы
		key = awh::net::origin_key_t(data, 4);
		// Выводим положительный результат
		return true;
	}));
	// Устанавливаем функцию обратного вызова на создание сессии
	this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&sessions](
		[[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::id_t oid
	) noexcept -> void {
		// Увеличиваем количество созданных сессий
		sessions++;
	}));
	// Устанавливаем функцию обратного вызова на изменение статуса события сервера
	this->_io->on(server, static_cast <awh::engine::callback::status_t> ([&cancelled](
		[[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status
	) noexcept -> void {
		// Если получен отказ в создании сессии
		if(status == awh::event::status_t::CANCELLED)
			// Увеличиваем количество отказов в создании сессии
			cancelled++;
	}));
	// Устанавливаем опции события клиента
	ASSERT_TRUE(this->_io->setOptions(client, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем локальный адрес события клиента
	ASSERT_TRUE(this->_io->setAddress(client, awh::event::address_t::IPV4, "0.0.0.0"));
	// Устанавливаем адрес назначения события клиента
	ASSERT_TRUE(this->_io->setTarget(client, "127.0.0.1"));
	/**
	 * Устанавливаем функцию обратного вызова на подключение события клиента:
	 * до завершения подключения событие к отправке не готово
	 */
	this->_io->on(client, static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
		// Если подключение не выполнено
		if(!ok)
			// Выходим из функции обработки
			return;
		// Отправляем датаграмму с первым ключом сессии
		this->_io->send(eid, "AAAApayload", 11);
		// Отправляем датаграмму со вторым ключом сессии
		this->_io->send(eid, "BBBBpayload", 11);
	}));
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 1000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(server));
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(server));
	ASSERT_TRUE(this->_io->launch(timer));
	// Выполняем фиксацию настроек события клиента
	ASSERT_TRUE(this->_io->commit(client));
	// Выполняем подключение события клиента к серверу
	ASSERT_TRUE(this->_io->connect(client));
	// Выполняем запуск события клиента
	ASSERT_TRUE(this->_io->launch(client));
	/**
	 *  Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Проверяем что создана только одна сессия
	ASSERT_EQ(sessions, 1u);
	// Проверяем что в создании второй сессии отказано
	ASSERT_GE(cancelled, 1u);
}

/**
 * @brief Тест разделения сессий между дейтаграммными событиями
 *
 * @details Ключ сессии закреплён за событием, поэтому один и тот же отправитель,
 *          обратившийся к двум серверам, порождает две независимые сессии,
 *          а не одну общую. Датаграммы отправляются одним сокетом, чтобы
 *          четвёрка источника у обоих серверов совпадала
 *
 */
TEST_F(IoFixture, IoOriginNamespaceTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий первого сервера
	size_t first = 0;
	// Количество созданных сессий второго сервера
	size_t second = 0;
	// Выполняем генерацию порта первого сервера
	const uint16_t primary = ::port();
	// Выполняем генерацию порта второго сервера
	const uint16_t secondary = static_cast <uint16_t> ((primary > 49152) ? (primary - 1) : (primary + 1));
	// Добавляем событие первого сервера UDP
	const awh::event::id_t alpha = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие второго сервера UDP
	const awh::event::id_t beta = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(alpha, 0u);
	ASSERT_GT(beta, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порты событий серверов
	ASSERT_TRUE(this->_io->setSourcePort(alpha, primary));
	ASSERT_TRUE(this->_io->setSourcePort(beta, secondary));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выполняем настройку событий серверов
	 */
	for(auto & server : {alpha, beta}){
		// Устанавливаем опции события сервера
		ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
		// Устанавливаем адрес события сервера
		ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на создание сессии
		this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&, server](
			[[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::id_t oid
		) noexcept -> void {
			// Увеличиваем счётчик сессий соответствующего сервера
			(server == alpha ? first : second)++;
			// Останавливаем тест после создания сессий обоими серверами
			stop = ((first > 0) && (second > 0));
		}));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(server));
		// Выполняем запуск события сервера
		ASSERT_TRUE(this->_io->launch(server));
	}
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 2000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек события таймера
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск события таймера
	ASSERT_TRUE(this->_io->launch(timer));
	/**
	 * Отправляем датаграммы обоим серверам одним сокетом: разные сокеты дали бы
	 * разные порты источника, и совпадения четвёрок, ради которого ставится
	 * проверка, не возникло бы
	 */
	const int32_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Проверяем что сокет отправителя создан
	ASSERT_GE(sock, 0);
	// Объект адреса назначения датаграммы
	struct sockaddr_in target{};
	// Устанавливаем семейство адреса назначения
	target.sin_family = AF_INET;
	// Устанавливаем адрес назначения датаграммы
	target.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	/**
	 * Перебираем порты серверов
	 */
	for(auto & port : {primary, secondary}){
		// Устанавливаем порт назначения датаграммы
		target.sin_port = htons(port);
		// Отправляем датаграмму серверу
		ASSERT_GT(::sendto(sock, "SAMEpayload", 11, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	}
	// Закрываем сокет отправителя
	::close(sock);
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	/**
	 * Проверяем что каждый сервер завёл собственную сессию: ключ закреплён за
	 * событием, поэтому общего пространства у них нет, хотя четвёрка источника
	 * у обеих датаграмм одна
	 */
	ASSERT_EQ(first, 1u);
	ASSERT_EQ(second, 1u);
}

/**
 * @brief Тест привязки дополнительных ключей маршрутизации к сессии
 *
 * @details Протоколы со сменой идентификатора на лету адресуют одну сессию
 *          произвольным числом ключей: датаграмма с любым привязанным ключом
 *          обязана попасть в неё, а снятый ключ маршрутизироваться перестаёт
 *
 */
TEST_F(IoFixture, IoOriginBindTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий
	size_t sessions = 0;
	// Идентификатор события созданной сессии
	awh::event::id_t session = 0;
	// Буфер принятых датаграмм с идентификаторами принявших их сессий
	std::vector <std::pair <awh::event::id_t, std::string>> received;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем событие сервера UDP
	const awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(server, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порт события сервера
	ASSERT_TRUE(this->_io->setSourcePort(server, port));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции события сервера
	ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем адрес события сервера
	ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем функцию обратного вызова определения сессии
	this->_io->on(server, static_cast <awh::engine::callback::origin_t> ([](
		[[maybe_unused]] const awh::event::id_t eid, const uint8_t * data, const size_t size, awh::net::origin_key_t & key
	) noexcept -> bool {
		// Если датаграмма короче ключа сессии
		if(size < 4)
			// Выводим отрицательный результат - датаграмма посторонняя
			return false;
		// Формируем ключ сессии из первых октетов датаграммы
		key = awh::net::origin_key_t(data, 4);
		// Выводим положительный результат
		return true;
	}));
	// Устанавливаем функцию обратного вызова на создание сессии
	this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&](
		[[maybe_unused]] const awh::event::id_t eid, const awh::event::id_t oid
	) noexcept -> void {
		// Увеличиваем количество созданных сессий
		sessions++;
		// Запоминаем идентификатор события созданной сессии
		session = oid;
		// Устанавливаем функцию обратного вызова на чтение из сессии
		this->_io->on(oid, [&](const awh::event::id_t oid, const uint8_t * data, const size_t size) noexcept -> void {
			// Накапливаем принятую датаграмму вместе с идентификатором принявшей сессии
			received.emplace_back(oid, std::string(reinterpret_cast <const char *> (data), size));
			// Останавливаем тест после приёма всех ожидаемых датаграмм
			stop = (received.size() >= 2);
		});
	}));
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 2000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(server));
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(server));
	ASSERT_TRUE(this->_io->launch(timer));
	// Создаём сокет отправителя датаграмм
	const int32_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Проверяем что сокет отправителя создан
	ASSERT_GE(sock, 0);
	// Объект адреса назначения датаграммы
	struct sockaddr_in target{};
	// Устанавливаем семейство адреса назначения
	target.sin_family = AF_INET;
	// Устанавливаем порт назначения датаграммы
	target.sin_port = htons(port);
	// Устанавливаем адрес назначения датаграммы
	target.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	// Отправляем датаграмму с исходным ключом сессии
	ASSERT_GT(::sendto(sock, "AAAAfirst", 9, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	/**
	 * Выполняем опрос событий до создания сессии
	 */
	while(!stop && (sessions == 0) && this->_io->poll());
	// Проверяем что сессия создана
	ASSERT_EQ(sessions, 1u);
	// Проверяем что идентификатор события сессии получен
	ASSERT_GT(session, 0u);
	/**
	 * Запоминаем идентификатор первой сессии отдельно: обработчик создания
	 * сессии перезаписывает его при каждом принятии
	 */
	const awh::event::id_t primary = session;
	// Формируем дополнительный ключ маршрутизации сессии
	const awh::net::origin_key_t extra(reinterpret_cast <const uint8_t *> ("BBBB"), 4);
	// Привязываем дополнительный ключ к созданной сессии
	ASSERT_TRUE(this->_io->bind(primary, extra));
	// Проверяем что повторная привязка того же ключа к той же сессии допустима
	ASSERT_TRUE(this->_io->bind(primary, extra));
	// Проверяем отказ привязки пустого ключа
	ASSERT_FALSE(this->_io->bind(session, awh::net::origin_key_t()));
	// Проверяем отказ привязки ключа к несуществующей сессии
	ASSERT_FALSE(this->_io->bind(0, extra));
	// Отправляем датаграмму с дополнительным ключом маршрутизации
	ASSERT_GT(::sendto(sock, "BBBBsecond", 10, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	/**
	 *  Выполняем опрос событий до приёма датаграммы
	 */
	while(!stop && this->_io->poll());
	/**
	 * Проверяем что датаграмма с дополнительным ключом попала в ту же сессию:
	 * новая сессия под неё создаваться не должна
	 */
	ASSERT_EQ(sessions, 1u);
	// Проверяем что принято две датаграммы
	ASSERT_EQ(received.size(), 2u);
	// Проверяем что обе датаграммы приняты одной и той же сессией
	ASSERT_EQ(received[0].first, primary);
	ASSERT_EQ(received[1].first, primary);
	// Проверяем содержимое принятых сессией датаграмм
	ASSERT_EQ(received[0].second, "AAAAfirst");
	ASSERT_EQ(received[1].second, "BBBBsecond");
	// Снимаем дополнительный ключ с сессии
	ASSERT_TRUE(this->_io->unbind(primary, extra));
	// Проверяем что повторное снятие того же ключа отвергается
	ASSERT_FALSE(this->_io->unbind(primary, extra));
	// Сбрасываем флаг остановки теста
	stop = false;
	// Отправляем датаграмму со снятым с маршрутизации ключом
	ASSERT_GT(::sendto(sock, "BBBBthird", 9, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	/**
	 *  Выполняем опрос событий до создания новой сессии
	 */
	while(!stop && (sessions < 2) && this->_io->poll());
	// Закрываем сокет отправителя
	::close(sock);
	/**
	 * Проверяем что снятый ключ маршрутизироваться перестал: датаграмма с ним
	 * образовала новую сессию, а не попала в прежнюю
	 */
	ASSERT_EQ(sessions, 2u);
	// Проверяем что датаграмма со снятым ключом принята
	ASSERT_EQ(received.size(), 3u);
	// Проверяем что приняла её другая сессия, а не прежняя
	ASSERT_NE(received[2].first, primary);
	// Проверяем содержимое датаграммы со снятым ключом
	ASSERT_EQ(received[2].second, "BBBBthird");
}
