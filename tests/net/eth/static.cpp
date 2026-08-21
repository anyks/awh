/**
 * @file static.cpp
 * @date 2025-12-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Статические тесты модуля работы с сетевым уровнем Ethernet — проверка создания и сброса объекта модуля,
 *        а также корректности перечисления интерфейсов, получения адресов и маршрутов,
 *        настройки сокетов и проброса портов
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем стандартные модули
 */
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют:
 *       соответствующие им объявления приходят там из winsock2.h,
 *       подключаемого через единую точку sys/win32.hpp
 *
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Тест создания объекта работы с Ethernet
 *
 */
TEST_F(EthFixture, CreateEthTest){
	// Проверяем, что объект работы с Ethernet создан
	ASSERT_TRUE(this->_eth != nullptr);
	// Сбрасываем объект работы с Ethernet
	this->_eth.reset();
	// Проверяем, что объект работы с Ethernet сброшен
	ASSERT_TRUE(this->_eth == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта работы с Ethernet
 *
 */
TEST_F(EthFixture, ResetAndCreateEthTest){
	// Проверяем, что объект работы с Ethernet создан
	ASSERT_TRUE(this->_eth != nullptr);
	// Сбрасываем объект работы с Ethernet
	this->_eth.reset();
	// Проверяем, что объект работы с Ethernet сброшен
	ASSERT_TRUE(this->_eth == nullptr);
	// Создаём объект работы с Ethernet заново
	this->_eth = std::make_unique <awh::eth_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект работы с Ethernet создан
	ASSERT_TRUE(this->_eth != nullptr);
}

/**
 * @brief Тест повторного создания объекта работы с Ethernet
 *
 */
TEST_F(EthFixture, ReCreateEthTest){
	// Проверяем, что объект работы с Ethernet создан
	ASSERT_TRUE(this->_eth != nullptr);
	// Создаём объект работы с Ethernet заново
	this->_eth = std::make_unique <awh::eth_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект работы с Ethernet создан
	ASSERT_TRUE(this->_eth != nullptr);
}

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(EthFixture, EthSuiteTest){
	// Создаём UDP сокет
	awh::net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Проверяем, что сокет создан
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Устанавливаем таймаут на чтение сокета
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::READ, 32000));
	// Устанавливаем таймаут на запись сокета
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::WRITE, 32000));

	// Получаем таймаут на чтение сокета
	ASSERT_EQ(32000, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::READ));
	// Получаем таймаут на запись сокета
	ASSERT_EQ(32000, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::WRITE));

	// Получаем размер буфера на чтение сокета
	int32_t rcvbuf = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::READ);
	// Проверяем, что размер буфера на чтение сокета получен
	ASSERT_GT(rcvbuf, 0);
	// Устанавливаем размер буфера на чтение сокета
	ASSERT_GT(this->_eth->socket.setBufferSize(sock, awh::net::socket_event_t::READ, rcvbuf * 2), 0);
	// Получаем размер буфера на запись сокета
	int32_t sndbuf = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::WRITE);
	// Проверяем, что размер буфера на запись сокета получен
	ASSERT_GT(sndbuf, 0);
	// Устанавливаем размер буфера на запись сокета
	ASSERT_GT(this->_eth->socket.setBufferSize(sock, awh::net::socket_event_t::WRITE, sndbuf * 2), 0);
	// Блокируем сигнал SIGILL
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_SIGILL));
	/**
	 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
	 *
	 * @details Признак взят тот же, что и у самого модуля SCTP в net.cpp - иначе набор
	 *          и то, что он проверяет, расходились бы в понимании, где протокол есть
	 *
	 * @note У NetBSD заголовки протокола есть, но ядро его не даёт, у OpenBSD нет и
	 *       заголовков - обе системы сюда не входят намеренно
	 *
	 */
	#if __linux__ || __FreeBSD__ || __sun
		// Активируем получение SCTP-событий для сокета
		ASSERT_FALSE(this->_eth->sctp.eventsSubscribe(sock, {
			awh::net::sctp::event_type_t::ASSOC_CHANGE,
			awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
			awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
			awh::net::sctp::event_type_t::REMOTE_ERROR,
			awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
		}));
		// Текст инициализационных сообщений SCTP
		awh::net::sctp::initmsg_t initmsg;
		// Устанавливаем количество попыток подключения SCTP
		initmsg.attempts = 4;
		// Устанавливаем количество исходящих потоков SCTP
		initmsg.ostreams = 5;
		// Устанавливаем количество входящих потоков SCTP
		initmsg.istreams = 5;
		// Инициализируем рукопожатие SCTP для сокета
		ASSERT_FALSE(this->_eth->sctp.initMessages(sock, initmsg));
		// Объект для извлечения статуса SCTP сокета
		awh::net::sctp::status_t status;
		// Получаем статус SCTP сокета
		ASSERT_FALSE(this->_eth->sctp.status(sock, status));
	#endif
	// Получаем код ошибки сокета
	ASSERT_EQ(0, this->_eth->socket.getError(sock));
	// Включаем режим cork для TCP-сокета
	ASSERT_FALSE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::TCP_CORKING));
	/**
	 * Включаем или отключаем режим отображения IPv4 => IPv6
	 *
	 * @details Испытуемый сокет заведён семейством IPv4, а настройка спрашивается у
	 *          IPv6: ядра POSIX такое сочетание отвергают, и отказ здесь утверждается
	 *          именно как их поведение
	 *
	 * @warning MS Windows то же сочетание ПРИНИМАЕТ, обращая настройку пустым
	 *          действием, и отказом не отвечает. Утверждается оттого местное
	 *          поведение каждой системы, а не поведение одной из них: прежде проверка
	 *          ждала отказа всюду и падала на всякой машине Windows
	 */
	#if _WIN32 || _WIN64
		// Настройка принимается и обращается пустым действием
		ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED, awh::event::options::IPV6_ONLY));
	#else
		// Настройка отвергается ядром: семейства сокета и настройки не сходятся
		ASSERT_FALSE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED, awh::event::options::IPV6_ONLY));
	#endif
	// Устанавливаем повторное использование адреса сокета
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_ADDR));
	// Устанавливаем повторное использование порта сокета
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_PORT));
	// Игнорируем отключение сигнала записи в убитый сокет
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_SIGPIPE));
	// Разрешаем широковещательный адрес
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::BROADCAST));
	// Отключаем алгоритм Нейгла
	ASSERT_FALSE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::TCP_NO_DELAY));
	// Устанавливаем блокирующий сокет
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_IO_BLOCK));
	// Устанавливаем режим автоматического закрытия файлового дескриптора при вызове exec
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::CLOSE_ON_EXEC));
	/**
	 * @par Намеренные решения
	 *
	 * Отказ тут ждётся оттого, что сокет создан датаграммным, а сроки проверки
	 * живости задаются опциями уровня IPPROTO_TCP - на таком сокете система их
	 * не принимает, и установка честно проваливается
	 *
	 * У OpenBSD опций TCP_KEEPIDLE, TCP_KEEPINTVL и TCP_KEEPCNT нет вовсе, сроки
	 * там общесистемные. Единственный доступный шаг - SO_KEEPALIVE уровня
	 * SOL_SOCKET, а его система принимает на сокете любого рода. Проваливаться
	 * там попросту нечему, и установка возвращает истину. Это правда о системе,
	 * а не расхождение поведения, потому проверка разделена по системам
	 *
	 */
	#if !__OpenBSD__
		// Устанавливаем постоянное подключение на сокет
		ASSERT_FALSE(this->_eth->socket.setKeepalive(sock, 30, 60, 10));
	#else
		// Устанавливаем постоянное подключение на сокет
		ASSERT_TRUE(this->_eth->socket.setKeepalive(sock, 30, 60, 10));
	#endif
	// Включаем заголовки в сокете
	ASSERT_FALSE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::HDRINCL));
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(source);
	// Проверяем, что название сетевого интерфейса получено
	ASSERT_FALSE(source.iface.empty());
	// Устанавливаем интерфейс мультикаст группы по имени
	ASSERT_TRUE(this->_eth->socket.setMulticastIface(sock, awh::event::family_t::IPV4, source.iface));
	// Устанавливаем режим обратной петли для multicast пакетов
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::MULTICAST_LOOPBACK));

	// Устанавливаем максимальное количество хопов, через которые может пройти пакет
	ASSERT_TRUE(this->_eth->socket.setHops(sock, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST, static_cast <uint8_t> (awh::event::hops_t::NETWORK)));
	// Получаем максимальное количество хопов, через которые может пройти пакет
	ASSERT_EQ(static_cast <uint8_t> (awh::event::hops_t::NETWORK), this->_eth->socket.getHops(sock, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST));

	// Устанавливаем дифференцированные услуги (DSCP) для сокета
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock, awh::event::family_t::IPV4, awh::event::dscp_t::CS0));
	// Получаем дифференцированные услуги (DSCP) для сокета
	ASSERT_EQ(awh::event::dscp_t::CS0, this->_eth->socket.getDifferentiatedServicesCodePoint(sock, awh::event::family_t::IPV4));

	/**
	 * Обнаружение MTU проверяется лишь там, где запрет фрагментации на отдельном
	 * сокете системой задаётся: NetBSD имеет его только для IPv6, OpenBSD - ни для
	 * одного семейства, и обнаружение пути там ведёт ядро само
	 */
	#if defined(IP_DONTFRAG)
		// Устанавливаем обнаружение максимального размера пакета (MTU) для сокета
		ASSERT_TRUE(this->_eth->socket.setMaximumTransmissionUnitDiscover(sock, awh::event::family_t::IPV4, awh::event::mtu_discover_t::DO));
		// Получаем обнаружение максимального размера пакета (MTU) для сокета
		ASSERT_EQ(awh::event::mtu_discover_t::DO, this->_eth->socket.getMaximumTransmissionUnitDiscover(sock, awh::event::family_t::IPV4));
	#endif

	// Вычисляем контрольную сумму транспортного уровня с некорректными данными
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, nullptr, nullptr, nullptr, 0));
	// Выполняем проверку принадлежности IP-адреса подсети
	ASSERT_TRUE(this->_eth->addr.isInSubnet(ntohl(4178028736), htonl(4178028736), 32));
	{
		// Создаём объект IPv4-адреса
		std::unique_ptr <awh::net::addr_net_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Устанавливаем адрес мультикаст-группы
		static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = 0;
		// Устанавливаем членство в мультикаст группе
		ASSERT_FALSE(this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, addr.get(), addr.get()));
	}{
		// Создаём два пустых IPv6-адреса
		uint8_t a[16] = {0};
		uint8_t b[16] = {0};
		// Выполняем проверку равенства префиксов IPv6-адресов
		ASSERT_TRUE(this->_eth->addr.ipv6PrefixEqual(a, b, 16));
	}{
		// Создаём объект IPv4-адреса
		std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Устанавливаем адрес мультикаст-группы
		static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = 0;
		// Получаем имя сетевого интерфейса по IP-адресу
		ASSERT_TRUE(this->_eth->iface.name(addr.get()).empty());
	}{
		// Временный объект для извлечения сетевого интерфейса
		awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
		// Создаём объект IPv4-адреса
		std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Устанавливаем адрес мультикаст-группы
		static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = 0;
		// Выполняем извлечение сетевых параметров
		this->_eth->addr.fillSource(addr.get(), source);
		// Получаем имя сетевого интерфейса по IP-адресу
		ASSERT_TRUE(source.iface.empty());
	}{
		// Временный объект для извлечения сетевого интерфейса
		awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
		// Выполняем извлечение сетевых параметров
		this->_eth->addr.fillSource(source);
		// Получаем имя сетевого интерфейса по IP-адресу
		ASSERT_FALSE(source.iface.empty());
	}{
		// Временный объект для извлечения сетевого интерфейса
		awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
		// Выполняем извлечение сетевых параметров
		this->_eth->addr.fillSource(awh::event::node_t::NONE, source);
		// Получаем имя сетевого интерфейса по IP-адресу
		ASSERT_FALSE(source.iface.empty());
	}
	// Закрываем сокет
	::closesocket(sock);
}
