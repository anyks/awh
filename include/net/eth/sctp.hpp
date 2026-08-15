/**
 * @file sctp.hpp
 * @date 2026-01-28
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
 * @brief Заголовочный файл модуля управления протоколом SCTP —
 *        класс eth::Stream_Control_Transmission_Protocol для настройки параметров ассоциаций, потоков, heartbeat,
 *        авторизации и уведомлений SCTP-сокета
 *
 * \~english
 * @brief Header file of the module of the management of the SCTP protocol —
 *        the eth::Stream_Control_Transmission_Protocol class for setting the parameters of the associations, of the streams, of the heartbeat,
 *        of the authorization and of the notifications of an SCTP socket
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SCTP__
#define __AWH_SCTP__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * \~russian
 * @brief Предварительное объявление структуры метаданных сообщения SCTP
 *
 * @note Системный заголовок протокола здесь не подключается намеренно: заголовкам
 *       проекта положено быть чистыми, а ссылка на неполный тип объявления
 *       не требует. Объявление стоит в глобальном пространстве имён, ибо
 *       внутри пространства имён проекта родился бы иной, свой тип
 *
 * \~english
 * @brief Preliminary declaration of the structure of the metadata of an SCTP message
 * @note The system header of the protocol is not included here deliberately: the headers of
 *       the project are due to be clean, and a reference to an incomplete type does not require
 *       a declaration. The declaration stands in the global namespace, for
 *       inside the namespace of the project a different, its own type would be born
 *
 * \~
 */
