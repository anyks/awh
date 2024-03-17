/**
 * @file: web.cpp
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
#include <scheme/web.hpp>

/**
 * clear Метод очистки
 */
void awh::server::scheme::WEB::clear() noexcept {
	// Очищаем данные вокера
	scheme_t::clear();
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
void awh::server::scheme::WEB::set(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (this->_options.count(bid) < 1)){
		// Создаём объект параметров активного клиента
		auto ret = this->_options.emplace(bid, unique_ptr <options_t> (new options_t(this->_fmk, this->_log)));
		// Устанавливаем список доступных компрессоров
		ret.first->second->http.compressors(this->compressors);
		// Устанавливаем контрольную точку
		ret.first->second->point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
	}
}
/**
 * rm Метод удаления параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB::rm(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto i = this->_options.find(bid);
		// Если брокер найден, удаляем его
		if(i != this->_options.end())
			// Выполняем удаление брокеров
			this->_options.erase(i);
	}
}
/**
 * get Метод получения параметров активного клиента
 * @param bid идентификатор брокера
 * @return    параметры активного клиента
 */
const awh::server::scheme::WEB::options_t * awh::server::scheme::WEB::get(const uint64_t bid) const noexcept {
	// Результат работы функции
	options_t * result = nullptr;
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto i = this->_options.find(bid);
		// Если брокер найден, выводим его параметры
		if(i != this->_options.end())
			// Выводим параметры подключения брокера
			result = i->second.get();
	}
	// Выводим результат
	return result;
}
/**
 * get Метод извлечения списка параметров активных клиентов
 * @return список параметров активных клиентов
 */
const map <uint64_t, unique_ptr <awh::server::scheme::WEB::options_t>> & awh::server::scheme::WEB::get() const noexcept {
	// Выводим результат
	return this->_options;
}
