/**
 * @file: socket.cpp
 * @date: 2026-01-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бэкенда низкоуровневой работы с сокетами — установка неблокирующего режима, таймаутов,
 *        размеров буферов, keep-alive, TCP_NODELAY, TOS/DSCP,
 *        multicast и параметров переиспользования адреса на каждой поддерживаемой платформе
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Для операционной системы macOS
 */
#if __APPLE__ && !__APPLE_USE_RFC_3542
	/**
	 * Подключаем экспериментальные функции для получения кода ошибки на сокете
	 */
    #define __APPLE_USE_RFC_3542
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <cerrno>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <shared_mutex>

/**
 * Системные заголовочные файлы
 */
#include <fcntl.h>
#include <signal.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

/**
 * Для операционной системы FreeBSD
 */
#if __FreeBSD__
	/**
	 * Заголовочный файл для работы с протоколом SCTP
	 */
	#include <netinet/sctp.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/locker.hpp>
#include <net/eth/socket.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Время жизни кеша списка сетевых интерфейсов в миллисекундах
	 *
	 * @note Сетевые интерфейсы могут менять адреса (DHCP, переключение сети), поэтому кеш периодически обновляется
	 *
	 */
	constexpr uint64_t AWH_IFACE_CACHE_TTL = 0x1388;

	/**
	 * @brief Флаг одноразовой инициализации мьютексов для кешей сетевых интерфейсов
	 *
	 */
	once_flag __awh_init_once__;

	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;

	/**
	 * Блокировка доступа к глобальному кешу сетевых интерфейсов
	 */
	static lock_state_t <std::shared_mutex> __awh_iface_cache_mutex__;
};

