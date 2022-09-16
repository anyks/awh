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
#include <lib/event2/sys/signals.hpp>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * intCallback Функция обработки информационных сигналов SIGINT
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Signals::intCallback(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Получаем объект сигнала
		sig_t * sig = reinterpret_cast <sig_t *> (ctx);
		// Выполняем очистку памяти сигнала
		sig->clear(&sig->_ev.sint);
		// Если функция обратного вызова установлена, выводим её
		if(sig->_fn != nullptr)
			// Выполняем функцию обратного вызова
			sig->_fn(SIGINT);
		// Завершаем работу дочернего процесса
		exit(SIGINT);
	}
	/**
	 * fpeCallback Функция обработки информационных сигналов SIGFPE
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Signals::fpeCallback(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Получаем объект сигнала
		sig_t * sig = reinterpret_cast <sig_t *> (ctx);
		// Выполняем очистку памяти сигнала
		sig->clear(&sig->_ev.sfpe);
		// Если функция обратного вызова установлена, выводим её
		if(sig->_fn != nullptr)
			// Выполняем функцию обратного вызова
			sig->_fn(SIGFPE);
		// Завершаем работу дочернего процесса
		exit(SIGFPE);
	}
	/**
	 * illCallback Функция обработки информационных сигналов SIGILL
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Signals::illCallback(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Получаем объект сигнала
		sig_t * sig = reinterpret_cast <sig_t *> (ctx);
		// Выполняем очистку памяти сигнала
		sig->clear(&sig->_ev.sill);
		// Если функция обратного вызова установлена, выводим её
		if(sig->_fn != nullptr)
			// Выполняем функцию обратного вызова
			sig->_fn(SIGILL);
		// Завершаем работу дочернего процесса
		exit(SIGILL);
	}
	/**
	 * termCallback Функция обработки информационных сигналов SIGTERM
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Signals::termCallback(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Получаем объект сигнала
		sig_t * sig = reinterpret_cast <sig_t *> (ctx);
		// Выполняем очистку памяти сигнала
		sig->clear(&sig->_ev.sterm);
		// Если функция обратного вызова установлена, выводим её
		if(sig->_fn != nullptr)
			// Выполняем функцию обратного вызова
			sig->_fn(SIGTERM);
		// Завершаем работу дочернего процесса
		exit(SIGTERM);
	}
	/**
	 * abrtCallback Функция обработки информационных сигналов SIGABRT
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Signals::abrtCallback(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Получаем объект сигнала
		sig_t * sig = reinterpret_cast <sig_t *> (ctx);
		// Выполняем очистку памяти сигнала
		sig->clear(&sig->_ev.sabrt);
		// Если функция обратного вызова установлена, выводим её
		if(sig->_fn != nullptr)
			// Выполняем функцию обратного вызова
			sig->_fn(SIGABRT);
		// Завершаем работу дочернего процесса
		exit(SIGABRT);
	}
	/**
	 * segvCallback Функция обработки информационных сигналов SIGSEGV
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Signals::segvCallback(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Получаем объект сигнала
		sig_t * sig = reinterpret_cast <sig_t *> (ctx);
		// Выполняем очистку памяти сигнала
		sig->clear(&sig->_ev.ssegv);
		// Если функция обратного вызова установлена, выводим её
		if(sig->_fn != nullptr)
			// Выполняем функцию обратного вызова
			sig->_fn(SIGSEGV);
		// Завершаем работу дочернего процесса
		exit(SIGSEGV);
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
		exit(signal);
	}
#endif
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * clear Метод очистки сигнала
	 * @param signal сигнал для очистки
	 */
	void awh::Signals::clear(struct event ** signal) noexcept {
		// Если сигнал передан
		if((signal != nullptr) && (* signal != nullptr)){
			// Выполняем остановку отслеживания сигнала
			evsignal_del(* signal);
			// Выполняем очистку памяти сигнала
			event_free(* signal);
			// Зануляем объект сигнала
			(* signal) = nullptr;
		}
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
			// Выполняем очистку памяти сигналов
			this->clear(&this->_ev.sint);
			this->clear(&this->_ev.sfpe);
			this->clear(&this->_ev.sill);
			this->clear(&this->_ev.sterm);
			this->clear(&this->_ev.sabrt);
			this->clear(&this->_ev.ssegv);
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
			::signal(SIGABRT, SIG_IGN);
			// Устанавливаем базу событий для сигналов
			this->_ev.sint = evsignal_new(this->_base, SIGINT, &sig_t::intCallback, this);
			this->_ev.sfpe = evsignal_new(this->_base, SIGFPE, &sig_t::fpeCallback, this);
			this->_ev.sill = evsignal_new(this->_base, SIGILL, &sig_t::illCallback, this);
			this->_ev.sterm = evsignal_new(this->_base, SIGTERM, &sig_t::termCallback, this);
			this->_ev.sabrt = evsignal_new(this->_base, SIGABRT, &sig_t::abrtCallback, this);
			this->_ev.ssegv = evsignal_new(this->_base, SIGSEGV, &sig_t::segvCallback, this);
			// Выполняем отслеживание возникающего сигнала
			evsignal_add(this->_ev.sint, nullptr);
			evsignal_add(this->_ev.sfpe, nullptr);
			evsignal_add(this->_ev.sill, nullptr);
			evsignal_add(this->_ev.sterm, nullptr);
			evsignal_add(this->_ev.sabrt, nullptr);
			evsignal_add(this->_ev.ssegv, nullptr);
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
void awh::Signals::base(struct event_base * base) noexcept {
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
/**
 * ~Signals Деструктор
 */
awh::Signals::~Signals() noexcept {
	// Останавливаем работу отслеживания событий
	this->stop();
}
