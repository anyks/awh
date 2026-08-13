/**
 * @file: socket.hpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл подмены системных вызовов работы с сокетами под Linux —
 *        посредники, разводящие обращения между ядром и обходными стеками,
 *        работающими в пространстве пользователя
 *
 * @details Стеки вроде DPDK через F-Stack уносят обработку пакетов из ядра в
 *          пространство пользователя, и обращаться к ним системными вызовами нельзя:
 *          их сокеты ядру не принадлежат вовсе. Взамен предлагается собственный набор
 *          вызовов - `ff_socket`, `ff_sendto` и прочие, - совпадающий с POSIX по
 *          смыслу и по составу доводов, но не по имени
 *
 *          Посредники эти сводят расхождение к одному месту: модули сети зовут
 *          `gnu::socket`, а во что оно развернётся - в системный вызов или в обходной
 *          стек - решается при сборке. Ветка выбирается **на сборку целиком**, а не
 *          на подключение: смешивать стеки в одном приложении нельзя
 *
 * @par Намеренные решения
 *
 *      **Посредники, а не макросы.** В черновике подмена делалась макросами вида
 *      `#define socket_compat(...)`. Здесь она сделана встраиваемыми функциями:
 *      макрос не знает типов, не поддаётся отладке и захватывает имя во всём
 *      переводимом наборе, включая чужие заголовки. Посредник с `always_inline`
 *      разворачивается в тот же самый вызов - ни одной лишней машинной команды, -
 *      но проверяет доводы и живёт в своём пространстве имён
 *
 *      **Отказ вместо тишины.** Ветка `USE_XDP` в черновике была объявлена, но не
 *      написана. Собранное с нею молчаливо не получало ни одного объявления, и
 *      сборка разваливалась сотней невнятных отказов в чужих модулях. Здесь такая
 *      сборка отвергается сразу и по существу
 *
 * @note Набор посредников отвечает тому, чем модули сети действительно пользуются.
 *       Полной заменой POSIX он не является и таковой считаться не должен
 *
 * @warning Обходные стеки выдают **свои** описатели, а не описатели ядра. Смешивать
 *          их с обычными нельзя: описатель, полученный от `gnu::socket`, закрывать
 *          следует только через `gnu::close`, и передавать его в системные вызовы
 *          напрямую недопустимо
 *
 * \~english
 * @brief Header file of the substitution of the system calls of the work with the sockets under Linux —
 *        the mediators dividing the addresses between the kernel and the bypassing stacks
 *        working in the user space
 * @details The stacks like DPDK through F-Stack take the handling of the packets out of the kernel into
 *          the user space, and they cannot be addressed by the system calls:
 *          their sockets do not belong to the kernel at all. In exchange their own set of
 *          the calls is offered — `ff_socket`, `ff_sendto` and the others, — coinciding with POSIX in
 *          the meaning and in the composition of the arguments, but not in the name
 *          These mediators reduce the divergence to one place: the modules of the network call
 *          `gnu::socket`, and what it will unfold into — into a system call or into a bypassing
 *          stack — is decided at the build. The branch is chosen **for the whole build**, and not
 *          per connection: mixing the stacks in one application is not allowed
 * @par Deliberate decisions
 *      **Mediators, and not macros.** In the draft the substitution was made by the macros of the kind
 *      `#define socket_compat(...)`. Here it is made by inlined functions:
 *      a macro does not know the types, does not yield to the debugging and captures the name in the whole
 *      translated set, including the foreign headers. A mediator with `always_inline`
 *      unfolds into the very same call — not a single extra machine instruction, —
 *      but checks the arguments and lives in its own namespace
 *      **A refusal instead of the silence.** The `USE_XDP` branch in the draft was declared, but not
 *      written. What was built with it silently received not a single declaration, and
 *      the build fell apart with a hundred of unintelligible refusals in the foreign modules. Here such
 *      a build is rejected at once and in essence
 * @note The set of the mediators answers what the modules of the network actually use.
 *       It is not a full replacement of POSIX and must not be considered as one
 * @warning The bypassing stacks give out **their own** handles, and not the handles of the kernel. Mixing
 *          them with the ordinary ones is not allowed: a handle obtained from `gnu::socket` should be closed
 *          only through `gnu::close`, and passing it into the system calls
 *          directly is inadmissible
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GNU_SOCKET__
#define __AWH_GNU_SOCKET__

/**
 * Модуль предназначен только для операционной системы Linux
 */
