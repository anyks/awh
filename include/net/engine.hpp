/**
 * @file: engine.hpp
 * @date: 2025-11-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл базового класса асинхронного сетевого движка — класс Engine,
 *        задающий общий контракт цикла событий: регистрация дескрипторов, управление подписками,
 *        таймеры и диспетчеризация событий поверх платформенного бэкенда
 *
 * @copyright: Copyright © 2025
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
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
			 * @brief Метод фиксации настроек события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения фиксации
			 *
			 */
			virtual bool commit(const event::id_t id) noexcept = 0;
			/**
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
			 */
			virtual bool rebuild(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса события
			 *
			 * @param id идентификатор события
			 * @return   сетевой интерфейс события
			 *
			 */
			virtual string getIface(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки сетевого интерфейса события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setIface(const event::id_t id, string_view name) noexcept = 0;
		public:
			/**
			 * @brief Метод получения локального порта события
			 *
			 * @param id идентификатор события
			 * @return   локальный порт события
			 *
			 */
			virtual uint16_t getSourcePort(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки локального порта события
			 *
			 * @param id   идентификатор события
			 * @param port локальный порт события
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setSourcePort(const event::id_t id, const uint16_t port) noexcept = 0;
		public:
			/**
			 * @brief Метод получения порта назначения события
			 *
			 * @param id идентификатор события
			 * @return   порт назначения события
			 *
			 */
			virtual uint16_t getTargetPort(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки порта назначения события
			 *
			 * @param id   идентификатор события
			 * @param port порт назначения события
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setTargetPort(const event::id_t id, const uint16_t port) noexcept = 0;
		public:
			/**
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @param id идентификатор события
			 * @return   адрес хоста целевой машины
			 *
			 */
			virtual string getTarget(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param id     идентификатор события
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setTarget(const event::id_t id, string_view target) noexcept = 0;
		public:
			/**
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @param id     идентификатор события
			 * @param target объект для извлечения адреса хоста целевой машины
			 * @return       результат выполнения извлечения адреса хоста целевой машины
			 *
			 */
			virtual bool getTarget(const event::id_t id, unique_ptr <net::addr_t> & target) const noexcept = 0;
			/**
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param id     идентификатор события
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setTarget(const event::id_t id, const net::addr_t * target) noexcept = 0;
		public:
			/**
			 * @brief Метод получения адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @return        значение адреса события
			 *
			 */
			virtual string getAddress(const event::id_t id, const event::address_t address) const noexcept = 0;
			/**
			 * @brief Метод установки адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @param value   значение адреса события
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setAddress(const event::id_t id, const event::address_t address, string_view value) noexcept = 0;
		public:
			/**
			 * @brief Метод получения адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @param value   объект для извлечения адреса события
			 * @return        результат выполнения извлечения адреса события
			 *
			 */
			virtual bool getAddress(const event::id_t id, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept = 0;
			/**
			 * @brief Метод установки адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @param value   значение адреса события
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setAddress(const event::id_t id, const event::address_t address, const net::addr_t * value) noexcept = 0;
		public:
			/**
			 * @brief Метод получения MTU сетевого интерфейса
			 *
			 * @param id идентификатор события
			 * @return   MTU сетевого интерфейса
			 *
			 */
			virtual uint16_t getMaximumTransmissionUnit(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки MTU сетевого интерфейса
			 *
			 * @param id  идентификатор события
			 * @param mtu размер MTU интерфейса
			 * @return    результат установки MTU сетевого интерфейса
			 *
			 */
			virtual bool setMaximumTransmissionUnit(const event::id_t id, const uint32_t mtu) const noexcept = 0;
		public:
			/**
			 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @param id     идентификатор события
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @return       значение DSCP
			 *
			 */
			virtual event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family) const noexcept = 0;
			/**
			 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @param id     идентификатор события
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param dscp   значение DSCP
			 * @return       результат работы функции
			 *
			 */
			virtual bool setDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family, const event::dscp_t dscp) const noexcept = 0;
		public:
			/**
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
			 */
			virtual event::ecn_t getExplicitCongestionNotification(const event::id_t id, const event::family_t family) const noexcept = 0;
			/**
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
			 */
			virtual bool setExplicitCongestionNotification(const event::id_t id, const event::family_t family, const event::ecn_t ecn) const noexcept = 0;
		public:
			/**
			 * @brief Метод получения обнаружения максимального размера пакета (MTU)
			 *
			 * @param id     идентификатор события
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @return       режим обнаружения максимального размера пакета (MTU)
			 *
			 */
			virtual event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family) const noexcept = 0;
			/**
			 * @brief Метод установки обнаружения максимального размера пакета (MTU)
			 *
			 * @param id     идентификатор события
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param mode   режим обнаружения максимального размера пакета (MTU)
			 * @return       результат работы функции
			 *
			 */
			virtual bool setMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family, const event::mtu_discover_t mode) const noexcept = 0;
		public:
			/**
			 * @brief Метод активации/деактивации мультикаст группы события
			 *
			 * @param id     идентификатор события
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @param port   порт мультикаст-группы с которого выполняется подписка
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool membership(const event::id_t id, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept = 0;
			/**
			 * @brief Метод активации/деактивации мультикаст группы события
			 *
			 * @param id     идентификатор события
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @param port   порт мультикаст-группы с которого выполняется подписка
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool membership(const event::id_t id, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept = 0;
		public:
			/**
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
			 */
			virtual bool bind(const event::id_t id, const net::origin_key_t & key) noexcept = 0;
			/**
			 * @brief Метод снятия ключа маршрутизации с сессии
			 *
			 * @param id  идентификатор события сессии
			 * @param key снимаемый ключ сессии
			 * @return    результат снятия
			 *
			 */
			virtual bool unbind(const event::id_t id, const net::origin_key_t & key) noexcept = 0;
		public:
			/**
			 * @brief Метод получения предельного количества одновременных подключений события
			 *
			 * @param id идентификатор события
			 * @return   предельное количество одновременных подключений
			 *
			 */
			virtual uint32_t getMaxConnections(const event::id_t id) const noexcept = 0;
			/**
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
			 */
			virtual bool setMaxConnections(const event::id_t id, const uint32_t max) noexcept = 0;
		public:
			/**
			 * @brief Метод удаления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения удаления
			 *
			 */
			virtual bool destroy(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод получения пары событий для сокета
			 *
			 * @param family   семейство адресов
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         пара идентификаторов созданных событий
			 *
			 */
			virtual std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
		public:
			/**
			 * @brief Метод создания нового события
			 *
			 * @param node     узел события
			 * @param family   семейство адресов
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 *
			 */
			virtual event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
		public:
			/**
			 * @brief Метод получения смещения в файле события
			 *
			 * @param id   идентификатор события
			 * @param seek тип смещения в файле события
			 * @return     смещение в файле события
			 *
			 */
			virtual size_t getSeek(const event::id_t id, const event::seek_t seek) noexcept = 0;
			/**
			 * @brief Метод установки смещения в файле события
			 *
			 * @param id     идентификатор события
			 * @param seek   тип смещения в файле события
			 * @param offset смещение в файле события
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setSeek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept = 0;
		public:
			/**
			 * @brief Метод получения опций события
			 *
			 * @param id идентификатор события
			 * @return   опции события
			 *
			 */
			virtual uint16_t getOptions(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки опций события
			 *
			 * @param id      идентификатор события
			 * @param options опции события для установки
			 * @return        результат выполнения установки
			 *
			 */
			virtual bool setOptions(const event::id_t id, const uint16_t options) noexcept = 0;
			/**
			 * @brief Метод установки опции события
			 *
			 * @param id     идентификатор события
			 * @param option опция события для установки
			 * @param mode   режим установки опции события
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setOption(const event::id_t id, const uint16_t option, const bool mode) noexcept = 0;
		public:
			/**
			 * @brief Метод объединения данных между событиями
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения объединения
			 *
			 */
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept = 0;
		public:
			/**
			 * @brief Метод запуска события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения запуска
			 *
			 */
			virtual bool launch(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод отключения события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения отключения
			 *
			 */
			virtual bool disconnect(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод мультиподключения события к удалённым хостам
			 *
			 * @param ids список идентификаторов событий для подключения
			 * @return    результат выполнения подключения
			 *
			 */
			virtual bool connect(const vector <event::id_t> & ids) noexcept = 0;
		public:
			/**
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param id  идентификатор события
			 * @param max максимальное количество входящих соединений
			 * @return    результат выполнения перевода в режим прослушивания
			 *
			 */
			virtual bool listen(const event::id_t id, const uint32_t max) noexcept = 0;
		public:
			/**
			 * @brief Метод получения данных события
			 *
			 * @param id идентификатор события
			 * @return   результат получения данных
			 *
			 */
			virtual bool recv(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод отправки данных события
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных событием
			 *
			 */
			virtual size_t send(const event::id_t id, const void * buffer, const size_t size) noexcept = 0;
			/**
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
			 */
			virtual size_t relay(const event::id_t id, const void * buffer, const size_t size) noexcept = 0;
		public:
			/**
			 * @brief Метод установки глубины очереди принятия входящих соединений события
			 *
			 * @param id       идентификатор события
			 * @param depth    глубина очереди принятия входящих соединений
			 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
			 *
			 */
			virtual void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept = 0;
		public:
			/**
			 * @brief Метод получения размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       размер буфера события
			 *
			 */
			virtual size_t getBufferSize(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setBufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept = 0;
		public:
			/**
			 * @brief Метод установки пропускной способности события
			 *
			 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
			 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 *
			 */
			virtual void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept = 0;
			/**
			 * @brief Метод установки пропускной способности события для события
			 *
			 * @param id        идентификатор события
			 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
			 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
			 */
			virtual bool bandwidth(const event::id_t id, const event::limiting_t limiting, string_view bandwidth) noexcept = 0;
		public:
			/**
			 * @brief Метод получения режима трансляции пакетов для события
			 *
			 * @param id идентификатор события
			 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
			 *
			 */
			virtual event::delivery_mode_t getDelivery(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки режима трансляции пакетов для события
			 *
			 * @param id       идентификатор события
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 *
			 */
			virtual bool setDelivery(const event::id_t id, const event::delivery_mode_t delivery) noexcept = 0;
		public:
			/**
			 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
			 *
			 * @param id идентификатор события
			 * @return   метаданные последнего принятого дейтаграммного пакета
			 *
			 */
			virtual net::dgram_info_t getTrafficInfo(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод получения количества хопов последнего принятого пакета
			 *
			 * @param id идентификатор события
			 * @return   количество хопов последнего принятого пакета
			 *
			 */
			virtual uint8_t getCountHops(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки количества хопов последнего принятого пакета
			 *
			 * @param id   идентификатор события
			 * @param hops количество хопов последнего принятого пакета
			 * @return     результат выполнения установки
			 *
			 */
			virtual bool setCountHops(const event::id_t id, const uint8_t hops) noexcept = 0;
		public:
			/**
			 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
			 *
			 * @param id идентификатор события
			 * @return   максимальное количество хопов
			 *
			 */
			virtual event::hops_t getHops(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param id   идентификатор события
			 * @param hops максимальное количество хопов
			 * @return     результат работы функции
			 *
			 */
			virtual bool setHops(const event::id_t id, const event::hops_t hops) noexcept = 0;
		public:
			/**
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @param id идентификатор события
			 * @return   режим использования таймаута на чтение события
			 *
			 */
			virtual event::usage_t getUsageReadTimeout(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param id    идентификатор события
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 *
			 */
			virtual void setUsageReadTimeout(const event::id_t id, const event::usage_t usage) noexcept = 0;
		public:
			/**
			 * @brief Метод получения таймаута события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       значение таймаута в миллисекундах
			 *
			 */
			virtual uint32_t getTimeout(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * @brief Метод установки таймаута события
			 *
			 * @param id      идентификатор события
			 * @param action  тип действия события
			 * @param timeout значение таймаута в миллисекундах
			 *
			 */
			virtual void setTimeout(const event::id_t id, const event::action_t action, const uint32_t timeout) noexcept = 0;
		public:
			/**
			 * @brief Метод получения действия события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       режим действия события
			 *
			 */
			virtual event::mode_t getAction(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * @brief Метод установки действия события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @param mode   режим установки действия события
			 * @return       результат выполнения установки
			 *
			 */
			virtual bool setAction(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept = 0;
		public:
			/**
			 * @brief Метод установки параметров keep-alive для события
			 *
			 * @param id    идентификатор события
			 * @param cnt   количество пакетов keep-alive
			 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
			 * @param intvl интервал между пакетами keep-alive в секундах
			 * @return      результат выполнения установки
			 *
			 */
			virtual bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept = 0;
		public:
			/**
			 * @brief Метод приостановки события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения приостановки
			 *
			 */
			virtual bool pause(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод возобновления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения возобновления
			 *
			 */
			virtual bool resume(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод проверки состояния события
			 *
			 * @param id идентификатор события
			 * @return   состояние события
			 *
			 */
			virtual bool isAlive(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод очистки сетевого движка
			 *
			 */
			virtual void clear() noexcept = 0;
		public:
			/**
			 * @brief Метод принудительного пинка базе событий
			 *
			 * @return результат выполнения операции
			 *
			 */
			virtual bool kick() noexcept = 0;
			/**
			 * @brief Метод инициализации сетевого движка
			 *
			 * @return результат выполнения инициализации
			 *
			 */
			virtual bool initialize() noexcept = 0;
			/**
			 * @brief Метод реинициализации сетевого движка
			 *
			 * @return результат выполнения реинициализации
			 *
			 */
			virtual bool reinitialize() noexcept = 0;
			/**
			 * @brief Метод деинициализации сетевого движка
			 *
			 * @return результат выполнения деинициализации
			 *
			 */
			virtual bool deinitialize() noexcept = 0;
		public:
			/**
			 * @brief Метод проверки состояния инициализации сетевого движка
			 *
			 * @return состояние инициализации
			 *
			 */
			virtual bool isInitialized() const noexcept = 0;
		public:
			/**
			 * @brief Метод получения количества событий в сетевом движке
			 *
			 * @return количество событий
			 *
			 */
			virtual size_t eventsCount() const noexcept = 0;
		public:
			/**
			 * @brief Метод получения типа внутренних таймеров
			 *
			 * @return тип таймера для событий сетевого движка
			 *
			 */
			virtual event::timer_t getInternalTimer() const noexcept = 0;
			/**
			 * @brief Метод установки типа внутренних таймеров
			 *
			 * @param timer тип таймера для событий сетевого движка
			 *
			 */
			virtual void setInternalTimer(const event::timer_t timer) noexcept = 0;
		public:
			/**
			 * @brief Метод получения размера отслеживаемого файла
			 *
			 * @param id идентификатор события
			 * @return   размер файла
			 *
			 */
			virtual size_t size(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод получения количества байт, доступных для записи в очередь события
			 *
			 * @param id идентификатор события
			 * @return   количество байт, доступных для записи
			 *
			 */
			virtual size_t available(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод получения типа события
			 *
			 * @param id идентификатор события
			 * @return   тип события
			 *
			 */
			virtual event::type_t type(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения типа узла события
			 *
			 * @param id идентификатор события
			 * @return   тип узла события
			 *
			 */
			virtual event::node_t node(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения семейства события
			 *
			 * @param id идентификатор события
			 * @return   семейство адресов
			 *
			 */
			virtual event::family_t family(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения статуса события
			 *
			 * @param id идентификатор события
			 * @return   статус события
			 *
			 */
			virtual event::status_t status(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения протокола события
			 *
			 * @param id идентификатор события
			 * @return   протокол события
			 *
			 */
			virtual event::protocol_t protocol(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод опроса событий
			 *
			 * @param timeout таймаут опроса в миллисекундах
			 * @return        результат выполнения опроса
			 *
			 */
			virtual bool poll(const int32_t timeout = -1) noexcept = 0;
		public:
			/**
			 * @brief Метод установки функции обратного вызова на чтение события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::read_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на запись события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::write_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на возврат неотправленных данных события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::spool_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на получение общего события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::event_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на ошибку события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::error_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на изменение события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::vnode_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на инъекцию объединённых данных (splice)
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::inject_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на изменение статуса события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::status_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на принятие события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::accept_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова для определения сессии дейтаграммного пакета
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::origin_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на получение информационных метаданных о дейтаграммном пакете
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::traffic_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на подключение события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::connect_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на получение информации о пакетах в туннельном интерфейсе
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::tuninfo_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на таймаут события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::timeout_t cb) noexcept = 0;
			/**
			 * @brief Метод установки функции обратного вызова на доступность очереди события
			 *
			 * @param id идентификатор события
			 * @param cb функция обратного вызова
			 *
			 */
			virtual void on(const event::id_t id, engine::callback::available_t cb) noexcept = 0;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 *
			 */
			explicit Engine(const fmk_t * fmk, const log_t * log) noexcept :
			 _eth(fmk, log), _addr(fmk, log), _fmk(fmk), _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Engine() noexcept {}
	} engine_t;
};

#endif // __AWH_ENGINE__
