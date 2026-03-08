/**
 * @file: ntp.cpp
 * @date: 2026-03-05
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <array>
#include <chrono>
#include <vector>
#include <random>
#include <cerrno>
#include <cstdint>
#include <string_view>

/**
 * Системные модули
 */
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * Подключаем заголовочный файл модуля
 */
#include <units/ntp.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
 */
using namespace placeholders;

/**
 * Если стандартные NTP-серверы не установлены
 */
#ifndef AWH_NTP_SERVERS
	/**
	 * Устанавливаем стандартные NTP-серверы
	 */
	#define AWH_NTP_SERVERS \
		"ntp1.NL.net", \
		"ntp2.NL.net", \
		"pool.ntp.org", \
		"ntp.msk-ix.ru", \
		"0.pool.ntp.org", \
		"1.pool.ntp.org", \
		"2.pool.ntp.org", \
		"3.pool.ntp.org", \
		"ntp.ubuntu.com", \
		"time.ntp.org.ua", \
		"pool.ntp.org.ua", \
		"0.ru.pool.ntp.org", \
		"1.ru.pool.ntp.org", \
		"europe.pool.ntp.org", \
		"time.cloudflare.com", \
		"ntp0.ntp-servers.net", \
		"ntp1.ntp-servers.net", \
		"ntp2.ntp-servers.net", \
		"ntp3.ntp-servers.net", \
		"ntp5.ntp-servers.net", \
		"ntp6.ntp-servers.net", \
		"ntp7.ntp-servers.net"
#endif

/**
 * Инкапсулируем параметры NTP-серверов в пространство имён
 */
namespace servers {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Общий список NTP-серверов
	 *
	 */
	vector <unique_ptr <net::addr_t>> general;

	/**
	 * @brief Метод добавления NTP-сервера в список
	 *
	 * @param server NTP-сервер для добавления в список
	 */
	static void push(string_view server) noexcept {
		// Создаём объект IP-адреса для параметров NTP-сервера
		struct addrinfo hints = {};
		// Результат получения параметров NTP-сервера
		struct addrinfo * result = nullptr;
		// Устанавливаем семейство протоколов для NTP-сервера (IPv4 + IPv6)
		hints.ai_family = AF_UNSPEC;
		// Устанавливаем тип сокета для NTP-сервера (TCP)
		hints.ai_socktype = SOCK_STREAM;
		/**
		 * Выполняем получение параметров NTP-сервера по его адресу
		 */
		if(::getaddrinfo(server.data(), nullptr, &hints, &result) == 0){
			/**
			 * Выполняем перебор всех полученных параметров NTP-сервера и сохраняем их в общий список NTP-серверов
			 */
			for(auto * p = result; p != nullptr; p = p->ai_next){
				/**
				 * Определяем тип адреса NTP-сервера
				 */
				switch(p->ai_family){
					// Если адрес является IPv4
					case AF_INET: {
						// Выполняем инициализацию объекта IP-адреса
						unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
						// Получаем объект с IPv4-адресом NTP-сервера
						auto * sa = reinterpret_cast <sockaddr_in *> (p->ai_addr);
						// Копируем IPv4-адрес NTP-сервера в объект IP-адреса
						awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = sa->sin_addr.s_addr;
						// Добавляем сервер в общий список NTP-серверов
						general.push_back(::move(ip));
					} break;
					// Если адрес является IPv6
					case AF_INET6: {
						// Выполняем инициализацию объекта IP-адреса
						unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
						// Получаем объект с IPv6-адресом NTP-сервера
						auto * sa = reinterpret_cast <sockaddr_in6 *> (p->ai_addr);
						// Копируем IPv6-адрес NTP-сервера в объект IP-адреса
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &sa->sin6_addr.s6_addr[0], 16);
						// Добавляем сервер в общий список NTP-серверов
						general.push_back(::move(ip));
					} break;
				}
			}
			// Освобождаем память, выделенную для хранения параметров NTP-сервера
			::freeaddrinfo(result);
		}
	}
};

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для рандомизации NTP-серверов
	 *
	 */
	random_device __awh_randev__;
	/**
	 * @brief Мютекс для блокировки потока
	 *
	 */
	lock_state_t <std::mutex> __awh_mtx__;
	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;
};

/**
 * Инкапсулируем структуры протокола NTP в собственное пространство имён
 */
namespace ntp {
	/**
	 * @brief Структура пакетов NTP-запроса
	 *
	 */
	typedef struct Packet {
		/**
		 * Восемь бит (li, vn и mode):
		 * li:   Два бита, индикатор прыжка
		 * vn:   Три бита, номер версии протокола
		 * mode: Три бита, клиент выберет режим 3 для клиента
		 */
		uint8_t mode;
		// Уровень страты местных часов
		uint8_t stratum;
		// Максимальный интервал между последовательными сообщениями
		uint8_t poll;
		// Точность местных часов
		uint8_t precision;
		// Общее время задержки туда и обратно
		uint32_t rootDelay;
		// Максимально-ощутимая ошибка от основного источника синхронизации
		uint32_t rootDispersion;
		// Идентификатор опорных часов
		uint32_t refId;
		// Отметка времени в секундах
		uint32_t refTimeStampSec;
		// Эталонная отметка времени в долях секунды
		uint32_t refTimeStampSecFrac;
		// Исходная метка времени в секундах
		uint32_t origTimeStampSec;
		// Исходная временная метка в долях секунды
		uint32_t origTimeStampSecFrac;
		// Полученная метка времени в секундах
		uint32_t receivedTimeStampSec;
		// Полученная временная метка в долях секунды
		uint32_t receivedTimeStampSecFrac;
		// Передача метки времени в секундах
		uint32_t transmitedTimeStampSec;
		// Отметки времени в долях секунды для передачи
		uint32_t transmitedTimeStampSecFrac;
		/**
		 * @brief Конструктор
		 *
		 */
		Packet() noexcept :
		 mode(0), stratum(0), poll(0), precision(0),
		 rootDelay(htonl(0)), rootDispersion(htonl(0)), refId(htonl(0)),
		 refTimeStampSec(htonl(0)), refTimeStampSecFrac(htonl(0)),
		 origTimeStampSec(htonl(0)), origTimeStampSecFrac(htonl(0)),
		 receivedTimeStampSec(htonl(0)), receivedTimeStampSecFrac(htonl(0)),
		 transmitedTimeStampSec(htonl(0)), transmitedTimeStampSecFrac(htonl(0)) {}
	} __attribute__((packed)) packet_t;

