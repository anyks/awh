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
#include <core/cluster.hpp>

/**
 * active Метод вывода статуса работы сетевого ядра
 * @param status флаг запуска сетевого ядра
 */
void awh::cluster::Core::active(const status_t status) noexcept {
	// Определяем статус активности сетевого ядра
	switch(static_cast <uint8_t> (status)){
		// Если система запущена
		case static_cast <uint8_t> (status_t::START): {
			// Устанавливаем перехват сигналов падения процессов
			this->_cluster.trackCrash(this->_signals == scheme_t::mode_t::ENABLED);
			// Выполняем запуск кластера
			this->_cluster.start(0);
		} break;
		// Если система остановлена
		case static_cast <uint8_t> (status_t::STOP): {
			// Выполняем остановку кластера
			this->_cluster.stop(0);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("statusCluster"))
				// Выводим результат в отдельном потоке
				std::thread(this->_callbacks.get <void (const status_t)> ("statusCluster"), status_t::STOP).detach();
		} break;
	}
}
/**
 * cluster Метод информирования о статусе кластера
 * @param wid   идентификатор воркера
 * @param pid   идентификатор процесса
 * @param event идентификатор события
 */
void awh::cluster::Core::cluster(const uint16_t wid, const pid_t pid, const cluster_t::event_t event) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("events")){
		// Определяем производится ли инициализация кластера
		if(this->master()){
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("statusCluster"))
				// Выводим результат в отдельном потоке
				std::thread(this->_callbacks.get <void (const status_t)> ("statusCluster"), status_t::START).detach();
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const cluster_t::family_t, const pid_t, const cluster_t::event_t)> ("events", cluster_t::family_t::MASTER, pid, event);
		// Если производится запуск воркера, выполняем функцию обратного вызова
		} else this->_callbacks.call <void (const cluster_t::family_t, const pid_t, const cluster_t::event_t)> ("events", cluster_t::family_t::CHILDREN, pid, event);
	}
}
/**
 * message Метод получения сообщения
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::message(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("message")){
		// Определяем производится ли инициализация кластера
		if(this->master())
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const cluster_t::family_t, const pid_t, const char *, const size_t)> ("message", cluster_t::family_t::MASTER, pid, buffer, size);
		// Если производится запуск воркера, если функция обратного вызова установлена
		else this->_callbacks.call <void (const cluster_t::family_t, const pid_t, const char *, const size_t)> ("message", cluster_t::family_t::CHILDREN, pid, buffer, size);
	}
}
/**
 * master Метод проверки является ли процесс родительским
 * @return результат проверки
 */
bool awh::cluster::Core::master() const noexcept {
	// Выводим результат проверки
	return this->_cluster.master();
}
/**
 * pids Метод получения списка дочерних процессов
 * @return список дочерних процессов
 */
set <pid_t> awh::cluster::Core::pids() const noexcept {
	// Выполняем извлечение списка доступных идентификаторов процессов
	return this->_cluster.pids(0);
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
	// Если процесс является родительским
	if(this->master()){
		// Выполняем блокировку потока
		this->_mtx.status.lock();
		// Если система уже запущена
		if(this->_mode){
			// Запрещаем работу Websocket
			this->_mode = !this->_mode;
			// Выполняем разблокировку потока
			this->_mtx.status.unlock();
			// Выполняем остановку чтения базы событий
			this->_dispatch.stop();
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("statusCluster"))
				// Выполняем получение функции обратного вызова
				this->_callbacks.set("statusCluster", "status", this->_callbacks);
		// Выполняем разблокировку потока
		} else this->_mtx.status.unlock();
	// Если процесс является дочерним, выполняем остановку процесса
	} else this->_cluster.stop(0);
}
/**
 * start Метод запуска клиента
 */
void awh::cluster::Core::start() noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Выполняем блокировку потока
		this->_mtx.status.lock();
		// Если система ещё не запущена
		if(!this->_mode){
			// Разрешаем работу Websocket
			this->_mode = !this->_mode;
			// Выполняем разблокировку потока
			this->_mtx.status.unlock();
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("status"))
				// Выполняем получение функции обратного вызова
				this->_callbacks.set("status", "statusCluster", this->_callbacks);
			// Устанавливаем функцию обратного вызова на запуск системы
			this->_callbacks.set <void (const status_t)> ("status", std::bind(&cluster::core_t::active, this, _1));
			// Выполняем запуск чтения базы событий
			this->_dispatch.start();
		// Выполняем разблокировку потока
		} else this->_mtx.status.unlock();
	// Если процесс является дочерним
	} else {
		// Выводим сообщение об ошибке
		this->_log->print("It is not possible to start a cluster from a child process", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::START, "It is not possible to start a cluster from a child process");
	}
}
/**
 * close Метод закрытия всех подключений
 */
void awh::cluster::Core::close() noexcept {
	// Выполняем закрытие подключение передачи данных между процессами
	this->_cluster.close(0);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::cluster::Core::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функций обратного вызова
	awh::core_t::callbacks(callbacks);
	// Выполняем установку функции обратного вызова при получении события
	this->_callbacks.set("events", callbacks);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callbacks.set("message", callbacks);
}
/**
 * size Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::cluster::Core::size(const uint16_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Устанавливаем количество рабочих процессов кластера
		this->_size = size;
		// Выполняем инициализацию кластера
		this->_cluster.init(0, this->_size);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * autoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::cluster::Core::autoRestart(const bool mode) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку флага автоматического перезапуска убитых дочерних процессов
		this->_cluster.restart(0, mode);
		// Устанавливаем флаг отслеживания упавших процессов
		this->_cluster.trackCrash(mode);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * asyncMessages Метод установки флага асинхронного режима обмена сообщениями
 * @param mode флаг асинхронного режима обмена сообщениями
 */
void awh::cluster::Core::asyncMessages(const bool mode) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Устанавливаем флаг асинхронного режима обмена сообщениями
		this->_cluster.asyncMessages(0, mode);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::cluster::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), _size(1), _cluster(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Отключаем отслеживание упавших процессов
	this->_cluster.trackCrash(false);
	// Выполняем инициализацию кластера
	this->_cluster.init(0, this->_size);
	// Выполняем установку базы данных
	this->_cluster.base(this->_dispatch.base);
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.callback <void (const uint16_t, const pid_t, cluster_t::event_t)> ("process", std::bind(&core_t::cluster, this, _1, _2, _3));
	// Устанавливаем функцию получения входящих сообщений
	this->_cluster.callback <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", std::bind(&core_t::message, this, _1, _2, _3, _4));
}
