/**
 * @file: core.cpp
 * @date: 2022-09-03
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
#include <lib/ev/core/core.hpp>

/**
 * read Метод вызова при чтении данных с сокета
 * @param watcher объект события чтения
 * @param revents идентификатор события
 */
void awh::scheme_t::adj_t::read(ev::io & watcher, int revents) noexcept {
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->core);
	// Если разрешено выполнять чтения данных из сокета
	if(!this->bev.locked.read)
		// Выполняем передачу данных
		core->transfer(engine_t::method_t::READ, this->aid);
	// Если выполнять чтение данных запрещено
	else {
		// Если запрещено выполнять чтение данных из сокета
		if(this->bev.locked.read)
			// Останавливаем чтение данных
			core->disabled(engine_t::method_t::READ, this->aid);
		// Если запрещено выполнять запись данных в сокет
		if(this->bev.locked.write)
			// Останавливаем запись данных
			core->disabled(engine_t::method_t::WRITE, this->aid);
		// Если запрещено и читать и записывать в сокет
		if(this->bev.locked.read && this->bev.locked.write)
			// Выполняем отключение от сервера
			core->close(this->aid);
	}
}
/**
 * write Метод вызова при записи данных в сокет
 * @param watcher объект события записи
 * @param revents идентификатор события
 */
void awh::scheme_t::adj_t::write(ev::io & watcher, int revents) noexcept {
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->core);
	// Если разрешено выполнять запись данных в сокет
	if(!this->bev.locked.write)
		// Выполняем передачу данных
		core->transfer(engine_t::method_t::WRITE, this->aid);
	// Если выполнять запись данных запрещено
	else {
		// Если запрещено выполнять чтение данных из сокета
		if(this->bev.locked.read)
			// Останавливаем чтение данных
			core->disabled(engine_t::method_t::READ, this->aid);
		// Если запрещено выполнять запись данных в сокет
		if(this->bev.locked.write)
			// Останавливаем запись данных
			core->disabled(engine_t::method_t::WRITE, this->aid);
		// Если запрещено и читать и записывать в сокет
		if(this->bev.locked.read && this->bev.locked.write)
			// Выполняем отключение от сервера
			core->close(this->aid);
	}
}
/**
 * connect Метод вызова при подключении к серверу
 * @param watcher объект события подключения
 * @param revents идентификатор события
 */
void awh::scheme_t::adj_t::connect(ev::io & watcher, int revents) noexcept {
	// Выполняем остановку чтения
	watcher.stop();
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->core);
	// Останавливаем подключение
	core->disabled(engine_t::method_t::CONNECT, this->aid);
	// Выполняем передачу данных об удачном подключении к серверу
	core->connected(this->aid);
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param timer   объект события таймаута
 * @param revents идентификатор события
 */
void awh::scheme_t::adj_t::timeout(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку таймера
	timer.stop();
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->core);
	// Выполняем передачу данных
	core->timeout(this->aid);
}
/**
 * callback Метод обратного вызова
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::Core::Timer::callback(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку таймера
	timer.stop();
	// Получаем функцияю обратного вызова
	auto callback = this->fn;
	// Получаем объект сетевого ядра
	core_t * core = this->core;
	// Получаем идентификатор таймера
	const u_short id = this->id;
	// Если персистентная работа не установлена, удаляем таймер
	if(!this->persist){
		// Если родительский объект установлен
		if(core != nullptr)
			// Удаляем объект таймера
			core->_timers.erase(id);
	// Если нужно продолжить работу таймера
	} else timer.start(this->delay);
	// Если функция обратного вызова установлена
	if(callback != nullptr)
		// Если функция обратного вызова установлена
		callback(id, core);
}
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
	if(this->_work){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Снимаем флаг работы модуля
		this->_work = !this->_work;
		// Выполняем пинок
		this->kick();
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
		// Выполняем запуск функции активации базы событий
		this->_launching();
		// Выполняем чтение базы событий пока это разрешено
		while(this->_work){
			// Если база событий проинициализированна
			if(this->_init){
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
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_core->log->print("%s", log_t::flag_t::WARNING, error.what());
						// Если функция обратного вызова установлена
						if(this->_core->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_core->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::START, error.what());
					#endif
				}
			}
			// Замораживаем поток на период времени частоты обновления базы событий
			this_thread::sleep_for(this->_freq);
		}
		// Выполняем остановку функции активации базы событий
		this->_closedown();
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации
 */
