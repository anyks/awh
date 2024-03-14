/**
 * @file: core.cpp
 * @date: 2024-03-08
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
#include <lib/ev/scheme/core.hpp>

/**
 * method Метод установки метода режима работы
 * @param method метод режима работы
 */
void awh::Scheme::Timeout::method(const engine_t::method_t method) noexcept {
	// Выполняем установку метода режима работы
	this->_method = method;
}
/**
 * operator Оператор [()] Получения события таймера
 * @param timer   объект события таймаута
 * @param revents идентификатор события
 */
void awh::Scheme::Timeout::operator()(ev::timer & timer, int revents) noexcept {
	// Зануляем неиспользуемые переменные
	(void) revents;
	// Выполняем остановку таймера
	timer.stop();
	// Выполняем функцию таймаута
	this->_broker->timeout(this->_method);
}
/**
 * id Метод извлечения идентификатора брокера
 * @return идентификатор брокера
 */
uint64_t awh::Scheme::Broker::id() const noexcept {
	// Извлекаем идентификатор брокера
	return this->_id;
}
/**
 * sid Метод извлечения идентификатора схемы сети
 * @return идентификатор схемы сети
 */
uint16_t awh::Scheme::Broker::sid() const noexcept {
	// Извлекаем идентификатор схемы сети
	return this->_sid;
}
/**
 * port Метод извлечения порта подключения
 * @return установленный порт подключения
 */
u_int awh::Scheme::Broker::port() const noexcept {
	// Извлекаем установленный порт сервера
	return this->_port;
}
/**
 * port Метод установки порта подключения
 * @param порт подключения для установки
 */
void awh::Scheme::Broker::port(const u_int port) noexcept {
	// Если порт передан
	if(port > 0)
		// Выполняем установку порта
		this->_port = port;
}
/**
 * ip Метод извлечения IP-адреса
 * @return установленный IP-адрес
 */
const string & awh::Scheme::Broker::ip() const noexcept {
	// Выводим установленный IP-адрес
	return this->_ip;
}
/**
 * ip Метод установки IP-адреса
 * @param ip адрес для установки
 */
void awh::Scheme::Broker::ip(const string & ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty())
		// Выполняем установку IP-адреса
		this->_ip = ip;
}
/**
 * mac Метод извлечения MAC-адреса
 * @return установленный MAC-адрес
 */
const string & awh::Scheme::Broker::mac() const noexcept {
	// Выполняем извлечение MAC-адреса
	return this->_mac;
}
/**
 * mac Метод установки MAC-адреса
 * @param mac адрес для установки
 */
