/**
 * @file: server.hpp
 * @date: 2026-05-17
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл фасада сервера — публичный API класса Server, объединяющего приём подключений, транспорт,
 *        TLS с поддержкой нескольких сертификатов, DNS-резолвинг, кластеризацию и подписку на события для TCP, UDP,
 *        SCTP, UDS, DTLS и QUIC
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SERVER__
#define __AWH_SERVER__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../units/dns.hpp"
#include "../units/quic.hpp"
#include "../units/server.hpp"
#include "../cryptography/tls/coder.hpp"

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
	 * @brief Класс сервера
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Server {
		protected:
			/**
			 * @brief Структура для хранения параметров DNS-резолвера
			 *
			 * @details Хранит идентификатор DNS-резолвера, время жизни DNS-запроса и объект DNS-резолвера.
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Domain_Name_System {
				// Идентификатор DNS-резолвера
				unit::dns_t::id_t id;
				// Время жизни DNS запроса (в миллисекундах, по умолчанию 15 секунд)
				atomic_uint32_t alive;
				// Объект DNS-резолвера
				unit::dns_t * client;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Domain_Name_System() noexcept;
			} dns_t;
			/**
			 * @brief Структура идентификаторов клиента
			 *
			 * @details Хранит идентификатор сервера и идентификатор безопасности TLS.
			 *
			 * @warning Структура упакована, и ссылку на её поле связывать нельзя: выравнивание
			 *          у полей упакованной структуры не обеспечено, а ссылка его требует. GCC
			 *          отвечает на такое отказом «cannot bind packed field», clang же берёт
			 *          молча, отчего на macOS и системах BSD этого не видно вовсе, а на Solaris
			 *          и Linux сборка встаёт
			 *
			 * @note Передавать поля отсюда следует значением - через static_cast к их же типу.
			 *       Ссылку связывают не только явные её объявления: доводы шаблонов, взятые как
			 *       Args &&, тоже, и оттого отказ вылезал у вызовов записи в журнал
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Identifier {
				// Идентификатор клиента
				event::id_t eid;
				// Контекст шаблона безопасности TLS
				tls::coder_t::id_t cts;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Identifier() noexcept;
			} __attribute__((packed)) id_t;
			/**
			 * @brief Структура для хранения параметров транспортного уровня безопасности
			 *
			 * @details Хранит объект транспортного уровня безопасности и список для сопоставления идентификаторов клиентов с идентификаторами TLS.
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ TLS {
				// Объект транспортного уровня безопасности
				tls::coder_t * coder;
				// Список для сопоставления идентификаторов клиентов с идентификаторами TLS
				unordered_map <event::id_t, tls::coder_t::id_t> safety;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit TLS() noexcept;
			} tls_t;
			/**
			 * @brief Структура юнита сервера
			 *
			 * @details Хранит объект работы с сетевыми адресами и объект юнита сервера.
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Unit {
				// Объект работы с сетевыми адресами
				net_addr_t addr;
				// Объект юнита сервера (транспорты TCP/UDP/SCTP и прикладные протоколы поверх них)
				unit::server_t server;
				// Объект юнита сервера QUIC (выбирается при инициализации транспортом protocol_t::QUIC)
				unit::quic_server_t quic;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Unit(const fmk_t * fmk, const log_t * log) noexcept;
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
			// Протокол транспорта сервера (выбирается при инициализации, определяет обработку данных)
			event::protocol_t _protocol;
		protected:
			// Тип сокета транспорта сервера (STREAM/DATAGRAM/SEQPACKET - определяет доступность датаграмм)
			event::type_t _type;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * @brief Метод проверки рабочего состояния сервера
			 *
			 * @note Проверяет рабочее состояние активного юнита транспорта, выбираемого
			 *       по протоколу (QUIC - выделенный юнит, остальные транспорты - общий
			 *       юнит сервера)
			 *
			 * @return результат проверки рабочего состояния
			 *
			 */
			bool active() const noexcept;
		protected:
			/**
			 * @brief Методы диспетчеризации к активному юниту транспорта
			 *
			 * @note Транспорт выбирается по протоколу сервера: для protocol_t::QUIC
			 *       работает выделенный юнит сервера QUIC, для остальных транспортов -
			 *       общий юнит сервера. Событием прослушивания выступает _id.eid
			 *
			 */
			bool commitUnit() noexcept;
			bool launchUnit() noexcept;
			bool listenUnit(const uint32_t max) noexcept;
			void startUnit() noexcept;
			void stopUnit() noexcept;
			event::family_t familyUnit() const noexcept;
			event::status_t statusUnit() const noexcept;
			string getAddressUnit(const event::address_t address) const noexcept;
			uint16_t getPortUnit() const noexcept;
			bool setAddressUnit(const event::address_t address, string_view value) noexcept;
		protected:
			/**
			 * @brief Метод изменения статуса сервера
			 *
			 * @param index  индекс обрабатываемого события
			 * @param status новый статус сервера
			 *
			 */
			virtual void status(const uint8_t index, const event::status_t status) noexcept;
		protected:
			/**
			 * @brief Метод обработки события разрешения подключения
			 *
			 * @param eid идентификатор сервера
			 * @param cid идентификатор клиента
			 *
			 */
			virtual void accept(const event::id_t eid, const event::id_t cid) noexcept;
			/**
			 * @brief Метод обработки установленного соединения QUIC (RFC 9000)
			 *
			 * @note Транслируется приложению как принятие нового соединения; рукопожатие
			 *       TLS 1.3 ведёт само соединение QUIC, поэтому слой TLS-over-stream
			 *       обходится (RFC 9001)
			 *
			 * @param cid идентификатор сессии соединения
			 *
			 */
			virtual void opened(const event::id_t cid) noexcept;
			/**
			 * @brief Метод обработки собранных данных потока соединения QUIC
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param sid  идентификатор потока приложения
			 * @param data собранные данные потока
			 * @param fin  флаг завершения потока удалённым эндпоинтом
			 *
			 */
			virtual void stream(const event::id_t cid, const uint64_t sid, const string & data, const bool fin) noexcept;
			/**
			 * @brief Метод обработки освобождения буфера отправки потока соединения QUIC (сигнал writable)
			 *
			 * @param cid идентификатор сессии соединения
			 * @param sid идентификатор потока приложения
			 *
			 */
			virtual void writable(const event::id_t cid, const uint64_t sid) noexcept;
			/**
			 * @brief Метод обработки принятой датаграммы приложения QUIC (RFC 9221)
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param data данные принятой датаграммы
			 *
			 */
			virtual void message(const event::id_t cid, const string & data) noexcept;
			/**
			 * @brief Метод обработки завершения соединения QUIC (RFC 9000 §10)
			 *
			 * @param cid   идентификатор сессии соединения
			 * @param error код ошибки завершения соединения
			 *
			 */
			virtual void closed(const event::id_t cid, const quic::error_t error) noexcept;
			/**
			 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
			 *
			 * @param eid  идентификатор события
			 * @param info информационные метаданные о дейтаграммном пакете
			 *
			 */
			virtual void traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept;
			/**
			 * @brief Метод обработки попыток подключения клиента к удалённому серверу
			 *
			 * @param domain   доменное имя для разрешения
			 * @param attempts количество попыток подключения
			 *
			 */
			virtual void attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept;
			/**
			 * @brief Метод обработки неудачного разрешения доменного имени
			 *
			 * @param id     идентификатор DNS-запроса
			 * @param record тип записи DNS
			 * @param domain доменное имя
			 *
			 */
			virtual void failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept;
			/**
			 * @brief Метод разрешения доменного имени удалённого хоста в сетевой адрес
			 *
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для разрешения
			 * @param addr   указатель на структуру для хранения результата разрешения
			 *
			 */
			virtual void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
		public:
			/**
			 * @brief Метод обработки событий записи данных клиентом
			 *
			 * @param eid  идентификатор клиента
			 * @param size размер данных для записи
			 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void write(const event::id_t eid, const size_t size, void * ctx) noexcept;
			/**
			 * @brief Метод обработки событий изменения состояния сервера
			 *
			 * @param eid    идентификатор клиента
			 * @param status новый статус сервера
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void state(const event::id_t eid, const event::status_t status, void * ctx) noexcept;
			/**
			 * @brief Метод обработки действий сервера
			 *
			 * @param eid    идентификатор клиента
			 * @param action действие сервера
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void action(const event::id_t eid, const event::action_t action, void * ctx) noexcept;
			/**
			 * @brief Метод обработки событий получения данных сервером
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер данных сервера
			 * @param size   размер данных сервера
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void read(const event::id_t eid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			/**
			 * @brief Метод обработки события ошибки
			 *
			 * @param eid     идентификатор события
			 * @param error   код ошибки
			 * @param message сообщение об ошибке
			 * @param ctx     промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void error(const event::id_t eid, const event::error_t error, const string & message, void * ctx) noexcept;
			/**
			 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void available(const event::id_t eid, const event::status_t status, const size_t size, void * ctx) noexcept;
			/**
			 * @brief Метод обработки событий истечения таймаута клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param action тип действия для истёкшего таймаута
			 * @param delay  задержка таймаута в миллисекундах
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 * @return       нужно ли завершить клиента после истечения таймаута
			 *
			 */
			virtual bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay, void * ctx) noexcept;
			/**
			 * @brief Метод обработки события невозможности отправки данных клиенту
			 *
			 * @param eid    идентификатор клиента
			 * @param error  тип ошибки отправки данных
			 * @param buffer данные, которые не получилось отправить
			 * @param size   размер данных, которые не получилось отправить
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
		protected:
			/**
			 * @brief Метод обработки события пересоздания процесса
			 *
			 * @param old старый идентификатор процесса
			 * @param pid текущий идентификатор процесса
			 *
			 */
			virtual void rebaseCluster(const pid_t old, const pid_t pid) noexcept;
			/**
			 * @brief Метод получения события завершения работы процесса
			 *
			 * @param pid    идентификатор процесса
			 * @param status состояние, с которым завершился процесс
			 *
			 */
			virtual void exitCluster(const pid_t pid, const int32_t status) noexcept;
			/**
			 * @brief Метод обработки события отправки сообщения процессу кластера
			 *
			 * @param pid  идентификатор процесса
			 * @param size размер отправленного сообщения
			 *
			 */
			virtual void sendingCluster(const pid_t pid, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий изменения статуса кластера
			 *
			 * @param pid    идентификатор события
			 * @param status новый статус кластера
			 *
			 */
			virtual void stateCluster(const pid_t pid, const event::status_t status) noexcept;
			/**
			 * @brief Метод обработки событий активации/деактивации кластера
			 *
			 * @param pid   идентификатор процесса
			 * @param event флаг события кластера
			 *
			 */
			virtual void eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
			/**
			 * @brief Метод обработки события получения сообщения от процесса кластера
			 *
			 * @param pid  идентификатор процесса
			 * @param data данные полученного сообщения
			 * @param size размер данных полученного сообщения
			 *
			 */
			virtual void messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
			/**
			 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
			 *
			 * @param pid    идентификатор процесса
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 *
			 */
			virtual void availableCluster(const pid_t pid, const event::status_t status, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий ошибок кластера
			 *
			 * @param pid         идентификатор процесса
			 * @param error       тип ошибки
			 * @param description описание ошибки
			 *
			 */
			virtual void errorCluster(const pid_t pid, const event::error_t error, const string & description) noexcept;
		protected:
			/**
			 * @brief Метод получения состояния TLS
			 *
			 * @param id    идентификатор TLS
			 * @param eid   идентификатор клиента
			 * @param state состояние TLS
			 *
			 */
			virtual void stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept;
			/**
			 * @brief Метод получения отпечатка TLS
			 *
			 * @param id      идентификатор TLS
			 * @param eid     идентификатор клиента
			 * @param browser информация о браузере для отпечатка TLS
			 *
			 */
			virtual void fingerprintTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::fgp_t::browser_t & browser) noexcept;
			/**
			 * @brief Метод обработки ошибок TLS
			 *
			 * @param id      идентификатор TLS
			 * @param eid     идентификатор клиента
			 * @param error   код ошибки TLS
			 * @param message сообщение об ошибке TLS
			 *
			 */
			virtual void errorTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод обработки событий шифрования/дешифрования данных TLS
			 *
			 * @param id     идентификатор TLS
			 * @param eid    идентификатор клиента
			 * @param event  тип события TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 * @param size   размер данных для события шифрования/дешифрования TLS
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 */
			virtual void processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
		public:
			/**
			 * @brief Метод очистки чёрного списка события
			 *
			 * @param eid идентификатор события
			 * @return    результат выполнения очистки
			 *
			 */
			bool clearBlacklist(const event::id_t eid) noexcept;
			/**
			 * @brief Метод очистки белого списка события
			 *
			 * @param eid идентификатор события
			 * @return    результат выполнения очистки
			 *
			 */
			bool clearWhitelist(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод добавления адреса в чёрный список события
			 *
			 * @param eid   идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 *
			 */
			bool addToBlacklist(const event::id_t eid, string_view value) noexcept;
			/**
			 * @brief Метод добавления адреса в белый список события
			 *
			 * @param eid   идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 *
			 */
			bool addToWhitelist(const event::id_t eid, string_view value) noexcept;
		public:
			/**
			 * @brief Метод удаления адреса из чёрного списка события
			 *
			 * @param eid   идентификатор события
			 * @param value адрес для удаления из чёрного списка
			 * @return      результат выполнения удаления
			 *
			 */
			bool removeFromBlacklist(const event::id_t eid, string_view value) noexcept;
			/**
			 * @brief Метод удаления адреса из белого списка события
			 *
			 * @param eid   идентификатор события
			 * @param value адрес для удаления из белого списка
			 * @return      результат выполнения удаления
			 *
			 */
			bool removeFromWhitelist(const event::id_t eid, string_view value) noexcept;
		public:
			/**
			 * @brief Метод получения чёрного списка события
			 *
			 * @param eid идентификатор события
			 * @return    чёрный список события
			 *
			 */
			const unordered_map <string, event::address_t> & getFromBlacklist(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод получения белого списка события
			 *
			 * @param eid идентификатор события
			 * @return    белый список события
			 *
			 */
			const unordered_map <string, event::address_t> & getFromWhitelist(const event::id_t eid) const noexcept;
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
			 *
			 */
			virtual bool pause(const event::id_t eid) noexcept;
			/**
			 * @brief Метод возобновления работы клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат выполнения возобновления работы
			 *
			 */
			virtual bool resume(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод уничтожения события клиента или сервера
			 *
			 * @param eid идентификатор события клиента для уничтожения
			 *
			 */
			virtual void destroy(const event::id_t eid) noexcept;
		public:
			/**
			 * @brief Метод проверки, жив ли клиент или сервер
			 *
			 * @param eid идентификатор события клиента для проверки
			 * @return    результат проверки
			 *
			 */
			virtual bool isAlive(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод установки промежуточного контекста события подключённого клиента
			 *
			 * @param eid идентификатор события сервера
			 * @param ctx указатель на контекст события
			 * @return    результат выполнения установки
			 *
			 */
			virtual bool setContext(const event::id_t eid, void * ctx) noexcept;
		public:
			/**
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param max максимальное количество входящих соединений
			 * @return    результат выполнения перевода в режим прослушивания
			 *
			 */
			virtual bool listen(const uint16_t max) noexcept;
		public:
			/**
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 *
			 */
			virtual void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Метод получения данных от клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат получения данных
			 *
			 */
			virtual bool recv(const event::id_t eid) noexcept;
			/**
			 * @brief Метод отправки данных клиенту
			 *
			 * @param eid    идентификатор события клиента
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных клиенту
			 *
			 */
			virtual size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод открытия потока приложения соединения QUIC
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param mode режим однонаправленного потока
			 * @return     идентификатор открытого потока
			 *
			 */
			virtual uint64_t open(const event::id_t cid, const bool mode = false) noexcept;
			/**
			 * @brief Метод отправки данных в поток приложения соединения QUIC
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param sid    идентификатор потока приложения
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @param fin    флаг завершения потока
			 * @return       количество байт данных, поставленных в очередь отправки
			 *
			 */
			virtual size_t send(const event::id_t cid, const uint64_t sid, const void * buffer, const size_t size, const bool fin = false) noexcept;
			/**
			 * @brief Метод установки водяных меток буфера отправки потоков сессии соединения QUIC (backpressure)
			 *
			 * @note Верхняя метка ограничивает несобранный буфер потока (send() принимает частично),
			 *       по опустошению ниже нижней метки поток сигнализируется колбэком "writable". Ноль - снято
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
			 * @param low  нижняя водяная метка (порог сигнала "writable")
			 *
			 */
			virtual void sendWaterMarks(const event::id_t cid, const size_t high, const size_t low) noexcept;
			/**
			 * @brief Метод назначения pull-источника данных потока сессии соединения QUIC (RFC 9000 §2.2)
			 *
			 * @note Альтернатива send() для больших тел: движок сам тянет данные у источника по мере места
			 *       в буфере отправки, не требуя держать копию тела
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param sid    идентификатор потока приложения
			 * @param source pull-источник данных тела потока
			 *
			 */
			virtual void dataSource(const event::id_t cid, const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
			/**
			 * @brief Метод отправки датаграммы приложения соединению QUIC (RFC 9221)
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param buffer буфер данных датаграммы для отправки
			 * @param size   размер данных датаграммы для отправки
			 * @return       результат отправки
			 *
			 */
			virtual bool datagram(const event::id_t cid, const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод получения предельного размера отправляемой датаграммы QUIC (RFC 9221 §3)
			 *
			 * @param cid идентификатор сессии соединения
			 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
			 *
			 */
			virtual size_t datagrams(const event::id_t cid) const noexcept;
			/**
			 * @brief Метод завершения соединения QUIC приложением (RFC 9000 §10.2)
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param code   код ошибки приложения
			 * @param reason человекочитаемая причина завершения
			 *
			 */
			virtual void close(const event::id_t cid, const uint64_t code = 0, string_view reason = "") noexcept;
		public:
			/**
			 * @brief Метод установки локальных транспортных параметров соединений QUIC (RFC 9000 §7.4)
			 *
			 * @note Применяется только к транспорту QUIC. Задаётся до запуска сервера
			 *
			 * @param params локальные транспортные параметры
			 *
			 */
			virtual void params(const quic::params::params_t & params) noexcept;
			/**
			 * @brief Метод установки проверки адреса клиента через пакет Retry QUIC (RFC 9000 §8.1.2)
			 *
			 * @note Применяется только к транспорту QUIC. Задаётся до запуска сервера
			 *
			 * @param mode режим проверки адреса клиента
			 *
			 */
			virtual void retry(const bool mode) noexcept;
			/**
			 * @brief Метод установки уведомления о перегрузке пути QUIC (RFC 9000 §13.4)
			 *
			 * @note Применяется только к транспорту QUIC. Задаётся до запуска сервера
			 *
			 * @param mode режим уведомления о перегрузке пути
			 *
			 */
			virtual void ecn(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод объединения данных между сервером и другим событием
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения объединения
			 *
			 */
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept;
		public:
			/**
			 * @brief Метод получения опций сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    опции сервера или клиента
			 *
			 */
			virtual uint16_t getOptions(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки опций сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param options опции сервера или клиента для установки
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
			/**
			 * @brief Метод установки опции сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param option опция сервера или клиента для установки
			 * @param mode   режим установки опции сервера или клиента
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
			 *
			 * @return метаданные последнего принятого дейтаграммного пакета
			 *
			 */
			virtual net::dgram_info_t getTrafficInfo() const noexcept;
		public:
			/**
			 * @brief Метод получения количества хопов последнего принятого пакета
			 *
			 * @return количество хопов последнего принятого пакета
			 *
			 */
			virtual uint8_t getCountHops() const noexcept;
			/**
			 * @brief Метод установки количества хопов последнего принятого пакета
			 *
			 * @param hops количество хопов последнего принятого пакета
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setCountHops(const uint8_t hops) noexcept;
		public:
			/**
			 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
			 *
			 * @param eid идентификатор события сервера
			 * @return    максимальное количество хопов
			 *
			 */
			virtual event::hops_t getHops(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param eid  идентификатор события сервера
			 * @param hops максимальное количество хопов
			 * @return     результат работы функции
			 *
			 */
			virtual bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса сервера
			 *
			 * @return сетевой интерфейс сервера
			 *
			 */
			virtual string getIface() const noexcept;
			/**
			 * @brief Метод установки сетевого интерфейса сервера
			 *
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setIface(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения порта сервера
			 *
			 * @return порт сервера
			 *
			 */
			virtual uint16_t getPort() const noexcept;
			/**
			 * @brief Метод установки порта сервера
			 *
			 * @param port порт сервера для установки
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setPort(const uint16_t port) noexcept;
			/**
			 * @brief Метод получения порта подключённого клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    порт подключённого клиента
			 *
			 */
			virtual uint16_t getPort(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод получения адреса хоста текущей машины
			 *
			 * @return адрес хоста текущей машины
			 *
			 */
			virtual const string & getHost() const noexcept;
			/**
			 * @brief Метод установки адреса хоста текущей машины
			 *
			 * @param host адрес хоста текущей машины
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setHost(string_view host) noexcept;
		public:
			/**
			 * @brief Метод получения адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @return        значение адреса сервера
			 *
			 */
			virtual string getAddress(const event::address_t address) const noexcept;
			/**
			 * @brief Метод установки адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   значение адреса сервера
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setAddress(const event::address_t address, string_view value) noexcept;
			/**
			 * @brief Метод получения адреса сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param address тип адреса сервера или клиента
			 * @return        значение адреса сервера или клиента
			 *
			 */
			virtual string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
		public:
			/**
			 * @brief Метод установки адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   значение адреса сервера
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			/**
			 * @brief Метод получения адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   объект для извлечения адреса сервера
			 * @return        результат выполнения извлечения адреса сервера
			 *
			 */
			virtual bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			/**
			 * @brief Метод получения адреса сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param address тип адреса сервера или клиента
			 * @param value   объект для извлечения адреса сервера или клиента
			 * @return        результат выполнения извлечения адреса сервера или клиента
			 *
			 */
			virtual bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * @brief Метод получения MTU сетевого интерфейса
			 *
			 * @param eid идентификатор события сервера
			 * @return    MTU сетевого интерфейса
			 *
			 */
			virtual uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки MTU сетевого интерфейса
			 *
			 * @param eid идентификатор события сервера
			 * @param mtu размер MTU интерфейса
			 * @return    результат установки MTU сетевого интерфейса
			 *
			 */
			virtual bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
		public:
			/**
			 * @brief Метод получения режима трансляции пакетов сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
			 *
			 */
			virtual event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
			/**
			 * @brief Метод установки режима трансляции пакетов сервера или клиента
			 *
			 * @param eid      идентификатор события сервера или клиента
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 *
			 */
			virtual bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @return       размер буфера сервера или клиента
			 *
			 */
			virtual size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @param size   размер буфера сервера или клиента
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения времени жизни DNS запроса
			 *
			 * @return время жизни DNS запроса в миллисекундах
			 *
			 */
			virtual uint32_t getAliveDNS() const noexcept;
			/**
			 * @brief Метод установки времени жизни DNS запроса
			 *
			 * @param alive время жизни DNS запроса в миллисекундах
			 *
			 */
			virtual void setAliveDNS(const uint32_t alive) noexcept;
		public:
			/**
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @return режим использования таймаута на чтение события
			 *
			 */
			virtual event::usage_t getUsageReadTimeout() const noexcept;
			/**
			 * @brief Метод получения режима использования таймаута на чтение события сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    режим использования таймаута на чтение события сервера или клиента
			 *
			 */
			virtual event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
		public:
			/**
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 *
			 */
			virtual void setUsageReadTimeout(const event::usage_t usage) noexcept;
			/**
			 * @brief Метод установки режима использования таймаута на чтение события сервера или клиента
			 *
			 * @param eid   идентификатор события сервера или клиента
			 * @param usage режим использования таймаута на чтение события сервера или клиента (reusable или disposable)
			 *
			 */
			virtual void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
		public:
			/**
			 * @brief Метод получения таймаута сервера
			 *
			 * @param action тип действия сервера
			 * @return       значение таймаута в миллисекундах
			 *
			 */
			virtual uint32_t getTimeout(const event::action_t action) const noexcept;
			/**
			 * @brief Метод получения таймаута сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @return       значение таймаута в миллисекундах
			 *
			 */
			virtual uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
		public:
			/**
			 * @brief Метод установки таймаута сервера
			 *
			 * @param action  тип действия сервера
			 * @param timeout значение таймаута в миллисекундах
			 *
			 */
			virtual void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
			/**
			 * @brief Метод установки таймаута сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param action  тип действия сервера или клиента
			 * @param timeout значение таймаута в миллисекундах
			 *
			 */
			virtual void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
		public:
			/**
			 * @brief Метод установки пропускной способности сервера
			 *
			 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
			 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
			 */
			virtual bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			/**
			 * @brief Метод установки пропускной способности сервера или клиента
			 *
			 * @param eid       идентификатор события сервера или клиента
			 * @param limiting  режим ограничения пропускной способности сервера или клиента (egress или ingress)
			 * @param bandwidth пропускная способность сервера или клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
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
			 *
			 */
			virtual bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
		public:
			/**
			 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @return значение DSCP
			 *
			 */
			virtual event::dscp_t getDifferentiatedServicesCodePoint() const noexcept;
			/**
			 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @param dscp значение DSCP
			 * @return     результат работы функции
			 *
			 */
			virtual bool setDifferentiatedServicesCodePoint(const event::dscp_t dscp) const noexcept;
		public:
			/**
			 * @brief Метод получения обнаружения максимального размера пакета (MTU)
			 *
			 * @return режим обнаружения максимального размера пакета (MTU)
			 *
			 */
			virtual event::mtu_discover_t getMaximumTransmissionUnitDiscover() const noexcept;
			/**
			 * @brief Метод установки обнаружения максимального размера пакета (MTU)
			 *
			 * @param mode режим обнаружения максимального размера пакета (MTU)
			 * @return     результат работы функции
			 *
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
			 *
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
			 *
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
			 *
			 */
			virtual event::id_t init(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * @brief Метод установки названия кластера
			 *
			 * @param name название кластера для установки
			 *
			 */
			virtual void clusterName(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения семейства кластера
			 *
			 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
			 *
			 */
			virtual unit::cluster_t::family_t clusterFamily() const noexcept;
		public:
			/**
			 * @brief Метод получения режима активации кластера
			 *
			 * @return режим активации кластера
			 *
			 */
			virtual event::mode_t clusterMode() const noexcept;
			/**
			 * @brief Метод установки количества процессов кластера
			 *
			 * @param mode флаг активации/деактивации кластера
			 * @param size количество рабочих процессов
			 *
			 */
			virtual void clusterMode(const event::mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод получения максимального количества процессов
			 *
			 * @return максимальное количество процессов
			 *
			 */
			virtual uint16_t clusterCount() const noexcept;
			/**
			 * @brief Метод установки максимального количества процессов
			 *
			 * @param count максимальное количество процессов
			 *
			 */
			virtual void clusterCount(const uint16_t count) noexcept;
		public:
			/**
			 * @brief Метод получения списка дочерних процессов
			 *
			 * @return список дочерних процессов
			 *
			 */
			virtual unordered_set <pid_t> clusterWorkers() const noexcept;
		public:
			/**
			 * @brief Метод установки диапазона портов для выделения дочерним процессам кластера
			 *
			 * @note Применяется только к транспорту QUIC: родительский процесс раздаёт
			 *       порты из диапазона дочерним процессам, каждый из которых поднимает
			 *       собственный сокет сервера. На Linux/FreeBSD порт может повторяться
			 *       (процессы делят его через SO_REUSEPORT), на прочих системах порт
			 *       выделяется дочернему процессу монопольно. Пустой диапазон означает
			 *       использование единственного порта прослушивания
			 *
			 * @param begin начальный порт диапазона (0 - использовать порт прослушивания)
			 * @param end   конечный порт диапазона (0 - использовать порт прослушивания)
			 *
			 */
			virtual void clusterRange(const uint16_t begin, const uint16_t end) noexcept;
		public:
			/**
			 * @brief Метод получения списка дочерних процессов, не получивших порт прослушивания
			 *
			 * @note Актуально только для транспорта QUIC на системах, где порт выделяется
			 *       дочернему процессу монопольно (macOS/Solaris/OpenBSD/NetBSD): при нехватке
			 *       портов диапазона часть дочерних процессов остаётся без сокета сервера.
			 *       Их порт можно доотправить вручную методом clusterAssign()
			 *
			 * @return список идентификаторов дочерних процессов, работающих в холостую
			 *
			 */
			virtual unordered_set <pid_t> clusterIdle() const noexcept;
			/**
			 * @brief Метод отправки порта прослушивания конкретному дочернему процессу кластера
			 *
			 * @note Применяется только к транспорту QUIC: позволяет вручную поднять сервер
			 *       на дочернем процессе, которому при автоматической раздаче порт не достался
			 *
			 * @param pid  идентификатор дочернего процесса
			 * @param port порт прослушивания для дочернего процесса
			 * @return     результат отправки порта дочернему процессу
			 *
			 */
			virtual bool clusterAssign(const pid_t pid, const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения родительскому процессу
			 *
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 * @return       количество байт отправленного сообщения
			 *
			 */
			virtual size_t clusterSend(const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод отправки сообщения дочернему процессу
			 *
			 * @param pid    идентификатор процесса для получения сообщения
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 * @return       количество байт отправленного сообщения
			 *
			 */
			virtual size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения всем дочерним процессам
			 *
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 * @return       количество байт отправленного сообщения
			 *
			 */
			virtual size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки флага автоматического возрождения процессов
			 *
			 * @param mode флаг возрождения процессов
			 *
			 */
			virtual void clusterRebirth(const bool mode) noexcept;
			/**
			 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
			 *
			 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
			 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
			 *
			 */
			virtual void clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
		public:
			/**
			 * @brief Метод получения типа протокола передачи данных между воркерами
			 *
			 * @return тип протокола передачи данных между воркерами
			 *
			 */
			virtual event::type_t clusterGetTypeEventMessage() const noexcept;
			/**
			 * @brief Метод установки типа протокола передачи данных между воркерами
			 *
			 * @param type тип протокола передачи данных между воркерами для установки
			 *
			 */
			virtual void clusterSetTypeEventMessage(const event::type_t type) noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера события
			 *
			 * @param pid    идентификатор процесса
			 * @param action тип действия события
			 * @return       размер буфера события
			 *
			 */
			virtual size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param pid    идентификатор процесса
			 * @param action тип действия события
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			auto on(const char * name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			auto on(string_view name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			auto on(const string & name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
				// Если мы получили идентификатор функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			auto on(const A fid, Args... args) noexcept -> uint32_t {
				// Если мы получили на вход число
				if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
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
			 *
			 */
			Server & operator = (const Server &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Server(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param dns объект DNS-резолвера
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Server(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param cts   идентификатор шаблона контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 */
			explicit Server(const tls::coder_t::id_t cts, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param cts   идентификатор шаблона контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param dns   объект DNS-резолвера
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 */
			explicit Server(const tls::coder_t::id_t cts, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Server() noexcept;
	} server_t;
};

#endif // __AWH_SERVER__
