/**
 * @file tunnel.cpp
 * @date 2026-08-20
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
 * @brief Проверки туннельных устройств асинхронного движка ввода-вывода
 *
 * @details Туннели прежде не проверялись ничем: ни сборка, ни наборы их не трогали
 *          вовсе. Дефекты движка, вскрытые работами по переносу под MS Windows
 *          (приём из устройства, привязка к очереди событий, установка опций,
 *          порядок сноса, снятие с учёта дескриптора, пакет чужого семейства),
 *          вернулись бы молча и узнал бы о них потребитель, а не набор
 *
 * @note Заведение туннельного устройства требует надзорных прав на ВСЕХ системах, а
 *       у MS Windows требует ещё и установленного стороннего драйвера. Оттого
 *       проверки эти пропускаются там, где устройство завести не вышло: опыт
 *       поставить не на чем, и объявлять это отказом движка было бы неверно
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * @brief Восполнение средств POSIX, отсутствующих у MS Windows
 *
 * @note Обращение `::closesocket` у MS Windows приходит из winsock2.h, а у прочих
 *       систем его нет вовсе - там имя это приводится к `::close` посредником
 *       тестового окружения. Без подключения проверка собиралась ОДНОЙ системой из
 *       девяти, и молчание это выдало себя лишь переносом на FreeBSD
 *
 */
#include "../../posix.hpp"

/**
 * Стандартные модули
 */
#include <chrono>
#include <string>

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
	#include <sys/socket.h>
#endif

/**
 * @brief Защита объявлений от макросов операционной системы
 *
 * @note Пара эта нужна по той же причине, что и в соседних проверках: заголовки
 *       MS Windows заводят макросами имена, какие движок называет своими членами
 *
 */
#include <sys/macro_push.hpp>

/**
 * @brief Адреса концов испытуемого туннеля
 *
 * @details Берутся они из блока, отведённого под частные сети (RFC 1918), и берутся
 *          нарочно НЕ из тех блоков, какими пользуются проверки подключений:
 *          устройство туннеля заводится на всю машину, и совпадение адресов
 *          столкнуло бы проверки между собой через таблицу маршрутов
 *
 */
static constexpr const char * TUNNEL_LOCAL = "10.77.0.1";
static constexpr const char * TUNNEL_PEER  = "10.77.0.2";

/**
 * @brief Генерация случайного порта в диапазоне 49152-65535
 *
 * @note Помощник этот повторяет соседний из static.cpp намеренно: связывание у него
 *       внутреннее, и общего состояния между файлами не возникает
 *
 * @return случайный порт
 *
 */
static uint16_t tunnelPort() noexcept {
	// Нижняя и верхняя границы разряда портов, отведённого под временные
	constexpr uint16_t BEGIN = 49152, END = 65535;
	// Счётчик выданных портов
	static uint16_t count = 0;
	// Выводим следующий порт разряда
	return static_cast <uint16_t> (BEGIN + ((static_cast <uint32_t> (::getpid()) + (count++) * 7) % (END - BEGIN)));
}

/**
 * @brief Тест полного оборота жизни туннельного устройства
 *
 * @details Оборот проходится ДВАЖДЫ подряд одним и тем же движком, и это
 *          существенно. Половина дефектов, вскрытых работами по переносу, выдавала
 *          себя не первым заведением, а вторым: дескриптор, не снятый с учёта при
 *          сносе, оставлял за собой запись, и следующее устройство получало отказ
 *          привязки к очереди событий
 *
 * @note Проверяется здесь именно СОГЛАСИЕ движка на каждом шаге, а не обмен: обмен
 *       требует встречной стороны, и разбирается он проверкой соседней
 *
 */
