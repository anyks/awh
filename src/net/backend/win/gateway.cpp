/**
 * @file gateway.cpp
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
 * @brief Реализация бэкенда работы с путями сети для MS Windows — опрос пути к
 *        заданному адресу, прокладка пути и его снятие
 *
 * @details Слой этот отвечает эталонным backend/bsd/gateway.cpp и
 *          backend/gnu/gateway.cpp. У BSD таблица путей читается вызовом sysctl с
 *          ветвью PF_ROUTE, у Linux - перепиской с ядром через netlink. MS Windows
 *          отдаёт её средствами netioapi, и опрос пути к заданному адресу здесь
 *          устроен проще прочих: система сама отвечает, каким путём отправит пакет,
 *          вызовом GetBestRoute2 - искать по таблице не приходится вовсе
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
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
 * @brief Средства опроса и правки таблицы путей сети
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
static constexpr const char * __AWH_GATEWAY_BACKEND__ = "MS Windows gateway backend";

/**
 * @brief Инкапсулируем состояние слоя в пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция переноса адреса набора в запись системы
	 *
	 * @param addr переносимый адрес набора
	 * @param out  запись системы, в которую переносится адрес
	 * @return     признак успешного переноса
	 *
	 */
	bool __awh_to_sockaddr__(const awh::net::addr_t * addr, SOCKADDR_INET & out) noexcept {
		// Если адрес не передан
		if(addr == nullptr)
			// Выводим признак неуспешного переноса
			return false;
		/**
		 * Определяем вид переносимого адреса
		 */
		switch(addr->size){
			// Если переносится адрес IPv4
			case 4: {
				// Устанавливаем семейство адреса
				out.Ipv4.sin_family = AF_INET;
				// Устанавливаем сам адрес
				out.Ipv4.sin_addr.s_addr = awh_cast <const awh::net::addr_net_ipv4_t *> (addr)->address;
				// Выводим признак успешного переноса
				return true;
			}
			// Если переносится адрес IPv6
			case 16: {
				// Устанавливаем семейство адреса
				out.Ipv6.sin6_family = AF_INET6;
				// Устанавливаем сам адрес
				::memcpy(&out.Ipv6.sin6_addr, &awh_cast <const awh::net::addr_net_ipv6_t *> (addr)->address[0], sizeof(struct in6_addr));
				// Устанавливаем номер устройства зоны адреса
				out.Ipv6.sin6_scope_id = static_cast <ULONG> (awh_cast <const awh::net::addr_net_ipv6_t *> (addr)->zone);
				// Выводим признак успешного переноса
				return true;
			}
		}
		// Выводим признак неуспешного переноса
		return false;
	}

	/**
	 * @brief Функция переноса записи системы в адрес набора
	 *
	 * @param in запись системы, из которой переносится адрес
	 * @return   перенесённый адрес набора
	 *
	 */
	unique_ptr <awh::net::addr_t> __awh_from_sockaddr__(const SOCKADDR_INET & in) noexcept {
		/**
		 * Определяем семейство переносимого адреса
		 */
		switch(in.si_family){
			// Если переносится адрес IPv4
			case AF_INET: {
				// Заводим переносимый адрес
				unique_ptr <awh::net::addr_t> result = make_unique <awh::net::addr_net_ipv4_t> ();
				// Устанавливаем сам адрес
				awh_cast <awh::net::addr_net_ipv4_t *> (result.get())->address = in.Ipv4.sin_addr.s_addr;
				// Выводим перенесённый адрес
				return result;
			}
			// Если переносится адрес IPv6
			case AF_INET6: {
				// Заводим переносимый адрес
				unique_ptr <awh::net::addr_t> result = make_unique <awh::net::addr_net_ipv6_t> ();
				// Устанавливаем сам адрес
				::memcpy(&awh_cast <awh::net::addr_net_ipv6_t *> (result.get())->address[0], &in.Ipv6.sin6_addr, sizeof(struct in6_addr));
				// Устанавливаем номер устройства зоны адреса
				awh_cast <awh::net::addr_net_ipv6_t *> (result.get())->zone = static_cast <uint32_t> (in.Ipv6.sin6_scope_id);
				// Выводим перенесённый адрес
				return result;
			}
		}
		// Выводим пустой адрес
		return nullptr;
	}

	/**
	 * @brief Функция получения названия устройства по его местному номеру
	 *
	 * @details Названием устройства слой сетевых устройств считает неизменное имя вида
	 *          «{GUID}», и путь обязан отдавать устройство в том же виде - иначе
	 *          названием этим нельзя было бы воспользоваться нигде более
	 *
	 * @param luid местный номер устройства
	 * @return     название устройства в виде «{GUID}»
	 *
	 */
	string __awh_ifname__(const NET_LUID & luid) noexcept {
		// Уникальный номер устройства
		GUID guid{};
		// Если получить уникальный номер устройства не удалось
		if(::ConvertInterfaceLuidToGuid(&luid, &guid) != NO_ERROR)
			// Выводим пустое название устройства
			return string{};
		// Буфер под название устройства
		char buffer[64];
		// Выполняем сборку названия устройства
		const int32_t size = ::snprintf(
			buffer, sizeof(buffer),
			"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			static_cast <unsigned long> (guid.Data1), guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
		);
		// Выводим собранное название устройства
		return (size > 0 ? string(buffer, static_cast <size_t> (size)) : string{});
	}

	/**
	 * @brief Функция получения местного номера устройства по его названию
	 *
	 * @param name название искомого устройства
	 * @param luid местный номер найденного устройства
	 * @return     признак того, что устройство найдено
	 *
	 */
	bool __awh_ifluid__(const string & name, NET_LUID & luid) noexcept {
		// Если название устройства не передано
		if(name.empty())
			// Выводим признак того, что устройство не найдено
			return false;
		// Буфер под название устройства в широкой кодировке
		wchar_t buffer[64];
		// Выполняем перевод названия устройства в широкую кодировку
		if(::MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, buffer, static_cast <int32_t> (sizeof(buffer) / sizeof(buffer[0]))) <= 0)
			// Выводим признак того, что устройство не найдено
			return false;
		// Уникальный номер устройства
		GUID guid{};
		// Если разобрать название устройства не удалось
		if(::CLSIDFromString(buffer, &guid) != NOERROR)
			// Выводим признак того, что устройство не найдено
			return false;
		// Выводим признак успешного перевода уникального номера в местный
		return (::ConvertInterfaceGuidToLuid(&guid, &luid) == NO_ERROR);
	}
};

