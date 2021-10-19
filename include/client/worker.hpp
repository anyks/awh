/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WORKER_CLIENT__
#define __AWH_WORKER_CLIENT__

/**
 * Наши модули
 */
#include <core/worker.hpp>
#include <client/http.hpp>
#include <client/socks5.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
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
			// Создаём объект для работы с HTTP
			httpCli_t http;
			// Создаём объект для работы с Socks5
			socks5Cli_t socks5;
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
			Proxy(const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), http(fmk, log, &uri), socks5(fmk, log, &uri), fmk(fmk), log(log), type(type_t::NONE) {}
			/**
			 * ~Proxy Деструктор
			 */
			~Proxy() noexcept {}
	} proxy_t;
	/**
	 * WorkerClient Структура клиента воркера
	 */
	typedef struct WorkerClient : public worker_t {
		private:
			// Core Устанавливаем дружбу с классом ядра
			friend class Core;
			// CoreClient Устанавливаем дружбу с клиентским классом ядра
			friend class CoreClient;
		private:
			/**
			 * Формат сжатия тела запроса
			 */
			enum class connect_t : uint8_t {SERVER, PROXY};
		public:
			// Параметры прокси-сервера
			proxy_t proxy;
			// Параметры адреса для запроса
			uri_t::url_t url;
		private:
			// Контекст SSL для работы с защищённым подключением
			ssl_t::ctx_t ssl;
		private:
			// Текущее количество попыток
			u_short attempt = 0;
		private:
			// Устанавливаем тип подключения
			connect_t connect = connect_t::SERVER;
		public:
			// Функция обратного вызова при открытии подключения к прокси-серверу
			function <void (const size_t, const size_t, Core *, void *)> connectProxyFn = nullptr;
			// Функция обратного вызова при получении данных с прокси-сервера
			function <void (const char *, const size_t, const size_t, const size_t, Core *, void *)> readProxyFn = nullptr;
			// Функция обратного вызова при записи данных с прокси-сервера
			function <void (const char *, const size_t, const size_t, const size_t, Core *, void *)> writeProxyFn = nullptr;
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
		public:
			/**
			 * WorkerClient Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			WorkerClient(const fmk_t * fmk, const log_t * log) noexcept : worker_t(fmk, log), proxy(fmk, log) {}
			/**
			 * ~WorkerClient Деструктор
			 */
			~WorkerClient() noexcept {}
	} workCli_t;
};

#endif // __AWH_WORKER_CLIENT__