/**
 * @brief Внутренние служебные объекты модуля работы с сокетами
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Структура записи кеша сетевого интерфейса
	 *
	 */
	struct IfaceEntry {
		// Флаг активности интерфейса (IFF_UP)
		bool up;
		// Имя сетевого интерфейса
		string name;
		// Семейство адреса (AF_INET или AF_INET6)
		int32_t family;
		// Индекс сетевого интерфейса
		uint32_t index;
		// IPv4-адрес интерфейса (сетевой порядок байт)
		uint32_t address4;
		// IPv6-адрес интерфейса
		array <uint8_t, 16> address6;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit IfaceEntry() noexcept :
		 up(false), name{""}, family(0),
		 index(0), address4(0), address6{0} {}
	};

	/**
	 * @brief Структура кеша списка сетевых интерфейсов
	 *
	 */
	struct IfaceCache {
		// Абсолютное время истечения кеша в миллисекундах (0 - кеш пустой)
		uint64_t expire;
		// Список записей сетевых интерфейсов
		vector <IfaceEntry> entries;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit IfaceCache() noexcept : expire(0) {}
	} __awh_iface_cache__;

	/**
	 * @brief Функция обновления кеша списка сетевых интерфейсов через getifaddrs
	 *
	 * @note Сбор списка интерфейсов выполняется без удержания блокировки, публикация - под эксклюзивной блокировкой
	 *
	 * @param now текущая метка времени в миллисекундах
	 * @return    результат обновления кеша
	 *
	 */
	static bool refreshIfaceCache(const uint64_t now) noexcept {
		// Список собранных записей сетевых интерфейсов
		vector <IfaceEntry> entries;
		// Список сетевых интерфейсов
		struct ifaddrs * ptr = nullptr;
		// Выполняем получение списка сетевых интерфейсов
		if(::getifaddrs(&ptr) != 0)
			// Если список интерфейсов получить не удалось, выходим
			return false;
		/**
		 * Перебираем все сетевые интерфейсы
		 */
		for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
			// Пропускаем интерфейсы без адреса
			if(ifa->ifa_addr == nullptr)
				// Переходим к следующему интерфейсу
				continue;
			// Определяем семейство адреса интерфейса
			const int32_t family = ifa->ifa_addr->sa_family;
			// Если интерфейс не относится к IPv4 или IPv6
			if((family != AF_INET) && (family != AF_INET6))
				// Переходим к следующему интерфейсу
				continue;
			// Формируем запись сетевого интерфейса
			IfaceEntry entry;
			// Устанавливаем имя интерфейса
			entry.name = ifa->ifa_name;
			// Устанавливаем семейство адреса
			entry.family = family;
			// Устанавливаем флаг активности интерфейса
			entry.up = static_cast <bool> (ifa->ifa_flags & IFF_UP);
			// Получаем индекс сетевого интерфейса
			entry.index = ::if_nametoindex(ifa->ifa_name);
			// Если адрес является IPv4
			if(family == AF_INET)
				// Сохраняем IPv4-адрес интерфейса
				entry.address4 = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
			// Если адрес является IPv6
			else ::memcpy(entry.address6.data(), &reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr, entry.address6.size());
			// Добавляем запись в список
			entries.push_back(std::move(entry));
		}
		// Освобождаем память списка сетевых интерфейсов
		::freeifaddrs(ptr);
		/**
		 * Публикуем результат в кеш под эксклюзивной блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу сетевых интерфейсов на запись
			const locker_t <std::shared_mutex> lock(::__awh_iface_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Сохраняем собранный список интерфейсов в кеш
			::__awh_iface_cache__.entries = std::move(entries);
			// Устанавливаем время истечения кеша
			::__awh_iface_cache__.expire = (now + AWH_IFACE_CACHE_TTL);
		}
		// Все удачно
		return true;
	}

	/**
	 * @brief Функция получения IPv4-адреса активного сетевого интерфейса по его имени
	 *
	 * @param out  переменная для записи найденного IPv4-адреса (сетевой порядок байт)
	 * @param fmk  объект фреймворка
	 * @param name имя сетевого интерфейса
	 * @return     результат поиска IPv4-адреса
	 *
	 */
	static bool resolveIfaceIPv4(uint32_t & out, const fmk_t * fmk, string_view name) noexcept {
		// Текущая метка времени в миллисекундах
		const uint64_t now = fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
		/**
		 * Выполняем до двух попыток: чтение из кеша и повтор после принудительного обновления
		 */
		for(uint8_t attempt = 0; attempt < 2; ++attempt){
			// Флаг актуальности кеша
			bool fresh = false;
			{
				// Блокируем доступ к глобальному кешу сетевых интерфейсов на чтение
				const locker_t <std::shared_mutex> lock(::__awh_iface_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
				// Определяем актуальность кеша
				fresh = (::__awh_iface_cache__.expire > now);
				// Если кеш актуален
				if(fresh){
					/**
					 * Перебираем закешированные интерфейсы
					 */
					for(const IfaceEntry & entry : ::__awh_iface_cache__.entries){
						// Если найден активный IPv4-интерфейс с искомым именем
						if((entry.family == AF_INET) && entry.up && fmk->compare(entry.name, name)){
							// Сохраняем найденный IPv4-адрес
							out = entry.address4;
							// Сообщаем об успехе
							return true;
						}
					}
				}
			}
			// Если кеш был актуален, но интерфейс не найден, дальнейший поиск бессмысленен
			if(fresh)
				// Выходим
				return false;
			// Кеш устарел - выполняем его принудительное обновление
			if(!refreshIfaceCache(now))
				// Если обновить кеш не удалось, выходим
				return false;
		}
		// Интерфейс не найден
		return false;
	}

	/**
	 * @brief Функция получения индекса сетевого интерфейса по его IPv6-адресу
	 *
	 * @param fmk     объект фреймворка
	 * @param address IPv6-адрес сетевого интерфейса (16 байт)
	 * @return        индекс сетевого интерфейса (0 - интерфейс не найден)
	 *
	 */
	static uint32_t resolveIfaceIndexIPv6(const fmk_t * fmk, const uint8_t * address) noexcept {
		// Текущая метка времени в миллисекундах
		const uint64_t now = fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
		/**
		 * Выполняем до двух попыток: чтение из кеша и повтор после принудительного обновления
		 */
		for(uint8_t attempt = 0; attempt < 2; ++attempt){
			// Флаг актуальности кеша
			bool fresh = false;
			{
				// Блокируем доступ к глобальному кешу сетевых интерфейсов на чтение
				const locker_t <std::shared_mutex> lock(::__awh_iface_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
				// Определяем актуальность кеша
				fresh = (::__awh_iface_cache__.expire > now);
				// Если кеш актуален
				if(fresh){
					/**
					 * Перебираем закешированные интерфейсы
					 */
					for(const IfaceEntry & entry : ::__awh_iface_cache__.entries){
						// Если найден активный IPv6-интерфейс с искомым адресом
						if((entry.family == AF_INET6) && entry.up && (::memcmp(entry.address6.data(), address, entry.address6.size()) == 0))
							// Возвращаем индекс найденного интерфейса
							return entry.index;
					}
				}
			}
			// Если кеш был актуален, но интерфейс не найден, дальнейший поиск бессмысленен
			if(fresh)
				// Выходим
				return 0;
			// Кеш устарел - выполняем его принудительное обновление
			if(!refreshIfaceCache(now))
				// Если обновить кеш не удалось, выходим
				return 0;
		}
		// Интерфейс не найден
		return 0;
	}
};

/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::eth::Socket::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с глобальным кешом сетевых интерфейсов
	::__awh_iface_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод получения кода ошибки
 *
 * @param sock сетевой сокет
 * @return     код ошибки на сокете если присутствует
 *
 */
int32_t awh::eth::Socket::getError(const net::socket_t sock) const noexcept {
	// Переменная результата
	int32_t result = -1;
	// Размер кода ошибки
	socklen_t size = sizeof(result);
	// Если мы получили ошибку, выводим сообщение
	if(::getsockopt(sock, SOL_SOCKET, SO_ERROR, &result, &size) != 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(sock),
				log_t::flag_t::CRITICAL,
				::strerror(errno)
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения таймаута сокета
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @return      время таймаута в миллисекундах
 *
 */
uint32_t awh::eth::Socket::getTimeout(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Создаём объект таймаута
	struct timeval timeout{0};
	// Получаем размер объекта таймаута
	socklen_t length = sizeof(timeout);
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо установить таймаут на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Считываем установленный размер таймаута на чтение данных из сокета
			if(::getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event)
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо установить таймаут на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Считываем установленный размер таймаута на запись данных в сокет
			if(::getsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event)
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Возвращаем итоговый результат в миллисекундах
	return (
		(static_cast <uint32_t> (timeout.tv_sec) * 1000U) +
		(static_cast <uint32_t> (timeout.tv_usec) / 1000U)
	);
}
/**
 * @brief Метод установки таймаута сокета
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @param msec  время таймаута в миллисекундах
 * @return      результат установки таймаута
 *
 */
bool awh::eth::Socket::setTimeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept {
	// Переменная результата
	bool result = false;
	// Создаём объект таймаута
	struct timeval timeout{0};
	// Устанавливаем время в секундах
	timeout.tv_sec = (msec > 0 ? (msec / 1000) : 0);
	// Устанавливаем время счётчика (микросекунды)
	timeout.tv_usec = (msec > 0 ? ((msec % 1000) * 1000) : 0);
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо установить таймаут на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Выполняем установку таймаута на чтение данных из сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event),
							msec
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо установить таймаут на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Выполняем установку таймаута на запись данных в сокет
			if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event),
							msec
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Все удачно
	return result;
}
/**
 * @brief Метод получения размера буфера
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @return      размер буфера сокета
 *
 */
int32_t awh::eth::Socket::getBufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Переменная результата
	int32_t result = 0;
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо получить размер буфера на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера
			if(::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event)
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо получить размер буфера на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера
			if(::getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event)
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки размеров буфера
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @param size  размер буфера сокета
 * @return      установленный размер буфера сокета
 *
 */
int32_t awh::eth::Socket::setBufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept {
	// Переменная результата
	int32_t result = -1;
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо установить размер буфера на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Устанавливаем размер буфера на чтение
			if(::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event),
							size
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Установка прошла успешно, поэтому в качестве запасного значения используем запрошенный размер
			result = size;
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера на чтение
			if(::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event),
							size
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо установить размер буфера на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Устанавливаем размер буфера на запись
			if(::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event),
							size
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Установка прошла успешно, поэтому в качестве запасного значения используем запрошенный размер
			result = size;
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера
			if(::getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"%s", __PRETTY_FUNCTION__,
						make_tuple(
							sock,
							static_cast <uint16_t> (event),
							size
						), log_t::flag_t::CRITICAL,
						::strerror(errno)
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки сетевого интерфейса для multicast пакетов
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param ifname имя сетевого интерфейса
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::setMulticastIface(const net::socket_t sock, const event::family_t family, string_view ifname) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса не пустое
	if(!ifname.empty()){
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// IPv4-адрес найденного сетевого интерфейса
				uint32_t address = 0;
				// Получаем IPv4-адрес активного сетевого интерфейса по его имени (из кеша или через getifaddrs)
				if(!::resolveIfaceIPv4(address, this->_fmk, ifname)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"Unable to resolve address of network interface",
							__PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								ifname
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to resolve address of network interface", log_t::flag_t::WARNING);
					#endif
					// Возвращаем пустой результат
					return result;
				}
				// Создаём объект сетевого интерфейса
				struct in_addr iface = {};
				// Присваиваем найденный IP-адрес
				iface.s_addr = address;
				// Устанавливаем сетевой интерфейс для multicast пакетов
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								ifname
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Формируем нуль-терминированную строку имени интерфейса (string_view может не быть нуль-терминированной)
				const string name(ifname);
				// Получаем индекс сетевого интерфейса по его имени
				const uint32_t index = ::if_nametoindex(name.c_str());
				// Если индекс сетевого интерфейса получить не удалось
				if(index == 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"Unable to get index of network interface",
							__PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								ifname
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to get index of network interface", log_t::flag_t::WARNING);
					#endif
					// Возвращаем пустой результат
					return result;
				}
				// Устанавливаем сетевой интерфейс для multicast пакетов
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, sizeof(index))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								ifname
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
		}
	// Если название сетевого интерфейса пустое
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"Interface name is empty",
				__PRETTY_FUNCTION__,
				make_tuple(
					sock,
					static_cast <uint16_t> (family),
					ifname
				), log_t::flag_t::WARNING
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Interface name is empty", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод устанавливает постоянное подключение на сокет
 *
 * @param sock  сетевой сокет
 * @param cnt   максимальное количество попыток
 * @param idle  время через которое происходит проверка подключения
 * @param intvl время между попытками
 * @return      результат работы функции
 *
 */
bool awh::eth::Socket::setKeepalive(const net::socket_t sock, int32_t cnt, int32_t idle, int32_t intvl) const noexcept {
	// Переменная результата
	bool result = false;
	// Если максимальное количество попыток передано неправильно
	if(cnt < 0)
		// Выполняем компенсацию
		cnt = 0;
	// Если время через которое происходит проверка подключения передано неправильно
	if(idle < 0)
		// Выполняем компенсацию
		idle = 0;
	// Если время между попытками передано неправильно
	if(intvl < 0)
		// Выполняем компенсацию
		intvl = 0;
	// Устанавливаем параметр
	int32_t keepAlive = 1;
	// Активация постоянного подключения
	if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					sock, cnt,
					idle, intvl
				), log_t::flag_t::WARNING,
				::strerror(errno)
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	/**
	 * Посокетная настройка сроков проверки живости
	 *
	 * @details Задать их вправе не всякая система. У OpenBSD опций TCP_KEEPIDLE,
	 *          TCP_KEEPINTVL и TCP_KEEPCNT нет вовсе: сроки задаются на всю машину
	 *          настройками net.inet.tcp.keepidle и net.inet.tcp.keepintvl, а сокету
	 *          достаётся лишь включение проверки целиком - оно выполнено выше и
	 *          работает везде
	 *
	 * @warning Трогать общесистемные настройки отсюда нельзя: они принадлежат машине,
	 *          а не нашему сокету, и правка их задела бы всякую службу на ней
	 *
	 * @note Замена посокетным срокам есть, но лежит она выше уровня сокета: проверку
	 *       живости своими сроками ведёт сам движок, у него для этого есть и учёт
	 *       сроков, и отправка. Здесь же честно сообщается, что заданные сроки взяты
	 *       системой из своих умолчаний
	 *
	 */
	#if !__OpenBSD__
	// Максимальное количество попыток
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					sock, cnt,
					idle, intvl
				), log_t::flag_t::WARNING,
				::strerror(errno)
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	/**
	 * Если мы работаем в macOS
	 */
	#if __APPLE__
		// Время через которое происходит проверка подключения
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(
						sock, cnt,
						idle, intvl
					), log_t::flag_t::WARNING,
					::strerror(errno)
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			#endif
			// Выходим из функции
			return result;
		}
	/**
	 * Если мы работаем в Linux, FreeBSD, NetBSD или OpenBSD или Sun Solaris
	 */
	#elif __FreeBSD__ || __NetBSD__
		// Время через которое происходит проверка подключения
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(
						sock, cnt,
						idle, intvl
					), log_t::flag_t::WARNING,
					::strerror(errno)
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			#endif
			// Выходим из функции
			return result;
		}
	#endif
	// Время между попытками
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					sock, cnt,
					idle, intvl
				), log_t::flag_t::WARNING,
				::strerror(errno)
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	/**
	 * Если система посокетных сроков проверки живости не даёт
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Сообщаем, что заданные сроки взяты системой из своих умолчаний
			this->_log->debug(
				"Per-socket keep-alive tuning is unavailable, system defaults applied",
				__PRETTY_FUNCTION__,
				make_tuple(
					sock, cnt,
					idle, intvl
				), log_t::flag_t::INFO
			);
		#endif
	#endif
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       значение DSCP
 *
 */
