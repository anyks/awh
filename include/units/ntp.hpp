/**
 * @file: ntp.hpp
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_NTP__
#define __AWH_UNIT_NTP__

/**
 * Наши модули
 */
#include "unit.hpp"
#include "../sys/locker.hpp"

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
		 * @brief Класс NTP-клиента
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ NTP : public unit_t {
			public:
				/**
				 * @brief Версии протокола NTP
				 *
				 */
				enum class version_t : uint8_t {
					V1 = 0x01, // Версия 1
					V2 = 0x02, // Версия 2
					V3 = 0x03, // Версия 3
					V4 = 0x04  // Версия 4 (наиболее распространённая)
				};
			private:
				/**
				 * @brief Класс для управления списком NTP-серверов
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Servers {
					private:
						// Индекс текущего NTP-сервера для выполнения запроса IPv4 (для раунд-робин распределения нагрузки)
						size_t _indexIPv4;
						// Индекс текущего NTP-сервера для выполнения запроса IPv6 (для раунд-робин распределения нагрузки)
						size_t _indexIPv6;
					private:
						// Флаг инициализации списка NTP-серверов IPv4
						bool _initializedIPv4;
						// Флаг инициализации списка NTP-серверов IPv6
						bool _initializedIPv6;
					private:
						// Список NTP-серверов для выполнения запросов (IPv4)
						vector <unique_ptr <net::addr_t>> _ipv4;
						// Список NTP-серверов для выполнения запросов (IPv6)
						vector <unique_ptr <net::addr_t>> _ipv6;
					public:
						/**
						 * @brief Метод инициализации списка NTP-серверов из переменных окружения или стандартных значений
						 *
						 */
						void init() noexcept;
					public:
						/**
						 * @brief Метод сброса списка NTP-серверов
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 */
						void reset(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод получения текущего NTP-сервера
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 * @return       объект NTP-сервера для выполнения запроса
						 */
						const net::addr_t * get(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод добавления NTP-сервера в список
						 *
						 * @param server объект NTP-сервера для добавления в список
						 */
						void push(const net::addr_t * server) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Servers() noexcept;
				} servers_t;
				/**
				 * @brief Структура для управления состоянием NTP-клиента
				 *
				 */
				typedef struct Client {
					// Префикс для переменных окружения
					string prefix;
					// Порт NTP-сервера
					uint16_t port;
					// Идентификатор события для NTP-клиента
					event::id_t eid;
					// Адрес NTP-сервера для выполнения запросов
					servers_t servers;
					// Адрес сети для выполнения запроса
					unique_ptr <net::addr_t> source;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Client() noexcept :
					 prefix{AWH_SHORT_NAME},
                     port(123), eid(0), source(nullptr) {}
				} client_t;
				/**
				 * @brief Структура активного пакета при выполнении запросов NTP-клиента
				 *
				 */
				typedef struct Packet {
					// Время ожидания ответа от NTP-сервера (в миллисекундах)
					uint32_t delay;
					// Количество попыток получения ответа от NTP-сервера
					uint8_t attempt;
					// Идентификатор события для таймера NTP-клиента
					event::id_t eid;
					// Версия протокола NTP для выполнения запроса
					version_t version;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Packet() noexcept :
					 delay(5000), attempt(0), eid(0),
					 version(version_t::V4) {}
				} packet_t;
				/**
				 * @brief Структура для управления передачей данных при выполнении запросов NTP-клиента
				 *
				 */
				typedef struct Transfer {
					// Количество попыток получения ответа от NTP-сервера
					uint8_t attempts;
					// Мьютекс для блокировки потока
					lock_state_t <std::shared_mutex> mtx;
					// Активные пакеты при выполнении запросов NTP-клиента
					unordered_map <event::id_t, packet_t> waiting;
					/**
					 * @brief Конструктор
					 *
					 */
					 explicit Transfer() noexcept : attempts(3) {}
				} transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
			private:
				// Состояние NTP-клиента
				client_t _client;
				// Объект управления передачей данных при выполнении запросов NTP-клиента
				transfer_t _transfer;
			private:
				/**
				 * @brief Метод создания события NTP-клиента
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 */
				void create(const event::family_t family) noexcept;
			private:
				/**
				 * @brief Метод обработки ошибок событий NTP-клиента
				 *
				 * @param eid         идентификатор события NTP-клиента
				 * @param error       код ошибки события NTP-клиента
				 * @param description описание ошибки события NTP-клиента
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * @brief Метод обработки ответов от NTP-сервера на запросы NTP-клиента
				 *
				 * @param eid  идентификатор события чтения из NTP-клиента
				 * @param data данные события чтения из NTP-клиента
				 * @param size размер данных события чтения из NTP-клиента
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий таймаута при ожидании ответа от NTP-сервера
				 *
				 * @param eid    идентификатор таймера NTP-клиента
				 * @param status статус события таймера NTP-клиента
				 * @param packet объект активного пакета при выполнении запроса NTP-клиента
				 */
				void timeout(const event::id_t eid, const event::status_t status, packet_t * packet) noexcept;
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
				 * @brief Метод установки количества попыток получения ответа от NTP-сервера
				 *
				 * @param attempts количество попыток получения ответа от NTP-сервера
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * @brief Метод установки префикса переменной окружения
				 *
				 * @param prefix префикс переменной окружения для установки
				 */
				void setPrefixEnvironment(string_view prefix) noexcept;
			public:
				/**
				 * @brief Метод сброса NTP-клиента
				 *
				 * @return результат выполнения операции
				 */
				bool reset() noexcept;
			public:
				/**
				 * @brief Метод фиксации параметров NTP-клиента
				 *
				 * @return результат выполнения операции
				 */
				bool commit() noexcept;
			public:
				/**
				 * @brief Метод получения типа события
				 *
				 * @return тип события
				 */
				event::type_t type() const noexcept;
				/**
				 * @brief Метод получения типа узла события
				 *
				 * @return тип узла события
				 */
				event::node_t node() const noexcept;
				/**
				 * @brief Метод получения семейства события
				 *
				 * @return семейство адресов
				 */
				event::family_t family() const noexcept;
				/**
				 * @brief Метод получения статуса события
				 *
				 * @return статус события
				 */
				event::status_t status() const noexcept;
			public:
				/**
				 * @brief Метод получения порта NTP-сервера
				 *
				 * @return порт NTP-сервера
				 */
				uint16_t getPort() const noexcept;
				/**
				 * @brief Метод установки порта NTP-сервера
				 *
				 * @param port порт NTP-сервера для установки
				 */
				void setPort(const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для установки
				 */
				void setServer(string_view server) noexcept;
				/**
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для установки
				 */
				void setServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес NTP-сервера для установки
				 */
				void setServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для добавления
				 */
				void addServer(string_view server) noexcept;
				/**
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для добавления
				 */
				void addServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес NTP-сервера для добавления
				 */
				void addServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param server адреса NTP-серверов для установки
				 */
				void setServers(const vector <string> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param server адреса NTP-серверов для установки
				 */
				void setServers(const vector <const net::addr_t *> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param family  семейство IP-адресов IPv4/IPv6
				 * @param servers адреса NTP-серверов для установки
				 */
				void setServers(const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 */
				void setSource(string_view source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 */
				void setSource(const net::addr_t * source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 */
				void setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод синхронизации времени с NTP-сервером
				 *
				 * @param version версия протокола NTP для выполнения запроса
				 * @param timeout время ожидания ответа от NTP-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool sync(const version_t version = version_t::V4, const uint32_t timeout = 0) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				NTP(const NTP &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 */
				NTP & operator = (const NTP &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit NTP(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~NTP() noexcept;
		} ntp_t;
	};
};

#endif // __AWH_UNIT_NTP__