void awh::Scheme::Broker::mac(const string & mac) noexcept {
	// Если MAC-адрес передан
	if(!mac.empty())
		// Выполняем установку MAC-адреса
		this->_mac = mac;
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::Scheme::Broker::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова при активации сокета на чтение
	this->_callbacks.set("read", callbacks);
	// Выполняем установку функции обратного вызова при готовности сокета к записи
	this->_callbacks.set("write", callbacks);
	// Выполняем установку функции обратного вызова при выполнении подключения к серверу
	this->_callbacks.set("connect", callbacks);
	// Выполняем установку функции обратного вызова при срабатывании таймаута
	this->_callbacks.set("timeout", callbacks);
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения
 */
void awh::Scheme::Broker::sonet(const sonet_t sonet) noexcept {
	// Выполняем установку типа сокета подключения
	this->_sonet = sonet;
}
/**
 * read Метод вызова при чтении данных с сокета
 * @param watcher объект события чтения
 * @param revents идентификатор события
 */
void awh::Scheme::Broker::read(ev::io & watcher, int revents) noexcept {
	// Зануляем неиспользуемые переменные
	(void) watcher;
	(void) revents;
	// Если разрешено выполнять чтения данных из сокета и функция обратного вызова установлена
	if(!this->_bev.locked.read && this->_callbacks.is("read"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t)> ("read", this->_id);
}
/**
 * write Метод вызова при активации сокета на запись данных
 * @param watcher объект события чтения
 * @param revents идентификатор события
 */
void awh::Scheme::Broker::write(ev::io & watcher, int revents) noexcept {
	// Зануляем неиспользуемые переменные
	(void) watcher;
	(void) revents;
	// Если разрешено выполнять запись данных в сокет и функция обратного вызова установлена
	if(!this->_bev.locked.write && this->_callbacks.is("write"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t)> ("write", this->_id);
}
/**
 * accept Метод вызова при подключении к серверу
 * @param watcher объект события чтения
 * @param revents идентификатор события
 */
void awh::Scheme::Broker::accept(ev::io & watcher, int revents) noexcept {
	// Зануляем неиспользуемые переменные
	(void) revents;
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("accept"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const SOCKET, const uint16_t)> ("accept", static_cast <SOCKET> (watcher.fd), this->_sid);
}
/**
 * connect Метод вызова при подключении к серверу
 * @param watcher объект события подключения
 * @param revents идентификатор события
 */
void awh::Scheme::Broker::connect(ev::io & watcher, int revents) noexcept {
	// Зануляем неиспользуемые переменные
	(void) revents;
	// Выполняем остановку чтения
	watcher.stop();
	// Останавливаем ожидания события подключения
	this->events(mode_t::DISABLED, engine_t::method_t::CONNECT);
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("connect"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t)> ("connect", this->_id);
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param method метод режима работы
 */
void awh::Scheme::Broker::timeout(engine_t::method_t method) noexcept {
	// Останавливаем событие таймера чтения данных
	this->_bev.timers.read.stop();
	// Останавливаем событие таймера записи данных
	this->_bev.timers.write.stop();
	// Останавливаем событие таймера коннекта
	this->_bev.timers.connect.stop();
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("timeout"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t, const engine_t::method_t)> ("timeout", this->_id, method);
}
/**
 * lockup Метод блокировки метода режима работы
 * @param mode   флаг блокировки метода
 * @param method метод режима работы
 */
void awh::Scheme::Broker::lockup(const mode_t mode, const engine_t::method_t method) noexcept {
	// Определяем метод режима работы
	switch(static_cast <uint8_t> (method)){
		// Режим работы ЧТЕНИЕ
		case static_cast <uint8_t> (engine_t::method_t::READ):
			// Запрещаем получение данных
			this->_bev.locked.read = (mode == mode_t::DISABLED);
		break;
		// Режим работы ЗАПИСЬ
		case static_cast <uint8_t> (engine_t::method_t::WRITE):
			// Запрещаем чтение данных
			this->_bev.locked.write = (mode == mode_t::DISABLED);
		break;
	}
}
/**
 * events Метод активации/деактивации метода события сокета
 * @param mode   сигнал активации сокета
 * @param method метод режима работы
 */
void awh::Scheme::Broker::events(const mode_t mode, const engine_t::method_t method) noexcept {
	// Если сокет подключения активен и база событий установлена и активна
	if((this->_addr.fd != INVALID_SOCKET) && (this->_addr.fd < MAX_SOCKETS) && (this->_base != nullptr)){
		// Определяем метод события сокета
		switch(static_cast <uint8_t> (method)){
			// Если передано событие подписки на чтение
			case static_cast <uint8_t> (engine_t::method_t::READ): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Разрешаем чтение данных из сокета
						this->_bev.locked.read = false;
						// Устанавливаем приоритет выполнения для события на чтение
						ev_set_priority(&this->_bev.events.read, -2);
						// Устанавливаем базу событий
						this->_bev.events.read.set(this->_base);
						// Устанавливаем сокет для чтения
						this->_bev.events.read.set(this->_addr.fd, ev::READ);
						// Устанавливаем событие на чтение данных подключения
						this->_bev.events.read.set <awh::scheme_t::broker_t, &awh::scheme_t::broker_t::read> (this);
						// Запускаем чтение данных
						this->_bev.events.read.start();
						// Если флаг ожидания входящих сообщений, активирован
						if(this->_timeouts.read > 0){
							// Определяем тип активного сокета
							switch(static_cast <uint8_t> (this->_sonet)){
								// Если тип сокета установлен как UDP
								case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
								// Если тип сокета установлен как DTLS
								case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
									// Выполняем установку таймаута ожидания
									this->_ectx.timeout(this->_timeouts.read * 1000, engine_t::method_t::READ);
								break;
								// Для всех остальных протоколов
								default: {
									// Устанавливаем приоритет выполнения для таймаута на чтение
									ev_set_priority(&this->_bev.timers.read, 0);
									// Устанавливаем базу событий
									this->_bev.timers.read.set(this->_base);
									// Устанавливаем событие на таймаут чтения данных из сокета
									this->_bev.timers.read.set(&this->_readEventTimeout);
									// Запускаем ожидание чтения данных
									this->_bev.timers.read.start(static_cast <float> (this->_timeouts.read));
								}
							}
						}
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED): {
						// Запрещаем чтение данных из сокета
						this->_bev.locked.read = true;
						// Останавливаем работу таймера
						this->_bev.timers.read.stop();
						// Останавливаем работу события
						this->_bev.events.read.stop();
					} break;
				}
			} break;
			// Если передано событие подписки на запись
			case static_cast <uint8_t> (engine_t::method_t::WRITE): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Разрешаем запись данных из сокета
						this->_bev.locked.write = false;
						// Устанавливаем приоритет выполнения для события на запись
						ev_set_priority(&this->_bev.events.write, -2);
						// Устанавливаем базу событий
						this->_bev.events.write.set(this->_base);
						// Устанавливаем сокет для записи
						this->_bev.events.write.set(this->_addr.fd, ev::WRITE);
						// Устанавливаем событие на запись данных подключения
						this->_bev.events.write.set <awh::scheme_t::broker_t, &awh::scheme_t::broker_t::write> (this);
						// Запускаем запись данных
						this->_bev.events.write.start();
						// Если флаг ожидания входящих сообщений, активирован
						if(this->_timeouts.write > 0){
							// Определяем тип активного сокета
							switch(static_cast <uint8_t> (this->_sonet)){
								// Если тип сокета установлен как UDP
								case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
								// Если тип сокета установлен как DTLS
								case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
									// Выполняем установку таймаута ожидания
									this->_ectx.timeout(this->_timeouts.write * 1000, engine_t::method_t::WRITE);
								break;
								// Для всех остальных протоколов
								default: {
									// Устанавливаем приоритет выполнения для таймаута на запись
									ev_set_priority(&this->_bev.timers.write, 0);
									// Устанавливаем базу событий
									this->_bev.timers.write.set(this->_base);
									// Устанавливаем событие на таймаут записи данных в сокет
									this->_bev.timers.write.set(&this->_writeEventTimeout);
									// Запускаем ожидание записи данных
									this->_bev.timers.write.start(static_cast <float> (this->_timeouts.write));
								}
							}
						}
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED): {
						// Запрещаем записи данных в сокет
						this->_bev.locked.write = true;
						// Останавливаем работу таймера
						this->_bev.timers.write.stop();
						// Останавливаем работу события
						this->_bev.events.write.stop();
					} break;
				}
			} break;
			// Если передано событие подписки на запросы подключения
			case static_cast <uint8_t> (engine_t::method_t::ACCEPT): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Устанавливаем приоритет выполнения для события
						ev_set_priority(&this->_bev.events.accept, -2);
						// Устанавливаем базу событий
						this->_bev.events.accept.set(this->_base);
						// Устанавливаем сокет для чтения
						this->_bev.events.accept.set(this->_addr.fd, ev::READ);
						// Устанавливаем функцию обратного вызова
						this->_bev.events.accept.set <awh::scheme_t::broker_t, &awh::scheme_t::broker_t::accept> (this);
						// Выполняем запуск работы события
						this->_bev.events.accept.start();
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED):
						// Останавливаем работу события
						this->_bev.events.accept.stop();
					break;
				}
			} break;
			// Если передано событие подписки на подключение
			case static_cast <uint8_t> (engine_t::method_t::CONNECT): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Устанавливаем приоритет выполнения для события на чтения
						ev_set_priority(&this->_bev.events.connect, -2);
						// Устанавливаем базу событий
						this->_bev.events.connect.set(this->_base);
						// Устанавливаем сокет для записи
						this->_bev.events.connect.set(this->_addr.fd, ev::WRITE);
						// Устанавливаем событие подключения
						this->_bev.events.connect.set <awh::scheme_t::broker_t, &awh::scheme_t::broker_t::connect> (this);
						// Выполняем запуск подключения
						this->_bev.events.connect.start();
						// Если время ожидания записи данных установлено
						if(this->_timeouts.connect > 0){
							// Устанавливаем приоритет выполнения для таймаута на подключение
							ev_set_priority(&this->_bev.timers.connect, 0);
							// Устанавливаем базу событий
							this->_bev.timers.connect.set(this->_base);
							// Устанавливаем событие на подключение к серверу
							this->_bev.timers.connect.set(&this->_connectEventTimeout);
							// Запускаем запись данных на сервер
							this->_bev.timers.connect.start(static_cast <float> (this->_timeouts.connect));
						}
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED): {
						// Останавливаем работу таймера
						this->_bev.timers.connect.stop();
						// Останавливаем работу события
						this->_bev.events.connect.stop();
					} break;
				}
			} break;
			// Если передано событие подписки на таймаут таймера
			case static_cast <uint8_t> (engine_t::method_t::TIMEOUT): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Устанавливаем приоритет выполнения для события
						ev_set_priority(&this->_bev.events.timeout, -2);
						// Устанавливаем базу событий
						this->_bev.events.timeout.set(this->_base);
						// Устанавливаем функцию обратного вызова
						this->_bev.events.timeout.set(&this->_timeoutEventTimeout);
						// Запускаем запись данных на сервер
						this->_bev.events.timeout.start(this->_timeouts.timeout / static_cast <float> (1000));
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED):
						// Останавливаем работу события
						this->_bev.events.timeout.stop();
					break;
				}
			} break;
		}
	}
}
/**
 * timeout Метод установки таймаута ожидания появления данных
 * @param seconds время ожидания в секундах
 * @param method  метод режима работы
 */
