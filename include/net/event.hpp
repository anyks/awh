/**
 * @file events.hpp
 * @date 2025-10-26
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
 * @brief Заголовочный файл системы типов событий — перечисления идентификаторов, действий, узлов, семейств адресов,
 *        протоколов, режимов доставки, статусов, кодов ошибок, DSCP, ECN и лимитов,
 *        образующие общий протокол-нейтральный словарь событий библиотеки
 *
 * \~english
 * @brief Header file of the system of the types of the events — the enumerations of the identifiers, of the actions, of the nodes, of the families of the addresses,
 *        of the protocols, of the modes of the delivery, of the statuses, of the codes of the errors, of the DSCP, of the ECN and of the limits,
 *        forming the common protocol-neutral dictionary of the events of the library
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_EVENTS__
#define __AWH_EVENTS__

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

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
	 * \~russian
	 * @brief пространство имён работы с событием
	 *
	 * \~english
	 * @brief namespace of the work with an event
	 *
	 * \~
	 */
	namespace event {
		/**
		 * \~russian
		 * @brief Тип идентификатора события
		 *
		 * \~english
		 * @brief Type of the identifier of an event
		 *
		 * \~
		 */
		using id_t = uint32_t;

		/**
		 * \~russian
		 * @brief Типы событий
		 *
		 * @details Выбирает устройство, на котором движок держит отсчёты времени.
		 *          Простой отсчёт устроен на упорядоченном наборе и словаре: завод
		 *          каждого отсчёта обходится в пару обращений к куче, зато устройство
		 *          нетребовательно и предсказуемо на любых объёмах. Сложный отсчёт
		 *          устроен на двоичной куче и разбитой на части таблице ячеек и
		 *          заводится **без единого обращения к куче**, отчего на больших
		 *          количествах отсчётов оказывается кратно быстрее
		 *
		 * @note По умолчанию движок берёт **простой** отсчёт, хотя по всем замерам
		 *       сложный его превосходит. Выбор этот намеренный: умолчания
		 *       рассчитываются на самые слабые системы и самые общие случаи, а тот,
		 *       кому отсчёты времени нужны быстрыми, включит сложные сам. Делается
		 *       это методом `setInternalTimer()`
		 *
		 * \~english
		 * @brief Types of the events
		 * @details Chooses the arrangement on which the engine holds the counts of the time.
		 *          The simple count is arranged on an ordered set and on a dictionary: the starting
		 *          of every count costs a couple of addresses to the heap, but in exchange the arrangement
		 *          is undemanding and predictable at any volumes. The complex count is
		 *          arranged on a binary heap and on a table of the cells broken into the parts and
		 *          is started **without a single address to the heap**, and therefore at large
		 *          numbers of the counts turns out to be multiply faster
		 * @note By default the engine takes the **simple** count, although by all the measurements
		 *       the complex one surpasses it. This choice is a deliberate one: the defaults
		 *       are reckoned on the weakest systems and on the most common cases, and the one
		 *       who needs the counts of the time to be fast will switch on the complex ones himself. This is done
		 *       by the `setInternalTimer()` method
		 *
		 * \~
		 */
		enum class timer_t : uint8_t {
			SIMPLE    = 0x00, // Простой таймер
			DIFFICULT = 0x01  // Сложный таймер
		};

		/**
		 * \~russian
		 * @brief Действия событий
		 *
		 * @details Отвечает на вопрос, что именно с узлом произошло. Набор общий на
		 *          все виды узлов, но приложима к каждому лишь своя часть: сетевому
		 *          узлу достаются чтение, запись, приём подключения и отключение, а
		 *          файловому - изменение, удаление, переименование и правка признаков
		 *
		 * @note Отзыв доступа стоит особняком: он приходит не от правки файла, а от
		 *       исчезновения самой опоры под ним - файл удалён или носитель отключён.
		 *       Наблюдение за таким узлом продолжать бессмысленно, узел следует
		 *       завести заново
		 *
		 * \~english
		 * @brief Actions of the events
		 * @details Answers the question what exactly has happened to a node. The set is common for
		 *          all the kinds of the nodes, but only its own part is applicable to each: a network
		 *          node gets the reading, the writing, the acceptance of a connection and the disconnection, and
		 *          a file one — the change, the removal, the renaming and the correction of the signs
		 * @note The revocation of the access stands apart: it comes not from a correction of a file, but from
		 *       the disappearance of the very support under it — the file is removed or the medium is disconnected.
		 *       Continuing the observation of such a node is meaningless, the node should be
		 *       started anew
		 *
		 * \~
		 */
		enum class action_t : uint8_t {
			NONE       = 0x00, // Экшен события отсутствует
			READ       = 0x01, // Чтение объекта
			WRITE      = 0x02, // Запись объекта
			CLOSE      = 0x03, // Закрытие объекта
			CHANGE     = 0x04, // Изменение файла или каталога
			DELETE     = 0x05, // Удаление файла или каталога
			RENAME     = 0x06, // Переименование файла или каталога
			ATTRIB     = 0x07, // Атрибуты файла изменены (chmod, chown, utime)
			REVOKE     = 0x08, // Доступ к файлу отозван (например, файл был удалён или размонтирована ФС)
			HDLINK     = 0x09, // Счётчик жёстких ссылок на файл изменился
			ACCEPT     = 0x0A, // Входящее соединение принято
			CONNECT    = 0x0B, // Подключение к серверу установлено
			RECONNECT  = 0x0C, // Переподключение к серверу после обрыва подключения
			DISCONNECT = 0x0D  // Отключение от сервера выполнено
		};

		/**
		 * \~russian
		 * @brief Типы узлов
		 *
		 * @details Узел - это то, за чем движок наблюдает: подключение, файл,
		 *          каталог, отсчёт времени. Вид узла определяет, какие обратные
		 *          связи к нему приложимы и какие действия он порождает, поэтому
		 *          задаётся он при заведении узла и потом не меняется
		 *
		 *          Виды делятся на несколько семейств. Сетевые - `CLIENT`, `SERVER`,
		 *          `PEER`, `ORIGIN` - разнятся тем, кто заводит подключение и кто его
		 *          принимает. Файловые - `FILE`, `DIR` - наблюдают за изменениями в
		 *          файловой системе. Временные - `TIMEOUT` и `INTERVAL` - различаются
		 *          тем, срабатывает отсчёт единожды или повторяется. Прочие -
		 *          `IPC`, `NOTIFY`, `TUNNEL`, `MEDIATOR` - служат межпроцессному
		 *          взаимодействию, пробуждению цикла и работе с туннелями
		 *
		 * @note Обратная связь, к виду узла неприложимая, тихо не подключается: движок
		 *       заносит в журнал предупреждение и продолжает работу. Отсутствие
		 *       вызовов обратной связи стоит потому искать сперва в журнале
		 *
		 * \~english
		 * @brief Types of the nodes
		 * @details A node is that which the engine observes: a connection, a file,
		 *          a directory, a count of the time. The kind of a node determines which callbacks
		 *          are applicable to it and which actions it generates, and therefore
		 *          it is set at the starting of the node and does not change afterwards
		 *          The kinds are divided into several families. The network ones — `CLIENT`, `SERVER`,
		 *          `PEER`, `ORIGIN` — differ by who starts the connection and who
		 *          accepts it. The file ones — `FILE`, `DIR` — observe the changes in
		 *          the file system. The time ones — `TIMEOUT` and `INTERVAL` — differ
		 *          by whether the count triggers once or repeats. The others —
		 *          `IPC`, `NOTIFY`, `TUNNEL`, `MEDIATOR` — serve the interprocess
		 *          communication, the awakening of the loop and the work with the tunnels
		 * @note A callback not applicable to the kind of a node is not quietly connected: the engine
		 *       enters a warning into the log and continues the work. The absence of
		 *       the calls of a callback is therefore worth looking for in the log first
		 *
		 * \~
		 */
		enum class node_t : uint8_t {
			NONE     = 0x00, // Узел не определён
			IPC      = 0x01, // Узел межпроцессного взаимодействия
			DIR      = 0x02, // Узел директории в файловой системе
			FILE     = 0x03, // Узел файла в файловой системе
			PEER     = 0x04, // Одноранговый узел
			ORIGIN   = 0x05, // Исходящий узел
			NOTIFY   = 0x06, // Узел уведомления
			CLIENT   = 0x07, // Узел клиента
			SERVER   = 0x08, // Узел сервера
			TIMEOUT  = 0x09, // Узел таймаута времени
			INTERVAL = 0x0A, // Узел интервала времени
			TUNNEL   = 0x0B, // Узел туннеля
			MEDIATOR = 0x0C  // Узел посредника
		};

		/**
		 * \~russian
		 * @brief Типы виртуальных узлов
		 *
		 * @details Сообщает, чем оказалась запись файловой системы, за которой ведётся
		 *          наблюдение: обычным файлом, каталогом, символической ссылкой,
		 *          именованным каналом, сокетом или узлом устройства. Приходит
		 *          значение вместе с известием об изменении - там, где важно не
		 *          только что запись изменилась, но и что она собой представляет
		 *
		 * @note Набор этот описывает запись в файловой системе и с видами узлов
		 *       движка не связан, хотя названия и перекликаются
		 *
		 * \~english
		 * @brief Types of the virtual nodes
		 * @details Reports what the record of the file system being observed
		 *          has turned out to be: an ordinary file, a directory, a symbolic link,
		 *          a named pipe, a socket or a node of a device. The value comes
		 *          together with the notice of a change — where it is important not
		 *          only that the record has changed, but what it represents as well
		 * @note This set describes a record in the file system and is not connected with the kinds of the nodes
		 *       of the engine, although the names do echo each other
		 *
		 * \~
		 */
		enum class vnode_t : uint8_t {
			NONE = 0x00, // Узел не определён
			CHR  = 0x01, // Виртуальный символьный узел устройства
			BLK  = 0x02, // Виртуальный блочный узел устройства
			DIR  = 0x03, // Виртуальный узел каталога
			FILE = 0x04, // Виртуальный узел файла
			FIFO = 0x05, // Виртуальный узел канала FIFO
			SOCK = 0x06, // Виртуальный сокет узел
			LINK = 0x07  // Виртуальный узел символической ссылки
		};

		/**
		 * \~russian
		 * @brief Типы адресов событий
		 *
		 * @details Указывает, какой из адресов узла запрашивается или выставляется:
		 *          у одного подключения их несколько - свой адрес, адрес другого
		 *          конца, аппаратный адрес устройства, путь к местному сокету
		 *
		 * @note Приложимость видов зависит от узла: у местного сокета есть путь, но
		 *       нет адреса сети, а у сетевого подключения наоборот. Запрос
		 *       неподходящего вида даёт отрицательный итог, ошибкой это не считается
		 *
		 * \~english
		 * @brief Types of the addresses of the events
		 * @details Specifies which of the addresses of a node is requested or set out:
		 *          one connection has several of them — its own address, the address of the other
		 *          end, the hardware address of the device, the path to the local socket
		 * @note The applicability of the kinds depends on the node: a local socket has a path, but
		 *       has no address of the network, and a network connection is the other way round. A request of
		 *       an unsuitable kind gives a negative result, this is not considered an error
		 *
		 * \~
		 */
		enum class address_t : uint8_t {
			NONE    = 0x00, // Адрес не определён
			FS      = 0x01, // Адрес файловой системы
			MAC     = 0x02, // MAC-адрес
			UDS     = 0x03, // Адрес Unix Domain Socket
			IPV4    = 0x04, // Адрес IPv4
			IPV6    = 0x05, // Адрес IPv6
			NETWORK = 0x06  // Сетевой адрес
		};

		/**
		 * \~russian
		 * @brief Типы сетевых интерфейсов
		 *
		 * @details Разновидности сетевых устройств, какие движок различает при
		 *          опросе системы. Часть из них отвечает настоящему оборудованию -
		 *          проводному и беспроводному, - а часть существует лишь в ядре:
		 *          туннели, переносящие пакеты одной сети внутри другой, устройства
		 *          логической сегментации, объединение нескольких устройств в одно и
		 *          мост, сводящий их на канальном уровне
		 *
		 * @note Туннельные устройства делятся по уровню, на котором работают: одни
		 *       переносят готовые пакеты, другие - кадры вместе с аппаратными
		 *       адресами. Разница эта существенна: через первые мост не построить
		 *
		 * \~english
		 * @brief Types of the network interfaces
		 * @details The kinds of the network devices which the engine tells apart at
		 *          the polling of the system. A part of them answers to a real equipment —
		 *          a wired and a wireless one, — and a part exists only in the kernel:
		 *          the tunnels carrying the packets of one network inside another one, the devices
		 *          of the logical segmentation, the joining of several devices into one and
		 *          the bridge bringing them together at the link level
		 * @note The tunnel devices are divided by the level they work at: some
		 *       carry the ready packets, the others — the frames together with the hardware
		 *       addresses. This difference is essential: through the first ones a bridge cannot be built
		 *
		 * \~
		 */
		enum class eth_t : uint8_t {
			NONE   = 0x00, // Интерфейс не определён
			NET    = 0x01, // Прямое подключение к сети через драйвер сетевой карты
			TUN    = 0x02, // Передача сырых IP-пакетов между точками
			TAP    = 0x03, // Передача кадров Ethernet (с MAC-адресами)
			GIF    = 0x04, // Общий туннельный интерфейс (IPv6-in-IPv4, IPv4-in-IPv6, IPv6-in-IPv6)
			GRE    = 0x05, // GRE-туннель (включая с ключом)
			WLAN   = 0x06, // Беспроводной интерфейс
			VLAN   = 0x07, // Интерфейс логической сегментации на основе 802.1Q
			BOND   = 0x08, // Интерфейс агрегации каналов
			BRIDGE = 0x09  // Объединение интерфейсов на уровне L2
		};

		/**
		 * \~russian
		 * @brief Флаги сетевых интерфейсов
		 *
		 * @details Признаки состояния и настроек сетевого устройства, какими их
		 *          сообщает система: поднято ли устройство, передаёт ли оно данные
		 *          сейчас, принимает ли рассылку на группу и широковещание, отключено
		 *          ли разрешение аппаратных адресов
		 *
		 * @note Поднятое устройство и работающее - разные признаки. Первый означает,
		 *       что устройство включено настройкой, второй - что связь на нём есть в
		 *       самом деле. Провод, выдернутый из поднятого устройства, снимет второй
		 *       признак, но не первый
		 *
		 * @note Значения этого набора - не разряды двоичного слова, а порядковые
		 *       номера, и складывать их побитово нельзя, несмотря на название
		 *
		 * \~english
		 * @brief Flags of the network interfaces
		 * @details The signs of the state and of the settings of a network device, as they are
		 *          reported by the system: whether the device is brought up, whether it transmits the data
		 *          now, whether it accepts the multicast to a group and the broadcast, whether the resolution
		 *          of the hardware addresses is switched off
		 * @note A brought up device and a working one are different signs. The first one means
		 *       that the device is switched on by a setting, the second one — that the link on it is present in
		 *       reality. A cable pulled out of a brought up device will remove the second
		 *       sign, but not the first one
		 * @note The values of this set are not the digits of a binary word, but the ordinal
		 *       numbers, and adding them bitwise is not allowed, despite the name
		 *
		 * \~
		 */
		enum class eth_flag_t : uint8_t {
			NONE         = 0x00, // Флаг не определён
			UP           = 0x01, // Флаг интерфейс поднят
			DEBUG 	     = 0x02, // Флаг debug режим
			NOARP 	     = 0x03, // Флаг отключён ARP
			ALLMULTI     = 0x04, // Флаг приём всех multicast-пакетов
			PROMISC      = 0x05, // Флаг promiscuous режим
			DYNAMIC      = 0x06, // Флаг динамический интерфейс (DHCP и т.д. только Linux)
			RUNNING      = 0x07, // Флаг интерфейс работает
			BROADCAST    = 0x08, // Флаг broadcast
			MULTICAST    = 0x09, // Флаг multicast
			LOOPBACK     = 0x0A, // Флаг loopback интерфейс
			POINTTOPOINT = 0x0B  // Флаг point-to-point
		};

		/**
		 * \~russian
		 * @brief Режимы ошибок отправки данных
		 *
		 * @details Указывает, откуда вернулись неотправленные данные, когда движок
		 *          отдаёт их обратно. Из самого события - значит, отправка сорвалась
		 *          при обращении к сокету. Из очереди события - значит, данные
		 *          сорваться ещё не успели: они дожидались своей очереди в накопителе,
		 *          а подключение закрылось раньше
		 *
		 * @note Различать эти два случая стоит при повторной отправке. Данные из
		 *       очереди сеть не видела вовсе и отправить их заново безопасно, а по
		 *       данным из события сказать, сколько дошло до другой стороны, нельзя
		 *
		 * \~english
		 * @brief Modes of the errors of the sending of the data
		 * @details Specifies where the unsent data has returned from, when the engine
		 *          gives it back. From the event itself — it means the sending has broken down
		 *          at the address to the socket. From the queue of the event — it means the data has not managed
		 *          to break down yet: it was waiting for its turn in the accumulator,
		 *          and the connection has closed earlier
		 * @note Telling these two cases apart is worthwhile at a repeated sending. The data from
		 *       the queue the network has not seen at all and sending it anew is safe, and by
		 *       the data from the event it is impossible to say how much has reached the other side
		 *
		 * \~
		 */
		enum class send_error_t : uint8_t {
			NONE     = 0x00, // Режим не определён
			IO_EVENT = 0x01, // Возвращение данных из события
			IO_QUEUE = 0x02  // Возвращение данных из очереди события
		};

		/**
		 * \~russian
		 * @brief Типы трансляции пакетов
		 *
		 * @details Задаёт, кому предназначен пакет. Одному указанному узлу - обычная
		 *          передача. Группе подписавшихся - рассылка, при которой сеть сама
		 *          размножает пакет по ветвям и доставляет лишь туда, где есть
		 *          подписчики. Всем узлам сегмента - широковещание
		 *
		 * @note Рассылка на группу и широковещание требуют, чтобы сокет был к ним
		 *       заранее подготовлен, и за пределы своего сегмента без поддержки со
		 *       стороны сети не выходят. Широковещание у IPv6 не существует вовсе -
		 *       его заменяет рассылка на предопределённую группу
		 *
		 * \~english
		 * @brief Types of the transmission of the packets
		 * @details Sets whom a packet is meant for. To one specified node — the ordinary
		 *          transmission. To a group of the subscribed ones — the multicast, at which the network itself
		 *          multiplies the packet over the branches and delivers it only there, where there are
		 *          subscribers. To all the nodes of the segment — the broadcast
		 * @note The multicast to a group and the broadcast require the socket to be
		 *       prepared for them in advance, and do not go beyond their own segment without the support from
		 *       the side of the network. The broadcast does not exist at IPv6 at all —
		 *       the multicast to a predefined group replaces it
		 *
		 * \~
		 */
		enum class delivery_mode_t : uint8_t {
			NONE      = 0x00, // Режим не определён
			UNICAST   = 0x01, // Режим unicast
			MULTICAST = 0x02, // Режим multicast
			BROADCAST = 0x03  // Режим broadcast
		};

		/**
		 * \~russian
		 * @brief Максимальное количество хопов через которые может пройти пакет
		 *
		 * @details Задаёт, сколько маршрутизаторов пакету дозволено пройти. Каждый
		 *          встречный уменьшает счётчик на единицу, а исчерпавший его пакет
		 *          отбрасывается. Служит счётчик двум целям: обрывает пакеты,
		 *          заплутавшие в кольцевом маршруте, и **ограничивает область
		 *          распространения** - последнее важно для рассылки на группу, где
		 *          счётчик задаёт, насколько далеко разойдётся объявление
		 *
		 *          Значения перечисления - это именованные пороги, отвечающие
		 *          привычным рубежам: не покидать хост, не покидать подсеть,
		 *          оставаться внутри организации, региона, материка либо идти без
		 *          ограничения
		 *
		 * @note Пороги эти условны и жёсткой границе не отвечают: число переходов
		 *       до узла зависит от устройства сети, а не от расстояния. Опираться на
		 *       них стоит как на приблизительный рубеж, а не как на точную границу
		 *
		 * \~english
		 * @brief Maximum number of the hops a packet may pass through
		 * @details Sets how many routers a packet is allowed to pass. Every
		 *          one met decreases the counter by one, and a packet that has exhausted it
		 *          is discarded. The counter serves two goals: it breaks off the packets
		 *          that have strayed into a circular route, and **limits the area
		 *          of the propagation** — the latter is important for the multicast to a group, where
		 *          the counter sets how far an announcement will spread
		 *          The values of the enumeration are the named thresholds answering
		 *          to the customary boundaries: not to leave the host, not to leave the subnet,
		 *          to remain inside the organization, the region, the continent or to go without
		 *          a limitation
		 * @note These thresholds are conditional and do not answer to a strict boundary: the number of the hops
		 *       to a node depends on the arrangement of the network, and not on the distance. It is worth relying on
		 *       them as on an approximate boundary, and not as on an exact one
		 *
		 * \~
		 */
		enum class hops_t : uint8_t {
			LOOPBACK  = 0x00, // Только в хосте (loopback)
			NETWORK   = 0x01, // Только локальная сеть (подсеть)
			COMPANY   = 0x20, // Внутри организации
			REGION    = 0x40, // Внутри региона
			CONTINENT = 0x80, // Внутри континента
			WORLD     = 0xFF  // Глобально (максимум)
		};

		/**
		 * \~russian
		 * @brief Статусы событий
		 *
		 * @details Перечисление служит сразу двум задачам, и это стоит держать в уме.
		 *          Часть значений - это внутренние состояния жизненного цикла события,
		 *          по которым движок решает, какой вызов событию сейчас позволен. Часть
		 *          приходит наружу, в функцию обратного вызова `status_t`, как сообщение
		 *          о происходящем. Некоторые значения делают и то и другое.
		 *
		 *          **Жизненный цикл.** `NONE` - событие заведено вызовом `event()`, но не
		 *          настроено. `INITIAL` - настройки закреплены вызовом `commit()`.
		 *          `SUCCESS` - выполнен `connect()` или `listen()`. `LAUNCHED` и
		 *          `LISTENING` - событие запущено и участвует в опросе: первое у событий,
		 *          которым подключаться не нужно, второе у слушающего сервера.
		 *          `CANCELLED` - соединение разорвано вызовом `disconnect()`. `GARBAGE` и
		 *          `DESTROYED` - событие помечено к освобождению, которое отложено на два
		 *          оборота цикла.
		 *
		 *          **Сообщения наружу.** `CONNECTED` и `RECONNECTED` - соединение
		 *          установлено или восстановлено. `ACCEPTED` - принято входящее.
		 *          `PAUSED` и `RESUMED` - чтение приостановлено или возвращено.
		 *          `FAILURE` - операция не удалась. `PENDING` - операция начата и ждёт
		 *          завершения. `REBIRTHED` - дескриптор пересоздан вызовом `rebirth()`.
		 *          `QUEUE_OVERFLOW` и `QUEUE_AVAILABLE` - очередь отправки переполнилась
		 *          или в ней снова есть место.
		 *
		 * @note У значения `SUCCESS` две роли, и путать их не следует. Для узлов
		 *       соединений это состояние «подключено или слушает». А для
		 *       **узлов-таймеров** именно с этим значением приходит сообщение о
		 *       срабатывании: отдельной функции обратного вызова у таймеров нет, и
		 *       `SUCCESS` в их подписке `status_t` означает «сработал»
		 *
		 * \~english
		 * @brief Statuses of the events
		 * @details The enumeration serves two tasks at once, and this is worth keeping in mind.
		 *          A part of the values are the internal states of the life cycle of an event,
		 *          by which the engine decides which call is allowed to the event now. A part
		 *          comes outwards, into the `status_t` callback function, as a message
		 *          about what is happening. Some values do both.
		 *          **The life cycle.** `NONE` — the event is started by the `event()` call, but is not
		 *          set up. `INITIAL` — the settings are fixed by the `commit()` call.
		 *          `SUCCESS` — a `connect()` or a `listen()` is performed. `LAUNCHED` and
		 *          `LISTENING` — the event is launched and participates in the polling: the first one at the events
		 *          that need not connect, the second one at a listening server.
		 *          `CANCELLED` — the connection is broken by the `disconnect()` call. `GARBAGE` and
		 *          `DESTROYED` — the event is marked for the release, which is postponed for two
		 *          turns of the loop.
		 *          **The messages outwards.** `CONNECTED` and `RECONNECTED` — the connection
		 *          is established or restored. `ACCEPTED` — an incoming one is accepted.
		 *          `PAUSED` and `RESUMED` — the reading is paused or returned.
		 *          `FAILURE` — the operation has failed. `PENDING` — the operation is begun and awaits
		 *          the completion. `REBIRTHED` — the descriptor is recreated by the `rebirth()` call.
		 *          `QUEUE_OVERFLOW` and `QUEUE_AVAILABLE` — the queue of the sending has overflowed
		 *          or there is room in it again.
		 * @note The `SUCCESS` value has two roles, and they should not be confused. For the nodes
		 *       of the connections this is the state «connected or listening». And for
		 *       the **timer nodes** it is exactly with this value that the message about
		 *       the triggering comes: the timers have no separate callback function, and
		 *       `SUCCESS` in their `status_t` subscription means «has triggered»
		 *
		 * \~
		 */
		enum class status_t : uint8_t {
			NONE            = 0x00, // Статус не определён
			INITIAL         = 0x01, // Статус инициализации
			REBIRTHED       = 0x02, // Статус возрождения
			DESTROYED       = 0x03, // Статус удаления
			ACCEPTED        = 0x04, // Статус принятия
			LAUNCHED	    = 0x05, // Статус выполнения
			PAUSED          = 0x06, // Статус паузы
			RESUMED         = 0x07, // Статус возобновления
			SUCCESS         = 0x08, // Операция выполнена успешно
			FAILURE         = 0x09, // Операция завершилась неудачей
			PENDING         = 0x0A, // Операция в ожидании
			GARBAGE         = 0x0B, // Статус мусора
			LISTENING       = 0x0C, // Статус прослушивания
			CONNECTED       = 0x0D, // Статус подключено
			CANCELLED       = 0x0E, // Операция отменена
			RECONNECTED     = 0x0F, // Статус переподключения
			QUEUE_OVERFLOW  = 0x10, // Статус переполнения очереди
			QUEUE_AVAILABLE = 0x11  // Статус доступности очереди
		};

		/**
		 * \~russian
		 * @brief Типы ошибок событий
		 *
		 * @details Приходят в функцию обратного вызова `error_t` вместе с текстовым
		 *          описанием. Текст пригоден для журнала, а решения следует принимать
		 *          по коду: описание берётся у системы и от неё же и зависит.
		 *
		 *          Различать их удобно по тому, что делать дальше. `INVALID`,
		 *          `NOT_FOUND` и `ALREADY_EXISTS` означают ошибку вызывающей стороны -
		 *          не то состояние события, не тот идентификатор, повторное действие;
		 *          повтор здесь не поможет. `CONNECTION_FAIL`, `INVALID_SOCKET` и
		 *          `EVENT_FAIL` говорят о самом соединении и обычно ведут к его
		 *          пересозданию. `PACKET_TOO_BIG` относится к одной записи, а не к
		 *          соединению: его лечит уменьшение порции. `INSUFFICIENT_RES` -
		 *          исчерпание ресурсов, чаще всего дескрипторов, и с ним разумно
		 *          снизить нагрузку, а не повторять
		 *
		 * @note Пока функция обратного вызова ошибок не установлена, движок печатает
		 *       ошибки сам. С её установкой печать прекращается, и весь разбор
		 *       переходит к вызывающей стороне
		 *
		 * \~english
		 * @brief Types of the errors of the events
		 * @details Come into the `error_t` callback function together with a text
		 *          description. The text is fit for a log, and the decisions should be taken
		 *          by the code: the description is taken from the system and depends on it as well.
		 *          Telling them apart is convenient by what should be done next. `INVALID`,
		 *          `NOT_FOUND` and `ALREADY_EXISTS` mean an error of the calling side —
		 *          a wrong state of the event, a wrong identifier, a repeated action;
		 *          a repetition will not help here. `CONNECTION_FAIL`, `INVALID_SOCKET` and
		 *          `EVENT_FAIL` speak about the connection itself and usually lead to its
		 *          recreation. `PACKET_TOO_BIG` concerns one record, and not
		 *          the connection: a decrease of the portion cures it. `INSUFFICIENT_RES` —
		 *          the exhaustion of the resources, most often of the descriptors, and with it it is reasonable
		 *          to lower the load, and not to repeat
		 * @note While the callback function of the errors is not set, the engine prints
		 *       the errors itself. With its setting the printing stops, and the whole handling
		 *       passes over to the calling side
		 *
		 * \~
		 */
		enum class error_t : uint8_t {
			NONE             = 0x00, // Ошибка не определена
			UNKNOWN          = 0x01, // Неизвестная ошибка
			INVALID          = 0x02, // Недопустимая операция
			NOT_FOUND        = 0x03, // Объект не найден
			EVENT_FAIL       = 0x04, // Ошибка события
			ACCESS_DENIED    = 0x05, // Доступ запрещён
			PACKET_TOO_BIG   = 0x06, // Слишком большой пакет для записи
			ALREADY_EXISTS   = 0x07, // Объект уже существует
			INVALID_SOCKET   = 0x08, // Ошибка доступа к сокету
			INVALID_ADDRESS  = 0x09, // Некорректный адрес
			CONNECTION_FAIL  = 0x0A, // Ошибка подключения
			INSUFFICIENT_RES = 0x0B  // Недостаточно ресурсов
		};

		/**
		 * \~russian
		 * @brief Режимы обнаружения MTU
		 *
		 * @details Задаёт, как узнавать наибольший размер пакета, проходящий по пути
		 *          без дробления. Размер этот определяется самым узким участком
		 *          маршрута и заранее неизвестен: пакет крупнее либо дробится, либо
		 *          отбрасывается с уведомлением, по которому отправитель и понимает,
		 *          что хватил лишку
		 *
		 *          Режимы разнятся тем, насколько деятельно ведётся поиск и насколько
		 *          резко размер снижается при неудачах: от полного отказа от поиска до
		 *          отправки пробных пакетов и подстройки по отклику сети
		 *
		 * @note Поддержка режимов **зависит от системы**, и одинакового поведения
		 *       всюду ждать не следует: часть режимов на отдельных системах
		 *       приводится к ближайшему доступному
		 *
		 * \~english
		 * @brief Modes of the discovery of the MTU
		 * @details Sets how the largest size of a packet passing over a path
		 *          without a fragmentation should be found out. That size is determined by the narrowest section of
		 *          the route and is unknown in advance: a larger packet is either fragmented, or
		 *          discarded with a notification, by which the sender understands
		 *          that it has overshot
		 *          The modes differ by how actively the search is performed and how
		 *          sharply the size is lowered at the failures: from a full refusal from the search to
		 *          the sending of the probe packets and the adjustment by the response of the network
		 * @note The support of the modes **depends on the system**, and the same behaviour
		 *       everywhere should not be expected: a part of the modes at some systems
		 *       is brought to the nearest available one
		 *
		 * \~
		 */
		enum class mtu_discover_t : uint8_t {
			NONE         = 0x00, // Режим не определён
			DONT         = 0x01, // Не выполнять обнаружение MTU
			WANT         = 0x02, // Выполнять обнаружение MTU
			DO           = 0x03, // Выполнять обнаружение MTU и устанавливать оптимальный размер пакетов
			PROBE        = 0x04, // Выполнять обнаружение MTU и отправлять пробные пакеты для определения оптимального размера
			ADAPT        = 0x05, // Выполнять адаптивное обнаружение MTU, автоматически регулируя размер пакетов на основе обратной связи от сети
			STRICT       = 0x06, // Выполнять строгое обнаружение MTU, отбрасывая пакеты, превышающие установленный размер MTU
			AGGRESSIVE   = 0x07, // Выполнять агрессивное обнаружение MTU, быстро уменьшая размер пакетов при возникновении проблем с доставкой
			CONSERVATIVE = 0x08, // Выполнять консервативное обнаружение MTU, медленно уменьшая размер пакетов при возникновении проблем с доставкой
			SMART        = 0x09  // Выполнять интеллектуальное обнаружение MTU, используя алгоритмы машинного обучения для оптимизации размера пакетов на основе исторических данных о сети
		};

		/**
		 * \~russian
		 * @brief Точка кода дифференцированных услуг
		 *
		 * @details Пометка в заголовке пакета, которой отправитель заявляет о нужном
		 *          качестве обслуживания. Сеть, настроенная эту пометку различать,
		 *          раскладывает пакеты по очередям: чувствительные к задержке
		 *          пропускает вперёд, объёмные придерживает при заторе
		 *
		 *          Значения разбиты на семейства. Простые разряды приоритета -
		 *          восемь ступеней от обычного трафика до управления самой сетью.
		 *          Гарантированная экспедиция - разряды, где первое число задаёт
		 *          очередь, а второе вероятность отбрасывания при заторе. Ускоренная
		 *          пересылка - отдельный разряд для передачи голоса и видео, где
		 *          задержка и её разброс дороже потерь
		 *
		 * @warning Пометка эта - **просьба, а не обязательство**. За пределами
		 *          управляемой сети её обычно либо не замечают, либо сбрасывают, и
		 *          опираться на неё в открытой сети не следует
		 *
		 * \~english
		 * @brief Differentiated services code point
		 * @details A mark in the header of a packet by which the sender declares the needed
		 *          quality of the service. A network set up to tell this mark apart
		 *          lays the packets out into the queues: the ones sensitive to the delay
		 *          it lets ahead, the voluminous ones it holds back at a congestion
		 *          The values are broken into the families. The simple digits of the priority —
		 *          eight steps from the ordinary traffic to the control of the network itself.
		 *          The assured forwarding — the digits where the first number sets the
		 *          queue, and the second one the probability of the discarding at a congestion. The expedited
		 *          forwarding — a separate digit for the transmission of the voice and of the video, where
		 *          the delay and its spread are dearer than the losses
		 * @warning This mark is a **request, and not an obligation**. Beyond
		 *          a managed network it is usually either not noticed, or reset, and
		 *          relying on it in an open network is not advisable
		 *
		 * \~
		 */
		enum class dscp_t : uint8_t {
			CS0  = 0x00, // По умолчанию (обычный трафик)
			CS1  = 0x08, // Приоритетный
			CS2  = 0x10, // Немедленный
			CS3  = 0x18, // Интерактивный
			CS4  = 0x20, // Ресурсоёмкий
			CS5  = 0x28, // Критический
			CS6  = 0x30, // Интернет-система
			CS7  = 0x38, // Контроль сети
			AF11 = 0x0A, // Гарантированная экспедиция 11
			AF12 = 0x0C, // Гарантированная экспедиция 12
			AF13 = 0x0E, // Гарантированная экспедиция 13
			AF21 = 0x12, // Гарантированная экспедиция 21
			AF22 = 0x14, // Гарантированная экспедиция 22
			AF23 = 0x16, // Гарантированная экспедиция 23
			AF31 = 0x1A, // Гарантированная экспедиция 31 (Контроль звука)
			AF32 = 0x1C, // Гарантированная экспедиция 32 (Контроль звука)
			AF33 = 0x1E, // Гарантированная экспедиция 33 (Контроль звука)
			AF41 = 0x22, // Гарантированная экспедиция 41 (Видео)
			AF42 = 0x24, // Гарантированная экспедиция 42 (Видео)
			AF43 = 0x26, // Гарантированная экспедиция 43 (Видео)
			AF51 = 0x2A, // Гарантированная экспедиция 51
			AF52 = 0x2C, // Гарантированная экспедиция 52
			EF   = 0x2E  // Ускоренная пересылка пакетов (VoIP, видео и т.д.)
		};

		/**
		 * \~russian
		 * @brief Значения поля Explicit Congestion Notification (ECN)
		 *
		 * @details Младшие два бита байта TOS (IPv4) и Traffic Class (IPv6),
		 *          которыми маршрутизаторы сигнализируют о перегрузке пути,
		 *          не отбрасывая пакет (RFC 3168 §5).
		 *
		 * \~english
		 * @brief Values of the Explicit Congestion Notification (ECN) field
		 * @details The lower two bits of the TOS byte (IPv4) and of the Traffic Class one (IPv6),
		 *          by which the routers signal about a congestion of the path,
		 *          without discarding the packet (RFC 3168 §5).
		 *
		 * \~
		 */
		enum class ecn_t : uint8_t {
			NOT_ECT = 0x00, // Отправитель не поддерживает ECN
			ECT1    = 0x01, // Отправитель поддерживает ECN (ECT(1))
			ECT0    = 0x02, // Отправитель поддерживает ECN (ECT(0))
			CE      = 0x03  // Маршрутизатор отметил пакет как испытавший перегрузку
		};

		/**
		 * \~russian
		 * @brief Режимы ограничения скорости событий
		 *
		 * @details Задаёт, отдавать ли данные в сеть сразу или придержать. Мгновенная
		 *          отправка выталкивает каждую порцию немедленно - так нужно там, где
		 *          дорога задержка: обмен короткими сообщениями, отклик на нажатие,
		 *          передача звука. Отложенная накапливает мелкие порции и отправляет
		 *          их вместе, отчего доля служебных заголовков падает, а
		 *          пропускная способность растёт
		 *
		 * @note Придержанная порция уходит не по воле отправителя, а по истечении
		 *       короткого срока или по приходу подтверждения. Отложенный режим потому
		 *       добавляет задержку, незаметную при потоковой передаче и весьма
		 *       заметную при обмене короткими сообщениями
		 *
		 * \~english
		 * @brief Modes of the limitation of the speed of the events
		 * @details Sets whether the data should be given into the network at once or held back. The instant
		 *          sending pushes out every portion immediately — that is how it is needed where
		 *          the delay is dear: the exchange of the short messages, the response to a keypress,
		 *          the transmission of the sound. The postponed one accumulates the small portions and sends
		 *          them together, and therefore the share of the service headers falls, and
		 *          the bandwidth grows
		 * @note A held back portion goes not by the will of the sender, but at the expiration of
		 *       a short term or at the arrival of an acknowledgement. The postponed mode therefore
		 *       adds a delay imperceptible at a streaming transmission and quite
		 *       perceptible at the exchange of the short messages
		 *
		 * \~
		 */
		enum class rate_t : uint8_t {
			NONE     = 0x00, // Режим не определён
			INSTANT  = 0x01, // Режим мгновенной отправки данных (instant)
			DEFERRED = 0x02  // Режим отложенной отправки данных (defer)
		};

		/**
		 * \~russian
		 * @brief Происхождение событий
		 *
		 * @details Указывает, о чьей стороне подключения идёт речь - о нашей или о
		 *          той. Свойства подключения у двух концов свои, и опрашивать их
		 *          приходится порознь: этим значением и задаётся, чьи именно
		 *          запрашиваются
		 *
		 * @note Пользуются им пока лишь свойства подключений SCTP, где набор
		 *       разрешённых частей сообщения у каждой стороны свой и договориться о
		 *       нём стороны должны заранее
		 *
		 * \~english
		 * @brief Origin of the events
		 * @details Specifies whose side of a connection is meant — ours or
		 *          that one. The properties of a connection are their own at the two ends, and they have to
		 *          be polled separately: by this value it is set whose exactly
		 *          are requested
		 * @note So far only the properties of the SCTP connections use it, where the set
		 *       of the allowed parts of a message is its own at every side and the sides must agree about
		 *       it in advance
		 *
		 * \~
		 */
		enum class origin_t : uint8_t {
			LOCAL  = 0x00, // Локальное событие
			REMOTE = 0x01  // Удалённое событие
		};

		/**
		 * \~russian
		 * @brief Режимы событий
		 *
		 * @details Обычный переключатель на два положения: включить или отключить.
		 *          Пользуются им там, где настройка сводится к одному этому выбору -
		 *          вступление в группу рассылки и выход из неё, разрешение и запрет
		 *          отдельного действия у узла
		 *
		 * @note Значения по умолчанию здесь нет, и «включено» стоит первым не как
		 *       умолчание, а лишь по порядку перечисления
		 *
		 * \~english
		 * @brief Modes of the events
		 * @details An ordinary switch with two positions: to switch on or to switch off.
		 *          It is used where a setting comes down to this one choice —
		 *          the entry into a multicast group and the exit from it, the permission and the prohibition of
		 *          a separate action at a node
		 * @note There is no value by default here, and «switched on» stands first not as
		 *       a default, but only by the order of the enumeration
		 *
		 * \~
		 */
		enum class mode_t : uint8_t {
			ENABLED  = 0x00, // Режим включён
			DISABLED = 0x01  // Режим отключён
		};

		/**
		 * \~russian
		 * @brief Режимы использования событий
		 *
		 * @details Задаёт, как понимать предел ожидания на чтении. Одноразовый режим
		 *          означает ожидание ответа: предел отсчитывается от отправки запроса
		 *          и истекает, если ответ не пришёл. Многоразовый означает надзор за
		 *          простоем: предел взводится заново после каждой порции данных и
		 *          истекает лишь тогда, когда подключение молчит слишком долго
		 *
		 *          Разница видна на долгих обменах. Одноразовый режим оборвёт
		 *          подключение, по которому данные идут дольше отведённого предела,
		 *          даже если идут они непрерывно. Многоразовый такое подключение
		 *          сохранит и оборвёт лишь замолчавшее
		 *
		 * @note По умолчанию берётся одноразовый режим - тот, что отвечает обмену
		 *       «запрос-ответ». Для подписок, потоков и долгих загрузок его следует
		 *       менять на многоразовый, иначе предел ожидания оборвёт исправное
		 *       подключение
		 *
		 * \~english
		 * @brief Modes of the use of the events
		 * @details Sets how the limit of the waiting at the reading should be understood. The single-use mode
		 *          means the waiting for an answer: the limit is counted from the sending of a request
		 *          and expires if the answer has not come. The multiple-use one means the supervision over
		 *          an idling: the limit is raised anew after every portion of the data and
		 *          expires only when the connection is silent for too long
		 *          The difference is seen at the long exchanges. The single-use mode will break off
		 *          a connection over which the data goes longer than the allotted limit,
		 *          even if it goes continuously. The multiple-use one will preserve such a connection
		 *          and will break off only a silent one
		 * @note By default the single-use mode is taken — the one that answers the exchange
		 *       «request-answer». For the subscriptions, the streams and the long downloads it should
		 *       be changed to the multiple-use one, otherwise the limit of the waiting will break off a sound
		 *       connection
		 *
		 * \~
		 */
		enum class usage_t : uint8_t {
			NONE       = 0x00, // Режим не определён
			REUSABLE   = 0x01, // Режим многократного использования события
			DISPOSABLE = 0x02  // Режим одноразового использования события
		};

		/**
		 * \~russian
		 * @brief Режимы ограничения пропускной способности событий
		 *
		 * @details Указывает, какое из двух направлений ограничивается: исходящее -
		 *          то, что уходит от нас, - или входящее. Направления ограничиваются
		 *          порознь, и предел, выставленный одному, другого не касается
		 *
		 * @note Ограничение входящего направления работает иначе исходящего:
		 *       отправку мы придерживаем сами, а на приёме придержать можем лишь
		 *       вычитывание из сокета, отчего данные копятся в приёмном накопителе, а
		 *       сама сеть узнаёт о пределе не сразу
		 *
		 * \~english
		 * @brief Modes of the limitation of the bandwidth of the events
		 * @details Specifies which of the two directions is limited: the outgoing one —
		 *          that which goes away from us, — or the incoming one. The directions are limited
		 *          separately, and a limit set out to one does not concern the other one
		 * @note The limitation of the incoming direction works otherwise than of the outgoing one:
		 *       the sending we hold back ourselves, and at the reception we can hold back only
		 *       the reading out of the socket, and therefore the data accumulates in the receiving accumulator, and
		 *       the network itself learns about the limit not at once
		 *
		 * \~
		 */
		enum class limiting_t : uint8_t {
			EGRESS  = 0x00, // Режим исходящего трафика
			INGRESS = 0x01  // Режим входящего трафика
		};

		/**
		 * \~russian
		 * @brief Типы контрольных списков событий
		 *
		 * @details Задаёт, каким способом узел решает, пускать ли подключение.
		 *          Чёрный список запрещает перечисленное и разрешает всё прочее,
		 *          белый - наоборот, разрешает только перечисленное
		 *
		 * @note Пустой список проверку **отключает**, а не запирает узел: пустой
		 *       белый список разрешает всех, а не отвергает всех. Списки потому
		 *       безопасно заводить заранее и наполнять по мере надобности, но и
		 *       случайное опустошение белого списка снимает ограничение молча
		 *
		 * @note Списки эти проверяются оба, и чёрный имеет перевес: узел, попавший в
		 *       оба списка сразу, отвергается
		 *
		 * \~english
		 * @brief Types of the control lists of the events
		 * @details Sets by which way a node decides whether a connection should be let in.
		 *          A black list forbids what is enumerated and allows everything else,
		 *          a white one — the other way round, allows only what is enumerated
		 * @note An empty list **switches the check off**, and does not lock the node: an empty
		 *       white list allows everyone, and does not reject everyone. The lists are therefore
		 *       safe to start in advance and to fill as needed, but an
		 *       accidental emptying of a white list removes the limitation silently as well
		 * @note These lists are both checked, and the black one has the upper hand: a node that has got into
		 *       both lists at once is rejected
		 *
		 * \~
		 */
		enum class control_list_t : uint8_t {
			NONE    = 0x00, // Список не определён
			BLACK   = 0x01, // Чёрный список
			WHITE   = 0x02  // Белый список
		};

		/**
		 * \~russian
		 * @brief Флаг направления передачи данных
		 *
		 * @details Задаёт, в какую сторону перекладываются данные при сращивании двух
		 *          подключений. Сращивание нужно посредникам: принятые от одной
		 *          стороны данные переливаются другой без разбора их содержимого
		 *
		 *          Направление определяется тем, кто источник, а кто приёмник. Прямое
		 *          ведёт от подключения, к которому применён вызов, к указанному;
		 *          обратное - наоборот
		 *
		 * @note Сращивание **одностороннее**. Для сквозного обмена его следует
		 *       завести дважды, в оба направления, иначе данные пойдут только в одну
		 *       сторону
		 *
		 * \~english
		 * @brief Flag of the direction of the transmission of the data
		 * @details Sets in which direction the data is shifted at the splicing of two
		 *          connections. The splicing is needed by the mediators: the data accepted from one
		 *          side is poured over into the other one without the parsing of its content
		 *          The direction is determined by who is the source, and who is the receiver. The direct one
		 *          leads from the connection the call is applied to, to the specified one;
		 *          the reverse one — the other way round
		 * @note The splicing is a **one-way** one. For a through exchange it should be
		 *       started twice, in both directions, otherwise the data will go only in one
		 *       direction
		 *
		 * \~
		 */
		enum class direct_t : uint8_t {
			NONE   = 0x00,  // Направление данных не определено
			FORWARD = 0x01, // Прямое направление данных
			REVERSE = 0x02  // Обратное направление данных
		};

		/**
		 * \~russian
		 * @brief Типы смещений в файле
		 *
		 * \~english
		 * @brief Types of the offsets in a file
		 *
		 * \~
		 */
		enum class seek_t : uint8_t {
			BEGIN   = 0x00, // Смещение от начала файла
			CURRENT = 0x01, // Смещение от текущей позиции
			END     = 0x02  // Смещение от конца файла
		};

		/**
		 * \~russian
		 * @brief Типы протоколов сокетов
		 *
		 * @details Задаёт, по каким правилам ведётся обмен. Вместе с семейством
		 *          адресов и видом сокета образует тройку, полностью описывающую
		 *          узел, - и не всякое их сочетание существует: потоковый обмен
		 *          неразлучен с надёжной доставкой, дейтаграммный с ненадёжной
		 *
		 *          Помимо привычных протоколов сети сюда входят два особых значения -
		 *          файл и каталог. Наблюдение за файловой системой устроено движком
		 *          теми же средствами, что и сетевые подключения, и вид наблюдаемого
		 *          задаётся здесь же
		 *
		 * @note Протоколы эти - равноправные участники, и выделенного среди них нет.
		 *       Хранить в узле следует само значение, а не признак вида «это ли
		 *       такой-то протокол»: последнее ломается при добавлении нового
		 *
		 * \~english
		 * @brief Types of the protocols of the sockets
		 * @details Sets by which rules the exchange is performed. Together with the family of
		 *          the addresses and the kind of the socket it forms a triple fully describing
		 *          a node, — and not every combination of them exists: the stream exchange
		 *          is inseparable from the reliable delivery, the datagram one from the unreliable one
		 *          Besides the customary protocols of the network two special values enter here —
		 *          a file and a directory. The observation of the file system is arranged by the engine
		 *          by the same means as the network connections, and the kind of what is observed
		 *          is set right here
		 * @note These protocols are equal participants, and there is no distinguished one among them.
		 *       The value itself should be held in a node, and not a sign of the kind «is this
		 *       such-and-such a protocol»: the latter breaks at the addition of a new one
		 *
		 * \~
		 */
		enum class protocol_t : uint8_t {
			NONE = 0x00, // Протокол не определён
			RAW  = 0x01, // Протокол соответствует RAW
			UDP  = 0x02, // Протокол соответствует UDP
			TCP  = 0x03, // Протокол соответствует TCP
			ICMP = 0x04, // Протокол соответствует ICMP
			IGMP = 0x05, // Протокол соответствует IGMP
			SCTP = 0x06, // Протокол соответствует SCTP
			QUIC = 0x07, // Протокол соответствует QUIC (транспорт поверх UDP с собственным слоем записей)
			FILE = 0x08, // Протокол соответствует файлу
			DIR  = 0x09  // Протокол соответствует каталогу
		};

		/**
		 * \~russian
		 * @brief Семейства сокетов
		 *
		 * @details Задаёт, каким способом узел адресуется. Сетевые семейства - по
		 *          адресу IPv4 или IPv6, местное - по пути в файловой системе, прочие
		 *          - вовсе не адресом: канал, наблюдение за файловой системой и
		 *          отсчёт времени сокетами в привычном смысле не являются, но движком
		 *          обслуживаются наравне с ними
		 *
		 * @note Семейство определяет и вид адреса, какой узел примет. Адрес IPv4,
		 *       переданный узлу семейства IPv6, будет отвергнут - если только он не
		 *       обёрнут в отражённый вид `::FFFF:x.x.x.x`
		 *
		 * \~english
		 * @brief Families of the sockets
		 * @details Sets by which way a node is addressed. The network families — by
		 *          an IPv4 or an IPv6 address, the local one — by a path in the file system, the others
		 *          — not by an address at all: a pipe, the observation of the file system and
		 *          a count of the time are not sockets in the customary sense, but are served by the engine
		 *          on a par with them
		 * @note The family determines the kind of the address a node will accept as well. An IPv4 address
		 *       passed to a node of the IPv6 family will be rejected — unless it is
		 *       wrapped into the mapped form `::FFFF:x.x.x.x`
		 *
		 * \~
		 */
		enum class family_t : uint8_t {
			NONE  = 0x00, // Семейство событий не определено
			UDS   = 0x01, // Семейство событий Unix Domain Sockets
			PIPE  = 0x02, // Семейство событий пайпов
			FSYS  = 0x03, // Файловое семейство событий
			USER  = 0x04, // Пользовательское семейство событий
			IPV4  = 0x05, // Семейство событий IPv4
			IPV6  = 0x06, // Семейство событий IPv6
			TIMER = 0x07  // Семейство таймеров
		};

		/**
		 * \~russian
		 * @brief Типы сокетов
		 *
		 * @details Задаёт устройство обмена. Потоковый сокет даёт непрерывную
		 *          последовательность байт без внутренних границ: отправленное одной
		 *          записью может прийти несколькими и наоборот, отчего границы
		 *          сообщений приходится размечать самому. Дейтаграммный сохраняет
		 *          границы, но ни доставки, ни порядка не обещает. Сокет
		 *          последовательных пакетов сочетает то и другое - границы
		 *          сохраняются, доставка надёжна. Сырой сокет отдаёт пакеты вместе с
		 *          заголовками и требует особых прав
		 *
		 * @warning Отсутствие границ у потокового обмена - источник самой частой
		 *          ошибки при работе с сетью: считать, что одному вызову отправки
		 *          отвечает один вызов приёма, нельзя. Принятые данные следует
		 *          накапливать и разбирать по границам, заданным самим протоколом
		 *
		 * \~english
		 * @brief Types of the sockets
		 * @details Sets the arrangement of the exchange. A stream socket gives a continuous
		 *          sequence of the bytes without the internal boundaries: what is sent by one
		 *          record may come by several and the other way round, and therefore the boundaries
		 *          of the messages have to be marked up by oneself. A datagram one preserves the
		 *          boundaries, but promises neither the delivery nor the order. A socket
		 *          of the sequential packets combines both — the boundaries
		 *          are preserved, the delivery is reliable. A raw socket gives back the packets together with
		 *          the headers and requires special rights
		 * @warning The absence of the boundaries at a stream exchange is the source of the most frequent
		 *          error at the work with the network: it is not allowed to consider that one call of the sending
		 *          answers to one call of the reception. The accepted data should be
		 *          accumulated and parsed by the boundaries set by the protocol itself
		 *
		 * \~
		 */
		enum class type_t : uint8_t {
			NONE      = 0x00, // Тип сокета не определён
			RAW       = 0x01, // Сырой сокет
			STREAM    = 0x02, // Потоковый сокет
			DATAGRAM  = 0x03, // Дейтаграммный сокет
			SEQPACKET = 0x04  // Сокет с последовательными пакетами
		};

		/**
		 * \~russian
		 * @brief пространство имён опций событий
		 *
		 * \~english
		 * @brief namespace of the options of the events
		 *
		 * \~
		 */
		namespace options {
			/**
			 * \~russian
			 * @brief Опция не определена
			 *
			 * \~english
			 * @brief The option is not defined
			 *
			 * \~
			 */
			static constexpr uint16_t NONE = 0x00;
			/**
			 * \~russian
			 * @brief Опция ручной установки заголовков IP пакетов
			 *
			 * \~english
			 * @brief Option of the manual setting of the headers of the IP packets
			 *
			 * \~
			 */
			static constexpr uint16_t HDRINCL = 0x01;
			/**
			 * \~russian
			 * @brief Опция отложенной отправки TCP пакетов
			 *
			 * @details Смысл её - придержать отправку, накапливая мелкие записи до полного
			 *          сегмента, пока придержка не будет снята
			 *
			 * @warning Имя намеренно расходится с системным TCP_CORK: у Solaris и Linux то
			 *          занято макросом из netinet/tcp.h, и препроцессор, разбирающий текст
			 *          прежде языка, обращал объявление в присвоение числу - сборка отвечала
			 *          отказом «expected unqualified-id before numeric constant». Снимать
			 *          чужой макрос ради своего имени значило бы менять окружение
			 *          потребителя заголовка, потому расходится имя, а не окружение
			 *
			 * @note Приём этот в наборе уже принят: сосед зовётся TCP_NO_DELAY, а не
			 *       системным TCP_NODELAY, и по той же причине. Проверено опытом на семи
			 *       системах, что имя TCP_CORKING не занято ни одной из них
			 *
			 * @see Настройку саму backend подставляет по системе: TCP_NOPUSH у BSD и macOS,
			 *      а у NetBSD - снятие TCP_NODELAY
			 *
			 * \~english
			 * @brief Option of the postponed sending of the TCP packets
			 * @details Its point is to hold back the sending, accumulating the small records up to a full
			 *          segment, until the holding back is removed
			 * @warning The name deliberately diverges from the system TCP_CORK: at Solaris and Linux that one
			 *          is taken by a macro from netinet/tcp.h, and the preprocessor, parsing the text
			 *          before the language, turned the declaration into an assignment to a number — the build answered with
			 *          a refusal «expected unqualified-id before numeric constant». To remove
			 *          a foreign macro for the sake of one's own name would mean changing the environment
			 *          of the consumer of the header, and therefore the name diverges, and not the environment
			 * @note This device is already accepted in the set: the neighbour is called TCP_NO_DELAY, and not
			 *       by the system TCP_NODELAY, and for the same reason. It is verified by an experiment on seven
			 *       systems that the name TCP_CORKING is taken by none of them
			 * @see The setting itself the backend substitutes by the system: TCP_NOPUSH at BSD and macOS,
			 *      and at NetBSD — the removal of TCP_NODELAY
			 *
			 * \~
			 */
			static constexpr uint16_t TCP_CORKING = 0x02;
			/**
			 * \~russian
			 * @brief Опция отключения сигнала SIGILL
			 *
			 * \~english
			 * @brief Option of the switching off of the SIGILL signal
			 *
			 * \~
			 */
			static constexpr uint16_t NO_SIGILL = 0x04;
			/**
			 * \~russian
			 * @brief Опция широковещательного адреса
			 *
			 * \~english
			 * @brief Option of the broadcast address
			 *
			 * \~
			 */
			static constexpr uint16_t BROADCAST = 0x08;
			/**
			 * \~russian
			 * @brief Опция самостоятельного переподключения события
			 *
			 * @details Событие с этой опцией после обрыва не уничтожается, а поднимается
			 *          заново: пересоздаётся сокет, переустанавливаются опции, заново
			 *          проходятся фиксация, подключение и запуск. Идентификатор события и
			 *          все внесённые в него настройки при этом сохраняются
			 *
			 * @note Опция **включает** переподключение, а разрешает его действие
			 *       `action_t::RECONNECT`: нужны оба. Задержка задаётся `setTimeout()` с
			 *       действием `RECONNECT`, при нулевом значении берётся пять секунд
			 *
			 * @warning К `SO_KEEPALIVE` опция отношения не имеет, хотя прежде и звалась
			 *          `KEEPALIVE`, чем вводила в заблуждение поголовно всех. Пробы
			 *          живости соединения включаются отдельным методом `keepAlive()`,
			 *          принимающим их собственные счёт, простой и промежуток. Здесь же
			 *          речь о постоянстве **события**, а не соединения
			 *
			 * @see AUTO_FOLLOW - то же самое свойство для узла файловой системы
			 *
			 * \~english
			 * @brief Option of the independent reconnection of an event
			 * @details An event with this option after a break is not destroyed, but is raised
			 *          anew: the socket is recreated, the options are reset, the fixation,
			 *          the connection and the launch are passed anew. The identifier of the event and
			 *          all the settings entered into it are at that preserved
			 * @note The option **switches on** the reconnection, and its action is allowed by
			 *       `action_t::RECONNECT`: both are needed. The delay is set by `setTimeout()` with
			 *       the action `RECONNECT`, at a zero value five seconds are taken
			 * @warning The option has no relation to `SO_KEEPALIVE`, although formerly it was called
			 *          `KEEPALIVE`, by which it misled absolutely everyone. The probes
			 *          of the liveness of a connection are switched on by the separate `keepAlive()` method,
			 *          taking their own count, idle and interval. Here, though,
			 *          the matter is about the persistence of the **event**, and not of the connection
			 * @see AUTO_FOLLOW — the same property for a node of the file system
			 *
			 * \~
			 */
			static constexpr uint16_t AUTO_RECONNECT = 0x10;
			/**
			 * \~russian
			 * @brief Опция самостоятельного продолжения чтения файла
			 *
			 * @details Событие файловой системы с этой опцией при новом чтении продолжает
			 *          с сохранённого места: смещение в файле и время последней правки не
			 *          сбрасываются, и вызывающему достаётся лишь дописанное с прошлого
			 *          раза. Без опции файл читается всякий раз сначала
			 *
			 * @note Это **то же самое** значение, что и `AUTO_RECONNECT`, а не отдельная
			 *       опция: свойство у них одно - движок сам продолжает работу события
			 *       после того, как она прервалась, - и разнятся лишь развязки по виду
			 *       узла. У клиента продолжение выливается в новое подключение, у файла -
			 *       в дочитывание с сохранённого места
			 *
			 *       Второе имя заведено ради вызывающего, а не ради движка. Узел он берёт
			 *       одного вида зараз и видит то имя, которое к его узлу относится:
			 *       клиенту нечего отслеживать, файлу не к чему переподключаться, и
			 *       чужое имя сбивало бы с толку на пустом месте. Тем же приёмом
			 *       пользуются `htons` и `ntohs` - действие одно, имён два, потому что
			 *       читаются они в разных местах
			 *
			 * @warning Общее значение обязано таковым и остаться. Разведи их по разным
			 *          битам - и уйдёт последний свободный (`0x20`), единственный запас на
			 *          опцию, которая понадобится узлам всех видов разом
			 *
			 * @see AUTO_RECONNECT - то же самое свойство для узла клиента
			 *
			 * \~english
			 * @brief Option of the independent continuation of the reading of a file
			 * @details An event of the file system with this option at a new reading continues
			 *          from the saved place: the offset in the file and the time of the last correction are not
			 *          reset, and the caller gets only what has been appended since the previous
			 *          time. Without the option the file is read from the beginning every time
			 * @note This is the **very same** value as `AUTO_RECONNECT`, and not a separate
			 *       option: their property is one — the engine continues the work of an event by itself
			 *       after it has been interrupted, — and only the outcomes by the kind of
			 *       the node differ. At a client the continuation results in a new connection, at a file —
			 *       in the reading up from the saved place
			 *       The second name is started for the sake of the caller, and not for the sake of the engine. He takes a node
			 *       of one kind at a time and sees the name that relates to his node:
			 *       a client has nothing to track, a file has nothing to reconnect to, and
			 *       a foreign name would confuse for no reason. The same device is used by
			 *       `htons` and `ntohs` — the action is one, the names are two, because
			 *       they are read in different places
			 * @warning A common value is obliged to remain such. Divide them by different
			 *          bits — and the last free one (`0x20`) will go, the only reserve for
			 *          an option that will be needed by the nodes of all the kinds at once
			 * @see AUTO_RECONNECT — the same property for a node of a client
			 *
			 * \~
			 */
			static constexpr uint16_t AUTO_FOLLOW = AUTO_RECONNECT;
			/**
			 * \~russian
			 * @brief Опция только IPv6 для сокета
			 *
			 * \~english
			 * @brief Option of the IPv6 only for a socket
			 *
			 * \~
			 */
			static constexpr uint16_t IPV6_ONLY = 0x20;
			/**
			 * \~russian
			 * @brief Опция отключения сигнала SIGPIPE
			 *
			 * \~english
			 * @brief Option of the switching off of the SIGPIPE signal
			 *
			 * \~
			 */
			static constexpr uint16_t NO_SIGPIPE = 0x40;
			/**
			 * \~russian
			 * @brief Опция повторного использования адреса
			 *
			 * \~english
			 * @brief Option of the reuse of the address
			 *
			 * \~
			 */
			static constexpr uint16_t REUSE_ADDR = 0x80;
			/**
			 * \~russian
			 * @brief Опция повторного использования порта
			 *
			 * \~english
			 * @brief Option of the reuse of the port
			 *
			 * \~
			 */
			static constexpr uint16_t REUSE_PORT = 0x100;
			/**
			 * \~russian
			 * @brief Опция получения метаданных дейтаграммного пакета
			 *
			 * \~english
			 * @brief Option of the getting of the metadata of a datagram packet
			 *
			 * \~
			 */
			static constexpr uint16_t DGRAM_INFO = 0x200;
			/**
			 * \~russian
			 * @brief Опция неблокирующего ввода-вывода
			 *
			 * \~english
			 * @brief Option of the non-blocking input-output
			 *
			 * \~
			 */
			static constexpr uint16_t NO_IO_BLOCK = 0x400;
			/**
			 * \~russian
			 * @brief Опция умного неблокирующего ввода-вывода
			 *
			 * \~english
			 * @brief Option of the smart non-blocking input-output
			 *
			 * \~
			 */
			static constexpr uint16_t SM_IO_BLOCK = 0x800;
			/**
			 * \~russian
			 * @brief Опция отключения алгоритма Нейгла
			 *
			 * \~english
			 * @brief Option of the switching off of the Nagle algorithm
			 *
			 * \~
			 */
			static constexpr uint16_t TCP_NO_DELAY = 0x1000;
			/**
			 * \~russian
			 * @brief Опция закрытия сокета при выполнении exec
			 *
			 * \~english
			 * @brief Option of the closing of the socket at the performance of exec
			 *
			 * \~
			 */
			static constexpr uint16_t CLOSE_ON_EXEC = 0x2000;
			/**
			 * \~russian
			 * @brief Опция включения мультикастовой петли
			 *
			 * \~english
			 * @brief Option of the switching on of the multicast loop
			 *
			 * \~
			 */
			static constexpr uint16_t MULTICAST_LOOPBACK = 0x4000;
			/**
			 * \~russian
			 * @brief Опция немедленного обрыва соединения при закрытии сокета
			 *
			 * @details Обычное закрытие соединения выполняется обменом прощаниями:
			 *          закрывающая сторона отправляет FIN, дожидается FIN от соседа
			 *          и подтверждает его. Сторона, закрывшаяся первой, обязана
			 *          после этого продержать четвёрку адресов в состоянии TIME_WAIT
			 *          вдвое дольше предельного времени жизни сегмента - это тридцать
			 *          секунд на большинстве систем. Держится она ради двух вещей:
			 *          поглотить запоздавшие дубликаты сегментов, чтобы они не
			 *          попали в новое соединение с той же четвёркой, и суметь
			 *          повторить подтверждение, если у соседа потерялось прощание.
			 *
			 *          Опция заменяет этот порядок на немедленный обрыв: соединение
			 *          рвётся сегментом RST, обмена прощаниями нет, сокет уходит
			 *          прямо в состояние CLOSED, и TIME_WAIT не возникает ни у одной
			 *          из сторон. Порт освобождается сразу.
			 *
			 *          Задумана она для двух случаев. Первый - стороне, открывающей
			 *          короткоживущие соединения потоком: каждое из них паркует
			 *          динамический порт на тридцать секунд, а пул таких портов
			 *          обыкновенно составляет шестнадцать тысяч, и на сколько-нибудь
			 *          заметном темпе он выбирается за считанные секунды. Второй -
			 *          серверу, сбрасывающему соединения намеренно: отказ по
			 *          превышению предела, обрыв незваного клиента. Платить за такой
			 *          сброс тридцатью секундами занятой четвёрки незачем.
			 *
			 * @note Опция небезопасна по построению и потому не включена по
			 *       умолчанию. Всё, что осталось в очереди отправки, отбрасывается
			 *       без попытки доставки. Сосед получает признак сброса соединения
			 *       вместо чистого конца потока, и приложение, ожидающее
			 *       упорядоченного завершения, увидит отказ. Наконец, снимается
			 *       и сама гарантия, ради которой существует TIME_WAIT: запоздавший
			 *       дубликат старого соединения может быть доставлен в новое с той
			 *       же четвёркой адресов
			 *
			 * @note Разряд последний: поле опций события шестнадцатиразрядное и
			 *       этой опцией исчерпывается. Следующая потребует его расширения
			 *
			 * \~english
			 * @brief Option of the immediate break of the connection at the closing of the socket
			 * @details The ordinary closing of a connection is performed by an exchange of the farewells:
			 *          the closing side sends a FIN, waits for a FIN from the neighbour
			 *          and acknowledges it. The side that has closed first is obliged
			 *          after that to hold the quadruple of the addresses in the TIME_WAIT state
			 *          twice as long as the maximum lifetime of a segment — this is thirty
			 *          seconds at most of the systems. It is held for the sake of two things: to
			 *          absorb the belated duplicates of the segments, so that they would not
			 *          get into a new connection with the same quadruple, and to be able to
			 *          repeat the acknowledgement if the neighbour has lost the farewell.
			 *          The option replaces this order with an immediate break: the connection
			 *          is torn by an RST segment, there is no exchange of the farewells, the socket goes
			 *          right into the CLOSED state, and TIME_WAIT arises at neither
			 *          of the sides. The port is released at once.
			 *          It is conceived for two cases. The first one — for a side opening
			 *          the short-lived connections in a stream: each of them parks
			 *          a dynamic port for thirty seconds, and the pool of such ports
			 *          usually makes up sixteen thousand, and at any noticeable
			 *          rate it is exhausted in mere seconds. The second one — for
			 *          a server resetting the connections deliberately: a refusal by
			 *          the excess of a limit, a break of an uninvited client. To pay for such
			 *          a reset by thirty seconds of an occupied quadruple is pointless.
			 * @note The option is unsafe by construction and is therefore not switched on by
			 *       default. Everything that has remained in the queue of the sending is discarded
			 *       without an attempt of the delivery. The neighbour receives a sign of a reset of the connection
			 *       instead of a clean end of the stream, and an application expecting
			 *       an ordered completion will see a refusal. Finally, the very guarantee
			 *       is removed for the sake of which TIME_WAIT exists: a belated
			 *       duplicate of an old connection may be delivered into a new one with the
			 *       same quadruple of the addresses
			 * @note The digit is the last one: the field of the options of an event is a sixteen bit one and
			 *       is exhausted by this option. The next one will require its widening
			 *
			 * \~
			 */
			static constexpr uint16_t HARD_CLOSE = 0x8000;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../sys/macro_pop.hpp"

#endif // __AWH_EVENTS__
