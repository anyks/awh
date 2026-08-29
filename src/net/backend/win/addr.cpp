/**
 * @file addr.cpp
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
 * @brief Реализация бэкенда определения собственных адресов машины для MS Windows —
 *        подбор устройства, его адреса и аппаратного адреса под задачу обмена
 *
 * @details Слой этот отвечает эталонным backend/bsd/addr.cpp и backend/gnu/addr.cpp.
 *          Занят он одним: по неполному описанию источника обмена дозаполнить
 *          недостающее - устройство по адресу, адрес по устройству, аппаратный адрес
 *          соседа по его сетевому адресу
 *
 *          Средства для того у MS Windows свои. Перечень устройств вместе с их
 *          адресами и аппаратными адресами отдаёт GetAdaptersAddresses - за тем, за
 *          чем у POSIX ходят к getifaddrs. Соседей канального уровня отдаёт
 *          GetIpNetTable2 - за тем, за чем у BSD перебирают таблицу путей с признаком
 *          записей канального уровня, а у Linux спрашивают netlink
 *
 * @par Намеренные решения
 *
 *      **Исходящий адрес подбирается путём по умолчанию, а не подключением наружу.**
 *      Довод тот же, что и у эталонных бэкендов: подключение к чужому серверу ради
 *      собственного адреса требует выхода в интернет, зависит от случая при выборе
 *      сервера и на машине за туннелем отвечает адресом туннеля - отчего отправка
 *      соседу по своей же сети отвечала отказом
 *
 *      **Определённый адрес запоминается на время.** Подбор этот стоит опроса
 *      перечня устройств и таблицы путей, а событий машина заводит помногу: без
 *      запоминания работа эта повторялась бы на каждое из них
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <mutex>
#include <string>
#include <vector>
#include <cstring>
#include <shared_mutex>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/eth.hpp>

/**
 * @brief Средства опроса сетевых устройств и соседей канального уровня
 *
 */
#include <iphlpapi.h>
#include <netioapi.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название бэкенда для записей в журнале
 *
 */
static constexpr const char * __AWH_ADDR_BACKEND__ = "MS Windows address backend";

/**
 * @brief Инкапсулируем состояние слоя в пространство имён
 *
 */
