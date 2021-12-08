/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <worker/socks5.hpp>

/**
 * clear Метод очистки
 */
void awh::server::WorkerSocks5::clear() noexcept {
	// Очищаем данные вокера
	worker_t::clear();
	// Очищаем список пар клиентов
	this->pairs.clear();
	// Очищаем список параметров адъютантов
	this->adjParams.clear();
	// Освобождаем выделенную память
	map <size_t, adjp_t> ().swap(this->adjParams);
}
/**
 * createAdj Метод создания параметров адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::WorkerSocks5::createAdj(const size_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if((aid > 0) && (this->adjParams.count(aid) < 1))
		// Добавляем адъютанта в список адъютантов
		this->adjParams.emplace(aid, move(adjp_t(this->fmk, this->log, &this->uri)));
}
/**
 * removeAdj Метод удаления параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::WorkerSocks5::removeAdj(const size_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjParams.find(aid);
		// Если адъютант найден, удаляем его
		if(it != this->adjParams.end()) this->adjParams.erase(it);
	}
}
/**
 * getAdj Метод получения параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    параметры подключения адъютанта
 */
const awh::server::WorkerSocks5::adjp_t * awh::server::WorkerSocks5::getAdj(const size_t aid) const noexcept {
	// Результат работы функции
	adjp_t * result = nullptr;
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjParams.find(aid);
		// Если адъютант найден, выводим его параметры
		if(it != this->adjParams.end()) result = (adjp_t *) &it->second;
	}
	// Выводим результат
	return result;
}
