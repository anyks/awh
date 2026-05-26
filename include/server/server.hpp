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
			 * @brief Временное состояние сервера
			 *
			 */
			enum class state_t : uint8_t {
				NONE     = 0x00, // Нет состояния
				SERVER   = 0x01, // Состояние запуска сервера
				RESOLVER = 0x02  // Состояние запуска DNS-резолвера
			};
		protected:
			// Адрес хоста целевой машины
			string _host;
		protected:
			// Идентификатор сервера
			event::id_t _eid;
			// Идентификатор TLS шаблона
			tls::coder_t::id_t _tid;
		protected:
			// Объект работы с сетевыми адресами
			net_addr_t _addr;
		protected:
			// Функция обратного вызова для обработки сервера
			callback_t _callback;
		protected:
			// Мютекс для блокировки потоков при работе с TLS
			lock_state_t <std::shared_mutex> _mtx;
		protected:
			// Список для сопоставления идентификаторов клиентов с идентификаторами TLS
			unordered_map <event::id_t, tls::coder_t::id_t> _tls;
		protected:
			// Объект DNS-резолвера
			unit::dns_t * _dns;
			// Объект транспортного уровня безопасности
			tls::coder_t * _coder;
			// Объект сервера
			unit::server_t * _server;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * @brief Метод изменения статуса сервера
			 *
			 * @param status новый статус сервера
			 * @param state  новое временное состояние сервера
			 */
			virtual void status(const event::status_t status, const state_t state) noexcept;
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
			 * @brief Метод обработки события неотправленных данных клиенту
			 *
			 * @param eid   идентификатор клиента
			 * @param error тип ошибки отправки данных
			 * @param data  данные, которые не получилось отправить
			 * @param size  размер данных, которые не получилось отправить
			 */
			virtual void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept;
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
		protected:
			/**
			 * @brief Метод резолвинга доменного имени в сетевой адрес
			 *
			 * @param id     идентификатор DNS-запроса
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для резолвинга
			 * @param addr   указатель на структуру для хранения результата резолвинга
			 */
			virtual void resolveDNS(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
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
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param max максимальное количество входящих соединений
			 * @return    результат выполнения перевода в режим прослушивания
			 */
			virtual bool listen(const uint16_t max) noexcept;
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
			 * @brief Метод уничтожения события клиента
			 *
			 * @param eid идентификатор события клиента для уничтожения
			 */
			virtual void destroy(const event::id_t eid) noexcept;
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
			 * @brief Метод перемещения данных между сервером и другим событием
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения перемещения
			 */
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept;
		public:
			/**
			 * @brief Метод получения опций клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    опции клиента
			 */
			virtual uint16_t getOptions(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки опций клиента
			 *
			 * @param eid     идентификатор события клиента
			 * @param options опции клиента для установки
			 * @return        результат выполнения установки
			 */
			virtual bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
			/**
			 * @brief Метод установки опции клиента
			 *
			 * @param eid    идентификатор события клиента
			 * @param option опция клиента для установки
			 * @param mode   режим установки опции клиента
			 * @return       результат выполнения установки
			 */
			virtual bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
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
			 * @brief Метод получения порта удаленного клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    порт удаленного клиента
			 */
			virtual uint16_t getPort(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод получения адреса хоста текущей машины
			 *
			 * @return адрес хоста текущей машины
			 */
			virtual string getHost() const noexcept;
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
			 * @brief Метод получения адреса клиента
			 *
			 * @param eid     идентификатор события клиента
			 * @param address тип адреса клиента
			 * @return        значение адреса клиента
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
			 * @brief Метод получения адреса клиента
			 *
			 * @param eid     идентификатор события клиента
			 * @param address тип адреса клиента
			 * @param value   объект для извлечения адреса клиента
			 * @return        результат выполнения извлечения адреса клиента
			 */
			virtual bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера клиента
			 *
			 * @param eid    идентификатор события клиента
			 * @param action тип действия клиента
			 * @return       размер буфера клиента
			 */
			virtual size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера клиента
			 *
			 * @param eid    идентификатор события клиента
			 * @param action тип действия клиента
			 * @param size   размер буфера клиента
			 * @return       результат выполнения установки
			 */
			virtual bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @return режим использования таймаута на чтение события
			 */
			virtual event::usage_t getUsageReadTimeout() const noexcept;
			/**
			 * @brief Метод получения режима использования таймаута на чтение события клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    режим использования таймаута на чтение события клиента
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
			 * @brief Метод установки режима использования таймаута на чтение события клиента
			 *
			 * @param eid   идентификатор события клиента
			 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
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
			 * @brief Метод получения таймаута клиента
			 *
			 * @param eid    идентификатор события клиента
			 * @param action тип действия клиента
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
			 * @brief Метод установки таймаута клиента
			 *
			 * @param eid     идентификатор события клиента
			 * @param action  тип действия клиента
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
			 * @brief Метод установки пропускной способности клиента
			 *
			 * @param eid       идентификатор события клиента
			 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
			 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 */
			virtual bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
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
			virtual bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
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
			 * @brief Метод установки идентификатора события сервера
			 *
			 * @param eid идентификатор события для установки
			 */
			virtual void setEventId(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод установки идентификатора TLS шаблона
			 *
			 * @param tid идентификатор TLS шаблона для установки
			 */
			virtual void setSecurityId(const tls::coder_t::id_t tid) noexcept;
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
			 * @param server объект юнита сервера
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Server(unit::server_t * server, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param server объект юнита сервера
			 * @param coder  объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Server(unit::server_t * server, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param server объект юнита сервера
			 * @param dns    объект DNS-резолвера
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Server(unit::server_t * server, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param server объект юнита сервера
			 * @param dns    объект DNS-резолвера
			 * @param coder  объект транспортного уровня безопасности
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 */
			explicit Server(unit::server_t * server, unit::dns_t * dns, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Server() noexcept;
	} server_t;
};

#endif // __AWH_SERVER__
