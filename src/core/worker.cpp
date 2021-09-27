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
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept : type(type_t::NONE) {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект URI
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с HTTP
		this->http = new http_t(this->fmk, this->log, this->uri);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Proxy Деструктор
 */
awh::Proxy::~Proxy() noexcept {
	// Удаляем объект для работы с сетью
	if(this->nwk != nullptr) delete this->nwk;
	// Удаляем объект для работы с URI
	if(this->uri != nullptr) delete this->uri;
	// Удаляем объект работы с HTTP
	if(this->http != nullptr) delete this->http;
}
/**
 * clear Метод очистки
 */
void awh::Worker::clear() noexcept {
	// Выполняем сброс файлового дескриптора
	this->fd = -1;
	// Выполняем очистку объекта запроса
	this->url.clear();
	// Выполняем очистку счётчика попыток
	this->attempts.first = 0;
	// Выполняем очистку буфера данных
	memset((void *) this->buffer, 0, BUFFER_CHUNK);
	// Устанавливаем тип подключения
	this->connect = (this->proxy.type != proxy_t::type_t::NONE ? connect_t::PROXY : connect_t::SERVER);
}
/**
 * switchConnect Метод переключения типа подключения
 */
void awh::Worker::switchConnect() noexcept {
	// Определяем тип подключения
	switch((u_short) this->connect){
		// Если подключение выполняется через прокси-сервер
		case (u_short) connect_t::PROXY: this->connect = connect_t::SERVER; break;
		// Если подключение выполняется через сервер
		case (u_short) connect_t::SERVER: this->connect = connect_t::PROXY; break;
	}
}
/**
 * isProxy Метод проверки на подключение к прокси-серверу
 * @return результат проверки
 */
bool awh::Worker::isProxy() const noexcept {
	// Выполняем проверку типа подключения
	return (this->connect == connect_t::PROXY);
}
/**
 * Worker Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Worker::Worker(const fmk_t * fmk, const log_t * log) noexcept : proxy(fmk, log), fmk(fmk), log(log), attempts({0,0}) {
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
