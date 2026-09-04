/**
 * @file socket.cpp
 * @date 2026-02-06
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
 * @brief Тесты низкоуровневой работы с сокетами — проверка установки неблокирующего режима, таймаутов,
 *        размеров буферов, keep-alive и остальных опций сокета
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 */
#include <cstring>
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
	 * Подключаем посредник разбора служебных сообщений MS Windows
	 *
	 * @details Заголовок этот отдаёт проверкам `struct msghdr`, `struct iovec`,
	 *          `struct cmsghdr` и семейство `CMSG_*` - объявления, каких у MS Windows
	 *          нет вовсе и каким там отвечает расширенный вызов `WSARecvMsg`,
	 *          добываемый у гнезда отдельным запросом расширения
	 *
	 */
	#include <net/backend/win/message.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	/**
	 * @brief Закрепление снятия макросов CS5, CS6, CS7 и CS8
	 *
	 * @details Заголовок этот заводит перечисленные имена макросами размера знака у
	 *          последовательного порта, а проверка ниже пишет `dscp_t::CS5`, где CS
	 *          означает класс обслуживания пакета по RFC 2474. Пока <termios.h> сюда
	 *          не приходил, столкновение молчало, и вскрылось бы оно уже у потребителя
	 *          библиотеки. Подключение это намеренное: им проверка отвечает за то, что
	 *          пара macro/suppress.hpp и macro/restore.hpp имена эти снимает
	 *
	 * @note Обращения к членам защищены той же парой ниже - по правилу, изложенному у
	 *       macro/suppress.hpp: заголовки AWH защищают свои объявления, а называющий такие
	 *       члены у себя защищает свой файл сам
	 */
	#include <termios.h>
#endif
#include <sys/types.h>

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include <sys/macro/suppress.hpp>

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
	::closesocket(udp4);

	// Создаём TCP сокет IPv4
	auto tcp4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что TCP сокет IPv4 создан успешно
	ASSERT_NE(tcp4, awh::net::invalid_socket_t);
	// Закрываем сокет
	::closesocket(tcp4);

	// Создаём UDP сокет IPv6
	auto udp6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что UDP сокет IPv6 создан успешно
	ASSERT_NE(udp6, awh::net::invalid_socket_t);
	// Закрываем сокет
	::closesocket(udp6);

	// Создаём TCP сокет IPv6
	auto tcp6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что TCP сокет IPv6 создан успешно
	ASSERT_NE(tcp6, awh::net::invalid_socket_t);
	// Закрываем сокет
	::closesocket(tcp6);

	// Создаём STREAM сокет Unix Domain
	auto uds = this->_eth->socket.issue(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	// Проверяем что Unix Domain сокет создан успешно
	ASSERT_NE(uds, awh::net::invalid_socket_t);
	// Закрываем сокет
	::closesocket(uds);

	// Создаём DATAGRAM сокет Unix Domain
	auto udsg = this->_eth->socket.issue(awh::event::family_t::UDS, awh::event::type_t::DATAGRAM, awh::event::protocol_t::NONE);
	/**
	 * Для операционной системы MS Windows
	 *
	 * @details Дейтаграммных сокетов домена UNIX у MS Windows нет ВОВСЕ: подсистема
	 *          несёт только потоковые, и отказ здесь - местное поведение, а не дефект
	 *
	 * @warning Утверждается именно ОТКАЗ, а не пропуск: пропуск перестал бы стеречь
	 *          и тот случай, когда сокет вдруг заведётся вопреки устройству системы.
	 *          Прежде проверка ждала успеха и падала на всякой машине Windows
	 */
	#if _WIN32 || _WIN64
		// Дейтаграммный сокет домена UNIX завестись не может
		ASSERT_EQ(udsg, awh::net::invalid_socket_t) << "у MS Windows дейтаграммных сокетов домена UNIX нет, а сокет заведён";
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Проверяем что Unix Domain дейтаграммный сокет создан успешно
		ASSERT_NE(udsg, awh::net::invalid_socket_t);
		// Закрываем сокет
		::closesocket(udsg);
	#endif
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
	::closesocket(uds[0]);
	::closesocket(uds[1]);

	// Создаём пару сокетов Unix Domain Datagram
	auto udsg = this->_eth->socket.ipc(awh::event::family_t::UDS, awh::event::type_t::DATAGRAM, awh::event::protocol_t::NONE);
	// Проверяем что первый сокет пары создан успешно
	ASSERT_NE(udsg[0], awh::net::invalid_socket_t);
	// Проверяем что второй сокет пары создан успешно
	ASSERT_NE(udsg[1], awh::net::invalid_socket_t);
	// Закрываем сокеты пары
	::closesocket(udsg[0]);
	::closesocket(udsg[1]);

	// Создаём пару файловых дескрипторов канала PIPE
	auto pipe = this->_eth->socket.ipc(awh::event::family_t::PIPE, awh::event::type_t::NONE, awh::event::protocol_t::NONE);
	// Проверяем что дескриптор чтения канала создан успешно
	ASSERT_NE(pipe[0], awh::net::invalid_socket_t);
	// Проверяем что дескриптор записи канала создан успешно
	ASSERT_NE(pipe[1], awh::net::invalid_socket_t);
	// Закрываем дескрипторы канала
	::closesocket(pipe[0]);
	::closesocket(pipe[1]);
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
	::closesocket(sock);
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
	::closesocket(sock);
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
	::closesocket(sock);
}

