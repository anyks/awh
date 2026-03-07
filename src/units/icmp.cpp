/**
 * @file: icmp.cpp
 * @date: 2026-03-06
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
#include <cerrno>
#include <vector>
#include <random>
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
#include <netinet/ip.h>
#include <netinet/ip6.h>

/**
 * Подключаем заголовочный файл модуля
 */
#include <units/icmp.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
 */
using namespace placeholders;

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для рандомизации удалённых серверов
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
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * @brief Структура заголовков ICMP
	 *
	 */
	typedef struct IcmpHeader {
		uint8_t type;      // Тип запроса
		uint8_t code;      // Код запроса
		uint16_t checksum; // Контрольная сумма
		/**
		 * Объединение структур запроса
		 */
		union {
			/**
			 * @brief Структура отправляемого запроса
			 *
			 */
			struct {
				uint16_t identifier = 0; // Идентификатор запроса
				uint16_t sequence   = 0; // Номер последовательности
				uint64_t payload    = 0; // Тело полезной нагрузки
			} echo;
			/**
			 * @brief Структура указателя запроса
			 *
			 */
			struct ICMP_PACKET_POINTER_HEADER {
				// Указатель пакета
				uint8_t pointer = 0;
			} pointer;
			/**
			 * @brief Структура адреса ответа
			 *
			 */
			struct ICMP_PACKET_REDIRECT_HEADER {
				// Адрес ответа IPv4
				uint32_t gatewayAddress = 0;
			} redirect;
			/**
			 * @brief Структура адреса ответа
			 *
			 */
			struct ICMP6_PACKET_REDIRECT_HEADER {
				// Адрес ответа IPv6
				uint32_t gatewayAddress[4] = {0,0,0,0};
			} redirect6;
		} meta;
	} __attribute__((packed)) header_t;

	/**
	 * @brief Функция генерации уникального идентификатора
	 *
	 * @return уникальный идентификатор
	 */
	static unit::icmp_t::id_t identifier() noexcept {
		// Результат работы функции
		unit::icmp_t::id_t result = 0;
		// Начинаем с 1 (0 можно оставить как "invalid")
		static atomic_uint16_t id{1};
		// Выводим новое значение идентификатора
		result = id.fetch_add(1, memory_order_relaxed);
		// Если результат не получен
		if(result == 0)
			// Генерируем результат заново
			return identifier();
		// Выводим полученный результат
		return result;
	}

	/**
	 * @brief Функция подсчёта контрольной суммы
	 *
	 * @param buffer буфер данных для подсчёта
	 * @param size   размер данных для подсчёта
	 * @return       подсчитанная контрольная сумма
	 */
	static uint16_t checksum(const void * buffer, const size_t size) noexcept {
		// Результат работы функции
		uint16_t result = 0;
		// Если данные переданы верные
		if((buffer != nullptr) && (size > 0)){
			// Контрольная сумма расчёта
			uint32_t sum = 0;
			// Устанавливаем длину контрольной суммы
			size_t length = size;
			// Выполняем приведение буфера в нужную нам форму
			auto data = reinterpret_cast <const uint16_t *> (buffer);
			// Если длина буфера всего один байт
			if(length & 1)
				// Выполняем расчёт контрольной суммы
				sum = reinterpret_cast <const uint8_t *> (data)[length - 1];
			// Делим длину байт пополам
			length /= 2;
			/**
			 *  Выполняем перебор буфера байт
			 */
			while(length--){
				// Выполняем расчёт контрольной суммы
				sum += * data++;
				// Если контрольная сумма достигла предела
				if(sum & 0xffff0000)
					// Выполняем смещение на оставшиеся 16 байт
					sum = ((sum >> 16) + (sum & 0xffff));
			}
			// Выполняем получение результата контрольной суммы
			result = static_cast <uint16_t> (~sum);
		}
		// Выводим результат
		return result;
	}
};

/**
 * Инкапсулируем фуркции работы с резолвингом доменных имён в пространство имён
 */
