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
 * Наши модули
 */
#include "../../net.hpp"

/**
 * Если затребован обходной стек XDP
 */
#if defined(USE_XDP)
	#error "AWH: обходной стек XDP пока не поддерживается, соберите без USE_XDP"
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
 * Принудительная подстановка средствами GCC и Clang
 *
 * @details Посредники обязаны разворачиваться в место обращения целиком: смысл их в
 *          том, чтобы не стоить ничего сверх самого вызова
 */
#define AWH_GNU_INLINE inline __attribute__((always_inline))

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён средств операционной системы Linux
	 *
	 * @details Обращения записываются с явным указанием пространства -
	 *          `gnu::socket`, `gnu::sendto`, - и потому никогда не путаются с
	 *          одноимёнными вызовами системы
	 *
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
		 * @brief Метод создания сокета
		 *
		 * @param domain   семейство протоколов
		 * @param type     тип сокета
		 * @param protocol протокол сокета
		 * @return         созданный сокет либо признак ошибки
		 *
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
		 * @brief Метод привязки сокета к адресу
		 *
		 * @param sock сетевой сокет
		 * @param addr адрес для привязки
		 * @param size размер адреса
		 * @return     результат работы функции
		 *
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
		 * @brief Метод перевода сокета в режим ожидания подключений
		 *
		 * @param sock    сетевой сокет
		 * @param backlog размер очереди подключений
		 * @return        результат работы функции
		 *
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
		 * @brief Метод принятия входящего подключения
		 *
		 * @param sock сетевой сокет
		 * @param addr адрес подключившегося клиента
		 * @param size размер адреса
		 * @return     сокет подключения либо признак ошибки
		 *
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
		 * @brief Метод установки подключения
		 *
		 * @param sock сетевой сокет
		 * @param addr адрес подключения
		 * @param size размер адреса
		 * @return     результат работы функции
		 *
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
		 * @brief Метод закрытия сокета
		 *
		 * @param sock сетевой сокет
		 * @return     результат работы функции
		 *
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
		 * @brief Метод прекращения обмена по сокету
		 *
		 * @param sock сетевой сокет
		 * @param how  направление прекращения обмена
		 * @return     результат работы функции
		 *
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
		 * @brief Метод чтения данных из сокета
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер для чтения данных
		 * @param size   размер буфера
		 * @param flags  признаки чтения
		 * @return       количество прочитанных байт либо признак ошибки
		 *
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
		 * @brief Метод отправки данных в сокет
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер данных для отправки
		 * @param size   размер буфера
		 * @param flags  признаки отправки
		 * @return       количество отправленных байт либо признак ошибки
		 *
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
		 * @brief Метод отправки данных в сокет единым сообщением
		 *
		 * @param sock    сетевой сокет
		 * @param message сообщение для отправки данных
		 * @param flags   признаки отправки
		 * @return        количество отправленных байт либо признак ошибки
		 *
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
		 * @brief Метод чтения данных из описателя
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер для чтения данных
		 * @param size   размер буфера
		 * @return       количество прочитанных байт либо признак ошибки
		 *
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
		 * @brief Метод записи данных в описатель
		 *
		 * @param sock   сетевой сокет
		 * @param buffer буфер данных для записи
		 * @param size   размер буфера
		 * @return       количество записанных байт либо признак ошибки
		 *
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
		 * @brief Метод чтения данных из описателя набором буферов
		 *
		 * @param sock   сетевой сокет
		 * @param buffer набор буферов для чтения данных
		 * @param count  количество буферов в наборе
		 * @return       количество прочитанных байт либо признак ошибки
		 *
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
		 * @brief Метод записи данных в описатель набором буферов
		 *
		 * @param sock   сетевой сокет
		 * @param buffer набор буферов данных для записи
		 * @param count  количество буферов в наборе
		 * @return       количество записанных байт либо признак ошибки
		 *
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
		 * @brief Метод получения настройки сокета
		 *
		 * @param sock   сетевой сокет
		 * @param level  уровень настройки
		 * @param name   название настройки
		 * @param value  значение настройки
		 * @param length размер значения настройки
		 * @return       результат работы функции
		 *
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
		 * @brief Метод установки настройки сокета
		 *
		 * @param sock   сетевой сокет
		 * @param level  уровень настройки
		 * @param name   название настройки
		 * @param value  значение настройки
		 * @param length размер значения настройки
		 * @return       результат работы функции
		 *
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
		 * @brief Метод получения собственного адреса сокета
		 *
		 * @param sock   сетевой сокет
		 * @param addr   адрес сокета
		 * @param length размер адреса
		 * @return       результат работы функции
		 *
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
		 * @brief Метод получения адреса другого конца подключения
		 *
		 * @param sock   сетевой сокет
		 * @param addr   адрес другого конца подключения
		 * @param length размер адреса
		 * @return       результат работы функции
		 *
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
		 * @brief Метод управления описателем сокета
		 *
		 * @details Довод здесь передаётся набором переменной длины: у разных запросов
		 *          он разного вида, а у части его нет вовсе
		 *
		 * @param sock    сетевой сокет
		 * @param request запрос управления
		 * @return        результат работы функции
		 *
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
		 * @brief Метод управления устройством через сокет
		 *
		 * @param sock    сетевой сокет
		 * @param request запрос управления
		 * @param data    данные запроса
		 * @return        результат работы функции
		 *
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
		 * @brief Метод ожидания событий на наборе описателей
		 *
		 * @param fds     набор описателей
		 * @param count   количество описателей в наборе
		 * @param timeout предел ожидания в миллисекундах
		 * @return        количество описателей с событиями либо признак ошибки
		 *
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
		 * @brief Метод управления набором ожидания событий
		 *
		 * @param epfd  описатель набора ожидания
		 * @param op    действие над описателем
		 * @param sock  сетевой сокет
		 * @param event событие описателя
		 * @return      результат работы функции
		 *
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
		 * @brief Метод ожидания событий на наборе
		 *
		 * @param epfd    описатель набора ожидания
		 * @param events  набор произошедших событий
		 * @param count   размер набора событий
		 * @param timeout предел ожидания в миллисекундах
		 * @return        количество произошедших событий либо признак ошибки
		 *
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
	};
};

#endif // __linux__

#endif // __AWH_GNU_SOCKET__
