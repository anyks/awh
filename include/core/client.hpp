/**
 * @file: client.hpp
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

#ifndef __AWH_CORE_CLIENT__
#define __AWH_CORE_CLIENT__

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <worker/client.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class Core : public awh::core_t {
			private:
				/**
				 * Mutex Объект основных мютексов
				 */
				typedef struct Mutex {
					mutex proxy;             // Для работы с прокси-сервером
					mutex thread;            // Для работы в дочерних потоках
					mutex connect;           // Для выполнения подключения
					recursive_mutex close;   // Для закрытия подключения
					recursive_mutex timeout; // Для установки таймаутов
				} mtx_t;
				/**
				 * Timeout Объект работы таймаута
				 */
				typedef struct Timeout {
					size_t wid;                    // Идентификатор воркера
					Core * core;                   // Объект ядра клиента
					struct event ev;               // Объект события
					struct timeval tv;             // Параметры интервала времени
					client::worker_t::mode_t mode; // Режим работы клиента
					/**
					 * Timeout Конструктор
					 */
					Timeout() : wid(0), core(nullptr), mode(client::worker_t::mode_t::DISCONNECT) {}
				} timeout_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t mtx;
			private:
				// Список таймеров
				map <size_t, timeout_t> timeouts;
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
				 * reconnect Функция задержки времени на реконнект
				 * @param fd    файловый дескриптор (сокет)
				 * @param event произошедшее событие
				 * @param ctx   передаваемый контекст
				 */
				static void reconnect(evutil_socket_t fd, short event, void * ctx) noexcept;
				/**
				 * event Функция обработка входящих событий с сервера
				 * @param bev    буфер события
				 * @param events произошедшее событие
				 * @param ctx    передаваемый контекст
				 */
				static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
				/**
				 * thread Функция сборки чанков бинарного буфера в многопоточном режиме
				 * @param adj объект адъютанта
				 * @param wrk объект воркера
				 */
				static void thread(const awh::worker_t::adj_t & adj, const client::worker_t & wrk) noexcept;
			private:
				/**
				 * tuning Метод тюннинга буфера событий
				 * @param aid идентификатор адъютанта
				 */
				void tuning(const size_t aid) noexcept;
			private:
				/**
				 * connect Метод создания подключения к удаленному серверу
				 * @param wid идентификатор воркера
				 */
				void connect(const size_t wid) noexcept;
				/**
				 * reconnect Метод восстановления подключения
				 * @param wid идентификатор воркера
				 */
				void reconnect(const size_t wid) noexcept;
			private:
				/**
				 * createTimeout Метод создания таймаута
				 * @param wid  идентификатор воркера
				 * @param mode режим работы клиента
				 */
				void createTimeout(const size_t wid, const client::worker_t::mode_t mode) noexcept;
			public:
				/**
				 * sendTimeout Метод отправки принудительного таймаута
				 * @param aid идентификатор адъютанта
				 */
				void sendTimeout(const size_t aid) noexcept;
				/**
				 * clearTimeout Метод удаления установленного таймаута
				 * @param wid идентификатор воркера
				 */
				void clearTimeout(const size_t wid) noexcept;
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
				 * open Метод открытия подключения воркером
				 * @param wid идентификатор воркера
				 */
				void open(const size_t wid) noexcept;
				/**
				 * remove Метод удаления воркера из биндинга
				 * @param wid идентификатор воркера
				 */
				void remove(const size_t wid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения воркера
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
				/**
				 * switchProxy Метод переключения с прокси-сервера
				 * @param aid идентификатор адъютанта
				 */
				void switchProxy(const size_t aid) noexcept;
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

#endif // __AWH_CORE_CLIENT__
