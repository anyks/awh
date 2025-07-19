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
 * Подключаем заголовочный файл
 */
#include <scheme/core.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

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
uint32_t awh::Scheme::Broker::port() const noexcept {
	// Извлекаем установленный порт сервера
	return this->_port;
}
/**
 * port Метод установки порта подключения
 * @param порт подключения для установки
 */
void awh::Scheme::Broker::port(const uint32_t port) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
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
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
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
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если MAC-адрес передан
	if(!mac.empty())
		// Выполняем установку MAC-адреса
		this->_mac = mac;
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::Scheme::Broker::callback(const callback_t & callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку функции обратного вызова при активации сокета на чтение
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при готовности сокета к записи
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова при отключении сокета
	this->_callback.set("close", callback);
	// Выполняем установку функции обратного вызова при срабатывании таймаута
	this->_callback.set("timeout", callback);
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения
 */
void awh::Scheme::Broker::sonet(const sonet_t sonet) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку типа сокета подключения
	this->_sonet = sonet;
}
/**
 * callback Метод вызова при получении события сокета
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::Scheme::Broker::callback([[maybe_unused]] const SOCKET fd, const base_t::event_type_t event) noexcept {
	// Определяем тип события
	switch(static_cast <uint8_t> (event)){
		// Если выполняется событие закрытие подключения
		case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
			// Выполняем остановку работы события
			this->_event.stop();
			// Если функция обратного вызова на закратие подключения установлена
			if(this->_callback.is("close"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t)> ("close", this->_id);
		} break;
		// Если выполняется событие чтения данных с сокета
		case static_cast <uint8_t> (base_t::event_type_t::READ): {
			// Если функция обратного вызова на чтение данных с сокета установлена
			if(this->_callback.is("read"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t)> ("read", this->_id);
		} break;
		// Если выполняется событие записи данных с сокета
		case static_cast <uint8_t> (base_t::event_type_t::WRITE): {
			// Если функция обратного вызова на запись данных в сокет установлена
			if(this->_callback.is("write"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t)> ("write", this->_id);
		} break;
	}
}
/**
 * stop Метод остановки работы
 */
void awh::Scheme::Broker::stop() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем остановку работы события
	this->_event.stop();
}
/**
 * start Метод запуска работы
 */
void awh::Scheme::Broker::start() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Устанавливаем базу данных событий
	this->_event = this->_base;
	// Устанавливаем тип события
	this->_event = this->addr.fd;
	// Устанавливаем функцию обратного вызова
	this->_event = std::bind(static_cast <void (awh::scheme_t::broker_t::*)(const SOCKET, const base_t::event_type_t)> (&awh::scheme_t::broker_t::callback), this, _1, _2);
	// Выполняем запуск работы события
	this->_event.start();
}
/**
 * events Метод активации/деактивации метода события сокета
 * @param mode   сигнал активации сокета
 * @param method метод режима работы
 */
void awh::Scheme::Broker::events(const mode_t mode, const engine_t::method_t method) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если сокет подключения активен и база событий установлена и активна
	if((this->addr.fd != INVALID_SOCKET) && (this->addr.fd < AWH_MAX_SOCKETS) && (this->_base != nullptr)){
		// Определяем метод события сокета
		switch(static_cast <uint8_t> (method)){
			// Если передано событие подписки на чтение
			case static_cast <uint8_t> (engine_t::method_t::READ): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Выполняем активацию работы события
						this->_event.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
						// Выполняем активацию события закрытий подключения
						this->_event.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
						// Выполняем установку таймаута ожидания
						this->ectx.timeout(static_cast <uint32_t> (this->timeouts.read) * 1000, engine_t::method_t::READ);
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED):
						// Выполняем деактивацию работы события
						this->_event.mode(base_t::event_type_t::READ, base_t::event_mode_t::DISABLED);
					break;
				}
			} break;
			// Если передано событие подписки на запись
			case static_cast <uint8_t> (engine_t::method_t::WRITE): {
				// Определяем сигнал сокета
				switch(static_cast <uint8_t> (mode)){
					// Если установлен сигнал активации сокета
					case static_cast <uint8_t> (mode_t::ENABLED): {
						// Выполняем активацию работы события
						this->_event.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
						// Выполняем активацию события закрытий подключения
						this->_event.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
						// Выполняем установку таймаута ожидания
						this->ectx.timeout(static_cast <uint32_t> (this->timeouts.write) * 1000, engine_t::method_t::WRITE);
					} break;
					// Если установлен сигнал деактивации сокета
					case static_cast <uint8_t> (mode_t::DISABLED):
						// Выполняем деактивацию работы события
						this->_event.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
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
void awh::Scheme::Broker::timeout(const uint16_t seconds, const engine_t::method_t method) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Определяем метод режима работы
	switch(static_cast <uint8_t> (method)){
		// Режим работы ЧТЕНИЕ
		case static_cast <uint8_t> (engine_t::method_t::READ):
			// Устанавливаем время ожидания на входящие данные
			this->timeouts.read = seconds;
		break;
		// Режим работы ЗАПИСЬ
		case static_cast <uint8_t> (engine_t::method_t::WRITE):
			// Устанавливаем время ожидания на исходящие данные
			this->timeouts.write = seconds;
		break;
		// Режим работы ПОДКЛЮЧЕНИЕ
		case static_cast <uint8_t> (engine_t::method_t::CONNECT):
			// Устанавливаем время ожидания на исходящие данные
			this->timeouts.connect = seconds;
		break;
	}
}
/**
 * base Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Scheme::Broker::base(base_t * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку базы событий
	this->_base = base;
}
/**
 * Оператор [=] установки базы событий
 * @param base база событий для установки
 * @return     текущий объект
 */
awh::Scheme::Broker & awh::Scheme::Broker::operator = (base_t * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку базы событий
	this->_base = base;
	// Выводим текущий объект
	return (* this);
}
/**
 * Broker Конструктор
 * @param sid идентификатор схемы сети
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Scheme::Broker::Broker(const uint16_t sid, const fmk_t * fmk, const log_t * log) noexcept :
 _id(0), _sid(sid), _ip{""}, _mac{""}, _port(0), _sonet(sonet_t::TCP),
 _event(event_t::type_t::EVENT, fmk, log), _callback(log),
 ectx(fmk, log), addr(fmk, log), _fmk(fmk), _log(log), _base(nullptr) {
	// Устанавливаем идентификатор брокера
	this->_id = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
}
/**
 * ~Broker Деструктор
 */
awh::Scheme::Broker::~Broker() noexcept {
	// Выполняем остановку работы события
	this->_event.stop();
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
		auto i = this->_brokers.find(bid);
		// Если брокер найден, выводим
		if(i != this->_brokers.end())
			// Выводим полученный сокет
			return i->second->addr.fd;
	}
	// Выводим результат
	return result;
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return   порт подключения брокера
 */
uint32_t awh::Scheme::port(const uint64_t bid) const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	// Если идентификатор брокера передан
	if(bid > 0){
		// Выполняем поиск брокера
		auto i = this->_brokers.find(bid);
		// Если брокер найден, выводим
		if(i != this->_brokers.end())
			// Выводим полученный порт
			return i->second->port();
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
		auto i = this->_brokers.find(bid);
		// Если брокер найден, выводим
		if(i != this->_brokers.end())
			// Выводим полученный IP-адрес
			return i->second->ip();
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
		auto i = this->_brokers.find(bid);
		// Если брокер найден, выводим MAC адрес
		if(i != this->_brokers.end())
			// Выводим полученный MAC-адрес
			return i->second->mac();
	}
	// Выводим результат
	return result;
}
