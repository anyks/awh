/**
 * @file: core.cpp
 * @date: 2024-03-07
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

// Подключаем заголовочный файл
#include <lib/ev/core/core.hpp>

/**
 * kick Метод отправки пинка
 */
void awh::Core::Dispatch::kick() noexcept {
	// Если база событий проинициализированна
	if(this->_init){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Выполняем остановку всех событий
		this->base.break_loop(ev::how_t::ALL);
	}
}
/**
 * stop Метод остановки чтения базы событий
 */
void awh::Core::Dispatch::stop() noexcept {
	// Если чтение базы событий уже началось
	if(this->_work && this->_init){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Снимаем флаг работы модуля
		this->_work = !this->_work;
		// Выполняем пинок
		this->kick();
	// Если модуль не инициализирован
	} else if(!this->_init) {
		// Если функция обратного вызова установлена
		if(this->_closedown != nullptr)
			// Выполняем остановку функции активации базы событий
			this->_closedown(true, false);
	}
}
/**
 * start Метод запуска чтения базы событий
 */
void awh::Core::Dispatch::start() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если чтение базы событий ещё не началось
	if(!this->_work && this->_init){
		// Устанавливаем флаг работы модуля
		this->_work = !this->_work;
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Если функция обратного вызова установлена
		if(this->_launching != nullptr)
			// Выполняем запуск функции активации базы событий
			this->_launching(true, true);
		// Выполняем чтение базы событий пока это разрешено
		while(this->_work){
			// Если база событий проинициализированна
			if(this->_init && !this->_freeze){
				/**
				 * Выполняем обработку ошибки
				 */
				try {
					// Если не нужно использовать простой режим чтения
					if(!this->_easy)
						// Выполняем чтение базы событий
						this->base.run();
					// Выполняем чтение базы событий в простом режиме
					else this->base.run(ev::NOWAIT);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				}
			}
			// Замораживаем поток на период времени частоты обновления базы событий
			this_thread::sleep_for(this->_freq);
		}
		// Если функция обратного вызова установлена
		if(this->_closedown != nullptr)
			// Выполняем остановку функции активации базы событий
			this->_closedown(true, true);
	// Если модуль не инициализирован
	} else if(!this->_init) {
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Если функция обратного вызова установлена
		if(this->_launching != nullptr)
			// Выполняем запуск функции активации базы событий
			this->_launching(true, false);
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Core::Dispatch::rebase() noexcept {
	// Если база событий не является виртуальной
	if(!this->_virt){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Если работа уже запущена
		if(this->_work){
			// Выполняем блокировку чтения данных
			this->_init = false;
			// Выполняем пинок
			this->kick();
		}
		// Если объект базы событий нужно удалить
		if(this->base != nullptr)
			// Удаляем объект базы событий
			ev_loop_destroy(this->base);
		/**
		 * Устанавливаем функции обработки ошибок
		 * @param msg сообщение генерирующее ошибку
		 */
		ev::set_syserr_cb([](const char * msg) throw() -> void {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок запроса
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ ERROR LIBEV ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры запроса
				cout << msg << endl;
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		});
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Создаем новую базу
			this->base = ev::loop_ref(ev_default_loop(EVFLAG_NOINOTIFY));
		/**
		 * Если операционной системой является Linux
		 */
		#elif __linux__
			// Создаем новую базу
			this->base = ev::loop_ref(ev_default_loop(ev::EPOLL | ev::NOENV | EVFLAG_NOINOTIFY));
		/**
		 * Если операционной системой является FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Создаем новую базу
			this->base = ev::loop_ref(ev_default_loop(ev::KQUEUE | ev::NOENV | EVFLAG_NOINOTIFY));
		#endif
		// Выполняем разблокировку чтения данных
		this->_init = !this->_virt;
	}
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации
 */
void awh::Core::Dispatch::freeze(const bool mode) noexcept {
	// Если база событий проинициализированна
	if(this->_init){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Выполняем фриз получения данных
		this->_freeze = mode;
		// Если запрещено использовать простое чтение базы событий
		if(this->_freeze)
			// Выполняем фриз чтения данных
			ev_suspend(this->base);
		// Продолжаем чтение данных
		else ev_resume(this->base);
	}
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации
 */
void awh::Core::Dispatch::easily(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Устанавливаем флаг активации простого чтения базы событий
	this->_easy = mode;
	// Выполняем пинок
	this->kick();
}
/**
 * frequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Core::Dispatch::frequency(const uint8_t msec) noexcept {
	// Если база событий проинициализированна
	if(this->_init){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Если количество миллисекунд передано больше 0
		if((this->_easy = (msec > 0)))
			// Устанавливаем частоту обновления базы событий
			this->_freq = chrono::milliseconds(msec);
		// Выполняем сброс частоты обновления базы событий
		else this->_freq = 10ms;
	}
}
/**
 * on Метод установки функции обратного вызова
 * @param status   статус которому соответствует функция
 * @param callback функция обратного вызова
 */
void awh::Core::Dispatch::on(const status_t status, function <void (const bool, const bool)> callback) noexcept {
	// Определяем статус которому соответствует функции
	switch(static_cast <uint8_t> (status)){
		// Если статус функции соответствует запуску базы событий
		case static_cast <uint8_t> (status_t::START):
			// Выполняем установку функции активации базы событий
			this->_launching = callback;
		break;
		// Если статус функции соответствует остановки базы событий
		case static_cast <uint8_t> (status_t::STOP):
			// Выполняем установку функции активации базы событий
			this->_closedown = callback;
		break;
	}
}
/**
 * Dispatch Конструктор
 * @param log объект для работы с логами
 */
awh::Core::Dispatch::Dispatch(const log_t * log) noexcept :
 _easy(false), _work(false), _init(false), _virt(false), _freeze(false),
 base(nullptr), _freq(10ms), _launching(nullptr), _closedown(nullptr), _log(log) {
	// Выполняем получение базы событий
	struct ev_loop * base = ev_default_loop_uc_();
	// Если база событий ещё не проинициализированна
	if(base == nullptr){
		/**
		 * Если операционной системой является Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Очищаем сетевой контекст
			WSACleanup();
			// Идентификатор ошибки
			int error = 0;
			// Выполняем инициализацию сетевого контекста
			if((error = WSAStartup(MAKEWORD(2, 2), &this->_wsaData)) != 0){
				// Очищаем сетевой контекст
				WSACleanup();
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
			// Выполняем проверку версии WinSocket
			if((2 != LOBYTE(this->_wsaData.wVersion)) || (2 != HIBYTE(this->_wsaData.wVersion))){
				// Очищаем сетевой контекст
				WSACleanup();
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		#endif
		// Выполняем инициализацию базы событий
		this->rebase();
	// Если база событий уже инициализированна
	} else {
		// Отмечаем, что база событий является виртуальной
		this->_virt = true;
		// Выполняем установку базы событий
		this->base = ev::loop_ref(base);
	}
}
/**
 * ~Dispatch Деструктор
 */
awh::Core::Dispatch::~Dispatch() noexcept {
	// Если база событий проинициализированна
	if(this->_init){
		// Если база событий не является виртуальной
		if(!this->_virt && (this->base != nullptr)){
			// Удаляем объект базы событий
			ev_loop_destroy(this->base);
			// Выполняем зануление базы событий
			this->base = nullptr;
		}
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Очищаем сетевой контекст
			WSACleanup();
		#endif
	}
}
/**
 * signal Метод вывода полученного сигнала
 */
void awh::Core::signal(const int signal) noexcept {
	// Если процесс является дочерним
	if(this->_pid != ::getpid()){
		// Определяем тип сигнала
		switch(signal){
			// Если возникает сигнал ручной остановкой процесса
			case SIGINT:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] has been terminated, goodbye!", log_t::flag_t::INFO, ::getpid());
				// Выходим из приложения
				::exit(0);
			break;
			// Если возникает сигнал ошибки выполнения арифметической операции
			case SIGFPE:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGFPE");
			break;
			// Если возникает сигнал выполнения неверной инструкции
			case SIGILL:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGILL");
			break;
			// Если возникает сигнал запроса принудительного завершения процесса
			case SIGTERM:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGTERM");
			break;
			// Если возникает сигнал сегментации памяти (обращение к несуществующему адресу памяти)
			case SIGSEGV:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGSEGV");
			break;
			// Если возникает сигнал запроса принудительное закрытие приложения из кода программы
			case SIGABRT:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGABRT");
			break;
		}
		// Выходим принудительно из приложения
		::exit(EXIT_FAILURE);
	// Если процесс является родительским
	} else {
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("crash"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const int)> ("crash", signal);
		// Выходим из приложения
		else ::exit(signal);
	}
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Core::rebase() noexcept {
	// Если система уже запущена
	if(this->_mode){
		// Выполняем остановку работы
		this->stop();
		// Если перехват сигналов активирован
		if(this->_signals == scheme_t::mode_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем пересоздание базы событий
		this->_dispatch.rebase();
		// Если обработка сигналов включена
		if(this->_signals == scheme_t::mode_t::ENABLED){
			// Выполняем установку новой базы событий
			this->_sig.base(this->_dispatch.base);
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		}
		// Выполняем запуск работы
		this->start();
	}
}
/**
 * bind Метод подключения модуля ядра к текущей базе событий
 * @param core модуль ядра для подключения
 */
void awh::Core::bind(core_t * core) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->_mtx.bind);
	// Если база событий активна и она отличается от текущей базы событий
	if((core != nullptr) && (core != this)){
		// Если базы событий совпадают
		if(static_cast <struct ev_loop *> (core->_dispatch.base) == static_cast <struct ev_loop *> (this->_dispatch.base)){
			// Выполняем блокировку потока
			core->_mtx.status.lock();
			// Увеличиваем количество подключённых потоков
			this->_cores++;
			// Устанавливаем флаг запуска
			core->_mode = true;
			// Выполняем разблокировку потока
			core->_mtx.status.unlock();
		}
		// Выполняем запуск управляющей функции
		core->launching(false, true);
	}
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(core_t * core) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->_mtx.bind);
	// Если база событий активна и она совпадает с текущей базы событий
	if((core != nullptr) && (core != this) &&
	   (static_cast <struct ev_loop *> (core->_dispatch.base) == static_cast <struct ev_loop *> (this->_dispatch.base))){
		// Выполняем блокировку потока
		core->_mtx.status.lock();
		// Уменьшаем количество подключённых потоков
		this->_cores--;
		// Запрещаем работу WebSocket
		core->_mode = false;
		// Выполняем разблокировку потока
		core->_mtx.status.unlock();
		// Запускаем метод деактивации базы событий
		core->closedown(false, true);
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Выполняем блокировку потока
	this->_mtx.status.lock();
	// Если система уже запущена
	if(this->_mode){
		// Запрещаем работу WebSocket
		this->_mode = !this->_mode;
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем остановку чтения базы событий
		this->_dispatch.stop();
	// Выполняем разблокировку потока
	} else this->_mtx.status.unlock();
}
/**
 * start Метод запуска клиента
 */
void awh::Core::start() noexcept {
	// Выполняем блокировку потока
	this->_mtx.status.lock();
	// Если система ещё не запущена
	if(!this->_mode){
		// Разрешаем работу WebSocket
		this->_mode = !this->_mode;
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем запуск чтения базы событий
		this->_dispatch.start();
	// Выполняем разблокировку потока
	} else this->_mtx.status.unlock();
}
/**
 * launching Метод вызова при активации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Core::launching(const bool mode, const bool status) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.status);
	// Если требуется изменить статус
	if(status)
		// Устанавливаем статус сетевого ядра
		this->_status = status_t::START;
	// Если требуется изменить статус
	if(status){
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("status"))
			// Выполняем запуск функции в основном потоке
			this->_callbacks.call <void (const status_t)> ("status", this->_status);
		// Если разрешено выводить информацию в лог
		if(this->_verb)
			// Выводим в консоль информацию
			this->_log->print("[+] Start service: PID=%u", log_t::flag_t::INFO, ::getpid());
	}
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Core::closedown(const bool mode, const bool status) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.status);
	// Если требуется изменить статус
	if(status)
		// Устанавливаем статус сетевого ядра
		this->_status = status_t::STOP;
	// Если требуется изменить статус
	if(status){
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("status"))
			// Выполняем запуск функции в основном потоке
			this->_callbacks.call <void (const status_t)> ("status", this->_status);
		// Если разрешено выводить информацию в лог
		if(this->_verb)
			// Выводим в консоль информацию
			this->_log->print("[-] Stop service: PID=%u", log_t::flag_t::INFO, ::getpid());
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::Core::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку функции обратного вызова при краше приложения
	this->_callbacks.set("crash", callbacks);
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callbacks.set("error", callbacks);
	// Выполняем установку функции обратного вызова при запуске/остановки работы модуля
	this->_callbacks.set("status", callbacks);
}
/**
 * working Метод проверки на запуск работы
 * @return результат проверки
 */
bool awh::Core::working() const noexcept {
	// Выводим результат проверки
	return this->_mode;
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации простого чтения базы событий
 */
void awh::Core::easily(const bool mode) noexcept {
	// Определяем запущено ли ядро сети
	const bool start = this->_mode;
	// Если ядро сети уже запущено
	if(start)
		// Останавливаем ядро сети
		this->stop();
	// Устанавливаем режим чтения базы событий
	this->_dispatch.easily(mode);
	// Если ядро сети уже было запущено
	if(start)
		// Запускаем ядро сети
		this->start();
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации заморозки чтения данных
 */
void awh::Core::freeze(const bool mode) noexcept {
	// Устанавливаем режим заморозки чтения данных
	this->_dispatch.freeze(mode);
}
/**
 * verbose Метод установки флага запрета вывода информационных сообщений
 * @param mode флаг запрета вывода информационных сообщений
 */
void awh::Core::verbose(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем флаг запрета вывода информационных сообщений
	this->_verb = mode;
}
/**
 * frequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Core::frequency(const uint8_t msec) noexcept {
	// Устанавливаем частоту чтения базы событий
	this->_dispatch.frequency(msec);
}
/**
 * signalInterception Метод активации перехвата сигналов
 * @param mode флаг активации
 */
void awh::Core::signalInterception(const scheme_t::mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Если флаг активации отличается
	if(this->_signals != mode){
		// Определяем флаг активации
		switch(static_cast <uint8_t> (mode)){
			// Если передан флаг активации перехвата сигналов
			case static_cast <uint8_t> (scheme_t::mode_t::ENABLED): {
				// Устанавливаем функцию обработки сигналов
				this->_sig.on(std::bind(&core_t::signal, this, _1));
				// Выполняем запуск отслеживания сигналов
				this->_sig.start();
				// Устанавливаем флаг активации перехвата сигналов
				this->_signals = mode;
			} break;
			// Если передан флаг деактивации перехвата сигналов
			case static_cast <uint8_t> (scheme_t::mode_t::DISABLED): {
				// Выполняем остановку отслеживания сигналов
				this->_sig.stop();
				// Устанавливаем флаг деактивации перехвата сигналов
				this->_signals = mode;
			} break;
		}
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _mode(false), _verb(true), _cores(0),
 _callbacks(log), _dispatch(log), _sig(_dispatch.base),
 _status(status_t::STOP), _type(engine_t::type_t::NONE),
 _signals(scheme_t::mode_t::DISABLED), _fmk(fmk), _log(log) {
	// Выполняем установку функции активации базы событий
	this->_dispatch.on(status_t::START, std::bind(&awh::Core::launching, this, _1, _2));
	// Выполняем установку функции деактивации базы событий
	this->_dispatch.on(status_t::STOP, std::bind(&awh::Core::closedown, this, _1, _2));
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Устанавливаем статус сетевого ядра
	this->_status = status_t::STOP;
	// Если перехват сигналов активирован
	if(this->_signals == scheme_t::mode_t::ENABLED)
		// Выполняем остановку отслеживания сигналов
		this->_sig.stop();
	// Выполняем остановку сервиса
	this->stop();
}