#if __linux__

/**
 * Стандартная библиотека
 */
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/**
 * \~russian
 * Если собирается движок io_uring
 *
 * @details Заголовок ядра нужен исключительно ради устройства структур колец.
 *          Коды операций и признаки движок объявляет своими числами: заголовок
 *          дистрибутива отстаёт от ядра, и опираться на него значило бы молча
 *          потерять возможности, которые ядро даёт. Подробности - в
 *          src/net/backend/gnu/IO_URING.md
 *
 * \~english
 * If the io_uring engine is being built
 * @details The header of the kernel is needed exclusively for the sake of the layout of the structures of the rings.
 *          The codes of the operations and the signs the engine declares by its own numbers: the header
 *          of the distribution lags behind the kernel, and to rely on it would mean silently
 *          losing the possibilities the kernel gives. The details are in
 *          src/net/backend/gnu/IO_URING.md
 *
 * \~
 */
#if defined(AWH_ENGINE_IO_URING)
	#include <sys/syscall.h>
	#include <linux/io_uring.h>
#endif

/**
 * Наши модули
 */
#include "../../net.hpp"

/**
 * Если затребован обходной стек XDP
 */
#if defined(USE_XDP)
	#error "AWH: XDP bypass stack not supported yet, build without USE_XDP"
#endif

/**
 * Если затребован обходной стек DPDK через F-Stack
 */
#if defined(USE_FSTACK)
	/**
	 * Заголовочные файлы обходного стека
	 */
	#include <ff.h>
	#include <ff_epoll.h>
#endif

/**
 * \~russian
 * Принудительная подстановка средствами GCC и Clang
 *
 * @details Посредники обязаны разворачиваться в место обращения целиком: смысл их в
 *          том, чтобы не стоить ничего сверх самого вызова
 *
 * \~english
 * Forced substitution by the means of GCC and Clang
 * @details The mediators are obliged to unfold into the place of the address entirely: their point is in
 *          costing nothing beyond the call itself
 *
 * \~
 */
