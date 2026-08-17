/**
 * @file io.hpp
 * @date 2025-11-06
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
 * @brief Заголовочный файл асинхронного движка ввода-вывода — класс engine::IO, реализующий цикл событий,
 *        работу с сокетами всех поддерживаемых семейств и протоколов, таймеры, наблюдение за файлами и каталогами,
 *        списки контроля доступа и поддержку SCTP
 *
 * \~english
 * @brief Header file of the asynchronous input-output engine — the engine::IO class, implementing the loop of the events,
 *        the work with the sockets of all the supported families and protocols, the timers, the observation of the files and of the directories,
 *        the lists of the control of the access and the support of SCTP
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IO_ENGINE__
#define __AWH_IO_ENGINE__

/**
 * Подключаем заголовочный файл проекта
 */
#include "engine.hpp"

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
	 * @brief Пространство имён движков ввода-вывода
	 *
	 * \~english
	 * @brief Namespace of the input-output engines
	 *
	 * \~
	 */
	namespace engine {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
		 */
		#if __linux__ || __FreeBSD__ || __sun
			/**
			 * \~russian
			 * @brief Класс управления протоколом передачи с управлением потоком
			 *
			 * @details Спутник движка, обслуживающий то, что есть только у SCTP и не
			 *          укладывается в общую поверхность `engine::io_t`. Само событие
			 *          SCTP-сокета заводится движком обычным порядком - с протоколом
			 *          `event::protocol_t::SCTP`, - а этот класс правит уже
			 *          заведённое событие по его идентификатору.
			 *
			 *          Разделение сделано намеренно. SCTP - протокол с ассоциациями,
			 *          многопоточностью внутри одного соединения, собственными
			 *          таймерами и аутентификацией чанков; у TCP и UDP аналогов этому
			 *          нет. Свести всё в один интерфейс значило бы обвесить движок
			 *          методами, недействительными для девяти протоколов из десяти,
			 *          поэтому особенности SCTP вынесены отдельно, а общее -
			 *          подключение, приём, отправка, освобождение - остаётся за
			 *          движком.
			 *
			 *          Класс покрывает четыре области:
			 *
			 *          - **метаданные сообщения** - `messageInfo()`: поток, номер
			 *            последовательности, признак упорядоченности; у SCTP сообщение
			 *            несёт их само, в отличие от потока октетов у TCP;
			 *          - **параметры ассоциации** - `initMessages()` и `status()`:
			 *            количество потоков в каждую сторону, число попыток
			 *            установления, и текущее состояние ассоциации;
			 *          - **подписка на события протокола** - `eventsSubscribe()`:
			 *            какие уведомления протокола доставлять прикладному коду;
			 *          - **аутентификация по RFC 4895** - `authenticateKey()`,
			 *            `authenticateChunks()`, `authenticateSupportAlgorithms()`.
			 *
			 * @note    Класс объявлен для Linux, FreeBSD, Solaris и illumos, но
			 *          **отлажен и работает только под FreeBSD**. У macOS и OpenBSD
			 *          SCTP в ядре нет вовсе, а NetBSD держит заголовок без поддержки
			 *          в ядре; класс там не существует - обращения к нему не соберутся,
			 *          а не откажут во время работы. Прикладной код, рассчитанный на
			 *          переносимость, обязан заворачивать обращения в ту же проверку
			 *          `#if __linux__ || __FreeBSD__ || __sun`.
			 *
			 * @note    Рабочие примеры лежат в [`sample/net/sctp/`](../../sample/net/sctp):
			 *          потоковый и последовательно-пакетный режимы, поверх TLS и
			 *          DTLS, с аутентификацией чанков - по паре клиент и сервер на
			 *          каждый случай. Смотреть следует их, а не пример ниже: он
			 *          показывает только порядок вызовов.
			 *
			 * @note    Объект создаётся отдельно от движка и своего состояния о
			 *          событиях не держит: он лишь переводит вызовы в параметры
			 *          сокета по идентификатору события. Поэтому один объект
			 *          обслуживает сколько угодно событий, а порядок его создания
			 *          относительно движка не важен.
			 *
			 * @note    Таймауты `setTimeout()` - это таймеры **самого протокола**
			 *          (`INIT`, `DATA`, `SACK`, `SHUTDOWN`, `HEARTBEAT`, `COOKIE`,
			 *          `SHUTDOWNACK`), а не таймауты события движка. Путать их с
			 *          `engine::io_t::setTimeout()` нельзя: те отсчитывает движок и
			 *          сообщает о них функциями обратного вызова, эти отсчитывает ядро
			 *          и действует по ним само
			 *
			 * @par Пример: SCTP-клиент с несколькими потоками в ассоциации
			 *
			 * @code{.cpp}
			 * #if __linux__ || __FreeBSD__ || __sun
			 *     awh::engine::io_t io(&fmk, &log);
			 *     awh::engine::sctp_t sctp(&fmk, &log);
			 *     // Заводим событие SCTP-сокета обычным порядком движка
			 *     const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP);
			 *     io.setTargetPort(client, 9899);
			 *     io.setTarget(client, "127.0.0.1");
			 *     // Задаём параметры ассоциации: по четыре потока в каждую сторону
			 *     awh::net::sctp::initmsg_t initmsg;
			 *     initmsg.outputStreams = 4;
			 *     initmsg.inputStreams = 4;
			 *     sctp.initMessages(client, initmsg);
			 *     // Продлеваем таймер контрольных сообщений протокола до тридцати секунд
			 *     sctp.setTimeout(client, awh::net::sctp::timeout_t::HEARTBEAT, 30000);
			 *     // Подписываемся на метаданные принятых сообщений
			 *     sctp.on(client, static_cast <awh::engine::callback::sctp::minfo_t> ([](const awh::event::id_t id, const awh::net::sctp::minfo_t & info) noexcept -> void {
			 *         // Здесь известно, каким потоком ассоциации пришло сообщение
			 *     }));
			 *     io.initialize();
			 *     io.commit(client);
			 *     io.connect(client);
			 *     io.launch(client);
			 *     while(io.poll(100));
			 *     io.deinitialize();
			 * #endif
			 * @endcode
			 *
			 * \~english
			 * @brief Class of the management of the transmission protocol with the flow control
			 * @details A companion of the engine, serving what only SCTP has and what does not
			 *          fit into the common surface of `engine::io_t`. The event itself of
			 *          an SCTP socket is started by the engine in the ordinary order — with the protocol
			 *          `event::protocol_t::SCTP`, — and this class corrects an already
			 *          started event by its identifier.
			 *          The division is made deliberately. SCTP is a protocol with the associations,
			 *          with the multithreading inside one connection, with its own
			 *          timers and with the authentication of the chunks; TCP and UDP have no analogues of this.
			 *          To bring everything into one interface would mean hanging the engine
			 *          with the methods invalid for nine protocols out of ten,
			 *          and therefore the peculiarities of SCTP are taken out separately, and the common —
			 *          the connection, the reception, the sending, the release — remains at
			 *          the engine.
			 *          The class covers four areas:
			 *          - **the metadata of a message** — `messageInfo()`: the stream, the number
			 *            of the sequence, the sign of the orderedness; at SCTP a message
			 *            carries them itself, unlike the stream of the octets at TCP;
			 *          - **the parameters of an association** — `initMessages()` and `status()`:
			 *            the number of the streams in each direction, the number of the attempts
			 *            of the establishment, and the current state of the association;
			 *          - **the subscription to the events of the protocol** — `eventsSubscribe()`:
			 *            which notifications of the protocol should be delivered to the application code;
			 *          - **the authentication by RFC 4895** — `authenticateKey()`,
			 *            `authenticateChunks()`, `authenticateSupportAlgorithms()`.
			 * @note    The class is declared for Linux, FreeBSD, Solaris and illumos, but is
			 *          **debugged and works only under FreeBSD**. macOS and OpenBSD have
			 *          no SCTP in the kernel at all, and NetBSD holds the header without the support
			 *          in the kernel; the class does not exist there — the addresses to it will not be built,
			 *          and will not refuse during the work. The application code reckoned on
			 *          the portability is obliged to wrap the addresses into the same check
			 *          `#if __linux__ || __FreeBSD__ || __sun`.
			 * @note    The working examples lie in [`sample/net/sctp/`](../../sample/net/sctp):
			 *          the stream and the sequential-packet modes, over TLS and
			 *          DTLS, with the authentication of the chunks — a pair of a client and a server for
			 *          every case. They are the ones to look at, and not the example below: it
			 *          shows only the order of the calls.
			 * @note    The object is created separately from the engine and holds no state of its own about the
			 *          events: it only converts the calls into the parameters of
			 *          a socket by the identifier of an event. Therefore one object
			 *          serves however many events, and the order of its creation
			 *          relative to the engine does not matter.
			 * @note    The `setTimeout()` timeouts are the timers **of the protocol itself**
			 *          (`INIT`, `DATA`, `SACK`, `SHUTDOWN`, `HEARTBEAT`, `COOKIE`,
			 *          `SHUTDOWNACK`), and not the timeouts of an event of the engine. Confusing them with
			 *          `engine::io_t::setTimeout()` is not allowed: those the engine counts and
			 *          reports about them by the callback functions, these the kernel counts
			 *          and acts by them itself
			 * @par Example: an SCTP client with several streams in an association
			 *
			 * @code{.cpp}
			 * #if __linux__ || __FreeBSD__ || __sun
			 *     awh::engine::io_t io(&fmk, &log);
			 *     awh::engine::sctp_t sctp(&fmk, &log);
			 *     // Starting an event of an SCTP socket in the usual order of the engine
			 *     const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP);
			 *     io.setTargetPort(client, 9899);
			 *     io.setTarget(client, "127.0.0.1");
			 *     // Setting the parameters of the association: four streams in each direction
			 *     awh::net::sctp::initmsg_t initmsg;
			 *     initmsg.outputStreams = 4;
			 *     initmsg.inputStreams = 4;
			 *     sctp.initMessages(client, initmsg);
			 *     // Extending the timer of the control messages of the protocol up to thirty seconds
			 *     sctp.setTimeout(client, awh::net::sctp::timeout_t::HEARTBEAT, 30000);
			 *     // Subscribing to the metadata of the received messages
			 *     sctp.on(client, static_cast <awh::engine::callback::sctp::minfo_t> ([](const awh::event::id_t id, const awh::net::sctp::minfo_t & info) noexcept -> void {
			 *         // Here it is known by which stream of the association the message came
			 *     }));
			 *     io.initialize();
			 *     io.commit(client);
			 *     io.connect(client);
			 *     io.launch(client);
			 *     while(io.poll(100));
			 *     io.deinitialize();
			 * #endif
			 * @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Stream_Control_Transmission_Protocol {
				private:
					// Объект работы с сетью
					eth_t _eth;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод получения информационных метаданных SCTP сообщения
					 *
					 * @param id идентификатор события
					 * @return   информационные метаданные SCTP сообщения
					 *
					 * \~english
					 * @brief Method of getting the informational metadata of an SCTP message
					 * @param id identifier of the event
					 * @return   informational metadata of the SCTP message
					 *
					 * \~
					 */
					net::sctp::minfo_t messageInfo(const event::id_t id) const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки информационных метаданных SCTP сообщения
					 *
					 * @param id   идентификатор события
					 * @param info информационные метаданные SCTP сообщения
					 *
					 * \~english
					 * @brief Method of setting the informational metadata of an SCTP message
					 * @param id   identifier of the event
					 * @param info informational metadata of the SCTP message
					 *
					 * \~
					 */
					void messageInfo(const event::id_t id, const net::sctp::minfo_t & info) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения параметров статуса инициализации SCTP
					 *
					 * @param id идентификатор события
					 * @return   параметры статуса инициализации SCTP
					 *
					 * \~english
					 * @brief Method of getting the parameters of the status of the initialization of SCTP
					 * @param id identifier of the event
					 * @return   parameters of the status of the initialization of SCTP
					 *
					 * \~
					 */
					net::sctp::status_t status(const event::id_t id) const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки параметров инициализации SCTP
					 *
					 * @param id      идентификатор события
					 * @param initmsg параметры инициализации SCTP события
					 *
					 * \~english
					 * @brief Method of setting the parameters of the initialization of SCTP
					 * @param id      identifier of the event
					 * @param initmsg parameters of the initialization of the SCTP event
					 *
					 * \~
					 */
					void initMessages(const event::id_t id, const net::sctp::initmsg_t & initmsg) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения опций подписки SCTP событий
					 *
					 * @param id идентификатор события
					 * @return   список событий SCTP на которые выполнена подписка
					 *
					 * \~english
					 * @brief Method of getting the options of the subscription to the SCTP events
					 * @param id identifier of the event
					 * @return   list of the SCTP events the subscription is performed to
					 *
					 * \~
					 */
					const net::sctp::event_types_t & eventsSubscribed(const event::id_t id) const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки опций подписки SCTP событий
					 *
					 * @param id     идентификатор события
					 * @param events список событий SCTP для подписки
					 *
					 * \~english
					 * @brief Method of setting the options of the subscription to the SCTP events
					 * @param id     identifier of the event
					 * @param events list of the SCTP events to subscribe to
					 *
					 * \~
					 */
					void eventsSubscribe(const event::id_t id, const net::sctp::event_types_t & events) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения таймаута SCTP события
					 *
					 * @param id   идентификатор события
					 * @param type тип таймаута
					 * @return     значение таймаута в миллисекундах
					 *
					 * \~english
					 * @brief Method of getting the timeout of an SCTP event
					 * @param id   identifier of the event
					 * @param type type of the timeout
					 * @return     value of the timeout in milliseconds
					 *
					 * \~
					 */
					uint32_t getTimeout(const event::id_t id, const net::sctp::timeout_t type) const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки таймаута SCTP события
					 *
					 * @param id      идентификатор события
					 * @param type    тип таймаута
					 * @param timeout значение таймаута в миллисекундах
					 * @return        результат работы функции
					 *
					 * \~english
					 * @brief Method of setting the timeout of an SCTP event
					 * @param id      identifier of the event
					 * @param type    type of the timeout
					 * @param timeout value of the timeout in milliseconds
					 * @return        result of the work of the function
					 *
					 * \~
					 */
					bool setTimeout(const event::id_t id, const net::sctp::timeout_t type, const uint32_t timeout) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки ключа аутентификации SCTP сокета
					 *
					 * @param id  идентификатор события
					 * @param num номер ключа аутентификации
					 * @param key ключ аутентификации
					 * @return    результат работы функции
					 *
					 * \~english
					 * @brief Method of setting the key of the authentication of an SCTP socket
					 * @param id  identifier of the event
					 * @param num number of the key of the authentication
					 * @param key key of the authentication
					 * @return    result of the work of the function
					 *
					 * \~
					 */
					bool authenticateKey(const event::id_t id, const uint16_t num, string_view key) noexcept;
					/**
					 * \~russian
					 * @brief Метод активации/деактивации ключа аутентификации SCTP сокета
					 *
					 * @param id   идентификатор события
					 * @param mode режим установки действия события
					 * @param num  номер ключа аутентификации
					 * @return     результат работы функции
					 *
					 * \~english
					 * @brief Method of the activation/deactivation of the key of the authentication of an SCTP socket
					 * @param id   identifier of the event
					 * @param mode mode of the setting of the action of the event
					 * @param num  number of the key of the authentication
					 * @return     result of the work of the function
					 *
					 * \~
					 */
					bool authenticateKey(const event::id_t id, const event::mode_t mode, const uint16_t num) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки чанков аутентификации SCTP сокета
					 *
					 * @param id     идентификатор события
					 * @param chunks список чанков подлежащих аутентификации
					 * @return       результат работы функции
					 *
					 * \~english
					 * @brief Method of setting the chunks of the authentication of an SCTP socket
					 * @param id     identifier of the event
					 * @param chunks list of the chunks subject to the authentication
					 * @return       result of the work of the function
					 *
					 * \~
					 */
					bool authenticateChunks(const event::id_t id, const vector <net::sctp::auth_chunk_t> & chunks) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения чанков аутентификации SCTP сокета
					 *
					 * @param id     идентификатор события
					 * @param origin источник события
					 * @param chunks список чанков подлежащих аутентификации
					 * @return       результат работы функции
					 *
					 * \~english
					 * @brief Method of extracting the chunks of the authentication of an SCTP socket
					 * @param id     identifier of the event
					 * @param origin source of the event
					 * @param chunks list of the chunks subject to the authentication
					 * @return       result of the work of the function
					 *
					 * \~
					 */
					bool authenticateChunks(const event::id_t id, const event::origin_t origin, vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки поддерживаемых алгоритмов аутентификации SCTP сокета
					 *
					 * @param id    идентификатор события
					 * @param types список поддерживаемых алгоритмов аутентификации
					 * @return      результат работы функции
					 *
					 * \~english
					 * @brief Method of setting the supported algorithms of the authentication of an SCTP socket
					 * @param id    identifier of the event
					 * @param types list of the supported algorithms of the authentication
					 * @return      result of the work of the function
					 *
					 * \~
					 */
					bool authenticateSupportAlgorithms(const event::id_t id, const vector <net::sctp::auth_type_t> & types) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки поддержки отправки сообщения по частям
					 *
					 * @details Отправка сообщения по частям требует от системы явного режима
					 *          границы записи, и есть он не всюду: FreeBSD и Solaris его имеют,
					 *          Linux не имеет вовсе. Проверять поддержку следует до отправки,
					 *          а не по отказу
					 *
					 * @param id идентификатор события
					 * @return   результат проверки поддержки
					 *
					 * \~english
					 * @brief Method of the check of the support of the sending of a message in parts
					 * @details The sending of a message in parts requires from the system an explicit mode
					 *          of the boundary of a record, and it is not everywhere: FreeBSD and Solaris have it,
					 *          Linux does not have it at all. Checking the support follows before a sending,
					 *          and not by a refusal
					 * @param id identifier of the event
					 * @return   result of the check of the support
					 *
					 * \~
					 */
					bool partialSupported(const event::id_t id) const noexcept;
					/**
					 * \~russian
					 * @brief Метод отправки сообщения SCTP вместе с метаданными
					 *
					 * @details Отправка идёт той же очередью события, что и общая, и потому
					 *          порядок сообщений сохраняется, даже если приложение мешает
					 *          оба способа отправки
					 *
					 * @param id     идентификатор события
					 * @param buffer буфер отправляемых данных
					 * @param size   размер буфера отправляемых данных
					 * @param info   информационные метаданные SCTP сообщения
					 * @param end    признак завершения сообщения на этом куске
					 * @return       количество принятых к отправке октетов
					 *
					 * @warning Сообщение, отправляемое по частям, обязано уйти подряд: пока
					 *          признак завершения не выставлен, отправка иных сообщений тем
					 *          же потоком нарушит его границы.
					 *
					 * \~english
					 * @brief Method of the sending of an SCTP message together with the metadata
					 *
					 * @details The sending goes by the same queue of an event as the common one, and therefore
					 *          the order of the messages is preserved, even if an application mixes
					 *          both ways of the sending
					 *
					 * @param id     identifier of the event
					 * @param buffer buffer of the sent data
					 * @param size   size of the buffer of the sent data
					 * @param info   informational metadata of the SCTP message
					 * @param end    sign of the completion of the message on this piece
					 * @return       number of the octets accepted for the sending
					 *
					 * @warning A message sent in parts is due to go in a row: while
					 *          the sign of the completion is not set, the sending of other messages by the same
					 *          stream will break its boundaries.
					 *
					 * \~
					 */
					size_t send(const event::id_t id, const void * buffer, const size_t size, const net::sctp::minfo_t & info, const bool end = true) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки функции обратного вызова для получения метаданных SCTP-сообщения
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 * \~english
					 * @brief Method of setting the callback function for the getting of the metadata of an SCTP message
					 * @param id identifier of the event
					 * @param cb callback function
					 *
					 * \~
					 */
					void on(const event::id_t id, engine::callback::sctp::minfo_t cb) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки функции обратного вызова для получения SCTP-событий
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 * \~english
					 * @brief Method of setting the callback function for the getting of the SCTP events
					 * @param id identifier of the event
					 * @param cb callback function
					 *
					 * \~
					 */
					void on(const event::id_t id, engine::callback::sctp::events_t cb) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки функции обратного вызова для чтения данных вместе с метаданными
					 *
					 * @details Отклик этот необязателен: задачам, которым метаданные протокола
					 *          не нужны, проще пользоваться общим откликом чтения. Установка
					 *          же его переводит чтение события на выдачу данных вместе с
					 *          метаданными и включает подписку на них у ядра
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 * \~english
					 * @brief Method of setting the callback function for the reading of the data together with the metadata
					 * @details This callback is optional: for the tasks that do not need the metadata of the protocol
					 *          it is simpler to use the common callback of the reading. Setting
					 *          it, though, moves the reading of an event to the giving out of the data together with
					 *          the metadata and switches on the subscription to them at the kernel
					 * @param id identifier of the event
					 * @param cb callback function
					 *
					 * \~
					 */
					void on(const event::id_t id, engine::callback::sctp::message_t cb) noexcept;
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
					explicit Stream_Control_Transmission_Protocol(const fmk_t * fmk, const log_t * log) noexcept;
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
					virtual ~Stream_Control_Transmission_Protocol() noexcept;
			} sctp_t;
		#endif
		/**
		 * \~russian
		 * @brief Тип асинхронного движка ввода-вывода
		 *
		 * @details Движок обслуживает сокеты, файлы, каталоги, межпроцессное
		 *          взаимодействие, туннели и таймеры единым циклом событий. Поверх
		 *          механизма опроса операционной системы - kqueue у BSD и macOS,
		 *          epoll и io_uring у Linux, event ports у Solaris, IOCP у Windows -
		 *          лежит одна и та же модель, поэтому прикладной код от выбора
		 *          механизма не зависит.
		 *
		 *          **Событие вместо дескриптора.** Наружу движок отдаёт не
		 *          дескриптор, а числовой идентификатор события `event::id_t`.
		 *          Разница не косметическая: идентификатор нельзя разыменовать, а
		 *          освобождённое событие по нему просто не находится. Прикладной код
		 *          не держит указателей на внутренние объекты движка и потому не
		 *          может обратиться к уже уничтоженному соединению - самая частая
		 *          ошибка при работе с сырыми дескрипторами здесь невозможна.
		 *          Разрешение идентификатора обходится в три с половиной наносекунды
		 *          и того стоит.
		 *
		 *          **Порядок работы с событием.** Событие проходит четыре шага, и
		 *          порядок их обязателен:
		 *
		 *          1. `event()` - событие создаётся, под него заводится дескриптор,
		 *             событие переходит в состояние `INITIAL`;
		 *          2. настройка - адреса, порты, опции, таймауты, функции обратного
		 *             вызова; всё это только запоминается в событии;
		 *          3. `commit()` - настройки закрепляются, дальше менять их нельзя;
		 *          4. `launch()` - событие включается в опрос и начинает работать.
		 *
		 *          У подключающегося клиента между третьим и четвёртым шагом стоит
		 *          `connect()`, а у сервера - `listen()`. Освобождается событие
		 *          `destroy()` из любого состояния.
		 *
		 *          **Цикл событий ведёт вызывающий.** Движок своего потока не
		 *          создаёт: `poll()` выполняет один оборот и возвращает управление.
		 *          Крутить цикл - дело прикладного кода, и это позволяет вести его в
		 *          своём потоке, встраивать в чужой цикл событий и останавливать
		 *          когда угодно.
		 *
		 * @note    Функции обратного вызова вызываются **внутри** `poll()`. Пока
		 *          выполняется обратный вызов, оборот цикла не завершён, поэтому
		 *          долгая работа в нём задерживает все прочие события. Тяжёлое надо
		 *          уносить в свой поток.
		 *
		 * @note    Освобождать событие изнутри его же обратного вызова допустимо:
		 *          `destroy()` помечает событие и откладывает освобождение на два
		 *          оборота цикла. Отсрочка нужна не для удобства, а по необходимости
		 *          - записи подписки уходят в ядро вместе с ожиданием следующего
		 *          оборота, и закрой движок дескриптор раньше, его номер
		 *          операционная система успела бы выдать другому объекту.
		 *
		 * @note    Движок рассчитан на один поток опроса. Обращаться к событиям из
		 *          других потоков нельзя; для передачи работы в поток цикла заведено
		 *          пользовательское событие `event::node_t::NOTIFY`.
		 *
		 * @par Пример: клиент
		 * @par Пример: сервер
		 *
		 * @code{.cpp}
		 * awh::engine::io_t io(&fmk, &log);
		 * // Заводим событие клиента и настраиваем его
		 * const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		 * io.setTargetPort(client, 80);
		 * io.setTarget(client, "93.184.216.34");
		 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
		 * // Подписываемся на завершение подключения и на приём данных
		 * io.on(client, static_cast <awh::engine::callback::connect_t> ([](const awh::event::id_t id, const bool ok) noexcept -> void {
		 *     // Здесь известно, состоялось подключение или нет
		 * }));
		 * io.on(client, static_cast <awh::engine::callback::read_t> ([](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
		 *     // Принятые данные лежат в буфере и действительны только до выхода отсюда
		 * }));
		 * // Инициализируем движок, закрепляем настройки и запускаем событие
		 * io.initialize();
		 * io.commit(client);
		 * io.connect(client);
		 * io.launch(client);
		 * // Крутим цикл событий, пока он выполняется без ошибок
		 * while(io.poll(100));
		 * io.deinitialize();
		 * @endcode
		 *
		 * @code{.cpp}
		 * awh::engine::io_t io(&fmk, &log);
		 * // Заводим событие сервера и настраиваем его
		 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		 * io.setSourcePort(server, 8080);
		 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
		 * // Принятое подключение приходит готовым событием, отдельной настройки не требует
		 * io.on(server, static_cast <awh::engine::callback::accept_t> ([&io](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
		 *     // Подписываемся на приём данных уже принятого подключения
		 *     io.on(cid, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
		 *         // Возвращаем принятое отправителю
		 *         io.send(id, buffer, size);
		 *     }));
		 * }));
		 * io.initialize();
		 * io.commit(server);
		 * io.listen(server, 1024);
		 * io.launch(server);
		 * while(io.poll(100));
		 * io.deinitialize();
		 * @endcode
		 *
		 * \~english
		 * @brief Type of the asynchronous input-output engine
		 * @details The engine serves the sockets, the files, the directories, the interprocess
		 *          communication, the tunnels and the timers by a single loop of the events. Over
		 *          the mechanism of the polling of the operating system — kqueue at BSD and macOS,
		 *          epoll and io_uring at Linux, event ports at Solaris, IOCP at Windows —
		 *          there lies one and the same model, and therefore the application code does not depend on the choice
		 *          of the mechanism.
		 *          **An event instead of a descriptor.** Outwards the engine gives back not
		 *          a descriptor, but a numeric identifier of an event `event::id_t`.
		 *          The difference is not a cosmetic one: an identifier cannot be dereferenced, and
		 *          a released event is simply not found by it. The application code
		 *          holds no pointers to the internal objects of the engine and therefore
		 *          cannot address an already destroyed connection — the most frequent
		 *          error at the work with the raw descriptors is impossible here.
		 *          The resolution of an identifier costs three and a half nanoseconds
		 *          and is worth it.
		 *          **The order of the work with an event.** An event passes four steps, and
		 *          their order is obligatory:
		 *          1. `event()` — the event is created, a descriptor is started for it,
		 *             the event passes into the `INITIAL` state;
		 *          2. the setup — the addresses, the ports, the options, the timeouts, the callback
		 *             functions; all this is only remembered in the event;
		 *          3. `commit()` — the settings are fixed, further on they cannot be changed;
		 *          4. `launch()` — the event is included into the polling and begins to work.
		 *          At a connecting client between the third and the fourth step there stands
		 *          `connect()`, and at a server — `listen()`. An event is released by
		 *          `destroy()` from any state.
		 *          **The loop of the events is kept by the caller.** The engine creates no thread of its own:
		 *          `poll()` performs one turn and returns the control.
		 *          To spin the loop is the business of the application code, and this allows it to be kept in
		 *          one's own thread, to be embedded into a foreign loop of the events and to be stopped
		 *          whenever.
		 * @note    The callback functions are called **inside** `poll()`. While
		 *          a callback is being performed, the turn of the loop is not completed, and therefore
		 *          a long work in it delays all the other events. The heavy things need
		 *          to be taken away into one's own thread.
		 * @note    Releasing an event from inside its own callback is admissible:
		 *          `destroy()` marks the event and postpones the release for two
		 *          turns of the loop. The delay is needed not for the convenience, but by the necessity
		 *          — the records of the subscription go into the kernel together with the waiting for the next
		 *          turn, and were the engine to close the descriptor earlier, its number
		 *          the operating system would manage to give out to another object.
		 * @note    The engine is reckoned on one thread of the polling. Addressing the events from
		 *          the other threads is not allowed; for the passing of the work into the thread of the loop the
		 *          user event `event::node_t::NOTIFY` is started.
		 * @par Example: a client
		 * @par Example: a server
		 *
		 * @code{.cpp}
		 * awh::engine::io_t io(&fmk, &log);
		 * // Starting an event of a client and setting it up
		 * const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		 * io.setTargetPort(client, 80);
		 * io.setTarget(client, "93.184.216.34");
		 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
		 * // Subscribing to the completion of the connection and to the receiving of the data
		 * io.on(client, static_cast <awh::engine::callback::connect_t> ([](const awh::event::id_t id, const bool ok) noexcept -> void {
		 *     // Here it is known whether the connection took place or not
		 * }));
		 * io.on(client, static_cast <awh::engine::callback::read_t> ([](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
		 *     // The received data lie in the buffer and are valid only until the exit from here
		 * }));
		 * // Initializing the engine, committing the settings and starting the event
		 * io.initialize();
		 * io.commit(client);
		 * io.connect(client);
		 * io.launch(client);
		 * // Spinning the event loop while it runs without errors
		 * while(io.poll(100));
		 * io.deinitialize();
		 * @endcode
		 *
		 * @code{.cpp}
		 * awh::engine::io_t io(&fmk, &log);
		 * // Starting an event of a server and setting it up
		 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		 * io.setSourcePort(server, 8080);
		 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
		 * // An accepted connection comes as a ready event and does not require a separate setting up
		 * io.on(server, static_cast <awh::engine::callback::accept_t> ([&io](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
		 *     // Subscribing to the receiving of the data of the already accepted connection
		 *     io.on(cid, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
		 *         // Returning what was received back to the sender
		 *         io.send(id, buffer, size);
		 *     }));
		 * }));
		 * io.initialize();
		 * io.commit(server);
		 * io.listen(server, 1024);
		 * io.launch(server);
		 * while(io.poll(100));
		 * io.deinitialize();
		 * @endcode
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ IO : public engine_t {
			public:
				/**
				 * \~russian
				 * @brief Структура управления списками контроля доступа
				 *
				 * \~english
				 * @brief Structure of the management of the lists of the control of the access
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Control_List {
					private:
						// Объект работы с сетевыми адресами
						net_addr_t _addr;
					private:
						// Тип списка контроля доступа
						event::control_list_t _type;
					private:
						// Объект фреймворка
						const fmk_t * _fmk;
						// Объект работы с логами
						const log_t * _log;
					public:
						/**
						 * \~russian
						 * @brief Метод очистки контрольного списка события
						 *
						 * @param id идентификатор события
						 * @return   результат выполнения очистки
						 *
						 * \~english
						 * @brief Method of clearing the control list of an event
						 * @param id identifier of the event
						 * @return   result of the performance of the clearing
						 *
						 * \~
						 */
						bool clear(const event::id_t id) noexcept;
						/**
						 * \~russian
						 * @brief Метод добавления адреса в контрольный список события
						 *
						 * @param id    идентификатор события
						 * @param value значение адреса события
						 * @return      результат выполнения установки
						 *
						 * \~english
						 * @brief Method of adding an address into the control list of an event
						 * @param id    identifier of the event
						 * @param value value of the address of the event
						 * @return      result of the performance of the setting
						 *
						 * \~
						 */
						bool add(const event::id_t id, string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод удаления адреса из контрольного списка события
						 *
						 * @param id    идентификатор события
						 * @param value адрес для удаления из контрольного списка
						 * @return      результат выполнения удаления
						 *
						 * \~english
						 * @brief Method of removing an address from the control list of an event
						 * @param id    identifier of the event
						 * @param value address to remove from the control list
						 * @return      result of the performance of the removal
						 *
						 * \~
						 */
						bool remove(const event::id_t id, string_view value) noexcept;
						/**
						 * \~russian
						 * @brief Метод получения контрольного списка события
						 *
						 * @param id идентификатор события
						 * @return   контрольный список события
						 *
						 * \~english
						 * @brief Method of getting the control list of an event
						 * @param id identifier of the event
						 * @return   control list of the event
						 *
						 * \~
						 */
						const unordered_map <string, event::address_t> & get(const event::id_t id) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param type тип контрольного списка
						 *
						 * \~english
						 * @brief Constructor
						 * @param type type of the control list
						 *
						 * \~
						 */
						explicit Control_List(const event::control_list_t type, const fmk_t * fmk, const log_t * log) noexcept;
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
						virtual ~Control_List() noexcept;
				} control_list_t;
			public:
				// Объект управления белым списком
				control_list_t whitelist;
				// Объект управления чёрным списком
				control_list_t blacklist;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек события
				 *
				 * @details Закрепляет всё, что было выставлено событию после `event()`:
				 *          адреса, порты, опции сокета, размеры буферов, сроки. До этого
				 *          вызова настройки лежат в самом событии и до сокета не доходят, а
				 *          после - применены, и событие переходит из состояния «заведено» в
				 *          «инициализировано»
				 *
				 * @details Состояния события образуют последовательность, и каждый шаг
				 *          требует предыдущего:
				 *
				 *          | Вызов | Требует состояния | Оставляет состояние |
				 *          |---|---|---|
				 *          | `event()` | - | `NONE` |
				 *          | `commit()` | `NONE` | `INITIAL` |
				 *          | `connect()` | `INITIAL` | `SUCCESS` |
				 *          | `listen()` | `INITIAL` | `SUCCESS` |
				 *          | `launch()` | `INITIAL` или `SUCCESS` | `LAUNCHED` / `LISTENING` |
				 *
				 *          Отсюда следует, что `connect()` и `listen()` ставятся **между**
				 *          фиксацией и запуском, а не до фиксации и не после запуска.
				 *
				 * @note Повторная фиксация уже инициализированного события ничего не делает
				 *       и возвращает отрицательный результат: состояние `NONE` бывает у
				 *       события лишь однажды. Настройки, изменённые после фиксации,
				 *       применяются своими методами сразу, фиксации не требуя
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of the fixation of the settings of an event
				 * @details Fixes everything that has been set out to the event after `event()`:
				 *          the addresses, the ports, the options of the socket, the sizes of the buffers, the terms. Before this
				 *          call the settings lie in the event itself and do not reach the socket, and
				 *          after — they are applied, and the event passes from the state «started» into
				 *          «initialized»
				 * @details The states of an event form a sequence, and every step
				 *          requires the previous one:
				 *          | Call | Requires the state | Leaves the state |
				 *          |---|---|---|
				 *          | `event()` | - | `NONE` |
				 *          | `commit()` | `NONE` | `INITIAL` |
				 *          | `connect()` | `INITIAL` | `SUCCESS` |
				 *          | `listen()` | `INITIAL` | `SUCCESS` |
				 *          | `launch()` | `INITIAL` or `SUCCESS` | `LAUNCHED` / `LISTENING` |
				 *          Hence it follows that `connect()` and `listen()` are placed **between**
				 *          the fixation and the launch, and not before the fixation and not after the launch.
				 * @note A repeated fixation of an already initialized event does nothing
				 *       and returns a negative result: the `NONE` state happens at
				 *       an event only once. The settings changed after the fixation
				 *       are applied by their own methods at once, requiring no fixation
				 * @param id identifier of the event
				 * @return   result of the performance of the fixation
				 *
				 * \~
				 */
				bool commit(const event::id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод перестройки события: пересоздание нижележащего дескриптора с сохранением самого события
				 *
				 * @note Приложение работает с идентификатором события, а не с дескриптором,
				 *       поэтому дескриптор пересоздаётся, а событие (его идентификатор,
				 *       коллбэки, адрес/порт, опции и таймеры) сохраняется - подмена
				 *       дескриптора приложению незаметна. Всё состояние, живущее на
				 *       дескрипторе (регистрации (kqueue, epoll, ...), размеры буферов, DSCP/ECN/MTU,
				 *       интерфейс), снимается до закрытия и переприменяется на новый
				 *       дескриптор, а пройденные стадии подъёма (commit/listen/launch)
				 *       переигрываются по исходному статусу события. Поддерживается для
				 *       событий типа SERVER
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения перестройки
				 *
				 * \~english
				 * @brief Method of the rebuilding of an event: the recreation of the underlying descriptor with the preservation of the event itself
				 * @note The application works with the identifier of an event, and not with a descriptor,
				 *       and therefore the descriptor is recreated, and the event (its identifier,
				 *       the callbacks, the address/the port, the options and the timers) is preserved — the substitution
				 *       of the descriptor is imperceptible to the application. All the state living on
				 *       the descriptor (the registrations (kqueue, epoll, ...), the sizes of the buffers, the DSCP/ECN/MTU,
				 *       the interface) is removed before the closing and is reapplied to the new
				 *       descriptor, and the passed stages of the bringing up (commit/listen/launch)
				 *       are replayed by the original status of the event. Is supported for
				 *       the events of the SERVER type
				 * @param id identifier of the event
				 * @return   result of the performance of the rebuilding
				 *
				 * \~
				 */
				bool rebuild(const event::id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса события
				 *
				 * @param id идентификатор события
				 * @return   сетевой интерфейс события
				 *
				 * \~english
				 * @brief Method of getting the network interface of an event
				 * @param id identifier of the event
				 * @return   network interface of the event
				 *
				 * \~
				 */
				string getIface(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса события
				 *
				 * @param id   идентификатор события
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of an event
				 * @param id   identifier of the event
				 * @param name name of the network interface to set
				 * @return     result of the performance of the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t id, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения локального порта события
				 *
				 * @param id идентификатор события
				 * @return   локальный порт события
				 *
				 * \~english
				 * @brief Method of getting the local port of an event
				 * @param id identifier of the event
				 * @return   local port of the event
				 *
				 * \~
				 */
				uint16_t getSourcePort(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки локального порта события
				 *
				 * @param id   идентификатор события
				 * @param port локальный порт события
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the local port of an event
				 * @param id   identifier of the event
				 * @param port local port of the event
				 * @return     result of the performance of the setting
				 *
				 * \~
				 */
				bool setSourcePort(const event::id_t id, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения порта назначения события
				 *
				 * @param id идентификатор события
				 * @return   порт назначения события
				 *
				 * \~english
				 * @brief Method of getting the port of the destination of an event
				 * @param id identifier of the event
				 * @return   port of the destination of the event
				 *
				 * \~
				 */
				uint16_t getTargetPort(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта назначения события
				 *
				 * @param id   идентификатор события
				 * @param port порт назначения события
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the port of the destination of an event
				 * @param id   identifier of the event
				 * @param port port of the destination of the event
				 * @return     result of the performance of the setting
				 *
				 * \~
				 */
				bool setTargetPort(const event::id_t id, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param id идентификатор события
				 * @return   адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the address of the host of the target machine
				 * @param id identifier of the event
				 * @return   address of the host of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of the host of the target machine
				 * @param id     identifier of the event
				 * @param target address of the host of the target machine
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t id, string_view target) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the address of the host of the target machine
				 * @param id     identifier of the event
				 * @param target object to extract the address of the host of the target machine into
				 * @return       result of the performance of the extraction of the address of the host of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t id, unique_ptr <net::addr_t> & target) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of the host of the target machine
				 * @param id     identifier of the event
				 * @param target address of the host of the target machine
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t id, const net::addr_t * target) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @return        значение адреса события
				 *
				 * \~english
				 * @brief Method of getting the address of an event
				 * @param id      identifier of the event
				 * @param address type of the address of the event
				 * @return        value of the address of the event
				 *
				 * \~
				 */
				string getAddress(const event::id_t id, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   значение адреса события
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of an event
				 * @param id      identifier of the event
				 * @param address type of the address of the event
				 * @param value   value of the address of the event
				 * @return        result of the performance of the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t id, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   объект для извлечения адреса события
				 * @return        результат выполнения извлечения адреса события
				 *
				 * \~english
				 * @brief Method of getting the address of an event
				 * @param id      identifier of the event
				 * @param address type of the address of the event
				 * @param value   object to extract the address of the event into
				 * @return        result of the performance of the extraction of the address of the event
				 *
				 * \~
				 */
				bool getAddress(const event::id_t id, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   значение адреса события
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of an event
				 * @param id      identifier of the event
				 * @param address type of the address of the event
				 * @param value   value of the address of the event
				 * @return        result of the performance of the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t id, const event::address_t address, const net::addr_t * value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param id идентификатор события
				 * @return   MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of a network interface
				 * @param id identifier of the event
				 * @return   MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param id  идентификатор события
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of a network interface
				 * @param id  identifier of the event
				 * @param mtu size of the MTU of the interface
				 * @return    result of the setting of the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t id, const uint32_t mtu) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения признака выдачи системой поля Explicit Congestion Notification (ECN) принятых пакетов
				 *
				 * @details Отметку перегрузки пути ставит маршрутизатор в заголовок пакета,
				 *          а выдаёт её принимающему уже ядро - служебным сообщением при
				 *          приёме дейтаграммы. Выдают её не все системы: NetBSD и OpenBSD
				 *          по IPv4 не выдают вовсе, и опции запроса такой выдачи у них не
				 *          заведено. По IPv6 выдают обе
				 *
				 *          Вызывающему это знать необходимо. Обмен, помечающий свои пакеты
				 *          поддержкой отметок и не получающий отметок обратно, обязан по
				 *          договору признать проверку несостоявшейся и отметки отключить -
				 *          то есть проделать лишний круг там, где исход известен заранее
				 *
				 * @note Признак решается наличием средства запроса выдачи, а не перечнем
				 *       систем поимённо: перечень устареет с первым же выпуском, который
				 *       средство добавит
				 *
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       признак выдачи системой отметок перегрузки пути
				 *
				 * \~english
				 * @brief Method of getting the sign of the yielding by the system of the Explicit Congestion Notification (ECN) field of the received packets
				 * @details The mark of the congestion of a path is placed by a router into the header of a packet,
				 *          and it is yielded to the receiving side already by the kernel — by a service message at
				 *          the reception of a datagram. Not all the systems yield it: NetBSD and OpenBSD
				 *          over IPv4 do not yield it at all, and the option of the request of such a yielding is not
				 *          started at them. Over IPv6 both yield it
				 *          The caller needs to know this. An exchange marking its packets
				 *          with the support of the marks and not receiving the marks back is obliged by
				 *          the contract to recognize the check as not having taken place and to switch the marks off —
				 *          that is to make an extra round where the outcome is known in advance
				 * @note The sign is decided by the presence of the means of the request of the yielding, and not by a list
				 *       of the systems by name: a list will become obsolete with the very first release which
				 *       adds the means
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       sign of the yielding by the system of the marks of the congestion of a path
				 *
				 * \~
				 */
				bool availableExplicitCongestionNotification(const event::family_t family) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param id     identifier of the event
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       value of the DSCP
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param dscp   значение DSCP
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param id     identifier of the event
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param dscp   value of the DSCP
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Выдаёт значение, устанавливаемое на исходящих пакетах. Признак
				 *       перегрузки принятых пакетов приходит отдельно для каждой
				 *       датаграммы и извлекается методом getTrafficInfo
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение ECN
				 *
				 * \~english
				 * @brief Method of getting the value of the Explicit Congestion Notification (ECN) field in the header of an IP packet
				 * @note Yields the value set on the outgoing packets. The sign of
				 *       the congestion of the received packets comes separately for every
				 *       datagram and is extracted by the getTrafficInfo method
				 * @param id     identifier of the event
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       value of the ECN
				 *
				 * \~
				 */
				event::ecn_t getExplicitCongestionNotification(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Класс обслуживания (DSCP) сохраняется: оба поля занимают один
				 *       октет заголовка, поэтому установка затрагивает только младшие
				 *       два бита
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ecn    значение ECN
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Explicit Congestion Notification (ECN) field in the header of an IP packet
				 * @note The class of the service (DSCP) is preserved: both fields occupy one
				 *       octet of the header, and therefore the setting touches only the lower
				 *       two bits
				 * @param id     identifier of the event
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param ecn    value of the ECN
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setExplicitCongestionNotification(const event::id_t id, const event::family_t family, const event::ecn_t ecn) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       режим обнаружения максимального размера пакета (MTU)
				 *
				 * \~english
				 * @brief Method of getting the discovery of the maximum size of a packet (MTU)
				 * @param id     identifier of the event
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @return       mode of the discovery of the maximum size of a packet (MTU)
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим обнаружения максимального размера пакета (MTU)
				 * @return       результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery of the maximum size of a packet (MTU)
				 * @param id     identifier of the event
				 * @param family family of the protocols (IPv4 or IPv6)
				 * @param mode   mode of the discovery of the maximum size of a packet (MTU)
				 * @return       result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @details Метод привязывает дескриптор к порту группы: без этого приём
				 *          из группы невозможен, а `commit` такой привязки не делает - он
				 *          привязывает свою точку узла, а не точку группы.
				 *
				 *          Оттого **звать его следует до `commit`**. После `commit`
				 *          дескриптор уже привязан, привязка пропускается, и выполняется
				 *          одна лишь подписка на группу - для приёма этого довольно лишь
				 *          тогда, когда узел и без того привязан к нужному порту.
				 *
				 *          Отказ метода состояния события не меняет: событие остаётся
				 *          тем, чем было, и продолжает работать
				 *
				 * @param id     идентификатор события
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of the activation/deactivation of the multicast group of an event
				 * @details The method binds the descriptor to the port of the group: without this the reception
				 *          from the group is not possible, and `commit` does not make such a binding — it
				 *          binds the point of its own node, and not the point of the group.
				 *          Therefore **it should be called before `commit`**. After `commit`
				 *          the descriptor is already bound, the binding is skipped, and
				 *          the subscription to the group alone is performed — for the reception this is enough only
				 *          when the node is bound to the needed port anyway.
				 *          A refusal of the method does not change the state of the event: the event remains
				 *          what it was, and continues to work
				 * @param id     identifier of the event
				 * @param mode   mode of the activation/deactivation
				 * @param group  multicast group for the activation/deactivation
				 * @param source address of the network interface the subscription is performed from
				 * @param port   port of the multicast group the subscription is performed from
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t id, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @details Метод привязывает дескриптор к порту группы: без этого приём
				 *          из группы невозможен, а `commit` такой привязки не делает - он
				 *          привязывает свою точку узла, а не точку группы.
				 *
				 *          Оттого **звать его следует до `commit`**. После `commit`
				 *          дескриптор уже привязан, привязка пропускается, и выполняется
				 *          одна лишь подписка на группу - для приёма этого довольно лишь
				 *          тогда, когда узел и без того привязан к нужному порту.
				 *
				 *          Отказ метода состояния события не меняет: событие остаётся
				 *          тем, чем было, и продолжает работать
				 *
				 * @param id     идентификатор события
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of the activation/deactivation of the multicast group of an event
				 * @details The method binds the descriptor to the port of the group: without this the reception
				 *          from the group is not possible, and `commit` does not make such a binding — it
				 *          binds the point of its own node, and not the point of the group.
				 *          Therefore **it should be called before `commit`**. After `commit`
				 *          the descriptor is already bound, the binding is skipped, and
				 *          the subscription to the group alone is performed — for the reception this is enough only
				 *          when the node is bound to the needed port anyway.
				 *          A refusal of the method does not change the state of the event: the event remains
				 *          what it was, and continues to work
				 * @param id     identifier of the event
				 * @param mode   mode of the activation/deactivation
				 * @param group  multicast group for the activation/deactivation
				 * @param source address of the network interface the subscription is performed from
				 * @param port   port of the multicast group the subscription is performed from
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t id, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод привязки дополнительного ключа маршрутизации к сессии
				 *
				 * @note Одна сессия адресуется произвольным числом ключей: протоколы,
				 *       меняющие идентификатор по ходу работы, обращаются к ней по
				 *       любому из привязанных. Ключи снимаются автоматически при
				 *       уничтожении сессии
				 *
				 * @param id  идентификатор события сессии
				 * @param key привязываемый ключ сессии
				 * @return    результат привязки (false - ключ занят другой сессией)
				 *
				 * \~english
				 * @brief Method of binding an additional key of the routing to a session
				 * @note One session is addressed by an arbitrary number of the keys: the protocols
				 *       changing the identifier in the course of the work address it by
				 *       any of the bound ones. The keys are removed automatically at
				 *       the destruction of the session
				 * @param id  identifier of the event of the session
				 * @param key bound key of the session
				 * @return    result of the binding (false — the key is taken by another session)
				 *
				 * \~
				 */
				bool bind(const event::id_t id, const net::origin_key_t & key) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия ключа маршрутизации с сессии
				 *
				 * @param id  идентификатор события сессии
				 * @param key снимаемый ключ сессии
				 * @return    результат снятия (false - ключ сессии не принадлежит)
				 *
				 * \~english
				 * @brief Method of removing a key of the routing from a session
				 * @param id  identifier of the event of the session
				 * @param key removed key of the session
				 * @return    result of the removal (false — the key does not belong to the session)
				 *
				 * \~
				 */
				bool unbind(const event::id_t id, const net::origin_key_t & key) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения предельного количества одновременных подключений события
				 *
				 * @param id идентификатор события
				 * @return   предельное количество одновременных подключений
				 *
				 * \~english
				 * @brief Method of getting the limiting number of the simultaneous connections of an event
				 * @param id identifier of the event
				 * @return   limiting number of the simultaneous connections
				 *
				 * \~
				 */
				uint32_t getMaxConnections(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки предельного количества одновременных подключений события
				 *
				 * @note Для потоковых событий ограничивает число принятых подключений,
				 *       для дейтаграммных - число сессий. Достижение предела означает
				 *       отказ в создании новой сессии, поэтому предел служит защитой
				 *       от исчерпания памяти потоком датаграмм от чужих отправителей
				 *
				 * @param id  идентификатор события
				 * @param max предельное количество одновременных подключений
				 * @return    результат установки
				 *
				 * \~english
				 * @brief Method of setting the limiting number of the simultaneous connections of an event
				 * @note For the stream events it limits the number of the accepted connections,
				 *       for the datagram ones — the number of the sessions. The reaching of the limit means
				 *       a refusal in the creation of a new session, and therefore the limit serves as a protection
				 *       from the exhaustion of the memory by a stream of the datagrams from the foreign senders
				 * @param id  identifier of the event
				 * @param max limiting number of the simultaneous connections
				 * @return    result of the setting
				 *
				 * \~
				 */
				bool setMaxConnections(const event::id_t id, const uint32_t max) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удаления события
				 *
				 * @details Освобождает событие из любого состояния: настраиваемого,
				 *          работающего, приостановленного. Снимает таймауты, закрывает
				 *          дескриптор и убирает событие из опроса.
				 *
				 *          Освобождение выполняется **не сразу**, а откладывается на
				 *          два оборота цикла событий. Отсрочка нужна по необходимости,
				 *          а не для удобства: записи подписки уходят в ядро вместе с
				 *          ожиданием следующего оборота, и закрой движок дескриптор
				 *          раньше, его номер операционная система успела бы выдать
				 *          другому объекту - записи легли бы на чужой дескриптор.
				 *
				 *          Для вызывающего отсрочка незаметна: обращения по этому
				 *          идентификатору перестают действовать сразу, а функции
				 *          обратного вызова по нему больше не приходят.
				 *
				 * @note    Вызывать изнутри функции обратного вызова этого же события
				 *          **допустимо и безопасно** - ровно из-за отсрочки. Это
				 *          обычный способ закрыть соединение по ошибке разбора или по
				 *          завершении обмена.
				 *
				 * @note    Повторный вызов по тому же идентификатору отказывает, а не
				 *          освобождает узел дважды: событие уже помечено, и найти его
				 *          по идентификатору больше нельзя.
				 *
				 * @note    Освобождение события сервера не освобождает принятые им
				 *          подключения - у каждого свой идентификатор и свой срок
				 *          жизни. Закрывать их следует своими вызовами
				 *
				 * @par Пример: закрытие соединения из обратного вызова
				 * @param id идентификатор события
				 * @return   результат выполнения удаления
				 *
				 * @code{.cpp}
				 * io.on(client, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
				 *     // Если разбор принятых данных не удался, закрываем соединение
				 *     if(!parse(buffer, size))
				 *         // Освобождение отложится на два оборота цикла и выполнится безопасно
				 *         io.destroy(id);
				 * }));
				 * @endcode
				 *
				 * \~english
				 * @brief Method of removing an event
				 * @details Releases an event from any state: from a set up one, from
				 *          a working one, from a paused one. Removes the timeouts, closes
				 *          the descriptor and takes the event out of the polling.
				 *          The release is performed **not at once**, but is postponed for
				 *          two turns of the loop of the events. The delay is needed by the necessity,
				 *          and not for the convenience: the records of the subscription go into the kernel together with
				 *          the waiting for the next turn, and were the engine to close the descriptor
				 *          earlier, its number the operating system would manage to give out
				 *          to another object — the records would fall onto a foreign descriptor.
				 *          For the caller the delay is imperceptible: the addresses by this
				 *          identifier stop acting at once, and the callback
				 *          functions by it no longer come.
				 * @note    Calling it from inside the callback function of this same event
				 *          is **admissible and safe** — exactly because of the delay. This is
				 *          the ordinary way of closing a connection at an error of the parsing or at
				 *          the completion of an exchange.
				 * @note    A repeated call by the same identifier refuses, and does not
				 *          release the node twice: the event is already marked, and finding it
				 *          by the identifier is no longer possible.
				 * @note    The release of an event of a server does not release the connections accepted by it
				 *          — each has its own identifier and its own term of
				 *          the life. They should be closed by one's own calls
				 * @par Example: the closing of a connection from a callback
				 * @param id identifier of the event
				 * @return   result of the performance of the removal
				 *
				 * @code{.cpp}
				 * io.on(client, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
				 *     // If the parsing of the received data failed, closing the connection
				 *     if(!parse(buffer, size))
				 *         // The release will be deferred by two turns of the loop and will be performed safely
				 *         io.destroy(id);
				 * }));
				 * @endcode
				 *
				 */
				bool destroy(const event::id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения пары событий для сокета
				 *
				 * @param family   семейство адресов
				 * @param type     тип сокета
				 * @param protocol протокол сокета
				 * @return         пара идентификаторов созданных событий
				 *
				 * \~english
				 * @brief Method of getting a pair of the events for a socket
				 * @param family   family of the addresses
				 * @param type     type of the socket
				 * @param protocol protocol of the socket
				 * @return         pair of the identifiers of the created events
				 *
				 * \~
				 */
				std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания нового события
				 *
				 * @details Первый шаг работы с событием. Заводит узел события,
				 *          создаёт под него дескриптор операционной системы и
				 *          переводит событие в состояние `INITIAL`, в котором оно
				 *          принимает настройки, но ещё не работает.
				 *
				 *          Дескриптор создаётся **сразу**, а не при запуске, потому
				 *          что настройка события - адреса, порты, опции - выполняется
				 *          над готовым дескриптором. Отсюда следует, что созданное и
				 *          брошенное событие удерживает дескриптор до `destroy()`.
				 *
				 *          Тип узла определяет, чем событие будет: `CLIENT` и
				 *          `SERVER` - сокеты, `PEER` заводится движком сам при приёме
				 *          подключения, `TIMEOUT` и `INTERVAL` - таймеры, `FILE` и
				 *          `DIR` - наблюдение за файловой системой, `IPC` -
				 *          межпроцессное взаимодействие, `NOTIFY` - пользовательское
				 *          событие для передачи работы в поток цикла.
				 *
				 * @note    Для таймеров семейство адресов задаётся значением
				 *          `event::family_t::TIMER`, а тип и протокол не нужны вовсе.
				 *
				 * @note    Событие `PEER` через этот метод не создаётся: принятые
				 *          подключения движок заводит сам и отдаёт их идентификатор
				 *          в функцию обратного вызова приёма подключения уже готовым
				 *          и подписанным на чтение.
				 *
				 * @note    Нулевой идентификатор означает отказ создания. Проверять
				 *          его следует до настройки: обращения по недействительному
				 *          идентификатору молча ничего не делают, и без проверки
				 *          отказ обнаружился бы только отсутствием событий
				 *
				 * @par Пример: таймер
				 * @param node     узел события
				 * @param family   семейство адресов
				 * @param type     тип сокета
				 * @param protocol протокол сокета
				 * @return         идентификатор созданного события, нулевой при отказе
				 *
				 * @code{.cpp}
				 * // Заводим событие таймера и задаём ему задержку в две секунды
				 * const awh::event::id_t timer = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 2000);
				 * io.commit(timer);
				 * io.launch(timer);
				 * @endcode
				 *
				 * \~english
				 * @brief Method of creating a new event
				 * @details The first step of the work with an event. Starts a node of an event,
				 *          creates a descriptor of the operating system for it and
				 *          puts the event into the `INITIAL` state, in which it
				 *          takes the settings, but does not work yet.
				 *          The descriptor is created **at once**, and not at the launch, because
				 *          the setup of an event — the addresses, the ports, the options — is performed
				 *          over a ready descriptor. Hence it follows that a created and
				 *          abandoned event holds a descriptor until `destroy()`.
				 *          The type of the node determines what the event will be: `CLIENT` and
				 *          `SERVER` — the sockets, `PEER` is started by the engine itself at the acceptance
				 *          of a connection, `TIMEOUT` and `INTERVAL` — the timers, `FILE` and
				 *          `DIR` — the observation of the file system, `IPC` —
				 *          the interprocess communication, `NOTIFY` — a user
				 *          event for the passing of the work into the thread of the loop.
				 * @note    For the timers the family of the addresses is set by the value
				 *          `event::family_t::TIMER`, and the type and the protocol are not needed at all.
				 * @note    A `PEER` event is not created through this method: the accepted
				 *          connections the engine starts itself and gives their identifier
				 *          into the callback function of the acceptance of a connection already ready
				 *          and subscribed to the reading.
				 * @note    A zero identifier means a refusal of the creation. It should be checked
				 *          before the setup: the addresses by an invalid
				 *          identifier silently do nothing, and without the check
				 *          the refusal would be discovered only by the absence of the events
				 * @par Example: a timer
				 * @param node     node of the event
				 * @param family   family of the addresses
				 * @param type     type of the socket
				 * @param protocol protocol of the socket
				 * @return         identifier of the created event, a zero one at a refusal
				 *
				 * @code{.cpp}
				 * // Starting an event of a timer and setting it a delay of two seconds
				 * const awh::event::id_t timer = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 2000);
				 * io.commit(timer);
				 * io.launch(timer);
				 * @endcode
				 *
				 */
				event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения смещения в файле события
				 *
				 * @param id   идентификатор события
				 * @param seek тип смещения в файле события
				 * @return     смещение в файле события
				 *
				 * \~english
				 * @brief Method of getting the offset in the file of an event
				 * @param id   identifier of the event
				 * @param seek type of the offset in the file of the event
				 * @return     offset in the file of the event
				 *
				 * \~
				 */
				size_t getSeek(const event::id_t id, const event::seek_t seek) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки смещения в файле события
				 *
				 * @param id     идентификатор события
				 * @param seek   тип смещения в файле события
				 * @param offset смещение в файле события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the offset in the file of an event
				 * @param id     identifier of the event
				 * @param seek   type of the offset in the file of the event
				 * @param offset offset in the file of the event
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool setSeek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций события
				 *
				 * @param id идентификатор события
				 * @return   опции события
				 *
				 * \~english
				 * @brief Method of getting the options of an event
				 * @param id identifier of the event
				 * @return   options of the event
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций события
				 *
				 * @param id      идентификатор события
				 * @param options опции события для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the options of an event
				 * @param id      identifier of the event
				 * @param options options of the event to set
				 * @return        result of the performance of the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t id, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции события
				 *
				 * @param id     идентификатор события
				 * @param option опция события для установки
				 * @param mode   режим установки опции события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting an option of an event
				 * @param id     identifier of the event
				 * @param option option of the event to set
				 * @param mode   mode of the setting of the option of the event
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t id, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод запуска события
				 *
				 * @details Последний шаг подготовки: с него событие начинает участвовать в
				 *          опросе. До запуска событие заведено, настроено и, возможно,
				 *          подключено, но обратные вызовы ему не приходят.
				 *
				 *          Метод различает **два пути** по состоянию события, и оба
				 *          законны. Из состояния `INITIAL` запускается событие, которому
				 *          подключаться не нужно: таймер, наблюдение за файлом,
				 *          дейтаграммный сокет. Из состояния `SUCCESS` - событие, прошедшее
				 *          через `connect()` или `listen()`; для него запуск заодно
				 *          применяет накопившиеся изменения к ядру.
				 *
				 * @note Дейтаграммный сервер можно запускать и без `listen()`: слушать
				 *       очередь входящих соединений ему незачем. Потоковому `listen()`
				 *       обязателен, иначе запуск откажет
				 *
				 * @note Повторный запуск уже запущенного события отказывает: требуемых
				 *       состояний у него больше нет
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения запуска
				 *
				 * \~english
				 * @brief Method of the launch of an event
				 * @details The last step of the preparation: from it the event begins to participate in
				 *          the polling. Before the launch the event is started, set up and, possibly,
				 *          connected, but the callbacks do not come to it.
				 *          The method tells apart **two paths** by the state of the event, and both
				 *          are lawful. From the `INITIAL` state an event is launched that need not
				 *          connect: a timer, an observation of a file,
				 *          a datagram socket. From the `SUCCESS` state — an event that has passed
				 *          through `connect()` or `listen()`; for it the launch at the same time
				 *          applies the accumulated changes to the kernel.
				 * @note A datagram server may be launched without `listen()` as well: it has no reason to listen
				 *       to the queue of the incoming connections. For a stream one `listen()`
				 *       is obligatory, otherwise the launch will refuse
				 * @note A repeated launch of an already launched event refuses: it no longer has
				 *       the required states
				 * @param id identifier of the event
				 * @return   result of the performance of the launch
				 *
				 * \~
				 */
				bool launch(const event::id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отключения события
				 *
				 * @details Разрывает соединение: закрывает дескриптор и переводит событие в
				 *          состояние отмены, после чего вызывается подписка `event_t` с
				 *          действием `DISCONNECT`. Само событие при этом **остаётся
				 *          живым** - его идентификатор действителен, подписки сохранены.
				 *          Этим отключение и отличается от `destroy()`, который событие
				 *          уничтожает.
				 *
				 * @note Дескриптор закрыт, поэтому просто запустить событие снова нельзя:
				 *       вернуть его в работу можно через `rebirth()`, пересоздающий
				 *       дескриптор с сохранением самого события
				 *
				 * @note Отключение должно быть событию разрешено соответствующим действием.
				 *       Если оно запрещено, метод молча ничего не делает и возвращает
				 *       отрицательный результат
				 *
				 * @note Событие, уже помеченное к уничтожению или отключённое, повторно не
				 *       отключается
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения отключения
				 *
				 * \~english
				 * @brief Method of the disconnection of an event
				 * @details Breaks the connection: closes the descriptor and puts the event into
				 *          the state of the cancellation, after which the `event_t` subscription with
				 *          the action `DISCONNECT` is called. The event itself at that **remains
				 *          alive** — its identifier is valid, the subscriptions are preserved.
				 *          By this the disconnection differs from `destroy()`, which destroys
				 *          the event.
				 * @note The descriptor is closed, and therefore simply launching the event again is not possible:
				 *       returning it into the work is possible through `rebirth()`, recreating
				 *       the descriptor with the preservation of the event itself
				 * @note The disconnection must be allowed to the event by the corresponding action.
				 *       If it is forbidden, the method silently does nothing and returns
				 *       a negative result
				 * @note An event already marked for the destruction or disconnected is not disconnected
				 *       once more
				 * @param id identifier of the event
				 * @return   result of the performance of the disconnection
				 *
				 * \~
				 */
				bool disconnect(const event::id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Шаблон метода мультиподключения события к удалённым хостам
				 *
				 * @tparam Args список идентификаторов событий для подключения
				 *
				 * \~english
				 * @brief Template of the method of the multi-connection of an event to the remote hosts
				 * @tparam Args list of the identifiers of the events to connect
				 *
				 * \~
				 */
				template <typename... Args>
				/**
				 * \~russian
				 * @brief Метод мультиподключения события к удалённым хостам
				 *
				 * @param args список идентификаторов событий для подключения
				 * @return     результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of the multi-connection of an event to the remote hosts
				 * @param args list of the identifiers of the events to connect
				 * @return     result of the performance of the connection
				 *
				 * \~
				 */
				bool connect(Args&&... args) noexcept {
					// Выполняем подключение к списку удалённых серверов
					return this->connect({args...});
				}
				/**
				 * \~russian
				 * @brief Метод мультиподключения события к удалённым хостам
				 *
				 * @details Начинает подключение и **возвращается сразу**, не дожидаясь его
				 *          исхода: соединение на неблокирующем сокете устанавливается за
				 *          несколько оборотов цикла. Положительный результат означает лишь
				 *          то, что попытка начата успешно. Об исходе сообщает подписка
				 *          `connect_t`, и до её вызова отправлять данные некуда.
				 *
				 *          Список идентификаторов заведён ради **многодомности SCTP**: адреса
				 *          всех перечисленных событий укладываются в одну заявку `sctp_connectx`,
				 *          и получается ОДНА связь с узлом, у которого несколько адресов. Отчёт
				 *          придёт единственный - по первому событию списка; остальные события
				 *          служат лишь носителями адресов, своих подключений и своих вызовов
				 *          `connect_t` у них не будет.
				 *
				 *          Для прочих протоколов многодомности не существует, и список
				 *          обходится **по очереди**: каждое событие подключается само за себя
				 *          и отчитывается своим вызовом `connect_t`. Список принимается ими
				 *          только ради того, чтобы возможность не была мёртвой. Датаграммы идут
				 *          здесь наравне с потоком: подключения у них нет, но движок его
				 *          изображает, закрепляя получателя, и отчёт приходит такой же. Порядок
				 *          отчётов при этом задаётся готовностью соединений, а не порядком
				 *          в списке, и полагаться на него нельзя.
				 *
				 *          Подключаются **только узлы клиента**; всякий иной узел в списке
				 *          отбрасывается предупреждением. У SCTP, где список сливается в одну
				 *          заявку, образцом берётся ПЕРВЫЙ узел: события чужого протокола и
				 *          чужого семейства адресов отбрасываются предупреждением, потому что
				 *          связь одна и разрешить неоднозначность иначе нечем. У прочих
				 *          протоколов ни то, ни другое помехой не является - у каждого события
				 *          своя точка назначения и своё подключение.
				 *
				 * @warning Одновременности здесь нет ни у одного протокола: подключения идут
				 *          последовательно в одном потоке. Параллельность достижима только
				 *          потоками и этим методом не даётся
				 *
				 * @note Ставится **между** `commit()` и `launch()`: до фиксации адрес ещё не
				 *       применён, а запуск ожидает событие уже подключающимся
				 *
				 * @note Предел времени на установление соединения задаётся через
				 *       `setTimeout()` с действием `CONNECT`. Без него неудачная попытка
				 *       может висеть столько, сколько отведёт система
				 *
				 * @par Пример: клиент
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 * @code{.cpp}
				 * io.on(client, static_cast <awh::engine::callback::connect_t> ([&io](const awh::event::id_t id, const bool ok) noexcept -> void {
				 *     // Отправлять можно только отсюда: раньше соединения ещё нет
				 *     if(ok)
				 *         io.send(id, request.data(), request.size());
				 * }));
				 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
				 * if(io.commit(client) && io.connect(client) && io.launch(client))
				 *     while(io.poll(100));
				 * @endcode
				 *
				 * \~english
				 * @brief Method of the multi-connection of an event to the remote hosts
				 * @details Begins a connection and **returns at once**, without waiting for its
				 *          outcome: a connection on a non-blocking socket is established over
				 *          several turns of the loop. A positive result means only
				 *          that the attempt is begun successfully. About the outcome the `connect_t`
				 *          subscription reports, and before its call there is nowhere to send the data.
				 *
				 *          The list of the identifiers is made for the sake of the **multihoming of
				 *          SCTP**: the addresses of all the listed events are packed into one request
				 *          `sctp_connectx`, and ONE association with a host having several addresses
				 *          is obtained. The report will come single — by the first event of the list;
				 *          the rest of the events serve only as the carriers of the addresses, they
				 *          will have neither their own connections nor their own calls of `connect_t`.
				 *
				 *          For the other protocols no multihoming exists, and the list is passed
				 *          **in turn**: every event connects for itself and reports by its own call
				 *          of `connect_t`. The list is accepted by them only for the sake of the
				 *          possibility not being dead. The datagrams go here on a par with the
				 *          stream: they have no connection, but the engine imitates it, fixing the
				 *          receiver, and the report comes the same. The order of the reports is set by
				 *          the readiness of the connections, and not by the order in the list, and
				 *          it is impossible to rely upon it.
				 *          Only the client nodes are connected; every other node in the list is
				 *          dropped with a warning. At SCTP, where the list merges into one request,
				 *          the FIRST node is taken as the pattern: the events of a foreign protocol
				 *          and of a foreign address family are dropped with a warning, because the
				 *          association is one and there is nothing else to resolve the ambiguity by.
				 *          At the other protocols neither of the two is a hindrance — every event
				 *          has its own destination and its own connection.
				 * @warning There is no simultaneity here at any protocol: the connections go
				 *          sequentially in one thread. The parallelism is achievable only by the
				 *          threads and is not given by this method
				 * @note Is placed **between** `commit()` and `launch()`: before the fixation the address is not yet
				 *       applied, and the launch expects the event to be already connecting
				 * @note The limit of the time for the establishment of a connection is set through
				 *       `setTimeout()` with the action `CONNECT`. Without it an unsuccessful attempt
				 *       may hang for as long as the system allots
				 * @par Example: a client
				 * @param ids list of the identifiers of the events to connect
				 * @return    result of the performance of the connection
				 *
				 * @code{.cpp}
				 * io.on(client, static_cast <awh::engine::callback::connect_t> ([&io](const awh::event::id_t id, const bool ok) noexcept -> void {
				 *     // Sending is possible only from here: earlier the connection does not exist yet
				 *     if(ok)
				 *         io.send(id, request.data(), request.size());
				 * }));
				 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
				 * if(io.commit(client) && io.connect(client) && io.launch(client))
				 *     while(io.poll(100));
				 * @endcode
				 *
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перевода события в режим прослушивания входящих соединений
				 *
				 * @details Открывает очередь входящих соединений: с этого момента ядро
				 *          принимает их и складывает в очередь, а разбирать её событие
				 *          начнёт с вызова `launch()`. Второй параметр задаёт предел
				 *          одновременно ожидающих соединений - тот самый backlog.
				 *
				 * @note Требуется **только потоковым** серверам. Дейтаграммному серверу
				 *       очередь соединений не нужна, и он обходится одним `launch()`
				 *
				 * @note Принятые соединения приходят подпиской `accept_t` уже заведёнными
				 *       событиями, и заводить их своими вызовами не требуется
				 *
				 * @par Пример: потоковый сервер
				 * @param id  идентификатор события
				 * @param max максимальное количество входящих соединений
				 * @return    результат выполнения перевода в режим прослушивания
				 *
				 * @code{.cpp}
				 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
				 * io.setSourcePort(server, 8080);
				 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
				 * io.on(server, static_cast <awh::engine::callback::accept_t> (onAccept));
				 * // Фиксация, очередь входящих, запуск - именно в этом порядке
				 * if(io.commit(server) && io.listen(server, 1024) && io.launch(server))
				 *     while(io.poll(100));
				 * @endcode
				 *
				 * \~english
				 * @brief Method of putting an event into the mode of the listening for the incoming connections
				 * @details Opens the queue of the incoming connections: from this moment the kernel
				 *          accepts them and puts them into the queue, and the event will begin to disassemble it
				 *          from the call of `launch()`. The second parameter sets the limit of
				 *          the simultaneously waiting connections — that very backlog.
				 * @note Is required **only by the stream** servers. A datagram server does not need
				 *       a queue of the connections, and it gets by with one `launch()`
				 * @note The accepted connections come by the `accept_t` subscription as already started
				 *       events, and starting them by one's own calls is not required
				 * @par Example: a stream server
				 * @param id  identifier of the event
				 * @param max maximum number of the incoming connections
				 * @return    result of the performance of the putting into the mode of the listening
				 *
				 * @code{.cpp}
				 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
				 * io.setSourcePort(server, 8080);
				 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
				 * io.on(server, static_cast <awh::engine::callback::accept_t> (onAccept));
				 * // The commit, the queue of the incoming ones, the start — exactly in this order
				 * if(io.commit(server) && io.listen(server, 1024) && io.launch(server))
				 *     while(io.poll(100));
				 * @endcode
				 *
				 */
				bool listen(const event::id_t id, const uint32_t max) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения данных события
				 *
				 * @param id идентификатор события
				 * @return   результат получения данных
				 *
				 * \~english
				 * @brief Method of getting the data of an event
				 * @param id identifier of the event
				 * @return   result of the getting of the data
				 *
				 * \~
				 */
				bool recv(const event::id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных события
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных событием
				 *
				 * \~english
				 * @brief Method of sending the data of an event
				 * @param id     identifier of the event
				 * @param buffer buffer of the data to send
				 * @param size   size of the data to send
				 * @return       number of the bytes of the data sent by the event
				 *
				 * \~
				 */
				size_t send(const event::id_t id, const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод перенаправления объединённых данных в событие-приёмник (splice)
				 *
				 * @note Если на событии-приёмнике установлена функция инъекции (транспорт
				 *       шифрует данные на уровне соединения, напр. QUIC), данные передаются
				 *       ей для отправки собственным потоком; иначе выполняется обычная
				 *       отправка байт в сокет
				 *
				 * @note Дейтаграммный приёмник отправляет каждую запись очереди
				 *       отдельным сообщением, и запись, превышающую предел системы,
				 *       отправить нельзя вовсе. Такие данные переносятся частями:
				 *       порция источника устроена иначе, чем сообщение приёмника -
				 *       файл, например, читается страницами, которые предельную
				 *       дейтаграмму превышают. Делится только то, что иначе не
				 *       прошло бы ни одним октетом, поэтому границы сообщений у
				 *       проходящих целиком дейтаграмм сохраняются
				 *
				 * @param id     идентификатор события-приёмника
				 * @param buffer буфер перенаправляемых данных
				 * @param size   размер перенаправляемых данных
				 * @return       количество принятых на перенаправление байт
				 *
				 * \~english
				 * @brief Method of the redirection of the joined data into a receiver event (splice)
				 * @note If a function of the injection is set on the receiver event (the transport
				 *       encrypts the data at the level of the connection, e.g. QUIC), the data is passed
				 *       to it for the sending by its own stream; otherwise the ordinary
				 *       sending of the bytes into the socket is performed
				 * @note A datagram receiver sends every record of the queue
				 *       as a separate message, and a record exceeding the limit of the system
				 *       cannot be sent at all. Such data is carried in the parts:
				 *       a portion of the source is arranged otherwise than a message of the receiver —
				 *       a file, for example, is read by the pages, which exceed the limiting
				 *       datagram. Divided is only what would otherwise not pass
				 *       by a single octet, and therefore the boundaries of the messages at
				 *       the datagrams passing entirely are preserved
				 * @param id     identifier of the receiver event
				 * @param buffer buffer of the redirected data
				 * @param size   size of the redirected data
				 * @return       number of the bytes accepted for the redirection
				 *
				 * \~
				 */
				size_t relay(const event::id_t id, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод объединения данных между событиями
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения объединения
				 *
				 * \~english
				 * @brief Method of the joining of the data between the events
				 * @param eid  identifier of the source event
				 * @param dest identifier of the receiver event
				 * @return     result of the performance of the joining
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод подъёма события из снимка, снятого чужим процессом
				 *
				 * @details Заменяет собой `commit` у события, заведённого обычным путём:
				 *          `commit` завёл бы событию своё устройство, а здесь оно берётся
				 *          готовым - тем самым, какое передал чужой процесс
				 *
				 * @note Устройство события обязано совпадать с тем, какое было снято:
				 *       семейство, тип и протокол задаются при заведении события, и
				 *       расхождение с содержимым снимка движок отвергает отказом
				 *
				 * @warning Снимок годен ОДНОМУ подъёму и ОДНОМУ процессу - тому, который
				 *          был назван при снятии
				 *
				 * @param id       идентификатор заведённого события, которому достаётся снимок
				 * @param snapshot снимок события, снятый чужим процессом
				 * @param size     размер снимка события
				 * @return         результат подъёма события из снимка
				 *
				 * \~english
				 * @brief Method of the raising of an event from a snapshot taken by a foreign process
				 * @param id       identifier of the created event which receives the snapshot
				 * @param snapshot snapshot of the event taken by a foreign process
				 * @param size     size of the snapshot of the event
				 * @return         result of the raising of the event from the snapshot
				 *
				 * \~
				 */
				bool restore(const event::id_t id, const void * snapshot, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия переносимого снимка события для чужого процесса
				 *
				 * @details Снимок позволяет отдать УЖЕ ЗАВЕДЁННОЕ событие другому процессу:
				 *          тот поднимает у себя событие того же самого устройства методом
				 *          `restore`, минуя `commit`. Процесс-получатель называется не
				 *          номером, а событием `dest`, ведущим к нему: парой обмена,
				 *          подключением домена UNIX, принятым подключением - всяким
				 *          событием, у которого встречный конец находится в чужом процессе
				 *
				 * @note Содержимое снимка непрозрачно: движок волен вложить в него как
				 *       само описание события, так и одну лишь метку получения, отправив
				 *       событие обочиной по событию `dest`. Довозит снимок до получателя
				 *       вызывающая сторона - своим уговором и своей разметкой
				 *
				 * @note Снимок снимается лишь тогда, когда встречный конец события `dest`
				 *       уже подключён: до подключения получателя нет, и называть некого
				 *
				 * @param id       идентификатор передаваемого события
				 * @param dest     идентификатор события, ведущего к процессу-получателю
				 * @param snapshot буфер, куда складывается снятый снимок
				 * @return         результат снятия снимка события
				 *
				 * \~english
				 * @brief Method of the taking of a transferable snapshot of an event for a foreign process
				 * @param id       identifier of the transferred event
				 * @param dest     identifier of the event leading to the recipient process
				 * @param snapshot buffer where the taken snapshot is put
				 * @return         result of the taking of the snapshot of the event
				 *
				 * \~
				 */
				bool snapshot(const event::id_t id, const event::id_t dest, vector <uint8_t> & snapshot) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки глубины очереди принятия входящих соединений события
				 *
				 * @param id       идентификатор события
				 * @param depth    глубина очереди принятия входящих соединений
				 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
				 *
				 * \~english
				 * @brief Method of setting the depth of the queue of the acceptance of the incoming connections of an event
				 * @param id       identifier of the event
				 * @param depth    depth of the queue of the acceptance of the incoming connections
				 * @param adaptive flag of the adaptive depth of the queue of the acceptance of the incoming connections
				 *
				 * \~
				 */
				void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 * \~english
				 * @brief Method of getting the size of the buffer of an event
				 * @param id     identifier of the event
				 * @param action type of the action of the event
				 * @return       size of the buffer of the event
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the size of the buffer of an event
				 * @param id     identifier of the event
				 * @param action type of the action of the event
				 * @param size   size of the buffer of the event
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool setBufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности события
				 *
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the events
				 * @param limiting  mode of the limitation of the bandwidth of an event (egress or ingress)
				 * @param bandwidth bandwidth of an event to set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
				 *
				 * \~
				 */
				void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности события для события
				 *
				 * @param id        идентификатор события
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of an event for an event
				 * @param id        identifier of the event
				 * @param limiting  mode of the limitation of the bandwidth of the event (egress or ingress)
				 * @param bandwidth bandwidth of the event to set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
				 * @return          result of the performance of the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t id, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима трансляции пакетов для события
				 *
				 * @param id идентификатор события
				 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 * \~english
				 * @brief Method of getting the mode of the transmission of the packets for an event
				 * @param id identifier of the event
				 * @return   mode of the transmission of the packets (unicast, multicast, broadcast)
				 *
				 * \~
				 */
				event::delivery_mode_t getDelivery(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима трансляции пакетов для события
				 *
				 * @param id       идентификатор события
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the mode of the transmission of the packets for an event
				 * @param id       identifier of the event
				 * @param delivery mode of the transmission of the packets (unicast, multicast, broadcast)
				 * @return         result of the performance of the setting
				 *
				 * \~
				 */
				bool setDelivery(const event::id_t id, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param id идентификатор события
				 * @return   метаданные последнего принятого дейтаграммного пакета
				 *
				 * \~english
				 * @brief Method of getting the metadata of the last received datagram packet
				 * @param id identifier of the event
				 * @return   metadata of the last received datagram packet
				 *
				 * \~
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param id идентификатор события
				 * @return   количество хопов последнего принятого пакета
				 *
				 * \~english
				 * @brief Method of getting the number of the hops of the last received packet
				 * @param id identifier of the event
				 * @return   number of the hops of the last received packet
				 *
				 * \~
				 */
				uint8_t getCountHops(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param id   идентификатор события
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the number of the hops of the last received packet
				 * @param id   identifier of the event
				 * @param hops number of the hops of the last received packet
				 * @return     result of the performance of the setting
				 *
				 * \~
				 */
				bool setCountHops(const event::id_t id, const uint8_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param id идентификатор события
				 * @return   максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops a packet may pass through
				 * @param id identifier of the event
				 * @return   maximum number of the hops
				 *
				 * \~
				 */
				event::hops_t getHops(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param id   идентификатор события
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops a packet may pass through
				 * @param id   identifier of the event
				 * @param hops maximum number of the hops
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const event::id_t id, const event::hops_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима использования таймаута для обработки события чтения
				 *
				 * @param id идентификатор события
				 * @return   режим использования таймаута для обработки события чтения
				 *
				 * \~english
				 * @brief Method of getting the mode of the use of the timeout for the handling of an event of the reading
				 * @param id identifier of the event
				 * @return   mode of the use of the timeout for the handling of an event of the reading
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима использования таймаута для обработки события чтения
				 *
				 * @details Определяет, что делать со сроком чтения, заданным через
				 *          `setTimeout()` с действием `READ`, после того как данные пришли.
				 *          Два режима отвечают двум разным по смыслу задачам.
				 *
				 *          `REUSABLE` - срок взводится заново после каждого чтения. Это
				 *          постоянный страж простоя: соединение обязано подавать признаки
				 *          жизни не реже заданного срока, иначе срабатывает таймаут.
				 *          Подходит потокам данных и долгоживущим подпискам.
				 *
				 *          `DISPOSABLE` - срок снимается, как только данные пришли, и
				 *          взводится заново при успешной отправке. То есть это не предел
				 *          простоя, а **ожидание ответа**: отправили запрос - пошёл отсчёт,
				 *          получили ответ - отсчёт снят. Подходит обмену «запрос-ответ», где
				 *          молчание в паузе между запросами законно.
				 *
				 * @note Режимом по умолчанию является `DISPOSABLE`. Ожидающим постоянной
				 *       активности соединениям режим следует менять явно, иначе простой
				 *       между запросами замечен не будет
				 *
				 * @note Действует только на событиях с неблокирующим или частично
				 *       блокирующим вводом-выводом: на блокирующих сроки держит сам сокет
				 *
				 * @param id    идентификатор события
				 * @param usage режим использования таймаута для обработки события чтения (reusable или disposable)
				 *
				 * \~english
				 * @brief Method of setting the mode of the use of the timeout for the handling of an event of the reading
				 * @details Determines what to do with the term of the reading, set through
				 *          `setTimeout()` with the action `READ`, after the data has come.
				 *          The two modes answer two tasks different in the meaning.
				 *          `REUSABLE` — the term is raised anew after every reading. This is
				 *          a permanent guard of the idling: a connection is obliged to give the signs
				 *          of the life no less often than the given term, otherwise the timeout triggers.
				 *          Suits the streams of the data and the long-living subscriptions.
				 *          `DISPOSABLE` — the term is removed as soon as the data has come, and
				 *          is raised anew at a successful sending. That is this is not a limit of
				 *          the idling, but a **waiting for an answer**: a request is sent — the count has gone,
				 *          an answer is received — the count is removed. Suits the exchange «request-answer», where
				 *          the silence in the pause between the requests is lawful.
				 * @note The mode by default is `DISPOSABLE`. For the connections expecting a permanent
				 *       activity the mode should be changed explicitly, otherwise the idling
				 *       between the requests will not be noticed
				 * @note Is in force only on the events with a non-blocking or with a partially
				 *       blocking input-output: on the blocking ones the terms are held by the socket itself
				 * @param id    identifier of the event
				 * @param usage mode of the use of the timeout for the handling of an event of the reading (reusable or disposable)
				 *
				 * \~
				 */
				void setUsageReadTimeout(const event::id_t id, const event::usage_t usage) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения таймаута события
				 *
				 * @details Возвращает **заданный** срок, а не остаток до срабатывания:
				 *          сколько времени таймеру осталось, отсюда узнать нельзя. Нулевое
				 *          значение означает, что срок не выставлен или снят.
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of an event
				 * @details Returns the **set** term, and not the remainder until the triggering:
				 *          how much time is left to the timer cannot be found out from here. A zero
				 *          value means that the term is not set out or is removed.
				 * @param id     identifier of the event
				 * @param action type of the action of the event
				 * @return       value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута события
				 *
				 * @details Один метод обслуживает два разных по смыслу случая, и различает
				 *          их по типу узла.
				 *
				 *          **Узлы-таймеры** (`TIMEOUT`, `INTERVAL`) - здесь значение задаёт
				 *          саму задержку срабатывания, а действие не участвует и передаётся
				 *          как `action_t::NONE`. Разница между двумя типами узлов лишь в
				 *          том, срабатывает таймер однажды или повторяется. О срабатывании
				 *          сообщает подписка на `status_t` со статусом `SUCCESS` -
				 *          отдельной функции у таймеров нет.
				 *
				 *          **Узлы соединений** (`CLIENT`, `PEER`, `ORIGIN`, `MEDIATOR`) -
				 *          здесь значение задаёт предел простоя, а действие говорит, простой
				 *          в чём считать. По истечении срока вызывается подписка на
				 *          `timeout_t`, и если её нет, соединение уничтожается безусловно.
				 *
				 * @note Нулевое значение **снимает** срок: заведённый таймер разоружается,
				 *       а событие возвращается в исходное состояние. Это и есть способ
				 *       отменить ранее выставленный таймаут - отдельного метода для отмены
				 *       нет
				 *
				 * @note Выставлять можно в любой момент, в том числе уже работающему
				 *       событию: живой таймер перевзводится тут же, с новым сроком
				 *
				 * @note На **блокирующих** событиях сроки чтения и записи ставятся опциями
				 *       сокета, а не таймерами движка. Наблюдаемое поведение то же, но
				 *       подписка `timeout_t` в этом случае не работает - ждёт сам системный
				 *       вызов
				 *
				 * @par Допустимые действия
				 * | Действие | Для кого | Смысл |
				 * |---|---|---|
				 * | `NONE` | таймеры | задержка срабатывания |
				 * | `READ` | соединения | сколько ждать входящих данных |
				 * | `WRITE` | соединения | сколько ждать возможности отправить |
				 * | `CONNECT` | только `CLIENT` | сколько ждать установления соединения |
				 * | `RECONNECT` | только `CLIENT` | пауза перед повторной попыткой |
				 *
				 * Действие, узлу не подходящее, срок не выставляет: в лог уходит
				 * предупреждение, а подписка на `status_t` получает статус `FAILURE`.
				 *
				 * @par Пример: таймер и предел простоя
				 * @param id      идентификатор события
				 * @param action  тип действия события
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * @code{.cpp}
				 * // Интервал, срабатывающий каждые пять секунд
				 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
				 * // Клиенту - пять секунд на подключение и тридцать на молчание
				 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
				 * io.setTimeout(client, awh::event::action_t::READ, 30000);
				 * // Передумали: снимаем предел простоя, оставив предел подключения
				 * io.setTimeout(client, awh::event::action_t::READ, 0);
				 * @endcode
				 *
				 * \~english
				 * @brief Method of setting the timeout of an event
				 * @details One method serves two cases different in the meaning, and tells
				 *          them apart by the type of the node.
				 *          **The timer nodes** (`TIMEOUT`, `INTERVAL`) — here the value sets
				 *          the very delay of the triggering, and the action does not participate and is passed
				 *          as `action_t::NONE`. The difference between the two types of the nodes is only in
				 *          whether the timer triggers once or repeats. About the triggering
				 *          the subscription to `status_t` with the status `SUCCESS` reports —
				 *          the timers have no separate function.
				 *          **The nodes of the connections** (`CLIENT`, `PEER`, `ORIGIN`, `MEDIATOR`) —
				 *          here the value sets the limit of the idling, and the action says the idling
				 *          in what should be counted. At the expiration of the term the subscription to
				 *          `timeout_t` is called, and if there is none, the connection is destroyed unconditionally.
				 * @note A zero value **removes** the term: a started timer is disarmed,
				 *       and the event returns into the initial state. This is the way of
				 *       cancelling a previously set out timeout — there is no separate method for the cancellation
				 * @note It may be set out at any moment, including to an already working
				 *       event: a living timer is re-raised right away, with the new term
				 * @note On the **blocking** events the terms of the reading and of the writing are placed by the options
				 *       of the socket, and not by the timers of the engine. The observed behaviour is the same, but
				 *       the `timeout_t` subscription in this case does not work — the system
				 *       call itself waits
				 * @par The admissible actions
				 * | Action | For whom | Meaning |
				 * |---|---|---|
				 * | `NONE` | the timers | the delay of the triggering |
				 * | `READ` | the connections | how long to wait for the incoming data |
				 * | `WRITE` | the connections | how long to wait for the possibility to send |
				 * | `CONNECT` | only `CLIENT` | how long to wait for the establishment of a connection |
				 * | `RECONNECT` | only `CLIENT` | the pause before a repeated attempt |
				 * An action not suiting a node does not set out the term: a warning goes into the log,
				 * and the subscription to `status_t` receives the status `FAILURE`.
				 * @par Example: a timer and a limit of the idling
				 * @param id      identifier of the event
				 * @param action  type of the action of the event
				 * @param timeout value of the timeout in milliseconds
				 *
				 * @code{.cpp}
				 * // An interval firing every five seconds
				 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
				 * // The client gets five seconds for the connection and thirty for the silence
				 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
				 * io.setTimeout(client, awh::event::action_t::READ, 30000);
				 * // Changed the mind: removing the limit of the idling and leaving the limit of the connection
				 * io.setTimeout(client, awh::event::action_t::READ, 0);
				 * @endcode
				 *
				 */
				void setTimeout(const event::id_t id, const event::action_t action, const uint32_t timeout) noexcept;
				/**
				 * \~russian
				 * @brief Метод продолжения прерванного ожидания
				 *
				 * @details Одноразовый срок ожидания чтения (`usage_t::DISPOSABLE`) снимается
				 *          приходом данных, и снимается **до** вызова отклика. Движку этого
				 *          довольно: данные пришли, ожидание кончилось. Договору - не всегда:
				 *          на дейтаграммном обмене прийти вправе что угодно и от кого угодно,
				 *          и пришедшее бывает не ответом на заданный вопрос, а чужим ответом,
				 *          ответом запоздалым либо шумом сети. Разобрать это способен лишь
				 *          сам договор, и лишь он вправе решить, что ожидание не кончилось
				 *
				 *          Решив так, договор зовёт этот метод, и ожидание продолжается.
				 *          Без него вопрос повисает навсегда: срок снят, взводить его заново
				 *          нечем, а другого срабатывания не будет
				 *
				 *          Задержка нулевая означает продолжение **с того места, где ожидание
				 *          прервано**: движок помнит остаток снятого срока и взводит ожидание
				 *          ровно на него. Тем и отличается продолжение от нового ожидания:
				 *          чужой ответ не дарит вопросу лишнего времени, и сколько бы их ни
				 *          пришло, отказ наступит в свой черёд
				 *
				 *          Задержка ненулевая задаёт ожидание заново, на указанный срок.
				 *          Нужна там, где договор знает больше движка: ответ пришёл частью,
				 *          и остаток разумно ждать иначе, чем ждали целое
				 *
				 * @note Продолжать нечего, если ожидание не прерывалось, прервано у события
				 *       иного, у срока иного либо успело истечь. Во всех этих случаях метод
				 *       отвечает отказом, ничего не взводя
				 *
				 * @warning Продолжение нулевой задержкой осмысленно **только внутри отклика**,
				 *          вызванного тем самым чтением, что прервало ожидание. Движок помнит
				 *          один прерванный срок, а не все: обращение позднее застанет запись
				 *          уже чужой и получит отказ. Обходить это, запоминая остаток у себя,
				 *          не следует - лучше позвать метод там, где решение и принимается
				 *
				 * @par Пример: чужой ответ ожидания не прерывает
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param delay  задержка в миллисекундах, либо ноль для продолжения с остатка
				 * @return       результат продолжения ожидания
				 *
				 * @code{.cpp}
				 * // Разбираем пришедший ответ
				 * if(header.id != this->_awaiting){
				 *     // Ответ не на наш вопрос - продолжаем ожидание с прерванного места
				 *     io.rearmTimeout(id, awh::event::action_t::READ);
				 *     // Ответ чужой, разбирать его нечего
				 *     return;
				 * }
				 * @endcode
				 *
				 * \~english
				 * @brief Method of the continuation of an interrupted waiting
				 * @details A single-use term of the waiting for the reading (`usage_t::DISPOSABLE`) is removed
				 *          by the arrival of the data, and is removed **before** the call of the response. For the engine this
				 *          is enough: the data has come, the waiting has ended. For the contract — not always:
				 *          at a datagram exchange anything from anyone is free to come,
				 *          and what has come happens to be not an answer to the asked question, but a foreign answer,
				 *          a belated answer or a noise of the network. Only the contract itself is capable of resolving this,
				 *          and only it is free to decide that the waiting has not ended
				 *          Having decided so, the contract calls this method, and the waiting continues.
				 *          Without it the question hangs forever: the term is removed, there is nothing to raise it
				 *          anew by, and there will be no other triggering
				 *          A zero delay means the continuation **from the place where the waiting
				 *          is interrupted**: the engine remembers the remainder of the removed term and raises the waiting
				 *          exactly for it. By this the continuation differs from a new waiting:
				 *          a foreign answer does not gift the question an extra time, and however many of them may
				 *          come, the refusal will come in its turn
				 *          A non-zero delay sets the waiting anew, for the specified term.
				 *          Is needed where the contract knows more than the engine: an answer has come partially,
				 *          and it is reasonable to wait for the remainder otherwise than the whole was waited for
				 * @note There is nothing to continue if the waiting was not interrupted, is interrupted at another event,
				 *       at another term or has managed to expire. In all these cases the method
				 *       answers with a refusal, raising nothing
				 * @warning The continuation by a zero delay is meaningful **only inside the response**,
				 *          called by that very reading which has interrupted the waiting. The engine remembers
				 *          one interrupted term, and not all of them: an address later will find the record
				 *          already a foreign one and will receive a refusal. Going around this by remembering the remainder at oneself,
				 *          is not advisable — it is better to call the method there, where the decision is taken as well
				 * @par Example: a foreign answer does not interrupt the waiting
				 * @param id     identifier of the event
				 * @param action type of the action of the event
				 * @param delay  delay in milliseconds, or zero for the continuation from the remainder
				 * @return       result of the continuation of the waiting
				 *
				 * @code{.cpp}
				 * // Parsing the answer that came
				 * if(header.id != this->_awaiting){
				 *     // The answer is not to our question — continuing the waiting from the interrupted place
				 *     io.rearmTimeout(id, awh::event::action_t::READ);
				 *     // The answer is a foreign one, there is nothing to parse in it
				 *     return;
				 * }
				 * @endcode
				 *
				 */
				bool rearmTimeout(const event::id_t id, const event::action_t action, const uint32_t delay = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения действия события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       режим действия события
				 *
				 * \~english
				 * @brief Method of getting an action of an event
				 * @param id     identifier of the event
				 * @param action type of the action of the event
				 * @return       mode of the action of the event
				 *
				 * \~
				 */
				event::mode_t getAction(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки действия события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param mode   режим установки действия события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting an action of an event
				 * @param id     identifier of the event
				 * @param action type of the action of the event
				 * @param mode   mode of the setting of the action of the event
				 * @return       result of the performance of the setting
				 *
				 * \~
				 */
				bool setAction(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки параметров keep-alive для события
				 *
				 * @param id    идентификатор события
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the parameters of the keep-alive for an event
				 * @param id    identifier of the event
				 * @param cnt   number of the keep-alive packets
				 * @param idle  time of the idling before the sending of the first keep-alive packet in seconds
				 * @param intvl interval between the keep-alive packets in seconds
				 * @return      result of the performance of the setting
				 *
				 * \~
				 */
				bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод приостановки события
				 *
				 * @details Отключает чтение, не разрывая соединения: данные остаются в
				 *          приёмном буфере ядра, отправитель упирается в исчерпание окна и
				 *          сам сбавляет темп. Это штатный способ придержать поток, когда
				 *          принимающая сторона не успевает разбирать принятое, - в отличие
				 *          от `disconnect()`, соединение при этом цело.
				 *
				 * @note Снятие чтения выполняется **немедленно**, не дожидаясь очередного
				 *       оборота цикла: иначе успела бы прийти ещё порция данных. А вот
				 *       возобновление откладывается до следующего оборота, и это
				 *       расхождение намеренное
				 *
				 * @note Приостановить можно только запущенное событие, а возобновить -
				 *       только приостановленное. Повторные вызовы отказывают
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения приостановки
				 *
				 * \~english
				 * @brief Method of the pausing of an event
				 * @details Switches off the reading, without breaking the connection: the data remains in
				 *          the receiving buffer of the kernel, the sender runs into the exhaustion of the window and
				 *          lowers the rate itself. This is the regular way of holding back a stream, when
				 *          the receiving side does not manage to disassemble what has been received, — unlike
				 *          `disconnect()`, the connection at that is intact.
				 * @note The removal of the reading is performed **immediately**, without waiting for the next
				 *       turn of the loop: otherwise another portion of the data would manage to come. The
				 *       resumption, though, is postponed until the next turn, and this
				 *       divergence is a deliberate one
				 * @note Only a launched event may be paused, and resumed —
				 *       only a paused one. The repeated calls refuse
				 * @param id identifier of the event
				 * @return   result of the performance of the pausing
				 *
				 * \~
				 */
				bool pause(const event::id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления события
				 *
				 * @details Возвращает чтение приостановленному событию. Накопившееся в
				 *          приёмном буфере ядра придёт обычными подписками на чтение,
				 *          начиная с очередного оборота цикла/
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения возобновления
				 *
				 * \~english
				 * @brief Method of the resumption of an event
				 * @details Returns the reading to a paused event. What has accumulated in
				 *          the receiving buffer of the kernel will come by the ordinary subscriptions to the reading,
				 *          starting from the next turn of the loop/
				 * @param id identifier of the event
				 * @return   result of the performance of the resumption
				 *
				 * \~
				 */
				bool resume(const event::id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки состояния события
				 *
				 * @param id идентификатор события
				 * @return   состояние события
				 *
				 * \~english
				 * @brief Method of checking the state of an event
				 * @param id identifier of the event
				 * @return   state of the event
				 *
				 * \~
				 */
				bool isAlive(const event::id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки сетевого движка
				 *
				 * \~english
				 * @brief Method of clearing the network engine
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод принудительного пинка базе событий
				 *
				 * @return результат выполнения операции
				 *
				 * \~english
				 * @brief Method of the forced kick to the base of the events
				 * @return result of the performance of the operation
				 *
				 * \~
				 */
				bool kick() noexcept;
				/**
				 * \~russian
				 * @brief Метод инициализации сетевого движка
				 *
				 * @return результат выполнения инициализации
				 *
				 * \~english
				 * @brief Method of the initialization of the network engine
				 * @return result of the performance of the initialization
				 *
				 * \~
				 */
				bool initialize() noexcept;
				/**
				 * \~russian
				 * @brief Метод реинициализации сетевого движка
				 *
				 * @return результат выполнения реинициализации
				 *
				 * \~english
				 * @brief Method of the reinitialization of the network engine
				 * @return result of the performance of the reinitialization
				 *
				 * \~
				 */
				bool reinitialize() noexcept;
				/**
				 * \~russian
				 * @brief Метод деинициализации сетевого движка
				 *
				 * @return результат выполнения деинициализации
				 *
				 * \~english
				 * @brief Method of the deinitialization of the network engine
				 * @return result of the performance of the deinitialization
				 *
				 * \~
				 */
				bool deinitialize() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки состояния инициализации сетевого движка
				 *
				 * @return состояние инициализации
				 *
				 * \~english
				 * @brief Method of checking the state of the initialization of the network engine
				 * @return state of the initialization
				 *
				 * \~
				 */
				bool isInitialized() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества событий в сетевом движке
				 *
				 * @return количество событий
				 *
				 * \~english
				 * @brief Method of getting the number of the events in the network engine
				 * @return number of the events
				 *
				 * \~
				 */
				size_t eventsCount() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа внутренних таймеров
				 *
				 * @return тип таймера для событий сетевого движка
				 *
				 * \~english
				 * @brief Method of getting the type of the internal timers
				 * @return type of the timer for the events of the network engine
				 *
				 * \~
				 */
				event::timer_t getInternalTimer() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки типа внутренних таймеров
				 *
				 * @details Выбирает структуру, в которой движок держит сроки событий.
				 *          `SIMPLE` - упорядоченное множество с хэш-таблицей положений:
				 *          скромна по памяти, но каждая постановка срока выделяет по узлу в
				 *          обеих. `DIFFICULT` - двоичная куча со страничной таблицей слотов:
				 *          к аллокатору не обращается вовсе и на постановке быстрее в разы,
				 *          зато таблица слотов выделяется чанками по тысяче событий.
				 *
				 *          Умолчанием служит `SIMPLE` - как и прочие умолчания, оно
				 *          рассчитано на самую слабую машину и самый общий случай.
				 *          Приложению, которое держит много сроков и ставит их часто,
				 *          переключение выгодно, и выигрыш измеряется разами.
				 *
				 * @note Выбор общий для всего движка, а не для отдельного события, и менять
				 *       его следует **до** заведения событий: переключение сбрасывает уже
				 *       заведённые таймеры
				 *
				 * @par Пример: включить структуру для большого числа сроков
				 * @param timer тип таймера для событий сетевого движка
				 *
				 * @code{.cpp}
				 * awh::engine::io_t io(&fmk, &log);
				 * // Переключаем до заведения событий и до initialize()
				 * io.setInternalTimer(awh::event::timer_t::DIFFICULT);
				 * io.initialize();
				 * @endcode
				 *
				 * \~english
				 * @brief Method of setting the type of the internal timers
				 * @details Chooses the structure in which the engine holds the terms of the events.
				 *          `SIMPLE` — an ordered set with a hash table of the positions:
				 *          is modest in the memory, but every placement of a term allocates a node in
				 *          both. `DIFFICULT` — a binary heap with a paged table of the slots:
				 *          does not address the allocator at all and is multiply faster at the placement,
				 *          but the table of the slots is allocated by the chunks of a thousand events.
				 *          The default is `SIMPLE` — as the other defaults, it is
				 *          reckoned on the weakest machine and on the most common case.
				 *          For an application that holds many terms and places them often,
				 *          the switching is profitable, and the gain is measured in multiples.
				 * @note The choice is common for the whole engine, and not for a separate event, and it should be changed
				 *       **before** the starting of the events: the switching resets the already
				 *       started timers
				 * @par Example: switching on the structure for a large number of the terms
				 * @param timer type of the timer for the events of the network engine
				 *
				 * @code{.cpp}
				 * awh::engine::io_t io(&fmk, &log);
				 * // Switching before the starting of the events and before initialize()
				 * io.setInternalTimer(awh::event::timer_t::DIFFICULT);
				 * io.initialize();
				 * @endcode
				 *
				 */
				void setInternalTimer(const event::timer_t timer) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера отслеживаемого файла
				 *
				 * @param id идентификатор события
				 * @return   размер файла
				 *
				 * \~english
				 * @brief Method of getting the size of an observed file
				 * @param id identifier of the event
				 * @return   size of the file
				 *
				 * \~
				 */
				size_t size(const event::id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества байт, доступных для записи в очередь события
				 *
				 * @param id идентификатор события
				 * @return   количество байт, доступных для записи
				 *
				 * \~english
				 * @brief Method of getting the number of the bytes available for the writing into the queue of an event
				 * @param id identifier of the event
				 * @return   number of the bytes available for the writing
				 *
				 * \~
				 */
				size_t available(const event::id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа события
				 *
				 * @param id идентификатор события
				 * @return   тип события
				 *
				 * \~english
				 * @brief Method of getting the type of an event
				 * @param id identifier of the event
				 * @return   type of the event
				 *
				 * \~
				 */
				event::type_t type(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения типа узла события
				 *
				 * @param id идентификатор события
				 * @return   тип узла события
				 *
				 * \~english
				 * @brief Method of getting the type of the node of an event
				 * @param id identifier of the event
				 * @return   type of the node of the event
				 *
				 * \~
				 */
				event::node_t node(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения семейства события
				 *
				 * @param id идентификатор события
				 * @return   семейство адресов
				 *
				 * \~english
				 * @brief Method of getting the family of an event
				 * @param id identifier of the event
				 * @return   family of the addresses
				 *
				 * \~
				 */
				event::family_t family(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения статуса события
				 *
				 * @param id идентификатор события
				 * @return   статус события
				 *
				 * \~english
				 * @brief Method of getting the status of an event
				 * @param id identifier of the event
				 * @return   status of the event
				 *
				 * \~
				 */
				event::status_t status(const event::id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения протокола события
				 *
				 * @param id идентификатор события
				 * @return   протокол события
				 *
				 * \~english
				 * @brief Method of getting the protocol of an event
				 * @param id identifier of the event
				 * @return   protocol of the event
				 *
				 * \~
				 */
				event::protocol_t protocol(const event::id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод опроса событий
				 *
				 * @details Выполняет **один оборот** цикла событий и возвращает
				 *          управление. Своего потока движок не создаёт и сам себя не
				 *          крутит - цикл ведёт вызывающий:
				 *
				 *          За один оборот выполняется: освобождение узлов,
				 *          отложенных позапрошлым оборотом; отправка накопленного
				 *          пакета изменений подписки вместе с ожиданием - одним
				 *          обращением к ядру, а не двумя; разбор полученных событий с
				 *          вызовом функций обратного вызова; разбор истёкших
				 *          дедлайнов таймеров.
				 *          Все функции обратного вызова вызываются **внутри** этого
				 *          метода. Пока они выполняются, оборот не завершён, поэтому
				 *          долгая работа в обратном вызове задерживает и остальные
				 *          события, и срабатывание таймеров.
				 * @note    Время ожидания ограничивается не только переданным
				 *          таймаутом, но и сроком ближайшего внутреннего таймера:
				 *          движок обязан проснуться к дедлайну, даже если вызывающий
				 *          просил ждать дольше или бесконечно.
				 * @note    Отрицательный таймаут означает ожидание без предела, но с
				 *          учётом таймеров; нулевой - опрос без ожидания вовсе,
				 *          пригодный для встраивания в чужой цикл событий.
				 * @note    Отрицательный результат означает отказ опроса, а не
				 *          отсутствие событий: оборот без единого события - это
				 *          обычный успех. Прерывание системного вызова сигналом
				 *          отказом не считается, движок продолжает работу.
				 * @note    Метод обязан вызываться из одного и того же потока. Первый
				 *          вызов запоминает поток опроса, и обращения к событиям из
				 *          других потоков после этого недопустимы
				 * @par Встраивание в чужой цикл событий
				 * @param timeout таймаут опроса в миллисекундах: отрицательный - без
				 *                предела, нулевой - без ожидания
				 * @return        результат выполнения опроса
				 *
				 * @code{.cpp}
				 * while(io.poll(100));
				 * @endcode
				 *
				 * @code{.cpp}
				 * // Опрос без ожидания: управление возвращается сразу
				 * while(running){
				 *     io.poll(0);
				 *     foreignLoopIteration();
				 * }
				 * @endcode
				 *
				 * \~english
				 * @brief Method of the polling of the events
				 * @details Performs **one turn** of the loop of the events and returns
				 *          the control. The engine creates no thread of its own and does not spin
				 *          itself — the loop is kept by the caller:
				 *          Over one turn there are performed: the release of the nodes
				 *          postponed by the turn before the last one; the sending of the accumulated
				 *          packet of the changes of the subscription together with the waiting — by one
				 *          address to the kernel, and not by two; the disassembly of the obtained events with
				 *          the call of the callback functions; the disassembly of the expired
				 *          deadlines of the timers.
				 *          All the callback functions are called **inside** this
				 *          method. While they are being performed, the turn is not completed, and therefore
				 *          a long work in a callback delays both the other
				 *          events, and the triggering of the timers.
				 * @note    The time of the waiting is limited not only by the passed
				 *          timeout, but by the term of the nearest internal timer as well:
				 *          the engine is obliged to wake up by the deadline, even if the caller
				 *          has asked to wait longer or infinitely.
				 * @note    A negative timeout means the waiting without a limit, but with
				 *          the timers taken into account; a zero one — a polling without a waiting at all,
				 *          fit for the embedding into a foreign loop of the events.
				 * @note    A negative result means a refusal of the polling, and not
				 *          the absence of the events: a turn without a single event is
				 *          an ordinary success. The interruption of a system call by a signal
				 *          is not considered a refusal, the engine continues the work.
				 * @note    The method is obliged to be called from one and the same thread. The first
				 *          call remembers the thread of the polling, and the addresses to the events from
				 *          the other threads after this are inadmissible
				 * @par The embedding into a foreign loop of the events
				 * @param timeout timeout of the polling in milliseconds: a negative one — without
				 *                a limit, a zero one — without a waiting
				 * @return        result of the performance of the polling
				 *
				 * @code{.cpp}
				 * while(io.poll(100));
				 * @endcode
				 *
				 * @code{.cpp}
				 * // A poll without waiting: the control returns at once
				 * while(running){
				 *     io.poll(0);
				 *     foreignLoopIteration();
				 * }
				 * @endcode
				 *
				 */
				bool poll(const int32_t timeout = -1) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки события чтения
				 *
				 * @details Подписка на приём данных. Буфер, приходящий в функцию,
				 *          принадлежит движку и действителен **только на время вызова**:
				 *          он переиспользуется под следующее чтение, поэтому данные,
				 *          нужные позже, следует скопировать.
				 *
				 * @par Общие правила для всех перегрузок `on()`
				 * Все перегрузки устроены одинаково, и сказанное здесь относится к
				 * каждой из них.
				 *
				 * Подписка выполняется **присваиванием**: повторный вызов с тем же типом
				 * функции заменяет прежнюю, не добавляя вторую. Двух обработчиков одного
				 * события одного вида быть не может, а передача пустой функции подписку
				 * снимает.
				 *
				 * Подписываться можно в любой момент, а не только до `commit()`. В
				 * частности, принятое подключение приходит уже заведённым событием, и
				 * подписки ему выставляются прямо в функции приёма - как в примере к
				 * описанию класса.
				 *
				 * Неизвестный идентификатор и событие, помеченное к уничтожению,
				 * **игнорируются молча**: ни исключения, ни возвращаемого признака здесь
				 * нет. Если же тип функции узлу не подходит - скажем, чтение для узла
				 * сервера, - в лог уходит предупреждение, а подписка не выставляется.
				 * Поэтому список поддерживаемых типов узлов указан у каждой перегрузки
				 * отдельно, и сверяться с ним стоит: опечатка в типе узла тихо оставит
				 * событие без обработчика.
				 *
				 * Вызывать `on()` изнутри функции обратного вызова безопасно, включая
				 * замену обработчика на самого себя.
				 *
				 * @note Приведение через `static_cast` требуется там, где по одной лямбде
				 *       перегрузку не выбрать однозначно. Так происходит с парой
				 *       `write_t` и `connect_t`: их второй параметр - `size_t` и `bool`, а
				 *       они приводятся друг к другу неявно. Перегрузки, различающиеся
				 *       типами перечислений или типом возврата, выбираются сами, и
				 *       приведения не требуют
				 *
				 * @par Поддерживаемые типы узлов
				 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of an event of the reading
				 * @details The subscription to the reception of the data. The buffer coming into the function
				 *          belongs to the engine and is valid **only for the time of the call**:
				 *          it is reused for the next reading, and therefore the data
				 *          needed later should be copied.
				 * @par The common rules for all the overloads of `on()`
				 * All the overloads are arranged identically, and what is said here relates to
				 * each of them.
				 * The subscription is performed by an **assignment**: a repeated call with the same type
				 * of the function replaces the previous one, not adding a second one. There cannot be two handlers of one
				 * event of one kind, and the passing of an empty function removes the subscription.
				 * One may subscribe at any moment, and not only before `commit()`. In
				 * particular, an accepted connection comes as an already started event, and
				 * the subscriptions are set out to it right in the function of the acceptance — as in the example to
				 * the description of the class.
				 * An unknown identifier and an event marked for the destruction
				 * **are ignored silently**: there is neither an exception, nor a returned sign here.
				 * If, though, the type of the function does not suit the node — say, the reading for a node
				 * of a server, — a warning goes into the log, and the subscription is not set out.
				 * Therefore the list of the supported types of the nodes is specified at every overload
				 * separately, and it is worth checking against it: a typo in the type of the node will quietly leave
				 * the event without a handler.
				 * Calling `on()` from inside a callback function is safe, including
				 * the replacement of a handler by itself.
				 * @note The cast through `static_cast` is required where by one lambda
				 *       an overload cannot be chosen unambiguously. So it happens with the pair
				 *       `write_t` and `connect_t`: their second parameter is a `size_t` and a `bool`, and
				 *       they are cast to each other implicitly. The overloads differing by
				 *       the types of the enumerations or by the type of the return are chosen by themselves, and
				 *       require no cast
				 * @par The supported types of the nodes
				 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::read_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки события записи
				 *
				 * @details Сообщает, сколько байт ушло в сокет. Полезно там, где скорость
				 *          отправки нужно согласовать с источником данных: размер
				 *          записанного - это и есть освободившееся место в очереди.
				 *
				 * @note Требует приведения через `static_cast`, иначе перегрузка
				 *       неотличима от `connect_t`
				 *
				 * @par Поддерживаемые типы узлов
				 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of an event of the writing
				 * @details Reports how many bytes have gone into the socket. Is useful where the speed
				 *          of the sending needs to be agreed with the source of the data: the size of
				 *          what is written is the room freed in the queue.
				 * @note Requires a cast through `static_cast`, otherwise the overload
				 *       is indistinguishable from `connect_t`
				 * @par The supported types of the nodes
				 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::write_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки возврата неотправленных данных
				 *
				 * @details Срабатывает, когда отправить данные не удалось, и возвращает их
				 *          вызывающей стороне: движок их не сохраняет и после выхода из
				 *          функции освобождает. Решение о судьбе байтов - повторить
				 *          позже, отложить в свой буфер или отбросить - остаётся за
				 *          вызывающей стороной. Второй параметр говорит, откуда данные
				 *          вернулись: из самого события или из его очереди отправки.
				 *
				 * @note Без этой подписки неотправленные данные теряются без следа.
				 *       Событиям, где потеря недопустима, подписку следует выставлять
				 *       наравне с чтением
				 *
				 * @par Поддерживаемые типы узлов
				 * `FILE`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of the return of the unsent data
				 * @details Triggers when the data could not be sent, and returns it
				 *          to the calling side: the engine does not save it and after the exit from
				 *          the function releases it. The decision about the fate of the bytes — to repeat
				 *          later, to put aside into one's own buffer or to discard — remains at
				 *          the calling side. The second parameter says where the data
				 *          has returned from: from the event itself or from its queue of the sending.
				 * @note Without this subscription the unsent data is lost without a trace.
				 *       For the events where a loss is inadmissible the subscription should be set out
				 *       on a par with the reading
				 * @par The supported types of the nodes
				 * `FILE`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::spool_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки общего события
				 *
				 * @details Единая точка наблюдения за происходящим с событием: в функцию
				 *          приходит тип действия, а не его последствия. Нужна там, где
				 *          важен сам факт - для журналирования, счётчиков, отладки - а
				 *          разбирать данные незачем.
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `DIR`, `FILE`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`,
				 * `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of a common event
				 * @details A single point of the observation of what is happening with an event: into the function
				 *          the type of an action comes, and not its consequences. Is needed where
				 *          the fact itself matters — for the journaling, for the counters, for the debugging — and
				 *          there is no reason to disassemble the data.
				 * @par The supported types of the nodes
				 * `NOTIFY`, `DIR`, `FILE`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`,
				 * `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::event_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки ошибки события
				 *
				 * @details Получает код ошибки и её текстовое описание. Подписка эта
				 *          заодно **подавляет вывод ошибок в лог**: пока она не
				 *          выставлена, движок печатает ошибки сам, а с ней - передаёт их
				 *          целиком на усмотрение вызывающей стороны.
				 *
				 * @note Поддерживается почти всеми типами узлов, включая таймеры, и
				 *       выставлять её стоит всегда: без неё причина отказа события
				 *       остаётся только в логе
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of an error of an event
				 * @details Receives the code of an error and its text description. This subscription
				 *          at the same time **suppresses the output of the errors into the log**: while it is not
				 *          set out, the engine prints the errors itself, and with it — passes them
				 *          entirely to the discretion of the calling side.
				 * @note Is supported by almost all the types of the nodes, including the timers, and
				 *       it is worth setting out always: without it the reason of a refusal of an event
				 *       remains only in the log
				 * @par The supported types of the nodes
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::error_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки изменений события
				 *
				 * @details Наблюдение за файловой системой: сообщает, что именно
				 *          произошло с файлом или каталогом, и с каким именно.
				 *
				 * @note Имя, приходящее в функцию, действительно ТОЛЬКО на время вызова
				 *
				 * @par Поддерживаемые типы узлов
				 * `DIR`, `FILE`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of the changes of an event
				 * @details The observation of the file system: reports what exactly
				 *          has happened to a file or to a directory, and with which exactly.
				 * @note The name coming into the function is valid ONLY for the time of the call
				 * @par The supported types of the nodes
				 * `DIR`, `FILE`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::vnode_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова инъекции объединённых данных (splice)
				 *
				 * @details Позволяет транспорту, шифрующему данные на уровне соединения,
				 *          принять перенаправленные из события-источника байты и отправить
				 *          их собственным потоком, а не записывать сырьём в сокет.
				 *          Отрицательный результат означает отказ принять данные.
				 *
				 * @par Поддерживаемые типы узлов
				 * `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the injection of the joined data (splice)
				 * @details Allows a transport encrypting the data at the level of the connection
				 *          to accept the bytes redirected from a source event and to send
				 *          them by its own stream, and not to write them raw into the socket.
				 *          A negative result means a refusal to accept the data.
				 * @par The supported types of the nodes
				 * `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::inject_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обновления статуса события
				 *
				 * @details Сообщает о смене состояния события - подключено, отключено,
				 *          отказ и так далее.
				 *
				 * @note Через эту же подписку сообщают о срабатывании **узлы-таймеры**:
				 *       отдельной функции у них нет, и сработавший `TIMEOUT` или
				 *       `INTERVAL` приходит сюда со статусом `event::status_t::SUCCESS`.
				 *       Это единственный способ узнать о срабатывании таймера, и в
				 *       функции статус следует проверять: приходят и остальные состояния
				 *
				 * @par Пример: срабатывание интервала
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * @code{.cpp}
				 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
				 * io.on(timer, static_cast <awh::engine::callback::status_t> ([](const awh::event::id_t id, const awh::event::status_t status) noexcept -> void {
				 *     // Интервал сработал, и сработает снова через те же пять секунд
				 *     if(status == awh::event::status_t::SUCCESS)
				 *         tick();
				 * }));
				 * io.commit(timer);
				 * io.launch(timer);
				 * @endcode
				 *
				 * \~english
				 * @brief Method of setting the callback function for the update of the status of an event
				 * @details Reports the change of the state of an event — connected, disconnected,
				 *          a refusal and so on.
				 * @note Through this same subscription the **timer nodes** report about their triggering:
				 *       they have no separate function, and a triggered `TIMEOUT` or
				 *       `INTERVAL` comes here with the status `event::status_t::SUCCESS`.
				 *       This is the only way of finding out about the triggering of a timer, and in
				 *       the function the status should be checked: the other states come as well
				 * @par Example: the triggering of an interval
				 * @par The supported types of the nodes
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * @code{.cpp}
				 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
				 * io.on(timer, static_cast <awh::engine::callback::status_t> ([](const awh::event::id_t id, const awh::event::status_t status) noexcept -> void {
				 *     // The interval has fired and will fire again in the same five seconds
				 *     if(status == awh::event::status_t::SUCCESS)
				 *         tick();
				 * }));
				 * io.commit(timer);
				 * io.launch(timer);
				 * @endcode
				 *
				 */
				void on(const event::id_t id, engine::callback::status_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для приёма входящего подключения
				 *
				 * @details Принятое подключение приходит **уже заведённым событием**: его
				 *          идентификатор передаётся вторым параметром, и заводить его
				 *          через `event()`, настраивать и запускать не требуется. Всё, что
				 *          нужно сделать в этой функции - выставить принятому событию
				 *          подписки, иначе принимаемые им данные обрабатывать будет некому.
				 *
				 * @note Время жизни принятого события движку не принадлежит: закрывать его
				 *       следует своим вызовом `destroy()`
				 *
				 * @par Поддерживаемые типы узлов
				 * `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the acceptance of an incoming connection
				 * @details An accepted connection comes as an **already started event**: its
				 *          identifier is passed by the second parameter, and starting it
				 *          through `event()`, setting it up and launching it is not required. All that
				 *          needs to be done in this function is to set out the subscriptions to the accepted event,
				 *          otherwise there will be nobody to handle the data received by it.
				 * @note The time of the life of an accepted event does not belong to the engine: it should be closed
				 *       by one's own call of `destroy()`
				 * @par The supported types of the nodes
				 * `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::accept_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для определения сессии дейтаграммного пакета
				 *
				 * @note Поддерживается только серверными узлами. Установка функции
				 *       переводит событие на маршрутизацию датаграмм по ключу
				 *       приложения вместо адреса отправителя
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the determination of the session of a datagram packet
				 * @note Is supported only by the server nodes. The setting of the function
				 *       switches the event to the routing of the datagrams by the key
				 *       of the application instead of the address of the sender
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::origin_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова на получение информационных метаданных о дейтаграммном пакете
				 *
				 * @details Сопутствующие сведения о датаграмме - откуда пришла, каким
				 *          интерфейсом принята, что несёт в заголовках. Сами данные
				 *          приходят обычной подпиской на чтение, а сюда попадает то, что в
				 *          них не содержится.
				 *
				 * @par Поддерживаемые типы узлов
				 * `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function on the receipt of the informational metadata about a datagram packet
				 * @details The accompanying information about a datagram — where it has come from, by which
				 *          interface it is received, what it carries in the headers. The data itself
				 *          comes by the ordinary subscription to the reading, and here there gets what is not
				 *          contained in it.
				 * @par The supported types of the nodes
				 * `CLIENT`, `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::traffic_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки подключения
				 *
				 * @details Сообщает об исходе попытки подключения: признак говорит,
				 *          состоялось соединение или нет. До этого вызова отправлять данные
				 *          некуда, поэтому первая отправка клиента обычно делается именно
				 *          отсюда.
				 *
				 * @note Требует приведения через `static_cast`, иначе перегрузка
				 *       неотличима от `write_t`
				 *
				 * @par Поддерживаемые типы узлов
				 * `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of a connection
				 * @details Reports the outcome of an attempt of a connection: the sign says
				 *          whether the connection has taken place or not. Before this call there is nowhere to send the data,
				 *          and therefore the first sending of a client is usually done exactly
				 *          from here.
				 * @note Requires a cast through `static_cast`, otherwise the overload
				 *       is indistinguishable from `write_t`
				 * @par The supported types of the nodes
				 * `CLIENT`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::connect_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова на получение информации о пакетах в туннельном интерфейсе
				 *
				 * @details Сведения о пакетах, прошедших через туннельный интерфейс, вместе
				 *          с идентификатором удалённого узла, которому они принадлежат.
				 *
				 * @par Поддерживаемые типы узлов
				 * `TUNNEL`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function on the receipt of the information about the packets in a tunnel interface
				 * @details The information about the packets that have passed through a tunnel interface, together
				 *          with the identifier of the remote node they belong to.
				 * @par The supported types of the nodes
				 * `TUNNEL`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::tuninfo_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки таймаута события
				 *
				 * @details Срабатывает, когда истёк срок, заданный через `setTimeout()`:
				 *          соединение не приняло данных (`READ`), не смогло их отправить
				 *          (`WRITE`), не установилось (`CONNECT`) или подошла пора повторить
				 *          попытку (`RECONNECT`).
				 *
				 * @warning Смысл возвращаемого признака **зависит от действия**. Для
				 *          `READ`, `WRITE` и `CONNECT` положительный признак узел
				 *          уничтожает, а отрицательный оставляет жить. Для `RECONNECT` всё
				 *          наоборот: положительный означает «переподключаться», а прервать
				 *          попытку нужно отрицательным. Одна функция обслуживает все
				 *          действия сразу, и различать эти случаи обязана она
				 *
				 * @note Если подписка не выставлена, узел по истечении срока `READ`,
				 *       `WRITE` или `CONNECT` уничтожается **безусловно**. То есть она
				 *       нужна ровно затем, чтобы обрыв предотвратить или обставить своими
				 *       действиями
				 *
				 * @note По истечении срока `CONNECT` дополнительно вызывается подписка
				 *       `connect_t` с отрицательным исходом, и происходит это **до** вызова
				 *       этой функции
				 *
				 * @par Пример: разные действия - разный смысл ответа
				 * @par Поддерживаемые типы узлов
				 * `PEER`, `ORIGIN`, `CLIENT`
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * @code{.cpp}
				 * io.setTimeout(client, awh::event::action_t::READ, 30000);
				 * io.setTimeout(client, awh::event::action_t::RECONNECT, 5000);
				 * io.on(client, static_cast <awh::engine::callback::timeout_t> ([&attempts](const awh::event::id_t id, const awh::event::action_t action, const uint32_t delay) noexcept -> bool {
				 *     // Переподключение: положительный ответ означает «пробовать снова»
				 *     if(action == awh::event::action_t::RECONNECT)
				 *         return (attempts++ < 3);
				 *     // Простой: положительный ответ означает «рвать соединение»
				 *     return true;
				 * }));
				 * @endcode
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of the timeout of an event
				 * @details Triggers when the term set through `setTimeout()` has expired:
				 *          the connection has not accepted the data (`READ`), has not managed to send it
				 *          (`WRITE`), has not been established (`CONNECT`) or the time has come to repeat
				 *          the attempt (`RECONNECT`).
				 * @warning The meaning of the returned sign **depends on the action**. For
				 *          `READ`, `WRITE` and `CONNECT` a positive sign destroys the node,
				 *          and a negative one leaves it alive. For `RECONNECT` everything is
				 *          the other way round: a positive one means «reconnect», and the attempt should be interrupted
				 *          by a negative one. One function serves all
				 *          the actions at once, and it is obliged to tell these cases apart
				 * @note If the subscription is not set out, the node at the expiration of the term of `READ`,
				 *       `WRITE` or `CONNECT` is destroyed **unconditionally**. That is it
				 *       is needed exactly in order to prevent the break or to surround it with one's own
				 *       actions
				 * @note At the expiration of the term of `CONNECT` the `connect_t` subscription
				 *       is additionally called with a negative outcome, and this happens **before** the call
				 *       of this function
				 * @par Example: different actions — a different meaning of the answer
				 * @par The supported types of the nodes
				 * `PEER`, `ORIGIN`, `CLIENT`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * @code{.cpp}
				 * io.setTimeout(client, awh::event::action_t::READ, 30000);
				 * io.setTimeout(client, awh::event::action_t::RECONNECT, 5000);
				 * io.on(client, static_cast <awh::engine::callback::timeout_t> ([&attempts](const awh::event::id_t id, const awh::event::action_t action, const uint32_t delay) noexcept -> bool {
				 *     // The reconnection: a positive answer means "to try again"
				 *     if(action == awh::event::action_t::RECONNECT)
				 *         return (attempts++ < 3);
				 *     // The idling: a positive answer means "to tear the connection"
				 *     return true;
				 * }));
				 * @endcode
				 *
				 */
				void on(const event::id_t id, engine::callback::timeout_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки доступности очереди события
				 *
				 * @details Сообщает, что в очереди отправки освободилось место, и передаёт
				 *          доступный размер. Это обратная связь для источника данных:
				 *          отправлять следующую порцию имеет смысл отсюда, а не вслепую -
				 *          так очередь не растёт без предела, а отправка идёт со скоростью,
				 *          которую держит соединение.
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the handling of the availability of the queue of an event
				 * @details Reports that the room in the queue of the sending has been freed, and passes
				 *          the available size. This is the feedback for the source of the data:
				 *          it makes sense to send the next portion from here, and not blindly —
				 *          thus the queue does not grow without a limit, and the sending goes with the speed
				 *          which the connection holds.
				 * @par The supported types of the nodes
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 * @param id identifier of the event
				 * @param cb callback function
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::available_t cb) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки источника данных для вытягивающей модели отправки
				 *
				 * @details Разновидность отправки, обратная методу send(): движок сам просит данные
				 *          у источника ровно тогда, когда сокет готов к записи и в очереди есть
				 *          свободное место. Приложению не нужно держать в памяти всё тело - оно
				 *          выдаёт данные по мере их ухода в сеть, а движок сам держит темп по
				 *          скорости соединения. Источник снимается сам по достижении конца тела
				 * @par Поддерживаемые типы узлов
				 * `IPC`, `PEER`, `ORIGIN`, `TUNNEL`, `CLIENT`
				 * @param id идентификатор события
				 * @param cb функция обратного вызова источника данных
				 *
				 * \~english
				 * @brief Method of setting the source of the data for the pull model of the sending
				 * @par The supported types of the nodes
				 * `IPC`, `PEER`, `ORIGIN`, `TUNNEL`, `CLIENT`
				 * @param id identifier of the event
				 * @param cb callback function of the source of the data
				 *
				 * \~
				 */
				void on(const event::id_t id, engine::callback::source_t cb) noexcept;
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
				explicit IO(const fmk_t * fmk, const log_t * log) noexcept;
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
				~IO() noexcept;
		} io_t;
	};
};

#endif // __AWH_IO_ENGINE__
