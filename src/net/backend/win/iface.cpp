/**
 * @file iface.cpp
 * @date 2026-08-07
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
 * @brief Реализация бэкенда работы с сетевыми устройствами для MS Windows —
 *        перечисление устройств, их признаки, адреса и размер кадра
 *
 * @details Слой этот отвечает эталонным backend/bsd/iface.cpp и backend/gnu/iface.cpp.
 *          Соответствия getifaddrs у MS Windows нет, и перечисление ведётся вызовом
 *          GetAdaptersAddresses: он отдаёт разом и устройства, и их адреса, и признаки,
 *          и размер кадра - то, за чем у POSIX ходят порознь
 *
 * @note Названием устройства здесь считается имя вида «{GUID}», какое система заводит
 *       сама и какое неизменно, а не описание вроде «Ethernet 2», какое пользователь
 *       вправе переименовать. Опираться на переименуемое имя нельзя: устройство
 *       осталось бы тем же, а обращение к нему перестало бы работать
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstring>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/eth.hpp>
#include <net/backend/win/tunnel.hpp>

/**
 * @brief Средства опроса сетевых устройств
 *
 * @details Объявлены они у MS Windows в отдельных заголовках, а не там же, где прочие
 *          средства сокетов, как то заведено у систем POSIX
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
static constexpr const char * __AWH_IFACE_BACKEND__ = "MS Windows interface backend";

/**
 * @brief Инкапсулируем состояние слоя в пространство имён
 *
 */
namespace {
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
	 * @details Объём, потребный под перечень, заранее не известен, и система сообщает
	 *          его сама отказом ERROR_BUFFER_OVERFLOW. Опрос потому ведётся дважды:
	 *          первый раз ради объёма, второй ради самого перечня. Между ними
	 *          устройство вправе появиться, оттого попыток отводится несколько
	 *
	 * @param buffer буфер, в котором остаётся перечень устройств
	 * @param family семейство адресов, какое требуется опросить
	 * @return       указатель на первое устройство перечня, либо nullptr при отказе
	 *
	 */
	PIP_ADAPTER_ADDRESSES __awh_adapters__(std::vector <uint8_t> & buffer, const ULONG family = AF_UNSPEC) noexcept {
		/**
		 * Состав запрашиваемых сведений
		 *
		 * @note Всё лишнее отсекается намеренно: опрос это не бесплатный, а сведения о
		 *       серверах DNS, WINS и о шлюзах слою этому не нужны вовсе
		 *
		 */
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
			const ULONG result = ::GetAdaptersAddresses(family, flags, nullptr, reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data()), &size);
			// Если перечень устройств получен
			if(result == ERROR_SUCCESS)
				// Возвращаем указатель на первое устройство перечня
				return reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data());
			// Если объёма буфера не хватило - повторяем опрос с объёмом, названным системой
			if(result != ERROR_BUFFER_OVERFLOW)
				// Возвращаем признак отказа опроса
				return nullptr;
		}
		// Возвращаем признак отказа опроса
		return nullptr;
	}

	/**
	 * @brief Функция перевода широкой записи в однобайтовую
	 *
	 * @param value переводимая запись
	 * @return      переведённая запись
	 *
	 */
	std::string __awh_narrow__(const wchar_t * value) noexcept {
		// Если запись не передана
		if((value == nullptr) || (value[0] == L'\0'))
			// Возвращаем пустой результат
			return std::string();
		// Получаем объём, потребный под переведённую запись
		const int32_t size = ::WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
		// Если объём получить не удалось
		if(size <= 1)
			// Возвращаем пустой результат
			return std::string();
		// Отводим место под переведённую запись
		std::vector <char> buffer(static_cast <size_t> (size), 0);
		// Выполняем перевод записи
		if(::WideCharToMultiByte(CP_UTF8, 0, value, -1, buffer.data(), size, nullptr, nullptr) <= 0)
			// Возвращаем пустой результат
			return std::string();
		// Возвращаем переведённую запись без завершающего нуля
		return std::string(buffer.data());
	}

	/**
	 * @brief Функция получения местного номера устройства по его названию
	 *
	 * @details Настройка устройств у MS Windows ведётся не по названию, а по местному
	 *          номеру - неизменному числу, каким система метит устройство внутри себя
	 *
	 * @param name название искомого устройства
	 * @param luid местный номер найденного устройства
	 * @return     признак того, что устройство найдено
	 *
	 */
	bool __awh_luid__(string_view name, NET_LUID & luid) noexcept {
		// Если название устройства не передано
		if(name.empty())
			// Выводим признак того, что устройство не найдено
			return false;
		// Буфер под перечень сетевых устройств
		std::vector <uint8_t> buffer;
		// Выполняем опрос перечня сетевых устройств
		PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
		// Если перечень устройств получить не удалось
		if(adapters == nullptr)
			// Выводим признак того, что устройство не найдено
			return false;
		/**
		 * Выполняем перебор всех сетевых устройств машины
		 */
		for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
			// Если название устройства с искомым совпало
			if((adapter->AdapterName != nullptr) && ::__awh_iface_same__(name, adapter->AdapterName)){
				// Запоминаем местный номер найденного устройства
				luid = adapter->Luid;
				// Выводим признак того, что устройство найдено
				return true;
			}
		}
		// Выводим признак того, что устройство не найдено
		return false;
	}
};

/**
 * @brief Метод получения перечня доступных сетевых устройств
 *
 * @return перечень названий доступных сетевых устройств
 *
 */
unordered_set <string> awh::eth::Interface::available() const noexcept {
	// Переменная результата
	unordered_set <string> result;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Записываем ошибку в лог
		this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Возвращаем пустой результат
		return result;
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства получено
		if(adapter->AdapterName != nullptr)
			// Добавляем название устройства в перечень
			result.emplace(adapter->AdapterName);
	}
	// Возвращаем собранный перечень устройств
	return result;
}

/**
 * @brief Метод проверки доступности сетевого устройства
 *
 * @param name название сетевого устройства
 * @return     признак доступности сетевого устройства
 *
 */
bool awh::eth::Interface::isAvailable(string_view name) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем признак недоступности устройства
		return false;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr)
		// Возвращаем признак недоступности устройства
		return false;
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства совпало с искомым
		if((adapter->AdapterName != nullptr) && ::__awh_iface_same__(name, adapter->AdapterName))
			// Возвращаем признак доступности устройства
			return true;
	}
	// Возвращаем признак недоступности устройства
	return false;
}

/**
 * @brief Метод получения размера кадра сетевого устройства
 *
 * @param name название сетевого устройства
 * @return     размер кадра сетевого устройства
 *
 */
uint32_t awh::eth::Interface::mtu(string_view name) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем пустой размер кадра
		return 0;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Записываем ошибку в лог
		this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Возвращаем пустой размер кадра
		return 0;
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства совпало с искомым
		if((adapter->AdapterName != nullptr) && ::__awh_iface_same__(name, adapter->AdapterName))
			// Возвращаем размер кадра устройства
			return static_cast <uint32_t> (adapter->Mtu);
	}
	// Возвращаем пустой размер кадра
	return 0;
}

