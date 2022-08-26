/**
 * @file: proxy.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <worker/proxy.hpp>

/**
 * clear Метод очистки
 */
void awh::server::WorkerProxy::clear() noexcept {
	// Очищаем данные вокера
	worker_t::clear();
	// Очищаем список пар клиентов
	this->pairs.clear();
	// Очищаем список параметров адъютантов
	this->_coffers.clear();
	// Освобождаем выделенную память
	map <size_t, unique_ptr <coffer_t>> ().swap(this->_coffers);
	// Сбрасываем тип компрессии
	this->compress = http_t::compress_t::NONE;
}
/**
 * set Метод создания параметров адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::WorkerProxy::set(const size_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if((aid > 0) && (this->_coffers.count(aid) < 1)){
		// Добавляем адъютанта в список адъютантов
		auto ret = this->_coffers.emplace(aid, unique_ptr <coffer_t> (new coffer_t(this->_fmk, this->_log, &this->uri)));
		// Устанавливаем метод сжатия
		ret.first->second->cli.compress(this->compress);
		ret.first->second->srv.compress(this->compress);
	}
}
/**
 * rm Метод удаления параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::WorkerProxy::rm(const size_t aid) noexcept {	
	// Если идентификатор адъютанта передан
	if((aid > 0) && !this->_coffers.empty()){
		// Выполняем поиск адъютанта
		auto it = this->_coffers.find(aid);
		// Если адъютант найден, удаляем его
		if(it != this->_coffers.end()) this->_coffers.erase(it);
	}
}
/**
 * get Метод получения параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    параметры подключения адъютанта
 */
const awh::server::WorkerProxy::coffer_t * awh::server::WorkerProxy::get(const size_t aid) const noexcept {
	// Результат работы функции
	coffer_t * result = nullptr;
	// Если идентификатор адъютанта передан
	if((aid > 0) && !this->_coffers.empty()){
		// Выполняем поиск адъютанта
		auto it = this->_coffers.find(aid);
		// Если адъютант найден, выводим его параметры
		if(it != this->_coffers.end()) result = it->second.get();
	}
	// Выводим результат
	return result;
}
