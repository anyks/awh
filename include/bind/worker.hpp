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
#include <uri.hpp>
#include <auth.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Bind Прототип класса биндинга TCP/IP
	 */
	class Bind;
	/**
	 * Proxy структура прокси-сервера
	 */
	typedef struct Proxy {
		public:
			/**
			 * Типы прокси-сервера
			 */
			enum class type_t : u_short {NONE, HTTP, SOCKS};
		public:
			// Тип прокси-сервера
			type_t type;
			// Параметры сокет-сервера
			uri_t::url_t url;
			// Тип авторизации
			auth_t::type_t auth;
			// Алгоритм шифрования для Digest авторизации
			auth_t::algorithm_t algorithm;
		public:
			/**
			 * Proxy Конструктор
			 */
			Proxy() noexcept : type(type_t::NONE), auth(auth_t::type_t::BASIC), algorithm(auth_t::algorithm_t::MD5) {}
	} proxy_t;
	/**
	 * Worker Структура воркера
	 */
	typedef struct Worker {
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
		public:
			// Количество попыток реконнекта
			pair <size_t, size_t> attempts;
		public:
			// Буфер событий для сервера
			struct bufferevent * bev = nullptr;
		public:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект фреймворка
			const Bind * bind = nullptr;
			// Создаём объект данных вебсокета
			const char * buffer = nullptr;
		public:
			// Функция обратного вызова при открытии подключения
			function <void (const size_t, Bind *)> openFn = nullptr;
			// Функция обратного вызова при закрытии подключения
			function <void (const size_t, Bind *)> closeFn = nullptr;
			// Функция обратного вызова при запуске подключения
			function <void (const size_t, Bind *)> startFn = nullptr;
			// Функция обратного вызова при получении данных
			function <void (const char *, const size_t, const size_t, Bind *)> readFn = nullptr;
			// Функция обратного вызова при записи данных
			function <void (const char *, const size_t, const size_t, Bind *)> writeFn = nullptr;
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
