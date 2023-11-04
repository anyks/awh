/**
 * @file: core.cpp
 * @date: 2022-09-08
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
#include <lib/event2/core/core.hpp>

/**
 * read Метод вызова при чтении данных с сокета
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::scheme_t::broker_t::read(const evutil_socket_t fd, const short event) noexcept {
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->_core);
	// Если разрешено выполнять чтения данных из сокета
	if(!this->_bev.locked.read)
		// Выполняем передачу данных
		core->read(this->_bid);
	// Если выполнять чтение данных запрещено
	else {
		// Если запрещено выполнять чтение данных из сокета
		if(this->_bev.locked.read)
			// Останавливаем чтение данных
			core->events(core_t::mode_t::DISABLED, engine_t::method_t::READ, this->_bid);
		// Если запрещено выполнять запись данных в сокет
		if(this->_bev.locked.write)
			// Останавливаем запись данных
			core->events(core_t::mode_t::DISABLED, engine_t::method_t::WRITE, this->_bid);
		// Если запрещено и читать и записывать в сокет
		if(this->_bev.locked.read && this->_bev.locked.write)
			// Выполняем отключение от сервера
			core->close(this->_bid);
	}
}
/**
 * connect Метод вызова при подключении к серверу
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::scheme_t::broker_t::connect(const evutil_socket_t fd, const short event) noexcept {
	// Удаляем событие коннекта
	this->_bev.events.connect.stop();
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->_core);
	// Останавливаем подключение
	core->events(core_t::mode_t::DISABLED, engine_t::method_t::CONNECT, this->_bid);
	// Выполняем передачу данных об удачном подключении к серверу
	core->connected(this->_bid);
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::scheme_t::broker_t::timeout(const evutil_socket_t fd, const short event) noexcept {
	// Останавливаем событие таймера чтения данных
	this->_bev.timers.read.stop();
	// Останавливаем событие таймера записи данных
	this->_bev.timers.write.stop();
	// Останавливаем событие таймера коннекта
	this->_bev.timers.connect.stop();
	// Получаем объект подключения
	scheme_t * shm = const_cast <scheme_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (shm->_core);
	// Выполняем передачу данных
	core->timeout(this->_bid);
}
/**
 * callback Метод обратного вызова
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::Core::Timer::callback(const evutil_socket_t fd, const short event) noexcept {
	// Останавливаем событие таймера
	this->event.stop();
	// Получаем функцияю обратного вызова
	auto callback = this->fn;
	// Получаем объект сетевого ядра
	core_t * core = this->core;
	// Получаем идентификатор таймера
	const uint16_t id = this->id;
	// Если персистентная работа не установлена, удаляем таймер
	if(!this->persist){
		// Если родительский объект установлен
		if(core != nullptr)
			// Удаляем объект таймера
			core->_timers.erase(id);
	// Если нужно продолжить работу таймера
	} else this->event.start();
	// Если функция обратного вызова установлена
	if(callback != nullptr)
		// Если функция обратного вызова установлена
		callback(id, core);
}
/**
 * ~Timer Деструктор
 */
