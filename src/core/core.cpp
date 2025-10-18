/**
 * @file: core.cpp
 * @date: 2024-07-08
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
 * Стандартные модули
 */
#include <cstdlib>

/**
 * Подключаем заголовочный файл
 */
#include <core/core.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Объект глобальной базы событий
 */
std::unique_ptr <awh::base_t> awh::Core::Dispatch::_base;

/**
 * @brief Метод остановки чтения базы событий
 *
 */
void awh::Core::Dispatch::stop() noexcept {
	// Если чтение базы событий уже началось
	if(this->_work && this->_init && (this->_base != nullptr)){
		// Снимаем флаг работы модуля
		this->_work = !this->_work;
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Выполняем остановку базы событий
		this->_base->stop();
	// Если модуль не инициализирован
	} else if(!this->_init) {
		// Если функция обратного вызова установлена
		if(this->_closedown != nullptr)
			// Выполняем остановку функции активации базы событий
			this->_closedown(true, false);
	}
}
/**
 * @brief Метод запуска чтения базы событий
 *
 */
void awh::Core::Dispatch::start() noexcept {
	// Если чтение базы событий ещё не началось
	if(!this->_work && this->_init && (this->_base != nullptr)){
		// Устанавливаем флаг работы модуля
		this->_work = !this->_work;
		// Если функция обратного вызова установлена
		if(this->_launching != nullptr)
			// Выполняем запуск функции активации базы событий
			this->_launching(true, true);
		// Выполняем запуск базы событий
		this->_base->start();
		// Если функция обратного вызова установлена
		if(this->_closedown != nullptr)
			// Выполняем остановку функции активации базы событий
			this->_closedown(true, true);
	// Если модуль не инициализирован
	} else if(!this->_init) {
		// Если функция обратного вызова установлена
		if(this->_launching != nullptr)
			// Выполняем запуск функции активации базы событий
			this->_launching(true, false);
	}
}
/**
 * @brief Метод пересоздания базы событий
 *
 */
