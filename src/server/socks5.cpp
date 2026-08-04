/**
 * @file: socks5.cpp
 * @date: 2026-05-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация сервера SOCKS5-прокси — авторизация клиентов,
 *        установка исходящих соединений по командам CONNECT и BIND,
 *        проксирование трафика и обслуживание UDP-ассоциаций через пул выделенных UDP-серверов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Если размер MTU для UDP сообщений в IPv4 не определён
 */
#ifndef AWH_MTU_UDP_IPV4_PAYLOAD_SIZE
	/**
	 * Полезная нагрузка UDP для IPv4: 1400 байт.
	 *
	 * Для работы с DTLS лучше заложить 1454 байта, но для UDP достаточно 1400 байт.
	 *
	 * База: 1500 (Ethernet MTU) - 20 (IPv4) - 8 (UDP) = 1472.
	 * Дополнительно закладываем резерв 72 байта на инкапсуляцию
	 * (туннели, дополнительные сервисные заголовки).
	 */
	#define AWH_MTU_UDP_IPV4_PAYLOAD_SIZE 0x578
#endif

/**
 * Если размер MTU для UDP сообщений в IPv6 не определён
 */
#ifndef AWH_MTU_UDP_IPV6_PAYLOAD_SIZE
	/**
	 * Полезная нагрузка UDP для IPv6: 1380 байт.
	 *
	 * База: 1500 (Ethernet MTU) - 40 (IPv6) - 8 (UDP) = 1452.
	 * Дополнительно закладываем резерв 72 байта на инкапсуляцию.
	 * Для IPv6 это особенно важно: фрагментация маршрутизаторами не выполняется.
	 */
	#define AWH_MTU_UDP_IPV6_PAYLOAD_SIZE 0x564
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <random>
#include <climits>
#include <cstring>
#include <optional>
#include <algorithm>

/**
 * Системный заголовочный файл
 */
#include <netinet/in.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <server/socks5.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Инкапсулируем внутренние классы в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Класс для управления динамическим выделением портов для UDP серверов
	 *
	 */
	typedef class PortAllocator {
		private:
			// Список доступных портов для выделения
			vector <uint16_t> _ports;
			// Множество портов в пуле для быстрой проверки дубликатов
			unordered_set <uint16_t> _free;
		public:
			/**
			 * @brief Метод получения количества доступных портов для выделения
			 *
			 * @return количество доступных портов для выделения
			 *
			 */
			size_t available() const noexcept {
				// Возвращаем количество доступных портов для выделения
				return this->_ports.size();
			}
		public:
			/**
			 * @brief Метод возвращения порта в пул после его использования
			 *
			 * @param port порт для возвращения в пул
			 *
			 */
			void release(const uint16_t port) noexcept {
				// Защита от дублирования порта при повторном освобождении
				if(this->_free.insert(port).second)
					// Возвращаем порт в пул, добавляя его обратно в список доступных портов для выделения
					this->_ports.push_back(port);
			}
		public:
			/**
			 * @brief Метод выделения порта для UDP сервера
			 *
			 * @return выделенный порт или nullopt, если порты закончились
			 *
			 */
			optional <uint16_t> allocate() noexcept {
				// Если список портов для выделения пуст
				if(this->_ports.empty())
					// Возвращаем nullopt, если порты кончились
					return std::nullopt;
				// Получаем порт из списка доступных портов для выделения
				const uint16_t result = this->_ports.back();
				// Удаляем выделенный порт из списка доступных портов для выделения
				this->_ports.pop_back();
				// Удаляем порт из множества свободных портов
				this->_free.erase(result);
				// Возвращаем выделенный порт
				return result;
			}
		public:
			/**
			 * @brief Метод инициализации списка портов для выделения
			 *
			 * @param min минимальный порт для выделения
			 * @param max максимальный порт для выделения
			 * @throws    std::invalid_argument если диапазон портов некорректный
			 *
			 */
			void init(const uint32_t min, const uint32_t max) noexcept {
				// Очищаем множество свободных портов
				this->_free.clear();
				// Очищаем список портов для выделения
				this->_ports.clear();
				// Проверяем корректность диапазона портов
				if((min > max) || (min < 1024) || (max > 65535))
					// Выходим из функции, если диапазон портов некорректный
					return;
				// Резервируем память для списка портов
				this->_ports.reserve(max - min + 1);
				/**
				 * Заполняем список портов для выделения.
				 */
				for(uint32_t port = min; port <= max; ++port){
					// Добавляем порт в список доступных портов для выделения
					this->_ports.push_back(static_cast <uint16_t> (port));
					// Добавляем порт в множество свободных портов
					this->_free.insert(static_cast <uint16_t> (port));
				}
				/**
				 * Перемешиваем порты, чтобы избежать предсказуемой последовательности
				 * при выделении диапазона.
				 */
				::random_device randev;
				// Выбираем стандарт рандомайзера
				::mt19937 generator(randev());
				// Перемешиваем список портов для выделения
				::shuffle(this->_ports.begin(), this->_ports.end(), generator);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit PortAllocator() noexcept {}
	} port_alloc_t;

	/**
	 * @brief Статический объект для управления динамическим выделением портов для UDP серверов
	 *
	 */
	port_alloc_t __awh_port_allocator__;
};

/**
 * @brief Инкапсулируем внутренние статические функции в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Метод проверки наличия UDP-сервера в списке активных событий
	 *
	 * @param events список идентификаторов UDP-серверов
	 * @param eid    идентификатор события
	 * @return       результат проверки
	 *
	 */
	bool udpEventHas(const unordered_set <event::id_t> & events, const event::id_t eid) noexcept {
		// Выполняем поиск идентификатора события в множестве
		return (events.count(eid) > 0);
	}

	/**
	 * @brief Метод удаления UDP-сервера из списка активных событий
	 *
	 * @param events список идентификаторов UDP-серверов
	 * @param eid    идентификатор события
	 *
	 */
	void udpEventRemove(vector <event::id_t> & events, unordered_set <event::id_t> & eventSet, const event::id_t eid) noexcept {
		// Выполняем поиск идентификатора события в списке
		auto i = ::find(events.begin(), events.end(), eid);
		// Если идентификатор события найден
		if(i != events.end())
			// Удаляем идентификатор события из списка
			events.erase(i);
		// Удаляем идентификатор события из множества
		eventSet.erase(eid);
	}
};

/**
 * @brief Инкапсулируем статические параметры локального кэша в пространство имён
 *
 */
namespace {
	/**
	 * @brief Структура для хранения данных в локальном кэше
	 *
	 */
	typedef struct Cache {
		// Размер данных в буфере
		size_t size;
		// Доменное имя для резолвинга
		string domain;
		// Идентификатор события сервера, которому принадлежит клиент
		event::id_t eid;
		// Параметры подключения к удалённому серверу
		unique_ptr <net::attr_net_t> attr;
		// Буфер для хранения данных
		uint8_t buffer[AWH_MTU_UDP_IPV4_PAYLOAD_SIZE];
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Cache() noexcept :
		 size(0), domain{""}, eid(0),
		 attr(nullptr), buffer{0} {}
	} cache_t;

	/**
	 * @brief Локальный кэш для хранения данных UDP сообщений, индексируемый по идентификатору события клиента
	 *
	 */
	thread_local unordered_map <event::id_t, cache_t> __awh_cache__;
};

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * @brief Тип сообщений для обмена между процессами в кластере
	 *
	 */
	enum class cluster_message_t : uint8_t {
		NONE     = 0x00, // Нет сообщения
		INTERNAL = 0x01, // Сообщение для внутреннего обмена между процессами кластера
		EXTERNAL = 0x02  // Сообщение для обмена между процессами кластера и внешними событиями
	};

	/**
	 * @brief Функция комбинирования хеш-кодов
	 *
	 * @param seed  исходный хеш-код
	 * @param value добавочный хеш-код
	 *
	 */
	void combine(size_t & seed, const size_t value) noexcept {
		// Комбинируем хеш-коды
		seed ^= (value + 0x9E3779B9 + (seed << 6) + (seed >> 2));
	}

	/**
	 * @brief Размер данных в буфере
	 *
	 */
	thread_local size_t __awh_size__ = 0;

	/**
	 * @brief Нулевой MAC-адрес для сравнения
	 *
	 */
	static uint8_t __awh_zero_mac__[6] = {0};

	/**
	 * @brief Нулевой IPv6-адрес для сравнения
	 *
	 */
	static uint8_t __awh_zero_ipv6__[16] = {0};

	/**
	 * @brief Буфер временного хранения данных UDP сообщений
	 *
	 */
	thread_local uint8_t __awh_buffer__[AWH_MTU_UDP_IPV4_PAYLOAD_SIZE] = {0};
};

/**
 * @brief Фабричный метод создания идентификатора инициатора запроса
 *
 * @param addr объект параметров подключения инициатора запроса
 * @return     идентификатор инициатора запроса
 *
 */
awh::server::Socks5::Origin & awh::server::Socks5::Origin::from(const net::attr_t * addr) noexcept {
	// Если объект параметров подключения инициатора запроса передан корректно
	if(addr != nullptr){
		// Устанавливаем тип адреса инициатора запроса
		this->type = addr->type;
		/**
		 * Определяем тип адреса хоста для подключения
		 */
		switch(static_cast <uint8_t> (addr->type)){
			// Если тип адреса соответствует FQDN
			case static_cast <uint8_t> (net::type_t::FQDN): {
				// Извлекаем доменное имя хоста для подключения
				const string & fqdn = awh_cast <const net::attr_fqdn_t *> (addr)->domain;
				// Если доменное имя хоста для подключения не пустое
				if(!fqdn.empty()){
					// Определяем длину доменного имени хоста для подключения, ограничивая её размером буфера для хранения доменного имени
					const size_t length = ::min(fqdn.length(), sizeof(this->fqdn.data) - 1);
					// Копируем доменное имя хоста для подключения
					::memcpy(this->fqdn.data, &fqdn[0], length);
					// Устанавливаем завершающий нулевой символ
					this->fqdn.data[length] = '\0';
				// Зануляем буфер для хранения доменного имени, если доменное имя хоста для подключения пустое
				} else ::memset(this->fqdn.data, 0, sizeof(this->fqdn.data));
				// Устанавливаем порт хоста для подключения
				this->fqdn.port = htons(awh_cast <const net::attr_fqdn_t *> (addr)->port);
			} break;
			// Если тип адреса соответствует IPv4
			case static_cast <uint8_t> (net::type_t::IPV4): {
				// Устанавливаем порт
				this->ip4.port = htons(awh_cast <const net::attr_net_t *> (addr)->port);
				// Если адрес инициатора запроса установлен
				if(awh_cast <const net::attr_net_t *> (addr)->ip != nullptr)
					// Устанавливаем адрес инициатора запроса
					this->ip4.address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (addr)->ip.get())->address;
			} break;
			// Если тип адреса соответствует IPv6
			case static_cast <uint8_t> (net::type_t::IPV6): {
				// Устанавливаем порт
				this->ip6.port = htons(awh_cast <const net::attr_net_t *> (addr)->port);
				// Если адрес инициатора запроса установлен
				if(awh_cast <const net::attr_net_t *> (addr)->ip != nullptr)
					// Устанавливаем адрес инициатора запроса
					::memcpy(&this->ip6.address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (addr)->ip.get())->address[0], 16);
			} break;
		}
	}
	// Возвращаем идентификатор инициатора запроса
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param other другой объект для сравнения
 * @return      результат сравнения
 *
 */
bool awh::server::Socks5::Origin::operator == (const Origin & other) const noexcept {
	// Сравниваем семейство адресов
	if(this->type != other.type)
		// Возвращаем отрицательный результат
		return false;
	/**
	 * Сравниваем данные в зависимости от семейства адресов
	 */
	switch(static_cast <uint8_t> (this->type)){
		// Если тип адреса соответствует FQDN
		case static_cast <uint8_t> (net::type_t::FQDN):
			// Выполняем сравнение FQDN адресов
			return (
				(this->fqdn.port == other.fqdn.port) &&
				(::strcmp(this->fqdn.data, other.fqdn.data) == 0)
			);
		// Если адрес установлен как IPv4
		case static_cast <uint8_t> (net::type_t::IPV4):
			// Выполняем сравнение IPv4 адресов
			return (
				(this->ip4.port == other.ip4.port) &&
				(this->ip4.address == other.ip4.address)
			);
		// Если адрес установлен как IPv6
		case static_cast <uint8_t> (net::type_t::IPV6):
			// Выполняем сравнение IPv6 адресов
			return (
				(this->ip6.port == other.ip6.port) &&
				(this->ip6.address == other.ip6.address)
			);
		// В остальных случаях выводим отрицательный результат
		default: return false;
	}
}
/**
 * @brief Конструктор
 *
 */
awh::server::Socks5::Origin::Origin() noexcept : type(net::type_t::NONE) {};

/**
 * @brief Оператор вычисления хеш-кода
 *
 * @param id объект для вычисления хеш-кода
 * @return   хеш-код объекта
 *
 */
size_t awh::server::Socks5::Origin_Hash::operator()(const origin_t & id) const noexcept {
	// Вычисляем начальный хеш-код по семейству адресов
	size_t result = hash <uint8_t> {}(static_cast <uint8_t> (id.type));
	/**
	 * Хешируем данные в зависимости от семейства адресов
	 */
	switch(static_cast <uint8_t> (id.type)){
		// Если тип адреса соответствует FQDN
		case static_cast <uint8_t> (net::type_t::FQDN): {
			// Комбинируем хеш-код порта
			::combine(result, hash <uint16_t> {}(id.fqdn.port));
			// Определяем фактическую длину доменного имени
			const size_t length = ::strnlen(id.fqdn.data, sizeof(id.fqdn.data));
			// Комбинируем хеш-код доменного имени
			::combine(result, hash <string> {}(string(id.fqdn.data, length)));
		} break;
		// Если адрес установлен как IPv4
		case static_cast <uint8_t> (net::type_t::IPV4): {
			// Комбинируем хеш-код порта
			::combine(result, hash <uint16_t> {}(id.ip4.port));
			// Комбинируем хеш-код IPv4 адреса
			::combine(result, hash <uint32_t> {}(id.ip4.address));
		} break;
		// Если адрес установлен как IPv6
		case static_cast <uint8_t> (net::type_t::IPV6): {
			// Комбинируем хеш-код порта
			::combine(result, hash <uint16_t> {}(id.ip6.port));
			// Инициализируем переменные для хранения частей IPv6 адреса
			uint64_t hi = 0, lo = 0;
			// Читаем 128-битный адрес как два uint64_t без невыровненного доступа
			::memcpy(&hi, &id.ip6.address[0], sizeof(hi));
			::memcpy(&lo, &id.ip6.address[8], sizeof(lo));
			// Комбинируем хеш-коды IPv6 адреса
			::combine(result, hash <uint64_t> {}(hi));
			::combine(result, hash <uint64_t> {}(lo));
		} break;
	}
	// Возвращаем хеш-код
	return result;
}

/**
 * @brief Конструктор
 *
 */
awh::server::Socks5::Peer::Peer() noexcept : eid(0), did(0) {}

/**
 * @brief Конструктор
 *
 */
awh::server::Socks5::UDP_Server::UDP_Server() noexcept :
 begin(0), end(0), count(0), address(nullptr) {}

/**
 * @brief Метод удаления связи DNS-запроса с пиром
 *
 * @param did идентификатор DNS-запроса
 *
 */
void awh::server::Socks5::dropResolve(const unit::dns_t::id_t did) noexcept {
	// Выполняем поиск идентификатора DNS-запроса
	auto i = this->_resolves.find(did);
	// Если идентификатор DNS-запроса найден
	if(i != this->_resolves.end())
		// Удаляем связь DNS-резолвера и идентификатора пира
		this->_resolves.erase(i);
}
/**
 * @brief Метод отправки SOCKS5-ответа прокси-клиенту
 *
 * @param eid      идентификатор пира
 * @param ctx      контекст протокола SOCKS5
 * @param dropPeer закрыть пира после ответа об ошибке
 * @return         результат отправки ответа
 *
 */
bool awh::server::Socks5::sendReply(const event::id_t eid, proto::socks5_t::ctx_t & ctx, const bool dropPeer) noexcept {
	// Размер буфера данных
	size_t size = 0;
	// Буфер данных ответа
	uint8_t * buffer = nullptr;
	// Если извлечение буфера данных ответа не выполнено
	if(!this->_socks5.buffer(&buffer, size, ctx)){
		// Если требуется закрыть пира после ответа об ошибке
		if(dropPeer)
			// Удаляем подключённого пира
			this->_unit->server.destroy(eid);
		// Возвращаем отрицательный результат
		return false;
	}
	// Если отправка ответа прокси-клиенту не выполнена
	if(this->_unit->server.send(eid, buffer, size) != size){
		// Удаляем подключённого пира
		this->_unit->server.destroy(eid);
		// Возвращаем отрицательный результат
		return false;
	}
	// Если статус ответа от прокси-сервера соответствует успешному выполнению команды
	if(ctx.status == proto::socks5_t::status_t::SUCCESS)
		// Устанавливаем статус успешного выполнения команды
		ctx.state = proto::socks5_t::state_t::COMPLETED;
	// Если статус ответа от прокси-сервера не соответствует успешному выполнению команды
	else {
		// Устанавливаем статус ошибки работы с прокси-сервером
		ctx.state = proto::socks5_t::state_t::BROKEN;
		// Если требуется закрыть пира после ответа об ошибке
		if(dropPeer)
			// Удаляем подключённого пира
			this->_unit->server.destroy(eid);
	}
	// Возвращаем положительный результат
	return true;
}
/**
 * @brief Метод изменения статуса сервера
 *
 * @param index  индекс очереди запускаемого события
 * @param status новый статус сервера
 *
 */