awh::event::dscp_t awh::eth::Socket::getDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family) const noexcept {
	// Переменная результата
	event::dscp_t result = event::dscp_t::CS0;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		// Прочитать текущее значение
		int32_t tclass = 0;
		// Получаем размер установленного размера буфера
		socklen_t length = sizeof(tclass);
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Считываем установленный размер буфера
				if(::getsockopt(sock, IPPROTO_IP, IP_TOS, &tclass, &length) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family)
							), log_t::flag_t::CRITICAL,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Считываем установленный размер буфера
				if(::getsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS, &tclass, &length) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family)
							), log_t::flag_t::CRITICAL,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
		}
		// Маскируем значение поля Traffic Class (TC) для получения только DSCP
		/**
		 * Класс обслуживания занимает старшие шесть разрядов октета, а младшие два
		 * отданы признаку перегрузки, потому прочитанный октет сдвигается на два
		 * разряда вправо
		 *
		 * @warning Прежде сдвига здесь не было, а октет лишь чистился маской 0xFC. При
		 *          такой записи кодовая точка теряла два младших разряда: замер отвечал
		 *          44 на запрошенное EF, равное 46. Ошибка была вчетверо и у всякой
		 *          кодовой точки, не только у этой
		 */
		result = static_cast <event::dscp_t> ((tclass >> 2) & 0x3F);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param dscp   значение DSCP
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::setDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family, const event::dscp_t dscp) const noexcept {
	// Переменная результата
	bool result = false;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		/**
		 * Класс обслуживания (DSCP) и признак перегрузки (ECN) занимают один октет
		 * заголовка, поэтому установка выполняется чтением текущего значения с
		 * заменой только старших шести бит: запись целого октета сбросила бы
		 * установленный признак перегрузки
		 */
		const int32_t tclass = ((static_cast <int32_t> (dscp) << 2) | static_cast <int32_t> (this->getExplicitCongestionNotification(sock, family)));
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Устанавливаем значение поля Traffic Class (TC) в заголовке IPv4-пакета, которое включает в себя DSCP
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_TOS, &tclass, sizeof(tclass))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (dscp)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Устанавливаем значение поля Traffic Class (TC) в заголовке IPv6-пакета, которое включает в себя DSCP
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS, &tclass, sizeof(tclass))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (dscp)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
 *
 * @note Выдаёт значение, устанавливаемое на исходящих пакетах. Признак
 *       перегрузки принятых пакетов приходит отдельно для каждой
 *       датаграммы в метаданных дейтаграммного пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       значение ECN
 *
 */
