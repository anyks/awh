/**
 * @file: web.hpp
 * @date: 2025-10-04
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
#include <http/web2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Шаблон функции препарирования HTTP-заголовков
 * 
 * @tparam F функция обратного вызова
 */
template <typename F>
/**
 * @brief Функция препарирования HTTP-заголовков
 *
 * @param buffer   буфер данных для парсинга
 * @param size     размер буфера данных для парсинга
 * @param work     объект рабочих параметров
 * @param result   количество обработанных байт
 * @param callback функция обратного вызова
 */
static void prepare(const char * buffer, const size_t size, awh::web_t::work_t & work, size_t & result, F && callback) noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (size > 0)){
		// Флаг завершения работы сборки
		bool stop = false;
		// Значение текущей и предыдущей буквы
		int8_t actual = 0, last = 0;
		// Смещение в буфере и длина полученной строки
		size_t offset = 0, length = 0, count = 0;
		// Если позиция ещё не сброшена
		if((work.pos[0] != -1) || (work.pos[1] != -1))
			// Выполняем сброс массива сепараторов
			::memset(work.pos, -1, sizeof(work.pos));
		// Переходим по всему буферу
		for(size_t i = 0; i < size; i++){
			// Получаем значение текущей буквы
			actual = static_cast <int8_t> (buffer[i]);
			// Если текущий символ перенос строки и это конец, выходим
			if(stop && (actual == static_cast <int8_t> ('\n'))){
				// Устанавливаем количество обработанных байт
				result = (i + 1);
				// Выполняем функцию обратного вызова
				callback(nullptr, 0, stop);
				// Выходим из цикла
				break;
			}
			// Если флаг остановки не установлен
			if(!stop)
				// Если предыдущий символ был переносом строки а текущий возврат каретки
				stop = ((last == static_cast <int8_t> ('\n')) && (actual == static_cast <int8_t> ('\r')));
			// Если сепаратор найден, добавляем его в массив
			if((work.delim != static_cast <int8_t> ('\0')) && (actual == work.delim) && (count < 2)){
				// Устанавливаем позицию найденного разделителя
				work.pos[count] = (i - offset);
				// Увеличиваем количество найденных разделителей
				count++;
			}
			// Если текущая буква является переносом строк
			if((i > 0) && (actual == static_cast <int8_t> ('\n'))){
				// Устанавливаем количество обработанных байт
				result = (i + 1);
				// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
				length = ((last == static_cast <int8_t> ('\r') ? (i - 1) : i) - offset);
				// Если данные не получены но мы дошли до конца
				if((length == 0) && (last == static_cast <int8_t> ('\r')) && (actual == static_cast <int8_t> ('\n'))){
					// Устанавливаем флаг конца
					stop = (
						(work.state == awh::web_t::state_t::BODY) ||
						(work.state == awh::web_t::state_t::HEADERS)
					);
					// Выполняем функцию обратного вызова
					callback(nullptr, 0, stop);
				// Если длина слова получена, выводим полученную строку
				} else callback(buffer + offset, length, stop);
				// Если массив сепараторов получен
				if(work.delim != static_cast <int8_t> ('\0')){
					// Выполняем сброс количество найденных сепараторов
					count = 0;
					// Выполняем сброс массива сепараторов
					::memset(work.pos, -1, sizeof(work.pos));
				}
				// Выполняем смещение
				offset = (i + 1);
			}
			// Запоминаем предыдущую букву
			last = actual;
		}
	}
}
/**
 * @brief Оператор [=] перемещения параметров запроса клиента
 *
 * @param request объект параметров запроса клиента
 * @return        текущие параметры запроса клиента
 */
