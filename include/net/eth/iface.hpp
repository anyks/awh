/**
 * @file iface.hpp
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
 * @brief Заголовочный файл модуля работы с сетевыми интерфейсами —
 *        пространство имён eth::iface для перечисления интерфейсов машины, получения их адресов, флагов, MTU и состояния,
 *        а также создания и настройки TUN/TAP-устройств
 *
 * \~english
 * @brief Header file of the module of working with the network interfaces —
 *        the eth::iface namespace for enumerating the interfaces of the machine, getting their addresses, flags, MTU and state,
 *        as well as for creating and setting up the TUN/TAP devices
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/macro/global.hpp"

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
		 * @brief Пространство имён для работы с сетевым интерфейсом
		 *
		 * @details Ведает сетевыми устройствами машины: перечисляет их, читает и
		 *          правит их настройки, а также заводит устройства свои - туннельные,
		 *          каких в системе изначально нет.
		 *
		 *          Занятия делятся надвое. Чтение - перечислить устройства, узнать
		 *          адрес, наибольший размер пакета, признаки состояния - обходится
		 *          обычными правами. Правка же - завести устройство, выставить адрес,
		 *          поднять его - требует надзорных прав.
		 *
		 * @warning Правка меняет настройки **всей машины** и переживает
		 *          завершение процесса. Заведённое туннельное устройство следует
		 *          убирать за собой, иначе оно останется висеть в системе
		 *
		 * \~english
		 * @brief Namespace for working with a network interface
		 *
		 * @details Is in charge of the network devices of the machine: enumerates them, reads and
		 *          corrects their settings, and also starts devices of its own — the tunnel ones,
		 *          which are initially absent in the system.
		 *
		 *          The occupations are divided in two. The reading — to enumerate the devices, to find out
		 *          the address, the largest size of a packet, the signs of the state — gets by with
		 *          the ordinary rights. The correction, though — to start a device, to set out an address,
		 *          to bring it up — requires supervisory rights.
		 *
		 * @warning The correction changes the settings of the **whole machine** and outlives
		 *          the completion of the process. A started tunnel device should be
		 *          removed after oneself, otherwise it will remain hanging in the system
		 *
		 * \~
		 */
		namespace iface {
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64) || defined(DOXYGEN)
			/**
			 * \~russian
			 * @brief Драйвер, каким заводятся туннельные устройства
			 *
			 * @details Встроенного туннельного устройства у MS Windows нет вовсе,
			 *          и приносит его сторонний драйвер. Драйверов этих два, и различие
			 *          между ними не в одной скорости.
			 *
			 *          **Wintun** переносит лишь пакеты сетевого уровня и работает через
			 *          кольцо в общей с драйвером памяти - быстрее, но мостом не служит.
			 *
			 *          **tap-windows6** переносит кадры канального уровня вместе с
			 *          аппаратными адресами и отдаёт обыкновенный дескриптор файла.
			 *          Медленнее, зато годится и туда, где нужен мост.
			 *
			 * @note У прочих систем понятия этого нет: туннель там даёт само ядро,
			 *       и выбирать не из чего. Оттого настройка эта заведена только здесь
			 *
			 * \~english
			 * @brief The driver the tunnel devices are started by
			 *
			 * @details MS Windows has no built-in tunnel device at all,
			 *          and a third-party driver brings it. There are two of these drivers, and the difference
			 *          between them is not in the speed alone.
			 *
			 *          **Wintun** carries only the packets of the network level and works through
			 *          a ring in the memory shared with the driver — faster, but does not serve as a bridge.
			 *
			 *          **tap-windows6** carries the frames of the link level together with
			 *          the hardware addresses and gives back an ordinary file descriptor.
			 *          Slower, but is fit for where a bridge is needed as well.
			 *
			 * @note The other systems have no such notion: a tunnel there is given by the kernel itself,
			 *       and there is nothing to choose from. Therefore this setting is started only here
			 *
			 * \~
			 */
			enum class driver_t : uint8_t {
				AUTO   = 0x00, // Выбор драйвера по виду устройства и его доступности
				WINTUN = 0x01, // Кольцо в общей памяти, только сетевой уровень
				TAP    = 0x02  // Дескриптор файла, канальный уровень с адресами
			};
		#endif
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64) || defined(DOXYGEN)
			/**
			 * \~russian
			 * @brief Функция получения драйвера туннельных устройств
			 *
			 * @return драйвер, каким заводятся туннельные устройства
			 *
			 * \~english
			 * @brief Function of getting the driver of the tunnel devices
			 *
			 * @return the driver the tunnel devices are started by
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ driver_t driver() noexcept;
			/**
			 * \~russian
			 * @brief Функция установки драйвера туннельных устройств
			 *
			 * @details Задаёт, каким драйвером заводить туннельные устройства.
			 *          Значение AUTO, установленное изначально, поручает выбор самому
			 *          модулю: он берёт тот драйвер, что и доступен, и способен перенести
			 *          запрошенный вид устройства, а при наличии обоих - более
			 *          работоспособный.
			 *
			 * @note Заданный явно драйвер подмене не подлежит: если он на машине
			 *       недоступен либо запрошенного вида устройства не переносит,
			 *       заведение отвечает отказом с записью о причине в журнал
			 *
			 * @param driver драйвер туннельных устройств для установки
			 *
			 * \~english
			 * @brief Function of setting the driver of the tunnel devices
			 *
			 * @details Sets by which driver the tunnel devices should be started.
			 *          The value AUTO, set initially, entrusts the choice to the module
			 *          itself: it takes the driver that is both available and capable of carrying
			 *          the requested kind of the device, and at the presence of both — the more
			 *          operable one.
			 *
			 * @note A driver set explicitly is not subject to a substitution: if it is unavailable
			 *       on the machine or does not carry the requested kind of the device,
			 *       the starting answers with a refusal with a record about the reason into the log
			 *
			 * @param driver driver of the tunnel devices to set
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void driver(const driver_t driver) noexcept;
		#endif
		/**
		 * \~russian
		 * @brief Для операционной системы MS Windows, для систем Sun Microsystems и для macOS
		 *
		 * @details Прочие системы BSD снимают адрес вместе с самим устройством, и
		 *          отдельного снятия им не нужно. У macOS устройство `utun` снести
		 *          нечем - оно заводится управляющим сокетом ядра и исчезает само,
		 *          но НЕ СРАЗУ, - и адрес всё это время числится за мертвецом.
		 *
		 * \~english
		 * @brief For the Microsoft Windows operating system, for the Sun Microsystems systems and for macOS
		 *
		 * @details The other BSD systems remove the address together with the device itself, and
		 *          a separate removal is not needed by them. On macOS the `utun` device has nothing
		 *          to remove it with — it is started by the control socket of the kernel and disappears on its own,
		 *          but NOT AT ONCE — and the address all that time is counted as belonging to the dead one.
		 *
		 * \~
		 */
		#if defined(_WIN32) || defined(_WIN64) || defined(__sun__) || defined(__APPLE__) || defined(DOXYGEN)
			/**
			 * \~russian
			 * @brief Функция снятия адреса сетевого устройства и пути к тому концу связи
			 *
			 * @details Снимает то, что поставила установка адреса: сам адрес и проложенный
			 *          к тому концу связи путь.
			 *
			 * @note Функция заведена для MS Windows, для систем Sun и для macOS, и отсутствие
			 *       её у прочих систем - не пробел. У Linux и у прочих систем BSD устройство
			 *       туннеля исчезает вместе с процессом, унося адреса с собою, и снимать
			 *       нечего. Этим же трём системам устройство либо НЕ ПРИНАДЛЕЖИТ, либо
			 *       исчезает не сразу: у MS Windows устройство драйвера tap-windows6 живёт
			 *       в системе постоянно и приложением не сносится, у Sun Solaris и illumos
			 *       связь канального уровня заводится административно (`dladm`) и движком
			 *       не сносится тем более, а у macOS устройство `utun` снести нечем - оно
			 *       заводится управляющим сокетом ядра и исчезает само, но НЕ СРАЗУ. Адрес
			 *       на них ставил движок - убрать его за собою обязан он же
			 *
			 * @note Функция объявлена в общем договоре, а нужна не всякой системе - и это в
			 *       порядке вещей: слои этих систем отдельны и с прочими не пересекаются.
			 *       Решение от 21.08.2026
			 *
			 * @warning Без снятия прогон отравляет следующий. У MS Windows отравляет
			 *          ЧЕРЕЗ ГРАНИЦУ ДРАЙВЕРА: оставшийся адрес числится за прежним
			 *          устройством, система отвечает новому «объект уже существует», и
			 *          устройство остаётся без адреса вовсе. У систем Sun связь переживает
			 *          процесс вместе с чужим адресом, и следующий прогон получает её
			 *          занятой - заведение отвечает отказом «Not a data message», отчего
			 *          красным становится КАЖДЫЙ ВТОРОЙ прогон
			 *
			 * @param name название сетевого устройства
			 * @param ip   снимаемый адрес
			 * @param peer адрес того конца связи, путь к которому снимается
			 * @return     результат выполнения снятия
			 *
			 * \~english
			 * @brief Function of the removal of the address of the network interface and of the route to the peer
			 *
			 * @details Removes what the setting of the address has put: the address itself and the route laid
			 *          to that end of the connection.
			 *
			 * @note The function exists for MS Windows, for the Sun systems and for macOS, and its absence
			 *       in the other systems is not a gap. In Linux and in the other BSD systems the tunnel
			 *       device disappears together with the process, taking its addresses with it, and there is
			 *       nothing to remove. For these same three systems the device either does NOT BELONG
			 *       to them, or disappears not at once: in MS Windows the tap-windows6 driver device lives
			 *       in the system permanently and is not removed by the application, in Sun Solaris and illumos
			 *       the link of the link level is created administratively (`dladm`) and is not removed by the engine
			 *       even more so, and on macOS the `utun` device has nothing to remove it with — it
			 *       is started by the control socket of the kernel and disappears on its own, but NOT AT ONCE. The address
			 *       on them was put by the engine — to remove it after itself is its own duty
			 *
			 * @note The function is declared in the common contract, while it is needed not by every system — and this is
			 *       in the order of things: the layers of these systems are separate and do not intersect with the others.
			 *       Decision of 21.08.2026
			 *
			 * @warning Without the removal a run poisons the next one. In MS Windows it poisons
			 *          ACROSS THE DRIVER BOUNDARY: the remaining address is counted as belonging to the previous
			 *          device, the system answers the new one «the object already exists», and
			 *          the device remains without an address at all. In the Sun systems the link outlives the
			 *          process together with the foreign address, and the next run gets it
			 *          occupied — the creation answers with a «Not a data message» refusal, which is why
			 *          EVERY SECOND run turns red
			 *
			 * @param name name of the network interface
			 * @param ip   address to remove
			 * @param peer address of the peer whose route is removed
			 * @return     result of the removal
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool delAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer) noexcept;
		#endif
			/**
			 * \~russian
			 * @brief Функция установки безопасности работы потоков
			 *
			 * @details Включает защиту учёта связей, розданных туннелям процесса, от
			 *          одновременного обращения из разных потоков.
			 *
			 * @warning Настройка эта **общая на весь процесс**, а не своя у каждого
			 *          вызова: выставленная однажды, она действует на все. По
			 *          умолчанию защита **выключена** - в расчёте на однопоточную работу, -
			 *          и включать её следует до того, как заработает второй поток
			 *
			 * @note Защищать на разных системах приходится РАЗНОЕ, и там, где общего
			 *       учёта у модуля нет вовсе, вызов ничего не делает. Настройка от этого не
			 *       теряет смысла: она общая для всех систем, а не своя у каждой
			 *
			 * @param mode флаг режима безопасности потоков
			 *
			 * \~english
			 * @brief Function of setting the thread safety of the work
			 *
			 * @details Switches on the protection of the registry of links handed out to the tunnels of the
			 *          process from a simultaneous address from different threads.
			 *
			 * @warning This setting is a **common one for the whole process**, and not its own for every
			 *          call: once set, it is in force for all of them. By
			 *          default the protection is **switched off** — with the reckoning on a single-threaded work, —
			 *          and it should be switched on before the second thread starts working
			 *
			 * @note What has to be protected differs from system to system, and where the module keeps no
			 *       common registry at all, the call does nothing. The setting does not lose its meaning because of
			 *       that: it is common for all the systems, and not its own for each of them
			 *
			 * @param mode flag of the thread safety mode
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void threadSafety(const bool mode) noexcept;
			/**
			 * \~russian
			 * @brief Функция удаления сетевого интерфейса
			 *
			 * @details Убирает устройство из системы.
			 *
			 * @warning Требует надзорных прав. Подключения, шедшие через это
			 *          устройство, оборвутся
			 *
			 * @param name имя сетевого интерфейса
			 * @return     результат удаления сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of removing a network interface
			 *
			 * @details Removes a device from the system.
			 *
			 * @warning Requires supervisory rights. The connections that went through this
			 *          device will be broken
			 *
			 * @param name name of the network interface
			 * @return     result of the removal of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool destroy(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения списка сетевых интерфейсов системы
			 *
			 * @details Перечисляет устройства машины вместе с их настройками.
			 *
			 * @note В список попадают все устройства, включая петлевое и
			 *       неподнятые: отбирать нужные следует по их признакам
			 *
			 * @return список сетевых интерфейсов системы
			 *
			 * \~english
			 * @brief Function of getting the list of the network interfaces of the system
			 *
			 * @details Enumerates the devices of the machine together with their settings.
			 *
			 * @note All the devices get into the list, including the loopback one and
			 *       the ones not brought up: the needed ones should be selected by their signs
			 *
			 * @return list of the network interfaces of the system
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ unordered_set <string> available() noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки доступности сетевого интерфейса
			 *
			 * @param name имя сетевого интерфейса
			 * @return     результат проверки доступности сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of checking the availability of a network interface
			 *
			 * @param name name of the network interface
			 * @return     result of the check of the availability of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isAvailable(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки туннельного сетевого интерфейса
			 *
			 * @param name имя сетевого интерфейса
			 * @return     результат проверки туннельного сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of checking a tunnel network interface
			 *
			 * @param name name of the network interface
			 * @return     result of the check of the tunnel network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isTunnel(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки туннельного сетевого интерфейса по адресу
			 *
			 * @param addr адрес сетевого подключения
			 * @return     результат проверки туннельного сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of checking a tunnel network interface by an address
			 *
			 * @param addr address of the network connection
			 * @return     result of the check of the tunnel network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isTunnel(const net::addr_t * addr) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки виртуального сетевого интерфейса
			 *
			 * @param name имя сетевого интерфейса
			 * @return     результат проверки виртуального сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of checking a virtual network interface
			 *
			 * @param name name of the network interface
			 * @return     result of the check of the virtual network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isVirtual(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки виртуального сетевого интерфейса по адресу
			 *
			 * @param addr адрес сетевого подключения
			 * @return     результат проверки виртуального сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of checking a virtual network interface by an address
			 *
			 * @param addr address of the network connection
			 * @return     result of the check of the virtual network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool isVirtual(const net::addr_t * addr) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения имени сетевого интерфейса по адресу
			 *
			 * @details Отыскивает, какому устройству принадлежит заданный адрес.
			 *
			 * @note Пустая строка означает, что адрес ни за одним устройством не
			 *       закреплён, - ошибкой это не считается
			 *
			 * @param addr адрес сетевого подключения
			 * @return     имя сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of getting the name of a network interface by an address
			 *
			 * @details Finds which device the given address belongs to.
			 *
			 * @note An empty string means that the address is fastened to no device, —
			 *       this is not considered an error
			 *
			 * @param addr address of the network connection
			 * @return     name of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ string name(const net::addr_t * addr) noexcept;
			/**
			 * \~russian
			 * @brief Функция создания сетевого интерфейса
			 *
			 * @details Заводит в системе новое устройство - туннельное либо иного
			 *          вида - и отдаёт дескриптор для обмена через него.
			 *
			 * @note Имя здесь и входное, и выходное: пустое или содержащее образец
			 *       система дополнит сама, вписав выбранное обратно. Обращаться к
			 *       устройству следует по вписанному имени, а не по запрошенному
			 *
			 * @warning Требует надзорных прав. Устройство живёт, пока открыт
			 *          дескриптор, и исчезает с его закрытием - на некоторых системах,
			 *          однако, остаётся, и убирать его приходится отдельно
			 *
			 * @param type тип сетевого интерфейса
			 * @param name имя сетевого интерфейса
			 * @return     дескриптор созданного сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of creating a network interface
			 *
			 * @details Starts a new device in the system — a tunnel one or of another
			 *          kind — and gives back a descriptor for the exchange through it.
			 *
			 * @note The name here is both an input and an output one: an empty one or one containing a pattern
			 *       the system will fill up by itself, writing the chosen one back. The device
			 *       should be addressed by the written name, and not by the requested one
			 *
			 * @warning Requires supervisory rights. The device lives while the descriptor is open,
			 *          and disappears with its closing — on some systems,
			 *          however, it remains, and has to be removed separately
			 *
			 * @param type type of the network interface
			 * @param name name of the network interface
			 * @return     descriptor of the created network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ net::socket_t create(const event::eth_t type, string & name) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения MTU сетевого интерфейса
			 *
			 * @param name имя сетевого интерфейса
			 * @return     MTU сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of getting the MTU of a network interface
			 *
			 * @param name name of the network interface
			 * @return     MTU of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint32_t mtu(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки MTU сетевого интерфейса
			 *
			 * @details Задаёт наибольший размер пакета, проходящего через
			 *          устройство без дробления.
			 *
			 * @warning Требует надзорных прав. Размер сверх того, что позволяет
			 *          оборудование, будет отвергнут, а заниженный урежет пропускную
			 *          способность всего, что идёт через устройство
			 *
			 * @param name имя сетевого интерфейса
			 * @param mtu  размер MTU интерфейса
			 * @return     результат установки MTU сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of setting the MTU of a network interface
			 *
			 * @details Sets the largest size of a packet passing through
			 *          the device without a fragmentation.
			 *
			 * @warning Requires supervisory rights. A size beyond what the equipment
			 *          allows will be rejected, and an understated one will cut down the bandwidth
			 *          of everything that goes through the device
			 *
			 * @param name name of the network interface
			 * @param mtu  size of the MTU of the interface
			 * @return     result of the setting of the MTU of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool mtu(string_view name, const uint32_t mtu) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения установленных флагов сетевого интерфейса
			 *
			 * @param name имя сетевого интерфейса
			 * @return     флаги сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of getting the set flags of a network interface
			 *
			 * @param name name of the network interface
			 * @return     flags of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ unordered_set <event::eth_flag_t> flags(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки флага сетевого интерфейса
			 *
			 * @param name имя сетевого интерфейса
			 * @param flag флаг сетевого интерфейса
			 * @param mode режим включения/выключения флага
			 * @return     результат установки флага сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of setting a flag of a network interface
			 *
			 * @param name name of the network interface
			 * @param flag flag of the network interface
			 * @param mode mode of the switching on/off of the flag
			 * @return     result of the setting of the flag of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool flag(string_view name, const event::eth_flag_t flag, const event::mode_t mode) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки IP-адреса на сетевой интерфейс
			 *
			 * @details Закрепляет адрес за устройством вместе с длиной префикса сети.
			 *
			 * @warning Требует надзорных прав и действует на всю машину. Адрес
			 *          переживает завершение процесса
			 *
			 * @param name   имя сетевого интерфейса
			 * @param ip     адрес сетевого интерфейса для установки
			 * @param peer   адрес удалённого пира (для точка-точка)
			 * @param prefix префикс подсети
			 * @return       результат установки IP-адреса
			 *
			 * \~english
			 * @brief Function of setting an IP address on a network interface
			 *
			 * @details Fastens an address to a device together with the length of the prefix of the network.
			 *
			 * @warning Requires supervisory rights and is in force for the whole machine. The address
			 *          outlives the completion of the process
			 *
			 * @param name   name of the network interface
			 * @param ip     address of the network interface to set
			 * @param peer   address of the remote peer (for point-to-point)
			 * @param prefix prefix of the subnet
			 * @return       result of the setting of the IP address
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool setAddress(string_view name, const net::addr_t * ip, const uint8_t prefix) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения IP-адреса сетевого интерфейса
			 *
			 * @param name   имя сетевого интерфейса
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @return       IP-адрес сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of getting the IP address of a network interface
			 *
			 * @param name   name of the network interface
			 * @param family family of the protocols (IPv4 or IPv6)
			 * @return       IP address of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ unique_ptr <net::addr_t> getAddress(string_view name, const event::family_t family) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки параметров сетевого интерфейса точка-точка
			 *
			 * @param name   имя сетевого интерфейса
			 * @param ip     адрес сетевого интерфейса для установки
			 * @param peer   адрес удалённого пира (для точка-точка)
			 * @param prefix префикс подсети
			 * @return       результат установки параметров сетевого интерфейса точка-точка
			 *
			 * \~english
			 * @brief Function of setting the parameters of a point-to-point network interface
			 *
			 * @param name   name of the network interface
			 * @param ip     address of the network interface to set
			 * @param peer   address of the remote peer (for point-to-point)
			 * @param prefix prefix of the subnet
			 * @return       result of the setting of the parameters of the point-to-point network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool setAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix) noexcept;
			/**
			 * \~russian
			 * @brief Функция изменения параметров сетевого интерфейса точка-точка
			 *
			 * @param name   имя сетевого интерфейса
			 * @param ip     адрес сетевого интерфейса для получения
			 * @param peer   адрес удалённого пира (для точка-точка)
			 * @param prefix префикс подсети
			 * @return       результат изменения параметров сетевого интерфейса точка-точка
			 *
			 * \~english
			 * @brief Function of changing the parameters of a point-to-point network interface
			 *
			 * @param name   name of the network interface
			 * @param ip     address of the network interface to get
			 * @param peer   address of the remote peer (for point-to-point)
			 * @param prefix prefix of the subnet
			 * @return       result of the change of the parameters of the point-to-point network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool getAddress(string_view name, unique_ptr <net::addr_t> & ip, unique_ptr <net::addr_t> & peer, uint8_t & prefix) noexcept;
			/**
			 * \~russian
			 * @brief Функция комплексной настройки сетевого интерфейса (адрес + MTU + поднятие) за один управляющий сокет
			 *
			 * @details Делает за один вызов то, на что иначе ушло бы три:
			 *          выставляет адрес, задаёт наибольший размер пакета и поднимает
			 *          устройство. Смысл не в удобстве, а в цене - все три настройки
			 *          проходят через один управляющий сокет вместо трёх.
			 *
			 * @note Нулевой размер пакета означает «не трогать», а не «обнулить»:
			 *       устройство сохранит прежний
			 *
			 * @warning Требует надзорных прав. Настройка неделимой не является: при
			 *          отказе на середине часть настроек уже применена, и устройство
			 *          останется настроенным наполовину
			 *
			 * @param name   имя сетевого интерфейса
			 * @param ip     адрес сетевого интерфейса для установки
			 * @param prefix префикс подсети
			 * @param mtu    размер MTU интерфейса (0 - не изменять)
			 * @return       результат комплексной настройки сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of the complex setup of a network interface (address + MTU + bringing up) through one control socket
			 *
			 * @details Does in one call what would otherwise take three:
			 *          sets out the address, sets the largest size of a packet and brings
			 *          the device up. The point is not in the convenience, but in the price — all three settings
			 *          go through one control socket instead of three.
			 *
			 * @note A zero size of a packet means «do not touch», and not «zero out»:
			 *       the device will preserve the previous one
			 *
			 * @warning Requires supervisory rights. The setup is not an atomic one: at
			 *          a refusal in the middle a part of the settings is already applied, and the device
			 *          will remain set up halfway
			 *
			 * @param name   name of the network interface
			 * @param ip     address of the network interface to set
			 * @param prefix prefix of the subnet
			 * @param mtu    size of the MTU of the interface (0 — do not change)
			 * @return       result of the complex setup of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool configure(string_view name, const net::addr_t * ip, const uint8_t prefix, const uint32_t mtu = 0) noexcept;
			/**
			 * \~russian
			 * @brief Функция комплексной настройки сетевого интерфейса точка-точка (адрес + пир + MTU + поднятие) за один управляющий сокет
			 *
			 * @details То же, что и настройка обычного устройства, но для связи
			 *          точка-точка, где помимо своего адреса задаётся ещё и адрес того
			 *          конца. Так настраиваются туннели.
			 *
			 * @note Нулевой размер пакета означает «не трогать». Для туннеля его
			 *       стоит задавать осознанно: оболочка отнимает часть места, и размер по
			 *       умолчанию окажется велик
			 *
			 * @param name   имя сетевого интерфейса
			 * @param ip     адрес сетевого интерфейса для установки
			 * @param peer   адрес удалённого пира (для точка-точка)
			 * @param prefix префикс подсети
			 * @param mtu    размер MTU интерфейса (0 - не изменять)
			 * @return       результат комплексной настройки сетевого интерфейса
			 *
			 * \~english
			 * @brief Function of the complex setup of a point-to-point network interface (address + peer + MTU + bringing up) through one control socket
			 *
			 * @details The same as the setup of an ordinary device, but for a point-to-point
			 *          connection, where besides one's own address the address of that
			 *          end is set as well. That is how the tunnels are set up.
			 *
			 * @note A zero size of a packet means «do not touch». For a tunnel it
			 *       is worth setting deliberately: the wrapping takes away a part of the room, and the size by
			 *       default will turn out to be large
			 *
			 * @param name   name of the network interface
			 * @param ip     address of the network interface to set
			 * @param peer   address of the remote peer (for point-to-point)
			 * @param prefix prefix of the subnet
			 * @param mtu    size of the MTU of the interface (0 — do not change)
			 * @return       result of the complex setup of the network interface
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool configure(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix, const uint32_t mtu = 0) noexcept;
		};
	};
};
