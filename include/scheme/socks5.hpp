/**
 * @file: socks5.hpp
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

#ifndef __AWH_SCHEME_SOCKS5_SERVER__
#define __AWH_SCHEME_SOCKS5_SERVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <vector>

/**
 * Наши модули
 */
#include <scheme/client.hpp>
#include <scheme/server.hpp>
#include <socks5/server.hpp>

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
		 * SchemeSocks5 Структура схемы сети Socks5 сервера
		 */
		typedef struct SchemeSocks5 : public scheme_t {
			public:
				/**
				 * Locker Структура локера
				 */
				typedef struct Locker {
					bool mode;           // Флаг блокировки
					recursive_mutex mtx; // Мютекс для блокировки потока
					/**
					 * Locker Конструктор
					 */
					Locker() noexcept : mode(false) {}
				} locker_t;
				/**
				 * Allow Структура флагов разрешения обменом данных
				 */
				typedef struct Allow {
					bool send;    // Флаг разрешения отправки данных
					bool receive; // Флаг разрешения чтения данных
					/**
					 * Allow Конструктор
					 */
					Allow() noexcept : send(true), receive(true) {}
				} allow_t;
			public:
				/**
				 * Coffer Структура сундука параметров
				 */
				typedef struct Coffer {
					bool locked;             // Флаг блокировки обработки запроса
					bool connect;            // Флаг выполненного подключения
					bool stopped;            // Флаг принудительной остановки
					allow_t allow;           // Объект разрешения обмена данными
					locker_t locker;         // Объект блокировщика
					client::scheme_t scheme; // Объект схемы сети клиента
					server::socks5_t socks5; // Объект для работы с Socks5
					/**
					 * Coffer Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 * @param uri объект работы с URI ссылками
					 */
					Coffer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
					 locked(false), connect(false), stopped(false), scheme(fmk, log), socks5(log) {}
					/**
					 * ~Coffer Деструктор
					 */
					~Coffer() noexcept {}
				} coffer_t;
			public:
				// Создаем объект для работы с сетью
				net_t net;
				// Создаём объект работы с URI ссылками
				uri_t uri;
			public:
				// Список пар клиентов
				map <size_t, size_t> pairs;
			private:
				// Параметры подключения адъютантов
				map <size_t, unique_ptr <coffer_t>> _coffers;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			public:
				/**
				 * set Метод создания параметров адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void set(const size_t aid) noexcept;
				/**
				 * rm Метод удаления параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void rm(const size_t aid) noexcept;
				/**
				 * get Метод получения параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    параметры подключения адъютанта
				 */
				const coffer_t * get(const size_t aid) const noexcept;
			public:
				/**
				 * SchemeSocks5 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeSocks5(const fmk_t * fmk, const log_t * log) noexcept : scheme_t(fmk, log), net(fmk, log), uri(fmk, &net), _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeSocks5 Деструктор
				 */
				~SchemeSocks5() noexcept {}
		} socks5_scheme_t;
	};
};

#endif // __AWH_SCHEME_SOCKS5_SERVER__