/**
 * @brief Метод получения признаков сетевого устройства
 *
 * @param name название сетевого устройства
 * @return     перечень признаков сетевого устройства
 *
 * @details Признаки у MS Windows разложены по разным полям, а не собраны в одном
 *          битовом наборе, как то заведено полем ifi_flags у систем POSIX. Собираются
 *          они здесь:
 *
 *          | Признак | Откуда берётся |
 *          |---|---|
 *          | `UP`, `RUNNING` | `OperStatus == IfOperStatusUp` |
 *          | `LOOPBACK` | `IfType == IF_TYPE_SOFTWARE_LOOPBACK` |
 *          | `POINTTOPOINT` | `IfType == IF_TYPE_PPP` либо `IF_TYPE_TUNNEL` |
 *          | `MULTICAST` | отсутствие `IP_ADAPTER_NO_MULTICAST` |
 *          | `BROADCAST` | отсутствие `LOOPBACK` и `POINTTOPOINT` |
 *
 * @note Признаков `DEBUG`, `NOARP`, `ALLMULTI`, `PROMISC` и `DYNAMIC` система не
 *       отдаёт вовсе, и в перечне их потому не бывает. Отсутствие признака здесь
 *       означает «система о нём не сообщает», а не «признак снят»
 *
 */
unordered_set <awh::event::eth_flag_t> awh::eth::Interface::flags(string_view name) const noexcept {
	// Переменная результата
	unordered_set <event::eth_flag_t> result;
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем пустой перечень признаков
		return result;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Записываем ошибку в лог
		this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Возвращаем пустой перечень признаков
		return result;
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства с искомым не совпало
		if((adapter->AdapterName == nullptr) || !::__awh_iface_same__(name, adapter->AdapterName))
			// Переходим к следующему устройству
			continue;
		// Если устройство работает
		if(adapter->OperStatus == IfOperStatusUp){
			// Добавляем признак поднятого устройства
			result.emplace(event::eth_flag_t::UP);
			// Добавляем признак работающего устройства
			result.emplace(event::eth_flag_t::RUNNING);
		}
		// Признак того, что устройство является петлевым
		const bool loopback = (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK);
		// Признак того, что устройство является двухточечным
		const bool peer = ((adapter->IfType == IF_TYPE_PPP) || (adapter->IfType == IF_TYPE_TUNNEL));
		// Если устройство является петлевым
		if(loopback)
			// Добавляем признак петлевого устройства
			result.emplace(event::eth_flag_t::LOOPBACK);
		// Если устройство является двухточечным
		if(peer)
			// Добавляем признак двухточечного устройства
			result.emplace(event::eth_flag_t::POINTTOPOINT);
		// Если групповая рассылка устройством не запрещена
		if(!(adapter->Flags & IP_ADAPTER_NO_MULTICAST))
			// Добавляем признак групповой рассылки
			result.emplace(event::eth_flag_t::MULTICAST);
		// Если устройство не является ни петлевым, ни двухточечным
		if(!loopback && !peer)
			// Добавляем признак широковещательной рассылки
			result.emplace(event::eth_flag_t::BROADCAST);
		// Завершаем перебор устройств
		break;
	}
	// Возвращаем собранный перечень признаков
	return result;
}

/**
 * @brief Метод проверки того, что сетевое устройство является туннелем
 *
 * @param name название сетевого устройства
 * @return     признак того, что устройство является туннелем
 *
 * @note Опрос устройств туннели узнаёт не полностью. Вид IF_TYPE_TUNNEL система
 *       выставляет своим встроенным туннелям, а устройствам сторонних драйверов -
 *       нет: Wintun объявляется сборным видом IF_TYPE_PROP_VIRTUAL, каким объявлены
 *       и устройства виртуальных машин, а tap-windows6 и вовсе обыкновенным
 *       устройством Ethernet, ибо кадры канального уровня он и переносит
 *
 *       Оттого прежде опроса проверяется собственный перечень заведённых туннелей:
 *       что завело само приложение, то оно знает наверняка. Чужой же туннель
 *       стороннего драйвера от иного виртуального устройства не отличим, и признак
 *       этот для него будет отрицательным
 *
 */
bool awh::eth::Interface::isTunnel(string_view name) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем признак того, что устройство туннелем не является
		return false;
	// Если устройство заведено этим же приложением
	if(win::tunnel::find(string(name)) != net::invalid_socket_t)
		// Возвращаем признак того, что устройство является туннелем
		return true;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr)
		// Возвращаем признак того, что устройство туннелем не является
		return false;
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства совпало с искомым
		if((adapter->AdapterName != nullptr) && ::__awh_iface_same__(name, adapter->AdapterName))
			// Возвращаем признак того, чем устройство является
			return (adapter->IfType == IF_TYPE_TUNNEL);
	}
	// Возвращаем признак того, что устройство туннелем не является
	return false;
}

/**
 * @brief Метод получения адреса сетевого устройства
 *
 * @param name   название сетевого устройства
 * @param family семейство адресов
 * @return       адрес сетевого устройства
 *
 * @note Выбор адреса повторяет эталонные бэкенды: у IPv4 берётся первый найденный, у
 *       IPv6 предпочитается глобальный адресу канальной связи. Довод тот же - адрес
 *       канальной связи за пределы сегмента не уходит, и отдавать его вызывающей
 *       стороне, когда есть глобальный, значило бы отдавать заведомо непригодный
 *
 */
