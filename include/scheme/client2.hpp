/**
 * @file: client.hpp
 * @date: 2025-10-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_SCHEME_CLIENT__
#define __AWH_SCHEME_CLIENT__

/**
 * Наши модули
 */
#include "scheme.hpp"
#include "../http/client2.hpp"
#include "../socks5/client.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief структура прокси-сервера
		 *
		 */
		typedef struct AWH_SHARED_EXPORT Proxy {
			public:
				/**
				 * Типы прокси-сервера
				 */
				enum class type_t : uint8_t {
					NONE   = 0x00, // Прокси-сервер не установлен
					HTTP   = 0x01, // Прокси-сервер HTTP/1.1
					HTTPS  = 0x02, // Прокси-сервер HTTP/2
					SOCKS5 = 0x03  // Прокси-сервер Socks5
				};
			public:
				// Флаг включения прокси-склиента
				bool mode;
			public:
				// Тип прокси-сервера
				type_t type;
			public:
				// Семейство интернет-протоколов
				scheme_t::family_t family;
			public:
				// Параметры сокет-сервера
				uri_t::url_t url;
			public:
				// Создаем объект сети
				net_t net;
				// Объект для работы с Socks5
				socks5_t socks5;
				// Объект для работы с HTTP
				client::http_t http;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Proxy(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Proxy() noexcept {}
		} proxy_t;
		/**
		 * @brief Структура схемы сети клиента
		 *
		 */
		typedef struct AWH_SHARED_EXPORT Scheme : public awh::scheme_t {
			private:
				/**
				 * @brief Core Устанавливаем дружбу с клиентским классом ядра
				 *
				 */
				friend class Core;
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
				 * @brief Структура статуса подключения
				 *
				 */
				typedef struct Status {
					work_t work; // Статус разрешения на выполнение работы
					mode_t wait; // Статус ожидание
					mode_t real; // Статус действительность
					/**
					 * @brief Конструктор
					 *
					 */
					Status() noexcept :
					 work(work_t::ALLOW),
					 real(mode_t::DISCONNECT),
					 wait(mode_t::DISCONNECT) {}
				} status_t;
			public:
				// Флаг получения данных
				bool receiving;
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
				 * @brief Метод очистки
				 *
				 */
				void clear() noexcept;
				/**
				 * @brief Метод переключения типа подключения
				 *
				 */
				void switchConnect() noexcept;
			public:
				/**
				 * @brief Метод проверки на подключение к прокси-серверу
				 *
				 * @return результат проверки
				 */
				bool isProxy() const noexcept;
				/**
				 * @brief Метод получения идентификатора брокера
				 *
				 * @return идентификатор брокера
				 */
				uint32_t bid() const noexcept;
			public:
				/**
				 * @brief Метод активации прокси-клиента
				 *
				 * @param work флаг активации
				 */
				void activateProxy(const work_t work) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Scheme(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Scheme() noexcept {}
		} scheme_t;
	};
};

#endif // __AWH_SCHEME_CLIENT__
