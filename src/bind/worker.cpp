/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <bind/worker.hpp>

/**
 * Worker Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Worker::Worker(const fmk_t * fmk, const log_t * log) noexcept : fmk(fmk), log(log) {
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
 * ~Worker Деструктор
 */
awh::Worker::~Worker() noexcept {
	// Если буфер данных WebSocket создан
	if(this->buffer != nullptr) delete [] this->buffer;
}
