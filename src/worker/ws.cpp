/**
 * @file: ws.cpp
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
#include <worker/ws.hpp>

/**
 * clear Метод очистки
 */
void awh::server::WorkerWebSocket::clear() noexcept {
	// Очищаем данные вокера
	worker_t::clear();
	// Очищаем список параметров адъютантов
	this->settings.clear();
	// Сбрасываем тип компрессии
	this->compress = http_t::compress_t::NONE;
	// Освобождаем выделенную память
	map <size_t, unique_ptr <settings_t>> ().swap(this->settings);
}
/**
 * set Метод создания параметров адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::WorkerWebSocket::set(const size_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if((aid > 0) && (this->settings.count(aid) < 1)){
		// Добавляем адъютанта в список адъютантов
		auto ret = this->settings.emplace(aid, unique_ptr <settings_t> (new settings_t(this->fmk, this->log, &this->uri)));
		// Устанавливаем метод сжатия
		ret.first->second->http.setCompress(this->compress);
		// Устанавливаем контрольную точку
		ret.first->second->checkPoint = this->fmk->unixTimestamp();
	}
}
/**
 * rm Метод удаления параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::WorkerWebSocket::rm(const size_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->settings.find(aid);
		// Если адъютант найден, удаляем его
		if(it != this->settings.end()) this->settings.erase(it);
	}
}
/**
 * get Метод получения параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    параметры подключения адъютанта
 */
const awh::server::WorkerWebSocket::settings_t * awh::server::WorkerWebSocket::get(const size_t aid) const noexcept {
	// Результат работы функции
	settings_t * result = nullptr;
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->settings.find(aid);
		// Если адъютант найден, выводим его параметры
		if(it != this->settings.end()) result = it->second.get();
	}
	// Выводим результат
	return result;
}
