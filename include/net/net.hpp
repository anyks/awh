/**
 * @file net.hpp
 * @date 2025-11-06
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
 * @brief Заголовочный файл базовых сетевых структур — представление адресов подключения (IPv4, IPv6, MAC, UDS),
 *        атрибутов и источников соединения, информации о датаграммах, туннелях и сетевых интерфейсах,
 *        используемых всеми транспортными модулями
 *
 * \~english
 * @brief Header file of the base network structures — the representation of the addresses of a connection (IPv4, IPv6, MAC, UDS),
 *        of the attributes and of the sources of a connection, of the information about the datagrams, the tunnels and the network interfaces,
 *        used by all the transport modules
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NETWORK__
#define __AWH_NETWORK__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>

/**
 * \~russian
 * Для операционной системы не являющейся MS Windows
 *
 * @note Заголовков MS Windows здесь нет намеренно. Единая точка их подключения
 *       (sys/win32.hpp) снимает макросы, чьи имена заняты членами перечислений AWH,
 *       и подключайся она из открытого заголовка — снятия эти протекали бы в единицу
 *       трансляции потребителя библиотеки, отнимая у него DELETE, NO_ERROR и прочие.
 *       Поэтому заголовки MS Windows подключаются только в файлах реализации
 *
 * \~english
 * For an operating system that is not MS Windows
 * @note There are no headers of MS Windows here deliberately. The single point of their inclusion
 *       (sys/win32.hpp) removes the macros whose names are taken by the members of the enumerations of AWH,
 *       and were it included from a public header — these removals would leak into the translation
 *       unit of the consumer of the library, taking away from it DELETE, NO_ERROR and the others.
 *       Therefore the headers of MS Windows are included only in the files of the implementation
 *
 * \~
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системный заголовочный файл
	 */
	#include <sys/socket.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include "event.hpp"
