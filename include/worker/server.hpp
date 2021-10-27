/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WORKER_SERVER__
#define __AWH_WORKER_SERVER__

/**
 * Наши модули
 */
#include <worker/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WorkerServer Структура сервера воркера
	 */
	typedef struct WorkerServer : public worker_t {
		private:
			// Core Устанавливаем дружбу с классом ядра
			friend class Core;
			// CoreServer Устанавливаем дружбу с серверным классом ядра
			friend class CoreServer;
		protected:
			// Файловый дескриптор сервера
			evutil_socket_t fd = -1;
			// Порт сервера
			u_int port = SERVER_PORT;
			// Хост сервера
			string host = SERVER_HOST;
			// Максимальное количество одновременных подключений
			u_short total = SERVER_TOTAL_CONNECT;
		protected:
			// Событие подключения к серверу
			struct event * ev = nullptr;
		public:
			// Функция обратного вызова при подключении нового клиента
			function <bool (const string &, const string &, const size_t, Core *, void *)> acceptFn = nullptr;
		public:
			/**
			 * clear Метод очистки
			 */
			void clear() noexcept;
		public:
			/**
			 * WorkerServer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			WorkerServer(const fmk_t * fmk, const log_t * log) noexcept : worker_t(fmk, log) {}
			/**
			 * ~WorkerServer Деструктор
			 */
			~WorkerServer() noexcept {}
	} workSrv_t;
};

#endif // __AWH_WORKER_SERVER__