/**
 * @brief Тест занятости приёмного буфера сокета
 *
 * @par Намеренные решения
 *
 * Договор у обращения разнится по системам, и проверка закрепляет ИМЕННО ЭТО, а не одну
 * лишь величину:
 *
 * | наречие | чтение | запись |
 * |---|---|---|
 * | `gnu` | свободное место | свободное место |
 * | `win` | свободное место | -1, средства нет |
 * | `bsd` | свободное место | зависит от системы |
 * | `sun` | -1, средства нет | -1, средства нет |
 *
 * Оттого утверждается полоса, а не число: ответ либо -1 - «средства у системы нет», -
 * либо величина от нуля до размера буфера. Ответ вне полосы означает дефект наречия.
 *
 * @warning Проверки у обращения не было ВОВСЕ ни на одной системе, при том что оно
 *          выведено наружу заголовком. Под MS Windows оно отвечало -1 на оба направления
 *          и было исправлено 04.09.2026 - откат этой правки прошёл бы молча
 *
 */
TEST_F(EthFixture, SocketBufferAvailableTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);
	// Получаем размер приёмного буфера сокета
	const int32_t size = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::READ);
	// Проверяем что размер приёмного буфера положительный
	ASSERT_GT(size, 0);
	// Получаем занятость приёмного буфера сокета
	const int32_t reading = this->_eth->socket.getBufferAvailable(sock, awh::net::socket_event_t::READ);
	/**
	 * Ответ обязан лежать в полосе договора
	 */
	if(reading != -1){
		// Свободное место на свежем сокете обязано быть положительным
		ASSERT_GT(reading, 0) << "свежий сокет объявлен без свободного места в приёмном буфере";
		// Свободное место не вправе превышать сам буфер
		ASSERT_LE(reading, size) << "свободного места объявлено больше самого буфера";
	}
	// Получаем занятость буфера отправки сокета
	const int32_t writing = this->_eth->socket.getBufferAvailable(sock, awh::net::socket_event_t::WRITE);
	// Ответ обязан лежать в той же полосе договора
	if(writing != -1)
		// Свободное место не вправе превышать буфер отправки
		ASSERT_LE(writing, this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::WRITE))
		 << "свободного места объявлено больше буфера отправки";
	/**
	 * У систем, где средство приёма есть, оно обязано БЫТЬ
	 *
	 * @note Отступление здесь неуместно: обращение опирается на FIONREAD либо SIOCINQ,
	 *       какие у обеих названных систем есть всегда, и -1 означал бы не свойство
	 *       машины, а несделанную работу наречия
	 */
	#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
		// Занятость приёмного буфера обязана быть названа
		ASSERT_NE(reading, -1) << "занятость приёмного буфера не отдана, хотя средство у системы есть";
	#endif
	// Закрываем сокет
	::closesocket(sock);
}

/**
 * @brief Тест настроек, накладываемых при самом заведении сокета
 *
 * @par Намеренные решения
 *
 * Утверждается договор, а не платформенная частность: ответ обязан быть ПОДМНОЖЕСТВОМ
 * спрошенного, а закрытие при запуске стороннего образа накладывается при заведении
 * у всех четырёх наречий - у POSIX признаком `SOCK_CLOEXEC`, у MS Windows признаком
 * `WSA_FLAG_NO_HANDLE_INHERIT`. Неблокирующий режим наложенным при заведении числят
 * лишь наречия POSIX, и здесь он не утверждается вовсе
 *
 * @warning Проверки у обращения не было ВОВСЕ. Под MS Windows оно отвечало пустотой -
 *          признаки при заведении не накладывались, - и было исправлено 04.09.2026
 *
 * @note Наличие признака утверждается лишь там, где средство у системы есть: у macOS
 *       ни `SOCK_CLOEXEC`, ни `SOCK_NONBLOCK` не объявлены, и пустой ответ там верен
 *
 */
