/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WORKER__
#define __AWH_WORKER__

/**
 * Стандартная библиотека
 */
#include <string>
#include <event2/bufferevent.h>

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <log.hpp>
#include <ssl.hpp>
#include <http.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Прототип класса ядра биндинга TCP/IP
	 */
	class Core;
	/**
	 * Proxy структура прокси-сервера
	 */
	typedef struct Proxy {
		public:
			/**
			 * Типы прокси-сервера
			 */
			enum class type_t : u_short {NONE, HTTP, SOCKS5};
		public:
			// Тип прокси-сервера
			type_t type;
			// Параметры сокет-сервера
			uri_t::url_t url;
		public:
			// Создаём объект для работы с HTTP
			http_t * http = nullptr;
		public:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
		public:
			/**
			 * Proxy Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Proxy(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Proxy Деструктор
			 */
			~Proxy() noexcept;
	} proxy_t;
	/**
	 * Worker Структура воркера
	 */
	typedef struct Worker {
		private:
			/**
			 * Формат сжатия тела запроса
			 */
			enum class connect_t : u_short {SERVER, PROXY};
		public:
			// Параметры прокси-сервера
			proxy_t proxy;
			// Параметры адреса для запроса
			uri_t::url_t url;
			// Контекст SSL для работы с защищённым подключением
			ssl_t::ctx_t ssl;
		public:
			// Флаг ожидания входящих сообщений
			bool wait = false;
			// Флаг автоматического поддержания подключения
			bool alive = false;
		public:
			// Идентификатор воркера
			size_t wid = 0;
			// Размер байт на чтение
			size_t byteRead = 0;
			// Размер байт на запись
			size_t byteWrite = 0;
			// Таймер на чтение в секундах
			time_t timeRead = READ_TIMEOUT;
			// Таймер на запись в секундах
			time_t timeWrite = WRITE_TIMEOUT;
		public:
			// Сокет сервера для подключения
			evutil_socket_t fd = -1;
		private:
			// Устанавливаем тип подключения
			connect_t connect = connect_t::SERVER;
		public:
			// Количество попыток реконнекта
			pair <u_short, u_short> attempts;
		public:
			// Контекст передаваемый в сообщении
			void * context = nullptr;
			// Буфер событий для сервера
			struct bufferevent * bev = nullptr;
		public:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект фреймворка
			const Core * core = nullptr;
			// Создаём объект данных вебсокета
			const char * buffer = nullptr;
		public:
			// Функция обратного вызова при открытии подключения
			function <void (const size_t, Core *, void *)> openFn = nullptr;
			// Функция обратного вызова при закрытии подключения
			function <void (const size_t, Core *, void *)> closeFn = nullptr;
			// Функция обратного вызова при запуске подключения
			function <void (const size_t, Core *, void *)> startFn = nullptr;
			// Функция обратного вызова при открытии подключения к прокси-серверу
			function <void (const size_t, Core *, void *)> openProxyFn = nullptr;
			// Функция обратного вызова при получении данных
			function <void (const char *, const size_t, const size_t, Core *, void *)> readFn = nullptr;
			// Функция обратного вызова при записи данных
			function <void (const char *, const size_t, const size_t, Core *, void *)> writeFn = nullptr;
			// Функция обратного вызова при получении данных с прокси-сервера
			function <void (const char *, const size_t, const size_t, Core *, void *)> readProxyFn = nullptr;
			// Функция обратного вызова при записи данных с прокси-сервера
			function <void (const char *, const size_t, const size_t, Core *, void *)> writeProxyFn = nullptr;
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
			 * Worker Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Worker(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Worker Деструктор
			 */
			~Worker() noexcept;
	} worker_t;
};

#endif // __AWH_WORKER__
