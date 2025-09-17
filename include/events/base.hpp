/**
 * @file: base.hpp
 * @date: 2024-06-26
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

#ifndef __AWH_EVENT_BASE__
#define __AWH_EVENT_BASE__

/**
 * Для операционной системы Linux
 */
#if __linux__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/epoll.h>
/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/devpoll.h>
/**
 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/event.h>
#endif

/**
 * Стандартные модули
 */
#include <map>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <functional>

/**
 * Наши модули
 */
#include "watch.hpp"
#include "partners.hpp"
#include "../net/socket.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип класса события AWH event
	 *
	 */
	class Event;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс базы событий
	 *
	 */
	typedef class AWHSHARED_EXPORT Base {
		private:
			/**
			 * @brief Устанавливаем дружбу с модулем события
			 *
			 */
			friend class Event;
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
				NONE   = 0x00, // Тип активного события не установлено
				CLOSE  = 0x01, // Событие закрытия подключения
				READ   = 0x02, // Событие доступности данных на чтение
				WRITE  = 0x03, // Событие доступности сокета на запись
				TIMER  = 0x04, // Событие таймера в миллисекундах
				STREAM = 0x05  // Событие межпотоковое системное
			};
		private:
			/**
			 * Максимальное количество отслеживаемых сокетов
			 */
			static constexpr const uint32_t MAX_SOCKS = 0x20000;
		public:
			/**
			 * Создаём тип функции обратного вызова
			 */
			typedef function <void (const SOCKET, const event_type_t)> callback_t;
		private:
			/**
			 * @brief Структура работы вышестоящего потока
			 *
			 */
			typedef struct Upstream {
				// Дополнительный партнёрский сокет
				SOCKET sock;
				// Мютекс для блокировки потока
				std::mutex mtx;
				// Объект работы с уведомителем
				notifier_t notifier;
				// Функция обратного вызова
				function <void (const uint64_t)> callback;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Upstream(const fmk_t * fmk, const log_t * log) noexcept :
				 sock(INVALID_SOCKET), notifier(fmk, log), callback(nullptr) {}
			} upstream_t;
			/**
			 * @brief Структура участника
			 *
			 */
			typedef struct Peer {
				// Идентификатор участника
				uint64_t id;
				// Флаг персистентного таймера
				bool persist;
				// Отслеживаемый сокет
				SOCKET socks[2];
				// Задержка времени таймера
				uint32_t delay;
				// Тип участника по умолчанию
				event_type_t type;
				// Функция обратного вызова
				callback_t callback;
				// Список соответствия типов событий режиму работы
				std::map <event_type_t, event_mode_t> mode;
				/**
				 * @brief Конструктор
				 *
				 */
				Peer() noexcept :
				 id(0), persist(false),
				 socks{INVALID_SOCKET, INVALID_SOCKET},
				 delay(0), type(event_type_t::NONE), callback(nullptr) {}
			} peer_t;
		private:
			// Идентификатор потока
			uint64_t _wid;
		private:
			// Время блокировки базы событий в ожидании событий
			std::atomic_int _rate;
			// Максимальное количество обрабатываемых сокетов
			std::atomic_uint _sockmax;
		private:
			// Флаг запуска работы базы событий
			std::atomic_bool _works;
			// Флаг простого чтения базы событий
			std::atomic_bool _easily;
			// Флаг блокировки опроса базы событий
			std::atomic_bool _locker;
			// Флаг запущенного опроса базы событий
			std::atomic_bool _launched;
		private:
			/**
			 * Для операционной системы OS Windows
			 */
			#if _WIN32 || _WIN64
				// Объект данных запроса
				WSADATA _wsaData;
				// Флаг инициализации WinSocksAPI
				bool _winSockInit;
				// Список активных файловых дескрипторов
				vector <WSAPOLLFD> _fds;
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Идентификатор активного /dev/poll
				int32_t _wfd;
				// Список активных событий
				struct dvpoll _dopoll;
				// Список активных файловых дескрипторов
				vector <struct pollfd> _fds;
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Идентификатор активного EPoll
				SOCKET _efd;
				// Список активных изменений событий
				vector <struct epoll_event> _change;
				// Список активных событий
				vector <struct epoll_event> _events;
			/**
			 * Для операционной системы MacOS Xб FreeBSD, NetBSD или OpenBSD
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Идентификатор активного kqueue
				SOCKET _kq;
				// Список активных изменений событий
				vector <struct kevent> _change;
				// Список активных событий
				vector <struct kevent> _events;
			#endif
		private:
			// Объект работы с часами
			watch_t _watch;
			// Объект работы с партнёрами
			partners_t _partners;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Список отслеживаемых участников
			std::map <SOCKET, peer_t> _peers;
			// Спиоск активных верхнеуровневых потоков
			std::map <SOCKET, std::unique_ptr <upstream_t>> _upstream;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод получения идентификатора потока
			 *
			 * @return идентификатор потока для получения
			 */
			uint64_t wid() const noexcept;
		private:
			/**
			 * @brief Метод проверки запущен ли модуль в дочернем потоке
			 *
			 * @return результат проверки
			 */
			bool isChildThread() const noexcept;
		private:
			/**
			 * @brief Метод инициализации базы событий
			 *
			 * @param mode флаг инициализации
			 */
			void init(const event_mode_t mode) noexcept;
		private:
			/**
			 * @brief Метод получения событий верхнеуровневых потоков
			 *
			 * @param sock  окет межпотокового передатчика
			 * @param event входящее событие от межпотокового передатчика
			 */
			void stream(const SOCKET sock, const uint64_t event) noexcept;
		private:
			/**
			 * @brief Метод удаления файлового дескриптора из базы событий
			 *
			 * @param sock сокет для удаления
			 * @return     результат работы функции
			 */
			bool del(const SOCKET sock) noexcept;
			/**
			 * @brief Метод удаления файлового дескриптора из базы событий для всех событий
			 *
			 * @param id   идентификатор записи
			 * @param sock сокет для удаления
			 * @return     результат работы функции
			 */
			bool del(const uint64_t id, const SOCKET sock) noexcept;
			/**
			 * @brief Метод удаления файлового дескриптора из базы событий для указанного события
			 *
			 * @param id   идентификатор записи
			 * @param sock сокет для удаления
			 * @param type тип отслеживаемого события
			 * @return     результат работы функции
			 */
			bool del(const uint64_t id, const SOCKET sock, const event_type_t type) noexcept;
		private:
			/**
			 * @brief Метод добавления файлового дескриптора в базу событий
			 *
			 * @param id       идентификатор записи
			 * @param sock     сокет для добавления
			 * @param callback функция обратного вызова при получении события
			 * @param delay    задержка времени для создания таймеров
			 * @param persist  флаг персистентного таймера
			 * @return         результат работы функции
			 */
			bool add(const uint64_t id, SOCKET & sock, callback_t callback = nullptr, const uint32_t delay = 0, const bool persist = false) noexcept;
		private:
			/**
			 * @brief Метод установки режима работы модуля
			 *
			 * @param id   идентификатор записи
			 * @param sock сокет для установки режима работы
			 * @param type тип событий модуля для которого требуется сменить режим работы
			 * @param mode флаг режима работы модуля
			 * @return     результат работы функции
			 */
			bool mode(const uint64_t id, const SOCKET sock, const event_type_t type, const event_mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод проверки запущена ли в данный момент база событий
			 *
			 * @return результат проверки запущена ли база событий
			 */
			bool launched() const noexcept;
		public:
			/**
			 * @brief Метод очистки списка событий
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод отправки пинка
			 *
			 */
			void kick() noexcept;
			/**
			 * @brief Метод остановки чтения базы событий
			 *
			 */
			void stop() noexcept;
			/**
			 * @brief Метод запуска чтения базы событий
			 *
			 */
			void start() noexcept;
			/**
			 * @brief Метод пересоздания базы событий
			 *
			 */
			void rebase() noexcept;
		public:
			/**
			 * @brief Метод заморозки чтения данных
			 *
			 * @param mode флаг активации
			 */
			void freeze(const bool mode) noexcept;
			/**
			 * @brief Метод активации простого режима чтения базы событий
			 *
			 * @param mode флаг активации
			 */
			void easily(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки времени блокировки базы событий в ожидании событий
			 *
			 * @param msec время ожидания событий в миллисекундах
			 */
			void rate(const uint32_t msec = 10) noexcept;
		public:
			/**
			 * @brief Максимальное количество поддерживаемых сокетов
			 *
			 * @param count максимальное количество поддерживаемых сокетов
			 */
			void sockmax(const uint32_t count) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения между потоками
			 *
			 * @param sock сокет межпотокового передатчика
			 * @param tid  идентификатор трансферной передачи
			 */
			void upstream(const SOCKET sock, const uint64_t tid) noexcept;
			/**
			 * @brief Метод деактивации межпотокового передатчика
			 *
			 * @param sock сокет межпотокового передатчика
			 */
			void deactivationUpstream(const SOCKET sock) noexcept;
			/**
			 * @brief Метод активации межпотокового передатчика
			 *
			 * @param callback функция обратного вызова
			 * @return         сокет межпотокового передатчика
			 */
			SOCKET activationUpstream(function <void (const uint64_t)> callback) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Base(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Base() noexcept;
	} base_t;
};

#endif // __AWH_EVENT_BASE__
