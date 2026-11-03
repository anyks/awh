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
 *        пространство имён eth::sctp для настройки параметров ассоциаций, потоков, heartbeat,
 *        авторизации и уведомлений SCTP-сокета
 *
 * \~english
 * @brief Header file of the module of the management of the SCTP protocol —
 *        the eth::sctp namespace for setting the parameters of the associations, of the streams, of the heartbeat,
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
#pragma once

/**
 * Наши модули
 */
#include "../net.hpp"
#include <sys/macro/global.hpp>

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
 *
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
		 * @brief Пространство имён управления протоколом передачи с управлением потоком
		 *
		 * @details Настройки протокола, сочетающего надёжность потокового обмена
		 *          с сохранением границ сообщений. Ведает подпиской на его известия,
		 *          пределами ожидания на каждом этапе и подтверждением подлинности
		 *          частей сообщения.
		 *
		 * @note Собирается лишь на Linux и FreeBSD - прочие системы протокола не несут
		 *
		 * \~english
		 * @brief Namespace of the management of the transmission protocol with the flow control
		 *
		 * @details The settings of the protocol combining the reliability of the stream exchange
		 *          with the preservation of the boundaries of the messages. Is in charge of the subscription to its notices,
		 *          of the limits of the waiting at every stage and of the confirmation of the authenticity of
		 *          the parts of a message.
		 *
		 * @note Is built only on Linux and FreeBSD — the other systems do not carry the protocol
		 *
		 * \~
		 */
		namespace sctp {
			/**
			 * \~russian
			 * @brief Функция получения статуса SCTP сокета
			 *
			 * @param sock   сетевой сокет
			 * @param status объект для извлечения статуса инициализации SCTP сокета
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Function of getting the status of an SCTP socket
			 *
			 * @param sock   network socket
			 * @param status object to extract the status of the initialization of the SCTP socket into
			 * @return       result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool status(const net::socket_t sock, net::sctp::status_t & status) noexcept;
			/**
			 * \~russian
			 * @brief Функция инициализации SCTP сокета
			 *
			 * @details Задаёт исходные свойства подключения: сколько потоков
			 *          завести в каждую сторону и сколько раз повторять попытки.
			 *
			 * @warning Свойства эти согласуются при установке подключения, и
			 *          задавать их следует **до** неё - на установленном подключении они
			 *          уже не изменятся
			 *
			 * @param sock    сетевой сокет
			 * @param initmsg параметры инициализации SCTP сокета
			 * @return        результат работы функции
			 *
			 * \~english
			 * @brief Function of the initialization of an SCTP socket
			 *
			 * @details Sets the initial properties of the connection: how many streams
			 *          should be started in each direction and how many times the attempts should be repeated.
			 *
			 * @warning These properties are agreed at the establishment of the connection, and
			 *          they should be set **before** it — on an established connection they
			 *          will no longer change
			 *
			 * @param sock    network socket
			 * @param initmsg parameters of the initialization of the SCTP socket
			 * @return        result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool initMessages(const net::socket_t sock, const net::sctp::initmsg_t & initmsg) noexcept;
			/**
			 * \~russian
			 * @brief Функция подписки на SCTP события
			 *
			 * @details Указывает, о каких событиях протокол должен уведомлять.
			 *          Уведомления приходят вперемешку с обычными данными и отличаются от
			 *          них признаком при чтении.
			 *
			 * @warning Подписавшись, читающий обязан отличать уведомления от
			 *          данных, иначе служебное сообщение будет принято за полезное
			 *
			 * @param sock   сетевой сокет
			 * @param events список событий SCTP для активации
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Function of the subscription to the SCTP events
			 *
			 * @details Specifies which events the protocol should notify about.
			 *          The notifications come mixed with the ordinary data and differ from
			 *          them by a sign at the reading.
			 *
			 * @warning Having subscribed, the reading side is obliged to tell the notifications from
			 *          the data, otherwise a service message will be taken for a useful one
			 *
			 * @param sock   network socket
			 * @param events list of the SCTP events to activate
			 * @return       result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool eventsSubscribe(const net::socket_t sock, const net::sctp::event_types_t & events) noexcept;
			/**
			 * \~russian
			 * @brief Функция опроса возможности протокола SCTP у текущей системы
			 *
			 * @details Отвечает, несёт ли система запрошенную возможность. Спросить заранее
			 *          вернее, чем узнавать отказом: отказ по неумению системы и отказ по
			 *          негодным доводам вызова с виду одинаковы, и отличить их можно лишь
			 *          по журналу - то есть глазами человека, а не кодом.
			 *
			 * @note Ответ не зависит ни от сокета, ни от состояния связи: он определяется
			 *       единственно системой, под которую собран движок, и потому опрашивается
			 *       без описателя
			 *
			 * @warning Утвердительный ответ означает лишь наличие возможности у системы, а
			 *          не успех вызова: тот вправе отказать по своим доводам - негодному
			 *          сокету, неверной длине ключа, неподходящему состоянию связи
			 *
			 * @par Пример: выбор пути по возможностям системы
			 *
			 * @code{.cpp}
			 * if(io.sctp.supported(awh::net::sctp::feature_t::AUTHENTICATION))
			 *     io.sctp.authenticateKey(sock, 1, key);
			 * @endcode
			 *
			 * @param feature опрашиваемая возможность протокола
			 * @return        результат опроса возможности
			 *
			 * \~english
			 * @brief Function of the inquiry of a capability of the SCTP protocol at the current system
			 *
			 * @details Answers whether the system carries the requested capability. To ask
			 *          beforehand is more correct than to learn by a refusal: a refusal by the
			 *          inability of the system and a refusal by the unsuitable arguments of a call
			 *          look alike, and they can be distinguished only by the log — that is, by the eyes of a human, and not by the code.
			 *
			 * @note The answer depends neither on a socket nor on the state of an association: it
			 *       is determined solely by the system the engine is built for, and therefore it is
			 *       inquired without a descriptor
			 *
			 * @warning An affirmative answer means only the presence of the capability at the
			 *          system, and not the success of a call: that one has the right to refuse by its own
			 *          reasons — an unsuitable socket, a wrong length of a key, an inappropriate state of an association
			 *
			 * @par Example: the choice of a path by the capabilities of the system
			 *
			 * @code{.cpp}
			 * if(awh::eth::sctp::supported(awh::net::sctp::feature_t::AUTHENTICATION))
			 *     awh::eth::sctp::authenticateKey(sock, 1, key);
			 * @endcode
			 *
			 * @param feature inquired capability of the protocol
			 * @return        result of the inquiry of the capability
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool supported(const net::sctp::feature_t feature) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки поддерживаемых алгоритмов аутентификации SCTP сокета
			 *
			 * @param sock  сетевой сокет
			 * @param types список поддерживаемых алгоритмов аутентификации
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Function of setting the supported algorithms of the authentication of an SCTP socket
			 *
			 * @param sock  network socket
			 * @param types list of the supported algorithms of the authentication
			 * @return      result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool authenticateSupportAlgorithms(const net::socket_t sock, const vector <net::sctp::auth_type_t> & types) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки ключа аутентификации SCTP сокета
			 *
			 * @details Задаёт общий ключ, которым подтверждается подлинность
			 *          частей сообщения.
			 *
			 * @note Ключ должен совпадать у обеих сторон и передаваться им иным
			 *       путём: сам протокол его не согласует. Ключей может быть несколько,
			 *       и различаются они номером - так их и меняют, не разрывая связи
			 *
			 * @param sock сетевой сокет
			 * @param num  номер ключа аутентификации
			 * @param key  ключ аутентификации
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Function of setting the key of the authentication of an SCTP socket
			 *
			 * @details Sets the common key by which the authenticity of
			 *          the parts of a message is confirmed.
			 *
			 * @note The key must coincide at both sides and be passed to them by another
			 *       path: the protocol itself does not agree it. There may be several keys,
			 *       and they differ by a number — that is how they are changed, without breaking the connection
			 *
			 * @param sock network socket
			 * @param num  number of the key of the authentication
			 * @param key  key of the authentication
			 * @return     result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool authenticateKey(const net::socket_t sock, const uint16_t num, string_view key) noexcept;
			/**
			 * \~russian
			 * @brief Функция активации/деактивации ключа аутентификации SCTP сокета
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @param id   идентификатор ассоциации
			 * @param num  номер ключа аутентификации
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Function of the activation/deactivation of the key of the authentication of an SCTP socket
			 *
			 * @param sock network socket
			 * @param mode mode of the setting of the type of the socket
			 * @param id   identifier of the association
			 * @param num  number of the key of the authentication
			 * @return     result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool authenticateKey(const net::socket_t sock, const net::socket_mode_t mode, const uint32_t id, const uint16_t num) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки чанков аутентификации SCTP сокета
			 *
			 * @details Задаёт, какие части сообщения подлежат подтверждению подлинности.
			 *
			 * @warning Части, в набор не вошедшие, идут **без подтверждения**.
			 *          Включённое подтверждение само по себе подлинности всего обмена не
			 *          обещает - важно ещё и что именно в набор попало
			 *
			 * @param sock   сетевой сокет
			 * @param chunks список чанков подлежащих аутентификации
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Function of setting the chunks of the authentication of an SCTP socket
			 *
			 * @details Sets which parts of a message are subject to the confirmation of the authenticity.
			 *
			 * @warning The parts that have not entered the set go **without a confirmation**.
			 *          A switched on confirmation by itself does not promise the authenticity of the whole exchange —
			 *          what exactly has entered the set matters as well
			 *
			 * @param sock   network socket
			 * @param chunks list of the chunks subject to the authentication
			 * @return       result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool authenticateChunks(const net::socket_t sock, const vector <net::sctp::auth_chunk_t> & chunks) noexcept;
			/**
			 * \~russian
			 * @brief Функция извлечения чанков аутентификации SCTP сокета
			 *
			 * @param sock   сетевой сокет
			 * @param origin источник события
			 * @param id     идентификатор ассоциации
			 * @param chunks список чанков подлежащих аутентификации
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Function of extracting the chunks of the authentication of an SCTP socket
			 *
			 * @param sock   network socket
			 * @param origin source of the event
			 * @param id     identifier of the association
			 * @param chunks list of the chunks subject to the authentication
			 * @return       result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool authenticateChunks(const net::socket_t sock, const event::origin_t origin, const uint32_t id, vector <net::sctp::auth_chunk_t> & chunks) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения таймаута SCTP сокета
			 *
			 * @param sock сетевой сокет
			 * @param id   идентификатор ассоциации
			 * @param type тип таймаута
			 * @param ctx  контекст установки таймаута
			 * @return     значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Function of getting the timeout of an SCTP socket
			 *
			 * @param sock network socket
			 * @param id   identifier of the association
			 * @param type type of the timeout
			 * @param ctx  context of the setting of the timeout
			 * @return     value of the timeout in milliseconds
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint32_t timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, void * ctx = nullptr) noexcept;
			/**
			 * \~russian
			 * @brief Функция установки таймаута SCTP сокета
			 *
			 * @param sock    сетевой сокет
			 * @param id      идентификатор ассоциации
			 * @param type    тип таймаута
			 * @param timeout значение таймаута в миллисекундах
			 * @param ctx     контекст установки таймаута
			 * @return        результат работы функции
			 *
			 * \~english
			 * @brief Function of setting the timeout of an SCTP socket
			 *
			 * @param sock    network socket
			 * @param id      identifier of the association
			 * @param type    type of the timeout
			 * @param timeout value of the timeout in milliseconds
			 * @param ctx     context of the setting of the timeout
			 * @return        result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, const uint32_t timeout, void * ctx = nullptr) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки поддержки системой современного набора вызовов SCTP
			 *
			 * @details Набор этот (RFC 6458) выдаёт метаданные сообщения отдельной
			 *          структурой и позволяет задавать отправку политиками частичной
			 *          надёжности. Он есть у FreeBSD, Solaris и Linux, но его нет
			 *          вовсе у illumos, где остаётся лишь прежний способ.
			 *
			 * @return результат проверки поддержки
			 *
			 * \~english
			 * @brief Function of the check of the support by the system of the modern set of the calls of SCTP
			 *
			 * @details This set (RFC 6458) gives out the metadata of a message by a separate
			 *          structure and allows setting the sending by the policies of the partial
			 *          reliability. FreeBSD, Solaris and Linux have it, but illumos does not have it
			 *          at all, where only the former way remains.
			 *
			 * @return result of the check of the support
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool modern() noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки поддержки системой явной границы записи
			 *
			 * @details Режим этот нужен для отправки сообщения по частям: без него
			 *          система ставит границу записи на каждую отправку сама. Он есть
			 *          у FreeBSD и Solaris, но его нет у Linux.
			 *
			 * @return результат проверки поддержки
			 *
			 * \~english
			 * @brief Function of the check of the support by the system of the explicit boundary of a record
			 *
			 * @details This mode is needed for the sending of a message in parts: without it
			 *          the system puts the boundary of a record on each sending itself. FreeBSD
			 *          and Solaris have it, but Linux does not.
			 *
			 * @return result of the check of the support
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool partial() noexcept;
			/**
			 * \~russian
			 * @brief Функция управления подпиской на метаданные принимаемых сообщений
			 *
			 * @details Пока подписка не выдана, ядро метаданных не присылает вовсе, и
			 *          приём их ничего не стоит. Выдавать её следует лишь тем событиям,
			 *          у которых установлен отклик чтения с метаданными.
			 *
			 * @param sock сетевой сокет
			 * @param mode режим подписки на метаданные
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Function of the control of the subscription to the metadata of the received messages
			 *
			 * @details While the subscription is not given out, the kernel does not send the metadata at all, and
			 *          their reception costs nothing. Giving it out follows only to those events
			 *          that have the callback of the reading with the metadata set.
			 *
			 * @param sock network socket
			 * @param mode mode of the subscription to the metadata
			 * @return     result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool receiveInfo(const net::socket_t sock, const bool mode) noexcept;
			/**
			 * \~russian
			 * @brief Функция управления режимом явной границы записи
			 *
			 * @param sock сетевой сокет
			 * @param mode режим явной границы записи
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Function of the control of the mode of the explicit boundary of a record
			 *
			 * @param sock network socket
			 * @param mode mode of the explicit boundary of a record
			 * @return     result of the work of the function
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool explicitEndOfRecord(const net::socket_t sock, const bool mode) noexcept;
			/**
			 * \~russian
			 * @brief Функция чтения сообщения SCTP вместе с метаданными
			 *
			 * @details Способ чтения выбирается по поддержке системой современного
			 *          набора вызовов, а не по надобности вызывающего: путь чтения
			 *          обязан быть один, иначе движки разойдутся между собой.
			 *
			 * @note Метаданные заполняются лишь при выданной подписке: без неё ядру
			 *       сообщать нечего, и структура остаётся пустой
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
			 * \~english
			 * @brief Function of the reading of an SCTP message together with the metadata
			 *
			 * @details The way of the reading is chosen by the support by the system of the modern
			 *          set of the calls, and not by the need of the caller: the path of the reading
			 *          is due to be one, otherwise the engines will diverge among themselves.
			 *
			 * @note The metadata are filled only at a given out subscription: without it the kernel
			 *       has nothing to report, and the structure remains empty
			 *
			 * @param sock   network socket
			 * @param buffer buffer of the received data
			 * @param size   size of the buffer of the received data
			 * @param addr   address of the remote node
			 * @param length size of the address of the remote node
			 * @param info   metadata of the received message
			 * @param legacy metadata of the received message of the former kind
			 * @param flags  flags of the received message in the system kind
			 * @return       number of the received octets or -1 at a refusal
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ ssize_t receive(const net::socket_t sock, void * buffer, const size_t size, struct sockaddr * addr, socklen_t * length, net::sctp::rinfo_t & info, struct sctp_sndrcvinfo & legacy, int32_t & flags) noexcept;
			/**
			 * \~russian
			 * @brief Функция отправки сообщения SCTP вместе с метаданными
			 *
			 * @warning Признак незавершённости требует включённого режима явной границы
			 *          записи: без него система закроет запись сама, и сообщение
			 *          разобьётся на несколько
			 *
			 * @param sock     сетевой сокет
			 * @param buffer   буфер отправляемых данных
			 * @param size     размер буфера отправляемых данных
			 * @param addr     адрес удалённого узла
			 * @param length   размер адреса удалённого узла
			 * @param info     информационные метаданные сообщения в системном виде
			 * @param complete признак завершения сообщения на этом куске
			 * @return         количество отправленных октетов либо -1 при отказе
			 *
			 * \~english
			 * @brief Function of the sending of an SCTP message together with the metadata
			 *
			 * @warning The sign of the incompleteness requires the switched on mode of the explicit boundary
			 *          of a record: without it the system will close the record itself, and the message
			 *          will break into several
			 *
			 * @param sock     network socket
			 * @param buffer   buffer of the sent data
			 * @param size     size of the buffer of the sent data
			 * @param addr     address of the remote node
			 * @param length   size of the address of the remote node
			 * @param info     informational metadata of the message
			 * @param complete sign of the completion of the message on this piece
			 * @return         number of the sent octets or -1 at a refusal
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ ssize_t send(const net::socket_t sock, const void * buffer, const size_t size, const struct sockaddr * addr, const socklen_t length, const struct sctp_sndrcvinfo & info, const bool complete = true) noexcept;
		};
	};
};