#define AWH_GNU_INLINE inline __attribute__((always_inline))

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
	 * @brief Пространство имён средств операционной системы Linux
	 *
	 * @details Обращения записываются с явным указанием пространства -
	 *          `gnu::socket`, `gnu::sendto`, - и потому никогда не путаются с
	 *          одноимёнными вызовами системы
	 *
	 * \~english
	 * @brief Namespace of the means of the Linux operating system
	 * @details The addresses are written with the namespace specified explicitly —
	 *          `gnu::socket`, `gnu::sendto`, — and therefore are never confused with
	 *          the calls of the system of the same name
	 *
	 * \~
	 */
	namespace gnu {
		/**
		 * Если работа ведётся через обходной стек DPDK
		 */
		#if defined(USE_FSTACK)
			// Признак работы через обходной стек
			static constexpr bool BYPASS = true;
		/**
		 * Если работа ведётся средствами ядра
		 */
		#else
			// Признак работы через обходной стек
			static constexpr bool BYPASS = false;
		#endif
		/**
		 * \~russian
		 * @brief Метод создания сокета
		 *
		 * @param domain   семейство протоколов
		 * @param type     тип сокета
		 * @param protocol протокол сокета
		 * @return         созданный сокет либо признак ошибки
		 *
		 * \~english
		 * @brief Method of creating a socket
		 * @param domain   family of the protocols
		 * @param type     type of the socket
		 * @param protocol protocol of the socket
		 * @return         the created socket or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE net::socket_t socket(const int32_t domain, const int32_t type, const int32_t protocol) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return static_cast <net::socket_t> (::ff_socket(domain, type, protocol));
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return static_cast <net::socket_t> (::socket(domain, type, protocol));
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод привязки сокета к адресу
		 *
		 * @param sock сетевой сокет
		 * @param addr адрес для привязки
		 * @param size размер адреса
		 * @return     результат работы функции
		 *
		 * \~english
		 * @brief Method of binding a socket to an address
		 * @param sock network socket
		 * @param addr address to bind to
		 * @param size size of the address
		 * @return     result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t bind(const net::socket_t sock, const struct sockaddr * addr, const socklen_t size) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_bind(static_cast <int32_t> (sock), const_cast <struct linux_sockaddr *> (reinterpret_cast <const struct linux_sockaddr *> (addr)), size);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::bind(static_cast <int32_t> (sock), addr, size);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод перевода сокета в режим ожидания подключений
		 *
		 * @param sock    сетевой сокет
		 * @param backlog размер очереди подключений
		 * @return        результат работы функции
		 *
		 * \~english
		 * @brief Method of putting a socket into the mode of the waiting for the connections
		 * @param sock    network socket
		 * @param backlog size of the queue of the connections
		 * @return        result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t listen(const net::socket_t sock, const int32_t backlog) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_listen(static_cast <int32_t> (sock), backlog);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::listen(static_cast <int32_t> (sock), backlog);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод принятия входящего подключения
		 *
		 * @param sock сетевой сокет
		 * @param addr адрес подключившегося клиента
		 * @param size размер адреса
		 * @return     сокет подключения либо признак ошибки
		 *
		 * \~english
		 * @brief Method of accepting an incoming connection
		 * @param sock network socket
		 * @param addr address of the connected client
		 * @param size size of the address
		 * @return     socket of the connection or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE net::socket_t accept(const net::socket_t sock, struct sockaddr * addr, socklen_t * size) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return static_cast <net::socket_t> (::ff_accept(static_cast <int32_t> (sock), reinterpret_cast <struct linux_sockaddr *> (addr), size));
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return static_cast <net::socket_t> (::accept(static_cast <int32_t> (sock), addr, size));
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод установки подключения
		 *
		 * @param sock сетевой сокет
		 * @param addr адрес подключения
		 * @param size размер адреса
		 * @return     результат работы функции
		 *
		 * \~english
		 * @brief Method of establishing a connection
		 * @param sock network socket
		 * @param addr address of the connection
		 * @param size size of the address
		 * @return     result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t connect(const net::socket_t sock, const struct sockaddr * addr, const socklen_t size) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_connect(static_cast <int32_t> (sock), const_cast <struct linux_sockaddr *> (reinterpret_cast <const struct linux_sockaddr *> (addr)), size);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::connect(static_cast <int32_t> (sock), addr, size);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод закрытия сокета
		 *
		 * @param sock сетевой сокет
		 * @return     результат работы функции
		 *
		 * \~english
		 * @brief Method of closing a socket
		 * @param sock network socket
		 * @return     result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t close(const net::socket_t sock) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_close(static_cast <int32_t> (sock));
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::close(static_cast <int32_t> (sock));
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод прекращения обмена по сокету
		 *
		 * @param sock сетевой сокет
		 * @param how  направление прекращения обмена
		 * @return     результат работы функции
		 *
		 * \~english
		 * @brief Method of stopping the exchange over a socket
		 * @param sock network socket
		 * @param how  direction of the stopping of the exchange
		 * @return     result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t shutdown(const net::socket_t sock, const int32_t how) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_shutdown(static_cast <int32_t> (sock), how);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::shutdown(static_cast <int32_t> (sock), how);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод чтения данных из сокета
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер для чтения данных
		 * @param size   размер буфера
		 * @param flags  признаки чтения
		 * @return       количество прочитанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of reading the data from a socket
		 * @param sock   network socket
		 * @param buffer buffer to read the data into
		 * @param size   size of the buffer
		 * @param flags  signs of the reading
		 * @return       number of the read bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t recv(const net::socket_t sock, void * buffer, const size_t size, const int32_t flags) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_recv(static_cast <int32_t> (sock), buffer, size, flags);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::recv(static_cast <int32_t> (sock), buffer, size, flags);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод отправки данных в сокет
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер данных для отправки
		 * @param size   размер буфера
		 * @param flags  признаки отправки
		 * @return       количество отправленных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of sending the data into a socket
		 * @param sock   network socket
		 * @param buffer buffer of the data to send
		 * @param size   size of the buffer
		 * @param flags  signs of the sending
		 * @return       number of the sent bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t send(const net::socket_t sock, const void * buffer, const size_t size, const int32_t flags) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_send(static_cast <int32_t> (sock), buffer, size, flags);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::send(static_cast <int32_t> (sock), buffer, size, flags);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод чтения данных из сокета с получением адреса отправителя
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер для чтения данных
		 * @param size   размер буфера
		 * @param flags  признаки чтения
		 * @param addr   адрес отправителя
		 * @param length размер адреса отправителя
		 * @return       количество прочитанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of reading the data from a socket with the getting of the address of the sender
		 * @param sock   network socket
		 * @param buffer buffer to read the data into
		 * @param size   size of the buffer
		 * @param flags  signs of the reading
		 * @param addr   address of the sender
		 * @param length size of the address of the sender
		 * @return       number of the read bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t recvfrom(const net::socket_t sock, void * buffer, const size_t size, const int32_t flags, struct sockaddr * addr, socklen_t * length) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_recvfrom(static_cast <int32_t> (sock), buffer, size, flags, reinterpret_cast <struct linux_sockaddr *> (addr), length);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::recvfrom(static_cast <int32_t> (sock), buffer, size, flags, addr, length);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод отправки данных в сокет по указанному адресу
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер данных для отправки
		 * @param size   размер буфера
		 * @param flags  признаки отправки
		 * @param addr   адрес получателя
		 * @param length размер адреса получателя
		 * @return       количество отправленных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of sending the data into a socket to the specified address
		 * @param sock   network socket
		 * @param buffer buffer of the data to send
		 * @param size   size of the buffer
		 * @param flags  signs of the sending
		 * @param addr   address of the receiver
		 * @param length size of the address of the receiver
		 * @return       number of the sent bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t sendto(const net::socket_t sock, const void * buffer, const size_t size, const int32_t flags, const struct sockaddr * addr, const socklen_t length) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_sendto(static_cast <int32_t> (sock), buffer, size, flags, const_cast <struct linux_sockaddr *> (reinterpret_cast <const struct linux_sockaddr *> (addr)), length);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::sendto(static_cast <int32_t> (sock), buffer, size, flags, addr, length);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод чтения данных из сокета единым сообщением
		 *
		 * @details Через сообщение приходят и служебные сведения: метки перегрузки
		 *          сети, адрес назначения, признак усечения. Протоколу QUIC они нужны
		 *          напрямую, и обойтись простым чтением там нельзя
		 *
		 * @param sock    сетевой сокет
		 * @param message сообщение для чтения данных
		 * @param flags   признаки чтения
		 * @return        количество прочитанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of reading the data from a socket as a single message
		 * @details Through a message the service information comes as well: the marks of the congestion of
		 *          the network, the address of the destination, the sign of the truncation. The QUIC protocol needs them
		 *          directly, and getting by with a simple reading there is not possible
		 * @param sock    network socket
		 * @param message message to read the data into
		 * @param flags   signs of the reading
		 * @return        number of the read bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t recvmsg(const net::socket_t sock, struct msghdr * message, const int32_t flags) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_recvmsg(static_cast <int32_t> (sock), message, flags);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::recvmsg(static_cast <int32_t> (sock), message, flags);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод отправки данных в сокет единым сообщением
		 *
		 * @param sock    сетевой сокет
		 * @param message сообщение для отправки данных
		 * @param flags   признаки отправки
		 * @return        количество отправленных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of sending the data into a socket as a single message
		 * @param sock    network socket
		 * @param message message to send the data from
		 * @param flags   signs of the sending
		 * @return        number of the sent bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t sendmsg(const net::socket_t sock, const struct msghdr * message, const int32_t flags) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_sendmsg(static_cast <int32_t> (sock), message, flags);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::sendmsg(static_cast <int32_t> (sock), message, flags);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод чтения данных из описателя
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер для чтения данных
		 * @param size   размер буфера
		 * @return       количество прочитанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of reading the data from a handle
		 * @param sock   network socket
		 * @param buffer buffer to read the data into
		 * @param size   size of the buffer
		 * @return       number of the read bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t read(const net::socket_t sock, void * buffer, const size_t size) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_read(static_cast <int32_t> (sock), buffer, size);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::read(static_cast <int32_t> (sock), buffer, size);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод записи данных в описатель
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер данных для записи
		 * @param size   размер буфера
		 * @return       количество записанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of writing the data into a handle
		 * @param sock   network socket
		 * @param buffer buffer of the data to write
		 * @param size   size of the buffer
		 * @return       number of the written bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t write(const net::socket_t sock, const void * buffer, const size_t size) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_write(static_cast <int32_t> (sock), buffer, size);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::write(static_cast <int32_t> (sock), buffer, size);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод чтения данных из описателя набором буферов
		 *
		 * @param sock   сетевой сокет
		 * @param buffer набор буферов для чтения данных
		 * @param count  количество буферов в наборе
		 * @return       количество прочитанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of reading the data from a handle by a set of buffers
		 * @param sock   network socket
		 * @param buffer set of the buffers to read the data into
		 * @param count  number of the buffers in the set
		 * @return       number of the read bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t readv(const net::socket_t sock, const struct iovec * buffer, const int32_t count) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_readv(static_cast <int32_t> (sock), buffer, count);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::readv(static_cast <int32_t> (sock), buffer, count);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод записи данных в описатель набором буферов
		 *
		 * @param sock   сетевой сокет
		 * @param buffer набор буферов данных для записи
		 * @param count  количество буферов в наборе
		 * @return       количество записанных байт либо признак ошибки
		 *
		 * \~english
		 * @brief Method of writing the data into a handle by a set of buffers
		 * @param sock   network socket
		 * @param buffer set of the buffers of the data to write
		 * @param count  number of the buffers in the set
		 * @return       number of the written bytes or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE ssize_t writev(const net::socket_t sock, const struct iovec * buffer, const int32_t count) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_writev(static_cast <int32_t> (sock), buffer, count);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::writev(static_cast <int32_t> (sock), buffer, count);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод получения настройки сокета
		 *
		 * @param sock   сетевой сокет
		 * @param level  уровень настройки
		 * @param name   название настройки
		 * @param value  значение настройки
		 * @param length размер значения настройки
		 * @return       результат работы функции
		 *
		 * \~english
		 * @brief Method of getting a setting of a socket
		 * @param sock   network socket
		 * @param level  level of the setting
		 * @param name   name of the setting
		 * @param value  value of the setting
		 * @param length size of the value of the setting
		 * @return       result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t getsockopt(const net::socket_t sock, const int32_t level, const int32_t name, void * value, socklen_t * length) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_getsockopt(static_cast <int32_t> (sock), level, name, value, length);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::getsockopt(static_cast <int32_t> (sock), level, name, value, length);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод установки настройки сокета
		 *
		 * @param sock   сетевой сокет
		 * @param level  уровень настройки
		 * @param name   название настройки
		 * @param value  значение настройки
		 * @param length размер значения настройки
		 * @return       результат работы функции
		 *
		 * \~english
		 * @brief Method of setting a setting of a socket
		 * @param sock   network socket
		 * @param level  level of the setting
		 * @param name   name of the setting
		 * @param value  value of the setting
		 * @param length size of the value of the setting
		 * @return       result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t setsockopt(const net::socket_t sock, const int32_t level, const int32_t name, const void * value, const socklen_t length) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_setsockopt(static_cast <int32_t> (sock), level, name, value, length);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::setsockopt(static_cast <int32_t> (sock), level, name, value, length);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод получения собственного адреса сокета
		 *
		 * @param sock   сетевой сокет
		 * @param addr   адрес сокета
		 * @param length размер адреса
		 * @return       результат работы функции
		 *
		 * \~english
		 * @brief Method of getting the own address of a socket
		 * @param sock   network socket
		 * @param addr   address of the socket
		 * @param length size of the address
		 * @return       result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t getsockname(const net::socket_t sock, struct sockaddr * addr, socklen_t * length) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_getsockname(static_cast <int32_t> (sock), reinterpret_cast <struct linux_sockaddr *> (addr), length);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::getsockname(static_cast <int32_t> (sock), addr, length);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод получения адреса другого конца подключения
		 *
		 * @param sock   сетевой сокет
		 * @param addr   адрес другого конца подключения
		 * @param length размер адреса
		 * @return       результат работы функции
		 *
		 * \~english
		 * @brief Method of getting the address of the other end of a connection
		 * @param sock   network socket
		 * @param addr   address of the other end of the connection
		 * @param length size of the address
		 * @return       result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t getpeername(const net::socket_t sock, struct sockaddr * addr, socklen_t * length) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_getpeername(static_cast <int32_t> (sock), reinterpret_cast <struct linux_sockaddr *> (addr), length);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::getpeername(static_cast <int32_t> (sock), addr, length);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод управления описателем сокета
		 *
		 * @details Довод здесь передаётся набором переменной длины: у разных запросов
		 *          он разного вида, а у части его нет вовсе
		 *
		 * @param sock    сетевой сокет
		 * @param request запрос управления
		 * @return        результат работы функции
		 *
		 * \~english
		 * @brief Method of the control of the handle of a socket
		 * @details The argument here is passed as a set of a variable length: at different requests
		 *          it is of a different kind, and at a part of them it is absent at all
		 * @param sock    network socket
		 * @param request request of the control
		 * @return        result of the work of the function
		 *
		 * \~
		 */
		template <typename... Args>
		AWH_GNU_INLINE int32_t fcntl(const net::socket_t sock, const int32_t request, Args... args) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_fcntl(static_cast <int32_t> (sock), request, args...);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::fcntl(static_cast <int32_t> (sock), request, args...);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод управления устройством через сокет
		 *
		 * @param sock    сетевой сокет
		 * @param request запрос управления
		 * @param data    данные запроса
		 * @return        результат работы функции
		 *
		 * \~english
		 * @brief Method of the control of a device through a socket
		 * @param sock    network socket
		 * @param request request of the control
		 * @param data    data of the request
		 * @return        result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t ioctl(const net::socket_t sock, const uint64_t request, void * data) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_ioctl(static_cast <int32_t> (sock), request, data);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::ioctl(static_cast <int32_t> (sock), request, data);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод ожидания событий на наборе описателей
		 *
		 * @param fds     набор описателей
		 * @param count   количество описателей в наборе
		 * @param timeout предел ожидания в миллисекундах
		 * @return        количество описателей с событиями либо признак ошибки
		 *
		 * \~english
		 * @brief Method of waiting for the events on a set of handles
		 * @param fds     set of the handles
		 * @param count   number of the handles in the set
		 * @param timeout limit of the waiting in milliseconds
		 * @return        number of the handles with the events or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t poll(struct pollfd * fds, const nfds_t count, const int32_t timeout) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_poll(fds, count, timeout);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::poll(fds, count, timeout);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод создания набора ожидания событий
		 *
		 * @details Признаки создания передаются напрямую, и обращение это всегда
		 *          пользуется `epoll_create1`: устаревший `epoll_create` не умеет
		 *          закрывать описатель при запуске другой программы, отчего тот
		 *          протекает в порождённые процессы
		 *
		 * @param flags признаки создания набора
		 * @return      описатель набора ожидания либо признак ошибки
		 *
		 * \~english
		 * @brief Method of creating a set of the waiting for the events
		 * @details The signs of the creation are passed directly, and this address always
		 *          uses `epoll_create1`: the obsolete `epoll_create` cannot
		 *          close the handle at the start of another program, and therefore it
		 *          leaks into the spawned processes
		 * @param flags signs of the creation of the set
		 * @return      handle of the set of the waiting or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t epollCreate(const int32_t flags) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_epoll_create(flags);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::epoll_create1(flags);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод управления набором ожидания событий
		 *
		 * @param epfd  описатель набора ожидания
		 * @param op    действие над описателем
		 * @param sock  сетевой сокет
		 * @param event событие описателя
		 * @return      результат работы функции
		 *
		 * \~english
		 * @brief Method of the control of a set of the waiting for the events
		 * @param epfd  handle of the set of the waiting
		 * @param op    action over the handle
		 * @param sock  network socket
		 * @param event event of the handle
		 * @return      result of the work of the function
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t epollCtl(const int32_t epfd, const int32_t op, const net::socket_t sock, struct epoll_event * event) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_epoll_ctl(epfd, op, static_cast <int32_t> (sock), event);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::epoll_ctl(epfd, op, static_cast <int32_t> (sock), event);
			#endif
		}
		/**
		 * \~russian
		 * @brief Метод ожидания событий на наборе
		 *
		 * @param epfd    описатель набора ожидания
		 * @param events  набор произошедших событий
		 * @param count   размер набора событий
		 * @param timeout предел ожидания в миллисекундах
		 * @return        количество произошедших событий либо признак ошибки
		 *
		 * \~english
		 * @brief Method of waiting for the events on a set
		 * @param epfd    handle of the set of the waiting
		 * @param events  set of the happened events
		 * @param count   size of the set of the events
		 * @param timeout limit of the waiting in milliseconds
		 * @return        number of the happened events or a sign of an error
		 *
		 * \~
		 */
		AWH_GNU_INLINE int32_t epollWait(const int32_t epfd, struct epoll_event * events, const int32_t count, const int32_t timeout) noexcept {
			/**
			 * Если работа ведётся через обходной стек DPDK
			 */
			#if defined(USE_FSTACK)
				return ::ff_epoll_wait(epfd, events, count, timeout);
			/**
			 * Если работа ведётся средствами ядра
			 */
			#else
				return ::epoll_wait(epfd, events, count, timeout);
			#endif
		}
		/**
		 * Если собирается движок io_uring
		 */
		#if defined(AWH_ENGINE_IO_URING)
			/**
			 * \~russian
			 * @brief Метод устройства колец io_uring
			 *
			 * @details Обходного стека здесь нет и быть не может: io_uring - средство
			 *          ядра, а DPDK ядро как раз обходит. Оттого посредник в отличие от
			 *          соседних ветвления не имеет
			 *
			 * @note Библиотека liburing намеренно не используется: на стендах её нет
			 *       ни на одном, а обращений к ядру у io_uring всего три, и обходятся
			 *       они своими силами без внешней зависимости
			 *
			 * @param entries количество записей кольца подачи
			 * @param params  устройство колец, заполняется ядром
			 * @return        дескриптор колец либо признак ошибки
			 *
			 * \~english
			 * @brief Method of the setup of the io_uring rings
			 * @details There is no bypassing stack here and there cannot be one: io_uring is a means
			 *          of the kernel, and DPDK bypasses the kernel exactly. Therefore the mediator, unlike
			 *          the neighbouring ones, has no branching
			 * @note The liburing library is deliberately not used: it is present on none
			 *       of the stands, and io_uring has only three addresses to the kernel, and they
			 *       get by on their own without an external dependency
			 * @param entries number of the records of the ring of the submission
			 * @param params  layout of the rings, is filled by the kernel
			 * @return        descriptor of the rings or a sign of an error
			 *
			 * \~
			 */
			AWH_GNU_INLINE int32_t ioUringSetup(const uint32_t entries, struct io_uring_params * params) noexcept {
				return static_cast <int32_t> (::syscall(__NR_io_uring_setup, entries, params));
			}
			/**
			 * \~russian
			 * @brief Метод подачи операций и ожидания завершений io_uring
			 *
			 * @details Одним обращением выполняются оба действия сразу: подаётся всё,
			 *          что положено в кольцо подачи, и ожидается указанное число
			 *          завершений. Этим io_uring и отличается от epoll, где подписка и
			 *          ожидание - два разных обращения
			 *
			 * @param fd        дескриптор колец
			 * @param submitted количество поданных записей
			 * @param waiting   количество ожидаемых завершений, ноль - не ожидать
			 * @param flags     признаки обращения
			 * @param arg       дополнительный довод, зависит от признаков
			 * @param size      размер дополнительного довода
			 * @return          количество принятых записей либо признак ошибки
			 *
			 * \~english
			 * @brief Method of the submission of the operations and of the waiting for the completions of io_uring
			 * @details By one address both actions are performed at once: everything that is put into
			 *          the ring of the submission is submitted, and the specified number of the completions
			 *          is waited for. That is how io_uring differs from epoll, where the subscription and
			 *          the waiting are two different addresses
			 * @param fd        descriptor of the rings
			 * @param submitted number of the submitted records
			 * @param waiting   number of the awaited completions, zero — do not wait
			 * @param flags     signs of the address
			 * @param arg       additional argument, depends on the signs
			 * @param size      size of the additional argument
			 * @return          number of the accepted records or a sign of an error
			 *
			 * \~
			 */
			AWH_GNU_INLINE int32_t ioUringEnter(const int32_t fd, const uint32_t submitted, const uint32_t waiting, const uint32_t flags, const void * arg, const size_t size) noexcept {
				return static_cast <int32_t> (::syscall(__NR_io_uring_enter, fd, submitted, waiting, flags, arg, size));
			}
			/**
			 * \~russian
			 * @brief Метод закрепления ресурсов за кольцами io_uring
			 *
			 * @details Через него выполняется и проба возможностей ядра, и закрепление
			 *          колец буферов приёма, и закрепление дескрипторов
			 *
			 * @param fd    дескриптор колец
			 * @param op    действие закрепления
			 * @param arg   довод действия
			 * @param count количество записей довода
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method of the registration of the resources with the io_uring rings
			 * @details Through it both the probing of the possibilities of the kernel is performed, and the registration of
			 *          the rings of the buffers of the reception, and the registration of the descriptors
			 * @param fd    descriptor of the rings
			 * @param op    action of the registration
			 * @param arg   argument of the action
			 * @param count number of the records of the argument
			 * @return      result of the work of the function
			 *
			 * \~
			 */
			AWH_GNU_INLINE int32_t ioUringRegister(const int32_t fd, const uint32_t op, void * arg, const uint32_t count) noexcept {
				return static_cast <int32_t> (::syscall(__NR_io_uring_register, fd, op, arg, count));
			}
		#endif
	};
};

#endif // __linux__

#endif // __AWH_GNU_SOCKET__