awh::Core::Timer::~Timer() noexcept {
	// Останавливаем работу таймера
	this->event.stop();
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
		event_base_loopbreak(this->base);
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
		this->_work = false;
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
			if(this->_init && !this->_freeze){
				// Если не нужно использовать простой режим чтения
				if(!this->_easy)
					// Выполняем чтение базы событий
					event_base_dispatch(this->base);
				// Выполняем чтение базы событий в простом режиме
				else event_base_loop(this->base, EVLOOP_ONCE | EVLOOP_NONBLOCK);
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
		// Выполняем фриз получения данных
		this->_freeze = mode;
		// Если запрещено использовать простое чтение базы событий
		if(this->_freeze)
			// Завершаем работу базы событий
			event_base_loopbreak(this->base);
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
	if(clear && (this->base != nullptr))
		// Удаляем объект базы событий
		event_base_free(this->base);
	// Создаем новую базу событий
	this->base = event_base_new();
	// Если работа уже запущена
	if(this->_work)
		// Выполняем разблокировку чтения данных
		this->_init = !this->_init;
}
/**
 * setBase Метод установки базы событий
 * @param base база событий
 */
void awh::Core::Dispatch::setBase(struct event_base * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если база событий передана
	if(base != nullptr){
		// Если работа уже запущена
		if(this->_work){
			// Выполняем блокировку получения данных
			this->_init = !this->_init;
			// Выполняем пинок
			this->kick();
		}
		// Если база событий проинициализированна
		if(this->base != nullptr)
			// Удаляем объект базы событий
			event_base_free(this->base);
		// Создаем новую базу
		this->base = base;
		// Если работа уже запущена
		if(this->_work)
			// Выполняем разблокировку получения данных
			this->_init = !this->_init;
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
awh::Core::Dispatch::Dispatch(core_t * core) noexcept :
 _core(core), _easy(false), _work(false), _init(true), _freeze(false), base(nullptr), _freq(10ms) {
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
	// Если база событий проинициализированна
	if(this->base != nullptr)
		// Удаляем объект базы событий
		event_base_free(this->base);
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
	this->_status = status_t::START;
	// Если список схем сети существует
	if(!this->_schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & scheme : this->_schemes){
			// Если функция обратного вызова установлена
			if(scheme.second->callback.is("open"))
				// Устанавливаем полученную функцию обратного вызова
				callback.set <void (const uint16_t, core_t *)> (scheme.first, scheme.second->callback.get <void (const uint16_t, core_t *)> ("open"), scheme.first, this);
		}
		// Выполняем все функции обратного вызова
		callback.bind <const uint16_t, core_t *> ();
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("status"))
		// Выполняем запуск функции в основном потоке
		this->_callback.call <const status_t, core_t *> ("status", this->_status, this);
	// Если разрешено выводить информацию в лог
	if(!this->_noinfo)
		// Выводим в консоль информацию
		this->_log->print("[+] Start service: pid = %u", log_t::flag_t::INFO, getpid());
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 */
void awh::Core::closedown() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.status);
	// Устанавливаем статус сетевого ядра
	this->_status = status_t::STOP;
	// Выполняем отключение всех брокеров
	this->close();
	// Если функция обратного вызова установлена
	if(this->_callback.is("status"))
		// Выполняем запуск функции в основном потоке
		this->_callback.call <const status_t, core_t *> ("status", this->_status, this);
	// Если разрешено выводить информацию в лог
	if(!this->_noinfo)
		// Выводим в консоль информацию
		this->_log->print("[-] Stop service: pid = %u", log_t::flag_t::INFO, getpid());
}
/**
 * signal Метод вывода полученного сигнала
 */
void awh::Core::signal(const int signal) noexcept {
	// Если процесс является дочерним
	if(this->_pid != getpid()){
		// Определяем тип сигнала
		switch(signal){
			// Если возникает сигнал ручной остановкой процесса
			case SIGINT:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] has been terminated, goodbye!", log_t::flag_t::INFO, getpid());
				// Выходим из приложения
				exit(0);
			break;
			// Если возникает сигнал ошибки выполнения арифметической операции
			case SIGFPE:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGFPE");
			break;
			// Если возникает сигнал выполнения неверной инструкции
			case SIGILL:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGILL");
			break;
			// Если возникает сигнал запроса принудительного завершения процесса
			case SIGTERM:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGTERM");
			break;
			// Если возникает сигнал сегментации памяти (обращение к несуществующему адресу памяти)
			case SIGSEGV:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGSEGV");
			break;
			// Если возникает сигнал запроса принудительное закрытие приложения из кода программы
			case SIGABRT:
				// Выводим сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, getpid(), "SIGABRT");
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
 * @param bid идентификатор брокера
 */
void awh::Core::clean(const uint64_t bid) const noexcept {
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		// Получаем объект брокера
		awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
		// Получаем объект сетевого ядра
		core_t * core = const_cast <core_t *> (this);
		// Останавливаем чтение данных
		core->events(mode_t::DISABLED, engine_t::method_t::READ, it->first);
		// Останавливаем запись данных
		core->events(mode_t::DISABLED, engine_t::method_t::WRITE, it->first);
		// Останавливаем выполнения подключением
		core->events(mode_t::DISABLED, engine_t::method_t::CONNECT, it->first);
		// Выполняем блокировку на чтение/запись данных
		adj->_bev.locked = scheme_t::locked_t();
	}
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Core::rebase() noexcept {
	// Если система уже запущена
	if(this->_mode){
		/**
		 * Timer Структура таймера
		 */
		typedef struct Timer {
			bool persist;                                  // Таймер является персистентным
			time_t delay;                                  // Задержка времени в миллисекундах
			function <void (const uint16_t, core_t *)> fn; // Функция обратного вызова
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
				// Останавливаем работу таймера
				it->second->event.stop();
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
		if(this->_signals == mode_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем пересоздание базы событий
		this->_dispatch.rebase();
		// Если обработка сигналов включена
		if(this->_signals == mode_t::ENABLED){
			// Выполняем установку новой базы событий
			this->_sig.base(this->_dispatch.base);
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		}
		// Если DNS-резолвер установлен
		if(this->_dns != nullptr)
			// Выполняем сброс кэша DNS-резолвера
			this->_dns->flush();
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
 * bind Метод подключения модуля ядра к текущей базе событий
 * @param core модуль ядра для подключения
 */
void awh::Core::bind(core_t * core) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->_mtx.bind);
	// Если база событий активна и она отличается от текущей базы событий
	if((core != nullptr) && (core != this)){
		// Если базы событий отличаются
		if(core->_dispatch.base != this->_dispatch.base){
			// Выполняем остановку базы событий
			core->stop();
			// Устанавливаем новую базу событий
			core->_dispatch.setBase(this->_dispatch.base);
			// Выполняем блокировку потока
			core->_mtx.status.lock();
			// Увеличиваем количество подключённых потоков
			this->_cores++;
			// Устанавливаем флаг запуска
			core->_mode = true;
			// Выполняем установку объекта DNS-резолвера
			core->_dns = this->_dns;
			// Выполняем разблокировку потока
			core->_mtx.status.unlock();
		// Если базы событий совпадают
		} else {
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
		core->launching();
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
	if((core != nullptr) && (core != this) && (core->_dispatch.base == this->_dispatch.base)){
		// Выполняем блокировку потока
		core->_mtx.status.lock();
		// Уменьшаем количество подключённых потоков
		this->_cores--;
		// Запрещаем работу WebSocket
		core->_mode = false;
		// Выполняем разблокировку потока
		core->_mtx.status.unlock();
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		core->clearTimers();
		// Выполняем блокировку потока
		core->_mtx.status.lock();
		// Выполняем разблокировку потока
		core->_mtx.status.unlock();
		// Если DNS-резолвер установлен
		if(core->_dns != nullptr)
			// Выполняем удаление модуля DNS-резолвера
			core->_dns->clear();	
		// Запускаем метод деактивации базы событий
		core->closedown();
		// Зануляем базу событий
		core->_dispatch.base = nullptr;
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
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		this->clearTimers();
		// Выполняем блокировку потока
		this->_mtx.status.lock();
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем отключение всех клиентов
		this->close();
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
 * working Метод проверки на запуск работы
 * @return результат проверки
 */
bool awh::Core::working() const noexcept {
	// Выводим результат проверки
	return this->_mode;
}
/**
 * add Метод добавления схемы сети
 * @param scheme схема рабочей сети
 * @return       идентификатор схемы сети
 */
uint16_t awh::Core::add(const scheme_t * scheme) noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если схема сети передана и URL адрес существует
	if(scheme != nullptr){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.scheme);
		// Получаем объект схемы сети
		scheme_t * shm = const_cast <scheme_t *> (scheme);
		// Получаем идентификатор схемы сети
		result = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
		// Устанавливаем родительский объект
		shm->_core = this;
		// Устанавливаем идентификатор схемы сети
		shm->sid = result;
		// Добавляем схему сети в список
		this->_schemes.emplace(result, shm);
	}
	// Выводим результат
	return result;
}
/**
 * method Метод получения текущего метода работы
 * @param bid идентификатор брокера
 * @return    результат работы функции
 */
awh::engine_t::method_t awh::Core::method(const uint64_t bid) const noexcept {
	// Результат работы функции
	engine_t::method_t result = engine_t::method_t::DISCONNECT;
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		// Получаем объект брокера
		awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
		// Если подключение только установлено
		if(adj->_method == engine_t::method_t::CONNECT)
			// Устанавливаем результат работы функции
			result = adj->_method;
		// Если производится запись или чтение
		else if((adj->_method == engine_t::method_t::READ) || (adj->_method == engine_t::method_t::WRITE)) {
			// Устанавливаем результат работы функции
			result = engine_t::method_t::CONNECT;
			// Если производится чтение или запись данных
			if(((adj->_method == engine_t::method_t::READ) && !adj->_bev.locked.read && adj->_bev.locked.write) || ((adj->_method == engine_t::method_t::WRITE) && !adj->_bev.locked.write))
				// Устанавливаем результат работы функции
				result = adj->_method;
		}
	}
	// Выводим результат
	return result;
}
/**
 * close Метод отключения всех брокеров
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
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::Core::close(const uint64_t bid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) bid;
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::Core::remove(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.scheme);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->_schemes.end())
			// Выполняем удаление схему сети
			this->_schemes.erase(it);
	}
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param bid идентификатор брокера
 */
void awh::Core::timeout(const uint64_t bid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) bid;
}
/**
 * connected Метод вызова при удачном подключении к серверу
 * @param bid идентификатор брокера
 */
void awh::Core::connected(const uint64_t bid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) bid;
}
/**
 * read Метод чтения данных для брокера
 * @param bid идентификатор брокера
 */
void awh::Core::read(const uint64_t bid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) bid;
}
/**
 * write Метод записи буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 */
void awh::Core::write(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Экранируем ошибку неиспользуемых переменных
	(void) bid;
	(void) size;
	(void) buffer;
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::Core::bandWidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) bid;
	(void) read;
	(void) write;
}
/**
 * lockup Метод блокировки метода режима работы
 * @param method метод режима работы
 * @param mode   флаг блокировки метода
 * @param bid    идентификатор брокера
 */
void awh::Core::lockup(const engine_t::method_t method, const bool mode, const uint64_t bid) noexcept {
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		// Определяем метод режима работы
		switch(static_cast <uint8_t> (method)){
			// Режим работы ЧТЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::READ):
				// Выполняем установку разрешения на получение данных
				const_cast <scheme_t::broker_t *> (it->second)->_bev.locked.read = mode;
			break;
			// Режим работы ЗАПИСЬ
			case static_cast <uint8_t> (engine_t::method_t::WRITE):
				// Выполняем установку разрешения на передачу данных
				const_cast <scheme_t::broker_t *> (it->second)->_bev.locked.write = mode;
			break;
		}
	}
}
/**
 * events Метод активации/деактивации метода события сокета
 * @param mode   сигнал активации сокета
 * @param method метод события сокета
 * @param bid    идентификатор брокера
 */
void awh::Core::events(const mode_t mode, const engine_t::method_t method, const uint64_t bid) noexcept {
	// Если работа базы событий продолжается
	if(this->working()){
		// Выполняем извлечение брокера
		auto it = this->_brokers.find(bid);
		// Если брокер получен
		if(it != this->_brokers.end()){
			// Получаем объект брокера
			awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
			// Если сокет подключения активен
			if((adj->_addr.fd != INVALID_SOCKET) && (adj->_addr.fd < MAX_SOCKETS)){
				// Получаем объект подключения
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
				// Определяем метод события сокета
				switch(static_cast <uint8_t> (method)){
					// Если событием является чтение
					case static_cast <uint8_t> (engine_t::method_t::READ): {
						// Определяем сигнал сокета
						switch(static_cast <uint8_t> (mode)){
							// Если установлен сигнал активации сокета
							case static_cast <uint8_t> (mode_t::ENABLED): {
								// Разрешаем чтение данных из сокета
								adj->_bev.locked.read = false;
								// Устанавливаем размер детектируемых байт на чтение
								adj->_marker.read = shm->marker.read;
								// Устанавливаем время ожидания чтения данных
								adj->_timeouts.read = shm->timeouts.read;
								// Устанавливаем тип события
								adj->_bev.events.read.set(adj->_addr.fd, EV_READ);
								// Устанавливаем базу данных событий
								adj->_bev.events.read.set(this->_dispatch.base);
								// Устанавливаем функцию обратного вызова
								adj->_bev.events.read.set(std::bind(&awh::scheme_t::broker_t::read, adj, _1, _2));
								// Выполняем запуск работы события
								adj->_bev.events.read.start();
								// Если флаг ожидания входящих сообщений, активирован
								if(adj->_timeouts.read > 0){
									// Определяем тип активного сокета
									switch(static_cast <uint8_t> (this->_settings.sonet)){
										// Если тип сокета установлен как UDP
										case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
										// Если тип сокета установлен как DTLS
										case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
											// Выполняем установку таймаута ожидания
											adj->_ectx.timeout(adj->_timeouts.read * 1000, engine_t::method_t::READ);
										break;
										// Для всех остальных протоколов
										default: {
											// Устанавливаем тип таймера
											adj->_bev.timers.read.set(-1, EV_TIMEOUT);
											// Устанавливаем базу данных событий
											adj->_bev.timers.read.set(this->_dispatch.base);
											// Устанавливаем функцию обратного вызова
											adj->_bev.timers.read.set(std::bind(&awh::scheme_t::broker_t::timeout, adj, _1, _2));
											// Выполняем запуск работы таймера
											adj->_bev.timers.read.start(adj->_timeouts.read * 1000);
										}
									}
								}
							} break;
							// Если установлен сигнал деактивации сокета
							case static_cast <uint8_t> (mode_t::DISABLED): {
								// Запрещаем чтение данных из сокета
								adj->_bev.locked.read = true;
								// Останавливаем работу таймера
								adj->_bev.timers.read.stop();
								// Останавливаем работу события
								adj->_bev.events.read.stop();
							} break;
						}
					} break;
					// Если событием является запись
					case static_cast <uint8_t> (engine_t::method_t::WRITE): {
						// Определяем сигнал сокета
						switch(static_cast <uint8_t> (mode)){
							// Если установлен сигнал активации сокета
							case static_cast <uint8_t> (mode_t::ENABLED): {
								// Устанавливаем размер детектируемых байт на запись
								adj->_marker.write = shm->marker.write;
								// Устанавливаем время ожидания записи данных
								adj->_timeouts.write = shm->timeouts.write;
								// Если флаг ожидания исходящих сообщений, активирован
								if(adj->_timeouts.write > 0){
									// Определяем тип активного сокета
									switch(static_cast <uint8_t> (this->_settings.sonet)){
										// Если тип сокета установлен как UDP
										case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
										// Если тип сокета установлен как DTLS
										case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
											// Выполняем установку таймаута ожидания
											adj->_ectx.timeout(adj->_timeouts.write * 1000, engine_t::method_t::WRITE);
										break;
										// Для всех остальных протоколов
										default: {
											// Устанавливаем тип таймера
											adj->_bev.timers.write.set(-1, EV_TIMEOUT);
											// Устанавливаем базу данных событий
											adj->_bev.timers.write.set(this->_dispatch.base);
											// Устанавливаем функцию обратного вызова
											adj->_bev.timers.write.set(std::bind(&awh::scheme_t::broker_t::timeout, adj, _1, _2));
											// Выполняем запуск работы таймера
											adj->_bev.timers.write.start(adj->_timeouts.write * 1000);
										}
									}
								}
							} break;
							// Если установлен сигнал деактивации сокета
							case static_cast <uint8_t> (mode_t::DISABLED):
								// Останавливаем работу таймера
								adj->_bev.timers.write.stop();
							break;
						}
					} break;
					// Если событием является подключение
					case static_cast <uint8_t> (engine_t::method_t::CONNECT): {
						// Определяем сигнал сокета
						switch(static_cast <uint8_t> (mode)){
							// Если установлен сигнал активации сокета
							case static_cast <uint8_t> (mode_t::ENABLED): {
								// Устанавливаем время ожидания записи данных
								adj->_timeouts.connect = shm->timeouts.connect;
								// Устанавливаем тип события
								adj->_bev.events.connect.set(adj->_addr.fd, EV_WRITE);
								// Устанавливаем базу данных событий
								adj->_bev.events.connect.set(this->_dispatch.base);
								// Устанавливаем функцию обратного вызова
								adj->_bev.events.connect.set(std::bind(&awh::scheme_t::broker_t::connect, adj, _1, _2));
								// Выполняем запуск работы события
								adj->_bev.events.connect.start();
								// Если время ожидания записи данных установлено
								if(adj->_timeouts.connect > 0){
									// Устанавливаем тип таймера
									adj->_bev.timers.connect.set(-1, EV_TIMEOUT);
									// Устанавливаем базу данных событий
									adj->_bev.timers.connect.set(this->_dispatch.base);
									// Устанавливаем функцию обратного вызова
									adj->_bev.timers.connect.set(std::bind(&awh::scheme_t::broker_t::timeout, adj, _1, _2));
									// Выполняем запуск работы таймера
									adj->_bev.timers.connect.start(adj->_timeouts.connect * 1000);
								}
							} break;
							// Если установлен сигнал деактивации сокета
							case static_cast <uint8_t> (mode_t::DISABLED): {
								// Останавливаем работу таймера
								adj->_bev.timers.connect.stop();
								// Останавливаем работу события
								adj->_bev.events.connect.stop();
							} break;
						}
					} break;
				}
			// Если файловый дескриптор сломан, значит с памятью что-то не то
			} else if(adj->_addr.fd > 65535)
				// Удаляем из памяти объект брокера
				this->_brokers.erase(it);
		}
	}
}
/**
 * dataTimeout Метод установки таймаута ожидания появления данных
 * @param method  метод режима работы
 * @param seconds время ожидания в секундах
 * @param bid     идентификатор брокера
 */
void awh::Core::dataTimeout(const engine_t::method_t method, const time_t seconds, const uint64_t bid) noexcept {
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		// Определяем метод режима работы
		switch(static_cast <uint8_t> (method)){
			// Режим работы ЧТЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::READ):
				// Устанавливаем время ожидания на входящие данные
				const_cast <scheme_t::broker_t *> (it->second)->_timeouts.read = seconds;
			break;
			// Режим работы ЗАПИСЬ
			case static_cast <uint8_t> (engine_t::method_t::WRITE):
				// Устанавливаем время ожидания на исходящие данные
				const_cast <scheme_t::broker_t *> (it->second)->_timeouts.write = seconds;
			break;
			// Режим работы ПОДКЛЮЧЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::CONNECT):
				// Устанавливаем время ожидания на исходящие данные
				const_cast <scheme_t::broker_t *> (it->second)->_timeouts.connect = seconds;
			break;
		}
	}
}
/**
 * marker Метод установки маркера на размер детектируемых байт
 * @param method метод режима работы
 * @param min    минимальный размер детектируемых байт
 * @param min    максимальный размер детектируемых байт
 * @param bid    идентификатор брокера
 */
