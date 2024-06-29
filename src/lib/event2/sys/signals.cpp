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
	// ::exit(sig);
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
	 * busCallback Функция обработки информационных сигналов SIGBUS
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Signals::busCallback(evutil_socket_t fd, short event) noexcept {
		// Выполняем игнорирование сигнала
		::signal(SIGBUS, SIG_IGN);
		// Выполняем функцию обратного вызова
		std::thread(&sig_t::callback, this, SIGBUS).detach();
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
			// Выполняем игнорирование сигналов SIGPIPE и SIGABRT
			::signal(SIGPIPE, SIG_IGN);
			// Устанавливаем базу событий для перехвата сигналов
			this->_ev.sigint.set(this->_base);
			this->_ev.sigfpe.set(this->_base);
			this->_ev.sigill.set(this->_base);
			this->_ev.sigbus.set(this->_base);
			this->_ev.sigterm.set(this->_base);
			this->_ev.sigabrt.set(this->_base);
			this->_ev.sigsegv.set(this->_base);
			// Устанавливаем тип отслеживаемого сигнала
			this->_ev.sigint.set(-1, SIGINT);
			this->_ev.sigfpe.set(-1, SIGFPE);
			this->_ev.sigill.set(-1, SIGILL);
			this->_ev.sigbus.set(-1, SIGBUS);
			this->_ev.sigterm.set(-1, SIGTERM);
			this->_ev.sigabrt.set(-1, SIGABRT);
			this->_ev.sigsegv.set(-1, SIGSEGV);
			// Устанавливаем событие на получение сигналов
			this->_ev.sigint.set(std::bind(&sig_t::intCallback, this, _1, _2));
			this->_ev.sigfpe.set(std::bind(&sig_t::fpeCallback, this, _1, _2));
			this->_ev.sigill.set(std::bind(&sig_t::illCallback, this, _1, _2));
			this->_ev.sigbus.set(std::bind(&sig_t::busCallback, this, _1, _2));
			this->_ev.sigterm.set(std::bind(&sig_t::termCallback, this, _1, _2));
			this->_ev.sigabrt.set(std::bind(&sig_t::abrtCallback, this, _1, _2));
			this->_ev.sigsegv.set(std::bind(&sig_t::segvCallback, this, _1, _2));
			// Выполняем запуск отслеживания сигналов
			this->_ev.sigint.start();
			this->_ev.sigfpe.start();
			this->_ev.sigill.start();
			this->_ev.sigbus.start();
			this->_ev.sigterm.start();
			this->_ev.sigabrt.start();
			this->_ev.sigsegv.start();
			// Отправка сигнала для теста
			// ::raise(SIGABRT);
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