awh::event::ecn_t awh::eth::Socket::getExplicitCongestionNotification(const net::socket_t sock, const event::family_t family) const noexcept {
	// Переменная результата
	event::ecn_t result = event::ecn_t::NOT_ECT;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		// Прочитать текущее значение
		int32_t tclass = 0;
		// Получаем размер прочитанного значения
		socklen_t length = sizeof(tclass);
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Считываем значение поля Type Of Service (TOS) заголовка IPv4-пакета
				if(::getsockopt(sock, IPPROTO_IP, IP_TOS, &tclass, &length) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family)
							), log_t::flag_t::CRITICAL,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Считываем значение поля Traffic Class (TC) заголовка IPv6-пакета
				if(::getsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS, &tclass, &length) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family)
							), log_t::flag_t::CRITICAL,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
		}
		// Маскируем значение поля Traffic Class (TC) для получения только ECN
		result = static_cast <event::ecn_t> (tclass & 0x03);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
 *
 * @note Класс обслуживания (DSCP) сохраняется: оба поля занимают один
 *       октет заголовка, поэтому установка выполняется чтением текущего
 *       значения с последующей заменой только младших двух бит
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param ecn    значение ECN
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::setExplicitCongestionNotification(const net::socket_t sock, const event::family_t family, const event::ecn_t ecn) const noexcept {
	// Переменная результата
	bool result = false;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		/**
		 * Класс обслуживания (DSCP) и признак перегрузки (ECN) занимают один октет
		 * заголовка, поэтому установка выполняется чтением текущего значения с
		 * заменой только младших двух бит: запись целого октета сбросила бы
		 * настроенный класс обслуживания
		 */
		const int32_t tclass = ((static_cast <int32_t> (this->getDifferentiatedServicesCodePoint(sock, family)) << 2) | static_cast <int32_t> (ecn));
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				/**
				 * @par Намеренные решения
				 *
				 * Отметка выставляется одинаково на всех системах, в том числе там, где
				 * ядро признак перегрузки принятых IPv4-пакетов не выдаёт (NetBSD и
				 * OpenBSD не имеют IP_RECVTOS). Прежде на этих системах отметка молча
				 * не выставлялась, а метод отвечал согласием: установка признака
				 * расходилась с его последующим чтением, и потребитель узнать об этом
				 * не мог. Довод был в том, что метить пакеты, не видя обратных отметок,
				 * бесполезно, - но решение это принадлежит потребителю, а не установщику
				 * параметра сокета: QUIC спрашивает доступность отдельно через
				 * availableExplicitCongestionNotification() и сам отключает ECN там,
				 * где обратной связи не будет
				 *
				 * Класс обслуживания (DSCP) задаётся отдельно методом
				 * setDifferentiatedServicesCodePoint(), и здесь он лишь сохраняется
				 * нетронутым: оба поля делят один октет заголовка
				 *
				 */
				// Устанавливаем значение поля Type Of Service (TOS) заголовка IPv4-пакета
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_TOS, &tclass, sizeof(tclass))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (ecn)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Устанавливаем значение поля Traffic Class (TC) заголовка IPv6-пакета
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS, &tclass, sizeof(tclass))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (ecn)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод активации/деактивации генерации информации о трафике
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим активации или деактивации
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::trafficInfoGeneration(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept {
	// Переменная результата
	bool result = false;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		// Флаги установки опции
		int32_t flags = 0;
		/**
		 * Определяем режим блокировки
		 */
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо активировать заголовки в сокете
			case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
				// Устанавливаем флаг активации
				flags = 1;
			break;
			// Если необходимо деактивировать заголовки в сокете
			case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
				// Устанавливаем флаг деактивации
				flags = 0;
			break;
		}
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Флаг выполнения операции
				bool ok1 = false, ok2 = false;
				// Активируем/деактивируем генерацию информации о хопах (TTL) в сокете
				if(!(ok1 = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_RECVTTL, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
				// Тип сокета для определения его семейства
				int32_t socktype = 0;
				// Получаем размер буфера для извлечения типа сокета
				socklen_t length = sizeof(socktype);
				// Считываем тип сокета для определения его семейства
				if(!(ok2 = !static_cast <bool> (::getsockopt(sock, SOL_SOCKET, SO_TYPE, &socktype, &length)))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::CRITICAL,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
				/**
				 * Если сокет является дейтаграммным либо RAW-сокетом: класс обслуживания
				 * принятого пакета выдаётся служебным сообщением только для них, а
				 * потоковым сокетам он и не нужен - разбирать заголовки IP-пакетов
				 * потока приложению не приходится
				 */
				if(ok2 && ((socktype == SOCK_RAW) || (socktype == SOCK_DGRAM))){
				/**
				 * Выдача класса обслуживания принятого пакета служебным сообщением
				 *
				 * @details Опции IP_RECVTOS нет ни у NetBSD, ни у OpenBSD, и подменить её
				 *          нечем: соседние IP_RECVTTL, IP_RECVDSTADDR и IP_RECVIF выдают
				 *          что угодно, кроме класса обслуживания, а из самого пакета
				 *          прочесть его дейтаграммному сокету неоткуда - заголовок IP ядро
				 *          ему не отдаёт
				 *
				 * @warning Подставлять сюда ноль нельзя. Ноль - это законный класс
				 *          обслуживания CS0, и вызывающий принял бы выдуманное значение за
				 *          прочитанное. Эмуляция здесь выродилась бы в ложь, поэтому
				 *          настройка отвечает согласием, а класс обслуживания у принятых
				 *          пакетов остаётся просто не выданным
				 *
				 */
				#if __NetBSD__ || __OpenBSD__
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Сообщаем, что класс обслуживания принятых пакетов система не выдаёт
						this->_log->debug(
							"IP_RECVTOS is unavailable, service class of received packets is not reported",
							__PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::INFO
						);
					#endif
				#else
					// Активируем/деактивируем генерацию информации о типе сервиса (TOS) в сокете
					if(!(ok2 = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_RECVTOS, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				#endif
				}
				// Формируем итоговый результат
				result = (ok1 && ok2);
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Тип сокета для определения его семейства
				int32_t socktype = 0;
				// Получаем размер буфера для извлечения типа сокета
				socklen_t length = sizeof(socktype);
				// Считываем тип сокета для определения его семейства
				if(!(result = !static_cast <bool> (::getsockopt(sock, SOL_SOCKET, SO_TYPE, &socktype, &length)))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::CRITICAL,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
				// Если сокет не является RAW-сокетом
				if(socktype != SOCK_RAW){
					// Флаг выполнения операции
					bool ok1 = false, ok2 = false, ok3 = false;
					// Активируем/деактивируем генерацию информации о хопах (Hop Limit) в сокете
					if(!(ok1 = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
					// Активируем/деактивируем генерацию информации о типе трафика (Traffic Class) в сокете
					if(!(ok2 = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_RECVTCLASS, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
					// Активируем/деактивируем генерацию информации о пакете (Packet Info) в сокете
					if(!(ok3 = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
					// Формируем итоговый результат
					result = (ok1 && ok2 && ok3);
				}
			} break;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод переключения опции сокета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим активации или деактивации
 * @param option опция сокета
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::switchOption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option) const noexcept {
	// Переменная результата
	bool result = false;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		/**
		 * Определяем опции сокета которые необходимо установить
		 */
		switch(option){
			// Если необходимо установить опцию HDRINCL
			case event::options::HDRINCL: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4): {
						// Активируем/деактивируем заголовки в сокете
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Для IPv6 протокола опция HDRINCL не поддерживается, поэтому просто устанавливаем положительный результат
						result = true;
					break;
				}
			} break;
			// Если необходимо установить опцию TCP CORK
			case event::options::TCP_CORKING: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Включаем/отключаем придержку отправки
				 *
				 * @details Имя механизма у каждой породы систем своё: у BSD и macOS это
				 *          TCP_NOPUSH, у Linux - TCP_CORK. Наружу выставлено одно имя
				 *          опции, а backend подставляет то, что даёт система
				 *
				 * @note У NetBSD нет ни того, ни другого, и ближайшее, что даёт там сам
				 *       TCP, - алгоритм Нейгла: назначение у него то же, копить мелкие
				 *       отправки до заполнения сегмента. Придержка включается снятием
				 *       TCP_NODELAY, снимается - его возвратом
				 *
				 * @warning Равенством это не является: придержка держит до явного снятия,
				 *          Нейгл - лишь до подтверждения предыдущего сегмента. Обмен,
				 *          которому придержка нужна как строгая, на NetBSD получит
				 *          приближение, а не её саму
				 *
				 */
				#if __NetBSD__
					// Придержке отправки соответствует включённый алгоритм Нейгла, снятию - выключенный
					const int32_t nodelay = (flags ? 0 : 1);
					// Включаем/отключаем алгоритм Нейгла вместо придержки отправки
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay))))){
				#else
					// Включаем/отключаем придержку отправки
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NOPUSH, &flags, sizeof(flags))))){
				#endif
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо отключить алгоритм Нейгла
			case event::options::TCP_NO_DELAY: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Если операционной системой является FreeBSD
				 */
				#if __FreeBSD__
					/**
					 * Определяем протокол сокета
					 *
					 * @details Названный вызывающим протокол принимается как есть: он завёл
					 *          этот сокет сам и знает о нём больше, чем ядро сообщит
					 *          настройкой. Обращение к ядру ради уже известного стоило
					 *          одного системного вызова на каждую установку опции
					 *
					 * @note Неназванный протокол разыскивается у сокета по-прежнему:
					 *       обращений к методу много, и не всякому из них протокол известен
					 *
					 */
					int32_t protocol = 0;
					// Признак того, что протокол сокета определён
					bool resolved = true;
					/**
					 * Определяем названный вызывающим протокол
					 */
					switch(static_cast <uint8_t> (proto)){
						// Если протокол интернета установлен как TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Запоминаем протокол сокета
							protocol = IPPROTO_TCP;
						break;
						// Если протокол интернета установлен как UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
							// Запоминаем протокол сокета
							protocol = IPPROTO_UDP;
						break;
						// Если протокол интернета установлен как SCTP
						case static_cast <uint8_t> (event::protocol_t::SCTP):
							// Запоминаем протокол сокета
							protocol = IPPROTO_SCTP;
						break;
						// Если протокол вызывающим не назван
						default: {
							// Длина протокола сокета
							socklen_t length = sizeof(protocol);
							// Получаем протокол сокета у ядра
							resolved = (::getsockopt(sock, SOL_SOCKET, SO_PROTOCOL, &protocol, &length) == 0);
						}
					}
					// Если протокол сокета определён
					if(resolved){
						/**
						 * Определяем протокол сокета
						 */
						switch(protocol){
							// Если протокол TCP
							case IPPROTO_TCP: {
								// Активируем/деактивируем алгоритм Нейгла
								if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug(
											"%s", __PRETTY_FUNCTION__,
											make_tuple(
												sock,
												static_cast <uint16_t> (family),
												static_cast <uint16_t> (mode),
												option
											), log_t::flag_t::WARNING,
											::strerror(errno)
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
									#endif
								}
							} break;
							// Если протокол SCTP
							case IPPROTO_SCTP: {
								// Активируем/деактивируем алгоритм Нейгла
								if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_NODELAY, &flags, sizeof(flags))))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug(
											"%s", __PRETTY_FUNCTION__,
											make_tuple(
												sock,
												static_cast <uint16_t> (family),
												static_cast <uint16_t> (mode),
												option
											), log_t::flag_t::WARNING,
											::strerror(errno)
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
									#endif
								}
							} break;
						}
					// Если возникает ошибка получения протокола сокета
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				/**
				 * Для остальных операционных систем
				 */
				#else
					// Активируем/деактивируем алгоритм Нейгла
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				#endif
			} break;
			// Если необходимо установить опцию IPV6 ONLY
			case event::options::IPV6_ONLY: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Разрешаем/запрещаем отображение IPv4 => IPv6
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо отключить сигнал SIGILL
			case event::options::NO_SIGILL: {
				// Создаем структуру активации сигнала
				struct sigaction act{0};
				// Обнуляем маску блокируемых сигналов
				sigemptyset(&act.sa_mask);
				/**
				 * Устанавливаем флаги перезагрузки
				 *
				 * @warning Признак SA_SIGINFO здесь **не ставится** намеренно: он
				 *          обязывает заполнять sa_sigaction, а заполняется sa_handler,
				 *          и поля эти у большинства систем лежат в одном объединении.
				 *          Для SIG_IGN и SIG_DFL последствий нет - ядро смотрит на само
				 *          значение, - но признак противоречил бы тому, что задано
				 */
				act.sa_flags = (SA_ONSTACK | SA_RESTART);
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
						// Устанавливаем макрос игнорирования сигнала
						act.sa_handler = SIG_IGN;
						// Устанавливаем блокировку сигнала
						if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr)))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
						// Устанавливаем макрос по умолчанию для сигнала
						act.sa_handler = SIG_DFL;
						// Снимаем блокировку сигнала
						if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr)))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
				}
			} break;
			// Если необходимо установить опцию BROADCAST
			case event::options::BROADCAST: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Активируем/деактивируем широковещательный адрес
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо отключить сигнал SIGPIPE
			case event::options::NO_SIGPIPE: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Устанавливаем/снимаем игнорирование отключения сигнала записи в убитый сокет
				 *
				 * @details У OpenBSD опции SO_NOSIGPIPE нет вовсе - ни в заголовках, ни в
				 *          ядре. Замена ей там та же, что модуль применяет для систем, где
				 *          посокетного способа не заведено: сигнал глушится обработчиком
				 *          через sigaction, ровно как выше сделано для SIGILL
				 *
				 * @warning Способ этот не посокетный, а на весь процесс, и молчать сигнал
				 *          будет для всякого сокета, а не только для этого. Иного OpenBSD
				 *          не даёт, а последствие у настройки то же самое: запись в
				 *          оборванное соединение отвечает отказом вместо гибели процесса
				 *
				 * @note Отвечать здесь согласием, не сделав ничего, нельзя. Настройка эта
				 *       бережёт процесс от гибели, и мнимое её выполнение оставило бы
				 *       вызывающего беззащитным ровно там, где он считает себя укрытым
				 *
				 */
				#if __OpenBSD__
					// Создаем структуру активации сигнала
					struct sigaction act{0};
					// Обнуляем маску блокируемых сигналов
					sigemptyset(&act.sa_mask);
					/**
					 * Устанавливаем флаги перезагрузки
					 *
					 * @warning Признак SA_SIGINFO здесь **не ставится** намеренно: он
					 *          обязывает заполнять sa_sigaction, а заполняется sa_handler,
					 *          и поля эти у большинства систем лежат в одном объединении.
					 *          Для SIG_IGN и SIG_DFL последствий нет - ядро смотрит на само
					 *          значение, - но признак противоречил бы тому, что задано
					 */
					act.sa_flags = (SA_ONSTACK | SA_RESTART);
					// Глушим сигнал при включении настройки и возвращаем его обработку при снятии
					act.sa_handler = (flags ? SIG_IGN : SIG_DFL);
					// Устанавливаем обработку сигнала записи в оборванное соединение
					if(!(result = !static_cast <bool> (::sigaction(SIGPIPE, &act, nullptr)))){
				#else
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &flags, sizeof(flags))))){
				#endif
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо перевести сокет в неблокирующий режим
			case event::options::NO_IO_BLOCK:
			// Если необходимо перевести сокет в полублокирующий режим
			case event::options::SM_IO_BLOCK: {
				// Флаги установки опции
				int32_t flags = 0;
				// Получаем флаги сетевого сокета
				if(!(result = ((flags = ::fcntl(sock, F_GETFL, nullptr)) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо перевести сокет в неблокирующий режим
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
						// Если флаг ещё не установлен
						if(!(result = (flags & O_NONBLOCK))){
							// Устанавливаем неблокирующий режим
							if(!(result = (::fcntl(sock, F_SETFL, flags | O_NONBLOCK) >= 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					} break;
					// Если необходимо перевести сокет в блокирующий режим
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
						// Если флаг уже установлен
						if(!(result = !(flags & O_NONBLOCK))){
							// Снимаем неблокирующий режим
							if(!(result = (::fcntl(sock, F_SETFL, flags ^ O_NONBLOCK) >= 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					} break;
				}
			} break;
			// Если необходимо установить опцию переиспользования адреса
			case event::options::REUSE_ADDR: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Разрешаем/запрещаем повторно использовать тот же сокет после отключения
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо установить опцию переиспользования порта
			case event::options::REUSE_PORT: {
				/**
				 * Для юникс-сокетов настройка эта смысла не имеет
				 *
				 * @details Переиспользование порта описывает поведение при связывании
				 *          нескольких сокетов с одним портом, а у юникс-сокета порта нет
				 *          вовсе - он связывается с путём в файловой системе. Настройка
				 *          неприложима к нему по устройству, а не по недостатку поддержки.
				 *
				 *          BSD-системы принимают её на таком сокете вхолостую, поэтому
				 *          обращение к ядру здесь заведомо ничего не меняет и лишь тратит
				 *          системный вызов. Ядро Linux ту же настройку отвергает отказом,
				 *          из-за чего в движке для Linux отсечка эта обязательна - здесь
				 *          же она стоит ради единообразия поведения и экономии вызова
				 *
				 * @note Договор движка один на все системы: отсечка стоит в обоих движках
				 *       на одном и том же месте, чтобы поведение их не расходилось
				 *
				 */
				if(family == event::family_t::UDS)
					// Выводим успешный результат, не обращаясь к ядру
					return true;
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Если операционной системой является FreeBSD
				 */
				#if __FreeBSD__
					/**
					 * Если версия FreeBSD 12.0 или выше
					 */
					#if __FreeBSD_version >= 1200000
						// Разрешаем/запрещаем использовать один и тот же порт (с возможностью балансировки нагрузки) для нескольких сокетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT_LB, &flags, sizeof(flags))))){
							// Разрешаем/запрещаем использовать один и тот же порт для нескольких сокетов
							if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &flags, sizeof(flags))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					/**
					 * Если версия FreeBSD ниже 12.0
					 */
					#else
						// Разрешаем/запрещаем использовать один и тот же порт для нескольких сокетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					#endif
				/**
				 * Если операционная система поддерживает SO_REUSEPORT
				 */
				#elif SO_REUSEPORT
					// Разрешаем/запрещаем использовать один и тот же порт для нескольких сокетов
					if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				#endif
			} break;
			// Если необходимо установить опцию CLOSE ON EXEC
			case event::options::CLOSE_ON_EXEC: {
				// Флаги установки опции
				int32_t flags = 0;
				// Получаем флаги сетевого сокета
				if(!(result = ((flags = ::fcntl(sock, F_GETFD, nullptr)) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать режим закрытия сокета после запуска
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
						// Если флаг ещё не установлен
						if(!(result = (flags & FD_CLOEXEC))){
							// Устанавливаем режим закрытия сокета после запуска
							if(!(result = (::fcntl(sock, F_SETFD, flags | FD_CLOEXEC) >= 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					} break;
					// Если необходимо деактивировать режим закрытия сокета после запуска
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
						// Если флаг уже установлен
						if(!(result = !(flags & FD_CLOEXEC))){
							// Снимаем режим закрытия сокета после запуска
							if(!(result = (::fcntl(sock, F_SETFD, flags ^ FD_CLOEXEC) >= 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					} break;
				}
			} break;
			/**
			 * Если необходимо установить опцию немедленного обрыва соединения
			 *
			 * @details Порядок закрытия соединения задаётся ядру структурой из двух
			 *          полей: признаком задержки и её сроком. Осмысленных сочетаний
			 *          три, и опция выбирает между первым и третьим:
			 *
			 *          - признак снят: закрытие возвращает управление сразу, ядро
			 *            дописывает очередь в фоне, соединение закрывается обменом
			 *            прощаниями, а закрывшая первой сторона уходит в TIME_WAIT.
			 *            Это порядок по умолчанию;
			 *          - признак взведён, срок положителен: закрытие ожидает
			 *            подтверждения очереди соседом либо истечения срока. Здесь же
			 *            кроется ловушка переносимости, описанная ниже;
			 *          - признак взведён, срок нулевой: очередь отбрасывается,
			 *            соединение рвётся сегментом RST, TIME_WAIT не возникает ни
			 *            у одной из сторон. Это и есть режим, включаемый опцией
			 *
			 * @note Ловушка переносимости: на Darwin опция SO_LINGER принимает срок
			 *       в тиках планировщика, а не в секундах, и для срока в секундах там
			 *       заведена отдельная опция SO_LINGER_SEC. На FreeBSD, NetBSD и
			 *       OpenBSD SO_LINGER принимает секунды и второй опции нет. Ниже
			 *       поэтому выбирается SO_LINGER_SEC везде, где она объявлена: при
			 *       нулевом сроке разницы нет, но единица измерения перестаёт
			 *       зависеть от системы, и правка срока на ненулевой не обернётся
			 *       молчаливой ошибкой на Darwin
			 *
			 */
			case event::options::HARD_CLOSE: {
				/**
				 * Если система различает срок задержки в тиках и в секундах
				 */
				#if SO_LINGER_SEC
					// Выбираем опцию ядра, принимающую срок задержки в секундах
					static constexpr int32_t LINGER_OPTION = SO_LINGER_SEC;
				/**
				 * Если система принимает срок задержки единственной опцией
				 */
				#else
					// Выбираем единственную опцию ядра задержки закрытия
					static constexpr int32_t LINGER_OPTION = SO_LINGER;
				#endif
				// Параметры задержки закрытия сокета
				struct linger value{};
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать немедленный обрыв соединения
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Взводим признак задержки: вместе с нулевым сроком это отбрасывает очередь и рвёт соединение сегментом RST
						value.l_onoff = 1;
						// Устанавливаем нулевой срок задержки закрытия
						value.l_linger = 0;
					break;
					// Если необходимо деактивировать немедленный обрыв соединения
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Снимаем признак задержки: закрытие возвращается к обмену прощаниями
						value.l_onoff = 0;
						// Устанавливаем нулевой срок задержки закрытия
						value.l_linger = 0;
					break;
				}
				// Активируем/деактивируем немедленный обрыв соединения при закрытии сокета
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, LINGER_OPTION, &value, sizeof(value))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо установить опцию MULTICAST LOOPBACK
			case event::options::MULTICAST_LOOPBACK: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4): {
						/**
						 * @brief Значение признака обратной петли для IPv4
						 *
						 * @details Договор BSD отводит этому параметру для IPv4 **один
						 *          октет**, тогда как одноимённому параметру IPv6 - целое.
						 *          Прежде и тому и другому передавалось целое: macOS и
						 *          NetBSD принимают оба размера, а OpenBSD отвечает отказом
						 *          «недопустимый довод», и рассылка на петлю там не
						 *          включалась вовсе
						 *
						 */
						const uint8_t loopback = static_cast <uint8_t> (flags);
						// Активируем/деактивируем режим обратной петли для multicast пакетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6): {
						// Активируем/деактивируем режим обратной петли для multicast пакетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
				}
			} break;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения обнаружения максимального размера пакета (MTU)
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       режим обнаружения максимального размера пакета (MTU)
 *
 */
awh::event::mtu_discover_t awh::eth::Socket::getMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family) const noexcept {
	// Переменная результата
	event::mtu_discover_t result = event::mtu_discover_t::NONE;
	// Буфер для получения значения опции
	int32_t value = 0;
	// Длина буфера для получения значения опции
	socklen_t length = sizeof(value);
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Если опция установки режима обнаружения MTU доступна
			 */
			#if IP_DONTFRAG
				// Получаем режим обнаружения MTU (Dont Fragment flag)
				if(::getsockopt(sock, IPPROTO_IP, IP_DONTFRAG, &value, &length) == 0)
					// Устанавливаем полученный результат
					result = (value ? event::mtu_discover_t::DO : event::mtu_discover_t::WANT);
			#endif
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			/**
			 * Если опция установки режима обнаружения MTU доступна
			 */
			#if IPV6_DONTFRAG
				// Получаем режим обнаружения MTU (Dont Fragment flag)
				if(::getsockopt(sock, IPPROTO_IPV6, IPV6_DONTFRAG, &value, &length) == 0)
					// Устанавливаем полученный результат
					result = (value ? event::mtu_discover_t::DO : event::mtu_discover_t::WANT);
			#endif
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки обнаружения максимального размера пакета (MTU)
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим обнаружения максимального размера пакета (MTU)
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::setMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family, const event::mtu_discover_t mode) const noexcept {
	// Переменная результата
	bool result = false;
	// Значение опции
	int32_t value = 0;
	/**
	 * Определяем режим обнаружения максимального размера пакета (MTU)
	 *
	 * NOTE: В macOS и FreeBSD сетевой стек не поддерживает гранулярные режимы MTU Discover,
	 * аналогичные Linux (IP_PMTUDISC_PROBE, ADAPT и т.д.).
	 * Доступен только флаг запрета фрагментации (IP_DONTFRAG / IPV6_DONTFRAG).
	 * Поэтому мы маппим все активные режимы на включение этого флага.
	 *
	 * @par Намеренные решения
	 *
	 * У NetBSD и OpenBSD запрет фрагментации на отдельном сокете не задаётся вовсе:
	 * NetBSD имеет его только для IPv6, OpenBSD - ни для одного семейства. Правдивой
	 * замены здесь нет: обнаружение пути ведёт ядро само по своим настройкам, и
	 * приложению остаётся лишь узнать итог. Согласиться, ничего не сделав, значило бы
	 * выдать невыполненное за выполненное, поэтому метод отвечает отказом и пишет
	 * причину в журнал - потребитель узнаёт, что запрет фрагментации ему не достался,
	 * и обходится осторожным размером датаграммы
	 */
	switch(static_cast <uint8_t> (mode)){
		/**
		 * Активные режимы требуют запрета фрагментации
		 */
		// Если установлен флаг - выполнять обнаружение MTU и устанавливать оптимальный размер пакетов
		case static_cast <uint8_t> (event::mtu_discover_t::DO):
		// Если установлен флаг - выполнять обнаружение MTU и отправлять пробные пакеты для определения оптимального размера
		case static_cast <uint8_t> (event::mtu_discover_t::PROBE):
		// Если установлен флаг - выполнять адаптивное обнаружение MTU, автоматически регулируя размер пакетов на основе обратной связи от сети
		case static_cast <uint8_t> (event::mtu_discover_t::ADAPT):
		// Если установлен флаг - выполнять строгое обнаружение MTU, отбрасывая пакеты, превышающие установленный размер MTU
		case static_cast <uint8_t> (event::mtu_discover_t::STRICT):
		// Если установлен флаг - выполнять агрессивное обнаружение MTU, быстро уменьшая размер пакетов при обнаружении проблем с фрагментацией
		case static_cast <uint8_t> (event::mtu_discover_t::AGGRESSIVE):
		// Если установлен флаг - выполнять консервативное обнаружение MTU, медленно уменьшая размер пакетов при возникновении проблем с доставкой
		case static_cast <uint8_t> (event::mtu_discover_t::CONSERVATIVE):
		// Если установлен флаг - выполнять интеллектуальное обнаружение MTU, используя алгоритмы машинного обучения для оптимизации размера пакетов на основе исторических данных о сети
		case static_cast <uint8_t> (event::mtu_discover_t::SMART):
			// Устанавливаем флаг Don't Fragment
			value = 1;
		break;
		/**
		 * Пассивные режимы разрешают фрагментацию
		 */
		// Если установлен флаг - не выполнять обнаружение MTU
		case static_cast <uint8_t> (event::mtu_discover_t::DONT):
		// Если установлен флаг - выполнять обнаружение MTU
		case static_cast <uint8_t> (event::mtu_discover_t::WANT):
		default:
			// Снимаем флаг Don't Fragment
			value = 0;
		break;
	}
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Если опция установки режима обнаружения MTU доступна
			 */
			#if IP_DONTFRAG
				// Устанавливаем режим обнаружения MTU (Dont Fragment flag)
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_DONTFRAG, &value, sizeof(value))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			/**
			 * Если запрет фрагментации на отдельном сокете системой не задаётся
			 */
			#else
				// Записываем причину отказа в журнал
				this->_log->print("Per-socket fragmentation control is not supported by the system for IPv4", log_t::flag_t::WARNING);
			#endif
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			/**
			 * Если опция установки режима обнаружения MTU доступна
			 */
			#if IPV6_DONTFRAG
				// Устанавливаем режим обнаружения MTU (Dont Fragment flag)
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_DONTFRAG, &value, sizeof(value))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			/**
			 * Если запрет фрагментации на отдельном сокете системой не задаётся
			 */
			#else
				// Записываем причину отказа в журнал
				this->_log->print("Per-socket fragmentation control is not supported by the system for IPv6", log_t::flag_t::WARNING);
			#endif
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param sock     сетевой сокет
 * @param family   семейство протоколов (IPv4 или IPv6)
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         максимальное количество хопов
 *
 */
uint8_t awh::eth::Socket::getHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery) const noexcept {
	// Переменная результата
	uint8_t result = 0;
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Определяем режим трансляции пакетов
			 */
			switch(static_cast <uint8_t> (delivery)){
				// Если необходимо установить максимальное количество хопов для unicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::UNICAST):
				// Если необходимо установить максимальное количество хопов для broadcast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::BROADCAST): {
					// Флаг для извлечения максимального количества хопов
					int32_t flag = 0;
					// Размер буфера для получения значения опции
					socklen_t length = sizeof(flag);
					// Считываем максимальное количество хопов, через которые может пройти пакет
					if(::getsockopt(sock, IPPROTO_IP, IP_TTL, &flag, &length) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (delivery)
								), log_t::flag_t::CRITICAL,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
					// Формируем итоговый результат
					result = static_cast <uint8_t> (flag);
				} break;
				// Если необходимо установить максимальное количество хопов для multicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::MULTICAST): {
					// Размер буфера для получения значения опции
					socklen_t length = sizeof(result);
					// Считываем максимальное количество хопов, через которые может пройти пакет
					if(::getsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &result, &length) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (delivery)
								), log_t::flag_t::CRITICAL,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				} break;
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			/**
			 * Определяем режим трансляции пакетов
			 */
			switch(static_cast <uint8_t> (delivery)){
				// Если необходимо установить максимальное количество хопов для unicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::UNICAST):
				// Если необходимо установить максимальное количество хопов для broadcast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::BROADCAST): {
					// Флаг для извлечения максимального количества хопов
					int32_t flag = 0;
					// Размер буфера для получения значения опции
					socklen_t length = sizeof(flag);
					// Считываем максимальное количество хопов, через которые может пройти пакет
					if(::getsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &flag, &length) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (delivery)
								), log_t::flag_t::CRITICAL,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
					// Формируем итоговый результат
					result = static_cast <uint8_t> (flag);
				} break;
				// Если необходимо установить максимальное количество хопов для multicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::MULTICAST): {
					// Флаг для извлечения максимального количества хопов
					int32_t flag = 0;
					// Размер буфера для получения значения опции
					socklen_t length = sizeof(flag);
					// Считываем максимальное количество хопов, через которые может пройти пакет
					if(::getsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &flag, &length) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (delivery)
								), log_t::flag_t::CRITICAL,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
					// Формируем итоговый результат
					result = static_cast <uint8_t> (flag);
				} break;
			}
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param sock     сетевой сокет
 * @param family   семейство протоколов (IPv4 или IPv6)
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @param hops     максимальное количество хопов
 * @return         результат работы функции
 *
 */
bool awh::eth::Socket::setHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const uint8_t hops) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Определяем режим трансляции пакетов
			 */
			switch(static_cast <uint8_t> (delivery)){
				// Если необходимо установить максимальное количество хопов для unicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::UNICAST):
				// Если необходимо установить максимальное количество хопов для broadcast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::BROADCAST): {
					// Получаем флаг содержания максимального количества хопов
					const int32_t flag = static_cast <int32_t> (hops);
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_TTL, &flag, sizeof(flag))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (hops)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
				// Если необходимо установить максимальное количество хопов для multicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::MULTICAST): {
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &hops, sizeof(hops))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (hops)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			/**
			 * Определяем режим трансляции пакетов
			 */
			switch(static_cast <uint8_t> (delivery)){
				// Если необходимо установить максимальное количество хопов для unicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::UNICAST):
				// Если необходимо установить максимальное количество хопов для broadcast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::BROADCAST): {
					// Получаем флаг содержания максимального количества хопов
					const int32_t flag = static_cast <int32_t> (hops);
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &flag, sizeof(flag))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (hops)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
				// Если необходимо установить максимальное количество хопов для multicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::MULTICAST): {
					// Получаем флаг содержания максимального количества хопов
					const int32_t flag = static_cast <int32_t> (hops);
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &flag, sizeof(flag))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									sock,
									static_cast <uint16_t> (hops)
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
			}
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод активации/деактивации мультикаст группы события
 *
 * @param sock   сетевой сокет
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @return       результат работы функции
 *
 */
bool awh::eth::Socket::membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept {
	// Переменная результата
	bool result = false;
	// Если переданные адреса не инициализированы
	if((group == nullptr) || (source == nullptr)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"It is impossible to work with a multicast group because the address of the group or the source is not initialized",
				__PRETTY_FUNCTION__,
				make_tuple(
					sock,
					static_cast <uint16_t> (mode)
				), log_t::flag_t::CRITICAL
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("It is impossible to work with a multicast group because the address of the group or the source is not initialized", log_t::flag_t::CRITICAL);
		#endif
		// Возвращаем пустой результат
		return result;
	}
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если тип IP-адресов совпадает
		if(group->size == source->size){
			/**
			 * Определяем режим блокировки
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо активировать заголовки в сокете
				case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
					/**
					 * Определяем тип адреса
					 */
					switch(group->size){
						// Если адрес является IPv4
						case 4: {
							// Формируем объект multicast request
							struct ip_mreq mreq{0};
							// Устанавливаем адрес multicast-группы
							mreq.imr_multiaddr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (group)->address;
							// Устанавливаем адрес сетевого интерфейса
							mreq.imr_interface.s_addr = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
							// Добавляем новую multicast-группу к сокету
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (mode)
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
						// Если адрес является IPv6
						case 16: {
							// Формируем объект multicast request
							struct ipv6_mreq mreq{0};
							// Устанавливаем адрес multicast-группы
							::memcpy(&mreq.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(mreq.ipv6mr_multiaddr));
							// Получаем индекс сетевого интерфейса по его IPv6-адресу (из кеша или через getifaddrs)
							mreq.ipv6mr_interface = ::resolveIfaceIndexIPv6(this->_fmk, &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0]);
							// Добавляем новую multicast-группу к сокету
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (mode)
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
					}
				} break;
				// Если необходимо деактивировать заголовки в сокете
				case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
					/**
					 * Определяем тип адреса
					 */
					switch(group->size){
						// Если адрес является IPv4
						case 4: {
							// Формируем объект multicast request
							struct ip_mreq mreq{0};
							// Устанавливаем адрес multicast-группы
							mreq.imr_multiaddr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (group)->address;
							// Устанавливаем адрес сетевого интерфейса
							mreq.imr_interface.s_addr = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
							// Удаляем multicast-группу из сокета
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (mode)
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
						// Если адрес является IPv6
						case 16: {
							// Формируем объект multicast request
							struct ipv6_mreq mreq{0};
							// Устанавливаем адрес multicast-группы
							::memcpy(&mreq.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(mreq.ipv6mr_multiaddr));
							// Получаем индекс сетевого интерфейса по его IPv6-адресу (из кеша или через getifaddrs)
							mreq.ipv6mr_interface = ::resolveIfaceIndexIPv6(this->_fmk, &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0]);
							// Удаляем multicast-группу из сокета
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											sock,
											static_cast <uint16_t> (mode)
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
					}
				} break;
			}
		// Если IP-адреса отличаются
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"It is impossible to work with a multicast group because the IP address types are different",
					__PRETTY_FUNCTION__,
					make_tuple(
						sock,
						static_cast <uint16_t> (mode)
					), log_t::flag_t::CRITICAL
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("It is impossible to work with a multicast group because the IP address types are different", log_t::flag_t::CRITICAL);
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
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					sock,
					static_cast <uint16_t> (mode)
				), log_t::flag_t::CRITICAL,
				error.what()
			);
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
 * @brief Метод выдачи нового сокета
 *
 * @param family семейство протоколов сокета
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @return       созданный сокет
 *
 */