void awh::Scheme::Broker::timeout(const time_t seconds, const engine_t::method_t method) noexcept {
	// Определяем метод режима работы
	switch(static_cast <uint8_t> (method)){
		// Режим работы ЧТЕНИЕ
		case static_cast <uint8_t> (engine_t::method_t::READ):
			// Устанавливаем время ожидания на входящие данные
			this->_timeouts.read = seconds;
		break;
		// Режим работы ЗАПИСЬ
		case static_cast <uint8_t> (engine_t::method_t::WRITE):
			// Устанавливаем время ожидания на исходящие данные
			this->_timeouts.write = seconds;
		break;
		// Режим работы ПОДКЛЮЧЕНИЕ
		case static_cast <uint8_t> (engine_t::method_t::CONNECT):
			// Устанавливаем время ожидания на исходящие данные
			this->_timeouts.connect = seconds;
		break;
	}
}
/**
 * marker Метод установки маркера на размер детектируемых байт
 * @param min    минимальный размер детектируемых байт
 * @param min    максимальный размер детектируемых байт
 * @param method метод режима работы
 */
void awh::Scheme::Broker::marker(const size_t min, const size_t max, const engine_t::method_t method) noexcept {
	// Определяем метод режима работы
	switch(static_cast <uint8_t> (method)){
		// Режим работы ЧТЕНИЕ
		case static_cast <uint8_t> (engine_t::method_t::READ): {
			// Устанавливаем минимальный размер байт
			this->_marker.read.min = min;
			// Устанавливаем максимальный размер байт
			this->_marker.read.max = max;
			// Если минимальный размер данных для чтения, не установлен
			if(this->_marker.read.min == 0)
				// Устанавливаем размер минимальных для чтения данных по умолчанию
				this->_marker.read.min = BUFFER_READ_MIN;
		} break;
		// Режим работы ЗАПИСЬ
		case static_cast <uint8_t> (engine_t::method_t::WRITE): {
			// Устанавливаем минимальный размер байт
			this->_marker.write.min = min;
			// Устанавливаем максимальный размер байт
			this->_marker.write.max = max;
			// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
			if(this->_marker.write.max == 0)
				// Устанавливаем размер максимальных записываемых данных по умолчанию
				this->_marker.write.max = BUFFER_WRITE_MAX;
		} break;
	}
}
/**
 * base Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Scheme::Broker::base(struct ev_loop * base) noexcept {
	// Выполняем установку базы событий
	this->_base = base;
}
/**
 * Оператор [=] установки базы событий
 * @param base база событий для установки
 * @return     текущий объект
 */