	/**
	 * @brief Функция получения времени в формате NTP (секунды с 1900 года)
	 *
	 * @return текущее время в формате NTP
	 */
	static uint32_t timesec() noexcept {
		// Получаем текущее время в секундах с 1 января 1970 года (Unix epoch)
		auto now = chrono::system_clock::now();
		// Получаем количество секунд, прошедших с 1 января 1970 года (Unix epoch)
		auto epoch = now.time_since_epoch();
		// Получаем количество секунд, прошедших с 1 января 1900 года (NTP epoch)
		auto seconds = chrono::duration_cast <chrono::seconds> (epoch).count();
		/**
		 * NTP epoch = 1 Jan 1900, Unix epoch = 1 Jan 1970
		 * Разница: 70 лет + 17 високосных дней = 2208988800 секунд
		 */
		return htonl(static_cast <uint32_t> (seconds + 2208988800ULL));
	}
};

/**
 * @brief Метод инициализации списка NTP-серверов из переменных окружения или стандартных значений
 *
 */
void awh::unit::NTP::Servers::init() noexcept {
	/**
	 * Выполняем перебор всех общих серверов
	 */
	for(const auto & server : ::servers::general){
		/**
		 * Определяем тип адреса
		 */
		switch(server->size){
			// Если адрес является IPv4
			case 4: {
				// Активируем флаг инициализации списка NTP-серверов IPv4
				this->_initializedIPv4 = true;
				// Создаём объект IP-адреса для хранения IPv4-адреса
				unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
				// Копируем IP-адрес из NTP-сервера в объект IP-адреса
				awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (server.get())->address;
				// Добавляем сервер в список NTP-серверов для выполнения запросов
				this->_ipv4.push_back(::move(ip));
			} break;
			// Если адрес является IPv6
			case 16: {
				// Активируем флаг инициализации списка NTP-серверов IPv6
				this->_initializedIPv6 = true;
				// Создаём объект IP-адреса для хранения IPv6-адреса
				unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
				// Копируем IP-адрес из NTP-сервера в объект IP-адреса
				::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (server.get())->address[0], 16);
				// Добавляем сервер в список NTP-серверов для выполнения запросов
				this->_ipv6.push_back(::move(ip));
			} break;
		}
	}
}
/**
 * @brief Метод сброса списка NTP-серверов
 *
 * @param family семейство IP-адресов IPv4/IPv6
 */
void awh::unit::NTP::Servers::reset(const event::family_t family) noexcept {
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Сбрасываем индекс текущего NTP-сервера
			this->_indexIPv4 = 0;
			// Выполняем очистку списка NTP-серверов
			this->_ipv4.clear();
			/**
			 * Выполняем перебор всех общих серверов
			 */
			for(const auto & server : ::servers::general){
				// Если адрес является IPv4
				if(server->size == 4){
					// Активируем флаг инициализации списка NTP-серверов IPv4
					this->_initializedIPv4 = true;
					// Создаём объект IP-адреса для хранения IPv4-адреса
					unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
					// Копируем IP-адрес из NTP-сервера в объект IP-адреса
					awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (server.get())->address;
					// Добавляем сервер в список NTP-серверов для выполнения запросов
					this->_ipv4.push_back(::move(ip));
				}
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Сбрасываем индекс текущего NTP-сервера
			this->_indexIPv6 = 0;
			// Выполняем очистку списка NTP-серверов
			this->_ipv6.clear();
			/**
			 * Выполняем перебор всех общих серверов
			 */
			for(const auto & server : ::servers::general){
				// Если адрес является IPv6
				if(server->size == 16){
					// Активируем флаг инициализации списка NTP-серверов IPv6
					this->_initializedIPv6 = true;
					// Создаём объект IP-адреса для хранения IPv6-адреса
					unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
					// Копируем IP-адрес из NTP-сервера в объект IP-адреса
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (server.get())->address[0], 16);
					// Добавляем сервер в список NTP-серверов для выполнения запросов
					this->_ipv6.push_back(::move(ip));
				}
			}
		} break;
	}
}
/**
 * @brief Метод получения текущего NTP-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @return       объект NTP-сервера для выполнения запроса
 */
