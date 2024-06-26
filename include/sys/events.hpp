/**
 * @file: events.hpp
 * @date: 2024-06-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_EVENTS__
#define __AWH_EVENTS__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <functional>

/**
 * Если это Linux
 */
#if __linux__
	// Подключаем модуль EPoll
	#include <sys/epoll.h>
/**
 * Если это FreeBSD или MacOS X
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__
	// Подключаем модуль Kqueue
	#include <sys/event.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/socket.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Base Класс базы событий
	 */
	typedef class Base {
		public:
			/**
			 * Тип режима получения события
			 */
			enum class event_mode_t : uint8_t {
				ENABLED  = 0x01, // Разрешено получение события
				DISABLED = 0x00  // Запрещено получение события 
			};
			/**
			 * Тип активного события
			 */
			enum class event_type_t : uint8_t {
				NONE  = 0x00, // Тип активного события не установлено
				CLOSE = 0x01, // Активное событие закрытия подключения
				READ  = 0x02, // Активное событие доступности данных на чтение
				WRITE = 0x03  // Активное событие доступности сокета на запись
			};
		private:
			// Максимальное количество отслеживаемых сокетов
			static constexpr const uint32_t MAX_COUNT_FDS = 20480;
		public:
			/**
			 * Создаём тип функции обратного вызова
			 */
			typedef std::function <void (const SOCKET, const event_type_t)> callback_t;
		private:
			/**
			 * Item Структура данных участника
			 */
			typedef struct Item {
				// Отслеживаемый файловый дескриптор
				SOCKET fd;
				// Функция обратного вызова
				callback_t callback;
				/**
				 * Если это FreeBSD или MacOS X
				 */
				#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__) || defined(_WIN32) || defined(_WIN64)
					// Список индексов типов события
					std::map <event_type_t, size_t> indexes;
				#endif
				// Список соответствия типов событий режиму работы
				std::map <event_type_t, event_mode_t> mode;
				/**
				 * Item Конструктор
				 */
				Item() noexcept : fd(-1), callback(nullptr) {}
			} item_t;
		private:
			// Флаг запуска работы базы событий
			bool _mode;
			// Флаг простого чтения базы событий
			bool _easily;
			// Флаг блокировки опроса базы событий
			bool _locker;
			// Флаг запущенного опроса базы событий
			bool _launched;
		private:
			// Таймаут времени блокировки базы событий
			int32_t _timeout;
		private:
			// Максимальное количество обрабатываемых сокетов
			uint32_t _maxCount;
		private:
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Объект данных запроса
				WSADATA _wsaData;
				// Флаг инициализации WinSocksAPI
				bool _winSockInit;
				// Список активных файловых дескрипторов
				std::vector <WSAPOLLFD> _fds;
			/**
			 * Если это Linux
			 */
			#elif __linux__
				// Идентификатор активного EPoll
				int _efd;
				// Список активных изменений событий
				std::vector <struct epoll_event> _change;
				// Список активных событий
				std::vector <struct epoll_event> _events;
			/**
			 * Если это FreeBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__
				// Идентификатор активного kqueue
				int _kq;
				// Список активных изменений событий
				std::vector <struct kevent> _change;
				// Список активных событий
				std::vector <struct kevent> _events;
			#endif
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Список отслеживаемых участников
			std::map <SOCKET, item_t> _items;
			// Список индексов файловых дескрипторов
			std::multimap <SOCKET, size_t> _indexes;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * init Метод инициализации базы событий
			 * @param mode флаг инициализации
			 */
			void init(const bool mode) noexcept;
		private:
			/**
			 * del Метод удаления файлового дескриптора из базы событий для всех событий
			 * @param fd файловый дескриптор для удаления
			 * @return   результат работы функции
			 */
			bool del(const SOCKET fd) noexcept;
			/**
			 * del Метод удаления файлового дескриптора из базы событий для указанного события
			 * @param fd   файловый дескриптор для удаления
			 * @param type тип отслеживаемого события
			 * @return     результат работы функции
			 */
			bool del(const SOCKET fd, const event_type_t type) noexcept;
			/**
			 * add Метод добавления файлового дескриптора в базу событий
			 * @param fd       файловый дескриптор для добавления
			 * @param type     тип отслеживаемого события
			 * @param callback функция обратного вызова при получении события
			 * @return         результат работы функции
			 */
			bool add(const SOCKET fd, const event_type_t type, callback_t callback = nullptr) noexcept;
		private:
			/**
			 * mode Метод установки режима работы модуля
			 * @param fd   файловый дескриптор для установки режима работы
			 * @param type тип событий модуля для которого требуется сменить режим работы
			 * @param mode флаг режима работы модуля
			 * @return     результат работы функции
			 */
			bool mode(const SOCKET fd, const event_type_t type, const event_mode_t mode) noexcept;
		public:
			/**
			 * launched Метод проверки запущена ли в данный момент база событий
			 * @return результат проверки запущена ли база событий
			 */
			bool launched() const noexcept;
		public:
			/**
			 * clear Метод очистки списка событий
			 */
			void clear() noexcept;
		public:
			/**
			 * kick Метод отправки пинка
			 */
			void kick() noexcept;
			/**
			 * stop Метод остановки чтения базы событий
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска чтения базы событий
			 */
			void start() noexcept;
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
			/**
			 * freeze Метод заморозки чтения данных
			 * @param mode флаг активации
			 */
			void freeze(const bool mode) noexcept;
			/**
			 * easily Метод активации простого режима чтения базы событий
			 * @param mode флаг активации
			 */
			void easily(const bool mode) noexcept;
		public:
			/**
			 * frequency Метод установки частоты обновления базы событий
			 * @param msec частота обновления базы событий в миллисекундах
			 */
			void frequency(const uint32_t msec = 10) noexcept;
		public:
			/**
			 * Base Конструктор
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 * @param count максимальное количество обрабатываемых сокетов
			 */
			Base(const fmk_t * fmk, const log_t * log, const uint32_t count = MAX_COUNT_FDS) noexcept;
			/**
			 * ~Base Деструктор
			 */
			~Base() noexcept;
	} base_t;
};

#endif // __AWH_EVENTS__
