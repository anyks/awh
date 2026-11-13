/**
 * @file client.hpp
 * @date 2026-11-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модулей QUIC — классы unit::QuicServer и unit::QuicClient,
 *        связывающие конечный автомат соединения QUIC с движком ввода-вывода: управление сессиями,
 *        миграция клиентского адреса, кластеризация сервера и возобновление сессии по 0-RTT
 *
 * \~english
 * @brief Header file of the QUIC modules — the unit::QuicServer and unit::QuicClient classes,
 *        which link the state machine of a QUIC connection with the input-output engine: the management of the sessions,
 *        the migration of the client address, the clustering of the server and the resumption of a session by 0-RTT
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../unit.hpp"
#include "../../proto/quic/connection.hpp"
#include "../../cryptography/tls/coder.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс модуля клиента транспортного протокола QUIC
		 *
		 * @details Ведёт одно соединение с удалённым сервером поверх асинхронного
		 *          сетевого движка. Билет возобновления, присланный сервером после
		 *          установления соединения, сохраняется модулем и подставляется
		 *          при следующем подключении, поэтому повторное соединение с тем
		 *          же сервером обходится без полного хендшейка (RFC 9001 §4.6).
		 *
		 * \~english
		 * @brief Class of the client module of the QUIC transport protocol
		 *
		 * @details Conducts a single connection with a remote server on top of the asynchronous
		 *          network engine. The resumption ticket sent by the server after
		 *          the establishment of the connection is preserved by the module and is substituted
		 *          at the next connection, therefore a repeated connection with the same
		 *          server makes do without a full handshake (RFC 9001 §4.6).
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ QuicClient : public unit_t {
			private:
				// Флаг уведомления о перегрузке пути (RFC 9000 §13.4)
				bool _ecn;
				// Флаг выдачи буфера исходящей датаграммы сетевому движку
				bool _handed;
				// Флаг выполненного оповещения приложения о завершённом соединении
				bool _notified;
				// Флаг оповещения приложения об установленном соединении
				bool _connected;
			private:
				// Идентификатор события клиента
				event::id_t _eid;
				// Идентификатор события интервала таймеров соединения
				event::id_t _tid;
				/**
				 * \~russian
				 * @brief Идентификатор события-приёмника объединения данных (splice): собранные
				 *        данные потоков соединения перенаправляются в это событие вместо выдачи
				 *        приложению (0 - объединение не установлено)
				 *
				 * \~english
				 * @brief Identifier of an event-receiver of a merging of the data (splice): the assembled
				 *        data of the streams of a connection are redirected into this event instead of issuing
				 *        to an application (0 - a merging is not established)
				 *
				 * \~
				 */
				event::id_t _dest;
			private:
				/**
				 * \~russian
				 * @brief Идентификатор туннельного потока для входящих объединённых данных:
				 *        байты, поступающие в соединение из события-источника, отправляются
				 *        этим потоком приложения (INVALID_STREAM - поток ещё не открыт)
				 *
				 * \~english
				 * @brief Identifier of a tunnel stream for the incoming merged data: the
				 *        bytes arriving into a connection from an event-source are sent
				 *        by this stream of the application (INVALID_STREAM - a stream is not yet opened)
				 *
				 * \~
				 */
				uint64_t _tunnel;
			private:
				// Порт удалённого сервера (устанавливается фасадом до подключения)
				uint16_t _targetPort;
			private:
				// Маркировка, установленная на сокете события клиента
				event::ecn_t _marking;
				// Семейство адресов события клиента
				event::family_t _family;
			private:
				// Идентификатор шаблона контекста безопасности
				tls::coder::id_t _ctx;
			private:
				// Локальные транспортные параметры соединения
				quic::params::params_t _params;
			private:
				/**
				 * \~russian
				 * @brief Сохранённый токен проверки адреса фрейма NEW_TOKEN. Подставляется
				 *        в первый пакет следующего соединения и позволяет пропустить обмен
				 *        пакетом Retry, сэкономив круг задержки (RFC 9000 §8.1.3)
				 *
				 * \~english
				 * @brief A kept token of an address validation of a NEW_TOKEN frame. It is put
				 *        into a first packet of the next connection and allows to skip an exchange
				 *        by a Retry packet, sparing a round of a delay (RFC 9000 §8.1.3)
				 *
				 * \~
				 */
				string _token;
				/**
				 * \~russian
				 * @brief Буфер исходящей датаграммы, выданной сетевому движку вытягивающей
				 *        моделью: движок принимает выданное целиком, но указатель обязан
				 *        оставаться действительным до возврата из функции-источника, поэтому
				 *        буфер переиспользуется и очищается только на следующем вызове
				 *
				 * \~english
				 * @brief A buffer of an outgoing datagram, issued to a network engine by a pulling
				 *        model: an engine accepts the issued wholly, but a pointer is obliged
				 *        to stay valid up to a return from a function-source, therefore
				 *        a buffer is reused and cleared only on the next call
				 *
				 * \~
				 */
				string _datagram;
			private:
				// Структура сетевого адреса удалённого сервера (устанавливается фасадом до подключения)
				unique_ptr <net::addr_t> _target;
				// Объект соединения QUIC
				unique_ptr <quic::connection_t> _connection;
			private:
				/**
				 * \~russian
				 * @brief Метод отправки готовых исходящих датаграмм соединения
				 *
				 * \~english
				 * @brief Method of sending the ready outgoing datagrams of a connection
				 *
				 * \~
				 */
				void flush() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выдачи собранных данных потоков приложения
				 *
				 * \~english
				 * @brief Method of issuing the assembled data of the application streams
				 *
				 * \~
				 */
				void process() noexcept;
				/**
				 * \~russian
				 * @brief Метод оповещения приложения о завершённом соединении
				 *
				 * @note Соединение завершается как удалённым эндпоинтом, так и по
				 *       истечении периода завершения, выдерживаемого после завершения
				 *       соединения самим приложением (RFC 9000 §10.2). Оповещение
				 *       выполняется однократно
				 *
				 * \~english
				 * @brief Method of notifying the application about a terminated connection
				 *
				 * @note A connection is terminated both by the remote endpoint and by
				 *       the expiration of the closing period held after the termination of the
				 *       connection by the application itself (RFC 9000 §10.2). The notification
				 *       is performed once
				 *
				 * \~
				 */
				void complete() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод перенаправления собранных данных соединения в событие-приёмник объединения
				 *
				 * @param data данные для перенаправления
				 *
				 * \~english
				 * @brief Method of redirecting the assembled data of a connection into the destination event of the joining
				 *
				 * @param data data to be redirected
				 *
				 * \~
				 */
				void forward(string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения маркировки соединения к сокету события клиента
				 *
				 * @param marking требуемая маркировка исходящих датаграмм
				 *
				 * \~english
				 * @brief Method of applying the marking of a connection to the socket of the client event
				 *
				 * @param marking required marking of the outgoing datagrams
				 *
				 * \~
				 */
				void mark(const event::ecn_t marking) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки актуальности события клиента
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 *
				 * \~english
				 * @brief Method of checking the relevance of a client event
				 *
				 * @param eid event identifier
				 * @return    result of checking the relevance of the event
				 *
				 * \~
				 */
				bool isActual(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска/остановки работы клиента
				 *
				 * @param status статус запуска/остановки клиента
				 *
				 * \~english
				 * @brief Method of launching/stopping the work of the client
				 *
				 * @param status status of the launch/stop of the client
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки завершения подключения к серверу
				 *
				 * @param eid идентификатор события клиента
				 * @param ok  результат подключения к серверу
				 *
				 * \~english
				 * @brief Method of processing the completion of the connection to the server
				 *
				 * @param eid client event identifier
				 * @param ok  result of the connection to the server
				 *
				 * \~
				 */
				void connected(const event::id_t eid, const bool ok) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки просроченных таймеров соединения
				 *
				 * @param eid    идентификатор события интервала
				 * @param status статус события интервала
				 *
				 * \~english
				 * @brief Method of processing the expired timers of a connection
				 *
				 * @param eid    event identifier of the interval
				 * @param status status of the interval event
				 *
				 * \~
				 */
				void tick(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отправки объединённых данных в туннельный поток соединения
				 *
				 * @note Байты, поступившие из события-источника объединения (splice),
				 *       отправляются туннельным потоком соединения с их шифрованием
				 *       на уровне соединения (RFC 9000 §2.1)
				 *
				 * @param data данные для отправки в туннельный поток
				 * @param size размер данных для отправки
				 * @return     результат постановки данных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending the joined data into the tunnel stream of a connection
				 *
				 * @note The bytes that have arrived from the source event of the joining (splice)
				 *       are sent by the tunnel stream of the connection with their encryption
				 *       at the level of the connection (RFC 9000 §2.1)
				 *
				 * @param data data to be sent into the tunnel stream
				 * @param size size of the data to be sent
				 * @return     result of placing the data into the sending queue
				 *
				 * \~
				 */
				bool inject(const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки принятой датаграммы соединения
				 *
				 * @param eid  идентификатор события клиента
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 *
				 * \~english
				 * @brief Method of processing an accepted datagram of a connection
				 *
				 * @param eid  client event identifier
				 * @param data data of the datagram
				 * @param size size of the datagram
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи исходящей датаграммы соединения сетевому движку
				 *
				 * @note Функция-источник вытягивающей модели: движок спрашивает данные ровно
				 *       тогда, когда готов их отправить, поэтому маркировка ECN накладывается
				 *       здесь же и соответствует выдаваемой датаграмме
				 *
				 * @note Датаграмма отдаётся целиком либо не отдаётся вовсе: если выданной
				 *       ёмкости не хватает, она удерживается до следующего запроса
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer адрес указателя на буфер выдаваемых данных
				 * @param size   ёмкость запроса на входе и размер выданных данных на выходе
				 * @return       признак продолжения вытягивания данных
				 *
				 * \~english
				 * @brief Method of giving out an outgoing datagram of a connection to the network engine
				 *
				 * @note Source function of the pull model: the engine asks for the data exactly
				 *       when it is ready to send them, so the ECN marking is imposed here as well
				 *       and corresponds to the given out datagram
				 *
				 * @note A datagram is given out entirely or not given out at all: if the given
				 *       capacity is not enough, it is held until the next request
				 *
				 * @param eid    event identifier of the client
				 * @param buffer address of the pointer to the buffer of the given out data
				 * @param size   capacity of the request at the input and size of the given out data at the output
				 * @return       flag of the continuation of the pulling of the data
				 *
				 * \~
				 */
				bool source(const event::id_t eid, const uint8_t ** buffer, size_t & size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки принятия ранних данных удалённым сервером (RFC 9001 §4.6.2)
				 *
				 * @note Сервер вправе отказать в ранних данных, и тогда отправленное
				 *       ими содержимое передаётся заново после хендшейка. Проверка
				 *       нужна приложению, которому важно знать, состоялся ли 0-RTT
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the acceptance of the early data by the remote server (RFC 9001 §4.6.2)
				 *
				 * @note The server has the right to refuse the early data, and then the content sent
				 *       by it is transmitted anew after the handshake. The check
				 *       is needed by an application to which it is important to know whether the 0-RTT has taken place
				 *
				 * @return result of the check
				 *
				 * \~
				 */
				bool early() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
				 *
				 * @note Исходящие датаграммы помечаются поддержкой ECN, а маркировка
				 *       принятых извлекается из заголовка IP-пакета. Путь, стирающий
				 *       маркировку, соединение выявляет само и маркировку снимает
				 *
				 * @param mode режим уведомления о перегрузке пути
				 *
				 * \~english
				 * @brief Method of setting the notification about the congestion of the path (RFC 9000 §13.4)
				 *
				 * @note The outgoing datagrams are marked with the support of ECN, while the marking of the
				 *       accepted ones is extracted from the header of the IP packet. A path that erases
				 *       the marking is revealed by the connection itself, and it removes the marking
				 *
				 * @param mode mode of the notification about the congestion of the path
				 *
				 * \~
				 */
				void ecn(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения сохранённого токена проверки адреса (RFC 9000 §8.1.3)
				 *
				 * @note Токен присылается сервером по завершении соединения и сохраняется
				 *       модулем самостоятельно, переживая соединение и пригодный
				 *       к предъявлению между запусками приложения
				 *
				 * @return токен проверки адреса (пусто - токен не получен)
				 *
				 * \~english
				 * @brief Method of extracting the preserved address verification token (RFC 9000 §8.1.3)
				 *
				 * @note The token is sent by the server upon the completion of the connection and is preserved
				 *       by the module by itself, outliving the connection and being suitable
				 *       for a presentation between the runs of the application
				 *
				 * @return address verification token (empty — the token has not been received)
				 *
				 * \~
				 */
				const string & token() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сохранённого токена проверки адреса (RFC 9000 §8.1.3)
				 *
				 * @param token токен проверки адреса
				 *
				 * \~english
				 * @brief Method of setting the preserved address verification token (RFC 9000 §8.1.3)
				 *
				 * @param token address verification token
				 *
				 * \~
				 */
				void token(string_view token) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the settings of a client event
				 *
				 * @param eid client event identifier
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения запуска
				 *
				 * \~english
				 * @brief Method of launching the work of a client event
				 *
				 * @param eid client event identifier
				 * @return    result of performing the launch
				 *
				 * \~
				 */
				bool launch(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a client event
				 *
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод приостановки работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 *
				 * \~english
				 * @brief Method of suspending the work of a client event
				 *
				 * @param eid client event identifier
				 * @return    result of performing the suspension of the work
				 *
				 * \~
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 *
				 * \~english
				 * @brief Method of resuming the work of a client event
				 *
				 * @param eid client event identifier
				 * @return    result of performing the resumption of the work
				 *
				 * \~
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отключения клиента от удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения отключения
				 *
				 * \~english
				 * @brief Method of disconnecting the client from the remote server
				 *
				 * @param eid client event identifier
				 * @return    result of performing the disconnection
				 *
				 * \~
				 */
				bool disconnect(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод подключения клиента к удалённому серверу
				 *
				 * @note Создаёт соединение QUIC на шаблоне контекста безопасности и
				 *       начинает хендшейк (RFC 9001). Адрес и порт удалённого сервера
				 *       берутся из ранее установленной штатной структуры сетевого адреса
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of connecting the client to a remote server
				 *
				 * @note Creates a QUIC connection on the template of the security context and
				 *       begins the handshake (RFC 9001). The address and the port of the remote server
				 *       are taken from the previously set regular structure of the network address
				 *
				 * @param eid client event identifier
				 * @return    result of performing the connection
				 *
				 * \~
				 */
				bool connect(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод подключения клиента к удалённому серверу
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of connecting the client to a remote server
				 *
				 * @param ids list of the event identifiers to be connected
				 * @return    result of performing the connection
				 *
				 * \~
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки шаблона контекста безопасности соединения
				 *
				 * @param ctx идентификатор шаблона контекста безопасности
				 *
				 * \~english
				 * @brief Method of setting the template of the security context of the connection
				 *
				 * @param ctx identifier of the template of the security context
				 *
				 * \~
				 */
				void context(const tls::coder::id_t ctx) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки локальных транспортных параметров соединения (RFC 9000 §7.4)
				 *
				 * @param params локальные транспортные параметры
				 *
				 * \~english
				 * @brief Method of setting the local transport parameters of the connection (RFC 9000 §7.4)
				 *
				 * @param params local transport parameters
				 *
				 * \~
				 */
				void params(const quic::params::params_t & params) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    сетевой интерфейс клиента
				 *
				 * \~english
				 * @brief Method of getting the network interface of the client
				 *
				 * @param eid client event identifier
				 * @return    network interface of the client
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса клиента
				 *
				 * @param eid  идентификатор события клиента
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of the client
				 *
				 * @param eid  client event identifier
				 * @param name name of the network interface to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения внутреннего порта события
				 *
				 * @param eid идентификатор события
				 * @return    внутренний порт события
				 *
				 * \~english
				 * @brief Method of getting the internal port of the event
				 *
				 * @param eid event identifier
				 * @return    internal port of the event
				 *
				 * \~
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки внутреннего порта события
				 *
				 * @param eid  идентификатор события
				 * @param port внутренний порт события
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the internal port of the event
				 *
				 * @param eid  event identifier
				 * @param port internal port of the event
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setSourcePort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения порта удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    порт удалённого сервера
				 *
				 * \~english
				 * @brief Method of getting the port of the remote server
				 *
				 * @param eid client event identifier
				 * @return    port of the remote server
				 *
				 * \~
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта удалённого сервера
				 *
				 * @param eid  идентификатор события клиента
				 * @param port порт удалённого сервера для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the port of the remote server
				 *
				 * @param eid  client event identifier
				 * @param port port of the remote server to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setTargetPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 *
				 * @param eid client event identifier
				 * @return    host address of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 *
				 * @param eid    client event identifier
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target структура сетевого адреса хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 *
				 * @param eid    client event identifier
				 * @param target structure of the network address of the host of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 *
				 * @param eid    client event identifier
				 * @param target object for extracting the host address of the target machine
				 * @return       result of extracting the host address of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @note Для транспорта QUIC объединение на уровне сокета не поддерживается:
				 *       данные шифруются на уровне соединения, поэтому метод всегда
				 *       возвращает отрицательный результат
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 *
				 * \~english
				 * @brief Method of joining the data streams between two events
				 *
				 * @note For the QUIC transport the joining at the level of the socket is not supported:
				 *       the data is encrypted at the level of the connection, therefore the method always
				 *       returns a negative result
				 *
				 * @param eid  identifier of the source event
				 * @param dest identifier of the destination event
				 * @return     result of the joining
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 *
				 * \~english
				 * @brief Method of getting the metadata of the last received datagram packet
				 *
				 * @param eid client event identifier
				 * @return    metadata of the last received datagram packet
				 *
				 * \~
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции события клиента
				 *
				 * \~english
				 * @brief Method of getting the options of a client event
				 *
				 * @param eid client event identifier
				 * @return    options of the client event
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций события клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции события клиента для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the options of a client event
				 *
				 * @param eid     client event identifier
				 * @param options options of the client event to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции события клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция события клиента для установки
				 * @param mode   режим установки опции события клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting an option of a client event
				 *
				 * @param eid    client event identifier
				 * @param option option of the client event to be set
				 * @param mode   mode of setting the option of the client event
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события клиента
				 * @return    максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops through which a packet can pass
				 *
				 * @param eid client event identifier
				 * @return    maximum number of the hops
				 *
				 * \~
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops through which a packet can pass
				 *
				 * @param eid  client event identifier
				 * @param hops maximum number of the hops
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    количество хопов последнего принятого пакета
				 *
				 * \~english
				 * @brief Method of getting the number of the hops of the last received packet
				 *
				 * @param eid client event identifier
				 * @return    number of the hops of the last received packet
				 *
				 * \~
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the number of the hops of the last received packet
				 *
				 * @param eid  client event identifier
				 * @param hops number of the hops of the last received packet
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения данных от удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 *
				 * \~english
				 * @brief Method of receiving data from the remote server
				 *
				 * @param eid client event identifier
				 * @return    result of receiving the data
				 *
				 * \~
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных в поток приложения соединения
				 *
				 * @param sid  идентификатор потока приложения
				 * @param data отправляемые данные
				 * @param fin  флаг завершения потока
				 * @return     результат постановки данных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending data into an application stream of the connection
				 *
				 * @param sid  identifier of the application stream
				 * @param data data being sent
				 * @param fin  flag of the termination of the stream
				 * @return     result of placing the data into the sending queue
				 *
				 * \~
				 */
				size_t send(const uint64_t sid, string_view data, const bool fin = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных удалённому серверу
				 *
				 * @note Отправляет прикладные данные потоком по умолчанию через открытие
				 *       двунаправленного потока (RFC 9000 §2.1); для явного выбора потока
				 *       используется перегрузка send(sid, data, fin)
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, поставленных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending data to the remote server
				 *
				 * @note Sends the application data by the default stream through the opening
				 *       of a bidirectional stream (RFC 9000 §2.1); for an explicit choice of the stream
				 *       the send(sid, data, fin) overload is used
				 *
				 * @param eid    client event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of bytes placed into the sending queue
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки водяных меток буфера отправки потоков соединения (backpressure)
				 *
				 * @note Верхняя метка ограничивает объём несобранных данных на поток: сверх неё send()
				 *       принимает данные лишь частично, а по опустошению буфера ниже нижней метки поток
				 *       сигнализируется колбэком "writable". Ноль снимает ограничение (буфер не ограничен)
				 *
				 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
				 * @param low  нижняя водяная метка (порог сигнала "writable")
				 *
				 * \~english
				 * @brief Method of setting the watermarks of the sending buffer of the streams of the connection (backpressure)
				 *
				 * @note The upper mark limits the volume of the unassembled data per stream: beyond it send()
				 *       accepts the data only partially, while upon the emptying of the buffer below the lower mark the stream
				 *       is signalled by the "writable" callback. Zero removes the limit (the buffer is not limited)
				 *
				 * @param high upper watermark (capacity of the sending buffer of the stream)
				 * @param low  lower watermark (threshold of the "writable" signal)
				 *
				 * \~
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных потока (RFC 9000 §2.2)
				 *
				 * @note Альтернатива send() для больших тел: движок сам запрашивает данные у источника
				 *       по мере места в буфере отправки потока, не требуя держать копию всего тела
				 *
				 * @param sid    идентификатор потока приложения
				 * @param source pull-источник данных тела потока
				 *
				 * \~english
				 * @brief Method of assigning a pull source of the data of a stream (RFC 9000 §2.2)
				 *
				 * @note An alternative to send() for large bodies: the engine itself requests the data from the source
				 *       as the space in the sending buffer of the stream appears, without requiring to keep a copy of the whole body
				 *
				 * @param sid    identifier of the application stream
				 * @param source pull source of the data of the body of the stream
				 *
				 * \~
				 */
				void dataSource(const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @return        значение адреса клиента
				 *
				 * \~english
				 * @brief Method of getting the client address
				 *
				 * @param eid     client event identifier
				 * @param address client address type
				 * @return        value of the client address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client address
				 *
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   value of the client address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   структура сетевого адреса клиента
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client address
				 *
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   structure of the network address of the client
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   объект для извлечения адреса клиента
				 * @return        результат выполнения извлечения адреса клиента
				 *
				 * \~english
				 * @brief Method of getting the client address
				 *
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   object for extracting the client address
				 * @return        result of extracting the client address
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима трансляции пакетов клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 * \~english
				 * @brief Method of getting the packet delivery mode of the client
				 *
				 * @param eid client event identifier
				 * @return    packet delivery mode (unicast, multicast, broadcast)
				 *
				 * \~
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима трансляции пакетов клиента
				 *
				 * @param eid      идентификатор события клиента
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the packet delivery mode of the client
				 *
				 * @param eid      client event identifier
				 * @param delivery packet delivery mode (unicast, multicast, broadcast)
				 * @return         result of performing the setting
				 *
				 * \~
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 *
				 * \~english
				 * @brief Method of getting the buffer size of the client
				 *
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @return       buffer size of the client
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the buffer size of the client
				 *
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @param size   buffer size of the client
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 *
				 * \~english
				 * @brief Method of getting the usage mode of the read timeout of the event
				 *
				 * @param eid event identifier
				 * @return    usage mode of the read timeout of the event
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 *
				 * \~english
				 * @brief Method of setting the usage mode of the read timeout of the event
				 *
				 * @param eid   event identifier
				 * @param usage usage mode of the read timeout of the event (reusable or disposable)
				 *
				 * \~
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of the client
				 *
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @return       value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the timeout of the client
				 *
				 * @param eid     client event identifier
				 * @param action  action type of the client
				 * @param timeout value of the timeout in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the client
				 *
				 * @param eid       client event identifier
				 * @param limiting  mode of limiting the bandwidth of the client (egress or ingress)
				 * @param bandwidth bandwidth of the client to be set
				 * @return          result of performing the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров keep-alive для клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the keep-alive parameters for the client
				 *
				 * @param eid   client event identifier
				 * @param cnt   number of the keep-alive packets
				 * @param idle  idle time before sending the first keep-alive packet in seconds
				 * @param intvl interval between the keep-alive packets in seconds
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @return    MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of the network interface
				 *
				 * @param eid client event identifier
				 * @return    MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of the network interface
				 *
				 * @param eid client event identifier
				 * @param mtu MTU size of the interface
				 * @return    result of setting the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события клиента
				 * @return    текущий режим обнаружения MTU
				 *
				 * \~english
				 * @brief Method of getting the discovery mode of the maximum packet size (MTU)
				 *
				 * @param eid client event identifier
				 * @return    current MTU discovery mode
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события клиента
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery mode of the maximum packet size (MTU)
				 *
				 * @param eid  client event identifier
				 * @param mode discovery mode of the maximum packet size (MTU)
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 *
				 * @param eid client event identifier
				 * @return    DSCP value
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 *
				 * @param eid  client event identifier
				 * @param dscp DSCP value
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 *
				 * @param eid    client event identifier
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 *
				 * @param eid    client event identifier
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания события клиента QUIC поверх UDP
				 *
				 * @note Транспорт QUIC работает поверх дейтаграммного UDP-сокета: тип и
				 *       протокол принудительно приводятся к DATAGRAM/UDP независимо от
				 *       переданных значений (RFC 9000)
				 *
				 * @param family   семейство адресов события клиента
				 * @param type     тип события (игнорируется, приводится к DATAGRAM)
				 * @param protocol протокол события (игнорируется, приводится к UDP)
				 * @return         идентификатор созданного события клиента
				 *
				 * \~english
				 * @brief Method of creating a QUIC client event on top of UDP
				 *
				 * @note The QUIC transport works on top of a datagram UDP socket: the type and the
				 *       protocol are forcibly brought to DATAGRAM/UDP regardless of the
				 *       passed values (RFC 9000)
				 *
				 * @param family   address family of the client event
				 * @param type     event type (ignored, brought to DATAGRAM)
				 * @param protocol event protocol (ignored, brought to UDP)
				 * @return         identifier of the created client event
				 *
				 * \~
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод остановки клиента
				 *
				 * \~english
				 * @brief Method of stopping the client
				 *
				 * \~
				 */
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска клиента
				 *
				 * \~english
				 * @brief Method of launching the client
				 *
				 * \~
				 */
				void start() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @note Поддерживаются функции обратного вызова: "open" на установленное
				 *       соединение, "read" на собранные данные потока приложения,
				 *       "datagram" на принятую датаграмму приложения и "close"
				 *       на завершённое соединение
				 *
				 * @param callback функции обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 *
				 * @note The following callback functions are supported: "open" on an established
				 *       connection, "read" on the assembled data of an application stream,
				 *       "datagram" on an accepted application datagram and "close"
				 *       on a terminated connection
				 *
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения согласованного ALPN-протокола соединения
				 *
				 * @return согласованный ALPN-протокол
				 *
				 * \~english
				 * @brief Method of getting the negotiated ALPN protocol of the connection
				 *
				 * @return negotiated ALPN protocol
				 *
				 * \~
				 */
				tls::coder::alpn_t alpn() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
				 *
				 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
				 *
				 * \~english
				 * @brief Method of getting the limit size of a datagram being sent (RFC 9221 §3)
				 *
				 * @return limit size of the data of a datagram in octets (0 — the datagrams are not supported)
				 *
				 * \~
				 */
				size_t datagrams() const noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки датаграммы приложения серверу (RFC 9221)
				 *
				 * @note Доставка датаграмм ненадёжна: потерянная датаграмма повторно
				 *       не отправляется, порядок доставки не гарантируется. Отправка
				 *       возможна только когда сервер анонсировал их приём
				 *
				 * @param data данные датаграммы приложения
				 * @return     результат отправки
				 *
				 * \~english
				 * @brief Method of sending an application datagram to the server (RFC 9221)
				 *
				 * @note The delivery of the datagrams is unreliable: a lost datagram is not
				 *       sent again, the order of the delivery is not guaranteed. The sending
				 *       is possible only when the server has announced their reception
				 *
				 * @param data data of the application datagram
				 * @return     result of the sending
				 *
				 * \~
				 */
				bool datagram(string_view data) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод открытия потока приложения соединения
				 *
				 * @param mode режим однонаправленного потока
				 * @return     идентификатор открытого потока
				 *
				 * \~english
				 * @brief Method of opening an application stream of the connection
				 *
				 * @param mode mode of a unidirectional stream
				 * @return     identifier of the opened stream
				 *
				 * \~
				 */
				uint64_t open(const bool mode = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 *
				 * \~english
				 * @brief Method of the termination of the connection by the application (RFC 9000 §10.2)
				 *
				 * @param code   error code of the application
				 * @param reason human-readable reason of the termination
				 *
				 * \~
				 */
				void close(const uint64_t code = 0, string_view reason = "") noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				QuicClient(const QuicClient &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 *
				 * @return current value of the object
				 *
				 * \~
				 */
				QuicClient & operator = (const QuicClient &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit QuicClient() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~QuicClient() noexcept;
		} quic_client_t;
	};
};
