/**
 * @file: server.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Наши модули
 */
#include <net/if.hpp>
#include <core/core.hpp>
#include <worker/server.hpp>

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
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class Core : public awh::core_t {
			private:
				/**
				 * Mutex Объект основных мютексов
				 */
				typedef struct Mutex {
					mutex thread;           // Для работы в дочерних потоках
					recursive_mutex close;  // Для закрытия подключения
					recursive_mutex accept; // Для одобрения подключения
					recursive_mutex system; // Для установки системных параметров
				} mtx_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t mtx;
			private:
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet;
			private:
				/**
				 * resolver Функция выполнения резолвинга домена
				 * @param ip  полученный IP адрес
				 * @param ctx передаваемый контекст
				 */
				static void resolver(const string ip, void * ctx) noexcept;
				/**
				 * read Функция чтения данных с сокета сервера
				 * @param bev буфер события
				 * @param ctx передаваемый контекст
				 */
				static void read(struct bufferevent * bev, void * ctx) noexcept;
				/**
				 * write Функция записи данных в сокет сервера
				 * @param bev буфер события
				 * @param ctx передаваемый контекст
				 */
				static void write(struct bufferevent * bev, void * ctx) noexcept;
				/**
				 * event Функция обработка входящих событий с сервера
				 * @param bev    буфер события
				 * @param events произошедшее событие
				 * @param ctx    передаваемый контекст
				 */
				static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
				/**
				 * accept Функция подключения к серверу
				 * @param fd    файловый дескриптор (сокет)
				 * @param event событие на которое сработала функция обратного вызова
				 * @param ctx   объект передаваемый как значение
				 */
				static void accept(const evutil_socket_t fd, const short event, void * ctx) noexcept;
				/**
				 * thread Функция сборки чанков бинарного буфера в многопоточном режиме
				 * @param adj объект адъютанта
				 * @param wrk объект воркера
				 */
				static void thread(const awh::worker_t::adj_t & adj, const server::worker_t & wrk) noexcept;
			private:
				/**
				 * tuning Метод тюннинга буфера событий
				 * @param aid идентификатор адъютанта
				 */
				void tuning(const size_t aid) noexcept;
				/**
				 * close Метод закрытия сокета
				 * @param fd файловый дескриптор (сокет) для закрытия
				 */
				void close(const evutil_socket_t fd) noexcept;
			public:
				/**
				 * close Метод отключения всех воркеров
				 */
				void close() noexcept;
				/**
				 * remove Метод удаления всех воркеров
				 */
				void remove() noexcept;
			public:
				/**
				 * run Метод запуска сервера воркером
				 * @param wid идентификатор воркера
				 */
				void run(const size_t wid) noexcept;
				/**
				 * remove Метод удаления воркера
				 * @param wid идентификатор воркера
				 */
				void remove(const size_t wid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения воркера
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
			public:
				/**
				 * setBandwidth Метод установки пропускной способности сети
				 * @param aid   идентификатор адъютанта
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void setBandwidth(const size_t aid, const string & read, const string & write) noexcept;
			public:
				/**
				 * setIpV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void setIpV6only(const bool mode) noexcept;
				/**
				 * setTotal Метод установки максимального количества одновременных подключений
				 * @param wid   идентификатор воркера
				 * @param total максимальное количество одновременных подключений
				 */
				void setTotal(const size_t wid, const u_short total) noexcept;
				/**
				 * init Метод инициализации сервера
				 * @param wid  идентификатор воркера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const size_t wid, const u_int port, const string & host = "") noexcept;
				/**
				 * setCert Метод установки файлов сертификата
				 * @param cert  корневой сертификат
				 * @param key   приватный ключ сертификата
				 * @param chain файл цепочки сертификатов
				 */
				void setCert(const string & cert, const string & key, const string & chain = "") noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept;
		} core_t;
	};
};

#endif // __AWH_CORE_SERVER__