void awh::server::Socks5::status(const uint8_t index, const event::status_t status) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Временное состояние сервера
		 */
		switch(index){
			// Если мы получили статус события сервера
			case 0: {
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("status", status);
				// Если работа сервера запущена
				if(status == event::status_t::LAUNCHED){
					// Выполняем запуск работы сервера, если сервер не запущен
					if(!this->_unit->server.launch(this->_id.eid)){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("This server ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_id.eid);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("This server ID=%u cannot be started", log_t::flag_t::WARNING, this->_id.eid);
							#endif
						}
					// Если сервер запущен удачно
					} else {
						/**
						 * Определяем семейство адресов с которым работает сервер
						 */
						switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
							// Если сервер работает с адресами IPv4
							case static_cast <uint8_t> (event::family_t::IPV4):
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", static_cast <event::id_t> (this->_id.eid), this->_unit->server.getAddress(static_cast <event::id_t> (this->_id.eid), event::address_t::IPV4), this->_unit->server.getPort(static_cast <event::id_t> (this->_id.eid)));
							break;
							// Если сервер работает с адресами IPv6
							case static_cast <uint8_t> (event::family_t::IPV6):
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", static_cast <event::id_t> (this->_id.eid), this->_unit->server.getAddress(static_cast <event::id_t> (this->_id.eid), event::address_t::IPV6), this->_unit->server.getPort(static_cast <event::id_t> (this->_id.eid)));
							break;
						}
						/**
						 * Определяем режим работы сервера
						 */
						switch(static_cast <uint8_t> (this->_unit->server.clusterMode())){
							// Если сервер запущен в режиме кластера
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								// Если DNS-резолвер подключён
								if(this->_dns.client != nullptr){
									// Количество активных DNS-резолверов
									uint16_t count = 0;
									// Если количество активных DNS-резолверов для семейства адресов IPv4 больше нуля
									if((count = this->_dns.client->resolvers(event::family_t::IPV4)) > 0)
										// Выполняем инициализацию DNS-резолвера для текущего сервера
										this->_dns.client->init(event::family_t::IPV4, count);
									// Если количество активных DNS-резолверов для семейства адресов IPv6 больше нуля
									if((count = this->_dns.client->resolvers(event::family_t::IPV6)) > 0)
										// Выполняем инициализацию DNS-резолвера для текущего сервера
										this->_dns.client->init(event::family_t::IPV6, count);
								}
							} break;
							// Если сервер не запущен в режиме кластера
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								// Если порты для UDP серверов установлены корректно
								if((this->_udp.begin > 0) && (this->_udp.begin < this->_udp.end)){
									// Инициализируем пул портов для подключения к сети клиентов
									::__awh_port_allocator__.init(this->_udp.begin, this->_udp.end);
									// Если количество выделенных портов для UDP серверов больше нуля
									if(this->_udp.count > 0){
										/**
										 * Определяем семейство адресов для запуска UDP сервера
										 */
										switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
											// Если процесс работает с адресами IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												/**
												 * Выполняем инициализацию указанного количества UDP серверов
												 */
												for(uint16_t i = 0; i < this->_udp.count; i++){
													// Выделяем порт для UDP-сервера
													auto port = ::__awh_port_allocator__.allocate();
													// Если порт выделен успешно, отправляем его воркеру для запуска UDP сервера
													if(port){
														// Создаём UDP сервер для работы с клиентами
														const event::id_t uid = this->_unit->server.issue(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
														// Добавляем событие UDP сервера в множество событий
														this->_udp.eventSet.insert(uid);
														// Добавляем событие UDP сервера в список событий
														this->_udp.events.push_back(uid);
														// Устанавливаем опции события UDP сервера
														if(this->_unit->server.setOptions(uid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
															// Устанавливаем порт и адрес для UDP сервера
															if(this->_unit->server.setPort(uid, * port) && this->_unit->server.setAddress(uid, event::address_t::IPV4, this->_udp.address.get())){
																// Выполняем фиксацию настроек события сервера
																if(this->_unit->server.commit(uid)){
																	// Выполняем запуск события
																	if(!this->_unit->server.launch(uid)){
																		// Если функция обратного вызова не установлена
																		if(!this->_callback.is("error")){
																			/**
																			 * Если включён режим отладки
																			 */
																			#if DEBUG_MODE
																				// Записываем ошибку в лог
																				this->_log->debug("Creating UDP server for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																			/**
																			 * Если режим отладки не включён
																			 */
																			#else
																				// Записываем ошибку в лог
																				this->_log->print("Creating UDP server for peer ID=%u is failed", log_t::flag_t::WARNING, uid);
																			#endif
																		}
																	// Если сервер удачно запущен
																	} else {
																		// Выполняем функцию обратного вызова
																		this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", uid, this->_unit->server.getAddress(uid, event::address_t::IPV4), this->_unit->server.getPort(uid));
																		// Продолжаем цикл для запуска следующего UDP сервера
																		continue;
																	}
																// Если фиксация настроек события сервера не выполнена
																} else {
																	// Если функция обратного вызова не установлена
																	if(!this->_callback.is("error")){
																		/**
																		 * Если включён режим отладки
																		 */
																		#if DEBUG_MODE
																			// Записываем ошибку в лог
																			this->_log->debug("UDP server parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																		/**
																		 * Если режим отладки не включён
																		 */
																		#else
																			// Записываем ошибку в лог
																			this->_log->print("UDP server parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, uid);
																		#endif
																	}
																}
															// Если установка порта и адреса удалённого сервера для подключения не выполнена
															} else {
																// Если функция обратного вызова не установлена
																if(!this->_callback.is("error")){
																	/**
																	 * Если включён режим отладки
																	 */
																	#if DEBUG_MODE
																		// Записываем ошибку в лог
																		this->_log->debug("Port and address of the UDP server were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																	/**
																	 * Если режим отладки не включён
																	 */
																	#else
																		// Записываем ошибку в лог
																		this->_log->print("Port and address of the UDP server were not set correctly for node with ID=%u", log_t::flag_t::WARNING, uid);
																	#endif
																}
															}
														// Если установка опций события не выполнена
														} else {
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Записываем ошибку в лог
																	this->_log->debug("Failed to configure UDP server events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Записываем ошибку в лог
																	this->_log->print("Failed to configure UDP server events settings for node with ID=%u", log_t::flag_t::WARNING, uid);
																#endif
															}
														}
														// Удаляем UDP сервер, если его создание не выполнено
														this->_unit->server.destroy(uid);
														// Удаляем событие UDP сервера из списка событий
														::udpEventRemove(this->_udp.events, this->_udp.eventSet, uid);
													// Если порт для UDP сервера не выделен, выходим из цикла, так как порты закончились
													} else break;
												}
											} break;
											// Если процесс работает с адресами IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												/**
												 * Выполняем инициализацию указанного количества UDP серверов
												 */
												for(uint16_t i = 0; i < this->_udp.count; i++){
													// Выделяем порт для UDP-сервера
													auto port = ::__awh_port_allocator__.allocate();
													// Если порт выделен успешно, отправляем его воркеру для запуска UDP сервера
													if(port){
														// Создаём UDP сервер для работы с клиентами
														const event::id_t uid = this->_unit->server.issue(event::family_t::IPV6, event::type_t::DATAGRAM, event::protocol_t::UDP);
														// Добавляем событие UDP сервера в множество событий
														this->_udp.eventSet.insert(uid);
														// Добавляем событие UDP сервера в список событий
														this->_udp.events.push_back(uid);
														// Устанавливаем опции события UDP сервера
														if(this->_unit->server.setOptions(uid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
															// Устанавливаем порт и адрес для UDP сервера
															if(this->_unit->server.setPort(uid, * port) && this->_unit->server.setAddress(uid, event::address_t::IPV6, "::")){
																// Выполняем фиксацию настроек события сервера
																if(this->_unit->server.commit(uid)){
																	// Выполняем запуск события
																	if(!this->_unit->server.launch(uid)){
																		// Если функция обратного вызова не установлена
																		if(!this->_callback.is("error")){
																			/**
																			 * Если включён режим отладки
																			 */
																			#if DEBUG_MODE
																				// Записываем ошибку в лог
																				this->_log->debug("Creating UDP server for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																			/**
																			 * Если режим отладки не включён
																			 */
																			#else
																				// Записываем ошибку в лог
																				this->_log->print("Creating UDP server for peer ID=%u is failed", log_t::flag_t::WARNING, uid);
																			#endif
																		}
																	// Если сервер удачно запущен
																	} else {
																		// Выполняем функцию обратного вызова
																		this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", uid, this->_unit->server.getAddress(uid, event::address_t::IPV6), this->_unit->server.getPort(uid));
																		// Продолжаем цикл для запуска следующего UDP сервера
																		continue;
																	}
																// Если фиксация настроек события сервера не выполнена
																} else {
																	// Если функция обратного вызова не установлена
																	if(!this->_callback.is("error")){
																		/**
																		 * Если включён режим отладки
																		 */
																		#if DEBUG_MODE
																			// Записываем ошибку в лог
																			this->_log->debug("UDP server parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																		/**
																		 * Если режим отладки не включён
																		 */
																		#else
																			// Записываем ошибку в лог
																			this->_log->print("UDP server parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, uid);
																		#endif
																	}
																}
															// Если установка порта и адреса удалённого сервера для подключения не выполнена
															} else {
																// Если функция обратного вызова не установлена
																if(!this->_callback.is("error")){
																	/**
																	 * Если включён режим отладки
																	 */
																	#if DEBUG_MODE
																		// Записываем ошибку в лог
																		this->_log->debug("Port and address of the UDP server were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																	/**
																	 * Если режим отладки не включён
																	 */
																	#else
																		// Записываем ошибку в лог
																		this->_log->print("Port and address of the UDP server were not set correctly for node with ID=%u", log_t::flag_t::WARNING, uid);
																	#endif
																}
															}
														// Если установка опций события не выполнена
														} else {
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Записываем ошибку в лог
																	this->_log->debug("Failed to configure UDP server events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, uid);
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Записываем ошибку в лог
																	this->_log->print("Failed to configure UDP server events settings for node with ID=%u", log_t::flag_t::WARNING, uid);
																#endif
															}
														}
														// Удаляем UDP сервер, если его создание не выполнено
														this->_unit->server.destroy(uid);
														// Удаляем событие UDP сервера из списка событий
														::udpEventRemove(this->_udp.events, this->_udp.eventSet, uid);
													// Если порт для UDP сервера не выделен, выходим из цикла, так как порты закончились
													} else break;
												}
											} break;
										}
									}
								}
							} break;
						}
					}
				}
			} break;
			// Если мы получили статус события DNS-резолвера
			case 1: {
				/**
				 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
				 */
				switch(static_cast <uint8_t> (status)){
					// Если событие DNS-резолвера запущено
					case static_cast <uint8_t> (event::status_t::LAUNCHED): {
						/**
						 * Определяем принадлежит ли хост, к IP-адресу
						 */
						switch(static_cast <uint8_t> (this->_unit->addr.host(this->_host))){
							// Для типа IPv4
							case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
								// Устанавливаем адрес хоста целевой текущей машины
								if(this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV4, this->_host)){
									// Если событие сервера не запущено, запускаем его
									if(awh_cast <unit::unit_t *> (&this->_unit->server)->status(this->_id.eid) == event::status_t::NONE){
										// Выполняем фиксацию параметров сервера
										if(this->_unit->server.commit(this->_id.eid)){
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), event::family_t::IPV4, this->_host, this->_unit->server.getAddress(static_cast <event::id_t> (this->_id.eid), event::address_t::IPV4));
											// Запускаем сервер
											this->_unit->server.start();
										}
									}
									// Выходим из функции
									return;
								}
							} break;
							// Для типа IPv6
							case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
								// Устанавливаем адрес хоста целевой текущей машины
								if(this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV6, this->_host)){
									// Если событие сервера не запущено, запускаем его
									if(awh_cast <unit::unit_t *> (&this->_unit->server)->status(this->_id.eid) == event::status_t::NONE){
										// Выполняем фиксацию параметров сервера
										if(this->_unit->server.commit(this->_id.eid)){
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), event::family_t::IPV6, this->_host, this->_unit->server.getAddress(static_cast <event::id_t> (this->_id.eid), event::address_t::IPV6));
											// Запускаем сервер
											this->_unit->server.start();
										}
									}
									// Выходим из функции
									return;
								}
							} break;
						}
						// Выполняем резолвинг хоста текущего сервера
						if(!this->_dns.client->resolve(this->_dns.id, this->_unit->server.family(this->_id.eid), this->_host, this->_dns.alive.load(std::memory_order_acquire))){
							// Создаём текст ошибки резолвинга хоста текущего сервера
							const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", this->_host.c_str());
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
								#endif
							// Выполняем функцию обратного вызова
							} else this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", static_cast <event::id_t> (this->_id.eid), event::error_t::NOT_FOUND, error, nullptr);
						}
					} break;
					// Если событие DNS-резолвера остановлено
					case static_cast <uint8_t> (event::status_t::DESTROYED):
						// Останавливаем сервер
						this->_unit->server.stop();
					break;
				}
			} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод инициализации запуска или остановки кластера
 *
 * @param pid   идентификатор процесса
 * @param event событие кластера
 *
 */
void awh::server::Socks5::eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем полученное событие кластера
		 */
		switch(static_cast <uint8_t> (event)){
			// Если событие представляет собой запуск процесса
			case static_cast <uint8_t> (unit::cluster_t::event_t::START): {
				// Если запущенный процесс является мастером кластера
				if(this->_unit->server.clusterFamily() == ::unit::cluster_t::family_t::MASTER){
					// Если порты для UDP серверов установлены корректно
					if((this->_udp.begin > 0) && (this->_udp.begin < this->_udp.end)){
						// Инициализируем пул портов для подключения к сети клиентов
						::__awh_port_allocator__.init(this->_udp.begin, this->_udp.end);
						// Если количество выделенных портов для UDP серверов больше нуля
						if(this->_udp.count > 0){
							// Получаем список идентификаторов всех воркеров в кластере
							const auto & workers = this->_unit->server.clusterWorkers();
							// Индекс для распределения UDP портов по воркерам
							uint16_t index = 0;
							// Устанавливаем размер флага для отправки данных
							const size_t length = sizeof(cluster_message_t);
							// Устанавливаем размер буфера полезной нагрузки для отправки
							::__awh_size__ = (length + sizeof(uint16_t));
							// Устанавливаем тип сообщения для отправки данных
							const cluster_message_t message = cluster_message_t::INTERNAL;
							// Копируем данные запроса в буфер полезной нагрузки
							::memcpy(&::__awh_buffer__[0], &message, length);
							/**
							 * Выполняем инициализацию указанного количества UDP серверов
							 */
							for(uint16_t i = 0; i < this->_udp.count; i++){
								// Выделяем порт для UDP-сервера
								auto port = ::__awh_port_allocator__.allocate();
								// Если порт выделен успешно, отправляем его воркеру для запуска UDP сервера
								if(port){
									// Получаем индекс воркера для которого выделяется порт UDP сервера
									index = (i % static_cast <uint16_t> (workers.size()));
									// Получаем идентификатор воркера для которого выделяется порт UDP сервера
									auto worker = ::next(workers.begin(), index);
									// Добавляем к буферу данных для отправки полезную нагрузку
									::memcpy(&::__awh_buffer__[length], &(* port), sizeof(uint16_t));
									// Отправляем выделенный порт UDP сервера, воркеру для запуска UDP сервера на этом порту
									this->_unit->server.clusterSend(* worker, ::__awh_buffer__, ::__awh_size__);
								// Если порты для UDP серверов закончились, выходим из цикла
								} else break;
							}
						}
					}
				}
			} break;
			// Если событие представляет собой остановку процесса
			case static_cast <uint8_t> (unit::cluster_t::event_t::STOP): {
				// Если запущенный процесс является дочерним процессом кластера
				if(this->_unit->server.clusterFamily() == ::unit::cluster_t::family_t::CHILDREN){
					// Если события UDP серверов активны
					if(!this->_udp.events.empty()){
						/**
						 * Удаляем все события UDP серверов, так как дочерний процесс, который их обслуживает, остановлен
						 */
						for(const auto & eid : this->_udp.events)
							// Удаляем событие UDP сервера
							this->_unit->server.destroy(eid);
						// Очищаем список активных UDP-серверов
						this->_udp.events.clear();
						// Очищаем множество активных UDP-серверов
						this->_udp.eventSet.clear();
					}
				}
			} break;
		}
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", pid, event);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pid, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки события получения сообщения от процесса кластера
 *
 * @param pid  идентификатор процесса
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 *
 */