awh::net::socket_t awh::eth::Socket::issue(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Если сокет принадлежит к типу STREAM
					case static_cast <uint8_t> (event::type_t::STREAM):
						// Печатаем дескриптор созданного сокета
						return ::socket(AF_UNIX, SOCK_STREAM, 0);
					// Если сокет принадлежит к типу DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM):
						// Печатаем дескриптор созданного сокета
						return ::socket(AF_UNIX, SOCK_DGRAM, 0);
					// Если сокет принадлежит к типу SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						/**
						 * Для операционной системы macOS, NetBSD, OpenBSD
						 */
						#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
							// Печатаем дескриптор созданного сокета
							return ::socket(AF_UNIX, SOCK_DGRAM, 0);
						/**
						 * Для операционной системы FreeBSD
						 */
						#elif __FreeBSD__
							// Печатаем дескриптор созданного сокета
							return ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
						#endif
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"A socket for a Unix event cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__,
								make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("A socket for a Unix event cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Флаг удачного выполнения объединение событий
				bool ok = true;
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Если сокет принадлежит к типу RAW
					case static_cast <uint8_t> (event::type_t::RAW): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_RAW, 0);
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::RAW):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_RAW, 0);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_RAW, IPPROTO_TCP);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"RAW socket type only supports UDP or ICMP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("RAW socket type only supports UDP or ICMP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Если сокет принадлежит к типу STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_STREAM, 0);
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_STREAM, 0);
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Если сокет принадлежит к типу DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_DGRAM, 0);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_DGRAM, 0);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"DGRAM socket type only supports UDP, DTLS or ICMP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("DGRAM socket type only supports UDP, DTLS or ICMP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Если сокет принадлежит к типу SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										/**
										 * Для операционной системы macOS, NetBSD, OpenBSD
										 */
										#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
											// Печатаем дескриптор созданного сокета
											return ::socket(AF_INET, SOCK_DGRAM, 0);
										/**
										 *Для операционной системы FreeBSD
										 */
										#elif __FreeBSD__
											// Печатаем дескриптор созданного сокета
											return ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
										#endif
									} break;
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Печатаем дескриптор созданного сокета
										return ::socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"A socket for an IP event cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__,
								make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("A socket for an IP event cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для неизвестного семейства
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"A socket cannot be created, because family it belongs to is not defined",
						__PRETTY_FUNCTION__,
						make_tuple(
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (type),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("A socket cannot be created, because family it belongs to is not defined", log_t::flag_t::WARNING);
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
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					static_cast <uint16_t> (family),
					static_cast <uint16_t> (type),
					static_cast <uint16_t> (proto)
				), log_t::flag_t::CRITICAL, error.what()
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return net::invalid_socket_t;
}
/**
 * @brief Метод создания пары сокетов для межпроцессного взаимодействия (IPC)
 *
 * @param family семейство протоколов сокета
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @return       созданный сокет
 *
 */
