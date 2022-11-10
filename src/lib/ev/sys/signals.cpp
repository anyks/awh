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
		// Если функция обратного вызова установлена, выводим её
		if(this->_fn != nullptr)
			// Выполняем функцию обратного вызова
			this->_fn(watcher.signum);
		// Завершаем работу дочернего процесса
		// exit(watcher.signum);
	}
/**
 * Если операционной системой является MS Windows
 */
#else
	// Функция обратного вызова при получении сигнала
	function <void (const int)> fn = nullptr;
	/**
	 * winHandler Функция фильтр перехватчика сигналов
	 * @param except объект исключения
	 * @return       тип полученного исключения
	 */
	static void SignalHandler(int signal) noexcept {
		// Если функция обратного вызова установлена, выводим её
		if(fn != nullptr)
			// Выполняем функцию обратного вызова
			fn(signal);
		// Завершаем работу основного процесса
		// exit(signal);
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
			// Выполняем остановку отслеживания сигналов
			this->_ev.sint.stop();
			this->_ev.sfpe.stop();
			this->_ev.sill.stop();
			this->_ev.sterm.stop();
			this->_ev.sabrt.stop();
			this->_ev.ssegv.stop();
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sfpe = signal(SIGFPE, nullptr);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sill = signal(SIGILL, nullptr);
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sint = signal(SIGINT, nullptr);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sabrt = signal(SIGABRT, nullptr);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.ssegv = signal(SIGSEGV, nullptr);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sterm = signal(SIGTERM, nullptr);
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
			// Выполняем игнорирование сигналов SIGPIPE и SIGABRT
			::signal(SIGPIPE, SIG_IGN);
			// ::signal(SIGABRT, SIG_IGN);
			// Устанавливаем базу событий для сигналов
			this->_ev.sint.set(this->_base);
			this->_ev.sfpe.set(this->_base);
			this->_ev.sill.set(this->_base);
			this->_ev.sterm.set(this->_base);
			this->_ev.sabrt.set(this->_base);
			this->_ev.ssegv.set(this->_base);
			// Устанавливаем событие на отслеживание сигнала
			this->_ev.sint.set <sig_t, &sig_t::signal> (this);
			this->_ev.sfpe.set <sig_t, &sig_t::signal> (this);
			this->_ev.sill.set <sig_t, &sig_t::signal> (this);
			this->_ev.sterm.set <sig_t, &sig_t::signal> (this);
			this->_ev.sabrt.set <sig_t, &sig_t::signal> (this);
			this->_ev.ssegv.set <sig_t, &sig_t::signal> (this);
			// Выполняем отслеживание возникающего сигнала
			this->_ev.sint.start(SIGINT);
			this->_ev.sfpe.start(SIGFPE);
			this->_ev.sill.start(SIGILL);
			this->_ev.sterm.start(SIGTERM);
			this->_ev.sabrt.start(SIGABRT);
			this->_ev.ssegv.start(SIGSEGV);
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sfpe = signal(SIGFPE, SignalHandler);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sill = signal(SIGILL, SignalHandler);
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sint = signal(SIGINT, SignalHandler);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sabrt = signal(SIGABRT, SignalHandler);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.ssegv = signal(SIGSEGV, SignalHandler);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sterm = signal(SIGTERM, SignalHandler);
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
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку функцию обратного вызова
		this->_fn = callback;
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выполняем установку функцию обратного вызова
		fn = callback;
	#endif
}
