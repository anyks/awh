/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core/worker.hpp>

/**
 * Adjutant Конструктор
 * @param parent объект родительского воркера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Worker::Adjutant::Adjutant(const Worker * parent, const fmk_t * fmk, const log_t * log) noexcept : parent(parent), fmk(fmk), log(log), aid(time(nullptr)), attempt(0), timeRead(READ_TIMEOUT), timeWrite(WRITE_TIMEOUT), bev(nullptr) {
	try {
		// Резервируем память для работы с буфером данных WebSocket
		this->buffer = new char[BUFFER_CHUNK];
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Adjutant Деструктор
 */
awh::Worker::Adjutant::~Adjutant() noexcept {
	// Если буфер данных WebSocket создан
	if(this->buffer != nullptr){
		// Удаляем выделенную память
		delete [] this->buffer;
		// Зануляем буфер данных
		this->buffer = nullptr;
	}
}
/**
 * clear Метод очистки
 */
void awh::Worker::clear() noexcept {
	// Выполняем очистку списка адъютантов
	this->adjutants.clear();
}
