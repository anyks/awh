/**
 * @file: core.cpp
 * @date: 2022-09-03
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
#include <scheme/core.hpp>

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
 * ip Метод получения IP адреса брокера
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
 * mac Метод получения MAC адреса брокера
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
