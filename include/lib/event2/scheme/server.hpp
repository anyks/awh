/**
 * @file: server.hpp
 * @date: 2022-09-03
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

#ifndef __AWH_SCHEME_SERVER__
#define __AWH_SCHEME_SERVER__

/**
 * Наши модули
 */
#include <net/engine.hpp>
#include <scheme/core.hpp>

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
		 * Scheme Структура схемы сети сервера
		 */
		typedef struct Scheme : public awh::scheme_t {
			private:
				/**
				 * Server Core Устанавливаем дружбу с серверным классом ядра
				 */
				friend class Core;
				/**
				 * Core Устанавливаем дружбу с классом ядра
				 */
				friend class awh::Core;
			protected:
				// Событие подключения к серверу
				struct event ev;
			protected:
				// Контекст двигателя для работы с передачей данных
				engine_t::ctx_t ectx;
				// Объект подключения
				engine_t::addr_t addr;
			protected:
				// Максимальное количество одновременных подключений
				u_int total;
			protected:
				// Порт сервера
				u_int port;
				// Хост сервера
				string host;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			private:
				/**
				 * accept Функция подключения к серверу
				 * @param fd    файловый дескриптор (сокет)
				 * @param event произошедшее событие
				 * @param ctx   передаваемый контекст
				 */
				static void accept(evutil_socket_t fd, short event, void * ctx) noexcept;
			public:
				/**
				 * Scheme Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Scheme(const fmk_t * fmk, const log_t * log) noexcept :
				 awh::scheme_t(fmk, log), ectx(fmk, log), addr(fmk, log),
				 total(SERVER_TOTAL_CONNECT), port(SERVER_PORT), host(SERVER_HOST) {}
				/**
				 * ~Scheme Деструктор
				 */
				~Scheme() noexcept {}
		} scheme_t;
	};
};

#endif // __AWH_SCHEME_SERVER__