unique_ptr <awh::net::addr_t> awh::eth::Interface::getAddress(string_view name, const event::family_t family) const noexcept {
	// Переменная результата
	unique_ptr <net::addr_t> result = nullptr;
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем пустой результат
		return result;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Записываем ошибку в лог
		this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Возвращаем пустой результат
		return result;
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства с искомым не совпало
		if((adapter->AdapterName == nullptr) || !::__awh_iface_same__(name, adapter->AdapterName))
			// Переходим к следующему устройству
			continue;
		// Если устройство не работает - адреса его непригодны
		if(adapter->OperStatus != IfOperStatusUp)
			// Переходим к следующему устройству
			continue;
		/**
		 * Выполняем перебор всех адресов устройства
		 */
		for(PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next){
			// Если адрес устройства не задан
			if((address->Address.lpSockaddr == nullptr))
				// Переходим к следующему адресу
				continue;
			/**
			 * Определяем семейство адреса устройства
			 */
			switch(address->Address.lpSockaddr->sa_family){
				// Если адрес устройства является адресом IPv4
				case AF_INET: {
					// Если запрошено иное семейство адресов
					if(family != event::family_t::IPV4)
						// Переходим к следующему адресу
						continue;
					// Создаём объект для хранения адреса IPv4
					result = make_unique <net::addr_net_ipv4_t> ();
					// Копируем адрес устройства в результат
					awh_cast <net::addr_net_ipv4_t *> (result.get())->address = reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr;
					// Запоминаем длину префикса сети
					awh_cast <net::addr_net_ipv4_t *> (result.get())->prefix = static_cast <uint8_t> (address->OnLinkPrefixLength);
					// Возвращаем найденный адрес устройства
					return result;
				}
				// Если адрес устройства является адресом IPv6
				case AF_INET6: {
					// Если запрошено иное семейство адресов
					if(family != event::family_t::IPV6)
						// Переходим к следующему адресу
						continue;
					// Получаем адрес устройства вида IPv6
					struct sockaddr_in6 * value = reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr);
					// Определяем, является ли адрес адресом канальной связи
					const bool link = IN6_IS_ADDR_LINKLOCAL(&value->sin6_addr);
					// Если адрес ещё не найден либо найден глобальный взамен канального
					if((result == nullptr) || !link){
						// Создаём объект для хранения адреса IPv6
						result = make_unique <net::addr_net_ipv6_t> ();
						// Копируем адрес устройства в результат
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (result.get())->address[0], &value->sin6_addr, sizeof(struct in6_addr));
						// Запоминаем длину префикса сети
						awh_cast <net::addr_net_ipv6_t *> (result.get())->prefix = static_cast <uint8_t> (address->OnLinkPrefixLength);
						// Запоминаем номер устройства зоны адреса
						awh_cast <net::addr_net_ipv6_t *> (result.get())->zone = static_cast <uint32_t> (value->sin6_scope_id);
						// Если найден глобальный адрес - поиск на нём и оканчивается
						if(!link)
							// Возвращаем найденный адрес устройства
							return result;
					}
				} break;
			}
		}
		// Завершаем перебор устройств
		break;
	}
	// Возвращаем найденный адрес устройства
	return result;
}

/**
 * @brief Метод получения названия сетевого устройства по его адресу
 *
 * @param addr адрес сетевого устройства
 * @return     название сетевого устройства
 *
 */
string awh::eth::Interface::name(const net::addr_t * addr) const noexcept {
	// Если адрес устройства не передан
	if(addr == nullptr)
		// Возвращаем пустой результат
		return string();
	/**
	 * Отсеиваем неназначенный адрес
	 *
	 * @details Адрес из одних нулей (`0.0.0.0`, `::`, нулевой MAC) означает «любой», а
	 *          не адрес какого-то устройства: искать по нему нечего, и ответом обязана
	 *          быть пустота
	 *
	 * @note Замером у MS Windows дефекта НЕ показано, хотя условие для него на стенде
	 *       было - семь связей без назначенного адреса: перебор идёт по
	 *       `FirstUnicastAddress`, то есть по НАЗНАЧЕННЫМ адресам, и связь без адреса
	 *       записей там не имеет вовсе. У систем Sun перебор устроен иначе, и там связь
	 *       без адреса выдавалась с нулевым адресом - розыск возвращал её имя (найдено
	 *       соседней сессией на Solaris и OpenIndiana). Заслон ставится ради ЕДИНОГО
	 *       договора движков, а не по измеренному здесь отказу
	 */
	switch(addr->size){
		// Для адреса IPv4
		case 4: {
			// Если адрес не назначен
			if(awh_cast <const net::addr_net_ipv4_t *> (addr)->address == 0)
				// Возвращаем пустой результат
				return string();
		} break;
		// Для адреса IPv6
		case 16: {
			// Признак того, что адрес состоит из одних нулей
			bool empty = true;
			// Перебираем октеты переданного адреса
			for(uint8_t i = 0; (i < 16) && empty; i++)
				// Запоминаем, остаётся ли адрес пустым
				empty = (awh_cast <const net::addr_net_ipv6_t *> (addr)->address[i] == 0);
			// Если адрес не назначен
			if(empty)
				// Возвращаем пустой результат
				return string();
		} break;
		// Для адреса канального уровня
		case 6: {
			// Признак того, что адрес состоит из одних нулей
			bool empty = true;
			// Перебираем октеты переданного адреса
			for(uint8_t i = 0; (i < 6) && empty; i++)
				// Запоминаем, остаётся ли адрес пустым
				empty = (awh_cast <const net::addr_mac_t *> (addr)->address[i] == 0);
			// Если адрес не назначен
			if(empty)
				// Возвращаем пустой результат
				return string();
		} break;
	}
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr){
		// Записываем ошибку в лог
		this->_log->print("%s: unable to get list of network interfaces", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Возвращаем пустой результат
		return string();
	}
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства не задано
		if(adapter->AdapterName == nullptr)
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
			/**
			 * Определяем семейство адреса устройства
			 */
			switch(address->Address.lpSockaddr->sa_family){
				// Если адрес устройства является адресом IPv4
				case AF_INET: {
					// Если искомый адрес имеет иной размер
					if(addr->size != 4)
						// Переходим к следующему адресу
						continue;
					// Если адрес устройства совпал с искомым
					if(reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr == awh_cast <const net::addr_net_ipv4_t *> (addr)->address)
						// Возвращаем название найденного устройства
						return string(adapter->AdapterName);
				} break;
				// Если адрес устройства является адресом IPv6
				case AF_INET6: {
					// Если искомый адрес имеет иной размер
					if(addr->size != 16)
						// Переходим к следующему адресу
						continue;
					// Если адрес устройства совпал с искомым
					if(::memcmp(&reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr)->sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], sizeof(struct in6_addr)) == 0)
						// Возвращаем название найденного устройства
						return string(adapter->AdapterName);
				} break;
			}
		}
	}
	// Возвращаем пустой результат
	return string();
}

/**
 * @brief Метод проверки того, что сетевое устройство является виртуальным
 *
 * @param name название сетевого устройства
 * @return     признак того, что устройство является виртуальным
 *
 * @note Признака «устройство виртуально» система не отдаёт вовсе, и ответ собирается
 *       по виду устройства: туннели, петлевое устройство и программные соединения
 *       вида PPP настоящим оборудованием не подкреплены. Ответ этот приблизителен, и
 *       иным он у MS Windows быть не может - виртуальность там не свойство устройства,
 *       а свойство драйвера, который его завёл
 *
 */
bool awh::eth::Interface::isVirtual(string_view name) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем признак того, что устройство виртуальным не является
		return false;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr)
		// Возвращаем признак того, что устройство виртуальным не является
		return false;
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства с искомым не совпало
		if((adapter->AdapterName == nullptr) || !::__awh_iface_same__(name, adapter->AdapterName))
			// Переходим к следующему устройству
			continue;
		/**
		 * Определяем вид сетевого устройства
		 */
		switch(adapter->IfType){
			// Если устройство является туннелем
			case IF_TYPE_TUNNEL:
			// Если устройство является петлевым
			case IF_TYPE_SOFTWARE_LOOPBACK:
			// Если устройство является программным соединением
			case IF_TYPE_PPP:
			/**
			 * Если устройство объявлено сборным видом «виртуальное»
			 *
			 * @note Видом этим объявляются устройства, каких нет в оборудовании вовсе:
			 *       туннели сторонних драйверов вроде Wintun, устройства виртуальных
			 *       машин, мосты. Чем именно устройство является, вид этот не сообщает
			 *
			 */
			case IF_TYPE_PROP_VIRTUAL:
				// Возвращаем признак виртуального устройства
				return true;
		}
		// Завершаем перебор устройств
		break;
	}
	// Возвращаем признак того, что устройство виртуальным не является
	return false;
}

