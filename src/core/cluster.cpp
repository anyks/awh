/**
 * @file: cluster.cpp
 * @date: 2023-07-01
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <core/cluster.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * activeCallback Метод вывода статуса работы сетевого ядра
 * @param status флаг запуска сетевого ядра
 */
void awh::cluster::Core::activeCallback(const status_t status) noexcept {
	// Определяем статус активности сетевого ядра
	switch(static_cast <uint8_t> (status)){
		// Если система запущена
		case static_cast <uint8_t> (status_t::START):
			// Выполняем запуск кластера
			this->_cluster.start(0);
		break;
		// Если система остановлена
		case static_cast <uint8_t> (status_t::STOP): {
			// Выполняем остановку кластера
			this->_cluster.stop(0);
			// Если функция обратного вызова установлена
			if(this->_callback.is("clusterStatus"))
				// Выводим результат в отдельном потоке
				std::thread(this->_callback.get <void (const status_t)> ("clusterStatus"), status_t::STOP).detach();
		} break;
	}
}
/**
 * readyCallback Метод получения события подключения дочерних процессов
 * @param wid  идентификатор воркера
 * @param pid идентификатор процесса
 */
void awh::cluster::Core::readyCallback([[maybe_unused]] const uint16_t wid, const pid_t pid) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("ready"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t)> ("ready", pid);
}
/**
 * rebaseCallback Метод события пересоздании процесса
 * @param wid  идентификатор воркера
 * @param pid  идентификатор процесса
 * @param opid идентификатор старого процесса
 */
void awh::cluster::Core::rebaseCallback([[maybe_unused]] const uint16_t wid, const pid_t pid, const pid_t opid) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("rebase"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const pid_t)> ("rebase", pid, opid);
}
/**
 * exitCallback Метод события завершения работы процесса
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса
 * @param status статус остановки работы процесса
 */
void awh::cluster::Core::exitCallback([[maybe_unused]] const uint16_t wid, const pid_t pid, const int32_t status) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("exit"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const int32_t)> ("exit", pid, status);
}
/**
 * eventsCallback Метод информирования о статусе кластера
 * @param wid   идентификатор воркера
 * @param pid   идентификатор процесса
 * @param event идентификатор события
 */
void awh::cluster::Core::eventsCallback([[maybe_unused]] const uint16_t wid, const pid_t pid, const cluster_t::event_t event) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("events")){
		// Определяем члена семейства кластера
		switch(static_cast <uint8_t> (this->family())){
			// Если процесс является родительским
			case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
				// Если функция обратного вызова установлена
				if(this->_callback.is("clusterStatus"))
					// Выводим результат в отдельном потоке
					std::thread(this->_callback.get <void (const status_t)> ("clusterStatus"), status_t::START).detach();
			} break;
		}
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const cluster_t::family_t, const pid_t, const cluster_t::event_t)> ("events", this->family(), pid, event);
	}
}
/**
 * messageCallback Метод получения сообщения
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::messageCallback([[maybe_unused]] const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("message"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const cluster_t::family_t, const pid_t, const char *, const size_t)> ("message", this->family(), pid, buffer, size);
}
/**
 * family Меод получения семейства кластера
 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
 */
awh::cluster_t::family_t awh::cluster::Core::family() const noexcept {
	// Если мы работаем в режиме MASTER
	if(this->_cluster.master())
		// Устанавливаем семейство кластера
		return cluster_t::family_t::MASTER;
	// Устанавливаем семейство кластера
	return cluster_t::family_t::CHILDREN;
}
/**
 * pids Метод получения списка дочерних процессов
 * @return список дочерних процессов
 */
std::set <pid_t> awh::cluster::Core::pids() const noexcept {
	// Выполняем извлечение списка доступных идентификаторов процессов
	return this->_cluster.pids(0);
}
/**
 * emplace Метод размещения нового воркера
 */
void awh::cluster::Core::emplace() noexcept {
	// Выполняем добавление нового процесса
	this->_cluster.emplace(0);
}
/**
 * erase Метод удаления активного процесса
 * @param pid идентификатор процесса
 */
void awh::cluster::Core::erase(const pid_t pid) noexcept {
	// Выполняем удаление дочернего процесса
	this->_cluster.erase(0, pid);
}
/**
 * send Метод отправки сообщение родительскому процессу
 */
void awh::cluster::Core::send() const noexcept {
	// Выполняем отправку сообщения родительскому процессу
	const_cast <cluster_t &> (this->_cluster).send(0);
}
/**
 * send Метод отправки сообщение родительскому процессу
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::send(const char * buffer, const size_t size) const noexcept {
	// Выполняем отправку сообщения родительскому процессу
	const_cast <cluster_t &> (this->_cluster).send(0, buffer, size);
}
/**
 * send Метод отправки сообщение процессу
 * @param pid идентификатор процесса для отправки
 */
