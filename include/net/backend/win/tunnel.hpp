/**
 * @file tunnel.hpp
 * @date 2026-08-08
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
 * @brief Заголовочный файл модуля туннельных устройств MS Windows — заведение
 *        устройства, обмен пакетами через него и его устранение
 *
 * @details У систем POSIX туннель есть обыкновенный дескриптор: `/dev/net/tun` у Linux,
 *          `/dev/tunN` либо управляющий сокет `utun` у BSD. Открыл - и читай `read`,
 *          пиши `write`, наравне с сокетом. У MS Windows встроенного туннеля нет вовсе,
 *          и его приносит сторонний драйвер, а драйверов этих два, и устроены они
 *          по-разному
 *
 *          **Wintun** дескриптора не отдаёт совсем. Пакеты ходят через кольцо в общей
 *          с драйвером памяти: `WintunReceivePacket` выдаёт указатель внутрь кольца,
 *          `WintunReleaseReceivePacket` возвращает место обратно. Готовность к чтению
 *          сообщается событием, какое отдаёт `WintunGetReadWaitEvent`
 *
 *          **tap-windows6** отдаёт настоящий дескриптор файла, открываемый по имени
 *          вида `\\.\Global\{GUID}.tap`. Через него идут перекрытые `ReadFile` и
 *          `WriteFile` - ровно как у именованного канала, каким уже работает `SEQPACKET`
 *
 *          Модуль этот скрывает обе разницы. Наружу выдаётся одно: дескриптор, чтение,
 *          запись и устранение - вне зависимости от того, каким драйвером устройство
 *          заведено
 *
 * @par Намеренные решения
 *
 *      **Дескриптором Wintun служит его событие готовности.** `net::socket_t` у MS
 *      Windows есть `uintptr_t`, и описатель системы ложится в него без потерь. Событие
 *      же уникально на сеанс, живёт ровно столько, сколько сеанс, и годится ключом
 *      реестра. Тем подпись `Interface::create` остаётся общей на все системы, а не
 *      обзаводится особым видом ради одной
 *
 *      **Приём Wintun идёт копированием, а не выдачей указателя внутрь кольца.** Выдача
 *      без копирования была бы быстрее, но обязала бы вызывающего возвращать место в
 *      кольцо, и порядок этот пришлось бы соблюдать всему, что стоит выше. Проба на
 *      стенде показала, что кольцо сносит возврат вразнобой, - стало быть, отказ от
 *      копирования возможен и позже, отдельной надстройкой, не ломая нынешнего обмена
 *
 *      **Драйвер выбирает тот, кто заводит устройство.** Разница между ними не в одной
 *      скорости: Wintun переносит лишь пакеты сетевого уровня и мостом не служит,
 *      tap-windows6 переносит кадры вместе с аппаратными адресами. Выбирать за
 *      вызывающего наличием библиотеки значило бы менять поведение устройства от того,
 *      что случилось оказаться на машине
 *
 * @warning Заведение туннеля требует надзорных прав. Их отсутствие отвечает отказом
 *          ERROR_ACCESS_DENIED, и отказ этот заносится в журнал предупреждением
 *
 * @warning tap-windows6 новых устройств не заводит: их ставит установщик драйвера, а
 *          модуль лишь занимает свободное. Если свободных нет, заведение отвечает
 *          отказом с внятной о том записью
 *
 * \~english
 * @brief Header file of the module of the tunnel devices of MS Windows — the starting
 *        of a device, the exchange of the packets through it and its elimination
 * @details At the POSIX systems a tunnel is an ordinary descriptor: `/dev/net/tun` at Linux,
 *          `/dev/tunN` or the `utun` control socket at BSD. One opened it — and reads by `read`,
 *          writes by `write`, on a par with a socket. MS Windows has no built-in tunnel at all,
 *          and a third-party driver brings it, and there are two of these drivers, and they are arranged
 *          differently
 *          **Wintun** gives back no descriptor at all. The packets go through a ring in the memory shared
 *          with the driver: `WintunReceivePacket` gives out a pointer inside the ring,
 *          `WintunReleaseReceivePacket` returns the room back. The readiness for the reading
 *          is reported by an event, which `WintunGetReadWaitEvent` gives back
 *          **tap-windows6** gives back a real file descriptor, opened by a name
 *          of the kind `\\.\Global\{GUID}.tap`. Through it go the overlapped `ReadFile` and
 *          `WriteFile` — exactly as at a named pipe, by which `SEQPACKET` already works
 *          This module hides both differences. Outwards one thing is given: a descriptor, the reading,
 *          the writing and the elimination — regardless of by which driver the device
 *          is started
 * @par Deliberate decisions
 *      **The descriptor of Wintun is its event of the readiness.** `net::socket_t` at MS
 *      Windows is a `uintptr_t`, and a handle of the system fits into it without losses. The event,
 *      though, is unique per session, lives exactly as long as the session, and is fit as a key of
 *      a registry. Thereby the signature of `Interface::create` remains common for all the systems, and does not
 *      acquire a special kind for the sake of one
 *      **The reception of Wintun goes by a copying, and not by the issuing of a pointer inside the ring.** The issuing
 *      without a copying would be faster, but would oblige the caller to return the room into
 *      the ring, and that order would have to be observed by everything that stands above. A trial on
 *      the stand has shown that the ring tolerates the return out of order, — hence, the refusal from
 *      the copying is possible later as well, by a separate superstructure, without breaking the present exchange
 *      **The driver is chosen by the one who starts the device.** The difference between them is not in the speed
 *      alone: Wintun carries only the packets of the network level and does not serve as a bridge,
 *      tap-windows6 carries the frames together with the hardware addresses. To choose for
 *      the caller by the presence of a library would mean changing the behaviour of the device by that,
 *      what happened to turn out to be on the machine
 * @warning The starting of a tunnel requires supervisory rights. Their absence answers with the refusal
 *          ERROR_ACCESS_DENIED, and that refusal is entered into the log as a warning
 * @warning tap-windows6 starts no new devices: they are placed by the installer of the driver, and
 *          the module only occupies a free one. If there are no free ones, the starting answers with
 *          a refusal with an intelligible record about it
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_NET_BACKEND_WIN_TUNNEL__
#define __AWH_NET_BACKEND_WIN_TUNNEL__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/net.hpp>
#include <net/event.hpp>
#include <sys/log.hpp>

/**
 * \~russian
 * @brief пространство имён библиотеки
 *
 * \~english
 * @brief namespace of the library
 *
 * \~
 */
