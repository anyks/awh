/**
 * @file: socks5.hpp
 * @date: 2026-05-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл сервера SOCKS5-прокси — публичный API класса server::Socks5,
 *        выполняющего авторизацию клиентов, установку исходящих соединений по командам CONNECT и BIND и проксирование
 *        UDP-трафика через пул выделенных UDP-серверов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовка
 */
#ifndef __AWH_SERVER_SOCKS5__
#define __AWH_SERVER_SOCKS5__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "server.hpp"
#include "../units/client.hpp"
#include "../proto/socks5/server.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён сервера
	 *
	 */
	namespace server {
		/**
		 * @brief Класс сервера SOCKS5-прокси
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
						 * @param addr объект параметров подключения инициатора запроса
						 * @return     идентификатор инициатора запроса
						 *
						 */
						Origin & from(const net::attr_t * addr) noexcept;
					public:
						/**
						 * @brief Оператор сравнения
						 *
						 * @param other другой объект для сравнения
						 * @return      результат сравнения
						 *
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
						 *
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
				 * @details Пир — это удалённый клиент, который подключился к прокси-серверу и выполняет через него свои запросы.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Peer {
					// Идентификатор события клиента для конечной точки
					event::id_t eid;
					// Идентификатор DNS-резолвера
					unit::dns_t::id_t did;
					// Контекст для хранения параметров сообщений
					proto::socks5_t::ctx_t ctx;
					// Контекст UDP-заголовка для текущего пира
					proto::socks5_t::udp_head_t udp;
					// Буфер накопления входящих SOCKS5-кадров по TCP
					vector <uint8_t> rx;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Peer() noexcept;
				} peer_t;
				/**
				 * @brief Структура для хранения информации о UDP-серверах
				 *
				 * @details UDP-серверы используются для обработки UDP-запросов от клиентов,
				 *          которые подключаются к прокси-серверу через протокол SOCKS5.
				 *          Каждый UDP-сервер имеет диапазон портов, на которых он может принимать запросы от клиентов.
				 *          При получении UDP-запроса от клиента, прокси-сервер выбирает свободный порт из диапазона и перенаправляет запрос на соответствующий UDP-сервер.
				 *          После обработки запроса, UDP-сервер отправляет ответ обратно на прокси-сервер, который затем пересылает его клиенту.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ UDP_Server {
					// Начальный порт диапазона для выделения портов UDP серверов
					uint16_t begin;
					// Конечный порт диапазона для выделения портов UDP серверов
					uint16_t end;
					// Количество выделенных портов для UDP-серверов
					uint16_t count;
					// Список идентификаторов активных событий UDP-серверов
					vector <event::id_t> events;
					// Объект контекста заголовка UDP пакета
					proto::socks5_t::udp_head_t ctx;
					// Адрес для запуска UDP-серверов
					unique_ptr <net::addr_t> address;
					// Множество идентификаторов UDP-серверов для быстрой проверки
					unordered_set <event::id_t> eventSet;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit UDP_Server() noexcept;
				} udp_server_t;
			private:
				// Объект работы с сетью
				eth_t _eth;
			private:
				// Объект UDP-серверов для SOCKS5-прокси
				udp_server_t _udp;
			private:
				// Объект юнита клиента
				unit::client_t _client;
			private:
				// Объект для работы с протоколом SOCKS5
				proto::server_socks5_t _socks5;
			private:
				// Список для сопоставления идентификаторов пиров с удалёнными клиентами
				unordered_map <event::id_t, peer_t> _peers;
				// Список для сопоставления идентификаторов клиентов с пирами, которым они принадлежат
				unordered_map <event::id_t, event::id_t> _clients;
				// Список для сопоставления идентификаторов DNS-запросов с пирами
				unordered_map <unit::dns_t::id_t, event::id_t> _resolves;
			private:
				// Отображение идентификаторов событий клиентов для конечных точек
				unordered_map <event::id_t, origin_t> _mapping;
				// Алиасы для внутренних адресов если мы работаем за NAT
				unordered_map <origin_t, unique_ptr <net::attr_t>, origin_hash_t> _aliases;
				// Активные сессии клиентов, работающих через прокси
				unordered_map <origin_t, pair <event::id_t, event::id_t>, origin_hash_t> _sessions;
			private:
				/**
				 * @brief Метод удаления связи DNS-запроса с пиром
				 *
				 * @param did идентификатор DNS-запроса
				 *
				 */
				void dropResolve(const unit::dns_t::id_t did) noexcept;
				/**
				 * @brief Метод отправки SOCKS5-ответа прокси-клиенту
				 *
				 * @param eid      идентификатор пира
				 * @param ctx      контекст протокола SOCKS5
				 * @param dropPeer закрыть пира после ответа об ошибке
				 * @return         результат отправки ответа
				 *
				 */
				bool sendReply(const event::id_t eid, proto::socks5_t::ctx_t & ctx, const bool dropPeer = false) noexcept;
			private:
				/**
				 * @brief Метод изменения статуса сервера
				 *
				 * @param index  индекс очереди запускаемого события
				 * @param status новый статус сервера
				 *
				 */
				void status(const uint8_t index, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод инициализации запуска или остановки кластера
				 *
				 * @param pid   идентификатор процесса
				 * @param event событие кластера
				 *
				 */
				void eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
				/**
				 * @brief Метод обработки события получения сообщения от процесса кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param data данные полученного сообщения
				 * @param size размер данных полученного сообщения
				 *
				 */
				void messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки событий подключения клиента к удалённому серверу
				 *
				 * @param eid идентификатор клиента
				 * @param ok  результат подключения
				 *
				 */
				void connectClient(const event::id_t eid, const bool ok) noexcept;
				/**
				 * @brief Метод обработки событий изменения состояния клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param status новый статус события
				 *
				 */
				void statusClient(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий получения данных клиентом
				 *
				 * @param eid    идентификатор клиента
				 * @param buffer буфер данных клиента
				 * @param size   размер данных клиента
				 *
				 */
				void readClient(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод получения события ошибок
				 *
				 * @param eid     идентификатор события
				 * @param error   код ошибки
				 * @param message сообщение об ошибке
				 *
				 */
				void errorClient(const event::id_t eid, const event::error_t error, const string & message) noexcept;
				/**
				 * @brief Метод обработки событий истечения таймаута клиента
				 *
				 * @param eid    идентификатор клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  задержка таймаута в миллисекундах
				 * @return       нужно ли завершить клиента после истечения таймаута
				 *
				 */
				bool timeoutClient(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
			private:
				/**
				 * @brief Метод обработки события разрешения подключения
				 *
				 * @param eid идентификатор сервера
				 * @param cid идентификатор клиента
				 *
				 */
				void accept(const event::id_t eid, const event::id_t cid) noexcept;
				/**
				 * @brief Метод обработки событий изменения состояния сервера
				 *
				 * @param eid    идентификатор клиента
				 * @param status новый статус сервера
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 *
				 */
				void state(const event::id_t eid, const event::status_t status, void * ctx) noexcept;
				/**
				 * @brief Метод обработки событий получения данных сервером
				 *
				 * @param eid    идентификатор клиента
				 * @param buffer буфер данных сервера
				 * @param size   размер данных сервера
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 *
				 */
				void read(const event::id_t eid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки неудачного резолвинга доменного имени
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param record тип записи DNS
				 * @param domain доменное имя
				 *
				 */
				void failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept;
				/**
				 * @brief Метод резолвинга доменного имени удалённого хоста в сетевой адрес
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство адресов (IPv4/IPv6)
				 * @param domain доменное имя для резолвинга
				 * @param addr   указатель на структуру для хранения результата резолвинга
				 *
				 */
				void resolve(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
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
				 * @brief Метод приостановки работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 *
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * @brief Метод возобновления работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 *
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события клиента для уничтожения
				 *
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод получения данных от клиента (заглушка для сервера SOCKS5)
				 *
				 * @return результат получения данных
				 *
				 */
				bool recv(const event::id_t) noexcept;
				/**
				 * @brief Метод отправки данных клиенту (заглушка для сервера SOCKS5)
				 *
				 * @return количество байт данных, отправленных клиенту
				 *
				 */
				size_t send(const event::id_t, const void *, const size_t) noexcept;
			public:
				/**
				 * @brief Метод перемещения данных между сервером и другим событием (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения перемещения
				 *
				 */
				bool splice(const event::id_t, const event::id_t) noexcept;
			public:
				/**
				 * @brief Метод получения опций клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции клиента
				 *
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции клиента для установки
				 * @return        результат выполнения установки
				 *
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция клиента для установки
				 * @param mode   режим установки опции клиента
				 * @return       результат выполнения установки
				 *
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса для подключения к сети клиентов
				 *
				 * @return сетевой интерфейс сервера
				 *
				 */
				string getIface() const noexcept;
				/**
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 *
				 */
				string getIface(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод установки сетевого интерфейса для подключения к сети клиентов
				 *
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setIface(string_view name) noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения порта сервера
				 *
				 * @return порт сервера
				 *
				 */
				uint16_t getSourcePort() const noexcept;
				/**
				 * @brief Метод получения внутреннего порта клиента, подключённого к серверу
				 *
				 * @param eid идентификатор события клиента
				 * @return    внутренний порт клиента
				 *
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта сервера
				 *
				 * @param port порт сервера для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setSourcePort(const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения порта удалённого клиента или текущего сервера
				 *
				 * @param eid идентификатор события клиента или сервера
				 * @return    порт удалённого клиента или текущего сервера
				 *
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 *
				 */
				string getTarget(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::address_t address, string_view value) noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
			public:
				/**
				 * @brief Метод получения адреса сервера
				 *
				 * @param address тип адреса сервера
				 * @return        значение адреса сервера
				 *
				 */
				string getAddress(const event::address_t address) const noexcept;
				/**
				 * @brief Метод получения адреса сервера
				 *
				 * @param address тип адреса сервера
				 * @param value   объект для извлечения адреса сервера
				 * @return        результат выполнения извлечения адреса сервера
				 *
				 */
				bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса клиента или текущего сервера
				 *
				 * @param eid     идентификатор события клиента или сервера
				 * @param address тип адреса клиента или сервера
				 * @return        значение адреса клиента или сервера
				 *
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод получения адреса клиента или текущего сервера
				 *
				 * @param eid     идентификатор события клиента или сервера
				 * @param address тип адреса клиента или сервера
				 * @param value   объект для извлечения адреса клиента или сервера
				 * @return        результат выполнения извлечения адреса клиента или сервера
				 *
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 *
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 *
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @return режим использования таймаута на чтение события
				 *
				 */
				event::usage_t getUsageReadTimeout() const noexcept;
				/**
				 * @brief Метод получения режима использования таймаута на чтение события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим использования таймаута на чтение события клиента
				 *
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 *
				 */
				void setUsageReadTimeout(const event::usage_t usage) noexcept;
				/**
				 * @brief Метод установки режима использования таймаута на чтение события клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
				 *
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута сервера
				 *
				 * @param action тип действия сервера
				 * @return       значение таймаута в миллисекундах
				 *
				 */
				uint32_t getTimeout(const event::action_t action) const noexcept;
				/**
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 *
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
			public:
				/**
				 * @brief Метод установки таймаута сервера
				 *
				 * @param action  тип действия сервера
				 * @param timeout значение таймаута в миллисекундах
				 *
				 */
				void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
				/**
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 *
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сервера
				 *
				 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
				 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 */
				bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
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
				 *
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 */
				bool membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 */
				bool membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 */
				size_t clusterSend(const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 */
				size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 */
				size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки диапазона портов для выделения портов UDP серверов
				 *
				 * @param count количество портов для выделения
				 * @param begin начальный порт диапазона для выделения
				 * @param end   конечный порт диапазона для выделения
				 * @param addr  адрес для запуска UDP-серверов
				 *
				 */
				void udp(const uint16_t count, const uint16_t begin, const uint16_t end, string_view addr) noexcept;
				/**
				 * @brief Метод установки диапазона портов для выделения портов UDP серверов
				 *
				 * @param count количество портов для выделения
				 * @param begin начальный порт диапазона для выделения
				 * @param end   конечный порт диапазона для выделения
				 * @param addr  адрес для запуска UDP-серверов
				 *
				 */
				void udp(const uint16_t count, const uint16_t begin, const uint16_t end, const net::addr_t * addr) noexcept;
			public:
				/**
				 * @brief Метод установки алиаса для внутреннего адреса при работе за NAT
				 *
				 * @param addr  объект параметров подключения внутреннего адреса
				 * @param alias объект параметров подключения алиаса для внутреннего адреса
				 *
				 */
				void setAlias(const net::attr_t * addr, const net::attr_t * alias) noexcept;
				/**
				 * @brief Метод установки алиаса для внутреннего адреса при работе за NAT
				 *
				 * @param addr    внутренний адрес работающий за NAT
				 * @param intPort порт внутреннего адреса работающий за NAT
				 * @param alias   внешний адрес для алиаса внутреннего адреса
				 * @param extPort внешний порт для алиаса внутреннего адреса
				 *
				 */
				void setAlias(string_view addr, const uint16_t intPort, string_view alias, const uint16_t extPort = 0) noexcept;
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
				 *
				 */
				Socks5 & operator = (const Socks5 &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Socks5(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param dns объект DNS-резолвера
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Socks5(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
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
