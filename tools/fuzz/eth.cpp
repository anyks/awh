/**
 * @file eth.cpp
 * @date 2026-08-29
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
 * @brief Инструмент фаззинга модуля работы с сетевыми устройствами — подача договору
 *        `awh::eth_t` случайных доводов: имён устройств из мусорных октетов, адресов
 *        любой длины и содержимого, описателей негодных, закрытых и чужого рода,
 *        семейств и наречий в несочетаемых парах — для поиска аварийных завершений,
 *        выходов за границы буфера и утечек описателей
 *
 * @details Ворошитель этот устроен по образцу соседнего `io.cpp`: поверяется не разбор
 *          входного потока, а обхождение с ДОВОДАМИ. Договор `eth_t` тем и отличается
 *          от договора движка, что почти всякий его метод принимает довод, пришедший
 *          снаружи, - имя устройства от потребителя, описатель от него же, адрес из
 *          разобранного текста, - и обязан устоять на любом из них
 *
 * @warning МАШИНЫ ВОРОШИТЕЛЬ НЕ ТРОГАЕТ. Изменяющие методы - `iface.setAddress`,
 *          `iface.delAddress`, `iface.configure`, `iface.destroy`, `iface.mtu` с
 *          доводом, `iface.flag`, `gateway.add`, `gateway.remove` - исключены НАМЕРЕННО
 *          и в перечни не входят вовсе. Они правят настройку хозяйской машины, а не
 *          состояние объекта, и ворошить ими нельзя ни с надзорными правами, ни без
 *          них. Спрашивающие методы того же объекта ворошатся все
 *
 * @warning Отказы вызовов ворошителем НЕ считаются находкой: отвечать отказом на
 *          негодный довод - это и есть правильное обхождение договора. Находкой
 *          считается лишь то, что отказом сообщить нельзя: аварийное завершение,
 *          порча памяти, утечка описателей
 *
 * @note Надзорных прав ворошителю не нужно. Сокеты он заводит себе сам и сам же их
 *       закрывает, а спрашивающие методы устройств прав не требуют нигде
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <random>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>

/**
 * Если операционной системой является не MS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
/**
 * Если операционной системой является MS Windows
 */
#else
	#include <io.h>
	#include <fcntl.h>
	#include <sys/win32.hpp>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/eth/eth.hpp>

/**
 * Подключаем пространство имён
 */
using namespace std;

/**
 * @brief Функция получения объекта фреймворка
 *
 * @note Объект заводится функционально-статическим намеренно, а не на уровне файла:
 *       порядок построения статических объектов между единицами трансляции не задан
 *
 * @return объект фреймворка
 *
 */
static const awh::fmk_t * framework() noexcept {
	// Объект фреймворка
	static awh::fmk_t result;
	// Выводим объект фреймворка
	return &result;
}
/**
 * @brief Функция получения объекта работы с логами
 *
 * @return объект работы с логами
 *
 */
static const awh::log_t * logger() noexcept {
	// Объект работы с логами
	static awh::log_t result(::framework());
	// Снимаем вывод журнала: ворошитель делает десятки тысяч заведомо отказных вызовов,
	// и журнал их обращает в гигабайты шума, за каким находки не видно
	const_cast <awh::log_t *> (&result)->level(awh::log_t::level_t::NONE);
	// Выводим объект работы с логами
	return &result;
}
/**
 * @brief Функция снятия случайного числа из промежутка
 *
 * @param engine источник случайных чисел
 * @param bound  верхняя граница промежутка, не включая её саму
 * @return       случайное число из промежутка
 *
 */
static uint64_t pick(mt19937_64 & engine, const uint64_t bound) noexcept {
	// Если граница вырождена, выбирать не из чего
	if(bound == 0)
		// Выводим ноль
		return 0;
	// Выводим случайное число из промежутка
	return (engine() % bound);
}

/**
 * @brief Итоги работы ворошителя
 *
 */
static struct Totals {
	// Число сделанных проходов
	uint64_t rounds;
	// Число вызовов методов устройств
	uint64_t ifaces;
	// Число вызовов методов адресов
	uint64_t addrs;
	// Число вызовов методов сокетов
	uint64_t sockets;
	// Число вызовов методов шлюзов
	uint64_t gateways;
	// Число заведённых пробных сокетов
	uint64_t issued;
	// Число вызовов с заведомо негодным описателем
	uint64_t invalids;
	// Число вызовов методов SCTP
	uint64_t sctps;
} totals = {0, 0, 0, 0, 0, 0, 0, 0};

/**
 * Семейства адресов, какие ворошитель подаёт
 *
 * @note Семейство NONE входит намеренно: договор обязан отвечать отказом, а не падать
 */
static const awh::event::family_t FAMILIES[] = {
	awh::event::family_t::NONE,
	awh::event::family_t::IPV4,
	awh::event::family_t::IPV6,
	awh::event::family_t::UDS,
	awh::event::family_t::TIMER
};
/**
 * Наречия, какие ворошитель подаёт
 */
