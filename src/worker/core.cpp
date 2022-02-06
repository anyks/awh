/**
 * @file: core.cpp
 * @date: 2021-02-06
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
#include <worker/core.hpp>

/**
 * end Метод установки флага завершения работы
 */
void awh::Worker::Adjutant::end() noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->locker);
	// Устанавливаем блокировку
	this->chunks.end = true;
}
/**
 * get Метод получения буфера чанка
 * @return буфер чанка в бинарном виде 
 */
vector <char> awh::Worker::Adjutant::get() noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->locker);
	// Если нужно продолжить выполнять сбор данных
	if(!this->chunks.end){
		// Флаг полученного значения буфера
		bool received = false;
		// Ищем переданный индекс чанка
		auto it = this->chunks.data.find(this->chunks.index);
		// Выполняем проверку, получен ли бинарный буфер чанка
		received = (it != this->chunks.data.end());
		// Если индекс выше количества элементов в списке, сбрасываем его
		if(!received && (this->chunks.index > this->chunks.count)){
			// Выполняем сброс значения индекса
			this->chunks.index = 0;
			// Ищем переданный индекс чанка
			it = this->chunks.data.find(this->chunks.index);
			// Выполняем проверку, получен ли бинарный буфер чанка
			received = (it != this->chunks.data.end());
		}
		// Если бинарный буфер чанка получен
		if(received){
			// Формируем полученный результат
			result = move(it->second);
			// Выполняем удаление обработанного буфера данных
			this->chunks.data.erase(this->chunks.index);
			// Увеличиваем значение индекса
			this->chunks.index++;
		}
	}
	// Выводим результат
	return result;
}
/**
 * add Метод добавления чанка бинарных данных
 * @param buffer буфер чанка бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::Worker::Adjutant::add(const char * buffer, const size_t size) noexcept {
	// Если даныне переданы
	if((buffer != nullptr) && (size > 0)){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->locker);
		// Устанавливаем данные чанка
		this->chunks.data.emplace(this->chunks.count, vector <char> (buffer, buffer + size));
		// Увеличиваем количество обработанных чанков
		this->chunks.count++;
		// Если счётчик чанков выше количества чанков
		if(this->chunks.count > this->chunks.data.size()){
			// Если количества чанков в десять миллионов раз меньше счётчика, выполняем сброс счётчика
			if(((this->chunks.count - this->chunks.data.size()) / (float) this->chunks.data.size() * 100.0f) > 10000000.0f)
				// Выполняем сброс счётчика чанков
				this->chunks.count = 0;
		}
	}
}
/**
 * clear Метод очистки
 */
void awh::Worker::clear() noexcept {
	// Выполняем очистку списка адъютантов
	this->adjutants.clear();
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::Worker::ip(const size_t aid) const noexcept {
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант найден, выводим IP адрес
		if(it != this->adjutants.end()) return it->second.get()->ip;
	}
	// Выводим результат
	return this->result;
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::Worker::mac(const size_t aid) const noexcept {
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Выполняем поиск адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант найден, выводим MAC адрес
		if(it != this->adjutants.end()) return it->second.get()->mac;
	}
	// Выводим результат
	return this->result;
}