/**
 * @brief Метод установки размера кадра сетевого устройства
 *
 * @param name название сетевого устройства
 * @param mtu  размер кадра сетевого устройства
 * @return     результат выполнения установки
 *
 * @note Размер кадра задаётся порознь для IPv4 и IPv6: у MS Windows это две разные
 *       записи настроек одного устройства, тогда как у систем POSIX размер этот один
 *       на устройство. Устанавливается он здесь обеим - тем поведение приводится к
 *       общему виду
 *
 * @warning Настройка эта требует надзорных прав. Их отсутствие отвечает отказом
 *          ERROR_ACCESS_DENIED, и отказ этот заносится в журнал, а не скрывается
 *
 */
bool awh::eth::Interface::mtu(string_view name, const uint32_t mtu) const noexcept {
	// Если название сетевого устройства не передано либо размер кадра пуст
	if(name.empty() || (mtu == 0))
		// Возвращаем отрицательный результат установки
		return false;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr)
		// Возвращаем отрицательный результат установки
		return false;
	// Результат выполнения установки
	bool result = false;
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства с искомым не совпало
		if((adapter->AdapterName == nullptr) || !::__awh_iface_same__(name, adapter->AdapterName))
			// Переходим к следующему устройству
			continue;
		/**
		 * Выставляем размер кадра обоим семействам адресов
		 */
		for(const ADDRESS_FAMILY family : {static_cast <ADDRESS_FAMILY> (AF_INET), static_cast <ADDRESS_FAMILY> (AF_INET6)}){
			// Настройки устройства для семейства адресов
			MIB_IPINTERFACE_ROW row{};
			// Выполняем начальную подготовку настроек устройства
			::InitializeIpInterfaceEntry(&row);
			// Устанавливаем семейство адресов настроек
			row.Family = family;
			// Устанавливаем местный номер устройства
			row.InterfaceLuid = adapter->Luid;
			// Если снять нынешние настройки устройства не удалось
			if(::GetIpInterfaceEntry(&row) != NO_ERROR)
				// Переходим к следующему семейству адресов
				continue;
			// Устанавливаем размер кадра устройства
			row.NlMtu = static_cast <ULONG> (mtu);
			/**
			 * Поле это система требует обнулить перед записью
			 *
			 * @note Требование описано самой системой: SetIpInterfaceEntry отвечает
			 *       отказом ERROR_INVALID_PARAMETER, если оставить снятое значение
			 *
			 */
			// Обнуляем срок достижимости устройства
			row.SitePrefixLength = 0;
			// Выполняем запись настроек устройства
			const DWORD code = ::SetIpInterfaceEntry(&row);
			// Если настройки устройства записаны
			if(code == NO_ERROR)
				// Запоминаем положительный результат установки
				result = true;
			// Если записать настройки устройства не удалось
			else this->_log->print("%s: interface MTU could not be set, error %lu", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, code);
		}
		// Завершаем перебор устройств
		break;
	}
	// Возвращаем результат выполнения установки
	return result;
}

/**
 * @brief Метод создания сетевого устройства
 *
 * @param type вид создаваемого сетевого устройства
 * @param name название создаваемого сетевого устройства
 * @return     дескриптор созданного сетевого устройства
 *
 * @details Встроенного туннельного устройства у MS Windows нет вовсе, и приносит его
 *          сторонний драйвер. Их два: Wintun переносит пакеты сетевого уровня, а
 *          tap-windows6 - кадры канального уровня вместе с аппаратными адресами
 *
 * @note Каким драйвером заводить устройство, задаётся настройкой driver. Значение
 *       AUTO, установленное изначально, поручает выбор самому модулю: кадры
 *       канального уровня переносит один tap-windows6, пакеты же сетевого переносят
 *       оба, и при доступности обоих берётся Wintun - обмен у него идёт кольцом в
 *       общей с драйвером памяти, без обращения к системе на каждый пакет
 *
 *       Заданный явно драйвер подмене не подлежит: недоступный отвечает отказом, а
 *       не молчаливой заменой на другой. Устройства эти неравнозначны, и подмена
 *       изменила бы поведение того, что через них идёт
 *
 * @warning Создание требует надзорных прав. Устройства tap-windows6 приложением не
 *          заводятся вовсе - их ставит установщик драйвера, а занимается здесь
 *          свободное
 *
 */