static const awh::event::protocol_t PROTOCOLS[] = {
	awh::event::protocol_t::NONE,
	awh::event::protocol_t::TCP,
	awh::event::protocol_t::UDP,
	awh::event::protocol_t::SCTP
};
/**
 * Режимы доставки, какие ворошитель подаёт
 */
static const awh::event::delivery_mode_t DELIVERIES[] = {
	awh::event::delivery_mode_t::NONE,
	awh::event::delivery_mode_t::UNICAST,
	awh::event::delivery_mode_t::MULTICAST,
	awh::event::delivery_mode_t::BROADCAST
};
/**
 * Имена сетевых устройств, какие ворошитель подаёт
 *
 * @details Часть имён настоящая, часть заведомо негодная. Настоящие нужны, чтобы
 *          договор доходил до работы с системой, а не отсекал довод на первой же
 *          проверке: на одних лишь негодных именах весь код за проверкой остался бы
 *          неворошённым
 */
static const char * IFACES[] = {
	"lo", "lo0", "eth0", "en0", "em0", "wlan0", "tun0", "utun0",
	"", " ", "0", "-", ".", "..", "/", "\\", ":", "\t", "\n",
	"lo\0hidden", "оно", "LO0", "lo0 ", " lo0",
	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
	"%s", "%n", "../../etc/passwd", "\xff\xfe\xfd", "\x7f", "lo0;reboot"
};
/**
 * Ворошение SCTP берётся лишь у систем, где протокол есть
 *
 * @note Список тот же, что и у самой библиотеки (include/net/eth/eth.hpp): у прочих
 *       систем объекта `sctp` нет вовсе, и обращение к нему не собралось бы
 */
#if __linux__ || __FreeBSD__ || __sun
	/**
	 * Возможности SCTP, о каких ворошитель спрашивает
	 */
	static const awh::net::sctp::feature_t FEATURES[] = {
		awh::net::sctp::feature_t::NONE,
		awh::net::sctp::feature_t::AUTHENTICATION,
		awh::net::sctp::feature_t::STREAM_RESET,
		awh::net::sctp::feature_t::ASSOC_RESET,
		awh::net::sctp::feature_t::STREAM_CHANGE,
		awh::net::sctp::feature_t::SENDER_DRY,
		awh::net::sctp::feature_t::MULTIHOMING,
		awh::net::sctp::feature_t::PARTIAL_MESSAGE
	};
	/**
	 * Виды сроков SCTP, какие ворошитель подаёт
	 */
	static const awh::net::sctp::timeout_t TIMEOUTS[] = {
		awh::net::sctp::timeout_t::NONE,
		awh::net::sctp::timeout_t::INIT,
		awh::net::sctp::timeout_t::DATA,
		awh::net::sctp::timeout_t::SACK,
		awh::net::sctp::timeout_t::SHUTDOWN,
		awh::net::sctp::timeout_t::HEARTBEAT,
		awh::net::sctp::timeout_t::COOKIE,
		awh::net::sctp::timeout_t::SHUTDOWNACK
	};
	/**
	 * Виды проверки подлинности SCTP, какие ворошитель подаёт
	 */
	static const awh::net::sctp::auth_type_t AUTHS[] = {
		awh::net::sctp::auth_type_t::HMAC_RSVD,
		awh::net::sctp::auth_type_t::HMAC_SHA1,
		awh::net::sctp::auth_type_t::HMAC_SHA256
	};
#endif

/**
 * @brief Функция снятия имени сетевого устройства
 *
 * @details Половина имён берётся из перечня, половина собирается из случайных октетов:
 *          перечень покрывает виды негодности, о каких мы догадались сами, а случайные
 *          октеты - те, о каких не догадались
 *
 * @param engine источник случайных чисел
 * @return       имя сетевого устройства
 *
 */
static string interface(mt19937_64 & engine) noexcept {
	// Если выпало брать имя из перечня
	if(::pick(engine, 2) == 0)
		// Выводим имя из перечня
		return string(IFACES[::pick(engine, sizeof(IFACES) / sizeof(IFACES[0]))]);
	// Длина собираемого имени: ноль тоже возможен намеренно
	const size_t length = static_cast <size_t> (::pick(engine, 40));
	// Собираемое имя устройства
	string result(length, '\0');
	/**
	 * Собираем имя устройства из случайных октетов
	 */
	for(size_t i = 0; i < length; i++)
		// Записываем очередной октет имени
		result[i] = static_cast <char> (::pick(engine, 256));
	// Выводим собранное имя устройства
	return result;
}
/**
 * @brief Функция сборки случайного сетевого адреса
 *
 * @param engine источник случайных чисел
 * @return       собранный сетевой адрес
 *
 */
