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
 * Подписываемся на пространство имён placeholders
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
 * Инкапсулируем функции работы с резолвингом доменных имён в пространство имён
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
		if(::getaddrinfo(string(domain).c_str(), nullptr, &hints, &result) == 0){
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
 * @brief Метод обработки ошибок событий ICMP-клиента
 *
 * @param eid         идентификатор события ICMP-клиента
 * @param error       код ошибки события ICMP-клиента
 * @param description описание ошибки события ICMP-клиента
 */
void awh::unit::ICMP::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки событий таймаута при ожидании ответа от ICMP-клиента
 *
 * @param eid    идентификатор события ICMP-клиента
 * @param action действие события таймера ICMP-клиента
 * @param delay  задержка таймера ICMP-клиента
 * @return       нужно ли завершить клиента после истечения таймаута
 */
bool awh::unit::ICMP::timeout([[maybe_unused]] const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Снимаем флаг ожидания ответа от сервера
	this->_transfer.waiting = !this->_transfer.waiting;
	// Если функция обратного вызова установлена
	if(this->_callback.is("timeout"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const id_t, const uint16_t, const uint32_t)> ("timeout", this->_transfer.id, this->_transfer.count, delay);
	// Если функция обратного вызова не установлена
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug(
				"ICMP-client timeout (delay: %u)",
				__PRETTY_FUNCTION__,
				make_tuple(eid, static_cast <uint16_t> (action), delay),
				log_t::flag_t::WARNING, delay
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("ICMP-client timeout (delay: %u)", log_t::flag_t::WARNING, delay);
		#endif
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
			this->_log->debug("ICMP-client waiting time expired", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), delay), log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("ICMP-client waiting time expired", log_t::flag_t::CRITICAL);
		#endif
	}
	// Запрещаем завершение клиента после истечения таймаута
	return false;
}
/**
 * @brief Метод обработки ответов от удалённого сервера на запросы ICMP-клиента
 *
 * @param eid  идентификатор события чтения из ICMP-клиента
 * @param mode режим обработки события чтения из ICMP-клиента
 * @param data данные события чтения из ICMP-клиента
 * @param size размер данных события чтения из ICMP-клиента
 */
void awh::unit::ICMP::response([[maybe_unused]] const event::id_t eid, const mode_t mode, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Длина IP-заголовка
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
					awh_cast <net::addr_net_ipv4_t *> (address.get())->address = iph->ip_src.s_addr;
				}
			} break;
			// Если адрес является IPv6
			case 6: {
				/**
				 * Добавляем выравнивание структуры для корректного чтения данных из буфера
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
					uint8_t  hlim;    // Лимит времени жизни
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
				// Извлекаем IP-адрес установленный в событии
				this->_io->getTarget(eid, address);
			}
		}
		// Извлекаем идентификатор запроса
		const id_t id = ntohs(icmp->meta.echo.identifier);
		// Извлекаем номер последовательности запроса
		const uint16_t sequence = ntohs(icmp->meta.echo.sequence);
		// Получаем текущую метку времени
		const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
		// Если функция обратного вызова установлена для получения ответа от удалённого сервера
		if(this->_callback.is("ping")){
			// Создаём объект ответа от удалённого сервера
			response_t response{};
			// Устанавливаем размер полученных данных от удалённого сервера
			response.size = size;
			// Устанавливаем номер последовательности запроса
			response.sequence = sequence;
			// Устанавливаем IP-адрес удалённого сервера
			response.address = address.get();
			// Устанавливаем время жизни ответа от удалённого сервера
			response.timeToLive = this->_client.delay;
			// Устанавливаем время ответа от удалённого сервера
			response.elapsed = (now - this->_transfer.timestamp);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const id_t, const response_t &)> ("ping", id, response);
		}
		// Выполняем фиксацию текущей метки времени для данного запроса
		this->_transfer.timestamp = now;
		// Если выполняется синхронный режим пинга удалённого сервера
		if(mode == mode_t::ASYNC){
			// Если номер последовательности запроса меньше количества отправленных запросов
			if(sequence < (this->_transfer.count - 1)){
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
			// Снимаем флаг ожидания ответа от сервера
			} else this->_transfer.waiting = !this->_transfer.waiting;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (mode), data, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод инициализации события ICMP-клиента
 *
 * @param family семейство протоколов (например: IPv4 или IPv6)
 * @return       результат инициализации события ICMP-клиента
 */
bool awh::unit::ICMP::init(const event::family_t family) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие ICMP-клиента уже существует
		if(this->_client.eid > 0)
			// Удаляем событие ICMP-клиента
			this->_io->destroy(this->_client.eid);
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Добавляем новое событие клиента ICMP
			this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Если пользователь является непривилегированным
			if(::getuid() > 0)
				// Добавляем новое событие клиента ICMP
				this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::ICMP);
			// Добавляем новое событие клиента ICMP
			else this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
		#endif
		// Если адрес назначения сервера установлен
		if((result = ((this->_client.eid > 0) && (this->_client.target != nullptr)))){
			// Устанавливаем адрес сервера назначения
			this->_io->setTarget(this->_client.eid, this->_client.target.get());
			// Если адрес сети для выполнения запроса установлен
			if(this->_client.source != nullptr){
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Устанавливаем IP-адрес события
						result = this->_io->setAddress(this->_client.eid, event::address_t::IPV4, this->_client.source.get());
					break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Устанавливаем IP-адрес события
						result = this->_io->setAddress(this->_client.eid, event::address_t::IPV6, this->_client.source.get());
					break;
				}
			}
			// Если адрес сети для выполнения запроса установлен успешно
			if(result){
				// Устанавливаем время ожидания ответа от NTP-сервера
				this->_io->setTimeout(this->_client.eid, event::action_t::READ, this->_client.delay);
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
							this->_log->debug("Failed to launch ICMP-client", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to launch ICMP-client", log_t::flag_t::CRITICAL);
						#endif
					}
				// Если событие ICMP-клиента запущено успешно
				} else {
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(this->_client.eid, static_cast <engine::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие чтения данных
					this->_io->on(this->_client.eid, static_cast <engine::callback::read_t> (std::bind(&icmp_t::response, this, _1, mode_t::ASYNC, _2, _3)));
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
					this->_log->debug("ICMP-client target address is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::ICMP::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении ответа от удалённого сервера
	this->_callback.set("ping", callback);
	// Выполняем установку функции обратного вызова при наступлении таймаута ожидания ответа от удалённого сервера
	this->_callback.set("timeout", callback);
}
/**
 * @brief Метод установки таймаута для ожидания ответа от сервера
 *
 * @param delay время ожидания ответа от сервера (в миллисекундах)
 */
void awh::unit::ICMP::setTimeout(const uint32_t delay) noexcept {
	// Устанавливаем время ожидания ответа от сервера
	this->_client.delay = delay;
}
/**
 * @brief Метод получения типа события
 *
 * @return тип события
 */
awh::event::type_t awh::unit::ICMP::type() const noexcept {
	// Получаем тип события ICMP-клиента
	return this->_io->type(this->_client.eid);
}
/**
 * @brief Метод получения типа узла события
 *
 * @return тип узла события
 */
awh::event::node_t awh::unit::ICMP::node() const noexcept {
	// Получаем тип узла события ICMP-клиента
	return this->_io->node(this->_client.eid);
}
/**
 * @brief Метод получения семейства события
 *
 * @return семейство адресов
 */
awh::event::family_t awh::unit::ICMP::family() const noexcept {
	// Получаем семейство события ICMP-клиента
	return this->_io->family(this->_client.eid);
}
/**
 * @brief Метод получения статуса события
 *
 * @return статус события
 */
awh::event::status_t awh::unit::ICMP::status() const noexcept {
	// Получаем статус события ICMP-клиента
	return this->_io->status(this->_client.eid);
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
		if(!target.empty()){
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr.host(target))){
				// Если адрес является IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV4))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
					}
				} break;
				// Если адрес является IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV6))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
					}
				} break;
				// Если мы получили другой тип адреса, то считаем, что это доменное имя
				default: {
					/**
					 * Выполняем перебор всего списка полученных доменных имён
					 */
					for(auto & ip : ::dns::resolve(target)){
						/**
						 * Определяем семейство события
						 */
						switch(ip->size){
							// Для семейства IPv4
							case 4: {
								// Устанавливаем адрес сервера назначения
								this->_client.target = ::move(ip);
								// Выводим полученный результат
								return true;
							}
							// Для семейства IPv6
							case 16: {
								// Устанавливаем адрес сервера назначения
								this->_client.target = ::move(ip);
								// Выводим полученный результат
								return true;
							}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(target), log_t::flag_t::CRITICAL, error.what());
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
		if(target != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(target->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем инициализацию объекта IP-адреса
					this->_client.target = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_client.target.get())->address = awh_cast <const net::addr_net_ipv4_t *> (target)->address;
					// Выводим положительный результат
					return true;
				}
				// Если адрес является IPv6
				case 16: {
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
		if(!target.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV4))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим полученный результат
						return result;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV6))){
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), target), log_t::flag_t::CRITICAL, error.what());
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
		if(!source.empty()){
			// Выполняем парсинг IP-адреса
			if((result = this->_addr.parse(source))){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
					}
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
					}
				}
			}
		// Сбрасываем IP-адрес события
		} else this->_client.source.reset(nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(source), log_t::flag_t::CRITICAL, error.what());
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
		if(source != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(source->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем инициализацию объекта IP-адреса
					this->_client.source = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_client.source.get())->address = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
					// Выводим положительный результат
					return true;
				}
				// Если адрес является IPv6
				case 16: {
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
		if(!source.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(source, net_addr_t::type_t::IPV4))){
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(source, net_addr_t::type_t::IPV6))){
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Выводим результат
						return result;
					}
				} break;
			}
		// Сбрасываем IP-адрес события
		} else this->_client.source.reset(nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), source), log_t::flag_t::CRITICAL, error.what());
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
 * @param id    идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
 * @param count количество выполняемых запросов
 * @param mode  режим выполнения запросов
 * @return      результат выполнения запроса
 */
