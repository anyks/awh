/**
 * @file: addr.hpp
 * @date: 2025-10-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля работы с сетевыми адресами — класс Network_Address для разбора, нормализации,
 *        сравнения и форматирования IPv4, IPv6 и MAC-адресов, работы с префиксами и масками сети,
 *        определения типов и принадлежности адреса зарезервированным диапазонам
 *
 * \~english
 * @brief Header file of the module of working with the network addresses — the Network_Address class for the parsing, the normalization,
 *        the comparison and the formatting of the IPv4, IPv6 and MAC addresses, the work with the prefixes and the masks of a network,
 *        the determination of the types and of the belonging of an address to the reserved ranges
 *
 * \~
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NET_ADDR__
#define __AWH_NET_ADDR__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <memory>
#include <cstdint>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "net.hpp"
#include "../sys/log.hpp"

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
	 * @brief Класс для работы с сетевыми адресами
	 *
	 * @details Держит **один** адрес и умеет с ним всё: разобрать из строки,
	 *          вывести обратно в разных видах, сравнить с другим, наложить маску,
	 *          проверить принадлежность сети или диапазону. Внутри адрес лежит
	 *          двоичным буфером постоянной ёмкости, отчего разбор и проверки
	 *          обходятся без обращений к куче
	 *
	 *          Работает объект с тремя видами записи: IPv4, IPv6 и аппаратный
	 *          адрес. Прочие значения `type_t` - `URL`, `FQDN`, `FS`, `NETV4`,
	 *          `NETV6` - служат только распознаванию в `host()`: разобрать их
	 *          `parse()` не возьмётся
	 *
	 * @note Объект хранит **состояние**, и почти все методы работают с уже
	 *       разобранным адресом. Пока `parse()` не отработал успешно, буфер пуст, и
	 *       выдающие методы возвращают нули, а не признак ошибки: `v4()` даст `0`,
	 *       `print()` - пустую строку. Итог разбора проверять следует всегда
	 *
	 * @warning Наложение маски и префикса методом `impose()` **меняет сам объект**,
	 *          а не возвращает новое значение. Если исходный адрес ещё нужен, его
	 *          следует сохранить до наложения
	 *
			 * @note Копирование объекта разрешено и в кучу не ходит: буфер адреса
			 *       постоянного размера и лежит в самом объекте. Годится и заведение
			 *       копией, и присваивание, и хранение адресов в наборах, копирующих
			 *       свои элементы
			 *
			 * @warning Заведение копией и присваивание **переносят разное**: копия
			 *          повторяет объект целиком, вместе со строгим режимом разбора, а
			 *          присваивание переносит лишь сам адрес, его вид и зону IPv6 -
			 *          строгий режим получателя остаётся прежним. Так и задумано:
			 *          присваивание меняет содержимое уже настроенного объекта, и
			 *          настройки его чужим содержимым сбиваться не должны
	 *
	 * @par Пример: разбор адреса и проверка принадлежности сети
	 *
	 * \~english
	 * @brief Class for working with the network addresses
	 * @details Holds **one** address and is able to do everything with it: to parse it from a string,
	 *          to yield it back in different kinds, to compare it with another one, to impose a mask,
	 *          to check the belonging to a network or to a range. Inside the address lies as
	 *          a binary buffer of a constant capacity, and therefore the parsing and the checks
	 *          get by without addresses to the heap
	 *          The object works with three kinds of the record: IPv4, IPv6 and a hardware
	 *          address. The other values of `type_t` — `URL`, `FQDN`, `FS`, `NETV4`,
	 *          `NETV6` — serve only the recognition in `host()`: `parse()` will not undertake
	 *          to parse them
	 * @note The object holds a **state**, and almost all the methods work with an already
	 *       parsed address. While `parse()` has not worked out successfully, the buffer is empty, and
	 *       the yielding methods return zeroes, and not a sign of an error: `v4()` will give `0`,
	 *       `print()` — an empty string. The result of the parsing should always be checked
	 * @warning The imposition of a mask and of a prefix by the `impose()` method **changes the object itself**,
	 *          and does not return a new value. If the original address is still needed, it
	 *          should be saved before the imposition
	 * @note The copying of the object is allowed and does not go to the heap: the buffer of the address is
	 *       of a constant size and lies in the object itself. Both the starting by
	 *       a copy is fit, and the assignment, and the holding of the addresses in the sets copying
	 *       their elements
	 * @warning The starting by a copy and the assignment **carry different things**: a copy
	 *          repeats the object entirely, together with the strict mode of the parsing, and
	 *          the assignment carries only the address itself, its kind and the zone of IPv6 —
	 *          the strict mode of the receiver remains the previous one. It is intended so:
	 *          the assignment changes the content of an already set up object, and
	 *          its settings must not be knocked down by a foreign content
	 * @par Example: the parsing of an address and the check of the belonging to a network
	 *
	 * \~
	 *
	 * @code{.cpp}
	 * awh::net_addr_t addr(&fmk, &log);
	 * // Разбираем адрес, вид записи определяется сам
	 * if(addr.parse("192.168.1.42")){
	 *     // Узнаём, откуда адрес: из локальной сети, из внешней или зарезервирован
	 *     if(addr.own() == awh::net_addr_t::own_t::LAN)
	 *         allow(addr.print());
	 *     // Заводим отдельный объект под сеть: impose меняет тот объект, к которому применён
	 *     awh::net_addr_t network(addr);
	 *     // Обнуляем хостовую часть и получаем сеть 192.168.1.0
	 *     network.impose(24, awh::net_addr_t::addr_t::NETWORK);
	 * }
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Network_Address {
		public:
			/**
			 * \~russian
			 * @brief Режим дислокации IP-адреса
			 *
			 * @details Отвечает на вопрос, откуда адрес родом. `LAN` - частные
			 *          диапазоны, те самые `10/8`, `172.16/12`, `192.168/16` и их
			 *          собратья у IPv6. `SYS` - диапазоны служебные: петля,
			 *          многоадресная рассылка, документационные сети - словом,
			 *          выданные не хостам, а назначению. `WAN` - всё прочее, то есть
			 *          адрес, направленный во внешнюю сеть
			 *
			 * @note Разделение это следует RFC 6890: в `LAN` попадают лишь диапазоны,
			 *       отведённые под свободную раздачу внутри сети, - `10/8`,
			 *       `172.16/12`, `192.168/16`, локальные адреса связи и `fc00::/7`.
			 *       Всё прочее особое - петля, документационные сети, рассылка на
			 *       группу, служебные блоки - относится к `SYS`: эти адреса выданы
			 *       назначению, а не хостам, и встретить их в живой сети признак
			 *       неполадки, а не обычное дело
			 *
			 * @note Петля (`127.0.0.0/8`, `::1`) относится к `SYS`, а не к `LAN`:
			 *       адрес этот выдан назначению, как и прочие особые. Тому, кто строит
			 *       по этому разряду проверку доступа, стоит помнить, что обращения с
			 *       самой машины под `LAN` не подпадают и разрешать их следует
			 *       отдельно - либо принимать оба разряда сразу, как это делает сам
			 *       движок
			 *
			 * @note `WAN` выдаётся **по остаточному признаку**: не «адрес признан
			 *       внешним», а «ни в один известный диапазон не попал». Для видов
			 *       записи, у которых диапазонов не заведено вовсе - аппаратного
			 *       адреса, к примеру, - итогом тоже будет `WAN`, и опираться на
			 *       него без проверки `type()` не следует
			 *
			 * \~english
			 * @brief Mode of the dislocation of an IP address
			 * @details Answers the question where an address hails from. `LAN` — the private
			 *          ranges, those very `10/8`, `172.16/12`, `192.168/16` and their
			 *          brethren at IPv6. `SYS` — the service ranges: the loopback,
			 *          the multicast, the documentation networks — in a word,
			 *          given out not to the hosts, but to a purpose. `WAN` — everything else, that is
			 *          an address directed into the external network
			 * @note This division follows RFC 6890: into `LAN` fall only the ranges
			 *       given over to the free distribution inside a network — `10/8`,
			 *       `172.16/12`, `192.168/16`, the link-local addresses and `fc00::/7`.
			 *       Everything else special — the loopback, the documentation networks, the multicast
			 *       to a group, the service blocks — belongs to `SYS`: these addresses are given out
			 *       to a purpose, and not to the hosts, and to meet them in a living network is a sign
			 *       of a trouble, and not an ordinary thing
			 * @note The loopback (`127.0.0.0/8`, `::1`) belongs to `SYS`, and not to `LAN`:
			 *       that address is given out to a purpose, as the other special ones. The one who builds
			 *       a check of the access by this class is worth remembering that the addresses from
			 *       the machine itself do not fall under `LAN` and should be allowed
			 *       separately — or both classes should be accepted at once, as the engine itself
			 *       does
			 * @note `WAN` is given out **by the residual sign**: not «the address is recognized as
			 *       an external one», but «it has fallen into no known range». For the kinds
			 *       of the record for which no ranges are started at all — for a hardware
			 *       address, for example, — the result will also be `WAN`, and relying on
			 *       it without a check of `type()` is not advisable
			 *
			 * \~
			 */
			enum class own_t : uint8_t {
				NONE = 0x00, // Адрес не установлен
				LAN  = 0x01, // Адрес является локальным
				WAN  = 0x02, // Адрес является глобальным
				SYS  = 0x03  // Адрес является зарезервированным
			};
			/**
			 * \~russian
			 * @brief Составная часть IP-адреса
			 *
			 * @details Указывает `impose()`, какую половину адреса оставить. Маска
			 *          делит адрес надвое: старшая часть говорит о сети, младшая о
			 *          хосте внутри неё. `NETWORK` оставляет сетевую часть и зануляет
			 *          хостовую - так из `192.168.1.42/24` выходит `192.168.1.0`.
			 *          `HOST` поступает наоборот и оставляет `0.0.0.42`
			 *
			 * \~english
			 * @brief Constituent part of an IP address
			 * @details Specifies to `impose()` which half of the address should be left. The mask
			 *          divides the address in two: the higher part speaks about the network, the lower one about
			 *          the host inside it. `NETWORK` leaves the network part and zeroes
			 *          the host one — that is how `192.168.1.0` comes out of `192.168.1.42/24`.
			 *          `HOST` acts the other way round and leaves `0.0.0.42`
			 *
			 * \~
			 */
			enum class addr_t : uint8_t {
				NONE    = 0x00, // Адрес не установлен
				HOST    = 0x01, // Адрес хоста
				NETWORK = 0x02  // Адрес сети
			};
			/**
			 * \~russian
			 * @brief Порядок следования байт
			 *
			 * @details Указывает, в каком порядке выдавать и принимать байты адреса
			 *          в числовом виде. `LITTLE` - порядок, принятый на большинстве
			 *          машин: байты переносятся из буфера в число как есть, и число
			 *          это держит их в сетевом порядке - в том, в каком адрес идёт по
			 *          проводу и в каком его ждут системные вызовы. `BIG` - порядок от
			 *          старшего разряда к младшему: байты при переносе
			 *          переворачиваются, и число получает истинное численное значение
			 *          адреса, годное для сравнения и арифметики
			 *
			 * @warning Имена разрядов говорят о самом числе, а не о буфере адреса, и
			 *          на машине с обратным порядком байт значат противоположное
			 *          привычному: `sockaddr` заполняется через `LITTLE` - разряд этот
			 *          и стоит умолчанием у выдающих методов, - а сравнивать адреса
			 *          числами следует через `BIG`. На машине с прямым порядком байт
			 *          оба разряда дают одно и то же
			 *
			 * @note Обратная читаемость эта идёт от работы с сокетами: как обычное
			 *       число приходится явно переводить в сетевой порядок через `htons`,
			 *       так и здесь на машине с обратным порядком байт нужный разряд
			 *       приходится называть `LITTLE`. Одно и то же число, лежащее в памяти
			 *       и снятое из неё, - вещи разные, и разряд описывает второе:
			 *       перекладывать байты нужно ровно в тот порядок, каким машина числа
			 *       снимает
			 *
			 * @warning Значение `NONE` не означает «порядок по умолчанию»: методы
			 *          перекладывают байты выбором из двух известных порядков, и
			 *          `NONE` не подходит ни к одному, отчего адрес остаётся нулевым.
			 *          Передавать его выдающим и принимающим методам не следует
			 *
			 * \~english
			 * @brief Order of the following of the bytes
			 * @details Specifies in which order the bytes of the address should be yielded and taken
			 *          in the numeric form. `LITTLE` — the order accepted at most of the
			 *          machines: the bytes are carried from the buffer into the number as they are, and that number
			 *          holds them in the network order — in the one the address goes over
			 *          the wire in and in the one the system calls expect it. `BIG` — the order from
			 *          the higher digit to the lower one: the bytes at the carrying
			 *          are turned over, and the number receives the true numeric value
			 *          of the address, fit for the comparison and for the arithmetic
			 * @warning The names of the classes speak about the number itself, and not about the buffer of the address, and
			 *          on a machine with the reverse order of the bytes mean the opposite of
			 *          the customary: `sockaddr` is filled through `LITTLE` — that class
			 *          stands as the default at the yielding methods, — and the addresses should be compared
			 *          as numbers through `BIG`. On a machine with the direct order of the bytes
			 *          both classes give one and the same
			 * @note This reverse readability comes from the work with the sockets: as an ordinary
			 *       number has to be explicitly converted into the network order through `htons`,
			 *       so here on a machine with the reverse order of the bytes the needed class
			 *       has to be called `LITTLE`. One and the same number lying in the memory
			 *       and taken out of it are different things, and the class describes the second one:
			 *       the bytes need to be shifted exactly into the order the machine takes the numbers
			 *       out in
			 * @warning The `NONE` value does not mean «the order by default»: the methods
			 *          shift the bytes by a choice out of the two known orders, and
			 *          `NONE` suits neither of them, and therefore the address remains a zero one.
			 *          Passing it to the yielding and to the taking methods is not advisable
			 *
			 * \~
			 */
			enum class endian_t : uint8_t {
				NONE   = 0x00, // Порядок следования байт не установлен
				BIG    = 0x01, // Порядок байт от старшего к младшему
				LITTLE = 0x02  // Порядок байт от младшего к старшему
			};
			/**
			 * \~russian
			 * @brief Размер формата IP-адреса
			 *
			 * @details Задаёт, насколько подробно записывать адрес. `SHORT` - привычная
			 *          сокращённая запись, та, которой пользуются люди. `LONG` -
			 *          развёрнутая, с ведущими нулями и без стяжки нулевых групп: в
			 *          такой записи все адреса одной длины, отчего она удобна для
			 *          сортировки как строк и для колонок в выводе. `MIDDLE` -
			 *          промежуточная: у IPv6 группы выписываются все, но без ведущих
			 *          нулей
			 *
			 * @note `NONE` не отменяет форматирование, а выбирает вид по умолчанию,
			 *       свой для каждой разновидности адреса
			 *
			 * \~english
			 * @brief Size of the format of an IP address
			 * @details Sets how detailed the address should be written. `SHORT` — the customary
			 *          abbreviated record, the one the people use. `LONG` —
			 *          the unfolded one, with the leading zeroes and without the contraction of the zero groups: in
			 *          such a record all the addresses are of one length, and therefore it is convenient for
			 *          the sorting as strings and for the columns in the output. `MIDDLE` —
			 *          the intermediate one: at IPv6 all the groups are written out, but without the leading
			 *          zeroes
			 * @note `NONE` does not cancel the formatting, but chooses the kind by default,
			 *       its own for every variety of the address
			 *
			 * \~
			 */
			enum class format_size_t : uint8_t {
				NONE   = 0x00, // Размер формата не установлен
				LONG   = 0x01, // Полный формат IP-адреса [0000:0000:0000:0000:0000:0000:ae21:ad12 / 192.168.000.001]
				SHORT  = 0x02, // Короткий формат IP-адреса [::ae21:ad12 / 192.168.0.1]
				MIDDLE = 0x03  // Средний формат IP-адреса [0:0:0:0:0:0:ae21:ad12 / 192.168.0.1]
			};
			/**
			 * \~russian
			 * @brief Флаги форматирования IP-адреса
			 *
			 * @details Задаёт систему счисления записи и то, как поступать со
			 *          встроенным адресом. `HEX`, `OCTAL` и `DECIMAL` выбирают
			 *          основание, в котором выписываются части адреса
			 *
			 *          Два последних значения ведают не основанием, а стыком версий.
			 *          IPv6 умеет нести в себе адрес IPv4 - четыре младших байта
			 *          записываются тогда привычными четырьмя числами через точку,
			 *          `::FFFF:192.168.1.1`. `HEX_IPV4` требует такой записи,
			 *          `HEX_IPV6` её запрещает и выводит адрес целиком
			 *          шестнадцатеричным. При `NONE` встроенный адрес распознаётся и
			 *          выписывается сам
			 *
			 * @note Разделитель у `print()` задаётся отдельным доводом и на выбор
			 *       здесь не влияет. Нулевой разделитель убирает его вовсе, отчего
			 *       запись становится сплошной - вид, удобный ключом отображения, но
			 *       не для чтения
			 *
			 * \~english
			 * @brief Flags of the formatting of an IP address
			 * @details Sets the numeral system of the record and how the
			 *          embedded address should be dealt with. `HEX`, `OCTAL` and `DECIMAL` choose
			 *          the base in which the parts of the address are written out
			 *          The last two values are in charge not of the base, but of the junction of the versions.
			 *          IPv6 is able to carry an IPv4 address inside itself — the four lower bytes
			 *          are written then by the customary four numbers through a dot,
			 *          `::FFFF:192.168.1.1`. `HEX_IPV4` demands such a record,
			 *          `HEX_IPV6` forbids it and yields the address entirely
			 *          in hexadecimal. At `NONE` an embedded address is recognized and
			 *          written out by itself
			 * @note The separator at `print()` is set by a separate argument and does not influence the choice
			 *       here. A zero separator removes it altogether, and therefore the
			 *       record becomes a continuous one — a kind convenient as a key of a mapping, but
			 *       not for the reading
			 *
			 * \~
			 */
			enum class format_flag_t : uint8_t {
				NONE      = 0x00, // Флаг не установлен
				HEX       = 0x01, // Шестнадцатеричный формат
				OCTAL     = 0x02, // Восьмеричный формат
				DECIMAL   = 0x03, // Десятичный формат
				HEX_IPV4  = 0x04, // Шестнадцатеричный формат IPv4
				HEX_IPV6  = 0x05  // Шестнадцатеричный формат IPv6
			};
			/**
			 * \~russian
			 * @brief Идентификаторы разновидностей адресов
			 *
			 * @details Набор этот шире того, что объект умеет хранить, и делится
			 *          надвое. Хранимых видов три - `IPV4`, `IPV6` и `MAC`: только их
			 *          принимает `parse()`, и только с ними работают сравнение,
			 *          наложение маски и вывод. Остальные - `URL`, `FQDN`, `FS`,
			 *          `NETV4`, `NETV6` - существуют ради `host()`, который отвечает
			 *          на вопрос «что это за запись», ничего никуда не сохраняя
			 *
			 * @note `NETV4` и `NETV6` обозначают запись сети целиком, с префиксом или
			 *       маской (`192.168.1.0/24`), а не адрес хоста. Разобрать такую
			 *       запись `parse()` не возьмётся: сеть задаётся адресом и отдельно
			 *       наложенным префиксом через `impose()`
			 *
			 * \~english
			 * @brief Identifiers of the varieties of the addresses
			 * @details This set is wider than what the object is able to hold, and is divided
			 *          in two. The held kinds are three — `IPV4`, `IPV6` and `MAC`: only them
			 *          `parse()` takes, and only with them the comparison,
			 *          the imposition of a mask and the output work. The others — `URL`, `FQDN`, `FS`,
			 *          `NETV4`, `NETV6` — exist for the sake of `host()`, which answers
			 *          the question «what kind of a record is this», saving nothing anywhere
			 * @note `NETV4` and `NETV6` designate a record of a network entirely, with a prefix or
			 *       a mask (`192.168.1.0/24`), and not an address of a host. `parse()` will not undertake to parse such
			 *       a record: a network is set by an address and by a separately
			 *       imposed prefix through `impose()`
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE  = 0x00, // Не определено
				FS    = 0x01, // Адрес в файловой системе
				MAC   = 0x02, // Аппаратный адрес сетевого интерфейса
				URL   = 0x03, // URL-адрес
				IPV4  = 0x04, // Адрес подключения IPv4
				IPV6  = 0x05, // Адрес подключения IPv6
				FQDN  = 0x06, // Доменная зона
				NETV4 = 0x07, // Адрес/Маска сети
				NETV6 = 0x08  // Адрес/Маска сети
			};
		private:
						/**
						 * \~russian
			 * @brief Структура бинарного буфера адреса постоянной ёмкости
			 *
			 * @details Буфер держит адрес в двоичном виде, и длина его известна
			 *          заранее: четыре байта у IPv4, шесть у аппаратного адреса,
			 *          шестнадцать у IPv6. Динамический массив на этом месте
			 *          заводил выделение памяти на каждый разобранный адрес, а
			 *          проверка принадлежности сети создаёт временный объект
			 *          адреса на каждый вызов, и выделение приходилось на каждую
			 *          проверку. Постоянный буфер снимает и то, и другое, а объект
			 *          адреса становится копируемым без обращений к куче.
			 *
			 * @note Набор методов повторяет ту часть работы с динамическим массивом,
			 *       которой модуль пользовался, чтобы замена оставалась подстановкой,
			 *       а не переписыванием мест обращения к буферу
						 *
						 * \~english
						 * @brief Structure of the binary buffer of an address of a constant capacity
						 * @details The buffer holds the address in the binary form, and its length is known
						 *          in advance: four bytes at IPv4, six at a hardware address,
						 *          sixteen at IPv6. A dynamic array in this place
						 *          started an allocation of the memory at every parsed address, and
						 *          the check of the belonging to a network creates a temporary object
						 *          of an address at every call, and an allocation fell on every
						 *          check. A constant buffer removes both, and the object
						 *          of an address becomes copyable without addresses to the heap.
						 * @note The set of the methods repeats that part of the work with a dynamic array,
						 *       which the module used, so that the replacement would remain a substitution,
						 *       and not a rewriting of the places of the address to the buffer
						 *
						 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Buffer {
				public:
					// Предельный размер бинарного буфера адреса
					static constexpr size_t CAPACITY = 16;
				private:
					// Количество занятых байт буфера
					size_t _size;
					// Байты бинарного буфера адреса
					uint8_t _data[CAPACITY];
				public:
					/**
					 * \~russian
					 * @brief Метод проверки заполненности буфера
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking the filledness of the buffer
					 * @return result of the check
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения размера буфера
					 *
					 * @return размер буфера
					 *
					 * \~english
					 * @brief Method of getting the size of the buffer
					 * @return size of the buffer
					 *
					 * \~
					 */
					size_t size() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод очистки буфера
					 *
					 * \~english
					 * @brief Method of clearing the buffer
					 *
					 * \~
					 */
					void clear() noexcept;
					/**
					 * \~russian
					 * @brief Метод изменения размера буфера
					 *
					 * @param size  новый размер буфера
					 * @param value значение заполнения добавленных байт
					 *
					 * @note Размер сверх ёмкости обрезается: адреса длиннее
					 *       шестнадцати байт не бывает, и запрос такого размера
					 *       означал бы ошибку вызывающей стороны
					 *
					 * \~english
					 * @brief Method of changing the size of the buffer
					 * @param size  new size of the buffer
					 * @param value value of the filling of the added bytes
					 * @note A size beyond the capacity is cut off: an address longer than
					 *       sixteen bytes does not happen, and a request of such a size
					 *       would mean an error of the calling side
					 *
					 * \~
					 */
					void resize(const size_t size, const uint8_t value = 0) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения указателя на данные буфера
					 *
					 * @return указатель на данные буфера
					 *
					 * \~english
					 * @brief Method of getting the pointer to the data of the buffer
					 * @return pointer to the data of the buffer
					 *
					 * \~
					 */
					uint8_t * data() noexcept;
					/**
					 * \~russian
					 * @brief Метод получения указателя на данные буфера
					 *
					 * @return указатель на данные буфера
					 *
					 * \~english
					 * @brief Method of getting the pointer to the data of the buffer
					 * @return pointer to the data of the buffer
					 *
					 * \~
					 */
					const uint8_t * data() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор получения байта буфера по индексу
					 *
					 * @param index индекс байта буфера
					 * @return      байт буфера
					 *
					 * \~english
					 * @brief Operator of getting a byte of the buffer by an index
					 * @param index index of the byte of the buffer
					 * @return      byte of the buffer
					 *
					 * \~
					 */
					uint8_t & operator [] (const size_t index) noexcept;
					/**
					 * \~russian
					 * @brief Оператор получения байта буфера по индексу
					 *
					 * @param index индекс байта буфера
					 * @return      байт буфера
					 *
					 * \~english
					 * @brief Operator of getting a byte of the buffer by an index
					 * @param index index of the byte of the buffer
					 * @return      byte of the buffer
					 *
					 * \~
					 */
					const uint8_t & operator [] (const size_t index) const noexcept;
				public:
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
					explicit Buffer() noexcept;
			} buffer_t;
		private:
			// Тип обрабатываемого адреса
			type_t _type;
		private:
			// Флаг строгого режима парсинга/проверки адресов
			bool _strict;
		private:
			// Зона IPv6 адреса
			string _zone;
		private:
			// Бинарный буфер данных
			buffer_t _buffer;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
					public:
			/**
			 * \~russian
			 * @brief Метод очистки данных IP-адреса
			 *
			 * @details Опустошает объект: сбрасываются сам адрес, пометка о его виде
			 *          и зона IPv6
			 *
			 * @note Настройки объекта очистка не трогает - строгий режим остаётся
			 *       выставленным, и объект годится для повторного разбора без
			 *       перенастройки
			 *
			 * \~english
			 * @brief Method of clearing the data of an IP address
			 * @details Empties the object: the address itself, the mark about its kind
			 *          and the zone of IPv6 are reset
			 * @note The clearing does not touch the settings of the object — the strict mode remains
			 *       set out, and the object is fit for a repeated parsing without
			 *       a resetting
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки соответствия адреса зеркалу IPv6 => IPv4
			 *
			 * @details Отвечает, несёт ли хранимый адрес IPv6 внутри себя адрес IPv4.
			 *          Такие адреса заведены ради совместимости: узел, работающий по
			 *          IPv6, получает подключения от узлов IPv4 в виде
			 *          `::FFFF:192.168.1.1`, где младшие четыре байта и есть исходный
			 *          адрес
			 *
			 *          Пригождается на приёме подключений: сервер, слушающий по IPv6,
			 *          видит адреса клиентов IPv4 именно в этом виде, и записывать их
			 *          в журнал или сверять со списками разумнее в исходном
			 *
			 * @note Проверяется только вложение с приметой `FFFF`. Прочие способы
			 *       уложить адрес IPv4 в IPv6 - те, что стандартами объявлены
			 *       устаревшими, - за зеркало не считаются
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an address to the IPv6 => IPv4 mapping
			 * @details Answers whether the held IPv6 address carries an IPv4 address inside itself.
			 *          Such addresses are started for the sake of the compatibility: a node working over
			 *          IPv6 receives the connections from the IPv4 nodes in the form
			 *          `::FFFF:192.168.1.1`, where the lower four bytes are the original
			 *          address
			 *          Comes in handy at the acceptance of the connections: a server listening over IPv6,
			 *          sees the addresses of the IPv4 clients exactly in this form, and writing them
			 *          into a log or checking them against the lists is more reasonable in the original one
			 * @note Only the embedding with the `FFFF` marker is checked. The other ways
			 *       of laying an IPv4 address into an IPv6 one — those declared obsolete
			 *       by the standards — are not considered a mapping
			 * @return result of the check
			 *
			 * \~
			 */
			bool broadcastIPv6ToIPv4() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения зоны IPv6 адреса
			 *
			 * @details Зона - это хвост адреса IPv6 после знака процента,
			 *          `fe80::1%en0`. Нужна она адресам локальной связи: такие адреса
			 *          неоднозначны сами по себе и обретают смысл лишь вместе с
			 *          сетевым устройством, через которое достижимы. Заполняется зона
			 *          при разборе строки и хранится отдельно от самого адреса
			 *
			 * @note Зона в сравнении адресов не участвует: два адреса локальной связи
			 *       с разными зонами будут признаны равными. Если различать их важно,
			 *       зону следует сверить отдельно
			 *
			 * @note Обозначается зона именем устройства либо его номером, и пустой она
			 *       не бывает: запись `fe80::1%` разбору не поддаётся (RFC 4007 6).
			 *       Экранированный знак `%25` снимается только у записи в квадратных
			 *       скобках - экранирование это заведено для записи адреса внутри
			 *       строки запроса (RFC 6874), а вне её знак означает сам себя, и
			 *       запись `fe80::1%25` даёт зону с номером 25
			 *
			 * @note Зона принадлежит содержимому объекта и снимается вместе с ним:
			 *       разбор, очистка и установка адреса в чистом виде её сбрасывают.
			 *       Пережить их она не может даже при неудаче разбора
			 *
			 * @return зона IPv6 адреса
			 *
			 * \~english
			 * @brief Method of extracting the zone of an IPv6 address
			 * @details The zone is the tail of an IPv6 address after the percent sign,
			 *          `fe80::1%en0`. It is needed by the link-local addresses: such addresses
			 *          are ambiguous by themselves and acquire a meaning only together with the
			 *          network device through which they are reachable. The zone is filled
			 *          at the parsing of a string and is held separately from the address itself
			 * @note The zone does not participate in the comparison of the addresses: two link-local addresses
			 *       with different zones will be recognized as equal. If telling them apart is important,
			 *       the zone should be checked separately
			 * @note The zone is designated by the name of a device or by its number, and it does not happen
			 *       to be an empty one: the record `fe80::1%` does not yield to the parsing (RFC 4007 6).
			 *       The escaped sign `%25` is removed only at a record in the square
			 *       brackets — that escaping is started for a record of an address inside
			 *       the string of a request (RFC 6874), and outside it the sign means itself, and
			 *       the record `fe80::1%25` gives the zone with the number 25
			 * @note The zone belongs to the content of the object and is removed together with it:
			 *       the parsing, the clearing and the setting of the address in a clean form reset it.
			 *       It cannot outlive them even at a failure of the parsing
			 * @return zone of the IPv6 address
			 *
			 * \~
			 */
			const string & zone() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки зоны IPv6 адреса
			 *
			 * @param zone зона IPv6 адреса для установки
			 *
			 * \~english
			 * @brief Method of setting the zone of an IPv6 address
			 * @param zone zone of the IPv6 address to set
			 *
			 * \~
			 */
			void zone(string_view zone) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения типа IP-адреса
			 *
			 * @return тип IP-адреса
			 *
			 * \~english
			 * @brief Method of extracting the type of an IP address
			 * @return type of the IP address
			 *
			 * \~
			 */
			type_t type() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки типа IP-адреса
			 *
			 * @warning Метод меняет **только пометку** о виде адреса, не трогая самих
			 *          данных и не проверяя, отвечает ли новый вид тому, что хранится.
			 *          Выставленный поверх заполненного объекта чужой вид приведёт к
			 *          тому, что сравнение и вывод станут читать буфер не так, как он
			 *          заполнялся. Разбирать адрес заведомо известного вида следует
			 *          через `parse()` с указанием вида, а не установкой вида отдельно
			 *
			 * @param type тип IP-адреса для установки
			 *
			 * \~english
			 * @brief Method of setting the type of an IP address
			 * @warning The method changes **only the mark** about the kind of the address, without touching the
			 *          data itself and without checking whether the new kind answers to what is held.
			 *          A foreign kind set out over a filled object will lead to
			 *          the comparison and the output reading the buffer not the way it was
			 *          filled. An address of a knowingly known kind should be parsed
			 *          through `parse()` with the kind specified, and not by a separate setting of the kind
			 * @param type type of the IP address to set
			 *
			 * \~
			 */
			void type(const type_t type) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения флага строгого режима парсинга/проверки адресов
			 *
			 * @details Строгий режим отсекает у IPv4 вольные записи, доставшиеся в
			 *          наследство от старых разборщиков: сокращённые формы `a.b.c`,
			 *          `a.b`, `a`, а также восьмеричную и шестнадцатеричную запись
			 *          частей. Записи эти обозначают настоящие адреса, но обозначают
			 *          их неочевидно - `010.1` и `10.1` окажутся разными адресами, -
			 *          отчего годятся на обход проверок по спискам
			 *
			 * @note Режим этот выключен по умолчанию: разбор принимает всё, что
			 *       принимают системные средства. Включать его стоит там, где адрес
			 *       приходит извне и сверяется со списками разрешённых
			 *
			 * @note Ведущий ноль и в нестрогом режиме объявляет часть восьмеричной
			 *       безоговорочно: запись `09` отвергается, а не перечитывается
			 *       десятичной, - ровно так же поступает `inet_aton`. Закреплено
			 *       сличением с системным разборщиком в `NetSystemDifferentialAcceptanceTest`
			 *
			 * @return флаг строгого режима
			 *
			 * \~english
			 * @brief Method of extracting the flag of the strict mode of the parsing/of the check of the addresses
			 * @details The strict mode cuts off at IPv4 the loose records inherited
			 *          from the old parsers: the abbreviated forms `a.b.c`,
			 *          `a.b`, `a`, as well as the octal and the hexadecimal record of the
			 *          parts. These records designate real addresses, but designate
			 *          them non-obviously — `010.1` and `10.1` will turn out to be different addresses, —
			 *          and therefore are fit for a bypassing of the checks by the lists
			 * @note This mode is switched off by default: the parsing accepts everything that
			 *       the system means accept. Switching it on is worthwhile where an address
			 *       comes from the outside and is checked against the lists of the allowed ones
			 * @note A leading zero in the non-strict mode as well declares a part an octal one
			 *       unconditionally: the record `09` is rejected, and is not reread as
			 *       a decimal one, — exactly so does `inet_aton`. Fixed by
			 *       a matching with the system parser in `NetSystemDifferentialAcceptanceTest`
			 * @return flag of the strict mode
			 *
			 * \~
			 */
			bool strict() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки строгого режима парсинга/проверки адресов
			 *
			 * @param mode флаг строгого режима (в строгом режиме для IPv4 запрещены legacy-формы
			 *             [a.b.c, a.b, a] и не-десятичные системы счисления [0x..., 0...])
			 *
			 * \~english
			 * @brief Method of setting the strict mode of the parsing/of the check of the addresses
			 * @param mode flag of the strict mode (in the strict mode for IPv4 the legacy forms
			 *             [a.b.c, a.b, a] and the non-decimal numeral systems [0x..., 0...] are forbidden)
			 *
			 * \~
			 */
			void strict(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения типа хоста
			 *
			 * @details Отвечает на вопрос, чем является строка, ничего никуда не
			 *          сохраняя: состояния объекта метод не меняет и разобранного
			 *          адреса после себя не оставляет. Пригождается там, где вид
			 *          записи заранее неизвестен - в настройках, где на месте узла
			 *          стоит то адрес, то имя, то путь к сокету
			 *
			 *          Распознаёт метод весь набор `type_t`, а не только хранимые три
			 *          вида: доменное имя, URL-адрес и путь файловой системы он
			 *          отличит, хотя разобрать их и не сможет
			 *
			 * @note Разбирается запись по первому подошедшему виду, а порядок перебора
			 *       ведёт от строгих видов к вольным - доменное имя стоит последним.
			 *       Неоднозначные записи потому относятся к более строгому виду
			 *
			 * @param host хост для определения
			 * @return     определённый тип хоста
			 *
			 * \~english
			 * @brief Method of the determination of the type of a host
			 * @details Answers the question what a string is, saving nothing
			 *          anywhere: the method does not change the state of the object and leaves no parsed
			 *          address after itself. Comes in handy where the kind of the
			 *          record is unknown in advance — in the settings, where in the place of a node
			 *          there stands now an address, now a name, now a path to a socket
			 *          The method recognizes the whole set of `type_t`, and not only the three held
			 *          kinds: it will tell a domain name, a URL address and a path of the file system
			 *          apart, although it will not be able to parse them
			 * @note A record is parsed by the first suitable kind, and the order of the traversal
			 *       leads from the strict kinds to the loose ones — a domain name stands last.
			 *       The ambiguous records therefore belong to the stricter kind
			 * @param host host to determine
			 * @return     the determined type of the host
			 *
			 * \~
			 */
			type_t host(string_view host) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения аппаратного адреса в чистом виде
			 *
			 * @return аппаратный адрес в чистом виде
			 *
			 * \~english
			 * @brief Method of extracting a hardware address in the clean form
			 * @return hardware address in the clean form
			 *
			 * \~
			 */
			array <uint8_t, 6> mac() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки аппаратного адреса в чистом виде
			 *
			 * @param addr аппаратный адрес в чистом виде
			 *
			 * \~english
			 * @brief Method of setting a hardware address in the clean form
			 * @param addr hardware address in the clean form
			 *
			 * \~
			 */
			void mac(const array <uint8_t, 6> & addr) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения адреса IPv4 в чистом виде
			 *
			 * @details Отдаёт адрес числом - в том виде, в каком его принимают
			 *          системные вызовы и структуры сокетов. Порядок байт выбирается
			 *          доводом: сетевой порядок нужен при передаче наружу, машинный -
			 *          при вычислениях над адресом
			 *
			 * @details Объект, хранящий адрес IPv6, отдаёт вложенный в него адрес IPv4:
			 *          стандарт (RFC 4291) определяет зеркало `::FFFF:a.b.c.d` и
			 *          устаревшую совместимую форму `::a.b.c.d`, и в обеих адрес IPv4
			 *          занимает последние четыре октета. Прочие адреса IPv6 в IPv4 не
			 *          переводятся никак - такого преобразования стандарт не определяет
			 *
			 * @warning Нуль возвращается **и как ошибка, и как значение**. Если объект
			 *          пуст либо хранит адрес IPv6 без вложенного в него IPv4, итогом
			 *          будет нуль - тот же, что у настоящего адреса `0.0.0.0`. Различать
			 *          их следует по `type()`: у пустого объекта он даёт `NONE`
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес IPv4 в чистом виде
			 *
			 * \~english
			 * @brief Method of extracting an IPv4 address in the clean form
			 * @details Gives back the address as a number — in the form in which the
			 *          system calls and the structures of the sockets take it. The order of the bytes is chosen by
			 *          an argument: the network order is needed at the transmission outwards, the machine one —
			 *          at the computations over the address
			 * @details An object holding an IPv6 address gives back the IPv4 address embedded in it:
			 *          the standard (RFC 4291) defines the mapping `::FFFF:a.b.c.d` and
			 *          the obsolete compatible form `::a.b.c.d`, and in both the IPv4 address
			 *          occupies the last four octets. The other IPv6 addresses are not converted into IPv4
			 *          in any way — the standard defines no such conversion
			 * @warning Zero is returned **both as an error and as a value**. If the object
			 *          is empty or holds an IPv6 address without an IPv4 embedded in it, the result
			 *          will be zero — the same as at the real address `0.0.0.0`. They should be told
			 *          apart by `type()`: at an empty object it gives `NONE`
			 * @param endian flag of the building of the address in the set order of the following of the bytes
			 * @return       IPv4 address in the clean form
			 *
			 * \~
			 */
			uint32_t v4(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса IPv4 в чистом виде
			 *
			 * @details Заполняет объект числовым адресом, минуя разбор строки. Вид
			 *          адреса при этом выставляется в `IPV4` сам, поэтому вызывать
			 *          `type()` отдельно не требуется
			 *
			 * @param addr   адрес IPv4 в чистом виде
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 *
			 * \~english
			 * @brief Method of setting an IPv4 address in the clean form
			 * @details Fills the object with a numeric address, bypassing the parsing of a string. The kind
			 *          of the address is at that set out to `IPV4` by itself, and therefore calling
			 *          `type()` separately is not required
			 * @param addr   IPv4 address in the clean form
			 * @param endian flag of the building of the address in the set order of the following of the bytes
			 *
			 * \~
			 */
			void v4(const uint32_t addr, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения адреса IPv6 в чистом виде
			 *
			 * @details Объект, хранящий адрес IPv4, отдаёт его вложенным в адрес IPv6
			 *          зеркалом `::FFFF:a.b.c.d` (RFC 4291, §2.5.5.2). Перевод в эту
			 *          сторону возможен **всегда** и однозначен, в отличие от обратного,
			 *          определённого лишь для вложенных форм
			 *
			 * @note Извлечение числом отвечает согласно с выводом строкой: `print()`
			 *       выводит для такого объекта то же самое зеркало
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес IPv6 в чистом виде
			 *
			 * \~english
			 * @brief Method of extracting an IPv6 address in the clean form
			 * @details An object holding an IPv4 address gives it back embedded into an IPv6 address
			 *          as the mapping `::FFFF:a.b.c.d` (RFC 4291, §2.5.5.2). The conversion in this
			 *          direction is possible **always** and is unambiguous, unlike the reverse one,
			 *          defined only for the embedded forms
			 * @note The extraction as a number answers in agreement with the output as a string: `print()`
			 *       yields for such an object the very same mapping
			 * @param endian flag of the building of the address in the set order of the following of the bytes
			 * @return       IPv6 address in the clean form
			 *
			 * \~
			 */
			array <uint8_t, 16> v6(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса IPv6 в чистом виде
			 *
			 * @param addr   адрес IPv6 в чистом виде
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 *
			 * \~english
			 * @brief Method of setting an IPv6 address in the clean form
			 * @param addr   IPv6 address in the clean form
			 * @param endian flag of the building of the address in the set order of the following of the bytes
			 *
			 * \~
			 */
			void v6(const array <uint8_t, 16> & addr, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения адреса в чистом виде
			 *
			 * @details Отдаёт адрес разбором по видам: возвращается объект той
			 *          разновидности, какая хранится, - `addr_mac_t`, `addr_net_ipv4_t`
			 *          или `addr_net_ipv6_t`. Пригождается там, где вид адреса заранее
			 *          неизвестен и разбирать его вызовом `v4()` или `v6()` пришлось бы
			 *          через проверку `type()`
			 *
			 * @note Вид определяется **длиной** хранимого адреса, а не полем `type()`.
			 *       Расходятся они лишь тогда, когда вид выставляли вручную поверх уже
			 *       заполненного объекта
			 *
			 * @note Пустой объект даёт пустой указатель, и проверять итог следует
			 *       всегда
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес в чистом виде
			 *
			 * \~english
			 * @brief Method of extracting an address in the clean form
			 * @details Gives back the address by a resolution over the kinds: an object of the
			 *          variety that is held is returned — `addr_mac_t`, `addr_net_ipv4_t`
			 *          or `addr_net_ipv6_t`. Comes in handy where the kind of the address is unknown
			 *          in advance and parsing it by a call of `v4()` or `v6()` would have to be done
			 *          through a check of `type()`
			 * @note The kind is determined by the **length** of the held address, and not by the `type()` field.
			 *       They diverge only when the kind was set out by hand over an already
			 *       filled object
			 * @note An empty object gives a null pointer, and the result should always be
			 *       checked
			 * @param endian flag of the building of the address in the set order of the following of the bytes
			 * @return       address in the clean form
			 *
			 * \~
			 */
			unique_ptr <net::addr_t> source(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса в чистом виде
			 *
			 * @param value  адрес в чистом виде для установки
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 *
			 * \~english
			 * @brief Method of setting an address in the clean form
			 * @param value  address in the clean form to set
			 * @param endian flag of the building of the address in the set order of the following of the bytes
			 *
			 * \~
			 */
			void source(const net::addr_t * value, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки валидности IP-адреса
			 *
			 * @details Проверяет, годится ли строка на роль записи указанного вида, не
			 *          заполняя объект и не меняя его состояния. Отличается от
			 *          `parse()` тем, что вид задаётся, а не определяется, и от
			 *          `host()` тем, что проверяется одна разновидность, а не
			 *          перебираются все
			 *
			 * @note Строгий режим, выставляемый `strict()`, на проверку влияет: с ним
			 *       у IPv4 отвергаются сокращённые записи вроде `10.1` и записи не в
			 *       десятичной системе
			 *
			 * @note Разделители аппаратного адреса **намеренно допускаются
			 *       вперемешку**: годны и `AA:BB:CC:DD:EE:FF`, и `AA-BB-CC-DD-EE-FF`,
			 *       и запись из двенадцати разрядов подряд, и любое их сочетание.
			 *       Записи эти приходят от разных источников - от системных средств,
			 *       из настроек, из журналов, - и разбирать их порознь пришлось бы
			 *       вызывающей стороне. Разбор `parse()` допускает ровно то же самое:
			 *       расхождение между проверкой и разбором было дефектом и устранено
			 *
			 * @param addr адрес аппаратный или интернет подключения для проверки
			 * @param type тип адреса аппаратного или интернет подключения для проверки
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking the validity of an IP address
			 * @details Checks whether a string is fit for the role of a record of the specified kind, without
			 *          filling the object and without changing its state. Differs from
			 *          `parse()` in that the kind is set, and not determined, and from
			 *          `host()` in that one variety is checked, and not
			 *          all of them are traversed
			 * @note The strict mode, set out by `strict()`, influences the check: with it
			 *       at IPv4 the abbreviated records like `10.1` and the records not in
			 *       the decimal system are rejected
			 * @note The separators of a hardware address are **deliberately allowed
			 *       mixed together**: both `AA:BB:CC:DD:EE:FF` is fit, and `AA-BB-CC-DD-EE-FF`,
			 *       and a record of twelve digits in a row, and any combination of them.
			 *       These records come from different sources — from the system means,
			 *       from the settings, from the logs, — and the calling side would have to parse them
			 *       separately. The `parse()` parsing allows exactly the same:
			 *       the divergence between the check and the parsing was a defect and is eliminated
			 * @param addr address, a hardware one or of an internet connection, to check
			 * @param type type of the address, a hardware one or of an internet connection, to check
			 * @return     result of the check
			 *
			 * \~
			 */
			bool check(const string_view addr, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод наложения маски сети
			 *
			 * @details Делит адрес маской надвое и оставляет ту половину, которую
			 *          просят: `NETWORK` - сетевую, `HOST` - хостовую, - а вторую
			 *          зануляет. Так из адреса получают сеть, к которой он относится,
			 *          и так же сравнивают два адреса на принадлежность одной сети
			 *
			 *          Есть четыре разновидности метода: сеть задаётся либо маской,
			 *          либо длиной префикса, а вид адреса либо берётся из объекта,
			 *          либо указывается явно. Явное указание нужно тогда, когда объект
			 *          ещё не разобран или заполнялся числовым видом в обход `parse()`
			 *
			 * @warning Метод **меняет сам объект**, а не возвращает новое значение.
			 *          Прежний адрес после наложения не восстановить: если он ещё
			 *          нужен, объект следует скопировать заранее
			 *
			 * @note Нулевая длина префикса не занимает адрес целиком, а не делает
			 *       ничего: наложение при ней пропускается. Длина сверх разрядности
			 *       адреса - свыше 32 у IPv4 и свыше 128 у IPv6 - тоже оставляет
			 *       объект нетронутым
			 *
			 * @note Нуль **намеренно означает и отказ, и настоящую длину префикса**
			 *       сети `/0` - как здесь, так и у `mask2Prefix()` с `prefix2Mask()`.
			 *       Развести их можно лишь подписью, отдающей признак годности
			 *       отдельно от значения, а сеть `/0` в настройках не встречается:
			 *       она означает «любой адрес», и записывать её маской незачем. Тому,
			 *       кому различие нужно, следует проверять запись прежде перевода
			 *
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 *
			 * \~english
			 * @brief Method of the imposition of a mask of a network
			 * @details Divides the address in two by a mask and leaves that half which
			 *          is asked for: `NETWORK` — the network one, `HOST` — the host one, — and zeroes
			 *          the second one. That is how a network an address belongs to is obtained from it,
			 *          and that is how two addresses are compared for the belonging to one network
			 *          There are four varieties of the method: the network is set either by a mask,
			 *          or by the length of a prefix, and the kind of the address is either taken from the object,
			 *          or specified explicitly. An explicit specification is needed when the object
			 *          is not parsed yet or was filled by a numeric form bypassing `parse()`
			 * @warning The method **changes the object itself**, and does not return a new value.
			 *          The previous address cannot be restored after the imposition: if it is still
			 *          needed, the object should be copied in advance
			 * @note A zero length of a prefix does not occupy the address entirely, but does
			 *       nothing: the imposition at it is skipped. A length beyond the width of
			 *       the address — above 32 at IPv4 and above 128 at IPv6 — also leaves
			 *       the object untouched
			 * @note Zero **deliberately means both a refusal and a real length of the prefix**
			 *       of the network `/0` — both here, and at `mask2Prefix()` with `prefix2Mask()`.
			 *       They can be divided only by a signature giving back the sign of the fitness
			 *       separately from the value, and the network `/0` does not occur in the settings:
			 *       it means «any address», and there is no point in writing it as a mask. The one
			 *       who needs the difference should check the record before the conversion
			 * @param mask mask of the network to impose
			 * @param addr type of the obtained address
			 *
			 * \~
			 */
			void impose(string_view mask, const addr_t addr) noexcept;
			/**
			 * \~russian
			 * @brief Метод наложения маски сети
			 *
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 * @param type тип адреса аппаратного или интернет подключения
			 *
			 * \~english
			 * @brief Method of the imposition of a mask of a network
			 * @param mask mask of the network to impose
			 * @param addr type of the obtained address
			 * @param type type of the address, a hardware one or of an internet connection
			 *
			 * \~
			 */
			void impose(string_view mask, const addr_t addr, const type_t type) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод наложения префикса
			 *
			 * @param prefix префикс для наложения
			 * @param addr   тип получаемого адреса
			 *
			 * \~english
			 * @brief Method of the imposition of a prefix
			 * @param prefix prefix to impose
			 * @param addr   type of the obtained address
			 *
			 * \~
			 */
			void impose(const uint8_t prefix, const addr_t addr) noexcept;
			/**
			 * \~russian
			 * @brief Метод наложения префикса
			 *
			 * @param prefix префикс для наложения
			 * @param addr   тип получаемого адреса
			 * @param type   тип адреса аппаратного или интернет подключения
			 *
			 * \~english
			 * @brief Method of the imposition of a prefix
			 * @param prefix prefix to impose
			 * @param addr   type of the obtained address
			 * @param type   type of the address, a hardware one or of an internet connection
			 *
			 * \~
			 */
			void impose(const uint8_t prefix, const addr_t addr, const type_t type) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод перевода маски сети в префикс адреса
			 *
			 * @details Переводит запись сети из одного вида в другой: маска
			 *          `255.255.255.0` и префикс `24` говорят об одном и том же, но
			 *          первая привычна настройкам, а второй - записи сети через косую
			 *          черту. Обратный перевод делает `prefix2Mask()`
			 *
			 * @warning Маска обязана быть **сплошной**: единичные разряды идут подряд
			 *          от старшего, а за первым нулевым стоят одни нули. Запись вроде
			 *          `255.0.255.0` отвергается нулём, а не пересчитывается в
			 *          префикс `16` - иначе правило легло бы на чужую сеть
			 *
			 * @note Нуль отдаётся и при отказе, и как настоящая длина префикса нулевой
			 *       маски `0.0.0.0`. Различать эти случаи по одному лишь итогу нельзя
			 *
			 * @note Метод состояния объекта не меняет и с хранимым адресом не связан -
			 *       разобранный адрес для перевода не нужен
			 *
			 * @param mask маска сети для перевода
			 * @return     полученный префикс адреса
			 *
			 * \~english
			 * @brief Method of the conversion of a mask of a network into a prefix of an address
			 * @details Converts a record of a network from one kind into another: the mask
			 *          `255.255.255.0` and the prefix `24` speak about one and the same, but
			 *          the first one is customary to the settings, and the second one to a record of a network through a slash.
			 *          The reverse conversion is done by `prefix2Mask()`
			 * @warning A mask is obliged to be a **continuous** one: the one digits go in a row
			 *          from the higher one, and behind the first zero one there stand zeroes alone. A record like
			 *          `255.0.255.0` is rejected by zero, and is not recounted into
			 *          the prefix `16` — otherwise the rule would fall onto a foreign network
			 * @note Zero is given back both at a refusal, and as the real length of the prefix of the zero
			 *       mask `0.0.0.0`. Telling these cases apart by the result alone is not possible
			 * @note The method does not change the state of the object and is not connected with the held address —
			 *       a parsed address is not needed for the conversion
			 * @param mask mask of the network to convert
			 * @return     the obtained prefix of the address
			 *
			 * \~
			 */
			uint8_t mask2Prefix(string_view mask) const noexcept;
			/**
			 * \~russian
			 * @brief Метод перевода маски сети в префикс адреса
			 *
			 * @param mask маска сети для перевода
			 * @param type тип адреса аппаратного или интернет подключения
			 * @return     полученный префикс адреса
			 *
			 * \~english
			 * @brief Method of the conversion of a mask of a network into a prefix of an address
			 * @param mask mask of the network to convert
			 * @param type type of the address, a hardware one or of an internet connection
			 * @return     the obtained prefix of the address
			 *
			 * \~
			 */
			uint8_t mask2Prefix(string_view mask, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод преобразования префикса адреса в маску сети
			 *
			 * @details Перевод, обратный `mask2Prefix()`: из длины префикса получается
			 *          маска сети той разрядности, какая отвечает виду адреса
			 *
			 * @note Метод состояния объекта не меняет и с хранимым адресом не связан -
			 *       разобранный адрес для перевода не нужен
			 *
			 * @param prefix префикс адреса для преобразования
			 * @return       полученная маска сети
			 *
			 * \~english
			 * @brief Method of the conversion of a prefix of an address into a mask of a network
			 * @details The conversion reverse to `mask2Prefix()`: from the length of a prefix a mask
			 *          of a network of the width that answers the kind of the address is obtained
			 * @note The method does not change the state of the object and is not connected with the held address —
			 *       a parsed address is not needed for the conversion
			 * @param prefix prefix of the address to convert
			 * @return       the obtained mask of the network
			 *
			 * \~
			 */
			string prefix2Mask(const uint8_t prefix) const noexcept;
			/**
			 * \~russian
			 * @brief Метод преобразования префикса адреса в маску сети
			 *
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       полученная маска сети
			 *
			 * \~english
			 * @brief Method of the conversion of a prefix of an address into a mask of a network
			 * @param prefix prefix of the address to convert
			 * @param type   type of the address, a hardware one or of an internet connection
			 * @return       the obtained mask of the network
			 *
			 * \~
			 */
			string prefix2Mask(const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @details Проверяет, лежит ли хранимый адрес между двумя границами
			 *          включительно. Маска или префикс отсекает при этом **сетевую**
			 *          часть у всех трёх адресов - у обеих границ и у проверяемого, -
			 *          и сравниваются уже хостовые остатки. Так задаётся диапазон
			 *          внутри сети: старшие разряды, общие у всей сети, из сравнения
			 *          выпадают
			 *
			 *          Восемь разновидностей метода различаются лишь тем, как задаются
			 *          доводы: границы принимаются готовыми объектами адреса или
			 *          строками, сеть - маской или длиной префикса, вид адреса берётся
			 *          из объекта или указывается явно
			 *
			 * @note Границы разного вида, равно как и граница, чей вид расходится с
			 *       хранимым адресом, дают отрицательный итог, а не ошибку: сравнивать
			 *       адрес IPv4 с рубежами IPv6 бессмысленно
			 *
			 * @note Порядок границ важен: начало должно быть не больше конца.
			 *       Переставленные местами рубежи дадут пустой диапазон
			 *
			 * @warning Сама сеть в сравнении не участвует, поэтому адрес из **другой**
			 *          сети с подходящим хостовым остатком проверку пройдёт. Если
			 *          требуется ещё и совпадение сети, его следует проверить отдельно
			 *          - методом `mapping()`
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @details Checks whether the held address lies between two boundaries
			 *          inclusively. A mask or a prefix at that cuts off the **network**
			 *          part at all three addresses — at both boundaries and at the checked one, —
			 *          and the host remainders are already compared. That is how a range is set
			 *          inside a network: the higher digits common to the whole network fall out of
			 *          the comparison
			 *          The eight varieties of the method differ only by how the
			 *          arguments are set: the boundaries are taken as ready objects of an address or as
			 *          strings, the network — as a mask or as the length of a prefix, the kind of the address is taken
			 *          from the object or is specified explicitly
			 * @note The boundaries of a different kind, as well as a boundary whose kind diverges from
			 *       the held address, give a negative result, and not an error: comparing
			 *       an IPv4 address with the IPv6 boundaries is meaningless
			 * @note The order of the boundaries matters: the beginning must be not greater than the end.
			 *       The boundaries swapped around will give an empty range
			 * @warning The network itself does not participate in the comparison, and therefore an address from **another**
			 *          network with a suitable host remainder will pass the check. If
			 *          a coincidence of the network is required as well, it should be checked separately
			 *          — by the `mapping()` method
			 * @param begin beginning of the range of the addresses
			 * @param end   end of the range of the addresses
			 * @param mask  mask of the network to convert
			 * @return      result of the check
			 *
			 * \~
			 */
			bool range(const Network_Address & begin, const Network_Address & end, string_view mask) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @param type  тип адреса аппаратного или интернет подключения
			 * @return      результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin beginning of the range of the addresses
			 * @param end   end of the range of the addresses
			 * @param mask  mask of the network to convert
			 * @param type  type of the address, a hardware one or of an internet connection
			 * @return      result of the check
			 *
			 * \~
			 */
			bool range(const Network_Address & begin, const Network_Address & end, string_view mask, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin  beginning of the range of the addresses
			 * @param end    end of the range of the addresses
			 * @param prefix prefix of the address to convert
			 * @return       result of the check
			 *
			 * \~
			 */
			bool range(const Network_Address & begin, const Network_Address & end, const uint8_t prefix) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin  beginning of the range of the addresses
			 * @param end    end of the range of the addresses
			 * @param prefix prefix of the address to convert
			 * @param type   type of the address, a hardware one or of an internet connection
			 * @return       result of the check
			 *
			 * \~
			 */
			bool range(const Network_Address & begin, const Network_Address & end, const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin beginning of the range of the addresses
			 * @param end   end of the range of the addresses
			 * @param mask  mask of the network to convert
			 * @return      result of the check
			 *
			 * \~
			 */
			bool range(string_view begin, string_view end, string_view mask) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @param type  тип адреса аппаратного или интернет подключения
			 * @return      результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin beginning of the range of the addresses
			 * @param end   end of the range of the addresses
			 * @param mask  mask of the network to convert
			 * @param type  type of the address, a hardware one or of an internet connection
			 * @return      result of the check
			 *
			 * \~
			 */
			bool range(string_view begin, string_view end, string_view mask, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin  beginning of the range of the addresses
			 * @param end    end of the range of the addresses
			 * @param prefix prefix of the address to convert
			 * @return       result of the check
			 *
			 * \~
			 */
			bool range(string_view begin, string_view end, const uint8_t prefix) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       результат првоерки
			 *
			 * \~english
			 * @brief Method of checking the entry of an IP address into a range of the addresses
			 * @param begin  beginning of the range of the addresses
			 * @param end    end of the range of the addresses
			 * @param prefix prefix of the address to convert
			 * @param type   type of the address, a hardware one or of an internet connection
			 * @return       result of the check
			 *
			 * \~
			 */
			bool range(string_view begin, string_view end, const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @details Проверяет принадлежность хранимого адреса сети, заданной без
			 *          маски и без префикса. Границу сети метод определяет сам: хвостовые
			 *          нули записи считаются подстановочными, и сопоставляются только
			 *          значащие части. Сеть `192.168.0.0` потому означает `192.168.*.*`,
			 *          а `10.0.0.0` - всю сеть `10.*.*.*`
			 *
			 * @note Способ этот удобен для записей, где граница сети приходится на
			 *       границу октета, но выразить сеть вроде `/12` или `/28` им нельзя.
			 *       Для них служат разновидности с маской или префиксом
			 *
			 * @warning Значащий нуль внутри записи от подстановочного не отличается по
			 *          виду, и сеть `10.0.0.0` шире, чем можно ожидать: под неё подойдёт
			 *          любой адрес, начинающийся с десятки. Если граница сети известна,
			 *          задавать её лучше явно
			 *
			 * @warning Сеть из одних нулей - `0.0.0.0` или `::` - значащих частей не
			 *          имеет вовсе, и подойдёт под неё любой адрес: правило хвостовых
			 *          нулей доходит здесь до предела, отвечающего сети `/0`. Записи,
			 *          пришедшей со стороны, метод этот поэтому доверять не следует -
			 *          в перечне разрешений такая сеть открывает доступ всем
			 *
			 * @param network сеть для проверки соответствия
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an IP address to the specified network
			 * @details Checks the belonging of the held address to a network set without
			 *          a mask and without a prefix. The method determines the boundary of the network itself: the trailing
			 *          zeroes of the record are considered the wildcard ones, and only the
			 *          significant parts are matched. The network `192.168.0.0` therefore means `192.168.*.*`,
			 *          and `10.0.0.0` — the whole network `10.*.*.*`
			 * @note This way is convenient for the records where the boundary of a network falls on
			 *       the boundary of an octet, but a network like `/12` or `/28` cannot be expressed by it.
			 *       For them the varieties with a mask or with a prefix serve
			 * @warning A significant zero inside a record does not differ from a wildcard one by
			 *          the look, and the network `10.0.0.0` is wider than one may expect: any address
			 *          beginning with a ten will suit it. If the boundary of the network is known,
			 *          it is better to set it explicitly
			 * @warning A network of the zeroes alone — `0.0.0.0` or `::` — has no significant parts
			 *          at all, and any address will suit it: the rule of the trailing
			 *          zeroes reaches its limit here, answering to the network `/0`. This method should therefore not be trusted with a record
			 *          that has come from the outside — in a list of the permissions such a network opens the access to everyone
			 * @param network network to check the correspondence to
			 * @return        result of the check
			 *
			 * \~
			 */
			bool mapping(string_view network) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an IP address to the specified network
			 * @param network network to check the correspondence to
			 * @param type    type of the address, a hardware one or of an internet connection
			 * @return        result of the check
			 *
			 * \~
			 */
			bool mapping(string_view network, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @details Проверяет принадлежность сети, граница которой задана явно -
			 *          маской или длиной префикса. В отличие от разновидности без
			 *          маски, здесь подстановочных частей нет, и граница может
			 *          приходиться на любой разряд
			 *
			 *          Довод выбора части указывает, какие половины сопоставлять:
			 *          `NETWORK` сравнивает сетевые части - обычная проверка «из этой
			 *          ли сети адрес», - а `HOST` сравнивает хостовые остатки
			 *
			 * @note Состояния объекта проверка не меняет: маска накладывается на
			 *       временные копии, а хранимый адрес остаётся прежним - в отличие от
			 *       `impose()`
			 *
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an IP address to the specified network
			 * @details Checks the belonging to a network whose boundary is set explicitly —
			 *          by a mask or by the length of a prefix. Unlike the variety without
			 *          a mask, here there are no wildcard parts, and the boundary may
			 *          fall on any digit
			 *          The argument of the choice of the part specifies which halves should be matched:
			 *          `NETWORK` compares the network parts — the ordinary check «is the address from this
			 *          network», — and `HOST` compares the host remainders
			 * @note The check does not change the state of the object: the mask is imposed on
			 *       the temporary copies, and the held address remains the previous one — unlike at
			 *       `impose()`
			 * @param network network to check the correspondence to
			 * @param mask    mask of the network to impose
			 * @param addr    type of the obtained address
			 * @return        result of the check
			 *
			 * \~
			 */
			bool mapping(string_view network, string_view mask, const addr_t addr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an IP address to the specified network
			 * @param network network to check the correspondence to
			 * @param mask    mask of the network to impose
			 * @param addr    type of the obtained address
			 * @param type    type of the address, a hardware one or of an internet connection
			 * @return        result of the check
			 *
			 * \~
			 */
			bool mapping(string_view network, string_view mask, const addr_t addr, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an IP address to the specified network
			 * @param network network to check the correspondence to
			 * @param prefix  prefix to impose
			 * @param addr    type of the obtained address
			 * @return        result of the check
			 *
			 * \~
			 */
			bool mapping(string_view network, const uint8_t prefix, const addr_t addr) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking the correspondence of an IP address to the specified network
			 * @param network network to check the correspondence to
			 * @param prefix  prefix to impose
			 * @param addr    type of the obtained address
			 * @param type    type of the address, a hardware one or of an internet connection
			 * @return        result of the check
			 *
			 * \~
			 */
			bool mapping(string_view network, const uint8_t prefix, const addr_t addr, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения принадлежности адреса
			 *
			 * @details Относит хранимый адрес к одному из трёх разрядов: частный,
			 *          служебный или внешний. Проверка идёт по встроенному перечню
			 *          диапазонов, отведённых стандартами под особые нужды, - тех
			 *          самых, что не маршрутизируются во внешнюю сеть или закреплены
			 *          за назначением, а не за хостом
			 *
			 *          Пригождается там, где от происхождения адреса зависит решение:
			 *          пускать ли подключение без проверки прав, доверять ли
			 *          заголовку с адресом клиента, выпускать ли запрос наружу
			 *
			 * @note Перечень диапазонов заведён только для `IPV4` и `IPV6`. Адрес
			 *       иного вида в них не попадёт и получит `WAN` - не потому, что он
			 *       внешний, а потому, что сравнивать было не с чем
			 *
			 * @note Пустой объект даёт `NONE`, и это единственный случай, когда
			 *       разряд не определён: у любого разобранного адреса итог ненулевой
			 *
			 * @note Перечень читается сверху вниз, и записи в нём **намеренно
			 *       перекрываются**: сеть рассылки `224.0.0.0/4` покрывает собой и
			 *       `224.0.0.0/24`, и прочие свои части. Записаны они порознь оттого,
			 *       что перечень ведётся по реестру особых сетей, а не сжимается до
			 *       наименьшего набора: сверять его со стандартом построчно должно
			 *       быть можно
			 *
			 * @note Отнесение сетей к разрядам - решение **принятое, а не выведенное**,
			 *       и на нём стоят: адреса связи (`169.254.0.0/16`, `fe80::/10`) -
			 *       `LAN`, ибо хост назначает их себе сам; общий выход поставщика
			 *       (`100.64.0.0/10`) - `SYS`, ибо хосту он не выдаётся; отменённые
			 *       адреса места (`fec0::/10`) - `LAN` по прежнему их назначению.
			 *       Вложенный адрес IPv4 (`::ffff:0:0/96`) разбирается таблицей своей
			 *       разновидности: узел, слушающий по IPv6, получает подключения
			 *       узлов IPv4 записью `::ffff:192.168.1.1`, и разряд такого адреса
			 *       определяет вложенный адрес, а не оболочка
			 *
			 * @return флаг принадлежности адреса
			 *
			 * \~english
			 * @brief Method of the determination of the belonging of an address
			 * @details Attributes the held address to one of the three classes: a private,
			 *          a service or an external one. The check goes by the built-in list of the
			 *          ranges given over by the standards to the special needs — those very
			 *          ones that are not routed into the external network or are fastened
			 *          to a purpose, and not to a host
			 *          Comes in handy where a decision depends on the origin of the address:
			 *          whether a connection should be let in without a check of the rights, whether a header with the address
			 *          of a client should be trusted, whether a request should be let out
			 * @note The list of the ranges is started only for `IPV4` and `IPV6`. An address
			 *       of another kind will not fall into them and will receive `WAN` — not because it is
			 *       an external one, but because there was nothing to compare with
			 * @note An empty object gives `NONE`, and this is the only case when
			 *       the class is not determined: at any parsed address the result is a non-zero one
			 * @note The list is read from the top downwards, and the records in it **deliberately
			 *       overlap**: the multicast network `224.0.0.0/4` covers `224.0.0.0/24`
			 *       as well, and its other parts. They are written separately because
			 *       the list is kept by the registry of the special networks, and is not compressed to
			 *       the smallest set: checking it against the standard line by line must
			 *       be possible
			 * @note The attribution of the networks to the classes is a decision **taken, and not derived**,
			 *       and they stand: the link addresses (`169.254.0.0/16`, `fe80::/10`) —
			 *       `LAN`, for a host assigns them to itself; the common outlet of a provider
			 *       (`100.64.0.0/10`) — `SYS`, for it is not given out to a host; the cancelled
			 *       site addresses (`fec0::/10`) — `LAN` by their former purpose.
			 *       An embedded IPv4 address (`::ffff:0:0/96`) is resolved by the table of its own
			 *       variety: a node listening over IPv6 receives the connections
			 *       of the IPv4 nodes as the record `::ffff:192.168.1.1`, and the class of such an address
			 *       is determined by the embedded address, and not by the wrapping
			 * @return flag of the belonging of the address
			 *
			 * \~
			 */
			own_t own() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Получение записи в формате ARPA
			 *
			 * @details Выводит адрес в перевёрнутом виде со служебным суффиксом - в
			 *          том, каким его спрашивают у службы имён при обратном
			 *          разрешении, когда по адресу требуется узнать имя. Адрес
			 *          `192.168.0.1` превращается в `1.0.168.192.in-addr.arpa`, а
			 *          адрес IPv6 - в запись из полутетрад с суффиксом `ip6.arpa`
			 *
			 * @note Объект без адреса даёт пустую запись, а не один лишь суффикс.
			 *       Пустую же запись даёт и объект, чья длина буфера выставленному
			 *       виду не отвечает: собирать перевёрнутую запись из байт, буфером
			 *       не занятых, значит выдать за адрес то, чего в нём нет
			 *
			 * @return запись в формате ARPA
			 *
			 * \~english
			 * @brief Getting a record in the ARPA format
			 * @details Yields the address in the reversed form with a service suffix — in
			 *          the one it is asked for at the name service at the reverse
			 *          resolution, when by an address a name is required to be found out. The address
			 *          `192.168.0.1` turns into `1.0.168.192.in-addr.arpa`, and
			 *          an IPv6 address — into a record of the half-octets with the `ip6.arpa` suffix
			 * @note An object without an address gives an empty record, and not the suffix alone.
			 *       An empty record is given as well by an object whose length of the buffer does not answer
			 *       the set out kind: to assemble a reversed record from the bytes not
			 *       occupied by the buffer means to pass off as an address what is not in it
			 * @return record in the ARPA format
			 *
			 * \~
			 */
			string arpa() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки записи в формате ARPA
			 *
			 * @details Разбор, обратный выводу: из перевёрнутой записи восстанавливается
			 *          адрес. Пригождается при чтении ответов службы имён и настроек
			 *          обратных зон
			 *
			 * @note Запись проверяется на полноту: число частей перед суффиксом должно
			 *       отвечать разрядности адреса. Укороченная запись зоны, обозначающая
			 *       сеть, а не хост, будет отвергнута - восстановить из неё один адрес
			 *       нельзя
			 *
			 * @warning Неудачный разбор оставляет объект пустым, а не прежним: разбор
			 *          ведётся прямо в буфере, и отказ посреди него оставлял бы буфер
			 *          новой разновидности при прежнем виде адреса. Область действия
			 *          снимается тоже - записью ARPA она не задаётся
			 *
			 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
			 * @return     результат установки записи
			 *
			 * \~english
			 * @brief Method of setting a record in the ARPA format
			 * @details The parsing reverse to the output: from a reversed record an address is restored.
			 *          Comes in handy at the reading of the answers of the name service and of the settings
			 *          of the reverse zones
			 * @note The record is checked for the completeness: the number of the parts before the suffix must
			 *       answer the width of the address. An abbreviated record of a zone designating
			 *       a network, and not a host, will be rejected — restoring one address from it
			 *       is not possible
			 * @warning An unsuccessful parsing leaves the object empty, and not the previous one: the parsing
			 *          is performed right in the buffer, and a refusal in the middle of it would leave the buffer
			 *          of a new variety at the previous kind of the address. The scope is
			 *          removed as well — it is not set by an ARPA record
			 * @param addr address in the ARPA format (1.0.168.192.in-addr.arpa)
			 * @return     result of the setting of the record
			 *
			 * \~
			 */
			bool arpa(string_view addr) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод парсинга адреса
			 *
			 * @details Разбирает строку и заполняет объект, определяя вид записи сам.
			 *          Перебираются три хранимых вида в порядке `IPv4`, `IPv6`,
			 *          аппаратный адрес, и берётся первый подошедший; распознанный вид
			 *          затем отдаёт `type()`
			 *
			 * @note Разобрать метод берётся **только** эти три вида. Доменное имя,
			 *       URL-адрес и путь файловой системы он отвергнет, хотя `host()` их и
			 *       распознаёт: хранить объекту нечего - за ними стоит не адрес, а имя
			 *
			 * @warning Неудача **очищает объект целиком** - и сам адрес, и пометку о
			 *          его виде. Разбор поверх уже заполненного объекта отрицательным
			 *          итогом оставит его пустым, а `type()` даст `NONE`: негодная
			 *          запись прежней разновидности не наследует
			 *
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 *
			 * \~english
			 * @brief Method of the parsing of an address
			 * @details Parses a string and fills the object, determining the kind of the record itself.
			 *          The three held kinds are traversed in the order `IPv4`, `IPv6`,
			 *          a hardware address, and the first suitable one is taken; the recognized kind
			 *          is then given back by `type()`
			 * @note The method undertakes to parse **only** these three kinds. A domain name,
			 *       a URL address and a path of the file system it will reject, although `host()` does
			 *       recognize them: the object has nothing to hold — behind them there stands not an address, but a name
			 * @warning A failure **clears the object entirely** — both the address itself, and the mark about
			 *          its kind. A parsing over an already filled object with a negative
			 *          result will leave it empty, and `type()` will give `NONE`: an unfit
			 *          record does not inherit the previous variety
			 * @param addr address, a hardware one or of an internet connection, to parse
			 * @return     result of the work of the parsing
			 *
			 * \~
			 */
			bool parse(string_view addr) noexcept;
			/**
			 * \~russian
			 * @brief Метод парсинга адреса
			 *
			 * @details Разбирает строку как запись заведомо известного вида, минуя
			 *          перебор. Пригождается там, где вид задан извне - настройкой или
			 *          протоколом, - и запись, подошедшая под другой вид, должна быть
			 *          отвергнута, а не принята
			 *
			 * @note Годятся здесь только `IPV4`, `IPV6` и `MAC`. Прочие значения
			 *       `type_t` дадут отрицательный итог, ошибкой это не считается
			 *
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @param type тип адреса аппаратного или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 *
			 * \~english
			 * @brief Method of the parsing of an address
			 * @details Parses a string as a record of a knowingly known kind, bypassing
			 *          the traversal. Comes in handy where the kind is set from the outside — by a setting or
			 *          by a protocol, — and a record that has suited another kind must be
			 *          rejected, and not accepted
			 * @note Only `IPV4`, `IPV6` and `MAC` are fit here. The other values of
			 *       `type_t` will give a negative result, this is not considered an error
			 * @param addr address, a hardware one or of an internet connection, to parse
			 * @param type type of the address, a hardware one or of an internet connection, to parse
			 * @return     result of the work of the parsing
			 *
			 * \~
			 */
			bool parse(string_view addr, const type_t type) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения данных IP-адреса
			 *
			 * @details Выводит хранимый адрес строкой. Три довода задают вид записи:
			 *          подробность, систему счисления и разделитель частей. Оставленные
			 *          по умолчанию, они дают привычную запись, свою для каждой
			 *          разновидности адреса
			 *
			 *          Разделитель различает три случая. Значение по умолчанию берёт
			 *          привычный знак - точку у IPv4, двоеточие у IPv6 и у аппаратного
			 *          адреса. Нуль убирает разделитель вовсе, и запись становится
			 *          сплошной - вид, годный ключом отображения, но не для чтения.
			 *          Любой иной знак ставится на место привычного
			 *
			 * @note Пустой объект даёт пустую строку. Отличить его от адреса, чья
			 *       запись пуста, не нужно: таких адресов не бывает. Пустую же строку
			 *       даёт и объект, чья длина буфера выставленному виду не отвечает:
			 *       выписывать по виду то, чего в буфере нет, значит собирать запись
			 *       из байт, буфером уже не занятых
			 *
			 * @note Сплошная запись - та, у которой разделитель задан нулём -
			 *       выписывается разрядами полной ширины, каким бы ни был запрошенный
			 *       вид: снимать ведущие нули у записи, разряды которой нечем
			 *       разделить, значит сделать её нечитаемой обратно. Сжатие нулевых
			 *       разрядов у адреса IPv6 сплошной записи по той же причине не
			 *       применяется
			 *
			 * @note Перегрузка, дописывающая запись в готовый накопитель, обходится без
			 *       выделения памяти под саму строку. Там, где адрес выводится в
			 *       журнал или склеивается с другими частями, пользоваться разумнее ею
			 *
			 * @note Разряды шестнадцатеричной записи **намеренно выводятся заглавными** -
			 *       и у адреса IPv6, и у аппаратного адреса, - тогда как RFC 5952 §4.3
			 *       предписывает строчные. Разбор при этом принимает любой регистр,
			 *       потому что шестнадцатеричная запись регистронезависима, и сломать
			 *       регистр вывода не может ничего: перевод в строчные обходится
			 *       вызывающему в одну строку. Заглавная запись принята в AWH за
			 *       правильную, и переоткрывать этот разбор незачем
			 *
			 *       Прочее с каноническим видом сходится целиком: сличение с системным
			 *       разборщиком на двадцати тысячах случайных адресов дало ту же самую
			 *       запись - то же сжатие нулевых разрядов, то же размещение двойного
			 *       двоеточия, то же отбрасывание ведущих нулей, - и разошлось с ней
			 *       одним лишь регистром. Закреплено проверкой
			 *       `NetUpperCaseRecordIsDeliberateTest`
			 *
			 * @param size  размер формата формирования IP-адреса
			 * @param flag  флаг форматирования IP-адреса
			 * @param delim разделитель формата формирования IP-адреса
			 * @return      сформированная строка IP-адреса
			 *
			 * \~english
			 * @brief Method of extracting the data of an IP address
			 * @details Yields the held address as a string. The three arguments set the kind of the record:
			 *          the detail, the numeral system and the separator of the parts. Left
			 *          by default, they give the customary record, its own for every
			 *          variety of the address
			 *          The separator tells apart three cases. The value by default takes
			 *          the customary sign — a dot at IPv4, a colon at IPv6 and at a hardware
			 *          address. Zero removes the separator altogether, and the record becomes
			 *          a continuous one — a kind fit as a key of a mapping, but not for the reading.
			 *          Any other sign is placed in the place of the customary one
			 * @note An empty object gives an empty string. Telling it apart from an address whose
			 *       record is empty is not needed: such addresses do not happen. An empty string
			 *       is given as well by an object whose length of the buffer does not answer the set out kind:
			 *       to write out by the kind what is not in the buffer means to assemble a record
			 *       from the bytes no longer occupied by the buffer
			 * @note A continuous record — the one whose separator is set as zero —
			 *       is written out by the digits of the full width, whatever the requested
			 *       kind may be: to remove the leading zeroes at a record whose digits there is nothing
			 *       to separate by means to make it unreadable back. The compression of the zero
			 *       digits at an IPv6 address is for the same reason not applied to a continuous record
			 * @note The overload appending the record into a ready accumulator gets by without
			 *       an allocation of the memory for the string itself. Where an address is yielded into
			 *       a log or is glued with other parts, it is more reasonable to use it
			 * @note The digits of the hexadecimal record are **deliberately yielded as the capital ones** —
			 *       both at an IPv6 address, and at a hardware address, — while RFC 5952 §4.3
			 *       prescribes the lowercase ones. The parsing at that accepts any case,
			 *       because the hexadecimal record is case-insensitive, and nothing can break
			 *       the case of the output: the conversion into the lowercase ones costs the caller
			 *       one line. The capital record is accepted in AWH as
			 *       the correct one, and there is no point in reopening this analysis
			 *       The rest agrees with the canonical form entirely: the matching with the system
			 *       parser on twenty thousand random addresses gave the very same
			 *       record — the same compression of the zero digits, the same placement of the double
			 *       colon, the same discarding of the leading zeroes, — and diverged with it
			 *       by the case alone. Fixed by the check
			 *       `NetUpperCaseRecordIsDeliberateTest`
			 * @param size  size of the format of the building of the IP address
			 * @param flag  flag of the formatting of the IP address
			 * @param delim separator of the format of the building of the IP address
			 * @return      the built string of the IP address
			 *
			 * \~
			 */
			string print(const format_size_t size = format_size_t::NONE, const format_flag_t flag = format_flag_t::NONE, const int32_t delim = -1) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи IP-адреса в накопитель в виде строки
			 *
			 * @details Запись дописывается в конец накопителя, а не замещает его
			 *          содержимое: сборщику строки, склеивающему её из частей, запись
			 *          адреса нужна не сама по себе, а на своём месте в накопителе.
			 *          Возвращающая же перегрузка обходится ему выделением памяти под
			 *          саму отдаваемую строку - запись IPv6-адреса длиннее порога
			 *          размещения строки внутри объекта, - и всё это выделение уходит
			 *          на перенос трёх десятков октетов в накопитель, который уже есть.
			 *
			 * @param result накопитель, в который дописывается строка IP-адреса
			 * @param size   размер формата вывода IP-адреса
			 * @param flag   флаг формата вывода IP-адреса
			 * @param delim  разделитель для формата вывода IP-адреса
			 *
			 * \~english
			 * @brief Method of writing an IP address into an accumulator as a string
			 * @details The record is appended at the end of the accumulator, and does not replace its
			 *          content: to the assembler of a string, gluing it from the parts, the record
			 *          of an address is needed not by itself, but in its place in the accumulator.
			 *          The returning overload, though, costs it an allocation of the memory for
			 *          the given back string itself — a record of an IPv6 address is longer than the threshold
			 *          of the placement of a string inside an object, — and all this allocation goes
			 *          onto the carrying of three dozens of octets into an accumulator that already exists.
			 * @param result accumulator the string of the IP address is appended into
			 * @param size   size of the format of the output of the IP address
			 * @param flag   flag of the format of the output of the IP address
			 * @param delim  separator for the format of the output of the IP address
			 *
			 * \~
			 */
			void print(string & result, const format_size_t size = format_size_t::NONE, const format_flag_t flag = format_flag_t::NONE, const int32_t delim = -1) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор вывода IP-адреса в качестве строки
			 *
			 * @details Приведение к строке равносильно вызову `print()` без доводов и
			 *          даёт запись по умолчанию. Там, где нужен иной вид записи,
			 *          обращаться следует к `print()` напрямую
			 *
			 * @note Приведение это неявное, и адрес подставится всюду, где ждут строку.
			 *       Удобно при сборке записей журнала, но и ошибочная подстановка
			 *       вместо ожидаемого числа пройдёт незамеченной
			 *
			 * @return IP-адрес в качестве строки
			 *
			 * \~english
			 * @brief Operator of the output of an IP address as a string
			 * @details The conversion to a string is equivalent to a call of `print()` without the arguments and
			 *          gives the record by default. Where another kind of the record is needed,
			 *          one should address `print()` directly
			 * @note This conversion is an implicit one, and an address will be substituted everywhere a string is expected.
			 *       It is convenient at the assembly of the records of a log, but an erroneous substitution
			 *       instead of an expected number will pass unnoticed as well
			 * @return IP address as a string
			 *
			 * \~
			 */
			operator string() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор [<] сравнения IP-адреса
			 *
			 * @details Сравнение ведётся по **значению** адреса, а не по его записи:
			 *          `192.168.1.9` встанет раньше `192.168.1.10`, тогда как сравнение
			 *          строк расставило бы их наоборот. Благодаря этому адреса годятся
			 *          ключом упорядоченных наборов и сортируются привычным образом
			 *
			 * @warning Адреса **разных видов** несравнимы, и все сравнивающие
			 *          операторы, кроме `!=`, дают на них ложь: адрес IPv4 не меньше и
			 *          не больше адреса IPv6, они попросту несопоставимы. Полного
			 *          порядка на смешанном наборе потому нет, и сортировать такой
			 *          набор нельзя - вид адресов следует разделять заранее
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief The [<] operator of the comparison of an IP address
			 * @details The comparison is performed by the **value** of the address, and not by its record:
			 *          `192.168.1.9` will stand before `192.168.1.10`, while the comparison
			 *          of the strings would arrange them the other way round. Thanks to this the addresses are fit as
			 *          a key of the ordered sets and are sorted in the customary way
			 * @warning The addresses of **different kinds** are incomparable, and all the comparing
			 *          operators except `!=` give falsehood on them: an IPv4 address is neither less nor
			 *          greater than an IPv6 address, they are simply incomparable. There is therefore no full
			 *          order on a mixed set, and sorting such a set is not allowed — the kind of the addresses should be divided in advance
			 * @param addr address to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			bool operator < (const Network_Address & addr) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [>] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief The [>] operator of the comparison of an IP address
			 * @param addr address to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			bool operator > (const Network_Address & addr) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [<=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief The [<=] operator of the comparison of an IP address
			 * @param addr address to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			bool operator <= (const Network_Address & addr) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [>=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief The [>=] operator of the comparison of an IP address
			 * @param addr address to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			bool operator >= (const Network_Address & addr) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [!=] сравнения IP-адреса
			 *
			 * @note Оператор этот - точное отрицание равенства, поэтому адреса разных
			 *       видов он признаёт различными. Прочие сравнивающие операторы на
			 *       смешанных видах дают ложь, и `!=` среди них единственный, кто
			 *       ведёт себя ожидаемо
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief The [!=] operator of the comparison of an IP address
			 * @note This operator is the exact negation of the equality, and therefore it recognizes the addresses of different
			 *       kinds as different ones. The other comparing operators on the mixed kinds give falsehood, and `!=` is the only one among them that
			 *       behaves as expected
			 * @param addr address to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			bool operator != (const Network_Address & addr) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [==] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief The [==] operator of the comparison of an IP address
			 * @param addr address to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			bool operator == (const Network_Address & addr) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @details Переносит адрес из другого объекта: сам адрес, его вид и зону
			 *          IPv6. Строгий режим остаётся у получателя прежним - он задаёт
			 *          правила разбора, а не содержимое
			 *
			 * @note Заведение копией переносит объект целиком, вместе со строгим
			 *       режимом: присваивание меняет содержимое уже настроенного объекта,
			 *       а копия заводит новый, и настройки ей взять неоткуда, кроме как у
			 *       образца
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of the assignment of an IP address
			 * @details Carries the address from another object: the address itself, its kind and the zone
			 *          of IPv6. The strict mode remains the previous one at the receiver — it sets
			 *          the rules of the parsing, and not the content
			 * @note The starting by a copy carries the object entirely, together with the strict
			 *       mode: the assignment changes the content of an already set up object,
			 *       and a copy starts a new one, and it has nowhere to take the settings from, except from
			 *       the sample
			 * @param addr address to assign
			 * @return     the current object
			 *
			 * \~
			 */
			Network_Address & operator = (const Network_Address & addr) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @details Присваивание строки равносильно вызову `parse()` и вид записи
			 *          определяет само. Годится оно для заведомо верных адресов -
			 *          заданных в исходном тексте или уже проверенных
			 *
			 * @warning Итог разбора оператор **не отдаёт**, и неудача остаётся
			 *          незамеченной: объект попросту окажется пуст. Адрес, пришедший
			 *          извне, разбирать следует вызовом `parse()`, чей итог можно
			 *          проверить
			 *
			 * @param ip адрес для присвоения
			 * @return   текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of the assignment of an IP address
			 * @details The assignment of a string is equivalent to a call of `parse()` and determines the kind of the record
			 *          itself. It is fit for the knowingly correct addresses —
			 *          set in the source text or already checked
			 * @warning The operator **does not give back** the result of the parsing, and a failure remains
			 *          unnoticed: the object will simply turn out to be empty. An address that has come
			 *          from the outside should be parsed by a call of `parse()`, whose result can be
			 *          checked
			 * @param ip address to assign
			 * @return   the current object
			 *
			 * \~
			 */
			Network_Address & operator = (string_view ip) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания установки типа IP-адреса
			 *
			 * @warning Как и метод `type()`, оператор меняет только пометку о виде
			 *          адреса, не трогая данных и не проверяя их на соответствие
			 *
			 * @param type тип IP-адреса для установки
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of the setting of the type of an IP address
			 * @warning As the `type()` method, the operator changes only the mark about the kind of
			 *          the address, without touching the data and without checking it for the correspondence
			 * @param type type of the IP address to set
			 * @return     the current object
			 *
			 * \~
			 */
			Network_Address & operator = (const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @details Присваивание числа заполняет объект адресом IPv4 и выставляет
			 *          вид сам. Порядок байт при этом берётся машинный: если число
			 *          пришло из структур сокетов в сетевом порядке, пользоваться
			 *          следует методом `v4()` с явным указанием порядка
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of the assignment of an IP address
			 * @details The assignment of a number fills the object with an IPv4 address and sets out
			 *          the kind itself. The order of the bytes at that is taken as the machine one: if the number
			 *          has come from the structures of the sockets in the network order, one should use
			 *          the `v4()` method with the order specified explicitly
			 * @param addr address to assign
			 * @return     the current object
			 *
			 * \~
			 */
			Network_Address & operator = (const uint32_t addr) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения MAC-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of the assignment of a MAC address
			 * @param addr address to assign
			 * @return     the current object
			 *
			 * \~
			 */
			Network_Address & operator = (const array <uint8_t, 6> & addr) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of the assignment of an IP address
			 * @param addr address to assign
			 * @return     the current object
			 *
			 * \~
			 */
			Network_Address & operator = (const array <uint8_t, 16> & addr) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Network_Address(const fmk_t * fmk, const log_t * log) noexcept;
		public:
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
			~Network_Address() noexcept;
	} net_addr_t;
	/**
	 * \~russian
	 * @brief Оператор [>>] чтения из потока IP-адреса
	 *
	 * @param is   поток для чтения
	 * @param addr адрес для присвоения
	 *
	 * \~english
	 * @brief The [>>] operator of the reading of an IP address from a stream
	 * @param is   stream to read from
	 * @param addr address to assign
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, net_addr_t & addr) noexcept;
	/**
	 * \~russian
	 * @brief Оператор [<<] вывода в поток IP-адреса
	 *
	 * @param os   поток куда нужно вывести данные
	 * @param addr адрес для присвоения
	 *
	 * \~english
	 * @brief The [<<] operator of the output of an IP address into a stream
	 * @param os   stream the data needs to be yielded into
	 * @param addr address to assign
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const net_addr_t & addr) noexcept;
};

#endif // __AWH_NET_ADDR__