static unique_ptr <awh::net::addr_t> address(mt19937_64 & engine) noexcept {
	/**
	 * Определяем вид собираемого адреса
	 */
	switch(static_cast <uint8_t> (::pick(engine, 3))){
		// Если собирается адрес IPv4
		case 0: {
			// Заводим адрес IPv4
			unique_ptr <awh::net::addr_net_ipv4_t> result = make_unique <awh::net::addr_net_ipv4_t> ();
			// Заполняем адрес случайным значением
			result->address = static_cast <uint32_t> (engine());
			// Задаём префикс сети: значения свыше тридцати двух подаются намеренно
			result->prefix = static_cast <uint8_t> (::pick(engine, 40));
			// Выводим собранный адрес
			return result;
		}
		// Если собирается адрес IPv6
		case 1: {
			// Заводим адрес IPv6
			unique_ptr <awh::net::addr_net_ipv6_t> result = make_unique <awh::net::addr_net_ipv6_t> ();
			// Заполняем адрес случайными октетами
			for(size_t i = 0; i < result->address.size(); i++)
				// Записываем очередной октет адреса
				result->address[i] = static_cast <uint8_t> (::pick(engine, 256));
			// Задаём префикс сети: значения свыше ста двадцати восьми подаются намеренно
			result->prefix = static_cast <uint8_t> (::pick(engine, 160));
			// Выводим собранный адрес
			return result;
		}
	}
	// Выводим пустой адрес: договор обязан устоять и на нём
	return nullptr;
}

/**
 * @brief Функция ворошения методов сетевых устройств
 *
 * @warning Изменяющих методов здесь нет НИ ОДНОГО: они правят настройку хозяйской
 *          машины. Перечень их приведён в заголовке файла
 *
 * @param eth    объект работы с сетевыми устройствами
 * @param engine источник случайных чисел
 *
 */
static void ifaces(const awh::eth_t & eth, mt19937_64 & engine) noexcept {
	// Имя устройства, каким ведётся ворошение
	const string name = ::interface(engine);
	// Собранный сетевой адрес
	unique_ptr <awh::net::addr_t> addr = ::address(engine);
	// Считаем вызов метода устройств
	::totals.ifaces++;
	/**
	 * Определяем ворошимый метод
	 */
	switch(static_cast <uint8_t> (::pick(engine, 7))){
		// Спрашиваем доступность устройства
		case 0: static_cast <void> (eth.iface.isAvailable(name)); break;
		// Спрашиваем, является ли устройство туннелем
		case 1: static_cast <void> (eth.iface.isTunnel(name)); break;
		// Спрашиваем, является ли устройство туннелем по адресу
		case 2: static_cast <void> (eth.iface.isTunnel(addr.get())); break;
		// Спрашиваем, является ли устройство надуманным
		case 3: static_cast <void> (eth.iface.isVirtual(name)); break;
		// Спрашиваем имя устройства по адресу
		case 4: static_cast <void> (eth.iface.name(addr.get())); break;
		// Спрашиваем наибольший размер пакета устройства
		case 5: static_cast <void> (eth.iface.mtu(name)); break;
		// Спрашиваем адрес устройства
		case 6: {
			// Снимаемый адрес устройства
			unique_ptr <awh::net::addr_t> ip = nullptr;
			// Снимаемый адрес встречной стороны
			unique_ptr <awh::net::addr_t> peer = nullptr;
			// Снимаемый префикс сети
			uint8_t prefix = 0;
			// Выполняем снятие адреса устройства
			static_cast <void> (eth.iface.getAddress(name, ip, peer, prefix));
		} break;
	}
}
/**
 * @brief Функция ворошения методов работы с адресами
 *
 * @param eth    объект работы с сетевыми устройствами
 * @param engine источник случайных чисел
 *
 */
