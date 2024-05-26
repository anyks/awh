/**
 * @file: signals.cpp
 * @date: 2022-08-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <lib/ev/sys/signals.hpp>

/**
 * callback Функция обратного вызова
 * @param sig идентификатор сигнала
 */
void awh::Signals::callback(const int sig) noexcept {
	// Выполняем остановку всех сотальных сигналов
	this->stop();
	// Если функция обратного вызова установлена, выводим её
	if(this->_fn != nullptr)
		// Выполняем функцию обратного вызова
		this->_fn(sig);
	// Завершаем работу дочернего процесса
	// ::exit(watcher.signum);
}
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * signal Функция обратного вызова при возникновении сигнала
	 * @param watcher объект события сигнала
	 * @param revents идентификатор события
	 */
	void awh::Signals::signal(ev::sig & watcher, int revents) noexcept {
		// Останавливаем сигнал
		watcher.stop();
		// Выполняем игнорирование сигнала
		::signal(watcher.signum, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, watcher.signum).detach();
	}
/**
 * Если операционной системой является MS Windows
 */
#else
	/**
	 * Функция обратного вызова при получении сигнала
	 */
	function <void (const int)> callbackFn = nullptr;
	/**
	 * signalHandler Функция перехватчика сигналов
	 * @param signal идентификатор сигнала
	 */
	static void signalHandler(int signal) noexcept {
		// Если функция обратного вызова установлена, выводим её
		if(callbackFn != nullptr)
			// Выполняем функцию обратного вызова
			std::thread(callbackFn, signal).detach();
	}
#endif
/**
 * stop Метод остановки обработки сигналов
 */
void awh::Signals::stop() noexcept {
	// Если отслеживание сигналов уже запущено
	if(this->_mode){
		// Снимаем флаг запуска отслеживания сигналов
		this->_mode = !this->_mode;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем остановку перехвата сигнала SIGINT
			this->_ev.sigint.stop();
			// Выполняем остановку перехвата сигнала SIGFPE
			this->_ev.sigfpe.stop();
			// Выполняем остановку перехвата сигнала SIGILL
			this->_ev.sigill.stop();
			// Выполняем остановку перехвата сигнала SIGBUS
			this->_ev.sigbus.stop();
			// Выполняем остановку перехвата сигнала SIGTERM
			this->_ev.sigterm.stop();
			// Выполняем остановку перехвата сигнала SIGABRT
			this->_ev.sigabrt.stop();
			// Выполняем остановку перехвата сигнала SIGSEGV
			this->_ev.sigsegv.stop();
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Выполняем остановку перехвата сигнала SIGINT
			this->_ev.sigint = signal(SIGINT, nullptr);
			// Выполняем остановку перехвата сигнала SIGFPE
			this->_ev.sigfpe = signal(SIGFPE, nullptr);
			// Выполняем остановку перехвата сигнала SIGILL
			this->_ev.sigill = signal(SIGILL, nullptr);
			// Выполняем остановку перехвата сигнала SIGBUS
			this->_ev.sigbus = signal(SIGBUS, nullptr);
			// Выполняем остановку перехвата сигнала SIGTERM
			this->_ev.sigterm = signal(SIGTERM, nullptr);
			// Выполняем остановку перехвата сигнала SIGABRT
			this->_ev.sigabrt = signal(SIGABRT, nullptr);
			// Выполняем остановку перехвата сигнала SIGSEGV
			this->_ev.sigsegv = signal(SIGSEGV, nullptr);
		#endif
	}
}
/**
 * start Метод запуска обработки сигналов
 */
void awh::Signals::start() noexcept {
	// Если отслеживание сигналов ещё не запущено
	if(!this->_mode){
		// Устанавливаем флаг запуска отслеживания сигналов
		this->_mode = !this->_mode;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигналов SIGPIPE
			::signal(SIGPIPE, SIG_IGN);
			// Устанавливаем базу событий для сигналов
			this->_ev.sigint.set(this->_base);
			this->_ev.sigfpe.set(this->_base);
			this->_ev.sigill.set(this->_base);
			this->_ev.sigbus.set(this->_base);
			this->_ev.sigterm.set(this->_base);
			this->_ev.sigabrt.set(this->_base);
			this->_ev.sigsegv.set(this->_base);
			// Устанавливаем событие на отслеживание сигнала
			this->_ev.sigint.set <sig_t, &sig_t::signal> (this);
			this->_ev.sigfpe.set <sig_t, &sig_t::signal> (this);
			this->_ev.sigill.set <sig_t, &sig_t::signal> (this);
			this->_ev.sigbus.set <sig_t, &sig_t::signal> (this);
			this->_ev.sigterm.set <sig_t, &sig_t::signal> (this);
			this->_ev.sigabrt.set <sig_t, &sig_t::signal> (this);
			this->_ev.sigsegv.set <sig_t, &sig_t::signal> (this);
			// Выполняем отслеживание возникающего сигнала
			this->_ev.sigint.start(SIGINT);
			this->_ev.sigfpe.start(SIGFPE);
			this->_ev.sigill.start(SIGILL);
			this->_ev.sigbus.start(SIGBUS);
			this->_ev.sigterm.start(SIGTERM);
			this->_ev.sigabrt.start(SIGABRT);
			this->_ev.sigsegv.start(SIGSEGV);
			// Отправка сигнала для теста
			// raise(SIGABRT);
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sigint = signal(SIGINT, signalHandler);
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sigfpe = signal(SIGFPE, signalHandler);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sigill = signal(SIGILL, signalHandler);
			// Создаём обработчик сигнала для SIGBUS
			this->_ev.sigbus = signal(SIGBUS, signalHandler);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sigterm = signal(SIGTERM, signalHandler);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sigabrt = signal(SIGABRT, signalHandler);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.sigsegv = signal(SIGSEGV, signalHandler);
		#endif
	}
}
/**
 * base Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Signals::base(struct ev_loop * base) noexcept {
	// Останавливаем работу отслеживания событий
	this->stop();
	// Выполняем установку базы событий
	this->_base = base;
}
/**
 * on Метод установки функции обратного вызова, которая должна сработать при получении сигнала
 * @param callback функция обратного вызова
 */
void awh::Signals::on(function <void (const int)> callback) noexcept {
	// Выполняем установку функцию обратного вызова
	this->_fn = callback;
	/**
	 * Если операционной системой является MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выполняем установки функции обратного вызова
		callbackFn = std::bind(&sig_t::callback, this, _1);
	#endif
}