TEST_F(IoFixture, IoTunnelLifecycleTest){
	// Выполняем инициализацию движка
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Проходим оборот жизни устройства дважды
	 */
	for(uint8_t round = 0; round < 2; round++){
		// Роспись оборота для сообщений об отказе
		const std::string sign = std::string("оборот=") + std::to_string(round + 1);
		/**
		 * Заводим событие туннеля
		 *
		 * @note Устройство заводится ЗДЕСЬ, а не фиксацией: пустой опознаватель означает,
		 *       что завести его не вышло - надзорных прав нет либо драйвера. Опыт в таком
		 *       окружении поставить не на чем, и объявлять это отказом движка неверно
		 */
		const awh::event::id_t tid = this->_io->event(awh::event::node_t::TUNNEL, awh::event::family_t::IPV4);
		// Если завести устройство туннеля не удалось
		if(tid == 0){
			// Сворачиваем движок
			this->_io->deinitialize();
			// Пропускаем проверку
			GTEST_SKIP() << "туннельное устройство завести не удалось: нет надзорных прав либо драйвера";
		}
		/**
		 * Устанавливаем опции события туннеля
		 *
		 * @details Отказ здесь был дефектом движка: разбор опций у туннеля шёл общим
		 *          путём с каталогами, файлами и каналами, а тот отвечал отказом
		 *          10038 - «дескриптор не является сокетом». Туннель им и не является
		 */
		ASSERT_TRUE(this->_io->setOptions(tid, awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC)) << sign;
		// Устанавливаем адреса концов туннеля
		ASSERT_TRUE(this->_io->setAddress(tid, awh::event::address_t::IPV4, TUNNEL_LOCAL)) << sign;
		ASSERT_TRUE(this->_io->setTarget(tid, TUNNEL_PEER)) << sign;
		// Фиксируем событие туннеля
		ASSERT_TRUE(this->_io->commit(tid)) << sign;
		// Запускаем событие туннеля
		ASSERT_TRUE(this->_io->launch(tid)) << sign;
		/**
		 * Крутим опрос недолго
		 *
		 * @details Обороты эти нужны не ради пакетов, а ради самой привязки: отказ
		 *          привязки устройства к очереди событий выдавал себя именно здесь -
		 *          опрос возвращал отказ с кодом 6, «неверный описатель»
		 */
		const auto start = std::chrono::steady_clock::now();
		while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 300))
			ASSERT_TRUE(this->_io->poll(50)) << sign;
		// Снимаем событие туннеля
		ASSERT_TRUE(this->_io->destroy(tid)) << sign;
	}
	// Сворачиваем движок
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест переноса пакета из туннеля наружу
 *
 * @details Проверяется весь путь целиком: пакет, посланный на адрес встречной стороны
 *          туннеля, забирается системой в устройство, читается движком, заворачивается
 *          посредником и уходит несущей связью - той самой, на другом конце которой
 *          стоит сервер этой же проверки. Дошедшее сличается с пакетом IPv4
 *
 * @details Проверка эта закрывает дефект настройки драйвера у MS Windows: драйвер
 *          tap-windows6 в режиме переноса пакетов сетевого уровня отбирает их по
 *          настроенной сети, а настраивался он числами, зашитыми в коде. Обмен по
 *          IPv4 через устройство не шёл ВОВСЕ, и выдавала себя беда лишь молчанием
 *          туннеля - ни отказа, ни записи в журнале. Установлено щупом на стенде
 *          Windows ARM64 20.08.2026
 *
 * @note Несущая связь заведена на петле: проверке нужен не обмен между машинами, а
 *       доказательство того, что пакет через туннель ПРОШЁЛ
 *
 */