void awh::Core::Dispatch::rebase() noexcept {
	// Если требуется активировать базу событий как виртуальную
	if(!this->_virt){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если работа уже запущена
			if(this->_work)
				// Выполняем блокировку чтения данных
				this->_init = static_cast <bool> (this->_virt);
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Если база событий уже создана
			if(this->_base != nullptr)
				// Выполняем пересоздание базы событий
				this->_base->rebase();
			// Создаем новую базу событий
			else this->_base = std::make_unique <base_t> (this->_fmk, this->_log);
			// Выполняем разблокировку чтения данных
			this->_init = !this->_virt;
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод реинициализации базы событий
 *
 */
void awh::Core::Dispatch::reinit() noexcept {
	// Если требуется активировать базу событий как виртуальную
	if(!this->_virt){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если работа уже запущена
			if(this->_work)
				// Выполняем блокировку чтения данных
				this->_init = static_cast <bool> (this->_virt);
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Если база событий уже создана
			if(this->_base != nullptr)
				// Удаляем объект базы событий
				this->_base.reset(nullptr);
			// Создаем новую базу событий
			this->_base = std::make_unique <base_t> (this->_fmk, this->_log);
			// Выполняем разблокировку чтения данных
			this->_init = !this->_virt;
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод активации простого режима чтения базы событий
 *
 * @param mode флаг активации
 */
void awh::Core::Dispatch::easily(const bool mode) noexcept {
	// Если база событий инициализированна
	if(this->_base != nullptr){
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Устанавливаем флаг активации простого чтения базы событий
		this->_base->easily(mode);
	}
}
/**
 * @brief Метод установки времени блокировки базы событий в ожидании событий
 *
 * @param msec время ожидания событий в миллисекундах
 */
void awh::Core::Dispatch::rate(const uint8_t msec) noexcept {
	// Если база событий проинициализированна
	if(this->_init && (this->_base != nullptr)){
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Устанавливаем частоту обновления базы событий
		this->_base->rate(msec);
	}
}
/**
 * @brief Метод установки функции обратного вызова
 *
 * @param status   статус которому соответствует функция
 * @param callback функция обратного вызова
 */
void awh::Core::Dispatch::on(const status_t status, function <void (const bool, const bool)> callback) noexcept {
	/**
	 * Определяем статус которому соответствует функции
	 */
	switch(static_cast <uint8_t> (status)){
		// Если статус функции соответствует запуску базы событий
		case static_cast <uint8_t> (status_t::START):
			// Выполняем установку функции активации базы событий
			this->_launching = ::move(callback);
		break;
		// Если статус функции соответствует остановки базы событий
		case static_cast <uint8_t> (status_t::STOP):
			// Выполняем установку функции активации базы событий
			this->_closedown = ::move(callback);
		break;
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Dispatch::Dispatch(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _work(false), _init(false), _virt(false),
 _launching(nullptr), _closedown(nullptr), _fmk(fmk), _log(log) {
	// Если база событий ещё не проинициализированна
	if(!(this->_virt = (this->_base != nullptr)))
		// Выполняем инициализацию базы событий
		this->reinit();
}
/**
 * @brief Деструктор
 *
 */
awh::Core::Dispatch::~Dispatch() noexcept {
	// Если база событий проинициализированна
	if(this->_init){
		// Если база событий не является виртуальной
		if(!this->_virt && (this->_base != nullptr))
			// Удаляем объект базы событий
			this->_base.reset(nullptr);
	}
}
/**
 * @brief Метод вывода полученного сигнала
 *
 */
void awh::Core::signal(const int32_t signal) noexcept {
	// Если процесс является дочерним
	if(this->_pid != ::getpid()){
		/**
		 * Определяем тип сигнала
		 */
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
		if(this->_callback.is("crash"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const int32_t)> ("crash", signal);
		// Выходим из приложения
		else ::exit(signal);
	}
}
/**
 * @brief Метод пересоздания базы событий
 *
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
		if(this->_signals == scheme_t::mode_t::ENABLED)
			// Выполняем запуск отслеживания сигналов
			this->_sig.start();
		// Выполняем запуск работы
		this->start();
	}
}
/**
 * @brief Метод реинициализации базы событий
 *
 */
void awh::Core::reinit() noexcept {
	// Выполняем переинициализацию базы событий
	this->_dispatch.reinit();
}
/**
 * @brief Метод подключения модуля ядра к текущей базе событий
 *
 * @param core модуль ядра для подключения
 */
void awh::Core::bind(core_t & core) noexcept {
	// Если база событий активна и она отличается от текущей базы событий
	if(&core != this)
		// Выполняем запуск управляющей функции
		core.launching(false, true);
}
/**
 * @brief Метод отключения модуля ядра от текущей базы событий
 *
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(core_t & core) noexcept {
	// Если база событий активна и она совпадает с текущей базы событий
	if(&core != this)
		// Запускаем метод деактивации базы событий
		core.closedown(false, true);
}
/**
 * @brief Метод остановки клиента
 *
 */
void awh::Core::stop() noexcept {
	// Если система уже запущена
	if(this->_mode){
		// Запрещаем работу Websocket
		this->_mode = !this->_mode;
		// Выполняем остановку чтения базы событий
		this->_dispatch.stop();
	}
}
/**
 * @brief Метод запуска клиента
 *
 */
void awh::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->_mode){
		// Разрешаем работу Websocket
		this->_mode = !this->_mode;
		// Выполняем запуск чтения базы событий
		this->_dispatch.start();
	}
}
/**
 * @brief Метод вызова при активации базы событий
 *
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Core::launching([[maybe_unused]] const bool mode, const bool status) noexcept {
	// Если требуется изменить статус
	if(status && (this->_status != status_t::START)){
		// Устанавливаем статус сетевого ядра
		this->_status = status_t::START;
		// Если функция обратного вызова установлена
		if(this->_callback.is("status"))
			// Выполняем запуск функции в основном потоке
			this->_callback.call <void (const status_t)> ("status", this->_status);
		// Если разрешено выводить информацию в лог
		if(this->_info)
			// Выводим в консоль информацию
			this->_log->print("[+] Start service: PID=%u", log_t::flag_t::INFO, ::getpid());
	}
}
/**
 * @brief Метод вызова при деакцтивации базы событий
 *
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Core::closedown([[maybe_unused]] const bool mode, const bool status) noexcept {
	// Если требуется изменить статус
	if(status && (this->_status != status_t::STOP)){
		// Устанавливаем статус сетевого ядра
		this->_status = status_t::STOP;
		// Если функция обратного вызова установлена
		if(this->_callback.is("status"))
			// Выполняем запуск функции в основном потоке
			this->_callback.call <void (const status_t)> ("status", this->_status);
		// Если разрешено выводить информацию в лог
		if(this->_info)
			// Выводим в консоль информацию
			this->_log->print("[-] Stop service: PID=%u", log_t::flag_t::INFO, ::getpid());
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::Core::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова при краше приложения
	this->_callback.set("crash", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова при запуске/остановки работы модуля
	this->_callback.set("status", callback);
}
/**
 * @brief Метод проверки на запуск работы
 *
 * @return результат проверки
 */
bool awh::Core::working() const noexcept {
	// Выводим результат проверки
	return this->_mode;
}
/**
 * @brief Метод получения базы событий
 *
 * @return инициализированная база событий
 */
awh::base_t * awh::Core::base() noexcept {
	// Выполняем получение базы событий
	return this->_dispatch._base.get();
}
/**
 * @brief Метод активации простого режима чтения базы событий
 *
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
 * @brief Метод установки флага запрета вывода информационных сообщений
 *
 * @param mode флаг запрета вывода информационных сообщений
 */
void awh::Core::verbose(const bool mode) noexcept {
	// Устанавливаем флаг запрета вывода информационных сообщений
	this->_info = mode;
}
/**
 * @brief Метод установки времени блокировки базы событий в ожидании событий
 *
 * @param msec время ожидания событий в миллисекундах
 */
void awh::Core::rate(const uint8_t msec) noexcept {
	// Устанавливаем частоту чтения базы событий
	this->_dispatch.rate(msec);
}
/**
 * @brief Метод активации перехвата сигналов
 *
 * @param mode флаг активации
 */
void awh::Core::signalInterception(const scheme_t::mode_t mode) noexcept {
	// Если флаг активации отличается
	if(this->_signals != mode){
		// Устанавливаем флаг активации перехвата сигналов
		this->_signals = mode;
		/**
		 * Определяем флаг активации
		 */
		switch(static_cast <uint8_t> (mode)){
			// Если передан флаг активации перехвата сигналов
			case static_cast <uint8_t> (scheme_t::mode_t::ENABLED):
				// Устанавливаем функцию обработки сигналов
				this->_sig.on(std::bind(&core_t::signal, this, _1));
				// Выполняем запуск отслеживания сигналов
				this->_sig.start();
			break;
			// Если передан флаг деактивации перехвата сигналов
			case static_cast <uint8_t> (scheme_t::mode_t::DISABLED):
				// Выполняем остановку отслеживания сигналов
				this->_sig.stop();
			break;
		}
	}
}
/**
 * @brief Метод отправки события через потоки
 *
 * @param id  идентификатор события для отправки
 * @param tid идентификатор трансферной передачи
 * @return    результат отправки события
 */
bool awh::Core::trigger(const uint32_t id, const uint32_t tid) noexcept {
	// Если база событий инициализированна
	if(this->_dispatch._base != nullptr)
		// Выполняем отправку сообщения
		this->_dispatch._base->trigger(id, tid);
	// Выводим значение по умолчанию
	return false;
}
/**
 * @brief Метод отмены регистрации события
 *
 * @param id идентификатор события
 * @return   результат отмены регистрации события
 */
bool awh::Core::detach(const uint32_t id) noexcept {
	// Если база событий инициализированна
	if(this->_dispatch._base != nullptr)
		// Выполняем деактивации межпотокового передатчика
		this->_dispatch._base->detach(id);
	// Выводим значение по умолчанию
	return false;
}
/**
 * @brief Метод регистрации нового события
 *
 * @param callback функция обратного вызова
 * @return         идентификатор события
 */
uint32_t awh::Core::attach(function <void (const uint32_t)> callback) noexcept {
	// Если база событий инициализированна
	if(this->_dispatch._base != nullptr)
		// Выполняем активации межпотокового передатчика
		return this->_dispatch._base->attach(callback);
	// Выводим значение по умолчанию
	return 0;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _sig(fmk, log),
 _dispatch(fmk, log), _callback(log),
 _mode(false), _info(true),
 _status(status_t::STOP), _type(engine_t::type_t::NONE),
 _signals(scheme_t::mode_t::DISABLED), _fmk(fmk), _log(log) {
	// Выполняем установку функции активации базы событий
	this->_dispatch.on(status_t::START, std::bind(&awh::Core::launching, this, _1, _2));
	// Выполняем установку функции деактивации базы событий
	this->_dispatch.on(status_t::STOP, std::bind(&awh::Core::closedown, this, _1, _2));
}
/**
 * @brief Деструктор
 *
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