namespace {
	/**
	 * @brief Срок годности запомненного адреса в миллисекундах
	 *
	 * @note Пять секунд взяты вслед за эталонными бэкендами: срок этот короче
	 *       обыкновенной перенастройки сети и длиннее череды создаваемых разом событий
	 *
	 */
	constexpr uint64_t __awh_outward_lifetime__ = 5000ULL;
	// Пустой аппаратный адрес для сличения
	static const uint8_t __awh_zero_mac__[6] = {0};
	// Пустой адрес IPv6 для сличения
	static const uint8_t __awh_zero_ipv6__[16] = {0};
	/**
	 * @brief Запомненные сведения об адресе выхода во внешнюю сеть
	 *
	 */
	struct outward_t {
		bool filled;                    // Признак заполненности записи
		uint64_t deadline;              // Время устаревания записи
		string iface;                   // Название сетевого устройства
		std::array <uint8_t, 16> address; // Адрес сетевого устройства
		std::array <uint8_t, 6> mac;      // Аппаратный адрес устройства
		/**
		 * @brief Конструктор
		 *
		 */
		outward_t() noexcept : filled(false), deadline(0), address{}, mac{} {}
	};
	// Замок, оберегающий запомненные сведения
	static std::shared_mutex __awh_outward_mutex__;
	// Запомненные сведения порознь для IPv4 и IPv6
	static outward_t __awh_outward__[2];
	/**
	 * @brief Функция сличения названий сетевых устройств
	 *
	 * @details Сличение ведётся БЕЗ учёта регистра, и это не послабление, а
	 *          устройство системы: названием устройства у MS Windows служит GUID, и
	 *          система выдаёт его заглавными буквами (`{F49A2CB0-...}`), тогда как
	 *          слои выше приводят названия к нижнему регистру - у систем POSIX
	 *          названия устройств в нижнем регистре и живут. Посимвольное сличение
	 *          не совпадало оттого НИКОГДА: установлено прогоном, набор терял
	 *          `IoBroadcastTest`, а привязка события к устройству отвечала отказом
	 *          «устройство не найдено» при живом устройстве
	 *
	 * @note Свёртка регистра здесь своя, ASCII, а не средствами языка: `tolower`
	 *       зависит от местности, а GUID - данные протокольные, и разбирать их по
	 *       местности нельзя
	 *
	 * @param name    искомое название устройства
	 * @param adapter название устройства, выданное системой
	 * @return        признак совпадения названий
	 *
	 */
	static bool __awh_iface_same__(const std::string_view name, const char * adapter) noexcept {
		// Если название устройства не задано
		if(adapter == nullptr)
			// Выводим несовпадение названий
			return false;
		// Выполняем перебор символов искомого названия
		size_t i = 0;
		/**
		 * Выполняем сличение названий посимвольно, свёртывая регистр
		 */
		for(; (i < name.size()) && (adapter[i] != '\0'); i++){
			// Получаем очередной символ искомого названия
			char first = name.at(i);
			// Получаем очередной символ названия, выданного системой
			char second = adapter[i];
			// Свёртываем регистр символа искомого названия
			if((first >= 'A') && (first <= 'Z'))
				// Приводим символ к нижнему регистру
				first = static_cast <char> (first + ('a' - 'A'));
			// Свёртываем регистр символа названия, выданного системой
			if((second >= 'A') && (second <= 'Z'))
				// Приводим символ к нижнему регистру
				second = static_cast <char> (second + ('a' - 'A'));
			// Если символы разошлись
			if(first != second)
				// Выводим несовпадение названий
				return false;
		}
		// Выводим совпадение названий, если оба окончились разом
		return ((i == name.size()) && (adapter[i] == '\0'));
	}
	/**
	 * @brief Функция получения перечня сетевых устройств машины
	 *
	 * @param buffer буфер, в котором остаётся перечень устройств
	 * @return       указатель на первое устройство перечня, либо nullptr при отказе
	 *
	 */
	static PIP_ADAPTER_ADDRESSES __awh_adapters__(std::vector <uint8_t> & buffer) noexcept {
		// Состав запрашиваемых сведений
		const ULONG flags = (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME);
		// Объём буфера под перечень устройств
		ULONG size = 16384;
		/**
		 * Отводим место под перечень устройств и опрашиваем систему
		 */
		for(uint8_t attempt = 0; attempt < 4; attempt++){
			// Отводим место под перечень устройств
			buffer.assign(static_cast <size_t> (size), 0);
			// Выполняем опрос перечня сетевых устройств
			const ULONG result = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data()), &size);
			// Если перечень устройств получен
			if(result == ERROR_SUCCESS)
				// Выводим указатель на первое устройство перечня
				return reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data());
			// Если объёма буфера не хватило - повторяем опрос с объёмом, названным системой
			if(result != ERROR_BUFFER_OVERFLOW)
				// Выводим признак отказа опроса
				return nullptr;
		}
		// Выводим признак отказа опроса
		return nullptr;
	}
	/**
	 * @brief Функция проверки того, что примет для подбора устройства нет
	 *
	 * @details Пустые адрес и аппаратный адрес разом означают не «устройство не
	 *          найдено», а осознанный отказ от выбора: запись читается как «любой
	 *          адрес», и устройство отбирает система при привязке сокета
	 *
	 * @param source описание источника обмена
	 * @return       признак отсутствия приметы для подбора
	 *
	 */
	static bool __awh_zero__(const awh::net::src_t & source) noexcept {
		// Если адрес либо аппаратный адрес не заданы, примет для подбора нет
		if((source.ip == nullptr) || (source.mac == nullptr))
			// Выводим признак отсутствия приметы
			return true;
		// Если аппаратный адрес задан, примета для подбора устройства есть
		if(::memcmp(&awh_cast <awh::net::addr_mac_t *> (source.mac.get())->address[0], ::__awh_zero_mac__, 6) != 0)
			// Выводим наличие приметы для подбора
			return false;
		/**
		 * Определяем вид адреса источника обмена
		 */
		switch(source.ip->size){
			// Если адрес является адресом IPv4
			case 4: return (awh_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->address == 0);
			// Если адрес является адресом IPv6
			case 16: return (::memcmp(&awh_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->address[0], ::__awh_zero_ipv6__, 16) == 0);
		}
		// Выводим признак отсутствия приметы
		return true;
	}
	/**
	 * @brief Функция проверки годности адреса IPv6 источником обмена
	 *
	 * @details Годным считается любой адрес IPv6, кроме канального и петли. Канальный
	 *          без зоны устройства бессмыслен, петля же за пределы машины не выходит
	 *
	 * @param source описание источника обмена
	 * @return       признак годности адреса
	 *
	 */
	static bool __awh_routable__(const awh::net::src_t & source) noexcept {
		// Если источник хранит адрес не того семейства, годным он быть не может
		if((source.ip == nullptr) || (source.ip->size != 16))
			// Выводим признак негодности адреса
			return false;
		// Адрес источника обмена
		struct in6_addr addr{};
		// Выполняем перенос адреса источника обмена
		::memcpy(&addr, &awh_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr));
		// Выводим годность адреса источником обмена
		return (!IN6_IS_ADDR_UNSPECIFIED(&addr) && !IN6_IS_ADDR_LINKLOCAL(&addr) && !IN6_IS_ADDR_LOOPBACK(&addr));
	}
	/**
	 * @brief Функция восстановления адреса из запомненных сведений
	 *
	 * @param source описание источника обмена
	 * @return       признак успешного восстановления
	 *
	 */
	static bool __awh_recall__(awh::net::src_t & source) noexcept {
		// Если размер адреса не отвечает ни одному из семейств
		if((source.ip == nullptr) || ((source.ip->size != 4) && (source.ip->size != 16)))
			// Выводим признак неуспешного восстановления
			return false;
		// Блокируем запомненные сведения на чтение
		const std::shared_lock <std::shared_mutex> lock(::__awh_outward_mutex__);
		// Получаем запись, отвечающую семейству адресов
		const outward_t & outward = ::__awh_outward__[source.ip->size == 16];
		// Если запись не заполнена либо устарела
		if(!outward.filled || (::GetTickCount64() >= outward.deadline))
			// Выводим признак неуспешного восстановления
			return false;
		// Восстанавливаем название сетевого устройства
		source.iface = outward.iface;
		// Если адрес является адресом IPv4
		if(source.ip->size == 4)
			// Восстанавливаем адрес сетевого устройства
			::memcpy(&awh_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->address, &outward.address[0], 4);
		// Если адрес является адресом IPv6
		else ::memcpy(&awh_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->address[0], &outward.address[0], 16);
		// Если аппаратный адрес задан
		if(source.mac != nullptr)
			// Восстанавливаем аппаратный адрес устройства
			::memcpy(&awh_cast <awh::net::addr_mac_t *> (source.mac.get())->address[0], &outward.mac[0], 6);
		// Выводим признак успешного восстановления
		return true;
	}
	/**
	 * @brief Функция запоминания определённого адреса
	 *
	 * @param source описание источника обмена
	 *
	 */
	static void __awh_remember__(const awh::net::src_t & source) noexcept {
		// Если размер адреса не отвечает ни одному из семейств
		if((source.ip == nullptr) || ((source.ip->size != 4) && (source.ip->size != 16)))
			// Выходим из функции
			return;
		// Блокируем запомненные сведения на запись
		const std::lock_guard <std::shared_mutex> lock(::__awh_outward_mutex__);
		// Получаем запись, отвечающую семейству адресов
		outward_t & outward = ::__awh_outward__[source.ip->size == 16];
		// Запоминаем название сетевого устройства
		outward.iface = source.iface;
		// Если адрес является адресом IPv4
		if(source.ip->size == 4)
			// Запоминаем адрес сетевого устройства
			::memcpy(&outward.address[0], &awh_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->address, 4);
		// Если адрес является адресом IPv6
		else ::memcpy(&outward.address[0], &awh_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->address[0], 16);
		// Если аппаратный адрес задан
		if(source.mac != nullptr)
			// Запоминаем аппаратный адрес устройства
			::memcpy(&outward.mac[0], &awh_cast <awh::net::addr_mac_t *> (source.mac.get())->address[0], 6);
		// Устанавливаем время устаревания записи
		outward.deadline = (::GetTickCount64() + ::__awh_outward_lifetime__);
		// Отмечаем запись заполненной
		outward.filled = true;
	}
	/**
	 * @brief Функция поиска годного адреса IPv6 среди устройств машины
	 *
	 * @details Нужна там, где устройство пути по умолчанию годного адреса не дало: на
	 *          машине без пути IPv6 наружу путём оказывается туннель, держащий один
	 *          лишь канальный адрес
	 *
	 * @param source описание источника обмена
	 *
	 */
	static void __awh_discover__(awh::net::src_t & source) noexcept {
		// Буфер под перечень сетевых устройств
		std::vector <uint8_t> buffer;
		// Выполняем опрос перечня сетевых устройств
		PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
		// Если перечень устройств получить не удалось
		if(adapters == nullptr)
			// Выходим из функции
			return;
		/**
		 * Выполняем перебор всех сетевых устройств машины
		 */
		for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
			// Если устройство не работает - адреса его непригодны
			if(adapter->OperStatus != IfOperStatusUp)
				// Переходим к следующему устройству
				continue;
			/**
			 * Выполняем перебор всех адресов устройства
			 */
			for(PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next){
				// Если адрес устройства не задан либо не является адресом IPv6
				if((address->Address.lpSockaddr == nullptr) || (address->Address.lpSockaddr->sa_family != AF_INET6))
					// Переходим к следующему адресу
					continue;
				// Получаем адрес устройства вида IPv6
				struct sockaddr_in6 * value = reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr);
				// Если адрес источником обмена не годится
				if(IN6_IS_ADDR_UNSPECIFIED(&value->sin6_addr) || IN6_IS_ADDR_LINKLOCAL(&value->sin6_addr) || IN6_IS_ADDR_LOOPBACK(&value->sin6_addr))
					// Переходим к следующему адресу
					continue;
				// Запоминаем адрес сетевого устройства
				::memcpy(&awh_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->address[0], &value->sin6_addr, sizeof(struct in6_addr));
				// Запоминаем номер устройства зоны адреса
				awh_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->zone = static_cast <uint32_t> (value->sin6_scope_id);
				// Если название устройства получено
				if(adapter->AdapterName != nullptr)
					// Запоминаем название сетевого устройства
					source.iface = adapter->AdapterName;
				// Если аппаратный адрес задан и устройство его имеет
				if((source.mac != nullptr) && (adapter->PhysicalAddressLength == 6))
					// Запоминаем аппаратный адрес устройства
					::memcpy(&awh_cast <awh::net::addr_mac_t *> (source.mac.get())->address[0], adapter->PhysicalAddress, 6);
				// Выходим из функции
				return;
			}
		}
	}
};

