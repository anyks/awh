/**
 * @file: portmap.hpp
 * @date: 2026-01-28
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PORTMAP__
#define __AWH_PORTMAP__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../addr.hpp"
#include "gateway.hpp"
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
		 * @brief Класс для работы с пробросом портов
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Port_Mapping {
			public:
				/**
				 * @brief Типы протоколов сокетов
				 *
				 */
				enum class proto_t : uint8_t {
					NONE = 0x00, // Сокет не определён
					UDP  = 0x01, // Сокет UDP
					TCP  = 0x02, // Сокет TCP
				};
				/**
				 * @brief Типы проброса порта на маршрутизаторе
				 *
				 */
				enum class type_t : uint8_t {
					NONE    = 0x00, // Тип не определён
					PCP     = 0x01, // Тип проброса PCP
					UPNP    = 0x02, // Тип проброса UPnP
					NAT_PMP = 0x03  // Тип проброса NAT-PMP
				};
				/**
				 * @brief Структура проброса порта на маршрутизаторе
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Forwarding {
					// Тип проброса порта
					type_t type;
					// Протокол проброса порта
					proto_t proto;
					// Время жизни проброса порта в секундах
					uint32_t lifeTime;
					// Внутренний порт
					uint16_t internalPort;
					// Внешний порт
					uint16_t externalPort;
					// Описание проброса порта
					char description[128];
					// Внутренний IP-адрес
					unique_ptr <net::addr_t> internalAddress;
					// Внешний IP-адрес
					unique_ptr <net::addr_t> externalAddress;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Forwarding() noexcept;
				} fwd_t;
			private:
				// Объект работы с маршрутами
				gateway_t _gateway;
			private:
				// Объект работы с сетевыми аресами
				mutable net_addr_t _addr;
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
				 * @brief Метод получения списка проброшенных портов на маршрутизаторе
				 *
				 * @return список параметров проброшенных портов на маршрутизаторе
				 */
				vector <fwd_t> mappings() const noexcept;
			public:
				/**
				 * @brief Метод установки/удаления проброса портов на маршрутизаторе
				 *
				 * @note При успешном пробросе (ENABLED) назначенный маршрутизатором внешний порт записывается в fwd.externalPort
				 *
				 * @param fwd  объект параметров проброса порта (при успехе обновляется назначенным внешним портом)
				 * @param mode режим включения/выключения проброса порта
				 * @return     результат выполнения установки
				 */
				bool mapping(fwd_t & fwd, const event::mode_t mode) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 */
				explicit Port_Mapping(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Port_Mapping() noexcept;
		} portmap_t;
	};
};

#endif // __AWH_PORTMAP__
