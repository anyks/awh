/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <ws/worker.hpp>

/**
 * clear Метод очистки
 */
void awh::WorkerServerWS::clear() noexcept {
	// Очищаем данные вокера
	workSrv_t::clear();
	// Очищаем список параметров адъютантов
	this->adjParams.clear();
	// Освобождаем выделенную память
	map <size_t, adjp_t> ().swap(this->adjParams);
	// Сбрасываем тип компрессии
	this->compress = http_t::compress_t::NONE;
}
/**
 * createAdj Метод создания параметров адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::WorkerServerWS::createAdj(const size_t aid) noexcept {
	// Если идентификатор адъютанта передан
	if((aid > 0) && (this->adjParams.count(aid) < 1)){
		// Добавляем адъютанта в список адъютантов
		auto ret = this->adjParams.emplace(aid, move(adjp_t(this->fmk, this->log, &this->uri)));
		// Устанавливаем метод сжатия
		ret.first->second.http.setCompress(this->compress);
		// Устанавливаем контрольную точку
		ret.first->second.checkPoint = this->fmk->unixTimestamp();
	}
}
/**
 * removeAdj Метод удаления параметров подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::WorkerServerWS::removeAdj(const size_t aid) noexcept {
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
const awh::WorkerServerWS::adjp_t * awh::WorkerServerWS::getAdj(const size_t aid) const noexcept {
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
