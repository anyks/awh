/**
 * @file: reactor.hpp
 * @date: 2025-10-14
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

#ifndef __AWH_EVENT_REACTOR__
#define __AWH_EVENT_REACTOR__

/**
 * Стандартные модули
 */
#include <cstdint>

/**
 * Наши модули
 */
#include "watch.hpp"
#include "../net/socket.hpp"

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
	 * @brief Класс реактора для модели Event Loop
	 *
	 */
	typedef class AWH_SHARED_EXPORT Reactor {
		public:
			/**
			 * Флаг событие не инициализированно
			 */
			static constexpr uint8_t AWH_NONE = 0x00;
			/**
			 * Флаг готовности сокета на чтение данных
			 */
			static constexpr uint8_t AWH_READ = 0x01;
			/**
			 * Флаг готовности сокета на запись данных
			 */
			static constexpr uint8_t AWH_WRITE = 0x02;
			/**
			 * Флаг события ошибки в сокете
			 */
			static constexpr uint8_t AWH_ERROR = 0x04;
			/**
			 * Флаг события закрытия сокета
			 */
			static constexpr uint8_t AWH_CLOSE = 0x08;
			/**
			 * Флаг события внутреннего таймера
			 */
			static constexpr uint8_t AWH_TIMER = 0x10;
			/**
			 * Флаг события внутреннего интервала
			 */
			static constexpr uint8_t AWH_INTERVAL = 0x20;
			/**
			 * Флаг события истечения времени ожидания событий
			 */
			static constexpr uint8_t AWH_TIMEOUT = 0x40;
			/**
			 * Флаг события межпотоковой передачи данных
			 */
			static constexpr uint8_t AWH_STREAM = 0x80;
		public:
			/**
			 * @brief Структура полера
			 *
			 */
			typedef struct Poller {
				// Идентификатор сокета
				uint32_t id;
				// События сокета
				uint8_t events;
				/**
				 * @brief Конструктор
				 *
				 */
				Poller() noexcept : id(0), events(0) {}
			} __attribute__((packed)) poller_t;
		private:
			// Объект работы с часами
			watch_t _watch;
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод инициализации событийной модели
			 *
			 * @return результат инициализации
			 */
			bool init() noexcept;
			/**
			 * @brief Метод разрушения событийной модели
			 *
			 * @return результат разрушения
			 */
			bool destroy() noexcept;
		public:
			/**
			 * @brief Метод извлечения входящих уведомлений
			 *
			 * @param id идентификатор события
			 * @return   данные уведомления
			 */
			uint32_t notifications(const uint32_t id) noexcept;
		public:
			/**
			 * @brief Метод отправки уведомления о событии
			 *
			 * @param id   идентификатор события
			 * @param data данные уведомления для отправки
			 * @return     результат выполнения уведомления
			 */
			bool notify(const uint32_t id, const uint32_t data) noexcept;
		public:
			/**
			 * @brief Метод ожидания получения событий
			 *
			 * @param pollers список сработавших событий
			 * @param msec    время ожидания события в миллисекундах
			 * @return        количество полученных событий
			 */
			uint32_t wait(poller_t * pollers, const int32_t msec = -1) noexcept;
		public:
			/**
			 * @brief Метод удаления событий
			 *
			 * @param id   идентификатор события
			 * @param sock сетевой сокет для удаления
			 * @return     результат удаления
			 */
			bool del(const uint32_t id, const SOCKET sock = INVALID_SOCKET) noexcept;
		public:
			/**
			 * @brief Метод модификации события сокета
			 *
			 * @param id     идентификатор события
			 * @param sock   сетевой сокет для модификации
			 * @param events модифицированные типы событий
			 * @return       результат модификации
			 */
			bool modify(const uint32_t id, const SOCKET sock, const uint8_t events) noexcept;
		public:
			/**
			 * @brief Метод добавления несетевых событий
			 *
			 * @param id     идентификатор несетевого события
			 * @param events поддерживаемые типы событий
			 * @param msec   время ожидания срабатывания в миллисекундах
			 * @return       результат добавления
			 */
			bool add(const uint32_t id, const uint8_t events, const uint32_t msec = 0) noexcept;
			/**
			 * @brief Метод добавления сетевого события
			 *
			 * @param id     идентификатор сетевого события
			 * @param sock   сетевой сокет для добавления
			 * @param events поддерживаемые типы событий
			 * @return       результат добавления
			 */
			bool add(const uint32_t id, const SOCKET sock, const uint8_t events = AWH_NONE) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Reactor(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Reactor() noexcept;
	} react_t;
};

#endif // __AWH_EVENT_REACTOR__
