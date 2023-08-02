/**
 * @file: core.cpp
 * @date: 2023-07-01
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <cluster/core.hpp>

/**
 * active Метод вывода статуса работы сетевого ядра
 * @param status флаг запуска сетевого ядра
 * @param core   объект сетевого ядра
 */
void awh::cluster::Core::active(const status_t status, awh::core_t * core) noexcept {
	// Определяем статус активности сетевого ядра
	switch(static_cast <uint8_t> (status)){
		// Если система запущена
		case static_cast <uint8_t> (status_t::START):
			// Выполняем запуск кластера
			this->_cluster.start(0);
		break;
		// Если система остановлена
		case static_cast <uint8_t> (status_t::STOP): {
			// Если функция обратного вызова установлена
			if(this->_activeClusterFn != nullptr)
				// Выводим результат в отдельном потоке
				std::thread(this->_activeClusterFn, status_t::STOP, this).detach();
			// Выполняем остановку кластера
			this->_cluster.stop(0);
		} break;
	}
}
/**
 * cluster Метод информирования о статусе кластера
 * @param wid   идентификатор воркера
 * @param pid   идентификатор процесса
 * @param event идентификатор события
 */
void awh::cluster::Core::cluster(const size_t wid, const pid_t pid, const cluster_t::event_t event) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_eventsFn != nullptr){
		// Определяем производится ли инициализация кластера
		if(this->_pid == getpid()){
			// Если функция обратного вызова установлена
			if(this->_activeClusterFn != nullptr)
				// Выводим результат в отдельном потоке
				std::thread(this->_activeClusterFn, status_t::START, const_cast <core_t *> (this)).detach();
			// Выполняем функцию обратного вызова
			this->_eventsFn(worker_t::MASTER, pid, event, const_cast <core_t *> (this));
		// Если производится запуск воркера, выполняем функцию обратного вызова
		} else this->_eventsFn(worker_t::CHILDREN, pid, event, const_cast <core_t *> (this));
	}
}
/**
 * message Метод получения сообщения
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::message(const size_t wid, const pid_t pid, const char * buffer, const size_t size) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_messageFn != nullptr){
		// Определяем производится ли инициализация кластера
		if(this->_pid == getpid())
			// Выполняем функцию обратного вызова
			this->_messageFn(worker_t::MASTER, pid, buffer, size, const_cast <core_t *> (this));
		// Если производится запуск воркера, если функция обратного вызова установлена
		else this->_messageFn(worker_t::CHILDREN, pid, buffer, size, const_cast <core_t *> (this));
	}
}
/**
 * isMaster Метод проверки является ли процесс дочерним
 * @return результат проверки
 */
bool awh::cluster::Core::isMaster() const noexcept {
	// Выводим результат проверки
	return (this->_pid == getpid());
}
/**
 * send Метод отправки сообщение родительскому процессу
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::send(const char * buffer, const size_t size) const noexcept {
	// Выполняем отправку сообщения родительскому процессу
	const_cast <cluster_t *> (&this->_cluster)->send(0, buffer, size);
}
/**
 * send Метод отправки сообщение процессу
 * @param pid    идентификатор процесса для отправки
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::send(const pid_t pid, const char * buffer, const size_t size) const noexcept {
	// Выполняем отправку сообщения указанному процессу
	const_cast <cluster_t *> (&this->_cluster)->send(0, pid, buffer, size);
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::cluster::Core::broadcast(const char * buffer, const size_t size) const noexcept {
	// Выполняем отправку сообщения всем процессам
	const_cast <cluster_t *> (&this->_cluster)->broadcast(0, buffer, size);
}
/**
 * stop Метод остановки клиента
 */