void awh::cluster::Core::send(const pid_t pid) const noexcept {
	// Выполняем отправку сообщения указанному процессу
	const_cast <cluster_t &> (this->_cluster).send(0, pid);
}
/**
 * send Метод отправки сообщение процессу
 * @param pid    идентификатор процесса для отправки
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::cluster::Core::send(const pid_t pid, const char * buffer, const size_t size) const noexcept {
	// Выполняем отправку сообщения указанному процессу
	const_cast <cluster_t &> (this->_cluster).send(0, pid, buffer, size);
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 */
void awh::cluster::Core::broadcast() const noexcept {
	// Выполняем отправку сообщения всем процессам
	const_cast <cluster_t &> (this->_cluster).broadcast(0);
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::cluster::Core::broadcast(const char * buffer, const size_t size) const noexcept {
	// Выполняем отправку сообщения всем процессам
	const_cast <cluster_t &> (this->_cluster).broadcast(0, buffer, size);
}
/**
 * stop Метод остановки клиента
 */
void awh::cluster::Core::stop() noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->family())){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
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
				if(this->_callback.is("clusterStatus"))
					// Выполняем получение функции обратного вызова
					this->_callback.set("clusterStatus", "status", this->_callback);
			// Выполняем разблокировку потока
			} else this->_mtx.status.unlock();
		} break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
			// Выполняем остановку работы кластера
			this->_cluster.stop(0);
		break;
	}
}
/**
 * start Метод запуска клиента
 */
void awh::cluster::Core::start() noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->family())){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
			// Выполняем блокировку потока
			this->_mtx.status.lock();
			// Если система ещё не запущена
			if(!this->_mode){
				// Разрешаем работу Websocket
				this->_mode = !this->_mode;
				// Выполняем разблокировку потока
				this->_mtx.status.unlock();
				// Если функция обратного вызова установлена
				if(this->_callback.is("status"))
					// Выполняем получение функции обратного вызова
					this->_callback.set("status", "clusterStatus", this->_callback);
				// Устанавливаем функцию обратного вызова на запуск системы
				this->_callback.on <void (const status_t)> ("status", &cluster::core_t::activeCallback, this, _1);
				// Выполняем запуск чтения базы событий
				this->_dispatch.start();
			// Выполняем разблокировку потока
			} else this->_mtx.status.unlock();
		} break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
			// Выводим сообщение об ошибке
			this->_log->print("It is not possible to start a cluster from a child process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::START, "It is not possible to start a cluster from a child process");
		} break;
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
 * name Метод установки названия кластера
 * @param name название кластера для установки
 */
void awh::cluster::Core::name(const string & name) noexcept {
	// Выполняем установку названия кластера
	this->_cluster.name(name);
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::cluster::Core::callback(const callback_t & callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функций обратного вызова
	awh::core_t::callback(callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса
	this->_callback.set("exit", callback);
	// Выполняем установку функции обратного вызова при подключения дочерних процессов
	this->_callback.set("ready", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса
	this->_callback.set("rebase", callback);
	// Выполняем установку функции обратного вызова при получении события
	this->_callback.set("events", callback);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callback.set("message", callback);
}
/**
 * size Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::cluster::Core::size(const uint16_t size) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Устанавливаем количество рабочих процессов кластера
		this->_size = size;
		// Выполняем инициализацию кластера
		this->_cluster.init(0, this->_size);
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OSBROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * autoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::cluster::Core::autoRestart(const bool mode) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Выполняем установку флага автоматического перезапуска убитых дочерних процессов
		this->_cluster.autoRestart(0, mode);
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OSBROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * salt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::cluster::Core::salt(const string & salt) noexcept {
	// Выполняем установку соли для шифрования
	this->_cluster.salt(salt);
}
/**
 * password Метод установки пароля шифрования
 * @param password пароль шифрования
 */
void awh::cluster::Core::password(const string & password) noexcept {
	// Выполняем установку пароля для шифрования
	this->_cluster.password(password);
}
/**
 * cipher Метод установки размера шифрования
 * @param cipher размер шифрования
 */
void awh::cluster::Core::cipher(const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку размера шифрования
	this->_cluster.cipher(cipher);
}
/**
 * compressor Метод установки метода компрессии
 * @param compressor метод компрессии для установки
 */
void awh::cluster::Core::compressor(const hash_t::method_t compressor) noexcept {
	// Выполняем установку метода компрессии
	this->_cluster.compressor(compressor);
}
/**
 * transfer Метод установки режима передачи данных
 * @param transfer режим передачи данных
 */
void awh::cluster::Core::transfer(const cluster_t::transfer_t transfer) noexcept {
	// Выполняем установку режима передачи данных
	this->_cluster.transfer(transfer);
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::cluster::Core::bandwidth(const string & read , const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	this->_cluster.bandwidth(read, write);
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::cluster::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), _size(1), _cluster(this, fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Выполняем инициализацию кластера
	this->_cluster.init(0, this->_size);
	// Устанавливаем функцию получения события подключения дочерних процессов
	this->_cluster.on <void (const uint16_t, const pid_t)> ("ready", &core_t::readyCallback, this, _1, _2);
	// Устанавливаем функцию получения события завершения работы процесса
	this->_cluster.on <void (const uint16_t, const pid_t, const int32_t)> ("exit", &core_t::exitCallback, this, _1, _2, _3);
	// Устанавливаем функцию получения события пересоздании процесса
	this->_cluster.on <void (const uint16_t, const pid_t, const pid_t)> ("rebase", &core_t::rebaseCallback, this, _1, _2, _3);
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.on <void (const uint16_t, const pid_t, cluster_t::event_t)> ("events", &core_t::eventsCallback, this, _1, _2, _3);
	// Устанавливаем функцию получения входящих сообщений
	this->_cluster.on <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", &core_t::messageCallback, this, _1, _2, _3, _4);
}
