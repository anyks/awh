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
	/**
	 * @brief Объявление функции получения опознавателя устройства по его названию
	 *
	 * @note Объявление нужно наперёд: поиск по таблице стоит выше её тела, а разносить
	 *       посредники по файлу ради порядка обращений незачем
	 *
	 * @param name название сетевого устройства
	 * @param luid опознаватель устройства
	 * @return     результат получения опознавателя
	 *
	 */
	bool __awh_ifluid__(const string & name, NET_LUID & luid) noexcept;
	/**
	 * @brief Функция поиска маршрута в таблице системы
	 *
	 * @details Отвечает на вопрос «есть ли ТАКОЙ маршрут», а не «каким путём пойдёт
	 *          пакет»: второе выдаёт `GetBestRoute2`, и после сноса заданного маршрута
	 *          он отвечает маршрутом по умолчанию, отчего проверить снос им нельзя
	 *
	 *          Условия совпадения взяты у эталонного бэкенда BSD: сличается лишь то,
	 *          что названо, - шлюз при ненулевом значении, назначение при ненулевом,
	 *          устройство при непустом названии. Длина префикса не сличается: ищущий
	 *          её обычно не знает, а у эталона её тоже нет в условиях
	 *
	 * @param route объект маршрута: и условия поиска, и место для ответа
	 * @param log   объект работы с логами
	 * @return      результат поиска маршрута
	 *
	 */
	bool __awh_lookup__(awh::eth::gateway_t::route_t & route, const awh::log_t * log) noexcept {
		// Семейство адресов, в каком идёт поиск
		const ADDRESS_FAMILY family = (route.destination->size == 4 ? AF_INET : AF_INET6);
		// Искомый адрес назначения в виде записи системы
		SOCKADDR_INET destination{};
		// Если перевести адрес назначения не удалось
		if(!::__awh_to_sockaddr__(route.destination.get(), destination)){
			// Выводим в журнал сообщение о неподдерживаемом виде адреса
			log->print("%s: only IPv4 and IPv6 destinations are supported", awh::log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
			// Выводим отрицательный результат поиска
			return false;
		}
		// Искомый адрес шлюза в виде записи системы
		SOCKADDR_INET gateway{};
		// Признак того, что шлюз участвует в условиях поиска
		bool byGateway = false;
		// Если шлюз передан, переводим его в запись системы
		if(route.gateway != nullptr){
			// Отмечаем участие шлюза в условиях по успеху перевода
			byGateway = ::__awh_to_sockaddr__(route.gateway.get(), gateway);
			/**
			 * Пара «назначение и шлюз» обязана быть одного вида
			 *
			 * @warning Оба довода объявлены общим основанием, и запретить их несличимую
			 *          пару языком нечем: вид несёт поле «size», по нему и надо сличать.
			 *          Разбор ниже ведётся по виду НАЗНАЧЕНИЯ, а шлюз переведён по СВОЕМУ
			 *          виду, и при расхождении сличение шло бы по чужой ветви записи
			 *          объединения - ответ выходил бы мусорным, притом молча. Заодно
			 *          мимо проходила бы и проверка нулевого шлюза: она стоит под тем же
			 *          условием семейства
			 *
			 * @note У наречий POSIX то же место отвечало не мусором, а чтением за границей
			 *       выделенной памяти: там адрес лежит по указателю, а не в объединении
			 */
			if(byGateway && (route.gateway->size != route.destination->size)){
				// Выводим в журнал сообщение о несличимой паре адресов
				log->print("%s: destination and gateway belong to different address families", awh::log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
				// Выводим отрицательный результат поиска
				return false;
			}
		}
		/**
		 * Если шлюз назван нулевым адресом, из условий он выбывает
		 *
		 * @warning Снятие это велось у одного лишь IPv4, а у IPv6 нулевой шлюз оставался
		 *          условием поиска. Следствие было не «лишним условием», а подменой
		 *          вопроса: непосредственный путь MS Windows держит в таблице с пустым
		 *          тем концом (`NextHop` равен `::`), и поиск по нулевому шлюзу находил
		 *          ОДНИ ЛИШЬ прямые пути вместо всякого пути к названному назначению.
		 *          Просьба же «найди путь, шлюз мне безразличен» выражается у
		 *          вызывающей стороны именно нулевым адресом - обращение `get` выше
		 *          разбирает его тем же порядком, обходя все шестнадцать октетов
		 *
		 * @note Эталонный слой BSD снимает условие у обоих семейств
		 *       (`bsd/gateway.cpp:1993` для IPv6, `1700` для IPv4)
		 *
		 */
		if(byGateway)
			// Снимаем условие при нулевом адресе шлюза
			byGateway = ((family == AF_INET)
				? (gateway.Ipv4.sin_addr.s_addr != 0)
				: (::memcmp(&gateway.Ipv6.sin6_addr, &::in6addr_any, sizeof(struct in6_addr)) != 0));
		// Опознаватель устройства, каким задан маршрут
		NET_LUID luid{};
		// Признак того, что устройство участвует в условиях поиска
		bool byInterface = false;
		// Если название устройства передано
		if(!route.ifname.empty()){
			// Если опознаватель устройства получить не удалось
			if(!::__awh_ifluid__(route.ifname, luid)){
				// Выводим в журнал сообщение о ненайденном устройстве
				log->print("%s: interface \"%s\" was not found", awh::log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, route.ifname.c_str());
				// Выводим отрицательный результат поиска
				return false;
			}
			// Отмечаем участие устройства в условиях поиска
			byInterface = true;
		}
		// Признак того, что назначение участвует в условиях поиска
		const bool byDestination = ((family == AF_INET)
			? (destination.Ipv4.sin_addr.s_addr != 0)
			: (::memcmp(&destination.Ipv6.sin6_addr, &::in6addr_any, sizeof(struct in6_addr)) != 0));
		// Таблица маршрутов системы
		PMIB_IPFORWARD_TABLE2 table = nullptr;
		// Если снять таблицу маршрутов не удалось
		if(::GetIpForwardTable2(family, &table) != NO_ERROR){
			// Выводим в журнал сообщение о невозможности снять таблицу
			log->print("%s: route table could not be read", awh::log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
			// Выводим отрицательный результат поиска
			return false;
		}
		// Признак того, что маршрут найден
		bool result = false;
		/**
		 * Обходим все записи таблицы маршрутов
		 */
		for(ULONG i = 0; (i < table->NumEntries) && !result; i++){
			// Получаем очередную запись таблицы
			const MIB_IPFORWARD_ROW2 & row = table->Table[i];
			// Если семейство записи не совпадает с искомым, запись пропускается
			if(row.DestinationPrefix.Prefix.si_family != family)
				// Переходим к следующей записи
				continue;
			// Если назначение названо и не совпало, запись пропускается
			if(byDestination){
				// Если адреса назначения различаются
				if((family == AF_INET)
					? (row.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr != destination.Ipv4.sin_addr.s_addr)
					: (::memcmp(&row.DestinationPrefix.Prefix.Ipv6.sin6_addr, &destination.Ipv6.sin6_addr, sizeof(struct in6_addr)) != 0))
					// Переходим к следующей записи
					continue;
			}
			// Если шлюз назван и не совпал, запись пропускается
			if(byGateway){
				// Если адреса шлюза различаются
				if((family == AF_INET)
					? (row.NextHop.Ipv4.sin_addr.s_addr != gateway.Ipv4.sin_addr.s_addr)
					: (::memcmp(&row.NextHop.Ipv6.sin6_addr, &gateway.Ipv6.sin6_addr, sizeof(struct in6_addr)) != 0))
					// Переходим к следующей записи
					continue;
			}
			// Если устройство названо и не совпало, запись пропускается
			if(byInterface && (row.InterfaceLuid.Value != luid.Value))
				// Переходим к следующей записи
				continue;
			// Заполняем название устройства найденного маршрута
			route.ifname = ::__awh_ifname__(row.InterfaceLuid);
			// Заполняем длину префикса найденного маршрута
			route.prefix = static_cast <uint8_t> (row.DestinationPrefix.PrefixLength);
			// Заполняем адрес шлюза найденного маршрута
			route.gateway = ::__awh_from_sockaddr__(row.NextHop);
			// Отмечаем маршрут найденным
			result = true;
		}
		// Освобождаем таблицу маршрутов
		::FreeMibTable(table);
		// Выводим результат поиска маршрута
		return result;
	}
	/**
	 * @brief Функция получения опознавателя устройства по его названию
	 *
	 * @param name название сетевого устройства
	 * @param luid опознаватель устройства
	 * @return     результат получения опознавателя
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
	/**
	 * Сличаем вид шлюза с видом адреса назначения
	 *
	 * @details Условие взято у эталонного наречия BSD дословно (bsd/gateway.cpp:404):
	 *          оба поля объявлены общим основанием `net::addr_t`, заполняются
	 *          потребителем порознь, и запретить их несличимую пару языком нечем
	 *
	 * @warning Заслона этого здесь не было вовсе, и метод отвечал УСПЕХОМ на пару
	 *          «шлюз IPv6, назначение IPv4». У систем POSIX та же пара даёт выход за
	 *          границу и ловится надзирателем, а у MS Windows `SOCKADDR_INET` есть
	 *          объединение - места хватает всегда, и порча оборачивается не отказом,
	 *          а мусорным ответом, принимаемым потребителем за истину. Ловится
	 *          проверкой GatewayMismatchedKindTest
	 */
	if((route.gateway != nullptr) && (route.destination != nullptr) &&
	   (route.gateway->size != route.destination->size)){
		// Выводим в журнал сообщение о несличимой паре адресов
		this->_log->print("%s: destination and gateway belong to different address families", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Работать с маршрутом по несличимой паре нечем
		return false;
	}
	/**
	 * Непереданный адрес назначения заводится сам
	 *
	 * @details Договор движков таков: не назван ни адрес назначения, ни шлюз - ищется
	 *          маршрут по умолчанию, и объект назначения заводится под семейство
	 *          названного шлюза. Движки POSIX так и поступают, отказом на это не
	 *          отвечая
	 *
	 * @warning Прежде здесь стоял отказ, и он расходился с прочими движками: проверка
	 *          GatewayGetDefaultIPv4NullDestination падала на всякой машине Windows.
	 *          Семейство берётся у шлюза, потому что иного указателя тут нет вовсе
	 */
	if((route.destination == nullptr) && (route.gateway != nullptr)){
		/**
		 * Определяем семейство названного шлюза
		 */
		switch(route.gateway->size){
			// Для адреса IPv4
			case 4:
				// Заводим объект адреса назначения семейства IPv4
				route.destination = make_unique <net::addr_net_ipv4_t> ();
			break;
			// Для адреса IPv6
			case 16:
				// Заводим объект адреса назначения семейства IPv6
				route.destination = make_unique <net::addr_net_ipv6_t> ();
			break;
		}
	}
	// Если адрес назначения не передан и завести его было не из чего
	if(route.destination == nullptr){
		// Выводим в журнал сообщение о непереданном адресе назначения
		this->_log->print("%s: destination address is not initialized", log_t::flag_t::CRITICAL, ::__AWH_GATEWAY_BACKEND__);
		// Выводим отрицательный результат получения
		return false;
	}
	/**
	 * Разделяем два вопроса, какие метод обязан различать
	 *
	 * @details Назван лишь пустой адрес - спрашивается маршрут ПО УМОЛЧАНИЮ, и на него
	 *          отвечает сама система. Названы адрес, шлюз либо устройство - спрашивается
	 *          НАЛИЧИЕ такого маршрута в таблице, и отвечать на него `GetBestRoute2`
	 *          нельзя: он выдаёт путь, каким машина пошла бы к адресу, и после сноса
	 *          заданного маршрута честно отвечает маршрутом по умолчанию
	 *
	 * @warning Различия этого прежде не было, и метод отвечал успехом ВСЕГДА, пока в
	 *          системе есть маршрут по умолчанию. Проверить снос им было нельзя вовсе:
	 *          GatewayRemoveByInterfaceIPv4 находила снесённый маршрут. Условия
	 *          совпадения взяты у эталонного бэкенда BSD дословно - шлюз, назначение и
	 *          устройство сличаются лишь тогда, когда названы; длина префикса не
	 *          сличается, потому что спрашивающий её обычно не знает
	 */
	{
		// Признак того, что назначение не названо
		bool emptyDestination = true;
		/**
		 * Определяем, названо ли назначение
		 */
		switch(route.destination->size){
			// Для адреса IPv4
			case 4:
				// Назначение считается неназванным при нулевом адресе
				emptyDestination = (awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address == 0);
			break;
			// Для адреса IPv6
			case 16: {
				// Обходим все октеты адреса назначения
				for(uint8_t i = 0; i < 16; i++){
					// Если очередной октет ненулевой
					if(awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[i] != 0){
						// Отмечаем назначение названным
						emptyDestination = false;
						// Прекращаем обход
						break;
					}
				}
			} break;
		}
		// Признак того, что шлюз не назван
		bool emptyGateway = true;
		// Если шлюз передан
		if(route.gateway != nullptr){
			/**
			 * Определяем, названо ли значение шлюза
			 */
			switch(route.gateway->size){
				// Для адреса IPv4
				case 4:
					// Шлюз считается неназванным при нулевом адресе
					emptyGateway = (awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address == 0);
				break;
				// Для адреса IPv6
				case 16: {
					// Обходим все октеты адреса шлюза
					for(uint8_t i = 0; i < 16; i++){
						// Если очередной октет ненулевой
						if(awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[i] != 0){
							// Отмечаем шлюз названным
							emptyGateway = false;
							// Прекращаем обход
							break;
						}
					}
				} break;
			}
		}
		// Если спрошено наличие ЗАДАННОГО маршрута, отвечаем поиском по таблице
		if(!emptyDestination || !emptyGateway || !route.ifname.empty())
			// Выводим результат поиска маршрута в таблице системы
			return ::__awh_lookup__(route, this->_log);
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
	/**
	 * Пара адресов обязана быть сличимой
	 *
	 * @warning Проверка эта стояла у `get`, а у `add` и `remove` её не было, тогда как
	 *          эталонные наречия POSIX ставят её во ВСЕХ ТРЁХ обращениях
	 *          (`bsd/gateway.cpp` 406, 1004, 1542). Пара из назначения IPv4 и шлюза
	 *          IPv6 переносилась при этом без единого возражения: оба конца пути
	 *          собирались порознь и каждый ложился в объединение верно, а несовпадение
	 *          семейств вскрывалось лишь ядром - отказом, по какому не понять, что
	 *          именно не так
	 *
	 */
	if((route.gateway != nullptr) && (route.destination != nullptr) &&
	   (route.gateway->size != route.destination->size)){
		// Выводим в журнал сообщение о несличимой паре адресов
		this->_log->print("%s: destination and gateway belong to different address families", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Работать с маршрутом по несличимой паре нечем
		return false;
	}

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
	/**
	 * Длина префикса обязана отвечать виду адреса
	 *
	 * @warning Длина эта уходила системе НЕПРОВЕРЕННОЙ, а хранится она в `uint8_t` и
	 *          принимает до 255 при пределе 32 у IPv4 и 128 у IPv6. Система такую
	 *          запись отвергает, но отвечает на неё общим `ERROR_INVALID_PARAMETER`, и
	 *          по журналу выходило, будто не так с самим путём, а не с его длиной.
	 *          Сличать длину при поиске нельзя (ищущий её не знает), а при прокладке -
	 *          обязательно: тут она задаётся, а не отыскивается
	 *
	 * @note Отвечаем отказом, а не приведением к пределу: приведение обратило бы
	 *       бессмысленную просьбу в осмысленную, но ДРУГУЮ - путь к одному узлу вместо
	 *       заказанной сети, - и сделало бы это молча
	 *
	 */
	if(route.prefix > ((route.destination->size == 4) ? 32 : 128)){
		// Выводим в журнал сообщение о недопустимой длине префикса
		this->_log->print("%s: prefix length %u is out of range for the %s destination", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, static_cast <uint32_t> (route.prefix), ((route.destination->size == 4) ? "IPv4" : "IPv6"));
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
	/**
	 * Пара адресов обязана быть сличимой
	 *
	 * @warning Проверка эта стояла у `get`, а у `add` и `remove` её не было, тогда как
	 *          эталонные наречия POSIX ставят её во ВСЕХ ТРЁХ обращениях
	 *          (`bsd/gateway.cpp` 406, 1004, 1542). Пара из назначения IPv4 и шлюза
	 *          IPv6 переносилась при этом без единого возражения: оба конца пути
	 *          собирались порознь и каждый ложился в объединение верно, а несовпадение
	 *          семейств вскрывалось лишь ядром - отказом, по какому не понять, что
	 *          именно не так
	 *
	 */
	if((route.gateway != nullptr) && (route.destination != nullptr) &&
	   (route.gateway->size != route.destination->size)){
		// Выводим в журнал сообщение о несличимой паре адресов
		this->_log->print("%s: destination and gateway belong to different address families", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
		// Работать с маршрутом по несличимой паре нечем
		return false;
	}

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
	/**
	 * Длина префикса обязана отвечать виду адреса
	 *
	 * @warning Длина эта уходила системе НЕПРОВЕРЕННОЙ, а хранится она в `uint8_t` и
	 *          принимает до 255 при пределе 32 у IPv4 и 128 у IPv6. Система такую
	 *          запись отвергает, но отвечает на неё общим `ERROR_INVALID_PARAMETER`, и
	 *          по журналу выходило, будто не так с самим путём, а не с его длиной.
	 *          Сличать длину при поиске нельзя (ищущий её не знает), а при снятии -
	 *          обязательно: снятие сличает записи целиком, и длина входит в сличение
	 *
	 * @note Отвечаем отказом, а не приведением к пределу: приведение обратило бы
	 *       бессмысленную просьбу в осмысленную, но ДРУГУЮ - путь к одному узлу вместо
	 *       заказанной сети, - и сделало бы это молча
	 *
	 */
	if(route.prefix > ((route.destination->size == 4) ? 32 : 128)){
		// Выводим в журнал сообщение о недопустимой длине префикса
		this->_log->print("%s: prefix length %u is out of range for the %s destination", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__, static_cast <uint32_t> (route.prefix), ((route.destination->size == 4) ? "IPv4" : "IPv6"));
		// Выводим отрицательный результат снятия
		return false;
	}
	// Устанавливаем длину префикса пути
	row.DestinationPrefix.PrefixLength = route.prefix;
	/**
	 * Если шлюз пути передан - переносим его в снимаемую запись
	 *
	 * @warning Итог переноса прежде НЕ проверялся вовсе, тогда как обращение add тот же
	 *          перенос проверяет и отказом на него отвечает. Шлюз неподдерживаемого
	 *          вида оставлял `NextHop` обнулённым, и снятие уходило системе не с тем
	 *          шлюзом, какой просили снять: система сличает записи целиком, и обнулённый
	 *          шлюз отвечает записи со шлюзом пустым - то есть пути НЕПОСРЕДСТВЕННОМУ.
	 *          Просьба снять путь через шлюз оборачивалась снятием пути прямого
	 *
	 */
	if(route.gateway != nullptr){
		// Если перенести шлюз пути не удалось
		if(!::__awh_to_sockaddr__(route.gateway.get(), row.NextHop)){
			// Выводим в журнал сообщение о неподдерживаемом виде адреса
			this->_log->print("%s: only IPv4 and IPv6 gateways are supported", log_t::flag_t::WARNING, ::__AWH_GATEWAY_BACKEND__);
			// Выводим отрицательный результат снятия
			return false;
		}
	// Если шлюз пути не передан
	} else row.NextHop.si_family = row.DestinationPrefix.Prefix.si_family;
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