void awh::server::Socks5::messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	// Если данные получены успешно и размер данных больше нуля
	if((data != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем тип сообщения для отправки данных
			cluster_message_t message = cluster_message_t::NONE;
			// Устанавливаем размер флага типа сообщения
			const size_t length = sizeof(cluster_message_t);
			// Извлекаем тип сообщения из данных полученного сообщения
			::memcpy(&message, data, length);
			/**
			 * Определяем тип полученного сообщения от процесса кластера
			 */
			switch(static_cast <uint8_t> (message)){
				// Если сообщение является внутренним сообщением для запуска UDP сервера
				case static_cast <uint8_t> (cluster_message_t::INTERNAL): {
					// Если сообщение получено от родительского процесса
					if(this->_unit->server.clusterFamily() == ::unit::cluster_t::family_t::CHILDREN){
						// Если размер данных полученного сообщения соответствует размеру порта
						if((size - length) == sizeof(uint16_t)){
							// Порт для запуска UDP сервера, который был выделен родительским процессом
							uint16_t port = 0;
							// Копируем данные сообщения в переменную порта
							::memcpy(&port, data + length, sizeof(port));
							/**
							 * Определяем семейство адресов для запуска UDP сервера
							 */
							switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
								// Если процесс работает с адресами IPv4
								case static_cast <uint8_t> (event::family_t::IPV4): {
									// Создаём UDP сервер для работы с клиентами
									const event::id_t uid = this->_unit->server.issue(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
									// Добавляем событие UDP сервера в множество событий
									this->_udp.eventSet.insert(uid);
									// Добавляем событие UDP сервера в список событий
									this->_udp.events.push_back(uid);
									// Устанавливаем опции события UDP сервера
									if(this->_unit->server.setOptions(uid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
										// Устанавливаем порт и адрес для UDP сервера
										if(this->_unit->server.setPort(uid, port) && this->_unit->server.setAddress(uid, event::address_t::IPV4, this->_udp.address.get())){
											// Выполняем фиксацию настроек события сервера
											if(this->_unit->server.commit(uid)){
												// Выполняем запуск события
												if(!this->_unit->server.launch(uid)){
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Записываем ошибку в лог
															this->_log->debug("Creating UDP server for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Записываем ошибку в лог
															this->_log->print("Creating UDP server for peer ID=%u is failed", log_t::flag_t::WARNING, uid);
														#endif
													}
												// Если сервер удачно запущен
												} else {
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", uid, this->_unit->server.getAddress(uid, event::address_t::IPV4), this->_unit->server.getPort(uid));
													// Выходим из функции
													return;
												}
											// Если фиксация настроек события сервера не выполнена
											} else {
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Записываем ошибку в лог
														this->_log->debug("UDP server parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Записываем ошибку в лог
														this->_log->print("UDP server parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, uid);
													#endif
												}
											}
										// Если установка порта и адреса удалённого сервера для подключения не выполнена
										} else {
											// Если функция обратного вызова не установлена
											if(!this->_callback.is("error")){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("Port and address of the UDP server were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("Port and address of the UDP server were not set correctly for node with ID=%u", log_t::flag_t::WARNING, uid);
												#endif
											}
										}
									// Если установка опций события не выполнена
									} else {
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Failed to configure UDP server events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Failed to configure UDP server events settings for node with ID=%u", log_t::flag_t::WARNING, uid);
											#endif
										}
									}
									// Удаляем UDP сервер, если его создание не выполнено
									this->_unit->server.destroy(uid);
									// Удаляем событие UDP сервера из списка событий
									::udpEventRemove(this->_udp.events, this->_udp.eventSet, uid);
								} break;
								// Если процесс работает с адресами IPv6
								case static_cast <uint8_t> (event::family_t::IPV6): {
									// Создаём UDP сервер для работы с клиентами
									const event::id_t uid = this->_unit->server.issue(event::family_t::IPV6, event::type_t::DATAGRAM, event::protocol_t::UDP);
									// Добавляем событие UDP сервера в множество событий
									this->_udp.eventSet.insert(uid);
									// Добавляем событие UDP сервера в список событий
									this->_udp.events.push_back(uid);
									// Устанавливаем опции события UDP сервера
									if(this->_unit->server.setOptions(uid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
										// Устанавливаем порт и адрес для UDP сервера
										if(this->_unit->server.setPort(uid, port) && this->_unit->server.setAddress(uid, event::address_t::IPV6, "::")){
											// Выполняем фиксацию настроек события сервера
											if(this->_unit->server.commit(uid)){
												// Выполняем запуск события
												if(!this->_unit->server.launch(uid)){
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Записываем ошибку в лог
															this->_log->debug("Creating UDP server for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Записываем ошибку в лог
															this->_log->print("Creating UDP server for peer ID=%u is failed", log_t::flag_t::WARNING, uid);
														#endif
													}
												// Если сервер удачно запущен
												} else {
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", uid, this->_unit->server.getAddress(uid, event::address_t::IPV6), this->_unit->server.getPort(uid));
													// Выходим из функции
													return;
												}
											// Если фиксация настроек события сервера не выполнена
											} else {
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Записываем ошибку в лог
														this->_log->debug("UDP server parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Записываем ошибку в лог
														this->_log->print("UDP server parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, uid);
													#endif
												}
											}
										// Если установка порта и адреса удалённого сервера для подключения не выполнена
										} else {
											// Если функция обратного вызова не установлена
											if(!this->_callback.is("error")){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("Port and address of the UDP server were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("Port and address of the UDP server were not set correctly for node with ID=%u", log_t::flag_t::WARNING, uid);
												#endif
											}
										}
									// Если установка опций события не выполнена
									} else {
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Failed to configure UDP server events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::WARNING, uid);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Failed to configure UDP server events settings for node with ID=%u", log_t::flag_t::WARNING, uid);
											#endif
										}
									}
									// Удаляем UDP сервер, если его создание не выполнено
									this->_unit->server.destroy(uid);
									// Удаляем событие UDP сервера из списка событий
									::udpEventRemove(this->_udp.events, this->_udp.eventSet, uid);
								} break;
							}
						}
					}
				} break;
				// Если сообщение является внешним сообщением для обработки данных от клиента
				case static_cast <uint8_t> (cluster_message_t::EXTERNAL):
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", pid, data + length, size - length);
				break;
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pid, data, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки событий подключения клиента к удалённому серверу
 *
 * @param eid идентификатор клиента
 * @param ok  результат подключения
 *
 */
void awh::server::Socks5::connectClient(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->server.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
			auto i = this->_clients.find(eid);
			// Если идентификатор клиента найден в списке
			if(i != this->_clients.end()){
				// Ищем идентификатор пира в списке сопоставления идентификаторов пиров с удалёнными клиентами
				auto j = this->_peers.find(i->second);
				// Если идентификатор пира найден в списке
				if(j != this->_peers.end()){
					// Если подключение к удалённому серверу не выполнено
					if(!ok){
						// Устанавливаем статус недоступности хоста
						j->second.ctx.status = proto::socks5_t::status_t::UNAVHOST;
						// Создаём объект параметров подключения для ответа об ошибке
						j->second.ctx.host = make_unique <net::attr_net_t> ();
						// Устанавливаем тип адреса как IPv4
						j->second.ctx.host->type = net::type_t::IPV4;
						// Создаём новый объект адреса IPv4 для ответа с адресом 0.0.0.0
						awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
						// Отправляем ответ прокси-клиенту и закрываем пира
						this->sendReply(i->second, j->second.ctx, true);
						// Если исходящий клиент был создан
						if(j->second.eid != 0){
							// Удаляем исходящего клиента
							this->_client.destroy(j->second.eid);
							// Обнуляем идентификатор исходящего клиента
							j->second.eid = 0;
						}
						// Удаляем связь между клиентом и пиром
						this->_clients.erase(i);
						// Выходим из функции
						return;
					}
					// Устанавливаем статус успешного выполнения команды
					j->second.ctx.status = proto::socks5_t::status_t::SUCCESS;
					// Размер буфера данных
					size_t size = 0;
					// Буфер данных ответа
					uint8_t * buffer = nullptr;
					// Создаём объект параметров подключения для хоста сервера
					j->second.ctx.host = make_unique <net::attr_net_t> ();
					/**
					 * Определяем семейство адресов для хоста сервера
					 */
					switch(static_cast <uint8_t> (this->_client.family(eid))){
						// Если семейство адресов соответствует IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Устанавливаем тип адреса как IPv4
							j->second.ctx.host->type = net::type_t::IPV4;
							// Устанавливаем IP-адрес хоста сервера
							this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip);
						} break;
						// Если семейство адресов соответствует IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Устанавливаем тип адреса как IPv6
							j->second.ctx.host->type = net::type_t::IPV6;
							// Создаём новый объект адреса клиента IPv6
							awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							// Устанавливаем IP-адрес хоста сервера
							this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip);
						} break;
					}
					// Устанавливаем порт хоста сервера
					awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->port = this->_client.getSourcePort(eid);
					// Если извлечение буфера данных ответа выполнено успешно
					if(this->_socks5.buffer(&buffer, size, j->second.ctx)){
						// Если отправка ответа прокси-клиенту не выполнена
						if(this->_unit->server.send(i->second, buffer, size) != size){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
								#endif
							}
							// Удаляем подключённого пира
							this->_unit->server.destroy(i->second);
						// Если отправка ответа прокси-клиенту выполнена успешно
						} else {
							// Если статус ответа от прокси-сервера соответствует успешному выполнению команды
							if(j->second.ctx.status == proto::socks5_t::status_t::SUCCESS){
								// Выполняем объединение событий пира и принадлежащего ему клиента
								if(!this->_unit->server.splice(i->second, j->second.eid) || !this->_unit->server.splice(j->second.eid, i->second)){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::WARNING, i->second);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, i->second);
										#endif
									}
									// Выполняем поиск пира, которому принадлежит идентификатор
									auto j = this->_peers.find(i->second);
									// Если пир для этого идентификатора найден
									if(j != this->_peers.end())
										// Удаляем подключённого пира
										this->_unit->server.destroy(i->second);
								// Если объединение событий пира и принадлежащего ему клиента выполнено успешно
								} else {
									// Устанавливаем статус успешного выполнения команды
									j->second.ctx.state = proto::socks5_t::state_t::COMPLETED;
									// Очищаем буфер накопления SOCKS5-кадров
									j->second.rx.clear();
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", static_cast <event::id_t> (this->_id.eid), i->second, true);
								}
							// Если статус ответа от прокси-сервера как запрещённый, так и не поддерживаемый
							} else j->second.ctx.state = proto::socks5_t::state_t::BROKEN;
						}
					}
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор события клиента
 * @param status новый статус события
 *
 */
void awh::server::Socks5::statusClient(const event::id_t eid, const event::status_t status) noexcept {
	// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
	auto i = this->_clients.find(eid);
	// Если идентификатор клиента найден в списке
	if(i != this->_clients.end()){
		// Извлекаем идентификатор пира, которому принадлежит клиент
		const event::id_t eid = i->second;
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (status)){
			// Если статус уничтожения
			case static_cast <uint8_t> (event::status_t::DESTROYED):
				// Удаляем подключённого пира
				this->_unit->server.destroy(eid);
			break;
		}
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t, void *)> ("state", eid, status, nullptr);
	}
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 *
 */
void awh::server::Socks5::readClient(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
	auto i = this->_clients.find(eid);
	// Если идентификатор клиента найден в списке
	if(i != this->_clients.end()){
		// Контекст UDP-заголовка для формирования ответа
		proto::socks5_t::udp_head_t udp{};
		// Если хост для ответа на запрос клиента не инициализирован, или если тип адреса хоста для ответа на запрос клиента соответствует FQDN
		if((udp.host == nullptr) || (udp.host->type == net::type_t::FQDN))
			// Выполняем инициализацию объекта хоста
			udp.host = make_unique <net::attr_net_t> ();
		/**
		 * Определяем тип адреса хоста для подключения
		 */
		switch(static_cast <uint8_t> (this->_unit->server.family(eid))){
			// Если тип адреса соответствует IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
				// Устанавливаем тип параметров подключения для идентификатора события клиента
				udp.host->type = net::type_t::IPV4;
			break;
			// Если тип адреса соответствует IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если тип адреса хоста для ответа на запрос клиента не соответствует IPv6
				if(udp.host->type != net::type_t::IPV6)
					// Создаём новый объект адреса клиента IPv6
					awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
				// Устанавливаем тип параметров подключения для идентификатора события клиента
				udp.host->type = net::type_t::IPV6;
			} break;
		}
		// Устанавливаем порт подключённого клиента для идентификатора события клиента
		awh_cast <net::attr_net_t *> (udp.host.get())->port = this->_client.getTargetPort(eid);
		// Извлекаем IP-адрес клиента для идентификатора события клиента
		this->_client.getTarget(eid, awh_cast <net::attr_net_t *> (udp.host.get())->ip);
		// Размер буфера данных
		size_t length = 0;
		// Буфер данных запроса
		uint8_t * data = nullptr;
		// Если извлечение буфера данных запроса выполнено успешно
		if(this->_socks5.buffer(&data, length, udp)){
			/**
			 * Определяем тип данных сесии клиента, работающего через прокси
			 */
			switch(static_cast <uint8_t> (udp.host->type)){
				// Если тип данных соответствует IPv4
				case static_cast <uint8_t> (net::type_t::IPV4):
					// Устанавливаем размер буфера полезной нагрузки для отправки
					::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
				break;
				// Если тип данных соответствует IPv6
				case static_cast <uint8_t> (net::type_t::IPV6):
					// Устанавливаем размер буфера полезной нагрузки для отправки
					::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV6_PAYLOAD_SIZE));
				break;
			}
			// Если размер буфера полезной нагрузки достаточно для отправки всех данных
			if(::__awh_size__ == (size + length)){
				// Копируем данные запроса в буфер полезной нагрузки
				::memcpy(&::__awh_buffer__[0], data, length);
				// Добавляем к буферу данных для отправки полезную нагрузку
				::memcpy(&::__awh_buffer__[length], buffer, size);
				// Выполняем отправку данных серверу
				this->_unit->server.send(i->second, ::__awh_buffer__, ::__awh_size__);
			// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
				#endif
			}
		// Если извлечение буфера данных запроса не выполнено
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод получения события ошибок
 *
 * @param eid     идентификатор события
 * @param error   код ошибки
 * @param message сообщение об ошибке
 *
 */
void awh::server::Socks5::errorClient(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("error");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid)){
		// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
		auto i = this->_clients.find(eid);
		// Если идентификатор клиента найден в списке
		if(i != this->_clients.end())
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> (fid, i->second, error, message, nullptr);
	}
}
/**
 * @brief Метод обработки событий истечения таймаута клиента
 *
 * @param eid    идентификатор клиента
 * @param action тип действия для истекшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @return       нужно ли завершить клиента после истечения таймаута
 *
 */
bool awh::server::Socks5::timeoutClient(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->server.working() : false)){
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("timeout");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid)){
			// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
			auto i = this->_clients.find(eid);
			// Если идентификатор клиента найден в списке
			if(i != this->_clients.end())
				// Выполняем функцию обратного вызова
				return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t, void *)> (fid, i->second, action, delay, nullptr);
		}
	}
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 *
 */