void awh::Core::Dispatch::freeze(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если база событий проинициализированна
	if(this->_init){
		// Если запрещено использовать простое чтение базы событий
		if(mode)
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
 * rebase Метод пересоздания базы событий
 * @param clear флаг очистки предыдущей базы событий
 */
void awh::Core::Dispatch::rebase(const bool clear) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если работа уже запущена
	if(this->_work){
		// Выполняем блокировку чтения данных
		this->_init = !this->_init;
		// Выполняем пинок
		this->kick();
	}
	// Удаляем объект базы событий
	if(clear) ev_loop_destroy(this->base);
	// Устанавливаем функции обработки ошибок
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
		exit(EXIT_FAILURE);
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
	// Если работа уже запущена
	if(this->_work) this->_init = !this->_init;
}
/**
 * setBase Метод установки базы событий
 * @param base база событий
 */
void awh::Core::Dispatch::setBase(struct ev_loop * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если база событий передана
	if(base != nullptr){
		// Если работа уже запущена
		if(this->_work){
			// Выполняем блокировку чтения данных
			this->_init = !this->_init;
			// Выполняем пинок
			this->kick();
		}
		// Удаляем объект базы событий
		ev_loop_destroy(this->base);
		// Создаем новую базу
		this->base = ev::loop_ref(base);
		// Если работа уже запущена
		if(this->_work) this->_init = !this->_init;
	}
}
/**
 * frequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Core::Dispatch::frequency(const uint8_t msec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если количество миллисекунд передано больше 0
	if((this->_easy = (msec > 0)))
		// Устанавливаем частоту обновления базы событий
		this->_freq = chrono::milliseconds(msec);
	// Выполняем сброс частоты обновления базы событий
	else this->_freq = 10ms;
}
/**
 * Dispatch Конструктор
 * @param core объект сетевого ядра
 */
awh::Core::Dispatch::Dispatch(core_t * core) noexcept : _core(core), _easy(false), _work(false), _init(true), base(nullptr), _freq(10ms), _launching(nullptr), _closedown(nullptr) {
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
			exit(EXIT_FAILURE);
		}
		// Выполняем проверку версии WinSocket
		if((2 != LOBYTE(this->_wsaData.wVersion)) || (2 != HIBYTE(this->_wsaData.wVersion))){
			// Очищаем сетевой контекст
			WSACleanup();
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
	#endif
	// Выполняем установку функции активации базы событий
	this->_launching = std::bind(&awh::Core::launching, this->_core);
	// Выполняем установку функции активации базы событий
	this->_closedown = std::bind(&awh::Core::closedown, this->_core);
	// Выполняем инициализацию базы событий
	this->rebase(false);
}
/**
 * ~Dispatch Деструктор
 */
awh::Core::Dispatch::~Dispatch() noexcept {
	// Выполняем остановку работы
	this->stop();
	// Удаляем объект базы событий
	ev_loop_destroy(this->base);
	/**
	 * Если операционной системой является MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		WSACleanup();
	#endif
}
/**
 * launching Метод вызова при активации базы событий
 */
void awh::Core::launching() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.status);
	// Устанавливаем статус сетевого ядра
	this->status = status_t::START;
	// Если список схем сети существует
	if(!this->schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->log);
		// Переходим по всему списку схем сети
		for(auto & scheme : this->schemes){
			// Если функция обратного вызова установлена
			if(scheme.second->callback.is("open"))
				// Устанавливаем полученную функцию обратного вызова
				callback.set <void (const size_t, core_t *)> (scheme.first, scheme.second->callback.get <void (const size_t, core_t *)> ("open"), scheme.first, this);
		}
		// Выполняем все функции обратного вызова
		callback.bind <const size_t, core_t *> ();
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("status")){
		// Если нужно запустить функцию в основном потоке
		if(!this->activeOnTrhead)
			// Выполняем запуск функции в основном потоке
			this->_callback.call <const status_t, core_t *> ("status", status_t::START, this);
		// Выводим результат в отдельном потоке
		else std::thread(this->_callback.get <void (const status_t, core_t *)> ("status"), status_t::START, this).detach();
	}
	// Если разрешено выводить информацию в лог
	if(!this->noinfo)
		// Выводим в консоль информацию
		this->log->print("[+] Start service: pid = %u", log_t::flag_t::INFO, getpid());
	// Если таймер периодического запуска коллбека активирован, запускаем персистентную работу
	if(this->persist){
		// Устанавливаем приоритет выполнения
		ev_set_priority(&this->_timer.io, 2);
		// Устанавливаем базу событий
		this->_timer.io.set(this->dispatch.base);
		// Устанавливаем время задержки персистентного вызова
		this->_timer.delay = (this->_persIntvl / static_cast <float> (1000));
		// Устанавливаем функцию обратного вызова
		this->_timer.io.set <core_t, &core_t::persistent> (this);
		// Запускаем работу таймера
		this->_timer.io.start(this->_timer.delay);
	}
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 */
void awh::Core::closedown() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.status);
	// Устанавливаем статус сетевого ядра
	this->status = status_t::STOP;
	// Если таймер периодического запуска коллбека активирован
	if(this->persist)
		// Останавливаем работу таймера
		this->_timer.io.stop();
	// Выполняем отключение всех адъютантов
	this->close();
	// Если функция обратного вызова установлена
	if(this->_callback.is("status")){
		// Если нужно запустить функцию в основном потоке
		if(!this->activeOnTrhead)
			// Выполняем запуск функции в основном потоке
			this->_callback.call <const status_t, core_t *> ("status", status_t::STOP, this);
		// Выводим результат в отдельном потоке
		else std::thread(this->_callback.get <void (const status_t, core_t *)> ("status"), status_t::STOP, this).detach();
	}
	// Если разрешено выводить информацию в лог
	if(!this->noinfo)
		// Выводим в консоль информацию
		this->log->print("[-] Stop service: pid = %u", log_t::flag_t::INFO, getpid());
}
/**
 * persistent Метод персистентного вызова по таймеру
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::Core::persistent(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку таймера
	timer.stop();
	// Если работа сетевого ядра запущена
	if(this->status == status_t::START){
		// Если список схем сети существует
		if(!this->schemes.empty() && !this->adjutants.empty()){
			// Объект работы с функциями обратного вызова
			fn_t callback(this->log);
			// Переходим по всему списку схем сети
			for(auto & item : this->schemes){
				// Получаем объект схемы сети
				scheme_t * shm = const_cast <scheme_t *> (item.second);
				// Если функция обратного вызова установлена и адъютанты существуют
				if(shm->callback.is("persist") && !shm->adjutants.empty()){
					// Переходим по всему списку адъютантов и формируем список их идентификаторов
					for(auto & adj : shm->adjutants)
						// Устанавливаем полученную функцию обратного вызова
						callback.set <void (const size_t, const size_t, core_t *)> (adj.first, shm->callback.get <void (const size_t, const size_t, core_t *)> ("persist"), adj.first, item.first, this);
				}
			}
			// Выполняем все функции обратного вызова
			callback.bind <const size_t, const size_t, core_t *> ();
		}
		// Устанавливаем время задержки персистентного вызова
		this->_timer.delay = (this->_persIntvl / static_cast <float> (1000));
		// Если нужно продолжить работу таймера
		timer.start(this->_timer.delay);
	}
}
/**
 * signal Метод вывода полученного сигнала
 */
void awh::Core::signal(const int signal) noexcept {
	// Если процесс является дочерним
	if(this->pid != getpid()){
		// Определяем тип сигнала
		switch(signal){
			// Если возникает сигнал ручной остановкой процесса
			case SIGINT:
				// Выводим сообщение об завершении работы процесса
				this->log->print("Child process [%u] has been terminated, goodbye!", log_t::flag_t::INFO, getpid());
				// Выходим из приложения
				exit(0);
			break;
			// Если возникает сигнал ошибки выполнения арифметической операции
			case SIGFPE:
				// Выводим сообщение об завершении работы процесса
				this->log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGFPE");
			break;
			// Если возникает сигнал выполнения неверной инструкции
			case SIGILL:
				// Выводим сообщение об завершении работы процесса
				this->log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGILL");
			break;
			// Если возникает сигнал запроса принудительного завершения процесса
			case SIGTERM:
				// Выводим сообщение об завершении работы процесса
				this->log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGTERM");
			break;
			// Если возникает сигнал сегментации памяти (обращение к несуществующему адресу памяти)
			case SIGSEGV:
				// Выводим сообщение об завершении работы процесса
				this->log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGSEGV");
			break;
			// Если возникает сигнал запроса принудительное закрытие приложения из кода программы
			case SIGABRT:
				// Выводим сообщение об завершении работы процесса
				this->log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGABRT");
			break;
		}
		// Выходим принудительно из приложения
		exit(EXIT_FAILURE);
	// Если процесс является родительским
	} else {
		// Если функция обратного вызова установлена
		if(this->_callback.is("crash"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const int> ("crash", signal);
		// Выходим из приложения
		else exit(signal);
	}
}
/**
 * clean Метод буфера событий
 * @param aid идентификатор адъютанта
 */
void awh::Core::clean(const size_t aid) const noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект сетевого ядра
		core_t * core = const_cast <core_t *> (this);
		// Останавливаем чтение данных
		core->disabled(engine_t::method_t::READ, it->first);
		// Останавливаем запись данных
		core->disabled(engine_t::method_t::WRITE, it->first);
		// Останавливаем выполнения подключением
		core->disabled(engine_t::method_t::CONNECT, it->first);
		// Выполняем блокировку на чтение/запись данных
		adj->bev.locked = scheme_t::locked_t();
	}
}
/**
 * bind Метод подключения модуля ядра к текущей базе событий
 * @param core модуль ядра для подключения
 */