awh::net::socket_t awh::eth::Interface::create(const event::eth_t type, string & name) const noexcept {
	// Если создаётся устройство неподдерживаемого вида
	if((type != event::eth_t::TUN) && (type != event::eth_t::TAP)){
		// Выводим в журнал сообщение о неподдерживаемом виде устройства
		this->_log->print("%s: only TUN and TAP devices can be created", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Выводим пустой дескриптор
		return net::invalid_socket_t;
	}
	// Драйвер, каким заводится устройство
	win::tunnel::driver_t driver = win::tunnel::driver_t::NONE;
	/**
	 * Определяем установленный драйвер туннельных устройств
	 */
	switch(static_cast <uint8_t> (this->_driver)){
		// Если драйвер задан вызывающим явно
		case static_cast <uint8_t> (driver_t::WINTUN):
			// Запоминаем заданный драйвер
			driver = win::tunnel::driver_t::WINTUN;
		break;
		// Если драйвер задан вызывающим явно
		case static_cast <uint8_t> (driver_t::TAP):
			// Запоминаем заданный драйвер
			driver = win::tunnel::driver_t::TAP;
		break;
		/**
		 * Если выбор драйвера поручен модулю
		 *
		 * @note Кадры канального уровня переносит один tap-windows6, и выбора здесь
		 *       нет вовсе. Пакеты же сетевого уровня переносят оба, и при доступности
		 *       обоих предпочитается Wintun: обмен у него идёт кольцом в общей с
		 *       драйвером памяти, без обращения к системе на каждый пакет
		 *
		 */
		default: {
			// Если создаётся перенос пакетов сетевого уровня и драйвер Wintun доступен
			if((type == event::eth_t::TUN) && win::tunnel::available(win::tunnel::driver_t::WINTUN))
				// Запоминаем выбранный драйвер
				driver = win::tunnel::driver_t::WINTUN;
			// Если драйвер tap-windows6 доступен
			else if(win::tunnel::available(win::tunnel::driver_t::TAP))
				// Запоминаем выбранный драйвер
				driver = win::tunnel::driver_t::TAP;
			// Если ни одного драйвера на машине не нашлось
			else {
				// Выводим в журнал сообщение об отсутствии драйверов
				this->_log->print("%s: neither Wintun nor tap-windows6 is available on this machine", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			}
		}
	}
	/**
	 * Закрепляем сделанный выбор драйвера
	 *
	 * @details Выбор, поручённый модулю значением AUTO, делался в местной переменной, а
	 *          поле оставалось прежним - и `driver()` отдавал AUTO даже после того, как
	 *          устройство завелось. Узнать, каким драйвером оно заведено на деле,
	 *          вызывающему было нечем: годность драйверов наружу не выведена, а два
	 *          драйвера у MS Windows несут разное - Wintun пакеты сетевого уровня, а
	 *          tap-windows6 кадры канального
	 *
	 * @note Закрепляется лишь выбор, СДЕЛАННЫЙ ЗДЕСЬ: драйвер, названный вызывающим
	 *       явно, и без того лежит в поле, а переписывать его значением того же смысла
	 *       незачем
	 *
	 */
	if(this->_driver == driver_t::AUTO)
		// Запоминаем драйвер, каким устройство заводится на деле
		this->_driver = static_cast <driver_t> (static_cast <uint8_t> (driver));
	// Выполняем создание туннельного устройства выбранным драйвером
	return win::tunnel::create(type, driver, name, this->_log);
}
/**
 * @brief Метод получения драйвера туннельных устройств
 *
 * @return драйвер, каким заводятся туннельные устройства
 *
 */
awh::eth::Interface::driver_t awh::eth::Interface::driver() const noexcept {
	// Выводим установленный драйвер туннельных устройств
	return this->_driver;
}
/**
 * @brief Метод установки драйвера туннельных устройств
 *
 * @param driver драйвер туннельных устройств для установки
 *
 */
void awh::eth::Interface::driver(const driver_t driver) noexcept {
	// Выполняем установку драйвера туннельных устройств
	this->_driver = driver;
}
/**
 * @brief Метод удаления сетевого устройства
 *
 * @param name название удаляемого сетевого устройства
 * @return     результат выполнения удаления
 *
 * @note Удаляются здесь лишь устройства, заведённые этим же приложением: сторонние
 *       заводит установщик драйвера, и снимать их приложению не положено
 *
 */
bool awh::eth::Interface::destroy(string_view name) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Выводим отрицательный результат удаления
		return false;
	// Выполняем поиск устройства среди заведённых
	const net::socket_t sock = win::tunnel::find(string(name));
	// Если устройство среди заведённых не значится
	if(sock == net::invalid_socket_t){
		// Выводим в журнал сообщение о невозможности удаления устройства
		this->_log->print("%s: interface \"%s\" was not created by this application and cannot be removed", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(name).c_str());
		// Выводим отрицательный результат удаления
		return false;
	}
	// Выполняем удаление найденного устройства
	return win::tunnel::destroy(sock, this->_log);
}

/**
 * @brief Метод установки адреса сетевого устройства
 *
 * @param name   название сетевого устройства
 * @param ip     устанавливаемый адрес
 * @param prefix длина префикса сети
 * @return       результат выполнения установки
 *
 * @note Адрес добавляется к устройству, а не замещает прежние: у MS Windows за
 *       устройством числится перечень адресов, и понятия «единственного» адреса нет
 *       вовсе
 *
 * @warning Ответ ERROR_OBJECT_ALREADY_EXISTS сам по себе за успех НЕ принимается:
 *          система отвечает так и тогда, когда адрес занят ДРУГИМ устройством машины.
 *          Успехом он считается лишь после сличения записи по нашему номеру
 *          устройства - подробности у самого места разбора
 *
 * @warning Установка требует надзорных прав и переживает завершение процесса
 *
 */
bool awh::eth::Interface::setAddress(string_view name, const net::addr_t * ip, const uint8_t prefix) const noexcept {
	// Если название устройства либо адрес не переданы
	if(name.empty() || (ip == nullptr))
		// Выводим отрицательный результат установки
		return false;
	// Местный номер сетевого устройства
	NET_LUID luid{};
	// Если сетевое устройство найти не удалось
	if(!::__awh_luid__(name, luid)){
		// Выводим в журнал сообщение о ненайденном устройстве
		this->_log->print("%s: interface \"%s\" was not found", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(name).c_str());
		// Выводим отрицательный результат установки
		return false;
	}
	// Устанавливаемая запись адреса устройства
	MIB_UNICASTIPADDRESS_ROW row{};
	// Выполняем начальную подготовку записи адреса
	::InitializeUnicastIpAddressEntry(&row);
	// Устанавливаем местный номер устройства
	row.InterfaceLuid = luid;
	// Устанавливаем длину префикса сети
	row.OnLinkPrefixLength = static_cast <uint8_t> (prefix);
	/**
	 * Состояние адреса выставляется годным сразу
	 *
	 * @note Иначе система запускает проверку на повторение адреса в сети, и до её
	 *       окончания устройство адресом не пользуется. Туннелю проверка эта не нужна
	 *       вовсе: у него нет соседей, с которыми адрес мог бы столкнуться
	 *
	 */
	row.DadState = IpDadStatePreferred;
	/**
	 * Определяем семейство устанавливаемого адреса
	 */
	switch(ip->size){
		// Если устанавливается адрес IPv4
		case 4: {
			// Устанавливаем семейство адреса
			row.Address.Ipv4.sin_family = AF_INET;
			// Устанавливаем сам адрес
			row.Address.Ipv4.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (ip)->address;
		} break;
		// Если устанавливается адрес IPv6
		case 16: {
			// Устанавливаем семейство адреса
			row.Address.Ipv6.sin6_family = AF_INET6;
			// Устанавливаем сам адрес
			::memcpy(&row.Address.Ipv6.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (ip)->address[0], sizeof(struct in6_addr));
			// Устанавливаем номер устройства зоны адреса
			row.Address.Ipv6.sin6_scope_id = static_cast <ULONG> (awh_cast <const net::addr_net_ipv6_t *> (ip)->zone);
		} break;
		// Если семейство адреса определить не удалось
		default: {
			// Выводим в журнал сообщение о неподдерживаемом виде адреса
			this->_log->print("%s: only IPv4 and IPv6 addresses can be assigned to an interface", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
			// Выводим отрицательный результат установки
			return false;
		}
	}
	// Выполняем установку адреса устройства
	const DWORD code = ::CreateUnicastIpAddressEntry(&row);
	// Если адрес устройства установлен
	if(code == NO_ERROR)
		// Выводим положительный результат установки
		return true;
	/**
	 * Ответ «объект уже существует» согласием НЕ считается, покуда не сличён номер устройства
	 *
	 * @details Система отвечает так и тогда, когда адрес занят ДРУГИМ устройством машины, -
	 *          а такое случается сплошь и рядом: устройство tap-windows6 живёт в системе
	 *          постоянно, приложением не сносится, и назначенный ему адрес переживает
	 *          прогон. Следующее устройство, заведённое драйвером Wintun, получало от
	 *          системы «уже есть», движок считал дело сделанным - и устройство оставалось
	 *          БЕЗ АДРЕСА вовсе. Отправлять с него нечем: путь к встречной стороне проложен,
	 *          сосед недостижим, и система отвечает отправителю «ресурс временно недоступен»
	 *          бесконечно
	 *
	 * @note Установлено щупом на Windows ARM64 20.08.2026: адрес 10.77.0.1 числился за
	 *       ОТКЛЮЧЁННЫМ устройством «Подключение по локальной сети» в состоянии Deprecated,
	 *       а работающее устройство AWH стояло без адреса. Снятие застрявшего адреса
	 *       возвращало перенос пакета немедленно - 302 мс против бесконечного ожидания
	 *
	 * @warning Сличается именно ЗАПИСЬ ПО НАШЕМУ НОМЕРУ устройства, а не наличие адреса в
	 *          системе: второе как раз и вводило в заблуждение
	 */
	if(code == ERROR_OBJECT_ALREADY_EXISTS){
		// Запись адреса для сличения
		MIB_UNICASTIPADDRESS_ROW check{};
		// Выполняем начальную подготовку записи сличения
		::InitializeUnicastIpAddressEntry(&check);
		// Устанавливаем местный номер нашего устройства
		check.InterfaceLuid = luid;
		// Переносим сам адрес
		check.Address = row.Address;
		// Если адрес числится именно за нашим устройством
		if(::GetUnicastIpAddressEntry(&check) == NO_ERROR)
			// Выводим положительный результат установки
			return true;
		// Выводим в журнал сообщение о занятости адреса другим устройством
		this->_log->print("%s: address is already assigned to another interface, \"%s\" left without it", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(name).c_str());
		// Выводим отрицательный результат установки
		return false;
	}
	// Выводим в журнал сообщение о невозможности установки адреса
	this->_log->print("%s: interface address could not be assigned, error %lu", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, code);
	// Выводим отрицательный результат установки
	return false;
}
/**
 * @brief Метод снятия адреса сетевого устройства и пути к тому концу связи
 *
 * @param name название сетевого устройства
 * @param ip   снимаемый адрес
 * @param peer адрес того конца связи, путь к которому снимается
 * @return     результат выполнения снятия
 *
 * @details Снимает ровно то, что поставила установка адреса, и снимает молча: отказ
 *          системы записью в журнал не сопровождается. Причина в том, что снятие идёт
 *          при свёртывании туннеля, когда устройство может быть уже снесено вместе со
 *          всем, что за ним числилось, - и отказ «нет такого объекта» здесь законен
 *
 * @warning Снятие ОБЯЗАТЕЛЬНО у MS Windows и не нужно у систем POSIX: устройство
 *          драйвера tap-windows6 живёт в системе постоянно, приложением не сносится, и
 *          назначенный ему адрес переживает прогон. Установлено на Windows ARM64
 *          20.08.2026: адреса 10.77.0.1 и fd77:aa::1 вместе с путём 10.77.0.2/32
 *          оставались на устройстве от прежних прогонов, и следующее устройство,
 *          заведённое драйвером Wintun, оставалось без адреса вовсе
 *
 */
bool awh::eth::Interface::delAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer) const noexcept {
	// Если название сетевого устройства либо адрес не переданы
	if(name.empty() || (ip == nullptr))
		// Выводим отрицательный результат снятия
		return false;
	// Местный номер сетевого устройства
	NET_LUID luid{};
	// Если сетевое устройство найти не удалось
	if(!::__awh_luid__(name, luid))
		// Выводим отрицательный результат снятия
		return false;
	// Признак снятого адреса
	bool result = false;
	// Снимаемая запись адреса устройства
	MIB_UNICASTIPADDRESS_ROW row{};
	// Выполняем начальную подготовку записи адреса
	::InitializeUnicastIpAddressEntry(&row);
	// Устанавливаем местный номер устройства
	row.InterfaceLuid = luid;
	/**
	 * Определяем семейство снимаемого адреса
	 */
	switch(ip->size){
		// Если снимается адрес IPv4
		case 4: {
			// Устанавливаем семейство адреса
			row.Address.Ipv4.sin_family = AF_INET;
			// Устанавливаем сам адрес
			row.Address.Ipv4.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (ip)->address;
		} break;
		// Если снимается адрес IPv6
		case 16: {
			// Устанавливаем семейство адреса
			row.Address.Ipv6.sin6_family = AF_INET6;
			// Устанавливаем сам адрес
			::memcpy(&row.Address.Ipv6.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (ip)->address[0], sizeof(struct in6_addr));
		} break;
		// Если семейство адреса определить не удалось
		default: return false;
	}
	// Если запись адреса устройства найдена
	if(::GetUnicastIpAddressEntry(&row) == NO_ERROR)
		// Выполняем снятие адреса устройства
		result = (::DeleteUnicastIpAddressEntry(&row) == NO_ERROR);
	// Если адрес того конца связи передан
	if(peer != nullptr){
		// Снимаемая запись пути к тому концу связи
		MIB_IPFORWARD_ROW2 route{};
		// Выполняем начальную подготовку записи пути
		::InitializeIpForwardEntry(&route);
		// Устанавливаем местный номер устройства
		route.InterfaceLuid = luid;
		/**
		 * Определяем семейство адреса того конца связи
		 */
		switch(peer->size){
			// Если тот конец связи адресуется IPv4
			case 4: {
				// Устанавливаем семейство адреса назначения
				route.DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
				// Устанавливаем сам адрес назначения
				route.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (peer)->address;
				// Устанавливаем длину префикса сети назначения
				route.DestinationPrefix.PrefixLength = 32;
			} break;
			// Если тот конец связи адресуется IPv6
			case 16: {
				// Устанавливаем семейство адреса назначения
				route.DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
				// Устанавливаем сам адрес назначения
				::memcpy(&route.DestinationPrefix.Prefix.Ipv6.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (peer)->address[0], sizeof(struct in6_addr));
				// Устанавливаем длину префикса сети назначения
				route.DestinationPrefix.PrefixLength = 128;
			} break;
			// Если семейство адреса определить не удалось
			default: return result;
		}
		// Устанавливаем семейство следующего узла пути
		route.NextHop.si_family = route.DestinationPrefix.Prefix.si_family;
		// Если запись пути найдена
		if(::GetIpForwardEntry2(&route) == NO_ERROR)
			// Выполняем снятие пути к тому концу связи
			::DeleteIpForwardEntry2(&route);
	}
	// Выводим результат выполнения снятия
	return result;
}
/**
 * @brief Метод установки адреса сетевого устройства связи точка-точка
 *
 * @param name   название сетевого устройства
 * @param ip     устанавливаемый адрес
 * @param peer   адрес того конца связи
 * @param prefix длина префикса сети
 * @return       результат выполнения установки
 *
 * @note Понятия адреса того конца у MS Windows нет: система не держит при адресе
 *       устройства второго адреса, как то заведено у систем POSIX полем ifa_dstaddr.
 *       Тот же самый смысл достигается иначе - к тому концу прокладывается путь
 *       через это устройство, и обмен с ним идёт ровно так же, как шёл бы при связи
 *       точка-точка
 *
 * @warning Установка требует надзорных прав. Проложенный путь переживает завершение
 *          процесса и снимается вместе с устройством
 *
 */
bool awh::eth::Interface::setAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix) const noexcept {
	// Если установить адрес устройства не удалось
	if(!this->setAddress(name, ip, prefix))
		// Выводим отрицательный результат установки
		return false;
	// Если адрес того конца связи не передан
	if(peer == nullptr)
		// Выводим положительный результат установки
		return true;
	// Местный номер сетевого устройства
	NET_LUID luid{};
	// Если сетевое устройство найти не удалось
	if(!::__awh_luid__(name, luid))
		// Выводим отрицательный результат установки
		return false;
	// Прокладываемый к тому концу связи путь
	MIB_IPFORWARD_ROW2 row{};
	// Выполняем начальную подготовку пути
	::InitializeIpForwardEntry(&row);
	// Устанавливаем местный номер устройства
	row.InterfaceLuid = luid;
	/**
	 * Путь прокладывается к одному узлу, а не к сети
	 *
	 * @note Длина префикса берётся наибольшей: тот конец связи есть один узел, и
	 *       перехватывать путём этим что-либо ещё было бы прямым вредом
	 *
	 */
	switch(peer->size){
		// Если тот конец связи задан адресом IPv4
		case 4: {
			// Устанавливаем семейство адреса
			row.DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
			// Устанавливаем сам адрес
			row.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (peer)->address;
			// Устанавливаем длину префикса пути
			row.DestinationPrefix.PrefixLength = 32;
		} break;
		// Если тот конец связи задан адресом IPv6
		case 16: {
			// Устанавливаем семейство адреса
			row.DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
			// Устанавливаем сам адрес
			::memcpy(&row.DestinationPrefix.Prefix.Ipv6.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (peer)->address[0], sizeof(struct in6_addr));
			// Устанавливаем длину префикса пути
			row.DestinationPrefix.PrefixLength = 128;
		} break;
		// Если семейство адреса определить не удалось
		default: {
			// Выводим в журнал сообщение о неподдерживаемом виде адреса
			this->_log->print("%s: only IPv4 and IPv6 peer addresses are supported", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
			// Выводим отрицательный результат установки
			return false;
		}
	}
	// Устанавливаем семейство того конца пути
	row.NextHop.si_family = row.DestinationPrefix.Prefix.si_family;
	// Устанавливаем стоимость пути
	row.Metric = 1;
	// Выполняем прокладку пути к тому концу связи
	const DWORD code = ::CreateIpForwardEntry2(&row);
	// Если путь проложен либо уже был проложен
	if((code == NO_ERROR) || (code == ERROR_OBJECT_ALREADY_EXISTS))
		// Выводим положительный результат установки
		return true;
	// Выводим в журнал сообщение о невозможности прокладки пути
	this->_log->print("%s: route to the peer could not be created, error %lu", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, code);
	// Выводим отрицательный результат установки
	return false;
}
/**
 * @brief Метод получения адреса сетевого устройства вместе с адресом того конца связи
 *
 * @param name   название сетевого устройства
 * @param ip     заполняемый адрес устройства
 * @param peer   заполняемый адрес того конца связи
 * @param prefix заполняемая длина префикса сети
 * @return       результат выполнения получения
 *
 * @note Семейство искомого адреса задаётся заранее подготовленным объектом ip:
 *       четыре байта означают IPv4, шестнадцать - IPv6. Так же устроены и эталонные
 *       бэкенды
 *
 * @warning Адрес того конца связи остаётся нетронутым всегда: система при адресе
 *          устройства второго адреса не держит вовсе, и взять его неоткуда. Тот же
 *          смысл несёт проложенный к нему путь, но путь этот адресом устройства не
 *          является и выдаваться за него не должен
 *
 */
bool awh::eth::Interface::getAddress(string_view name, unique_ptr <net::addr_t> & ip, [[maybe_unused]] unique_ptr <net::addr_t> & peer, uint8_t & prefix) const noexcept {
	// Если название устройства либо заполняемый адрес не переданы
	if(name.empty() || (ip == nullptr))
		// Выводим отрицательный результат получения
		return false;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer;
	// Выполняем опрос перечня сетевых устройств
	PIP_ADAPTER_ADDRESSES adapters = ::__awh_adapters__(buffer);
	// Если перечень устройств получить не удалось
	if(adapters == nullptr)
		// Выводим отрицательный результат получения
		return false;
	// Результат выполнения получения
	bool result = false;
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next){
		// Если название устройства с искомым не совпало
		if((adapter->AdapterName == nullptr) || !::__awh_iface_same__(name, adapter->AdapterName))
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
			// Если искомым является адрес IPv4 и адрес устройства ему отвечает
			if((ip->size == 4) && (address->Address.lpSockaddr->sa_family == AF_INET)){
				// Запоминаем адрес устройства
				awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = reinterpret_cast <struct sockaddr_in *> (address->Address.lpSockaddr)->sin_addr.s_addr;
				// Запоминаем длину префикса сети
				prefix = static_cast <uint8_t> (address->OnLinkPrefixLength);
				// Запоминаем длину префикса сети в самом адресе
				awh_cast <net::addr_net_ipv4_t *> (ip.get())->prefix = prefix;
				// Выводим положительный результат получения
				return true;
			}
			// Если искомым является адрес IPv6 и адрес устройства ему отвечает
			if((ip->size == 16) && (address->Address.lpSockaddr->sa_family == AF_INET6)){
				// Получаем адрес устройства вида IPv6
				struct sockaddr_in6 * value = reinterpret_cast <struct sockaddr_in6 *> (address->Address.lpSockaddr);
				// Определяем, является ли адрес адресом канальной связи
				const bool link = IN6_IS_ADDR_LINKLOCAL(&value->sin6_addr);
				// Если адрес ещё не найден либо найден глобальный взамен канального
				if(!result || !link){
					// Запоминаем адрес устройства
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &value->sin6_addr, sizeof(struct in6_addr));
					// Запоминаем длину префикса сети
					prefix = static_cast <uint8_t> (address->OnLinkPrefixLength);
					// Запоминаем длину префикса сети в самом адресе
					awh_cast <net::addr_net_ipv6_t *> (ip.get())->prefix = prefix;
					// Запоминаем номер устройства зоны адреса
					awh_cast <net::addr_net_ipv6_t *> (ip.get())->zone = static_cast <uint32_t> (value->sin6_scope_id);
					// Запоминаем положительный результат получения
					result = true;
					// Если найден глобальный адрес - поиск на нём и оканчивается
					if(!link)
						// Выводим положительный результат получения
						return result;
				}
			}
		}
		// Завершаем перебор устройств
		break;
	}
	// Выводим результат выполнения получения
	return result;
}
/**
 * @brief Метод установки признака сетевого устройства
 *
 * @param name название сетевого устройства
 * @param flag устанавливаемый признак
 * @param mode режим включения либо выключения признака
 * @return     результат выполнения установки
 *
 * @note Правится здесь один признак поднятия устройства: у MS Windows он и есть
 *       единственный, каким приложение вправе распоряжаться. Прочие признаки система
 *       выводит из устройства драйвера и настройками не считает - выключить приём
 *       широковещательных кадров либо отключить разрешение адресов в сегменте здесь
 *       нельзя вовсе
 *
 * @warning Установка требует надзорных прав и действует на всю машину: устройство
 *          останется поднятым либо опущенным и после завершения процесса
 *
 */