namespace dns {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Метод резолвинга удалённого сервера
	 *
	 * @param domain доменное имя удалённого сервера
	 * @return       объекты IP-адреса принадлежащему удалённому серверу
	 */
	static vector <unique_ptr <net::addr_t>> resolve(string_view domain) noexcept {
		// Список полученных IP-адресов
		vector <unique_ptr <net::addr_t>> ips;
		// Создаём объект IP-адреса для параметров удалённого сервера
		struct addrinfo hints = {};
		// Результат получения параметров удалённого сервера
		struct addrinfo * result = nullptr;
		// Устанавливаем семейство протоколов для удалённого сервера (IPv4 + IPv6)
		hints.ai_family = AF_UNSPEC;
		// Устанавливаем тип сокета для удалённого сервера (TCP)
		hints.ai_socktype = SOCK_STREAM;
		/**
		 * Выполняем получение параметров удалённого сервера по его адресу
		 */
		if(::getaddrinfo(domain.data(), nullptr, &hints, &result) == 0){
			/**
			 * Выполняем перебор всех полученных параметров удалённого сервера и сохраняем их в общий список удалённых серверов
			 */
			for(auto * p = result; p != nullptr; p = p->ai_next){
				/**
				 * Определяем тип адреса удалённого сервера
				 */
				switch(p->ai_family){
					// Если адрес является IPv4
					case AF_INET: {
						// Выполняем инициализацию объекта IP-адреса
						unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
						// Получаем объект с IPv4-адресом удалённого сервера
						auto * sa = reinterpret_cast <sockaddr_in *> (p->ai_addr);
						// Копируем IPv4-адрес удалённого сервера в объект IP-адреса
						awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = sa->sin_addr.s_addr;
						// Добавляем сервер в общий список удалённых серверов
						ips.push_back(::move(ip));
					} break;
					// Если адрес является IPv6
					case AF_INET6: {
						// Выполняем инициализацию объекта IP-адреса
						unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
						// Получаем объект с IPv6-адресом удалённого сервера
						auto * sa = reinterpret_cast <sockaddr_in6 *> (p->ai_addr);
						// Копируем IPv6-адрес удалённого сервера в объект IP-адреса
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &sa->sin6_addr.s6_addr[0], 16);
						// Добавляем сервер в общий список удалённых серверов
						ips.push_back(::move(ip));
					} break;
				}
			}
			// Освобождаем память, выделенную для хранения параметров удалённого сервера
			::freeaddrinfo(result);
		}
		// Выбираем стандарт рандомайзера
		mt19937 generator(::__awh_randev__());
		// Выполняем рандомную сортировку списка DNS-серверов
		::shuffle(ips.begin(), ips.end(), generator);
		// Выводим полученные IP-адреса
		return ips;
	}
};

/**
 * @brief Метод создания события ICMP-клиента
 *
 * @param family семейство протоколов (например: IPv4 или IPv6)
 */