/**
 * @brief Метод заполнения источника обмена по названию сетевого устройства
 *
 * @param source описание источника обмена
 *
 * @details Название устройства здесь входное, а адрес и аппаратный адрес - выходные
 *
 * @note Перечень устройств у MS Windows отдаётся разом вместе с их адресами и
 *       аппаратными адресами: за тем, за чем у систем POSIX ходят к списку устройств,
 *       а следом к таблице путей, здесь довольно одного опроса
 *
 */
void awh::eth::Network_Address::fillSource(net::src_t & source) const noexcept {
	// Если описание источника обмена не передано
	if(source.ip == nullptr)
		// Выходим из функции
		return;
	/**
	 * Если название сетевого устройства не задано - подбираем устройство выхода
	 *
	 * @details Пустое название означает не отказ, а просьбу выбрать устройство
	 *          самому - то, которым машина выходит во внешнюю сеть. Ровно так
	 *          поступают эталонные бэкенды: у них пустое название уводит разбор к
	 *          подбору по пути умолчания
	 *
	 * @note Прежде пустое название уводило из функции молча, и источник оставался
	 *       незаполненным: свой адрес объявлялся нулём, а название устройства -
	 *       пустым. Установлено прогоном: набор терял `IoBroadcastTest`, где
	 *       устройство выхода спрашивают именно так
	 */
	if(source.iface.empty()){
		// Выполняем подбор устройства выхода во внешнюю сеть
		this->fillSource(event::node_t::NONE, source);
		// Выходим из функции
		return;
	}
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Выводим в журнал сообщение о невозможности опроса устройств
		this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_ADDR_BACKEND__);
		// Выходим из функции
		return;
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства с искомым не совпало
		if((adapter->AdapterName == nullptr) || !::__awh_iface_same__(source.iface, adapter->AdapterName))
			// Переходим к следующему устройству
			continue;
		// Если аппаратный адрес задан и устройство его имеет
		if((source.mac != nullptr) && (adapter->PhysicalAddressLength == 6))
			// Запоминаем аппаратный адрес устройства
			::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], adapter->PhysicalAddress, 6);
		/**
		 * Выполняем перебор всех адресов устройства
		 */
		for(PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next){
			// Если адрес устройства не задан
			if(address->Address.lpSockaddr == nullptr)
				// Переходим к следующему адресу
				continue;
			// Если искомым является адрес IPv4 и адрес устройства ему отвечает
			if((source.ip->size == 4) && (address->Address.lpSockaddr->sa_family == AF_INET)){
				// Запоминаем адрес сетевого устройства
				awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr;
				// Выходим из функции
				return;
			}
			// Если искомым является адрес IPv6 и адрес устройства ему отвечает
			if((source.ip->size == 16) && (address->Address.lpSockaddr->sa_family == AF_INET6)){
				// Получаем адрес устройства вида IPv6
				struct sockaddr_in6 * value = reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr);
				// Определяем, является ли адрес адресом канальной связи
				const bool link = IN6_IS_ADDR_LINKLOCAL(&value->sin6_addr);
				// Если адрес ещё не заполнен либо найден глобальный взамен канального
				if(!::__awh_routable__(source) || !link){
					// Запоминаем адрес сетевого устройства
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &value->sin6_addr, sizeof(struct in6_addr));
					// Запоминаем номер устройства зоны адреса
					awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->zone = static_cast <uint32_t> (value->sin6_scope_id);
					// Если найден глобальный адрес - поиск на нём и оканчивается
					if(!link)
						// Выходим из функции
						return;
				}
			}
		}
		// Завершаем перебор устройств
		break;
	}
}
/**
 * @brief Метод заполнения источника обмена по заданной сети
 *
 * @param net    адрес сети, среди которой отыскивается устройство
 * @param source описание источника обмена
 *
 * @note Не нашлось устройства с адресом в заданной сети - подбор ведётся так же, как
 *       если бы сеть не называли вовсе: тем поведение приводится к общему с
 *       эталонными бэкендами виду
 *
 */
