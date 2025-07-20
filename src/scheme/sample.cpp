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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <scheme/sample.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * clear Метод очистки
 */
void awh::server::scheme::Sample::clear() noexcept {
	// Очищаем данные вокера
	scheme_t::clear();
	// Очищаем список параметров активных клиентов
	this->_clients.clear();
	// Освобождаем выделенную память
	clients_t().swap(this->_clients);
}
/**
 * set Метод создания параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::scheme::Sample::set(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (this->_clients.count(bid) < 1))
		// Создаём объект параметров активного клиента
		this->_clients.emplace(bid, std::make_unique <options_t> (this->_fmk, this->_log));
}
/**
 * rm Метод удаления параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::scheme::Sample::rm(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_clients.empty()){
		// Выполняем поиск брокера
		auto i = this->_clients.find(bid);
		// Если брокер найден, удаляем его
		if(i != this->_clients.end())
			// Выполняем удаление брокеров
			this->_clients.erase(i);
	}
}
/**
 * get Метод извлечения списка параметров активных клиентов
 * @return список параметров активных клиентов
 */
const awh::server::scheme::Sample::clients_t & awh::server::scheme::Sample::get() const noexcept {
	// Выводим результат
	return this->_clients;
}
/**
 * get Метод получения параметров активного клиента
 * @param bid идентификатор брокера
 * @return    параметры активного клиента
 */
const awh::server::scheme::Sample::options_t * awh::server::scheme::Sample::get(const uint64_t bid) const noexcept {
	// Результат работы функции
	options_t * result = nullptr;
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_clients.empty()){
		// Выполняем поиск брокера
		auto i = this->_clients.find(bid);
		// Если брокер найден, выводим его параметры
		if(i != this->_clients.end())
			// Выводим параметры подключения брокера
			result = i->second.get();
	}
	// Выводим результат
	return result;
}
