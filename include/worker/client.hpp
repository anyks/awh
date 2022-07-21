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

#ifndef __AWH_WORKER_CLIENT__
#define __AWH_WORKER_CLIENT__

/**
 * Наши модули
 */
#include <worker/core.hpp>
#include <http/client.hpp>
#include <socks5/client.hpp>

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
		 * Proxy структура прокси-сервера
		 */
		typedef struct Proxy {
			public:
				/**
				 * Типы прокси-сервера
				 */
				enum class type_t : uint8_t {NONE, HTTP, SOCKS5};
			public:
				// Тип прокси-сервера
				type_t type;
				// Параметры сокет-сервера
				uri_t::url_t url;
			public:
				// Создаём объект работы с URI
				uri_t uri;
				// Создаем объект сети
				network_t nwk;
				// Создаём объект для работы с Socks5
				socks5_t socks5;
				// Создаём объект для работы с HTTP
				client::http_t http;
			public:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
			public:
				/**
				 * Proxy Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Proxy(const fmk_t * fmk, const log_t * log) noexcept :
				 nwk(fmk), uri(fmk, &nwk),
				 http(fmk, log, &uri),
				 socks5(fmk, log, &uri),
				 fmk(fmk), log(log),
				 type(type_t::NONE) {}
				/**
				 * ~Proxy Деструктор
				 */
				~Proxy() noexcept {}
		} proxy_t;
		/**
		 * Worker Структура клиента воркера
		 */
		typedef struct Worker : public awh::worker_t {
			private:
				// Client Core Устанавливаем дружбу с клиентским классом ядра
				friend class Core;
				// Core Устанавливаем дружбу с классом ядра
				friend class awh::Core;
			public:
				/**
				 * Разрешения на выполнение работы
				 */
				enum class work_t : uint8_t {ALLOW, DISALLOW};
				/**
				 * Формат сжатия тела запроса
				 */
				enum class connect_t : uint8_t {SERVER, PROXY};
				/**
				 * Режимы работы клиента
				 */
				enum class mode_t : uint8_t {CONNECT, PRECONNECT, RECONNECT, DISCONNECT};
			private:
				/**
				 * Status Структура статуса подключения
				 */
				typedef struct Status {
					mode_t wait; // Статус ожидание
					mode_t real; // Статус действительность
					work_t work; // Статус разрешения на выполнение работы
					/**
					 * Status Конструктор
					 */
					Status() : real(mode_t::DISCONNECT), wait(mode_t::DISCONNECT), work(work_t::ALLOW) {}
				} status_t;
			public:
				// Параметры прокси-сервера
				proxy_t proxy;
				// Статус сетевого ядра
				status_t status;
				// Параметры адреса для запроса
				uri_t::url_t url;
			public:
				// Идентификатор DNS запроса
				size_t did = 0;
			private:
				// Текущее количество попыток
				u_short attempt = 0;
			public:
				// Выполнять остановку работы воркера, после закрытия подключения
				bool stop = false;
				// Флаг получения данных
				bool acquisition = false;
			private:
				// Устанавливаем тип подключения
				connect_t connect = connect_t::SERVER;
			public:
				// Функция обратного вызова при открытии подключения к прокси-серверу
				function <void (const size_t, const size_t, awh::Core *)> connectProxyFn = nullptr;
				// Функция обратного вызова при получении данных с прокси-сервера
				function <void (const char *, const size_t, const size_t, const size_t, awh::Core *)> readProxyFn = nullptr;
				// Функция обратного вызова при записи данных с прокси-сервера
				function <void (const char *, const size_t, const size_t, const size_t, awh::Core *)> writeProxyFn = nullptr;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
				/**
				 * switchConnect Метод переключения типа подключения
				 */
				void switchConnect() noexcept;
			public:
				/**
				 * isProxy Метод проверки на подключение к прокси-серверу
				 * @return результат проверки
				 */
				bool isProxy() const noexcept;
				/**
				 * getAid Метод получения идентификатора адъютанта
				 * @return идентификатор адъютанта
				 */
				size_t getAid() const noexcept;
			public:
				/**
				 * Worker Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Worker(const fmk_t * fmk, const log_t * log) noexcept : awh::worker_t(fmk, log), proxy(fmk, log) {}
				/**
				 * ~Worker Деструктор
				 */
				~Worker() noexcept {}
		} worker_t;
	};
};

#endif // __AWH_WORKER_CLIENT__