TEST_F(EthFixture, SocketInbornOptionsTest){
	// Пустой набор настроек не даёт наложенных при заведении
	ASSERT_EQ(this->_eth->socket.inborn(0), 0);
	/**
	 * Закрытие при запуске стороннего образа утверждается лишь там, где средство ЕСТЬ
	 *
	 * @warning Первая редакция этой проверки утверждала признак у всех систем разом и
	 *          отказала на macOS - и отказала ВЕРНО: `SOCK_CLOEXEC` там не объявлен
	 *          вовсе, накладывать признак при заведении нечем, и пустой ответ наречия
	 *          правилен. Договор был выведен беглым просмотром тел, а не чтением их
	 *          условий сборки
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// У MS Windows признак кладётся при заведении всегда - WSA_FLAG_NO_HANDLE_INHERIT
		ASSERT_TRUE(this->_eth->socket.inborn(static_cast <uint16_t> (awh::event::options::CLOSE_ON_EXEC)) & static_cast <uint16_t> (awh::event::options::CLOSE_ON_EXEC))
		 << "закрытие при запуске стороннего образа не числится накладываемым при заведении";
	#elif defined(SOCK_CLOEXEC)
		// У систем POSIX - лишь при объявленном SOCK_CLOEXEC
		ASSERT_TRUE(this->_eth->socket.inborn(static_cast <uint16_t> (awh::event::options::CLOSE_ON_EXEC)) & static_cast <uint16_t> (awh::event::options::CLOSE_ON_EXEC))
		 << "закрытие при запуске стороннего образа не числится накладываемым при заведении";
	#endif
	// Набор наложенных обязан быть подмножеством спрошенного
	const uint16_t asked = static_cast <uint16_t> (awh::event::options::CLOSE_ON_EXEC) | static_cast <uint16_t> (awh::event::options::NO_IO_BLOCK) | static_cast <uint16_t> (awh::event::options::REUSE_ADDR);
	// Проверяем, что лишнего наречие не назвало
	ASSERT_EQ(static_cast <uint16_t> (this->_eth->socket.inborn(asked) & ~asked), 0)
	 << "наложенным при заведении названо то, о чём не просили";
	// Не спрошенное закрытие при запуске не вправе оказаться наложенным
	ASSERT_EQ(static_cast <uint16_t> (this->_eth->socket.inborn(static_cast <uint16_t> (awh::event::options::REUSE_ADDR)) & static_cast <uint16_t> (awh::event::options::CLOSE_ON_EXEC)), 0)
	 << "закрытие при запуске названо наложенным, хотя о нём не просили";
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
	 * @brief Сличение заданных сроков с тем, что приняло ядро
	 *
	 * @details Одного лишь положительного ответа наречия мало: у MS Windows наложение
	 *          идёт двумя путями - поимённым (TCP_KEEPIDLE, TCP_KEEPINTVL,
	 *          TCP_KEEPCNT) и запасным управляющим обращением SIO_KEEPALIVE_VALS, и
	 *          ВТОРОЙ числа попыток не принимает вовсе, отвечая при этом успехом.
	 *          Проверка, глядящая лишь на ответ, откат с первого пути на второй не
	 *          заметила бы. Оттого сроки читаются обратно и сличаются с заданными
	 *
	 * @note Имена настроек у систем расходятся: время простоя у macOS зовётся
	 *       TCP_KEEPALIVE, у прочих - TCP_KEEPIDLE. У OpenBSD посокетных сроков нет
	 *       вовсе, у Sun Solaris и illumos наречие их не накладывает - там сличать
	 *       нечего, и раздел этот опущен намеренно
	 *
	 * @note У MS Windows имена эти объявлены не во всяком издании заголовков, оттого
	 *       восполняются здесь теми же числами, что и у самого наречия
	 *
	 */
	#if defined(_WIN32) || defined(_WIN64)
		#ifndef TCP_KEEPIDLE
			#define TCP_KEEPIDLE 3
		#endif
		#ifndef TCP_KEEPCNT
			#define TCP_KEEPCNT 16
		#endif
		#ifndef TCP_KEEPINTVL
			#define TCP_KEEPINTVL 17
		#endif
	#endif
	#if !defined(__OpenBSD__) && !defined(__sun__) && !defined(__sun)
	{
		// Прочитанное у ядра значение настройки
		int32_t value = 0;
		// Размер прочитанного значения настройки
		socklen_t length = static_cast <socklen_t> (sizeof(value));
		/**
		 * Время простоя подключения
		 */
		#if defined(__APPLE__)
			const int32_t idle = TCP_KEEPALIVE;
		#else
			const int32_t idle = TCP_KEEPIDLE;
		#endif
		// Считываем время простоя подключения
		ASSERT_EQ(::getsockopt(static_cast <awh::net::socket_t> (sock), IPPROTO_TCP, idle, reinterpret_cast <char *> (&value), &length), 0)
		 << "время простоя подключения не читается обратно";
		// Сличаем время простоя подключения с заданным
		ASSERT_EQ(value, 5) << "ядру досталось время простоя подключения, отличное от заданного";
		// Обнуляем прочитанное значение настройки
		value = 0;
		// Восстанавливаем размер прочитанного значения настройки
		length = static_cast <socklen_t> (sizeof(value));
		// Считываем промежуток между попытками
		ASSERT_EQ(::getsockopt(static_cast <awh::net::socket_t> (sock), IPPROTO_TCP, TCP_KEEPINTVL, reinterpret_cast <char *> (&value), &length), 0)
		 << "промежуток между попытками не читается обратно";
		// Сличаем промежуток между попытками с заданным
		ASSERT_EQ(value, 5) << "ядру достался промежуток между попытками, отличный от заданного";
		// Обнуляем прочитанное значение настройки
		value = 0;
		// Восстанавливаем размер прочитанного значения настройки
		length = static_cast <socklen_t> (sizeof(value));
		// Считываем число попыток
		ASSERT_EQ(::getsockopt(static_cast <awh::net::socket_t> (sock), IPPROTO_TCP, TCP_KEEPCNT, reinterpret_cast <char *> (&value), &length), 0)
		 << "число попыток не читается обратно";
		// Сличаем число попыток с заданным
		ASSERT_EQ(value, 5) << "ядру досталось число попыток, отличное от заданного";
	}
	#endif

	/**
	 * Передача отрицательных параметров не должна приводить к аварийному завершению
	 * (ранее значения корректировались через const_cast, что являлось неопределённым поведением)
	 */
	ASSERT_NO_FATAL_FAILURE(this->_eth->socket.setKeepalive(sock, -1, -1, -1));

	// Закрываем сокет
	::closesocket(sock);
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
	::closesocket(sock);
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
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::TCP_CORKING));
	// Отключаем режим отложенной отправки TCP пакетов (TCP CORK)
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::DISABLED, awh::event::options::TCP_CORKING));

	// Закрываем сокет
	::closesocket(sock);
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
	/**
	 * @par Намеренные решения
	 *
	 * Отключение режима проверяется лишь там, где система его вообще допускает.
	 * OpenBSD отображённых адресов IPv4 в IPv6 не поддерживает вовсе - это её
	 * осознанное решение, а не пробел выпуска, - и режим у неё выключить нельзя:
	 * гнездо IPv6 там всегда только IPv6. Подменить это нечем, и ожидать успеха
	 * значило бы требовать от системы того, чего она не делает
	 *
	 */
	#if !__OpenBSD__
		// Отключаем режим только IPv6 на IPv6 сокете
		ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::DISABLED, awh::event::options::IPV6_ONLY));
	#endif

	/**
	 * Своя голова пакета на сокете TCP отсюда УБРАНА
	 *
	 * @details Прежде здесь стояло `ASSERT_TRUE` с доводом «для IPv6 настройка не
	 *          поддерживается и всегда возвращает успех». Довод неверен: настройку
	 *          имеют Linux (IPV6_HDRINCL = 36) и MS Windows (значение 2) - замерено
	 *          03.09.2026 на семи системах. Обе дают её ЛИШЬ сокету
	 *          НЕСТРУКТУРИРОВАННОМУ, а на TCP отвечают отказом (ENOPROTOOPT у Linux,
	 *          10022 у MS Windows)
	 *
	 * @warning Проверка на сокете TCP закрепляла молчаливый успех на невыполненное
	 *          дело - то есть закрепляла изъян. Своя голова пакета вынесена в
	 *          `SocketHeaderInclusionIPv6Test`, где сокет заводится тем видом, какому
	 *          настройка и предназначена
	 */
	// Закрываем сокет
	::closesocket(sock);
}