array <awh::net::socket_t, 2> awh::eth::Socket::ipc(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept {
	// Переменная результата
	array <net::socket_t, 2> result = {
		net::invalid_socket_t,
		net::invalid_socket_t
	};
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства PIPE
			case static_cast <uint8_t> (event::family_t::PIPE): {
				// Выполняем инициализацию файловых дескрипторов
				if(::pipe(&result[0]) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (type),
								static_cast <uint16_t> (proto)
							),
							log_t::flag_t::CRITICAL, ::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Если сокет принадлежит к типу STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_STREAM, 0, &result[0]) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									),
									log_t::flag_t::CRITICAL, ::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					} break;
					// Если сокет принадлежит к типу DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, &result[0]) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									),
									log_t::flag_t::CRITICAL, ::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					} break;
					// Если сокет принадлежит к типу SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						/**
						 * Для операционной системы macOS, NetBSD, OpenBSD
						 */
						#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
							// Выполняем инициализацию файловых дескрипторов
							if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, &result[0]) != 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (type),
											static_cast <uint16_t> (proto)
										),
										log_t::flag_t::CRITICAL, ::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							}
						/**
						 * Для остальных операционных систем
						 */
						#else
							// Выполняем инициализацию файловых дескрипторов
							if(::socketpair(AF_UNIX, SOCK_SEQPACKET, 0, &result[0]) != 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"%s", __PRETTY_FUNCTION__,
										make_tuple(
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (type),
											static_cast <uint16_t> (proto)
										),
										log_t::flag_t::CRITICAL, ::strerror(errno)
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							}
						#endif
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"An event for a Unix event cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__,
								make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("An event for a Unix event cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				/**
				 * Создаём нужное количество сокетов
				 */
				for(net::socket_t & socket : result)
					// Создаём сокет по указанным параметрам
					socket = this->issue(family, type, proto);
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
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					static_cast <uint16_t> (family),
					static_cast <uint16_t> (type),
					static_cast <uint16_t> (proto)
				), log_t::flag_t::CRITICAL, error.what()
			);
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::eth::Socket::Socket(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	/**
	 * Выполняем одноразовую инициализацию мьютексов для кешей IGD и шлюза для всех экземпляров класса Port_Mapping
	 */
	std::call_once(::__awh_init_once__, [this]() noexcept {
		// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем сетевых интерфейсов
		::__awh_iface_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	});
}
/**
 * @brief Деструктор
 *
 */
awh::eth::Socket::~Socket() noexcept {}
