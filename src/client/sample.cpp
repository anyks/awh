/**
 * @file: sample.cpp
 * @date: 2022-09-01
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
#include <client/sample.hpp>

/**
 * openCallback Метод обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::Sample::openCallback(const size_t wid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->_action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->_action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::Sample::connectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::client::Sample::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr)){
		// Устанавливаем экшен выполнения
		this->_action = action_t::DISCONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::client::Sample::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->_action == action_t::NONE) || (this->_action == action_t::READ)){
			// Устанавливаем экшен выполнения
			this->_action = action_t::READ;
			// Добавляем полученные данные в буфер
			this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
			// Выполняем запуск обработчика событий
			this->handler();
		}
	}
}
/**
 * handler Метод управления входящими методами
 */
void awh::client::Sample::handler() noexcept {
	// Если управляющий блокировщик не заблокирован
	if(!this->_locker.mode){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_locker.mtx);
		// Флаг разрешающий циклический перебор экшенов
		bool loop = true;
		// Выполняем блокировку обработчика
		this->_locker.mode = true;
		// Выполняем обработку всех экшенов
		while(loop && (this->_action != action_t::NONE)){
			// Определяем обрабатываемый экшен
			switch((uint8_t) this->_action){
				// Если необходимо запустить экшен открытия подключения
				case (uint8_t) action_t::OPEN: this->actionOpen(); break;
				// Если необходимо запустить экшен обработки данных поступающих с сервера
				case (uint8_t) action_t::READ: this->actionRead(); break;
				// Если необходимо запустить экшен обработки подключения к серверу
				case (uint8_t) action_t::CONNECT: this->actionConnect(); break;
				// Если необходимо запустить экшен обработки отключения от сервера
				case (uint8_t) action_t::DISCONNECT: this->actionDisconnect(); break;
				// Если сработал неизвестный экшен, выходим
				default: loop = false;
			}
		}
		// Выполняем разблокировку обработчика
		this->_locker.mode = false;
	}
}
/**
 * actionOpen Метод обработки экшена открытия подключения
 */
void awh::client::Sample::actionOpen() noexcept {
	// Выполняем подключение
	const_cast <client::core_t *> (this->_core)->open(this->_worker.wid);
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::OPEN)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::Sample::actionRead() noexcept {
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::READ)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	// Если функция обратного вызова установлена, выводим сообщение
	if(this->_callback.message != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.message(this->_buffer, this);
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::Sample::actionConnect() noexcept {
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
	// Создаём текст сообщения для сервера
	const string message = "Hello World!!!";
	/**
	 * Если включён режим отладки
	 */
	#if defined(DEBUG_MODE)
		// Выводим заголовок запроса
		cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
		// Выводим параметры запроса
		cout << message << endl << endl;
	#endif
	// Выполняем отправку заголовков запроса на сервер
	const_cast <client::core_t *> (this->_core)->write(message.data(), message.size(), this->_aid);	
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	// Если функция обратного вызова существует
	if(this->_callback.active != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.active(mode_t::CONNECT, this);
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::Sample::actionDisconnect() noexcept {
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
	// Очищаем адрес сервера
	this->_worker.url.clear();
	// Завершаем работу
	if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::DISCONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	// Если функция обратного вызова существует
	if(this->_callback.active != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.active(mode_t::DISCONNECT, this);
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Sample::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Очищаем адрес сервера
		this->_worker.url.clear();
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->_core)->stop();
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::Sample::start() noexcept {
	// Если биндинг не запущен
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <client::core_t *> (this->_core)->start();
	// Если биндинг уже запущен, выполняем запрос на сервер
	else const_cast <client::core_t *> (this->_core)->open(this->_worker.wid);
}
/**
 * close Метод закрытия подключения клиента
 */
void awh::client::Sample::close() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->_core)->close(this->_aid);
}
/**
 * init Метод инициализации Rest адъютанта
 * @param socket unix-сокет для биндинга
 */
void awh::client::Sample::init(const string & socket) noexcept {
	// Если unix-сокет передан
	if(!socket.empty()){
		// Устанавливаем хост сервера
		this->_worker.url.host = socket;
		// Устанавливаем тип сети
		this->_worker.url.family = AF_UNIX;
	}
}
/**
 * init Метод инициализации Rest адъютанта
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::client::Sample::init(const u_int port, const string & host) noexcept {
	// Если параметры подключения переданы
	if((port > 0) && !host.empty()){
		// Создаём объект работы с URI ссылками
		uri_t uri(this->_fmk, &this->_nwk);
		// Получаем новый адрес запроса для воркера
		this->_worker.url = uri.parse(this->_fmk->format("https://%s:%u", host.c_str(), port));
	}
}
/**
 * on Метод установки функции обратного вызова при подключении/отключении
 * @param callback функция обратного вызова
 */
void awh::client::Sample::on(function <void (const mode_t, Sample *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.active = callback;
}
/**
 * setMessageCallback Метод установки функции обратного вызова при получении сообщения
 * @param callback функция обратного вызова
 */
void awh::client::Sample::on(function <void (const vector <char> &, Sample *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.message = callback;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::Sample::bytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_worker.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_worker.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_worker.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_worker.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_worker.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_worker.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Sample::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_worker.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_worker.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_worker.timeouts.connect = connect;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::Sample::mode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Sample::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_worker.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_worker.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_worker.keepAlive.intvl = intvl;
}
/**
 * Sample Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Sample::Sample(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _nwk(fmk), _worker(fmk, log), _action(action_t::NONE),
 _aid(0), _unbind(true), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_worker.callback.open = std::bind(&Sample::openCallback, this, _1, _2);
	// Устанавливаем событие подключения
	this->_worker.callback.connect = std::bind(&Sample::connectCallback, this, _1, _2, _3);
	// Устанавливаем функцию чтения данных
	this->_worker.callback.read = std::bind(&Sample::readCallback, this, _1, _2, _3, _4, _5);
	// Устанавливаем событие отключения
	this->_worker.callback.disconnect = std::bind(&Sample::disconnectCallback, this, _1, _2, _3);
	// Добавляем воркер в биндер TCP/IP
	const_cast <client::core_t *> (this->_core)->add(&this->_worker);
}
