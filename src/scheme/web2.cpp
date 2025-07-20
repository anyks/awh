/**
 * @file: web2.cpp
 * @date: 2023-11-29
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
#include <scheme/web2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * clear Метод очистки
 */
void awh::server::scheme::WEB2::clear() noexcept {
	// Очищаем данные вокера
	scheme_t::clear();
	// Очищаем список параметров активных клиентов
	this->_clients.clear();
	// Очищаем доступный список доступных компрессоров
	this->compressors.clear();
	// Освобождаем выделенную память
	clients_t().swap(this->_clients);
}
/**
 * set Метод создания параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB2::set(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (this->_clients.count(bid) < 1)){
		// Создаём объект параметров активного клиента
		auto ret = this->_clients.emplace(bid, std::make_unique <options_t> (this->_fmk, this->_log));
		// Устанавливаем контрольную точку
		ret.first->second->respPong = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
	}
}
/**
 * rm Метод удаления параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB2::rm(const uint64_t bid) noexcept {
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
const awh::server::scheme::WEB2::clients_t & awh::server::scheme::WEB2::get() const noexcept {
	// Выводим результат
	return this->_clients;
}
/**
 * get Метод получения параметров активного клиента
 * @param bid идентификатор брокера
 * @return    параметры активного клиента
 */
const awh::server::scheme::WEB2::options_t * awh::server::scheme::WEB2::get(const uint64_t bid) const noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_clients.empty()){
		// Выполняем поиск брокера
		auto i = this->_clients.find(bid);
		// Если брокер найден, выводим его параметры
		if(i != this->_clients.end())
			// Выводим параметры подключения брокера
			return i->second.get();
	}
	// Выводим результат
	return nullptr;
}
/**
 * openStream Метод открытия потока
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB2::openStream(const int32_t sid, const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (sid > -1) && !this->_clients.empty()){
		// Выполняем поиск брокера
		auto i = this->_clients.find(bid);
		// Если брокер найден, выводим его параметры
		if(i != this->_clients.end()){
			// Создаём объект параметров активного клиента
			auto ret = i->second->streams.emplace(sid, make_unique <stream_t> (i->second->fmk, i->second->log));
			// Устанавливаем идентификатор потока
			ret.first->second->sid = sid;
			// Устанавливаем список доступных компрессоров
			ret.first->second->http.compressors(this->compressors);
		}
	}
}
/**
 * closeStream Метод закрытия потока
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB2::closeStream(const int32_t sid, const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (sid > -1) && !this->_clients.empty()){
		// Выполняем поиск брокера
		auto i = this->_clients.find(bid);
		// Если брокер найден, выводим его параметры
		if((i != this->_clients.end()) && !i->second->streams.empty()){
			// Выполняем поиск указанного потока
			auto j = i->second->streams.find(sid);
			// Если поток найден
			if(j != i->second->streams.end())
				// Выполняем удаление потока
				i->second->streams.erase(j);
		}
	}
}
/**
 * getStream Метод извлечения данных потока
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    данные запрашиваемого потока
 */
const awh::server::scheme::WEB2::stream_t * awh::server::scheme::WEB2::getStream(const int32_t sid, const uint64_t bid) const noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (sid > -1) && !this->_clients.empty()){
		// Выполняем поиск брокера
		auto i = this->_clients.find(bid);
		// Если брокер найден, выводим его параметры
		if((i != this->_clients.end()) && !i->second->streams.empty()){
			// Выполняем поиск указанного потока
			auto j = i->second->streams.find(sid);
			// Если поток найден
			if(j != i->second->streams.end())
				// Выводим полученный результат
				return j->second.get();
		}
	}
	// Выводим результат
	return nullptr;
}
