/**
 * @file: timeout.cpp
 * @date: 2024-07-02
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

/**
 * Подключаем заголовочный файл
 */
#include <sys/timeout.hpp>

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
void awh::Timeout::trigger() noexcept {
	// Получаем текущее значение даты
	const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Идентификатор файлового дескриптора (сокета)
		SOCKET fd = 0;
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end();){
			// Получаем файловый дескриптор
			fd = i->second;
			// Если время вышло
			if(date >= i->first){
				// Определяем размер погрешности
				const time_t infelicity = (date - i->first);
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
				// Выполняем поиск файловый дескриптор
				auto j = this->_fds.find(fd);
				// Если файловый дескриптор найден в списке
				if(j != this->_fds.end()){
					// Выполняем запись в сокет данных
					this->_pipe.write(fd, &infelicity, sizeof(infelicity), j->second);
					// Выполняем удаление идентификатора таймера
					this->_fds.erase(j);
				}
			// Продолжаем перебор дальше
			} else ++i;
		}
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Получаем первое значение даты в списке
		const time_t first = this->_timers.begin()->first;
		// Если список таймеров не пустой и время задержки выше 0
		if(!this->_timers.empty() && ((first > date ? (first - date) : 0) > 0))
			// Выполняем смену времени таймера
			this->_screen = static_cast <time_t> (first - date);
		// Выполняем обработку входящих данных
		else this->trigger();
	// Устанавливаем таймаут по умолчанию
	} else this->_screen.timeout();
}
/**
 * process Метод обработки процесса добавления таймеров
 * @param data данные таймера для добавления
 */
void awh::Timeout::process(const data_t data) noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Получаем текущее значение даты
	const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Идентификатор файлового дескриптора (сокета)
		SOCKET fd = 0;
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end();){
			// Получаем файловый дескриптор
			fd = i->second;
			// Если время вышло
			if(date >= i->first){
				// Определяем размер погрешности
				const time_t infelicity = (date - i->first);
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
				// Выполняем поиск файловый дескриптор
				auto j = this->_fds.find(fd);
				// Если файловый дескриптор найден в списке
				if(j != this->_fds.end()){
					// Выполняем запись в сокет данных
					this->_pipe.write(fd, &infelicity, sizeof(infelicity), j->second);
					// Выполняем удаление идентификатора таймера
					this->_fds.erase(j);
				}
			// Продолжаем перебор дальше
			} else ++i;
		}
	}
	// Если сокет таймера ещё не добавлен
	if(this->_fds.find(data.fd) == this->_fds.end()){
		// Выполняем добавление файлового дескриптора в список
		this->_fds.emplace(data.fd, data.port);
		// Выполняем добавления нового таймера
		this->_timers.emplace(data.delay + date, data.fd);
	}
	// Выполняем разблокировку потока
	this->_mtx.unlock();
	// Получаем первое значение даты в списке
	const time_t first = this->_timers.begin()->first;
	// Если список таймеров не пустой и время задержки выше 0
	if(!this->_timers.empty() && ((first > date ? (first - date) : 0) > 0))
		// Выполняем смену времени таймера
		this->_screen = static_cast <time_t> (first - date);
	// Выполняем обработку входящих данных
	else this->trigger();
}
/**
 * stop Метод остановки работы таймера
 */
void awh::Timeout::stop() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
/**
 * start Метод запуска работы таймера
 */
void awh::Timeout::start() noexcept {
	// Выполняем запуск работы экрана
	this->_screen.start();
}
/**
 * del Метод удаления таймера
 * @param fd файловый дескриптор таймера
 */
void awh::Timeout::del(const SOCKET fd) noexcept {
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end(); ++i){
			// Если таймер найден
			if(i->second == fd){
				// Выполняем удаление идентификатора таймера
				this->_fds.erase(fd);
				// Выполняем удаление таймера
				this->_timers.erase(i);
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * set Метод установки таймера
 * @param fd    файловый дескриптор таймера
 * @param delay задержка времени в наносекундах
 * @param port  порт сервера на который нужно отправить ответ
 */
void awh::Timeout::set(const SOCKET fd, const time_t delay, const uint32_t port) noexcept {
	// Создаём объект даты для передачи
	data_t data;
	// Устанавливаем файловый дескриптор
	data.fd = fd;
	// Выполняем установку порта
	data.port = port;
	// Устанавливаем задержку времени в наносекундах
	data.delay = delay;
	// Выполняем отправку события экрану
	this->_screen = data;
}
/**
 * Timeout Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Timeout::Timeout(const fmk_t * fmk, const log_t * log) noexcept :
 _pipe(fmk, log), _screen(screen_t <data_t>::health_t::DEAD), _fmk(fmk), _log(log) {
	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Устанавливаем тип пайпа
		this->_pipe.type(pipe_t::type_t::NETWORK);
	/**
	 * Методы для всех остальных операционных систем
	 */
	#else
		// Устанавливаем тип пайпа
		this->_pipe.type(pipe_t::type_t::NATIVE);
	#endif
	// Выполняем добавление функции обратного вызова триггера
	this->_screen = static_cast <function <void (void)>> (bind(&timeout_t::trigger, this));
	// Выполняем добавление функции обратного вызова процесса обработки
	this->_screen = static_cast <function <void (const data_t)>> (bind(&timeout_t::process, this, _1));
}
/**
 * ~Timeout Деструктор
 */
awh::Timeout::~Timeout() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
