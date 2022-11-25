/**
 * @file: client.hpp
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

#ifndef __AWH_SCHEME_CLIENT__
#define __AWH_SCHEME_CLIENT__

/**
 * Наши модули
 */
#include <scheme/core.hpp>
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
				enum class type_t : uint8_t {
					NONE   = 0x00, // Прокси-сервер не установлен
					HTTP   = 0x01, // Прокси-сервер HTTP(S)
					SOCKS5 = 0x02  // Прокси-сервер Socks5
				};
			public:
				// Семейство интернет-протоколов
				scheme_t::family_t family;
			public:
				// Тип прокси-сервера
				type_t type;
				// Параметры сокет-сервера
				uri_t::url_t url;
			public:
				// Создаем объект сети
				network_t nwk;
			public:
				// Создаём объект работы с URI
				uri_t uri;
				// Создаём объект для работы с Socks5
				socks5_t socks5;
				// Создаём объект для работы с HTTP
				client::http_t http;
			public:
				// Создаём объект фреймворка
				const fmk_t * fmk;
				// Создаём объект работы с логами
				const log_t * log;
			public:
				/**
				 * Proxy Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Proxy(const fmk_t * fmk, const log_t * log) noexcept :
				 family(scheme_t::family_t::IPV4), type(type_t::NONE), nwk(fmk),
				 uri(fmk, &nwk), socks5(log), http(fmk, log, &uri), fmk(fmk), log(log) {}
				/**
				 * ~Proxy Деструктор
				 */
				~Proxy() noexcept {}
		} proxy_t;
		/**
		 * Scheme Структура схемы сети клиента
		 */
		typedef struct Scheme : public awh::scheme_t {
			private:
				/**
				 * Client Core Устанавливаем дружбу с клиентским классом ядра
				 */
				friend class Core;
				/**
				 * Core Устанавливаем дружбу с классом ядра
				 */
				friend class awh::Core;
			public:
				/**
				 * Разрешения на выполнение работы
				 */
				enum class work_t : uint8_t {
					ALLOW    = 0x01, // Разрешено
					DISALLOW = 0x02  // Запрещено
				};
				/**
				 * Формат сжатия тела запроса
				 */
				enum class connect_t : uint8_t {
					SERVER = 0x01, // Сервер
					PROXY  = 0x02  // Прокси-сервер
				};
				/**
				 * Режимы работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Подключение выполнено
					PRECONNECT = 0x02, // Производится подготовка к подключению
					RECONNECT  = 0x03, // Производится переподключение
					DISCONNECT = 0x00  // Подключение не выполнено
				};
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
					Status() noexcept : real(mode_t::DISCONNECT), wait(mode_t::DISCONNECT), work(work_t::ALLOW) {}
				} status_t;
			public:
				// Идентификатор DNS запроса
				size_t did;
			public:
				// Флаг получения данных
				bool acquisition;
			public:
				// Параметры прокси-сервера
				proxy_t proxy;
				// Статус сетевого ядра
				status_t status;
				// Параметры адреса для запроса
				uri_t::url_t url;
			private:
				// Устанавливаем тип подключения
				connect_t _connect;
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
				 * Scheme Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Scheme(const fmk_t * fmk, const log_t * log) noexcept :
				 awh::scheme_t(fmk, log), did(0), acquisition(false),
				 proxy(fmk, log), _connect(connect_t::SERVER) {}
				/**
				 * ~Scheme Деструктор
				 */
				~Scheme() noexcept {}
		} scheme_t;
	};
};

#endif // __AWH_SCHEME_CLIENT__
