/**
 * @file: proxy.cpp
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
#include <scheme/proxy.hpp>

/**
 * clear Метод очистки
 */
void awh::server::SchemeProxy::clear() noexcept {
	// Очищаем данные вокера
	scheme_t::clear();
	// Очищаем список пар клиентов
	this->pairs.clear();
	// Очищаем список параметров активных клиентов
	this->_options.clear();
	// Очищаем доступный список доступных компрессоров
	this->compressors.clear();
	// Освобождаем выделенную память
	map <uint64_t, unique_ptr <options_t>> ().swap(this->_options);
}
/**
 * set Метод создания параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::SchemeProxy::set(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (this->_options.count(bid) < 1)){
		// Создаём объект параметров активного клиента
		auto ret = this->_options.emplace(bid, unique_ptr <options_t> (new options_t(this->_fmk, this->_log)));
		// Устанавливаем список доступных компрессоров
		ret.first->second->cli.compressors(this->compressors);
		ret.first->second->srv.compressors(this->compressors);
	}
}
/**
 * rm Метод удаления параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::SchemeProxy::rm(const uint64_t bid) noexcept {	
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto it = this->_options.find(bid);
		// Если брокер найден, удаляем его
		if(it != this->_options.end())
			// Выполняем удаление брокеров
			this->_options.erase(it);
	}
}
/**
 * get Метод получения параметров активного клиента
 * @param bid идентификатор брокера
 * @return    параметры активного клиента
 */
const awh::server::SchemeProxy::options_t * awh::server::SchemeProxy::get(const uint64_t bid) const noexcept {
	// Результат работы функции
	options_t * result = nullptr;
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto it = this->_options.find(bid);
		// Если брокер найден, выводим его параметры
		if(it != this->_options.end())
			// Выводим параметры подключения брокера
			result = it->second.get();
	}
	// Выводим результат
	return result;
}
