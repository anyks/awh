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
				ev::io _io;
			protected:
				// Контекст двигателя для работы с передачей данных
				engine_t::ctx_t _ectx;
				// Объект подключения
				engine_t::addr_t _addr;
			protected:
				// Максимальное количество одновременных подключений
				u_int _total;
			protected:
				// Порт сервера
				u_int _port;
				// Хост сервера
				string _host;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			private:
				/**
				 * accept Метод вызова при подключении к серверу
				 * @param watcher объект события подключения
				 * @param revents идентификатор события
				 */
				void accept(ev::io & watcher, int revents) noexcept;
			public:
				/**
				 * Scheme Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Scheme(const fmk_t * fmk, const log_t * log) noexcept :
				 awh::scheme_t(fmk, log), _ectx(fmk, log), _addr(fmk, log),
				 _total(SERVER_TOTAL_CONNECT), _port(SERVER_PORT), _host{SERVER_HOST} {}
				/**
				 * ~Scheme Деструктор
				 */
				~Scheme() noexcept {}
		} scheme_t;
	};
};

#endif // __AWH_SCHEME_SERVER__