const awh::net::addr_t * awh::unit::NTP::Servers::get(const event::family_t family) noexcept {
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Если список NTP-серверов не пустой
			if(!this->_ipv4.empty()){
				// Получаем текущий NTP-сервер из списка по индексу
				const net::addr_t * server = this->_ipv4[this->_indexIPv4].get();
				// Увеличиваем индекс для следующего запроса, циклически возвращаясь к началу списка при достижении конца
				this->_indexIPv4 = ((this->_indexIPv4 + 1) % this->_ipv4.size());
				// Возвращаем текущий NTP-сервер для выполнения запроса
				return server;
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Если список NTP-серверов не пустой
			if(!this->_ipv6.empty()){
				// Получаем текущий NTP-сервер из списка по индексу
				const net::addr_t * server = this->_ipv6[this->_indexIPv6].get();
				// Увеличиваем индекс для следующего запроса, циклически возвращаясь к началу списка при достижении конца
				this->_indexIPv6 = ((this->_indexIPv6 + 1) % this->_ipv6.size());
				// Возвращаем текущий NTP-сервер для выполнения запроса
				return server;
			}
		} break;
	}
	// Возвращаем пустой результат
	return nullptr;
}
/**
 * @brief Метод добавления NTP-сервера в список
 *
 * @param server объект NTP-сервера для добавления в список
 */
void awh::unit::NTP::Servers::push(const net::addr_t * server) noexcept {
	/**
	 * Определяем тип адреса
	 */
	switch(server->size){
		// Если адрес является IPv4
		case 4: {
			// Если список NTP-серверов IPv4 уже инициализирован
			if(this->_initializedIPv4){
				// Снимаем флаг инициализации списка NTP-серверов IPv4, так как мы будем его переинициализировать
				this->_initializedIPv4 = false;
				// Сбрасываем индекс текущего NTP-сервера
				this->_indexIPv4 = 0;
				// Выполняем очистку списка NTP-серверов
				this->_ipv4.clear();
			}
			// Создаём объект IP-адреса для хранения IPv4-адреса
			unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
			// Копируем IP-адрес из NTP-сервера в объект IP-адреса
			awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (server)->address;
			// Добавляем сервер в список NTP-серверов для выполнения запросов
			this->_ipv4.push_back(::move(ip));
		} break;
		// Если адрес является IPv6
		case 16: {
			// Если список NTP-серверов IPv6 уже инициализирован
			if(this->_initializedIPv6){
				// Снимаем флаг инициализации списка NTP-серверов IPv6, так как мы будем его переинициализировать
				this->_initializedIPv6 = false;
				// Сбрасываем индекс текущего NTP-сервера
				this->_indexIPv6 = 0;
				// Выполняем очистку списка NTP-серверов
				this->_ipv6.clear();
			}
			// Создаём объект IP-адреса для хранения IPv6-адреса
			unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
			// Копируем IP-адрес из NTP-сервера в объект IP-адреса
			::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (server)->address[0], 16);
			// Добавляем сервер в список NTP-серверов для выполнения запросов
			this->_ipv6.push_back(::move(ip));
		} break;
	}
}
/**
 * @brief Конструктор
 *
 */
awh::unit::NTP::Servers::Servers() noexcept :
 _indexIPv4(0), _indexIPv6(0),
 _initializedIPv4(false), _initializedIPv6(false) {}

/**
 * @brief Метод создания события NTP-клиента
 *
 * @param family семейство протоколов (например: IPv4 или IPv6)
 */
