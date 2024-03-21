/**
 * @file: server.hpp
 * @date: 2022-09-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Наши модули
 */
#include <core/node.hpp>
#include <core/timer.hpp>
#include <sys/cluster.hpp>
#include <scheme/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Core Класс серверного сетевого ядра
		 */
		typedef class Core : public awh::node_t {
			public:
				/**
				 * Коды ошибок клиента
				 */
				enum class error_t : uint8_t {
					NONE      = 0x00, // Ошибка не установлена
					START     = 0x01, // Ошибка запуска приложения
					ACCEPT    = 0x02, // Ошибка разрешения подключения
					CLUSTER   = 0x03, // Ошибка работы кластера
					PROTOCOL  = 0x04, // Ошибка активации протокола
					OS_BROKEN = 0x05  // Ошибка неподдерживаемой ОС
				};
			private:
				/**
				 * Mutex Объект основных мютексов
				 */
				typedef struct Mutex {
					recursive_mutex main;   // Для установки системных параметров
					recursive_mutex timer;  // Для создания нового таймера
					recursive_mutex close;  // Для закрытия подключения
					recursive_mutex accept; // Для одобрения подключения
				} mtx_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Объект кластера
				cluster_t _cluster;
			private:
				// Размер кластера
				int16_t _clusterSize;
				// Флаг автоматического перезапуска упавших процессов
				bool _clusterAutoRestart;
				// Флаг асинхронного режима обмена сообщениями
				bool _clusterAsyncMessages;
			private:
				// Флаг активации/деактивации кластера
				awh::scheme_t::mode_t _clusterMode;
			private:
				// Список активных дочерних процессов
				multimap <uint16_t, pid_t> _workers;
			private:
				// Список активных таймеров для сетевых схем
				map <uint16_t, unique_ptr <timer_t>> _timers;
				// Список подключённых брокеров
				map <uint16_t, unique_ptr <awh::scheme_t::broker_t>> _brokers;
			private:
				/**
				 * accept Метод вызова при подключении к серверу
				 * @param fd  файловый дескриптор (сокет) подключившегося клиента
				 * @param sid идентификатор схемы сети
				 */
				void accept(const SOCKET fd, const uint16_t sid) noexcept;
				/**
				 * accept Метод вызова при активации DTLS-подключения
				 * @param sid идентификатор схемы сети
				 * @param bid идентификатор брокера
				 */
				void accept(const uint16_t sid, const uint64_t bid) noexcept;
			private:
				/**
				 * launching Метод вызова при активации базы событий
				 * @param mode   флаг работы с сетевым протоколом
				 * @param status флаг вывода события статуса
				 */
				void launching(const bool mode, const bool status) noexcept;
				/**
				 * closedown Метод вызова при деакцтивации базы событий
				 * @param mode   флаг работы с сетевым протоколом
				 * @param status флаг вывода события статуса
				 */
				void closedown(const bool mode, const bool status) noexcept;
			private:
				/**
				 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
				 * @param sid   идентификатор схемы сети
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void cluster(const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept;
				/**
				 * message Метод получения сообщений от дочерних процессоров кластера
				 * @param sid    идентификатор схемы сети
				 * @param pid    идентификатор процесса
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void message(const uint16_t sid, const pid_t pid, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * disable Метод остановки активности брокера подключения
				 * @param bid идентификатор брокера
				 */
				void disable(const uint64_t bid) noexcept;
			private:
				/**
				 * initDTLS Метод инициализации DTLS-брокера
				 * @param sid идентификатор схемы сети
				 */
				void initDTLS(const uint16_t sid) noexcept;
			public:
				/**
				 * master Метод проверки является ли процесс родительским
				 * @return результат проверки
				 */
				bool master() const noexcept;
			public:
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
			public:
				/**
				 * close Метод отключения всех брокеров
				 */
				void close() noexcept;
				/**
				 * remove Метод удаления всех активных схем сети
				 */
				void remove() noexcept;
			public:
				/**
				 * close Метод закрытия подключения брокера
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
				/**
				 * remove Метод удаления схемы сети
				 * @param sid идентификатор схемы сети
				 */
				void remove(const uint16_t sid) noexcept;
			public:
				/**
				 * launch Метод запуска сервера
				 * @param sid идентификатор схемы сети
				 */
				void launch(const uint16_t sid) noexcept;
			private:
				/**
				 * create Метод создания сервера
				 * @param sid идентификатор схемы сети
				 * @return    результат создания сервера
				 */
				bool create(const uint16_t sid) noexcept;
			public:
				/**
				 * port Метод получения порта сервера
				 * @param sid идентификатор схемы сети
				 * @return    порт сервера который он прослушивает
				 */
				u_int port(const uint16_t sid) const noexcept;
				/**
				 * host Метод получения хоста сервера
				 * @param sid идентификатор схемы сети
				 * @return    хост на котором висит сервер
				 */
				const string & host(const uint16_t sid) const noexcept;
			public:
				/**
				 * workers Метод получения списка доступных воркеров
				 * @param sid идентификатор схемы сети
				 * @return    список доступных воркеров
				 */
				set <pid_t> workers(const uint16_t sid) const noexcept;
			public:
				/**
				 * send Метод отправки сообщения родительскому процессу
				 * @param wid    идентификатор воркера
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void send(const uint16_t wid, const char * buffer, const size_t size) noexcept;
				/**
				 * send Метод отправки сообщения дочернему процессу
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void send(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * broadcast Метод отправки сообщения всем дочерним процессам
				 * @param wid    идентификатор воркера
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void broadcast(const uint16_t wid, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * read Метод чтения данных для брокера
				 * @param bid идентификатор брокера
				 */
				void read(const uint64_t bid) noexcept;
			public:
				/**
				 * write Метод записи буфера данных в сокет
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 */
				void write(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * work Метод активации параметров запуска сервера
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void work(const uint16_t sid, const string & ip, const int family) noexcept;
			public:
				/**
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param sid   идентификатор схемы сети
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t sid, const u_short total) noexcept;
			public:
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
				/**
				 * clusterAsyncMessages Метод установки флага асинхронного режима обмена сообщениями
				 * @param mode флаг асинхронного режима обмена сообщениями
				 */
				void clusterAsyncMessages(const bool mode) noexcept;
				/**
				 * cluster Метод установки количества процессов кластера
				 * @param mode флаг активации/деактивации кластера
				 * @param size количество рабочих процессов
				 */
				void cluster(const awh::scheme_t::mode_t mode, const int16_t size = 0) noexcept;
			public:
				/**
				 * init Метод инициализации сервера
				 * @param sid  идентификатор схемы сети
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const uint16_t sid, const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Core Конструктор
				 * @param dns объект DNS-резолвера
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept {}
		} core_t;
	};
};

#endif // __AWH_CORE_SERVER__