/**
 * @brief Метод получения пути к заданному адресу
 *
 * @param route описание пути, куда заносится ответ системы
 * @return      результат выполнения получения
 *
 * @details Адрес назначения задаёт спрашивающий, а устройство, шлюз и длину префикса
 *          проставляет система
 *
 * @note Искать по таблице путей здесь не приходится: MS Windows отвечает на прямой
 *       вопрос «каким путём уйдёт пакет к этому адресу» вызовом GetBestRoute2. Ответ
 *       этот вернее перебора таблицы - он учитывает и стоимость пути, и состояние
 *       устройств, и порядок разрешения одинаковых по длине префиксов
 *
 */
bool awh::eth::Gateway::get(route_t & route) const noexcept {
	// Если адрес назначения не передан
	if(route.destination == nullptr){
		// Выводим в журнал сообщение о непереданном адресе назначения
		this->_log->print("%s: destination address is not initialized", log_t::flag_t::CRITICAL, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат получения
		return false;
	}
	// Адрес назначения в виде записи системы
	SOCKADDR_INET destination{};
	// Если перенести адрес назначения не удалось
	if(!::__awh_to_sockaddr__(route.destination.get(), destination)){
		// Выводим в журнал сообщение о неподдерживаемом виде адреса
		this->_log->print("%s: only IPv4 and IPv6 destinations are supported", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат получения
		return false;
	}
	// Найденный путь к адресу назначения
	MIB_IPFORWARD_ROW2 row{};
	// Адрес устройства, каким путь начинается
	SOCKADDR_INET source{};
	// Выполняем опрос пути к адресу назначения
	const DWORD code = ::GetBestRoute2(nullptr, 0, nullptr, &destination, 0, &row, &source);
	// Если путь к адресу назначения найти не удалось
	if(code != NO_ERROR){
		// Выводим в журнал сообщение о ненайденном пути
		this->_log->print("%s: route could not be resolved, error %lu", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, code);
		// Выводим отрицательный результат получения
		return false;
	}
	// Запоминаем название устройства, каким путь начинается
	route.ifname = ::__awh_ifname__(row.InterfaceLuid);
	// Запоминаем длину префикса пути
	route.prefix = static_cast <uint8_t> (row.DestinationPrefix.PrefixLength);
	// Запоминаем шлюз пути
	route.gateway = ::__awh_from_sockaddr__(row.NextHop);
	// Выводим положительный результат получения
	return true;
}
/**
 * @brief Метод прокладки пути сети
 *
 * @param route описание прокладываемого пути
 * @return      результат выполнения прокладки
 *
 * @warning Прокладка требует надзорных прав и действует на всю машину: путь переживает
 *          завершение процесса, и убирать его следует за собой
 *
 */
bool awh::eth::Gateway::add(const route_t & route) const noexcept {
	// Если адрес назначения не передан
	if(route.destination == nullptr){
		// Выводим в журнал сообщение о непереданном адресе назначения
		this->_log->print("%s: destination address is not initialized", log_t::flag_t::CRITICAL, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат прокладки
		return false;
	}
	// Прокладываемый путь
	MIB_IPFORWARD_ROW2 row{};
	// Выполняем начальную подготовку пути
	::InitializeIpForwardEntry(&row);
	// Если перенести адрес назначения не удалось
	if(!::__awh_to_sockaddr__(route.destination.get(), row.DestinationPrefix.Prefix)){
		// Выводим в журнал сообщение о неподдерживаемом виде адреса
		this->_log->print("%s: only IPv4 and IPv6 destinations are supported", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат прокладки
		return false;
	}
	// Устанавливаем длину префикса пути
	row.DestinationPrefix.PrefixLength = route.prefix;
	// Если шлюз пути передан и перенести его не удалось
	if((route.gateway != nullptr) && !::__awh_to_sockaddr__(route.gateway.get(), row.NextHop)){
		// Выводим в журнал сообщение о неподдерживаемом виде адреса
		this->_log->print("%s: only IPv4 and IPv6 gateways are supported", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат прокладки
		return false;
	}
	// Если шлюз пути не передан
	if(route.gateway == nullptr)
		// Устанавливаем семейство того конца пути по адресу назначения
		row.NextHop.si_family = row.DestinationPrefix.Prefix.si_family;
	// Если устройство пути найти не удалось
	if(!::__awh_ifluid__(route.ifname, row.InterfaceLuid)){
		// Выводим в журнал сообщение о ненайденном устройстве
		this->_log->print("%s: interface \"%s\" was not found", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, route.ifname.c_str());
		// Выводим отрицательный результат прокладки
		return false;
	}
	// Устанавливаем стоимость пути
	row.Metric = 1;
	// Выполняем прокладку пути
	const DWORD code = ::CreateIpForwardEntry2(&row);
	// Если путь проложен либо уже был проложен
	if((code == NO_ERROR) || (code == ERROR_OBJECT_ALREADY_EXISTS))
		// Выводим положительный результат прокладки
		return true;
	// Выводим в журнал сообщение о невозможности прокладки пути
	this->_log->print("%s: route could not be created, error %lu", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, code);
	// Выводим отрицательный результат прокладки
	return false;
}
/**
 * @brief Метод снятия пути сети
 *
 * @param route описание снимаемого пути
 * @return      результат выполнения снятия
 *
 * @note Путь отыскивается по адресу назначения, длине префикса и устройству, оттого
 *       описание должно отвечать тому, каким путь прокладывался
 *
 */
bool awh::eth::Gateway::remove(const route_t & route) const noexcept {
	// Если адрес назначения не передан
	if(route.destination == nullptr){
		// Выводим в журнал сообщение о непереданном адресе назначения
		this->_log->print("%s: destination address is not initialized", log_t::flag_t::CRITICAL, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат снятия
		return false;
	}
	// Снимаемый путь
	MIB_IPFORWARD_ROW2 row{};
	// Выполняем начальную подготовку пути
	::InitializeIpForwardEntry(&row);
	// Если перенести адрес назначения не удалось
	if(!::__awh_to_sockaddr__(route.destination.get(), row.DestinationPrefix.Prefix)){
		// Выводим в журнал сообщение о неподдерживаемом виде адреса
		this->_log->print("%s: only IPv4 and IPv6 destinations are supported", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат снятия
		return false;
	}
	// Устанавливаем длину префикса пути
	row.DestinationPrefix.PrefixLength = route.prefix;
	// Если шлюз пути передан
	if(route.gateway != nullptr)
		// Выполняем перенос шлюза пути
		::__awh_to_sockaddr__(route.gateway.get(), row.NextHop);
	// Если шлюз пути не передан
	else row.NextHop.si_family = row.DestinationPrefix.Prefix.si_family;
	// Если устройство пути найти не удалось
	if(!::__awh_ifluid__(route.ifname, row.InterfaceLuid)){
		// Выводим в журнал сообщение о ненайденном устройстве
		this->_log->print("%s: interface \"%s\" was not found", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, route.ifname.c_str());
		// Выводим отрицательный результат снятия
		return false;
	}
	// Выполняем снятие пути
	const DWORD code = ::DeleteIpForwardEntry2(&row);
	// Если путь снят
	if(code == NO_ERROR)
		// Выводим положительный результат снятия
		return true;
	// Выводим в журнал сообщение о невозможности снятия пути
	this->_log->print("%s: route could not be removed, error %lu", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, code);
	// Выводим отрицательный результат снятия
	return false;
}