void awh::eth::Network_Address::fillSource(const net::addr_t * net, net::src_t & source) const noexcept {
	// Если адрес сети либо описание источника не переданы
	if((net == nullptr) || (source.ip == nullptr) || (net->size != source.ip->size)){
		// Выполняем подбор устройства без оглядки на сеть
		this->fillSource(event::node_t::NONE, source);
		// Выходим из функции
		return;
	}
	/**
	 * Неназначенный адрес устройству не принадлежит
	 *
	 * @details Адрес из одних нулей (0.0.0.0 либо ::) сетью не является и назначения
	 *          не имеет: подбирать по нему устройство не из чего. Системы POSIX
	 *          отвечают на него пустотой, и договор движков оттого един
	 *
	 * @warning Без этого заслона подбор совпадал с ЛЮБЫМ устройством, у какого длина
	 *          префикса нулевая: маска сличения обращается в нуль, и сличение нуля с
	 *          нулём выходит верным всегда. Проверка EthSuiteTest падала оттого на
	 *          всякой машине Windows, а вина ложилась на устройство маршрутов
	 */
	switch(net->size){
		// Для адреса IPv4
		case 4: {
			// Если адрес не назначен
			if(awh_cast <const net::addr_net_ipv4_t *> (net)->address == 0)
				// Выходим из функции, ничего не заполняя
				return;
		} break;
		// Для адреса IPv6
		case 16: {
			// Признак того, что адрес состоит из одних нулей
			bool empty = true;
			// Перебираем октеты переданного адреса
			for(uint8_t i = 0; (i < 16) && empty; i++)
				// Запоминаем, остаётся ли адрес пустым
				empty = (awh_cast <const net::addr_net_ipv6_t *> (net)->address[i] == 0);
			// Если адрес не назначен
			if(empty)
				// Выходим из функции, ничего не заполняя
				return;
		} break;
	}
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Выполняем подбор устройства без оглядки на сеть
		this->fillSource(event::node_t::NONE, source);
		// Выходим из функции
		return;
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если устройство не работает - адреса его непригодны
		if(adapter->OperStatus != IfOperStatusUp)
			// Переходим к следующему устройству
			continue;
		/**
		 * Выполняем перебор всех адресов устройства
		 */
		for(PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next){
			// Если адрес устройства не задан
			if(address->Address.lpSockaddr == nullptr)
				// Переходим к следующему адресу
				continue;
			// Признак принадлежности адреса устройства заданной сети
			bool matched = false;
			// Если проверяется принадлежность адреса IPv4
			if((net->size == 4) && (address->Address.lpSockaddr->sa_family == AF_INET)){
				// Получаем длину префикса заданной сети
				const uint8_t prefix = awh_cast <const net::addr_net_ipv4_t *> (net)->prefix;
				// Получаем адрес устройства
				const uint32_t value = reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr;
				/**
				 * Собираем маску заданной сети в порядке следования байт сети
				 *
				 * @warning Верхний край проверяется наравне с нижним: длина префикса
				 *          приходит полем октета, и значение свыше 32 обращает сдвиг
				 *          в сдвиг на ОТРИЦАТЕЛЬНОЕ число разрядов - поведение
				 *          неопределённое. Найдено надзирателем undefined у наречий
				 *          POSIX: «shift exponent -2 is negative». Довод приходит от
				 *          потребителя, ограничивать его выше по течению нечем
				 */
				const uint32_t mask = (
					(prefix == 0) ? 0 :
					((prefix >= 32) ? 0xFFFFFFFFU : ::htonl(~((1U << (32 - prefix)) - 1)))
				);
				// Определяем принадлежность адреса устройства заданной сети
				matched = ((value & mask) == (awh_cast <const net::addr_net_ipv4_t *> (net)->address & mask));
			}
			// Если проверяется принадлежность адреса IPv6
			else if((net->size == 16) && (address->Address.lpSockaddr->sa_family == AF_INET6)){
				// Получаем длину префикса заданной сети
				const uint8_t prefix = awh_cast <const net::addr_net_ipv6_t *> (net)->prefix;
				// Получаем адрес устройства
				const uint8_t * value = reinterpret_cast <const uint8_t *> (&reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr)->sin6_addr);
				// Получаем адрес заданной сети
				const uint8_t * network = &awh_cast <const net::addr_net_ipv6_t *> (net)->address[0];
				// Запоминаем совпадение до сличения
				matched = true;
				/**
				 * Сличаем адреса по разрядам длины префикса
				 */
				for(uint8_t i = 0; i < prefix; i += 8){
					// Определяем число сличаемых разрядов очередного байта
					const uint8_t bits = ((prefix - i) < 8 ? static_cast <uint8_t> (prefix - i) : 8);
					// Собираем маску сличаемых разрядов
					const uint8_t mask = static_cast <uint8_t> (bits == 0 ? 0 : (0xFF << (8 - bits)));
					// Если разряды очередного байта не совпали
					if((value[i / 8] & mask) != (network[i / 8] & mask)){
						// Запоминаем несовпадение адресов
						matched = false;
						// Завершаем сличение адресов
						break;
					}
				}
			}
			// Если адрес устройства заданной сети не принадлежит
			if(!matched)
				// Переходим к следующему адресу
				continue;
			// Если название устройства получено
			if(adapter->AdapterName != nullptr)
				// Запоминаем название сетевого устройства
				source.iface = adapter->AdapterName;
			// Если аппаратный адрес задан и устройство его имеет
			if((source.mac != nullptr) && (adapter->PhysicalAddressLength == 6))
				// Запоминаем аппаратный адрес устройства
				::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], adapter->PhysicalAddress, 6);
			// Если адрес является адресом IPv4
			if(net->size == 4)
				// Запоминаем адрес сетевого устройства
				awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr;
			// Если адрес является адресом IPv6
			else {
				// Получаем адрес устройства вида IPv6
				struct sockaddr_in6 * value = reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr);
				// Запоминаем адрес сетевого устройства
				::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &value->sin6_addr, sizeof(struct in6_addr));
				// Запоминаем номер устройства зоны адреса
				awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->zone = static_cast <uint32_t> (value->sin6_scope_id);
			}
			// Выходим из функции
			return;
		}
	}
	// Выполняем подбор устройства без оглядки на сеть
	this->fillSource(event::node_t::NONE, source);
}
/**
 * @brief Метод заполнения источника обмена по виду узла
 *
 * @param node   вид узла обмена
 * @param source описание источника обмена
 *
 */
