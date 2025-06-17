/**
 * @file: timer.hpp
 * @date: 2024-07-15
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

#ifndef __AWH_CORE_TIMER__
#define __AWH_CORE_TIMER__

/**
 * Наши модули
 */
#include <core/core.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Timer Класс таймера ядра биндинга
	 */
	typedef class AWHSHARED_EXPORT Timer : public awh::core_t {
		private:
			/**
			 * Broker Класс брокера
			 */
			typedef class Broker {
				public:
					// Флаг персистентной работы
					bool persist;
				public:
					// Объект события таймера
					event_t event;
				public:
					/**
					 * Broker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const fmk_t * fmk, const log_t * log) noexcept :
					 persist(false), event(event_t::type_t::TIMER, fmk, log) {}
					/**
					 * ~Broker Деструктор
					 */
					~Broker() noexcept {}
			} broker_t;
		private:
			// Мютекс для блокировки основного потока
			recursive_mutex _mtx;
		private:
			// Список активных брокеров
			map <uint16_t, unique_ptr <broker_t>> _brokers;
			// Хранилище функций обратного вызова
			map <uint16_t, function <void (void)>> _callbacks;
		private:
			/**
			 * launching Метод вызова при активации базы событий
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			void launching(const bool mode, const bool status) noexcept;
			/**
			 * closedown Метод вызова при деакцтивации базы событий
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			void closedown(const bool mode, const bool status) noexcept;
		private:
			/**
			 * event Метод события таймера
			 * @param tid   идентификатор таймера
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 */
			void event(const uint16_t tid, const SOCKET fd, const base_t::event_type_t event) noexcept;
		public:
			/**
			 * clear Метод очистки всех таймеров
			 */
			void clear() noexcept;
			/**
			 * clear Метод очистки таймера
			 * @param tid идентификатор таймера для очистки
			 */
			void clear(const uint16_t tid) noexcept;
		public:
			/**
			 * attach Шаблон метода прикрепления финкции обратного вызова
			 */
			template <class... Args>
			/**
			 * attach Метод прикрепления финкции обратного вызова
			 * @param tid  идентификатор таймера
			 * @param args аргументы функции обратного вызова
			 */
			void attach(const uint16_t tid, Args... args) noexcept {
				// Если идентификатор таймера передан
				if(tid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск активного таймера
						auto i = this->_brokers.find(tid);
						// Если активный таймер найден
						if(i != this->_brokers.end()){
							// Выполняем блокировку потока
							const lock_guard <recursive_mutex> lock(this->_mtx);
							// Выполняем поиск функции обратного вызова
							auto j = this->_callbacks.find(tid);
							// Если функция найдена в списке
							if(j != this->_callbacks.end())
								// Выполняем замену функции обратного вызова
								j->second = std::bind(args...);
							// Выполняем установку функции обратного вызова
							else this->_callbacks.emplace(tid, std::bind(args...));
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(tid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		public:
			/**
			 * timeout Метод создания таймаута
			 * @param delay задержка времени в миллисекундах
			 * @return      идентификатор таймера
			 */
			uint16_t timeout(const uint32_t delay) noexcept;
			/**
			 * interval Метод создания интервала
			 * @param delay задержка времени в миллисекундах
			 * @return      идентификатор таймера
			 */
			uint16_t interval(const uint32_t delay) noexcept;
		public:
			/**
			 * Timer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Timer(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log) {}
			/**
			 * ~Timer Деструктор
			 */
			~Timer() noexcept;
	} timer_t;
};

#endif // __AWH_CORE_TIMER__