void awh::Core::marker(const engine_t::method_t method, const size_t min, const size_t max, const uint64_t bid) noexcept {
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		// Определяем метод режима работы
		switch(static_cast <uint8_t> (method)){
			// Режим работы ЧТЕНИЕ
			case static_cast <uint8_t> (engine_t::method_t::READ): {
				// Устанавливаем минимальный размер байт
				const_cast <scheme_t::broker_t *> (it->second)->_marker.read.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <scheme_t::broker_t *> (it->second)->_marker.read.max = max;
				// Если минимальный размер данных для чтения, не установлен
				if(it->second->_marker.read.min == 0)
					// Устанавливаем размер минимальных для чтения данных по умолчанию
					const_cast <scheme_t::broker_t *> (it->second)->_marker.read.min = BUFFER_READ_MIN;
			} break;
			// Режим работы ЗАПИСЬ
			case static_cast <uint8_t> (engine_t::method_t::WRITE): {
				// Устанавливаем минимальный размер байт
				const_cast <scheme_t::broker_t *> (it->second)->_marker.write.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <scheme_t::broker_t *> (it->second)->_marker.write.max = max;
				// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
				if(it->second->_marker.write.max == 0)
					// Устанавливаем размер максимальных записываемых данных по умолчанию
					const_cast <scheme_t::broker_t *> (it->second)->_marker.write.max = BUFFER_WRITE_MAX;
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
			// Останавливаем работу таймера
			it->second->event.stop();
			// Удаляем таймер из списка
			it = this->_timers.erase(it);
		}
	}
}
/**
 * clearTimer Метод очистки таймера
 * @param id идентификатор таймера для очистки
 */
