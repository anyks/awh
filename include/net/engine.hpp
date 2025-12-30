/**
 * @file: engine.hpp
 * @date: 2025-11-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_ENGINE__
#define __AWH_ENGINE__

/**
 * Стандартные модули
 */
#include <array>
#include <string>
#include <cstdint>
#include <unordered_set>

/**
 * Наши модули
 */
#include "eth.hpp"
#include "event.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Тип сетевого асинхронного движка
	 *
	 */
	typedef class Engine {
		protected:
			// Объект работы с сетью
			mutable eth_t _eth;
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
			 */
			virtual bool commit(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод получения порта события
			 *
			 * @param id идентификатор события
			 * @return   порт события
			 */
			virtual uint16_t port(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки порта события
			 *
			 * @param id   идентификатор события
			 * @param port порт события
			 * @return     результат выполнения установки
			 */
			virtual bool port(const event::id_t id, const uint16_t port) noexcept = 0;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса события
			 *
			 * @param id идентификатор события
			 * @return   сетевой интерфейс события
			 */
			virtual string iface(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки сетевого интерфейса события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 */
			virtual bool iface(const event::id_t id, const string & name) noexcept = 0;
		public:
			/**
			 * @brief Метод получения хоста целевой машины
			 *
			 * @param id идентификатор события
			 * @return   хост целевой машины
			 */
			virtual string target(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки хоста целевой машины
			 *
			 * @param id   идентификатор события
			 * @param host хост целевой машины
			 * @return     результат выполнения установки
			 */
			virtual bool target(const event::id_t id, const string & target) noexcept = 0;
		public:
			/**
			 * @brief Метод получения адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @return        значение адреса события
			 */
			virtual string address(const event::id_t id, const event::address_t address) const noexcept = 0;
			/**
			 * @brief Метод установки адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @param value   значение адреса события
			 * @return        результат выполнения установки
			 */
			virtual bool address(const event::id_t id, const event::address_t address, const string & value) noexcept = 0;
		public:
			/**
			 * @brief Метод удаления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения удаления
			 */
			virtual bool destroy(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод создания нового события
			 *
			 * @param node     узел события
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 */
			virtual event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
		public:
			/**
			 * @brief Метод получения пары событий для сокета
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         пара идентификаторов созданных событий
			 */
			virtual std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
		public:
			/**
			 * @brief Метод получения смещения в файле события
			 *
			 * @param id   идентификатор события
			 * @param seek тип смещения в файле события
			 * @return     смещение в файле события
			 */
			virtual size_t seek(const event::id_t id, const event::seek_t seek) noexcept = 0;
			/**
			 * @brief Метод установки смещения в файле события
			 *
			 * @param id     идентификатор события
			 * @param seek   тип смещения в файле события
			 * @param offset смещение в файле события
			 * @return       результат выполнения установки
			 */
			virtual bool seek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept = 0;
		public:
			/**
			 * @brief Метод получения опций события
			 *
			 * @param id идентификатор события
			 * @return   опции события
			 */
			virtual uint16_t options(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки опций события
			 *
			 * @param id      идентификатор события
			 * @param options опции события для установки
			 * @return        результат выполнения установки
			 */
			virtual bool options(const event::id_t id, const uint16_t options) noexcept = 0;
			/**
			 * @brief Метод установки опции события
			 *
			 * @param id     идентификатор события
			 * @param option опция события для установки
			 * @param mode   режим установки опции события
			 * @return       результат выполнения установки
			 */
			virtual bool option(const event::id_t id, const uint16_t option, const bool mode) noexcept = 0;
		public:
			/**
			 * @brief Метод перемещения данных между событиями
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения перемещения
			 */
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept = 0;
		public:
			/**
			 * @brief Метод получения информационных метаданных SCTP сообщения
			 *
			 * @param id идентификатор события
			 * @return   информационные метаданные SCTP сообщения
			 */
			virtual net::sctp_minfo_t sctpMessageInfo(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки информационных метаданных SCTP сообщения
			 *
			 * @param id   идентификатор события
			 * @param info информационные метаданные SCTP сообщения
			 */
			virtual void sctpMessageInfo(const event::id_t id, const net::sctp_minfo_t & info) noexcept = 0;
		public:
			/**
			 * @brief Метод запуска события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения запуска
			 */
			virtual bool launch(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод отключения события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения отключения
			 */
			virtual bool disconnect(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод подключения события к удалённому хосту
			 *
			 * @param id    идентификатор события
			 * @param async флаг асинхронного подключения
			 * @return      результат выполнения подключения
			 */
			virtual bool connect(const event::id_t id, const bool async = false) noexcept = 0;
		public:
			/**
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param id    идентификатор события
			 * @param max   максимальное количество входящих соединений
			 * @param async флаг асинхронного прослушивания
			 * @return      результат выполнения перевода в режим прослушивания
			 */
			virtual bool listen(const event::id_t id, const uint16_t max, const bool async = false) noexcept = 0;
		public:
			/**
			 * @brief Метод приёма данных события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения приёма
			 */
			virtual bool recv(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод отправки данных события
			 *
			 * @param id   идентификатор события
			 * @param data буфер данных для отправки
			 * @param size размер данных для отправки
			 * @return     результат выполнения отправки
			 */
			virtual bool send(const event::id_t id, const char * data, const size_t size) noexcept = 0;
		public:
			/**
			 * @brief Метод очистки чёрного списка события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			virtual bool clearBlacklist(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод добавления адреса в чёрный список события
			 *
			 * @param id    идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 */
			virtual bool addToBlacklist(const event::id_t id, const string & value) noexcept = 0;
			/**
			 * @brief Метод удаления адреса из чёрного списка события
			 *
			 * @param id    идентификатор события
			 * @param value адрес для удаления из чёрного списка
			 * @return      результат выполнения удаления
			 */
			virtual bool removeFromBlacklist(const event::id_t id, const string & value) noexcept = 0;
			/**
			 * @brief Метод получения чёрного списка события
			 *
			 * @param id идентификатор события
			 * @return   чёрный список события
			 */
			virtual const std::unordered_map <string, event::address_t> & blacklist(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод очистки белого списка события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			virtual bool clearWhitelist(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод добавления адреса в белый список события
			 *
			 * @param id    идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 */
			virtual bool addToWhitelist(const event::id_t id, const string & value) noexcept = 0;
			/**
			 * @brief Метод удаления адреса из белого списка события
			 *
			 * @param id    идентификатор события
			 * @param value адрес для удаления из белого списка
			 * @return      результат выполнения удаления
			 */
			virtual bool removeFromWhitelist(const event::id_t id, const string & value) noexcept = 0;
			/**
			 * @brief Метод получения белого списка события
			 *
			 * @param id идентификатор события
			 * @return   белый список события
			 */
			virtual const std::unordered_map <string, event::address_t> & whitelist(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод активации/деактивации мультикаст группы cобытия
			 *
			 * @param id     идентификатор события
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @return       результат выполнения установки
			 */
			virtual bool membership(const event::id_t id, const event::mode_t mode, const string & group, const string & source) noexcept = 0;
		public:
			/**
			 * @brief Метод установки глубины очереди принятия входящих соединений события
			 *
			 * @param id       идентификатор события
			 * @param depth    глубина очереди принятия входящих соединений
			 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
			 */
			virtual void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept = 0;
		public:
			/**
			 * @brief Метод получения размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       размер буфера события
			 */
			virtual size_t bufferSize(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 */
			virtual bool bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept = 0;
		public:
			/**
			 * @brief Метод получения режима трансляции пакетов для события
			 *
			 * @param id идентификатор события
			 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
			 */
			virtual event::delivery_mode_t delivery(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки режима трансляции пакетов для события
			 *
			 * @param id       идентификатор события
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 */
			virtual bool delivery(const event::id_t id, const event::delivery_mode_t delivery) noexcept = 0;
		public:
			/**
			 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
			 *
			 * @param id идентификатор события
			 * @return   максимальное количество хопов
			 */
			virtual event::hops_t hops(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param id     идентификатор события
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param hops   максимальное количество хопов
			 * @return       результат работы функции
			 */
			virtual bool hops(const event::id_t id, const event::family_t family, const event::hops_t hops) noexcept = 0;
		public:
			/**
			 * @brief Метод получения таймаута события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       значение таймаута в миллисекундах
			 */
			virtual uint16_t timeout(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * @brief Метод установки таймаута события
			 *
			 * @param id      идентификатор события
			 * @param action  тип действия события
			 * @param timeout значение таймаута в миллисекундах
			 */
			virtual void timeout(const event::id_t id, const event::action_t action, const uint16_t timeout) noexcept = 0;
		public:
			/**
			 * @brief Метод получения действия события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       режим действия события
			 */
			virtual event::mode_t action(const event::id_t id, const event::action_t action) const noexcept = 0;
			/**
			 * @brief Метод установки действия события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @param mode   режим установки действия события
			 * @return       результат выполнения установки
			 */
			virtual bool action(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept = 0;
		public:
			/**
			 * @brief Метод установки параметров keep-alive для события
			 *
			 * @param id    идентификатор события
			 * @param cnt   количество пакетов keep-alive
			 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
			 * @param intvl интервал между пакетами keep-alive в секундах
			 * @return      результат выполнения установки
			 */
			virtual bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept = 0;
		public:
			/**
			 * @brief Метод приостановки события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения приостановки
			 */
			virtual bool pause(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод возобновления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения возобновления
			 */
			virtual bool resume(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод проверки состояния события
			 *
			 * @param id идентификатор события
			 * @return   состояние события
			 */
			virtual bool isAlive(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод очистки основного движка фреймворка
			 *
			 */
			virtual void clear() noexcept = 0;
		public:
			/**
			 * @brief Метод принудительного срабатывания события
			 *
			 * @return результат выполнения операции
			 */
			virtual bool kick() noexcept = 0;
			/**
			 * @brief Метод инициализации основного движка фреймворка
			 *
			 * @return результат выполнения инициализации
			 */
			virtual bool initialize() noexcept = 0;
			/**
			 * @brief Метод реинициализации основного движка фреймворка
			 *
			 * @return результат выполнения реинициализации
			 */
			virtual bool reinitialize() noexcept = 0;
			/**
			 * @brief Метод деинициализации основного движка фреймворка
			 *
			 * @return результат выполнения деинициализации
			 */
			virtual bool deinitialize() noexcept = 0;
		public:
			/**
			 * @brief Метод проверки состояния инициализации основного движка фреймворка
			 *
			 * @return состояние инициализации
			 */
			virtual bool isInitialized() const noexcept = 0;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode режим безопасности потоков
			 */
			virtual void threadSafety(const event::mode_t mode) noexcept = 0;
			/**
			 * @brief Метод установки параметров пула потоков
			 *
			 * @param mode режим работы пула потоков
			 * @param size количество потоков в пуле
			 */
			virtual void threadPool(const event::mode_t mode, const uint16_t size) noexcept = 0;
		public:
			/**
			 * @brief Метод получения количества событий в основном движке фреймворка
			 *
			 * @return количество событий
			 */
			virtual size_t eventsCount() const noexcept = 0;
		public:
			/**
			 * @brief Метод получения размера отслеживаемого файла
			 *
			 * @param id идентификатор события
			 * @return   размер файла
			 */
			virtual size_t size(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод получения типа события
			 *
			 * @param id идентификатор события
			 * @return   тип события
			 */
			virtual event::type_t type(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения типа узла события
			 *
			 * @param id идентификатор события
			 * @return   тип узла события
			 */
			virtual event::node_t node(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения семейства события
			 *
			 * @param id идентификатор события
			 * @return   семейство события
			 */
			virtual event::family_t family(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод получения статуса события
			 *
			 * @param id идентификатор события
			 * @return   статус события
			 */
			virtual event::status_t status(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод опроса событий
			 *
			 * @param timeout таймаут опроса в миллисекундах
			 * @return        результат выполнения опроса
			 */
			virtual bool poll(const int32_t timeout = -1) noexcept = 0;
		public:
			/**
			 * @brief Методы установки функции обратного вызова на чтение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::read_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на запись события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::write_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на получение общего события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::event_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на ошибку события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::error_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на изменение статуса события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::status_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на изменение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::change_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на принятие события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::accept_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова срабатывающая при принятии первых событий однорангового узла-источника
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::origin_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на подключение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::connect_t & cb) noexcept = 0;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Engine(const fmk_t * fmk, const log_t * log) noexcept : _eth(fmk, log), _fmk(fmk), _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Engine() noexcept {}
	} engine_t;
};

#endif // __AWH_ENGINE__
