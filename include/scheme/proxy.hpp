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
				 * Coffer Структура сундука параметров
				 */
				typedef struct Coffer {
					bool crypt;                       // Флаг шифрования сообщений
					bool alive;                       // Флаг долгоживущего подключения
					bool close;                       // Флаг требования закрыть адъютанта
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
					 * Coffer Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 * @param uri объект работы с URI ссылками
					 */
					Coffer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
					 crypt(false), alive(false), close(false), locked(false),
					 connect(false), stopped(false), requests(0), checkPoint(0),
					 srv(fmk, log, uri), cli(fmk, log, uri), scheme(fmk, log),
					 method(web_t::method_t::NONE),
					 compress(awh::http_t::compress_t::NONE) {}
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
			public:
				// Флаги работы с сжатыми данными
				awh::http_t::compress_t compress;
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
				 * SchemeProxy Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeProxy(const fmk_t * fmk, const log_t * log) noexcept :
				 scheme_t(fmk, log), net(fmk, log), uri(fmk, &net),
				 compress(awh::http_t::compress_t::NONE), _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeProxy Деструктор
				 */
				~SchemeProxy() noexcept {}
		} proxy_scheme_t;
	};
};

#endif // __AWH_SCHEME_PROXY_SERVER__