void awh::Core::bind(Core * core) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->_mtx.bind);
	// Если база событий активна и она отличается от текущей базы событий
	if((core != nullptr) && (core != this)){
		// Если базы событий отличаются
		if(core->dispatch.base != this->dispatch.base){
			// Выполняем остановку базы событий
			core->stop();
			// Устанавливаем новую базу событий
			core->dispatch.setBase(this->dispatch.base);
			// Выполняем блокировку потока
			core->_mtx.status.lock();
			// Увеличиваем количество подключённых потоков
			this->cores++;
			// Устанавливаем флаг запуска
			core->mode = true;
			// Выполняем установку нейм-серверов для DNS-резолвера
			core->dns.replace(core->settings.net.second);
			// Выполняем разблокировку потока
			core->_mtx.status.unlock();
		// Если базы событий совпадают
		} else {
			// Выполняем блокировку потока
			core->_mtx.status.lock();
			// Увеличиваем количество подключённых потоков
			this->cores++;
			// Устанавливаем флаг запуска
			core->mode = true;
			// Выполняем разблокировку потока
			core->_mtx.status.unlock();
		}
		// Выполняем запуск управляющей функции
		core->launching();
	}
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(Core * core) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->_mtx.bind);
	// Если база событий активна и она совпадает с текущей базы событий
	if((core != nullptr) && (core != this) && (core->dispatch.base == this->dispatch.base)){
		// Выполняем блокировку потока
		core->_mtx.status.lock();
		// Уменьшаем количество подключённых потоков
		this->cores--;
		// Запрещаем работу WebSocket
		core->mode = false;
		// Выполняем разблокировку потока
		core->_mtx.status.unlock();
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		core->clearTimers();
		// Выполняем блокировку потока
		core->_mtx.status.lock();
		// Если таймер периодического запуска коллбека активирован
		if(core->persist)
			// Останавливаем работу персистентного таймера
			core->_timer.io.stop();
		// Выполняем разблокировку потока
		core->_mtx.status.unlock();
		// Выполняем удаление модуля DNS-резолвера
		core->dns.clear();
		// Запускаем метод деактивации базы событий
		core->closedown();
	}
}
/**
 * on Метод установки функции обратного вызова при краше приложения
 * @param callback функция обратного вызова для установки
 */
void awh::Core::on(function <void (const int)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int)> ("crash", callback);
}
/**
 * on Метод установки функции обратного вызова при запуске/остановки работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::Core::on(function <void (const status_t, Core *)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const status_t, core_t *)> ("status", callback);
}
/**
 * on установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::Core::on(function <void (const log_t::flag_t, const error_t, const string &)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const log_t::flag_t, const error_t, const string &)> ("error", callback);
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Выполняем блокировку потока
	this->_mtx.status.lock();
	// Если система уже запущена
	if(this->mode){
		// Запрещаем работу WebSocket
		this->mode = !this->mode;
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		this->clearTimers();
		// Выполняем блокировку потока
		this->_mtx.status.lock();
		// Если таймер периодического запуска коллбека активирован
		if(this->persist)
			// Останавливаем работу персистентного таймера
			this->_timer.io.stop();
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем отключение всех клиентов
		this->close();
		// Выполняем остановку чтения базы событий
		this->dispatch.stop();
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
	if(!this->mode){
		// Разрешаем работу WebSocket
		this->mode = !this->mode;
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем запуск чтения базы событий
		this->dispatch.start();
	// Выполняем разблокировку потока
	} else this->_mtx.status.unlock();
}
/**
 * working Метод проверки на запуск работы
 * @return результат проверки
 */
bool awh::Core::working() const noexcept {
	// Выводим результат проверки
	return this->mode;
}
/**
 * add Метод добавления схемы сети
 * @param scheme схема рабочей сети
 * @return       идентификатор схемы сети
 */
size_t awh::Core::add(const scheme_t * scheme) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если схема сети передана и URL адрес существует
	if(scheme != nullptr){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.scheme);
		// Получаем объект схемы сети
		scheme_t * shm = const_cast <scheme_t *> (scheme);
		// Получаем идентификатор схемы сети
		result = this->fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
		// Устанавливаем родительский объект
		shm->core = this;
		// Устанавливаем идентификатор схемы сети
		shm->sid = result;
		// Добавляем схему сети в список
		this->schemes.emplace(result, shm);
	}
	// Выводим результат
	return result;
}
/**
 * close Метод отключения всех адъютантов
 */
void awh::Core::close() noexcept {
	/** Реализация метода не требуется **/
}
/**
 * remove Метод удаления всех схем сети
 */
void awh::Core::remove() noexcept {
	/** Реализация метода не требуется **/
}
/**
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::Core::close(const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::Core::remove(const size_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.scheme);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->schemes.end()) this->schemes.erase(it);
	}
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::Core::timeout(const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
}
/**
 * connected Метод вызова при удачном подключении к серверу
 * @param aid идентификатор адъютанта
 */
void awh::Core::connected(const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
}
/**
 * transfer Метед передачи данных между клиентом и сервером
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::Core::transfer(const engine_t::method_t method, const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
	(void) method;
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::Core::bandWidth(const size_t aid, const string & read, const string & write) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
	(void) read;
	(void) write;
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Core::rebase() noexcept {
	// Если система уже запущена
	if(this->mode){
		/**
		 * Timer Структура таймера
		 */
		typedef struct Timer {
			bool persist;                               // Таймер является персистентным
			time_t delay;                               // Задержка времени в миллисекундах
			function <void (const u_short, Core *)> fn; // Функция обратного вызова
			/**
			 * Timer Конструктор
			 */
			Timer() noexcept : persist(false), delay(0), fn(nullptr) {}
		} timer_t;
		// Список пересоздаваемых таймеров
		vector <timer_t> mainTimers(this->_timers.size());
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		if(!this->_timers.empty()){
			// Индекс текущего таймера
			size_t index = 0;
			// Переходим по всем таймерам
			for(auto it = this->_timers.begin(); it != this->_timers.end();){
				// Выполняем блокировку потока
				this->_mtx.timer.lock();
				// Выполняем остановку таймера
				it->second->io.stop();
				// Устанавливаем функцию обратного вызова
				mainTimers.at(index).fn = it->second->fn;
				// Устанавливаем задержку времени в миллисекундах
				mainTimers.at(index).delay = it->second->delay;
				// Устанавливаем флаг персистентности
				mainTimers.at(index).persist = it->second->persist;
				// Удаляем таймер из списка
				it = this->_timers.erase(it);
				// Выполняем разблокировку потока
				this->_mtx.timer.unlock();
				// Увеличиваем значение индекса
				index++;
			}
		}
		// Выполняем остановку работы
		this->stop();
		// Если перехват сигналов активирован
		if(this->_signals == signals_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем пересоздание базы событий
		this->dispatch.rebase();
		// Если обработка сигналов включена
		if(this->_signals == signals_t::ENABLED){
			// Выполняем установку новой базы событий
			this->_sig.base(this->dispatch.base);
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		}
		// Выполняем установку нейм-серверов для DNS-резолвера
		this->dns.replace(this->settings.net.second);
		// Если список таймеров получен
		if(!mainTimers.empty()){
			// Переходим по всему списку таймеров
			for(auto & timer : mainTimers){
				// Если таймер персистентный
				if(timer.persist)
					// Создаём новый интервал таймера
					this->setInterval(timer.delay, timer.fn);
				// Создаём новый таймер
				else this->setTimeout(timer.delay, timer.fn);
			}
			// Выполняем очистку списка таймеров
			mainTimers.clear();
			// Выполняем освобождение выделенной памяти
			vector <timer_t> ().swap(mainTimers);
		}
		// Выполняем запуск работы
		this->start();
	}
}
/**
 * method Метод получения текущего метода работы
 * @param aid идентификатор адъютанта
 * @return    результат работы функции
 */