awh::Scheme::Broker & awh::Scheme::Broker::operator = (struct ev_loop * base) noexcept {
	// Выполняем установку базы событий
	this->_base = base;
	// Выводим текущий объект
	return (* this);
}
/**
 * Broker Конструктор
 * @param sid  идентификатор схемы сети
 * @param base объект базы событий
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Scheme::Broker::Broker(const uint16_t sid, const fmk_t * fmk, const log_t * log) noexcept :
 _id(0), _sid(sid), _ip{""}, _mac{""}, _port(0),
 _sonet(sonet_t::TCP), _callbacks(log),
 _readEventTimeout(this, engine_t::method_t::READ),
 _writeEventTimeout(this, engine_t::method_t::WRITE),
 _connectEventTimeout(this, engine_t::method_t::CONNECT),
 _timeoutEventTimeout(this, engine_t::method_t::TIMEOUT),
 _ectx(fmk, log), _addr(fmk, log), _fmk(fmk), _log(log), _base(nullptr) {
	// Устанавливаем идентификатор брокера
	this->_id = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
}
/**
 * clear Метод очистки
 */
void awh::Scheme::clear() noexcept {
	// Выполняем очистку списка брокеров
	this->_brokers.clear();
}
/**
 * socket Метод извлечения сокета брокера
 * @param bid идентификатор брокера
 * @return    активный сокет брокера
 */
