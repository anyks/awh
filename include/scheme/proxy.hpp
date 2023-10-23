/**
 * @file: proxy.hpp
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

#ifndef __AWH_SCHEME_PROXY_SERVER__
#define __AWH_SCHEME_PROXY_SERVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <vector>

/**
 * Наши модули
 */
#include <http/proxy.hpp>
#include <scheme/client.hpp>
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
		 * SchemeProxy Структура схемы сети REST сервера
		 */
		typedef struct SchemeProxy : public scheme_t {
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
				 * Options Структура параметров активного клиента
				 */
				typedef struct Options {
					bool crypt;                       // Флаг шифрования сообщений
					bool alive;                       // Флаг долгоживущего подключения
					bool close;                       // Флаг требования закрыть брокера
					bool locked;                      // Флаг блокировки обработки запроса
					bool connect;                     // Флаг выполненного подключения
					bool stopped;                     // Флаг принудительной остановки
					allow_t allow;                    // Объект разрешения обмена данными
					locker_t locker;                  // Объект блокировщика
					size_t requests;                  // Количество выполненных запросов
					time_t checkPoint;                // Контрольная точка ответа на пинг
					httpProxy_t srv;                  // Создаём объект для работы с HTTP сервером
					client::http_t cli;               // Создаём объект для работы с HTTP клиентом
					client::scheme_t scheme;          // Объект рабочей схемы клиента
					vector <char> client;             // Буфер бинарных необработанных данных клиента
					vector <char> server;             // Буфер бинарных необработанных данных сервера
					web_t::method_t method;           // Метод HTTP выполняемого запроса
					awh::http_t::compress_t compress; // Метод компрессии данных
					/**
					 * Options Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Options(const fmk_t * fmk, const log_t * log) noexcept :
					 crypt(false), alive(false), close(false), locked(false),
					 connect(false), stopped(false), requests(0), checkPoint(0),
					 srv(fmk, log), cli(fmk, log), scheme(fmk, log),
					 method(web_t::method_t::NONE),
					 compress(awh::http_t::compress_t::NONE) {}
					/**
					 * ~Options Деструктор
					 */
					~Options() noexcept {}
				} options_t;
			public:
				// Список пар клиентов
				map <uint64_t, uint64_t> pairs;
			private:
				// Список параметров активных клиентов
				map <uint64_t, unique_ptr <options_t>> _options;
			public:
				// Список доступных компрессоров
				vector <awh::http_t::compress_t> compressors;
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
				 * set Метод создания параметров активного клиента
				 * @param bid идентификатор брокера
				 */
				void set(const uint64_t bid) noexcept;
				/**
				 * rm Метод удаления параметров активного клиента
				 * @param bid идентификатор брокера
				 */
				void rm(const uint64_t bid) noexcept;
				/**
				 * get Метод получения параметров активного клиента
				 * @param bid идентификатор брокера
				 * @return    параметры активного клиента
				 */
				const options_t * get(const uint64_t bid) const noexcept;
			public:
				/**
				 * SchemeProxy Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeProxy(const fmk_t * fmk, const log_t * log) noexcept : scheme_t(fmk, log), _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeProxy Деструктор
				 */
				~SchemeProxy() noexcept {}
		} proxy_scheme_t;
	};
};

#endif // __AWH_SCHEME_PROXY_SERVER__
