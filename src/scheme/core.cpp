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
	// Выполняем очистку списка адъютантов
	this->adjutants.clear();
}
/**
 * getSocket Метод извлечения сокета адъютанта
 * @param aid идентификатор адъютанта
 * @return    активный сокет адъютанта
 */
SOCKET awh::Scheme::getSocket(const size_t aid) const noexcept {
	// Результат работы функции
	SOCKET result = -1;
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант найден, выводим MAC адрес
		if(it != this->adjutants.end()) return it->second->addr.fd;
	}
	// Выводим результат
	return result;
}
/**
 * getPort Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return   порт подключения адъютанта
 */
u_int awh::Scheme::getPort(const size_t aid) const noexcept {
	// Результат работы функции
	u_int result = 0;
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант найден, выводим IP адрес
		if(it != this->adjutants.end()) return it->second->port;
	}
	// Выводим результат
	return result;
}
/**
 * getIp Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::Scheme::getIp(const size_t aid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант найден, выводим IP адрес
		if(it != this->adjutants.end()) return it->second->ip;
	}
	// Выводим результат
	return result;
}
/**
 * getMac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::Scheme::getMac(const size_t aid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант найден, выводим MAC адрес
		if(it != this->adjutants.end()) return it->second->mac;
	}
	// Выводим результат
	return result;
}