bool awh::eth::Interface::flag(string_view name, const event::eth_flag_t flag, const event::mode_t mode) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Выводим отрицательный результат установки
		return false;
	// Если правится признак, каким система распоряжаться не даёт
	if(flag != event::eth_flag_t::UP){
		// Выводим в журнал сообщение о неподдерживаемом признаке
		this->_log->print("%s: only the UP flag can be changed, MS Windows derives the rest from the driver", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Выводим отрицательный результат установки
		return false;
	}
	// Местный номер сетевого устройства
	NET_LUID luid{};
	// Если сетевое устройство найти не удалось
	if(!::__awh_luid__(name, luid)){
		// Выводим в журнал сообщение о ненайденном устройстве
		this->_log->print("%s: interface \"%s\" was not found", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(name).c_str());
		// Выводим отрицательный результат установки
		return false;
	}
	// Порядковый номер сетевого устройства
	NET_IFINDEX index = 0;
	// Если порядковый номер устройства по местному номеру получить не удалось
	if(::ConvertInterfaceLuidToIndex(&luid, &index) != NO_ERROR){
		// Выводим в журнал сообщение о ненайденном устройстве
		this->_log->print("%s: interface \"%s\" has no index", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(name).c_str());
		// Выводим отрицательный результат установки
		return false;
	}
	/**
	 * Настройки сетевого устройства
	 *
	 * @warning Здесь обязана стоять именно MIB_IFROW, а не MIB_IF_ROW2: записи состояния
	 *          отвечает единственная SetIfEntry, и принимает она устаревшую структуру.
	 *          Прежде тут заполнялась MIB_IF_ROW2 и приводилась указателем к PMIB_IFROW -
	 *          раскладка полей у них не совпадает вовсе (MIB_IF_ROW2 открывается местным
	 *          номером InterfaceLuid, MIB_IFROW - названием и порядковым номером dwIndex),
	 *          оттого система читала номер устройства из чужого места и отвечала кодом 2
	 *          «не найдено». Подъём устройства под MS Windows не работал НИКОГДА и ни для
	 *          какого устройства - выдал себя дефект только проверкой переноса пакета
	 *          через туннель 20.08.2026. Замены в виде SetIfEntry2 в заголовках MinGW нет
	 */
	MIB_IFROW row{};
	// Устанавливаем порядковый номер устройства
	row.dwIndex = static_cast <DWORD> (index);
	// Если снять нынешние настройки устройства не удалось
	if(::GetIfEntry(&row) != NO_ERROR){
		// Выводим в журнал сообщение о невозможности опроса устройства
		this->_log->print("%s: interface state could not be read", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__);
		// Выводим отрицательный результат установки
		return false;
	}
	// Устанавливаем состояние устройства
	row.dwAdminStatus = (mode == event::mode_t::ENABLED ? MIB_IF_ADMIN_STATUS_UP : MIB_IF_ADMIN_STATUS_DOWN);
	// Выполняем запись настроек устройства
	const DWORD code = ::SetIfEntry(&row);
	// Если настройки устройства записаны
	if(code == NO_ERROR)
		// Выводим положительный результат установки
		return true;
	// Выводим в журнал сообщение о невозможности записи настроек
	this->_log->print("%s: interface state could not be changed, error %lu", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, code);
	// Выводим отрицательный результат установки
	return false;
}
/**
 * @brief Метод полной настройки сетевого устройства
 *
 * @param name   название сетевого устройства
 * @param ip     устанавливаемый адрес
 * @param prefix длина префикса сети
 * @param mtu    размер кадра устройства, ноль означает «не трогать»
 * @return       результат выполнения настройки
 *
 * @note Устройство поднимать здесь не приходится: у MS Windows оно поднимается само,
 *       едва драйвер сообщил системе о подключении. Попытка поднять уже поднятое
 *       отвечает отказом, и потому её здесь нет вовсе
 *
 */
