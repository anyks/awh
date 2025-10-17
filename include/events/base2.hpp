/**
 * @file: base.hpp
 * @date: 2025-10-17
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
 * Стандартные модули
 */
#include <mutex>
#include <string>
#include <atomic>
#include <vector>
#include <cstdint>
#include <functional>
#include <unordered_map>

/**
 * Наши модули
 */
#include "reactor.hpp"

/**
 * @brief основное пространство имён
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
	typedef class AWH_SHARED_EXPORT Base {
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
				NONE     = 0x00, // Тип активного события не установлено
				CLOSE    = 0x01, // Событие закрытия подключения
				READ     = 0x02, // Событие доступности данных на чтение
				WRITE    = 0x03, // Событие доступности сокета на запись
				TIMER    = 0x04, // Событие таймера в миллисекундах
				INTERVAL = 0x05  // Событие интервала в миллисекундах
			};
		public:
			/**
			 * Тип функции обратного вызова при получении событий сокета
			 */
			using callback_t = function <void (const SOCKET, const event_type_t)>;
		private:
			/**
			 * @brief Структура участника
			 *
			 */
			typedef struct Peer {
				// Отслеживаемый сокет
				SOCKET sock;
				// Задержка времени таймера
				uint32_t delay;
				// Активные события
				uint8_t events;
				// Функция обратного вызова
				const callback_t & callback;
				/**
				 * @brief Конструктор
				 *
				 * @param fn функция обратного вызова
				 */
				Peer(const callback_t & fn) noexcept :
				 sock(INVALID_SOCKET), delay(0),
				 events(react_t::AWH_NONE), callback(fn) {}
			} peer_t;
		private:
			// Объект реактора Event loop
			react_t _react;
		private:
			// Время блокировки базы событий в ожидании событий
			std::atomic_int _rate;
			// Флаг запуска работы базы событий
			std::atomic_bool _works;
			// Флаг простого чтения базы событий
			std::atomic_bool _easily;
			// Флаг запущенного опроса базы событий
			std::atomic_bool _launched;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Список активных событий событий
			vector <react_t::poller_t> _pollers;
		private:
			// Список отслеживаемых участников
			std::unordered_map <uint32_t, peer_t> _peers;
			// Спиоск активных верхнеуровневых потоков
			std::unordered_map <uint32_t, function <void (const uint32_t)>> _events;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод проверки запущена ли в данный момент база событий
			 *
			 * @return результат проверки запущена ли база событий
			 */
			bool launched() const noexcept;
		public:
			/**
			 * @brief Метод удаления события из базы событий
			 *
			 * @param id идентификатор события
			 * @return   результат работы функции
			 */
			bool erase(const uint32_t id) noexcept;
		private:
			/**
			 * @brief Метод установки режима работы сокета
			 *
			 * @param id   идентификатор события
			 * @param type тип события для установленного сокета
			 * @param mode флаг режима работы события
			 * @return     результат установки события
			 */
			bool mode(const uint32_t id, const event_type_t type, const event_mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод добавления сокета в базу событий
			 *
			 * @param sock     сокет для добавления
			 * @param callback функция обратного вызова при получении события
			 * @param delay    задержка времени таймера
			 * @return         идентификатор добавленного события
			 */
			uint32_t emplace(const SOCKET sock, const callback_t & callback, const uint32_t delay = 0) noexcept;
		public:
			/**
			 * @brief Метод очистки списка событий
			 *
			 */
			void clear() noexcept;
		public:
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
		public:
			/**
			 * @brief Метод пересоздания базы событий
			 *
			 */
			void rebase() noexcept;
		public:
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
			 * @brief Метод отправки события через потоки
			 *
			 * @param id  идентификатор события для отправки
			 * @param tid идентификатор трансферной передачи
			 * @return    результат отправки события
			 */
			bool trigger(const uint32_t id, const uint32_t tid) noexcept;
		public:
			/**
			 * @brief Метод отмены регистрации события
			 *
			 * @param id идентификатор события
			 * @return   результат отмены регистрации события
			 */
			bool detach(const uint32_t id) noexcept;
			/**
			 * @brief Метод регистрации нового события
			 *
			 * @param callback функция обратного вызова
			 * @return         идентификатор события
			 */
			uint32_t attach(function <void (const uint32_t)> callback) noexcept;
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