awh::engine_t::method_t awh::Core::method(const size_t aid) const noexcept {
	// Результат работы функции
	engine_t::method_t result = engine_t::method_t::DISCONNECT;
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Если подключение только установлено
		if(adj->method == engine_t::method_t::CONNECT)
			// Устанавливаем результат работы функции
			result = adj->method;
		// Если производится запись или чтение
		else if((adj->method == engine_t::method_t::READ) || (adj->method == engine_t::method_t::WRITE)) {
			// Устанавливаем результат работы функции
			result = engine_t::method_t::CONNECT;
			// Если производится чтение или запись данных
			if(((adj->method == engine_t::method_t::READ) && !adj->bev.locked.read && adj->bev.locked.write) || ((adj->method == engine_t::method_t::WRITE) && !adj->bev.locked.write))
				// Устанавливаем результат работы функции
				result = adj->method;
		}
	}
	// Выводим результат
	return result;
}
/**
 * enabled Метод активации метода события сокета
 * @param method метод события сокета
 * @param aid    идентификатор адъютанта
 */
void awh::Core::enabled(const engine_t::method_t method, const size_t aid) noexcept {
	// Если работа базы событий продолжается
	if(this->working()){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
			// Если сокет подключения активен
			if((adj->addr.fd != INVALID_SOCKET) && (adj->addr.fd < MAX_SOCKETS)){
				// Получаем объект подключения
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
				// Определяем метод события сокета
				switch(static_cast <uint8_t> (method)){
					// Если событием является чтение
					case static_cast <uint8_t> (engine_t::method_t::READ): {
						// Разрешаем чтение данных из сокета
						adj->bev.locked.read = false;
						// Устанавливаем размер детектируемых байт на чтение
						adj->marker.read = shm->marker.read;
						// Устанавливаем время ожидания чтения данных
						adj->timeouts.read = shm->timeouts.read;
						// Устанавливаем приоритет выполнения для события на чтение
						ev_set_priority(&adj->bev.event.read, -2);
						// Устанавливаем базу событий
						adj->bev.event.read.set(this->dispatch.base);
						// Устанавливаем сокет для чтения
						adj->bev.event.read.set(adj->addr.fd, ev::READ);
						// Устанавливаем событие на чтение данных подключения
						adj->bev.event.read.set <awh::scheme_t::adj_t, &awh::scheme_t::adj_t::read> (adj);
						// Запускаем чтение данных
						adj->bev.event.read.start();
						// Если флаг ожидания входящих сообщений, активирован
						if((adj->timeouts.read > 0) && (this->settings.sonet != scheme_t::sonet_t::UDP)){
							// Устанавливаем приоритет выполнения для таймаута на чтение
							ev_set_priority(&adj->bev.timer.read, 0);
							// Устанавливаем базу событий
							adj->bev.timer.read.set(this->dispatch.base);
							// Устанавливаем событие на таймаут чтения данных подключения
							adj->bev.timer.read.set <awh::scheme_t::adj_t, &awh::scheme_t::adj_t::timeout> (adj);
							// Устанавливаем время ожидания таймера
							adj->bev.timer.read.repeat = adj->timeouts.read;
							// Запускаем ожидание чтения данных
							adj->bev.timer.read.again();
						}
					} break;
					// Если событием является запись
					case static_cast <uint8_t> (engine_t::method_t::WRITE): {
						// Разрешаем запись данных в сокет
						adj->bev.locked.write = false;
						// Устанавливаем размер детектируемых байт на запись
						adj->marker.write = shm->marker.write;
						// Устанавливаем время ожидания записи данных
						adj->timeouts.write = shm->timeouts.write;
						// Устанавливаем приоритет выполнения для события на запись
						ev_set_priority(&adj->bev.event.write, -2);
						// Устанавливаем базу событий
						adj->bev.event.write.set(this->dispatch.base);
						// Устанавливаем сокет для записи
						adj->bev.event.write.set(adj->addr.fd, ev::WRITE);
						// Устанавливаем событие на запись данных подключения
						adj->bev.event.write.set <awh::scheme_t::adj_t, &awh::scheme_t::adj_t::write> (adj);
						// Запускаем запись данных
						adj->bev.event.write.start();
						// Если флаг ожидания исходящих сообщений, активирован
						if((adj->timeouts.write > 0) && (this->settings.sonet != scheme_t::sonet_t::UDP)){
							// Устанавливаем приоритет выполнения для таймаута на запись
							ev_set_priority(&adj->bev.timer.write, 0);
							// Устанавливаем базу событий
							adj->bev.timer.write.set(this->dispatch.base);
							// Устанавливаем событие на таймаут записи данных подключения
							adj->bev.timer.write.set <awh::scheme_t::adj_t, &awh::scheme_t::adj_t::timeout> (adj);
							// Устанавливаем время ожидания таймера
							adj->bev.timer.write.repeat = adj->timeouts.write;
							// Запускаем ожидание запись данных
							adj->bev.timer.write.again();
						}
					} break;
					// Если событием является подключение
					case static_cast <uint8_t> (engine_t::method_t::CONNECT): {
						// Устанавливаем время ожидания записи данных
						adj->timeouts.connect = shm->timeouts.connect;
						// Устанавливаем приоритет выполнения для события на чтения
						ev_set_priority(&adj->bev.event.connect, -2);
						// Устанавливаем базу событий
						adj->bev.event.connect.set(this->dispatch.base);
						// Устанавливаем сокет для записи
						adj->bev.event.connect.set(adj->addr.fd, ev::WRITE);
						// Устанавливаем событие подключения
						adj->bev.event.connect.set <awh::scheme_t::adj_t, &awh::scheme_t::adj_t::connect> (adj);
						// Выполняем запуск подключения
						adj->bev.event.connect.start();
						// Если время ожидания записи данных установлено
						if(adj->timeouts.connect > 0){
							// Устанавливаем базу событий
							adj->bev.timer.connect.set(this->dispatch.base);
							// Устанавливаем событие на запись данных подключения
							adj->bev.timer.connect.set <awh::scheme_t::adj_t, &awh::scheme_t::adj_t::timeout> (adj);
							// Запускаем запись данных на сервер
							adj->bev.timer.connect.start(adj->timeouts.connect);
						}
					} break;
				}
			// Если файловый дескриптор сломан, значит с памятью что-то не то
			} else if(adj->addr.fd > 65535)
				// Удаляем из памяти объект адъютанта
				this->adjutants.erase(it);
		}
	}
}
/**
 * disabled Метод деактивации метода события сокета
 * @param method метод события сокета
 * @param aid    идентификатор адъютанта
 */
