/**
 * @file: client.cpp
 * @date: 2022-09-03
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

// Подключаем заголовочный файл
#include <scheme/client.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * clear Метод очистки
 */
void awh::client::Scheme::clear() noexcept {
	// Выполняем очистку объекта запроса
	this->url.clear();
	// Очищаем данные вокера
	awh::scheme_t::clear();
	// Устанавливаем тип подключения
	this->_connect = (this->proxy.mode ? connect_t::PROXY : connect_t::SERVER);
}
/**
 * switchConnect Метод переключения типа подключения
 */
void awh::client::Scheme::switchConnect() noexcept {
	// Если работа с прокси-сервером активированна
	if(this->proxy.mode){
		// Определяем тип подключения
		switch(static_cast <uint8_t> (this->_connect)){
			// Если подключение выполняется через прокси-сервер
			case static_cast <uint8_t> (connect_t::PROXY):
				// Устанавливаем тип подключения
				this->_connect = connect_t::SERVER;
			break;
			// Если подключение выполняется через сервер
			case static_cast <uint8_t> (connect_t::SERVER):
				// Устанавливаем тип подключения
				this->_connect = connect_t::PROXY;
			break;
		}
	// Устанавливаем тип подключения
	} else this->_connect = connect_t::SERVER;
}
/**
 * isProxy Метод проверки на подключение к прокси-серверу
 * @return результат проверки
 */
bool awh::client::Scheme::isProxy() const noexcept {
	// Выполняем проверку типа подключения
	return (this->_connect == connect_t::PROXY);
}
/**
 * bid Метод получения идентификатора брокера
 * @return идентификатор брокера
 */
uint64_t awh::client::Scheme::bid() const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если список брокеров получен
	if(!this->_brokers.empty()){
		// Получаем первого брокера из списка
		auto i = this->_brokers.begin();
		// Получаем идентификатор брокера
		result = i->first;
	}
	// Выводим результат
	return result;
}
/**
 * activateProxy Метод активации прокси-клиента
 * @param work флаг активации
 */
void awh::client::Scheme::activateProxy(const work_t work) noexcept {
	// Определяем флаг активации
	switch(static_cast <uint8_t> (work)){
		// Если необходимо активировать прокси-сервер
		case static_cast <uint8_t> (work_t::ALLOW): {
			// Выполняем установку флага активации прокси-сервера
			this->proxy.mode = true;
			// Устанавливаем тип подключения
			this->_connect = connect_t::PROXY;
		} break;
		// Если необходимо деактивировать прокси-сервер
		case static_cast <uint8_t> (work_t::DISALLOW): {
			// Снимаем флаг активации прокси-сервера
			this->proxy.mode = false;
			// Устанавливаем тип подключения
			this->_connect = connect_t::SERVER;
		} break;
	}
}