TEST_F(IoFixture, IoTunnelCarriesPacketTest){
	// Счётчик пакетов, дошедших несущей связью
	uint16_t carried = 0;
	// Признак того, что дошедшее оказалось пакетом IPv4
	bool addressed = false;
	// Порт несущей связи
	const uint16_t bearer = tunnelPort();
	// Выполняем инициализацию движка
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Заводим событие туннеля
	 *
	 * @note Устройство заводится ЗДЕСЬ, а не фиксацией: пустой опознаватель означает
	 *       негодное окружение, а не дефект движка
	 */
	const awh::event::id_t tid = this->_io->event(awh::event::node_t::TUNNEL, awh::event::family_t::IPV4);
	// Если завести устройство туннеля не удалось
	if(tid == 0){
		// Сворачиваем движок
		this->_io->deinitialize();
		// Пропускаем проверку
		GTEST_SKIP() << "туннельное устройство завести не удалось: нет надзорных прав либо драйвера";
	}
	// Заводим событие посредника
	const awh::event::id_t mid = this->_io->event(awh::event::node_t::MEDIATOR, awh::event::family_t::IPV4);
	ASSERT_GT(mid, 0u);
	// Заводим событие сервера несущей связи
	const awh::event::id_t sid = this->_io->event(
		awh::event::node_t::SERVER, awh::event::family_t::IPV4,
		awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP
	);
	ASSERT_GT(sid, 0u);
	// Заводим событие клиента несущей связи
	const awh::event::id_t cid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP
	);
	ASSERT_GT(cid, 0u);
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->setOptions(tid, awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	ASSERT_TRUE(this->_io->setOptions(sid, awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK));
	ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK));
	// Устанавливаем адреса концов туннеля
	ASSERT_TRUE(this->_io->setAddress(tid, awh::event::address_t::IPV4, TUNNEL_LOCAL));
	ASSERT_TRUE(this->_io->setTarget(tid, TUNNEL_PEER));
	// Устанавливаем встречную сторону посреднику
	ASSERT_TRUE(this->_io->setTarget(mid, TUNNEL_PEER));
	// Устанавливаем адрес и порт несущей связи
	ASSERT_TRUE(this->_io->setAddress(sid, awh::event::address_t::IPV4, "127.0.0.1"));
	ASSERT_TRUE(this->_io->setSourcePort(sid, bearer));
	ASSERT_TRUE(this->_io->setTarget(cid, "127.0.0.1"));
	ASSERT_TRUE(this->_io->setTargetPort(cid, bearer));
	// Связываем посредника с клиентом несущей связи
	ASSERT_TRUE(this->_io->splice(mid, cid));
	/**
	 * Считаем прочитанное посредником
	 *
	 * @note Отклик чтения ставится ПОСРЕДНИКУ, а не серверу несущей связи: у события
	 *       сервера отклик этот не заводится вовсе - тот отвечает отказом «отклик чтения
	 *       данных этому виду события задать нельзя», - а посредник как раз и держит то,
	 *       что прочитано из туннеля
	 *
	 * @note Сличается именно ВИД прочитанного, а не его размер: пакет собирает система, и
	 *       утверждать его длину значило бы утверждать поведение системы
	 */
	this->_io->on(mid, static_cast <awh::engine::callback::read_t> (
		[&carried, &addressed]([[maybe_unused]] const awh::event::id_t mid, const uint8_t * data, const size_t size) noexcept -> void {
			// Считаем дошедший пакет
			carried++;
			// Если дошедшее оказалось пакетом IPv4
			if((data != nullptr) && (size > 0) && ((data[0] >> 4) == 4))
				// Запоминаем опознание пакета IPv4
				addressed = true;
		}
	));
	// Фиксируем события
	ASSERT_TRUE(this->_io->commit(tid));
	ASSERT_TRUE(this->_io->commit(sid));
	ASSERT_TRUE(this->_io->commit(cid));
	ASSERT_TRUE(this->_io->commit(mid));
	/**
	 * Подключаем несущую связь к своему же серверу
	 *
	 * @note Без этого дейтаграммный сокет несущей связи остаётся без встречной стороны:
	 *       очередь принимает данные и отчитывается об отправке, а наружу не уходит
	 *       ничего
	 */
	ASSERT_TRUE(this->_io->connect(cid));
	/**
	 * Запускаем события
	 *
	 * @note Посредник запуску вручную НЕ подлежит и отвечает на него отказом: ведёт его
	 *       та связь, с которой он соединён, и своей жизни у него нет
	 */
	ASSERT_TRUE(this->_io->launch(sid));
	ASSERT_TRUE(this->_io->launch(cid));
	ASSERT_TRUE(this->_io->launch(tid));
	/**
	 * Шлём дейтаграмму на адрес встречной стороны туннеля
	 *
	 * @details Обмен ведётся сокетом системы, а не движком: проверке нужен пакет,
	 *          пришедший В туннель извне движка, - иначе она проверяла бы движок
	 *          сам собою
	 */
	awh::net::socket_t fd = static_cast <awh::net::socket_t> (::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
	ASSERT_NE(fd, awh::net::invalid_socket_t);
	// Адрес встречной стороны туннеля
	struct sockaddr_in peer{};
	// Устанавливаем семейство адреса встречной стороны
	peer.sin_family = AF_INET;
	// Устанавливаем порт встречной стороны
	peer.sin_port = htons(9);
	// Устанавливаем адрес встречной стороны
	peer.sin_addr.s_addr = ::inet_addr(TUNNEL_PEER);
	// Полезная нагрузка отправляемой дейтаграммы
	const char payload[] = "AWH-TUNNEL-PROBE";
	// Отсчёт времени ожидания
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Крутим опрос, подсылая дейтаграммы
	 */
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 3000) && (carried == 0)){
		// Отправляем дейтаграмму на адрес встречной стороны туннеля
		::sendto(fd, payload, sizeof(payload) - 1, 0, reinterpret_cast <struct sockaddr *> (&peer), sizeof(peer));
		// Выполняем оборот опроса
		ASSERT_TRUE(this->_io->poll(100));
	}
	// Закрываем сокет отправки
	::closesocket(fd);
	// Роспись итога для сообщений об отказе
	const std::string sign = std::string("дошло=") + std::to_string(carried) + " опознано=" + (addressed ? "да" : "нет");
	// Через туннель обязан пройти хотя бы один пакет
	ASSERT_GE(carried, 1) << sign;
	// Дошедшее обязано оказаться пакетом IPv4
	ASSERT_TRUE(addressed) << sign;
	// Сворачиваем движок
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Снятие защиты объявлений от макросов операционной системы
 *
 */
#include <sys/macro_pop.hpp>