void awh::Core::disabled(const engine_t::method_t method, const size_t aid) noexcept {
	// Если работа базы событий продолжается
	if(this->working()){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
			// Если сокет подключения активен
			if(adj->addr.fd < 65535){
				// Определяем метод события сокета
				switch(static_cast <uint8_t> (method)){
					// Если событием является чтение
					case static_cast <uint8_t> (engine_t::method_t::READ): {
						// Запрещаем чтение данных из сокета
						adj->bev.locked.read = true;
						// Останавливаем ожидание чтения данных
						adj->bev.timer.read.stop();
						// Останавливаем чтение данных
						adj->bev.event.read.stop();
					} break;
					// Если событием является запись
					case static_cast <uint8_t> (engine_t::method_t::WRITE): {
						// Запрещаем запись данных в сокет
						adj->bev.locked.write = true;
						// Останавливаем ожидание записи данных
						adj->bev.timer.write.stop();
						// Останавливаем запись данных
						adj->bev.event.write.stop();
					} break;
					// Если событием является подключение
					case static_cast <uint8_t> (engine_t::method_t::CONNECT): {
						// Останавливаем ожидание подключения
						adj->bev.timer.connect.stop();
						// Останавливаем подключение
						adj->bev.event.connect.stop();
					} break;
				}
			// Если файловый дескриптор сломан, значит с памятью что-то не то, удаляем из памяти объект адъютанта
			} else this->adjutants.erase(it);
		}
	}
}
/**
 * write Метод записи буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param aid    идентификатор адъютанта
 */
void awh::Core::write(const char * buffer, const size_t size, const size_t aid) noexcept {
	// Если данные переданы
	if(this->working() && (buffer != nullptr) && (size > 0)){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
			// Если сокет подключения активен
			if((adj->addr.fd != INVALID_SOCKET) && (adj->addr.fd < MAX_SOCKETS)){
				// Добавляем буфер данных для записи
				adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
				// Если запись в сокет заблокирована
				if(adj->bev.locked.write){
					/**
					 * Если операционной системой является Nix-подобная
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						// Определяем протокол подключения
						switch(static_cast <uint8_t> (this->settings.sonet)){
							// Если протокол подключения UDP
							case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
							// Если протокол подключения DTLS
							case static_cast <uint8_t> (scheme_t::sonet_t::DTLS): {
								// Разрешаем запись данных в сокет
								adj->bev.locked.write = false;
								// Выполняем передачу данных
								this->transfer(engine_t::method_t::WRITE, it->first);
							} break;
							// Для всех остальных сокетов
							default:
								// Разрешаем выполнение записи в сокет
								this->enabled(engine_t::method_t::WRITE, it->first);
						}
					/**
					 * Если операционной системой является MS Windows
					 */
					#else
						// Разрешаем запись данных в сокет
						adj->bev.locked.write = false;
						// Выполняем передачу данных
						this->transfer(engine_t::method_t::WRITE, it->first);
					#endif
				}
			// Если файловый дескриптор сломан, значит с памятью что-то не то
			} else if(adj->addr.fd > 65535)
				// Удаляем из памяти объект адъютанта
				this->adjutants.erase(it);
		}
	}
}
/**
 * lockMethod Метод блокировки метода режима работы
 * @param method метод режима работы
 * @param mode   флаг блокировки метода
 * @param aid    идентификатор адъютанта
 */
void awh::Core::lockMethod(const engine_t::method_t method, const bool mode, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch(static_cast <uint8_t> (method)){
			// Режим работы ЧТЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::READ): {
				// Если нужно заблокировать метод
				if(mode)
					// Запрещаем чтение данных из сокета
					const_cast <scheme_t::adj_t *> (it->second)->bev.locked.read = true;
				// Если нужно разблокировать метод
				else const_cast <scheme_t::adj_t *> (it->second)->bev.locked.read = false;
			} break;
			// Режим работы ЗАПИСЬ
			case static_cast <uint8_t> (engine_t::method_t::WRITE): {
				// Если нужно заблокировать метод
				if(mode)
					// Запрещаем запись данных в сокет
					const_cast <scheme_t::adj_t *> (it->second)->bev.locked.write = true;
				// Если нужно разблокировать метод
				else const_cast <scheme_t::adj_t *> (it->second)->bev.locked.write = false;
			} break;
		}
	}
}
/**
 * dataTimeout Метод установки таймаута ожидания появления данных
 * @param method  метод режима работы
 * @param seconds время ожидания в секундах
 * @param aid     идентификатор адъютанта
 */
void awh::Core::dataTimeout(const engine_t::method_t method, const time_t seconds, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch(static_cast <uint8_t> (method)){
			// Режим работы ЧТЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::READ):
				// Устанавливаем время ожидания на входящие данные
				const_cast <scheme_t::adj_t *> (it->second)->timeouts.read = seconds;
			break;
			// Режим работы ЗАПИСЬ
			case static_cast <uint8_t> (engine_t::method_t::WRITE):
				// Устанавливаем время ожидания на исходящие данные
				const_cast <scheme_t::adj_t *> (it->second)->timeouts.write = seconds;
			break;
			// Режим работы ПОДКЛЮЧЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::CONNECT):
				// Устанавливаем время ожидания на исходящие данные
				const_cast <scheme_t::adj_t *> (it->second)->timeouts.connect = seconds;
			break;
		}
	}
}
/**
 * marker Метод установки маркера на размер детектируемых байт
 * @param method метод режима работы
 * @param min    минимальный размер детектируемых байт
 * @param min    максимальный размер детектируемых байт
 * @param aid    идентификатор адъютанта
 */
void awh::Core::marker(const engine_t::method_t method, const size_t min, const size_t max, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch(static_cast <uint8_t> (method)){
			// Режим работы ЧТЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::READ): {
				// Устанавливаем минимальный размер байт
				const_cast <scheme_t::adj_t *> (it->second)->marker.read.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <scheme_t::adj_t *> (it->second)->marker.read.max = max;
				// Если минимальный размер данных для чтения, не установлен
				if(it->second->marker.read.min == 0)
					// Устанавливаем размер минимальных для чтения данных по умолчанию
					const_cast <scheme_t::adj_t *> (it->second)->marker.read.min = BUFFER_READ_MIN;
			} break;
			// Режим работы ЗАПИСЬ
			case static_cast <uint8_t> (engine_t::method_t::WRITE): {
				// Устанавливаем минимальный размер байт
				const_cast <scheme_t::adj_t *> (it->second)->marker.write.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <scheme_t::adj_t *> (it->second)->marker.write.max = max;
				// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
				if(it->second->marker.write.max == 0)
					// Устанавливаем размер максимальных записываемых данных по умолчанию
					const_cast <scheme_t::adj_t *> (it->second)->marker.write.max = BUFFER_WRITE_MAX;
			} break;
		}
	}
}
/**
 * clearTimers Метод очистки всех таймеров
 */
void awh::Core::clearTimers() noexcept {
	// Если список таймеров существует
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.timer);
		// Переходим по всем таймерам
		for(auto it = this->_timers.begin(); it != this->_timers.end();){
			// Выполняем остановку таймера
			it->second->io.stop();
			// Удаляем таймер из списка
			it = this->_timers.erase(it);
		}
	}
}
/**
 * clearTimer Метод очистки таймера
 * @param id идентификатор таймера для очистки
 */