void awh::unit::ICMP::create(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока для создания события ICMP-клиента
		const locker_t <> lock(this->_client.mtx);
		/**
		 * Для операционной системы MS Windows или FreeBSD
		 */
		#if _WIN32 || _WIN64 || __FreeBSD__
			// Добавляем новое событие клиента ICMP
			this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Добавляем новое событие клиента UDP
			this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::ICMP);
		#endif
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_client.eid, static_cast <event::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_client.eid, static_cast <event::callback::read_t> (std::bind(&icmp_t::response, this, _1, mode_t::ASYNC, _2, _3)));
		// Если опции события не установлены
		if(!this->_io->setOptions(this->_client.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
			// Удаляем событие ICMP-клиента
			this->_io->destroy(this->_client.eid);
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to set options for ICMP-client event", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to set options for ICMP-client event", log_t::flag_t::CRITICAL);
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
 * @brief Метод обработки ошибок событий ICMP-клиента
 *
 * @param eid         идентификатор события ICMP-клиента
 * @param error       код ошибки события ICMP-клиента
 * @param description описание ошибки события ICMP-клиента
 */
void awh::unit::ICMP::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если событие относится к ICMP-клиенту
	if(eid == this->_client.eid)
		// Выполняем сброс ICMP-клиента
		this->reset();
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки событий таймаута при ожидании ответа от ICMP-клиента
 *
 * @param id     идентификатор ICMP-клиента
 * @param        идентификатор таймера ICMP-клиента
 * @param status статус события таймера ICMP-клиента
 */
void awh::unit::ICMP::timeout(const id_t id, [[maybe_unused]] const event::id_t, const event::status_t status) noexcept {
	// Если статус события успешен
	if(status == event::status_t::SUCCESS){
		// Запоминаем идентификатор клиента
		const event::id_t eid = this->_client.eid;
		// Получаем семейство IP-адресов текущего события ICMP-клиента
		const event::family_t family = this->_io->family(eid);
		{
			// Выполняем блокировку потока для уничтожения события ICMP-клиента
			const locker_t <> lock(this->_client.mtx);
			// Удаляем событие ICMP-клиента
			this->_io->destroy(eid);
		}
		// Выполняем создание события ICMP-клиента для указанного семейства IP-адресов
		this->create(family);
		// Выполняем фиксацию параметров ICMP-клиента
		this->commit();
		{
			// Выполняем блокировку потока для работы с контейнером таймаутов и обратных связей таймаутов
			const locker_t <std::shared_mutex> lock(this->_timeouts.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск таймаута в контейнере таймаутов
			auto i = this->_timeouts.waiting.find(id);
			// Если таймаут найден в контейнере таймаутов
			if(i != this->_timeouts.waiting.end())
				// Удаляем таймаут из контейнера таймаутов
				this->_timeouts.waiting.erase(i);
		}
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::INVALID_ADDRESS, "Waiting time expired");
		// Если функция обратного вызова не установлена
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("ICMP-client waiting time expired", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("ICMP-client waiting time expired", log_t::flag_t::CRITICAL);
			#endif
		}
	}
}
/**
 * @brief Метод обработки ответов от удалённого сервера на запросы ICMP-клиента
 *
 * @param eid  идентификатор события чтения из ICMP-клиента
 * @param mode режим обработки события чтения из ICMP-клиента
 * @param data данные события чтения из ICMP-клиента
 * @param size размер данных события чтения из ICMP-клиента
 */
void awh::unit::ICMP::response(const event::id_t eid, const mode_t mode, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Длина IP-заголовка`
		size_t length = 0;
		// Заголовок пакета ICMP протокола
		const header_t * icmp = nullptr;
		// IP-адрес для вывода результата
		unique_ptr <net::addr_t> address = nullptr;
		/**
		 * Определяем версию IP-адреса
		 * 0x40 = IPv4 (0100 0000), 0x60 = IPv6 (0110 0000)
		 */
		switch(((data[0] & 0xF0) >> 4)){
			// Если адрес является IPv4
			case 4: {
				// Если размер данных меньше размера заголовка IP
				if(size < sizeof(struct ip))
					// Выходим из функции
					return;
				// Приводим данные к структуре IP-заголовка
				const struct ip * iph = reinterpret_cast <const struct ip *> (data);
				// Извлекаем длину IP-заголовка
				length = (iph->ip_hl * 4);
				// Если заголовок пришёл битый
				if((length < 20) || (size < (length + 8)))
					// минимум ICMP-заголовок
					return;
				// Минимум 8 байт ICMP
				if(size >= (length + 8)){
					// Приводим данные к структуре ICMP-заголовка
					icmp = reinterpret_cast <const header_t *> (data + length);
					// Выполняем инициализацию объекта IP-адреса
					address = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (address.get())->address = icmp->meta.redirect.gatewayAddress;
				}
			} break;
			// Если адрес является IPv6
			case 6: {
				/**
				 * Добаявляем выравнивание структуры для корректного чтения данных из буфера
				 */
				#pragma pack(push, 1)
				/**
				 * @brief IPv6 заголовок фиксирован = 40 байт
				 *
				 */
				struct ip6_hdr_min {
					uint32_t flow;    // Потоковая метка (version, traffic class, flow label)
					uint16_t plen;    // Длина полезной нагрузки
					uint8_t  nxt;     // Следующий заголовок
					uint8_t  hlim;    // Лимитатор времени жизни
					uint8_t  src[16]; // Адрес источника
					uint8_t  dst[16]; // Адрес назначения
				};
				// Удаляем выравнивание структуры для корректного чтения данных из буфера
				#pragma pack(pop)
				// Если размер данных меньше размера заголовка IP
				if(size < 40)
					// Выходим из функции
					return;
				// Приводим данные к структуре IP-заголовка
				const struct ip6_hdr_min * ip6h = reinterpret_cast <const struct ip6_hdr_min *> (data);
				// Извлекаем длину IP-заголовка
				length = 40;
				// Если заголовок пришёл битый
				if((length < 20) || (size < (length + 8)))
					// минимум ICMP-заголовок
					return;
				// Минимум 8 байт ICMP
				if(size >= (length + 8)){
					// Приводим данные к структуре ICMP-заголовка
					icmp = reinterpret_cast <const header_t *> (data + length);
					// Выполняем инициализацию объекта IP-адреса
					address = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (address.get())->address[0], &ip6h->src, 16);
				}
			} break;
			// Если это какой-то другой адрес
			default: {
				// Результат полученных данных
				icmp = reinterpret_cast <const header_t *> (data);
				// Выполняем блокировку потока для установки IP-адреса события
				const locker_t <> lock(this->_client.mtx);
				// Извлекаем IP-адрес установленный в событии
				this->_io->getTarget(eid, address);
			}
		}
		// Извлекаем идентификатор запроса
		const id_t id = ntohs(icmp->meta.echo.identifier);
		// Извлекаем номер последовательности запроса
		const uint16_t sequence = ntohs(icmp->meta.echo.sequence);
		// Выполняем блокировку потока для работы с контейнером таймаутов и обратных связей таймаутов
		const locker_t <std::shared_mutex> lock(this->_timeouts.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск таймаута в контейнере таймаутов
		auto i = this->_timeouts.waiting.find(id);
		// Если таймаут найден в контейнере таймаутов
		if(i != this->_timeouts.waiting.end()){
			// Получаем текущую метку времени
			const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			// Если функция обратного вызова установлена для получения ответа от удалённого сервера
			if(this->_callback.is("ping"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const id_t, const uint16_t, const uint64_t, const net::addr_t *)> ("ping", id, sequence, (now - i->second.timestamp), address.get());
			// Выполняем фиксацию текущей метки времени для данного запроса
			i->second.timestamp = now;
			// Если выполняется синхронный режим пинга удалённого сервера
			if(mode == mode_t::ASYNC){
				// Выполняем блокировку потока для уничтожения события
				const locker_t <> lock(this->_client.mtx);
				// Удаляем событие таймера для ожидания ответа от удалённого сервера
				this->_io->destroy(i->second.eid);
				// Если номер последовательности запроса меньше количества отправленных запросов
				if(sequence < i->second.count){
					// Добавляем новое событие таймаута для ожидания ответа от удаленного сервера
					const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
					// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от удаленного сервера
					this->_io->setTimeout(tid, event::action_t::NONE, (i->second.delay > 0 ? i->second.delay : 5000));
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(tid, static_cast <event::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
					// Если не удалось установить таймер для ожидания ответа от удаленного сервера
					if(!this->_io->commit(tid)){
						// Удаляем событие таймера
						this->_io->destroy(tid);
						// Удаляем таймаут из контейнера ожидания выполнения запроса
						this->_timeouts.waiting.erase(i);
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to commit ICMP timeout", __PRETTY_FUNCTION__, std::make_tuple(id, i->second.count, static_cast <uint16_t> (mode), i->second.delay), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to commit ICMP timeout", log_t::flag_t::CRITICAL);
							#endif
						}
						// Выходим из функции
						return;
					// Если таймер для ожидания ответа от удаленного сервера успешно установлен
					} else {
						// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
						i->second.eid = tid;
						// Подключаем устройство генератора
						mt19937 generator(::__awh_randev__());
						// Выполняем генерирование случайного числа
						uniform_int_distribution <mt19937::result_type> dist6(0, numeric_limits <uint32_t>::max() - 1);
						// Создаём объект заголовков
						header_t icmp{};
						// Устанавливаем код запроса
						icmp.code = 0;
						/**
						 * Определяем семейство события
						 */
						switch(static_cast <uint8_t> (this->_io->family(eid))){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4):
								// Выполняем установку типа запроса
								icmp.type = 8;
							break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6):
								// Выполняем установку типа запроса
								icmp.type = 128;
							break;
						}
						// Устанавливаем идентификатор запроса
						icmp.meta.echo.identifier = htons(id);
						// Устанавливаем номер последовательности
						icmp.meta.echo.sequence = htons(sequence + 1);
						// Устанавливаем данные полезной нагрузки
						icmp.meta.echo.payload = static_cast <uint64_t> (dist6(generator));
						// Обнуляем структуру (ОЧЕНЬ ВАЖНО ТАК-КАК РАСЧЁТ КОНТРОЛЬНОЙ СУММЫ НАЧИНАЕТСЯ С НУЛЯ!!!)
						icmp.checksum = 0;
						// Выполняем подсчёт контрольной суммы
						icmp.checksum = ::checksum(&icmp, sizeof(icmp));
						// Отправляем сообщение серверу
						this->_io->send(eid, &icmp, sizeof(icmp));
					}
				// Удаляем таймаут из контейнера таймаутов
				} else this->_timeouts.waiting.erase(i);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (mode), size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::unit::ICMP::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с IP-адресами
	::__awh_mtx__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с ICMP-клиентом
	this->_client.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с таймаутами
	this->_timeouts.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод сброса ICMP-клиента
 *
 * @return результат выполнения операции
 */
bool awh::unit::ICMP::reset() noexcept {
	// Получаем семейство IP-адресов текущего события ICMP-клиента
	const event::family_t family = this->_io->family(this->_client.eid);
	{
		// Выполняем блокировку потока для уничтожения события ICMP-клиента
		const locker_t <> lock(this->_client.mtx);
		// Удаляем событие ICMP-клиента
		this->_io->destroy(this->_client.eid);
	}
	// Выполняем создание события ICMP-клиента для указанного семейства IP-адресов
	this->create(family);
	// Выполняем фиксацию параметров ICMP-клиента
	return this->commit();
}
/**
 * @brief Метод фиксации параметров ICMP-клиента
 *
 * @return результат выполнения операции
 */
bool awh::unit::ICMP::commit() noexcept {
	/**
	 * Если операционной системой не является FreeBSD
	 */
	#ifndef __FreeBSD__
		// Результат работы функции
		bool result = false;
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если адрес назначения сервера установлен
			if(this->_client.target != nullptr){
				// Выполняем блокировку потока для установки IP-адреса события
				const locker_t <> lock(this->_client.mtx);
				// Устанавливаем адрес сервера назначения
				this->_io->setTarget(this->_client.eid, this->_client.target.get());
				// Если адрес сети для выполнения запроса установлен
				if(this->_client.source != nullptr){
					// Получаем семейство IP-адресов текущего события ICMP-клиента
					const event::family_t family = this->_io->family(this->_client.eid);
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
					// Удаляем событие ICMP-клиента
					this->_io->destroy(this->_client.eid);
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to launch ICMP-client", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to launch ICMP-client", log_t::flag_t::CRITICAL);
						#endif
					}
				}
			// Если адрес назначения сервера не установлен
			} else {
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_client.eid, event::error_t::INVALID_ADDRESS, "Target address is not set");
				// Если функция обратного вызова не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("ICMP-client target address is not set", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("ICMP-client target address is not set", log_t::flag_t::CRITICAL);
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
	/**
	 * Если операционной системой является FreeBSD
	 */
	#else
		// Выводим результат по умолчанию
		return true;
	#endif
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::ICMP::setTarget(string_view target) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес ICMP-сервера передан
		if((this->_client.eid > 0) && !target.empty()){
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr.host(target))){
				// Если адрес является IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV4))){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
					}
				} break;
				// Если адрес является IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV6))){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
					}
				} break;
				// Если адресом является доменное имя
				case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
					// Получаем семейство IP-адресов текущего события ICMP-клиента
					const event::family_t family = this->_io->family(this->_client.eid);
					// Выполняем перебор всего списка полученных доменных имён
					for(auto & ip : ::dns::resolve(target)){
						/**
						 * Определяем семейство события
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								// Если IP-адрес принадлежит к IPv4
								if((result = (ip->size == 4))){
									// Выполняем блокировку потока для установки IP-адреса события
									const locker_t <> lock(this->_client.mtx);
									// Устанавливаем адрес сервера назначения
									this->_client.target = ::move(ip);
									// Выводим полученный результат
									return result;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								// Если IP-адрес принадлежит к IPv6
								if((result = (ip->size == 16))){
									// Выполняем блокировку потока для установки IP-адреса события
									const locker_t <> lock(this->_client.mtx);
									// Устанавливаем адрес сервера назначения
									this->_client.target = ::move(ip);
									// Выводим полученный результат
									return result;
								}
							} break;
						}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, target), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::ICMP::setTarget(const net::addr_t * target) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес ICMP-сервера передан
		if((this->_client.eid > 0) && (target != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(target->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Выполняем инициализацию объекта IP-адреса
					this->_client.target = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_client.target.get())->address = awh_cast <const net::addr_net_ipv4_t *> (target)->address;
					// Выводим положительный результат
					return true;
				}
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Выполняем инициализацию объекта IP-адреса
					this->_client.target = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_client.target.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (target)->address[0], 16);
					// Выводим положительный результат
					return true;
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::ICMP::setTarget(const event::family_t family, string_view target) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес ICMP-сервера передан
		if((this->_client.eid > 0) && !target.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV4))){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV6))){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_client.eid, static_cast <uint16_t> (family), target), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 * @return       результат выполнения установки
 */
bool awh::unit::ICMP::setSource(string_view source) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!(result = ((this->_client.eid == 0) || source.empty()))){
			// Выполняем блокировку потока для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if((result = this->_addr.parse(source))){
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
						// Выводим результат
						return result;
					}
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
					}
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 * @return       результат выполнения установки
 */
bool awh::unit::ICMP::setSource(const net::addr_t * source) noexcept {
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
					// Выводим положительный результат
					return true;
				}
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потока для установки IP-адреса события
					const locker_t <> lock(this->_client.mtx);
					// Выполняем инициализацию объекта IP-адреса
					this->_client.source = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_client.source.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], 16);
					// Выводим положительный результат
					return true;
				}
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Выполняем блокировку потока для установки IP-адреса события
			const locker_t <> lock(this->_client.mtx);
			// Сбрасываем IP-адрес события
			this->_client.source.reset(nullptr);
			// Выводим положительный результат
			return true;
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сети с которого будет выполняться запрос
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param source адрес сети для выполнения запроса
 * @return       результат выполнения установки
 */
bool awh::unit::ICMP::setSource(const event::family_t family, string_view source) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!(result = ((this->_client.eid == 0) || source.empty()))){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(source, net_addr_t::type_t::IPV4))){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потока для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(source, net_addr_t::type_t::IPV6))){
						// Выполняем блокировку потока для установки IP-адреса события
						const locker_t <> lock(this->_client.mtx);
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения идентификатора ICMP-клиента для выполнения запроса к удалённому серверу
 *
 * @return идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
 */
awh::unit::ICMP::id_t awh::unit::ICMP::issue() const noexcept {
	// Создаём идентификатор события DNS-резолвера
	return ::identifier();
}
/**
 * @brief Метод выполнения пингов удалённого сервера
 *
 * @param id      идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
 * @param count   количество выполняемых запросов
 * @param mode    режим выполнения запросов
 * @param timeout время ожидания ответа от удалённого сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::ICMP::ping(const id_t id, const uint16_t count, const mode_t mode, const uint32_t timeout) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем режим выполнения пинга удалённого сервера
		 */
		switch(static_cast <uint8_t> (mode)){
			// Если выполняется синхронный режим пинга удалённого сервера
			case static_cast <uint8_t> (mode_t::SYNC): {
				// Выполняем блокировку потока для работы с контейнером таймаутов и обратных связей таймаутов
				const locker_t <std::shared_mutex> lock(this->_timeouts.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Добавляем таймаут в контейнер таймаутов для отслеживания его выполнения
				auto ret = this->_timeouts.waiting.emplace(id, timeout_t());
				// Если таймаут уже существует для данного идентификатора ICMP-клиента
				if(!ret.second){
					// Формируем текст выводимой ошибки ICMP-клиента
					const string error = this->_fmk->format("ICMP request for ID=%d is still in progress, please wait for the result", id);
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, count, static_cast <uint16_t> (mode), timeout), log_t::flag_t::WARNING, error.c_str());
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
				// Если таймаут успешно добавлен для данного идентификатора ICMP-клиента
				} else {
					// Выполняем блокировку потока для создания события ICMP-клиента
					const locker_t <> lock(this->_client.mtx);
					// Получаем семейство IP-адресов текущего события ICMP-клиента
					const event::family_t family = this->_io->family(this->_client.eid);
					// Идентификатор события
					event::id_t eid = 0;
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Добавляем новое событие клиента ICMP
						eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Если пользователь является непривилигированным
						if(::getuid() > 0)
							// Добавляем новое событие клиента ICMP
							eid = this->_io->event(awh::event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::ICMP);
						// Добавляем новое событие клиента ICMP
						else eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
					#endif
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(eid, static_cast <event::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие чтения данных
					this->_io->on(eid, static_cast <event::callback::read_t> (std::bind(&icmp_t::response, this, _1, mode, _2, _3)));
					// Если опции события не установлены
					if(!this->_io->setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
						// Удаляем событие ICMP-клиента
						this->_io->destroy(eid);
						// Удаляем таймаут из контейнера таймаутов для данного идентификатора ICMP-клиента
						this->_timeouts.waiting.erase(id);
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"Failed to set options for ICMP-client event",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										count,
										static_cast <uint16_t> (mode),
										timeout
									), log_t::flag_t::CRITICAL
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to set options for ICMP-client event", log_t::flag_t::CRITICAL);
							#endif
						}
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Если адрес назначения сервера установлен
					if(this->_client.target != nullptr){
						// Устанавливаем адрес сервера назначения
						this->_io->setTarget(eid, this->_client.target.get());
						// Если адрес сети для выполнения запроса установлен
						if(this->_client.source != nullptr){
							// Получаем семейство IP-адресов текущего события ICMP-клиента
							const event::family_t family = this->_io->family(eid);
							/**
							 * Определяем семейство события
							 */
							switch(static_cast <uint8_t> (family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
									// Устанавливаем IP-адрес события
									this->_io->setAddress(eid, event::address_t::IPV4, this->_client.source.get());
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
									// Устанавливаем IP-адрес события
									this->_io->setAddress(eid, event::address_t::IPV6, this->_client.source.get());
								break;
							}
						}
						// Устанавливаем таймаут события на запись
						this->_io->setTimeout(eid, event::action_t::WRITE, (timeout > 0 ? timeout : 5000));
						// Устанавливаем таймаут события на чтение
						this->_io->setTimeout(eid, event::action_t::READ, (timeout > 0 ? timeout : 5000));
						// Выполняем фиксацию параметров события и его запуск
						if(!(result = this->_io->commit(eid) && this->_io->launch(eid))){
							// Удаляем событие ICMP-клиента
							this->_io->destroy(eid);
							// Удаляем таймаут из контейнера таймаутов для данного идентификатора ICMP-клиента
							this->_timeouts.waiting.erase(id);
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(
										"Failed to launch ICMP-client",
										__PRETTY_FUNCTION__,
										std::make_tuple(
											count,
											static_cast <uint16_t> (mode),
											timeout
										), log_t::flag_t::CRITICAL
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Failed to launch ICMP-client", log_t::flag_t::CRITICAL);
								#endif
							}
						// Если фиксация параметров события прошла успешно
						} else {
							// Устанавливаем количество выполняемых запросов
							ret.first->second.count = count;
							// Запоминаем текущее значение времени в миллисекундах для фиксации начала запроса
							ret.first->second.timestamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
							// Подключаем устройство генератора
							mt19937 generator(::__awh_randev__());
							// Выполняем генерирование случайного числа
							uniform_int_distribution <mt19937::result_type> dist6(0, numeric_limits <uint32_t>::max() - 1);
							// Создаём объект заголовков
							header_t icmp{};
							// Устанавливаем код запроса
							icmp.code = 0;
							/**
							 * Определяем семейство события
							 */
							switch(static_cast <uint8_t> (family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
									// Выполняем установку типа запроса
									icmp.type = 8;
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
									// Выполняем установку типа запроса
									icmp.type = 128;
								break;
							}
							// Последовательность
							uint16_t sequence = 0;
							// Выполняем пинг указанного количества раз
							for(uint16_t i = 0; i < count; i++){
								// Устанавливаем идентификатор запроса
								icmp.meta.echo.identifier = htons(id);
								// Устанавливаем номер последовательности
								icmp.meta.echo.sequence = htons(sequence);
								// Устанавливаем данные полезной нагрузки
								icmp.meta.echo.payload = static_cast <uint64_t> (dist6(generator));
								// Обнуляем структуру (ОЧЕНЬ ВАЖНО ТАК-КАК РАСЧЁТ КОНТРОЛЬНОЙ СУММЫ НАЧИНАЕТСЯ С НУЛЯ!!!)
								icmp.checksum = 0;
								// Выполняем подсчёт контрольной суммы
								icmp.checksum = ::checksum(&icmp, sizeof(icmp));
								// Отправляем сообщение серверу
								if(this->_io->send(eid, &icmp, sizeof(icmp))){
									// Выполняем чтение ответа
									if(this->_io->recv(eid))
										// Увеличиваем последовательность запроса
										sequence++;
								}
							}
						}
					// Если адрес назначения сервера не установлен
					} else {
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::INVALID_ADDRESS, "Target address is not set");
						// Если функция обратного вызова не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("ICMP-client target address is not set", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("ICMP-client target address is not set", log_t::flag_t::CRITICAL);
							#endif
						}
					}
					// Удаляем событие ICMP-клиента
					this->_io->destroy(eid);
					// Удаляем таймаут из контейнера таймаутов для данного идентификатора ICMP-клиента
					this->_timeouts.waiting.erase(id);
				}
			} break;
			/**
			 * Если операционной системой не является FreeBSD
			 */
			#ifndef __FreeBSD__
				// Если выполняется асинхронный режим пинга удалённого сервера
				case static_cast <uint8_t> (mode_t::ASYNC): {
					{
						// Выполняем блокировку потока для работы с контейнером таймаутов и обратных связей таймаутов
						const locker_t <std::shared_mutex> lock(this->_timeouts.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Добавляем таймаут в контейнер таймаутов для отслеживания его выполнения
						auto ret = this->_timeouts.waiting.emplace(id, timeout_t());
						// Если таймаут уже существует для данного идентификатора ICMP-клиента
						if(!ret.second){
							// Формируем текст выводимой ошибки ICMP-клиента
							const string error = this->_fmk->format("ICMP request for ID=%d is still in progress, please wait for the result", id);
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, count, static_cast <uint16_t> (mode), timeout), log_t::flag_t::WARNING, error.c_str());
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
						// Если таймаут успешно добавлен для данного идентификатора ICMP-клиента
						} else {
							// Выполняем блокировку потока для установки таймера
							const locker_t <> lock(this->_client.mtx);
							// Добавляем новое событие таймаута для ожидания ответа от удаленного сервера
							const event::id_t tid = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
							// Устанавливаем таймаут таймера по умолчанию на 5 секунд для ожидания ответа от удаленного сервера
							this->_io->setTimeout(tid, event::action_t::NONE, (timeout > 0 ? timeout : 5000));
							// Устанавливаем функцию обратного вызова на событие получения ошибок
							this->_io->on(tid, static_cast <event::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
							// Если не удалось установить таймер для ожидания ответа от удаленного сервера
							if(!this->_io->commit(tid)){
								// Удаляем событие таймера
								this->_io->destroy(tid);
								// Удаляем таймаут из контейнера ожидания выполнения запроса
								this->_timeouts.waiting.erase(id);
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Failed to commit ICMP timeout", __PRETTY_FUNCTION__, std::make_tuple(id, count, static_cast <uint16_t> (mode), timeout), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Failed to commit ICMP timeout", log_t::flag_t::CRITICAL);
									#endif
								}
								// Выводим результат по умолчанию
								return false;
							// Если таймер для ожидания ответа от удаленного сервера успешно установлен
							} else {
								// Устанавливаем идентификатор события таймаута для отслеживания его выполнения
								ret.first->second.eid = tid;
								// Устанавливаем количество выполняемых запросов
								ret.first->second.count = count;
								// Устанавливаем время жизни таймаута для отслеживания его выполнения
								ret.first->second.delay = timeout;
								// Запоминаем текущее значение времени в миллисекундах для фиксации начала запроса
								ret.first->second.timestamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
								// Устанавливаем обработчик события таймера для обработки таймаута при ожидании ответа от ICMP-клиента
								this->_io->on(tid, static_cast <event::callback::status_t> (std::bind(&icmp_t::timeout, this, id, _1, _2)));
								// Запускаем таймер для ожидания ответа от ICMP-клиента
								this->_io->launch(tid);
							}
						}
					}
					// Подключаем устройство генератора
					mt19937 generator(::__awh_randev__());
					// Выполняем генерирование случайного числа
					uniform_int_distribution <mt19937::result_type> dist6(0, numeric_limits <uint32_t>::max() - 1);
					// Создаём объект заголовков
					header_t icmp{};
					// Устанавливаем код запроса
					icmp.code = 0;
					/**
					* Определяем семейство события
					*/
					switch(static_cast <uint8_t> (this->_io->family(this->_client.eid))){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
							// Выполняем установку типа запроса
							icmp.type = 8;
						break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
							// Выполняем установку типа запроса
							icmp.type = 128;
						break;
					}
					// Устанавливаем номер последовательности
					icmp.meta.echo.sequence = htons(0);
					// Устанавливаем идентификатор запроса
					icmp.meta.echo.identifier = htons(id);
					// Устанавливаем данные полезной нагрузки
					icmp.meta.echo.payload = static_cast <uint64_t> (dist6(generator));
					// Обнуляем структуру (ОЧЕНЬ ВАЖНО ТАК-КАК РАСЧЁТ КОНТРОЛЬНОЙ СУММЫ НАЧИНАЕТСЯ С НУЛЯ!!!)
					icmp.checksum = 0;
					// Выполняем подсчёт контрольной суммы
					icmp.checksum = ::checksum(&icmp, sizeof(icmp));
					// Выполняем блокировку потока для выполнения запроса
					const locker_t <> lock(this->_client.mtx);
					// Отправляем сообщение серверу
					result = (this->_io->send(this->_client.eid, &icmp, sizeof(icmp)) > 0);
				} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				std::make_tuple(
					count,
					static_cast <uint16_t> (mode),
					timeout
				), log_t::flag_t::CRITICAL, error.what()
			);
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
 * @brief Конструктор
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::unit::ICMP::ICMP(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _addr(fmk, log) {
	// Активируем работу мьютекса блокировки потока при работе с клиентом
	this->_client.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с таймаутами
	this->_timeouts.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с IP-адресами
	::__awh_mtx__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	/**
	 * Выполняем создание события ICMP-клиента для указанного семейства IP-адресов
	 */
	this->create(family);
}
/**
 * @brief Деструктор
 *
 */
awh::unit::ICMP::~ICMP() noexcept {}