void awh::Core::clearTimer(const uint16_t id) noexcept {
	// Если список таймеров существует
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.timer);
		// Выполняем поиск идентификатора таймера
		auto it = this->_timers.find(id);
		// Если идентификатор таймера найден
		if(it != this->_timers.end()){
			// Останавливаем работу таймера
			it->second->event.stop();
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
uint16_t awh::Core::setTimeout(const time_t delay, function <void (const uint16_t, core_t *)> callback) noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если данные переданы
	if((this->_dispatch.base != nullptr) && (delay > 0) && (callback != nullptr)){
		// Выполняем блокировку потока
		this->_mtx.timer.lock();
		// Создаём объект таймера
		auto ret = this->_timers.emplace(this->_timers.size() + 1, unique_ptr <timer_t> (new timer_t(this->_log)));
		// Выполняем разблокировку потока
		this->_mtx.timer.unlock();
		// Получаем идентификатор таймера
		result = ret.first->first;
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем время задрежки таймера
		ret.first->second->delay = delay;
		// Устанавливаем тип таймера
		ret.first->second->event.set(-1, EV_TIMEOUT);
		// Устанавливаем базу данных событий
		ret.first->second->event.set(this->_dispatch.base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->event.set(std::bind(&timer_t::callback, ret.first->second.get(), _1, _2));
		// Выполняем запуск работы таймера
		ret.first->second->event.start(ret.first->second->delay);
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
uint16_t awh::Core::setInterval(const time_t delay, function <void (const uint16_t, core_t *)> callback) noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если данные переданы
	if((this->_dispatch.base != nullptr) && (delay > 0) && (callback != nullptr)){		
		// Выполняем блокировку потока
		this->_mtx.timer.lock();
		// Создаём объект таймера
		auto ret = this->_timers.emplace(this->_timers.size() + 1, unique_ptr <timer_t> (new timer_t(this->_log)));
		// Выполняем разблокировку потока
		this->_mtx.timer.unlock();
		// Получаем идентификатор таймера
		result = ret.first->first;
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем время задрежки таймера
		ret.first->second->delay = delay;
		// Устанавливаем флаг персистентной работы
		ret.first->second->persist = true;
		// Устанавливаем тип таймера
		ret.first->second->event.set(-1, EV_TIMEOUT);
		// Устанавливаем базу данных событий
		ret.first->second->event.set(this->_dispatch.base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->event.set(std::bind(&timer_t::callback, ret.first->second.get(), _1, _2));
		// Выполняем запуск работы таймера
		ret.first->second->event.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
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
void awh::Core::on(function <void (const status_t, core_t *)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const status_t, core_t *)> ("status", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::Core::on(function <void (const log_t::flag_t, const error_t, const string &)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const log_t::flag_t, const error_t, const string &)> ("error", callback);
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
			this->_settings.filename.clear();
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
		else filename = AWH_SHORT_NAME;
		// Устанавливаем адрес файла unix-сокета
		this->_settings.filename = this->_fmk->format("/tmp/%s.sock", this->_fmk->transform(filename, fmk_t::transform_t::LOWER).c_str());
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим в лог сообщение
		this->_log->print("Microsoft Windows does not support Unix sockets", log_t::flag_t::CRITICAL);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::OS_BROKEN, "Microsoft Windows does not support Unix sockets");
		// Выходим принудительно из приложения
		exit(EXIT_FAILURE);
	#endif
	// Выводим результат
	return !this->_settings.filename.empty();
}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::engine_t::proto_t awh::Core::proto() const noexcept {
	// Выполняем вывод поддерживаемого протокола подключения
	return this->_settings.proto;
}
/**
 * proto Метод извлечения активного протокола подключения
 * @param bid идентификатор брокера
 * @return активный протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::engine_t::proto_t awh::Core::proto(const uint64_t bid) const noexcept {
	// Результат работы функции
	engine_t::proto_t result = engine_t::proto_t::NONE;
	// Если данные переданы верные
	if(this->working() && (bid > 0)){
		// Выполняем извлечение брокера
		auto it = this->_brokers.find(bid);
		// Если брокер получен
		if(it != this->_brokers.end())
			// Выполняем извлечение активного протокола подключения
			result = this->_engine.proto(const_cast <awh::scheme_t::broker_t *> (it->second)->_ectx);
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
	this->_settings.proto = proto;
}
/**
 * sonet Метод извлечения типа сокета подключения
 * @return тип сокета подключения (TCP / UDP / SCTP)
 */
awh::scheme_t::sonet_t awh::Core::sonet() const noexcept {
	// Выполняем вывод тип сокета подключения
	return this->_settings.sonet;
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::Core::sonet(const scheme_t::sonet_t sonet) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	/**
	 * Если операционной системой не является Linux или FreeBSD
	 */
	#if !defined(__linux__) && !defined(__FreeBSD__)
		// Если установлен протокол SCTP
		if(this->_settings.sonet == scheme_t::sonet_t::SCTP){
			// Выводим в лог сообщение
			this->_log->print("SCTP protocol is allowed to be used only in the Linux or FreeBSD operating system", log_t::flag_t::CRITICAL);
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
	return this->_settings.family;
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::Core::family(const scheme_t::family_t family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if((this->_settings.family == scheme_t::family_t::NIX) && this->_settings.filename.empty()){
		// Если перехват сигналов активирован
		if(this->_signals == mode_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
	// Если тип сокета подключения - хост и порт
	} else if(this->_settings.family != scheme_t::family_t::NIX) {
		// Если перехват сигналов активирован
		if(this->_signals == mode_t::ENABLED)
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		// Выполняем очистку адреса файла unix-сокета
		this->removeUnixSocket();
	}
}
/**
 * resolver Метод установки объекта DNS-резолвера
 * @param dns объект DNS-резолвер
 */
void awh::Core::resolver(const dns_t * dns) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем DNS-резолвер
	this->_dns = const_cast <dns_t *> (dns);
}
/**
 * noInfo Метод установки флага запрета вывода информационных сообщений
 * @param mode флаг запрета вывода информационных сообщений
 */
void awh::Core::noInfo(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем флаг запрета вывода информационных сообщений
	this->_noinfo = mode;
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::verifySSL(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку флага проверки домена
	this->_engine.verify(mode);
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
void awh::Core::signalInterception(const mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Если флаг активации отличается
	if(this->_signals != mode){
		// Определяем флаг активации
		switch(static_cast <uint8_t> (mode)){
			// Если передан флаг активации перехвата сигналов
			case static_cast <uint8_t> (mode_t::ENABLED): {
				// Если тип сокета подключения не является unix-сокетом
				if(this->_settings.family != scheme_t::family_t::NIX){
					// Устанавливаем функцию обработки сигналов
					this->_sig.on(std::bind(&core_t::signal, this, placeholders::_1));
					// Выполняем запуск отслеживания сигналов
					this->_sig.start();
					// Устанавливаем флаг активации перехвата сигналов
					this->_signals = mode;
				}
			} break;
			// Если передан флаг деактивации перехвата сигналов
			case static_cast <uint8_t> (mode_t::DISABLED): {
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
	this->_engine.ciphers(ciphers);
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
	this->_engine.ca(trusted, path);
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
	this->_engine.certificate(chain, key);
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
	this->_settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if((this->_settings.family == scheme_t::family_t::NIX) && this->_settings.filename.empty()){
		// Если перехват сигналов активирован
		if(this->_signals == mode_t::ENABLED)
			// Выполняем остановку отслеживания сигналов
			this->_sig.stop();
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
	// Если тип сокета подключения - хост и порт
	} else if(this->_settings.family != scheme_t::family_t::NIX) {
		// Если перехват сигналов активирован
		if(this->_signals == mode_t::ENABLED)
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		// Выполняем очистку адреса файла unix-сокета
		this->removeUnixSocket();
	}
	// Если IP-адреса переданы
	if(!ips.empty()){
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr)
			// Выполняем установку параметров сети для DNS-резолвера
			this->_dns->network(ips);
		// Переходим по всему списку полученных адресов
		for(auto & host : ips){
			// Определяем к какому адресу относится полученный хост
			switch(static_cast <uint8_t> (this->_net.host(host))){
				// Если IP-адрес является IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Если IP-адрес является IPv6 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Устанавливаем полученные IP-адреса
					this->_settings.network.push_back(host);
				break;
				// Для всех остальных адресов
				default: {
					// Если объект DNS-резолвера установлен
					if(this->_dns != nullptr){
						// Определяем тип интернет-протокола
						switch(static_cast <uint8_t> (family)){
							// Если тип протокола интернета IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
								// Выполняем получение IP-адреса для IPv4
								const string & ip = this->_dns->host(AF_INET, host);
								// Если IP-адрес успешно получен
								if(!ip.empty())
									// Выполняем добавление полученного хоста в список
									this->_settings.network.push_back(ip);
							} break;
							// Если тип протокола интернета IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
								// Выполняем получение IP-адреса для IPv6
								const string & ip = this->_dns->host(AF_INET6, host);
								// Если результат получен, выполняем пинг
								if(!ip.empty())
									// Выполняем добавление полученного хоста в список
									this->_settings.network.push_back(ip);
							} break;
						}
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
 _pid(getpid()), _mode(false), _noinfo(false), _cores(0), _fs(fmk, log), _callback(log),
 _uri(fmk), _engine(fmk, log, &_uri), _dispatch(this), _sig(_dispatch.base, log),
 _signals(mode_t::DISABLED), _status(status_t::STOP), _type(engine_t::type_t::NONE),
 _dns(nullptr), _fmk(fmk), _log(log) {
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if(this->_settings.family == scheme_t::family_t::NIX)
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
}
/**
 * Core Конструктор
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
awh::Core::Core(const dns_t * dns, const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept :
 _pid(getpid()), _mode(false), _noinfo(false), _cores(0), _fs(fmk, log), _callback(log),
 _uri(fmk), _engine(fmk, log, &_uri), _dispatch(this), _sig(_dispatch.base, log),
 _signals(mode_t::DISABLED), _status(status_t::STOP), _type(engine_t::type_t::NONE),
 _dns(const_cast <dns_t *> (dns)), _fmk(fmk), _log(log) {
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if(this->_settings.family == scheme_t::family_t::NIX)
		// Выполняем активацию адреса файла сокета
		this->unixSocket();
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Выполняем остановку сервиса
	this->stop();
	// Выполняем блокировку потока
	this->_mtx.status.lock();
	// Выполняем удаление активных таймеров
	this->_timers.clear();
	// Выполняем удаление списка активных схем сети
	this->_schemes.clear();
	// Выполняем удаление активных брокеров
	this->_brokers.clear();
	// Устанавливаем статус сетевого ядра
	this->_status = status_t::STOP;
	// Если требуется использовать unix-сокет и ядро является сервером
	if((this->_settings.family == scheme_t::family_t::NIX) && (this->_type == engine_t::type_t::SERVER)){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(this->_settings.filename))
				// Удаляем файл сокета
				::unlink(this->_settings.filename.c_str());
		#endif
	}
	// Выполняем разблокировку потока
	this->_mtx.status.unlock();
	// Если перехват сигналов активирован
	if(this->_signals == mode_t::ENABLED)
		// Выполняем остановку отслеживания сигналов
		this->_sig.stop();
}
