/**
 * @file: socket.hpp
 * @date: 2026-01-28
 * @license: LicenseRef-AWH-1.0
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SOCKET__
#define __AWH_SOCKET__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён Ethernet протоколов
	 *
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс для работы с сокетами
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Socket {
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения кода ошибки
				 *
				 * @param sock сетевой сокет
				 * @return     код ошибки на сокете если присутствует
				 */
				int32_t getError(const net::socket_t sock) const noexcept;
			public:
				/**
				 * @brief Метод получения таймаута сокета
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @return      время таймаута в миллисекундах
				 */
				uint32_t getTimeout(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * @brief Метод установки таймаута сокета
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @param msec  время таймаута в миллисекундах
				 * @return      результат установки таймаута
				 */
				bool setTimeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @return      размер буфера сокета
				 */
				int32_t getBufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * @brief Метод установки размеров буфера
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @param size  размер буфера сокета
				 * @return      установленный размер буфера сокета
				 */
				int32_t setBufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept;
			public:
				/**
				 * @brief Метод установки сетевого интерфейса для multicast пакетов
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ifname имя сетевого интерфейса
				 * @return       результат работы функции
				 */
				bool setMulticastIface(const net::socket_t sock, const event::family_t family, string_view ifname) const noexcept;
			public:
				/**
				 * @brief Метод устанавливает постоянное подключение на сокет
				 *
				 * @param sock  сетевой сокет
				 * @param cnt   максимальное количество попыток
				 * @param idle  время через которое происходит проверка подключения
				 * @param intvl время между попытками
				 * @return      результат работы функции
				 */
				bool setKeepalive(const net::socket_t sock, int32_t cnt, int32_t idle, int32_t intvl) const noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение DSCP
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param dscp   значение DSCP
				 * @return       результат работы функции
				 */
				bool setDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family, const event::dscp_t dscp) const noexcept;
			public:
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
				 */
				event::ecn_t getExplicitCongestionNotification(const net::socket_t sock, const event::family_t family) const noexcept;
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
				 */
				bool setExplicitCongestionNotification(const net::socket_t sock, const event::family_t family, const event::ecn_t ecn) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации генерации информации о трафике
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим активации или деактивации
				 * @return       результат работы функции
				 */
				bool trafficInfoGeneration(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept;
			public:
				/**
				 * @brief Метод переключения опции сокета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим активации или деактивации
				 * @param option опция сокета
				 * @return       результат работы функции
				 */
				bool switchOption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option) const noexcept;
			public:
				/**
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       режим обнаружения максимального размера пакета (MTU)
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим обнаружения максимального размера пакета (MTU)
				 * @return       результат работы функции
				 */
				bool setMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param sock     сетевой сокет
				 * @param family   семейство протоколов (IPv4 или IPv6)
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         максимальное количество хопов
				 */
				uint8_t getHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param sock     сетевой сокет
				 * @param family   семейство протоколов (IPv4 или IPv6)
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @param hops     максимальное количество хопов
				 * @return         результат работы функции
				 */
				bool setHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const uint8_t hops) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @param sock   сетевой сокет
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @return       результат работы функции
				 */
				bool membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept;
			public:
				/**
				 * @brief Метод выдачи нового сокета
				 *
				 * @param family семейство протоколов сокета
				 * @param type   тип сокета
				 * @param proto  протокол сокета
				 * @return       созданный сокет
				 */
				net::socket_t issue(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept;
			public:
				/**
				 * @brief Метод создания пары сокетов для межпроцессного взаимодействия (IPC)
				 *
				 * @param family семейство протоколов сокета
				 * @param type   тип сокета
				 * @param proto  протокол сокета
				 * @return       созданный сокет
				 */
				array <net::socket_t, 2> ipc(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 */
				explicit Socket(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Socket() noexcept;
		} socket_t;
	};
};

#endif // __AWH_SOCKET__