void awh::Core::clearTimer(const u_short id) noexcept {
	// Если список таймеров существует
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.timer);
		// Выполняем поиск идентификатора таймера
		auto it = this->_timers.find(id);
		// Если идентификатор таймера найден
		if(it != this->_timers.end()){
			// Выполняем остановку таймера
			it->second->io.stop();
			// Удаляем объект таймера
			this->_timers.erase(it);
		}
	}
}
/**
 * setTimeout Метод установки таймаута
 * @param delay    задержка времени в миллисекундах
 * @param callback функция обратного вызова
 * @return         идентификатор созданного таймера
 */
u_short awh::Core::setTimeout(const time_t delay, function <void (const u_short, Core *)> callback) noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если данные переданы
	if((delay > 0) && (callback != nullptr)){
		// Выполняем блокировку потока
		this->_mtx.timer.lock();
		// Создаём объект таймера
		auto ret = this->_timers.emplace(this->_timers.size() + 1, unique_ptr <timer_t> (new timer_t));
		// Выполняем разблокировку потока
		this->_mtx.timer.unlock();
		// Получаем идентификатор таймера
		result = ret.first->first;
		// Устанавливаем приоритет выполнения
		ev_set_priority(&ret.first->second->io, 1);
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем задержку времени в миллисекундах
		ret.first->second->delay = (delay / static_cast <float> (1000));
		// Устанавливаем базу событий
		ret.first->second->io.set(this->dispatch.base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->io.set <timer_t, &timer_t::callback> (ret.first->second.get());
		// Запускаем работу таймера
		ret.first->second->io.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
}
/**
 * setInterval Метод установки интервала времени
 * @param delay    задержка времени в миллисекундах
 * @param callback функция обратного вызова
 * @return         идентификатор созданного таймера
 */
u_short awh::Core::setInterval(const time_t delay, function <void (const u_short, Core *)> callback) noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если данные переданы
	if((delay > 0) && (callback != nullptr)){
		// Выполняем блокировку потока
		this->_mtx.timer.lock();
		// Создаём объект таймера
		auto ret = this->_timers.emplace(this->_timers.size() + 1, unique_ptr <timer_t> (new timer_t));
		// Выполняем разблокировку потока
		this->_mtx.timer.unlock();
		// Получаем идентификатор таймера
		result = ret.first->first;
		// Устанавливаем приоритет выполнения
		ev_set_priority(&ret.first->second->io, 1);
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем флаг персистентной работы
		ret.first->second->persist = true;
		// Устанавливаем задержку времени в миллисекундах
		ret.first->second->delay = (delay / static_cast <float> (1000));
		// Устанавливаем базу событий
		ret.first->second->io.set(this->dispatch.base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->io.set <timer_t, &timer_t::callback> (ret.first->second.get());
		// Запускаем работу таймера
		ret.first->second->io.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации простого чтения базы событий
 */
void awh::Core::easily(const bool mode) noexcept {
	// Определяем запущено ли ядро сети
	const bool start = this->mode;
	// Если ядро сети уже запущено, останавливаем его
	if(start) this->stop();
	// Устанавливаем режим чтения базы событий
	this->dispatch.easily(mode);
	// Если ядро сети уже было запущено, запускаем его
	if(start) this->start();
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации заморозки чтения данных
 */
void awh::Core::freeze(const bool mode) noexcept {
	// Устанавливаем режим заморозки чтения данных
	this->dispatch.freeze(mode);
}
/**
 * removeUnixSocket Метод удаления unix-сокета
 * @return результат выполнения операции
 */
bool awh::Core::removeUnixSocket() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если сервер в данный момент не работает
		if((result = !this->working()))
			// Выполняем очистку unix-сокета
			this->settings.filename.clear();
	#endif
	// Выводим результат
	return result;
}
/**
 * unixSocket Метод установки адреса файла unix-сокета
 * @param socket адрес файла unix-сокета
 * @return       результат установки unix-сокета
 */
bool awh::Core::unixSocket(const string & socket) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Адрес файла сокета
		string filename = "";
		// Если адрес unix-сокета передан
		if(!socket.empty())
			// Получаем адрес файла
			filename = socket;
		// Если адрес unix-сокета не передан
		else filename = this->servName;
		// Устанавливаем адрес файла unix-сокета
		this->settings.filename = this->fmk->format("/tmp/%s.sock", this->fmk->transform(filename, fmk_t::transform_t::LOWER).c_str());
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим в лог сообщение
		this->log->print("Microsoft Windows does not support Unix sockets", log_t::flag_t::CRITICAL);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::OS_BROKEN, "Microsoft Windows does not support Unix sockets");
		// Выходим принудительно из приложения
		exit(EXIT_FAILURE);
	#endif
	// Выводим результат
	return !this->settings.filename.empty();
}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::engine_t::proto_t awh::Core::proto() const noexcept {
	// Выполняем вывод поддерживаемого протокола подключения
	return this->settings.proto;
}
/**
 * proto Метод извлечения активного протокола подключения
 * @param aid идентификатор адъютанта
 * @return активный протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::engine_t::proto_t awh::Core::proto(const size_t aid) const noexcept {
	// Результат работы функции
	engine_t::proto_t result = engine_t::proto_t::NONE;
	// Если данные переданы верные
	if(this->working() && (aid > 0)){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end())
			// Выполняем извлечение активного протокола подключения
			result = this->engine.proto(const_cast <awh::scheme_t::adj_t *> (it->second)->ectx);
	}
	// Выводим результат
	return result;
}
/**
 * proto Метод установки поддерживаемого протокола подключения
 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
void awh::Core::proto(const engine_t::proto_t proto) noexcept {
	// Выполняем установку поддерживаемого протокола подключения
	this->settings.proto = proto;
}
/**
 * sonet Метод извлечения типа сокета подключения
 * @return тип сокета подключения (TCP / UDP / SCTP)
 */
awh::scheme_t::sonet_t awh::Core::sonet() const noexcept {
	// Выполняем вывод тип сокета подключения
	return this->settings.sonet;
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::Core::sonet(const scheme_t::sonet_t sonet) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем тип сокета
	this->settings.sonet = sonet;
	/**
	 * Если операционной системой не является Linux или FreeBSD
	 */
	#if !defined(__linux__) && !defined(__FreeBSD__)
		// Если установлен протокол SCTP
		if(this->settings.sonet == scheme_t::sonet_t::SCTP){
			// Выводим в лог сообщение
			this->log->print("SCTP protocol is allowed to be used only in the Linux or FreeBSD operating system", log_t::flag_t::CRITICAL);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, "SCTP protocol is allowed to be used only in the Linux or FreeBSD operating system");
			// Выходим принудительно из приложения
			exit(EXIT_FAILURE);
		}
	#endif
}
/**
 * family Метод извлечения типа протокола интернета
 * @return тип протокола интернета (IPV4 / IPV6 / NIX)
 */
