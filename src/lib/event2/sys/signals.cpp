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
	// exit(sig);
}
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * intCallback Функция обработки информационных сигналов SIGINT
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::intCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGINT, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGINT).detach();
	}
	/**
	 * fpeCallback Функция обработки информационных сигналов SIGFPE
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::fpeCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGFPE, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGFPE).detach();
	}
	/**
	 * illCallback Функция обработки информационных сигналов SIGILL
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::illCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGILL, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGILL).detach();
	}
	/**
	 * termCallback Функция обработки информационных сигналов SIGTERM
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::termCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGTERM, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGTERM).detach();
	}
	/**
	 * abrtCallback Функция обработки информационных сигналов SIGABRT
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::abrtCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGABRT, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGABRT).detach();
	}
	/**
	 * segvCallback Функция обработки информационных сигналов SIGSEGV
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::segvCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGSEGV, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGSEGV).detach();
	}
/**
 * Если операционной системой является MS Windows
 */
#else
	// Функция обратного вызова при получении сигнала
	function <void (const int)> callbackFn = nullptr;
	/**
	 * winHandler Функция фильтр перехватчика сигналов
	 * @param except объект исключения
	 * @return       тип полученного исключения
	 */
	static void SignalHandler(int signal) noexcept {
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
			// Выполняем очистку памяти сигналов
			this->_ev.sigInt.stop();
			this->_ev.sigFpe.stop();
			this->_ev.sigIll.stop();
			this->_ev.sigTerm.stop();
			this->_ev.sigAbrt.stop();
			this->_ev.sigSegv.stop();
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
			// Устанавливаем базу событий для перехвата сигналов
			this->_ev.sigInt.set(this->_base);
			this->_ev.sigFpe.set(this->_base);
			this->_ev.sigIll.set(this->_base);
			this->_ev.sigTerm.set(this->_base);
			this->_ev.sigAbrt.set(this->_base);
			this->_ev.sigSegv.set(this->_base);
			// Устанавливаем тип отслеживаемого сигнала
			this->_ev.sigInt.set(-1, SIGINT);
			this->_ev.sigFpe.set(-1, SIGFPE);
			this->_ev.sigIll.set(-1, SIGILL);
			this->_ev.sigTerm.set(-1, SIGTERM);
			this->_ev.sigAbrt.set(-1, SIGABRT);
			this->_ev.sigSegv.set(-1, SIGSEGV);
			// Устанавливаем событие на получение сигналов
			this->_ev.sigInt.set(std::bind(&sig_t::intCallback, this, _1, _2));
			this->_ev.sigFpe.set(std::bind(&sig_t::fpeCallback, this, _1, _2));
			this->_ev.sigIll.set(std::bind(&sig_t::illCallback, this, _1, _2));
			this->_ev.sigTerm.set(std::bind(&sig_t::termCallback, this, _1, _2));
			this->_ev.sigAbrt.set(std::bind(&sig_t::abrtCallback, this, _1, _2));
			this->_ev.sigSegv.set(std::bind(&sig_t::segvCallback, this, _1, _2));
			// Выполняем запуск отслеживания сигналов
			this->_ev.sigInt.start();
			this->_ev.sigFpe.start();
			this->_ev.sigIll.start();
			this->_ev.sigTerm.start();
			this->_ev.sigAbrt.start();
			this->_ev.sigSegv.start();
			// Отправка сигнала для теста
			// raise(SIGABRT);
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
/**
 * ~Signals Деструктор
 */
awh::Signals::~Signals() noexcept {
	// Останавливаем работу отслеживания событий
	this->stop();
}
