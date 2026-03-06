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
		// Добавляем новое событие клиента UDP
		this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_client.eid, static_cast <event::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_client.eid, static_cast <event::callback::read_t> (std::bind(&icmp_t::response, this, _1, _2, _3)));
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
 * @param eid     идентификатор таймера ICMP-клиента
 * @param status  статус события таймера ICMP-клиента
 */
void awh::unit::ICMP::timeout(const event::id_t eid, const event::status_t status) noexcept {
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
			auto i = this->_timeouts.waiting.find(eid);
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
 * @param data данные события чтения из ICMP-клиента
 * @param size размер данных события чтения из ICMP-клиента
 */
void awh::unit::ICMP::response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {

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
				// Получаем семейство IP-адресов текущего события NTP-клиента
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
 * @brief Метод выполнения пингов удалённых серверов
 *
 * @param count   количество выполняемых запросов
 * @param mode    режим выполнения запросов
 * @param timeout время ожидания ответа от удалённого сервера (в миллисекундах)
 * @return        результат выполнения запроса
 */
bool awh::unit::ICMP::ping(const uint16_t count, const mode_t mode, const uint32_t timeout) noexcept {

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