static void addrs(const awh::eth_t & eth, mt19937_64 & engine) noexcept {
	// Считаем вызов метода адресов
	::totals.addrs++;
	/**
	 * Определяем ворошимый метод
	 */
	switch(static_cast <uint8_t> (::pick(engine, 5))){
		// Спрашиваем принадлежность адреса подсети
		case 0: static_cast <void> (eth.addr.isInSubnet(
			static_cast <uint32_t> (engine()),
			static_cast <uint32_t> (engine()),
			static_cast <uint8_t> (::pick(engine, 40))
		)); break;
		// Заполняем источник по имени устройства
		case 1: {
			// Заводим источник сетевых адресов
			awh::net::src_t source(::make_unique <awh::net::addr_net_ipv4_t> ());
			// Устанавливаем имя устройства источника
			source.iface = ::interface(engine);
			// Выполняем заполнение источника
			eth.addr.fillSource(source);
		} break;
		// Заполняем источник по заданной сети
		case 2: {
			// Заводим источник сетевых адресов
			awh::net::src_t source(::make_unique <awh::net::addr_net_ipv6_t> ());
			// Собираем сеть, по какой заполняется источник
			unique_ptr <awh::net::addr_t> net = ::address(engine);
			// Выполняем заполнение источника
			eth.addr.fillSource(net.get(), source);
		} break;
		// Считаем контрольную сумму переносимого уровня
		case 3: {
			// Длина переносимых данных
			const size_t length = static_cast <size_t> (::pick(engine, 512));
			// Переносимые данные
			vector <uint8_t> transport(length + 1, 0);
			/**
			 * Заполняем переносимые данные случайными октетами
			 */
			for(size_t i = 0; i < length; i++)
				// Записываем очередной октет данных
				transport[i] = static_cast <uint8_t> (::pick(engine, 256));
			// Адрес отправителя
			unique_ptr <awh::net::addr_t> src = ::address(engine);
			// Адрес получателя
			unique_ptr <awh::net::addr_t> dst = ::address(engine);
			/**
			 * @warning Длина подаётся ТА ЖЕ, что и выделена. Подавать длину больше
			 *          выделенной было бы не ворошением договора, а собственным выходом
			 *          за границы буфера: договор считает сумму по стольким октетам,
			 *          сколько ему названо, и назвать больше - изъян зовущего, не его
			 */
			static_cast <void> (eth.addr.checksum(
				FAMILIES[::pick(engine, sizeof(FAMILIES) / sizeof(FAMILIES[0]))],
				PROTOCOLS[::pick(engine, sizeof(PROTOCOLS) / sizeof(PROTOCOLS[0]))],
				src.get(), dst.get(), transport.data(), length
			));
		} break;
		// Считаем контрольную сумму на пустых доводах
		case 4: static_cast <void> (eth.addr.checksum(
			FAMILIES[::pick(engine, sizeof(FAMILIES) / sizeof(FAMILIES[0]))],
			PROTOCOLS[::pick(engine, sizeof(PROTOCOLS) / sizeof(PROTOCOLS[0]))],
			nullptr, nullptr, nullptr, 0
		)); break;
	}
}
/**
 * @brief Функция ворошения методов работы с сокетами
 *
 * @details Описатель подаётся трояким: заведённый ворошителем сокет, заведомо негодное
 *          число и описатель ЧУЖОГО РОДА - обычный файл. Последний важнее прочих:
 *          негодное число договор отсекает первой же проверкой, а годный описатель не
 *          того рода проходит её насквозь и доходит до самих настроек
 *
 * @param eth    объект работы с сетевыми устройствами
 * @param engine источник случайных чисел
 *
 */
