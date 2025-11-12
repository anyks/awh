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
#include "addr.hpp"
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
		public:
			/**
			 * @brief Метод создания нового события на основе существующего
			 *
			 * @param id       идентификатор существующего события
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 */
			virtual event::id_t event(const event::id_t id, const event::protocol_t protocol) noexcept = 0;
			/**
			 * @brief Метод создания нового события
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 */
			virtual event::id_t event(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept = 0;
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
			 * @brief Метод получения типа события
			 *
			 * @param id идентификатор события
			 * @return   тип события
			 */
			virtual event::type_t type(const event::id_t id) const noexcept = 0;
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
			 _eth(fmk, log), _addr(fmk, log), _fmk(fmk), _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Engine() noexcept {}
	} engine_t;
};

#endif // __AWH_ENGINE__
