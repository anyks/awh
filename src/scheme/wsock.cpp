/**
 * @file: wsock.cpp
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
#include <scheme/wsock.hpp>

/**
 * clear Метод очистки
 */
void awh::server::SchemeWebSocket::clear() noexcept {
	// Очищаем данные вокера
	scheme_t::clear();
	// Очищаем список параметров адъютантов
	this->_coffers.clear();
	// Сбрасываем тип компрессии
	this->compress = http_t::compress_t::NONE;
	// Освобождаем выделенную память
	map <uint64_t, unique_ptr <coffer_t>> ().swap(this->_coffers);
}
/**
 * set Метод создания параметров адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::SchemeWebSocket::set(const uint64_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if((aid > 0) && (this->_coffers.count(aid) < 1)){
		// Добавляем адъютанта в список адъютантов
		auto ret = this->_coffers.emplace(aid, unique_ptr <coffer_t> (new coffer_t(this->_fmk, this->_log)));
		// Устанавливаем метод сжатия
		ret.first->second->http.compress(this->compress);
		// Устанавливаем контрольную точку
		ret.first->second->point = this->fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
	}
}
/**
 * rm Метод удаления параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::SchemeWebSocket::rm(const uint64_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if(aid > 0){
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
const awh::server::SchemeWebSocket::coffer_t * awh::server::SchemeWebSocket::get(const uint64_t aid) const noexcept {
	// Результат работы функции
	coffer_t * result = nullptr;
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->_coffers.find(aid);
		// Если адъютант найден, выводим его параметры
		if(it != this->_coffers.end()) result = it->second.get();
	}
	// Выводим результат
	return result;
}