namespace awh {
	/**
	 * Активируем пространство имён следующего уровня
	 *
	 */
	using namespace std;
	/**
	 * \~russian
	 * @brief пространство имён средств MS Windows
	 *
	 * \~english
	 * @brief namespace of the means of MS Windows
	 *
	 * \~
	 */
	namespace win {
		/**
		 * \~russian
		 * @brief пространство имён туннельных устройств
		 *
		 * \~english
		 * @brief namespace of the tunnel devices
		 *
		 * \~
		 */
		namespace tunnel {
			/**
			 * \~russian
			 * @brief Драйвер, каким заведено туннельное устройство
			 *
			 * @details Драйверы эти устроены по-разному, и различать их приходится
			 *          всюду, где идёт обмен: у одного кольцо в общей памяти, у
			 *          другого дескриптор файла
			 *
			 * \~english
			 * @brief The driver a tunnel device is started by
			 * @details These drivers are arranged differently, and they have to be told apart
			 *          everywhere the exchange goes: one has a ring in the shared memory, the
			 *          other one a file descriptor
			 *
			 * \~
			 */
			enum class driver_t : uint8_t {
				NONE   = 0x00, // Драйвер не определён
				WINTUN = 0x01, // Кольцо в общей памяти, только сетевой уровень
				TAP    = 0x02  // Дескриптор файла, канальный уровень с адресами
			};
			/**
			 * \~russian
			 * @brief Функция заведения туннельного устройства
			 *
			 * @details Имя здесь и входное, и выходное: пустое система дополнит сама,
			 *          вписав выбранное обратно в виде «{GUID}»
			 *
			 * @param type   вид заводимого устройства
			 * @param driver драйвер, каким устройство заводится
			 * @param name   название заводимого устройства
			 * @param log    объект ведения журнала
			 * @return       дескриптор заведённого устройства
			 *
			 * \~english
			 * @brief Function of starting a tunnel device
			 * @details The name here is both an input and an output one: an empty one the system will fill up by itself,
			 *          writing the chosen one back in the form «{GUID}»
			 * @param type   kind of the started device
			 * @param driver driver the device is started by
			 * @param name   name of the started device
			 * @param log    object of the keeping of the log
			 * @return       descriptor of the started device
			 *
			 * \~
			 */
			net::socket_t create(const event::eth_t type, const driver_t driver, string & name, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки доступности драйвера туннельных устройств
			 *
			 * @details Wintun считается доступным, если его библиотека подключается и
			 *          состав её ожидаемый. tap-windows6 - если в системе есть хоть одно
			 *          свободное устройство: заводить новые приложению не дано
			 *
			 * @param driver проверяемый драйвер
			 * @return       признак доступности драйвера
			 *
			 * \~english
			 * @brief Function of checking the availability of a driver of the tunnel devices
			 * @details Wintun is considered available if its library is linked and
			 *          its composition is the expected one. tap-windows6 — if there is at least one
			 *          free device in the system: the application is not given to start new ones
			 * @param driver checked driver
			 * @return       sign of the availability of the driver
			 *
			 * \~
			 */
			bool available(const driver_t driver) noexcept;
			/**
			 * \~russian
			 * @brief Функция сообщения драйверу адресов туннеля
			 *
			 * @details Драйвер tap-windows6 в режиме переноса пакетов сетевого уровня
			 *          отбирает их по настроенной сети: пакет, посланный за её пределы,
			 *          до устройства не доходит вовсе. Оттого адреса ему сообщаются
			 *          настоящие - те, какими туннель заведён, - и сообщаются они тогда,
			 *          когда стали известны, а не в миг заведения устройства
			 *
			 * @note Устройству Wintun сообщать нечего: отбора по сети у него нет, и
			 *       обращение отвечает согласием, не делая ничего
			 *
			 * @warning Адреса берутся в порядке октетов сети - в том самом, в каком они
			 *          лежат в `sin_addr.s_addr`, - и перестановки не требуют
			 *
			 * @param sock   дескриптор туннельного устройства
			 * @param local  адрес своего конца туннеля
			 * @param remote адрес встречного конца туннеля
			 * @param log    объект ведения журнала
			 * @return       результат выполнения сообщения
			 *
			 * \~english
			 * @brief Function of telling a driver the addresses of a tunnel
			 * @details The tap-windows6 driver in the mode of the carrying of the network layer packets
			 *          selects them by the configured network: a packet sent beyond its bounds
			 *          does not reach the device at all. Therefore the addresses told to it are
			 *          the real ones — those the tunnel is started with — and they are told then,
			 *          when they have become known, and not at the moment of the starting of the device
			 * @note There is nothing to tell to a Wintun device: it has no selection by network, and
			 *       the call answers with a consent, doing nothing
			 * @warning The addresses are taken in the network octet order — in the very one they
			 *          lie in `sin_addr.s_addr` — and require no rearrangement
			 * @param sock   descriptor of the tunnel device
			 * @param local  address of the own end of the tunnel
			 * @param remote address of the opposite end of the tunnel
			 * @param log    object of the keeping of the log
			 * @return       result of the performance of the telling
			 *
			 * \~
			 */
			/**
			 * \~russian
			 * @brief Функция установки режима безопасной работы с потоками
			 *
			 * @details Замки модуля по умолчанию ПОГАШЕНЫ: работа в один поток -
			 *          обычный расклад, и платить за захват на каждом обращении
			 *          незачем. Включаются они этим обращением
			 *
			 * @param mode устанавливаемый режим безопасной работы с потоками
			 *
			 * \~english
			 * @brief Function of setting the mode of the thread-safe work
			 *
			 * @details The locks of the module are MUTED by default: the work in a
			 *          single thread is the usual case, and there is no reason to pay
			 *          for the capture at every call. They are enabled by this call
			 *
			 * @param mode the mode of the thread-safe work to set
			 *
			 * \~
			 */
			void threadSafety(const bool mode) noexcept;

			bool configure(const net::socket_t sock, const uint32_t local, const uint32_t remote, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Функция устранения туннельного устройства
			 *
			 * @param sock дескриптор устраняемого устройства
			 * @param log  объект ведения журнала
			 * @return     результат выполнения устранения
			 *
			 * \~english
			 * @brief Function of eliminating a tunnel device
			 * @param sock descriptor of the eliminated device
			 * @param log  object of the keeping of the log
			 * @return     result of the performance of the elimination
			 *
			 * \~
			 */
			bool destroy(const net::socket_t sock, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Функция поиска туннельного устройства по его названию
			 *
			 * @param name название искомого устройства
			 * @return     дескриптор найденного устройства
			 *
			 * \~english
			 * @brief Function of searching for a tunnel device by its name
			 * @param name name of the searched device
			 * @return     descriptor of the found device
			 *
			 * \~
			 */
			net::socket_t find(const string & name) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки принадлежности дескриптора туннельному устройству
			 *
			 * @param sock проверяемый дескриптор
			 * @return     признак принадлежности дескриптора туннелю
			 *
			 * \~english
			 * @brief Function of checking the belonging of a descriptor to a tunnel device
			 * @param sock checked descriptor
			 * @return     sign of the belonging of the descriptor to a tunnel
			 *
			 * \~
			 */
			bool exists(const net::socket_t sock) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения драйвера туннельного устройства
			 *
			 * @param sock дескриптор туннельного устройства
			 * @return     драйвер, каким устройство заведено
			 *
			 * \~english
			 * @brief Function of getting the driver of a tunnel device
			 * @param sock descriptor of the tunnel device
			 * @return     the driver the device is started by
			 *
			 * \~
			 */
			driver_t driver(const net::socket_t sock) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия туннельного устройства
			 *
			 * @param sock дескриптор туннельного устройства
			 * @return     название туннельного устройства
			 *
			 * \~english
			 * @brief Function of getting the name of a tunnel device
			 * @param sock descriptor of the tunnel device
			 * @return     name of the tunnel device
			 *
			 * \~
			 */
			string name(const net::socket_t sock) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения события готовности к чтению
			 *
			 * @details Событие это взводится, едва в устройстве появился пакет. Ждать
			 *          его следует средствами ожидания событий, а не портом завершения:
			 *          порт ловит завершение обмена, но не взведение события
			 *
			 * @param sock дескриптор туннельного устройства
			 * @return     событие готовности устройства к чтению
			 *
			 * \~english
			 * @brief Function of getting the event of the readiness for the reading
			 * @details This event is raised as soon as a packet has appeared in the device. It should be waited for
			 *          by the means of the waiting for the events, and not by a completion port:
			 *          the port catches the completion of an exchange, but not the raising of an event
			 * @param sock descriptor of the tunnel device
			 * @return     event of the readiness of the device for the reading
			 *
			 * \~
			 */
			HANDLE event(const net::socket_t sock) noexcept;
			/**
			 * \~russian
			 * @brief Функция приёма пакета из туннельного устройства
			 *
			 * @param sock   дескриптор туннельного устройства
			 * @param buffer буфер, в который принимается пакет
			 * @param size   размер буфера приёма
			 * @return       размер принятого пакета, ноль если пакетов нет, -1 при отказе
			 *
			 * \~english
			 * @brief Function of receiving a packet from a tunnel device
			 * @param sock   descriptor of the tunnel device
			 * @param buffer buffer the packet is received into
			 * @param size   size of the buffer of the reception
			 * @return       size of the received packet, zero if there are no packets, -1 at a refusal
			 *
			 * \~
			 */
			int64_t read(const net::socket_t sock, void * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Функция отправки пакета в туннельное устройство
			 *
			 * @param sock   дескриптор туннельного устройства
			 * @param buffer буфер отправляемого пакета
			 * @param size   размер отправляемого пакета
			 * @return       размер отправленного пакета, -1 при отказе
			 *
			 * \~english
			 * @brief Function of sending a packet into a tunnel device
			 * @param sock   descriptor of the tunnel device
			 * @param buffer buffer of the sent packet
			 * @param size   size of the sent packet
			 * @return       size of the sent packet, -1 at a refusal
			 *
			 * \~
			 */
			int64_t write(const net::socket_t sock, const void * buffer, const size_t size) noexcept;
		}
	}
}

#endif // __AWH_NET_BACKEND_WIN_TUNNEL__