bool awh::unit::ICMP::ping(const id_t id, const uint16_t count, const mode_t mode) noexcept {
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
				// Если пинг удалённого сервера ещё не выполняется и адрес назначения установлен
				if(!this->_transfer.waiting && (this->_client.target != nullptr)){
					// Устанавливаем флаг ожидания ответа от сервера
					this->_transfer.waiting = !this->_transfer.waiting;
					// Устанавливаем идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
					this->_transfer.id = id;
					// Получаем семейство IP-адресов текущего события ICMP-клиента
					event::family_t family = event::family_t::NONE;
					/**
					 * Определяем семейство события
					 */
					switch(this->_client.target->size){
						// Для семейства IPv4
						case 4:
							// Получаем семейство IP-адресов текущего события ICMP-клиента
							family = event::family_t::IPV4;
						break;
						// Для семейства IPv6
						case 16:
							// Получаем семейство IP-адресов текущего события ICMP-клиента
							family = event::family_t::IPV6;
						break;
					}
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
						// Если пользователь является непривилегированным
						if(::getuid() > 0)
							// Добавляем новое событие клиента ICMP
							eid = this->_io->event(awh::event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::ICMP);
						// Добавляем новое событие клиента ICMP
						else eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
					#endif
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(eid, static_cast <engine::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие чтения данных
					this->_io->on(eid, static_cast <engine::callback::read_t> (std::bind(&icmp_t::response, this, _1, mode, _2, _3)));
					// Если опции события не установлены
					if(!this->_io->setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
						// Удаляем событие ICMP-клиента
						this->_io->destroy(eid);
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to set options for ICMP-client event", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL);
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
						this->_io->setTimeout(eid, event::action_t::WRITE, this->_client.delay);
						// Устанавливаем таймаут события на чтение
						this->_io->setTimeout(eid, event::action_t::READ, this->_client.delay);
						// Выполняем фиксацию параметров события и его запуск
						if(!(result = this->_io->commit(eid) && this->_io->launch(eid))){
							// Удаляем событие ICMP-клиента
							this->_io->destroy(eid);
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Failed to launch ICMP-client", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL);
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
							this->_transfer.count = count;
							// Запоминаем текущее значение времени в миллисекундах для фиксации начала запроса
							this->_transfer.timestamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
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
						// Формируем текст выводимой ошибки ICMP-клиента
						const string error = "ICMP-client target address is not set";
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					}
					// Удаляем событие ICMP-клиента
					this->_io->destroy(eid);
					// Снимаем флаг ожидания ответа от сервера
					this->_transfer.waiting = !this->_transfer.waiting;
				}
			} break;
			// Если выполняется асинхронный режим пинга удалённого сервера
			case static_cast <uint8_t> (mode_t::ASYNC): {
				// Если пинг удалённого сервера ещё не выполняется
				if(!this->_transfer.waiting){
					// Устанавливаем флаг ожидания ответа от сервера
					this->_transfer.waiting = !this->_transfer.waiting;
					// Устанавливаем идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
					this->_transfer.id = id;
					// Устанавливаем количество выполняемых запросов
					this->_transfer.count = count;
					// Запоминаем текущее значение времени в миллисекундах для фиксации начала запроса
					this->_transfer.timestamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
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
					// Отправляем сообщение серверу
					result = (this->_io->send(this->_client.eid, &icmp, sizeof(icmp)) > 0);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::ICMP::ICMP(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _addr(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::ICMP::~ICMP() noexcept {
	// Если событие ICMP-клиента активно
	if(this->_client.eid > 0)
		// Удаляем событие ICMP-клиента
		this->_io->destroy(this->_client.eid);
}