#include "../sys/global.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../sys/macro_push.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён для работы с сетью
	 *
	 * \~english
	 * @brief Namespace for working with the network
	 *
	 * \~
	 */
	namespace net {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			/**
			 * \~russian
			 * @brief Тип сокета
			 *
			 * @details Написание повторяет тип SOCKET из winsock2.h дословно: там он
			 *          заведён как UINT_PTR, то есть беззнаковое целое разрядности
			 *          указателя. Выписан он здесь своими словами затем, чтобы
			 *          открытый заголовок не тянул за собой заголовки MS Windows
			 *
			 * @note Совпадение закреплено проверкой sizeof в файле реализации: разойдись
			 *       написание с системным, вызовы сокетного API стали бы усекать значение
			 *       на разрядных системах, где дескриптор сокета выходит за пределы int
			 *
			 * \~english
			 * @brief Type of a socket
			 * @details The spelling repeats the SOCKET type from winsock2.h word for word: there it
			 *          is started as a UINT_PTR, that is an unsigned integer of the width of
			 *          a pointer. It is written out here in one's own words so that
			 *          a public header would not drag the headers of MS Windows along with it
			 * @note The coincidence is fixed by a sizeof check in the file of the implementation: were
			 *       the spelling to diverge with the system one, the calls of the socket API would begin to truncate the value
			 *       on the wide systems, where the descriptor of a socket goes beyond the limits of an int
			 *
			 * \~
			 */
			using socket_t = uintptr_t;

			/**
			 * \~russian
			 * @brief Некорректный сокет
			 *
			 * @details Значение повторяет макрос INVALID_SOCKET из winsock2.h,
			 *          заведённый там как (SOCKET)(~0)
			 *
			 * \~english
			 * @brief Invalid socket
			 * @details The value repeats the INVALID_SOCKET macro from winsock2.h,
			 *          started there as (SOCKET)(~0)
			 *
			 * \~
			 */
			static constexpr socket_t invalid_socket_t = static_cast <socket_t> (~0);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			/**
			 * \~russian
			 * @brief Тип сокета
			 *
			 * \~english
			 * @brief Type of a socket
			 *
			 * \~
			 */
			using socket_t = int32_t;

			/**
			 * \~russian
			 * @brief Некорректный сокет
			 *
			 * \~english
			 * @brief Invalid socket
			 *
			 * \~
			 */
			static constexpr socket_t invalid_socket_t = -1;
		#endif

		/**
		 * \~russian
		 * @brief Режимы установки типа сокета
		 *
		 * @details Переключатель настройки сокета на два положения. Пользуются им
		 *          там, где настройка сводится к «включить или отключить»: сбор
		 *          сведений о проходящем трафике, отдельные свойства сокета
		 *
		 * @note Набор этот повторяет `event::mode_t`, но относится к самому сокету, а
		 *       не к узлу движка, и значения их **не совпадают числами**. Подменять
		 *       один другим через приведение нельзя
		 *
		 * \~english
		 * @brief Modes of the setting of the type of a socket
		 * @details A switch of a setting of a socket with two positions. It is used
		 *          where a setting comes down to «to switch on or to switch off»: the gathering
		 *          of the information about the passing traffic, the separate properties of a socket
		 * @note This set repeats `event::mode_t`, but relates to the socket itself, and
		 *       not to a node of the engine, and their values **do not coincide by the numbers**. Substituting
		 *       one for the other through a cast is not allowed
		 *
		 * \~
		 */
		enum class socket_mode_t : uint8_t {
			ENABLED  = 0x01, // Включено
			DISABLED = 0x02  // Выключено
		};

		/**
		 * \~russian
		 * @brief События сокета
		 *
		 * @details Указывает, к какому из двух направлений относится настройка -
		 *          к приёму или к отправке. Пределы ожидания и размеры накопителей
		 *          задаются направлениям порознь, и этим значением выбирается, о
		 *          котором из них речь
		 *
		 * \~english
		 * @brief Events of a socket
		 * @details Specifies which of the two directions a setting relates to —
		 *          to the reception or to the sending. The limits of the waiting and the sizes of the accumulators
		 *          are set to the directions separately, and by this value it is chosen which
		 *          of them is meant
		 *
		 * \~
		 */
		enum class socket_event_t : uint8_t {
			READ  = 0x01, // Чтение
			WRITE = 0x02  // Запись
		};

		/**
		 * \~russian
		 * @brief Идентификаторы разновидностей адресов
		 *
		 * @details Признак вида у атрибутов подключения - того, чем задана точка:
		 *          адресом IPv4 или IPv6, доменным именем, путём в файловой системе
		 *          либо аппаратным адресом
		 *
		 * @note Набор этот **не сплошной**: значение `0x03` пропущено намеренно.
		 *       Числа здесь согласованы с одноимённым набором класса `net_addr_t`,
		 *       который шире, и пропуск приходится на его значение `URL` -
		 *       разновидность, к точке подключения неприложимую. Согласование это
		 *       позволяет переносить значения между наборами, но опираться на
		 *       непрерывность номеров - перебирать их счётчиком или заводить по ним
		 *       таблицу - всё равно нельзя
		 *
		 * \~english
		 * @brief Identifiers of the varieties of the addresses
		 * @details The sign of the kind at the attributes of a connection — of that by which the point is set:
		 *          by an IPv4 or an IPv6 address, by a domain name, by a path in the file system
		 *          or by a hardware address
		 * @note This set is **not a continuous one**: the value `0x03` is skipped deliberately.
		 *       The numbers here are agreed with the set of the same name of the `net_addr_t` class,
		 *       which is wider, and the skip falls on its value `URL` —
		 *       a variety not applicable to a point of a connection. This agreement
		 *       allows the values to be carried between the sets, but relying on
		 *       the continuity of the numbers — traversing them by a counter or starting a table
		 *       by them — is still not allowed
		 *
		 * \~
		 */
		enum class type_t : uint8_t {
			NONE  = 0x00, // Не определено
			FS    = 0x01, // Адрес в файловой системе
			MAC   = 0x02, // Аппаратный адрес сетевого интерфейса
			IPV4  = 0x04, // Адрес подключения IPv4
			IPV6  = 0x05, // Адрес подключения IPv6
			FQDN  = 0x06, // Доменная зона
		};

		/**
		 * \~russian
		 * @brief Структура адреса
		 *
		 * @details Общее основание для всех видов адреса. Само по себе оно несёт
		 *          лишь длину, а сами байты добавляет наследник - свой для каждого
		 *          вида. Заведено это ради единого хранения: адрес передаётся и
		 *          хранится указателем на основание, а к нужному виду приводится по
		 *          месту через `awh_cast`
		 *
		 *          Наследников пять. Аппаратный адрес, адреса сети IPv4 и IPv6 - у
		 *          последних двух есть ещё и длина префикса, - а также адрес
		 *          файловой системы, за которым стоит не число, а путь
		 *
		 * @note Длина и служит **признаком вида**: шесть у аппаратного адреса,
		 *       четыре у IPv4, шестнадцать у IPv6 и ноль у пути файловой системы,
		 *       чья длина заранее неизвестна. Отдельного поля с видом здесь нет, и
		 *       разбирать приходящий адрес следует по длине
		 *
		 * @warning Приводить основание к виду, которому оно не отвечает, нельзя.
		 *          Приведение через `awh_cast` проверяется **только в отладочной
		 *          сборке**, где неверный вид даёт пустой указатель; в рабочей
		 *          сборке проверка снимается ради скорости. Сделано это намеренно:
		 *          в рабочую сборку попадает уже отлаженное, и неверное приведение
		 *          там - недосмотр разработчика, а не то, от чего библиотека
		 *          обязана стеречь ценой издержек на каждом обращении
		 *
		 * @note Отсюда правило: длину следует сверять до приведения и отлаживаться
		 *       в отладочной сборке, где чужой вид будет пойман сразу. Полагаться
		 *       же на итог самого приведения нельзя - в рабочей сборке он всегда
		 *       ненулевой
		 *
		 * @par Пример: разбор адреса по виду
		 *
		 * @code{.cpp}
		 * // Определяем вид пришедшего адреса по его длине
		 * switch(addr->size){
		 *     // Шесть байт - аппаратный адрес
		 *     case 6: use(awh_cast <const net::addr_mac_t *> (addr)->address); break;
		 *     // Четыре байта - адрес IPv4
		 *     case 4: use(awh_cast <const net::addr_net_ipv4_t *> (addr)->address); break;
		 *     // Шестнадцать байт - адрес IPv6
		 *     case 16: use(awh_cast <const net::addr_net_ipv6_t *> (addr)->address); break;
		 *     // Нулевая длина - путь файловой системы
		 *     case 0: use(awh_cast <const net::addr_fs_t *> (addr)->address); break;
		 * }
		 * @endcode
		 *
		 * \~english
		 * @brief Structure of an address
		 * @details The common base for all the kinds of an address. By itself it carries
		 *          only the length, and the bytes themselves are added by a descendant — its own for every
		 *          kind. This is started for the sake of a single storage: an address is passed and
		 *          held as a pointer to the base, and is cast to the needed kind by
		 *          the place through `awh_cast`
		 *          There are five descendants. A hardware address, the addresses of an IPv4 and of an IPv6 network — at
		 *          the last two there is also the length of a prefix, — as well as an address
		 *          of the file system, behind which there stands not a number, but a path
		 * @note The length serves as the **sign of the kind** as well: six at a hardware address,
		 *       four at IPv4, sixteen at IPv6 and zero at a path of the file system,
		 *       whose length is unknown in advance. There is no separate field with the kind here, and
		 *       an arriving address has to be resolved by the length
		 * @warning It is not allowed to cast the base to a kind it does not answer to.
		 *          A cast through `awh_cast` is checked **only in the debug
		 *          build**, where a wrong kind gives a null pointer; in the release
		 *          build the check is removed for the sake of the speed. This is done deliberately:
		 *          into the release build there gets what is already debugged, and a wrong cast
		 *          there is an oversight of the developer, and not that from which the library
		 *          is obliged to guard at the price of the costs at every address
		 * @note Hence the rule: the length should be checked before the cast and one should debug
		 *       in the debug build, where a foreign kind will be caught at once. Relying
		 *       on the result of the cast itself is not allowed — in the release build it is always
		 *       a non-null one
		 * @par Example: the resolution of an address by the kind
		 *
		 * @code{.cpp}
		 * // Determining the kind of the address that came by its length
		 * switch(addr->size){
		 *     // Six bytes — a hardware address
		 *     case 6: use(awh_cast <const net::addr_mac_t *> (addr)->address); break;
		 *     // Four bytes — an IPv4 address
		 *     case 4: use(awh_cast <const net::addr_net_ipv4_t *> (addr)->address); break;
		 *     // Sixteen bytes — an IPv6 address
		 *     case 16: use(awh_cast <const net::addr_net_ipv6_t *> (addr)->address); break;
		 *     // A zero length — a path of the file system
		 *     case 0: use(awh_cast <const net::addr_fs_t *> (addr)->address); break;
		 * }
		 * @endcode
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Address {
			// Размер адреса
			uint16_t size;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param size размер адреса
			 *
			 * \~english
			 * @brief Constructor
			 * @param size size of the address
			 *
			 * \~
			 */
			explicit Address(const uint16_t size = 0) noexcept;
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
			virtual ~Address() noexcept = default;
		} addr_t;

		/**
		 * \~russian
		 * @brief Структура MAC-адреса
		 *
		 * @details Аппаратный адрес сетевого устройства - шесть байт, закреплённых
		 *          за устройством изготовителем. Служит адресацией на канальном
		 *          уровне, в пределах одного сегмента сети: дальше первого
		 *          маршрутизатора он не уходит и подменяется на каждом переходе
		 *
		 * @note Адрес этот часто оказывается не тем, что закреплён изготовителем:
		 *       подмена его настройкой - обычное дело, а мобильные устройства
		 *       подставляют случайный при поиске беспроводных сетей. Опираться на
		 *       него как на неизменную примету устройства не следует
		 *
		 * \~english
		 * @brief Structure of a MAC address
		 * @details The hardware address of a network device — six bytes fastened
		 *          to the device by the manufacturer. Serves as the addressing at the link
		 *          level, within one segment of a network: further than the first
		 *          router it does not go and is substituted at every hop
		 * @note This address often turns out to be not the one fastened by the manufacturer:
		 *       its substitution by a setting is an ordinary thing, and the mobile devices
		 *       substitute a random one at the search for the wireless networks. Relying on
		 *       it as on an unchanging marker of a device is not advisable
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Address_MAC : public addr_t {
			// Буфер MAC-адреса
			array <uint8_t, 6> address;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Address_MAC() noexcept;
		} addr_mac_t;

		/**
		 * \~russian
		 * @brief Структура сетевого адреса
		 *
		 * @details Промежуточное основание для адресов, у которых помимо самого
		 *          адреса есть ещё и длина префикса - число старших разрядов,
		 *          отведённых под сеть. Собственных байт адреса не несёт, их
		 *          добавляют наследники, разные у IPv4 и IPv6
		 *
		 * @note Длина префикса заводится наибольшей - тридцать два у IPv4 и сто
		 *       двадцать восемь у IPv6, - то есть описывает поначалу один-
		 *       единственный узел, а не сеть. Задавать её следует тому, кто
		 *       выставляет адрес
		 *
		 * \~english
		 * @brief Structure of a network address
		 * @details An intermediate base for the addresses at which besides the address itself
		 *          there is also the length of a prefix — the number of the higher digits
		 *          given over to the network. It carries no bytes of the address of its own, they
		 *          are added by the descendants, different at IPv4 and IPv6
		 * @note The length of the prefix is started as the largest one — thirty two at IPv4 and one hundred
		 *       twenty eight at IPv6, — that is at first it describes one
		 *       single node, and not a network. It should be set by the one who
		 *       sets out the address
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Address_Network : public addr_t {
			// Префикс сети
			uint8_t prefix;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param prefix префикс сети
			 * @param size   размер адреса
			 *
			 * \~english
			 * @brief Constructor
			 * @param prefix prefix of the network
			 * @param size   size of the address
			 *
			 * \~
			 */
			explicit Address_Network(const uint8_t prefix, const uint16_t size) noexcept;
		} addr_net_t;

		/**
		 * \~russian
		 * @brief Структура IPv4 сетевого адреса
		 *
		 * @details Адрес IPv4 числом вместе с длиной префикса сети
		 *
		 * @warning Порядок байт в поле адреса структура **не оговаривает** и сама
		 *          его не переставляет: он таков, каким его записал заполнявший.
		 *          Передавая адрес системным вызовам, порядок следует привести к
		 *          сетевому явно
		 *
		 * \~english
		 * @brief Structure of an IPv4 network address
		 * @details An IPv4 address as a number together with the length of the prefix of the network
		 * @warning The structure **does not stipulate** the order of the bytes in the field of the address and does not
		 *          swap it itself: it is such as the one who filled it has written it. Passing
		 *          the address to the system calls, the order should be brought to the network one explicitly
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Address_Network_IPv4 : public addr_net_t {
			// IP-адрес сети
			uint32_t address;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Address_Network_IPv4() noexcept;
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
			virtual ~Address_Network_IPv4() noexcept = default;
		} addr_net_ipv4_t;

		/**
		 * \~russian
		 * @brief Структура IPv6 сетевого адреса
		 *
		 * @details Адрес IPv6 шестнадцатью байтами вместе с длиной префикса сети
		 *
		 * @note Зона адреса хранится **номером** устройства, а не его названием:
		 *       именно номер требуется системе в поле `sin6_scope_id`, и перевод
		 *       названия в номер выполняется однажды при разборе записи, а не при
		 *       каждой отправке. Ноль означает отсутствие зоны
		 *
		 *       Зона нужна адресам канальной связи (`FE80::/10`) и групповым
		 *       адресам связи (`FF02::/16`): без неё такой адрес неоднозначен -
		 *       машина не знает, которым устройством его достигать. Прочим адресам
		 *       зона не нужна, и у них поле остаётся нулевым
		 *
		 * \~english
		 * @brief Structure of an IPv6 network address
		 * @details An IPv6 address as sixteen bytes together with the length of the prefix of the network
		 * @note The zone of the address is held as the **number** of a device, and not as its name:
		 *       it is exactly the number that the system requires in the `sin6_scope_id` field, and the conversion of
		 *       a name into a number is performed once at the parsing of a record, and not at
		 *       every sending. Zero means the absence of a zone
		 *       The zone is needed by the link-local addresses (`FE80::/10`) and by the link-local multicast
		 *       addresses (`FF02::/16`): without it such an address is ambiguous —
		 *       the machine does not know by which device to reach it. The other addresses do not need
		 *       a zone, and at them the field remains a zero one
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Address_Network_IPv6 : public addr_net_t {
			// Номер устройства зоны адреса
			uint32_t zone;
			// Буфер IP-адрес сети
			array <uint8_t, 16> address;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Address_Network_IPv6() noexcept;
		} addr_net_ipv6_t;

		/**
		 * \~russian
		 * @brief Структура адреса файловой системы
		 *
		 * @details Путь к записи в файловой системе - к файлу, каталогу или местному
		 *          сокету. Единственный вид адреса, за которым стоит не число, а
		 *          строка переменной длины
		 *
		 * @note Длина у этого вида остаётся нулевой, и она же служит его приметой
		 *       при разборе: собственной длины у пути нет, она хранится в самой
		 *       строке
		 *
		 * @warning Путь к местному сокету ограничен системой примерно сотней
		 *          символов - предел этот куда короче обычного предела на путь, и
		 *          длинный путь будет молча обрезан при подключении
		 *
		 * \~english
		 * @brief Structure of an address of the file system
		 * @details A path to a record in the file system — to a file, to a directory or to a local
		 *          socket. The only kind of an address behind which there stands not a number, but
		 *          a string of a variable length
		 * @note The length at this kind remains a zero one, and it serves as its marker as well
		 *       at the resolution: the path has no length of its own, it is held in the string itself
		 * @warning The path to a local socket is limited by the system to about a hundred
		 *          characters — that limit is far shorter than the ordinary limit on a path, and
		 *          a long path will be silently cut off at the connection
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Address_Filesystem : public addr_t {
			// Путь к файлу, каталогу или сокету
			string address;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Address_Filesystem() noexcept;
		} addr_fs_t;

		/**
		 * \~russian
		 * @brief Структура сетевых адресов текущей машины
		 *
		 * @details Связка «устройство - его адреса»: название сетевого устройства,
		 *          закреплённый за ним адрес сети и его аппаратный адрес. Нужна там,
		 *          где машина имеет несколько устройств и важно, через какое именно
		 *          вести обмен, - при выборе исходного адреса, при рассылке на
		 *          группу, при опросе доступных устройств
		 *
		 * @note Аппаратный адрес заводится сразу и всегда, а вот адрес сети
		 *       передаётся снаружи и может отсутствовать: устройство, поднятое без
		 *       адреса, - обычное дело
		 *
		 * \~english
		 * @brief Structure of the network addresses of the current machine
		 * @details The bundle «a device — its addresses»: the name of a network device,
		 *          the address of a network fastened to it and its hardware address. Is needed where
		 *          a machine has several devices and it is important through which exactly
		 *          the exchange should be performed — at the choice of a source address, at the multicast to
		 *          a group, at the polling of the available devices
		 * @note The hardware address is started at once and always, but the address of a network
		 *       is passed from the outside and may be absent: a device brought up without
		 *       an address is an ordinary thing
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Source {
			// Название сетвого интерфейса
			string iface;
			// IP-адрес сети
			unique_ptr <addr_t> ip;
			// MAC-адрес сети
			unique_ptr <addr_t> mac;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param ip адрес сетевого подключения
			 *
			 * \~english
			 * @brief Constructor
			 * @param ip address of the network connection
			 *
			 * \~
			 */
			explicit Source(unique_ptr <addr_t> ip) noexcept;
		} src_t;

		/**
		 * \~russian
		 * @brief Структура атрибутов подключения
		 *
		 * @details Общее основание для описания **точки подключения** - того, куда
		 *          подключаются или где слушают. Отличается от адреса тем, что несёт
		 *          не одни лишь байты адреса, а всё нужное для установки связи:
		 *          вместе с адресом ещё и порт, а вместо адреса - доменное имя, если
		 *          узел задан именем
		 *
		 *          Наследников три: точка, заданная доменным именем и портом; точка,
		 *          заданная адресом сети и портом; и местный сокет, заданный путём в
		 *          файловой системе
		 *
		 * @note Признаком вида здесь служит **поле вида**, а не длина, как у адреса.
		 *       Иерархии эти разные и путать их не следует: адрес отвечает на вопрос
		 *       «куда», атрибуты - «куда и как»
		 *
		 * @note Точка, заданная адресом сети, заводится **пустой**: вид её не
		 *       определён, а память под сам адрес не выделена. Так и задумано -
		 *       адреса ведь ещё нет, и объявлять его вид заранее не из чего.
		 *       Прежде здесь выставлялся IPv4 с выделением памяти под него, но это
		 *       порождало неоднозначности: сперва непонятно было, отчего IPv6
		 *       память требует, а IPv4 нет, а после устранения этого - отчего
		 *       память под IPv4 выделяется, чтобы тут же быть перезаписанной
		 *       памятью под настоящий адрес. Пустота от неоднозначностей избавляет,
		 *       и плата за неё - помнить, что точка пуста
		 *
		 * @warning Отсюда обязанность заполняющего: выставить и адрес, и вид.
		 *          Разбирающий же точку по виду обязан предусмотреть случай
		 *          неопределённого вида и пустого адреса - иначе пустая точка будет
		 *          молча пропущена
		 *
		 * \~english
		 * @brief Structure of the attributes of a connection
		 * @details The common base for the description of a **point of a connection** — of that where
		 *          one connects to or where one listens. Differs from an address in that it carries
		 *          not the bytes of an address alone, but everything needed for the establishment of a link:
		 *          together with the address also a port, and instead of the address — a domain name, if
		 *          the node is set by a name
		 *          There are three descendants: a point set by a domain name and by a port; a point
		 *          set by an address of a network and by a port; and a local socket set by a path in
		 *          the file system
		 * @note The sign of the kind here is the **field of the kind**, and not the length, as at an address.
		 *       These hierarchies are different and they should not be confused: an address answers the question
		 *       «where to», the attributes — «where to and how»
		 * @note A point set by an address of a network is started as an **empty** one: its kind is not
		 *       defined, and the memory for the address itself is not allocated. It is intended so —
		 *       there is no address yet, after all, and there is nothing to declare its kind in advance from.
		 *       Formerly IPv4 was set out here with an allocation of the memory for it, but this
		 *       generated the ambiguities: at first it was unclear why IPv6
		 *       requires the memory, and IPv4 does not, and after the elimination of this — why
		 *       the memory for IPv4 is allocated only to be overwritten right away
		 *       by the memory for the real address. The emptiness rids of the ambiguities,
		 *       and the price for it is to remember that the point is empty
		 * @warning Hence the duty of the one who fills: to set out both the address and the kind.
		 *          The one who resolves a point by the kind, though, is obliged to provide for the case
		 *          of an undefined kind and of an empty address — otherwise an empty point will be
		 *          silently skipped
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Attributes {
			// Тип адреса подключения
			type_t type;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param type тип адреса подключения
			 *
			 * \~english
			 * @brief Constructor
			 * @param type type of the address of the connection
			 *
			 * \~
			 */
			explicit Attributes(const type_t type) noexcept;
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
			virtual ~Attributes() noexcept = default;
		} attr_t;

		/**
		 * \~russian
		 * @brief Структура FQDN-адреса подключения
		 *
		 * @details Точка подключения, заданная доменным именем и портом. Разрешение
		 *          имени в адрес откладывается до самого подключения, отчего одна и
		 *          та же точка может со временем указывать на разные узлы
		 *
		 * @note Имени может отвечать несколько адресов, и разных версий сразу.
		 *       Который из них будет выбран, здесь не задаётся - это решается при
		 *       разрешении имени
		 *
		 * \~english
		 * @brief Structure of an FQDN address of a connection
		 * @details A point of a connection set by a domain name and by a port. The resolution
		 *          of the name into an address is postponed until the connection itself, and therefore one and
		 *          the same point may with the time point at different nodes
		 * @note Several addresses may answer to a name, and of different versions at once.
		 *       Which of them will be chosen is not set here — this is decided at
		 *       the resolution of the name
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Attributes_FQDN : public attr_t {
			// Порт хоста
			uint16_t port;
			// Доменное имя хоста
			string domain;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Attributes_FQDN() noexcept;
		} attr_fqdn_t;

		/**
		 * \~russian
		 * @brief Структура IP-адреса подключения
		 *
		 * @details Точка подключения, заданная готовым адресом сети и портом.
		 *          Разрешения имени не требует, поэтому подключение по ней
		 *          устанавливается сразу
		 *
		 * @warning Вид при заведении остаётся **неопределённым** и не выводится из
		 *          выставленного адреса сам. Заполнить его - выбрав между IPv4 и
		 *          IPv6 - должен тот, кто выставляет адрес
		 *
		 * @note Пустота эта **намеренная**, и выставлять здесь IPv4 по умолчанию не
		 *       следует. Прежде так и было: заводился вид IPv4 и под него память, -
		 *       и выходило двоякое. Во-первых, непонятно было, отчего одной
		 *       разновидности память отводится, а другой нет. Во-вторых, всякому
		 *       адресу сперва отводилась память под IPv4, а затем перезаписывалась
		 *       под настоящую разновидность - работа впустую при любом исходе.
		 *       Плата за пустоту - помнить о ней: там, где вид оказался `NONE`, а
		 *       указатель нулевым, адрес нужно завести явно, и разновидность его в
		 *       таких местах всегда известна
		 *
		 * \~english
		 * @brief Structure of an IP address of a connection
		 * @details A point of a connection set by a ready address of a network and by a port.
		 *          Requires no resolution of a name, and therefore a connection by it
		 *          is established at once
		 * @warning The kind at the starting remains an **undefined** one and is not derived from
		 *          the set out address by itself. It must be filled — by a choice between IPv4 and
		 *          IPv6 — by the one who sets out the address
		 * @note This emptiness is a **deliberate** one, and setting out IPv4 by default here is not
		 *       advisable. Formerly it was exactly so: the kind IPv4 was started and the memory for it, —
		 *       and a double thing came out. Firstly, it was unclear why to one
		 *       variety the memory is given over, and to another one it is not. Secondly, for every
		 *       address at first the memory for IPv4 was given over, and then was overwritten
		 *       for the real variety — a work for nothing at any outcome.
		 *       The price for the emptiness is to remember about it: where the kind has turned out to be `NONE`, and
		 *       the pointer a null one, the address needs to be started explicitly, and its variety in
		 *       such places is always known
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Attributes_Network : public attr_t {
			// Порт хоста
			uint16_t port;
			// IP-адрес хоста
			unique_ptr <addr_t> ip;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Attributes_Network() noexcept;
		} attr_net_t;

		/**
		 * \~russian
		 * @brief Структура UDS-адреса подключения
		 *
		 * @details Точка подключения через местный сокет - тот, что живёт в файловой
		 *          системе и связывает лишь процессы одной машины. Порта у него нет,
		 *          его роль играет путь
		 *
		 * @note Обмен через местный сокет минует сетевой стек целиком и оттого много
		 *       быстрее подключения через петлевой адрес. Права на него - обычные
		 *       права файловой системы, что даёт разграничение доступа, которого у
		 *       сетевого сокета нет
		 *
		 * \~english
		 * @brief Structure of a UDS address of a connection
		 * @details A point of a connection through a local socket — the one that lives in the file
		 *          system and connects only the processes of one machine. It has no port,
		 *          its role is played by the path
		 * @note The exchange through a local socket bypasses the network stack entirely and is therefore much
		 *       faster than a connection through the loopback address. The rights on it are the ordinary
		 *       rights of the file system, which gives a delimitation of the access, which
		 *       a network socket has not
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Attributes_Unix_Domain_Socket : public attr_t {
			// Путь к сокету
			unique_ptr <addr_t> path;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Attributes_Unix_Domain_Socket() noexcept;
		} attr_uds_t;

		/**
		 * \~russian
		 * @brief Максимальный размер опакового ключа сессии
		 *
		 * @details Размера достаточно для идентификатора соединения QUIC, который
		 *          ограничен двадцатью октетами (RFC 9000 §17.2), с запасом на
		 *          иные протоколы с собственной адресацией сессий поверх UDP.
		 *
		 * \~english
		 * @brief Maximum size of an opaque key of a session
		 * @details The size is enough for the identifier of a QUIC connection, which
		 *          is limited to twenty octets (RFC 9000 §17.2), with a reserve for
		 *          the other protocols with their own addressing of the sessions over UDP.
		 *
		 * \~
		 */
		static constexpr uint8_t MAX_ORIGIN_KEY_SIZE = 32;

		/**
		 * \~russian
		 * @brief Структура опакового ключа сессии
		 *
		 * @details Протоколы, адресующие сессию не четвёркой сокета, а собственным
		 *          идентификатором внутри датаграммы, задают ключ маршрутизации
		 *          сами. Содержимое ключа сетевым движком не интерпретируется:
		 *          он лишь сравнивает и хеширует его.
		 *
		 * \~english
		 * @brief Structure of an opaque key of a session
		 * @details The protocols addressing a session not by the quadruple of a socket, but by their own
		 *          identifier inside a datagram, set the key of the routing
		 *          themselves. The content of the key is not interpreted by the network engine:
		 *          it only compares and hashes it.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Origin_Key {
			// Размер ключа сессии
			uint8_t size;
			// Данные ключа сессии
			uint8_t data[MAX_ORIGIN_KEY_SIZE];
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Origin_Key() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param data данные ключа сессии
			 * @param size размер ключа сессии
			 *
			 * \~english
			 * @brief Constructor
			 * @param data data of the key of the session
			 * @param size size of the key of the session
			 *
			 * \~
			 */
			explicit Origin_Key(const uint8_t * data, const uint8_t size) noexcept;
		} origin_key_t;

		/**
		 * \~russian
		 * @brief Структура метаданных последнего принятого дейтаграммного пакета
		 *
		 * @details Сведения о пакете, извлечённые не из его содержимого, а из
		 *          заголовков и вспомогательных сообщений ядра: через какое
		 *          устройство пакет пришёл, сколько переходов он прошёл, каким
		 *          разрядом обслуживания помечен и не сообщила ли сеть о заторе на
		 *          пути
		 *
		 *          Дейтаграммный обмен, в отличие от потокового, состоит из
		 *          отдельных сообщений, и у каждого сведения свои. Нужны они там,
		 *          где решение зависит не только от содержимого: выбор устройства
		 *          для ответа, отбраковка пакетов, пришедших не с той стороны,
		 *          снижение темпа отправки при заторе
		 *
		 * @warning Сведения относятся к **последнему принятому** пакету и
		 *          перезаписываются следующим. Читать их следует сразу по приёме, а
		 *          не отложенно
		 *
		 * @note Часть полей заполняется, лишь если приём этих сведений заранее
		 *       разрешён у сокета, - иначе они остаются в исходном состоянии
		 *
		 * \~english
		 * @brief Structure of the metadata of the last received datagram packet
		 * @details The information about a packet extracted not from its content, but from
		 *          the headers and from the auxiliary messages of the kernel: through which
		 *          device the packet has come, how many hops it has passed, by which
		 *          class of the service it is marked and whether the network has reported a congestion on
		 *          the path
		 *          The datagram exchange, unlike the stream one, consists of
		 *          separate messages, and every one has its own information. It is needed where
		 *          a decision depends not only on the content: the choice of a device
		 *          for an answer, the rejection of the packets that have come from the wrong side,
		 *          the lowering of the rate of the sending at a congestion
		 * @warning The information relates to the **last received** packet and
		 *          is overwritten by the next one. It should be read right at the reception, and
		 *          not in a postponed way
		 * @note A part of the fields is filled only if the reception of this information is allowed
		 *       at the socket in advance — otherwise they remain in the initial state
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Datagram_Info {
			// Сырое значение TTL/Hop Limit последнего принятого пакета (RFC, 0..255)
			uint8_t hops;
			// Индекс входного интерфейса
			uint32_t ifaceIndex;
			// Семейство принятого пакета
			event::family_t family;
			// Признак перегрузки пути принятого пакета (ECN)
			event::ecn_t congestion;
			// Протокол принятого пакета
			event::protocol_t protocol;
			// Класс трафика (TOS/Traffic Class)
			event::dscp_t trafficClass;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Datagram_Info() noexcept;
		} dgram_info_t;

		/**
		 * \~russian
		 * @brief Структура информации о пакетах в тоннеле
		 *
		 * @details Описание внешней оболочки туннеля - той, в которую упаковываются
		 *          переносимые пакеты. Задаёт, между какими точками проложен
		 *          туннель, каким протоколом переносятся данные и сколько переходов
		 *          отведено самой оболочке
		 *
		 * @note Точки эти относятся к **внешней** сети, той, по которой оболочка
		 *       идёт, и с адресами внутри туннеля не связаны. Семейства их вполне
		 *       могут расходиться: перенос пакетов IPv6 внутри IPv4 - обычное
		 *       назначение туннеля
		 *
		 * @warning Оболочка добавляет к каждому пакету свои заголовки, отчего
		 *          наибольший размер полезного пакета внутри туннеля оказывается
		 *          меньше, чем во внешней сети. Не учтённое, это приводит к дроблению
		 *          либо к молчаливой потере крупных пакетов
		 *
		 * \~english
		 * @brief Structure of the information about the packets in a tunnel
		 * @details The description of the external wrapping of a tunnel — of the one the
		 *          carried packets are packed into. Sets between which points
		 *          the tunnel is laid, by which protocol the data is carried and how many hops
		 *          are given over to the wrapping itself
		 * @note These points relate to the **external** network, the one the wrapping
		 *       goes over, and are not connected with the addresses inside the tunnel. Their families quite
		 *       may diverge: the carrying of the IPv6 packets inside IPv4 is an ordinary
		 *       purpose of a tunnel
		 * @warning The wrapping adds its own headers to every packet, and therefore
		 *          the largest size of a useful packet inside a tunnel turns out to be
		 *          smaller than in the external network. Not taken into account, this leads to a fragmentation
		 *          or to a silent loss of the large packets
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Tunnel_Info {
			// Количество хопов
			event::hops_t hops;
			// Семейство адресов
			event::family_t family;
			// Протокол подключения
			event::protocol_t protocol;
			// Адрес назначения подключения
			unique_ptr <attr_t> target;
			// Адрес источника подключения
			unique_ptr <attr_t> source;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Tunnel_Info() noexcept;
		} tun_info_t;

		/**
		 * \~russian
		 * @brief Структура сетевого интерфейса
		 *
		 * @details Описание сетевого устройства, каким его сообщает система:
		 *          название, наибольший размер передаваемого через него пакета и
		 *          набор признаков состояния
		 *
		 * @note Признаки собраны набором, а не двоичным словом, и проверяются
		 *       поиском в нём: значения перечисления признаков - порядковые номера,
		 *       а не разряды, и складывать их побитово нельзя
		 *
		 * @note Адресов устройства здесь нет - их держит связка `src_t`: у одного
		 *       устройства адресов может быть несколько
		 *
		 * \~english
		 * @brief Structure of a network interface
		 * @details The description of a network device as the system reports it:
		 *          the name, the largest size of a packet transmitted through it and
		 *          the set of the signs of the state
		 * @note The signs are gathered as a set, and not as a binary word, and are checked
		 *       by a search in it: the values of the enumeration of the signs are the ordinal numbers,
		 *       and not the digits, and adding them bitwise is not allowed
		 * @note There are no addresses of a device here — they are held by the `src_t` bundle: at one
		 *       device there may be several addresses
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Interface {
			string name;                             // Название интерфейса
			uint32_t mtu;                            // MTU интерфейса
			unordered_set <event::eth_flag_t> flags; // Флаги интерфейса
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Interface() noexcept;
		} iface_t;

		/**
		 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
		 */
		#if __linux__ || __FreeBSD__ || __sun
			/**
			 * \~russian
			 * @brief Пространство имён для работы с SCTP
			 *
			 * @details Средства протокола, сочетающего надёжность потокового обмена с
			 *          сохранением границ сообщений. Особенность его в том, что внутри
			 *          одного подключения ведётся несколько независимых потоков:
			 *          задержка сообщения в одном не задерживает остальные - беда,
			 *          которой страдает обычный потоковый обмен. Подключение к тому же
			 *          может опираться сразу на несколько адресов и переживать отказ
			 *          части из них
			 *
			 * @note Доступен протокол на Linux, FreeBSD, Solaris и illumos (OpenIndiana
			 *       и её родня) - прочие системы его не несут, отчего всё это
			 *       пространство имён собирается только там
			 *
			 *       Solaris держит собственную реализацию, а не перенесённую из FreeBSD,
			 *       и illumos унаследовал её от него. У macOS и OpenBSD протокола нет
			 *       вовсе; NetBSD же расставляет ловушку - заголовок `netinet/sctp.h`
			 *       там есть, и сборка проходит, а ядро протокол не даёт: ни модуля, ни
			 *       ветви `net.inet.sctp`. Отсутствие ошибок сборки на NetBSD
			 *       доказательством поддержки считать нельзя
			 *
			 * \~english
			 * @brief Namespace for working with SCTP
			 * @details The means of the protocol combining the reliability of the stream exchange with
			 *          the preservation of the boundaries of the messages. Its peculiarity is that inside
			 *          one connection several independent streams are kept:
			 *          a delay of a message in one does not delay the others — a trouble
			 *          which the ordinary stream exchange suffers from. The connection moreover
			 *          may rely on several addresses at once and outlive the failure
			 *          of a part of them
			 * @note The protocol is available on Linux, FreeBSD, Solaris and illumos (OpenIndiana
			 *       and its kin) — the other systems do not carry it, and therefore all this
			 *       namespace is built only there
			 *       Solaris holds its own implementation, and not one ported from FreeBSD,
			 *       and illumos has inherited it from it. macOS and OpenBSD have no protocol
			 *       at all; NetBSD, though, sets a trap — the `netinet/sctp.h` header
			 *       is there, and the build passes, but the kernel does not give the protocol: neither a module, nor
			 *       the `net.inet.sctp` branch. The absence of the errors of the build on NetBSD
			 *       cannot be considered a proof of the support
			 *
			 * \~
			 */
			namespace sctp {
				/**
				 * \~russian
				 * @brief Идентификатор полезной нагрузки SCTP
				 *
				 * @details Пометка, которой отправитель заявляет, что за данными
				 *          сообщения стоит. Протокол сам содержимое не разбирает и
				 *          пометку лишь переносит получателю, позволяя тому выбрать
				 *          разборщик, не заглядывая в сами данные
				 *
				 * @note Значения эти закреплены за назначениями сообща и произвольно
				 *       выбираться не должны: перечислены здесь лишь те, что нужны
				 *       движку, а весь их список ведётся отдельно
				 *
				 * \~english
				 * @brief Identifier of the payload of SCTP
				 * @details A mark by which the sender declares what stands behind the data
				 *          of a message. The protocol itself does not resolve the content and
				 *          only carries the mark to the receiver, allowing it to choose
				 *          a parser without looking into the data itself
				 * @note These values are fastened to the purposes jointly and must not be chosen
				 *       arbitrarily: enumerated here are only those needed by
				 *       the engine, and their whole list is kept separately
				 *
				 * \~
				 */
				enum class ppid_t : uint8_t {
					DTLS       = 0x32, // (RFC 6083) DTLS поверх SCTP
					WEBRTC_STR = 0x33, // Строковые данные канала WebRTC
					WEBRTC_BIN = 0x35  // Бинарные данные канала WebRTC
				};

				/**
				 * \~russian
				 * @brief Статусы таймаутов SCTP
				 *
				 * @details Указывает, какой из внутренних отсчётов протокола истёк.
				 *          Отсчёты эти стерегут каждый свой этап: установку
				 *          подключения, доставку данных, подтверждение приёма,
				 *          проверку живости и закрытие
				 *
				 * @note Истечение отсчёта само по себе разрывом не является -
				 *       протокол повторяет попытку и разрывает подключение лишь
				 *       исчерпав их число. Известие об истечении потому стоит
				 *       понимать как примету неполадок на пути, а не как отказ
				 *
				 * \~english
				 * @brief Statuses of the timeouts of SCTP
				 * @details Specifies which of the internal counts of the protocol has expired.
				 *          These counts guard each its own stage: the establishment of
				 *          a connection, the delivery of the data, the acknowledgement of the reception,
				 *          the check of the liveness and the closing
				 * @note The expiration of a count by itself is not a break —
				 *       the protocol repeats an attempt and breaks a connection only
				 *       having exhausted their number. The notice of an expiration is therefore worth
				 *       understanding as a marker of the troubles on the path, and not as a refusal
				 *
				 * \~
				 */
				enum class timeout_t : uint8_t {
					NONE        = 0x00, // Таймаут отсутствует
					INIT        = 0x01, // Таймаут INIT
					DATA        = 0x02, // Таймаут DATA
					SACK        = 0x03, // Таймаут SACK
					SHUTDOWN    = 0x04, // Таймаут SHUTDOWN
					HEARTBEAT   = 0x05, // Таймаут HEARTBEAT
					COOKIE      = 0x06, // Таймаут COOKIE-ECHO
					SHUTDOWNACK = 0x07  // Таймаут SHUTDOWN-ACK
				};

				/**
				 * \~russian
				 * @brief Типы аутентификации события SCTP
				 *
				 * \~english
				 * @brief Types of the authentication of an SCTP event
				 *
				 * \~
				 */
				enum class auth_type_t : uint8_t {
					HMAC_RSVD    = 0x00, // ЗаSCTPрезервировано
					HMAC_SHA1    = 0x01, // HMAC-SHA1 аутентификация
					HMAC_SHA256  = 0x02  // HMAC-SHA256 аутентификация
				};

				/**
				 * \~russian
				 * @brief Типы чанков попадающие под аутентификацию SCTP
				 *
				 * @details Сообщение протокола состоит из частей, и подтверждению
				 *          подлинности подлежит не всё подряд, а лишь оговорённый
				 *          набор. Набор этот стороны согласуют при установке
				 *          подключения, и значения перечисления его и описывают
				 *
				 * @warning Части, в набор не вошедшие, идут **без подтверждения
				 *          подлинности** и подменить их может кто угодно. Полагаться
				 *          на подлинность всего обмена лишь потому, что подтверждение
				 *          включено, не следует - важно ещё и что именно в набор
				 *          попало
				 *
				 * \~english
				 * @brief Types of the chunks falling under the authentication of SCTP
				 * @details A message of the protocol consists of the parts, and subject to the confirmation
				 *          of the authenticity is not everything in a row, but only a stipulated
				 *          set. This set the sides agree at the establishment of
				 *          a connection, and the values of the enumeration describe it
				 * @warning The parts that have not entered the set go **without a confirmation
				 *          of the authenticity** and anyone may substitute them. Relying
				 *          on the authenticity of the whole exchange only because the confirmation
				 *          is switched on is not advisable — what exactly has entered the set
				 *          matters as well
				 *
				 * \~
				 */
				enum class auth_chunk_t : uint8_t {
					DATA              = 0x00, // Чанк DATA подлежит аутентификации
					INIT              = 0x01, // Чанк INIT подлежит аутентификации
					INIT_ACK          = 0x02, // Чанк INIT-ACK подлежит аутентификации
					SACK              = 0x03, // Чанк SACK подлежит аутентификации
					HEARTBEAT         = 0x04, // Чанк HEARTBEAT подлежит аутентификации
					HEARTBEAT_ACK     = 0x05, // Чанк HEARTBEAT-ACK подлежит аутентификации
					ABORT             = 0x06, // Чанк ABORT подлежит аутентификации
					SHUTDOWN          = 0x07, // Чанк SHUTDOWN подлежит аутентификации
					SHUTDOWN_ACK      = 0x08, // Чанк SHUTDOWN-ACK подлежит аутентификации
					ERROR             = 0x09, // Чанк ERROR подлежит аутентификации
					COOKIE_ECHO       = 0x0A, // Чанк COOKIE-ECHO подлежит аутентификации
					COOKIE_ACK        = 0x0B, // Чанк COOKIE-ACK подлежит аутентификации
					ECNE              = 0x0C, // Чанк ECNE подлежит аутентификации
					CWR               = 0x0D, // Чанк CWR подлежит аутентификации
					SHUTDOWN_COMPLETE = 0x0E, // Чанк SHUTDOWN-COMPLETE подлежит аутентификации
					AUTH              = 0x0F, // Чанк AUTH подлежит аутентификации
					FORWARD_TSN       = 0x10, // Чанк FORWARD-TSN подлежит аутентификации
					RE_CONFIG         = 0x11  // Чанк RE-CONFIG подлежит аутентификации
				};

				/**
				 * \~russian
				 * @brief Типы индикаторов события аутентификации
				 *
				 * \~english
				 * @brief Types of the indicators of an event of the authentication
				 *
				 * \~
				 */
				enum class auth_indics_t : uint8_t {
					NONE     = 0x00, // Тип аутентификации отсутствует
					NEW_KEY  = 0x01, // Событие нового ключа
					NO_AUTH  = 0x02, // Событие отсутствия аутентификации
					FREE_KEY = 0x03  // Событие освобождения ключа
				};

				/**
				 * \~russian
				 * @brief Флаги отправки сообщения SCTP
				 *
				 * @details Сообщает о судьбе сообщения, доставить которое не
				 *          удалось: успело ли оно уйти в сеть или было отброшено,
				 *          не покинув очереди отправки
				 *
				 * @note Различие это существенно при повторной отправке. Не ушедшее
				 *       сообщение сеть не видела, и отправить его заново безопасно, а
				 *       про ушедшее сказать, дошло ли оно частично, нельзя
				 *
				 * \~english
				 * @brief Flags of the sending of an SCTP message
				 * @details Reports the fate of a message which could not be
				 *          delivered: whether it has managed to go into the network or has been discarded,
				 *          without leaving the queue of the sending
				 * @note This difference is essential at a repeated sending. A message that has not gone
				 *       the network has not seen, and sending it anew is safe, and
				 *       about a gone one it is impossible to say whether it has reached partially
				 *
				 * \~
				 */
				enum class send_failed_t : uint8_t {
					NONE   = 0x00, // Флаг отсутствует
					SENT   = 0x01, // Сообщение отправлено
					UNSENT = 0x02  // Сообщение не отправлено
				};

				/**
				 * \~russian
				 * @brief Индикаторы доставки SCTP
				 *
				 * \~english
				 * @brief Indicators of the delivery of SCTP
				 *
				 * \~
				 */
				enum class pdapi_indics_t : uint8_t {
					NONE                     = 0x00, // Индикатор отсутствует
					PARTIAL_DELIVERY_ABORTED = 0x01  // Частичная доставка прервана
				};

				/**
				 * \~russian
				 * @brief Типы сброса потоков SCTP
				 *
				 * @details Сброс возвращает нумерацию потока к началу, не разрывая
				 *          самого подключения, - так поток переиспользуют под новый
				 *          обмен, не платя за установку связи заново. Значения
				 *          описывают, что со сбросом стало: он проведён для
				 *          исходящих или входящих потоков, не удался либо отклонён
				 *          другой стороной
				 *
				 * @note Сброс требует согласия обеих сторон: сторона, его не
				 *       поддерживающая, отклонит просьбу, и поток останется прежним
				 *
				 * \~english
				 * @brief Types of the reset of the SCTP streams
				 * @details A reset returns the numbering of a stream to the beginning, without breaking
				 *          the connection itself, — that is how a stream is reused for a new
				 *          exchange, without paying for the establishment of a link anew. The values
				 *          describe what has become of the reset: it is performed for
				 *          the outgoing or for the incoming streams, has failed or is rejected
				 *          by the other side
				 * @note A reset requires the agreement of both sides: a side not
				 *       supporting it will reject the request, and the stream will remain the previous one
				 *
				 * \~
				 */
				enum class stream_reset_t : uint8_t {
					NONE         = 0x00, // Тип сброса отсутствует
					DENIED       = 0x01, // Сброс отклонён
					FAILED       = 0x02, // Сброс не выполнен
					OUTGOING_SSN = 0x03, // Сброс исходящих потоков
					INCOMING_SSN = 0x04  // Сброс входящих потоков
				};

				/**
				 * \~russian
				 * @brief Типы изменения потоков SCTP
				 *
				 * \~english
				 * @brief Types of the change of the SCTP streams
				 *
				 * \~
				 */
				enum class stream_change_t : uint8_t {
					NONE   = 0x00, // Тип изменения отсутствует
					FAILED = 0x01, // Изменение не выполнено
					DENIED = 0x02  // Изменение отклонено
				};

				/**
				 * \~russian
				 * @brief Статусы состояния сокета SCTP
				 *
				 * \~english
				 * @brief Statuses of the state of an SCTP socket
				 *
				 * \~
				 */
				enum class state_status_t : uint8_t {
					NONE              = 0x00, // Статус отсутствует
					BOUND             = 0x01, // Вызван bind(), но ассоциация не установлена
					CLOSED            = 0x02, // Ассоциация не существует (сокет создан, но не привязан/не использован)
					LISTEN            = 0x03, // Сервер вызвал listen() и ждёт входящих INIT (только для STREAM)
					ESTABLISHED       = 0x04, // Ассоциация установлена, можно передавать данные
					COOKIE_WAIT       = 0x05, // Клиент отправил INIT, ждёт INIT-ACK
					COOKIE_ECHOED     = 0x06, // Клиент получил INIT-ACK, отправил COOKIE-ECHO, ждёт COOKIE-ACK
					SHUTDOWN_SENT     = 0x07, // Отправлен SHUTDOWN, ждём SHUTDOWN-ACK
					SHUTDOWN_PENDING  = 0x08, // Приложение вызвало shutdown(), но есть неподтверждённые данные
					SHUTDOWN_RECEIVED = 0x09, // Получен SHUTDOWN от пираа, ждём
					SHUTDOWN_ACK_SENT = 0x0A  // Отправлен SHUTDOWN-ACK, ждём SHUTDOWN-COMPLETE
				};

				/**
				 * \~russian
				 * @brief Типы событий SCTP
				 *
				 * \~english
				 * @brief Types of the SCTP events
				 *
				 * \~
				 */
				enum class event_type_t : uint8_t {
					NONE                   = 0x00, // Тип события отсутствует
					DATA_IO                = 0x01, // Присылать уведомление о каждом входящем DATA-пакете
					SEND_FAILED            = 0x02, // Ошибка отправки сообщения
					REMOTE_ERROR           = 0x03, // Ошибка удалённого узла
					ASSOC_CHANGE           = 0x04, // Изменение ассоциации
					SHUTDOWN_EVENT         = 0x05, // Событие завершения работы
					SENDER_DRY_EVENT       = 0x06, // Событие "отправитель сухой"
					PEER_ADDR_CHANGE       = 0x07, // Изменение адреса однорангового узла
					ASSOC_RESET_EVENT      = 0x08, // Сброс ассоциации
					SEND_FAILED_EVENT      = 0x09, // Событие ошибки отправки
					STREAM_RESET_EVENT     = 0x0A, // Сброс потока
					STREAM_CHANGE_EVENT    = 0x0B, // Изменение потоков
					AUTHENTICATION_EVENT   = 0x0C, // Событие аутентификации
					ADAPTATION_INDICATION  = 0x0D, // Адаптационное указание
					PARTIAL_DELIVERY_EVENT = 0x0E  // Частичная доставка
				};

				/**
				 * \~russian
				 * @brief Типы сброса ассоциации SCTP
				 *
				 * \~english
				 * @brief Types of the reset of an SCTP association
				 *
				 * \~
				 */
				enum class assoc_reset_t : uint8_t {
					NONE   = 0x00, // Тип сброса отсутствует
					FAILED = 0x01, // Сброс не выполнен
					DENIED = 0x02  // Сброс отклонён
				};

				/**
				 * \~russian
				 * @brief Информация об ассоциации SCTP
				 *
				 * \~english
				 * @brief Information about an SCTP association
				 *
				 * \~
				 */
				enum class assoc_info_t : uint8_t {
					NONE                = 0x00, // Информация об ассоциации отсутствует
					SUPPORTS_PR         = 0x01, // Поддерживается частичное надёжное сообщение
					SUPPORTS_MAX        = 0x02, // Поддерживается максимальное количество сообщений
					SUPPORTS_AUTH       = 0x03, // Поддерживается аутентификация сообщений
					SUPPORTS_ASCONF     = 0x04, // Поддерживается динамическая конфигурация адресов
					SUPPORTS_MULTIBUF   = 0x05, // Поддерживается мультибуферизация сообщений
					SUPPORTS_RE_CONFIG  = 0x06, // Поддерживается повторная конфигурация ассоциации
					SUPPORTS_INTERLEAVE = 0x07  // Поддерживается перемежение сообщений
				};

				/**
				 * \~russian
				 * @brief Состояния ассоциации SCTP
				 *
				 * \~english
				 * @brief States of an SCTP association
				 *
				 * \~
				 */
				enum class assoc_state_t : uint8_t {
					NONE 	      = 0x00, // Состояние ассоциации отсутствует
					COMM_UP       = 0x01, // Связь установлена
					COMM_LOST     = 0x02, // Связь потеряна
					RESTARTED     = 0x03, // Связь перезапущена
					SHUTDOWN_COMP = 0x04, // Завершение работы выполнено
					CANT_START    = 0x05  // Не удалось запустить связь
				};

				/**
				 * \~russian
				 * @brief Состояния адреса однорангового узла SCTP
				 *
				 * \~english
				 * @brief States of the address of an SCTP peer node
				 *
				 * \~
				 */
				enum class paddr_state_t : uint8_t {
					NONE        = 0x00, // Состояние адреса отсутствует
					ADDED       = 0x01, // Адрес был добавлен в ассоциацию
					REMOVED     = 0x02, // Адрес был удалён из ассоциации
					MADE_PRIM   = 0x03, // Адрес был установлен как основной
					CONFIRMED   = 0x04, // Адрес подтверждён одноранговым узлом
					AVAILABLE   = 0x05, // Адрес стал доступен
					UNREACHABLE = 0x06  // Адрес стал недоступен
				};

				/**
				 * \~russian
				 * @brief Флаги информации о сообщении SCTP
				 *
				 * \~english
				 * @brief Flags of the information about an SCTP message
				 *
				 * \~
				 */
				enum class info_t : uint8_t {
					NONE               = 0x00, // Флаг отсутствует
					PR_TTL             = 0x01, // Сообщение имеет ограничение по времени жизни
					PR_RTX             = 0x02, // Сообщение имеет ограничение по количеству повторных попыток
					PR_PRIO            = 0x03, // Сообщение имеет приоритет
					SEND_ALL           = 0x04, // Отправка сообщения всем ассоциациям
					ADDR_OVER          = 0x05, // Использовать адрес из to, даже если сокет подключён
					STATUS_EOF 	       = 0x06, // Сообщение содержит признак грациозного завершения
					STATUS_ABORT       = 0x07, // Сообщение содержит признак аварийного завершения
					SACK_IMMEDIATELY   = 0x08, // Установка бита последнего фрагмента DATA, для мгновенной отправки
					DELIVERY_UNORDERED = 0x09  // Сообщение доставляется без учёта порядка в потоке
				};

				/**
				 * \~russian
				 * @brief Флаги полученного сообщения SCTP
				 *
				 * @details Описывают не содержимое сообщения, а обстоятельства его
				 *          получения: закончилось ли сообщение на этом куске, пришли ли
				 *          вместо данных известия протокола, соблюдался ли порядок в
				 *          потоке и не оказались ли данные усечены нехваткой буфера
				 *
				 * @warning Отсутствие признака конца сообщения означает, что сообщение
				 *          продолжается: следующий кусок принадлежит ему же, и склеивать
				 *          его надлежит до признака, а не по размеру буфера. Границы
				 *          сообщений протокол блюдёт, но узнать их можно только отсюда
				 *
				 * \~english
				 * @brief Flags of a received SCTP message
				 * @details They describe not the content of a message, but the circumstances of its
				 *          reception: whether the message has ended on this piece, whether instead of
				 *          the data the notices of the protocol have come, whether the order in
				 *          the stream has been observed and whether the data have turned out truncated by the lack of a buffer
				 * @warning The absence of the sign of the end of a message means that the message
				 *          continues: the next piece belongs to it as well, and glueing
				 *          it is due up to the sign, and not by the size of the buffer. The boundaries
				 *          of the messages the protocol keeps, but learning them is possible only from here
				 *
				 * \~
				 */
				enum class receipt_t : uint8_t {
					NONE               = 0x00, // Флаг отсутствует
					END_OF_RECORD      = 0x01, // Сообщение получено целиком, границей записи
					NOTIFICATION       = 0x02, // Вместо данных получено известие протокола
					DATA_TRUNCATED     = 0x03, // Данные сообщения усечены нехваткой буфера
					INFO_TRUNCATED     = 0x04, // Метаданные сообщения усечены нехваткой буфера
					DELIVERY_UNORDERED = 0x05  // Сообщение доставлено без учёта порядка в потоке
				};

				/**
				 * \~russian
				 * @brief Множество типов событий SCTP
				 *
				 * \~english
				 * @brief Set of the types of the SCTP events
				 *
				 * \~
				 */
				using event_types_t = unordered_set <event_type_t>;

				/**
				 * \~russian
				 * @brief Структура метаданных сообщения SCTP
				 *
				 * @details Структура содержит информацию о полезной нагрузке,
				 *          номере потока, времени жизни, контексте и флагах сообщения.
				 *
				 * @par Намеренные решения
				 *
				 *      **Структура не упакована.** Прежде на ней стояло `packed`, но
				 *      признак этот отвергался: упаковать структуру с полем-множеством
				 *      нельзя, оно не простого вида. GCC сообщал об этом
				 *      предупреждением, Clang умалчивал, - а упакована структура не
				 *      была ни там, ни там
				 *
				 *      Упаковка ей и не нужна: в сеть уходят не эти метаданные, а
				 *      сообщение, ими описанное. Здесь же поля лишь передаются между
				 *      своими вызовами, и выравнивание им только на пользу
				 *
				 * \~english
				 * @brief Structure of the metadata of an SCTP message
				 * @details The structure contains the information about the payload,
				 *          about the number of the stream, about the lifetime, about the context and about the flags of the message.
				 * @par Deliberate decisions
				 *      **The structure is not packed.** Formerly `packed` stood on it, but
				 *      that sign was rejected: a structure with a field-set cannot
				 *      be packed, it is not of a simple kind. GCC reported this
				 *      by a warning, Clang kept silent, — and the structure was packed
				 *      neither there nor there
				 *      It does not need the packing either: into the network go not these metadata, but
				 *      the message described by them. Here the fields are only passed between
				 *      one's own calls, and the alignment is only of use to them
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Message_Info {
					ppid_t ppid;                  // Идентификатор полезной нагрузки
					uint16_t num;                 // Номер потока
					uint32_t ttl;                 // Время жизни (в миллисекундах)
					uint32_t ctx;                 // Контекст для уведомлений об ошибках
					unordered_set <info_t> flags; // Флаги сообщения
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Message_Info() noexcept;
				} minfo_t;

				/**
				 * \~russian
				 * @brief Структура метаданных полученного сообщения SCTP
				 *
				 * @details Описывает пришедшее сообщение: каким потоком оно пришло, каким
				 *          по счёту в этом потоке, под какой пометкой полезной нагрузки и
				 *          при каких обстоятельствах получено
				 *
				 * @par Намеренные решения
				 *
				 *      **Структура отделена от `minfo_t`.** Прежде одна структура служила
				 *      обеим сторонам: ею же задавали отправку. Поля сторон, однако, не
				 *      совпадают - у отправки нет ни порядкового номера, ни номера
				 *      передачи, ни признака конца записи, а у приёма нет ни времени
				 *      жизни, ни политик частичной надёжности. Общая структура заставляла
				 *      бы каждую сторону обходить чужие поля, гадая, заполнены ли они
				 *
				 *      **Пометка полезной нагрузки хранится числом, а не `ppid_t`.**
				 *      Значения её закреплены за назначениями сообща, и прийти может
				 *      любое из них, а не только известные движку. Приводить пришедшее
				 *      к перечислению значило бы терять неизвестные пометки
				 *
				 * \~english
				 * @brief Structure of the metadata of a received SCTP message
				 * @details Describes an arrived message: by which stream it has come, which
				 *          by the count in that stream, under which mark of the payload and
				 *          under which circumstances it is received
				 * @par Deliberate decisions
				 *      **The structure is separated from `minfo_t`.** Formerly one structure served
				 *      both sides: by it the sending was set as well. The fields of the sides, however, do not
				 *      coincide — the sending has neither an ordinal number, nor a number
				 *      of the transmission, nor a sign of the end of a record, and the reception has neither a lifetime,
				 *      nor the policies of the partial reliability. A common structure would force
				 *      each side to walk around the foreign fields, guessing whether they are filled
				 *      **The mark of the payload is stored by a number, and not by `ppid_t`.**
				 *      Its values are fastened to the purposes jointly, and any of them may
				 *      come, and not only those known to the engine. Bringing an arrived one
				 *      to the enumeration would mean losing the unknown marks
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Received_Message_Info {
					uint32_t id;                     // Идентификатор ассоциации
					uint16_t num;                    // Номер потока
					uint16_t ssn;                    // Порядковый номер сообщения в потоке
					uint32_t tsn;                    // Номер передачи сообщения
					uint32_t ctx;                    // Контекст для уведомлений об ошибках
					uint32_t ppid;                   // Идентификатор полезной нагрузки
					uint32_t cumtsn;                 // Накопленный номер передачи
					unordered_set <receipt_t> flags; // Флаги полученного сообщения
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Received_Message_Info() noexcept;
				} rinfo_t;

				/**
				 * \~russian
				 * @brief Структура инициализации рукопожатия SCTP
				 *
				 * @details Структура содержит информацию о таймаутах, попытках подключения и количестве потоков.
				 *
				 * \~english
				 * @brief Structure of the initialization of an SCTP handshake
				 * @details The structure contains the information about the timeouts, about the attempts of the connection and about the number of the streams.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Initialization_Message {
					// Максимальное время инициализации SCTP
					uint16_t timeout;
					// Максимальное количество попыток подключения (по умолчанию 4)
					uint16_t attempts;
					// Максимальное количество исходящих потоков (по умолчанию 5)
					uint16_t ostreams;
					// Максимальное количество входящих потоков (по умолчанию 5)
					uint16_t istreams;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Initialization_Message() noexcept;
				} __attribute__((packed)) initmsg_t;

				/**
				 * \~russian
				 * @brief Структура статуса SCTP подключения
				 *
				 * @details Структура содержит информацию о состоянии ассоциации, размере окна передачи, количестве потоков и фрагментации.
				 *
				 * \~english
				 * @brief Structure of the status of an SCTP connection
				 * @details The structure contains the information about the state of the association, about the size of the window of the transmission, about the number of the streams and about the fragmentation.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Status {
					uint32_t id;          // ID ассоциации
					uint32_t ratewind;    // Размер окна скорости передачи
					uint16_t penddata;    // Количество ожидающих данных
					uint16_t ostreams;    // Количество исходящих потоков
					uint16_t istreams;    // Количество входящих потоков
					uint16_t unackdata;   // Количество неподтверждённых DATA чанков
					uint32_t fragpoint;   // Точка фрагментации в байтах
					state_status_t state; // Текущее состояние ассоциации
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Status() noexcept;
				} __attribute__((packed)) status_t;

				/**
				 * \~russian
				 * @brief Структура ошибки события SCTP
				 *
				 * @details Структура содержит информацию о коде и сообщении ошибки события.
				 *
				 * \~english
				 * @brief Structure of an error of an SCTP event
				 * @details The structure contains the information about the code and about the message of the error of the event.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Error {
					int32_t code;   // Код ошибки события
					string message; // Сообщение ошибки события
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Error() noexcept;
				} error_t;

				/**
				 * \~russian
				 * @brief Структура события SCTP
				 *
				 * @details Структура содержит идентификатор события и его тип.
				 *
				 * \~english
				 * @brief Structure of an SCTP event
				 * @details The structure contains the identifier of the event and its type.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event {
					// Идентификатор события
					uint32_t id;
					// Тип события
					event_type_t type;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event() noexcept;
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
					virtual ~Event() = default;
				} event_t;

				/**
				 * \~russian
				 * @brief Структура адаптационного указания SCTP
				 *
				 * @details Структура адаптационного указания SCTP.
				 *
				 * \~english
				 * @brief Structure of an SCTP adaptation indication
				 * @details The structure of an SCTP adaptation indication.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Adaptation : public event_t {
					// Адаптационное указание
					uint32_t indication;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Adaptation() noexcept;
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
					virtual ~Event_Adaptation() = default;
				} event_adaptation_t;

				/**
				 * \~russian
				 * @brief Структура изменения ассоциации события SCTP
				 *
				 * @details Структура содержит информацию о состоянии ассоциации,
				 *          количестве потоков и дополнительной информации события.
				 *
				 * \~english
				 * @brief Structure of the change of the association of an SCTP event
				 * @details The structure contains the information about the state of the association,
				 *          about the number of the streams and the additional information of the event.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Association_Change : public event_t {
					error_t error;              // Ошибка события
					uint16_t ostreams;          // Максимальное количество исходящих потоков
					uint16_t istreams;          // Максимальное количество входящих потоков
					assoc_state_t state;        // Состояние события
					vector <assoc_info_t> info; // Дополнительная информация события
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Association_Change() noexcept;
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
					virtual ~Event_Association_Change() = default;
				} event_assoc_change_t;

				/**
				 * \~russian
				 * @brief Структура сброса ассоциации SCTP
				 *
				 * @details Структура содержит информацию о последнем подтверждённом TSN,
				 *          последнем подтверждённом пиром TSN и флагах сброса ассоциации.
				 *
				 * \~english
				 * @brief Structure of the reset of an SCTP association
				 * @details The structure contains the information about the last acknowledged TSN,
				 *          about the last TSN acknowledged by the peer and about the flags of the reset of the association.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Association_Reset : public event_t {
					// Последний TSN (Transmission Sequence Number), подтверждённый вами (вы получили его от пира)
					uint32_t localTSN;
					// Последний TSN (Transmission Sequence Number), подтверждённый пиром (он получил его от вас)
					uint32_t remoteTSN;
					// Флаги сброса ассоциации
					unordered_set <assoc_reset_t> flags;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Association_Reset() noexcept;
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
					virtual ~Event_Association_Reset() = default;
				} event_assoc_reset_t;

				/**
				 * \~russian
				 * @brief Структура ошибки удалённого узла SCTP
				 *
				 * @details Структура содержит информацию о коде и сообщении ошибки удалённого узла,
				 *          а также дополнительную информацию события.
				 *
				 * \~english
				 * @brief Structure of an error of an SCTP remote node
				 * @details The structure contains the information about the code and about the message of the error of the remote node,
				 *          as well as the additional information of the event.
				 *
				 * \~
				 */
				typedef struct Event_Remote_Error : public event_t {
					error_t error;         // Ошибка события
					vector <uint8_t> data; // Дополнительная информация события
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Remote_Error() noexcept = default;
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
					virtual ~Event_Remote_Error() = default;
				} event_remote_error_t;

				/**
				 * \~russian
				 * @brief Структура изменения адреса однорангового узла SCTP
				 *
				 * @details Структура содержит информацию о коде ошибки события,
				 *          состоянии адреса однорангового узла и указатель на адрес однорангового узла.
				 *
				 * \~english
				 * @brief Structure of the change of the address of an SCTP peer node
				 * @details The structure contains the information about the code of the error of the event,
				 *          about the state of the address of the peer node and a pointer to the address of the peer node.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Address_Change : public event_t {
					error_t error;            // Ошибка события
					paddr_state_t state;      // Состояние события
					unique_ptr <addr_t> addr; // Адрес однорангового узла
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Address_Change() noexcept;
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
					virtual ~Event_Address_Change() = default;
				} event_addr_change_t;

				/**
				 * \~russian
				 * @brief Структура частичной доставки SCTP
				 *
				 * @details Структура содержит информацию о номере потока, последовательном номере сообщения и индикаторе частичной доставки.
				 *
				 * \~english
				 * @brief Structure of a partial delivery of SCTP
				 * @details The structure contains the information about the number of the stream, about the sequence number of the message and about the indicator of the partial delivery.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Partial_Delivery_Event : public event_t {
					uint16_t stream;           // Номер потока
					uint16_t sequence;         // Последовательный номер сообщения
					pdapi_indics_t indication; // Индикатор частичной доставки
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Partial_Delivery_Event() noexcept;
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
					virtual ~Partial_Delivery_Event() = default;
				} event_pdapi_t;

				/**
				 * \~russian
				 * @brief Структура аутентификации SCTP
				 *
				 * @details Структура содержит информацию о номере ключа и индикаторе аутентификации.
				 *
				 * \~english
				 * @brief Structure of the authentication of SCTP
				 * @details The structure contains the information about the number of the key and about the indicator of the authentication.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Authentication : public event_t {
					uint16_t key;             // Номер ключа
					auth_indics_t indication; // Индикатор аутентификации
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Authentication() noexcept;
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
					virtual ~Event_Authentication() = default;
				} event_auth_t;

				/**
				 * \~russian
				 * @brief Структура ошибки отправки SCTP
				 *
				 * @details Структура содержит информацию о коде ошибки события,
				 *          статусе отправки сообщения и дополнительную информацию события.
				 *
				 * \~english
				 * @brief Structure of an error of the sending of SCTP
				 * @details The structure contains the information about the code of the error of the event,
				 *          about the status of the sending of the message and the additional information of the event.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Send_Failed : public event_t {
					error_t error;         // Ошибка события
					send_failed_t status;  // Статус отправки сообщения
					vector <uint8_t> data; // Дополнительная информация события
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Send_Failed() noexcept;
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
					virtual ~Event_Send_Failed() = default;
				} event_send_failed_t;

				/**
				 * \~russian
				 * @brief Структура сброса потоков SCTP
				 *
				 * @details Структура содержит информацию о номерах сброшенных потоков и типах сброса потоков.
				 *
				 * \~english
				 * @brief Structure of the reset of the SCTP streams
				 * @details The structure contains the information about the numbers of the reset streams and about the types of the reset of the streams.
				 *
				 * \~
				 */
				typedef struct Event_Stream_Reset : public event_t {
					vector <uint16_t> streams;            // Номера сброшенных потоков
					unordered_set <stream_reset_t> flags; // Типы сброса потоков
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Stream_Reset() noexcept = default;
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
					virtual ~Event_Stream_Reset() = default;
				} event_stream_reset_t;

				/**
				 * \~russian
				 * @brief Структура изменения потоков SCTP
				 *
				 * @details Структура содержит информацию о максимальном количестве исходящих и входящих потоков, а также флаги сброса ассоциации.
				 *
				 * \~english
				 * @brief Structure of the change of the SCTP streams
				 * @details The structure contains the information about the maximum number of the outgoing and of the incoming streams, as well as the flags of the reset of the association.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Event_Stream_Change : public event_t {
					// Максимальное количество исходящих потоков
					uint16_t ostreams;
					// Максимальное количество входящих потоков
					uint16_t istreams;
					// Флаги сброса ассоциации
					unordered_set <stream_change_t> flags;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Event_Stream_Change() noexcept;
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
					virtual ~Event_Stream_Change() = default;
				} event_stream_change_t;
			};

			/**
			 * \~russian
			 * @brief Создаём тип данных SCTP события
			 *
			 * \~english
			 * @brief Create the data type of an SCTP event
			 *
			 * \~
			 */
			using sctp_event_t = unique_ptr <sctp::event_t>;
		#endif
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../sys/macro_pop.hpp"

#endif // __AWH_NETWORK__
