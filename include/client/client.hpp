/**
 * @file: client.hpp
 * @date: 2026-04-05
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
#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Наши модули
 */
#include "../net/tls.hpp"
#include "../units/dns.hpp"
#include "../units/client.hpp"

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
	 * @brief Класс клиента
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Client {
		protected:
			// Адрес хоста целевой машины
			string _host;
		protected:
			// Идентификатор TLS для выполнения запросов к серверу
			tls_t::id_t _tid;
			// Идентификатор клиента для выполнения запросов к серверу
			event::id_t _eid;
		protected:
			// Объект работы с сетевыми адресами
			net_addr_t _addr;
		protected:
			// Функция обратного вызова для обработки клиента
			callback_t _callback;
		protected:
			// Таймаут резолвинга доменного имени в миллисекундах
			atomic_uint32_t _timeoutDNS;
		protected:
			// Объект транспортного уровня безопасности
			tls_t * _tls;
			// Объект DNS-резолвера
			unit::dns_t * _dns;
			// Объект клиента
			unit::client_t * _client;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод изменения статуса клиента
			 *
			 * @param status новый статус клиента
			 */
			void status(const event::status_t status) noexcept;
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
			 * @brief Метод обработки действий клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param action действие клиента
			 */
			void action(const event::id_t eid, const event::action_t action) noexcept;
			/**
			 * @brief Метод обработки событий получения данных клиентом
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер данных клиента
			 * @param size   размер данных клиента
			 */
			void read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод получения события ошибок
			 *
			 * @param eid     идентификатор события
			 * @param error   код ошибки
			 * @param message сообщение об ошибке
			 */
			void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 */
			void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
			/**
			 * @brief Метод обработки события неотправленных данных клиента
			 *
			 * @param eid   идентификатор клиента
			 * @param error тип ошибки отправки данных
			 * @param data  данные, которые не получилось отправить
			 * @param size  размер данных, которые не получилось отправить
			 */
			void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept;
		private:
			/**
			 * @brief Метод получения состояния TLS
			 *
			 * @param id    идентификатор TLS
			 * @param state состояние TLS
			 */
			void stateTLS(const tls_t::id_t id, const tls_t::state_t state) noexcept;
			/**
			 * @brief Метод получения ошибок TLS
			 *
			 * @param id      идентификатор TLS
			 * @param error   код ошибки TLS
			 * @param message сообщение об ошибке TLS
			 */
			void errorTLS(const tls_t::id_t id, const tls_t::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод получения событий шифрования/дешифрования данных TLS
			 *
			 * @param id     идентификатор TLS
			 * @param event  тип события TLS
			 * @param size   размер данных для события шифрования/дешифрования TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 */
			void processTLS(const tls_t::id_t id, const tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
		private:
			/**
			 * @brief Метод получения события DNS-резолвера
			 *
			 * @param status статус события DNS-резолвера
			 */
			void statusDNS(const event::status_t status) noexcept;
			/**
			 * @brief Метод обработки попыток подключения клиента к удалённому серверу
			 *
			 * @param id       идентификатор DNS-запроса
			 * @param domain   доменное имя для резолвинга
			 * @param attempts количество попыток подключения
			 */
			void attemptsDNS(const unit::dns_t::id_t id, const string & domain, const uint8_t attempts) noexcept;
			/**
			 * @brief Метод резолвинга доменного имени в сетевой адрес
			 *
			 * @param id     идентификатор DNS-запроса
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для резолвинга
			 * @param addr   указатель на структуру для хранения результата резолвинга
			 */
			void resolveDNS(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
		public:
			/**
			 * @brief Метод остановки клиента
			 *
			 */
			virtual void stop() noexcept;
			/**
			 * @brief Метод запуска клиента
			 *
			 */
			virtual void start() noexcept;
		public:
			/**
			 * @brief Метод приостановки работы клиента
			 *
			 * @return результат выполнения приостановки работы
			 */
			virtual bool pause() noexcept;
			/**
			 * @brief Метод возобновления работы клиента
			 *
			 * @return результат выполнения возобновления работы
			 */
			virtual bool resume() noexcept;
		public:
			/**
			 * @brief Метод мультиподключения клиентов к удалённым хостам
			 *
			 * @return результат выполнения подключения
			 */
			virtual bool connect() noexcept;
			/**
			 * @brief Метод отключения клиента от удалённого сервера
			 *
			 * @return результат выполнения отключения
			 */
			virtual bool disconnect() noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 */
			virtual void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 */
			virtual void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Метод отправки данных серверу
			 *
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных серверу
			 */
			virtual size_t send(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса клиента
			 *
			 * @return сетевой интерфейс клиента
			 */
			virtual string getIface() const noexcept;
			/**
			 * @brief Метод установки сетевого интерфейса клиента
			 *
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 */
			virtual bool setIface(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения порта удаленного сервера
			 *
			 * @return порт удаленного сервера
			 */
			virtual uint16_t getPort() const noexcept;
			/**
			 * @brief Метод установки порта удаленного сервера
			 *
			 * @param port порт удаленного сервера для установки
			 * @return     результат выполнения установки
			 */
			virtual bool setPort(const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @return адрес хоста целевой машины
			 */
			virtual string getTarget() const noexcept;
			/**
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 */
			virtual bool setTarget(string_view target) noexcept;
		public:
			/**
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 */
			virtual bool setTarget(const net::addr_t * target) noexcept;
			/**
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @param target объект для извлечения адреса хоста целевой машины
			 * @return       результат выполнения извлечения адреса хоста целевой машины
			 */
			virtual bool getTarget(unique_ptr <net::addr_t> & target) const noexcept;
		public:
			/**
			 * @brief Метод получения адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @return        значение адреса клиента
			 */
			virtual string getAddress(const event::address_t address) const noexcept;
			/**
			 * @brief Метод установки адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   значение адреса клиента
			 * @return        результат выполнения установки
			 */
			virtual bool setAddress(const event::address_t address, string_view value) noexcept;
		public:
			/**
			 * @brief Метод установки адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   значение адреса клиента
			 * @return        результат выполнения установки
			 */
			virtual bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			/**
			 * @brief Метод получения адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   объект для извлечения адреса клиента
			 * @return        результат выполнения извлечения адреса клиента
			 */
			virtual bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера клиента
			 *
			 * @param action тип действия клиента
			 * @return       размер буфера клиента
			 */
			virtual size_t getBufferSize(const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера клиента
			 *
			 * @param action тип действия клиента
			 * @param size   размер буфера клиента
			 * @return       результат выполнения установки
			 */
			virtual bool setBufferSize(const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения таймаута резолвинга доменного имени
			 *
			 * @return таймаут резолвинга доменного имени в миллисекундах
			 */
			virtual uint32_t getTimeoutDNS() const noexcept;
			/**
			 * @brief Метод установки таймаута резолвинга доменного имени
			 *
			 * @param timeout таймаут резолвинга доменного имени в миллисекундах
			 */
			virtual void setTimeoutDNS(const uint32_t timeout) noexcept;
		public:
			/**
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @return режим использования таймаута на чтение события
			 */
			virtual event::usage_t getUsageReadTimeout() const noexcept;
			/**
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 */
			virtual void setUsageReadTimeout(const event::usage_t usage) noexcept;
		public:
			/**
			 * @brief Метод получения таймаута клиента
			 *
			 * @param action тип действия клиента
			 * @return       значение таймаута в миллисекундах
			 */
			virtual uint32_t getTimeout(const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки таймаута клиента
			 *
			 * @param action  тип действия клиента
			 * @param timeout значение таймаута в миллисекундах
			 */
			virtual void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
		public:
			/**
			 * @brief Метод установки пропускной способности клиента
			 *
			 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
			 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 */
			virtual bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
		public:
			/**
			 * @brief Метод установки параметров keep-alive для клиента
			 *
			 * @param cnt   количество пакетов keep-alive
			 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
			 * @param intvl интервал между пакетами keep-alive в секундах
			 * @return      результат выполнения установки
			 */
			virtual bool keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
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
			 * @brief Метод установки идентификатора события клиента
			 *
			 * @param eid идентификатор события для установки
			 */
			void setEventId(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод установки идентификатора TLS
			 *
			 * @param tid идентификатор TLS для установки
			 */
			void setSecurityId(const tls_t::id_t tid) noexcept;
		private:
			/**
			 * @brief Конструктор копирования (запрещаем)
			 *
			 */
			Client(const Client &) = delete;
			/**
			 * @brief Оператор копирования (запрещаем)
			 *
			 * @return текущее значение объекта
			 */
			Client & operator = (const Client &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param tls    объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, tls_t * tls, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param dns    объект DNS-резолвера
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param client объект юнита клиента
			 * @param dns    объект DNS-резолвера
			 * @param tls    объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Client(unit::client_t * client, unit::dns_t * dns, tls_t * tls, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Client() noexcept;
	} client_t;
};

#endif // __AWH_CLIENT__
