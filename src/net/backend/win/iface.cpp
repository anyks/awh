/**
 * @file: iface.cpp
 * @date: 2026-08-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
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
 * @copyright: Copyright © 2026
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
		if((adapter->AdapterName != nullptr) && (name.compare(adapter->AdapterName) == 0))
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
		if((adapter->AdapterName != nullptr) && (name.compare(adapter->AdapterName) == 0))
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
		if((adapter->AdapterName == nullptr) || (name.compare(adapter->AdapterName) != 0))
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
 */
bool awh::eth::Interface::isTunnel(string_view name) const noexcept {
	// Если название сетевого устройства не передано
	if(name.empty())
		// Возвращаем признак того, что устройство туннелем не является
		return false;
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
		if((adapter->AdapterName != nullptr) && (name.compare(adapter->AdapterName) == 0))
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
		if((adapter->AdapterName == nullptr) || (name.compare(adapter->AdapterName) != 0))
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
		if((adapter->AdapterName == nullptr) || (name.compare(adapter->AdapterName) != 0))
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
		if((adapter->AdapterName == nullptr) || (name.compare(adapter->AdapterName) != 0))
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
