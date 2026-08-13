/**
 * @file: gateway.cpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бэкенда работы со шлюзами под операционную систему Linux — чтение
 *        таблицы маршрутизации, определение шлюза по умолчанию, заведение и снятие
 *        маршрутов через netlink
 *
 * @details У BSD и macOS таблица маршрутов читается вызовом `sysctl` с ветвью
 *          `PF_ROUTE`, а правка её ведётся записью сообщений в маршрутный сокет. У
 *          Linux и то и другое делается через netlink: выборкой RTM_GETROUTE, а правка -
 *          сообщениями RTM_NEWROUTE и RTM_DELROUTE
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <cstring>
#include <vector>

/**
 * Системные заголовочные файлы
 */
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/eth/gateway.hpp>
#include <net/backend/gnu/netlink.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статичные функции в пространство имён работы с маршрутами
 *
 */
namespace routing {
	/**
	 * @brief Структура разобранной записи таблицы маршрутов
	 *
	 */
	typedef struct Entry {
		// Номер сетевого интерфейса
		uint32_t index;
		// Длина префикса адреса назначения
		uint8_t prefix;
		// Признак наличия шлюза у маршрута
		bool routed;
		// Адрес назначения маршрута
		uint8_t destination[16];
		// Адрес шлюза маршрута
		uint8_t gateway[16];
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Entry() noexcept : index(0), prefix(0), routed(false), destination{0}, gateway{0} {}
	} entry_t;
	/**
	 * @brief Функция разбора записи таблицы маршрутов
	 *
	 * @param header  сообщение ядра с записью таблицы маршрутов
	 * @param family  семейство протоколов
	 * @param entry   объект для извлечения разобранной записи
	 * @return        результат разбора записи
	 *
	 */
	static bool parse(const struct nlmsghdr * header, const uint8_t family, entry_t & entry) noexcept {
		// Если сообщение не является записью таблицы маршрутов
		if(header->nlmsg_type != RTM_NEWROUTE)
			// Сообщаем, что запись не разобрана
			return false;
		// Получаем описание записи таблицы маршрутов
		const struct rtmsg * message = reinterpret_cast <const struct rtmsg *> (NLMSG_DATA(header));
		// Если семейство протоколов записи не совпадает с искомым
		if(message->rtm_family != family)
			// Сообщаем, что запись не разобрана
			return false;
		/**
		 * Пропускаем записи не главной таблицы маршрутов
		 *
		 * @note Таблиц у Linux несколько: помимо главной есть местная, куда ядро
		 *       складывает адреса самой машины, и таблицы, заводимые правилами. Ответом
		 *       на вопрос «куда пойдёт пакет» служит главная, прочие лишь запутали бы
		 *       обход
		 */
		if(message->rtm_table != RT_TABLE_MAIN)
			// Сообщаем, что запись не разобрана
			return false;
		// Устанавливаем длину префикса адреса назначения
		entry.prefix = message->rtm_dst_len;
		// Получаем размер адреса семейства протоколов
		const size_t size = ((family == AF_INET) ? 4 : 16);
		// Получаем первый признак записи
		const struct rtattr * attribute = reinterpret_cast <const struct rtattr *> (reinterpret_cast <const char *> (message) + NLMSG_ALIGN(sizeof(struct rtmsg)));
		// Получаем размер оставшихся признаков записи
		int32_t length = static_cast <int32_t> (header->nlmsg_len - NLMSG_LENGTH(sizeof(struct rtmsg)));
		/**
		 * Переходим по всем признакам записи
		 */
		for(; RTA_OK(attribute, length); attribute = RTA_NEXT(attribute, length)){
			/**
			 * Определяем тип признака записи
			 */
			switch(attribute->rta_type){
				// Если признак содержит адрес назначения маршрута
				case RTA_DST:
					// Копируем адрес назначения маршрута
					::memcpy(entry.destination, RTA_DATA(attribute), size);
				break;
				// Если признак содержит адрес шлюза маршрута
				case RTA_GATEWAY: {
					// Копируем адрес шлюза маршрута
					::memcpy(entry.gateway, RTA_DATA(attribute), size);
					// Запоминаем, что у маршрута есть шлюз
					entry.routed = true;
				} break;
				// Если признак содержит номер сетевого интерфейса
				case RTA_OIF:
					// Запоминаем номер сетевого интерфейса
					entry.index = *reinterpret_cast <const uint32_t *> (RTA_DATA(attribute));
				break;
			}
		}
		// Сообщаем, что запись разобрана
		return true;
	}
	/**
	 * @brief Функция добавления признака в сообщение ядру
	 *
	 * @param header сообщение ядру
	 * @param limit  предел размера сообщения
	 * @param type   тип добавляемого признака
	 * @param data   данные добавляемого признака
	 * @param size   размер данных добавляемого признака
	 * @return       результат добавления признака
	 *
	 */
	static bool attribute(struct nlmsghdr * header, const size_t limit, const uint16_t type, const void * data, const size_t size) noexcept {
		// Если признак в сообщение не помещается
		if((NLMSG_ALIGN(header->nlmsg_len) + RTA_ALIGN(RTA_LENGTH(size))) > limit)
			// Сообщаем, что признак не добавлен
			return false;
		// Получаем место под очередной признак
		struct rtattr * result = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (header) + NLMSG_ALIGN(header->nlmsg_len));
		// Устанавливаем тип признака
		result->rta_type = type;
		// Устанавливаем размер признака
		result->rta_len = RTA_LENGTH(size);
		// Копируем данные признака
		::memcpy(RTA_DATA(result), data, size);
		// Увеличиваем размер сообщения
		header->nlmsg_len = (NLMSG_ALIGN(header->nlmsg_len) + RTA_ALIGN(result->rta_len));
		// Сообщаем, что признак добавлен
		return true;
	}
	/**
	 * @brief Функция правки таблицы маршрутов
	 *
	 * @param type  тип запроса (RTM_NEWROUTE либо RTM_DELROUTE)
	 * @param flags признаки запроса
	 * @param route правимый маршрут
	 * @param log   объект работы с логами
	 * @return      результат правки таблицы маршрутов
	 *
	 */
	static bool modify(const uint16_t type, const uint16_t flags, const awh::eth::Gateway::route_t & route, const awh::log_t * log) noexcept {
		// Если адрес шлюза не инициализирован
		if(route.gateway == nullptr)
			// Сообщаем, что правка не выполнена
			return false;
		/**
		 * @note Размер адреса сверяется с обоими допустимыми значениями, а не с одним
		 *       лишь размером адреса IPv4. Развилка «четыре либо всё остальное»
		 *       разбирала бы всякий иной адрес как IPv6 и читала бы шестнадцать
		 *       октетов там, где их нет: адрес канального уровня, к примеру, занимает
		 *       шесть, и чтение вышло бы за его пределы
		 */
		if((route.gateway->size != 4) && (route.gateway->size != 16))
			// Сообщаем, что правка не выполнена
			return false;
		// Определяем семейство протоколов по размеру адреса
		const uint8_t family = ((route.gateway->size == 4) ? AF_INET : AF_INET6);
		// Получаем размер адреса семейства протоколов
		const size_t size = ((family == AF_INET) ? 4 : 16);
		/**
		 * @brief Структура сообщения правки таблицы маршрутов
		 *
		 */
		struct {
			// Заголовок сообщения
			struct nlmsghdr header;
			// Описание маршрута
			struct rtmsg route;
			// Набор признаков маршрута
			uint8_t attributes[256];
		} message{};
		// Устанавливаем размер сообщения
		message.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
		// Устанавливаем тип запроса
		message.header.nlmsg_type = type;
		// Устанавливаем признаки запроса с подтверждением
		message.header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_ACK | flags);
		// Устанавливаем порядковый номер запроса
		message.header.nlmsg_seq = 1;
		// Устанавливаем семейство протоколов маршрута
		message.route.rtm_family = family;
		// Устанавливаем длину префикса адреса назначения
		message.route.rtm_dst_len = route.prefix;
		// Устанавливаем главную таблицу маршрутов
		message.route.rtm_table = RT_TABLE_MAIN;
		/**
		 * Устанавливаем способ заведения маршрута
		 *
		 * @warning При СНОСЕ поле это ядром сличается, а не описывает намерение: маршрут,
		 *          заведённый иным способом, совпадением не считается и снос отвечает
		 *          ESRCH («No such process»). Заводим мы маршруты способом RTPROT_STATIC,
		 *          и снос с тем же значением умел убирать ЛИШЬ СВОИ маршруты - а
		 *          заведённые системой при поднятии сети (RTPROT_BOOT, RTPROT_DHCP,
		 *          RTPROT_KERNEL) не убирал вовсе, в том числе маршрут по умолчанию.
		 *          Замерено на стенде Debian 12 (13.08.2026): маршрут, заведённый
		 *          снаружи как `proto dhcp`, снос отвергал, а `ip route del` убирал
		 *
		 * @note RTPROT_UNSPEC при сносе означает «способ заведения любой» - именно так
		 *       поступает и `ip route del`, не задающий способа вовсе
		 */
		message.route.rtm_protocol = ((type == RTM_DELROUTE) ? RTPROT_UNSPEC : RTPROT_STATIC);
		// Устанавливаем область действия маршрута
		message.route.rtm_scope = RT_SCOPE_UNIVERSE;
		// Устанавливаем вид маршрута
		message.route.rtm_type = RTN_UNICAST;
		// Получаем предел размера сообщения
		const size_t limit = sizeof(message);
		/**
		 * @note Адрес назначения сверяется не только с допустимыми размерами, но и с
		 *       семейством самого шлюза: маршрут описывает один путь, и адрес
		 *       назначения иного семейства, нежели шлюз, ядро отвергло бы, а прочтён
		 *       он был бы размером семейства шлюза - то есть за своими пределами
		 */
		if((route.destination != nullptr) && (static_cast <size_t> (route.destination->size) == size)){
			// Если адрес назначения является IPv4
			if(family == AF_INET)
				// Добавляем адрес назначения маршрута
				::routing::attribute(&message.header, limit, RTA_DST, &awh_cast <const awh::net::addr_net_ipv4_t *> (route.destination.get())->address, size);
			// Добавляем адрес назначения маршрута
			else ::routing::attribute(&message.header, limit, RTA_DST, &awh_cast <const awh::net::addr_net_ipv6_t *> (route.destination.get())->address[0], size);
		}
		// Если адрес шлюза является IPv4
		if(family == AF_INET){
			// Получаем адрес шлюза маршрута
			const uint32_t gateway = awh_cast <const awh::net::addr_net_ipv4_t *> (route.gateway.get())->address;
			// Если адрес шлюза задан, добавляем его в сообщение
			if(gateway > 0)
				// Добавляем адрес шлюза маршрута
				::routing::attribute(&message.header, limit, RTA_GATEWAY, &gateway, size);
		// Если адрес шлюза является IPv6
		} else {
			// Получаем адрес шлюза маршрута
			const uint8_t * gateway = &awh_cast <const awh::net::addr_net_ipv6_t *> (route.gateway.get())->address[0];
			// Буфер нулевого адреса для сравнения
			const uint8_t zero[16] = {0};
			// Если адрес шлюза задан, добавляем его в сообщение
			if(::memcmp(gateway, zero, 16) != 0)
				// Добавляем адрес шлюза маршрута
				::routing::attribute(&message.header, limit, RTA_GATEWAY, gateway, size);
		}
		// Если имя сетевого интерфейса задано
		if(!route.ifname.empty()){
			// Получаем номер сетевого интерфейса по его имени
			const uint32_t index = ::if_nametoindex(route.ifname.c_str());
			// Если номер сетевого интерфейса получен
			if(index > 0)
				// Добавляем номер сетевого интерфейса
				::routing::attribute(&message.header, limit, RTA_OIF, &index, sizeof(index));
		}
		// Выполняем инициализацию объекта опроса ядра
		const awh::gnu::netlink_t netlink(log);
		// Выполняем отправку сообщения ядру
		return netlink.commit(&message, message.header.nlmsg_len);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::eth::Gateway::Route::Route() noexcept :
 ifname{""}, prefix(0),
 destination(nullptr), gateway(nullptr) {}

