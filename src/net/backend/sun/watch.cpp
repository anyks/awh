/**
 * @file: watch.cpp
 * @date: 2025-10-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля системных часов цикла событий — отсчёт таймеров в отдельном потоке и генерация событий
 *        срабатывания для платформ без нативного механизма таймеров
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <engine/watch.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод обработки событий триггера
 * 
 */
void awh::Watch::trigger() noexcept {
	// Если не производится остановка
	if(this->_working){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку уникальным мютексом
			const locker_t lock(this->_mtx);
			// Получаем текущее значение даты
			const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
			// Если очередь таймеров ожидающих активации не пустая
			if(!this->_items.empty()){
				// Извлекаем таймер ожидающий активации
				const auto & item = this->_items.front();
				/**
				 * Выполняем перебор всего списка таймеров
				 */
				for(auto i = this->_timers.begin(); i != this->_timers.end();){
					// Если мы нашли наш таймер
					if(item.first == i->second)
						// Выполняем удаление значение таймера
						i = this->_timers.erase(i);
					// Если это другие таймеры
					else {
						// Если время вышло
						if(date >= i->first){
							// Выполняем отправку сообщения
							this->_notifier.notify(i->second);
							// Выполняем удаление значение таймера
							i = this->_timers.erase(i);
						// Продолжаем перебор дальше
						} else ++i;
					}
				}
				// Выполняем добавления нового таймера
				this->_timers.emplace(item.second + date, item.first);
				// Удаляем таймер ожидающий активации из очереди
				this->_items.pop();
				// Получаем наименьшее значение даты в списке
				const uint64_t smallest = this->_timers.begin()->first;
				// Если время задержки выше нуля
				if((smallest > date ? (smallest - date) : 0) > 0){
					// Выполняем смену времени таймера
					this->_delay = std::chrono::nanoseconds(smallest - date);
					// Выходим из функции
					return;
				}
			// Если очередь пустая
			} else {
				/**
				 * Выполняем перебор всего списка таймеров
				 */
				for(auto i = this->_timers.begin(); i != this->_timers.end();){
					// Если время вышло
					if(date >= i->first){
						// Выполняем отправку сообщения
						this->_notifier.notify(i->second);
						// Выполняем удаление значение таймера
						i = this->_timers.erase(i);
					// Продолжаем перебор дальше
					} else ++i;
				}
				// Если список таймеров не пустой
				if(!this->_timers.empty()){
					// Получаем наименьшее значение даты в списке
					const uint64_t smallest = this->_timers.begin()->first;
					// Если время задержки выше нуля
					if((smallest > date ? (smallest - date) : 0) > 0){
						// Выполняем смену времени таймера
						this->_delay = std::chrono::nanoseconds(smallest - date);
						// Выходим из функции
						return;
					}
				}
			}
			// Устанавливаем таймаут по умолчанию
			this->_delay = std::chrono::nanoseconds(TIMEOUT);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения данных
 *
 */
void awh::Watch::receiving() noexcept {
	/**
	 * Запускаем бесконечный цикл
	 */
	while(this->_working){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку уникальным мютексом
			unique_lock lock(static_cast <std::mutex &> (this->_locker));
			// Выполняем ожидание на поступление новых заданий
			this->_cv.wait_for(lock, this->_delay, [this]() noexcept -> bool {
				// Если произведена остановка выходим
				if(!this->_working)
					// Выходим из функции
					return true;
				// Выполняем проверку на пустоту очереди
				return !this->_items.empty();
			});
			// Триггерим событие
			this->trigger();
			// Если произведена остановка
			if(!this->_working)
				// Выходим из цикла
				break;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Триггерим событие
			this->trigger();
			// Если произведена остановка
			if(!this->_working)
				// Выходим из цикла
				break;
		}
	}
}
/**
 * @brief Метод остановки работы таймера
 *
 * @return результат работы функции
 *
 */
bool awh::Watch::stop() noexcept {
	// Переменная результата
	bool result = false;
	// Если работа модуля уже запущена
	if((result = this->_working)){
		// Снимаем флаг запуска работы модуля
		this->_working = !this->_working;
		// Отправляем сообщение, что данные записаны
		this->_cv.notify_all();
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод запуска работы таймера
 *
 * @return результат работы функции
 *
 */
bool awh::Watch::start() noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если работа модуля ещё не запущена
		if((result = !this->_working)){
			// Устанавливаем флаг запуска работы модуля
			this->_working = !this->_working;
			// Создаём дочерний поток для формирования лога
			this->_thr = std::thread(&watch_t::receiving, this);
			// Отсоединяемся от потока
			this->_thr.detach();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод создания нового таймера
 *
 * @return файловый дескриптор для отслеживания
 *
 */
SOCKET awh::Watch::create() noexcept {
	// Выполняем создание таймера
	return this->_notifier.init();
}
/**
 * @brief Метод извлечения идентификатора события
 *
 * @return идентификатор события
 *
 */
uint32_t awh::Watch::event() noexcept {
	// Выполняем вывод полученного уведомления
	return this->_notifier.event();
}
/**
 * @brief Метод убрать таймер из отслеживания
 *
 * @param id идентификатор таймера
 *
 */
void awh::Watch::away(const uint32_t id) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Выполняем блокировку потока
			const locker_t lock(this->_mtx);
			/**
			 * Выполняем перебор всего списка таймеров
			 */
			for(auto i = this->_timers.begin(); i != this->_timers.end(); ++i){
				// Если мы нашли наш таймер
				if(id == i->second){
					// Выполняем удаление значение таймера
					this->_timers.erase(i);
					// Выходим из цикла
					break;
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод ожидания указанного промежутка времени
 *
 * @param id    идентификатор таймера
 * @param delay задержка времени в миллисекундах
 *
 */
void awh::Watch::wait(const uint32_t id, const uint32_t delay) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока
			const locker_t lock(this->_mtx);
			// Выполняем добавление данных в очередь
			this->_items.push(std::make_pair(id, static_cast <uint64_t> (delay) * 1000000));
		}
		// Отправляем сообщение, что данные записаны
		this->_cv.notify_one();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, delay), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Watch::Watch(const fmk_t * fmk, const log_t * log) noexcept :
 _working(false), _delay(TIMEOUT), _notifier(fmk, log), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Watch::~Watch() noexcept {
	// Выполняем остановку работы модуля
	this->stop();
}
