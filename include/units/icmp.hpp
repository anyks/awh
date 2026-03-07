/**
 * @file: icmp.hpp
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_ICMP__
#define __AWH_UNIT_ICMP__

/**
 * Наши модули
 */
#include "unit.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён узла источника
	 *
	 */
	namespace unit {
		/**
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;
		/**
		 * @brief Класс ICMP-клиента
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ ICMP : public unit_t {
			public:
				/**
				 * @brief Идентификатор ICMP-клиента
				 *
				 */
				using id_t = uint16_t;
			public:
				/**
				 * @brief Режим работы ICMP-клиента
				 *
				 */
				enum class mode_t : uint8_t {
					SYNC  = 0x00, // Синхронный режим работы
					ASYNC = 0x01  // Асинхронный режим работы
				};
			private:
				/**
				 * @brief Класс для управления состоянием ICMP-клиента
				 *
				 */
				typedef struct Client {
					// Идентификатор события для ICMP-клиента
					event::id_t eid;
					// Адрес удалённого сервера для выполнения запросов
					unique_ptr <net::addr_t> target;
					// Адрес сети для выполнения запроса
					unique_ptr <net::addr_t> source;
					// Мьютекс для блокировки потока
					lock_state_t <std::mutex> mtx;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Client() noexcept :
                     eid(0), source(nullptr) {}
				} client_t;
				/**
				 * @brief Класс для управления тайм-аутами при ожидании ответа от удалённого сервера
				 *
				 */
				typedef struct Timeout {
					// Количество повторений запросов
					uint16_t count;
					// Время ожидания ответа от удалённого сервера (в миллисекундах)
					uint32_t delay;
					// Идентификатор события для таймера ICMP-клиента
					event::id_t eid;
					// Штамп времени начала запроса
					uint64_t timestamp;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Timeout() noexcept :
					 count(0), delay(5000),
					 eid(0), timestamp(0) {}
				} timeout_t;
				/**
				 * @brief Класс для управления тайм-аутами при ожидании ответа от удалённого сервера
				 *
				 */
				typedef struct Timeouts {
					// Мьютекс для блокировки потока
					lock_state_t <std::shared_mutex> mtx;
					// Активные тайм-ауты при ожидании ответа от удалённого сервера
					unordered_map <id_t, timeout_t> waiting;
					/**
					 * @brief Конструктор
					 *
					 */
					 explicit Timeouts() noexcept {}
				} timeouts_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
			private:
				// Состояние ICMP-клиента
				client_t _client;
			private:
				// Тайм-ауты при ожидании ответа от удалённого сервера
				timeouts_t _timeouts;
			private:
				/**
				 * @brief Метод создания события ICMP-клиента
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 */
				void create(const event::family_t family) noexcept;
			private:
				/**
				 * @brief Метод обработки ошибок событий ICMP-клиента
				 *
				 * @param eid         идентификатор события ICMP-клиента
				 * @param error       код ошибки события ICMP-клиента
				 * @param description описание ошибки события ICMP-клиента
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * @brief Метод обработки событий таймаута при ожидании ответа от ICMP-клиента
				 *
				 * @param id     идентификатор ICMP-клиента
				 * @param        идентификатор таймера ICMP-клиента
				 * @param status статус события таймера ICMP-клиента
				 */
				void timeout(const id_t id, const event::id_t, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки ответов от удалённого сервера на запросы ICMP-клиента
				 *
				 * @param eid  идентификатор события чтения из ICMP-клиента
				 * @param mode режим обработки события чтения из ICMP-клиента
				 * @param data данные события чтения из ICMP-клиента
				 * @param size размер данных события чтения из ICMP-клиента
				 */
				void response(const event::id_t eid, const mode_t mode, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод сброса ICMP-клиента
				 *
				 * @return результат выполнения операции
				 */
				bool reset() noexcept;
			public:
				/**
				 * @brief Метод фиксации параметров ICMP-клиента
				 *
				 * @return результат выполнения операции
				 */
				bool commit() noexcept;
			public:
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(string_view target) noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const net::addr_t * target) noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const event::family_t family, string_view target) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 */
				bool setSource(string_view source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 */
				bool setSource(const net::addr_t * source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 */
				bool setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора ICMP-клиента для выполнения запроса к удалённому серверу
				 *
				 * @return идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
				 */
				id_t issue() const noexcept;
			public:
				/**
				 * @brief Метод выполнения пингов удалённого сервера
				 *
				 * @param id      идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
				 * @param count   количество выполняемых запросов
				 * @param mode    режим выполнения запросов
				 * @param timeout время ожидания ответа от удалённого сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool ping(const id_t id, const uint16_t count, const mode_t mode, const uint32_t timeout = 0) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit ICMP(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~ICMP() noexcept;
		} icmp_t;
	};
};

#endif // __AWH_UNIT_ICMP__