void awh::server::Socks5::accept(const event::id_t eid, const event::id_t cid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
		if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->server.working() : false)){
			// Если идентификатор сервера соответствует идентификатору socks5-сервера
			if(eid == this->_id.eid){
				// Добавляем пира в список активных пиров
				auto ret = this->_peers.emplace(cid, peer_t{});
				// Если пир был добавлен успешно
				if(ret.second){
					// Устанавливаем опции события
					if(this->_unit->server.setOptions(cid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
						// Размер буфера данных
						size_t size = 0;
						// Буфер данных запроса
						uint8_t * buffer = nullptr;
						// Если извлечение буфера данных запроса выполнено успешно
						if(this->_socks5.buffer(&buffer, size, ret.first->second.ctx)){
							// Если отправка запроса на прокси-сервер не выполнена
							if(this->_unit->server.send(cid, buffer, size) != size){
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, cid), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
									#endif
								}
							}
						}
					}
				}
			// Если идентификатор сервера соответствует идентификатору одного из поддерживаемых UDP-серверов
			} else if(::udpEventHas(this->_udp.eventSet, eid)) {
				// Создаём объект параметров подключения для идентификатора события клиента
				unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
				/**
				 * Определяем тип адреса хоста для подключения
				 */
				switch(static_cast <uint8_t> (this->_unit->server.family(cid))){
					// Если тип адреса соответствует IPv4
					case static_cast <uint8_t> (event::family_t::IPV4): {
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						attr->type = net::type_t::IPV4;
						// Извлекаем IP-адрес клиента для идентификатора события клиента
						this->_unit->server.getAddress(cid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (attr.get())->ip);
					} break;
					// Если тип адреса соответствует IPv6
					case static_cast <uint8_t> (event::family_t::IPV6): {
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						attr->type = net::type_t::IPV6;
						// Извлекаем IP-адрес клиента для идентификатора события клиента
						this->_unit->server.getAddress(cid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (attr.get())->ip);
					} break;
				}
				// Устанавливаем порт подключённого клиента для идентификатора события клиента
				awh_cast <net::attr_net_t *> (attr.get())->port = this->_unit->server.getPort(cid);
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = origin_t().from(attr.get());
				// Выполняем поиск идентификатора конечной точки в списке активных сессий
				auto i = this->_sessions.find(endpoint);
				// Если идентификатор конечной точки найден в списке активных сессий
				if(i != this->_sessions.end()){
					// Запоминаем идентификатор пира для идентификатора события клиента
					i->second.second = cid;
					// Сопоставляем идентификатор события клиента с идентификатором конечной точки
					this->_mapping.emplace(i->second.first, i->first);
				// Если идентификатор конечной точки не найден в списке активных сессий
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Attempt to illegally connect to a UDP server for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, cid), log_t::flag_t::WARNING, eid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Attempt to illegally connect to a UDP server for node with ID=%u", log_t::flag_t::WARNING, eid);
					#endif
					// Удаляем подключённого пира
					this->_unit->server.destroy(cid);
					// Выходим из функции
					return;
				}
				// Добавляем пира в список активных пиров
				auto ret = this->_peers.emplace(cid, peer_t{});
				// Если пир был добавлен успешно
				if(ret.second){
					// Устанавливаем статус успешного выполнения команды
					ret.first->second.ctx.state = proto::socks5_t::state_t::COMPLETED;
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::id_t)> ("accept", eid, cid);
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, cid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки событий изменения состояния сервера
 *
 * @param eid    идентификатор клиента
 * @param status новый статус сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::server::Socks5::state(const event::id_t eid, const event::status_t status, void * ctx) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
		if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->server.working() : false)){
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t, void *)> ("state", eid, status, ctx);
			// Если статус сервера изменился на "уничтожен"
			if(status == event::status_t::DESTROYED){
				// Если производится завершение работы текущего сервера или одного из поддерживаемых UDP-серверов
				if(eid == this->_id.eid)
					// Обнуляем идентификатор сервера
					this->_id.eid = 0;
				// Если производится завершение работы клиента подключенного к одному из серверов
				else {
					// Выполняем поиск пира, которому принадлежит идентификатор
					auto i = this->_peers.find(eid);
					// Если пир для этого идентификатора найден
					if(i != this->_peers.end()){
						// Выполняем поиск идентификатора клиента принадлежащего этому пиру
						auto j = this->_clients.find(i->second.eid);
						// Если идентификатор клиента найден в списке
						if(j != this->_clients.end()){
							// Удаляем подключённого клиента
							this->_client.destroy(j->first);
							// Удаляем клиента из списка активных клиентов
							this->_clients.erase(j);
						}
						// Удаляем пира из списка активных пиров
						this->_peers.erase(i);
					}
					/**
					 * Если завершён UDP data-пир, сбрасываем ссылку в активной сессии
					 */
					for(auto s = this->_sessions.begin(); s != this->_sessions.end(); ++s){
						// Если идентификатор завершённого пира соответствует data-пиру сессии
						if(s->second.second == eid){
							// Сбрасываем идентификатор data-пира, control-соединение остаётся активным
							s->second.second = 0;
							// Выходим из цикла
							break;
						}
					}
					// Выполняем поиск идентификатора пира, для удаления принадлежащих ему UDP-сессий
					auto j = this->_mapping.find(eid);
					// Если идентификатор пира найден в списке
					if(j != this->_mapping.end()){
						// Выполняем поиск идентификатора пира в списке активных сессий
						auto i = this->_sessions.find(j->second);
						// Если идентификатор пира найден в списке активных сессий
						if(i != this->_sessions.end()){
							// Выполняем поиск пира, которому принадлежит идентификатор
							auto j = this->_peers.find(i->second.second);
							// Если пир для этого идентификатора найден
							if(j != this->_peers.end()){
								// Выполняем поиск идентификатора клиента принадлежащего этому пиру
								auto k = this->_clients.find(j->second.eid);
								// Если идентификатор клиента найден в списке
								if(k != this->_clients.end()){
									// Выполняем поиск кэша для идентификатора пира
									auto l = ::__awh_cache__.find(k->first);
									// Если кэш для этого идентификатора найден
									if(l != ::__awh_cache__.end())
										// Удаляем кэш для идентификатора пира
										::__awh_cache__.erase(l);
									// Удаляем подключённого клиента
									this->_client.destroy(k->first);
									// Удаляем клиента из списка активных клиентов
									this->_clients.erase(k);
								}
								// Выполняем поиск идентификатора резолвера, с которым связан этот пир
								auto m = this->_resolves.find(j->second.did);
								// Если идентификатор резолвера найден в списке
								if(m != this->_resolves.end())
									// Удаляем резолвер из списка активных резолверов
									this->_resolves.erase(m);
								// Удаляем пира из списка активных пиров
								this->_peers.erase(j);
							}
							// Запоминаем идентификатор пира для идентификатора события клиента
							const event::id_t eid = i->second.second;
							// Удаляем подключённого пира
							this->_unit->server.destroy(eid);
							// Удаляем сессию из списка активных сессий
							this->_sessions.erase(i);
						}
						// Удаляем всю UDP-сессию, принадлежащую этому пиру
						this->_mapping.erase(j);
					}
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных сервера
 * @param size   размер данных сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::server::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size, [[maybe_unused]] void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->server.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск пира, которому принадлежит идентификатор
			auto i = this->_peers.find(eid);
			// Если пир для этого идентификатора найден
			if(i != this->_peers.end()){
				// Если текущее состояние соответствует завершённому состоянию
				if(i->second.ctx.state == proto::socks5_t::state_t::COMPLETED){
					// Если парсинг данных от прокси-сервера выполнен успешно
					if(this->_socks5.parse(buffer, size, i->second.udp)){
						// Если хост клиента которому адресован UDP пакет установлен
						if(i->second.udp.host != nullptr){
							/**
							 * Устанавливаем метку начала проверки инициализации клиента
							 */
							Begin:
							// Если клиент для выполнения запросов ещё не инициализирован
							if(i->second.eid == 0){
								// Получаем семейство адресов для подключения к удалённому серверу
								const event::family_t family = this->_unit->server.family(eid);
								// Выполняем создание клиента для подключения к удалённому серверу
								i->second.eid = this->_client.issue(family, event::type_t::DATAGRAM, event::protocol_t::UDP);
								// Устанавливаем опции события
								if(this->_client.setOptions(i->second.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
									// Извлекаем сетевой интерфейс для подключения к удалённому серверу
									const string & iface = this->_unit->server.getIface(eid);
									// Устанавливаем интерфейс для подключения к удалённому серверу
									if(this->_client.setIface(i->second.eid, iface)){
										/**
										 * Определяем тип данных сесии клиента, работающего через прокси
										 */
										switch(static_cast <uint8_t> (i->second.udp.host->type)){
											// Если тип данных соответствует FQDN
											case static_cast <uint8_t> (net::type_t::FQDN): {
												// Вычисляем длину полезной нагрузки UDP с защитой от переполнения приёмника кэша фиксированного размера
												const size_t payload = ::min((size > i->second.udp.size) ? (size - i->second.udp.size) : static_cast <size_t> (0), sizeof(cache_t::buffer));
												// Если фактическая длина полезной нагрузки превышает размер приёмника кэша - фиксируем в логах (данные усекаются)
												if((size > i->second.udp.size) && ((size - i->second.udp.size) > sizeof(cache_t::buffer))){
													// Если включён режим отладки
													#if DEBUG_MODE
														// Записываем предупреждение в лог
														this->_log->debug("UDP relay payload size %zu exceeds cache buffer %zu, data truncated", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, size - i->second.udp.size, sizeof(cache_t::buffer));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Записываем ошибку в лог
														this->_log->print("UDP relay payload size %zu exceeds cache buffer %zu, data truncated", log_t::flag_t::WARNING, size - i->second.udp.size, sizeof(cache_t::buffer));
													#endif
												}
												// Выполняем поиск кэша для идентификатора пира
												auto j = ::__awh_cache__.find(i->second.eid);
												// Если кэш для этого идентификатора найден
												if(j != ::__awh_cache__.end()){
													// Устанавливаем идентификатор пира для кэша
													j->second.eid = eid;
													// Устанавливаем размер буфера данных в кэше
													j->second.size = payload;
													// Копируем данные запроса в кэш
													::memcpy(j->second.buffer, buffer + i->second.udp.size, payload);
													// Устанавливаем порт удалённого сервера для подключения
													j->second.attr->port = awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->port;
													// Устанавливаем доменное имя хоста для подключения
													j->second.domain = awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->domain;
												// Если кэш для этого идентификатора не найден
												} else {
													// Добавляем кэш для идентификатора пира
													auto ret = ::__awh_cache__.emplace(i->second.eid, cache_t{});
													// Устанавливаем идентификатор пира для кэша
													ret.first->second.eid = eid;
													// Устанавливаем размер буфера данных в кэше
													ret.first->second.size = payload;
													// Копируем данные запроса в кэш
													::memcpy(ret.first->second.buffer, buffer + i->second.udp.size, payload);
													// Создаём новый объект атрибутов сети для кэша
													ret.first->second.attr = make_unique <net::attr_net_t> ();
													// Устанавливаем порт удалённого сервера для подключения
													ret.first->second.attr->port = awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->port;
													// Устанавливаем доменное имя хоста для подключения из данных запроса в кэше
													ret.first->second.domain = awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->domain;
												}
												// Создаём идентификатор резолвера для текущего пира
												i->second.did = this->_dns.client->issue();
												// Выполняем добавление связи DNS-резолвера и идентификатора пира
												auto ret = this->_resolves.emplace(i->second.did, eid);
												// Выполняем резолвинг хоста текущего сервера
												if(!this->_dns.client->resolve(ret.first->first, family, awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->domain, this->_dns.alive.load(std::memory_order_acquire))){
													// Удаляем связь DNS-резолвера и идентификатора пира
													this->dropResolve(ret.first->first);
													// Обнуляем идентификатор DNS-резолвера
													i->second.did = 0;
													// Удаляем исходящего клиента
													this->_client.destroy(i->second.eid);
													// Удаляем кэш исходящего клиента
													::__awh_cache__.erase(i->second.eid);
													// Обнуляем идентификатор исходящего клиента
													i->second.eid = 0;
													// Создаём текст ошибки резолвинга хоста текущего сервера
													const string error = this->_fmk->format("It was not possible to obtain an IP address for the remote host \"%s\"", awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->domain.c_str());
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Записываем ошибку в лог
															this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Записываем ошибку в лог
															this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
														#endif
													// Выполняем функцию обратного вызова
													} else this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", eid, event::error_t::NOT_FOUND, error, nullptr);
												// Если резолвинг хоста не выполнен, выходим
												} else return;
											} break;
											// Если тип данных соответствует IPv6
											case static_cast <uint8_t> (net::type_t::IPV6):
											// Если тип данных соответствует IPv4
											case static_cast <uint8_t> (net::type_t::IPV4): {
												// Устанавливаем порт и адрес удалённого сервера для подключения
												if(this->_client.setTarget(i->second.eid, awh_cast <net::attr_net_t *> (i->second.udp.host.get())->ip.get()) &&
												   this->_client.setTargetPort(i->second.eid, awh_cast <net::attr_net_t *> (i->second.udp.host.get())->port)){
													// Выполняем получение идентификатора функции обратного вызова
													const callback_t::id_t fid = this->_callback.id("ready");
													// Если функция обратного вызова установлена
													if(this->_callback.is(fid)){
														// Получаем IP-адрес для подключения к удалённому серверу
														const string & address = this->_client.getTarget(i->second.eid);
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> (fid, eid, family, address, address);
													}
													// Выполняем фиксацию настроек события сервера
													if(this->_client.commit(i->second.eid)){
														// Выполняем запуск события
														if(!this->_client.launch(i->second.eid)){
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Записываем ошибку в лог
																	this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Записываем ошибку в лог
																	this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, eid);
																#endif
															}
														// Если резолвинг хоста не выполнен
														} else {
															// Получаем опции события для идентификатора события клиента
															uint16_t options = this->_client.getOptions(i->second.eid);
															// Если опция AUTO_RECONNECT установлена
															if(options & event::options::AUTO_RECONNECT){
																// Сбрасываем опцию AUTO_RECONNECT для идентификатора события клиента
																options &= ~event::options::AUTO_RECONNECT;
																// Устанавливаем опции события для идентификатора события клиента
																this->_client.setOptions(i->second.eid, options);
															}
															// Добавляем связь между клиентом и пиром которому он принадлежит
															this->_clients.emplace(i->second.eid, eid);
															// Выполняем отправку данных клиенту
															this->_client.send(i->second.eid, buffer + i->second.udp.size, size - i->second.udp.size);
															// Выходим из функции
															return;
														}
													// Если фиксация настроек события сервера не выполнена
													} else {
														// Если функция обратного вызова не установлена
														if(!this->_callback.is("error")){
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Записываем ошибку в лог
																this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Записываем ошибку в лог
																this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, eid);
															#endif
														}
													}
												// Если установка порта и адреса удалённого сервера для подключения не выполнена
												} else {
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Записываем ошибку в лог
															this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Записываем ошибку в лог
															this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, eid);
														#endif
													}
												}
											} break;
										}
									// Если установка интерфейса для подключения к удалённому серверу не выполнена
									} else {
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, iface.c_str(), eid);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, iface.c_str(), eid);
											#endif
										}
									}
								// Если установка опций события не выполнена
								} else {
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, eid);
										#endif
									}
								}
								// Удаляем клиента принадлежащего пиру
								this->_client.destroy(i->second.eid);
							// Если клиент для выполнения запросов уже инициализирован
							} else {
								/**
								 * Определяем тип данных сесии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (i->second.udp.host->type)){
									// Если тип данных соответствует FQDN
									case static_cast <uint8_t> (net::type_t::FQDN): {
										// Если порты хоста сервера и удалённого сервера для подключения соответствуют друг другу
										if(awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->port == this->_client.getTargetPort(i->second.eid)){
											// Выполняем поиск кэша для идентификатора пира
											auto j = ::__awh_cache__.find(i->second.eid);
											// Если кэш для этого идентификатора найден
											if(j != ::__awh_cache__.end()){
												// Если доменные имена хоста сервера и удалённого сервера для подключения соответствуют друг другу
												if(this->_fmk->compare(j->second.domain, awh_cast <net::attr_fqdn_t *> (i->second.udp.host.get())->domain)){
													/**
													 * Если доменное имя уже разрезолвено, выполняем запрос,
													 * если новый запрос пришёл раньше, просто дропаем пакет.
													 */
													if(j->second.attr != nullptr)
														// Выполняем отправку данных клиенту
														this->_client.send(i->second.eid, buffer + i->second.udp.size, size - i->second.udp.size);
													// Выходим из функции
													return;
												}
											}
										}
										// Удаляем клиента принадлежащего пиру
										this->_client.destroy(i->second.eid);
										// Устанавливаем идентификатор клиента для выполнения запросов
										i->second.eid = 0;
										// Выполняем переход к метке начала проверки инициализации клиента
										goto Begin;
									}
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6):
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4): {
										/**
										 * Определяем семейство адресов для хоста сервера
										 */
										switch(static_cast <uint8_t> (this->_unit->server.family(eid))){
											// Если семейство адресов соответствует IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Если порты хоста сервера и удалённого сервера для подключения соответствуют друг другу
												if(awh_cast <net::attr_net_t *> (i->second.udp.host.get())->port == this->_client.getTargetPort(i->second.eid)){
													// Создаём новый объект адреса клиента IPv4
													unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
													// Извлекаем IP-адрес удалённого сервера для подключения
													this->_client.getTarget(i->second.eid, ip);
													// Если IP-адрес удалённого сервера для подключения соответствует IP-адресу хоста сервера для выполнения запроса
													if(awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (i->second.udp.host.get())->ip.get())->address == awh_cast <net::addr_net_ipv4_t *> (ip.get())->address){
														// Выполняем отправку данных клиенту
														this->_client.send(i->second.eid, buffer + i->second.udp.size, size - i->second.udp.size);
														// Выходим из функции
														return;
													}
												}
												// Удаляем клиента принадлежащего пиру
												this->_client.destroy(i->second.eid);
												// Устанавливаем идентификатор клиента для выполнения запросов
												i->second.eid = 0;
												// Выполняем переход к метке начала проверки инициализации клиента
												goto Begin;
											}
											// Если семейство адресов соответствует IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Если порты хоста сервера и удалённого сервера для подключения соответствуют друг другу
												if(awh_cast <net::attr_net_t *> (i->second.udp.host.get())->port == this->_client.getTargetPort(i->second.eid)){
													// Создаём новый объект адреса клиента IPv6
													unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
													// Извлекаем IP-адрес удалённого сервера для подключения
													this->_client.getTarget(i->second.eid, ip);
													// Если IP-адрес удалённого сервера для подключения соответствует IP-адресу хоста сервера для выполнения запроса
													if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (i->second.udp.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], 16) == 0){
														// Выполняем отправку данных клиенту
														this->_client.send(i->second.eid, buffer + i->second.udp.size, size - i->second.udp.size);
														// Выходим из функции
														return;
													}
												}
												// Удаляем клиента принадлежащего пиру
												this->_client.destroy(i->second.eid);
												// Устанавливаем идентификатор клиента для выполнения запросов
												i->second.eid = 0;
												// Выполняем переход к метке начала проверки инициализации клиента
												goto Begin;
											}
										}
									} break;
								}
							}
							// Выходим из функции
							return;
						}
					}
					// Создаём текст ошибки резолвинга доменного имени
					const string error = "Client for whom the UDP packet was received was not found";
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					// Выполняем функцию обратного вызова
					} else this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", eid, event::error_t::NOT_FOUND, error, nullptr);
				// Если текущее состояние находится ещё в процессе общения с SOCKS5-прокси-клиентом
				} else {
					// Добавляем данные в буфер накопления SOCKS5-кадров
					i->second.rx.insert(i->second.rx.end(), buffer, buffer + size);
					/**
					 * Разбираем все полные кадры, накопленные в буфере
					 */
					while(i->second.ctx.state != proto::socks5_t::state_t::COMPLETED){
						// Если размер буфера превышает допустимый
						if(i->second.rx.size() > proto::socks5_t::SOCKS5_RX_MAX_FRAME){
							// Удаляем подключённого пира
							this->_unit->server.destroy(eid);
							// Выходим из функции
							return;
						}
						// Определяем полный размер SOCKS5-кадра
						const size_t frame = this->_socks5.frameSize(i->second.ctx.state, i->second.rx.data(), i->second.rx.size());
						// Если кадр ещё неполный
						if(frame == 0)
							// Выходим и ожидаем продолжение кадра
							return;
						// Если кадр некорректный
						if(frame == SIZE_MAX){
							// Удаляем подключённого пира
							this->_unit->server.destroy(eid);
							// Выходим из функции
							return;
						}
						// Если парсинг данных от прокси-клиента выполнен успешно
						if(this->_socks5.parse(i->second.rx.data(), frame, i->second.ctx)){
							// Удаляем обработанный кадр из буфера
							i->second.rx.erase(i->second.rx.begin(), i->second.rx.begin() + frame);
							/**
							 * Определяем состояние парсинга данных от прокси-клиента
							 */
							switch(static_cast <uint8_t> (i->second.ctx.state)){
								// Если текущее состояние соответствует ошибке работе с прокси-сервером
								case static_cast <uint8_t> (proto::socks5_t::state_t::BROKEN): {
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, this->_socks5.statusMessage(i->second.ctx.status).c_str());
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socks5.statusMessage(i->second.ctx.status).c_str());
										#endif
									// Выполняем функцию обратного вызова
									} else this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", eid, event::error_t::CONNECTION_FAIL, this->_socks5.statusMessage(i->second.ctx.status), nullptr);
									// Удаляем подключённого пира
									this->_unit->server.destroy(eid);
									// Выходим из функции
									return;
								} break;
								// Если текущее состояние соответствует выполненному рукопожатию
								case static_cast <uint8_t> (proto::socks5_t::state_t::HANDSHAKE): {
									/**
									 * Определяем команду, которую выполняет прокси-клиент
									 */
									switch(static_cast <uint8_t> (i->second.ctx.command)){
										// Если команда соответствует CONNECT
										case static_cast <uint8_t> (proto::socks5_t::command_t::CONNECT): {
											/**
											 * Определяем тип адреса хоста для подключения
											 */
											switch(static_cast <uint8_t> (i->second.ctx.host->type)){
												// Если тип адреса соответствует FQDN
												case static_cast <uint8_t> (net::type_t::FQDN): {
													// Если DNS-резолвер не установлен или не находится в рабочем состоянии
													if((this->_dns.client == nullptr) || !this->_dns.client->working())
														// Устанавливаем статус недоступности хоста
														i->second.ctx.status = proto::socks5_t::status_t::UNAVHOST;
													// Если DNS-резолвер установлен и находится в рабочем состоянии
													else {
														// Создаём идентификатор резолвера для текущего пира
														i->second.did = this->_dns.client->issue();
														// Выполняем добавление связи DNS-резолвера и идентификатора пира
														auto ret = this->_resolves.emplace(i->second.did, eid);
														// Выполняем резолвинг хоста текущего сервера
														if(!this->_dns.client->resolve(ret.first->first, this->_unit->server.family(eid), awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->domain, this->_dns.alive.load(std::memory_order_acquire))){
															// Удаляем связь DNS-резолвера и идентификатора пира
															this->dropResolve(ret.first->first);
															// Обнуляем идентификатор DNS-резолвера
															i->second.did = 0;
															// Устанавливаем статус недоступности адреса
															i->second.ctx.status = proto::socks5_t::status_t::NOADDR;
															// Создаём текст ошибки резолвинга хоста текущего сервера
															const string error = this->_fmk->format("It was not possible to obtain an IP address for the remote host \"%s\"", awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->domain.c_str());
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Записываем ошибку в лог
																	this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Записываем ошибку в лог
																	this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
																#endif
															// Выполняем функцию обратного вызова
															} else this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", eid, event::error_t::NOT_FOUND, error, nullptr);
														// Если резолвинг хоста не выполнен, выходим
														} else return;
													}
												} break;
												// Если тип адреса соответствует IPv4
												case static_cast <uint8_t> (net::type_t::IPV4):
												// Если тип адреса соответствует IPv6
												case static_cast <uint8_t> (net::type_t::IPV6): {
													// Получаем семейство адресов для подключения к удалённому серверу
													const event::family_t family = this->_unit->server.family(eid);
													// Выполняем создание клиента для подключения к удалённому серверу
													i->second.eid = this->_client.issue(family, event::type_t::STREAM, event::protocol_t::TCP);
													// Устанавливаем опции события
													if(this->_client.setOptions(i->second.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
														// Извлекаем сетевой интерфейс для подключения к удалённому серверу
														const string & iface = this->_unit->server.getIface(eid);
														// Устанавливаем интерфейс для подключения к удалённому серверу
														if(this->_client.setIface(i->second.eid, iface)){
															// Устанавливаем порт и адрес удалённого сервера для подключения
															if(this->_client.setTarget(i->second.eid, awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get()) &&
															   this->_client.setTargetPort(i->second.eid, awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port)){
																// Выполняем получение идентификатора функции обратного вызова
																const callback_t::id_t fid = this->_callback.id("ready");
																// Если функция обратного вызова установлена
																if(this->_callback.is(fid)){
																	// Получаем IP-адрес для подключения к удалённому серверу
																	const string & address = this->_client.getTarget(i->second.eid);
																	// Выполняем функцию обратного вызова
																	this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> (fid, eid, family, address, address);
																}
																// Выполняем функцию обратного вызова
																this->_callback.call <void (const event::id_t, const event::id_t)> ("accept", static_cast <event::id_t> (this->_id.eid), eid);
																// Выполняем фиксацию настроек события сервера
																if(this->_client.commit(i->second.eid)){
																	// Если подключение к серверу прошло успешно
																	if(this->_client.connect(i->second.eid)){
																		// Выполняем запуск события
																		if(!this->_client.launch(i->second.eid)){
																			// Если функция обратного вызова не установлена
																			if(!this->_callback.is("error")){
																				/**
																				 * Если включён режим отладки
																				 */
																				#if DEBUG_MODE
																					// Записываем ошибку в лог
																					this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																				/**
																				 * Если режим отладки не включён
																				 */
																				#else
																					// Записываем ошибку в лог
																					this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, eid);
																				#endif
																			}
																		// Если резолвинг хоста не выполнен
																		} else {
																			// Получаем опции события для идентификатора события клиента
																			uint16_t options = this->_client.getOptions(i->second.eid);
																			// Если опция AUTO_RECONNECT установлена
																			if(options & event::options::AUTO_RECONNECT){
																				// Сбрасываем опцию AUTO_RECONNECT для идентификатора события клиента
																				options &= ~event::options::AUTO_RECONNECT;
																				// Устанавливаем опции события для идентификатора события клиента
																				this->_client.setOptions(i->second.eid, options);
																			}
																			// Добавляем связь между клиентом и пиром которому он принадлежит
																			this->_clients.emplace(i->second.eid, eid);
																			// Выходим из функции
																			return;
																		}
																	// Если подключение к серверу не прошло успешно
																	} else {
																		// Если функция обратного вызова не установлена
																		if(!this->_callback.is("error")){
																			/**
																			 * Если включён режим отладки
																			 */
																			#if DEBUG_MODE
																				// Записываем ошибку в лог
																				this->_log->debug("Connection to the server \"%s:%u\" is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, this->_client.getTarget(i->second.eid).c_str(), awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port);
																			/**
																			 * Если режим отладки не включён
																			 */
																			#else
																				// Записываем ошибку в лог
																				this->_log->print("Connection to the server \"%s:%u\" is failed", log_t::flag_t::WARNING, this->_client.getTarget(i->second.eid).c_str(), awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port);
																			#endif
																		}
																	}
																// Если фиксация настроек события сервера не выполнена
																} else {
																	// Если функция обратного вызова не установлена
																	if(!this->_callback.is("error")){
																		/**
																		 * Если включён режим отладки
																		 */
																		#if DEBUG_MODE
																			// Записываем ошибку в лог
																			this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																		/**
																		 * Если режим отладки не включён
																		 */
																		#else
																			// Записываем ошибку в лог
																			this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, eid);
																		#endif
																	}
																}
															// Если установка порта и адреса удалённого сервера для подключения не выполнена
															} else {
																// Если функция обратного вызова не установлена
																if(!this->_callback.is("error")){
																	/**
																	 * Если включён режим отладки
																	 */
																	#if DEBUG_MODE
																		// Записываем ошибку в лог
																		this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																	/**
																	 * Если режим отладки не включён
																	 */
																	#else
																		// Записываем ошибку в лог
																		this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, eid);
																	#endif
																}
															}
														// Если установка интерфейса для подключения к удалённому серверу не выполнена
														} else {
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Записываем ошибку в лог
																	this->_log->debug("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, iface.c_str(), eid);
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Записываем ошибку в лог
																	this->_log->print("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, iface.c_str(), eid);
																#endif
															}
														}
													// Если установка опций события не выполнена
													} else {
														// Если функция обратного вызова не установлена
														if(!this->_callback.is("error")){
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Записываем ошибку в лог
																this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Записываем ошибку в лог
																this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, eid);
															#endif
														}
													}
													// Удаляем клиента принадлежащего пиру
													this->_client.destroy(i->second.eid);
													// Устанавливаем статус ошибки, так как мы получили ошибку
													i->second.ctx.status = proto::socks5_t::status_t::NOADDR;
												} break;
											}
											// Если команда CONNECT завершилась ошибкой, отправляем ответ прокси-клиенту
											if(i->second.ctx.status != proto::socks5_t::status_t::SUCCESS)
												// Выполняем отправку ответа прокси-клиенту
												this->sendReply(eid, i->second.ctx, true);
										} break;
										// Если команда соответствует UDP ASSOCIATE
										case static_cast <uint8_t> (proto::socks5_t::command_t::UDP): {
											// Если список поддерживаемых UDP-серверов пустой
											if(!this->_udp.events.empty()){
												// Создаём объект параметров подключения для идентификатора события клиента
												unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
												/**
												 * Определяем тип адреса хоста для подключения
												 */
												switch(static_cast <uint8_t> (i->second.ctx.host->type)){
													// Если тип адреса соответствует IPv4
													case static_cast <uint8_t> (net::type_t::IPV4): {
														// Устанавливаем тип параметров подключения для идентификатора события клиента
														attr->type = net::type_t::IPV4;
														// Если IP-адрес которому разрешено подключаться к SOCKS5-прокси-серверу соответствует
														if(awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address == 0)
															// Устанавливаем IP-адрес подключённого клиента для идентификатора события клиента
															this->_unit->server.getAddress(eid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (attr.get())->ip);
														// Устанавливаем полученный IP-адрес
														else awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip);
													} break;
													// Если тип адреса соответствует IPv6
													case static_cast <uint8_t> (net::type_t::IPV6): {
														// Устанавливаем тип параметров подключения для идентификатора события клиента
														attr->type = net::type_t::IPV6;
														// Если IP-адрес которому разрешено подключаться к SOCKS5-прокси-серверу соответствует
														if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address[0], ::__awh_zero_ipv6__, 16) == 0)
															// Устанавливаем IP-адрес подключённого клиента для идентификатора события клиента
															this->_unit->server.getAddress(eid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (attr.get())->ip);
														// Устанавливаем полученный IP-адрес
														else awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip);
													} break;
												}
												// Если порт которому разрешено подключаться к SOCKS5-прокси-серверу соответствует
												if(awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port == 0)
													// Устанавливаем порт подключённого клиента для идентификатора события клиента
													awh_cast <net::attr_net_t *> (attr.get())->port = this->_unit->server.getPort(eid);
												// Устанавливаем полученный порт
												else awh_cast <net::attr_net_t *> (attr.get())->port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
												// Создаём идентификатор конечной точки для идентификатора события клиента
												const origin_t endpoint = origin_t().from(attr.get());
												// Добавляем идентификатор события клиента для конечной точки
												auto ret = this->_sessions.emplace(endpoint, make_pair(eid, 0));
												// Обновляем control-сессию, если endpoint уже зарегистрирован
												if(!ret.second)
													// Обновляем идентификатор события клиента для конечной точки
													ret.first->second.first = eid;
												// Устанавливаем статус успешного выполнения команды
												i->second.ctx.status = proto::socks5_t::status_t::SUCCESS;
												// Инициализируем генератор случайных чисел
												static ::random_device randev;
												// Инициализируем генератор случайных чисел с помощью устройства случайных чисел
												static ::mt19937 gen(randev());
												// Генерируем индекс от 0 до size - 1
												::uniform_int_distribution <> distrib(0, this->_udp.events.size() - 1);
												// Выбираем UDP-сервер случайным образом
												const event::id_t sid = this->_udp.events[distrib(gen)];
												// Устанавливаем порт для UDP-сервера
												awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port = this->_unit->server.getPort(sid);
												/**
												 * Определяем тип адреса хоста для подключения
												 */
												switch(static_cast <uint8_t> (i->second.ctx.host->type)){
													// Если тип адреса соответствует IPv4
													case static_cast <uint8_t> (net::type_t::IPV4): {
														// Если семейство адресов для этого сервера соответствует IPv4
														if(this->_unit->server.family(sid) == event::family_t::IPV4){
															// Создаём новый объект адреса сервера IPv4
															unique_ptr <net::addr_t> addr = nullptr;
															// Извлекаем IP-адрес этого сервера
															this->_unit->server.getAddress(sid, event::address_t::IPV4, addr);
															// Если IP-адрес которому разрешено подключаться к SOCKS5-прокси-серверу установлен
															if(awh_cast <net::addr_net_ipv4_t *> (addr.get())->address == 0)
																// Устанавливаем текущий IP-адрес этого сервера для установки подключения
																this->_unit->server.getAddress(eid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip);
															// Устанавливаем полученный IP-адрес
															else awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip = ::move(addr);
															// Если список алиасов для этого сервера не пустой
															if(!this->_aliases.empty()){
																// Создаём объект параметров подключения
																unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
																// Устанавливаем тип параметров подключения
																attr->type = net::type_t::IPV4;
																// Создаём новый объект адреса IPv4
																awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv4_t> ();
																// Устанавливаем полученный порт
																awh_cast <net::attr_net_t *> (attr.get())->port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																// Устанавливаем полученный IP-адрес
																awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address;
																// Создаём идентификатор конечной точки для идентификатора события клиента
																const origin_t endpoint = origin_t().from(attr.get());
																// Выполняем поиск алиаса для указанного адреса конечной точки
																auto j = this->_aliases.find(endpoint);
																// Если алиас для указанного адреса конечной точки найден
																if(j != this->_aliases.end()){
																	/**
																	 * Определяем тип полученного IP-адреса
																	 */
																	switch(static_cast <uint8_t> (j->second->type)){
																		// Для типа FQDN
																		case static_cast <uint8_t> (net::type_t::FQDN): {
																			// Получаем порт для подключения к удалённому серверу
																			uint16_t port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																			// Если порт для подключения к удалённому серверу установлен в алиасе, то используем его
																			if(awh_cast <const net::attr_fqdn_t *> (j->second.get())->port > 0)
																				// Устанавливаем порт для подключения к удалённому серверу, полученный из алиаса
																				port = awh_cast <const net::attr_fqdn_t *> (j->second.get())->port;
																			// Создаём объект параметров подключения для алиаса
																			i->second.ctx.host = make_unique <net::attr_fqdn_t> ();
																			// Устанавливаем тип параметров подключения
																			i->second.ctx.host->type = net::type_t::FQDN;
																			// Устанавливаем полученный порт
																			awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->port = port;
																			// Устанавливаем полученное доменное имя
																			awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->domain = awh_cast <const net::attr_fqdn_t *> (j->second.get())->domain;
																		} break;
																		// Для типа IPv4
																		case static_cast <uint8_t> (net::type_t::IPV4): {
																			// Получаем порт для подключения к удалённому серверу
																			uint16_t port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																			// Если порт для подключения к удалённому серверу установлен в алиасе, то используем его
																			if(awh_cast <const net::attr_net_t *> (j->second.get())->port > 0)
																				// Устанавливаем порт для подключения к удалённому серверу, полученный из алиаса
																				port = awh_cast <const net::attr_net_t *> (j->second.get())->port;
																			// Создаём объект параметров подключения для алиаса
																			i->second.ctx.host = make_unique <net::attr_net_t> ();
																			// Устанавливаем тип параметров подключения
																			i->second.ctx.host->type = net::type_t::IPV4;
																			// Создаём новый объект адреса IPv4
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
																			// Устанавливаем полученный порт
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port = port;
																			// Устанавливаем полученный IP-адрес
																			awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (j->second.get())->ip.get())->address;
																		} break;
																		// Для типа IPv6
																		case static_cast <uint8_t> (net::type_t::IPV6): {
																			// Получаем порт для подключения к удалённому серверу
																			uint16_t port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																			// Если порт для подключения к удалённому серверу установлен в алиасе, то используем его
																			if(awh_cast <const net::attr_net_t *> (j->second.get())->port > 0)
																				// Устанавливаем порт для подключения к удалённому серверу, полученный из алиаса
																				port = awh_cast <const net::attr_net_t *> (j->second.get())->port;
																			// Создаём объект параметров подключения для алиаса
																			i->second.ctx.host = make_unique <net::attr_net_t> ();
																			// Устанавливаем тип параметров подключения
																			i->second.ctx.host->type = net::type_t::IPV6;
																			// Создаём новый объект адреса IPv6
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
																			// Устанавливаем полученный порт
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port = port;
																			// Устанавливаем полученный IP-адрес
																			::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (j->second.get())->ip.get())->address[0], 16);
																		} break;
																	}
																}
															}
														// Если семейство адресов для этого сервера не соответствует IPv4
														} else {
															// Устанавливаем статус ошибки, так как команда не поддерживается
															i->second.ctx.status = proto::socks5_t::status_t::DENIED;
															// Отправляем ответ прокси-клиенту, так как команда не поддерживается
															goto End;
														}
													} break;
													// Если тип адреса соответствует IPv6
													case static_cast <uint8_t> (net::type_t::IPV6): {
														// Если семейство адресов для этого сервера соответствует IPv6
														if(this->_unit->server.family(sid) == event::family_t::IPV6){
															// Создаём новый объект адреса сервера IPv6
															unique_ptr <net::addr_t> addr = nullptr;
															// Извлекаем IP-адрес этого сервера
															this->_unit->server.getAddress(sid, event::address_t::IPV6, addr);
															// Если IP-адрес которому разрешено подключаться к SOCKS5-прокси-серверу установлен
															if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (addr.get())->address[0], ::__awh_zero_ipv6__, 16) == 0)
																// Устанавливаем текущий IP-адрес этого сервера для установки подключения
																this->_unit->server.getAddress(eid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip);
															// Устанавливаем полученный IP-адрес
															else awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip = ::move(addr);
															// Если список алиасов для этого сервера не пустой
															if(!this->_aliases.empty()){
																// Создаём объект параметров подключения
																unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
																// Устанавливаем тип параметров подключения
																attr->type = net::type_t::IPV6;
																// Создаём новый объект адреса IPv6
																awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
																// Устанавливаем полученный порт
																awh_cast <net::attr_net_t *> (attr.get())->port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																// Устанавливаем полученный IP-адрес
																::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address[0], 16);
																// Создаём идентификатор конечной точки для идентификатора события клиента
																const origin_t endpoint = origin_t().from(attr.get());
																// Выполняем поиск алиаса для указанного адреса конечной точки
																auto j = this->_aliases.find(endpoint);
																// Если алиас для указанного адреса конечной точки найден
																if(j != this->_aliases.end()){
																	/**
																	 * Определяем тип полученного IP-адреса
																	 */
																	switch(static_cast <uint8_t> (j->second->type)){
																		// Для типа FQDN
																		case static_cast <uint8_t> (net::type_t::FQDN): {
																			// Получаем порт для подключения к удалённому серверу
																			uint16_t port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																			// Если порт для подключения к удалённому серверу установлен в алиасе, то используем его
																			if(awh_cast <const net::attr_fqdn_t *> (j->second.get())->port > 0)
																				// Устанавливаем порт для подключения к удалённому серверу, полученный из алиаса
																				port = awh_cast <const net::attr_fqdn_t *> (j->second.get())->port;
																			// Создаём объект параметров подключения для алиаса
																			i->second.ctx.host = make_unique <net::attr_fqdn_t> ();
																			// Устанавливаем тип параметров подключения
																			i->second.ctx.host->type = net::type_t::FQDN;
																			// Устанавливаем полученный порт
																			awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->port = port;
																			// Устанавливаем полученное доменное имя
																			awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->domain = awh_cast <const net::attr_fqdn_t *> (j->second.get())->domain;
																		} break;
																		// Для типа IPv4
																		case static_cast <uint8_t> (net::type_t::IPV4): {
																			// Получаем порт для подключения к удалённому серверу
																			uint16_t port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																			// Если порт для подключения к удалённому серверу установлен в алиасе, то используем его
																			if(awh_cast <const net::attr_net_t *> (j->second.get())->port > 0)
																				// Устанавливаем порт для подключения к удалённому серверу, полученный из алиаса
																				port = awh_cast <const net::attr_net_t *> (j->second.get())->port;
																			// Создаём объект параметров подключения для алиаса
																			i->second.ctx.host = make_unique <net::attr_net_t> ();
																			// Устанавливаем тип параметров подключения
																			i->second.ctx.host->type = net::type_t::IPV4;
																			// Создаём новый объект адреса IPv4
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
																			// Устанавливаем полученный порт
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port = port;
																			// Устанавливаем полученный IP-адрес
																			awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (j->second.get())->ip.get())->address;
																		} break;
																		// Для типа IPv6
																		case static_cast <uint8_t> (net::type_t::IPV6): {
																			// Получаем порт для подключения к удалённому серверу
																			uint16_t port = awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port;
																			// Если порт для подключения к удалённому серверу установлен в алиасе, то используем его
																			if(awh_cast <const net::attr_net_t *> (j->second.get())->port > 0)
																				// Устанавливаем порт для подключения к удалённому серверу, полученный из алиаса
																				port = awh_cast <const net::attr_net_t *> (j->second.get())->port;
																			// Создаём объект параметров подключения для алиаса
																			i->second.ctx.host = make_unique <net::attr_net_t> ();
																			// Устанавливаем тип параметров подключения
																			i->second.ctx.host->type = net::type_t::IPV6;
																			// Создаём новый объект адреса IPv6
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
																			// Устанавливаем полученный порт
																			awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port = port;
																			// Устанавливаем полученный IP-адрес
																			::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (j->second.get())->ip.get())->address[0], 16);
																		} break;
																	}
																}
															}
														// Если семейство адресов для этого сервера не соответствует IPv6
														} else {
															// Устанавливаем статус ошибки, так как команда не поддерживается
															i->second.ctx.status = proto::socks5_t::status_t::DENIED;
															// Отправляем ответ прокси-клиенту, так как команда не поддерживается
															goto End;
														}
													} break;
												}
											// Если список поддерживаемых UDP-серверов пустой
											} else {
												// Устанавливаем статус ошибки, так как UDP-серверы недоступны
												i->second.ctx.status = proto::socks5_t::status_t::DENIED;
												// Отправляем ответ прокси-клиенту
												goto End;
											}
										} break;
										// В остальных случаях
										default:
											// Устанавливаем статус ошибки, так как команда не поддерживается
											i->second.ctx.status = proto::socks5_t::status_t::NOCOMMAND;
									}
									/**
									 * Устанавливаем метку для формирования ответа прокси-сервером, который будет отправлен прокси-клиенту.
									 * Эта метка будет указывать на то, что ответ должен быть отправлен после выполнения команды CONNECT или UDP ASSOCIATE,
									 * так как эти команды требуют установления подключения к удалённому серверу и отправки ответа после выполнения этих действий.
									 * В остальных случаях, ответ может быть отправлен сразу же после обработки данных от прокси-клиента,
									 * так как эти команды не требуют установления подключения к удалённому серверу и могут быть обработаны в автоматическом режиме.
									 */
									End:
									// Размер буфера данных
									size_t size = 0;
									// Буфер данных ответа
									uint8_t * buffer = nullptr;
									// Если извлечение буфера данных ответа выполнено успешно
									if(this->_socks5.buffer(&buffer, size, i->second.ctx)){
										// Если отправка ответа прокси-клиенту не выполнена
										if(this->_unit->server.send(eid, buffer, size) != size){
											// Если функция обратного вызова не установлена
											if(!this->_callback.is("error")){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
												#endif
											}
											// Удаляем подключённого пира
											this->_unit->server.destroy(eid);
										// Если отправка ответа прокси-клиенту выполнена успешно
										} else {
											// Если статус ответа от прокси-сервера соответствует успешному выполнению команды
											if(i->second.ctx.status == proto::socks5_t::status_t::SUCCESS){
												// Устанавливаем статус успешного выполнения команды
												i->second.ctx.state = proto::socks5_t::state_t::COMPLETED;
												// Очищаем буфер накопления SOCKS5-кадров
												i->second.rx.clear();
											}
											// Если статус ответа от прокси-сервера как запрещённый, так и не поддерживаемый
											else i->second.ctx.state = proto::socks5_t::state_t::BROKEN;
										}
									// Если извлечение буфера данных ответа не выполнено
									} else if(i->second.ctx.status != proto::socks5_t::status_t::SUCCESS)
										// Удаляем подключённого пира
										this->_unit->server.destroy(eid);
								} break;
								// В остальных случаях, проходим процедуру общения с клиентом в автоматическом режиме
								default: {
									// Размер буфера данных
									size_t size = 0;
									// Буфер данных ответа
									uint8_t * buffer = nullptr;
									// Если извлечение буфера данных ответа выполнено успешно
									if(this->_socks5.buffer(&buffer, size, i->second.ctx)){
										// Если отправка ответа прокси-клиенту не выполнена
										if(this->_unit->server.send(eid, buffer, size) != size){
											// Если функция обратного вызова не установлена
											if(!this->_callback.is("error")){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
												#endif
											}
											// Удаляем подключённого пира
											this->_unit->server.destroy(eid);
										}
									}
								}
							}
							// Прекращаем разбор накопленных кадров
							if(i->second.rx.empty() ||
							   i->second.ctx.state == proto::socks5_t::state_t::BROKEN ||
							   i->second.ctx.state == proto::socks5_t::state_t::COMPLETED)
								// Выходим из цикла обработки кадров
								break;
						// Если парсинг полного SOCKS5-кадра не выполнен
						} else {
							// Выполняем поиск идентификатора клиента принадлежащего этому пиру
							auto j = this->_clients.find(i->second.eid);
							// Если идентификатор клиента найден в списке
							if(j != this->_clients.end()){
								// Удаляем подключённого клиента
								this->_client.destroy(j->first);
								// Удаляем клиента из списка активных клиентов
								this->_clients.erase(j);
							}
							// Удаляем подключённого пира
							this->_unit->server.destroy(eid);
							// Выходим из функции
							return;
						}
					}
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод обработки неудачного резолвинга доменного имени
 *
 * @param id     идентификатор DNS-запроса
 * @param record тип записи DNS
 * @param domain доменное имя
 *
 */