static void sockets(const awh::eth_t & eth, mt19937_64 & engine) noexcept {
	// Описатель, каким ведётся ворошение
	awh::net::socket_t sock = awh::net::invalid_socket_t;
	// Признак того, что описатель заведён ворошителем и подлежит закрытию
	bool owned = false;
	/**
	 * Род заведённого описателя: 0 - гнездо, 1 - обычный описатель
	 *
	 * @note Нужен закрытию: у MS Windows у них РАЗНЫЕ вызовы закрытия, и перепутать их
	 *       нельзя - каждый отвечает отказом на чужой род
	 */
	uint8_t kind = 0;
	/**
	 * Определяем род подаваемого описателя
	 */
	switch(static_cast <uint8_t> (::pick(engine, 4))){
		// Подаём заведённый ворошителем сокет
		case 0:
		case 1: {
			// Заводим пробный сокет
			sock = static_cast <awh::net::socket_t> (::socket(
				((::pick(engine, 2) == 0) ? AF_INET : AF_INET6),
				((::pick(engine, 2) == 0) ? SOCK_STREAM : SOCK_DGRAM), 0
			));
			// Если сокет завести удалось
			if(sock != awh::net::invalid_socket_t){
				// Запоминаем, что описатель принадлежит ворошителю
				owned = true;
				// Считаем заведённый сокет
				::totals.issued++;
			}
		} break;
		// Подаём описатель чужого рода
		case 2: {
			/**
			 * Заводим обычный файл вместо сокета
			 *
			 * @note Имя пустого устройства у систем разное: у POSIX это «/dev/null», у
			 *       MS Windows - «NUL». Открывается оно там же обычным описателем времени
			 *       выполнения, а не гнездом, - чего проверка и добивается
			 */
			#if defined(_WIN32) || defined(_WIN64)
				sock = static_cast <awh::net::socket_t> (::open("NUL", O_RDWR));
			#else
				sock = static_cast <awh::net::socket_t> (::open("/dev/null", O_RDWR));
			#endif
			// Если файл открыть удалось
			if(sock != awh::net::invalid_socket_t){
				// Запоминаем, что описатель принадлежит ворошителю
				owned = true;
				// Запоминаем, что описатель гнездом не является
				kind = 1;
			}
		} break;
		// Подаём заведомо негодный описатель
		case 3: {
			// Выбираем негодное число описателя
			sock = static_cast <awh::net::socket_t> (::pick(engine, 2) == 0 ? -1 : 100000);
			// Считаем вызов с негодным описателем
			::totals.invalids++;
		} break;
	}
	// Семейство, каким ведётся ворошение
	const awh::event::family_t family = FAMILIES[::pick(engine, sizeof(FAMILIES) / sizeof(FAMILIES[0]))];
	// Считаем вызов метода сокетов
	::totals.sockets++;
	/**
	 * Определяем ворошимый метод
	 */
	switch(static_cast <uint8_t> (::pick(engine, 12))){
		// Спрашиваем последний отказ сокета
		case 0: static_cast <void> (eth.socket.getError(sock)); break;
		// Спрашиваем размер буфера сокета
		case 1: static_cast <void> (eth.socket.getBufferSize(sock, awh::net::socket_event_t::READ)); break;
		// Спрашиваем доступный объём буфера сокета
		case 2: static_cast <void> (eth.socket.getBufferAvailable(sock, awh::net::socket_event_t::WRITE)); break;
		// Задаём размер буфера сокета
		case 3: static_cast <void> (eth.socket.setBufferSize(sock, awh::net::socket_event_t::READ, static_cast <int32_t> (engine()))); break;
		// Задаём срок ожидания сокета
		case 4: static_cast <void> (eth.socket.setTimeout(sock, awh::net::socket_event_t::WRITE, static_cast <uint32_t> (engine()))); break;
		// Спрашиваем срок ожидания сокета
		case 5: static_cast <void> (eth.socket.getTimeout(sock, awh::net::socket_event_t::READ)); break;
		// Задаём устройство выхода групповой рассылки
		case 6: static_cast <void> (eth.socket.setMulticastIface(sock, family, ::interface(engine))); break;
		// Задаём поддержание связи
		case 7: static_cast <void> (eth.socket.setKeepalive(sock,
			static_cast <int32_t> (engine()), static_cast <int32_t> (engine()), static_cast <int32_t> (engine())
		)); break;
		// Спрашиваем число прыжков сокета
		case 8: static_cast <void> (eth.socket.getHops(sock, family,
			DELIVERIES[::pick(engine, sizeof(DELIVERIES) / sizeof(DELIVERIES[0]))]
		)); break;
		// Задаём число прыжков сокета
		case 9: static_cast <void> (eth.socket.setHops(sock, family,
			DELIVERIES[::pick(engine, sizeof(DELIVERIES) / sizeof(DELIVERIES[0]))],
			static_cast <uint8_t> (::pick(engine, 256))
		)); break;
		// Задаём код дифференцированного обслуживания
		case 10: static_cast <void> (eth.socket.setDifferentiatedServicesCodePoint(sock, family,
			static_cast <awh::event::dscp_t> (::pick(engine, 64))
		)); break;
		// Задаём режим уведомления о перегрузке
		case 11: static_cast <void> (eth.socket.setExplicitCongestionNotification(sock, family,
			static_cast <awh::event::ecn_t> (::pick(engine, 8))
		)); break;
	}
	/**
	 * Закрываем описатель, заведённый ворошителем
	 *
	 * @warning Закрывать обязан ворошитель, а не договор: договор описатель лишь
	 *          настраивает и владения им не принимает. Не закрой мы его - утечка была
	 *          бы СВОЯ, и ворошитель отчитался бы находкой на самом себе
	 */
	if(owned){
		/**
		 * Гнездо закрывается СВОИМ вызовом, а обычный описатель - своим
		 *
		 * @warning У MS Windows «close» гнездо НЕ закрывает вовсе: гнёзда и файловые
		 *          описатели разведены порознь, вызов отвечает отказом, а гнездо остаётся
		 *          жить до конца процесса. Ворошитель на этом отчитался находкой НА СЕБЕ:
		 *          20 000 проходов дали рост 255 → 2508 при 2506 заведённых пробных
		 *          сокетах, то есть «утекло» ровно то, что он завёл сам и не закрыл
		 *
		 * @note Описатель чужого рода (пустое устройство) закрывается именно «close»:
		 *       гнездом он не является, и «closesocket» ему отвечает отказом
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если описатель является гнездом
			if(kind == 0)
				// Закрываем гнездо своим вызовом
				static_cast <void> (::closesocket(static_cast <SOCKET> (sock)));
			// Если описатель гнездом не является
			else static_cast <void> (::close(static_cast <int32_t> (sock)));
		#else
			// Закрываем описатель ворошителя
			static_cast <void> (::close(static_cast <int32_t> (sock)));
		#endif
	}
}
/**
 * @brief Функция ворошения методов работы со шлюзами
 *
 * @warning Заведение и снос маршрутов исключены НАМЕРЕННО: они правят таблицу
 *          маршрутов хозяйской машины. Ворошится лишь подбор маршрута
 *
 * @param eth    объект работы с сетевыми устройствами
 * @param engine источник случайных чисел
 *
 */
static void gateways(const awh::eth_t & eth, mt19937_64 & engine) noexcept {
	// Маршрут, каким ведётся ворошение
	awh::eth::gateway_t::route_t route{};
	// Задаём имя устройства маршрута
	route.ifname = ::interface(engine);
	// Задаём префикс сети маршрута
	route.prefix = static_cast <uint8_t> (::pick(engine, 160));
	// Если выпало задать адрес назначения маршрута
	if(::pick(engine, 2) == 0)
		// Задаём адрес назначения маршрута
		route.destination = ::address(engine);
	// Если выпало задать шлюз маршрута
	if(::pick(engine, 2) == 0)
		// Задаём шлюз маршрута
		route.gateway = ::address(engine);
	// Считаем вызов метода шлюзов
	::totals.gateways++;
	// Выполняем подбор маршрута
	static_cast <void> (eth.gateway.get(route));
}

/**
 * @brief Функция ворошения методов работы с SCTP
 *
 * @details Поверхность эта вся из внешних доводов: описатель, число потоков, номер
 *          ключа, состав чанков - всё приходит от потребителя. Дважды она уже давала
 *          тонкие откаты (смена договора приёма, длина служебных данных у систем Sun),
 *          и оба раза отказом они не сообщались
 *
 * @warning Сокет заводится ворошителем и им же закрывается. Заведение SCTP-сокета
 *          волен отвергнуть сам ядро - настройка протокола есть не всюду, - и это НЕ
 *          находка: тогда подаётся обычный потоковый сокет, чтобы договор получил
 *          описатель не того наречия. Это тоже ворошение, притом полезное
 *
 * @param eth    объект работы с сетевыми устройствами
 * @param engine источник случайных чисел
 *
 */
#if __linux__ || __FreeBSD__ || __sun
	static void sctps(const awh::eth_t & eth, mt19937_64 & engine) noexcept {
		// Описатель, каким ведётся ворошение
		awh::net::socket_t sock = awh::net::invalid_socket_t;
		// Признак того, что описатель заведён ворошителем
		bool owned = false;
		/**
		 * Определяем род подаваемого описателя
		 */
		switch(static_cast <uint8_t> (::pick(engine, 4))){
			// Подаём сокет SCTP
			case 0:
			case 1: {
				// Заводим сокет SCTP
				sock = static_cast <awh::net::socket_t> (::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP));
				// Если сокет SCTP завести не удалось, подаём обычный потоковый
				if(sock == awh::net::invalid_socket_t)
					// Заводим обычный потоковый сокет
					sock = static_cast <awh::net::socket_t> (::socket(AF_INET, SOCK_STREAM, 0));
				// Если сокет заведён, запоминаем владение
				owned = (sock != awh::net::invalid_socket_t);
				// Считаем заведённый сокет
				::totals.issued += (owned ? 1 : 0);
			} break;
			// Подаём описатель чужого рода
			case 2: {
				// Заводим обычный файл вместо сокета
				sock = static_cast <awh::net::socket_t> (::open("/dev/null", O_RDWR));
				// Если файл открыть удалось, запоминаем владение
				owned = (sock != awh::net::invalid_socket_t);
			} break;
			// Подаём заведомо негодный описатель
			case 3: {
				// Выбираем негодное число описателя
				sock = static_cast <awh::net::socket_t> (::pick(engine, 2) == 0 ? -1 : 100000);
				// Считаем вызов с негодным описателем
				::totals.invalids++;
			} break;
		}
		// Считаем вызов метода SCTP
		::totals.sctps++;
		/**
		 * Определяем ворошимый метод
		 */
		switch(static_cast <uint8_t> (::pick(engine, 10))){
			// Спрашиваем состояние связи
			case 0: {
				// Состояние связи SCTP
				awh::net::sctp::status_t status{};
				// Выполняем снятие состояния связи
				static_cast <void> (eth.sctp.status(sock, status));
			} break;
			// Задаём параметры установки связи
			case 1: {
				// Параметры установки связи SCTP
				awh::net::sctp::initmsg_t initmsg{};
				// Задаём число попыток подключения
				initmsg.attempts = static_cast <uint16_t> (engine());
				// Задаём число исходящих потоков
				initmsg.ostreams = static_cast <uint16_t> (engine());
				// Задаём число входящих потоков
				initmsg.istreams = static_cast <uint16_t> (engine());
				// Выполняем установку параметров связи
				static_cast <void> (eth.sctp.initMessages(sock, initmsg));
			} break;
			// Спрашиваем поддержку возможности протокола
			case 2: static_cast <void> (eth.sctp.supported(
				FEATURES[::pick(engine, sizeof(FEATURES) / sizeof(FEATURES[0]))]
			)); break;
			// Задаём состав видов проверки подлинности
			case 3: {
				// Собираемый состав видов проверки подлинности
				vector <awh::net::sctp::auth_type_t> types;
				/**
				 * Собираем состав видов проверки подлинности
				 */
				for(size_t i = 0, count = static_cast <size_t> (::pick(engine, 6)); i < count; i++)
					// Добавляем очередной вид проверки подлинности
					types.push_back(AUTHS[::pick(engine, sizeof(AUTHS) / sizeof(AUTHS[0]))]);
				// Выполняем установку состава видов проверки подлинности
				static_cast <void> (eth.sctp.authenticateSupportAlgorithms(sock, types));
			} break;
			// Задаём ключ проверки подлинности
			case 4: {
				// Длина задаваемого ключа: пустой подаётся намеренно
				const size_t length = static_cast <size_t> (::pick(engine, 128));
				// Собираемый ключ проверки подлинности
				string key(length, '\0');
				/**
				 * Собираем ключ проверки подлинности из случайных октетов
				 */
				for(size_t i = 0; i < length; i++)
					// Записываем очередной октет ключа
					key[i] = static_cast <char> (::pick(engine, 256));
				// Выполняем установку ключа проверки подлинности
				static_cast <void> (eth.sctp.authenticateKey(sock, static_cast <uint16_t> (engine()), key));
			} break;
			// Спрашиваем срок протокола
			case 5: static_cast <void> (eth.sctp.timeout(sock, static_cast <uint32_t> (engine()),
				TIMEOUTS[::pick(engine, sizeof(TIMEOUTS) / sizeof(TIMEOUTS[0]))]
			)); break;
			// Задаём срок протокола
			case 6: static_cast <void> (eth.sctp.timeout(sock, static_cast <uint32_t> (engine()),
				TIMEOUTS[::pick(engine, sizeof(TIMEOUTS) / sizeof(TIMEOUTS[0]))],
				static_cast <uint32_t> (engine())
			)); break;
			// Спрашиваем современность оснастки протокола
			case 7: static_cast <void> (eth.sctp.modern()); break;
			// Спрашиваем поддержку частичной выдачи
			case 8: static_cast <void> (eth.sctp.partial()); break;
			// Задаём выдачу сведений о принятом сообщении
			case 9: {
				// Задаём выдачу сведений о принятом сообщении
				static_cast <void> (eth.sctp.receiveInfo(sock, (::pick(engine, 2) == 0)));
				// Задаём явную границу записи
				static_cast <void> (eth.sctp.explicitEndOfRecord(sock, (::pick(engine, 2) == 0)));
			} break;
		}
		// Если описатель заведён ворошителем, закрываем его
		if(owned)
			// Закрываем описатель ворошителя
			static_cast <void> (::close(static_cast <int32_t> (sock)));
	}
#endif

/**
 * @brief Функция снятия перечня открытых описателей процесса
 *
 * @details Утечка описателей отказом не сообщается и падением не проявляется: она
 *          копится молча, пока процесс не упрётся в предел
 *
 * @return перечень открытых описателей процесса
 *
 */
static vector <int32_t> descriptorList() noexcept {
	// Собираемый перечень описателей
	vector <int32_t> result;
/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Перебираем ЗНАЧЕНИЯ ГНЁЗД, а не описатели процесса
	 *
	 * @details У MS Windows гнёзда и файловые описатели разведены порознь: `fcntl`
	 *          там нет вовсе, а перечня открытых описателей процесса не выдаёт ничто.
	 *          Зато значения гнёзд кратны четырём и перебираются напрямую
	 *
	 * @warning Живость спрашивается `SO_TYPE`, а НЕ `getsockname`: имя непривязанного
	 *          гнезда у этой системы спросить нельзя вовсе (WSAEINVAL), а сокет
	 *          заводится раньше, чем привязывается. Щуп, спрашивавший имя, дважды
	 *          отчитался «утечки нет» на пустом месте - он попросту не видел искомого
	 *
	 * @note Предел взят с большим запасом: набор проверок к своей середине держит
	 *       сотни гнёзд, значения их растут, и узкий предел прячет держателя за собой
	 */
	for(uintptr_t value = 4; value < 262144; value += 4){
		// Разновидность гнезда: она есть и у непривязанного
		int32_t kind = 0;
		// Длина места под ответ
		int32_t size = static_cast <int32_t> (sizeof(kind));
		// Если разновидность снять удалось, гнездо живо
		if(::getsockopt(static_cast <SOCKET> (value), SOL_SOCKET, SO_TYPE, reinterpret_cast <char *> (&kind), &size) == 0)
			// Запоминаем живое гнездо
			result.push_back(static_cast <int32_t> (value));
	}
/**
 * Если операционной системой является не MS Windows
 */
#else
	/**
	 * Перебираем описатели процесса
	 *
	 * @note Предел взят с запасом: договор `eth_t` держит сокетов единицы, и обход
	 *       тысячи чисел стоит меньше миллисекунды
	 */
	/**
	 * Предел перебора описателей процесса
	 *
	 * @warning Узкий предел прячет держателя ЗА собой, и прибор отчитывается чистым
	 *          итогом, ничего не проверив. Установлено Андреем при переносе ворошителя
	 *          под MS Windows: перебор до 16384 не видел держателя, лежавшего выше
	 */
	constexpr int32_t LIMIT = 262144;
	for(int32_t fd = 0; fd < LIMIT; fd++){
		// Сведения об описателе
		struct stat info{};
		/**
		 * Описатель считается открытым лишь когда его признают ОБА вопроса
		 *
		 * @warning Одного `fcntl` мало: у некоторых систем он отвечает удачей и на
		 *          описатель, какого нет. Сведения о нём в таком случае снять нечем,
		 *          и второй вопрос это вскрывает
		 */
		if((::fcntl(fd, F_GETFD) != -1) && (::fstat(fd, &info) == 0))
			// Запоминаем открытый описатель
			result.push_back(fd);
	}
#endif
	// Выводим перечень открытых описателей
	return result;
}

/**
 * @brief Точка входа ворошителя
 *
 * @param argc число доводов командной строки
 * @param argv перечень доводов командной строки
 * @return     итог работы ворошителя
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Число проходов ворошителя
	uint64_t rounds = 1000;
	// Зерно источника случайных чисел
	uint64_t seed = 1;
	// Если число проходов задано доводом
	if(argc > 1)
		// Снимаем число проходов из довода
		rounds = ::strtoull(argv[1], nullptr, 10);
	// Если зерно задано доводом
	if(argc > 2)
		// Снимаем зерно из довода
		seed = ::strtoull(argv[2], nullptr, 10);
	// Заводим источник случайных чисел
	mt19937_64 engine(seed);
	// Заводим объект работы с сетевыми устройствами
	const awh::eth_t eth(::framework(), ::logger());
	/**
	 * Число проходов разогрева
	 *
	 * @warning Снимок описателей ДО работы негоден эталоном: часть описателей договор
	 *          заводит лениво - при первом обращении к системе, - и они попали бы в
	 *          находку, ничем ей не будучи. Эталон снимается ПОСЛЕ разогрева
	 */
	const uint64_t warmup = ((rounds / 10) + 1);
	// Эталонный перечень описателей, снятый после разогрева
	vector <int32_t> opened;
	/**
	 * Выполняем проходы ворошителя
	 */
	for(uint64_t round = 0; round < rounds; round++){
		// Считаем сделанный проход
		::totals.rounds++;
		/**
		 * Определяем ворошимую часть договора
		 */
		/**
		 * @note Долей ворошения SCTP отведена та же, что и прочим частям, лишь у систем
		 *       с протоколом: у прочих её нет вовсе, и разбор идёт по четырём
		 */
		#if __linux__ || __FreeBSD__ || __sun
			const uint8_t parts = 5;
		#else
			const uint8_t parts = 4;
		#endif
		switch(static_cast <uint8_t> (::pick(engine, parts))){
			// Ворошим методы сетевых устройств
			case 0: ::ifaces(eth, engine); break;
			// Ворошим методы работы с адресами
			case 1: ::addrs(eth, engine); break;
			// Ворошим методы работы с сокетами
			case 2: ::sockets(eth, engine); break;
			// Ворошим методы работы со шлюзами
			case 3: ::gateways(eth, engine); break;
			/**
			 * Ворошим методы работы с SCTP
			 */
			#if __linux__ || __FreeBSD__ || __sun
				case 4: ::sctps(eth, engine); break;
			#endif
		}
		// Если разогрев закончен, снимаем эталонный перечень описателей
		if(round == warmup)
			// Снимаем эталонный перечень описателей
			opened = ::descriptorList();
	}
	// Признак того, что находка сделана
	bool leaked = false;
	// Перечень описателей, оставшихся после работы
	const vector <int32_t> remained = ::descriptorList();
	// Если описателей стало больше, чем было после разогрева
	if(!opened.empty() && (remained.size() > opened.size())){
		// Выводим находку
		::fprintf(stderr, "НАХОДКА: описатели утекли, после разогрева было %zu, в конце %zu\n", opened.size(), remained.size());
		/**
		 * Перебираем описатели, оставшиеся после работы
		 */
		for(auto & fd : remained){
			// Признак того, что описатель был открыт и до работы
			bool known = false;
			/**
			 * Перебираем описатели, открытые до работы
			 */
			for(auto & item : opened){
				// Если описатель был открыт и до работы
				if(item == fd){
					// Запоминаем, что описатель не нов
					known = true;
					// Прекращаем перебор
					break;
				}
			}
			// Если описатель появился за время работы, называем его
			if(!known)
				// Выводим утёкший описатель
				::fprintf(stderr, "  утёк описатель %d\n", fd);
		}
		// Запоминаем, что находка сделана: выйти отказом надо ПОСЛЕ вывода итогов
		leaked = true;
	}
	// Выводим итоги проделанной работы
	::fprintf(stdout,
		"ЗЕРНО=%llu ПРОХОДОВ=%llu\n"
		"  вызовов методов устройств: %llu\n"
		"  вызовов методов адресов: %llu\n"
		"  вызовов методов сокетов: %llu\n"
		"  вызовов методов шлюзов: %llu\n"
		"  заведено пробных сокетов: %llu\n"
		"  вызовов с негодным описателем: %llu\n"
		"  вызовов методов SCTP: %llu\n"
		"  описателей после разогрева (%llu проходов): %zu, в конце: %zu\n",
		static_cast <unsigned long long> (seed), static_cast <unsigned long long> (::totals.rounds),
		static_cast <unsigned long long> (::totals.ifaces), static_cast <unsigned long long> (::totals.addrs),
		static_cast <unsigned long long> (::totals.sockets), static_cast <unsigned long long> (::totals.gateways),
		static_cast <unsigned long long> (::totals.issued), static_cast <unsigned long long> (::totals.invalids),
		static_cast <unsigned long long> (::totals.sctps),
		static_cast <unsigned long long> (warmup), opened.size(), remained.size()
	);
	// Выводим итог работы ворошителя
	return (leaked ? EXIT_FAILURE : EXIT_SUCCESS);
}