awh::scheme_t::family_t awh::Core::family() const noexcept {
	// Выполняем вывод тип протокола интернета
	return this->settings.family;
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::Core::family(const scheme_t::family_t family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем тип активного интернет-подключения
	this->settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if((this->settings.family == scheme_t::family_t::NIX) && this->settings.filename.empty()){
		// Если перехват сигналов активирован
		if(this->_signals == signals_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
	// Если тип сокета подключения - хост и порт
	} else if(this->settings.family != scheme_t::family_t::NIX) {
		// Если перехват сигналов активирован
		if(this->_signals == signals_t::ENABLED)
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		// Выполняем очистку адреса файла unix-сокета
		this->removeUnixSocket();
	}
}
/**
 * clearDNS Метод сброса кэша резолвера
 * @return результат работы функции
 */
bool awh::Core::clearDNS() noexcept {
	// Выполняем сброс кэша резолвера
	return this->dns.clear();
}
/**
 * flushDNS Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::Core::flushDNS() noexcept {
	// Выполняем очистку кэша DNS-резолвера
	return this->dns.flush();
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения DNS запроса
 * @param sec интервал времени ожидания в секундах
 */
void awh::Core::timeoutDNS(const uint8_t sec) noexcept {
	// Выполняем установку времени ожидания выполнения запроса
	this->dns.timeout(sec);
}
/**
 * prefixDNS Метод установки префикса переменной окружения
 * @param prefix префикс переменной окружения для установки
 */
void awh::Core::prefixDNS(const string & prefix) noexcept {
	// Выполняем установку префикса переменной окружения
	this->dns.setPrefix(prefix);
}
/**
 * cashTimeToLiveDNS Время жизни кэша DNS
 * @param msec время жизни в миллисекундах
 */
void awh::Core::cashTimeToLiveDNS(const time_t msec) noexcept {
	// Выполняем установку времени жизни кэша
	this->dns.timeToLive(msec);
}
/**
 * readHostsDNS Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::Core::readHostsDNS(const string & filename) noexcept {
	// Если адрес файла хостов в файловой системе передан
	if(!filename.empty())
		// Выполняем установку адрес файла хостов в файловой системе
		this->dns.readHosts(filename);
}
/**
 * serversDNS Метод установки серверов имён DNS
 * @param ns список серверов имён
 */
void awh::Core::serversDNS(const vector <string> & ns) noexcept {
	// Если сервера имён переданы, устанавливаем их
	if(!ns.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем установку нейм-серверов для DNS-резолвера
		this->dns.replace(ns);
		// Выполняем установку DNS-резолвера
		this->settings.net.second.assign(ns.begin(), ns.end());
	}
}
/**
 * serversDNS Метод установки серверов имён DNS
 * @param ns     список серверов имён
 * @param family тип протокола интернета (IPV4 / IPV6)
 */
void awh::Core::serversDNS(const vector <string> & ns, const scheme_t::family_t family) noexcept {
	// Если сервера имён переданы, устанавливаем их
	if(!ns.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Устанавливаем полученный список серверов имён
		this->settings.net.second.assign(ns.cbegin(), ns.cend());
		// Определяем тип интернет-протокола
		switch(static_cast <uint8_t> (family)){
			// Если тип протокола интернета IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Выполняем установку нейм-серверов для DNS-резолвера
				this->dns.replace(AF_INET, ns);
			break;
			// Если тип протокола интернета IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Выполняем установку нейм-серверов для DNS-резолвера
				this->dns.replace(AF_INET6, ns);
			break;
		}
	}
}
/**
 * clearBlackListDNS Метод очистки чёрного списка
 * @param domain доменное имя соответствующее IP-адресу
 */
void awh::Core::clearBlackListDNS(const string & domain) noexcept {
	// Выполняем очистку чёрного списка IP-адресов
	this->dns.clearBlackList(domain);
}
/**
 * clearBlackListDNS Метод очистки чёрного списка
 * @param family тип протокола интернета (IPV4 / IPV6)
 * @param domain доменное имя соответствующее IP-адресу
 */
void awh::Core::clearBlackListDNS(const scheme_t::family_t family, const string & domain) noexcept {
	// Определяем тип интернет-протокола
	switch(static_cast <uint8_t> (family)){
		// Если тип протокола интернета IPv4
		case static_cast <uint8_t> (scheme_t::family_t::IPV4):
			// Выполняем очистку чёрного списка IP-адресов
			this->dns.clearBlackList(AF_INET, domain);
		break;
		// Если тип протокола интернета IPv6
		case static_cast <uint8_t> (scheme_t::family_t::IPV6):
			// Выполняем очистку чёрного списка IP-адресов
			this->dns.clearBlackList(AF_INET6, domain);
		break;
	}
}
/**
 * delInBlackListDNS Метод удаления IP-адреса из чёрного списока
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::Core::delInBlackListDNS(const string & domain, const string & ip) noexcept {
	// Выполняем удаление из чёрного списка IP-адреса
	this->dns.delInBlackList(domain, ip);
}
/**
 * delInBlackListDNS Метод удаления IP-адреса из чёрного списока
 * @param family тип протокола интернета (IPV4 / IPV6)
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::Core::delInBlackListDNS(const scheme_t::family_t family, const string & domain, const string & ip) noexcept {
	// Определяем тип интернет-протокола
	switch(static_cast <uint8_t> (family)){
		// Если тип протокола интернета IPv4
		case static_cast <uint8_t> (scheme_t::family_t::IPV4):
			// Выполняем удаление из чёрного списка IP-адреса
			this->dns.delInBlackList(AF_INET, domain, ip);
		break;
		// Если тип протокола интернета IPv6
		case static_cast <uint8_t> (scheme_t::family_t::IPV6):
			// Выполняем удаление из чёрного списка IP-адреса
			this->dns.delInBlackList(AF_INET6, domain, ip);
		break;
	}
}
/**
 * setToBlackListDNS Метод добавления IP-адреса в чёрный список
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::Core::setToBlackListDNS(const string & domain, const string & ip) noexcept {
	// Выполняем установку в чёрный список IP-адреса
	this->dns.setToBlackList(domain, ip);
}
/**
 * setToBlackListDNS Метод добавления IP-адреса в чёрный список
 * @param family тип протокола интернета (IPV4 / IPV6)
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::Core::setToBlackListDNS(const scheme_t::family_t family, const string & domain, const string & ip) noexcept {
	// Определяем тип интернет-протокола
	switch(static_cast <uint8_t> (family)){
		// Если тип протокола интернета IPv4
		case static_cast <uint8_t> (scheme_t::family_t::IPV4):
			// Выполняем установку в чёрный список IP-адреса
			this->dns.setToBlackList(AF_INET, domain, ip);
		break;
		// Если тип протокола интернета IPv6
		case static_cast <uint8_t> (scheme_t::family_t::IPV6):
			// Выполняем установку в чёрный список IP-адреса
			this->dns.setToBlackList(AF_INET6, domain, ip);
		break;
	}
}
/**
 * noInfo Метод установки флага запрета вывода информационных сообщений
 * @param mode флаг запрета вывода информационных сообщений
 */
void awh::Core::noInfo(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем флаг запрета вывода информационных сообщений
	this->noinfo = mode;
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::verifySSL(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку флага проверки домена
	this->engine.verifyEnable(mode);
}
/**
 * persistEnable Метод установки персистентного флага
 * @param mode флаг персистентного запуска каллбека
 */
void awh::Core::persistEnable(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку флага персистентного запуска каллбека
	this->persist = mode;
	// Если работу сетевого ядра уже запущена
	if(this->working()){
		// Если таймер периодического запуска коллбека активирован, запускаем персистентную работу
		if(this->persist){
			// Выполняем остановку работы таймера
			this->_timer.io.stop();
			// Устанавливаем приоритет выполнения
			ev_set_priority(&this->_timer.io, 2);
			// Устанавливаем базу событий
			this->_timer.io.set(this->dispatch.base);
			// Устанавливаем время задержки персистентного вызова
			this->_timer.delay = (this->_persIntvl / static_cast <float> (1000));
			// Устанавливаем функцию обратного вызова
			this->_timer.io.set <core_t, &core_t::persistent> (this);
			// Запускаем работу таймера
			this->_timer.io.start(this->_timer.delay);
		// Выполняем остановку работы таймера
		} else this->_timer.io.stop();
	}
}
/**
 * persistInterval Метод установки персистентного таймера
 * @param itv интервал персистентного таймера в миллисекундах
 */
void awh::Core::persistInterval(const time_t itv) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем интервал персистентного таймера
	this->_persIntvl = itv;
}
/**
 * frequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Core::frequency(const uint8_t msec) noexcept {
	// Устанавливаем частоту чтения базы событий
	this->dispatch.frequency(msec);
}
/**
 * serverName Метод добавления названия сервера
 * @param name название сервера для добавления
 */
void awh::Core::serverName(const string & name) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Если название сервера передано
	if(!name.empty())
		// Устанавливаем новое название сервера
		this->servName = name;
	// Иначе устанавливаем название сервера по умолчанию
	else this->servName = AWH_SHORT_NAME;
}
/**
 * signalInterception Метод активации перехвата сигналов
 * @param mode флаг активации
 */
void awh::Core::signalInterception(const signals_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Если флаг активации отличается
	if(this->_signals != mode){
		// Определяем флаг активации
		switch(static_cast <uint8_t> (mode)){
			// Если передан флаг активации перехвата сигналов
			case static_cast <uint8_t> (signals_t::ENABLED): {
				// Если тип сокета подключения не является unix-сокетом
				if(this->settings.family != scheme_t::family_t::NIX){
					// Устанавливаем функцию обработки сигналов
					this->_sig.on(std::bind(&core_t::signal, this, placeholders::_1));
					// Выполняем запуск отслеживания сигналов
					this->_sig.start();
					// Устанавливаем флаг активации перехвата сигналов
					this->_signals = mode;
				}
			} break;
			// Если передан флаг деактивации перехвата сигналов
			case static_cast <uint8_t> (signals_t::DISABLED): {
				// Выполняем остановку отслеживания сигналов
				this->_sig.stop();
				// Устанавливаем флаг деактивации перехвата сигналов
				this->_signals = mode;
			} break;
		}
	}
}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::Core::ciphers(const vector <string> & ciphers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку алгоритмов шифрования
	this->engine.ciphers(ciphers);
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::Core::ca(const string & trusted, const string & path) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем адрес CA-файла
	this->engine.ca(trusted, path);
}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::Core::certificate(const string & chain, const string & key) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем файлы сертификата
	this->engine.certificate(chain, key);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::Core::network(const vector <string> & ips, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем тип сокета
	this->settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if((this->settings.family == scheme_t::family_t::NIX) && this->settings.filename.empty()){
		// Если перехват сигналов активирован
		if(this->_signals == signals_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
	// Если тип сокета подключения - хост и порт
	} else if(this->settings.family != scheme_t::family_t::NIX) {
		// Если перехват сигналов активирован
		if(this->_signals == signals_t::ENABLED)
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		// Выполняем очистку адреса файла unix-сокета
		this->removeUnixSocket();
	}
	// Если IP-адреса переданы
	if(!ips.empty()){
		// Выполняем установку параметров сети для DNS-резолвера
		this->dns.network(ips);
		// Переходим по всему списку полученных адресов
		for(auto & host : ips){
			// Определяем к какому адресу относится полученный хост
			switch(static_cast <uint8_t> (this->net.host(host))){
				// Если IP-адрес является IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Если IP-адрес является IPv6 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Устанавливаем полученные IP-адреса
					this->settings.net.first.push_back(host);
				break;
				// Для всех остальных адресов
				default: {
					// Определяем тип интернет-протокола
					switch(static_cast <uint8_t> (family)){
						// Если тип протокола интернета IPv4
						case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
							// Выполняем получение IP-адреса для IPv4
							const string & ip = this->dns.host(AF_INET, host);
							// Если IP-адрес успешно получен
							if(!ip.empty())
								// Выполняем добавление полученного хоста в список
								this->settings.net.first.push_back(ip);
						} break;
						// Если тип протокола интернета IPv6
						case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
							// Выполняем получение IP-адреса для IPv6
							const string & ip = this->dns.host(AF_INET6, host);
							// Если результат получен, выполняем пинг
							if(!ip.empty())
								// Выполняем добавление полученного хоста в список
								this->settings.net.first.push_back(ip);
						} break;
					}
				}
			}
		}
	}
}
/**
 * Core Конструктор
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept :
 pid(getpid()), uri(fmk), engine(fmk, log, &uri), dns(fmk, log),
 dispatch(this), _fs(fmk, log), _sig(dispatch.base), _callback(log),
 status(status_t::STOP), type(engine_t::type_t::CLIENT), _signals(signals_t::DISABLED),
 mode(false), noinfo(false), persist(false), activeOnTrhead(true), cores(0),
 servName(AWH_SHORT_NAME), _persIntvl(PERSIST_INTERVAL), fmk(fmk), log(log) {
	// Устанавливаем тип сокета
	this->settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if(this->settings.family == scheme_t::family_t::NIX)
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Выполняем остановку сервиса
	this->stop();
	// Выполняем удаление модуля DNS-резолвера
	this->dns.clear();
	// Выполняем блокировку потока
	this->_mtx.status.lock();
	// Выполняем удаление активных таймеров
	this->_timers.clear();
	// Выполняем удаление списка активных схем сети
	this->schemes.clear();
	// Выполняем остановку персистентного таймера
	this->_timer.io.stop();
	// Выполняем удаление активных адъютантов
	this->adjutants.clear();
	// Устанавливаем статус сетевого ядра
	this->status = status_t::STOP;
	// Если требуется использовать unix-сокет и ядро является сервером
	if((this->settings.family == scheme_t::family_t::NIX) && (this->type == engine_t::type_t::SERVER)){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(this->settings.filename))
				// Удаляем файл сокета
				::unlink(this->settings.filename.c_str());
		#endif
	}
	// Выполняем разблокировку потока
	this->_mtx.status.unlock();
	// Если перехват сигналов активирован
	if(this->_signals == signals_t::ENABLED)
		// Выполняем остановку отслеживания сигналов
		this->_sig.stop();
}
