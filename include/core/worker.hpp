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
#include <map>
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
		private:
			// Core Устанавливаем дружбу с классом ядра
			friend class Core;
			// CoreClient Устанавливаем дружбу с клиентским классом ядра
			friend class CoreClient;
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
				Mark() : min(0), max(BUFFER_CHUNK) {}
			} __attribute__((packed)) mark_t;
		public:
			/**
			 * Adjutant Структура адъютанта
			 */
			typedef struct Adjutant {
				private:
					// Core Устанавливаем дружбу с классом ядра
					friend class Core;
					// Worker Устанавливаем дружбу с родительским объектом
					friend class Worker;
					// CoreClient Устанавливаем дружбу с клиентским классом ядра
					friend class CoreClient;
				private:
					// Маркера размера детектируемых байт на чтение
					mark_t markRead;
					// Маркера размера детектируемых байт на запись
					mark_t markWrite;
				private:
					// Идентификатор адъютанта
					size_t aid = 0;
					// Текущее количество попыток
					u_short attempt = 0;
				private:
					// Таймер на чтение в секундах
					time_t timeRead = READ_TIMEOUT;
					// Таймер на запись в секундах
					time_t timeWrite = WRITE_TIMEOUT;
				private:
					// Объект буфера данныхs
					char buffer[BUFFER_CHUNK];
				private:
					// Объект буфера событий
					struct bufferevent * bev = nullptr;
				public:
					// Создаём объект фреймворка
					const fmk_t * fmk = nullptr;
					// Создаём объект работы с логами
					const log_t * log = nullptr;
					// Объект родительского воркера
					const Worker * parent = nullptr;
				public:
					/**
					 * Adjutant Конструктор
					 * @param parent объект родительского воркера
					 * @param fmk    объект фреймворка
					 * @param log    объект для работы с логами
					 */
					Adjutant(const Worker * parent, const fmk_t * fmk, const log_t * log) noexcept : parent(parent), fmk(fmk), log(log) {}
					/**
					 * ~Adjutant Деструктор
					 */
					~Adjutant() noexcept {}
			} adj_t;
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
			// Общее количество попыток
			u_short attempts = 0;
		public:
			// Таймер на чтение в секундах
			time_t timeRead = READ_TIMEOUT;
			// Таймер на запись в секундах
			time_t timeWrite = WRITE_TIMEOUT;
		protected:
			// Список подключённых адъютантов
			map <size_t, adj_t> adjutants;
		public:
			// Контекст передаваемый в сообщении
			void * ctx = nullptr;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект фреймворка
			const Core * core = nullptr;
		public:
			// Функция обратного вызова при открытии подключения
			function <void (const size_t, Core *, void *)> openFn = nullptr;
			// Функция обратного вызова при закрытии подключения
			function <void (const size_t, Core *, void *)> closeFn = nullptr;
			// Функция обратного вызова при запуске подключения
			function <void (const size_t, Core *, void *)> connectFn = nullptr;
			// Функция обратного вызова при получении данных
			function <void (const char *, const size_t, const size_t, Core *, void *)> readFn = nullptr;
			// Функция обратного вызова при записи данных
			function <void (const char *, const size_t, const size_t, Core *, void *)> writeFn = nullptr;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept {
				// Выполняем очистку списка адъютантов
				this->adjutants.clear();
			}
		public:
			/**
			 * Worker Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Worker(const fmk_t * fmk, const log_t * log) noexcept : fmk(fmk), log(log) {}
			/**
			 * ~Worker Деструктор
			 */
			virtual ~Worker() noexcept {}
	} worker_t;
};

#endif // __AWH_WORKER__
