/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <worker/client.hpp>

/**
 * clear Метод очистки
 */
void awh::client::Worker::clear() noexcept {
	// Очищаем количество попыток
	this->attempt = 0;
	// Выполняем очистку объекта запроса
	this->url.clear();
	// Очищаем данные вокера
	awh::worker_t::clear();
	// Устанавливаем тип подключения
	this->connect = (this->proxy.type != proxy_t::type_t::NONE ? connect_t::PROXY : connect_t::SERVER);
}
/**
 * switchConnect Метод переключения типа подключения
 */
void awh::client::Worker::switchConnect() noexcept {
	// Определяем тип подключения
	switch((uint8_t) this->connect){
		// Если подключение выполняется через прокси-сервер
		case (uint8_t) connect_t::PROXY: this->connect = connect_t::SERVER; break;
		// Если подключение выполняется через сервер
		case (uint8_t) connect_t::SERVER: this->connect = connect_t::PROXY; break;
	}
}
/**
 * isProxy Метод проверки на подключение к прокси-серверу
 * @return результат проверки
 */
bool awh::client::Worker::isProxy() const noexcept {
	// Выполняем проверку типа подключения
	return (this->connect == connect_t::PROXY);
}
/**
 * getAid Метод получения идентификатора адъютанта
 * @return идентификатор адъютанта
 */
size_t awh::client::Worker::getAid() const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если список адъютантов получен
	if(!this->adjutants.empty()){
		// Получаем первого адъютанта из списка
		auto it = this->adjutants.begin();
		// Получаем идентификатор адъютанта
		result = it->first;
	}
	// Выводим результат
	return result;
}