void awh::server::Socks5::failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept {
	// Если DNS-резолвер не установлен или не находится в рабочем состоянии
	if((this->_dns.client == nullptr) || !this->_dns.client->working())
		// Выходим из функции
		return;
	// Выполняем поиск идентификатора DNS-запроса для получения идентификатора пира
	auto i = this->_resolves.find(id);
	// Если идентификатор DNS-запроса найден
	if(i != this->_resolves.end()){
		// Получаем идентификатор пира, которому принадлежит этот DNS-запрос
		const event::id_t peer = i->second;
		// Выполняем поиск пира в списке активных пиров
		auto j = this->_peers.find(peer);
		// Флаг необходимости закрытия control-соединения пира
		bool destroyPeer = true;
		// Если пир найден в списке активных пиров
		if(j != this->_peers.end()){
			// Если для пира создан исходящий UDP-клиент, ожидающий DNS-резолвинг
			const bool pendingUdp = ((j->second.eid != 0) && (::__awh_cache__.find(j->second.eid) != ::__awh_cache__.end()));
			// Если ожидается DNS-резолвинг для UDP-пакета
			if(pendingUdp){
				// Сохраняем control-соединение пира
				destroyPeer = false;
				// Удаляем кэш исходящего UDP-клиента
				::__awh_cache__.erase(j->second.eid);
				// Удаляем исходящего клиента
				this->_client.destroy(j->second.eid);
				// Удаляем связь между клиентом и пиром
				this->_clients.erase(j->second.eid);
				// Обнуляем идентификатор исходящего клиента
				j->second.eid = 0;
				// Обнуляем идентификатор DNS-резолвера
				j->second.did = 0;
			// Если пир ожидает завершения TCP CONNECT по FQDN
			} else if(static_cast <uint8_t> (j->second.ctx.state) == static_cast <uint8_t> (proto::socks5_t::state_t::HANDSHAKE)){
				// Устанавливаем статус недоступности хоста
				j->second.ctx.status = proto::socks5_t::status_t::UNAVHOST;
				// Создаём объект параметров подключения для ответа об ошибке
				j->second.ctx.host = make_unique <net::attr_net_t> ();
				// Устанавливаем тип адреса как IPv4
				j->second.ctx.host->type = net::type_t::IPV4;
				// Создаём новый объект адреса IPv4 для ответа с адресом 0.0.0.0
				awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
				// Удаляем связь DNS-резолвера и идентификатора пира
				this->_resolves.erase(i);
				// Отправляем ответ прокси-клиенту и закрываем пира
				this->sendReply(peer, j->second.ctx, true);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)> ("failure_dns", peer, id, record, domain);
				// Выходим из функции
				return;
			// Если для пира создан исходящий клиент
			} else if(j->second.eid != 0){
				// Удаляем кэш исходящего UDP-клиента
				::__awh_cache__.erase(j->second.eid);
				// Удаляем исходящего клиента
				this->_client.destroy(j->second.eid);
				// Удаляем связь между клиентом и пиром
				this->_clients.erase(j->second.eid);
				// Обнуляем идентификатор исходящего клиента
				j->second.eid = 0;
			}
		}
		// Если требуется закрыть control-соединение пира
		if(destroyPeer)
			// Удаляем пира к которому принадлежит этот DNS-запрос
			this->_unit->server.destroy(peer);
		// Удаляем связь DNS-резолвера и идентификатора пира
		this->_resolves.erase(i);
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)> ("failure_dns", peer, id, record, domain);
	// Если функция обратного вызова установлена, выполняем функцию обратного вызова
	} else this->_callback.call <void (const event::id_t, const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)> ("failure_dns", static_cast <event::id_t> (this->_id.eid), id, record, domain);
}
/**
 * @brief Метод резолвинга доменного имени удалённого хоста в сетевой адрес
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для резолвинга
 * @param addr   указатель на структуру для хранения результата резолвинга
 *
 */