awh::Web::Request & awh::Web::Request::operator = (req_t && request) noexcept {
	// Выполняем установку метода запроса клиента
	this->method = request.method;
	// Выполняем установку версии протокола
	this->version = request.version;
	// Выполняем перемещение данных ссылки
	this->url = ::move(request.url);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присванивания параметров запроса клиента
 *
 * @param request объект параметров запроса клиента
 * @return        текущие параметры запроса клиента
 */
awh::Web::Request & awh::Web::Request::operator = (const req_t & request) noexcept {
	// Выполняем копирование данных ссылки
	this->url = request.url;
	// Выполняем установку метода запроса клиента
	this->method = request.method;
	// Выполняем установку версии протокола
	this->version = request.version;
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param request объект параметров запроса клиента
 * @return        результат сравнения
 */
bool awh::Web::Request::operator == (const req_t & request) noexcept {
	// Выполняем сравнение параметров
	return (
		(this->method == request.method) &&
		(this->version == request.version) &&
		(this->url == request.url)
	);
}
/**
 * @brief Конструктор перемещения
 *
 * @param request объект параметров запроса клиента
 */
awh::Web::Request::Request(req_t && request) noexcept {
	// Выполняем установку метода запроса клиента
	this->method = request.method;
	// Выполняем установку версии протокола
	this->version = request.version;
	// Выполняем перемещение данных ссылки
	this->url = ::move(request.url);
}
/**
 * @brief Конструктор копирования
 *
 * @param request объект параметров запроса клиента
 */
awh::Web::Request::Request(const req_t & request) noexcept {
	// Выполняем копирование данных ссылки
	this->url = request.url;
	// Выполняем установку метода запроса клиента
	this->method = request.method;
	// Выполняем установку версии протокола
	this->version = request.version;
}
/**
 * @brief Конструктор
 *
 */
awh::Web::Request::Request() noexcept : provider_t(), method(method_t::NONE) {}
/**
 * @brief Конструктор
 *
 * @param method метод запроса клиента
 */
awh::Web::Request::Request(const method_t method) noexcept : provider_t(), method(method) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 */
awh::Web::Request::Request(const double version) noexcept : provider_t(version), method(method_t::NONE) {}
/**
 * @brief Конструктор
 *
 * @param url адрес URL-запроса
 */
awh::Web::Request::Request(const uri_t::url_t & url) noexcept : provider_t(), method(method_t::NONE), url(url) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param method  метод запроса клиента
 */
awh::Web::Request::Request(const double version, const method_t method) noexcept : provider_t(version), method(method) {}
/**
 * @brief Конструктор
 *
 * @param method метод запроса клиента
 * @param url    адрес URL-запроса
 */
awh::Web::Request::Request(const method_t method, const uri_t::url_t & url) noexcept : provider_t(), method(method), url(url) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param url     адрес URL-запроса
 */
awh::Web::Request::Request(const double version, const uri_t::url_t & url) noexcept : provider_t(version), method(method_t::NONE), url(url) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param method  метод запроса клиента
 * @param url     адрес URL-запроса
 */
awh::Web::Request::Request(const double version, const method_t method, const uri_t::url_t & url) noexcept : provider_t(version), method(method), url(url) {}
/**
 * @brief Оператор [=] перемещения параметров ответа сервера
 *
 * @param response объект параметров ответа сервера
 * @return         текущие параметры ответа сервера
 */
awh::Web::Response & awh::Web::Response::operator = (res_t && response) noexcept {
	// Выполняем установку кода ответа сервера
	this->code = response.code;
	// Выполняем установку версии протокола
	this->version = response.version;
	// Выполняем перемещение сообщение сервера
	this->message = ::move(response.message);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присванивания параметров ответа сервера
 *
 * @param response объект параметров ответа сервера
 * @return         текущие параметры ответа сервера
 */
awh::Web::Response & awh::Web::Response::operator = (const res_t & response) noexcept {
	// Выполняем установку кода ответа сервера
	this->code = response.code;
	// Выполняем установку версии протокола
	this->version = response.version;
	// Выполняем копирование сообщение сервера
	this->message = response.message;
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param response объект параметров ответа сервера
 * @return         результат сравнения
 */
bool awh::Web::Response::operator == (const res_t & response) noexcept {
	// Выполняем сравнение параметров
	return (
		(this->code == response.code) &&
		(this->version == response.version) &&
		(this->message.compare(response.message) == 0)
	);
}
/**
 * @brief Конструктор перемещения
 *
 * @param response объект параметров ответа сервера
 */
awh::Web::Response::Response(res_t && response) noexcept {
	// Выполняем установку кода ответа сервера
	this->code = response.code;
	// Выполняем установку версии протокола
	this->version = response.version;
	// Выполняем перемещение сообщение сервера
	this->message = ::move(response.message);
}
/**
 * @brief Конструктор копирования
 *
 * @param response объект параметров ответа сервера
 */
awh::Web::Response::Response(const res_t & response) noexcept {
	// Выполняем установку кода ответа сервера
	this->code = response.code;
	// Выполняем установку версии протокола
	this->version = response.version;
	// Выполняем копирование сообщение сервера
	this->message = response.message;
}
/**
 * @brief Конструктор
 *
 */
awh::Web::Response::Response() noexcept : provider_t(), code(0), message{""} {}
/**
 * @brief Конструктор
 *
 * @param code код ответа сервера
 */
awh::Web::Response::Response(const uint32_t code) noexcept : provider_t(), code(code), message{""} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 */
awh::Web::Response::Response(const double version) noexcept : provider_t(version), code(0), message{""} {}
/**
 * @brief Конструктор
 *
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const string & message) noexcept : provider_t(), code(0), message(message) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param code    код ответа сервера
 */
awh::Web::Response::Response(const double version, const uint32_t code) noexcept : provider_t(version), code(code), message{""} {}
/**
 * @brief Конструктор
 *
 * @param code    код ответа сервера
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const uint32_t code, const string & message) noexcept : provider_t(), code(code), message(message) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const double version, const string & message) noexcept : provider_t(version), code(0), message(message) {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param code    код ответа сервера
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const double version, const uint32_t code, const string & message) noexcept : provider_t(version), code(code), message(message) {}
/**
 * @brief Метод очистки данных чанка
 *
 */
void awh::Web::Chunk::clear() noexcept {
	// Обнуляем размер чанка
	this->size = 0;
	// Обнуляем буфер данных
	this->buffer.clear();
	// Выполняем сброс стейта чанка
	this->state = chunk_t::state_t::SIZE;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Web::Chunk::Chunk(const fmk_t * fmk, const log_t * log) noexcept :
 size(0), state(chunk_t::state_t::SIZE), buffer(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::Web::Chunk::~Chunk() noexcept {}
/**
 * @brief Метод очистки данных тела сообщения
 *
 */
void awh::Web::Body::clear() noexcept {
	// Обнуляем размер тела сообщения
	this->size = -1;
	// Выполняем очистку данных тела HTTP-протокола
	this->data.clear();
	// Выполняем очистку блока чанка
	this->chunk.clear();
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Web::Body::Body(const fmk_t * fmk, const log_t * log) noexcept :
 size(-1), chunk(fmk, log), data(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::Web::Body::~Body() noexcept {}
/**
 * @brief Метод инициализации модуля
 * 
 */
void awh::Web::init() noexcept {
	// Формируем идентификатор объекта
	this->_id = const_cast <fmk_t *> (this->_fmk)->identifier();
	// Выполняем заполнение списка стандартных заголовков
	this->_standards.insert({
		{"via", {proto_t::PROXY}},
		{"date", {proto_t::HTTP1}},
		{"link", {proto_t::HTTP1}},
		{"age", {proto_t::HTTP1_1}},
		{"dnt", {proto_t::HTTP1_1}},
		{"allow", {proto_t::HTTP1}},
		{"host", {proto_t::HTTP1_1}},
		{"etag", {proto_t::HTTP1_1}},
		{"from", {proto_t::HTTP1_1}},
		{"vary", {proto_t::HTTP1_1}},
		{"server", {proto_t::HTTP1}},
		{"accept", {proto_t::HTTP1}},
		{"cookie", {proto_t::HTTP1}},
		{"pragma", {proto_t::HTTP1}},
		{"range", {proto_t::HTTP1_1}},
		{"referer", {proto_t::HTTP1}},
		{"expires", {proto_t::HTTP1}},
		{"origin", {proto_t::HTTP1_1}},
		{"location", {proto_t::HTTP1}},
		{"upgrade", {proto_t::HTTP1_1}},
		{"warning", {proto_t::HTTP1_1}},
		{"if-match", {proto_t::HTTP1_1}},
		{"if-range", {proto_t::HTTP1_1}},
		{"user-agent", {proto_t::HTTP1}},
		{"content-md5", {proto_t::NONE}},
		{"accept-ch", {proto_t::HTTP1_1}},
		{"negotiate", {proto_t::HTTP1_1}},
		{"retry-after", {proto_t::HTTP1}},
		{"set-cookie", {proto_t::HTTP1_1}},
		{"alternates", {proto_t::HTTP1_1}},
		{"connection", {proto_t::HTTP1_1}},
		{"content-type", {proto_t::HTTP1}},
		{"set-cookie2", {proto_t::HTTP1_1}},
		{"last-modified", {proto_t::HTTP1}},
		{"authorization", {proto_t::HTTP1}},
		{"max-forwards", {proto_t::HTTP1_1}},
		{"accept-charset", {proto_t::HTTP1}},
		{"content-length", {proto_t::HTTP1}},
		{"variant-vary", {proto_t::HTTP1_1}},
		{"content-range", {proto_t::HTTP1_1}},
		{"accept-ranges", {proto_t::HTTP1_1}},
		{"cache-control", {proto_t::HTTP1_1}},
		{"last-event-id", {proto_t::HTTP1_1}},
		{"if-none-match", {proto_t::HTTP1_1}},
		{"accept-encoding", {proto_t::HTTP1}},
		{"accept-language", {proto_t::HTTP1}},
		{"x-requested-with", {proto_t::NONE}},
		{"content-encoding", {proto_t::HTTP1}},
		{"content-language", {proto_t::HTTP1}},
		{"www-authenticate", {proto_t::HTTP1}},
		{"accept-features", {proto_t::HTTP1_1}},
		{"x-frame-options", {proto_t::HTTP1_1}},
		{"if-modified-since", {proto_t::HTTP1}},
		{"content-location", {proto_t::HTTP1_1}},
		{"proxy-authenticate", {proto_t::HTTP1}},
		{"transfer-encoding", {proto_t::HTTP1_1}},
		{"proxy-authorization", {proto_t::HTTP1}},
		{"te", {proto_t::HTTP2, proto_t::HTTP1_1}},
		{"x-content-duration", {proto_t::HTTP1_1}},
		{"tcn", {proto_t::HTTP2, proto_t::HTTP1_1}},
		{"if-unmodified-since", {proto_t::HTTP1_1}},
		{"sec-websocket-key", {proto_t::WEBSOCKET}},
		{"x-dnsprefetch-control", {proto_t::HTTP1_1}},
		{"sec-Websocket-origin", {proto_t::WEBSOCKET}},
		{"access-control-max-age", {proto_t::HTTP1_1}},
		{"expect", {proto_t::HTTP2, proto_t::HTTP1_1}},
		{"content-security-policy", {proto_t::HTTP1_1}},
		{"sec-websocket-version", {proto_t::WEBSOCKET}},
		{"trailer", {proto_t::HTTP2, proto_t::HTTP1_1}},
		{"sec-websocket-protocol", {proto_t::WEBSOCKET}},
		{"x-content-security-policy", {proto_t::HTTP1_1}},
		{"strict-transport-security", {proto_t::HTTP1_1}},
		{"sec-websocket-extensions", {proto_t::WEBSOCKET}},
		{"access-control-allow-origin", {proto_t::HTTP1_1}},
		{"access-control-allow-methods", {proto_t::HTTP1_1}},
		{"access-control-allow-headers", {proto_t::HTTP1_1}},
		{"access-control-expose-headers", {proto_t::HTTP1_1}},
		{"access-control-request-method", {proto_t::HTTP1_1}},
		{"access-control-request-meaders", {proto_t::HTTP1_1}},
		{"access-control-allow-credentials", {proto_t::HTTP1_1}}
	});
}
/**
 * @brief Метод извлечения полученных данных
 *
 * @param buffer буфер данных для чтения
 * @param size   размер буфера данных для чтения
 * @param unit   HTTP-юнит с которым производится работа
 * @return       размер обработанных данных
 */
size_t awh::Web::extraction(const char * buffer, const size_t size, const unit_t unit) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы
	if((buffer != nullptr) && (size > 0) && (this->_work.state != state_t::END)){
		/**
		 * Определяем текущий HTTP-юнит с которым производится работа
		 */
		switch(static_cast <uint8_t> (unit)){
			// Если производится работы с HTTP-телом
			case static_cast <uint8_t> (unit_t::BODY): {
				// Если мы собираем тело полезной нагрузки
				if(this->_work.state == state_t::BODY){
					// Если размер тела сообщения получен
					if(this->_body.size > -1){
						// Если размер тела не получен
						if(this->_body.size == 0){
							// Запоминаем количество обработанных байт
							result = size;
							// Выполняем очистку буфера тела HTTP-протокола
							this->_body.chunk.buffer.clear();
							// Заполняем собранные данные в промежуточный буфер
							this->_body.chunk.buffer.push(buffer, result);
							// Если функция обратного вызова на перехват входящих чанков установлена
							if(this->_callback.is("binary"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const buffer_t &, const web_t *)> ("binary", this->_id, this->_body.chunk.buffer, this);
						// Если размер установлен конкретный
						} else {
							// Получаем актуальный размер тела
							result = (this->_body.size - this->_body.data.size());
							// Фиксируем актуальный размер тела
							result = (size > result ? result : size);
							// Увеличиваем общий размер полученных данных
							this->_body.chunk.size += result;
							// Выполняем очистку буфера тела HTTP-протокола
							this->_body.chunk.buffer.clear();
							// Заполняем собранные данные в промежуточный буфер
							this->_body.chunk.buffer.push(buffer, result);
							// Если функция обратного вызова на перехват входящих чанков установлена
							if(this->_callback.is("binary"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const buffer_t &, const web_t *)> ("binary", this->_id, this->_body.chunk.buffer, this);
							// Если тело сообщения полностью собранно
							if(this->_body.size == this->_body.chunk.size){
								// Очищаем собранные данные
								this->_body.chunk.clear();
								/**
								 * Определяем тип HTTP-модуля
								 */
								switch(static_cast <uint8_t> (this->_hid)){
									// Если мы работаем с клиентом
									case static_cast <uint8_t> (hid_t::CLIENT): {
										// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
										if(this->_callback.is("entityClient"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const uint32_t, const uint32_t, const string &, const buffer_t &)> ("entityClient", this->_id, this->_response.code, this->_response.message, this->_body.data);
									} break;
									// Если мы работаем с сервером
									case static_cast <uint8_t> (hid_t::SERVER): {
										// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
										if(this->_callback.is("entityServer"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const uint32_t, const method_t, const uri_t::url_t &, const buffer_t &)> ("entityServer", this->_id, this->_request.method, this->_request.url, this->_body.data);
									} break;
								}
								// Тело в запросе не передано
								this->_work.state = state_t::END;
								// Выходим из функции
								return result;
							}
						}
					// Если получение данных ведётся чанками
					} else {
						// Символ буфера в котором допущена ошибка
						char error = '\0';
						// Получаем размер смещения
						size_t offset = 0;
						// Переходим по всему буферу данных
						for(size_t i = 0; i < size; i++){
							/**
							 * Определяем стейт чанка
							 */
							switch(static_cast <uint8_t> (this->_body.chunk.state)){
								// Если мы собираем трейделы переданные сервером
								case static_cast <uint8_t> (chunk_t::state_t::TRAILERS): {
									// Устанавливаем смещение
									offset = (i + 1);
									// Запоминаем количество обработанных байт
									result = offset;
									// Если мы получили последний символ получения трейлеров
									if(buffer[i] == '\n'){
										// Если трейлеров в списке больше нет
										if(this->_trailers.empty())
											// Меняем стейт чанка на завершение сбора данных
											this->_body.chunk.state = chunk_t::state_t::STOP_BODY;
									// Если мы получили возврат каретки
									} else if(buffer[i] == '\r') {
										// Получаем заголовок переданного трейлера
										const string header(
											static_cast <const char *> (this->_body.chunk.buffer),
											static_cast <size_t> (this->_body.chunk.buffer)
										);
										// Выполняем поиск разделителя заголовка
										const size_t pos = header.find(':');
										// Если позиция разделителя найдена
										if(pos != string::npos){
											// Получаем название заголовка HTTP-протокола
											string name = header.substr(0, pos);
											// Получаем содержимое заголовка HTTP-протокола
											string content = header.substr(pos + 1);
											// Добавляем заголовок в список
											this->_headers.emplace(name, this->_fmk->transform(content, fmk_t::transform_t::TRIM));
											// Если функция обратного вызова на вывод полученного заголовка с сервера установлена
											if(this->_callback.is("header"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint32_t, const string &, const string &)> ("header", this->_id, name, content);
											// Выполняем поиск ключа заголовка в списке трейлеров
											auto i = this->_trailers.find(name);
											// Если трейлер найден в списке
											if(i != this->_trailers.end())
												// Выполняем удаление полученного трейлера
												this->_trailers.erase(i);
											// Если трейлер не соответствует
											else {
												// Устанавливаем код внутренней ошибки сервера
												// this->_response.code = 500;
												// Стираем сообщение ответа сервера
												this->_response.message = this->_fmk->format("Trailer \"%s\" does not exist", name.c_str());
												// Выполняем очистку списка трейлеров
												this->_trailers.clear();
												// Выводим сообщение об ошибке, что трейлер не существует
												this->_log->print("Trailer \"%s\" does not exist", log_t::flag_t::WARNING, name.c_str());
												// Если функция обратного вызова на на вывод ошибок установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_response.message.c_str());
												// Выполняем переход к ошибке
												goto Stop;
											}
										}
										// Выполняем сброс тела данных
										this->_body.chunk.buffer.clear();
									// Выполняем сборку трейлера, выполняем сборку размера чанка
									} else this->_body.chunk.buffer.push(buffer[i]);
								} break;
								// Если мы ожидаем получения размера тела чанка
								case static_cast <uint8_t> (chunk_t::state_t::SIZE): {
									// Если мы получили возврат каретки
									if(buffer[i] == '\r'){
										// Меняем стейт чанка
										this->_body.chunk.state = chunk_t::state_t::END_SIZE;
										// Получаем размер чанка
										this->_body.chunk.size = this->_fmk->atoi <size_t> (
											static_cast <const char *> (this->_body.chunk.buffer),
											static_cast <size_t> (this->_body.chunk.buffer), 16
										);
										// Устанавливаем смещение
										offset = (i + 1);
										// Запоминаем количество обработанных байт
										result = offset;
										// Выполняем сброс тела данных
										this->_body.chunk.buffer.clear();
										// Если размер тела слишком большой
										if((this->_body.chunk.size + this->_body.data.size()) > AWH_MAX_BODY_SIZE){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"HTTP-body is %s and is too large, the HTTP-body cannot exceed %s",
													__PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL,
													this->_fmk->bytes(static_cast <double> (this->_body.chunk.size + this->_body.data.size())).c_str(),
													this->_fmk->bytes(static_cast <double> (AWH_MAX_BODY_SIZE)).c_str()
												);
											/**
											* Если режим отладки не включён
											*/
											#else
												// Выводим сообщение об ошибке
												this->_log->print(
													"HTTP-body is %s and is too large, the HTTP-body cannot exceed %s",
													log_t::flag_t::CRITICAL,
													this->_fmk->bytes(static_cast <double> (this->_body.chunk.size + this->_body.data.size())).c_str(),
													this->_fmk->bytes(static_cast <double> (AWH_MAX_BODY_SIZE)).c_str()
												);
											#endif
											// Выполняем переход к ошибке
											goto Stop;
										}
									// Выполняем сборку 16-го размера чанка
									} else {
										// Запоминаем количество обработанных байт
										result = (i + 1);
										// Выполняем сборку размера чанка
										this->_body.chunk.buffer.push(buffer[i]);
									}
								} break;
								// Если мы ожидаем получение окончания сбора размера тела чанка
								case static_cast <uint8_t> (chunk_t::state_t::END_SIZE): {
									// Увеличиваем смещение
									offset = (i + 1);
									// Запоминаем количество обработанных байт
									result = offset;
									// Если мы получили перевод строки
									if(buffer[i] == '\n'){
										// Если размер получен 0-й значит мы завершили сбор данных
										if(this->_body.chunk.size == 0){
											// Если список трейлеров собран
											if(!this->_trailers.empty()){
												// Если мы работаем с клиентом
												if(this->_hid == hid_t::CLIENT){
													// Выполняем сброс тела данных
													this->_body.chunk.buffer.clear();
													// Меняем стейт чанка на получение трейлеров
													this->_body.chunk.state = chunk_t::state_t::TRAILERS;
												// Если мы работаем с сервером
												} else {
													// Выводим сообщение об ошибке
													this->_log->print("Client cannot transfer trailers", log_t::flag_t::WARNING);
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Client cannot transfer trailers");
												}
											// Меняем стейт чанка на завершение сбора данных
											} else this->_body.chunk.state = chunk_t::state_t::STOP_BODY;
										// Если данные собраны не полностью
										} else {
											// Если количества байт достаточно для сбора тела чанка
											if((size - offset) >= this->_body.chunk.size){
												// Меняем стейт чанка
												this->_body.chunk.state = chunk_t::state_t::STOP_BODY;
												// Собираем тело чанка
												this->_body.chunk.buffer.push(buffer + offset, this->_body.chunk.size);
												// Выполняем смещение итератора
												i = ((offset + this->_body.chunk.size) - 1);
												// Увеличиваем смещение
												offset += this->_body.chunk.size;
												// Запоминаем количество обработанных байт
												result = offset;
											// Если количества байт не достаточно для сбора тела
											} else {
												// Меняем стейт чанка
												this->_body.chunk.state = chunk_t::state_t::BODY;
												// Собираем тело чанка
												this->_body.chunk.buffer.push(buffer + offset, size - offset);
												// Запоминаем количество обработанных байт
												result = size;
												// Выходим из функции
												return result;
											}
										}
									// Если символ отличается, значит ошибка
									} else {
										// Устанавливаем символ ошибки
										error = 'n';
										// Выполняем переход к ошибке
										goto Stop;
									}
								} break;
								// Если мы ожидаем сбора тела чанка
								case static_cast <uint8_t> (chunk_t::state_t::BODY): {
									// Определяем количество необходимых байт
									size_t rem = (this->_body.chunk.size - this->_body.chunk.buffer.size());
									// Если количества байт достаточно для сбора тела чанка
									if(size >= rem){
										// Меняем стейт чанка
										this->_body.chunk.state = chunk_t::state_t::STOP_BODY;
										// Собираем тело чанка
										this->_body.chunk.buffer.push(buffer, rem);
										// Выполняем смещение итератора
										i = (rem - 1);
										// Увеличиваем смещение
										offset = rem;
										// Запоминаем количество обработанных байт
										result = offset;
									// Если количества байт не достаточно для сбора тела
									} else {
										// Собираем тело чанка
										this->_body.chunk.buffer.push(buffer, size);
										// Запоминаем количество обработанных байт
										result = size;
										// Выходим из функции
										return result;
									}
								} break;
								// Если мы ожидаем перевод строки после сбора данных тела чанка
								case static_cast <uint8_t> (chunk_t::state_t::STOP_BODY): {
									// Увеличиваем смещение
									offset = (i + 1);
									// Запоминаем количество обработанных байт
									result = offset;
									// Если мы получили возврат каретки
									if(buffer[i] == '\r')
										// Меняем стейт чанка
										this->_body.chunk.state = chunk_t::state_t::END_BODY;
									// Если символ отличается, значит ошибка
									else {
										// Устанавливаем символ ошибки
										error = 'r';
										// Выполняем переход к ошибке
										goto Stop;
									}
								} break;
								// Если мы ожидаем получение окончания сбора данных тела чанка
								case static_cast <uint8_t> (chunk_t::state_t::END_BODY): {
									// Увеличиваем смещение
									offset = (i + 1);
									// Запоминаем количество обработанных байт
									result = offset;
									// Если мы получили перевод строки
									if(buffer[i] == '\n'){
										// Если размер получен 0-й значит мы завершили сбор данных
										if(this->_body.chunk.size == 0)
											// Выполняем переход к ошибке
											goto Stop;
										// Если функция обратного вызова на перехват входящих чанков установлена
										else if(this->_callback.is("binary"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const uint32_t, const buffer_t &, const web_t *)> ("binary", this->_id, this->_body.chunk.buffer, this);
										// Выполняем очистку чанка
										this->_body.chunk.clear();
									// Если символ отличается, значит ошибка
									} else {
										// Устанавливаем символ ошибки
										error = 'n';
										// Выполняем переход к ошибке
										goto Stop;
									}
								} break;
							}
						}
						// Выходим из функции
						return result;
						// Устанавливаем метку выхода
						Stop:
						// Выполняем очистку чанка
						this->_body.chunk.clear();
						/**
						 * Определяем тип HTTP-модуля
						 */
						switch(static_cast <uint8_t> (this->_hid)){
							// Если мы работаем с клиентом
							case static_cast <uint8_t> (hid_t::CLIENT): {
								// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
								if(this->_callback.is("entityClient"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint32_t, const uint32_t, const string &, const buffer_t &)> ("entityClient", this->_id, this->_response.code, this->_response.message, this->_body.data);
							} break;
							// Если мы работаем с сервером
							case static_cast <uint8_t> (hid_t::SERVER): {
								// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
								if(this->_callback.is("entityServer"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint32_t, const method_t, const uri_t::url_t &, const buffer_t &)> ("entityServer", this->_id, this->_request.method, this->_request.url, this->_body.data);
							} break;
						}
						// Тело в запросе не передано
						this->_work.state = state_t::END;
						// Если мы получили ошибку обработки данных
						if(error != '\0'){
							// Сообщаем, что переданное тело содержит ошибки
							this->_log->print("Body chunk contains errors, [\\%c] is expected", log_t::flag_t::WARNING, error);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Body chunk contains errors, [\\%c] is expected", error));
						}
					}
				}
			} break;
			// Если производится работа с HTTP-заголовками
			case static_cast <uint8_t> (unit_t::HEADERS): {
				// Если мы собираем заголовки или стартовый запрос
				if((this->_work.state == state_t::HEADERS) || (this->_work.state == state_t::QUERY)){
					/**
					 * Определяем статус режима работы
					 */
					switch(static_cast <uint8_t> (this->_work.state)){
						// Если передан режим ожидания получения запроса
						case static_cast <uint8_t> (state_t::QUERY):
							// Устанавливаем разделитель
							this->_work.delim = ' ';
						break;
						// Если передан режим получения заголовков
						case static_cast <uint8_t> (state_t::HEADERS):
							// Устанавливаем разделитель
							this->_work.delim = ':';
						break;
					}
					/**
					 * Выполняем парсинг заголовков запроса
					 * @param buffer буфер бинарных данных
					 * @param size   размер бинарных данных
					 * @param stop   флаг завершения обработки данных
					 */
					::prepare(buffer, size, this->_work, result, ::move([this](const char * buffer, const size_t size, const bool stop) noexcept {
						// Если все данные получены
						if(stop){
							/**
							 * Выполняем отлов ошибок
							 */
							try {
								/**
								 * Определяем тип HTTP-модуля
								 */
								switch(static_cast <uint8_t> (this->_hid)){
									// Если мы работаем с клиентом
									case static_cast <uint8_t> (hid_t::CLIENT): {
										// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
										if(this->_callback.is("headersResponse"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const uint32_t, const uint32_t, const string &, const headers_t &)> ("headersResponse", this->_id, this->_response.code, this->_response.message, this->_headers);
									} break;
									// Если мы работаем с сервером
									case static_cast <uint8_t> (hid_t::SERVER): {
										// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
										if(this->_callback.is("headersRequest"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const uint32_t, const method_t, const uri_t::url_t &, const headers_t &)> ("headersRequest", this->_id, this->_request.method, this->_request.url, this->_headers);
									} break;
								}
								// Получаем размер тела HTTP-протокола
								const string & length = this->_headers["Content-Length"];
								// Если размер HTTP-тела запроса получен
								if(!length.empty()){
									// Запоминаем размер тела сообщения
									this->_body.size = static_cast <int32_t> (::stoul(length));
									// Если размер тела не получен
									if(this->_body.size == 0){
										// Запрашиваем заголовок подключения
										const string & header = this->_headers["Connection"];
										// Если заголовок подключения найден
										if(header.empty() || !this->_fmk->exists("close", header)){
											// Тело в запросе не передано
											this->_work.state = state_t::END;
											// Выходим из функции
											return;
										}
									// Если размер тела слишком большой
									} else if(this->_body.size > AWH_MAX_BODY_SIZE) {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"HTTP-body is %s and is too large, the HTTP-body cannot exceed %s",
												__PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL,
												this->_fmk->bytes(static_cast <double> (this->_body.size)).c_str(),
												this->_fmk->bytes(static_cast <double> (AWH_MAX_BODY_SIZE)).c_str()
											);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print(
												"HTTP-body is %s and is too large, the HTTP-body cannot exceed %s",
												log_t::flag_t::CRITICAL,
												this->_fmk->bytes(static_cast <double> (this->_body.size)).c_str(),
												this->_fmk->bytes(static_cast <double> (AWH_MAX_BODY_SIZE)).c_str()
											);
										#endif
										// Тело в запросе не передано
										this->_work.state = state_t::END;
										// Выходим из функции
										return;
									}
									// Устанавливаем стейт поиска тела запроса
									this->_work.state = state_t::BODY;
									// Продолжаем работу
									goto end;
								// Если тело приходит
								} else {
									// Выполняем перебор всего списка указанных заголовков
									for(auto & header : this->_headers.range("Transfer-Encoding")){
										// Если нужно получать размер тела чанками
										if(this->_fmk->exists("chunked", header)){
											// Устанавливаем стейт поиска тела запроса
											this->_work.state = state_t::BODY;
											// Продолжаем работу
											goto end;
										}
									}
								}
								// Тело в запросе не передано
								this->_work.state = state_t::END;
							/**
							 * Если возникает ошибка
							 */
							} catch(const exception & error) {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
								#endif
							}
							// Устанавливаем метку завершения работы
							end:
							// Выходим из функции
							return;
						// Если необходимо  получить оставшиеся данные
						} else if((size > 0) && (this->_work.pos[0] > -1)) {
							/**
							 * Определяем статус режима работы
							 */
							switch(static_cast <uint8_t> (this->_work.state)){
								// Если передан режим ожидания получения запроса
								case static_cast <uint8_t> (state_t::QUERY): {
									/**
									 * Определяем тип HTTP-модуля
									 */
									switch(static_cast <uint8_t> (this->_hid)){
										// Если мы работаем с клиентом
										case static_cast <uint8_t> (hid_t::CLIENT): {
											/**
											 * Выполняем отлов ошибок
											 */
											try {
												// Буфер для проверки протокола
												char protocol[4];
												// Копируем полученную строку
												::strncpy(protocol, buffer, 4);
												// Если мы получили ответ от сервера
												if(::strncmp(protocol, "HTTP", 4) == 0){
													// Выполняем очистку всех ранее полученных данных
													this->clear();
													// Выполняем сброс размера тела
													this->_body.size = -1;
													// Устанавливаем разделитель
													this->_work.delim = ':';
													// Устанавливаем стейт ожидания получения заголовков
													this->_work.state = state_t::HEADERS;
													// Получаем версию протокол запроса
													this->_response.version = ::stod(string(buffer + 5, this->_work.pos[0] - 5));
													// Получаем сообщение ответа
													this->_response.message.assign(buffer + (this->_work.pos[1] + 1), size - (this->_work.pos[1] + 1));
													// Получаем код ответа
													this->_response.code = static_cast <uint32_t> (::stoi(string(buffer + (this->_work.pos[0] + 1), this->_work.pos[1] - (this->_work.pos[0] + 1))));
													// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
													if(this->_callback.is("response"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const uint32_t, const string &)> ("response", this->_id, this->_response.code, this->_response.message);
												// Если данные пришли неправильные
												} else {
													// Выполняем очистку всех ранее полученных данных
													this->clear();
													// Сообщаем, что переданное тело содержит ошибки
													this->_log->print("Broken response server", log_t::flag_t::WARNING);
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken response server");
												}
											/**
											 * Если возникает ошибка
											 */
											} catch(const exception & error) {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
												/**
												* Если режим отладки не включён
												*/
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
												#endif
											}
										} break;
										// Если мы работаем с сервером
										case static_cast <uint8_t> (hid_t::SERVER): {
											/**
											 * Выполняем отлов ошибок
											 */
											try {
												// Буфер для проверки протокола
												char protocol[4];
												// Копируем полученную строку
												::strncpy(protocol, buffer + (this->_work.pos[1] + 1), 4);
												// Если мы получили ответ от сервера
												if(::strncmp(protocol, "HTTP", 4) == 0){
													// Выполняем очистку всех ранее полученных данных
													this->clear();
													// Выполняем сброс размера тела
													this->_body.size = -1;
													// Устанавливаем разделитель
													this->_work.delim = ':';
													// Выполняем смену стейта
													this->_work.state = state_t::HEADERS;
													// Получаем метод запроса
													const string method(buffer, this->_work.pos[0]);
													// Получаем параметры URI-запроса
													const string uri(buffer + (this->_work.pos[0] + 1), this->_work.pos[1] - (this->_work.pos[0] + 1));
													// Получаем версию протокол запроса
													this->_request.version = ::stod(string(buffer + (this->_work.pos[1] + 6), size - (this->_work.pos[1] + 6)));
													// Выполняем установку URI-параметров запроса
													this->_request.url = this->_uri.parse(uri);
													// Если метод определён как GET
													if(this->_fmk->compare("get", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::GET;
													// Если метод определён как PUT
													else if(this->_fmk->compare("put", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::PUT;
													// Если метод определён как POST
													else if(this->_fmk->compare("post", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::POST;
													// Если метод определён как HEAD
													else if(this->_fmk->compare("head", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::HEAD;
													// Если метод определён как DELETE
													else if(this->_fmk->compare("delete", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::DEL;
													// Если метод определён как PATCH
													else if(this->_fmk->compare("patch", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::PATCH;
													// Если метод определён как TRACE
													else if(this->_fmk->compare("trace", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::TRACE;
													// Если метод определён как OPTIONS
													else if(this->_fmk->compare("options", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::OPTIONS;
													// Если метод определён как CONNECT
													else if(this->_fmk->compare("connect", method))
														// Выполняем установку метода запроса
														this->_request.method = method_t::CONNECT;
													// Если функция обратного вызова на вывод запроса клиента на выполненный запрос к серверу установлена
													if(this->_callback.is("request"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const method_t, const uri_t::url_t &)> ("request", this->_id, this->_request.method, this->_request.url);
												// Если данные пришли неправильные
												} else {
													// Выполняем очистку всех ранее полученных данных
													this->clear();
													// Сообщаем, что переданное тело содержит ошибки
													this->_log->print("Broken request client", log_t::flag_t::WARNING);
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken request client");
												}
											/**
											 * Если возникает ошибка
											 */
											} catch(const exception & error) {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
												/**
												* Если режим отладки не включён
												*/
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
												#endif
											}
										} break;
									}
								} break;
								// Если передан режим получения заголовков
								case static_cast <uint8_t> (state_t::HEADERS): {
									/**
									 * Выполняем отлов ошибок
									 */
									try {
										// Получаем название заголовка HTTP-протокола
										string name(buffer, this->_work.pos[0]);
										// Получаем содержимое заголовка HTTP-протокола
										string content(buffer + (this->_work.pos[0] + 1), size - (this->_work.pos[0] + 1));
										// Добавляем заголовок в список заголовков
										if(!name.empty() && !content.empty()){
											// Если название заголовка соответствует HOST
											if(this->_fmk->compare("host", name)){
												// Создаём объект работы с IP-адресами
												net_t net(this->_log);
												// Выполняем установку порта по умолчанию
												this->_request.url.port = 80;
												// Выполняем установку схемы запроса
												this->_request.url.schema = "http";
												// Выполняем установку хоста
												this->_request.url.host = this->_fmk->transform(content, fmk_t::transform_t::TRIM);
												// Выполняем поиск разделителя
												const size_t pos = this->_request.url.host.rfind(':');
												// Если разделитель найден
												if(pos != string::npos){
													// Получаем порт сервера
													const string & port = this->_request.url.host.substr(pos + 1);
													// Если данные порта являются числом
													if(this->_fmk->is(port, fmk_t::check_t::NUMBER)){
														// Выполняем установку порта сервера
														this->_request.url.port = static_cast <uint32_t> (::stoi(port));
														// Выполняем получение хоста сервера
														this->_request.url.host = this->_request.url.host.substr(0, pos);
														// Если порт установлен как 443
														if(this->_request.url.port == 443)
															// Выполняем установку защищённую схему запроса
															this->_request.url.schema = "https";
													}
												}
												/**
												 * Определяем тип домена
												 */
												switch(static_cast <uint8_t> (net.host(this->_request.url.host))){
													// Если передан IP-адрес сети IPv4
													case static_cast <uint8_t> (net_t::type_t::IPV4): {
														// Выполняем установку семейства IP-адресов
														this->_request.url.family = AF_INET;
														// Выполняем установку IPv4 адреса
														this->_request.url.ip = this->_request.url.host;
													} break;
													// Если передан IP-адрес сети IPv6
													case static_cast <uint8_t> (net_t::type_t::IPV6): {
														// Выполняем установку семейства IP-адресов
														this->_request.url.family = AF_INET6;
														// Выполняем установку IPv6 адреса
														this->_request.url.ip = net = this->_request.url.host;
													} break;
													// Если передана доменная зона
													case static_cast <uint8_t> (net_t::type_t::FQDN):
														// Выполняем установку IPv6 адреса
														this->_request.url.domain = this->_fmk->transform(this->_request.url.host, fmk_t::transform_t::LOWER);
													break;
												}
											// Если название заголовка соответствует переключению протокола
											} else if(this->_fmk->compare("upgrade", name)) {
												// Если протокол принадлежит Websocket
												if(this->_fmk->compare("websocket", content))
													// Выполняем установку типа протокола для переключению на Websocket
													this->_work.upgrade = proto_t::WEBSOCKET;
												// Если протокол принадлежит HTTP/2
												else if(this->_fmk->compare("HTTP/2.0", content) || this->_fmk->compare("h2c", content))
													// Выполняем установку типа протокола для переключению на HTTP/2
													this->_work.upgrade = proto_t::HTTP2;
												// Устанавливаем тип протокола как неизвестный
												else this->_work.upgrade = proto_t::UNKNOWN;
											// Если название заголовка соответствует трейлеру
											} else if(this->_fmk->compare("trailer", name)) {
												// Выполняем сбор трейлеров
												this->_trailers.emplace(this->_fmk->transform(this->_fmk->transform(content, fmk_t::transform_t::TRIM), fmk_t::transform_t::LOWER));
												// Выводим результат
												return;
											}
											// Добавляем заголовок в список
											this->_headers.emplace(
												this->_fmk->transform(name, fmk_t::transform_t::LOWER),
												this->_fmk->transform(content, fmk_t::transform_t::TRIM)
											);
											// Если функция обратного вызова на вывод полученного заголовка с сервера установлена
											if(this->_callback.is("header"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint32_t, const string &, const string &)> ("header", this->_id, name, content);
										}
									/**
									 * Если возникает ошибка
									 */
									} catch(const exception & error) {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
										#endif
									}
								} break;
							}
						}
					}));
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод фиксирования добавленных данных
 *
 */
void awh::Web::commit() noexcept {
	// Фиксируем полученный результат
	this->_work.state = state_t::END;
}
/**
 * @brief Метод сброса стейтов парсера
 *
 */
void awh::Web::reset() noexcept {
	// Выполняем сброс рабочих параметров
	this->_work = work_t();
}
/**
 * @brief Метод очистки собранных данных
 *
 */
void awh::Web::clear() noexcept {
	// Выполняем очистку тела HTTP-запроса
	this->_body.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->_headers.clear();
	// Выполняем сброс списка трейлеров
	this->_trailers.clear();
	// Выполняем сброс параметров запроса
	this->_request = req_t();
	// Выполняем сброс параметров ответа
	this->_response = res_t();
}
/**
 * @brief Метод очистки данных HTTP-юнита
 *
 * @param unit HTTP-юнит данные которого очищаются
 */
void awh::Web::clear(const unit_t unit) noexcept {
	/**
	 * Определяем текущий HTTP-юнит с которым производится работа
	 */
	switch(static_cast <uint8_t> (unit)){
		// Если производится работы с HTTP-телом
		case static_cast <uint8_t> (unit_t::BODY):
			// Выполняем очистку данных тела
			this->_body.clear();
		break;
		// Если производится работа с HTTP-заголовками
		case static_cast <uint8_t> (unit_t::HEADERS):
			// Выполняем очистку заголовков
			this->_headers.clear();
		break;
	}
}
/**
 * @brief Метод проверки завершения обработки
 *
 * @return результат проверки
 */
bool awh::Web::finish() const noexcept {
	// Выводрим результат проверки
	return (this->_work.state == state_t::END);
}
/**
 * @brief Проверка заголовка HTTP-протокола является ли он стандартным
 *
 * @param name название заголовка HTTP-протокола для проверки
 * @return     результат проверки
 */
bool awh::Web::standard(const string name) const noexcept {
	// Если ключ передан
	if(!name.empty())
		// Выполняем проверку заголовка
		return (this->_standards.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER)) != this->_standards.end());
	// Выводим результат
	return false;
}
/**
 * @brief Метод получения идентификатора объекта
 *
 * @return идентификатор объекта
 */
uint32_t awh::Web::id() const noexcept {
	// Выводим идентификатор объекта
	return this->_id;
}
/**
 * @brief Метод установки идентификатора объекта
 *
 * @param id идентификатор объекта
 */
void awh::Web::id(const uint32_t id) noexcept {
	// Выполняем установку идентификатора объекта
	this->_id = id;
}
/**
 * @brief Метод вывода идентификатора модуля
 *
 * @return тип используемого HTTP-модуля
 */
const awh::Web::hid_t awh::Web::hid() const noexcept {
	// Выводим тип используемого HTTP-модуля
	return this->_hid;
}
/**
 * @brief Метод установки идентификатора модуля
 *
 * @param hid тип используемого HTTP-модуля
 */
void awh::Web::hid(const hid_t hid) noexcept {
	// Устанавливаем тип используемого HTTP-модуля
	this->_hid = hid;
}
/**
 * @brief Метод получения бинарного дампа
 *
 * @return бинарный дамп данных
 */
awh::buffer_t awh::Web::dump() const noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	{
		// Устанавливаем идентификатор HTTP-модуля
		result.push(this->_id);
		// Устанавливаем тип используемого HTTP-модуля
		result.push(static_cast <uint8_t> (this->_hid));
		// Устанавливаем объект рабочих параметров
		result.push(&this->_work, sizeof(this->_work));
		// Устанавливаем код ответа на HTTP-ответа
		result.push(this->_response.code);
		// Устанавливаем версию протокола HTTP-ответа
		result.push(this->_response.version);
		// Устанавливаем метод HTTP-запроса
		result.push(static_cast <uint8_t> (this->_request.method));
		// Устанавливаем версию протокола HTTP-запроса
		result.push(this->_request.version);
		// Если URL-адрес запроса установлен
		if(!this->_request.url.empty()){
			// Получаем адрес URL-запроса
			const string & url = this->_uri.url(this->_request.url);
			// Устанавливаем размер записи параметров HTTP-запроса
			result.push(url.size());
			// Устанавливаем параметры HTTP-запроса
			result.push(url.data(), url.size());
		// Устанавливаем размер записи параметров HTTP-запроса
		} else result.push(static_cast <size_t> (0));
		// Если текст ответа установлен
		if(!this->_response.message.empty()){
			// Устанавливаем размер сообщения HTTP-ответа
			result.push(this->_response.message.length());
			// Устанавливаем данные сообщения HTTP-ответа
			result.push(this->_response.message);
		// Устанавливаем размер записи параметров HTTP-запроса
		} else result.push(static_cast <size_t> (0));
		// Устанавливаем размер тела сообщения
		result.push(this->_body.size);
		// Устанавливаем размер тела сообщения
		result.push(this->_body.data.size());
		// Устанавливаем данные тела сообщения
		result.push(this->_body.data);
		// Устанавливаем количество HTTP-заголовков
		result.push(this->_headers.count());
		// Выполняем перебор всех HTTP-заголовков
		for(auto & header : const_cast <web_t *> (this)->_headers){
			// Устанавливаем размер названия HTTP заголовка
			result.push(header.first.size());
			// Устанавливаем данные названия HTTP заголовка
			result.push(header.first);
			// Устанавливаем размер значения HTTP заголовка
			result.push(header.second.size());
			// Устанавливаем данные значения HTTP заголовка
			result.push(header.second);
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки бинарного дампа
 *
 * @param data бинарный дамп данных
 */
void awh::Web::dump(const buffer_t & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty())
		// Выполняем установку дампа данных
		this->dump(static_cast <const char *> (data), static_cast <size_t> (data));
}
/**
 * @brief Метод установки бинарного дампа
 *
 * @param buffer буфер бинарных данных
 * @param size   размер бинарных данных
 */
void awh::Web::dump(const char * buffer, const size_t size) noexcept {
	// Если данные бинарного дампа переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Длина строки, количество элементов и смещение в буфере
			size_t length = 0, count = 0, offset = 0;
			// Выполняем получение идентификатора HTTP-модуля
			::memcpy(&this->_id, buffer + offset, sizeof(this->_id));
			// Выполняем смещение в буфере
			offset += sizeof(this->_id);
			// Выполняем получение типа используемого HTTP-модуля
			::memcpy(&this->_hid, buffer + offset, sizeof(this->_hid));
			// Выполняем смещение в буфере
			offset += sizeof(this->_hid);
			// Выполняем получение массива позиций в буфере сепаратора
			::memcpy(&this->_work.pos, buffer + offset, sizeof(this->_work.pos));
			// Выполняем смещение в буфере
			offset += sizeof(this->_work.pos);
			// Выполняем получение стейта текущего запроса
			::memcpy(&this->_work.state, buffer + offset, sizeof(this->_work.state));
			// Выполняем смещение в буфере
			offset += sizeof(this->_work.state);
			// Выполняем получение сепаратора для детекции в буфере
			::memcpy(&this->_work.delim, buffer + offset, sizeof(this->_work.delim));
			// Выполняем смещение в буфере
			offset += sizeof(this->_work.delim);
			// Выполняем получение кода ответа на HTTP-запрос
			::memcpy(&this->_response.code, buffer + offset, sizeof(this->_response.code));
			// Выполняем смещение в буфере
			offset += sizeof(this->_response.code);
			// Выполняем получение версии протокола HTTP ответа
			::memcpy(&this->_response.version, buffer + offset, sizeof(this->_response.version));
			// Выполняем смещение в буфере
			offset += sizeof(this->_response.version);
			// Выполняем получение метода HTTP-запроса
			::memcpy(&this->_request.method, buffer + offset, sizeof(this->_request.method));
			// Выполняем смещение в буфере
			offset += sizeof(this->_request.method);
			// Выполняем получение версии протокола HTTP-запроса
			::memcpy(&this->_request.version, buffer + offset, sizeof(this->_request.version));
			// Выполняем смещение в буфере
			offset += sizeof(this->_request.version);
			// Выполняем получение размера записи параметров HTTP-запроса
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если URL-адрес запроса установлен
			if(length > 0){
				// Создаём URL-адрес запроса
				string url(length, 0);
				// Выполняем получение параметров HTTP-запроса
				::memcpy(url.data(), buffer + offset, length);
				// Устанавливаем URL-адрес запроса
				this->_request.url = this->_uri.parse(url);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера сообщения HTTP ответа
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если сообщение ответа установлено
			if(length > 0){
				// Выделяем память для сообщения HTTP ответа
				this->_response.message.resize(length, 0);
				// Выполняем получение сообщения HTTP ответа
				::memcpy(this->_response.message.data(), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера тела сообщения
			::memcpy(&this->_body.size, buffer + offset, sizeof(this->_body.size));
			// Выполняем смещение в буфере
			offset += sizeof(this->_body.size);
			// Выполняем получение размера тела сообщения
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если сообщение ответа установлено
			if(length > 0){
				// Выполняем получение данных тела сообщения
				this->_body.data.push(buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение количества HTTP заголовков
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем сброс заголовков
			this->_headers.clear();
			// Если количество заголовков больше чем ничего
			if(count > 0){
				// Выполняем последовательную загрузку всех заголовков
				for(size_t i = 0; i < count; i++){
					// Выполняем получение размера названия HTTP заголовка
					::memcpy(&length, buffer + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выпделяем память для названия заголовка HTTP-протокола
						string name(length, 0);
						// Выполняем получение ключа заголовка
						::memcpy(name.data(), buffer + offset, length);
						// Выполняем смещение в буфере
						offset += length;
						// Выполняем получение размера значения HTTP заголовка
						::memcpy(&length, buffer + offset, sizeof(length));
						// Выполняем смещение в буфере
						offset += sizeof(length);
						// Если размер получен
						if(length > 0){
							// Выпделяем память для значения заголовка
							string value(length, 0);
							// Выполняем получение значения заголовка
							::memcpy(value.data(), buffer + offset, length);
							// Выполняем смещение в буфере
							offset += length;
							// Если и ключ и значение заголовка получены
							if(!name.empty() && !value.empty())
								// Добавляем заголовок в список заголовков
								this->_headers.emplace(name, value);
						}
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод выполнения парсинга HTTP буфера данных
 *
 * @param buffer буфер данных для парсинга
 * @param size   размер буфера данных для парсинга
 * @return       размер обработанных данных
 */
size_t awh::Web::parse(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы или обработка полностью выполнена
	if((buffer != nullptr) && (size > 0) && (this->_work.state != state_t::END)){
		/**
		 * Определяем текущий стейт
		 */
		switch(static_cast <uint8_t> (this->_work.state)){
			// Если установлен стейт чтения параметров запроса/ответа
			case static_cast <uint8_t> (state_t::QUERY):
			// Если установлен стейт чтения заголовков
			case static_cast <uint8_t> (state_t::HEADERS): {
				// Выполняем чтение заголовков
				result = this->extraction(buffer, size, unit_t::HEADERS);
				// Если требуется продолжить извлечение данных тела сообщения
				if((result < size) && (this->_work.state == state_t::BODY))
					// Выполняем извлечение данных тела сообщения
					result += this->extraction(buffer + result, size - result, unit_t::BODY);
			} break;
			// Если установлен стейт чтения полезной нагрузки
			case static_cast <uint8_t> (state_t::BODY):
				// Выполняем извлечение данных тела сообщения
				result = this->extraction(buffer, size, unit_t::BODY);
			break;
		}
	}
	// Выводим реузльтат
	return result;
}
/**
 * @brief Метод получения объекта запроса на сервер
 *
 * @return объект запроса на сервер
 */
const awh::Web::req_t & awh::Web::request() const noexcept {
	// Выводим объект запроса на сервер
	return this->_request;
}
/**
 * @brief Метод установки объекта запроса на сервер
 *
 * @param request объект запроса на сервер
 */
void awh::Web::request(req_t && request) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_request = ::move(request);
}
/**
 * @brief Метод установки объекта запроса на сервер
 *
 * @param request объект запроса на сервер
 */
void awh::Web::request(const req_t & request) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_request = request;
}
/**
 * @brief Метод получения объекта ответа сервера
 *
 * @return объект ответа сервера
 */
const awh::Web::res_t & awh::Web::response() const noexcept {
	// Выводим объект ответа сервера
	return this->_response;
}
/**
 * @brief Метод установки объекта ответа сервера
 *
 * @param response объект ответа сервера
 */
void awh::Web::response(res_t && response) noexcept {
	// Устанавливаем объект ответа сервера
	this->_response = ::move(response);
}
/**
 * @brief Метод установки объекта ответа сервера
 *
 * @param response объект ответа сервера
 */
void awh::Web::response(const res_t & response) noexcept {
	// Устанавливаем объект ответа сервера
	this->_response = response;
}
/**
 * @brief Метод получения данных тела HTTP-протокола
 *
 * @return буфер данных тела HTTP-протокола
 */
awh::buffer_t & awh::Web::body() noexcept {
	// Выводим данные тела HTTP-протокола
	return this->_body.data;
}
/**
 * @brief Метод переноса данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для переноса
 */
void awh::Web::body(buffer_t && body) noexcept {
	// Выполняем перенос данных тела HTTP-протокола
	this->_body.data = ::move(body);
}
/**
 * @brief Метод установки данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для установки
 */
void awh::Web::body(const buffer_t & body) noexcept {
	// Выполняем установку данных тела HTTP-протокола
	this->_body.data = body;
}
/**
 * @brief Метод перемещения данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для установки
 */
void awh::Web::body(vector <char> && body) noexcept {
	// Выполняем установку данных тела HTTP-протокола
	this->_body.data = ::move(body);
}
/**
 * @brief Метод установки данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для установки
 */
void awh::Web::body(const vector <char> & body) noexcept {
	// Выполняем установку данных тела HTTP-протокола
	this->_body.data = body;
}
/**
 * @brief Метод добавления данных тела HTTP-протокола
 *
 * @param buffer буфер тела HTTP-протокола для добавления
 * @param size   размер буфера теля HTTP-протокола для добавления
 */
void awh::Web::body(const char * buffer, const size_t size) noexcept {
	// Если тело данных передано
	if((buffer != nullptr) && (size > 0))
		// Выполняем добавление данных тела HTTP-протокола
		this->_body.data.push(buffer, size);
}
/**
 * @brief Метод получения списка заголовков HTTP-протокола
 *
 * @return список существующих заголовков HTTP-протокола
 */
awh::headers_t & awh::Web::headers() noexcept {
	// Выводим список доступных заголовков HTTP-протокола
	return this->_headers;
}
/**
 * @brief Метод переноса списка заголовков HTTP-протокола
 *
 * @param headers список заголовков HTTP-протокола для переноса
 */
void awh::Web::headers(headers_t && headers) noexcept {
	// Выполняем перенос заголовков HTTP-протокола
	this->_headers = ::move(headers);
}
/**
 * @brief Метод установки списка заголовков HTTP-протокола
 *
 * @param headers список заголовков HTTP-протокола для установки
 */
void awh::Web::headers(const headers_t & headers) noexcept {
	// Выполняем установку заголовков HTTP-протокола
	this->_headers = headers;
}
/**
 * @brief Метод получения данных заголовка HTTP-протокола
 *
 * @param name название заголовка HTTP-протокола
 * @return     содержимое заголовка HTTP-протокола
 */
const string & awh::Web::header(const string & name) const noexcept {
	// Выводим запрашиваемый заголовок HTTP-протокола
	return this->_headers.at(name);
}
/**
 * @brief Метод добавления заголовка HTTP-протокола
 *
 * @param name    название заголовка HTTP-протокола
 * @param content содержимое заголовка HTTP-протокола
 */
void awh::Web::header(const string & name, const string & content) noexcept {
	// Если даныне заголовка HTTP-протокола переданы
	if(!name.empty() && !content.empty())
		// Выполняем добавление передаваемого заголовка HTTP-протокола
		this->_headers.emplace(name, content);
}
/**
 * @brief Метод получение типа протокола для переключения
 *
 * @return тип протокола для переключения
 */
const awh::web_t::proto_t awh::Web::upgrade() const noexcept {
	// Выполняем вывод название протокола для переключения
	return this->_work.upgrade;
}
/**
 * @brief Метод установки типа протокола для переключения
 *
 * @param upgrade тип протокола для переключения
 */
void awh::Web::upgrade(const proto_t upgrade) noexcept {
	// Выполняем установку названия протокола для переключения
	this->_work.upgrade = upgrade;
}
/**
 * @brief Метод извлечения список протоколов к которому принадлежит заголовок HTTP-протокола
 *
 * @param name название заголовка HTTP-протокола
 * @return     список соответствующих протоколов
 */
const std::set <awh::Web::proto_t> & awh::Web::proto(const string name) const noexcept {
	// Результат работы функции
	static const std::set <awh::Web::proto_t> result;
	// Если название заголовка HTTP-протокола передано
	if(!name.empty()){
		// Выполняем поиск заголовка HTTP-протокола
		auto i = this->_standards.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER));
		// Если заголовок HTTP-протокола найден
		if(i != this->_standards.end())
			// Выводим результат
			return i->second;
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::Web::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callback.set("error", callback);
	// Выполняем установку функции вывода полученного заголовка с сервера
	this->_callback.set("header", callback);
	// Выполняем установку функции вывода ответа сервера на ранее выполненный запрос
	this->_callback.set("response", callback);
	// Выполняем установку функции вывода запроса клиента на выполненный запрос к серверу
	this->_callback.set("request", callback);
	// Выполняем установку функции обратного вывода полученного тела данных с сервера
	this->_callback.set("entityServer", callback);
	// Выполняем установку функции обратного вывода полученного тела данных с клиента
	this->_callback.set("entityClient", callback);
	// Выполняем установку функции вывода полученных заголовков с сервера
	this->_callback.set("headersRequest", callback);
	// Выполняем установку функции вывода полученных заголовков с клинета
	this->_callback.set("headersResponse", callback);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Web::Web(const fmk_t * fmk, const log_t * log) noexcept :
 _hid(hid_t::NONE), _id(0), _uri(fmk, log), _body(fmk, log),
 _headers(fmk, log), _callback(log), _fmk(fmk), _log(log) {
	// Выполняем первоначальную инициализацию модуля
	this->init();
}
/**
 * @brief Конструктор
 *
 * @param hid тип используемого HTTP-модуля
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Web::Web(const hid_t hid, const fmk_t * fmk, const log_t * log) noexcept :
 _hid(hid), _id(0), _uri(fmk, log), _body(fmk, log),
 _headers(fmk, log), _callback(log), _fmk(fmk), _log(log) {
	// Выполняем первоначальную инициализацию модуля
	this->init();
}
