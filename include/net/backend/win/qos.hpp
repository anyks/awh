/**
 * @file: qos.hpp
 * @date: 2026-08-08
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля отметки исходящих пакетов классом обслуживания
 *        для MS Windows
 *
 * @details У систем POSIX класс обслуживания ставится настройкой сокета: IP_TOS у
 *          IPv4, IPV6_TCLASS у IPv6, - записал октет, и он ушёл в заголовок пакета.
 *          MS Windows настройки эти держит, но не исполняет: с версии Windows XP SP2
 *          запись отвечает согласием, а в заголовок не попадает ничего. Сделано это
 *          намеренно - отметка чужим приложением чужого трафика позволяла бы всякому
 *          объявить себя важнее прочих
 *
 *          Отмечать пакеты система даёт подсистемой качества обслуживания qWAVE.
 *          Устроена она иначе: отметка ставится не сокету, а **потоку** - связке
 *          сокета с назначением, заводимой отдельно. Поток этот держит собственный
 *          описатель, и хранить его приходится модулю
 *
 * @par Намеренные решения
 *
 *      **Реестр намерений, а не одних описателей.** Поток qWAVE требует знать, куда
 *      пойдёт трафик: `QOSAddSocketToFlow` берёт либо уже соединённый сокет, либо
 *      назначение доводом. Отметка же выставляется когда угодно, в том числе до
 *      соединения, - и тогда запомнить её больше некуда. Запомненная отметка
 *      применяется позже, когда назначение станет известно
 *
 *      **Отдаётся запрошенное, а не показание системы.** Обратного чтения отметки
 *      qWAVE не даёт вовсе, и выдаётся то, что запрашивали. Оговорка эта существенна:
 *      значение это есть намерение, а не свидетельство того, что ушло в сеть
 *
 *      **Библиотека подключается по ходу работы.** Машина вправе обойтись без
 *      qwave.dll - на урезанных изданиях сервера её нет, - и отсутствие её обязано
 *      отвечать внятным отказом, а не срывом запуска всего приложения
 *
 * @warning Поток обязан быть снят прежде закрытия сокета. Закрытие в обход снятия
 *          оставляет поток висеть в подсистеме до перезапуска службы - потому
 *          закрытие сокета и обязано идти одной обёрткой, где снятие это стоит
 *
 * \~english
 * @brief Header file of the module of marking the outgoing packets with a class of the service
 *        for MS Windows
 * @details At the POSIX systems the class of the service is set by a setting of a socket: IP_TOS at
 *          IPv4, IPV6_TCLASS at IPv6, — one wrote an octet, and it went into the header of the packet.
 *          MS Windows holds these settings, but does not execute them: since the version Windows XP SP2
 *          the writing answers with an agreement, and nothing gets into the header. This is done
 *          deliberately — the marking of a foreign traffic by a foreign application would allow anyone
 *          to declare himself more important than the others
 *          The system gives the marking of the packets by the qWAVE quality of service subsystem.
 *          It is arranged otherwise: the mark is set not on a socket, but on a **flow** — a bundle of
 *          a socket with a destination, started separately. That flow holds its own
 *          handle, and the module has to keep it
 * @par Deliberate decisions
 *      **A registry of the intentions, and not of the handles alone.** A qWAVE flow requires to know where
 *      the traffic will go: `QOSAddSocketToFlow` takes either an already connected socket, or
 *      the destination as an argument. The mark, though, is set out whenever, including before
 *      the connection, — and then there is nowhere else to remember it. A remembered mark
 *      is applied later, when the destination becomes known
 *      **What is given back is what was requested, and not the reading of the system.** qWAVE gives no reverse reading of the mark
 *      at all, and what is yielded is what was requested. This reservation is essential:
 *      this value is an intention, and not a testimony of what went into the network
 *      **The library is linked in the course of the work.** A machine is free to get by without
 *      qwave.dll — on the cut-down editions of the server it is absent, — and its absence is obliged
 *      to answer with an intelligible refusal, and not with a breakdown of the startup of the whole application
 * @warning A flow is obliged to be removed before the closing of the socket. A closing bypassing the removal
 *          leaves the flow hanging in the subsystem until the restart of the service — that is why
 *          the closing of the socket is obliged to go through a single wrapper, where that removal stands
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_NET_BACKEND_WIN_QOS__
#define __AWH_NET_BACKEND_WIN_QOS__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/net.hpp>
#include <net/event.hpp>
#include <sys/log.hpp>

/**
 * \~russian
 * @brief пространство имён библиотеки
 *
 * \~english
 * @brief namespace of the library
 *
 * \~
 */