void awh::server::Socks5::resolve(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	// Если DNS-резолвер не установлен или не находится в рабочем состоянии
	if((this->_dns.client == nullptr) || !this->_dns.client->working())
		// Выходим из функции
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор DNS-запроса соответствует идентификатору DNS-резолвера сервера
		if(id == this->_dns.id){
			/**
			 * Определяем семейство адресов с которым работает сервер
			 */
			switch(static_cast <uint8_t> (family)){
				// Если сервер работает с адресами IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Устанавливаем адрес хоста целевой текущей машины
					if(this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV4, addr)){
						// Если событие сервера не запущено, запускаем его
						if(this->_unit->server.commit(this->_id.eid)){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), family, domain, this->_unit->server.getAddress(static_cast <event::id_t> (this->_id.eid), event::address_t::IPV4));
							// Запускаем сервер
							this->_unit->server.start();
						}
					}
				} break;
				// Если сервер работает с адресами IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Устанавливаем адрес хоста целевой текущей машины
					if(this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV6, addr)){
						// Если событие сервера не запущено, запускаем его
						if(this->_unit->server.commit(this->_id.eid)){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), family, domain, this->_unit->server.getAddress(static_cast <event::id_t> (this->_id.eid), event::address_t::IPV6));
							// Запускаем сервер
							this->_unit->server.start();
						}
					}
				} break;
			}
		// Если это клиентский запрос на резолвинг доменного имени удалённого хоста
		} else {
			// Выполняем поиск идентификатора DNS-запроса для получения идентификатора пира, которому принадлежит этот DNS-запрос
			auto i = this->_resolves.find(id);
			// Если идентификатор DNS-запроса найден
			if(i != this->_resolves.end()){
				// Получаем идентификатор пира, которому принадлежит DNS-запрос
				const event::id_t peer = i->second;
				// Выполняем поиск пира в списке активных пиров
				auto peerIt = this->_peers.find(peer);
				// Если пир найден в списке активных пиров
				if(peerIt != this->_peers.end()){
					// Идентификатор исходящего клиента пира
					const event::id_t cid = peerIt->second.eid;
					// Выполняем поиск кэша для исходящего UDP-клиента
					auto cacheIt = (cid != 0) ? ::__awh_cache__.find(cid) : ::__awh_cache__.end();
					// Если найден кэш UDP-запроса, ожидающего резолвинг
					if(cacheIt != ::__awh_cache__.end()){
						// Ссылка на кэш UDP-запроса
						auto & cache = cacheIt->second;
						// Устанавливаем порт и адрес удалённого сервера для подключения
						if(this->_client.setTarget(cid, addr) && this->_client.setTargetPort(cid, cache.attr->port)){
							// Устанавливаем IP-адрес удалённого сервера для подключения
							this->_client.getTarget(cid, cache.attr->ip);
							// Выполняем получение идентификатора функции обратного вызова
							const callback_t::id_t fid = this->_callback.id("ready");
							// Если функция обратного вызова установлена
							if(this->_callback.is(fid)){
								// Получаем IP-адрес для подключения к удалённому серверу
								const string & address = this->_client.getTarget(cid);
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> (fid, cache.eid, family, domain, address);
							}
							// Выполняем фиксацию настроек события сервера
							if(this->_client.commit(cid)){
								// Выполняем запуск события
								if(!this->_client.launch(cid)){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, i->second);
										#endif
									}
								// Если резолвинг хоста не выполнен
								} else {
									// Получаем опции события для идентификатора события клиента
									uint16_t options = this->_client.getOptions(cid);
									// Если опция AUTO_RECONNECT установлена
									if(options & event::options::AUTO_RECONNECT){
										// Сбрасываем опцию AUTO_RECONNECT для идентификатора события клиента
										options &= ~event::options::AUTO_RECONNECT;
										// Устанавливаем опции события для идентификатора события клиента
										this->_client.setOptions(cid, options);
									}
									// Добавляем связь между клиентом и пиром которому он принадлежит
									this->_clients.emplace(cid, peer);
									// Выполняем отправку данных клиенту
									this->_client.send(cid, cache.buffer, cache.size);
								}
							// Если фиксация настроек события сервера не выполнена
							} else {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, i->second);
									#endif
								}
							}
						// Если установка порта и адреса удалённого сервера для подключения не выполнена
						} else {
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, i->second);
								#endif
							}
						}
					// Если это TCP-запрос с FQDN, ожидающий резолвинг
					} else {
						// Ссылка на параметры пира
						auto j = peerIt;
						// Выполняем создание клиента для подключения к удалённому серверу
						j->second.eid = this->_client.issue(this->_unit->server.family(i->second), event::type_t::STREAM, event::protocol_t::TCP);
						// Устанавливаем опции события
						if(this->_client.setOptions(j->second.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
							// Извлекаем сетевой интерфейс для подключения к удалённому серверу
							const string & iface = this->_unit->server.getIface(i->second);
							// Устанавливаем интерфейс для подключения к удалённому серверу
							if(this->_client.setIface(j->second.eid, iface)){
								// Устанавливаем порт и адрес удалённого сервера для подключения
								if(this->_client.setTarget(j->second.eid, addr) && this->_client.setTargetPort(j->second.eid, awh_cast <net::attr_fqdn_t *> (j->second.ctx.host.get())->port)){
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", i->second, family, domain, this->_client.getTarget(j->second.eid));
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const event::id_t)> ("accept", static_cast <event::id_t> (this->_id.eid), i->second);
									// Выполняем фиксацию настроек события сервера
									if(this->_client.commit(j->second.eid)){
										// Если подключение к серверу прошло успешно
										if(this->_client.connect(j->second.eid)){
											// Выполняем запуск события
											if(!this->_client.launch(j->second.eid)){
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Записываем ошибку в лог
														this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Записываем ошибку в лог
														this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, i->second);
													#endif
												}
											// Если резолвинг хоста не выполнен
											} else {
												// Получаем опции события для идентификатора события клиента
												uint16_t options = this->_client.getOptions(j->second.eid);
												// Если опция AUTO_RECONNECT установлена
												if(options & event::options::AUTO_RECONNECT){
													// Сбрасываем опцию AUTO_RECONNECT для идентификатора события клиента
													options &= ~event::options::AUTO_RECONNECT;
													// Устанавливаем опции события для идентификатора события клиента
													this->_client.setOptions(j->second.eid, options);
												}
												// Добавляем связь между клиентом и пиром которому он принадлежит
												this->_clients.emplace(j->second.eid, i->second);
												// Удаляем связь DNS-резолвера и идентификатора пира
												this->_resolves.erase(i);
												// Выходим из функции
												return;
											}
										// Если подключение к серверу не прошло успешно
										} else {
											// Если функция обратного вызова не установлена
											if(!this->_callback.is("error")){
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("Connection to the server \"%s:%u\" is failed", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, domain.c_str(), awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->port);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("Connection to the server \"%s:%u\" is failed", log_t::flag_t::WARNING, domain.c_str(), awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->port);
												#endif
											}
										}
									// Если фиксация настроек события сервера не выполнена
									} else {
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, i->second);
											#endif
										}
									}
								// Если установка порта и адреса удалённого сервера для подключения не выполнена
								} else {
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, i->second);
										#endif
									}
								}
							// Если установка интерфейса для подключения к удалённому серверу не выполнена
							} else {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, iface.c_str(), i->second);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, iface.c_str(), i->second);
									#endif
								}
							}
						// Если установка опций события не выполнена
						} else {
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, i->second);
								#endif
							}
						}
						// Удаляем клиента принадлежащего пиру
						this->_client.destroy(j->second.eid);
						// Устанавливаем статус ошибки, так как мы получили ошибку
						j->second.ctx.status = proto::socks5_t::status_t::NOADDR;
						// Отправляем ответ прокси-клиенту и закрываем пира
						this->sendReply(i->second, j->second.ctx, true);
					}
				}
				// Удаляем связь DNS-резолвера и идентификатора пира
				this->_resolves.erase(i);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
		// Выполняем поиск идентификатора DNS-запроса для получения идентификатора пира, которому принадлежит этот DNS-запрос
		auto i = this->_resolves.find(id);
		// Если идентификатор DNS-запроса найден
		if(i != this->_resolves.end()){
			// Получаем идентификатор пира, которому принадлежит этот DNS-запрос
			const event::id_t peer = i->second;
			// Выполняем поиск пира, которому принадлежит идентификатор
			auto j = this->_peers.find(peer);
			// Если пир для этого идентификатора найден
			if(j != this->_peers.end()){
				// Если для пира создан исходящий UDP-клиент
				if(j->second.eid != 0){
					// Удаляем кэш исходящего UDP-клиента
					::__awh_cache__.erase(j->second.eid);
					// Удаляем исходящего клиента
					this->_client.destroy(j->second.eid);
					// Удаляем связь между клиентом и пиром
					this->_clients.erase(j->second.eid);
				}
				// Удаляем подключённого пира
				this->_unit->server.destroy(peer);
			}
			// Удаляем связь DNS-резолвера и идентификатора пира
			this->_resolves.erase(i);
		}
	}
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::server::Socks5::stop() noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->server.working() : false)){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns.client->stop();
			// Если объект DNS-резолвера не установлен, останавливаем событие сервера
			else this->_unit->server.stop();
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::server::Socks5::start() noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
		// Устанавливаем функцию обратного вызова для аутентификации
		this->_socks5.on(this->_callback.get <bool (const string &, const string &)> ("auth"));
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Запускаем событие DNS-резолвера
				this->_dns.client->start();
			// Если объект DNS-резолвера не установлен
			else {
				// Если событие сервера не запущено, запускаем его
				if(awh_cast <unit::unit_t *> (&this->_unit->server)->status(this->_id.eid) == event::status_t::NONE){
					// Если событие сервера не запущено, запускаем его
					if(this->_unit->server.commit(this->_id.eid)){
						// Выполняем получение идентификатора функции обратного вызова
						const callback_t::id_t fid = this->_callback.id("ready");
						// Если функция обратного вызова установлена
						if(this->_callback.is(fid)){
							// Хост текущего сервера
							string host = "";
							/**
							 * Определяем семейство адресов с которым работает сервер
							 */
							switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
								// Если сервер работает с адресами Unix Domain Socket
								case static_cast <uint8_t> (event::family_t::UDS):
									// Извлекаем адрес хоста текущей машины для адресов Unix Domain Socket
									host = ::move(this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS));
								break;
								// Если сервер работает с адресами IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
									// Извлекаем адрес хоста текущей машины для адресов IPv4
									host = ::move(this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV4));
								break;
								// Если сервер работает с адресами IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
									// Извлекаем адрес хоста текущей машины для адресов IPv6
									host = ::move(this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV6));
								break;
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> (fid, static_cast <event::id_t> (this->_id.eid), this->_unit->server.family(static_cast <event::id_t> (this->_id.eid)), host, host);
						}
						// Запускаем сервер
						this->_unit->server.start();
					}
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 *
 */
