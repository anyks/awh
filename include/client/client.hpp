/**
 * @file: client.hpp
 * @date: 2026-04-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл фасада клиента — публичный API класса Client, объединяющего транспорт, TLS,
 *        DNS-резолвинг и подписку на события в единую точку входа для построения клиентских подключений по TCP, UDP,
 *        SCTP, UDS, DTLS и QUIC
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../units/dns.hpp"
#include "../units/quic.hpp"
#include "../units/client.hpp"
#include "../net/tls/coder.hpp"
#include "../proto/quic/connection.hpp"

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
	 * @brief Класс клиента
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Client {
		protected:
			/**
			 * @brief Структура для хранения параметров DNS-резолвера
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
			 */
			typedef struct __AWH_SHARED_EXPORT__ Identifier {
				// Идентификатор клиента
				event::id_t eid;
				// Контекст безопасности TLS
				tls::coder_t::id_t ctl;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Identifier() noexcept;
			} __attribute__((packed)) id_t;
			/**
			 * @brief Структура юнита клиента
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Unit {
				// Объект работы с сетевыми адресами
				net_addr_t addr;
				// Объект юнита клиента (транспорты TCP/UDP/SCTP и прикладные протоколы поверх них)
				unit::client_t client;
				// Объект юнита клиента QUIC (выбирается при инициализации транспортом protocol_t::QUIC)
				unit::quic_client_t quic;
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
			// Идентификатор клиента
			id_t _id;
		protected:
			// Объект DNS-резолвера
			dns_t _dns;
		protected:
			// Адрес хоста целевой машины
			string _host;
		protected:
			// Функция обратного вызова для обработки клиента
			callback_t _callback;
		protected:
			// Объект юнита клиента
			unique_ptr <unit_t> _unit;
		protected:
			// Объект транспортного уровня безопасности
			tls::coder_t * _coder;
		protected:
			// Протокол транспорта клиента (выбирается при инициализации, определяет обработку данных)
			event::protocol_t _protocol;
		protected:
			// Тип сокета транспорта клиента (STREAM/DATAGRAM/SEQPACKET - определяет доступность датаграмм)
			event::type_t _type;
		protected:
			// Идентификатор потока по умолчанию для отправки без явного sid (INVALID_STREAM - не открыт)
			uint64_t _stream;
		protected:
			/**
			 * Флаг завершения отправки данных потоковым транспортом: устанавливается
			 * при отправке с флагом fin, а по факту записи данных в сокет соединение
			 * завершается (для потоковых транспортов клиент всегда единственный)
			 */
			bool _fin;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * @brief Метод проверки рабочего состояния клиента
			 *
			 * @note Проверяет рабочее состояние DNS-резолвера либо активного юнита
			 *       транспорта, выбираемого по протоколу (QUIC - выделенный юнит,
			 *       остальные транспорты - общий юнит клиента)
			 *
			 * @return результат проверки рабочего состояния
			 *
			 */
			bool active() const noexcept;
		protected:
			/**
			 * @brief Методы диспетчеризации к активному юниту транспорта
			 *
			 * @note Транспорт выбирается по протоколу клиента: для protocol_t::QUIC
			 *       работает выделенный юнит клиента QUIC, для остальных транспортов -
			 *       общий юнит клиента. Событием во всех случаях выступает _id.eid
			 *
			 */
			bool commitUnit() noexcept;
			bool launchUnit() noexcept;
			void startUnit() noexcept;
			void stopUnit() noexcept;
			void destroyUnit() noexcept;
			bool pauseUnit() noexcept;
			bool resumeUnit() noexcept;
			bool connectUnit() noexcept;
			bool disconnectUnit() noexcept;
			bool recvUnit() noexcept;
			size_t sendUnit(const void * buffer, const size_t size) noexcept;
			event::family_t familyUnit() const noexcept;
			event::status_t statusUnit() const noexcept;
			string getTargetUnit() const noexcept;
			uint16_t getTargetPortUnit() const noexcept;
			bool setTargetUnit(string_view target) noexcept;
			bool setTargetUnit(const net::addr_t * target) noexcept;
		protected:
			/**
			 * @brief Метод изменения статуса клиента
			 *
			 * @param index  индекс очереди запускаемого события
			 * @param status новый статус клиента
			 *
			 */
			virtual void status(const uint8_t index, const event::status_t status) noexcept;
		protected:
			/**
			 * @brief Метод обработки событий подключения клиента к удалённому серверу
			 *
			 * @param    идентификатор клиента
			 * @param ok результат подключения
			 *
			 */
			virtual void connect(const event::id_t, const bool ok) noexcept;
			/**
			 * @brief Метод обработки событий записи данных клиентом
			 *
			 * @param      идентификатор клиента
			 * @param size размер данных для записи
			 *
			 */
			virtual void write(const event::id_t, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий изменения состояния клиента
			 *
			 * @param        идентификатор клиента
			 * @param status новый статус клиента
			 *
			 */
			virtual void state(const event::id_t, const event::status_t status) noexcept;
			/**
			 * @brief Метод обработки действий клиента
			 *
			 * @param        идентификатор клиента
			 * @param action действие клиента
			 *
			 */
			virtual void action(const event::id_t, const event::action_t action) noexcept;
			/**
			 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
			 *
			 * @param      идентификатор события
			 * @param info информационные метаданные о дейтаграммном пакете
			 *
			 */
			virtual void traffic(const event::id_t, const net::dgram_info_t & info) noexcept;
			/**
			 * @brief Метод обработки событий получения данных клиентом
			 *
			 * @param        идентификатор клиента
			 * @param buffer буфер данных клиента
			 * @param size   размер данных клиента
			 *
			 */
			virtual void read(const event::id_t, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод обработки собранных данных потока соединения QUIC
			 *
			 * @param      идентификатор события
			 * @param sid  идентификатор потока приложения
			 * @param data собранные данные потока
			 * @param fin  флаг завершения потока удалённым эндпоинтом
			 *
			 */
			virtual void stream(const event::id_t, const uint64_t sid, const string & data, const bool fin) noexcept;
			/**
			 * @brief Метод обработки принятой датаграммы приложения QUIC (RFC 9221)
			 *
			 * @param      идентификатор события
			 * @param data данные принятой датаграммы
			 *
			 */
			virtual void message(const event::id_t, const string & data) noexcept;
			/**
			 * @brief Метод обработки готовности к отправке ранних данных QUIC (RFC 9001 §4.6)
			 *
			 * @param идентификатор события
			 *
			 */
			virtual void earlyData(const event::id_t) noexcept;
			/**
			 * @brief Метод обработки завершения соединения QUIC (RFC 9000 §10)
			 *
			 * @param       идентификатор события
			 * @param error код ошибки завершения соединения
			 *
			 */
			virtual void closed(const event::id_t, const quic::error_t error) noexcept;
			/**
			 * @brief Метод обработки события ошибки
			 *
			 * @param         идентификатор события
			 * @param error   код ошибки
			 * @param message сообщение об ошибке
			 *
			 */
			virtual void error(const event::id_t, const event::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
			 *
			 * @param        идентификатор клиента
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 *
			 */
			virtual void available(const event::id_t, const event::status_t status, const size_t size) noexcept;
			/**
			 * @brief Метод обработки событий истечения таймаута клиента
			 *
			 * @param        идентификатор клиента
			 * @param action тип действия для истекшего таймаута
			 * @param delay  задержка таймаута в миллисекундах
			 * @return       нужно ли завершить клиента после истечения таймаута
			 *
			 */
			virtual bool timeout(const event::id_t, const event::action_t action, const uint32_t delay) noexcept;
			/**
			 * @brief Метод обработки попыток подключения клиента к удалённому серверу
			 *
			 * @param          идентификатор DNS-запроса
			 * @param domain   доменное имя для резолвинга
			 * @param attempts количество попыток подключения
			 *
			 */
			virtual void attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept;
			/**
			 * @brief Метод обработки неудачного резолвинга доменного имени
			 *
			 * @param        идентификатор DNS-запроса
			 * @param record тип записи DNS
			 * @param domain доменное имя
			 *
			 */
			virtual void failure(const unit::dns_t::id_t, const unit::dns_t::record_t record, const string & domain) noexcept;
			/**
			 * @brief Метод обработки события неотправленных данных клиента
			 *
			 * @param        идентификатор клиента
			 * @param error  тип ошибки отправки данных
			 * @param buffer данные, которые не удалось отправить
			 * @param size   размер данных, которые не удалось отправить
			 *
			 */
			virtual void spool(const event::id_t, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод резолвинга доменного имени в сетевой адрес
			 *
			 * @param        идентификатор DNS-запроса
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для резолвинга
			 * @param addr   указатель на структуру для хранения результата резолвинга
			 *
			 */
			virtual void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
		protected:
			/**
			 * @brief Метод получения состояния TLS
			 *
			 * @param       идентификатор TLS
			 * @param state состояние TLS
			 *
			 */
			virtual void stateTLS(const tls::coder_t::id_t, const tls::coder_t::state_t state) noexcept;
			/**
			 * @brief Метод получения ошибок TLS
			 *
			 * @param         идентификатор TLS
			 * @param error   код ошибки TLS
			 * @param message сообщение об ошибке TLS
			 *
			 */
			virtual void errorTLS(const tls::coder_t::id_t, const tls::coder_t::error_t error, const string & message) noexcept;
			/**
			 * @brief Метод получения событий шифрования/дешифрования данных TLS
			 *
			 * @param        идентификатор TLS
			 * @param event  тип события TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 * @param size   размер полезной нагрузки в буфере для события шифрования/дешифрования TLS
			 *
			 */
			virtual void processTLS(const tls::coder_t::id_t, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
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
			 *
			 */
			virtual bool pause() noexcept;
			/**
			 * @brief Метод возобновления работы клиента
			 *
			 * @return результат выполнения возобновления работы
			 *
			 */
			virtual bool resume() noexcept;
		public:
			/**
			 * @brief Метод уничтожения события клиента
			 *
			 */
			virtual void destroy() noexcept;
		public:
			/**
			 * @brief Метод проверки, жив ли клиент
			 *
			 * @return результат проверки
			 *
			 */
			virtual bool isAlive() const noexcept;
		public:
			/**
			 * @brief Метод подключения клиента к удалённому хосту
			 *
			 * @return результат выполнения подключения
			 *
			 */
			virtual bool connect() noexcept;
			/**
			 * @brief Метод отключения клиента от удалённого сервера
			 *
			 * @return результат выполнения отключения
			 *
			 */
			virtual bool disconnect() noexcept;
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
			 * @brief Метод получения данных от сервера
			 *
			 * @return результат получения данных
			 *
			 */
			virtual bool recv() noexcept;
			/**
			 * @brief Метод отправки данных серверу
			 *
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных серверу
			 *
			 */
			virtual size_t send(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки локальных транспортных параметров соединения QUIC (RFC 9000 §7.4)
			 *
			 * @param params локальные транспортные параметры
			 *
			 */
			virtual void params(const quic::params::params_t & params) noexcept;
			/**
			 * @brief Метод установки уведомления о перегрузке пути QUIC (RFC 9000 §13.4)
			 *
			 * @param mode режим уведомления о перегрузке пути
			 *
			 */
			virtual void ecn(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод извлечения сохранённого токена проверки адреса QUIC (RFC 9000 §8.1.3)
			 *
			 * @return токен проверки адреса (пусто - токен не получен)
			 *
			 */
			virtual const string & token() const noexcept;
			/**
			 * @brief Метод установки сохранённого токена проверки адреса QUIC (RFC 9000 §8.1.3)
			 *
			 * @param token токен проверки адреса
			 *
			 */
			virtual void token(string_view token) noexcept;
			/**
			 * @brief Метод проверки принятия ранних данных удалённым сервером QUIC (RFC 9001 §4.6.2)
			 *
			 * @return результат проверки
			 *
			 */
			virtual bool early() const noexcept;
		public:
			/**
			 * @brief Метод открытия потока данных соединения (QUIC-поток; для потоковых транспортов - единственный поток)
			 *
			 * @param mode режим однонаправленного потока
			 * @return     идентификатор открытого потока
			 *
			 */
			virtual uint64_t open(const bool mode = false) noexcept;
			/**
			 * @brief Метод отправки данных в поток соединения (QUIC/HTTP2-поток либо единственный поток потокового транспорта)
			 *
			 * @param sid    идентификатор потока
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @param fin    флаг завершения потока
			 * @return       количество байт данных, поставленных в очередь отправки
			 *
			 */
			virtual size_t send(const uint64_t sid, const void * buffer, const size_t size, const bool fin = false) noexcept;
			/**
			 * @brief Метод установки водяных меток буфера отправки потоков соединения QUIC (backpressure)
			 *
			 * @note Верхняя метка ограничивает несобранный буфер потока (send() принимает частично),
			 *       по опустошению ниже нижней метки поток сигнализируется колбэком "writable". Ноль -
			 *       ограничение снято. Для потоковых транспортов метод не действует
			 *
			 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
			 * @param low  нижняя водяная метка (порог сигнала "writable")
			 *
			 */
			virtual void sendWaterMarks(const size_t high, const size_t low) noexcept;
			/**
			 * @brief Метод назначения pull-источника данных потока соединения QUIC (RFC 9000 §2.2)
			 *
			 * @note Альтернатива send() для больших тел: движок сам тянет данные у источника по мере
			 *       места в буфере отправки, не требуя держать копию тела. Для потоковых транспортов не действует
			 *
			 * @param sid    идентификатор потока приложения
			 * @param source pull-источник данных тела потока
			 *
			 */
			virtual void dataSource(const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
			/**
			 * @brief Метод отправки датаграммы соединению (QUIC DATAGRAM по RFC 9221 либо дейтаграмма UDP)
			 *
			 * @param buffer буфер данных датаграммы для отправки
			 * @param size   размер данных датаграммы для отправки
			 * @return       результат отправки
			 *
			 */
			virtual bool datagram(const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод получения предельного размера отправляемой датаграммы QUIC (RFC 9221 §3)
			 *
			 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
			 *
			 */
			virtual size_t datagrams() const noexcept;
			/**
			 * @brief Метод завершения соединения (QUIC CONNECTION_CLOSE по RFC 9000 §10.2 либо уничтожение события для остальных транспортов)
			 *
			 * @param code   код ошибки приложения
			 * @param reason человекочитаемая причина завершения
			 *
			 */
			virtual void close(const uint64_t code = 0, string_view reason = "") noexcept;
		public:
			/**
			 * @brief Метод объединения данных между клиентом и другим событием
			 *
			 * @param eid    идентификатор события
			 * @param direct направление объединения данных (клиент -> событие, событие -> клиент)
			 * @return       результат выполнения объединения
			 *
			 */
			virtual bool splice(const event::id_t eid, const event::direct_t direct) noexcept;
		public:
			/**
			 * @brief Метод получения опций клиента
			 *
			 * @return опции клиента
			 *
			 */
			virtual uint16_t getOptions() const noexcept;
			/**
			 * @brief Метод установки опций клиента
			 *
			 * @param options опции клиента для установки
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setOptions(const uint16_t options) noexcept;
			/**
			 * @brief Метод установки опции клиента
			 *
			 * @param option опция клиента для установки
			 * @param mode   режим установки опции клиента
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setOption(const uint16_t option, const bool mode) noexcept;
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
			 * @return максимальное количество хопов
			 *
			 */
			virtual event::hops_t getHops() const noexcept;
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param hops максимальное количество хопов
			 * @return     результат работы функции
			 *
			 */
			virtual bool setHops(const event::hops_t hops) noexcept;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса клиента
			 *
			 * @return сетевой интерфейс клиента
			 *
			 */
			virtual string getIface() const noexcept;
			/**
			 * @brief Метод установки сетевого интерфейса клиента
			 *
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setIface(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения внутреннего порта события
			 *
			 * @return внутренний порт события
			 *
			 */
			virtual uint16_t getSourcePort() const noexcept;
			/**
			 * @brief Метод установки внутреннего порта события
			 *
			 * @param port внутренний порт события
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setSourcePort(const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод получения порта удаленного сервера
			 *
			 * @return порт удаленного сервера
			 *
			 */
			virtual uint16_t getTargetPort() const noexcept;
			/**
			 * @brief Метод установки порта удаленного сервера
			 *
			 * @param port порт удаленного сервера для установки
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setTargetPort(const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @return адрес хоста целевой машины
			 *
			 */
			virtual string getTarget() const noexcept;
			/**
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setTarget(string_view target) noexcept;
		public:
			/**
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setTarget(const net::addr_t * target) noexcept;
			/**
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @param target объект для извлечения адреса хоста целевой машины
			 * @return       результат выполнения извлечения адреса хоста целевой машины
			 *
			 */
			virtual bool getTarget(unique_ptr <net::addr_t> & target) const noexcept;
		public:
			/**
			 * @brief Метод получения адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @return        значение адреса клиента
			 *
			 */
			virtual string getAddress(const event::address_t address) const noexcept;
			/**
			 * @brief Метод установки адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   значение адреса клиента
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setAddress(const event::address_t address, string_view value) noexcept;
		public:
			/**
			 * @brief Метод установки адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   значение адреса клиента
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			/**
			 * @brief Метод получения адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   объект для извлечения адреса клиента
			 * @return        результат выполнения извлечения адреса клиента
			 *
			 */
			virtual bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * @brief Метод получения режима трансляции пакетов клиента
			 *
			 * @return режим трансляции пакетов (unicast, multicast, broadcast)
			 *
			 */
			virtual event::delivery_mode_t getDelivery() const noexcept;
			/**
			 * @brief Метод установки режима трансляции пакетов клиента
			 *
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 *
			 */
			virtual bool setDelivery(const event::delivery_mode_t delivery) noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера клиента
			 *
			 * @param action тип действия клиента
			 * @return       размер буфера клиента
			 *
			 */
			virtual size_t getBufferSize(const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера клиента
			 *
			 * @param action тип действия клиента
			 * @param size   размер буфера клиента
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setBufferSize(const event::action_t action, const size_t size) noexcept;
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
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 *
			 */
			virtual void setUsageReadTimeout(const event::usage_t usage) noexcept;
		public:
			/**
			 * @brief Метод получения таймаута клиента
			 *
			 * @param action тип действия клиента
			 * @return       значение таймаута в миллисекундах
			 *
			 */
			virtual uint32_t getTimeout(const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки таймаута клиента
			 *
			 * @param action  тип действия клиента
			 * @param timeout значение таймаута в миллисекундах
			 *
			 */
			virtual void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
		public:
			/**
			 * @brief Метод установки пропускной способности клиента
			 *
			 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
			 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
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
			 *
			 */
			virtual bool keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
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
			 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
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
			 * @brief Метод инициализации клиента
			 *
			 * @param family   семейство адресов
			 * @param type     тип события
			 * @param protocol протокол события
			 * @return         идентификатор созданного клиента
			 *
			 */
			virtual event::id_t init(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения функции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения функции обратного вызова
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
			Client(const Client &) = delete;
			/**
			 * @brief Оператор копирования (запрещаем)
			 *
			 * @return текущее значение объекта
			 *
			 */
			Client & operator = (const Client &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Client(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param dns объект DNS-резолвера
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Client(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param ctl   идентификатор контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 */
			explicit Client(const tls::coder_t::id_t ctl, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param ctl   идентификатор контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param dns   объект DNS-резолвера
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 */
			explicit Client(const tls::coder_t::id_t ctl, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Client() noexcept;
	} client_t;
};

#endif // __AWH_CLIENT__
