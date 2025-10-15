/**
 * @file: poll.hpp
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

#ifndef __AWH_EVENT_POLL_BASE__
#define __AWH_EVENT_POLL_BASE__

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
	 * @brief Класс движка Events Loop
	 *
	 */
	typedef class AWH_SHARED_EXPORT Poll {
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
			 * @brief Структура события
			 *
			 */
			typedef struct Event {
				// Идентификатор сокета
				uint32_t id;
				// События сокета
				uint8_t events;
				/**
				 * @brief Конструктор
				 * 
				 */
				Event() noexcept : id(0), events(0) {}
			} __attribute__((packed)) event_t;
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
			 * @brief Метод удаления событий
			 *
			 * @param sock сетевой сокет для удаления
			 * @param id   идентификатор события
			 * @return     результат удаления
			 */
			bool del(const SOCKET sock, const uint32_t id) noexcept;
		public:
			/**
			 * @brief Метод добавления несетевых событий
			 *
			 * @param id     идентификатор несетевого события
			 * @param msec   время ожидания срабатывания в миллисекундах
			 * @param events поддерживаемые типы событий
			 * @return       результат добавления
			 */
			bool add(const uint32_t id, const uint32_t msec, const uint8_t events) noexcept;
			/**
			 * @brief Метод добавления сетевого события
			 *
			 * @param sock   сетевой сокет для добавления
			 * @param id     идентификатор сетевого события
			 * @param events поддерживаемые типы событий
			 * @return       результат добавления
			 */
			bool add(const SOCKET sock, const uint32_t id, const uint8_t events) noexcept;
		public:
			/**
			 * @brief Метод ожидания получения событий
			 *
			 * @param events список сработавших событий
			 * @param max    максимальное количество ожидаемых событий
			 * @param msec   время ожидания события в миллисекундах
			 * @return       количество полученных событий
			 */
			uint32_t wait(event_t * events, const uint16_t max, const int32_t msec) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Poll(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Poll() noexcept;
	} poll_t;
};

#endif // __AWH_EVENT_POLL_BASE__
