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

#ifndef __AWH_WORKER_SERVER__
#define __AWH_WORKER_SERVER__

/**
 * Наши модули
 */
#include <worker/core.hpp>

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
		 * Worker Структура сервера воркера
		 */
		typedef struct Worker : public awh::worker_t {
			private:
				// Server Core Устанавливаем дружбу с серверным классом ядра
				friend class Core;
				// Core Устанавливаем дружбу с классом ядра
				friend class awh::Core;
			protected:
				// Файловый дескриптор сервера
				int fd = -1;
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
				function <bool (const string &, const string &, const size_t, awh::Core *, void *)> acceptFn = nullptr;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			public:
				/**
				 * Worker Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Worker(const fmk_t * fmk, const log_t * log) noexcept : awh::worker_t(fmk, log) {}
				/**
				 * ~Worker Деструктор
				 */
				~Worker() noexcept {}
		} worker_t;
	};
};

#endif // __AWH_WORKER_SERVER__
