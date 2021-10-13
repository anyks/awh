/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <server/worker.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс клиентского ядра биндинга TCP/IP
	 */
	typedef class CoreServer : public core_t {
		private:
			// Функция обратного вызова при подключении нового клиента
			function <bool (const string &, const string &, const size_t, Core *, void *)> acceptFn = nullptr;
		private:
			/**
			 * read Метод чтения данных с сокета сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void read(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * write Метод записи данных в сокет сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void write(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * event Метод обработка входящих событий с сервера
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
			 * removeAll Метод удаления всех воркеров
			 */
			void removeAll() noexcept;
			/**
			 * remove Метод удаления воркера
			 * @param wid идентификатор воркера
			 */
			void remove(const size_t wid) noexcept;
		public:
			/**
			 * run Метод запуска сервера воркером
			 * @param wid идентификатор воркера
			 */
			void run(const size_t wid) noexcept;
			/**
			 * close Метод закрытия подключения воркера
			 * @param aid идентификатор адъютанта
			 */
			void close(const size_t aid) noexcept;
		public:
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
		public:
			/**
			 * setAccept Метод установки функции обратного вызова при подключении нового клиента
			 * @param ctx      передаваемый объект контекста
			 * @param callback функция обратного вызова для установки
			 */
			void setAccept(void * ctx, function <bool (const string &, const string &, const size_t, Core *, void *)> callback) noexcept;
		public:
			/**
			 * CoreServer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			CoreServer(const fmk_t * fmk, const log_t * log) noexcept : core_t(fmk, log) {}
			/**
			 * ~CoreServer Деструктор
			 */
			~CoreServer() noexcept;
	} coreSrv_t;
};

#endif // __AWH_CORE_SERVER__
