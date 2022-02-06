/**
 * @file: core.hpp
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

#ifndef __AWH_WORKER__
#define __AWH_WORKER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <mutex>
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

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Core Прототип класса ядра биндинга TCP/IP
		 */
		class Core;
	};
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Core Прототип класса ядра биндинга TCP/IP
		 */
		class Core;
	};
}

/**
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
			// Client Core Устанавливаем дружбу с клиентским классом ядра
			friend class client::Core;
			// Server Core Устанавливаем дружбу с серверным классом ядра
			friend class server::Core;
		public:
			/**
			 * Mark Структура маркера на размер детектируемых байт
			 */
			typedef struct Mark {
				size_t min; // Минимальный размер детектируемых байт
				size_t max; // Максимальный размер детектируемых байт
				/**
				 * Mark Конструктор
				 * @param min минимальный размер детектируемых байт
				 * @param max максимальный размер детектируемых байт
				 */
				Mark(const size_t min = 0, const size_t max = 0) : min(min), max(max) {}
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
					// Client Core Устанавливаем дружбу с клиентским классом ядра
					friend class client::Core;
					// Server Core Устанавливаем дружбу с серверным классом ядра
					friend class server::Core;
				private:
					/**
					 * Chunks Структура чанков
					 */
					typedef struct Chunks {
						bool end;                         // Флаг завершения обработки данных
						size_t count;                     // Общее количество обработанных чанков
						size_t index;                     // Текущее значение индекса чанка
						map <size_t, vector <char>> data; // Полученные бинарные чанки
						/**
						 * Chunks Конструктор
						 */
						Chunks() : end(false), count(0), index(0) {}
					} chunks_t;
				private:
					// Маркера размера детектируемых байт на чтение
					mark_t markRead;
					// Маркера размера детектируемых байт на запись
					mark_t markWrite;
				private:
					// Контекст SSL для работы с защищённым подключением
					ssl_t::ctx_t ssl;
				private:
					// Мютекс для блокировки потоков
					mutex locker;
					// Объект чанков данных
					chunks_t chunks;
				private:
					// Адрес интернет подключения клиента
					string ip = "";
					// Мак адрес подключившегося клиента
					string mac = "";
				private:
					// Идентификатор адъютанта
					size_t aid = 0;
				private:
					// Таймер на чтение в секундах
					time_t timeRead = READ_TIMEOUT;
					// Таймер на запись в секундах
					time_t timeWrite = WRITE_TIMEOUT;
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
				private:
					/**
					 * end Метод установки флага завершения работы
					 */
					void end() noexcept;
					/**
					 * get Метод получения буфера чанка
					 * @return буфер чанка в бинарном виде 
					 */
					vector <char> get() noexcept;
					/**
					 * add Метод добавления чанка бинарных данных
					 * @param buffer буфер чанка бинарных данных
					 * @param size   размер буфера бинарных данных
					 */
					void add(const char * buffer, const size_t size) noexcept;
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
		private:
			// Результат работы функции
			string result = "";
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
		protected:
			// Список подключённых адъютантов
			map <size_t, unique_ptr <adj_t>> adjutants;
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
			// Функция обратного вызова при открытии приложения
			function <void (const size_t, Core *, void *)> openFn = nullptr;
			// Функция обратного вызова для персистентного вызова
			function <void (const size_t, const size_t, Core *, void *)> persistFn = nullptr;
			// Функция обратного вызова при запуске подключения
			function <void (const size_t, const size_t, Core *, void *)> connectFn = nullptr;
			// Функция обратного вызова при закрытии подключения
			function <void (const size_t, const size_t, Core *, void *)> disconnectFn = nullptr;
			// Функция обратного вызова при получении данных
			function <void (const char *, const size_t, const size_t, const size_t, Core *, void *)> readFn = nullptr;
			// Функция обратного вызова при записи данных
			function <void (const char *, const size_t, const size_t, const size_t, Core *, void *)> writeFn = nullptr;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept;
			/**
			 * ip Метод получения IP адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес интернет подключения адъютанта
			 */
			const string & ip(const size_t aid) const noexcept;
			/**
			 * mac Метод получения MAC адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес устройства адъютанта
			 */
			const string & mac(const size_t aid) const noexcept;
		public:
			/**
			 * Worker Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Worker(const fmk_t * fmk, const log_t * log) noexcept : markRead(0, 0), fmk(fmk), log(log) {}
			/**
			 * ~Worker Деструктор
			 */
			virtual ~Worker() noexcept {}
	} worker_t;
};

#endif // __AWH_WORKER__