void awh::unit::NTP::create(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока для создания события NTP-клиента
		const locker_t <> lock(this->_client.mtx);
		// Добавляем новое событие клиента UDP
		this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_client.eid, static_cast <event::callback::error_t> (std::bind(&ntp_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_client.eid, static_cast <event::callback::read_t> (std::bind(&ntp_t::response, this, _1, _2, _3)));
		// Если опции события не установлены
		if(!this->_io->setOptions(this->_client.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
			// Удаляем событие NTP-клиента
			this->_io->destroy(this->_client.eid);
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to set options for NTP-client event", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to set options for NTP-client event", log_t::flag_t::CRITICAL);
				#endif
			}
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки ошибок событий NTP-клиента
 *
 * @param eid         идентификатор события NTP-клиента
 * @param error       код ошибки события NTP-клиента
 * @param description описание ошибки события NTP-клиента
 */
void awh::unit::NTP::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если событие относится к NTP-клиенту
	if(eid == this->_client.eid)
		// Выполняем сброс NTP-клиента
		this->reset();
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки ответов от NTP-сервера на запросы NTP-клиента
 *
 * @param eid  идентификатор события чтения из NTP-клиента
 * @param data данные события чтения из NTP-клиента
 * @param size размер данных события чтения из NTP-клиента
 */
void awh::unit::NTP::response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с контейнером активных пакетов
			const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск активного пакета в контейнере активных пакетов
			auto i = this->_transfer.waiting.find(this->_client.eid);
			// Если активный пакет найден в контейнере активных пакетов
			if(i != this->_transfer.waiting.end()){
				// Выполняем блокировку потока для уничтожения события
				const locker_t <> lock(this->_client.mtx);
				// Удаляем событие таймера для ожидания ответа от NTP-сервера
				this->_io->destroy(i->second.eid);
				// Удаляем активный пакет из контейнера активных пакетов
				this->_transfer.waiting.erase(i);
			}
		}
		// Если функция обратного вызова установлена для синхронизации с NTP-сервером
		if(this->_callback.is("timestamp")){
			// Если данные события чтения из DNS-резолвера не пустые
			if((data != nullptr) && (size > 0)){
				// Получаем объект заголовка запроса
				const ::ntp::packet_t * packet = reinterpret_cast <const ::ntp::packet_t *> (data);
				/**
				 * 1. Читаем время отправки ответа сервером (T3) и конвертируем из сетевого порядка байтов в порядок байтов хоста
				 */
				uint32_t sec  = ntohl(packet->transmitedTimeStampSec);
				uint32_t frac = ntohl(packet->transmitedTimeStampSecFrac);
				/**
				 * 2. Конвертируем эпоху NTP (1900) в Unix (1970) и получаем миллисекунды
				 * NTP epoch = 1 Jan 1900, Unix epoch = 1 Jan 1970
				 * Разница: 70 лет + 17 високосных дней = 2208988800 секунд
				 * 2208988800 = разница в секундах между 1900 и 1970 годами
				 */
				constexpr uint64_t NTP_TO_UNIX_EPOCH = 2208988800ULL;
				/** 
				 * Получаем время в миллисекундах, вычитая секунды с 1900 года из секунд с 1970 года и умножая на 1000 для получения миллисекунд
				 */
				uint64_t timestamp = ((static_cast <uint64_t> (sec) - NTP_TO_UNIX_EPOCH) * 1000ULL);
				/**
				 * 3. Добавляем дробную часть для точности до миллисекунд (опционально)
				 * 2^32 / 1000 ≈ 4294967
				 */
				timestamp += (static_cast <uint64_t> (frac) / 4294967ULL);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t)> ("timestamp", timestamp);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки событий таймаута при ожидании ответа от NTP-сервера
 *
 * @param eid    идентификатор таймера NTP-клиента
 * @param status статус события таймера NTP-клиента
 * @param packet объект активного пакета при выполнении запроса NTP-клиента
 */
void awh::unit::NTP::timeout(const event::id_t eid, const event::status_t status, packet_t * packet) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		// Запоминаем идентификатор клиента
		const event::id_t eid = this->_client.eid;
		// Получаем семейство IP-адресов текущего события NTP-клиента
		const event::family_t family = this->_io->family(eid);
		{
			// Выполняем блокировку потока для уничтожения события NTP-клиента
			const locker_t <> lock(this->_client.mtx);
			// Удаляем событие NTP-клиента
			this->_io->destroy(eid);
		}
		// Выполняем создание события NTP-клиента для указанного семейства IP-адресов
		this->create(family);
		// Если попытки резолвинга не превышают максимально допустимое количество
		if(packet->attempt < this->_transfer.attempts){
			// Выполняем фиксацию параметров NTP-клиента
			if(this->commit()){
				// Сохраняем время ожидания ответа от NTP-сервера (в миллисекундах)
				const uint32_t delay = packet->delay;
				// Сохраняем количество попыток получения ответа от NTP-сервера
				const uint8_t attempt = packet->attempt;
				// Сохраняем версию протокола NTP для текущего запроса
				const version_t version = packet->version;
				{
					// Выполняем блокировку потока для работы с контейнером активных пакетов
					const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Выполняем поиск активного пакета в контейнере активных пакетов
					auto i = this->_transfer.waiting.find(eid);
					// Если активный пакет найден в контейнере активных пакетов
					if(i != this->_transfer.waiting.end())
						// Удаляем активный пакет из контейнера активных пакетов
						this->_transfer.waiting.erase(i);
					// Добавляем новый пакет в контейнер ожидания выполнения запроса для отслеживания его выполнения
					auto ret = this->_transfer.waiting.emplace(this->_client.eid, packet_t());
					// Если пакет уже существует для данного идентификатора NTP-клиента
					if(!ret.second){
						// Формируем текст выводимой ошибки NTP-клиента
						const string error = "NTP request is still in progress, please wait for the result";
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_client.eid, event::error_t::INVALID, error);
						// Если функция вывода ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
						// Выходим из функции
						return;
					// Если пакет успешно добавлен для данного идентификатора NTP-клиента
					} else {
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Добавляем новое событие таймаута для ожидания ответа от NTP-сервера
						const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
						// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от NTP-сервера
						this->_io->setTimeout(tid, event::action_t::NONE, (delay > 0 ? delay : 5000));
						// Устанавливаем функцию обратного вызова на событие получения ошибок
						this->_io->on(tid, static_cast <event::callback::error_t> (std::bind(&ntp_t::error, this, _1, _2, _3)));
						// Если не удалось установить таймер для ожидания ответа от NTP-сервера
						if(!this->_io->commit(tid)){
							// Удаляем событие таймера
							this->_io->destroy(tid);
							// Удаляем активный пакет из контейнера активных пакетов
							this->_transfer.waiting.erase(this->_client.eid);
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Failed to commit NTP-client timeout", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Failed to commit NTP-client timeout", log_t::flag_t::CRITICAL);
								#endif
							}
							// Выходим из функции
							return;
						// Если таймер для ожидания ответа от NTP-сервера успешно установлен
						} else {
							// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
							ret.first->second.eid = tid;
							// Устанавливаем время жизни пакета для отслеживания его выполнения
							ret.first->second.delay = delay;
							// Устанавливаем версию протокола NTP для выполнения запроса
							ret.first->second.version = version;
							// Устанавливаем количество попыток получения ответа от NTP-сервера
							ret.first->second.attempt = (attempt + 1);
							// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от NTP-сервера
							this->_io->on(tid, static_cast <event::callback::status_t> (std::bind(&ntp_t::timeout, this, _1, _2, &ret.first->second)));
							// Запускаем таймер для ожидания ответа от NTP-сервера
							this->_io->launch(tid);
						}
					}
				}
				// Создаём объект пакета запроса
				::ntp::packet_t packet{};
				/**
				 * Определяем версию протокола NTP для выполнения запроса
				 */
				switch(static_cast <uint8_t> (version)){
					// Если версия протокола NTPv1
					case 0x01: packet.mode = 0x0B; break;
					// Если версия протокола NTPv2
					case 0x02: packet.mode = 0x13; break;
					// Если версия протокола NTPv3
					case 0x03: packet.mode = 0x1B; break;
					// Если версия протокола NTPv4
					case 0x04: packet.mode = 0x23; break;
				}
				// Устанавливаем версию протокола NTP
				packet.origTimeStampSec = ::ntp::timesec();
				// Отправляем запрос на NTP-сервер для синхронизации времени
				this->_io->send(this->_client.eid, &packet, sizeof(packet));
			}
		// Если попытки резолвинга превышают максимально допустимое количество
		} else {
			{
				// Если функция обратного вызова установлена
				if(this->_callback.is("attempts"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const uint8_t)> ("attempts", packet->attempt);
				// Если функция обратного вызова не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"NTP-client timeout (attempts: %u)",
							__PRETTY_FUNCTION__,
							std::make_tuple(eid, static_cast <uint16_t> (status)),
							log_t::flag_t::WARNING, packet->attempt
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("NTP-client timeout (attempts: %u)", log_t::flag_t::WARNING, packet->attempt);
					#endif
				}
				// Выполняем блокировку потока для работы с контейнером активных пакетов
				const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Выполняем поиск активного пакета в контейнере активных пакетов
				auto i = this->_transfer.waiting.find(eid);
				// Если активный пакет найден в контейнере активных пакетов
				if(i != this->_transfer.waiting.end())
					// Удаляем активный пакет из контейнера активных пакетов
					this->_transfer.waiting.erase(i);
			}
			// Выполняем фиксацию параметров NTP-клиента
			this->commit();
		}
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::unit::NTP::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с IP-адресами
	::__awh_mtx__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с NTP-клиентом
	this->_client.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с контейнером активных пакетов
	this->_transfer.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод установки количества попыток получения ответа от NTP-сервера
 *
 * @param attempts количество попыток получения ответа от NTP-сервера
 */
