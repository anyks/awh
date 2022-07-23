/**
 * @file: core.cpp
 * @date: 2021-02-06
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
#include <worker/core.hpp>

/**
 * clear Метод очистки
 */
void awh::Worker::clear() noexcept {
	// Выполняем очистку списка адъютантов
	this->adjutants.clear();
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::Worker::ip(const size_t aid) const noexcept {
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
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::Worker::mac(const size_t aid) const noexcept {
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