SOCKET awh::Scheme::socket(const uint64_t bid) const noexcept {
	// Результат работы функции
	SOCKET result = -1;
	// Если идентификатор брокера передан
	if(bid > 0){
		// Выполняем поиск брокера
		auto it = this->_brokers.find(bid);
		// Если брокер найден, выводим
		if(it != this->_brokers.end())
			// Выводим полученный сокет
			return it->second->_addr.fd;
	}
	// Выводим результат
	return result;
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return   порт подключения брокера
 */
u_int awh::Scheme::port(const uint64_t bid) const noexcept {
	// Результат работы функции
	u_int result = 0;
	// Если идентификатор брокера передан
	if(bid > 0){
		// Выполняем поиск брокера
		auto it = this->_brokers.find(bid);
		// Если брокер найден, выводим
		if(it != this->_brokers.end())
			// Выводим полученный порт
			return it->second->port();
	}
	// Выводим результат
	return result;
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::Scheme::ip(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если идентификатор брокера передан
	if(bid > 0){
		// Выполняем поиск брокера
		auto it = this->_brokers.find(bid);
		// Если брокер найден, выводим
		if(it != this->_brokers.end())
			// Выводим полученный IP-адрес
			return it->second->ip();
	}
	// Выводим результат
	return result;
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::Scheme::mac(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если идентификатор брокера передан
	if(bid > 0){
		// Выполняем поиск брокера
		auto it = this->_brokers.find(bid);
		// Если брокер найден, выводим MAC адрес
		if(it != this->_brokers.end())
			// Выводим полученный MAC-адрес
			return it->second->mac();
	}
	// Выводим результат
	return result;
}
