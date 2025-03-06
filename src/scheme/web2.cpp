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
void awh::server::scheme::WEB2::set(const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (this->_options.count(bid) < 1)){
		// Создаём объект параметров активного клиента
		auto ret = this->_options.emplace(bid, unique_ptr <options_t> (new options_t(this->_fmk, this->_log)));
		// Устанавливаем контрольную точку
		ret.first->second->point = this->_fmk->timestamp(fmk_t::chrono_t::MILLISECONDS);
	}
}
/**
 * rm Метод удаления параметров активного клиента
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB2::rm(const uint64_t bid) noexcept {
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
const awh::server::scheme::WEB2::options_t * awh::server::scheme::WEB2::get(const uint64_t bid) const noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto i = this->_options.find(bid);
		// Если брокер найден, выводим его параметры
		if(i != this->_options.end())
			// Выводим параметры подключения брокера
			return i->second.get();
	}
	// Выводим результат
	return nullptr;
}
/**
 * get Метод извлечения списка параметров активных клиентов
 * @return список параметров активных клиентов
 */
const map <uint64_t, unique_ptr <awh::server::scheme::WEB2::options_t>> & awh::server::scheme::WEB2::get() const noexcept {
	// Выводим результат
	return this->_options;
}
/**
 * openStream Метод открытия потока
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 */
void awh::server::scheme::WEB2::openStream(const int32_t sid, const uint64_t bid) noexcept {
	// Если идентификатор брокера передан
	if((bid > 0) && (sid > -1) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto i = this->_options.find(bid);
		// Если брокер найден, выводим его параметры
		if(i != this->_options.end()){
			// Создаём объект параметров активного клиента
			auto ret = i->second->streams.emplace(sid, unique_ptr <stream_t> (new stream_t(i->second->fmk, i->second->log)));
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
	if((bid > 0) && (sid > -1) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto i = this->_options.find(bid);
		// Если брокер найден, выводим его параметры
		if((i != this->_options.end()) && !i->second->streams.empty()){
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
	if((bid > 0) && (sid > -1) && !this->_options.empty()){
		// Выполняем поиск брокера
		auto i = this->_options.find(bid);
		// Если брокер найден, выводим его параметры
		if((i != this->_options.end()) && !i->second->streams.empty()){
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