void awh::unit::NTP::setAttempts(const uint8_t attempts) noexcept {
	// Выполняем блокировку потока для работы с контейнером активных пакетов
	const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Устанавливаем количество попыток получения ответов от NTP-сервера
	this->_transfer.attempts = attempts;
}
/**
 * @brief Метод установки префикса переменной окружения
 *
 * @param prefix префикс переменной окружения для установки
 */
void awh::unit::NTP::setPrefixEnvironment(string_view prefix) noexcept {
	// Выполняем блокировку потока для установки IP-адреса события
	const locker_t <> lock(this->_client.mtx);
	// Если префикс переменной окружения передан
	if(!prefix.empty())
		// Устанавливаем префикс переменной окружения
		this->_client.prefix = this->_fmk->transform(prefix, fmk_t::transform_t::UPPER_CASE);
	// Если префикс переменной окружения не передан, очищаем префикс переменной окружения
	else this->_client.prefix.clear();
}
/**
 * @brief Метод сброса NTP-клиента
 *
 * @return результат выполнения операции
 */
bool awh::unit::NTP::reset() noexcept {
	// Получаем семейство IP-адресов текущего события NTP-клиента
	const event::family_t family = this->_io->family(this->_client.eid);
	{
		// Выполняем блокировку потока для уничтожения события NTP-клиента
		const locker_t <> lock(this->_client.mtx);
		// Удаляем событие NTP-клиента
		this->_io->destroy(this->_client.eid);
	}
	// Выполняем создание события NTP-клиента для указанного семейства IP-адресов
	this->create(family);
	// Выполняем фиксацию параметров NTP-клиента
	return this->commit();
}
/**
 * @brief Метод фиксации параметров NTP-клиента
 *
 * @return результат выполнения операции
 */
bool awh::unit::NTP::commit() noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока для установки IP-адреса события
		const locker_t <> lock(this->_client.mtx);
		// Устанавливаем порт события
		this->_io->setPort(this->_client.eid, this->_client.port);
		// Получаем семейство IP-адресов текущего события NTP-клиента
		const event::family_t family = this->_io->family(this->_client.eid);
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если префикс для переменных окружения установлен
				if(!this->_client.prefix.empty()){
					// Получаем значение переменной
					const char * env = ::getenv(this->_fmk->format("%s_NTP_IPV4_SERVER", this->_client.prefix.c_str()).c_str());
					// Если IP-адрес из переменной окружения получен
					if(env != nullptr){
						// Устанавливаем адрес сервера назначения
						this->_io->setTarget(this->_client.eid, env);
						// Выходим из условия
						break;
					}
				}
				// Устанавливаем адрес сервера назначения
				this->_io->setTarget(this->_client.eid, this->_client.servers.get(family));
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если префикс для переменных окружения установлен
				if(!this->_client.prefix.empty()){
					// Получаем значение переменной
					const char * env = ::getenv(this->_fmk->format("%s_NTP_IPV6_SERVER", this->_client.prefix.c_str()).c_str());
					// Если IP-адрес из переменной окружения получен
					if(env != nullptr){
						// Устанавливаем адрес сервера назначения
						this->_io->setTarget(this->_client.eid, env);
						// Выходим из условия
						break;
					}
				}
				// Устанавливаем адрес сервера назначения
				this->_io->setTarget(this->_client.eid, this->_client.servers.get(family));
			} break;
		}
		// Если адрес сети для выполнения запроса установлен
		if(this->_client.source != nullptr){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
					// Устанавливаем IP-адрес события
					this->_io->setAddress(this->_client.eid, event::address_t::IPV4, this->_client.source.get());
				break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
					// Устанавливаем IP-адрес события
					this->_io->setAddress(this->_client.eid, event::address_t::IPV6, this->_client.source.get());
				break;
			}
		}
		// Выполняем фиксацию параметров события и его запуск
		if(!(result = this->_io->commit(this->_client.eid) && this->_io->launch(this->_client.eid))){
			// Удаляем событие NTP-клиента
			this->_io->destroy(this->_client.eid);
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to launch NTP-client", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to launch NTP-client", log_t::flag_t::CRITICAL);
				#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения порта NTP-сервера
 *
 * @return порт NTP-сервера
 */