/**
 * @brief Тест своей головы пакета на сокете IPv6
 *
 * @details Настройка применима ЛИШЬ к сокету неструктурированному - так у обеих
 *          систем, её имеющих, и так же устроена IP_HDRINCL у IPv4. Проверяется здесь
 *          не число ответа, а ДОГОВОР: движок обязан отвечать успехом, а не молча
 *          бездействовать
 *
 * @note Пропуск ставится по ОТКАЗУ ЗАВЕДЕНИЯ сокета, а не по опросу прав: сырой сокет
 *       обычному приложению не даёт ни одна из систем, а `geteuid` у MS Windows нет
 *       вовсе, и опрос прав там не годится (замечание владельца наречия MS Windows)
 *
 */
TEST_F(EthFixture, SocketHeaderInclusionIPv6Test){
	/**
	 * Создаём неструктурированный сокет IPv6 поверх ICMPv6
	 *
	 * @note Взят ICMPv6, а не RAW, намеренно: проверке нужен ВИД сокета, а не именно
	 *       этот протокол, и ICMPv6 доступен у всех систем. Ветвь `IPPROTO_RAW` у IPv6
	 *       заведена отдельной правкой того же дня и закрепляется своей проверкой
	 */
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::RAW, awh::event::protocol_t::ICMP);
	// Если сокет завести не удалось, полномочий у нас нет
	if(sock == awh::net::invalid_socket_t)
		// Проверять нечего: сырой сокет обычному приложению не даётся
		GTEST_SKIP() << "Неструктурированный сокет требует надзорных полномочий";
	// Включаем свою голову пакета
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED, awh::event::options::HDRINCL))
	 << "Движок отказал во включении своей головы пакета на сокете, какому она предназначена";
	/**
	 * Опрашиваем сокет напрямую там, где система настройку имеет
	 *
	 * @warning Без этого опроса проверка СЛЕПА: договор отвечает успехом и тогда,
	 *          когда слой не делает ничего, - именно так он и вёл себя до 03.09.2026,
	 *          и одно лишь `ASSERT_TRUE` выше молчаливое бездействие не отличит от
	 *          исполненного дела. Чтения настроек в договоре нет, потому опрос идёт
	 *          средствами системы
	 *
	 * @note Там, где настройки нет вовсе (FreeBSD, NetBSD, OpenBSD, Sun Solaris,
	 *       OpenIndiana), опрашивать нечего, и договор там иной - успех на пустое
	 *       действие
	 */
	#ifdef IPV6_HDRINCL
		// Значение настройки, прочитанное у системы
		int32_t value = 0;
		// Длина значения настройки
		socklen_t length = sizeof(value);
		// Настройка обязана быть ВКЛЮЧЕНА у самого сокета, а не только по ответу движка
		ASSERT_EQ(0, ::getsockopt(sock, IPPROTO_IPV6, IPV6_HDRINCL, reinterpret_cast <char *> (&value), &length))
		 << "Настройку своей головы пакета не прочитать у сокета";
		// Прочитанное значение обязано быть истинным
		ASSERT_NE(0, value) << "Движок ответил успехом, но своей головы пакета у сокета НЕ включил";
	#endif
	// Выключаем свою голову пакета
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV6, awh::net::socket_mode_t::DISABLED, awh::event::options::HDRINCL))
	 << "Движок отказал в выключении своей головы пакета";
	/**
	 * Опрашиваем сокет заново: выключение обязано дойти до системы так же
	 */
	#ifdef IPV6_HDRINCL
		// Восстанавливаем длину значения настройки
		length = sizeof(value);
		// Настройка обязана быть ВЫКЛЮЧЕНА у самого сокета
		ASSERT_EQ(0, ::getsockopt(sock, IPPROTO_IPV6, IPV6_HDRINCL, reinterpret_cast <char *> (&value), &length))
		 << "Настройку своей головы пакета не прочитать у сокета";
		// Прочитанное значение обязано быть ложным
		ASSERT_EQ(0, value) << "Движок ответил успехом, но своей головы пакета у сокета НЕ выключил";
	#endif
	// Закрываем сокет
	::closesocket(sock);
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
	::closesocket(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);
	// Включаем генерацию информации о трафике для IPv6
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(sock6, awh::event::family_t::IPV6, awh::net::socket_mode_t::ENABLED));
	// Отключаем генерацию информации о трафике для IPv6
	ASSERT_TRUE(this->_eth->socket.trafficInfoGeneration(sock6, awh::event::family_t::IPV6, awh::net::socket_mode_t::DISABLED));
	// Закрываем сокет
	::closesocket(sock6);
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
	::closesocket(sock4);

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
	::closesocket(sock6);
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
	::closesocket(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);
	// Устанавливаем и проверяем значение DSCP критического класса для IPv6
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6, awh::event::dscp_t::CS5));
	ASSERT_EQ(awh::event::dscp_t::CS5, this->_eth->socket.getDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6));
	// Закрываем сокет
	::closesocket(sock6);
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
/**
 * @brief Функция ожидаемого показания метки перегрузки на сокете
 *
 * @details У систем POSIX метка живёт в октете заголовка IP и читается тем же
 *          значением, каким поставлена. У MS Windows настройка IP_ECN на СОКЕТЕ -
 *          признак способности, а не двухразрядный код: щуп на стенде Windows ARM64
 *          показал, что значения 1, 2 и 3 читаются оттуда одинаково единицей, а сам
 *          код метки задаётся ОТДЕЛЬНО, на каждую датаграмму управляющим сообщением
 *
 * @warning Проверка оттого утверждает МЕСТНОЕ показание каждой системы, а не
 *          показание одной из них. Прежде она ждала всюду точного кода и падала на
 *          всякой машине Windows на первом же круговом обходе
 *
 * @param ecn запрошенная метка перегрузки
 * @return    показание, какого следует ждать от системы
 *
 */