bool awh::eth::Interface::configure(string_view name, const net::addr_t * ip, const uint8_t prefix, const uint32_t mtu) const noexcept {
	// Если установить адрес устройства не удалось
	if(!this->setAddress(name, ip, prefix))
		// Выводим отрицательный результат настройки
		return false;
	// Если размер кадра задан и установить его не удалось
	if((mtu > 0) && !this->mtu(name, mtu))
		// Выводим отрицательный результат настройки
		return false;
	// Выводим положительный результат настройки
	return true;
}
/**
 * @brief Метод полной настройки сетевого устройства связи точка-точка
 *
 * @param name   название сетевого устройства
 * @param ip     устанавливаемый адрес
 * @param peer   адрес того конца связи
 * @param prefix длина префикса сети
 * @param mtu    размер кадра устройства, ноль означает «не трогать»
 * @return       результат выполнения настройки
 *
 */
bool awh::eth::Interface::configure(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix, const uint32_t mtu) const noexcept {
	// Если установить адрес устройства не удалось
	if(!this->setAddress(name, ip, peer, prefix))
		// Выводим отрицательный результат настройки
		return false;
	// Если размер кадра задан и установить его не удалось
	if((mtu > 0) && !this->mtu(name, mtu))
		// Выводим отрицательный результат настройки
		return false;
	// Выводим положительный результат настройки
	return true;
}
/**
 * @brief Метод проверки того, что устройство с заданным адресом является туннелем
 *
 * @param addr адрес искомого сетевого устройства
 * @return     признак того, что устройство является туннелем
 *
 * @note Устройство отыскивается по его адресу, а дальше проверка ведётся та же, что и
 *       по названию, - со всеми её оговорками
 *
 */
bool awh::eth::Interface::isTunnel(const net::addr_t * addr) const noexcept {
	// Если адрес сетевого устройства не передан
	if(addr == nullptr)
		// Выводим признак того, что устройство туннелем не является
		return false;
	// Выполняем поиск названия устройства по его адресу
	const string & name = this->name(addr);
	// Выводим признак того, чем найденное устройство является
	return (!name.empty() ? this->isTunnel(name) : false);
}
/**
 * @brief Метод проверки того, что устройство с заданным адресом является виртуальным
 *
 * @param addr адрес искомого сетевого устройства
 * @return     признак того, что устройство является виртуальным
 *
 */
bool awh::eth::Interface::isVirtual(const net::addr_t * addr) const noexcept {
	// Если адрес сетевого устройства не передан
	if(addr == nullptr)
		// Выводим признак того, что устройство виртуальным не является
		return false;
	// Выполняем поиск названия устройства по его адресу
	const string & name = this->name(addr);
	// Выводим признак того, чем найденное устройство является
	return (!name.empty() ? this->isVirtual(name) : false);
}
