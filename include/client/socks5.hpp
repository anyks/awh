/**
 * @file: socks5.hpp
 * @date: 2026-05-26
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
#ifndef __AWH_CLIENT_SOCKS5__
#define __AWH_CLIENT_SOCKS5__

/**
 * Стандартные модули
 */
#include <string>
#include <unordered_map>

/**
 * Наши модули
 */
#include "client.hpp"
#include "../proto/socks5/client.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс клиента socks5 прокси
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Socks5 : public client_t {
		private:
			/**
			 * @brief Класс идентификатора сессии клиента, работающего через прокси
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Origin {
				public:
					// Тип адреса инициатора запроса
					net::type_t type;
					// Протокол инициатора запроса
					event::protocol_t protocol;
				public:
					/**
					 * @brief Универсальная структура для хранения различных типов адресов
					 *
					 */
					union {
						/**
						 * @brief Структура FQDN адреса инициатора запроса
						 *
						 */
						struct {
							// Порт инициатора запроса
							uint16_t port = 0;
							// Данные доменного имени инициатора запроса
							char data[256] = {0};
						} fqdn;
						/**
						 * @brief Структура IPv4 адреса инициатора запроса
						 *
						 */
						struct {
							// Порт инициатора запроса
							uint16_t port = 0;
							// Адрес инициатора запроса
							uint32_t address = 0;
						} ip4;
						/**
						 * @brief Структура IPv6 адреса инициатора запроса
						 *
						 */
						struct {
							// Порт инициатора запроса
							uint16_t port = 0;
							// Адрес инициатора запроса
							array <uint8_t, 16> address = {0};
						} ip6;
					};
				public:
					/**
					 * @brief Фабричный метод создания идентификатора инициатора запроса
					 *
					 * @param addr     объект параметров подключения инициатора запроса
					 * @param protocol протокол инициатора запроса
					 * @return         идентификатор инициатора запроса
					 */
					Origin & from(const net::attr_t * addr, const event::protocol_t protocol) noexcept;
				public:
					/**
					 * @brief Оператор сравнения
					 *
					 * @param other другой объект для сравнения
					 * @return      результат сравнения
					 */
					bool operator == (const Origin & other) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Origin() noexcept;
			} origin_t;
			/**
			 * @brief Специализация хеш-функции для структуры идентификатора инициатора запроса
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Origin_Hash {
				public:
					/**
					 * @brief Оператор вычисления хеш-кода
					 *
					 * @param id объект для вычисления хеш-кода
					 * @return   хеш-код объекта
					 */
					size_t operator()(const origin_t & id) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Origin_Hash() noexcept = default;
			} origin_hash_t;
		private:
			// Объект для работы с протоколом SOCKS5
			proto::client_socks5_t _socks5;
		private:
			// Контекст для хранения параметров сообщений
			proto::client_socks5_t::ctx_t _ctx;
		private:
			// Мютекс для блокировки потоков при работе с TLS
			lock_state_t <std::shared_mutex> _mtx;
		private:
			// Отображение идентификаторов событий клиентов для конечных точек
			unordered_map <event::id_t, pair <const origin_t *, tls::coder_t::id_t>> _mapping;
			// Активные сессии клиентов, работающих через прокси
			unordered_map <origin_t, pair <event::id_t, tls::coder_t::id_t>, origin_hash_t> _sessions;
		private:
			/**
			 * @brief Метод изменения статуса клиента
			 *
			 * @param status новый статус клиента
			 * @param state  новое временное состояние клиента
			 */
			void status(const event::status_t status, const state_t state) noexcept;
		private:
			/**
			 * @brief Метод обработки событий подключения клиента к удалённому серверу
			 *
			 * @param eid идентификатор клиента
			 * @param ok  результат подключения
			 */
			void connect(const event::id_t eid, const bool ok) noexcept;
			/**
			 * @brief Метод обработки событий записи данных клиентом
			 *
			 * @param eid  идентификатор клиента
			 * @param size размер данных для записи
			 */
			void write(const event::id_t eid, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий изменения состояния клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param status новый статус клиента
			 */
			void state(const event::id_t eid, const event::status_t status) noexcept;
			/**
			 * @brief Метод обработки событий получения данных клиентом
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер данных клиента
			 * @param size   размер данных клиента
			 */
			void read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
		private:
			/**
			 * @brief Метод получения состояния TLS
			 *
			 * @param id    идентификатор TLS
			 * @param eid   идентификатор клиента
			 * @param state состояние TLS
			 */
			void stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept;
			/**
			 * @brief Метод получения событий шифрования/дешифрования данных TLS
			 *
			 * @param id     идентификатор TLS
			 * @param eid    идентификатор клиента
			 * @param event  тип события TLS
			 * @param size   размер данных для события шифрования/дешифрования TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 */
			void processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод очистки активных сессий клиентов, работающих через прокси
			 *
			 */
			void clearSessions() noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод отправки данных серверу
			 *
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных серверу
			 */
			size_t send(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения данных от сервера
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат получения данных
			 */
			bool recv(const event::id_t eid) noexcept;
			/**
			 * @brief Метод отправки данных клиенту
			 *
			 * @param eid    идентификатор события клиента
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных клиенту
			 */
			size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки параметров авторизации
			 *
			 * @param username имя пользователя для авторизации на сервере
			 * @param password пароль пользователя для авторизации на сервере
			 */
			void setUser(const string & username, const string & password) noexcept;
		public:
			/**
			 * @brief Метод проверки наличия идентификатора события клиента для конечной точки
			 *
			 * @param eid идентификатор события для проверки
			 * @return    результат проверки наличия идентификатора события клиента для конечной точки
			 */
			bool isEventIdEndpoint(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод добавления идентификатора события клиента для конечной точки
			 *
			 * @param eid идентификатор события для добавления
			 * @return    результат выполнения добавления идентификатора события клиента для конечной точки
			 */
			bool addEventIdEndpoint(const event::id_t eid) noexcept;
			/**
			 * @brief Метод добавления идентификатора события клиента для конечной точки
			 *
			 * @param eid идентификатор события для добавления
			 * @param tid идентификатор TLS для добавления
			 * @return    результат выполнения добавления идентификатора события клиента для конечной точки
			 */
			bool addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid) noexcept;
		public:
			/**
			 * @brief Метод добавления идентификатора события клиента для конечной точки
			 *
			 * @param eid  идентификатор события для добавления
			 * @param addr адрес хоста для добавления
			 * @param port порт хоста для добавления
			 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
			 */
			bool addEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) noexcept;
			/**
			 * @brief Метод добавления идентификатора события клиента для конечной точки
			 *
			 * @param eid  идентификатор события для добавления
			 * @param tid  идентификатор TLS для добавления
			 * @param addr адрес хоста для добавления
			 * @param port порт хоста для добавления
			 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
			 */
			bool addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid, string_view addr, const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод добавления идентификатора события клиента для конечной точки
			 *
			 * @param eid  идентификатор события для добавления
			 * @param addr адрес хоста для добавления
			 * @param port порт хоста для добавления
			 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
			 */
			bool addEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) noexcept;
			/**
			 * @brief Метод добавления идентификатора события клиента для конечной точки
			 *
			 * @param eid  идентификатор события для добавления
			 * @param tid  идентификатор TLS для добавления
			 * @param addr адрес хоста для добавления
			 * @param port порт хоста для добавления
			 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
			 */
			bool addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid, const net::addr_t * addr, const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод удаления идентификатора события клиента для конечной точки
			 *
			 * @param eid идентификатор события для удаления
			 * @return    результат выполнения удаления идентификатора события клиента для конечной точки
			 */
			bool delEventIdEndpoint(const event::id_t eid) noexcept;
			/**
			 * @brief Метод удаления идентификатора события клиента для конечной точки
			 *
			 * @param eid  идентификатор события для удаления
			 * @param addr адрес хоста для удаления
			 * @param port порт хоста для удаления
			 * @return     результат выполнения удаления идентификатора события клиента для конечной точки
			 */
			bool delEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) noexcept;
			/**
			 * @brief Метод удаления идентификатора события клиента для конечной точки
			 *
			 * @param eid  идентификатор события для удаления
			 * @param addr адрес хоста для удаления
			 * @param port порт хоста для удаления
			 * @return     результат выполнения удаления идентификатора события клиента для конечной точки
			 */
			bool delEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод установки идентификатора TLS
			 *
			 * @param tid идентификатор TLS для установки
			 */
			void setSecurityId(const tls::coder_t::id_t tid) noexcept;
		private:
			/**
			 * @brief Конструктор копирования (запрещаем)
			 *
			 */
			Socks5(const Socks5 &) = delete;
			/**
			 * @brief Оператор копирования (запрещаем)
			 *
			 * @return текущее значение объекта
			 */
			Socks5 & operator = (const Socks5 &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Socks5(unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param coder  объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Socks5(unit::client_t * client, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param dns    объект DNS-резолвера
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Socks5(unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param dns    объект DNS-резолвера
			 * @param coder  объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Socks5(unit::client_t * client, unit::dns_t * dns, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Socks5() noexcept;
	} socks5_t;
};

#endif // __AWH_CLIENT_SOCKS5__
