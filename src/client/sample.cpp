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
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Sample::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->_action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->_action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::Sample::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если функция обратного вызова установлена
		if(this->_callback.events != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.events(status, core);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Sample::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
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
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Sample::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (core != nullptr)){
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
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Sample::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
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
			switch(static_cast <uint8_t> (this->_action)){
				// Если необходимо запустить экшен открытия подключения
				case static_cast <uint8_t> (action_t::OPEN): this->actionOpen(); break;
				// Если необходимо запустить экшен обработки данных поступающих с сервера
				case static_cast <uint8_t> (action_t::READ): this->actionRead(); break;
				// Если необходимо запустить экшен обработки подключения к серверу
				case static_cast <uint8_t> (action_t::CONNECT): this->actionConnect(); break;
				// Если необходимо запустить экшен обработки отключения от сервера
				case static_cast <uint8_t> (action_t::DISCONNECT): this->actionDisconnect(); break;
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
	const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
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
	// Если подключение является постоянным
	if(this->_scheme.alive){
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	// Если подключение не является постоянным
	} else {
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
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
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Sample::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Очищаем адрес сервера
		this->_scheme.url.clear();
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
	else const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
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
		// Создаём объект работы с URI ссылками
		uri_t uri(this->_fmk);
		// Устанавливаем URL адрес запроса (как заглушка)
		this->_scheme.url = uri.parse("http://unixsocket");
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокет
			const_cast <client::core_t *> (this->_core)->unixSocket(socket);
		#endif
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
		// Устанавливаем порт сервера
		this->_scheme.url.port = port;
		// Устанавливаем хост сервера
		this->_scheme.url.host = host;
		// Определяем тип передаваемого сервера
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если хост является доменом или IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Устанавливаем IP адрес
				this->_scheme.url.ip = host;
			break;
			// Если хост является IPv6 адресом, переводим ip адрес в полную форму
			case static_cast <uint8_t> (net_t::type_t::IPV6): {
				// Создаём объкт для работы с адресами
				net_t net{};
				// Получаем данные хоста
				net = host;
				// Устанавливаем IP адрес
				this->_scheme.url.ip = net;
			} break;
			// Если хост является доменным именем
			case static_cast <uint8_t> (net_t::type_t::DOMN):
				// Устанавливаем доменное имя
				this->_scheme.url.domain = host;
			break;
		}
	}
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Удаляем unix-сокет ранее установленный
		const_cast <client::core_t *> (this->_core)->removeUnixSocket();
	#endif
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
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::Sample::on(function <void (const awh::core_t::status_t status, awh::core_t * core)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.events = callback;
}
/**
 * response Метод отправки сообщения адъютанту
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::client::Sample::send(const char * buffer, const size_t size) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если включён режим отладки
		#if defined(DEBUG_MODE)
			// Выводим заголовок ответа
			cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
			// Выводим параметры ответа
			cout << string(buffer, size) << endl;
		#endif
		// Отправляем тело на сервер
		((awh::core_t *) const_cast <client::core_t *> (this->_core))->write(buffer, size, this->_aid);
	}
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::Sample::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_scheme.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_scheme.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_scheme.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_scheme.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_scheme.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_scheme.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Sample::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::Sample::mode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = !(flag & static_cast <uint8_t> (flag_t::NOT_STOP));
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flag & static_cast <uint8_t> (flag_t::ALIVE));
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & static_cast <uint8_t> (flag_t::WAIT_MESS));
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flag & static_cast <uint8_t> (flag_t::NOT_INFO));
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flag & static_cast <uint8_t> (flag_t::VERIFY_SSL));
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Sample::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * Sample Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Sample::Sample(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _scheme(fmk, log), _action(action_t::NONE),
 _aid(0), _unbind(true), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&sample_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&sample_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&sample_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&sample_t::readCallback, this, _1, _2, _3, _4, _5));
	// Добавляем схему сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем функцию активации ядра клиента
	const_cast <client::core_t *> (this->_core)->on(std::bind(&sample_t::eventsCallback, this, _1, _2));
}
