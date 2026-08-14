/**
 * @file engine.hpp
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
 * @brief Заголовочный файл базового класса асинхронного сетевого движка — класс Engine,
 *        задающий общий контракт цикла событий: регистрация дескрипторов, управление подписками,
 *        таймеры и диспетчеризация событий поверх платформенного бэкенда
 *
 * \~english
 * @brief Header file of the base class of the asynchronous network engine — the Engine class,
 *        setting the common contract of the loop of the events: the registration of the descriptors, the management of the subscriptions,
 *        the timers and the dispatching of the events over a platform backend
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ENGINE__
#define __AWH_ENGINE__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "addr.hpp"
#include "callback.hpp"
#include "eth/eth.hpp"

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
	 * @brief Базовый класс асинхронного сетевого движка
	 *
	 * @details Чистый интерфейс: все методы виртуальные и без реализации, состояний
	 *          класс не держит - только объекты работы с сетью, адресами, фреймворком
	 *          и логом, доступные наследнику. Смысл его в том, чтобы отделить
	 *          **договор** сетевого движка от способа, которым он выполнен на
	 *          конкретной системе.
	 *
	 *          Договор описывает полный набор того, что движок обязан уметь: заводить
	 *          и уничтожать события, закреплять их настройки и запускать, подключаться
	 *          и слушать, читать и отправлять, приостанавливать и возобновлять,
	 *          держать сроки, отдавать и принимать параметры сокета вплоть до полей
	 *          заголовка IP-пакета, и наконец опрашивать всё это одним циклом.
	 *
	 *          Реализация выбирается сборкой по системе, и приложение с этим классом
	 *          напрямую не работает: пользоваться следует классом `engine::io_t`, где
	 *          договор выполнен. Разделение нужно затем, чтобы механизм ожидания -
	 *          kqueue, epoll или иной - оставался подробностью реализации, а
	 *          прикладной код от него не зависел
	 *
	 * @note Механизмы ожидания различаются не только вызовами, но и самой моделью:
	 *       одни сообщают о **готовности** дескриптора, другие о **завершении**
	 *       операции. Поэтому договор здесь описан в терминах событий и их
	 *       идентификаторов, а не в терминах дескрипторов и подписок на них - иначе
	 *       он привязался бы к одной из моделей
	 *
	 * @note Наследник обязан выполнить **все** методы: заготовок интерфейс не даёт, и
	 *       незамеченный пробел обнаружится сборкой, а не поведением во время работы
	 *
	 * @see engine::io_t - выполнение этого договора, с которым и следует работать
	 *
	 * \~english
	 * @brief Base class of the asynchronous network engine
	 * @details A pure interface: all the methods are virtual and without an implementation, the class holds
	 *          no states — only the objects of the work with the network, with the addresses, with the framework
	 *          and with the log, available to a descendant. Its point is to separate
	 *          the **contract** of a network engine from the way it is performed on
	 *          a concrete system.
	 *          The contract describes the full set of what an engine is obliged to be able to do: to start
	 *          and to destroy the events, to fix their settings and to launch them, to connect
	 *          and to listen, to read and to send, to pause and to resume,
	 *          to hold the terms, to give back and to take the parameters of a socket down to the fields of
	 *          the header of an IP packet, and finally to poll all this by one loop.
	 *          The implementation is chosen by the build by the system, and the application does not work with this class
	 *          directly: one should use the `engine::io_t` class, where
	 *          the contract is performed. The division is needed so that the mechanism of the waiting —
	 *          kqueue, epoll or another one — would remain a detail of the implementation, and
	 *          the application code would not depend on it
	 * @note The mechanisms of the waiting differ not only by the calls, but by the very model:
	 *       some report the **readiness** of a descriptor, the others the **completion**
	 *       of an operation. Therefore the contract here is described in the terms of the events and of their
	 *       identifiers, and not in the terms of the descriptors and of the subscriptions to them — otherwise
	 *       it would be tied to one of the models
	 * @note A descendant is obliged to perform **all** the methods: the interface gives no stubs, and
	 *       an unnoticed gap will be discovered by the build, and not by the behaviour during the work
	 * @see engine::io_t — the performance of this contract, which is the one to work with
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Engine {
		protected:
			// Объект работы с сетью
			mutable eth_t _eth;
			// Объект работы с сетевыми адресами
			mutable net_addr_t _addr;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * \~russian
			 * @brief Метод фиксации настроек события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения фиксации
			 *
			 * \~english
			 * @brief Method of the fixation of the settings of an event
			 * @param id identifier of the event
			 * @return   result of the performance of the fixation
			 *
			 * \~
			 */
			virtual bool commit(const event::id_t id) noexcept = 0;
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
			virtual bool rebuild(const event::id_t id) noexcept = 0;
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
			virtual string getIface(const event::id_t id) const noexcept = 0;
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
			virtual bool setIface(const event::id_t id, string_view name) noexcept = 0;
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
			virtual uint16_t getSourcePort(const event::id_t id) const noexcept = 0;
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
			virtual bool setSourcePort(const event::id_t id, const uint16_t port) noexcept = 0;
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
			virtual uint16_t getTargetPort(const event::id_t id) const noexcept = 0;
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
			virtual bool setTargetPort(const event::id_t id, const uint16_t port) noexcept = 0;
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
			virtual string getTarget(const event::id_t id) const noexcept = 0;
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
			virtual bool setTarget(const event::id_t id, string_view target) noexcept = 0;
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
			virtual bool getTarget(const event::id_t id, unique_ptr <net::addr_t> & target) const noexcept = 0;
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
			virtual bool setTarget(const event::id_t id, const net::addr_t * target) noexcept = 0;
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
			virtual string getAddress(const event::id_t id, const event::address_t address) const noexcept = 0;
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
			virtual bool setAddress(const event::id_t id, const event::address_t address, string_view value) noexcept = 0;
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
			virtual bool getAddress(const event::id_t id, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept = 0;
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
			virtual bool setAddress(const event::id_t id, const event::address_t address, const net::addr_t * value) noexcept = 0;
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
			virtual uint16_t getMaximumTransmissionUnit(const event::id_t id) const noexcept = 0;
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
			virtual bool setMaximumTransmissionUnit(const event::id_t id, const uint32_t mtu) const noexcept = 0;
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
			virtual event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family) const noexcept = 0;
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
			virtual bool setDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family, const event::dscp_t dscp) const noexcept = 0;
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
			virtual event::ecn_t getExplicitCongestionNotification(const event::id_t id, const event::family_t family) const noexcept = 0;
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
			virtual bool setExplicitCongestionNotification(const event::id_t id, const event::family_t family, const event::ecn_t ecn) const noexcept = 0;
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
			virtual event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family) const noexcept = 0;
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
			virtual bool setMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family, const event::mtu_discover_t mode) const noexcept = 0;
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
			 *          одна лишь подписка на группу.
			 *
			 *          Отказ метода состояния события не меняет
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
			 *          the subscription to the group alone is performed.
			 *          A refusal of the method does not change the state of the event
			 * @param id     identifier of the event
			 * @param mode   mode of the activation/deactivation
			 * @param group  multicast group for the activation/deactivation
			 * @param source address of the network interface the subscription is performed from
			 * @param port   port of the multicast group the subscription is performed from
			 * @return       result of the performance of the setting
			 *
			 * \~
			 */
			virtual bool membership(const event::id_t id, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept = 0;
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
			 *          одна лишь подписка на группу.
			 *
			 *          Отказ метода состояния события не меняет
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
			 *          the subscription to the group alone is performed.
			 *          A refusal of the method does not change the state of the event
			 * @param id     identifier of the event
			 * @param mode   mode of the activation/deactivation
			 * @param group  multicast group for the activation/deactivation
			 * @param source address of the network interface the subscription is performed from
			 * @param port   port of the multicast group the subscription is performed from
			 * @return       result of the performance of the setting
			 *
			 * \~
			 */
			virtual bool membership(const event::id_t id, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept = 0;
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
			virtual bool bind(const event::id_t id, const net::origin_key_t & key) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод снятия ключа маршрутизации с сессии
			 *
			 * @param id  идентификатор события сессии
			 * @param key снимаемый ключ сессии
			 * @return    результат снятия
			 *
			 * \~english
			 * @brief Method of removing a key of the routing from a session
			 * @param id  identifier of the event of the session
			 * @param key removed key of the session
			 * @return    result of the removal
			 *
			 * \~
			 */
			virtual bool unbind(const event::id_t id, const net::origin_key_t & key) noexcept = 0;
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
			virtual uint32_t getMaxConnections(const event::id_t id) const noexcept = 0;
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
			virtual bool setMaxConnections(const event::id_t id, const uint32_t max) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения удаления
			 *
			 * \~english
			 * @brief Method of removing an event
			 * @param id identifier of the event
			 * @return   result of the performance of the removal
			 *
			 * \~
			 */
			virtual bool destroy(const event::id_t id) noexcept = 0;
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
			virtual std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод создания нового события
			 *
			 * @param node     узел события
			 * @param family   семейство адресов
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 *
			 * \~english
			 * @brief Method of creating a new event
			 * @param node     node of the event
			 * @param family   family of the addresses
			 * @param type     type of the socket
			 * @param protocol protocol of the socket
			 * @return         identifier of the created event
			 *
			 * \~
			 */
			virtual event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
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
			virtual size_t getSeek(const event::id_t id, const event::seek_t seek) noexcept = 0;
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
			virtual bool setSeek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept = 0;
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
			virtual uint16_t getOptions(const event::id_t id) const noexcept = 0;
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
			virtual bool setOptions(const event::id_t id, const uint16_t options) noexcept = 0;
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
			virtual bool setOption(const event::id_t id, const uint16_t option, const bool mode) noexcept = 0;
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
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод запуска события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения запуска
			 *
			 * \~english
			 * @brief Method of the launch of an event
			 * @param id identifier of the event
			 * @return   result of the performance of the launch
			 *
			 * \~
			 */
			virtual bool launch(const event::id_t id) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод отключения события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения отключения
			 *
			 * \~english
			 * @brief Method of the disconnection of an event
			 * @param id identifier of the event
			 * @return   result of the performance of the disconnection
			 *
			 * \~
			 */
			virtual bool disconnect(const event::id_t id) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод мультиподключения события к удалённым хостам
			 *
			 * @param ids список идентификаторов событий для подключения
			 * @return    результат выполнения подключения
			 *
			 * \~english
			 * @brief Method of the multi-connection of an event to the remote hosts
			 * @param ids list of the identifiers of the events to connect
			 * @return    result of the performance of the connection
			 *
			 * \~
			 */
			virtual bool connect(const vector <event::id_t> & ids) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param id  идентификатор события
			 * @param max максимальное количество входящих соединений
			 * @return    результат выполнения перевода в режим прослушивания
			 *
			 * \~english
			 * @brief Method of putting an event into the mode of the listening for the incoming connections
			 * @param id  identifier of the event
			 * @param max maximum number of the incoming connections
			 * @return    result of the performance of the putting into the mode of the listening
			 *
			 * \~
			 */
			virtual bool listen(const event::id_t id, const uint32_t max) noexcept = 0;
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
			virtual bool recv(const event::id_t id) noexcept = 0;
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
			virtual size_t send(const event::id_t id, const void * buffer, const size_t size) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод перенаправления объединённых данных в событие-приёмник (splice)
			 *
			 * @note Если на событии-приёмнике установлена функция инъекции (транспорт
			 *       шифрует данные на уровне соединения, напр. QUIC), данные передаются
			 *       ей для отправки собственным потоком; иначе выполняется обычная
			 *       отправка байт в сокет
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
			 * @param id     identifier of the receiver event
			 * @param buffer buffer of the redirected data
			 * @param size   size of the redirected data
			 * @return       number of the bytes accepted for the redirection
			 *
			 * \~
			 */
			virtual size_t relay(const event::id_t id, const void * buffer, const size_t size) noexcept = 0;
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
			virtual void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept = 0;
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
			virtual size_t getBufferSize(const event::id_t id, const event::action_t action) const noexcept = 0;
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
			virtual bool setBufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept = 0;
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
			virtual void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept = 0;
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
			virtual bool bandwidth(const event::id_t id, const event::limiting_t limiting, string_view bandwidth) noexcept = 0;
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
			virtual event::delivery_mode_t getDelivery(const event::id_t id) const noexcept = 0;
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
			virtual bool setDelivery(const event::id_t id, const event::delivery_mode_t delivery) noexcept = 0;
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
			virtual net::dgram_info_t getTrafficInfo(const event::id_t id) const noexcept = 0;
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
			virtual uint8_t getCountHops(const event::id_t id) const noexcept = 0;
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
			virtual bool setCountHops(const event::id_t id, const uint8_t hops) noexcept = 0;
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
			virtual event::hops_t getHops(const event::id_t id) const noexcept = 0;
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
			virtual bool setHops(const event::id_t id, const event::hops_t hops) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @param id идентификатор события
			 * @return   режим использования таймаута на чтение события
			 *
			 * \~english
			 * @brief Method of getting the mode of the use of the timeout on the reading of an event
			 * @param id identifier of the event
			 * @return   mode of the use of the timeout on the reading of the event
			 *
			 * \~
			 */
			virtual event::usage_t getUsageReadTimeout(const event::id_t id) const noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param id    идентификатор события
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 *
			 * \~english
			 * @brief Method of setting the mode of the use of the timeout on the reading of an event
			 * @param id    identifier of the event
			 * @param usage mode of the use of the timeout on the reading of the event (reusable or disposable)
			 *
			 * \~
			 */
			virtual void setUsageReadTimeout(const event::id_t id, const event::usage_t usage) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод получения таймаута события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Method of getting the timeout of an event
			 * @param id     identifier of the event
			 * @param action type of the action of the event
			 * @return       value of the timeout in milliseconds
			 *
			 * \~
			 */
			virtual uint32_t getTimeout(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки таймаута события
			 *
			 * @param id      идентификатор события
			 * @param action  тип действия события
			 * @param timeout значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Method of setting the timeout of an event
			 * @param id      identifier of the event
			 * @param action  type of the action of the event
			 * @param timeout value of the timeout in milliseconds
			 *
			 * \~
			 */
			virtual void setTimeout(const event::id_t id, const event::action_t action, const uint32_t timeout) noexcept = 0;
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
			virtual event::mode_t getAction(const event::id_t id, const event::action_t action) const noexcept = 0;
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
			virtual bool setAction(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept = 0;
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
			virtual bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод приостановки события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения приостановки
			 *
			 * \~english
			 * @brief Method of the pausing of an event
			 * @param id identifier of the event
			 * @return   result of the performance of the pausing
			 *
			 * \~
			 */
			virtual bool pause(const event::id_t id) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод возобновления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения возобновления
			 *
			 * \~english
			 * @brief Method of the resumption of an event
			 * @param id identifier of the event
			 * @return   result of the performance of the resumption
			 *
			 * \~
			 */
			virtual bool resume(const event::id_t id) noexcept = 0;
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
			virtual bool isAlive(const event::id_t id) const noexcept = 0;
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
			virtual void clear() noexcept = 0;
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
			virtual bool kick() noexcept = 0;
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
			virtual bool initialize() noexcept = 0;
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
			virtual bool reinitialize() noexcept = 0;
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
			virtual bool deinitialize() noexcept = 0;
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
			virtual bool isInitialized() const noexcept = 0;
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
			virtual size_t eventsCount() const noexcept = 0;
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
			virtual event::timer_t getInternalTimer() const noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки типа внутренних таймеров
			 *
			 * @param timer тип таймера для событий сетевого движка
			 *
			 * \~english
			 * @brief Method of setting the type of the internal timers
			 * @param timer type of the timer for the events of the network engine
			 *
			 * \~
			 */
			virtual void setInternalTimer(const event::timer_t timer) noexcept = 0;
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
			virtual size_t size(const event::id_t id) const noexcept = 0;
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
			virtual size_t available(const event::id_t id) const noexcept = 0;
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
			virtual event::type_t type(const event::id_t id) const noexcept = 0;
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
			virtual event::node_t node(const event::id_t id) const noexcept = 0;
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
			virtual event::family_t family(const event::id_t id) const noexcept = 0;
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
			virtual event::status_t status(const event::id_t id) const noexcept = 0;
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
			virtual event::protocol_t protocol(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод опроса событий
			 *
			 * @param timeout таймаут опроса в миллисекундах
			 * @return        результат выполнения опроса
			 *
			 * \~english
			 * @brief Method of the polling of the events
			 * @param timeout timeout of the polling in milliseconds
			 * @return        result of the performance of the polling
			 *
			 * \~
			 */
			virtual bool poll(const int32_t timeout = -1) noexcept = 0;
		public:
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на чтение события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the reading of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::read_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на запись события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the writing of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::write_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на возврат неотправленных данных события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the return of the unsent data of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::spool_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на получение общего события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the receipt of a common event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::event_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на ошибку события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on an error of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::error_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на изменение события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the change of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::vnode_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на инъекцию объединённых данных (splice)
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the injection of the joined data (splice)
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::inject_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на изменение статуса события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the change of the status of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::status_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на принятие события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the acceptance of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::accept_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова для определения сессии дейтаграммного пакета
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function for the determination of the session of a datagram packet
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::origin_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на получение информационных метаданных о дейтаграммном пакете
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the receipt of the informational metadata about a datagram packet
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::traffic_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на подключение события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the connection of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::connect_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на получение информации о пакетах в туннельном интерфейсе
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the receipt of the information about the packets in a tunnel interface
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::tuninfo_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на таймаут события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the timeout of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::timeout_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова на доступность очереди события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function on the availability of the queue of an event
			 * @param id identifier of the event
			 * @param cb callback function
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::available_t cb) noexcept = 0;
			/**
			 * \~russian
			 * @brief Метод установки источника данных для вытягивающей модели отправки
			 *
			 * @details Разновидность отправки, обратная методу send(): движок сам просит данные у
			 *          источника ровно тогда, когда сокет готов к записи и в очереди есть место.
			 *          Приложению не нужно держать в памяти всё тело - оно выдаёт данные по мере
			 *          их ухода в сеть. Источник снимается сам по достижении конца тела
			 *
			 * @note Источник и метод send() применимы к одному событию одновременно: данные обоих
			 *       ложатся в одну очередь и уходят в порядке попадания в неё
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова источника данных
			 *
			 * \~english
			 * @brief Method of setting the source of the data for the pull model of the sending
			 * @param id identifier of the event
			 * @param cb callback function of the source of the data
			 *
			 * \~
			 */
			virtual void on(const event::id_t id, engine::callback::source_t cb) noexcept = 0;
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
			explicit Engine(const fmk_t * fmk, const log_t * log) noexcept :
			 _eth(fmk, log), _addr(fmk, log), _fmk(fmk), _log(log) {}
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
			virtual ~Engine() noexcept {}
	} engine_t;
};

#endif // __AWH_ENGINE__
