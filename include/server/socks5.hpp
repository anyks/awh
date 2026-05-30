/**
 * @file: socks5.hpp
 * @date: 2026-05-30
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
#ifndef __AWH_SERVER_SOCKS5__
#define __AWH_SERVER_SOCKS5__

/**
 * Стандартные модули
 */
#include <string>
#include <unordered_set>
#include <unordered_map>

/**
 * Наши модули
 */
#include "server.hpp"
#include "../proto/socks5/server.hpp"

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
	 * @brief Пространство имён сервера
	 *
	 */
	namespace server {
		/**
		 * @brief Класс сервера socks5 прокси
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Socks5 : public server_t {
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
				/**
				 * @brief Структура для хранения информации о пирах
				 *
				 */
				typedef struct Peer {
					// Идентификатор события клиента для конечной точки
					event::id_t eid;
					// Контекст для хранения параметров сообщений
					proto::socks5_t::ctx_t ctx;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Peer() noexcept : eid(0) {};
				} peer_t;
			private:
				// Объект для работы с протоколом SOCKS5
				proto::server_socks5_t _socks5;
			private:
				// Мютекс для блокировки потоков
				lock_state_t <std::shared_mutex> _mtx;
			private:
				// Адрес хостов целевых UDP серверов
				unordered_set <string> _hosts;
				// Список сетевых интерфейсов для подключения к сети клиентов
				unordered_set <string> _interfaces;
				// Список идентификаторов активных UDP-серверов
				unordered_set <event::id_t> _servers;
			private:
				// Список для сопоставления идентификаторов пиров с удалёнными клиентами
				unordered_map <event::id_t, peer_t> _peers;
			private:
				// Отображение идентификаторов событий клиентов для конечных точек
				unordered_map <event::id_t, const origin_t *> _mapping;
				// Активные сессии клиентов, работающих через прокси
				unordered_map <origin_t, event::id_t, origin_hash_t> _sessions;
			private:
				/**
				 * @brief Метод обработки события разрешения подключения
				 *
				 * @param eid идентификатор сервера
				 * @param cid идентификатор клиента
				 */
				void accept(const event::id_t eid, const event::id_t cid) noexcept;
				/**
				 * @brief Метод обработки событий получения данных сервером
				 *
				 * @param eid    идентификатор клиента
				 * @param buffer буфер данных сервера
				 * @param size   размер данных сервера
				 */
				void read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод остановки сервера
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска сервера
				 *
				 */
				void start() noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения установки
				 */
				bool membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения установки
				 */
				bool membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept;
			public:
				/**
				 * @brief Метод приостановки работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * @brief Метод возобновления работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события клиента для уничтожения
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения данных от клиента (заглушка для сервера SOCKS5)
				 *
				 * @return результат получения данных
				 */
				bool recv(const event::id_t) noexcept;
				/**
				 * @brief Метод отправки данных клиенту (заглушка для сервера SOCKS5)
				 *
				 * @return количество байт данных, отправленных клиенту
				 */
				size_t send(const event::id_t, const void *, const size_t) noexcept;
			public:
				/**
				 * @brief Метод перемещения данных между сервером и другим событием (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения перемещения
				 */
				bool splice(const event::id_t, const event::id_t) noexcept;
			public:
				/**
				 * @brief Метод получения опций клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции клиента
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции клиента для установки
				 * @return        результат выполнения установки
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция клиента для установки
				 * @param mode   режим установки опции клиента
				 * @return       результат выполнения установки
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса для подключения к сети клиентов
				 *
				 * @return сетевой интерфейс сервера
				 */
				string getIface() const noexcept;
				/**
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 */
				string getIface(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод установки сетевого интерфейса для подключения к сети клиентов
				 *
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 */
				bool setIface(string_view name) noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения порта удаленного клиента или текущего сервера
				 *
				 * @param eid идентификатор события клиента или сервера
				 * @return    порт удаленного клиента или текущего сервера
				 */
				uint16_t getPort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param port порт сервера для установки
				 * @return     результат выполнения установки
				 */
				bool setPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения внутреннего порта события
				 *
				 * @param eid идентификатор события клиента
				 * @return    внутренний порт события
				 */
				uint16_t getInternalPort(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста текущей машины
				 *
				 * @param eid идентификатор события сервера
				 * @return    адрес хоста текущей машины
				 */
				string getHost(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки адреса хоста текущей машины
				 *
				 * @param eid  идентификатор события сервера
				 * @param host адрес хоста текущей машины
				 * @return     результат выполнения установки
				 */
				bool setHost(const event::id_t eid, string_view host) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 */
				string getTarget(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * @brief Метод установки адреса для подключения к сети клиентов
				 *
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::address_t address, string_view value) noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * @brief Метод установки адреса для подключения к сети клиентов
				 *
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
			public:
				/**
				 * @brief Метод получения адреса для подключения к сети клиентов
				 *
				 * @param address тип адреса клиента или сервера
				 * @return        значение адреса клиента или сервера
				 */
				string getAddress(const event::address_t address) const noexcept;
				/**
				 * @brief Метод получения адреса клиента или текущего сервера
				 *
				 * @param eid     идентификатор события клиента или сервера
				 * @param address тип адреса клиента или сервера
				 * @return        значение адреса клиента или сервера
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса для подключения к сети клиентов
				 *
				 * @param address тип адреса клиента или сервера
				 * @param value   объект для извлечения адреса клиента или сервера
				 * @return        результат выполнения извлечения адреса клиента или сервера
				 */
				bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
				/**
				 * @brief Метод получения адреса клиента или текущего сервера
				 *
				 * @param eid     идентификатор события клиента или сервера
				 * @param address тип адреса клиента или сервера
				 * @param value   объект для извлечения адреса клиента или сервера
				 * @return        результат выполнения извлечения адреса клиента или сервера
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута на чтение события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим использования таймаута на чтение события клиента
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута на чтение события клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод установки параметров keep-alive для клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод установки идентификатора события сервера
				 *
				 * @param eid идентификатор события сервера для установки
				 */
				void setEventId(const event::id_t eid) noexcept;
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
				 * @param server объект юнита сервера
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit Socks5(unit::server_t * server, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param server объект юнита сервера
				 * @param dns    объект DNS-резолвера
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit Socks5(unit::server_t * server, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Socks5() noexcept;
		} socks5_t;
	};
};

#endif // __AWH_SERVER_SOCKS5__