void awh::cluster::Core::stop() noexcept {
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
		if(this->persist){
			/**
			 * Если используется модуль LibEvent2
			 */
			#if defined(AWH_EVENT2)
				// Останавливаем работу таймера
				this->_timer.event.stop();
			/**
			 * Если используется модуль LibEv
			 */
			#elif defined(AWH_EV)
				// Останавливаем работу персистентного таймера
				this->_timer.io.stop();
			#endif
		}
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем отключение всех клиентов
		this->close();
		// Выполняем остановку чтения базы событий
		this->dispatch.stop();
		// Выполняем получение функции обратного вызова
		this->_activeFn = this->_activeClusterFn;
		// Выполняем отключение запуска функции обратного вызова в отдельном потоке
		this->activeOnTrhead = !this->activeOnTrhead;
	// Выполняем разблокировку потока
	} else this->_mtx.status.unlock();
}
/**
 * start Метод запуска клиента
 */
void awh::cluster::Core::start() noexcept {
	// Выполняем блокировку потока
	this->_mtx.status.lock();
	// Если система ещё не запущена
	if(!this->mode){
		// Разрешаем работу WebSocket
		this->mode = !this->mode;
		// Выполняем разблокировку потока
		this->_mtx.status.unlock();
		// Выполняем получение функции обратного вызова
		this->_activeClusterFn = this->_activeFn;
		// Выполняем отключение запуска функции обратного вызова в отдельном потоке
		this->activeOnTrhead = !this->activeOnTrhead;
		// Устанавливаем функцию обратного вызова на запуск системы
		this->_activeFn = std::bind(&cluster::core_t::active, this, _1, _2);
		// Выполняем запуск чтения базы событий
		this->dispatch.start();
	// Выполняем разблокировку потока
	} else this->_mtx.status.unlock();
}
/**
 * on Метод установки функции обратного вызова при получении события
 * @param callback функция обратного вызова для установки
 */
void awh::cluster::Core::on(function <void (const worker_t, const pid_t, const cluster_t::event_t, core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_eventsFn = callback;
}
/**
 * on Метод установки функции обратного вызова при получении сообщения
 * @param callback функция обратного вызова для установки
 */
void awh::cluster::Core::on(function <void (const worker_t, const pid_t, const char *, const size_t, core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_messageFn = callback;
}
/**
 * clusterAsync Метод установки флага асинхронного режима работы
 * @param wid  идентификатор воркера
 * @param mode флаг асинхронного режима работы
 */
void awh::cluster::Core::clusterAsync(const bool mode) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем перевод кластера в асинхронный режим работы
		this->_cluster.async(0, mode);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::cluster::Core::clusterSize(const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Устанавливаем количество рабочих процессов кластера
		this->_clusterSize = size;
		// Выполняем инициализацию кластера
		this->_cluster.init(0, this->_clusterSize);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::cluster::Core::clusterAutoRestart(const bool mode) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Разрешаем автоматический перезапуск упавших процессов
		this->_clusterAutoRestart = mode;
		// Выполняем установку флага автоматического перезапуска убитых дочерних процессов
		this->_cluster.restart(0, this->_clusterAutoRestart);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * Core Конструктор
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
awh::cluster::Core::Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept :
 awh::core_t(fmk, log, family, sonet), _pid(0), _cluster(fmk, log), _clusterSize(1), _clusterAutoRestart(false), _log(log), _eventsFn(nullptr), _messageFn(nullptr) {
	// Устанавливаем идентификатор процесса
	this->_pid = getpid();
	// Устанавливаем тип запускаемого ядра
	this->type = engine_t::type_t::SERVER;
	// Выполняем установку базы данных
	this->_cluster.base(this->dispatch.base);
	// Выполняем инициализацию кластера
	this->_cluster.init(0, this->_clusterSize);
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.on(std::bind(&core_t::cluster, this, _1, _2, _3));
	// Устанавливаем функцию получения входящих сообщений
	this->_cluster.on(std::bind(&core_t::message, this, _1, _2, _3, _4));
}
/**
 * ~Core Деструктор
 */
awh::cluster::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
