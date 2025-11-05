/**
 * @file: io.hpp
 * @date: 2025-10-27
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

#ifndef __AWH_IO_ENGINE__
#define __AWH_IO_ENGINE__

/**
 * Наши модули
 */
#include "engine.hpp"

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
	 * @brief Тип асинхронного движка ввода-вывода
	 *
	 */
	typedef class IO : public engine_t {
		public:
			/**
			 * @brief Метод настройки события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения настройки
			 */
			bool setup(const event::id_t id) noexcept;
		public:
			/**
			 * @brief Метод опроса событий
			 *
			 * @param timeout таймаут опроса в миллисекундах
			 * @return        результат выполнения опроса
			 */
			bool poll(const int32_t timeout = -1) noexcept;
		public:
			/**
			 * @brief Метод получения порта события
			 *
			 * @param id идентификатор события
			 * @return   порт события
			 */
			uint16_t port(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки порта события
			 *
			 * @param id   идентификатор события
			 * @param port порт события
			 * @return     результат выполнения установки
			 */
			bool port(const event::id_t id, const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод получения хоста события
			 *
			 * @param id идентификатор события
			 * @return   хост события
			 */
			string host(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки хоста события
			 *
			 * @param id   идентификатор события
			 * @param host хост события
			 * @return     результат выполнения установки
			 */
			bool host(const event::id_t id, const string & host) noexcept;
		public:
			/**
			 * @brief Метод получения типа узла события
			 *
			 * @param id идентификатор события
			 * @return   тип узла события
			 */
			event::node_t node(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки типа узла события
			 * @param id   идентификатор события
			 * @param node тип узла события
			 * @return     результат выполнения установки
			 */
			bool node(const event::id_t id, const event::node_t node) noexcept;
		public:
			/**
			 * @brief Метод получения адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @return        значение адреса события
			 */
			string address(const event::id_t id, const event::address_t address) const noexcept;
			/**
			 * @brief Метод установки адреса события
			 *
			 * @param id      идентификатор события
			 * @param address тип адреса события
			 * @param value   значение адреса события
			 * @return        результат выполнения установки
			 */
			bool address(const event::id_t id, const event::address_t address, const string & value) noexcept;
		public:
			/**
			 * @brief Метод удаления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения удаления
			 */
			bool destroy(const event::id_t id) noexcept;
		public:
			/**
			 * @brief Метод создания нового события на основе существующего
			 *
			 * @param id       идентификатор существующего события
			 * @param protocol протокол сокета
			 * @param mode     режим сокета
			 * @return         идентификатор созданного события
			 */
			event::id_t event(const event::id_t id, const event::protocol_t protocol, const event::mode_t mode) noexcept;
			/**
			 * @brief Метод создания нового события
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @param mode     режим сокета
			 * @return         идентификатор созданного события
			 */
			event::id_t event(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept;
			/**
			 * @brief Метод получения пары событий для сокета
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @param mode     режим сокета
			 * @return         пара идентификаторов созданных событий
			 */
			std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод получения режима действия события
			 *
			 * @param id     идентификатор события
			 * @param action действие события
			 * @return       режим действия события
			 */
			event::notify_t action(const event::id_t id, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки режима действия события
			 *
			 * @param id     идентификатор события
			 * @param action действие события
			 * @param notify уведомления события
			 * @return       результат выполнения установки
			 */
			bool action(const event::id_t id, const event::action_t action, const event::notify_t notify) noexcept;
		public:
			/**
			 * @brief Метод получения опций события
			 *
			 * @param id идентификатор события
			 * @return   опции события
			 */
			uint16_t options(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки опций события
			 *
			 * @param id      идентификатор события
			 * @param options опции события для установки
			 * @return        результат выполнения установки
			 */
			bool options(const event::id_t id, const uint16_t options) noexcept;
			/**
			 * @brief Метод установки опции события
			 *
			 * @param id     идентификатор события
			 * @param option опция события для установки
			 * @param mode   режим установки опции события
			 * @return       результат выполнения установки
			 */
			bool option(const event::id_t id, const uint16_t option, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод отключения события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения отключения
			 */
			bool disconnect(const event::id_t id) noexcept;
			/**
			 * @brief Метод подключения события к удалённому хосту
			 *
			 * @param id    идентификатор события
			 * @param async флаг асинхронного подключения
			 * @return      результат выполнения подключения
			 */
			bool connect(const event::id_t id, const bool async = false) noexcept;
			/**
			 * @brief Метод принятия входящего соединения события
			 *
			 * @param id    идентификатор события
			 * @param max   максимальное количество входящих соединений
			 * @param async флаг асинхронного принятия соединения
			 * @return      результат выполнения принятия соединения
			 */
			bool accept(const event::id_t id, const uint16_t max, const bool async = false) noexcept;
		public:
			/**
			 * @brief Метод отправки события
			 *
			 * @param value значение события для отправки
			 * @return      результат выполнения отправки
			 */
			bool post(const uint32_t value) noexcept;
			/**
			 * @brief Метод отправки данных события
			 *
			 * @param id   идентификатор события
			 * @param data указатель на данные для отправки
			 * @param size размер данных для отправки
			 * @return     результат выполнения отправки
			 */
			bool send(const event::id_t id, const char * data, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод очистки всех адресов сетей для выхода в интернет
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			bool clearNetworks(const event::id_t id) noexcept;
		public:
			/**
			 * @brief Метод получения списка адресов сетей для выхода в интернет
			 *
			 * @param id идентификатор события
			 * @return   список адресов сетей события
			 */
			std::unordered_set <string> networks(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Метод добавления адреса сети для выхода в интернет
			 *
			 * @param id      идентификатор события
			 * @param network адрес сети для добавления
			 * @return        результат выполнения добавления
			 */
			bool addNetwork(const event::id_t id, const string & network) noexcept;
			/**
			 * @brief Метод удаления адреса сети для выхода в интернет
			 *
			 * @param id      идентификатор события
			 * @param network адрес сети для удаления
			 * @return        результат выполнения удаления
			 */
			bool removeNetwork(const event::id_t id, const string & network) noexcept;
		public:
			/**
			 * @brief Метод добавления списка адресов сетей для выхода в интернет
			 *
			 * @param id       идентификатор события
			 * @param networks список адресов сетей для добавления
			 * @return         результат выполнения добавления
			 */
			bool addNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept;
			/**
			 * @brief Метод удаления списка адресов сетей для выхода в интернет
			 *
			 * @param id       идентификатор события
			 * @param networks список адресов сетей для удаления
			 * @return         результат выполнения удаления
			 */
			bool removeNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept;
		public:
			/**
			 * @brief Метод получения сетевого интерфейса события
			 *
			 * @param id идентификатор события
			 * @return   сетевой интерфейс события
			 */
			string networkInterface(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки сетевого интерфейса события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 */
			bool setNetworkInterface(const event::id_t id, const string & name) noexcept;
		public:
			/**
			 * @brief Метод очистки всех сетевых интерфейсов события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			bool clearNetworkInterfaces(const event::id_t id) noexcept;
			/**
			 * @brief Метод получения списка сетевых интерфейсов события
			 *
			 * @param id идентификатор события
			 * @return   список сетевых интерфейсов события
			 */
			std::unordered_set <string> networkInterfaces(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Метод добавления сетевого интерфейса для события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для добавления
			 * @return     результат выполнения добавления
			 */
			bool addNetworkInterface(const event::id_t id, const string & name) noexcept;
			/**
			 * @brief Метод удаления сетевого интерфейса для события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для удаления
			 * @return     результат выполнения удаления
			 */
			bool removeNetworkInterface(const event::id_t id, const string & name) noexcept;
		public:
			/**
			 * @brief Метод добавления списка сетевых интерфейсов для события
			 *
			 * @param id    идентификатор события
			 * @param names список сетевых интерфейсов для добавления
			 * @return      результат выполнения добавления
			 */
			bool addNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept;
			/**
			 * @brief Метод удаления списка сетевых интерфейсов для события
			 *
			 * @param id    идентификатор события
			 * @param names список сетевых интерфейсов для удаления
			 * @return      результат выполнения удаления
			 */
			bool removeNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept;
		public:
			/**
			 * @brief Метод присоединения события к мультикаст группе
			 *
			 * @param id               идентификатор события
			 * @param multicastAddress адрес мультикаст группы для присоединения
			 * @return                 результат выполнения присоединения
			 */
			bool multicastJoin(const event::id_t id, const string & multicastAddress) noexcept;
			/**
			 * @brief Метод выхода события из мультикаст группы
			 *
			 * @param id               идентификатор события
			 * @param multicastAddress адрес мультикаст группы для выхода
			 * @return                 результат выполнения выхода
			 */
			bool multicastLeave(const event::id_t id, const string & multicastAddress) noexcept;
		public:
			/**
			 * @brief Метод очистки чёрного списка события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			bool clearBlacklist(const event::id_t id) noexcept;
			/**
			 * @brief Метод добавления адреса в чёрный список события
			 *
			 * @param id    идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 */
			bool addToBlacklist(const event::id_t id, const string & value) noexcept;
			/**
			 * @brief Метод удаления адреса из чёрного списка события
			 *
			 * @param id    идентификатор события
			 * @param value адрес для удаления из чёрного списка
			 * @return      результат выполнения удаления
			 */
			bool removeFromBlacklist(const event::id_t id, const string & value) noexcept;
			/**
			 * @brief Метод получения чёрного списка события
			 *
			 * @param id идентификатор события
			 * @return   чёрный список события
			 */
			const std::unordered_map <string, event::address_t> & blacklist(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Метод очистки белого списка события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения очистки
			 */
			bool clearWhitelist(const event::id_t id) noexcept;
			/**
			 * @brief Метод добавления адреса в белый список события
			 *
			 * @param id    идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 */
			bool addToWhitelist(const event::id_t id, const string & value) noexcept;
			/**
			 * @brief Метод удаления адреса из белого списка события
			 *
			 * @param id    идентификатор события
			 * @param value адрес для удаления из белого списка
			 * @return      результат выполнения удаления
			 */
			bool removeFromWhitelist(const event::id_t id, const string & value) noexcept;
			/**
			 * @brief Метод получения белого списка события
			 *
			 * @param id идентификатор события
			 * @return   белый список события
			 */
			const std::unordered_map <string, event::address_t> & whitelist(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Метод установки глубины очереди принятия входящих соединений события
			 *
			 * @param id       идентификатор события
			 * @param depth    глубина очереди принятия входящих соединений
			 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
			 */
			void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия с буфером
			 * @return       размер буфера события
			 */
			size_t bufferSize(const event::id_t id, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия с буфером
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 */
			bool bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения таймаута события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       значение таймаута в миллисекундах
			 */
			uint16_t timeout(const event::id_t id, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки таймаута события
			 *
			 * @param id      идентификатор события
			 * @param action  тип действия события
			 * @param timeout значение таймаута в миллисекундах
			 */
			void timeout(const event::id_t id, const event::action_t action, const uint16_t timeout) noexcept;
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
			bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
		public:
			/**
			 * @brief Метод приостановки события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения приостановки
			 */
			bool pause(const event::id_t id) noexcept;
			/**
			 * @brief Метод возобновления события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения возобновления
			 */
			bool resume(const event::id_t id) noexcept;
		public:
			/**
			 * @brief Метод проверки состояния события
			 *
			 * @param id идентификатор события
			 * @return   состояние события
			 */
			bool isAlive(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Метод инициализации основного движка фреймворка
			 *
			 * @return результат выполнения инициализации
			 */
			bool initialize() noexcept;
			/**
			 * @brief Метод деинициализации основного движка фреймворка
			 *
			 * @return результат выполнения деинициализации
			 */
			bool deinitialize() noexcept;
		public:
			/**
			 * @brief Метод проверки состояния инициализации основного движка фреймворка
			 *
			 * @return состояние инициализации
			 */
			bool isInitialized() const noexcept;
		public:
			/**
			 * @brief Метод получения режима события
			 *
			 * @param id идентификатор события
			 * @return   режим события
			 */
			event::mode_t mode(const event::id_t id) const noexcept;
			/**
			 * @brief Метод получения типа события
			 *
			 * @param id идентификатор события
			 * @return   тип события
			 */
			event::type_t type(const event::id_t id) const noexcept;
			/**
			 * @brief Метод получения семейства события
			 *
			 * @param id идентификатор события
			 * @return   семейство события
			 */
			event::family_t family(const event::id_t id) const noexcept;
			/**
			 * @brief Метод получения статуса события
			 *
			 * @param id идентификатор события
			 * @return   статус события
			 */
			event::status_t status(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Методы установки функции обратного вызова на чтение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::read_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на запись события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::write_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на ошибку события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::error_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на изменение статуса события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::status_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на принятие события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::accept_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на подключение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::connect_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на получение пользовательского события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::user_t & cb) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit IO(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~IO() noexcept;
	} io_t;
};

#endif // __AWH_IO_ENGINE__
