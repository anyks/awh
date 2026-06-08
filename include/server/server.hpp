/**
 * @file: server.hpp
 * @date: 2026-05-17
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
#ifndef __AWH_SERVER__
#define __AWH_SERVER__

/**
 * Наши модули
 */
#include "../units/dns.hpp"
#include "../units/server.hpp"
#include "../net/tls/coder.hpp"

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
	 * @brief Класс сервера
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Server {
		protected:
			/**
			 * @brief Структура для хранения параметров DNS-резолвера
			 *
			 */
			typedef struct Domain_Name_System {
				// Идентификатор DNS-резолвера
				unit::dns_t::id_t id;
				// Время жизни DNS запроса (в миллисекундах)
				atomic_uint32_t alive;
				// Объект DNS-резолвера
				unit::dns_t * client;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Domain_Name_System() noexcept :
				 id(0), alive(15000), client(nullptr) {}
			} dns_t;
			/**
			 * @brief Структура идентификаторов клиента
			 *
			 */
			typedef struct Identifier {
				// Идентификатор клиента
				event::id_t eid;
				// Идентификатор безопасности
				tls::coder_t::id_t sid;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Identifier() noexcept : eid(0), sid(0) {}
			} __attribute__((packed)) id_t;
			/**
			 * @brief Структура для хранения параметров транспортного уровня безопасности
			 *
			 */
			typedef struct TLS {
				// Объект транспортного уровня безопасности
				tls::coder_t * coder;
				// Список для сопоставления идентификаторов клиентов с идентификаторами TLS
				unordered_map <event::id_t, tls::coder_t::id_t> safety;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit TLS() noexcept : coder(nullptr) {}
			} tls_t;
			/**
			 * @brief Структура юнита сервера
			 *
			 */
			typedef struct Unit {
				// Объект работы с сетевыми адресами
				net_addr_t addr;
				// Объект юнита сервера
				unit::server_t server;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Unit(const fmk_t * fmk, const log_t * log) noexcept :
				 addr(fmk, log), server(fmk, log) {}
			} unit_t;
		protected:
			// Идентификатор сервера
			id_t _id;
		protected:
			// Объект DNS-резолвера
			dns_t _dns;
		protected:
			// Объект параметров TLS
			tls_t _tls;
		protected:
			// Адрес хоста целевой машины
			string _host;
		protected:
			// Функция обратного вызова для обработки сервера
			callback_t _callback;
		protected:
			// Объект юнита сервера
			unique_ptr <unit_t> _unit;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * @brief Метод изменения статуса сервера
			 *
			 * @param index  индекс очереди запускаемого события
			 * @param status новый статус сервера
			 */
			virtual void status(const uint8_t index, const event::status_t status) noexcept;
		protected:
			/**
			 * @brief Метод обработки событий записи данных клиентом
			 *
			 * @param eid  идентификатор клиента
			 * @param size размер данных для записи
			 */
			virtual void write(const event::id_t eid, const size_t size) noexcept;
			/**
			 * @brief Метод обработки события разрешения подключения
			 *
			 * @param eid идентификатор сервера
			 * @param cid идентификатор клиента
			 */
			virtual void accept(const event::id_t eid, const event::id_t cid) noexcept;
			/**
			 * @brief Метод обработки событий изменения состояния сервера
			 *
			 * @param eid    идентификатор клиента
			 * @param status новый статус сервера
			 */
			virtual void state(const event::id_t eid, const event::status_t status) noexcept;
			/**
			 * @brief Метод обработки действий сервера
			 *
			 * @param eid    идентификатор клиента
			 * @param action действие сервера
			 */
			virtual void action(const event::id_t eid, const event::action_t action) noexcept;
			/**
			 * @brief Метод обработки событий получения данных сервером
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер данных сервера
			 * @param size   размер данных сервера
			 */
			virtual void read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод получения события ошибок
			 *
			 * @param eid     идентификатор события
			 * @param error   код ошибки
			 * @param message сообщение об ошибке
			 */
			virtual void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод обработки попыток подключения клиента к удалённому серверу
			 *
			 * @param domain   доменное имя для резолвинга
			 * @param attempts количество попыток подключения
			 */
			virtual void attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept;
			/**
			 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 */
			virtual void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий истечения таймаута клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param action тип действия для истекшего таймаута
			 * @param delay  задержка таймаута в миллисекундах
			 * @return       нужно ли завершить клиента после истечения таймаута
			 */
			virtual bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
			/**
			 * @brief Метод обработки неудачного резолвинга доменного имени
			 *
			 * @param id     идентификатор DNS-запроса
			 * @param record тип записи DNS
			 * @param domain доменное имя
			 */
			virtual void failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept;
			/**
			 * @brief Метод обработки события неотправленных данных клиенту
			 *
			 * @param eid   идентификатор клиента
			 * @param error тип ошибки отправки данных
			 * @param data  данные, которые не получилось отправить
			 * @param size  размер данных, которые не получилось отправить
			 */
			virtual void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод резолвинга доменного имени удалённого хоста в сетевой адрес
			 *
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для резолвинга
			 * @param addr   указатель на структуру для хранения результата резолвинга
			 */
			virtual void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
		protected:
			/**
			 * @brief Метод обработки события пересоздания процесса
			 *
			 * @param old старый идентификатор процесса
			 * @param pid текущий идентификатор процесса
			 */
			virtual void rebaseCluster(const pid_t old, const pid_t pid) noexcept;
			/**
			 * @brief Метод получения события завершения работы процесса
			 *
			 * @param pid    идентификатор процесса
			 * @param signal сигнал с которым завершился процесс
			 */
			virtual void exitCluster(const pid_t pid, const int32_t signal) noexcept;
			/**
			 * @brief Метод обработки события отправки сообщения процессу кластера
			 *
			 * @param pid  идентификатор процесса
			 * @param size размер отправленного сообщения
			 */
			virtual void sendingCluster(const pid_t pid, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий изменения статуса кластера
			 *
			 * @param pid    идентификатор события
			 * @param status новый статус кластера
			 */
			virtual void stateCluster(const pid_t pid, const event::status_t status) noexcept;
			/**
			 * @brief Метод получения событий активации/деактивации кластера
			 *
			 * @param pid   идентификатор процесса
			 * @param event флаг события кластера
			 */
			virtual void eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
			/**
			 * @brief Метод обработки события получения сообщения от процесса кластера
			 *
			 * @param pid  идентификатор процесса
			 * @param data данные полученного сообщения
			 * @param size размер данных полученного сообщения
			 */
			virtual void messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
			/**
			 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
			 *
			 * @param pid    идентификатор процесса
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 */
			virtual void availableCluster(const pid_t pid, const event::status_t status, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий ошибок кластера
			 *
			 * @param pid         идентификатор процесса
			 * @param error       тип ошибки
			 * @param description описание ошибки
			 */
			virtual void errorCluster(const pid_t pid, const event::error_t error, const string & description) noexcept;
		protected:
			/**
			 * @brief Метод получения состояния TLS
			 *
			 * @param id    идентификатор TLS
			 * @param eid   идентификатор клиента
			 * @param state состояние TLS
			 */
			virtual void stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept;
			/**
			 * @brief Метод получения отпечатка TLS
			 *
			 * @param id      идентификатор TLS
			 * @param eid     идентификатор клиента
			 * @param browser информация о браузере для отпечатка TLS
			 */
			virtual void fingerprintTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::fgp_t::browser_t & browser) noexcept;
			/**
			 * @brief Метод получения ошибок TLS
			 *
			 * @param id      идентификатор TLS
			 * @param eid     идентификатор клиента
			 * @param error   код ошибки TLS
			 * @param message сообщение об ошибке TLS
			 */
			virtual void errorTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод получения событий шифрования/дешифрования данных TLS
			 *
			 * @param id     идентификатор TLS
			 * @param eid    идентификатор клиента
			 * @param event  тип события TLS
			 * @param size   размер данных для события шифрования/дешифрования TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 */
			virtual void processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод очистки чёрного списка события
			 *
			 * @param eid идентификатор события
			 * @return    результат выполнения очистки
			 */
			bool clearBlacklist(const event::id_t eid) noexcept;
			/**
			 * @brief Метод очистки белого списка события
			 *
			 * @param eid идентификатор события
			 * @return    результат выполнения очистки
			 */
			bool clearWhitelist(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод добавления адреса в чёрный список события
			 *
			 * @param eid   идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 */
			bool addToBlacklist(const event::id_t eid, string_view value) noexcept;
			/**
			 * @brief Метод добавления адреса в белый список события
			 *
			 * @param eid   идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 */
			bool addToWhitelist(const event::id_t eid, string_view value) noexcept;
		public:
			/**
			 * @brief Метод удаления адреса из чёрного списка события
			 *
			 * @param eid   идентификатор события
			 * @param value адрес для удаления из чёрного списка
			 * @return      результат выполнения удаления
			 */
			bool removeFromBlacklist(const event::id_t eid, string_view value) noexcept;
			/**
			 * @brief Метод удаления адреса из белого списка события
			 *
			 * @param eid   идентификатор события
			 * @param value адрес для удаления из белого списка
			 * @return      результат выполнения удаления
			 */
			bool removeFromWhitelist(const event::id_t eid, string_view value) noexcept;
		public:
			/**
			 * @brief Метод получения чёрного списка события
			 *
			 * @param eid идентификатор события
			 * @return    чёрный список события
			 */
			const std::unordered_map <string, event::address_t> & getFromBlacklist(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод получения белого списка события
			 *
			 * @param eid идентификатор события
			 * @return    белый список события
			 */
			const std::unordered_map <string, event::address_t> & getFromWhitelist(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод остановки сервера
			 *
			 */
			virtual void stop() noexcept;
			/**
			 * @brief Метод запуска сервера
			 *
			 */
			virtual void start() noexcept;
		public:
			/**
			 * @brief Метод приостановки работы клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат выполнения приостановки работы
			 */
			virtual bool pause(const event::id_t eid) noexcept;
			/**
			 * @brief Метод возобновления работы клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат выполнения возобновления работы
			 */
			virtual bool resume(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод уничтожения события клиента или сервера
			 *
			 * @param eid идентификатор события клиента для уничтожения
			 */
			virtual void destroy(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод проверки, жив ли клиент или сервер
			 *
			 * @param eid идентификатор события клиента для проверки
			 * @return    результат проверки
			 */
			virtual bool isAlive(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param max максимальное количество входящих соединений
			 * @return    результат выполнения перевода в режим прослушивания
			 */
			virtual bool listen(const uint16_t max) noexcept;
		public:
			/**
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 */
			virtual void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Метод получения данных от клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат получения данных
			 */
			virtual bool recv(const event::id_t eid) noexcept;
			/**
			 * @brief Метод отправки данных клиенту
			 *
			 * @param eid    идентификатор события клиента
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных клиенту
			 */
			virtual size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод объединения данных между сервером и другим событием
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения объединения
			 */
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept;
		public:
			/**
			 * @brief Метод получения опций сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    опции сервера или клиента
			 */
			virtual uint16_t getOptions(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки опций сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param options опции сервера или клиента для установки
			 * @return        результат выполнения установки
			 */
			virtual bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
			/**
			 * @brief Метод установки опции сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param option опция сервера или клиента для установки
			 * @param mode   режим установки опции сервера или клиента
			 * @return       результат выполнения установки
			 */
			virtual bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
			 *
			 * @param eid идентификатор события сервера
			 * @return    максимальное количество хопов
			 */
			virtual event::hops_t getHops(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param eid  идентификатор события сервера
			 * @param hops максимальное количество хопов
			 * @return     результат работы функции
			 */
			virtual bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса сервера
			 *
			 * @return сетевой интерфейс сервера
			 */
			virtual string getIface() const noexcept;
			/**
			 * @brief Метод установки сетевого интерфейса сервера
			 *
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 */
			virtual bool setIface(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения порта сервера
			 *
			 * @return порт сервера
			 */
			virtual uint16_t getPort() const noexcept;
			/**
			 * @brief Метод установки порта сервера
			 *
			 * @param port порт сервера для установки
			 * @return     результат выполнения установки
			 */
			virtual bool setPort(const uint16_t port) noexcept;
			/**
			 * @brief Метод получения порта удаленного сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    порт удаленного сервера или клиента
			 */
			virtual uint16_t getPort(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод получения адреса хоста текущей машины
			 *
			 * @return адрес хоста текущей машины
			 */
			virtual const string & getHost() const noexcept;
			/**
			 * @brief Метод установки адреса хоста текущей машины
			 *
			 * @param host адрес хоста текущей машины
			 * @return     результат выполнения установки
			 */
			virtual bool setHost(string_view host) noexcept;
		public:
			/**
			 * @brief Метод получения адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @return        значение адреса сервера
			 */
			virtual string getAddress(const event::address_t address) const noexcept;
			/**
			 * @brief Метод установки адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   значение адреса сервера
			 * @return        результат выполнения установки
			 */
			virtual bool setAddress(const event::address_t address, string_view value) noexcept;
			/**
			 * @brief Метод получения адреса сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param address тип адреса сервера или клиента
			 * @return        значение адреса сервера или клиента
			 */
			virtual string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
		public:
			/**
			 * @brief Метод установки адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   значение адреса сервера
			 * @return        результат выполнения установки
			 */
			virtual bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			/**
			 * @brief Метод получения адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   объект для извлечения адреса сервера
			 * @return        результат выполнения извлечения адреса сервера
			 */
			virtual bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			/**
			 * @brief Метод получения адреса сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param address тип адреса сервера или клиента
			 * @param value   объект для извлечения адреса сервера или клиента
			 * @return        результат выполнения извлечения адреса сервера или клиента
			 */
			virtual bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * @brief Метод получения MTU сетевого интерфейса
			 *
			 * @param eid идентификатор события сервера
			 * @return    MTU сетевого интерфейса
			 */
			virtual uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки MTU сетевого интерфейса
			 *
			 * @param eid идентификатор события сервера
			 * @param mtu размер MTU интерфейса
			 * @return    результат установки MTU сетевого интерфейса
			 */
			virtual bool setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept;
		public:
			/**
			 * @brief Метод получения режима трансляции пакетов сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
			 */
			virtual event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки режима трансляции пакетов сервера или клиента
			 *
			 * @param eid      идентификатор события сервера или клиента
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 */
			virtual bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @return       размер буфера сервера или клиента
			 */
			virtual size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @param size   размер буфера сервера или клиента
			 * @return       результат выполнения установки
			 */
			virtual bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения времени жизни DNS запроса
			 *
			 * @return время жизни DNS запроса в миллисекундах
			 */
			virtual uint32_t getAliveDNS() const noexcept;
			/**
			 * @brief Метод установки времени жизни DNS запроса
			 *
			 * @param alive время жизни DNS запроса в миллисекундах
			 */
			virtual void setAliveDNS(const uint32_t alive) noexcept;
		public:
			/**
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @return режим использования таймаута на чтение события
			 */
			virtual event::usage_t getUsageReadTimeout() const noexcept;
			/**
			 * @brief Метод получения режима использования таймаута на чтение события сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    режим использования таймаута на чтение события сервера или клиента
			 */
			virtual event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 */
			virtual void setUsageReadTimeout(const event::usage_t usage) noexcept;
			/**
			 * @brief Метод установки режима использования таймаута на чтение события сервера или клиента
			 *
			 * @param eid   идентификатор события сервера или клиента
			 * @param usage режим использования таймаута на чтение события сервера или клиента (reusable или disposable)
			 */
			virtual void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
		public:
			/**
			 * @brief Метод получения таймаута сервера
			 *
			 * @param action тип действия сервера
			 * @return       значение таймаута в миллисекундах
			 */
			virtual uint32_t getTimeout(const event::action_t action) const noexcept;
			/**
			 * @brief Метод получения таймаута сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @return       значение таймаута в миллисекундах
			 */
			virtual uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
		public:
			/**
			 * @brief Метод установки таймаута сервера
			 *
			 * @param action  тип действия сервера
			 * @param timeout значение таймаута в миллисекундах
			 */
			virtual void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
			/**
			 * @brief Метод установки таймаута сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param action  тип действия сервера или клиента
			 * @param timeout значение таймаута в миллисекундах
			 */
			virtual void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
		public:
			/**
			 * @brief Метод установки пропускной способности сервера
			 *
			 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
			 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 */
			virtual bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			/**
			 * @brief Метод установки пропускной способности сервера или клиента
			 *
			 * @param eid       идентификатор события сервера или клиента
			 * @param limiting  режим ограничения пропускной способности сервера или клиента (egress или ingress)
			 * @param bandwidth пропускная способность сервера или клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 */
			virtual bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
		public:
			/**
			 * @brief Метод установки параметров keep-alive для сервера или клиента
			 *
			 * @param eid   идентификатор события сервера или клиента
			 * @param cnt   количество пакетов keep-alive
			 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
			 * @param intvl интервал между пакетами keep-alive в секундах
			 * @return      результат выполнения установки
			 */
			virtual bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
		public:
			/**
			 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @return значение DSCP
			 */
			virtual event::dscp_t getDifferentiatedServicesCodePoint() const noexcept;
			/**
			 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @param dscp значение DSCP
			 * @return     результат работы функции
			 */
			virtual bool setDifferentiatedServicesCodePoint(const event::dscp_t dscp) const noexcept;
		public:
			/**
			 * @brief Метод получения обнаружения максимального размера пакета (MTU)
			 *
			 * @return режим обнаружения максимального размера пакета (MTU)
			 */
			virtual event::mtu_discover_t getMaximumTransmissionUnitDiscover() const noexcept;
			/**
			 * @brief Метод установки обнаружения максимального размера пакета (MTU)
			 *
			 * @param mode режим обнаружения максимального размера пакета (MTU)
			 * @return     результат работы функции
			 */
			virtual bool setMaximumTransmissionUnitDiscover(const event::mtu_discover_t mode) const noexcept;
		public:
			/**
			 * @brief Метод активации/деактивации мультикаст группы
			 *
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @param port   порт мультикаст-группы с которого выполняется подписка
			 * @return       результат выполнения установки
			 */
			virtual bool membership(const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
			/**
			 * @brief Метод активации/деактивации мультикаст группы
			 *
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @param port   порт мультикаст-группы с которого выполняется подписка
			 * @return       результат выполнения установки
			 */
			virtual bool membership(const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
		public:
			/**
			 * @brief Метод инициализации сервера
			 *
			 * @param family   семейство адресов
			 * @param type     тип события
			 * @param protocol протокол события
			 * @return         идентификатор созданного сервера
			 */
			virtual event::id_t init(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * @brief Метод установки флага автоматического возрождения процессов
			 *
			 * @param mode флаг возрождения процессов
			 */
			virtual void clusterRebirth(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки названия кластера
			 *
			 * @param name название кластера для установки
			 */
			virtual void clusterName(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения семейства кластера
			 *
			 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
			 */
			virtual unit::cluster_t::family_t clusterFamily() const noexcept;
		public:
			/**
			 * @brief Метод получения режима активации кластера
			 *
			 * @return режим активации кластера
			 */
			virtual event::mode_t clusterMode() const noexcept;
			/**
			 * @brief Метод установки количества процессов кластера
			 *
			 * @param mode флаг активации/деактивации кластера
			 * @param size количество рабочих процессов
			 */
			virtual void clusterMode(const event::mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод получения максимального количества процессов
			 *
			 * @return максимальное количество процессов
			 */
			virtual uint16_t clusterCount() const noexcept;
			/**
			 * @brief Метод установки максимального количества процессов
			 *
			 * @param count максимальное количество процессов
			 */
			virtual void clusterCount(const uint16_t count) noexcept;
		public:
			/**
			 * @brief Метод получения списка дочерних процессов
			 *
			 * @return список дочерних процессов
			 */
			virtual unordered_set <pid_t> clusterWorkers() const noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения родительскому процессу
			 *
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 * @return       количество байт отправленного сообщения
			 */
			virtual size_t clusterSend(const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод отправки сообщения дочернему процессу
			 *
			 * @param pid    идентификатор процесса для получения сообщения
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 * @return       количество байт отправленного сообщения
			 */
			virtual size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения всем дочерним процессам
			 *
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 * @return       количество байт отправленного сообщения
			 */
			virtual size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера события
			 *
			 * @param pid    идентификатор процесса
			 * @param action тип действия события
			 * @return       размер буфера события
			 */
			virtual size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param pid    идентификатор процесса
			 * @param action тип действия события
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 */
			virtual bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(string_view name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
				// Если мы получили идентификатор функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, Args... args) noexcept -> uint32_t {
				// Если мы получили на вход число
				if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @brief Метод установки идентификатора TLS шаблона
			 *
			 * @param sid идентификатор TLS шаблона для установки
			 */
			virtual void setSecurityId(const tls::coder_t::id_t sid) noexcept;
		private:
			/**
			 * @brief Конструктор копирования (запрещаем)
			 *
			 */
			Server(const Server &) = delete;
			/**
			 * @brief Оператор копирования (запрещаем)
			 *
			 * @return текущее значение объекта
			 */
			Server & operator = (const Server &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Server(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param tls объект транспортного уровня безопасности
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Server(tls::coder_t * tls, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param dns объект DNS-резолвера
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Server(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param dns объект DNS-резолвера
			 * @param tls объект транспортного уровня безопасности
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Server(unit::dns_t * dns, tls::coder_t * tls, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Server() noexcept;
	} server_t;
};

#endif // __AWH_SERVER__
