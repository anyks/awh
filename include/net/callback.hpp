/**
 * @file callback.hpp
 * @date 2026-03-10
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
 * @brief Заголовочный файл прототипов функций обратного вызова сетевого движка — сигнатуры колбэков подключения,
 *        чтения, записи, закрытия, ошибок, таймеров и событий SCTP,
 *        на которые подписываются пользователи движка ввода-вывода
 *
 * \~english
 * @brief Header file of the prototypes of the callback functions of the network engine — the signatures of the callbacks of the connection,
 *        of the reading, of the writing, of the closing, of the errors, of the timers and of the SCTP events,
 *        which the users of the input-output engine subscribe to
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ENGINE_CALLBACK__
#define __AWH_ENGINE_CALLBACK__

/**
 * Стандартный заголовочный файл
 */
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "net.hpp"
#include "event.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

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
		 * \~russian
		 * @brief пространство имён работы с обратными вызовами
		 *
		 * \~english
		 * @brief namespace of the work with the callbacks
		 *
		 * \~
		 */
		namespace callback {
			/**
			 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
			 */
			#if __linux__ || __FreeBSD__ || __sun
				/**
				 * \~russian
				 * @brief Пространство имён для работы с SCTP
				 *
				 * \~english
				 * @brief Namespace for working with SCTP
				 *
				 * \~
				 */
				namespace sctp {
					/**
					 * \~russian
					 * @brief Функция обратного вызова срабатывающая при получении информационных сообщений SCTP
					 *
					 * @param id   идентификатор события
					 * @param info объект информационных метаданных SCTP
					 *
					 * \~english
					 * @brief Callback function triggered at the receipt of the informational messages of SCTP
					 * @param id   identifier of the event
					 * @param info object of the informational metadata of SCTP
					 *
					 * \~
					 */
					using minfo_t = function <void (const event::id_t, const net::sctp::minfo_t &)>;
					/**
					 * \~russian
					 * @brief Функция обратного вызова срабатывающая при получении событий SCTP
					 *
					 * @param id    идентификатор события
					 * @param event объект события SCTP
					 *
					 * \~english
					 * @brief Callback function triggered at the receipt of the events of SCTP
					 * @param id    identifier of the event
					 * @param event object of the event of SCTP
					 *
					 * \~
					 */
					using events_t = function <void (const event::id_t, unique_ptr <net::sctp::event_t>)>;
					/**
					 * \~russian
					 * @brief Функция обратного вызова срабатывающая при чтении данных SCTP вместе с метаданными
					 *
					 * @details Отклик этот необязателен и заменяет собой общий отклик чтения:
					 *          установивший его получает данные и метаданные сообщения одним
					 *          вызовом, а не двумя, и потому не сшивает их по порядку прихода
					 *
					 * @note Пока отклик не установлен, подписка на метаданные ядру не выдаётся
					 *       вовсе, и приём метаданных ничего не стоит
					 *
					 * @param id     идентификатор события
					 * @param buffer буфер прочитанных данных
					 * @param size   размер прочитанных данных
					 * @param info   метаданные полученного сообщения SCTP
					 *
					 * \~english
					 * @brief Callback function triggered at the reading of the data of SCTP together with the metadata
					 * @details This callback is optional and replaces the common callback of the reading:
					 *          the one who has set it gets the data and the metadata of a message by one
					 *          call, and not by two, and therefore does not sew them by the order of the arrival
					 * @note While the callback is not set, the subscription to the metadata is not given out to the kernel
					 *       at all, and the reception of the metadata costs nothing
					 * @param id     identifier of the event
					 * @param buffer buffer of the read data
					 * @param size   size of the read data
					 * @param info   metadata of the received message of SCTP
					 *
					 * \~
					 */
					using message_t = function <void (const event::id_t, const uint8_t *, const size_t, const net::sctp::rinfo_t &)>;
				};
			#endif
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при подключении события
			 *
			 * @param id идентификатор события
			 * @param ok результат подключения события (true - подключено, false - ошибка)
			 *
			 * \~english
			 * @brief Callback function triggered at the connection of an event
			 * @param id identifier of the event
			 * @param ok result of the connection of the event (true — connected, false — an error)
			 *
			 * \~
			 */
			using connect_t = function <void (const event::id_t, const bool)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при записи в событие
			 *
			 * @param id   идентификатор события
			 * @param size размер записанных данных
			 *
			 * \~english
			 * @brief Callback function triggered at the writing into an event
			 * @param id   identifier of the event
			 * @param size size of the written data
			 *
			 * \~
			 */
			using write_t = function <void (const event::id_t, const size_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при принятии события
			 *
			 * @param id   идентификатор события
			 * @param peer идентификатор подключившегося события
			 *
			 * \~english
			 * @brief Callback function triggered at the accepting of an event
			 * @param id   identifier of the event
			 * @param peer identifier of the connected event
			 *
			 * \~
			 */
			using accept_t = function <void (const event::id_t, const event::id_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при общем событии
			 *
			 * @param id     идентификатор события
			 * @param action тип события
			 *
			 * \~english
			 * @brief Callback function triggered at a common event
			 * @param id     identifier of the event
			 * @param action type of the event
			 *
			 * \~
			 */
			using event_t = function <void (const event::id_t, const event::action_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при изменении статуса события
			 *
			 * @param id     идентификатор события
			 * @param status новый статус события
			 *
			 * \~english
			 * @brief Callback function triggered at the change of the status of an event
			 * @param id     identifier of the event
			 * @param status new status of the event
			 *
			 * \~
			 */
			using status_t = function <void (const event::id_t, const event::status_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при получении информационных метаданных о дейтаграммном пакете
			 *
			 * @param id   идентификатор события
			 * @param info объект информационных метаданных дейтаграммного пакета
			 *
			 * \~english
			 * @brief Callback function triggered at the receipt of the informational metadata about a datagram packet
			 * @param id   identifier of the event
			 * @param info object of the informational metadata of the datagram packet
			 *
			 * \~
			 */
			using traffic_t = function <void (const event::id_t, const net::dgram_info_t &)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при чтении из события
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер прочитанных данных
			 * @param size   размер прочитанных данных
			 *
			 * \~english
			 * @brief Callback function triggered at the reading from an event
			 * @param id     identifier of the event
			 * @param buffer buffer of the read data
			 * @param size   size of the read data
			 *
			 * \~
			 */
			using read_t = function <void (const event::id_t, const uint8_t *, const size_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова инъекции объединённых данных в событие (splice)
			 *
			 * @note Позволяет транспорту, шифрующему данные на уровне соединения (QUIC),
			 *       принять перенаправленные из события-источника байты и отправить их
			 *       собственным потоком, а не записывать их сырьём в сокет
			 *
			 * @param id     идентификатор события-приёмника
			 * @param buffer буфер перенаправляемых данных
			 * @param size   размер перенаправляемых данных
			 * @return       результат приёма данных на инъекцию
			 *
			 * \~english
			 * @brief Callback function of the injection of the joined data into an event (splice)
			 * @note Allows a transport encrypting the data at the level of the connection (QUIC)
			 *       to accept the bytes redirected from the source event and to send them
			 *       by its own stream, and not to write them raw into the socket
			 * @param id     identifier of the receiver event
			 * @param buffer buffer of the redirected data
			 * @param size   size of the redirected data
			 * @return       result of the acceptance of the data for the injection
			 *
			 * \~
			 */
			using inject_t = function <bool (const event::id_t, const uint8_t *, const size_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова источника данных для вытягивающей модели отправки
			 *
			 * @details Разновидность отправки, обратная методу send(): не приложение кладёт данные
			 *          в очередь события, а движок сам просит их у источника ровно тогда, когда
			 *          сокет готов к записи и в очереди есть свободное место. Приложению не нужно
			 *          держать в памяти всё тело - оно выдаёт данные по мере их ухода в сеть
			 *
			 * @note Источник НЕ копирует данные никуда: он отдаёт указатель на свой буфер, а
			 *       движок пишет в сокет прямо оттуда. Отдать больше size байт нельзя
			 *
			 * @note Отданные данные обязаны оставаться неизменными до возврата из вызова: движок
			 *       успевает за это время записать их в сокет, а непринятый сокетом остаток
			 *       уложить в очередь. Дальше буфер снова принадлежит источнику
			 *
			 * @note Отданное принимается ЦЕЛИКОМ: ёмкость запроса движок берёт по свободному
			 *       месту очереди, поэтому остаток после сокета в неё всегда помещается. Значит
			 *       источник вправе считать отданное отправленным и сдвинуть свой курсор сразу
			 *
			 * @note Вытягивание ведёт ТОЛЬКО возвращаемое значение: `false` означает «отдавать
			 *       больше нечего», и завести отправку заново сможет лишь приложение вызовом
			 *       `send(id, nullptr, 0)`
			 *
			 * @note Возврат `true` при НУЛЕ записанных байт означает «данных пока нет либо
			 *       отданная ёмкость мала»: движок прекращает круг запросов, но источник не
			 *       забывает и вернётся к нему на ближайшем освобождении очереди. Так источник
			 *       вправе отказать в тесном участке, дожидаясь места под неделимую дейтаграмму
			 *
			 * @param id     идентификатор события
			 * @param buffer указатель, куда источник помещает адрес своих данных
			 * @param size   на входе - ёмкость запроса в байтах, на выходе - количество отданных байт
			 * @return       требуется ли продолжать вытягивание (false - отдавать больше нечего)
			 *
			 * \~english
			 * @brief Callback function of the source of the data for the pull model of the sending
			 * @note The source does NOT copy the data anywhere: it gives out a pointer to its own buffer,
			 *       and the engine writes into the socket directly from there. It is not possible to give
			 *       out more than size bytes
			 * @note The given out data are obliged to remain unchanged until the return from the call: the
			 *       engine manages for this time to write them into the socket, and to place the remainder
			 *       not accepted by the socket into the queue. Further the buffer belongs to the source again
			 * @note The given out data are accepted ENTIRELY: the capacity of the request is taken by the
			 *       engine by the free place of the queue, therefore the remainder after the socket always
			 *       is placed into it. So the source is entitled to consider the given out as sent and to
			 *       move its cursor at once
			 * @note The pulling is driven ONLY by the returned value: `false` means "there is nothing
			 *       more to give", and only the application is able to start the sending again by the
			 *       call of `send(id, nullptr, 0)`
			 * @note The return of `true` with the ZERO of the written bytes means "there is no data yet
			 *       or the given capacity is too small": the engine stops the round of the requests, but
			 *       does not forget the source and returns to it at the nearest release of the queue
			 * @param id     identifier of the event
			 * @param buffer pointer where the source places the address of its data
			 * @param size   at the input — capacity of the request in the bytes, at the output — amount of the given out bytes
			 * @return       whether it is required to continue the pulling (false — there is nothing more to give)
			 *
			 * \~
			 */
			using source_t = function <bool (const event::id_t, const uint8_t **, size_t &)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при ошибке события
			 *
			 * @param id    идентификатор события
			 * @param error код ошибки события
			 * @param text  текстовое описание ошибки события
			 *
			 * \~english
			 * @brief Callback function triggered at an error of an event
			 * @param id    identifier of the event
			 * @param error code of the error of the event
			 * @param text  text description of the error of the event
			 *
			 * \~
			 */
			using error_t = function <void (const event::id_t, const event::error_t, const string &)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при доступности очереди на отправку данных
			 *
			 * @param id     идентификатор события
			 * @param status статус события
			 * @param size   размер доступных данных для отправки
			 *
			 * \~english
			 * @brief Callback function triggered at the availability of the queue for the sending of the data
			 * @param id     identifier of the event
			 * @param status status of the event
			 * @param size   size of the available data for the sending
			 *
			 * \~
			 */
			using available_t = function <void (const event::id_t, const event::status_t, const size_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при истечении таймера события
			 *
			 * @details Сообщает, что истёк срок, заданный через `setTimeout()`: соединение
			 *          не приняло данных (READ), не смогло их отправить (WRITE), не
			 *          установилось (CONNECT) или подошла пора повторить попытку
			 *          (RECONNECT)
			 *
			 * @warning Смысл возвращаемого признака **зависит от действия**. Для READ,
			 *          WRITE и CONNECT положительный признак узел уничтожает, а
			 *          отрицательный оставляет жить. Для RECONNECT всё наоборот:
			 *          положительный означает «переподключаться», а прервать попытку
			 *          нужно как раз отрицательным. Одна и та же функция, обслуживающая
			 *          все действия сразу, обязана эти случаи различать
			 *
			 * @note Если функция не установлена, узел по истечении срока READ, WRITE или
			 *       CONNECT уничтожается **безусловно**. То есть подписка нужна ровно
			 *       затем, чтобы обрыв предотвратить или обставить своими действиями
			 *
			 * @note По истечении срока CONNECT дополнительно вызывается `connect_t` с
			 *       отрицательным исходом, и происходит это **до** вызова этой функции
			 *
			 * @param id     идентификатор события
			 * @param action на чём истёк срок: READ, WRITE, CONNECT или RECONNECT
			 * @param delay  заданный срок в миллисекундах
			 * @return       для READ, WRITE и CONNECT - уничтожать ли узел; для RECONNECT -
			 *               выполнять ли переподключение
			 *
			 * \~english
			 * @brief Callback function triggered at the expiration of the timer of an event
			 * @details Reports that the term set through `setTimeout()` has expired: the connection
			 *          has not accepted the data (READ), has not managed to send it (WRITE), has not
			 *          been established (CONNECT) or the time has come to repeat the attempt
			 *          (RECONNECT)
			 * @warning The meaning of the returned sign **depends on the action**. For READ,
			 *          WRITE and CONNECT a positive sign destroys the node, and
			 *          a negative one leaves it alive. For RECONNECT everything is the other way round:
			 *          a positive one means «reconnect», and the attempt should be interrupted
			 *          exactly by a negative one. One and the same function serving
			 *          all the actions at once is obliged to tell these cases apart
			 * @note If the function is not set, the node at the expiration of the term of READ, WRITE or
			 *       CONNECT is destroyed **unconditionally**. That is the subscription is needed exactly
			 *       in order to prevent the break or to surround it with one's own actions
			 * @note At the expiration of the term of CONNECT `connect_t` is additionally called with
			 *       a negative outcome, and this happens **before** the call of this function
			 * @param id     identifier of the event
			 * @param action on what the term has expired: READ, WRITE, CONNECT or RECONNECT
			 * @param delay  the set term in milliseconds
			 * @return       for READ, WRITE and CONNECT — whether the node should be destroyed; for RECONNECT —
			 *               whether the reconnection should be performed
			 *
			 * \~
			 */
			using timeout_t = function <bool (const event::id_t, const event::action_t, const uint32_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при определении сессии дейтаграммного пакета
			 *
			 * @note Вызывается до маршрутизации датаграммы и позволяет протоколам с
			 *       собственной адресацией сессий задать ключ маршрутизации самим.
			 *       Отрицательный результат означает, что датаграмма протоколу не
			 *       принадлежит и подлежит отбрасыванию без создания сессии
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер принятой датаграммы
			 * @param size   размер принятой датаграммы
			 * @param key    выводимый ключ сессии
			 * @return       результат определения сессии
			 *
			 * \~english
			 * @brief Callback function triggered at the determination of the session of a datagram packet
			 * @note Is called before the routing of the datagram and allows the protocols with
			 *       their own addressing of the sessions to set the key of the routing themselves.
			 *       A negative result means that the datagram does not belong to the protocol
			 *       and is subject to the discarding without the creation of a session
			 * @param id     identifier of the event
			 * @param buffer buffer of the received datagram
			 * @param size   size of the received datagram
			 * @param key    yielded key of the session
			 * @return       result of the determination of the session
			 *
			 * \~
			 */
			using origin_t = function <bool (const event::id_t, const uint8_t *, const size_t, net::origin_key_t &)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова возвращающая неотправленные данные события
			 *
			 * @details Вызывается, когда отправить данные не удалось: очередь события
			 *          переполнена, соединение оборвано или запись отклонена. Байты
			 *          возвращаются вызывающей стороне, чтобы та решила их судьбу сама -
			 *          повторить отправку позже, отложить в свой буфер или отбросить.
			 *          Движок их не сохраняет и после возврата из функции освобождает
			 *
			 * @note Буфер действителен ТОЛЬКО на время вызова функции обратного вызова.
			 *       Данные, нужные после выхода, следует скопировать
			 *
			 * @param id     идентификатор события
			 * @param error  откуда возвращены данные: из самого события (IO_EVENT) или
			 *               из его очереди отправки (IO_QUEUE)
			 * @param buffer буфер данных события
			 * @param size   размер данных события
			 *
			 * \~english
			 * @brief Callback function returning the unsent data of an event
			 * @details Is called when the data could not be sent: the queue of the event
			 *          is overflowed, the connection is broken or the writing is rejected. The bytes
			 *          are returned to the calling side so that it would decide their fate itself —
			 *          to repeat the sending later, to put them aside into its own buffer or to discard them.
			 *          The engine does not save them and after the return from the function releases them
			 * @note The buffer is valid ONLY for the time of the call of the callback function.
			 *       The data needed after the exit should be copied
			 * @param id     identifier of the event
			 * @param error  where the data is returned from: from the event itself (IO_EVENT) or
			 *               from its queue of the sending (IO_QUEUE)
			 * @param buffer buffer of the data of the event
			 * @param size   size of the data of the event
			 *
			 * \~
			 */
			using spool_t = function <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при изменении каталога
			 *
			 * @note Параметр name действителен ТОЛЬКО на время вызова функции обратного вызова
			 *
			 * @param id     идентификатор события
			 * @param action тип события
			 * @param vnode  виртуальный узел события
			 * @param name   имя изменённого файла/каталога события
			 *
			 * \~english
			 * @brief Callback function triggered at the change of a directory
			 * @note The name parameter is valid ONLY for the time of the call of the callback function
			 * @param id     identifier of the event
			 * @param action type of the event
			 * @param vnode  virtual node of the event
			 * @param name   name of the changed file/directory of the event
			 *
			 * \~
			 */
			using vnode_t = function <void (const event::id_t, const event::action_t, const event::vnode_t, const string &)>;
			/**
			 * \~russian
			 * @brief Функция обратного вызова срабатывающая при получении информации о пакетах в туннельном интерфейсе
			 *
			 * @param id     идентификатор события
			 * @param peer   идентификатор удалённого узла
			 * @param action тип события
			 * @param info   информация о пакетах в туннельном интерфейсе
			 *
			 * \~english
			 * @brief Callback function triggered at the receipt of the information about the packets in a tunnel interface
			 * @param id     identifier of the event
			 * @param peer   identifier of the remote node
			 * @param action type of the event
			 * @param info   information about the packets in the tunnel interface
			 *
			 * \~
			 */
			using tuninfo_t = function <void (const event::id_t, const event::id_t, const event::action_t, const net::tun_info_t &)>;
		};
	};
};

#endif // __AWH_ENGINE_CALLBACK__
