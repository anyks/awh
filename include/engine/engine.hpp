/**
 * @file: engine.hpp
 * @date: 2025-10-26
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
#include "sys.hpp"
#include "event.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../net/net.hpp"

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
	 * @brief Тип основного движка фреймворка
	 *
	 */
	typedef class Engine {
		protected:
			// Объект работы с системой
			mutable sys_t _sys;
			// Объект работы с сетью
			mutable net_t _net;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
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
			 * @brief Метод настройки события
			 *
			 * @param id    идентификатор события
			 * @param delay задержка таймера события в миллисекундах
			 * @return      результат выполнения настройки
			 */
			virtual bool setup(const event::id_t id, const uint16_t delay = 0) noexcept = 0;
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
			 * @brief Метод получения хоста события
			 *
			 * @param id идентификатор события
			 * @return   хост события
			 */
			virtual string host(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки хоста события
			 *
			 * @param id   идентификатор события
			 * @param host хост события
			 * @return     результат выполнения установки
			 */
			virtual bool host(const event::id_t id, const string & host) noexcept = 0;
		public:
			/**
			 * @brief Метод получения типа узла события
			 *
			 * @param id идентификатор события
			 * @return   тип узла события
			 */
			virtual event::node_t node(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки типа узла события
			 * @param id   идентификатор события
			 * @param node тип узла события
			 * @return     результат выполнения установки
			 */
			virtual bool node(const event::id_t id, const event::node_t node) noexcept = 0;
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
			/**
			 * @brief Метод создания нового события на основе существующего
			 *
			 * @param id       идентификатор существующего события
			 * @param protocol протокол сокета
			 * @param mode     режим сокета
			 * @return         идентификатор созданного события
			 */
			virtual event::id_t event(const event::id_t id, const event::protocol_t protocol, const event::mode_t mode) noexcept = 0;
			/**
			 * @brief Метод создания нового события
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @param mode     режим сокета
			 * @return         идентификатор созданного события
			 */
			virtual event::id_t event(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept = 0;
			/**
			 * @brief Метод получения пары событий для сокета
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @param mode     режим сокета
			 * @return         пара идентификаторов созданных событий
			 */
			virtual std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept = 0;
			/**
			 * @brief Метод получения режима действия события
			 *
			 * @param id     идентификатор события
			 * @param action действие события
			 * @return       режим действия события
			 */
			virtual event::notify_t action(const event::id_t id, const event::action_t action) noexcept = 0;
			/**
			 * @brief Метод установки режима действия события
			 *
			 * @param id     идентификатор события
			 * @param action действие события
			 * @param notify уведомления события
			 * @return       результат выполнения установки
			 */
			virtual bool action(const event::id_t id, const event::action_t action, const event::notify_t notify) noexcept = 0;
		public:
			/**
			 * @brief Метод установки флага только IPv6 для события
			 *
			 * @param id     идентификатор события
			 * @param enable флаг только IPv6
			 * @return       результат выполнения установки
			 */
			virtual bool onlyIPv6(const event::id_t id, const bool enable) noexcept = 0;
		public:
			/**
			 * @brief Метод установки опции события
			 *
			 * @param id     идентификатор события
			 * @param option опция события
			 * @param value  значение опции события
			 * @return       результат выполнения установки
			 */
			virtual bool option(const event::id_t id, const event::option_t option, const int32_t value) noexcept = 0;
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
			/**
			 * @brief Метод принятия входящего соединения события
			 *
			 * @param id    идентификатор события
			 * @param max   максимальное количество входящих соединений
			 * @param async флаг асинхронного принятия соединения
			 * @return      результат выполнения принятия соединения
			 */
			virtual bool accept(const event::id_t id, const uint16_t max, const bool async = false) noexcept = 0;
		public:
			/**
			 * @brief Метод отправки события
			 *
			 * @param value значение события для отправки
			 * @return      результат выполнения отправки
			 */
			virtual bool post(const uint32_t value) noexcept = 0;
			/**
			 * @brief Метод отправки данных события
			 *
			 * @param id   идентификатор события
			 * @param data указатель на данные для отправки
			 * @param size размер данных для отправки
			 * @return     результат выполнения отправки
			 */
			virtual bool send(const event::id_t id, const char * data, const size_t size) noexcept = 0;
		public:
			/**
			 * @brief Метод очистки всех адресов сетей для выхода в интернет
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			virtual bool clearNetworks(const event::id_t id) noexcept = 0;
		public:
			/**
			 * @brief Метод получения списка адресов сетей для выхода в интернет
			 *
			 * @param id идентификатор события
			 * @return   список адресов сетей события
			 */
			virtual std::unordered_set <string> networks(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод добавления адреса сети для выхода в интернет
			 *
			 * @param id      идентификатор события
			 * @param network адрес сети для добавления
			 * @return        результат выполнения добавления
			 */
			virtual bool addNetwork(const event::id_t id, const string & network) noexcept = 0;
			/**
			 * @brief Метод удаления адреса сети для выхода в интернет
			 *
			 * @param id      идентификатор события
			 * @param network адрес сети для удаления
			 * @return        результат выполнения удаления
			 */
			virtual bool removeNetwork(const event::id_t id, const string & network) noexcept = 0;
		public:
			/**
			 * @brief Метод добавления списка адресов сетей для выхода в интернет
			 *
			 * @param id       идентификатор события
			 * @param networks список адресов сетей для добавления
			 * @return         результат выполнения добавления
			 */
			virtual bool addNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept = 0;
			/**
			 * @brief Метод удаления списка адресов сетей для выхода в интернет
			 *
			 * @param id       идентификатор события
			 * @param networks список адресов сетей для удаления
			 * @return         результат выполнения удаления
			 */
			virtual bool removeNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept = 0;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса события
			 *
			 * @param id идентификатор события
			 * @return   сетевой интерфейс события
			 */
			virtual string networkInterface(const event::id_t id) const noexcept = 0;
			/**
			 * @brief Метод установки сетевого интерфейса события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 */
			virtual bool setNetworkInterface(const event::id_t id, const string & name) noexcept = 0;
		public:
			/**
			 * @brief Метод очистки всех сетевых интерфейсов события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			virtual bool clearNetworkInterfaces(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод получения списка сетевых интерфейсов события
			 *
			 * @param id идентификатор события
			 * @return   список сетевых интерфейсов события
			 */
			virtual std::unordered_set <string> networkInterfaces(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод добавления сетевого интерфейса для события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для добавления
			 * @return     результат выполнения добавления
			 */
			virtual bool addNetworkInterface(const event::id_t id, const string & name) noexcept = 0;
			/**
			 * @brief Метод удаления сетевого интерфейса для события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для удаления
			 * @return     результат выполнения удаления
			 */
			virtual bool removeNetworkInterface(const event::id_t id, const string & name) noexcept = 0;
		public:
			/**
			 * @brief Метод добавления списка сетевых интерфейсов для события
			 *
			 * @param id    идентификатор события
			 * @param names список сетевых интерфейсов для добавления
			 * @return      результат выполнения добавления
			 */
			virtual bool addNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept = 0;
			/**
			 * @brief Метод удаления списка сетевых интерфейсов для события
			 *
			 * @param id    идентификатор события
			 * @param names список сетевых интерфейсов для удаления
			 * @return      результат выполнения удаления
			 */
			virtual bool removeNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept = 0;
		public:
			/**
			 * @brief Метод присоединения события к мультикаст группе
			 *
			 * @param id               идентификатор события
			 * @param multicastAddress адрес мультикаст группы для присоединения
			 * @return                 результат выполнения присоединения
			 */
			virtual bool multicastJoin(const event::id_t id, const string & multicastAddress) noexcept = 0;
			/**
			 * @brief Метод выхода события из мультикаст группы
			 *
			 * @param id               идентификатор события
			 * @param multicastAddress адрес мультикаст группы для выхода
			 * @return                 результат выполнения выхода
			 */
			virtual bool multicastLeave(const event::id_t id, const string & multicastAddress) noexcept = 0;
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
			 * @param id      идентификатор события
			 * @param address адрес для добавления в чёрный список
			 * @return        результат выполнения добавления
			 */
			virtual bool addToBlacklist(const event::id_t id, const string & address) noexcept = 0;
			/**
			 * @brief Метод удаления адреса из чёрного списка события
			 *
			 * @param id      идентификатор события
			 * @param address адрес для удаления из чёрного списка
			 * @return        результат выполнения удаления
			 */
			virtual bool removeFromBlacklist(const event::id_t id, const string & address) noexcept = 0;
			/**
			 * @brief Метод получения чёрного списка события
			 *
			 * @param id идентификатор события
			 * @return   чёрный список события
			 */
			virtual std::unordered_map <event::address_t, string> blacklist(const event::id_t id) const noexcept = 0;
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
			 * @param id      идентификатор события
			 * @param address адрес для добавления в белый список
			 * @return        результат выполнения добавления
			 */
			virtual bool addToWhitelist(const event::id_t id, const string & address) noexcept = 0;
			/**
			 * @brief Метод удаления адреса из белого списка события
			 *
			 * @param id      идентификатор события
			 * @param address адрес для удаления из белого списка
			 * @return        результат выполнения удаления
			 */
			virtual bool removeFromWhitelist(const event::id_t id, const string & address) noexcept = 0;
			/**
			 * @brief Метод получения белого списка события
			 *
			 * @param id идентификатор события
			 * @return   белый список события
			 */
			virtual std::unordered_map <event::address_t, string> whitelist(const event::id_t id) const noexcept = 0;
		public:
			/**
			 * @brief Метод установки таймаута на чтение события
			 *
			 * @param id      идентификатор события
			 * @param timeout значение таймаута в миллисекундах
			 */
			virtual void readTimeout(const event::id_t id, const uint16_t timeout) noexcept = 0;
			/**
			 * @brief Метод установки таймаута на запись события
			 *
			 * @param id      идентификатор события
			 * @param timeout значение таймаута в миллисекундах
			 */
			virtual void writeTimeout(const event::id_t id, const uint16_t timeout) noexcept = 0;
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
			 * @param action тип действия с буфером
			 * @return       размер буфера события
			 */
			virtual size_t bufferSize(const event::id_t id, const event::action_t action) noexcept = 0;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия с буфером
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 */
			virtual bool bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept = 0;
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
			 * @brief Метод инициализации основного движка фреймворка
			 *
			 * @return результат выполнения инициализации
			 */
			virtual bool initialize() noexcept = 0;
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
			 * @brief Метод получения режима события
			 *
			 * @param id идентификатор события
			 * @return   режим события
			 */
			virtual event::mode_t mode(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод получения типа события
			 *
			 * @param id идентификатор события
			 * @return   тип события
			 */
			virtual event::type_t type(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод получения семейства события
			 *
			 * @param id идентификатор события
			 * @return   семейство события
			 */
			virtual event::family_t family(const event::id_t id) noexcept = 0;
			/**
			 * @brief Метод получения статуса события
			 *
			 * @param id идентификатор события
			 * @return   статус события
			 */
			virtual event::status_t status(const event::id_t id) noexcept = 0;
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
			 * @brief Методы установки функции обратного вызова на принятие события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::accept_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на подключение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::connect_t & cb) noexcept = 0;
			/**
			 * @brief Методы установки функции обратного вызова на получение пользовательского события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			virtual void on(const event::id_t id, const event::callback::user_t & cb) noexcept = 0;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Engine(const fmk_t * fmk, const log_t * log) noexcept :
			 _sys(fmk, log), _net(fmk, log), _fmk(fmk), _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Engine() noexcept {}
	} engine_t;
};

#endif // __AWH_ENGINE__
