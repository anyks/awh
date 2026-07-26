/**
 * @file: io.hpp
 * @date: 2025-11-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл асинхронного движка ввода-вывода — класс engine::IO, реализующий цикл событий,
 *        работу с сокетами всех поддерживаемых семейств и протоколов, таймеры, наблюдение за файлами и каталогами,
 *        списки контроля доступа и поддержку SCTP
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IO_ENGINE__
#define __AWH_IO_ENGINE__

/**
 * Подключаем заголовочный файл проекта
 */
#include "engine.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён движков ввода-вывода
	 *
	 */
	namespace engine {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * Для операционной системы Linux или FreeBSD
		 */
		#if __linux__ || __FreeBSD__
			/**
			 * @brief Класс управления протоколом передачи с управлением потоком
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Stream_Control_Transmission_Protocol {
				private:
					// Объект работы с сетью
					eth_t _eth;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * @brief Метод получения информационных метаданных SCTP сообщения
					 *
					 * @param id идентификатор события
					 * @return   информационные метаданные SCTP сообщения
					 *
					 */
					net::sctp::minfo_t messageInfo(const event::id_t id) const noexcept;
					/**
					 * @brief Метод установки информационных метаданных SCTP сообщения
					 *
					 * @param id   идентификатор события
					 * @param info информационные метаданные SCTP сообщения
					 *
					 */
					void messageInfo(const event::id_t id, const net::sctp::minfo_t & info) noexcept;
				public:
					/**
					 * @brief Метод получения параметров статуса инициализации SCTP
					 *
					 * @param id идентификатор события
					 * @return   параметры статуса инициализации SCTP
					 *
					 */
					net::sctp::status_t status(const event::id_t id) const noexcept;
					/**
					 * @brief Метод установки параметров инициализации SCTP
					 *
					 * @param id      идентификатор события
					 * @param initmsg параметры инициализации SCTP события
					 *
					 */
					void initMessages(const event::id_t id, const net::sctp::initmsg_t & initmsg) noexcept;
				public:
					/**
					 * @brief Метод получения опций подписки SCTP событий
					 *
					 * @param id идентификатор события
					 * @return   список событий SCTP на которые выполнена подписка
					 *
					 */
					const net::sctp::event_types_t & eventsSubscribed(const event::id_t id) const noexcept;
					/**
					 * @brief Метод установки опций подписки SCTP событий
					 *
					 * @param id     идентификатор события
					 * @param events список событий SCTP для подписки
					 *
					 */
					void eventsSubscribe(const event::id_t id, const net::sctp::event_types_t & events) noexcept;
				public:
					/**
					 * @brief Метод получения таймаута SCTP события
					 *
					 * @param id   идентификатор события
					 * @param type тип таймаута
					 * @return     значение таймаута в миллисекундах
					 *
					 */
					uint32_t getTimeout(const event::id_t id, const net::sctp::timeout_t type) const noexcept;
					/**
					 * @brief Метод установки таймаута SCTP события
					 *
					 * @param id      идентификатор события
					 * @param type    тип таймаута
					 * @param timeout значение таймаута в миллисекундах
					 * @return        результат работы функции
					 *
					 */
					bool setTimeout(const event::id_t id, const net::sctp::timeout_t type, const uint32_t timeout) noexcept;
				public:
					/**
					 * @brief Метод установки ключа аутентификации SCTP сокета
					 *
					 * @param id  идентификатор события
					 * @param num номер ключа аутентификации
					 * @param key ключ аутентификации
					 * @return    результат работы функции
					 *
					 */
					bool authenticateKey(const event::id_t id, const uint16_t num, string_view key) noexcept;
					/**
					 * @brief Метод активации/деактивации ключа аутентификации SCTP сокета
					 *
					 * @param id   идентификатор события
					 * @param mode режим установки действия события
					 * @param num  номер ключа аутентификации
					 * @return     результат работы функции
					 *
					 */
					bool authenticateKey(const event::id_t id, const event::mode_t mode, const uint16_t num) noexcept;
				public:
					/**
					 * @brief Метод установки чанков аутентификации SCTP сокета
					 *
					 * @param id     идентификатор события
					 * @param chunks список чанков подлежащих аутентификации
					 * @return       результат работы функции
					 *
					 */
					bool authenticateChunks(const event::id_t id, const vector <net::sctp::auth_chunk_t> & chunks) noexcept;
					/**
					 * @brief Метод извлечения чанков аутентификации SCTP сокета
					 *
					 * @param id     идентификатор события
					 * @param origin источник события
					 * @param chunks список чанков подлежащих аутентификации
					 * @return       результат работы функции
					 *
					 */
					bool authenticateChunks(const event::id_t id, const event::origin_t origin, vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
				public:
					/**
					 * @brief Метод установки поддерживаемых алгоритмов аутентификации SCTP сокета
					 *
					 * @param id    идентификатор события
					 * @param types список поддерживаемых алгоритмов аутентификации
					 * @return      результат работы функции
					 *
					 */
					bool authenticateSupportAlgorithms(const event::id_t id, const vector <net::sctp::auth_type_t> & types) noexcept;
				public:
					/**
					 * @brief Метод установки функции обратного вызова для получения метаданных SCTP-сообщения
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 */
					void on(const event::id_t id, engine::callback::sctp::minfo_t cb) noexcept;
					/**
					 * @brief Метод установки функции обратного вызова для получения SCTP-событий
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 */
					void on(const event::id_t id, engine::callback::sctp::events_t cb) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект работы с логами
					 *
					 */
					explicit Stream_Control_Transmission_Protocol(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Stream_Control_Transmission_Protocol() noexcept;
			} sctp_t;
		#endif
		/**
		 * @brief Тип асинхронного движка ввода-вывода
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ IO : public engine_t {
			public:
				/**
				 * @brief Структура управления списками контроля доступа
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Control_List {
					private:
						// Объект работы с сетевыми адресами
						net_addr_t _addr;
					private:
						// Тип списка контроля доступа
						event::control_list_t _type;
					private:
						// Объект фреймворка
						const fmk_t * _fmk;
						// Объект работы с логами
						const log_t * _log;
					public:
						/**
						 * @brief Метод очистки контрольного списка события
						 *
						 * @param id идентификатор события
						 * @return   результат выполнения очистки
						 *
						 */
						bool clear(const event::id_t id) noexcept;
						/**
						 * @brief Метод добавления адреса в контрольный список события
						 *
						 * @param id    идентификатор события
						 * @param value значение адреса события
						 * @return      результат выполнения установки
						 *
						 */
						bool add(const event::id_t id, string_view value) noexcept;
						/**
						 * @brief Метод удаления адреса из контрольного списка события
						 *
						 * @param id    идентификатор события
						 * @param value адрес для удаления из контрольного списка
						 * @return      результат выполнения удаления
						 *
						 */
						bool remove(const event::id_t id, string_view value) noexcept;
						/**
						 * @brief Метод получения контрольного списка события
						 *
						 * @param id идентификатор события
						 * @return   контрольный список события
						 *
						 */
						const unordered_map <string, event::address_t> & get(const event::id_t id) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param type тип контрольного списка
						 *
						 */
						explicit Control_List(const event::control_list_t type, const fmk_t * fmk, const log_t * log) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						virtual ~Control_List() noexcept;
				} control_list_t;
			public:
				// Объект управления белым списком
				control_list_t whitelist;
				// Объект управления чёрным списком
				control_list_t blacklist;
			public:
				/**
				 * @brief Метод фиксации настроек события
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения фиксации
				 *
				 */
				bool commit(const event::id_t id) noexcept;
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
				bool rebuild(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса события
				 *
				 * @param id идентификатор события
				 * @return   сетевой интерфейс события
				 *
				 */
				string getIface(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса события
				 *
				 * @param id   идентификатор события
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setIface(const event::id_t id, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения локального порта события
				 *
				 * @param id идентификатор события
				 * @return   локальный порт события
				 *
				 */
				uint16_t getSourcePort(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки локального порта события
				 *
				 * @param id   идентификатор события
				 * @param port локальный порт события
				 * @return     результат выполнения установки
				 *
				 */
				bool setSourcePort(const event::id_t id, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения порта назначения события
				 *
				 * @param id идентификатор события
				 * @return   порт назначения события
				 *
				 */
				uint16_t getTargetPort(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки порта назначения события
				 *
				 * @param id   идентификатор события
				 * @param port порт назначения события
				 * @return     результат выполнения установки
				 *
				 */
				bool setTargetPort(const event::id_t id, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param id идентификатор события
				 * @return   адрес хоста целевой машины
				 *
				 */
				string getTarget(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t id, string_view target) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 */
				bool getTarget(const event::id_t id, unique_ptr <net::addr_t> & target) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t id, const net::addr_t * target) noexcept;
			public:
				/**
				 * @brief Метод получения адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @return        значение адреса события
				 *
				 */
				string getAddress(const event::id_t id, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   значение адреса события
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t id, const event::address_t address, string_view value) noexcept;
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
				bool getAddress(const event::id_t id, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
				/**
				 * @brief Метод установки адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   значение адреса события
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t id, const event::address_t address, const net::addr_t * value) noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param id идентификатор события
				 * @return   MTU сетевого интерфейса
				 *
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param id  идентификатор события
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 */
				bool setMaximumTransmissionUnit(const event::id_t id, const uint16_t mtu) const noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение DSCP
				 *
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param dscp   значение DSCP
				 * @return       результат работы функции
				 *
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family, const event::dscp_t dscp) const noexcept;
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
				event::ecn_t getExplicitCongestionNotification(const event::id_t id, const event::family_t family) const noexcept;
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
				bool setExplicitCongestionNotification(const event::id_t id, const event::family_t family, const event::ecn_t ecn) const noexcept;
			public:
				/**
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       режим обнаружения максимального размера пакета (MTU)
				 *
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим обнаружения максимального размера пакета (MTU)
				 * @return       результат работы функции
				 *
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family, const event::mtu_discover_t mode) const noexcept;
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
				bool membership(const event::id_t id, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
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
				bool membership(const event::id_t id, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
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
				bool bind(const event::id_t id, const net::origin_key_t & key) noexcept;
				/**
				 * @brief Метод снятия ключа маршрутизации с сессии
				 *
				 * @param id  идентификатор события сессии
				 * @param key снимаемый ключ сессии
				 * @return    результат снятия (false - ключ сессии не принадлежит)
				 *
				 */
				bool unbind(const event::id_t id, const net::origin_key_t & key) noexcept;
			public:
				/**
				 * @brief Метод получения предельного количества одновременных подключений события
				 *
				 * @param id идентификатор события
				 * @return   предельное количество одновременных подключений
				 *
				 */
				uint32_t getMaxConnections(const event::id_t id) const noexcept;
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
				bool setMaxConnections(const event::id_t id, const uint32_t max) noexcept;
			public:
				/**
				 * @brief Метод удаления события
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения удаления
				 *
				 */
				bool destroy(const event::id_t id) noexcept;
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
				std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
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
				event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * @brief Метод получения смещения в файле события
				 *
				 * @param id   идентификатор события
				 * @param seek тип смещения в файле события
				 * @return     смещение в файле события
				 *
				 */
				size_t getSeek(const event::id_t id, const event::seek_t seek) noexcept;
				/**
				 * @brief Метод установки смещения в файле события
				 *
				 * @param id     идентификатор события
				 * @param seek   тип смещения в файле события
				 * @param offset смещение в файле события
				 * @return       результат выполнения установки
				 *
				 */
				bool setSeek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept;
			public:
				/**
				 * @brief Метод получения опций события
				 *
				 * @param id идентификатор события
				 * @return   опции события
				 *
				 */
				uint16_t getOptions(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки опций события
				 *
				 * @param id      идентификатор события
				 * @param options опции события для установки
				 * @return        результат выполнения установки
				 *
				 */
				bool setOptions(const event::id_t id, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции события
				 *
				 * @param id     идентификатор события
				 * @param option опция события для установки
				 * @param mode   режим установки опции события
				 * @return       результат выполнения установки
				 *
				 */
				bool setOption(const event::id_t id, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод объединения данных между событиями
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения объединения
				 *
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * @brief Метод запуска события
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения запуска
				 *
				 */
				bool launch(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод отключения события
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения отключения
				 *
				 */
				bool disconnect(const event::id_t id) noexcept;
				/**
				 * @brief Шаблон метода мультиподключения события к удалённым хостам
				 *
				 * @tparam Args список идентификаторов событий для подключения
				 *
				 */
				template <typename... Args>
				/**
				 * @brief Метод мультиподключения события к удалённым хостам
				 *
				 * @param args список идентификаторов событий для подключения
				 * @return     результат выполнения подключения
				 *
				 */
				bool connect(Args&&... args) noexcept {
					// Выполняем подключение к списку удалённых серверов
					return this->connect({args...});
				}
				/**
				 * @brief Метод мультиподключения события к удалённым хостам
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
			public:
				/**
				 * @brief Метод перевода события в режим прослушивания входящих соединений
				 *
				 * @param id  идентификатор события
				 * @param max максимальное количество входящих соединений
				 * @return    результат выполнения перевода в режим прослушивания
				 *
				 */
				bool listen(const event::id_t id, const uint32_t max) noexcept;
			public:
				/**
				 * @brief Метод получения данных события
				 *
				 * @param id идентификатор события
				 * @return   результат получения данных
				 *
				 */
				bool recv(const event::id_t id) noexcept;
				/**
				 * @brief Метод отправки данных события
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных событием
				 *
				 */
				size_t send(const event::id_t id, const void * buffer, const size_t size) noexcept;
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
				size_t relay(const event::id_t id, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки глубины очереди принятия входящих соединений события
				 *
				 * @param id       идентификатор события
				 * @param depth    глубина очереди принятия входящих соединений
				 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
				 *
				 */
				void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 */
				size_t getBufferSize(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 */
				bool setBufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности события
				 *
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 *
				 */
				void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * @brief Метод установки пропускной способности события для события
				 *
				 * @param id        идентификатор события
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 */
				bool bandwidth(const event::id_t id, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод получения режима трансляции пакетов для события
				 *
				 * @param id идентификатор события
				 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 */
				event::delivery_mode_t getDelivery(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки режима трансляции пакетов для события
				 *
				 * @param id       идентификатор события
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 */
				bool setDelivery(const event::id_t id, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param id идентификатор события
				 * @return   метаданные последнего принятого дейтаграммного пакета
				 *
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param id идентификатор события
				 * @return   количество хопов последнего принятого пакета
				 *
				 */
				uint8_t getCountHops(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param id   идентификатор события
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 */
				bool setCountHops(const event::id_t id, const uint8_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param id идентификатор события
				 * @return   максимальное количество хопов
				 *
				 */
				event::hops_t getHops(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param id   идентификатор события
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 */
				bool setHops(const event::id_t id, const event::hops_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута для обработки события чтения
				 *
				 * @param id идентификатор события
				 * @return   режим использования таймаута для обработки события чтения
				 *
				 */
				event::usage_t getUsageReadTimeout(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута для обработки события чтения
				 *
				 * @param id    идентификатор события
				 * @param usage режим использования таймаута для обработки события чтения (reusable или disposable)
				 *
				 */
				void setUsageReadTimeout(const event::id_t id, const event::usage_t usage) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       значение таймаута в миллисекундах
				 *
				 */
				uint32_t getTimeout(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута события
				 *
				 * @param id      идентификатор события
				 * @param action  тип действия события
				 * @param timeout значение таймаута в миллисекундах
				 *
				 */
				void setTimeout(const event::id_t id, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод получения действия события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       режим действия события
				 *
				 */
				event::mode_t getAction(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки действия события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param mode   режим установки действия события
				 * @return       результат выполнения установки
				 *
				 */
				bool setAction(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept;
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
				bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод приостановки события
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения приостановки
				 *
				 */
				bool pause(const event::id_t id) noexcept;
				/**
				 * @brief Метод возобновления события
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения возобновления
				 *
				 */
				bool resume(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод проверки состояния события
				 *
				 * @param id идентификатор события
				 * @return   состояние события
				 *
				 */
				bool isAlive(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод очистки сетевого движка
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод принудительного пинка базе событий
				 *
				 * @return результат выполнения операции
				 *
				 */
				bool kick() noexcept;
				/**
				 * @brief Метод инициализации сетевого движка
				 *
				 * @return результат выполнения инициализации
				 *
				 */
				bool initialize() noexcept;
				/**
				 * @brief Метод реинициализации сетевого движка
				 *
				 * @return результат выполнения реинициализации
				 *
				 */
				bool reinitialize() noexcept;
				/**
				 * @brief Метод деинициализации сетевого движка
				 *
				 * @return результат выполнения деинициализации
				 *
				 */
				bool deinitialize() noexcept;
			public:
				/**
				 * @brief Метод проверки состояния инициализации сетевого движка
				 *
				 * @return состояние инициализации
				 *
				 */
				bool isInitialized() const noexcept;
			public:
				/**
				 * @brief Метод получения количества событий в сетевом движке
				 *
				 * @return количество событий
				 *
				 */
				size_t eventsCount() const noexcept;
			public:
				/**
				 * @brief Метод получения типа внутренних таймеров
				 *
				 * @return тип таймера для событий сетевого движка
				 *
				 */
				event::timer_t getInternalTimer() const noexcept;
				/**
				 * @brief Метод установки типа внутренних таймеров
				 *
				 * @param timer тип таймера для событий сетевого движка
				 *
				 */
				void setInternalTimer(const event::timer_t timer) noexcept;
			public:
				/**
				 * @brief Метод получения размера отслеживаемого файла
				 *
				 * @param id идентификатор события
				 * @return   размер файла
				 *
				 */
				size_t size(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения количества байт, доступных для записи в очередь события
				 *
				 * @param id идентификатор события
				 * @return   количество байт, доступных для записи
				 *
				 */
				size_t available(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения типа события
				 *
				 * @param id идентификатор события
				 * @return   тип события
				 *
				 */
				event::type_t type(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения типа узла события
				 *
				 * @param id идентификатор события
				 * @return   тип узла события
				 *
				 */
				event::node_t node(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения семейства события
				 *
				 * @param id идентификатор события
				 * @return   семейство адресов
				 *
				 */
				event::family_t family(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения статуса события
				 *
				 * @param id идентификатор события
				 * @return   статус события
				 *
				 */
				event::status_t status(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения протокола события
				 *
				 * @param id идентификатор события
				 * @return   протокол события
				 *
				 */
				event::protocol_t protocol(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод опроса событий
				 *
				 * @param timeout таймаут опроса в миллисекундах
				 * @return        результат выполнения опроса
				 *
				 */
				bool poll(const int32_t timeout = -1) noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова для обработки события чтения
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::read_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки события записи
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::write_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки возврата неотправленных данных
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::spool_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки общего события
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::event_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки ошибки события
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::error_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки изменений события
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::vnode_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова инъекции объединённых данных (splice)
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::inject_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обновления статуса события
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::status_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для приёма входящего подключения
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::accept_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для определения сессии дейтаграммного пакета
				 *
				 * @note Поддерживается только серверными узлами. Установка функции
				 *       переводит событие на маршрутизацию датаграмм по ключу
				 *       приложения вместо адреса отправителя
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::origin_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова на получение информационных метаданных о дейтаграммном пакете
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::traffic_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки подключения
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::connect_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова на получение информации о пакетах в туннельном интерфейсе
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::tuninfo_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки таймаута события
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::timeout_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки доступности очереди события
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::available_t cb) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 */
				explicit IO(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~IO() noexcept;
		} io_t;
	};
};

#endif // __AWH_IO_ENGINE__
