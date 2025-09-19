/**
 * @file: watch.hpp
 * @date: 2025-09-16
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

#ifndef __AWH_EVENT_WATCH__
#define __AWH_EVENT_WATCH__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <mutex>

/**
 * Наши модули
 */
#include "notifier.hpp"
#include "../sys/screen.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы с часами
	 *
	 */
	typedef class AWHSHARED_EXPORT Watch {
		private:
			/**
			 * @brief структура участника обмена данными
			 *
			 */
			typedef struct Unit {
				// Файловый дескрипторв (сокет)
				SOCKET sock;
				// Время задержки работы таймера
				uint64_t delay;
				/**
				 * @brief Конструктор
				 *
				 */
				Unit() noexcept : sock(INVALID_SOCKET), delay(0) {}
			} __attribute__((packed)) unit_t;
		private:
			// Мютекс для блокировки потока
			std::mutex _mtx;
		private:
			// Объект экрана для работы в дочернем потоке
			screen_t <unit_t> _screen;
		private:
			// Список существующих уведомителей
			std::map <SOCKET, std::unique_ptr <notifier_t>> _notifiers;
			// Список активных таймеров
			std::multimap <std::pair <uint64_t, uint64_t>, SOCKET> _timers;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод обработки событий триггера
			 *
			 */
			void trigger() noexcept;
			/**
			 * @brief Метод обработки процесса добавления таймеров
			 *
			 * @param unit параметры участника
			 */
			void process(const unit_t unit) noexcept;
		public:
			/**
			 * @brief Метод остановки работы таймера
			 *
			 */
			void stop() noexcept;
			/**
			 * @brief Метод запуска работы таймера
			 *
			 */
			void start() noexcept;
		public:
			/**
			 * @brief Метод создания нового таймера
			 *
			 * @return файловый дескриптор для отслеживания
			 */
			SOCKET create() noexcept;
		public:
			/**
			 * @brief Метод извлечения идентификатора события
			 *
			 * @param sock файловый дескриптор таймера
			 * @return     идентификатор события
			 */
			uint64_t event(const SOCKET sock) noexcept;
		public:
			/**
			 * @brief Метод убрать таймер из отслеживания
			 *
			 * @param sock файловый дескриптор таймера
			 */
			void away(const SOCKET sock) noexcept;
			/**
			 * @brief Метод ожидания указанного промежутка времени
			 *
			 * @param sock  файловый дескриптор таймера
			 * @param delay задержка времени в миллисекундах
			 */
			void wait(const SOCKET sock, const uint32_t delay) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Watch(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Watch() noexcept;
	} watch_t;
};

#endif // __AWH_EVENT_WATCH__
