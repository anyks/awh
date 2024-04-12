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
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::openCallback(const uint16_t sid) noexcept {
	// Если данные переданы верные
	if(sid > 0){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::READ, event_t::CONNECT}, event_t::OPEN))
			// Выполняем подключение
			const_cast <client::core_t *> (this->_core)->open(this->_scheme.id);
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::client::Sample::eventsCallback(const awh::core_t::status_t status) noexcept {
	// Если функция получения событий запуска и остановки сетевого ядра установлена
	if(this->_callbacks.is("status"))
		// Выводим функцию обратного вызова
		this->_callbacks.call <void (const awh::core_t::status_t)> ("status", status);
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::connectCallback(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::READ}, event_t::CONNECT)){
			// Запоминаем идентификатор брокера
			this->_bid = bid;
			// Если функция обратного вызова существует
			if(this->_callbacks.is("active"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const mode_t)> ("active", mode_t::CONNECT);
		}
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::disconnectCallback(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if(sid > 0){
		// Если подключение не является постоянным
		if(!this->_scheme.alive){
			// Очищаем адрес сервера
			this->_scheme.url.clear();
			// Завершаем работу базы событий
			if(this->_complete)
				// Выполняем остановку работы сетевого ядра
				const_cast <client::core_t *> (this->_core)->stop();
		}
		// Если функция обратного вызова существует
		if(this->_callbacks.is("active"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const mode_t)> ("active", mode_t::DISCONNECT);
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Sample::readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT}, event_t::READ)){
			// Если функция обратного вызова существует
			if(this->_callbacks.is("message"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const vector <char> &)> ("message", vector <char> (buffer, buffer + size));
		}
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
	else const_cast <client::core_t *> (this->_core)->open(this->_scheme.id);
}
/**
 * close Метод закрытия подключения клиента
 */
void awh::client::Sample::close() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->_core)->close(this->_bid);
}
/**
 * init Метод инициализации Rest брокера
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
			const_cast <client::core_t *> (this->_core)->sockname(socket);
		#endif
	}
}
/**
 * init Метод инициализации Rest брокера
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
			// Если хост является доменом или IPv4-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Устанавливаем IP адрес
				this->_scheme.url.ip = host;
			break;
			// Если хост является IPv6-адресом, переводим IP-адрес в полную форму
			case static_cast <uint8_t> (net_t::type_t::IPV6): {
				// Создаём объкт для работы с адресами
				net_t net{};
				// Устанавливаем IP-адрес
				this->_scheme.url.ip = net = host;
			} break;
			// Если хост является доменной зоной
			case static_cast <uint8_t> (net_t::type_t::FQDN):
				// Устанавливаем доменное имя
				this->_scheme.url.domain = host;
			break;
		}
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::Sample::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова при подключении/отключении
	this->_callbacks.set("active", callbacks);
	// Выполняем установку функции обратного вызова при получения событий запуска и остановки сетевого ядра
	this->_callbacks.set("status", callbacks);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callbacks.set("message", callbacks);
}
/**
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Sample::cork(const engine_t::mode_t mode) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем отключение/включение алгоритма TCP/CORK
		return const_cast <client::core_t *> (this->_core)->cork(this->_bid, mode);
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Sample::nodelay(const engine_t::mode_t mode) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем отключение/включение алгоритма Нейгла
		return const_cast <client::core_t *> (this->_core)->nodelay(this->_bid, mode);
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * response Метод отправки сообщения брокеру
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::client::Sample::send(const char * buffer, const size_t size) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
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
			const_cast <client::core_t *> (this->_core)->write(buffer, size, this->_bid);
		}
	}
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::Sample::bandwidth(const string & read, const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	const_cast <client::core_t *> (this->_core)->bandwidth(this->_bid, read, write);
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
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Sample::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
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
 _callbacks(log), _scheme(fmk, log), _bid(0), _complete(true), _fmk(fmk), _log(log), _core(core) {
	// Добавляем схему сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->scheme(&this->_scheme);
	// Устанавливаем событие на запуск системы
	const_cast <client::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&sample_t::openCallback, this, _1));
	// Устанавливаем событие подключения
	const_cast <client::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&sample_t::connectCallback, this, _1, _2));
	// Устанавливаем событие отключения
	const_cast <client::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&sample_t::disconnectCallback, this, _1, _2));
	// Устанавливаем функцию чтения данных
	const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&sample_t::readCallback, this, _1, _2, _3, _4));
	// Выполняем установку функций обратного вызова для клиента
	const_cast <client::core_t *> (this->_core)->callback <void (const awh::core_t::status_t)> ("status", std::bind(&sample_t::eventsCallback, this, _1));
}
