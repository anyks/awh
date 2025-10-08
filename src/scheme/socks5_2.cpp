/**
 * @file: socks5.cpp
 * @date: 2025-10-08
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
#include <scheme/socks5_2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод очистки
 *
 */
void awh::server::scheme::Socks5::clear() noexcept {
	// Очищаем данные вокера
	scheme_t::clear();
	// Очищаем список параметров активных клиентов
	this->_clients.clear();
	// Освобождаем выделенную память
	clients_t().swap(this->_clients);
}
/**
 * @brief Метод создания параметров активного клиента
 *
 * @param bid идентификатор брокера
 */
void awh::server::scheme::Socks5::set(const uint32_t bid) noexcept {
	// Если идентификатор брокера передан
	if(bid > 0)
		// Создаём объект параметров активного клиента
		this->_clients.emplace(bid, std::make_unique <options_t> (this->_fmk, this->_log));
}
/**
 * @brief Метод удаления параметров активного клиента
 *
 * @param bid идентификатор брокера
 */
void awh::server::scheme::Socks5::rm(const uint32_t bid) noexcept {
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
 * @brief Метод получения параметров активного клиента
 *
 * @param bid идентификатор брокера
 * @return    параметры активного клиента
 */
const awh::server::scheme::Socks5::options_t * awh::server::scheme::Socks5::get(const uint32_t bid) const noexcept {
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
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::scheme::Socks5::Socks5(const fmk_t * fmk, const log_t * log) noexcept :
 scheme_t(fmk, log), _fmk(fmk), _log(log) {}