struct sctp_sndrcvinfo;

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён Ethernet протоколов
	 *
	 * \~english
	 * @brief Namespace of the Ethernet protocols
	 *
	 * \~
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс управления протоколом передачи с управлением потоком
		 *
		 * @details Настройки протокола, сочетающего надёжность потокового обмена
		 * с сохранением границ сообщений. Ведает подпиской на его известия,
		 * пределами ожидания на каждом этапе и подтверждением подлинности
		 * частей сообщения
		 *
		 * @note Собирается лишь на Linux и FreeBSD - прочие системы протокола
		 * не несут
		 *
		 * \~english
		 * @brief Class of the management of the transmission protocol with the flow control
		 * @details The settings of the protocol combining the reliability of the stream exchange
		 * with the preservation of the boundaries of the messages. Is in charge of the subscription to its notices,
		 * of the limits of the waiting at every stage and of the confirmation of the authenticity of
		 * the parts of a message
		 * @note Is built only on Linux and FreeBSD — the other systems do not carry
		 * the protocol
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Stream_Control_Transmission_Protocol  {
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод получения статуса SCTP сокета
				 *
				 * @param sock   сетевой сокет
				 * @param status объект для извлечения статуса инициализации SCTP сокета
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of getting the status of an SCTP socket
				 * @param sock   network socket
				 * @param status object to extract the status of the initialization of the SCTP socket into
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool status(const net::socket_t sock, net::sctp::status_t & status) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод инициализации SCTP сокета
				 *
				 * @details Задаёт исходные свойства подключения: сколько потоков
				 * завести в каждую сторону и сколько раз повторять попытки
				 *
				 * @warning Свойства эти согласуются при установке подключения, и
				 * задавать их следует **до** неё - на установленном подключении они
				 * уже не изменятся
				 *
				 * @param sock    сетевой сокет
				 * @param initmsg параметры инициализации SCTP сокета
				 * @return        результат работы функции
				 *
				 * \~english
				 * @brief Method of the initialization of an SCTP socket
				 * @details Sets the initial properties of the connection: how many streams
				 * should be started in each direction and how many times the attempts should be repeated
				 * @warning These properties are agreed at the establishment of the connection, and
				 * they should be set **before** it — on an established connection they
				 * will no longer change
				 * @param sock    network socket
				 * @param initmsg parameters of the initialization of the SCTP socket
				 * @return        result of the work of the function
				 *
				 * \~
				 */
				bool initMessages(const net::socket_t sock, const net::sctp::initmsg_t & initmsg) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод подписки на SCTP события
				 *
				 * @details Указывает, о каких событиях протокол должен уведомлять.
				 * Уведомления приходят вперемешку с обычными данными и отличаются от
				 * них признаком при чтении
				 *
				 * @warning Подписавшись, читающий обязан отличать уведомления от
				 * данных, иначе служебное сообщение будет принято за полезное
				 *
				 * @param sock   сетевой сокет
				 * @param events список событий SCTP для активации
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of the subscription to the SCTP events
				 * @details Specifies which events the protocol should notify about.
				 * The notifications come mixed with the ordinary data and differ from
				 * them by a sign at the reading
				 * @warning Having subscribed, the reading side is obliged to tell the notifications from
				 * the data, otherwise a service message will be taken for a useful one
				 * @param sock   network socket
				 * @param events list of the SCTP events to activate
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool eventsSubscribe(const net::socket_t sock, const net::sctp::event_types_t & events) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки поддерживаемых алгоритмов аутентификации SCTP сокета
				 *
				 * @param sock  сетевой сокет
				 * @param types список поддерживаемых алгоритмов аутентификации
				 * @return      результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the supported algorithms of the authentication of an SCTP socket
				 * @param sock  network socket
				 * @param types list of the supported algorithms of the authentication
				 * @return      result of the work of the function
				 *
				 * \~
				 */
				bool authenticateSupportAlgorithms(const net::socket_t sock, const vector <net::sctp::auth_type_t> & types) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки ключа аутентификации SCTP сокета
				 *
				 * @details Задаёт общий ключ, которым подтверждается подлинность
				 * частей сообщения
				 *
				 * @note Ключ должен совпадать у обеих сторон и передаваться им иным
				 * путём: сам протокол его не согласует. Ключей может быть несколько,
				 * и различаются они номером - так их и меняют, не разрывая связи
				 *
				 * @param sock сетевой сокет
				 * @param num  номер ключа аутентификации
				 * @param key  ключ аутентификации
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the key of the authentication of an SCTP socket
				 * @details Sets the common key by which the authenticity of
				 * the parts of a message is confirmed
				 * @note The key must coincide at both sides and be passed to them by another
				 * path: the protocol itself does not agree it. There may be several keys,
				 * and they differ by a number — that is how they are changed, without breaking the connection
				 * @param sock network socket
				 * @param num  number of the key of the authentication
				 * @param key  key of the authentication
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool authenticateKey(const net::socket_t sock, const uint16_t num, string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации ключа аутентификации SCTP сокета
				 *
				 * @param sock сетевой сокет
				 * @param mode режим установки типа сокета
				 * @param id   идентификатор ассоциации
				 * @param num  номер ключа аутентификации
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of the activation/deactivation of the key of the authentication of an SCTP socket
				 * @param sock network socket
				 * @param mode mode of the setting of the type of the socket
				 * @param id   identifier of the association
				 * @param num  number of the key of the authentication
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool authenticateKey(const net::socket_t sock, const net::socket_mode_t mode, const uint32_t id, const uint16_t num) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки чанков аутентификации SCTP сокета
				 *
				 * @details Задаёт, какие части сообщения подлежат подтверждению
				 * подлинности
				 *
				 * @warning Части, в набор не вошедшие, идут **без подтверждения**.
				 * Включённое подтверждение само по себе подлинности всего обмена не
				 * обещает - важно ещё и что именно в набор попало
				 *
				 * @param sock   сетевой сокет
				 * @param chunks список чанков подлежащих аутентификации
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the chunks of the authentication of an SCTP socket
				 * @details Sets which parts of a message are subject to the confirmation of
				 * the authenticity
				 * @warning The parts that have not entered the set go **without a confirmation**.
				 * A switched on confirmation by itself does not promise the authenticity of the whole exchange —
				 * what exactly has entered the set matters as well
				 * @param sock   network socket
				 * @param chunks list of the chunks subject to the authentication
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool authenticateChunks(const net::socket_t sock, const vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения чанков аутентификации SCTP сокета
				 *
				 * @param sock   сетевой сокет
				 * @param origin источник события
				 * @param id     идентификатор ассоциации
				 * @param chunks список чанков подлежащих аутентификации
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of extracting the chunks of the authentication of an SCTP socket
				 * @param sock   network socket
				 * @param origin source of the event
				 * @param id     identifier of the association
				 * @param chunks list of the chunks subject to the authentication
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool authenticateChunks(const net::socket_t sock, const event::origin_t origin, const uint32_t id, vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения таймаута SCTP сокета
				 *
				 * @param sock сетевой сокет
				 * @param id   идентификатор ассоциации
				 * @param type тип таймаута
				 * @param ctx  контекст установки таймаута
				 * @return     значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of an SCTP socket
				 * @param sock network socket
				 * @param id   identifier of the association
				 * @param type type of the timeout
				 * @param ctx  context of the setting of the timeout
				 * @return     value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, void * ctx = nullptr) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута SCTP сокета
				 *
				 * @param sock    сетевой сокет
				 * @param id      идентификатор ассоциации
				 * @param type    тип таймаута
				 * @param timeout значение таймаута в миллисекундах
				 * @param ctx     контекст установки таймаута
				 * @return        результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the timeout of an SCTP socket
				 * @param sock    network socket
				 * @param id      identifier of the association
				 * @param type    type of the timeout
				 * @param timeout value of the timeout in milliseconds
				 * @param ctx     context of the setting of the timeout
				 * @return        result of the work of the function
				 *
				 * \~
				 */
				bool timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, const uint32_t timeout, void * ctx = nullptr) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки поддержки системой современного набора вызовов SCTP
				 *
				 * @details Набор этот (RFC 6458) выдаёт метаданные сообщения отдельной
				 *          структурой и позволяет задавать отправку политиками частичной
				 *          надёжности. Он есть у FreeBSD, Solaris и Linux, но его нет
				 *          вовсе у illumos, где остаётся лишь прежний способ
				 *
				 * @return результат проверки поддержки
				 *
				 * \~english
				 * @brief Method of the check of the support by the system of the modern set of the calls of SCTP
				 * @details This set (RFC 6458) gives out the metadata of a message by a separate
				 *          structure and allows setting the sending by the policies of the partial
				 *          reliability. FreeBSD, Solaris and Linux have it, but illumos does not have it
				 *          at all, where only the former way remains
				 * @return result of the check of the support
				 *
				 * \~
				 */
				bool modern() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки поддержки системой явной границы записи
				 *
				 * @details Режим этот нужен для отправки сообщения по частям: без него
				 *          система ставит границу записи на каждую отправку сама. Он есть
				 *          у FreeBSD и Solaris, но его нет у Linux
				 *
				 * @return результат проверки поддержки
				 *
				 * \~english
				 * @brief Method of the check of the support by the system of the explicit boundary of a record
				 * @details This mode is needed for the sending of a message in parts: without it
				 *          the system puts the boundary of a record on each sending itself. FreeBSD
				 *          and Solaris have it, but Linux does not
				 * @return result of the check of the support
				 *
				 * \~
				 */
				bool partial() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод управления подпиской на метаданные принимаемых сообщений
				 *
				 * @details Пока подписка не выдана, ядро метаданных не присылает вовсе, и
				 *          приём их ничего не стоит. Выдавать её следует лишь тем событиям,
				 *          у которых установлен отклик чтения с метаданными
				 *
				 * @param sock сетевой сокет
				 * @param mode режим подписки на метаданные
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of the control of the subscription to the metadata of the received messages
				 * @details While the subscription is not given out, the kernel does not send the metadata at all, and
				 *          their reception costs nothing. Giving it out follows only to those events
				 *          that have the callback of the reading with the metadata set
				 * @param sock network socket
				 * @param mode mode of the subscription to the metadata
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool receiveInfo(const net::socket_t sock, const bool mode) const noexcept;
				/**
				 * \~russian
				 * @brief Метод управления режимом явной границы записи
				 *
				 * @param sock сетевой сокет
				 * @param mode режим явной границы записи
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of the control of the mode of the explicit boundary of a record
				 * @param sock network socket
				 * @param mode mode of the explicit boundary of a record
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool explicitEndOfRecord(const net::socket_t sock, const bool mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод чтения сообщения SCTP вместе с метаданными
				 *
				 * @details Способ чтения выбирается по поддержке системой современного
				 *          набора вызовов, а не по надобности вызывающего: путь чтения
				 *          обязан быть один, иначе движки разойдутся между собой
				 *
				 * @param sock   сетевой сокет
				 * @param buffer буфер принимаемых данных
				 * @param size   размер буфера принимаемых данных
				 * @param addr   адрес удалённого узла
				 * @param length размер адреса удалённого узла
				 * @param info   метаданные полученного сообщения
				 * @param legacy метаданные полученного сообщения прежнего вида
				 * @param flags  флаги полученного сообщения в системном виде
				 * @return       количество принятых октетов либо -1 при отказе
				 *
				 * @note Метаданные заполняются лишь при выданной подписке: без неё ядру
				 *       сообщать нечего, и структура остаётся пустой
				 *
				 * \~english
				 * @brief Method of the reading of an SCTP message together with the metadata
				 * @details The way of the reading is chosen by the support by the system of the modern
				 *          set of the calls, and not by the need of the caller: the path of the reading
				 *          is due to be one, otherwise the engines will diverge among themselves
				 * @param sock   network socket
				 * @param buffer buffer of the received data
				 * @param size   size of the buffer of the received data
				 * @param addr   address of the remote node
				 * @param length size of the address of the remote node
				 * @param info   metadata of the received message
				 * @param legacy metadata of the received message of the former kind
				 * @param flags  flags of the received message in the system kind
				 * @return       number of the received octets or -1 at a refusal
				 * @note The metadata are filled only at a given out subscription: without it the kernel
				 *       has nothing to report, and the structure remains empty
				 *
				 * \~
				 */
				ssize_t receive(const net::socket_t sock, void * buffer, const size_t size, struct sockaddr * addr, socklen_t * length, net::sctp::rinfo_t & info, struct sctp_sndrcvinfo & legacy, int32_t & flags) const noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки сообщения SCTP вместе с метаданными
				 *
				 * @param sock     сетевой сокет
				 * @param buffer   буфер отправляемых данных
				 * @param size     размер буфера отправляемых данных
				 * @param addr     адрес удалённого узла
				 * @param length   размер адреса удалённого узла
				 * @param info     информационные метаданные сообщения
				 * @param complete признак завершения сообщения на этом куске
				 * @return         количество отправленных октетов либо -1 при отказе
				 *
				 * @warning Признак незавершённости требует включённого режима явной границы
				 *          записи: без него система закроет запись сама, и сообщение
				 *          разобьётся на несколько
				 *
				 * \~english
				 * @brief Method of the sending of an SCTP message together with the metadata
				 * @param sock     network socket
				 * @param buffer   buffer of the sent data
				 * @param size     size of the buffer of the sent data
				 * @param addr     address of the remote node
				 * @param length   size of the address of the remote node
				 * @param info     informational metadata of the message
				 * @param complete sign of the completion of the message on this piece
				 * @return         number of the sent octets or -1 at a refusal
				 * @warning The sign of the incompleteness requires the switched on mode of the explicit boundary
				 *          of a record: without it the system will close the record itself, and the message
				 *          will break into several
				 *
				 * \~
				 */
				ssize_t send(const net::socket_t sock, const void * buffer, const size_t size, const struct sockaddr * addr, const socklen_t length, const net::sctp::minfo_t & info, const bool complete = true) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				Stream_Control_Transmission_Protocol(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Stream_Control_Transmission_Protocol() noexcept {}
		} sctp_t;
	};
};

#endif // __AWH_SCTP__
