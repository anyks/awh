/**
 * @file socket.hpp
 * @date 2026-01-28
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
 * \~russian
 * @brief Заголовочный файл модуля низкоуровневой работы с сокетами — класс eth::Socket для установки опций сокета:
 *        неблокирующего режима, таймаутов, размеров буферов, keep-alive, TCP_NODELAY, TOS/DSCP,
 *        multicast и параметров переиспользования адреса
 *
 * \~english
 * @brief Header file of the module of the low level work with the sockets — the eth::Socket class for setting the options of a socket:
 *        of the non-blocking mode, of the timeouts, of the sizes of the buffers, of the keep-alive, of the TCP_NODELAY, of the TOS/DSCP,
 *        of the multicast and of the parameters of the reuse of the address
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SOCKET__
#define __AWH_SOCKET__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён Ethernet протоколов
	 *
	 * \~english
	 * @brief Namespace of the Ethernet protocols
	 *
	 * \~
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс для работы с сокетами
		 *
		 * @details Собирает в одном месте настройки сокета - те самые свойства,
		 * что задаются вызовом с длинным списком доводов и разнятся от системы
		 * к системе. Здесь они прикрыты понятными именами, а различия систем
		 * спрятаны внутрь
		 *
		 * Настройки делятся на несколько groups: пределы ожидания и размеры
		 * накопителей, пометки качества обслуживания, пределы числа переходов,
		 * участие в рассылке на группу, обнаружение наибольшего размера пакета
		 *
		 * @note Поддержка настроек **зависит от системы**: часть из них есть не
		 * всюду, и отрицательный итог нередко означает не сбой, а отсутствие
		 * такой возможности. Итог проверять следует всегда
		 *
		 * \~english
		 * @brief Class for working with the sockets
		 * @details Collects in one place the settings of a socket — those very properties
		 * that are set by a call with a long list of arguments and differ from system
		 * to system. Here they are covered by understandable names, and the differences of the systems
		 * are hidden inside
		 * The settings are divided into several groups: the limits of the waiting and the sizes
		 * of the accumulators, the marks of the quality of the service, the limits of the number of the hops,
		 * the participation in the multicast to a group, the discovery of the largest size of a packet
		 * @note The support of the settings **depends on the system**: a part of them is absent
		 * in some places, and a negative result not rarely means not a failure, but the absence of
		 * such a possibility. The result should always be checked
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Socket {
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @warning Настройка эта **общая на весь процесс**, а не своя у каждого
				 * объекта. По умолчанию защита выключена - в расчёте на однопоточную
				 * работу, - и включать её следует до запуска второго потока
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 * \~english
				 * @brief Method of setting the thread safety of the work
				 * @warning This setting is a **common one for the whole process**, and not its own for every
				 * object. By default the protection is switched off — with the reckoning on a single-threaded
				 * work, — and it should be switched on before the start of the second thread
				 * @param mode flag of the thread safety mode
				 *
				 * \~
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения кода ошибки
				 *
				 * @details Забирает у сокета код последней ошибки, попутно его сбрасывая
				 *
				 * @note Нужно это при неблокирующем подключении: сам вызов
				 * подключения там возвращается сразу, а удалось оно или нет,
				 * выясняется потом - как раз этим способом
				 *
				 * @param sock сетевой сокет
				 * @return     код ошибки на сокете если присутствует
				 *
				 * \~english
				 * @brief Method of getting the code of an error
				 * @details Takes from a socket the code of the last error, resetting it along the way
				 * @note This is needed at a non-blocking connection: the very call of
				 * the connection returns there at once, and whether it succeeded or not,
				 * is found out afterwards — exactly by this way
				 * @param sock network socket
				 * @return     code of the error on the socket if present
				 *
				 * \~
				 */
				int32_t getError(const net::socket_t sock) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения таймаута сокета
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @return      время таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of a socket
				 * @param sock  network socket
				 * @param event event of the socket
				 * @return      time of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута сокета
				 *
				 * @details Задаёт, сколько ждать при чтении или отправке, прежде чем
				 * прервать вызов
				 *
				 * @warning Действует лишь на **блокирующие** сокеты. У неблокирующих
				 * вызовы и без того возвращаются сразу, а ожиданием ведает цикл
				 * событий - и там пределы задаются движком, а не здесь
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @param msec  время таймаута в миллисекундах
				 * @return      результат установки таймаута
				 *
				 * \~english
				 * @brief Method of setting the timeout of a socket
				 * @details Sets how long to wait at the reading or at the sending before
				 * interrupting the call
				 * @warning Is in force only for the **blocking** sockets. For the non-blocking ones
				 * the calls return at once anyway, and the waiting is in charge of the loop of
				 * the events — and there the limits are set by the engine, and not here
				 * @param sock  network socket
				 * @param event event of the socket
				 * @param msec  time of the timeout in milliseconds
				 * @return      result of the setting of the timeout
				 *
				 * \~
				 */
				bool setTimeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @return      размер буфера сокета
				 *
				 * \~english
				 * @brief Method of getting the size of a buffer
				 * @param sock  network socket
				 * @param event event of the socket
				 * @return      size of the buffer of the socket
				 *
				 * \~
				 */
				int32_t getBufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения свободного места в буфере сокета
				 *
				 * @details Возвращает СВОБОДНОЕ место, а не вместимость: `SO_SNDBUF` и `SO_RCVBUF`
				 *          говорят, сколько буфер вмещает всего, тогда как обмен упирается в то,
				 *          сколько в нём осталось незанятого. Забитый сокет не примет и байта
				 *          при какой угодно вместимости
				 *
				 * @note Для буфера отправки замер безопасно занижен: писатель у сокета один -
				 *       сам движок, а ядро буфер только выгребает, поэтому свободного места
				 *       между замером и отправкой может стать лишь больше. Для буфера приёма
				 *       наоборот: место занимает встречная сторона, и замер стареет мгновенно
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета (чтение либо запись)
				 * @return      свободное место в буфере либо -1, если система его не сообщает
				 *
				 * \~english
				 * @brief Method of getting the free room in a buffer of a socket
				 * @param sock  network socket
				 * @param event event of the socket (the reading or the writing)
				 * @return      free room in the buffer or -1, if the system does not report it
				 *
				 * \~
				 */
				int32_t getBufferAvailable(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размеров буфера
				 *
				 * @details Задаёт, сколько данных ядро придержит у себя - порознь на
				 * приём и на отправку
				 *
				 * @note Система вправе выдать не то, что просят: запрошенный размер
				 * обычно удваивается под служебные нужды, а сверх общесистемного
				 * предела и вовсе урезается. Итог стоит перечитать
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @param size  размер буфера сокета
				 * @return      установленный размер буфера сокета
				 *
				 * \~english
				 * @brief Method of setting the sizes of a buffer
				 * @details Sets how much data the kernel will hold at itself — separately for
				 * the receiving and for the sending
				 * @note The system is free to give not what is asked for: the requested size
				 * is usually doubled for the service needs, and beyond the system-wide
				 * limit is cut down altogether. The result is worth reading back
				 * @param sock  network socket
				 * @param event event of the socket
				 * @param size  size of the buffer of the socket
				 * @return      the set size of the buffer of the socket
				 *
				 * \~
				 */
				int32_t setBufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса для multicast пакетов
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ifname имя сетевого интерфейса
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the network interface for the multicast packets
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param ifname name of the network interface
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setMulticastIface(const net::socket_t sock, const event::family_t family, string_view ifname) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод устанавливает постоянное подключение на сокет
				 *
				 * @details Включает проверку живости: подключение, по которому долго
				 * нет обмена, будет опробовано служебными пакетами, и молчащий конец
				 * обнаружится сам
				 *
				 * @note Без этого оборванное подключение может не обнаруживаться
				 * сколь угодно долго - обрыв в сети ничем себя не выдаёт, пока по
				 * подключению не пойдут данные
				 *
				 * @param sock  сетевой сокет
				 * @param cnt   максимальное количество попыток
				 * @param idle  время через которое происходит проверка подключения
				 * @param intvl время между попытками
				 * @return      результат работы функции
				 *
				 * \~english
				 * @brief The method sets a permanent connection on a socket
				 * @details Switches on the check of the liveness: a connection over which there is long
				 * no exchange will be probed by the service packets, and a silent end
				 * will be discovered by itself
				 * @note Without this a broken connection may not be discovered
				 * for however long — a break in the network gives itself away by nothing, until
				 * the data goes over the connection
				 * @param sock  network socket
				 * @param cnt   maximum number of the attempts
				 * @param idle  time after which the check of the connection happens
				 * @param intvl time between the attempts
				 * @return      result of the work of the function
				 *
				 * \~
				 */
				bool setKeepalive(const net::socket_t sock, int32_t cnt, int32_t idle, int32_t intvl) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       value of the DSCP
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param dscp   значение DSCP
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param dscp   value of the DSCP
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Выдаёт значение, устанавливаемое на исходящих пакетах. Признак
				 *       перегрузки принятых пакетов приходит отдельно для каждой
				 *       датаграммы в метаданных дейтаграммного пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение ECN
				 *
				 * \~english
				 * @brief Method of getting the value of the Explicit Congestion Notification (ECN) field in the header of an IP packet
				 * @note Yields the value set on the outgoing packets. The sign of
				 *       the congestion of the received packets comes separately for every
				 *       datagram in the metadata of the datagram packet
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       value of the ECN
				 *
				 * \~
				 */
				event::ecn_t getExplicitCongestionNotification(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Класс обслуживания (DSCP) сохраняется: оба поля занимают один
				 *       октет заголовка, поэтому установка выполняется чтением текущего
				 *       значения с последующей заменой только младших двух бит
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ecn    значение ECN
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Explicit Congestion Notification (ECN) field in the header of an IP packet
				 * @note The class of the service (DSCP) is preserved: both fields occupy one
				 *       octet of the header, and therefore the setting is performed by the reading of the current
				 *       value with the subsequent replacement of the lower two bits only
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param ecn    value of the ECN
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setExplicitCongestionNotification(const net::socket_t sock, const event::family_t family, const event::ecn_t ecn) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации генерации информации о трафике
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим активации или деактивации
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of the activation/deactivation of the generation of the information about the traffic
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param mode   mode of the activation or of the deactivation
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool trafficInfoGeneration(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод переключения опции сокета
				 *
				 * @details Протокол принимается доводом, а не разыскивается у самого сокета.
				 *          Часть опций приложима лишь к одному протоколу - `TCP_NO_DELAY`
				 *          к TCP, - и прежде протокол читался настройкой `SO_PROTOCOL`,
				 *          то есть обращением к ядру ради того, что вызывающему и без
				 *          того известно: он этот сокет сам и заводил
				 *
				 * @note Довод необязателен, и значение `NONE` означает «протокол не
				 *       назван». Тогда он разыскивается у сокета по-прежнему: обращений
				 *       к методу много, и обязать все их назвать протокол значило бы
				 *       править места, которым он безразличен
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим активации или деактивации
				 * @param option опция сокета
				 * @param proto  протокол сокета, `NONE` - протокол не назван
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of switching an option of a socket
				 * @details The protocol is taken as an argument, and is not sought at the socket itself.
				 *          A part of the options is applicable only to one protocol — `TCP_NO_DELAY`
				 *          to TCP, — and formerly the protocol was read by the `SO_PROTOCOL` setting,
				 *          that is by an address to the kernel for the sake of what the caller knows
				 *          anyway: it started this socket itself
				 * @note The argument is optional, and the value `NONE` means «the protocol is not
				 *       named». Then it is sought at the socket as before: there are many addresses
				 *       to the method, and to oblige all of them to name the protocol would mean
				 *       correcting the places it is indifferent to
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param mode   mode of the activation or of the deactivation
				 * @param option option of the socket
				 * @param proto  protocol of the socket, `NONE` — the protocol is not named
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool switchOption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option, const event::protocol_t proto = event::protocol_t::NONE) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       режим обнаружения максимального размера пакета (MTU)
				 *
				 * \~english
				 * @brief Method of getting the discovery of the maximum size of a packet (MTU)
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       mode of the discovery of the maximum size of a packet (MTU)
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим обнаружения максимального размера пакета (MTU)
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery of the maximum size of a packet (MTU)
				 * @param sock   network socket
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param mode   mode of the discovery of the maximum size of a packet (MTU)
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param sock     сетевой сокет
				 * @param family   семейство протоколов (IPv4 или IPv6)
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops a packet may pass through
				 * @param sock     network socket
				 * @param family   family of the protocols (IPv4 or IPv6)
				 * @param delivery mode of the transmission of the packets (unicast, multicast, broadcast)
				 * @return         maximum number of the hops
				 *
				 * \~
				 */
				uint8_t getHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param sock     сетевой сокет
				 * @param family   семейство протоколов (IPv4 или IPv6)
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @param hops     максимальное количество хопов
				 * @return         результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops a packet may pass through
				 * @param sock     network socket
				 * @param family   family of the protocols (IPv4 or IPv6)
				 * @param delivery mode of the transmission of the packets (unicast, multicast, broadcast)
				 * @param hops     maximum number of the hops
				 * @return         result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const uint8_t hops) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @details Записывает сокет в группу рассылки или выписывает из неё.
				 * Пакеты, разосланные на группу, доходят лишь до вписавшихся
				 *
				 * @note Вписываться следует с указанием устройства: машина с
				 * несколькими устройствами иначе выберет его сама, и рассылка может
				 * прийти не с той стороны
				 *
				 * @param sock   сетевой сокет
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of the activation/deactivation of the multicast group of an event
				 * @details Enrolls a socket into a multicast group or strikes it out of one.
				 * The packets sent to the group reach only the enrolled ones
				 * @note One should enroll with the device specified: a machine with
				 * several devices will otherwise choose it by itself, and the multicast may
				 * come from the wrong side
				 * @param sock   network socket
				 * @param mode   mode of the activation/deactivation
				 * @param group  multicast group for the activation/deactivation
				 * @param source address of the network interface the subscription is performed from
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки готовности средств сокетов системы
				 *
				 * @details Отвечает, годна ли система заводить сокеты вообще. Средства
				 * эти у отдельных систем требуют подъёма на процесс, и подъём этот
				 * вправе не удаться: у MS Windows такое обращение (`WSAStartup`)
				 * отвечает отказом при недоступной версии либо нехватке средств, и
				 * всякое обращение к сокетам после этого отвечает отказом 10093
				 *
				 * @details Спрашивают об этом на заведении движка, а не при выдаче
				 * сокета. Довод в том, КОГДА вызывающая сторона просит сеть: заводя
				 * движок, она сеть просит явно - и отказ обязан прийти прямо там, а не
				 * всплыть позже отдельными отказами каждого сокета, оставив приложение
				 * заведённым, но неработоспособным
				 *
				 * @note У систем, подъёма не требующих, ответ утвердителен всегда:
				 * средства сокетов там принадлежат ядру и в подъёме не нуждаются
				 *
				 * @return результат проверки готовности средств сокетов системы
				 *
				 * \~english
				 * @brief Method of checking the readiness of the means of the sockets of the system
				 * @details Answers whether the system is fit to start sockets at all. These means
				 * at some systems require a bring-up per process, and that bring-up
				 * is free to fail: on MS Windows such an address (`WSAStartup`)
				 * answers with a refusal at an unavailable version or at a shortage of the means, and
				 * every address to the sockets after that answers with the refusal 10093
				 * @details This is asked at the starting of the engine, and not at the issuing of
				 * a socket. The argument is in WHEN the calling side asks for the network: starting
				 * the engine, it asks for the network explicitly — and the refusal is obliged to come right there, and not
				 * to surface later as separate refusals of every socket, leaving the application
				 * started, but inoperable
				 * @note At the systems not requiring a bring-up the answer is affirmative always:
				 * the means of the sockets there belong to the kernel and need no bring-up
				 * @return result of the check of the readiness of the means of the sockets of the system
				 *
				 * \~
				 */
				bool ready() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи нового сокета
				 *
				 * @details Часть опций события ядро принимает прямо при создании
				 * сокета, не требуя отдельных обращений: неблокирующий режим и
				 * закрытие при запуске стороннего образа. Набор опций передаётся
				 * сюда, чтобы этой возможностью воспользоваться
				 *
				 * @note Опции, которые ядро при создании не принимает, метод
				 * пропускает молча - их накладывает вызывающая сторона обычным
				 * путём. На системах без такой возможности пропускаются все
				 *
				 * @param family  семейство протоколов сокета
				 * @param type    тип сокета
				 * @param proto   протокол сокета
				 * @param options набор опций события
				 * @return        созданный сокет
				 *
				 * \~english
				 * @brief Method of issuing a new socket
				 * @details A part of the options of an event the kernel takes right at the creation of
				 * a socket, requiring no separate addresses: the non-blocking mode and
				 * the closing at the start of a foreign image. The set of the options is passed
				 * here so that this possibility would be used
				 * @note The options the kernel does not take at the creation the method
				 * skips silently — they are applied by the calling side by the ordinary
				 * path. On the systems without such a possibility all of them are skipped
				 * @param family  family of the protocols of the socket
				 * @param type    type of the socket
				 * @param proto   protocol of the socket
				 * @param options set of the options of the event
				 * @return        the created socket
				 *
				 * \~
				 */
				net::socket_t issue(const event::family_t family, const event::type_t type, const event::protocol_t proto, const uint16_t options = event::options::NONE) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций, принимаемых при создании сокета
				 *
				 * @details Отвечает, какие из переданных опций ядро этой системы
				 * принимает прямо при создании сокета - то есть какие из них метод
				 * выдачи сокета уже наложил и накладывать повторно не следует
				 *
				 * @note Знание это держится здесь намеренно: оно зависит от системы
				 * и от условной сборки, и повторять его на стороне движка означало бы
				 * завести второй источник правды, который разойдётся с первым
				 *
				 * @param options набор опций события
				 * @return        подмножество опций, наложенных при создании сокета
				 *
				 * \~english
				 * @brief Method of getting the options taken at the creation of a socket
				 * @details Answers which of the passed options the kernel of this system
				 * takes right at the creation of a socket — that is which of them the method
				 * of the issuing of a socket has already applied and should not apply once more
				 * @note This knowledge is held here deliberately: it depends on the system
				 * and on the conditional build, and to repeat it at the side of the engine would mean
				 * starting a second source of the truth, which will diverge from the first one
				 * @param options set of the options of the event
				 * @return        subset of the options applied at the creation of the socket
				 *
				 * \~
				 */
				uint16_t inborn(const uint16_t options) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания пары сокетов для межпроцессного взаимодействия (IPC)
				 *
				 * @details Заводит два связанных сокета: записанное в один
				 * вычитывается из другого. Обмен идёт внутри ядра, минуя сеть
				 *
				 * @note Обычное применение - разделение процесса: пара заводится до
				 * разделения, после чего каждая сторона закрывает свой конец и
				 * остаётся связь между родителем и потомком
				 *
				 * @param family семейство протоколов сокета
				 * @param type   тип сокета
				 * @param proto  протокол сокета
				 * @return       созданный сокет
				 *
				 * \~english
				 * @brief Method of creating a pair of sockets for the interprocess communication (IPC)
				 * @details Starts two connected sockets: what is written into one
				 * is read out of the other. The exchange goes inside the kernel, bypassing the network
				 * @note The usual application is the splitting of a process: the pair is started before
				 * the splitting, after which each side closes its own end and
				 * a connection between the parent and the child remains
				 * @param family family of the protocols of the socket
				 * @param type   type of the socket
				 * @param proto  protocol of the socket
				 * @return       the created socket
				 *
				 * \~
				 */
				array <net::socket_t, 2> ipc(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Socket(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Socket() noexcept;
		} socket_t;
	};
};

#endif // __AWH_SOCKET__
