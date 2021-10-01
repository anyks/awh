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
	 * Worker Структура воркера
	 */
	typedef struct Worker {
		public:
			/**
			 * Mark Структура маркера на размер детектируемых байт
			 */
			typedef struct Mark {
				size_t min; // Минимальный размер детектируемых байт
				size_t max; // Максимальный размер детектируемых байт
				/**
				 * Mark Конструктор
				 */
				Mark() : min(0), max(0) {}
			} __attribute__((packed)) mark_t;
		public:
			// Параметры адреса для запроса
			uri_t::url_t url;
			// Контекст SSL для работы с защищённым подключением
			ssl_t::ctx_t ssl;
		public:
			// Маркера размера детектируемых байт на чтение
			mark_t markRead;
			// Маркера размера детектируемых байт на запись
			mark_t markWrite;
		public:
			// Идентификатор воркера
			size_t wid = 0;
		public:
			// Флаг ожидания входящих сообщений
			bool wait = false;
			// Флаг автоматического поддержания подключения
			bool alive = false;
		public:
			// Таймер на чтение в секундах
			time_t timeRead = READ_TIMEOUT;
			// Таймер на запись в секундах
			time_t timeWrite = WRITE_TIMEOUT;
		public:
			// Сокет сервера для подключения
			evutil_socket_t fd = -1;
		public:
			// Количество попыток реконнекта
			pair <u_short, u_short> attempts;
		public:
			// Контекст передаваемый в сообщении
			void * ctx = nullptr;
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
			// Функция обратного вызова при запуске подключения
			function <void (const size_t, Core *, void *)> runFn = nullptr;
			// Функция обратного вызова при открытии подключения
			function <void (const size_t, Core *, void *)> openFn = nullptr;
			// Функция обратного вызова при закрытии подключения
			function <void (const size_t, Core *, void *)> closeFn = nullptr;
			// Функция обратного вызова при получении данных
			function <void (const char *, const size_t, const size_t, Core *, void *)> readFn = nullptr;
			// Функция обратного вызова при записи данных
			function <void (const char *, const size_t, const size_t, Core *, void *)> writeFn = nullptr;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept;
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
			virtual ~Worker() noexcept;
	} worker_t;
};

#endif // __AWH_WORKER__
