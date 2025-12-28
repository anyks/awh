/**
 * @file: io.hpp
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
			 * @brief Метод фиксации настроек события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения фиксации
			 */
			bool commit(const event::id_t id) noexcept;
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
			 * @brief Метод получения сетевого интерфейса события
			 *
			 * @param id идентификатор события
			 * @return   сетевой интерфейс события
			 */
			string iface(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки сетевого интерфейса события
			 *
			 * @param id   идентификатор события
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 */
			bool iface(const event::id_t id, const string & name) noexcept;
		public:
			/**
			 * @brief Метод получения хоста целевой машины
			 *
			 * @param id идентификатор события
			 * @return   хост целевой машины
			 */
			string target(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки хоста целевой машины
			 *
			 * @param id   идентификатор события
			 * @param host хост целевой машины
			 * @return     результат выполнения установки
			 */
			bool target(const event::id_t id, const string & target) noexcept;
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
			 * @param node     узел события
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 */
			event::id_t event(const event::id_t id, const event::node_t node, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			/**
			 * @brief Метод создания нового события
			 *
			 * @param node     узел события
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         идентификатор созданного события
			 */
			event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * @brief Метод получения пары событий для сокета
			 *
			 * @param family   семейство сокета
			 * @param type     тип сокета
			 * @param protocol протокол сокета
			 * @return         пара идентификаторов созданных событий
			 */
			std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * @brief Метод получения смещения в файле события
			 *
			 * @param id   идентификатор события
			 * @param seek тип смещения в файле события
			 * @return     смещение в файле события
			 */
			size_t seek(const event::id_t id, const event::seek_t seek) noexcept;
			/**
			 * @brief Метод установки смещения в файле события
			 *
			 * @param id     идентификатор события
			 * @param seek   тип смещения в файле события
			 * @param offset смещение в файле события
			 * @return       результат выполнения установки
			 */
			bool seek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept;
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
			 * @brief Метод перемещения данных между событиями
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения перемещения
			 */
			bool splice(const event::id_t eid, const event::id_t dest) noexcept;
		public:
			/**
			 * @brief Метод получения информационных метаданных SCTP сообщения
			 *
			 * @param id идентификатор события
			 * @return   информационные метаданные SCTP сообщения
			 */
			net::sctp_minfo_t sctpMessageInfo(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки информационных метаданных SCTP сообщения
			 *
			 * @param id   идентификатор события
			 * @param info информационные метаданные SCTP сообщения
			 */
			void sctpMessageInfo(const event::id_t id, const net::sctp_minfo_t & info) noexcept;
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
		public:
			/**
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param id    идентификатор события
			 * @param max   максимальное количество входящих соединений
			 * @param async флаг асинхронного прослушивания
			 * @return      результат выполнения перевода в режим прослушивания
			 */
			bool listen(const event::id_t id, const uint16_t max, const bool async = true) noexcept;
		public:
			/**
			 * @brief Метод приёма данных события
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения приёма
			 */
			bool recv(const event::id_t id) noexcept;
			/**
			 * @brief Метод отправки данных события
			 *
			 * @param id   идентификатор события
			 * @param data буфер данных для отправки
			 * @param size размер данных для отправки
			 * @return     результат выполнения отправки
			 */
			bool send(const event::id_t id, const char * data, const size_t size) noexcept;
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
			 * @brief Метод активации/деактивации мультикаст группы события
			 *
			 * @param id    идентификатор события
			 * @param mode  режим активации/деактивации
			 * @param group мультикаст-группа для активации/деактивации
			 * @return      результат выполнения установки
			 */
			bool membership(const event::id_t id, const event::mode_t mode, const string & group) noexcept;
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
			 * @param action тип действия события
			 * @return       размер буфера события
			 */
			size_t bufferSize(const event::id_t id, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки размера буфера события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @param size   размер буфера события
			 * @return       результат выполнения установки
			 */
			bool bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения режима трансляции пакетов для события
			 *
			 * @param id идентификатор события
			 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
			 */
			event::cast_t cast(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки режима трансляции пакетов для события
			 *
			 * @param id   идентификатор события
			 * @param cast режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return     результат выполнения установки
			 */
			bool cast(const event::id_t id, const event::cast_t cast) noexcept;
		public:
			/**
			 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
			 *
			 * @param id идентификатор события
			 * @return   максимальное количество хопов
			 */
			event::hops_t hops(const event::id_t id) const noexcept;
			/**
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param id     идентификатор события
			 * @param family семейство протоколов (IPv4 или IPv6)
			 * @param hops   максимальное количество хопов
			 * @return       результат работы функции
			 */
			bool hops(const event::id_t id, const event::family_t family, const event::hops_t hops) noexcept;
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
			 * @brief Метод получения действия события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @return       режим действия события
			 */
			event::mode_t action(const event::id_t id, const event::action_t action) const noexcept;
			/**
			 * @brief Метод установки действия события
			 *
			 * @param id     идентификатор события
			 * @param action тип действия события
			 * @param mode   режим установки действия события
			 * @return       результат выполнения установки
			 */
			bool action(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept;
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
			 * @brief Метод очистки основного движка фреймворка
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод принудительного срабатывания события
			 *
			 * @return результат выполнения операции
			 */
			bool kick() noexcept;
			/**
			 * @brief Метод инициализации основного движка фреймворка
			 *
			 * @return результат выполнения инициализации
			 */
			bool initialize() noexcept;
			/**
			 * @brief Метод реинициализации основного движка фреймворка
			 *
			 * @return результат выполнения реинициализации
			 */
			bool reinitialize() noexcept;
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
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode режим безопасности потоков
			 */
			void threadSafety(const event::mode_t mode) noexcept;
			/**
			 * @brief Метод установки параметров пула потоков
			 *
			 * @param mode режим работы пула потоков
			 * @param size количество потоков в пуле
			 */
			void threadPool(const event::mode_t mode, const uint16_t size) noexcept;
		public:
			/**
			 * @brief Метод получения количества событий в основном движке фреймворка
			 *
			 * @return количество событий
			 */
			size_t eventsCount() const noexcept;
		public:
			/**
			 * @brief Метод получения размера отслеживаемого файла
			 *
			 * @param id идентификатор события
			 * @return   размер файла
			 */
			size_t size(const event::id_t id) const noexcept;
		public:
			/**
			 * @brief Метод получения типа события
			 *
			 * @param id идентификатор события
			 * @return   тип события
			 */
			event::type_t type(const event::id_t id) const noexcept;
			/**
			 * @brief Метод получения типа узла события
			 *
			 * @param id идентификатор события
			 * @return   тип узла события
			 */
			event::node_t node(const event::id_t id) const noexcept;
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
			 * @brief Метод опроса событий
			 *
			 * @param timeout таймаут опроса в миллисекундах
			 * @return        результат выполнения опроса
			 */
			bool poll(const int32_t timeout = -1) noexcept;
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
			 * @brief Методы установки функции обратного вызова на получение общего события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::event_t & cb) noexcept;
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
			 * @brief Методы установки функции обратного вызова на изменение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::change_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на принятие события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::accept_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова срабатывающая при принятии первых событий однорангового узла-источника
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::origin_t & cb) noexcept;
			/**
			 * @brief Методы установки функции обратного вызова на подключение события
			 *
			 * @param id идентификатор события
			 * @param cb объект обратного вызова события
			 */
			void on(const event::id_t id, const event::callback::connect_t & cb) noexcept;
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