bool awh::server::Socks5::pause(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_unit->server.pause(i->first))
			// Приостанавливаем работу события клиента, принадлежащего подключённому пиру
			return this->_client.pause(i->second.eid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 *
 */
bool awh::server::Socks5::resume(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_unit->server.resume(i->first))
			// Возобновляем работу события клиента, принадлежащего подключённому пиру
			return this->_client.resume(i->second.eid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события клиента для уничтожения
 *
 */
void awh::server::Socks5::destroy(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор сервера установлен
		if(eid == this->_id.eid)
			// Уничтожаем событие сервера
			return this->_unit->server.destroy(this->_id.eid);
		// Если идентификатор принадлежит пиру, подключённому к серверу
		else {
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Уничтожаем событие подключённого пира
				this->_unit->server.destroy(i->first);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::server::Socks5::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	server_t::callback(callback);
	// Выполняем установку функции обратного вызова на событие аутентификации клиента
	this->_callback.set("auth", callback);
	// Выполняем установку функции обратного вызова на событие успешного подключения к удалённому серверу
	this->_callback.set("connect", callback);
}
/**
 * @brief Метод получения данных от клиента (заглушка для сервера SOCKS5)
 *
 * @return результат получения данных
 *
 */
bool awh::server::Socks5::recv(const event::id_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных клиенту (заглушка для сервера SOCKS5)
 *
 * @return количество байт данных, отправленных клиенту
 *
 */
size_t awh::server::Socks5::send(const event::id_t, const void *, const size_t) noexcept {
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод перемещения данных между сервером и другим событием (заглушка для сервера SOCKS5)
 *
 * @return результат выполнения перемещения
 *
 */
bool awh::server::Socks5::splice(const event::id_t, const event::id_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения опций клиента
 *
 * @param eid идентификатор события клиента
 * @return    опции клиента
 *
 */
uint16_t awh::server::Socks5::getOptions(const event::id_t eid) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор сервера установлен
		if(eid == this->_id.eid)
			// Получаем опции сервера
			return this->_unit->server.getOptions(this->_id.eid);
		// Если идентификатор принадлежит пиру, подключённому к серверу
		else {
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Извлекаем опции для события клиента, принадлежащего подключённому пиру
				return this->_client.getOptions(i->second.eid);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции клиента для установки
 * @return        результат выполнения установки
 *
 */
bool awh::server::Socks5::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор сервера установлен
		if(eid == this->_id.eid)
			// Устанавливаем опции сервера
			return this->_unit->server.setOptions(this->_id.eid, options);
		// Если идентификатор принадлежит пиру, подключённому к серверу
		else {
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end()){
				// Если опция AUTO_RECONNECT установлена
				if(options & event::options::AUTO_RECONNECT)
					// Сбрасываем опцию AUTO_RECONNECT для идентификатора события клиента
					const_cast<uint16_t &> (options) &= ~event::options::AUTO_RECONNECT;
				// Устанавливаем опции для события клиента, принадлежащего подключённому пиру
				return this->_client.setOptions(i->second.eid, options);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, options), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция клиента для установки
 * @param mode   режим установки опции клиента
 * @return       результат выполнения установки
 *
 */
bool awh::server::Socks5::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор сервера установлен
		if(eid == this->_id.eid)
			// Устанавливаем опцию сервера
			return this->_unit->server.setOption(this->_id.eid, option, mode);
		// Если идентификатор принадлежит пиру, подключённому к серверу
		else {
			// Если опция AUTO_RECONNECT установлена
			if(option == event::options::AUTO_RECONNECT)
				// Возвращаем значение по умолчанию, так как опция AUTO_RECONNECT не может быть установлена для клиента
				return false;
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Устанавливаем опцию для события клиента, принадлежащего подключённому пиру
				return this->_client.setOption(i->second.eid, option, mode);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, option, mode), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса для подключения к сети клиентов
 *
 * @return сетевой интерфейс сервера
 *
 */
string awh::server::Socks5::getIface() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем сетевой интерфейс сервера
		return this->_unit->server.getIface(this->_id.eid);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @param eid идентификатор события сервера
 * @return    сетевой интерфейс сервера
 *
 */
string awh::server::Socks5::getIface(const event::id_t eid) const noexcept {
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if(eid == this->_id.eid)
		// Извлекаем сетевой интерфейс для сервера
		return this->_unit->server.getIface(eid);
	// Если идентификатор сервера не установлен
	else {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем сетевой интерфейс для события пира
			return this->_client.getIface(i->second.eid);
		// Если идентификатор события подключённого пира не найден
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server or peer ID is not set", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server or peer ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса для подключения к сети клиентов
 *
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::server::Socks5::setIface(string_view name) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0)
			// Устанавливаем сетевой интерфейс сервера
			return this->_unit->server.setIface(this->_id.eid, name);
		// Если идентификатор сервера не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param eid  идентификатор события сервера
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::server::Socks5::setIface(const event::id_t eid, string_view name) noexcept {
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if(eid == this->_id.eid){
		// Если DNS-резолвер или сервер находятся в нерабочем состоянии
		if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working())
			// Устанавливаем сетевой интерфейс сервера
			return this->_unit->server.setIface(eid, name);
	// Если идентификатор сервера не установлен
	} else {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем сетевой интерфейс для события пира
			return this->_client.setIface(i->second.eid, name);
		// Если идентификатор события подключённого пира не найден
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server or peer ID is not set", __PRETTY_FUNCTION__, make_tuple(eid, name), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server or peer ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта сервера
 *
 * @return порт сервера
 *
 */
uint16_t awh::server::Socks5::getSourcePort() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем порт текущего сервера
		return this->_unit->server.getPort(this->_id.eid);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения внутреннего порта клиента, подключённого к серверу
 *
 * @param eid идентификатор события клиента
 * @return    внутренний порт клиента
 *
 */
uint16_t awh::server::Socks5::getSourcePort(const event::id_t eid) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем внутренний порт события пира
			return this->_unit->server.getPort(i->first);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта сервера
 *
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 *
 */
bool awh::server::Socks5::setSourcePort(const uint16_t port) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0)
			// Устанавливаем порт текущего сервера
			return this->_unit->server.setPort(this->_id.eid, port);
		// Если идентификатор сервера не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удалённого клиента или текущего сервера
 *
 * @param eid идентификатор события клиента или сервера
 * @return    порт удалённого клиента или текущего сервера
 *
 */
uint16_t awh::server::Socks5::getTargetPort(const event::id_t eid) const noexcept {
	// Выполняем поиск идентификатор события подключённого пира
	auto i = this->_peers.find(eid);
	// Если идентификатор события подключённого пира найден
	if(i != this->_peers.end())
		// Получаем порт удалённого клиента принадлежащего подключённому пиру
		return this->_client.getTargetPort(i->second.eid);
	// Если идентификатор события подключённого пира не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or peer ID is not set", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or peer ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события клиента
 * @return    адрес хоста целевой машины
 *
 */
string awh::server::Socks5::getTarget(const event::id_t eid) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем адрес хоста целевой машины для клиента принадлежащего этому пиру
			return this->_client.getTarget(i->second.eid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 *
 */
bool awh::server::Socks5::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем адрес хоста целевой машины для клиента принадлежащего этому пиру
			return this->_client.getTarget(i->second.eid, target);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::server::Socks5::setAddress(const event::address_t address, string_view value) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Устанавливаем адрес сервера
			if((result = this->_unit->server.setAddress(this->_id.eid, address, value))){
				/**
				 * Определяем семейство адресов с которым работает сервер
				 */
				switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
					// Если сервер работает с адресами Unix Domain Socket
					case static_cast <uint8_t> (event::family_t::UDS):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS);
					break;
					// Если сервер работает с адресами IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV4);
					break;
					// Если сервер работает с адресами IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV6);
					break;
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address), value), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::server::Socks5::setAddress(const event::address_t address, const net::addr_t * value) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Устанавливаем адрес сервера
			if((result = this->_unit->server.setAddress(this->_id.eid, address, value))){
				/**
				 * Определяем семейство адресов с которым работает сервер
				 */
				switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
					// Если сервер работает с адресами IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV4);
					break;
					// Если сервер работает с адресами IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::IPV6);
					break;
					// Если сервер работает с адресами Unix Domain Socket
					case static_cast <uint8_t> (event::family_t::UDS):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS);
					break;
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::server::Socks5::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if(eid == this->_id.eid){
			// Если DNS-резолвер или сервер находятся в нерабочем состоянии
			if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working())
				// Устанавливаем внутренний адрес socks5-сервера
				return this->_unit->server.setAddress(eid, address, value);
		// Если идентификатор события передан другой
		} else {
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Устанавливаем внутренний адрес удалённого клиента подключённого пира
				return this->_client.setAddress(i->second.eid, address, value);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address), value), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::server::Socks5::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if(eid == this->_id.eid){
			// Если DNS-резолвер или сервер находятся в нерабочем состоянии
			if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working())
				// Устанавливаем внутренний адрес socks5-сервера
				return this->_unit->server.setAddress(eid, address, value);
		// Если идентификатор события передан другой
		} else {
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Устанавливаем внутренний адрес удалённого клиента подключённого пира
				return this->_client.setAddress(i->second.eid, address, value);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param address тип адреса сервера
 * @return        значение адреса сервера
 *
 */
string awh::server::Socks5::getAddress(const event::address_t address) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем адрес сервера
		return this->_unit->server.getAddress(this->_id.eid, address);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   объект для извлечения адреса сервера
 * @return        результат выполнения извлечения адреса сервера
 *
 */
bool awh::server::Socks5::getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем адрес сервера
		return this->_unit->server.getAddress(this->_id.eid, address, value);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента или текущего сервера
 *
 * @param eid     идентификатор события клиента или сервера
 * @param address тип адреса клиента или сервера
 * @return        значение адреса клиента или сервера
 *
 */
string awh::server::Socks5::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if(eid == this->_id.eid)
			// Получаем адрес внутренний адрес socks5-сервера
			return this->_unit->server.getAddress(eid, address);
		// Если идентификатор события передан другой
		else {
			// Если идентификатор события подключённого пира найден
			if(this->_peers.find(eid) != this->_peers.end())
				// Получаем адрес подключённого пира
				return this->_unit->server.getAddress(eid, address);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод получения адреса клиента или текущего сервера
 *
 * @param eid     идентификатор события клиента или сервера
 * @param address тип адреса клиента или сервера
 * @param value   объект для извлечения адреса клиента или сервера
 * @return        результат выполнения извлечения адреса клиента или сервера
 *
 */
bool awh::server::Socks5::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if(eid == this->_id.eid)
			// Получаем адрес внутренний адрес socks5-сервера
			return this->_unit->server.getAddress(eid, address, value);
		// Если идентификатор события передан другой
		else {
			// Если идентификатор события подключённого пира найден
			if(this->_peers.find(eid) != this->_peers.end())
				// Получаем адрес подключённого пира
				return this->_unit->server.getAddress(eid, address, value);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       размер буфера клиента
 *
 */
size_t awh::server::Socks5::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем размер буфера для клиента принадлежащего этому пиру
			return this->_client.getBufferSize(i->second.eid, action);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 *
 */
bool awh::server::Socks5::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_unit->server.setBufferSize(eid, action, size))
			// Устанавливаем размер буфера для клиента принадлежащего этому пиру
			return this->_client.setBufferSize(i->second.eid, action, size);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @return режим использования таймаута на чтение события
 *
 */
awh::event::usage_t awh::server::Socks5::getUsageReadTimeout() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем режим использования таймаута на чтение события
		return this->_unit->server.getUsageReadTimeout(this->_id.eid);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события клиента
 *
 * @param eid идентификатор события клиента
 * @return    режим использования таймаута на чтение события клиента
 *
 */
awh::event::usage_t awh::server::Socks5::getUsageReadTimeout(const event::id_t eid) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем режим использования таймаута на чтение для клиента принадлежащего этому пиру
			return this->_client.getUsageReadTimeout(i->second.eid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 *
 */
void awh::server::Socks5::setUsageReadTimeout(const event::usage_t usage) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем режим использования таймаута на чтение события
		this->_unit->server.setUsageReadTimeout(this->_id.eid, usage);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки режима использования таймаута на чтение события клиента
 *
 * @param eid   идентификатор события клиента
 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
 *
 */
void awh::server::Socks5::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем режим использования таймаута на чтение для клиента принадлежащего этому пиру
			return this->_client.setUsageReadTimeout(i->second.eid, usage);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (usage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения таймаута сервера
 *
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::server::Socks5::getTimeout(const event::action_t action) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем таймаут сервера
		return this->_unit->server.getTimeout(this->_id.eid, action);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::server::Socks5::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем таймаут для клиента принадлежащего этому пиру
			return this->_client.getTimeout(i->second.eid, action);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута сервера
 *
 * @param action  тип действия сервера
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::server::Socks5::setTimeout(const event::action_t action, const uint32_t timeout) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем таймаут сервера
		this->_unit->server.setTimeout(this->_id.eid, action, timeout);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки таймаута клиента
 *
 * @param eid     идентификатор события клиента
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::server::Socks5::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем таймаут для клиента принадлежащего этому пиру
			return this->_client.setTimeout(i->second.eid, action, timeout);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки пропускной способности сервера
 *
 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::server::Socks5::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем пропускную способность сервера
		this->_unit->server.bandwidth(this->_id.eid, limiting, bandwidth);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param eid       идентификатор события клиента
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::server::Socks5::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_unit->server.bandwidth(i->first, limiting, bandwidth))
			// Устанавливаем пропускную способность для клиента принадлежащего этому пиру
			return this->_client.bandwidth(i->second.eid, limiting, bandwidth);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для клиента
 *
 * @param eid   идентификатор события клиента
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 *
 */
bool awh::server::Socks5::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если протокол подключения клиента принадлежащего этому пиру является TCP
		if(this->_unit->server.protocol(eid) == event::protocol_t::TCP){
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if((i != this->_peers.end()) && this->_unit->server.keepAlive(i->first, cnt, idle, intvl))
				// Устанавливаем параметры жизни подключения для клиента принадлежащего этому пиру
				return this->_client.keepAlive(i->second.eid, cnt, idle, intvl);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, cnt, idle, intvl), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
 *
 * @return результат выполнения установки
 *
 */
bool awh::server::Socks5::membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
 *
 * @return результат выполнения установки
 *
 */
bool awh::server::Socks5::membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::server::Socks5::clusterSend(const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем размер флага для отправки данных
		const size_t length = sizeof(cluster_message_t);
		// Устанавливаем размер буфера полезной нагрузки для отправки
		::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
		// Если размер буфера полезной нагрузки достаточно для отправки всех данных
		if(::__awh_size__ == (size + length)){
			// Устанавливаем тип сообщения для отправки данных
			const cluster_message_t message = cluster_message_t::EXTERNAL;
			// Копируем данные запроса в буфер полезной нагрузки
			::memcpy(&::__awh_buffer__[0], &message, length);
			// Добавляем к буферу данных для отправки полезную нагрузку
			::memcpy(&::__awh_buffer__[length], buffer, size);
			// Отправляем данные родительскому процессу
			return this->_unit->server.clusterSend(::__awh_buffer__, ::__awh_size__);
		// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Message sent is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Message sent is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::server::Socks5::clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем размер флага для отправки данных
		const size_t length = sizeof(cluster_message_t);
		// Устанавливаем размер буфера полезной нагрузки для отправки
		::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
		// Если размер буфера полезной нагрузки достаточно для отправки всех данных
		if(::__awh_size__ == (size + length)){
			// Устанавливаем тип сообщения для отправки данных
			const cluster_message_t message = cluster_message_t::EXTERNAL;
			// Копируем данные запроса в буфер полезной нагрузки
			::memcpy(&::__awh_buffer__[0], &message, length);
			// Добавляем к буферу данных для отправки полезную нагрузку
			::memcpy(&::__awh_buffer__[length], buffer, size);
			// Отправляем данные дочернему процессу
			return this->_unit->server.clusterSend(pid, ::__awh_buffer__, ::__awh_size__);
		// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Message sent is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(pid, buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Message sent is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pid, buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::server::Socks5::clusterBroadcast(const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем размер флага для отправки данных
		const size_t length = sizeof(cluster_message_t);
		// Устанавливаем размер буфера полезной нагрузки для отправки
		::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
		// Если размер буфера полезной нагрузки достаточно для отправки всех данных
		if(::__awh_size__ == (size + length)){
			// Устанавливаем тип сообщения для отправки данных
			const cluster_message_t message = cluster_message_t::EXTERNAL;
			// Копируем данные запроса в буфер полезной нагрузки
			::memcpy(&::__awh_buffer__[0], &message, length);
			// Добавляем к буферу данных для отправки полезную нагрузку
			::memcpy(&::__awh_buffer__[length], buffer, size);
			// Отправляем данные всем дочерним процессам
			return this->_unit->server.clusterBroadcast(::__awh_buffer__, ::__awh_size__);
		// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Message sent is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Message sent is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки диапазона портов для выделения портов UDP серверов
 *
 * @param count количество портов для выделения
 * @param begin начальный порт диапазона для выделения
 * @param end   конечный порт диапазона для выделения
 * @param addr  адрес для запуска UDP-серверов
 *
 */
void awh::server::Socks5::udp(const uint16_t count, const uint16_t begin, const uint16_t end, string_view addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если DNS-резолвер или сервер находятся в нерабочем состоянии
		if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
			// Устанавливаем начальный порт диапазона для выделения
			this->_udp.begin = begin;
			// Устанавливаем конечный порт диапазона для выделения
			this->_udp.end = end;
			// Устанавливаем количество портов для выделения
			this->_udp.count = count;
			// Если адрес для запуска UDP-серверов передан
			if(!addr.empty()){
				// Выполняем парсинг IP-адреса
				if(this->_unit->addr.parse(addr))
					// Устанавливаем полученный IP-адрес
					this->_udp.address = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
			// Если адрес для запуска UDP-серверов не передан
			} else {
				/**
				 * Определяем семейство адресов для запуска UDP сервера
				 */
				switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
					// Если процесс работает с адресами IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_udp.address = make_unique <net::addr_net_ipv4_t> ();
					break;
					// Если процесс работает с адресами IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_udp.address = make_unique <net::addr_net_ipv6_t> ();
					break;
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(count, begin, end, addr), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки диапазона портов для выделения портов UDP серверов
 *
 * @param count количество портов для выделения
 * @param begin начальный порт диапазона для выделения
 * @param end   конечный порт диапазона для выделения
 * @param addr  адрес для запуска UDP-серверов
 *
 */
void awh::server::Socks5::udp(const uint16_t count, const uint16_t begin, const uint16_t end, const net::addr_t * addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если DNS-резолвер или сервер находятся в нерабочем состоянии
		if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->server.working()){
			// Устанавливаем начальный порт диапазона для выделения
			this->_udp.begin = begin;
			// Устанавливаем конечный порт диапазона для выделения
			this->_udp.end = end;
			// Устанавливаем количество портов для выделения
			this->_udp.count = count;
			// Если адрес для запуска UDP-серверов передан
			if(addr != nullptr){
				/**
				 * Определяем тип полученного IP-адреса
				 */
				switch(addr->size){
					// Для типа IPv4
					case 4: {
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_udp.address = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем полученный IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (this->_udp.address.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
					} break;
					// Для типа IPv6
					case 16: {
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_udp.address = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем полученный IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_udp.address.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
					} break;
				}
			// Если адрес для запуска UDP-серверов не передан
			} else {
				/**
				 * Определяем семейство адресов для запуска UDP сервера
				 */
				switch(static_cast <uint8_t> (this->_unit->server.family(this->_id.eid))){
					// Если процесс работает с адресами IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_udp.address = make_unique <net::addr_net_ipv4_t> ();
					break;
					// Если процесс работает с адресами IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_udp.address = make_unique <net::addr_net_ipv6_t> ();
					break;
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(count, begin, end), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки алиаса для внутреннего адреса при работе за NAT
 *
 * @param addr  объект параметров подключения внутреннего адреса
 * @param alias объект параметров подключения алиаса для внутреннего адреса
 *
 */
void awh::server::Socks5::setAlias(const net::attr_t * addr, const net::attr_t * alias) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если адреса для установки алиаса переданы
		if((addr != nullptr) && (alias != nullptr)){
			// Объект параметров подключения
			unique_ptr <net::attr_t> attr = nullptr;
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(static_cast <uint8_t> (addr->type)){
				// Для типа IPv4
				case static_cast <uint8_t> (net::type_t::IPV4): {
					// Создаём объект параметров подключения
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения
					attr->type = net::type_t::IPV4;
					// Создаём новый объект адреса IPv4
					awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = awh_cast <const net::attr_net_t *> (addr)->port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (addr)->ip.get())->address;
				} break;
				// Для типа IPv6
				case static_cast <uint8_t> (net::type_t::IPV6): {
					// Создаём объект параметров подключения
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения
					attr->type = net::type_t::IPV6;
					// Создаём новый объект адреса клиента IPv6
					awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = awh_cast <const net::attr_net_t *> (addr)->port;
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (addr)->ip.get())->address[0], 16);
				} break;
			}
			// Если объект параметров подключения успешно создан
			if(attr != nullptr){
				// Создаём идентификатор конечной точки для добавляемого адреса
				const origin_t endpoint = origin_t().from(attr.get());
				// Выполняем добавление идентификатора конечной точки в список алиасов
				auto ret = this->_aliases.emplace(endpoint, nullptr);
				/**
				 * Определяем тип полученного IP-адреса
				 */
				switch(static_cast <uint8_t> (alias->type)){
					// Для типа FQDN
					case static_cast <uint8_t> (net::type_t::FQDN): {
						// Создаём объект параметров подключения для алиаса
						ret.first->second = make_unique <net::attr_fqdn_t> ();
						// Устанавливаем тип параметров подключения
						ret.first->second->type = net::type_t::FQDN;
						// Устанавливаем полученный порт
						awh_cast <net::attr_fqdn_t *> (ret.first->second.get())->port = awh_cast <const net::attr_fqdn_t *> (alias)->port;
						// Устанавливаем полученное доменное имя
						awh_cast <net::attr_fqdn_t *> (ret.first->second.get())->domain = awh_cast <const net::attr_fqdn_t *> (alias)->domain;
					} break;
					// Для типа IPv4
					case static_cast <uint8_t> (net::type_t::IPV4): {
						// Создаём объект параметров подключения для алиаса
						ret.first->second = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения
						ret.first->second->type = net::type_t::IPV4;
						// Создаём новый объект адреса IPv4
						awh_cast <net::attr_net_t *> (ret.first->second.get())->ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (ret.first->second.get())->port = awh_cast <const net::attr_net_t *> (alias)->port;
						// Устанавливаем полученный IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (ret.first->second.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (alias)->ip.get())->address;
					} break;
					// Для типа IPv6
					case static_cast <uint8_t> (net::type_t::IPV6): {
						// Создаём объект параметров подключения для алиаса
						ret.first->second = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения
						ret.first->second->type = net::type_t::IPV6;
						// Создаём новый объект адреса IPv6
						awh_cast <net::attr_net_t *> (ret.first->second.get())->ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (ret.first->second.get())->port = awh_cast <const net::attr_net_t *> (alias)->port;
						// Устанавливаем полученный IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (ret.first->second.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (alias)->ip.get())->address[0], 16);
					} break;
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки алиаса для внутреннего адреса при работе за NAT
 *
 * @param addr    внутренний адрес работающий за NAT
 * @param intPort порт внутреннего адреса работающий за NAT
 * @param alias   внешний адрес для алиаса внутреннего адреса
 * @param extPort внешний порт для алиаса внутреннего адреса
 *
 */
void awh::server::Socks5::setAlias(string_view addr, const uint16_t intPort, string_view alias, const uint16_t extPort) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если адреса для установки алиаса переданы
		if(!addr.empty() && !alias.empty() && (intPort > 0)){
			// Создаём объект параметров подключения
			unique_ptr <net::attr_t> attr = nullptr;
			// Выполняем парсинг переданного адреса
			if(this->_unit->addr.parse(addr)){
				/**
				 * Определяем тип полученного IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_unit->addr.type())){
					// Для типа IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Создаём объект параметров подключения
						attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения
						attr->type = net::type_t::IPV4;
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (attr.get())->port = intPort;
						// Устанавливаем полученный IP-адрес
						awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
					} break;
					// Для типа IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Создаём объект параметров подключения
						attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения
						attr->type = net::type_t::IPV6;
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (attr.get())->port = intPort;
						// Устанавливаем полученный IP-адрес
						awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
					} break;
				}
			}
			// Если объект параметров подключения создан
			if(attr != nullptr){
				// Создаём идентификатор конечной точки для добавляемого адреса
				const origin_t endpoint = origin_t().from(attr.get());
				// Выполняем добавление идентификатора конечной точки в список алиасов
				auto ret = this->_aliases.emplace(endpoint, nullptr);
				// Выполняем парсинг алиаса для внутреннего адреса
				if(this->_unit->addr.parse(alias)){
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_unit->addr.type())){
						// Для типа FQDN
						case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
							// Создаём объект параметров подключения
							ret.first->second = make_unique <net::attr_fqdn_t> ();
							// Устанавливаем тип параметров подключения
							ret.first->second->type = net::type_t::FQDN;
							// Устанавливаем полученный порт
							awh_cast <net::attr_fqdn_t *> (ret.first->second.get())->port = extPort;
							// Устанавливаем полученный доменное имя хоста для подключения
							awh_cast <net::attr_fqdn_t *> (ret.first->second.get())->domain = alias;
						} break;
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
							// Создаём объект параметров подключения для алиаса
							ret.first->second = make_unique <net::attr_net_t> ();
							// Устанавливаем тип параметров подключения
							ret.first->second->type = net::type_t::IPV4;
							// Устанавливаем полученный порт
							awh_cast <net::attr_net_t *> (ret.first->second.get())->port = extPort;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::attr_net_t *> (ret.first->second.get())->ip = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Создаём объект параметров подключения для алиаса
							ret.first->second = make_unique <net::attr_net_t> ();
							// Устанавливаем тип параметров подключения
							ret.first->second->type = net::type_t::IPV6;
							// Устанавливаем полученный порт
							awh_cast <net::attr_net_t *> (ret.first->second.get())->port = extPort;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::attr_net_t *> (ret.first->second.get())->ip = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
						} break;
					}
				// Если распарсить адрес не удалось, значит будем считать, что это FQDN
				} else {
					// Создаём объект параметров подключения
					ret.first->second = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем тип параметров подключения
					ret.first->second->type = net::type_t::FQDN;
					// Устанавливаем полученный порт
					awh_cast <net::attr_fqdn_t *> (ret.first->second.get())->port = extPort;
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (ret.first->second.get())->domain = alias;
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr, intPort, alias, extPort), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::server::Socks5::Socks5(const fmk_t * fmk, const log_t * log) noexcept :
 server_t(fmk, log), _eth(fmk, log), _client(fmk, log), _socks5(fmk, log) {
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_client.on <void (const event::id_t, const bool)> ("connect", &server::socks5_t::connectClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_client.on <void (const event::id_t, const event::status_t)> ("state", &server::socks5_t::statusClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения данных от клиента
	this->_client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &server::socks5_t::readClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &server::socks5_t::errorClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_client.on <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server::socks5_t::timeoutClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие кластерных событий сервера
	this->_unit->server.on <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", &server::socks5_t::eventsCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения кластерного сообщения
	this->_unit->server.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server::socks5_t::messageCluster, this, _1, _2, _3);
}
/**
 * @brief Конструктор
 *
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::server::Socks5::Socks5(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 server_t(dns, fmk, log), _eth(fmk, log), _client(fmk, log), _socks5(fmk, log) {
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_client.on <void (const event::id_t, const bool)> ("connect", &server::socks5_t::connectClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_client.on <void (const event::id_t, const event::status_t)> ("state", &server::socks5_t::statusClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения данных от клиента
	this->_client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &server::socks5_t::readClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &server::socks5_t::errorClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_client.on <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server::socks5_t::timeoutClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие кластерных событий сервера
	this->_unit->server.on <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", &server::socks5_t::eventsCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения кластерного сообщения
	this->_unit->server.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server::socks5_t::messageCluster, this, _1, _2, _3);
}
/**
 * @brief Деструктор
 *
 */
awh::server::Socks5::~Socks5() noexcept {}