void awh::eth::Network_Address::fillSource(const event::node_t node, net::src_t & source) const noexcept {
	// Если описание источника обмена не передано
	if(source.ip == nullptr)
		// Выходим из функции
		return;
	/**
	 * Определяем вид узла обмена
	 */
	switch(static_cast <uint8_t> (node)){
		/**
		 * Если вид узла не назван - отыскивается адрес выхода во внешнюю сеть
		 *
		 * @note Подбор ведётся путём по умолчанию, а не подключением к чужому серверу:
		 *       довод к тому изложен при описании модуля
		 *
		 */
		case static_cast <uint8_t> (event::node_t::NONE): {
			// Если адрес удалось восстановить из запомненных сведений
			if(::__awh_recall__(source))
				// Выходим из функции
				return;
			// Описание пути по умолчанию
			gateway_t::route_t route{};
			// Если адрес является адресом IPv4
			if(source.ip->size == 4){
				// Заводим адрес шлюза пути
				route.gateway = make_unique <net::addr_net_ipv4_t> ();
				// Заводим адрес назначения пути
				route.destination = make_unique <net::addr_net_ipv4_t> ();
			// Если адрес является адресом IPv6
			} else if(source.ip->size == 16){
				// Заводим адрес шлюза пути
				route.gateway = make_unique <net::addr_net_ipv6_t> ();
				// Заводим адрес назначения пути
				route.destination = make_unique <net::addr_net_ipv6_t> ();
			// Если вид адреса неизвестен
			} else
				// Выходим из функции
				return;
			// Если объект работы с путями задан и путь по умолчанию получен
			if((this->_gateway != nullptr) && this->_gateway->get(route) && !route.ifname.empty()){
				// Запоминаем название сетевого устройства
				source.iface = ::move(route.ifname);
				// Выполняем получение адреса и аппаратного адреса устройства
				this->fillSource(source);
			}
			// Если адрес является адресом IPv4
			if(source.ip->size == 4){
				// Если адрес сетевого устройства получен
				if(awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0)
					// Запоминаем определённый адрес до устаревания записи
					::__awh_remember__(source);
				// Выходим из функции
				return;
			}
			/**
			 * Устройство пути по умолчанию годного адреса дать не смогло
			 *
			 * @note Так выходит на машине без пути IPv6 наружу: путём оказывается
			 *       туннель, а он держит один лишь канальный адрес
			 *
			 */
			if(!::__awh_routable__(source))
				// Выполняем поиск годного адреса среди прочих устройств
				::__awh_discover__(source);
			// Если адрес сетевого устройства получен
			if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], ::__awh_zero_ipv6__, 16) != 0)
				// Запоминаем определённый адрес до устаревания записи
				::__awh_remember__(source);
		} break;
		/**
		 * Если узел является одноранговым - отыскивается его аппаратный адрес
		 *
		 * @note Соседей канального уровня MS Windows отдаёт вызовом GetIpNetTable2 - за
		 *       тем, за чем у BSD перебирают таблицу путей с признаком записей
		 *       канального уровня, а у Linux спрашивают netlink
		 *
		 */
		case static_cast <uint8_t> (event::node_t::PEER): {
			// Если аппаратный адрес заполнять некуда
			if(source.mac == nullptr)
				// Выходим из функции
				return;
			// Перечень соседей канального уровня
			PMIB_IPNET_TABLE2 table = nullptr;
			// Семейство адресов искомого соседа
			const ADDRESS_FAMILY family = static_cast <ADDRESS_FAMILY> (source.ip->size == 16 ? AF_INET6 : AF_INET);
			// Если перечень соседей получить не удалось
			if(::GetIpNetTable2(family, &table) != NO_ERROR){
				// Выводим в журнал сообщение о невозможности опроса соседей
				this->_log->print("%s: unable to get the neighbour table", log_t::flag_t::WARNING, ::__AWH_ADDR_BACKEND__);
				// Выходим из функции
				return;
			}
			/**
			 * Выполняем перебор всех соседей канального уровня
			 */
			for(ULONG i = 0; i < table->NumEntries; i++){
				// Получаем очередного соседа канального уровня
				const MIB_IPNET_ROW2 & row = table->Table[i];
				// Если аппаратного адреса у соседа нет
				if(row.PhysicalAddressLength != 6)
					// Переходим к следующему соседу
					continue;
				// Признак совпадения адреса соседа с искомым
				bool matched = false;
				// Если ищется сосед по адресу IPv4
				if((source.ip->size == 4) && (row.Address.si_family == AF_INET))
					// Определяем совпадение адреса соседа с искомым
					matched = (row.Address.Ipv4.sin_addr.s_addr == awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address);
				// Если ищется сосед по адресу IPv6
				else if((source.ip->size == 16) && (row.Address.si_family == AF_INET6))
					// Определяем совпадение адреса соседа с искомым
					matched = (::memcmp(&row.Address.Ipv6.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(struct in6_addr)) == 0);
				// Если адрес соседа с искомым не совпал
				if(!matched)
					// Переходим к следующему соседу
					continue;
				// Запоминаем аппаратный адрес соседа
				::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], row.PhysicalAddress, 6);
				// Завершаем перебор соседей
				break;
			}
			// Выполняем освобождение перечня соседей
			::FreeMibTable(table);
		} break;
		// Если узел является клиентом
		case static_cast <uint8_t> (event::node_t::CLIENT):
		/**
		 * Если узел является сервером
		 *
		 * @note Подбор устройства ведётся либо по адресу, либо по аппаратному адресу:
		 *       заданный адрес ищется среди адресов устройств, а при пустом адресе
		 *       устройство отбирается совпадением аппаратного адреса
		 *
		 */
		case static_cast <uint8_t> (event::node_t::SERVER): {
			// Если приметы для подбора устройства нет
			if(::__awh_zero__(source))
				// Выходим из функции, оставляя выбор устройства системе
				return;
			// Буфер под перечень сетевых устройств
			std::vector <uint8_t> buffer;
			// Выполняем опрос перечня сетевых устройств
			PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
			// Если перечень устройств получить не удалось
			if(adapters == nullptr){
				// Выводим в журнал сообщение о невозможности опроса устройств
				this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_ADDR_BACKEND__);
				// Выходим из функции
				return;
			}
			// Определяем, ведётся ли подбор устройства по аппаратному адресу
			const bool byMac = ((source.mac != nullptr) && (::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ::__awh_zero_mac__, 6) != 0));
			/**
			 * Выполняем перебор всех сетевых устройств машины
			 */
			for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
				// Если подбор ведётся по аппаратному адресу
				if(byMac){
					// Если аппаратный адрес устройства с искомым не совпал
					if((adapter->PhysicalAddressLength != 6) || (::memcmp(adapter->PhysicalAddress, &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) != 0))
						// Переходим к следующему устройству
						continue;
					// Если название устройства получено
					if(adapter->AdapterName != nullptr)
						// Запоминаем название сетевого устройства
						source.iface = adapter->AdapterName;
					// Выполняем получение адреса устройства
					this->fillSource(source);
					// Выходим из функции
					return;
				}
				/**
				 * Выполняем перебор всех адресов устройства
				 */
				for(PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next){
					// Если адрес устройства не задан
					if(address->Address.lpSockaddr == nullptr)
						// Переходим к следующему адресу
						continue;
					// Признак совпадения адреса устройства с искомым
					bool matched = false;
					// Если ищется устройство по адресу IPv4
					if((source.ip->size == 4) && (address->Address.lpSockaddr->sa_family == AF_INET))
						// Определяем совпадение адреса устройства с искомым
						matched = (reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr == awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address);
					// Если ищется устройство по адресу IPv6
					else if((source.ip->size == 16) && (address->Address.lpSockaddr->sa_family == AF_INET6))
						// Определяем совпадение адреса устройства с искомым
						matched = (::memcmp(&reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr)->sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(struct in6_addr)) == 0);
					// Если адрес устройства с искомым не совпал
					if(!matched)
						// Переходим к следующему адресу
						continue;
					// Если название устройства получено
					if(adapter->AdapterName != nullptr)
						// Запоминаем название сетевого устройства
						source.iface = adapter->AdapterName;
					// Если аппаратный адрес задан и устройство его имеет
					if((source.mac != nullptr) && (adapter->PhysicalAddressLength == 6))
						// Запоминаем аппаратный адрес устройства
						::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], adapter->PhysicalAddress, 6);
					// Выходим из функции
					return;
				}
			}
		} break;
	}
}
