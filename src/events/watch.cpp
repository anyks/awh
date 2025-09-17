/**
 * @file: watch.cpp
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

/**
 * Подключаем заголовочный файл
 */
#include <events/watch.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * trigger Метод обработки событий триггера
 */
void awh::Watch::trigger() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Получаем текущее значение даты
		const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end();){
			// Если время вышло
			if(date >= i->first.first){
				// Выполняем поиск файловый дескриптор
				auto j = this->_notifiers.find(i->second);
				// Если файловый дескриптор найден в списке
				if(j != this->_notifiers.end())
					// Выполняем отправку сообщения
					j->second->notify((date - i->first.second) / 1000000);
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
			// Продолжаем перебор дальше
			} else ++i;
		}
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Получаем наименьшее значение даты в списке
			const uint64_t smallest = this->_timers.begin()->first.first;
			// Если время задержки выше нуля
			if((smallest > date ? (smallest - date) : 0) > 0){
				// Выполняем смену времени таймера
				this->_screen = static_cast <uint64_t> (smallest - date);
				// Выходим из функции
				return;
			}
		}
		// Устанавливаем таймаут по умолчанию
		this->_screen.timeout();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * process Метод обработки процесса добавления таймеров
 * @param unit параметры участника
 */
void awh::Watch::process(const unit_t unit) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Получаем текущее значение даты
		const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
		// Выполняем добавления нового таймера
		this->_timers.emplace(std::make_pair(unit.delay + date, date), unit.fd);
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end();){
			// Если время вышло
			if(date >= i->first.first){
				// Выполняем поиск файловый дескриптор
				auto j = this->_notifiers.find(i->second);
				// Если файловый дескриптор найден в списке
				if(j != this->_notifiers.end())
					// Выполняем отправку сообщения
					j->second->notify((date - i->first.second) / 1000000);
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
			// Продолжаем перебор дальше
			} else ++i;
		}
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Получаем наименьшее значение даты в списке
			const uint64_t smallest = this->_timers.begin()->first.first;
			// Если время задержки выше нуля
			if((smallest > date ? (smallest - date) : 0) > 0){
				// Выполняем смену времени таймера
				this->_screen = static_cast <uint64_t> (smallest - date);
				// Выходим из функции
				return;
			}
		}
		// Устанавливаем таймаут по умолчанию
		this->_screen.timeout();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(unit.fd, unit.delay), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * stop Метод остановки работы таймера
 */
void awh::Watch::stop() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
/**
 * start Метод запуска работы таймера
 */
void awh::Watch::start() noexcept {
	// Выполняем запуск работы экрана
	this->_screen.start();
}
/**
 * create Метод создания нового таймера
 * @return файловый дескриптор для отслеживания
 */
std::array <SOCKET, 2> awh::Watch::create() noexcept {
	// Результат работы функции
	std::array <SOCKET, 2> result = {
		INVALID_SOCKET,
		INVALID_SOCKET
	};
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем инициализацию нового уведомителя
		std::unique_ptr <notifier_t> notifier = std::make_unique <notifier_t> (this->_fmk, this->_log);
		// Выполняем инициализацию
		result = notifier->init();
		// Если уведомитель инициализирован правильно
		if((result[0] != INVALID_SOCKET) && (result[1] != INVALID_SOCKET)){
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			// Выполняем перенос нашего уведомителя в список уведомителей
			this->_notifiers.emplace(result[0], ::move(notifier));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим полученный результат
	return result;
}
/**
 * event Метод извлечения идентификатора события
 * @param fd файловый дескриптор таймера
 * @return   идентификатор события
 */
uint64_t awh::Watch::event(const SOCKET fd) noexcept {
	// Выполняем поиск нужного нам уведомителя
	auto i = this->_notifiers.find(fd);
	// Если уведомитель найден
	if(i != this->_notifiers.end())
		// Выполняем вывод полученного уведомления
		return i->second->event();
	// Выводим результат
	return 0;
}
/**
 * away Метод убрать таймер из отслеживания
 * @param fd файловый дескриптор таймера
 */
void awh::Watch::away(const SOCKET fd) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			// Выполняем перебор всего списка таймеров
			for(auto i = this->_timers.begin(); i != this->_timers.end(); ++i){
				// Если таймер найден
				if(i->second == fd){
					// Выполняем удаление уведомителя
					this->_notifiers.erase(i->second);
					// Выполняем удаление таймера
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(fd), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * wait Метод ожидания указанного промежутка времени
 * @param fd    файловый дескриптор таймера
 * @param delay задержка времени в миллисекундах
 */
void awh::Watch::wait(const SOCKET fd, const uint32_t delay) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск уведомитель
		auto i = this->_notifiers.find(fd);
		// Если уведомитель найден
		if(i != this->_notifiers.end()){
			// Выполняем обновление уведомителя
			i->second->init();
			// Создаём объект даты для передачи
			unit_t unit;
			// Устанавливаем идентификатор файлового дескриптора
			unit.fd = fd;
			// Устанавливаем задержку времени в наносекундах
			unit.delay = (static_cast <uint64_t> (delay) * 1000000);
			// Выполняем отправку события экрану
			this->_screen = unit;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(fd, delay), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * Watch Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Watch::Watch(const fmk_t * fmk, const log_t * log) noexcept :
 _screen(screen_t <unit_t>::health_t::DEAD), _fmk(fmk), _log(log) {
	// Выполняем добавление функции обратного вызова триггера
	this->_screen = static_cast <function <void (void)>> (std::bind(&watch_t::trigger, this));
	// Выполняем добавление функции обратного вызова процесса обработки
	this->_screen = static_cast <function <void (const unit_t)>> (std::bind(&watch_t::process, this, _1));
}
/**
 * ~Watch Деструктор
 */
awh::Watch::~Watch() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