namespace awh {
	/**
	 * Активируем пространство имён следующего уровня
	 *
	 */
	using namespace std;
	/**
	 * \~russian
	 * @brief пространство имён средств MS Windows
	 *
	 * \~english
	 * @brief namespace of the means of MS Windows
	 *
	 * \~
	 */
	namespace win {
		/**
		 * \~russian
		 * @brief пространство имён качества обслуживания
		 *
		 * \~english
		 * @brief namespace of the quality of the service
		 *
		 * \~
		 */
		namespace qos {
			/**
			 * \~russian
			 * @brief Функция установки класса обслуживания сокету
			 *
			 * @details Если назначение сокету уже известно - он соединён, - отметка
			 *          ставится сразу. Иначе она запоминается и будет применена
			 *          применением, когда назначение станет известно
			 *
			 * @param sock отмечаемый сокет
			 * @param dscp устанавливаемый класс обслуживания
			 * @param log  объект ведения журнала
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Function of setting the class of the service on a socket
			 * @details If the destination of the socket is already known — it is connected, — the mark
			 *          is set at once. Otherwise it is remembered and will be applied by the application,
			 *          when the destination becomes known
			 * @param sock marked socket
			 * @param dscp the set class of the service
			 * @param log  object of the keeping of the log
			 * @return     result of the performance of the setting
			 *
			 * \~
			 */
			bool mark(const net::socket_t sock, const event::dscp_t dscp, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения запрошенного класса обслуживания
			 *
			 * @param sock опрашиваемый сокет
			 * @return     запрошенный класс обслуживания
			 *
			 * \~english
			 * @brief Function of getting the requested class of the service
			 * @param sock polled socket
			 * @return     the requested class of the service
			 *
			 * \~
			 */
			event::dscp_t mark(const net::socket_t sock) noexcept;
			/**
			 * \~russian
			 * @brief Функция применения запомненной отметки к соединённому сокету
			 *
			 * @details Звать её надлежит движку, едва сокет соединён: до того
			 *          назначение неизвестно, и поток завести нельзя
			 *
			 * @param sock применяемый сокет
			 * @param log  объект ведения журнала
			 * @return     результат выполнения применения
			 *
			 * \~english
			 * @brief Function of applying a remembered mark to a connected socket
			 * @details It is to be called by the engine as soon as the socket is connected: before that
			 *          the destination is unknown, and a flow cannot be started
			 * @param sock applied socket
			 * @param log  object of the keeping of the log
			 * @return     result of the performance of the application
			 *
			 * \~
			 */
			bool apply(const net::socket_t sock, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Функция снятия потока с сокета
			 *
			 * @details Звать её надлежит прежде закрытия сокета - иначе поток
			 *          останется висеть в подсистеме
			 *
			 * @param sock освобождаемый сокет
			 * @return     результат выполнения снятия
			 *
			 * \~english
			 * @brief Function of removing a flow from a socket
			 * @details It is to be called before the closing of the socket — otherwise the flow
			 *          will remain hanging in the subsystem
			 * @param sock released socket
			 * @return     result of the performance of the removal
			 *
			 * \~
			 */
			bool release(const net::socket_t sock) noexcept;
		}
	}
}

#endif // __AWH_NET_BACKEND_WIN_QOS__