uint16_t awh::unit::NTP::getPort() const noexcept {
	// Получаем порт события
	return this->_client.port;
}
/**
 * @brief Метод установки порта NTP-сервера
 *
 * @param port порт NTP-сервера для установки
 */
void awh::unit::NTP::setPort(const uint16_t port) noexcept {
	// Если порт для установки передан
	if(port > 0){
		// Выполняем блокировку потока для установки порта события
		const locker_t <> lock(this->_client.mtx);
		// Устанавливаем порт события
		this->_client.port = port;
	}
}
/**
 * @brief Метод установки адреса NTP-сервера
 *
 * @param server адрес NTP-сервера для установки
 */
void awh::unit::NTP::setServer(string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес NTP-сервера передан
		if((this->_client.eid > 0) && !server.empty()){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(server)){
				// Выполняем блокировку потока для установки IP-адреса события
				const locker_t <> lock(this->_client.mtx);
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Очищаем список IP-адресов события для семейства IPv4
						this->_client.servers.reset(event::family_t::IPV4);
					break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
						// Очищаем список IP-адресов события для семейства IPv6
						this->_client.servers.reset(event::family_t::IPV6);
					break;
				}
				// Устанавливаем IP-адрес события
				this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
			}
		// Если адрес NTP-сервера не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Очищаем список IP-адресов события для семейства IPv4
			this->_client.servers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейства IPv6
			this->_client.servers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса NTP-сервера
 *
 * @param server адрес NTP-сервера для установки
 */
void awh::unit::NTP::setServer(const net::addr_t * server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес NTP-сервера передан
		if((this->_client.eid > 0) && (server != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv4
					this->_client.servers.reset(event::family_t::IPV4);
					// Устанавливаем IP-адрес события
					this->_client.servers.push(server);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv6
					this->_client.servers.reset(event::family_t::IPV6);
					// Устанавливаем IP-адрес события
					this->_client.servers.push(server);
				} break;
			}
		// Если адрес NTP-сервера не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Очищаем список IP-адресов события для семейства IPv4
			this->_client.servers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейства IPv6
			this->_client.servers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса NTP-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param server адрес NTP-сервера для установки
 */
void awh::unit::NTP::setServer(const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес NTP-сервера передан
		if((this->_client.eid > 0) && !server.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Очищаем список IP-адресов события для семейства IPv4
						this->_client.servers.reset(event::family_t::IPV4);
						// Устанавливаем IP-адрес события
						this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Очищаем список IP-адресов события для семейства IPv6
						this->_client.servers.reset(event::family_t::IPV6);
						// Устанавливаем IP-адрес события
						this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
					}
				} break;
			}
		// Если адрес NTP-сервера не передан
		} else {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv4
					this->_client.servers.reset(event::family_t::IPV4);
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv6
					this->_client.servers.reset(event::family_t::IPV6);
				} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса NTP-сервера
 *
 * @param server адрес NTP-сервера для добавления
 */