static awh::event::ecn_t ecnExpected(const awh::event::ecn_t ecn) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Всякая поставленная метка читается признаком способности
		return ((ecn == awh::event::ecn_t::NOT_ECT) ? awh::event::ecn_t::NOT_ECT : awh::event::ecn_t::ECT1);
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Метка читается тем же значением, каким поставлена
		return ecn;
	#endif
}

TEST_F(EthFixture, SocketEcnTest){
	// Создаём UDP сокет IPv4
	auto sock4 = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock4, awh::net::invalid_socket_t);
	// Проверяем что по умолчанию признак перегрузки не установлен
	ASSERT_EQ(awh::event::ecn_t::NOT_ECT, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Устанавливаем и проверяем признак поддержки ECN для IPv4
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock4, awh::event::family_t::IPV4, awh::event::ecn_t::ECT0));
	ASSERT_EQ(ecnExpected(awh::event::ecn_t::ECT0), this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Устанавливаем класс обслуживания поверх установленного признака перегрузки
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4, awh::event::dscp_t::CS3));
	// Проверяем что признак перегрузки установкой класса обслуживания не сброшен
	ASSERT_EQ(ecnExpected(awh::event::ecn_t::ECT0), this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Проверяем что класс обслуживания установлен
	ASSERT_EQ(awh::event::dscp_t::CS3, this->_eth->socket.getDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4));
	// Меняем признак перегрузки поверх установленного класса обслуживания
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock4, awh::event::family_t::IPV4, awh::event::ecn_t::ECT1));
	// Проверяем что класс обслуживания сменой признака перегрузки не сброшен
	ASSERT_EQ(awh::event::dscp_t::CS3, this->_eth->socket.getDifferentiatedServicesCodePoint(sock4, awh::event::family_t::IPV4));
	// Проверяем что признак перегрузки сменён
	ASSERT_EQ(ecnExpected(awh::event::ecn_t::ECT1), this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Снимаем признак поддержки ECN
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock4, awh::event::family_t::IPV4, awh::event::ecn_t::NOT_ECT));
	ASSERT_EQ(awh::event::ecn_t::NOT_ECT, this->_eth->socket.getExplicitCongestionNotification(sock4, awh::event::family_t::IPV4));
	// Закрываем сокет
	::closesocket(sock4);

	// Создаём UDP сокет IPv6
	auto sock6 = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock6, awh::net::invalid_socket_t);
	// Устанавливаем и проверяем признак поддержки ECN для IPv6
	ASSERT_TRUE(this->_eth->socket.setExplicitCongestionNotification(sock6, awh::event::family_t::IPV6, awh::event::ecn_t::ECT0));
	ASSERT_EQ(ecnExpected(awh::event::ecn_t::ECT0), this->_eth->socket.getExplicitCongestionNotification(sock6, awh::event::family_t::IPV6));
	// Устанавливаем класс обслуживания поверх установленного признака перегрузки
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6, awh::event::dscp_t::CS5));
	// Проверяем что оба поля октета сохранены независимо
	ASSERT_EQ(ecnExpected(awh::event::ecn_t::ECT0), this->_eth->socket.getExplicitCongestionNotification(sock6, awh::event::family_t::IPV6));
	ASSERT_EQ(awh::event::dscp_t::CS5, this->_eth->socket.getDifferentiatedServicesCodePoint(sock6, awh::event::family_t::IPV6));
	// Закрываем сокет
	::closesocket(sock6);
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
	/**
	 * Если проверка идёт под MS Windows
	 */
	#elif defined(_WIN32) || defined(_WIN64)
		/**
		 * @par Намеренные решения
		 *
		 * Отступление это НЕ от нехватки средств у проверки: посредник
		 * `net/backend/win/message.hpp` заведён 03.09.2026 именно ради неё, приём
		 * служебных сообщений под MS Windows проверке доступен, и проверка была
		 * написана целиком. Отступление - от свойства САМОЙ СИСТЕМЫ, замеренного
		 * владельцем наречия MS Windows 03.09.2026:
		 *
		 *     способ пометки                          принятый октет TOS
		 *     настройка сокета IP_ECN                        0
		 *     метка на датаграмму через WSASendMsg           2
		 *
		 * Настройка `IP_ECN` на сокете системой ПРИНИМАЕТСЯ - `setsockopt` отвечает
		 * нулём, отказа нет, - и на исходящие датаграммы НЕ ВЛИЯЕТ. Замер повторён
		 * дважды: по петле и по настоящему устройству машины, итог тот же
		 *
		 * @note Случай этот того же рода, что `IPV6_HDRINCL` и `protocol_t::RAW`:
		 *       согласие на невыполненное дело. Разница в том, что там соглашался
		 *       наш слой, а здесь соглашается система, и поделать движку нечего:
		 *       отличить принятую настройку от применённой ему нечем
		 *
		 * @warning Проверка эта закрепляет СОКЕТНОЕ обращение
		 *          `eth::Socket::setExplicitCongestionNotification`. Метку у обоих
		 *          движков на деле несёт УЗЛОВОЕ - `io_t::setExplicitCongestionNotification`,
		 *          и оно кладёт её каждой датаграмме управляющими данными; путь этот
		 *          под MS Windows работает и закрепляется отдельно, в наборе обмена.
		 *          Переводить эту проверку на узловое обращение ради её прохождения
		 *          нельзя: предмет у неё именно слой сокетов
		 *
		 */
		GTEST_SKIP() << "MS Windows accepts IP_ECN on the socket but does not apply it to the outgoing datagrams";
	#else
	/**
	 * @par Намеренные решения
	 *
	 * Под MS Windows приём ведётся посредником `win::message::receive`, а не глобальным
	 * `recvmsg`, какого там нет вовсе. Прежде проверка на этой системе ОТСТУПАЛА, и
	 * доставка метки не проверялась ничем - хотя движок её выполнял, что было доказано
	 * отдельным щупом. Посредник заведён 03.09.2026 (`net/backend/win/message.hpp`)
	 * владельцем наречия MS Windows именно ради снятия этого отступления
	 *
	 * @warning Посредник этот ведёт приём ПРОСТОЙ - прямо у системы расширенным вызовом
	 *          `WSARecvMsg`. Приём движка устроен иначе: он сперва спрашивает пул
	 *          родного приёма, куда порт завершений мог сложить датаграмму ДО прихода
	 *          потребителя, и лишь затем идёт к системе. Пул этот внутренность движка,
	 *          наружу ему нельзя
	 *
	 * @warning Оттого проверка эта утверждает «СИСТЕМА донесла метку на датаграмме», а
	 *          НЕ «движок донёс её через свой пул». Это разные утверждения, и путать их
	 *          нельзя: путь движка через пул проверяется наборами обмена, а не здесь
	 *
	 */
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
		::closesocket(rx);
		::closesocket(tx);
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
	/**
	 * Если проверка идёт под MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Код отказа приёма, отдаваемый посредником доводом
		int32_t error = 0;
		// Выполняем приём датаграммы вместе со служебными сообщениями
		const int64_t received = awh::win::message::receive(rx, &message, 0, &error);
		// Проверяем что датаграмма принята
		ASSERT_EQ(received, 1) << "Приём датаграммы отказал с кодом " << error;
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Проверяем что датаграмма принята
		ASSERT_EQ(::recvmsg(rx, &message, 0), 1);
	#endif
	// Маркировка ECN принятой датаграммы
	uint8_t congestion = 0xFF;
	/**
	 * Перебираем служебные сообщения принятой датаграммы
	 */
	for(struct cmsghdr * cmsg = CMSG_FIRSTHDR(&message); cmsg != nullptr; cmsg = CMSG_NXTHDR(&message, cmsg)){
		// Если служебное сообщение несёт класс обслуживания заголовка IPv4-пакета
		if((cmsg->cmsg_level == IPPROTO_IP) && ((cmsg->cmsg_type == IP_TOS) || (cmsg->cmsg_type == IP_RECVTOS)))
			// Извлекаем признак перегрузки пути из младших двух бит октета
			/**
			 * Начало данных метаданного берётся по-разному
			 *
			 * @warning Макрос `CMSG_DATA` под MS Windows употреблять НЕЛЬЗЯ: имя это
			 *          занято там дважды, и второй владелец - `wincrypt.h:2910`, где
			 *          `CMSG_DATA` есть число 1, вид криптографического сообщения.
			 *          Приходит он через `windows.h` во всякую единицу, где есть хоть
			 *          что-то от криптографии, и переопределяет имя ПОЗЖЕ подмены. Разбор
			 *          отвечает на это «called object type 'int' is not a function», и
			 *          понять причину по отказу нельзя никак
			 *
			 * @note Оттого здесь зовётся функция `win::message::data`: имя её занять
			 *       нечем. У систем POSIX столкновения этого нет, и там остаётся макрос
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Извлекаем признак перегрузки пути из младших двух бит октета
				congestion = static_cast <uint8_t> ((* reinterpret_cast <const uint8_t *> (awh::win::message::data(cmsg))) & 0x03);
			#else
				// Извлекаем признак перегрузки пути из младших двух бит октета
				congestion = static_cast <uint8_t> ((* reinterpret_cast <const uint8_t *> (CMSG_DATA(cmsg))) & 0x03);
			#endif
	}
	// Закрываем сокеты
	::closesocket(rx);
	::closesocket(tx);
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
		::closesocket(sock4);
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
		::closesocket(sock6);
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
	::closesocket(sock);
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
	::closesocket(sock);
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
	::closesocket(sock);
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
	::closesocket(sock);
}

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include <sys/macro/restore.hpp>

/**
 * @brief Тест заведения неструктурированного сокета IPv6 с протоколом RAW
 *
 * @details Ветви этой у IPv6 не было ни в одном наречии POSIX, тогда как у IPv4 она
 *          есть: довод `protocol_t::RAW` падал в умолчание, и сокет не заводился вовсе
 *
 * @warning `IPPROTO_RAW` и «протокол не задан» - НЕ одно и то же: с первым пакет
 *          уходит вместе с СОБСТВЕННОЙ головой, со вторым голову собирает система.
 *          Потребитель, просивший собрать голову своими руками, получал отказ, а у
 *          наречия MS Windows - молча сокет с протоколом нуль, что хуже отказа
 *
 * @note Замерено на пяти системах: сырой сокет IPv6 с этим протоколом принимают все -
 *       Astra Linux, FreeBSD 14.1, NetBSD 10.1, OpenBSD 7.9 и Sun Solaris 11.4
 *
 */
TEST_F(EthFixture, SocketRawProtocolIPv6Test){
	// Создаём неструктурированный сокет IPv6 с протоколом RAW
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV6, awh::event::type_t::RAW, awh::event::protocol_t::RAW);
	// Если сокет завести не удалось, полномочий у нас нет
	if(sock == awh::net::invalid_socket_t)
		// Проверять нечего: сырой сокет обычному приложению не даётся
		GTEST_SKIP() << "Неструктурированный сокет требует надзорных полномочий";
	// Сокет обязан быть годным
	ASSERT_NE(sock, awh::net::invalid_socket_t);
	/**
	 * Сличаем протокол сокета с запрошенным
	 *
	 * @note Утверждается не заведение само по себе, а ТОТ ЛИ протокол заведён: сокет с
	 *       протоколом нуль тоже завёлся бы, и без этого сличения подмена одного другим
	 *       осталась бы незамеченной
	 *
	 * @warning Под MS Windows утверждение это НЕ СТАВИТСЯ, и проверка там сводится к
	 *          «сокет завёлся». Сказано это прямо, а не спрятано в условие сборки:
	 *          молчаливый `#ifdef` выглядит как «проверено везде», тогда как покрытие
	 *          там неполно
	 *
	 * @warning Причина не в недосмотре, а в самой системе - протокол сокета там
	 *          средствами опроса НЕ НАБЛЮДАЕМ вовсе. Замерено владельцем наречия MS
	 *          Windows 03.09.2026, оба пути отрицательны: настройки `SO_PROTOCOL` там
	 *          нет, а `SO_PROTOCOL_INFOW` отдаёт `iProtocol` РАВНЫМ НУЛЮ при любом
	 *          запрошенном протоколе - поле описывает запись поставщика, а не сокет.
	 *          Косвенный путь через свою голову пакета тоже негоден: у Linux сокет с
	 *          `IPPROTO_RAW` имеет её включённой по умолчанию, а у MS Windows нет
	 */
	#ifdef SO_PROTOCOL
		/**
		 * Заводим ЭТАЛОННЫЙ сокет средствами системы, минуя движок
		 *
		 * @warning Сличать прочитанное с числом `IPPROTO_RAW` НЕЛЬЗЯ: системы отвечают
		 *          по-разному, и утверждение развалилось бы не по вине движка. Замер
		 *          03.09.2026: Astra Linux и OpenBSD 7.9 отдают запрошенный протокол,
		 *          а FreeBSD 14.1 для СЫРЫХ сокетов отдаёт НОЛЬ, хотя для TCP и UDP
		 *          отвечает верно. Оттого сличается не число, а ответ системы на
		 *          сокет, заведённый ею же напрямую
		 */
		const awh::net::socket_t reference = ::socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
		// Если эталонный сокет заведён
		if(reference != awh::net::invalid_socket_t){
			// Протокол сокета, заведённого движком
			int32_t value = 0;
			// Протокол сокета, заведённого системой напрямую
			int32_t expected = 0;
			// Длина значения протокола
			socklen_t length = sizeof(value);
			// Читаем протокол у сокета, заведённого движком
			const bool ok = (::getsockopt(sock, SOL_SOCKET, SO_PROTOCOL, reinterpret_cast <char *> (&value), &length) == 0);
			// Восстанавливаем длину значения протокола
			length = sizeof(expected);
			// Читаем протокол у эталонного сокета
			const bool okReference = (::getsockopt(reference, SOL_SOCKET, SO_PROTOCOL, reinterpret_cast <char *> (&expected), &length) == 0);
			// Закрываем эталонный сокет
			::closesocket(reference);
			// Оба чтения обязаны удаться
			ASSERT_TRUE(ok && okReference) << "Протокол сокета не прочитать";
			/**
			 * Сокет движка обязан отвечать тем же, чем эталонный
			 *
			 * @note Там, где система поле для сырых сокетов не заполняет (FreeBSD),
			 *       утверждение вырождается в «ноль равен нулю» и подмену протокола не
			 *       поймает. Это ПРЕДЕЛ НАБЛЮДАЕМОСТИ, а не изъян проверки: отличить
			 *       «ноль оттого, что не заполняют» от «ноль оттого, что подменили»
			 *       средствами опроса нечем
			 */
			ASSERT_EQ(expected, value) << "Заведён сокет с протоколом " << value << " вместо " << expected << ", какой система даёт сокету, заведённому ею напрямую";
		}
	#endif
	// Закрываем сокет
	::closesocket(sock);
}