/**
 * @brief Метод получения маршрута для указанного адреса
 *
 * @details Спрашивает у ядра, каким путём оно отправит пакет для заданного адреса, и
 *          дозаполняет объект маршрута ответом
 *
 * @par Намеренные решения
 *
 *      **Выборка таблицы, а не запрос одного маршрута.** У Linux есть и второй путь:
 *      послать RTM_GETROUTE с адресом назначения и получить в ответ ту запись, которой
 *      ядро воспользуется. Здесь взята выборка целиком - затем, что метод отвечает не
 *      только на вопрос «куда пойдёт пакет», но и на вопрос «каков маршрут по
 *      умолчанию», а его точечным запросом не задать: у маршрута по умолчанию адреса
 *      назначения нет вовсе
 *
 *      Обход прерывается на первой подошедшей записи, так что лишним чтением это не
 *      оборачивается
 *
 * @param route объект для извлечения маршрута
 * @return      результат получения маршрута
 *
 */
bool awh::eth::Gateway::get(route_t & route) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес шлюза не инициализирован
		if(route.gateway == nullptr)
			// Выходим из функции
			return result;
		/**
		 * @note Размер адреса сверяется с обоими допустимыми значениями, а не с одним
		 *       лишь размером адреса IPv4. Развилка «четыре либо всё остальное»
		 *       разбирала бы всякий иной адрес как IPv6 и читала бы шестнадцать
		 *       октетов там, где их нет
		 */
		if((route.gateway->size != 4) && (route.gateway->size != 16))
			// Выходим из функции
			return result;
		// Определяем семейство протоколов по размеру адреса
		const uint8_t family = ((route.gateway->size == 4) ? AF_INET : AF_INET6);
		// Получаем размер адреса семейства протоколов
		const size_t size = ((family == AF_INET) ? 4 : 16);
		// Номер искомого сетевого интерфейса
		uint32_t index = 0;
		// Если имя сетевого интерфейса задано
		if(!route.ifname.empty())
			// Получаем номер сетевого интерфейса по его имени
			index = ::if_nametoindex(route.ifname.c_str());
		// Буфер искомого адреса назначения
		uint8_t destination[16] = {0};
		// Признак того, что адрес назначения задан
		bool wanted = false;
		/**
		 * @note Адрес назначения сверяется с семейством самого шлюза: маршрут описывает
		 *       один путь, и адрес иного семейства был бы прочтён размером семейства
		 *       шлюза - то есть за своими пределами
		 */
		if((route.destination != nullptr) && (static_cast <size_t> (route.destination->size) == size)){
			// Если адрес назначения является IPv4
			if(family == AF_INET)
				// Копируем искомый адрес назначения
				::memcpy(destination, &awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address, 4);
			// Копируем искомый адрес назначения
			else ::memcpy(destination, &awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], 16);
			// Буфер нулевого адреса для сравнения
			const uint8_t zero[16] = {0};
			// Запоминаем, задан ли адрес назначения
			wanted = (::memcmp(destination, zero, size) != 0);
		}
		/**
		 * Определяем, ищется ли маршрут по умолчанию
		 *
		 * @note Маршрут по умолчанию отбирается тем, что ни адреса назначения, ни
		 *       устройства не задано: спрашивающему нужен путь «куда попало», а таким
		 *       путём и служит маршрут с нулевой длиной префикса
		 */
		const bool fallback = (!wanted && (index == 0));
		// Разобранная запись таблицы маршрутов
		::routing::entry_t entry;
		// Признак того, что запись найдена
		bool found = false;
		// Выполняем инициализацию объекта опроса ядра
		const gnu::netlink_t netlink(this->_log);
		/**
		 * Выполняем выборку таблицы маршрутов
		 */
		netlink.dump(RTM_GETROUTE, family, [&](const struct nlmsghdr * header) noexcept -> bool {
			// Разобранная запись таблицы маршрутов
			::routing::entry_t current;
			// Если запись разобрать не удалось
			if(!::routing::parse(header, family, current))
				// Продолжаем обход
				return true;
			// Если ищется маршрут по умолчанию
			if(fallback){
				// Маршрут по умолчанию узнаётся нулевой длиной префикса и наличием шлюза
				if((current.prefix != 0) || !current.routed)
					// Продолжаем обход
					return true;
			// Если задан искомый адрес назначения
			} else if(wanted){
				// Если адрес назначения записи не совпадает с искомым
				if(::memcmp(current.destination, destination, size) != 0)
					// Продолжаем обход
					return true;
			// Если задано лишь устройство
			} else if(current.index != index)
				// Продолжаем обход
				return true;
			// Если задано устройство, а запись принадлежит другому
			if((index > 0) && (current.index != index))
				// Продолжаем обход
				return true;
			// Запоминаем найденную запись
			entry = current;
			// Запоминаем, что запись найдена
			found = true;
			// Прекращаем обход
			return false;
		});
		// Если запись найдена
		if((result = found)){
			// Буфер имени сетевого интерфейса
			char ifname[IF_NAMESIZE];
			// Если имя сетевого интерфейса по его номеру получено
			if((entry.index > 0) && (::if_indextoname(entry.index, ifname) != nullptr))
				// Устанавливаем название сетевого интерфейса
				route.ifname = ifname;
			// Устанавливаем длину префикса адреса назначения
			route.prefix = entry.prefix;
			// Если адрес назначения не инициализирован
			if(route.destination == nullptr)
				// Инициализируем объект адреса назначения в маршруте
				route.destination = ((family == AF_INET) ? unique_ptr <net::addr_t> (new net::addr_net_ipv4_t()) : unique_ptr <net::addr_t> (new net::addr_net_ipv6_t()));
			// Если адрес является IPv4
			if(family == AF_INET){
				// Устанавливаем адрес назначения маршрута
				::memcpy(&awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address, entry.destination, 4);
				// Устанавливаем адрес шлюза маршрута
				::memcpy(&awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address, entry.gateway, 4);
			// Если адрес является IPv6
			} else {
				// Устанавливаем адрес назначения маршрута
				::memcpy(&awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], entry.destination, 16);
				// Устанавливаем адрес шлюза маршрута
				::memcpy(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], entry.gateway, 16);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(route.ifname), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления маршрута
 *
 * @warning Правка таблицы маршрутов затрагивает **всю машину**, а не наш процесс:
 *          заведённый маршрут переживёт завершение работы, и убирать его следует за
 *          собой
 *
 * @param route добавляемый маршрут
 * @return      результат добавления маршрута
 *
 */
bool awh::eth::Gateway::add(const route_t & route) const noexcept {
	// Выполняем заведение маршрута
	return ::routing::modify(RTM_NEWROUTE, (NLM_F_CREATE | NLM_F_EXCL), route, this->_log);
}
/**
 * @brief Метод удаления маршрута
 *
 * @param route удаляемый маршрут
 * @return      результат удаления маршрута
 *
 */
bool awh::eth::Gateway::remove(const route_t & route) const noexcept {
	// Выполняем снятие маршрута
	return ::routing::modify(RTM_DELROUTE, 0, route, this->_log);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::eth::Gateway::Gateway(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Gateway::~Gateway() noexcept {}