void awh::unit::NTP::addServer(string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес NTP-сервера передан
		if((this->_client.eid > 0) && !server.empty()){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(server)){
				// Выполняем блокировку потока для установки IP-адреса события
				const locker_t <> lock(this->_client.mtx);
				// Устанавливаем IP-адрес события
				this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса NTP-сервера
 *
 * @param server адрес NTP-сервера для добавления
 */
void awh::unit::NTP::addServer(const net::addr_t * server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес NTP-сервера передан
		if((this->_client.eid > 0) && (server != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Устанавливаем IP-адрес события
					this->_client.servers.push(server);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Устанавливаем IP-адрес события
					this->_client.servers.push(server);
				} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса NTP-сервера
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param server адрес NTP-сервера для добавления
 */
void awh::unit::NTP::addServer(const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес NTP-сервера передан
		if((this->_client.eid > 0) && !server.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Устанавливаем IP-адрес события
						this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Устанавливаем IP-адрес события
						this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
					}
				} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов NTP-серверов
 *
 * @param server адреса NTP-серверов для установки
 */
void awh::unit::NTP::setServers(const vector <string> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса NTP-серверов переданы
		if((this->_client.eid > 0) && !servers.empty()){
			// Результат выполнения парсинга IP-адреса
			bool result = true;
			// Флаг сброса списка IP-адресов события для семейства IPv4
			bool resetIPv4 = false;
			// Флаг сброса списка IP-адресов события для семейства IPv6
			bool resetIPv6 = false;
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			/**
			 * Проходим по каждому адресу NTP-сервера для проверки
			 */
			for(const auto & server : servers){
				// Выполняем парсинг IP-адреса
				if((result = this->_addr.parse(server))){
					/**
					 * Определяем тип IP-адреса для сброса соответствующего списка
					 */
					switch(static_cast <uint8_t> (this->_addr.type())){
						// Если адрес является IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv4
							resetIPv4 = true;
						break;
						// Если адрес является IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv6
							resetIPv6 = true;
						break;
					}
				// Если парсинг IP-адреса не выполнен, прерываем цикл
				} else break;
			}
			// Если парсинг всех адресов выполнен успешно
			if(result){
				// Выполняем блокировку потока для установки IP-адреса события
				const locker_t <> lock(this->_client.mtx);
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Сбрасываем список IP-адресов события для семейства IPv4
					this->_client.servers.reset(event::family_t::IPV4);
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Сбрасываем список IP-адресов события для семейства IPv6
					this->_client.servers.reset(event::family_t::IPV6);
				/**
				 * Проходим по каждому адресу NTP-сервера для установки
				 */
				for(const auto & server : servers){
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(server))
						// Устанавливаем IP-адрес события
						this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
				}
			}
		// Если адреса NTP-серверов не переданы
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Очищаем список IP-адресов события для семейства IPv4
			this->_client.servers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейства IPv6
			this->_client.servers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов NTP-серверов
 *
 * @param server адреса NTP-серверов для установки
 */
void awh::unit::NTP::setServers(const vector <const net::addr_t *> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса NTP-серверов переданы
		if((this->_client.eid > 0) && !servers.empty()){
			// Флаг сброса списка IP-адресов события для семейства IPv4
			bool resetIPv4 = false;
			// Флаг сброса списка IP-адресов события для семейства IPv6
			bool resetIPv6 = false;
			/**
			 * Проходим по каждому адресу NTP-сервера для установки
			 */
			for(const auto & server : servers){
				// Если адрес NTP-сервера передан
				if(server != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(server->size){
						// Если адрес является IPv4
						case 4:
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv4
							resetIPv4 = true;
						break;
						// Если адрес является IPv6
						case 16:
							// Устанавливаем флаг сброса списка IP-адресов события для семейства IPv6
							resetIPv6 = true;
						break;
					}
				}
			}
			// Если необходимо сбросить список IPv4 или IPv6
			if(resetIPv4 || resetIPv6){
				// Выполняем блокировку потока для установки IP-адреса события
				const locker_t <> lock(this->_client.mtx);
				// Если необходимо сбросить список IPv4
				if(resetIPv4)
					// Сбрасываем список IP-адресов события для семейства IPv4
					this->_client.servers.reset(event::family_t::IPV4);
				// Если необходимо сбросить список IPv6
				if(resetIPv6)
					// Сбрасываем список IP-адресов события для семейства IPv6
					this->_client.servers.reset(event::family_t::IPV6);
				/**
				 * Проходим по каждому адресу NTP-сервера для установки
				 */
				for(const auto & server : servers){
					// Если адрес NTP-сервера передан
					if(server != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(server->size){
							// Если адрес является IPv4
							case 4:
								// Устанавливаем IP-адрес события
								this->_client.servers.push(server);
							break;
							// Если адрес является IPv6
							case 16:
								// Устанавливаем IP-адрес события
								this->_client.servers.push(server);
							break;
						}
					}
				}
			}
		// Если адрес NTP-сервера не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Очищаем список IP-адресов события для семейства IPv4
			this->_client.servers.reset(event::family_t::IPV4);
			// Очищаем список IP-адресов события для семейства IPv6
			this->_client.servers.reset(event::family_t::IPV6);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка адресов NTP-серверов
 *
 * @param family  семейство IP-адресов IPv4/IPv6
 * @param servers адреса NTP-серверов для установки
 */
void awh::unit::NTP::setServers(const event::family_t family, const vector <string> & servers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адреса NTP-серверов переданы
		if((this->_client.eid > 0) && !servers.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv4
					this->_client.servers.reset(event::family_t::IPV4);
					/**
					 * Проходим по каждому адресу NTP-сервера для установки
					 */
					for(const auto & server : servers){
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						// Выполняем парсинг IPv4-адреса
						if(this->_addr.parse(server, net_addr_t::type_t::IPV4))
							// Устанавливаем IP-адрес события
							this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Выходим из цикла
						else break;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv6
					this->_client.servers.reset(event::family_t::IPV6);
					/**
					 * Проходим по каждому адресу NTP-сервера для установки
					 */
					for(const auto & server : servers){
						// Выполняем блокировку потока для парсинга IP-адреса
						const locker_t <> lock(::__awh_mtx__);
						// Выполняем парсинг IPv6-адреса
						if(this->_addr.parse(server, net_addr_t::type_t::IPV6))
							// Устанавливаем IP-адрес события
							this->_client.servers.push(this->_addr.source(net_addr_t::endian_t::LITTLE).get());
						// Выходим из цикла
						else break;
					}
				} break;
			}
		// Если адрес NTP-сервера не передан
		} else {
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv4
					this->_client.servers.reset(family);
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Очищаем список IP-адресов события для семейства IPv6
					this->_client.servers.reset(family);
				} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, static_cast <uint16_t> (family), servers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::NTP::setSource(string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((this->_client.eid > 0) && !source.empty()){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(source)){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					} break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					} break;
				}
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Сбрасываем IP-адрес события
			this->_client.source.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::NTP::setSource(const net::addr_t * source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((this->_client.eid > 0) && (source != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(source->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Выполняем инициализацию объекта IP-адреса
					this->_client.source = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_client.source.get())->address = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Выполняем инициализацию объекта IP-адреса
					this->_client.source = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_client.source.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], 16);
				} break;
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Сбрасываем IP-адрес события
			this->_client.source.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::NTP::setSource(const event::family_t family, string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((this->_client.eid > 0) && !source.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(source, net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(source, net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					}
				} break;
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Сбрасываем IP-адрес события
			this->_client.source.reset(nullptr);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, static_cast <uint16_t> (family), source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод синхронизации времени с NTP-сервером
 *
 * @param version версия протокола NTP для выполнения запроса
 * @param timeout время ожидания ответа от NTP-сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::NTP::sync(const version_t version, const uint32_t timeout) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если функция обратного вызова установлена для синхронизации с NTP-сервером
		if(this->_callback.is("timestamp")){
			{
				// Выполняем блокировку потока для работы с контейнером активных пакетов
				const locker_t <std::shared_mutex> lock(this->_transfer.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Добавляем новый активный пакет для ожидания выполнения запроса к NTP-серверу в контейнер активных пакетов
				auto ret = this->_transfer.waiting.emplace(this->_client.eid, packet_t());
				// Если активный пакет уже существует для данного идентификатора NTP-клиента
				if(!ret.second){
					// Формируем текст выводимой ошибки NTP-клиента
					const string error = "NTP request is still in progress, please wait for the result";
					// Если функция обратного вызова установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_client.eid, event::error_t::INVALID, error);
					// Если функция вывода ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(timeout), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					}
					// Выводим результат по умолчанию
					return false;
				// Если таймаут успешно добавлен для данного идентификатора NTP-клиента
				} else {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Добавляем новое событие таймаута для ожидания ответа от NTP-сервера
					const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
					// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от NTP-сервера
					this->_io->setTimeout(tid, event::action_t::NONE, (timeout > 0 ? timeout : 5000));
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(tid, static_cast <event::callback::error_t> (std::bind(&ntp_t::error, this, _1, _2, _3)));
					// Если не удалось установить таймер для ожидания ответа от NTP-сервера
					if(!this->_io->commit(tid)){
						// Удаляем событие таймера
						this->_io->destroy(tid);
						// Удаляем активный пакет из контейнера ожидания выполнения запроса
						this->_transfer.waiting.erase(this->_client.eid);
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to commit NTP-client timeout", __PRETTY_FUNCTION__, std::make_tuple(timeout), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to commit NTP-client timeout", log_t::flag_t::CRITICAL);
							#endif
						}
						// Выводим результат по умолчанию
						return false;
					// Если таймер для ожидания ответа от NTP-сервера успешно установлен
					} else {
						// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
						ret.first->second.eid = tid;
						// Устанавливаем время жизни пакета для отслеживания его выполнения
						ret.first->second.delay = timeout;
						// Устанавливаем версию протокола NTP для выполнения запроса
						ret.first->second.version = version;
						// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от NTP-сервера
						this->_io->on(tid, static_cast <event::callback::status_t> (std::bind(&ntp_t::timeout, this, _1, _2, &ret.first->second)));
						// Запускаем таймер для ожидания ответа от NTP-сервера
						this->_io->launch(tid);
					}
				}
			}
			// Выполняем блокировку потока для выполнения запроса
			const locker_t <> lock(this->_client.mtx);
			// Создаём объект пакета запроса
			::ntp::packet_t packet{};
			/**
			 * Определяем версию протокола NTP для выполнения запроса
			 */
			switch(static_cast <uint8_t> (version)){
				// Если версия протокола NTPv1
				case 0x01: packet.mode = 0x0B; break;
				// Если версия протокола NTPv2
				case 0x02: packet.mode = 0x13; break;
				// Если версия протокола NTPv3
				case 0x03: packet.mode = 0x1B; break;
				// Если версия протокола NTPv4
				case 0x04: packet.mode = 0x23; break;
			}
			// Устанавливаем версию протокола NTP
			packet.origTimeStampSec = ::ntp::timesec();
			// Отправляем запрос на NTP-сервер для синхронизации времени
			return (this->_io->send(this->_client.eid, &packet, sizeof(packet)) > 0);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::unit::NTP::NTP(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _addr(fmk, log) {
	// Активируем работу мьютекса блокировки потока при работе с клиентом
	this->_client.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с контейнером активных пакетов
	this->_transfer.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Если общие NTP-серверы ещё не добавлены в глобальный список
	if(::servers::general.empty()){
		// Активируем работу мьютекса блокировки потока при работе с IP-адресами
		::__awh_mtx__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Создаём массив стандартных NTP-серверов
		array <string_view, 22> resolvers = {AWH_NTP_SERVERS};
		// Выбираем стандарт рандомайзера
		mt19937 generator(::__awh_randev__());
		// Выполняем рандомную сортировку списка DNS-серверов
		::shuffle(resolvers.begin(), resolvers.end(), generator);
		// Выполняем перебор всех NTP-серверов из массива
		for(const auto & item : resolvers)
			// Добавляем NTP-сервер в глобальный список для использования при выполнении запросов к NTP-серверам
			::servers::push(item);
	}
	/**
	 * Инициализация NTP-сервера
	 */
	this->_client.servers.init();
	/**
	 * Выполняем создание события NTP-клиента для указанного семейства IP-адресов
	 */
	this->create(family);
}
/**
 * @brief Деструктор
 *
 */
awh::unit::NTP::~NTP() noexcept {}
