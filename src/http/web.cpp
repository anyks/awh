/**
 * @file: web.cpp
 * @date: 2021-12-19
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
#include <http/web.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Оператор [=] перемещения параметров запроса клиента
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
 * Оператор [=] присванивания параметров запроса клиента
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
 * Оператор сравнения
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
 * Request Конструктор перемещения
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
 * Request Конструктор копирования
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
 * Request Конструктор
 */
awh::Web::Request::Request() noexcept : provider_t(), method(method_t::NONE) {}
/**
 * Request Конструктор
 * @param method метод запроса клиента
 */
awh::Web::Request::Request(const method_t method) noexcept : provider_t(), method(method) {}
/**
 * Request Конструктор
 * @param version версия протокола
 */
awh::Web::Request::Request(const double version) noexcept : provider_t(version), method(method_t::NONE) {}
/**
 * Request Конструктор
 * @param url адрес URL-запроса
 */
awh::Web::Request::Request(const uri_t::url_t & url) noexcept : provider_t(), method(method_t::NONE), url(url) {}
/**
 * Request Конструктор
 * @param version версия протокола
 * @param method  метод запроса клиента
 */
awh::Web::Request::Request(const double version, const method_t method) noexcept : provider_t(version), method(method) {}
/**
 * Request Конструктор
 * @param method метод запроса клиента
 * @param url    адрес URL-запроса
 */
awh::Web::Request::Request(const method_t method, const uri_t::url_t & url) noexcept : provider_t(), method(method), url(url) {}
/**
 * Request Конструктор
 * @param version версия протокола
 * @param url     адрес URL-запроса
 */
awh::Web::Request::Request(const double version, const uri_t::url_t & url) noexcept : provider_t(version), method(method_t::NONE), url(url) {}
/**
 * Request Конструктор
 * @param version версия протокола
 * @param method  метод запроса клиента
 * @param url     адрес URL-запроса
 */
awh::Web::Request::Request(const double version, const method_t method, const uri_t::url_t & url) noexcept : provider_t(version), method(method), url(url) {}
/**
 * Оператор [=] перемещения параметров ответа сервера
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
 * Оператор [=] присванивания параметров ответа сервера
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
 * Оператор сравнения
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
 * Response Конструктор перемещения
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
 * Response Конструктор копирования
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
 * Response Конструктор
 */
awh::Web::Response::Response() noexcept : provider_t(), code(0), message{""} {}
/**
 * Response Конструктор
 * @param code код ответа сервера
 */
awh::Web::Response::Response(const uint32_t code) noexcept : provider_t(), code(code), message{""} {}
/**
 * Response Конструктор
 * @param version версия протокола
 */
awh::Web::Response::Response(const double version) noexcept : provider_t(version), code(0), message{""} {}
/**
 * Response Конструктор
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const string & message) noexcept : provider_t(), code(0), message(message) {}
/**
 * Response Конструктор
 * @param version версия протокола
 * @param code    код ответа сервера
 */
awh::Web::Response::Response(const double version, const uint32_t code) noexcept : provider_t(version), code(code), message{""} {}
/**
 * Response Конструктор
 * @param code    код ответа сервера
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const uint32_t code, const string & message) noexcept : provider_t(), code(code), message(message) {}
/**
 * Response Конструктор
 * @param version версия протокола
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const double version, const string & message) noexcept : provider_t(version), code(0), message(message) {}
/**
 * Response Конструктор
 * @param version версия протокола
 * @param code    код ответа сервера
 * @param message сообщение сервера
 */
awh::Web::Response::Response(const double version, const uint32_t code, const string & message) noexcept : provider_t(version), code(code), message(message) {}
/**
 * clear Метод очистки данных чанка
 */
void awh::Web::Chunk::clear() noexcept {
	// Обнуляем размер чанка
	this->size = 0;
	// Обнуляем буфер данных
	this->data.clear();
	// Выполняем сброс стейта чанка
	this->state = process_t::SIZE;
	// Если размер выделенной памяти выше максимального размера чанка
	if(this->data.capacity() > AWH_CHUNK_SIZE)
		// Выполняем удаление памяти чанка
		vector <char> ().swap(this->data);
}
/**
 * parseBody Метод извлечения полезной нагрузки
 * @param buffer буфер данных для чтения
 * @param size   размер буфера данных для чтения
 * @return       размер обработанных данных
 */
size_t awh::Web::readPayload(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы
	if((buffer != nullptr) && (size > 0) && (this->_state != state_t::END)){
		// Если мы собираем тело полезной нагрузки
		if(this->_state == state_t::BODY){
			// Если размер тела сообщения получен
			if(this->_bodySize > -1){
				// Если размер тела не получен
				if(this->_bodySize == 0){
					// Запоминаем количество обработанных байт
					result = size;
					// Заполняем собранные данные тела
					this->_chunk.data.assign(buffer, buffer + result);
					// Если функция обратного вызова на перехват входящих чанков установлена
					if(this->_callback.is("binary"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const vector <char> &, const web_t *)> ("binary", this->_id, this->_chunk.data, this);
				// Если размер установлен конкретный
				} else {
					// Получаем актуальный размер тела
					result = (this->_bodySize - this->_body.size());
					// Фиксируем актуальный размер тела
					result = (size > result ? result : size);
					// Увеличиваем общий размер полученных данных
					this->_chunk.size += result;
					// Заполняем собранные данные тела
					this->_chunk.data.assign(buffer, buffer + result);
					// Если функция обратного вызова на перехват входящих чанков установлена
					if(this->_callback.is("binary"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const vector <char> &, const web_t *)> ("binary", this->_id, this->_chunk.data, this);
					// Если тело сообщения полностью собранно
					if(this->_bodySize == this->_chunk.size){
						// Очищаем собранные данные
						this->_chunk.clear();
						// Определяем тип HTTP-модуля
						switch(static_cast <uint8_t> (this->_hid)){
							// Если мы работаем с клиентом
							case static_cast <uint8_t> (hid_t::CLIENT): {
								// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
								if(this->_callback.is("entityClient"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entityClient", this->_id, this->_response.code, this->_response.message, this->_body);
							} break;
							// Если мы работаем с сервером
							case static_cast <uint8_t> (hid_t::SERVER): {
								// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
								if(this->_callback.is("entityServer"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &)> ("entityServer", this->_id, this->_request.method, this->_request.url, this->_body);
							} break;
						}
						// Тело в запросе не передано
						this->_state = state_t::END;
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
					// Определяем стейт чанка
					switch(static_cast <uint8_t> (this->_chunk.state)){
						// Если мы собираем трейделы переданные сервером
						case static_cast <uint8_t> (process_t::TRAILERS): {
							// Устанавливаем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили последний символ получения трейлеров
							if(buffer[i] == '\n'){
								// Если трейлеров в списке больше нет
								if(this->_trailers.empty())
									// Меняем стейт чанка на завершение сбора данных
									this->_chunk.state = process_t::STOP_BODY;
							// Если мы получили возврат каретки
							} else if(buffer[i] == '\r') {
								// Получаем заголовок переданного трейлера
								const string header(this->_chunk.data.begin(), this->_chunk.data.end());
								// Выполняем поиск разделителя заголовка
								const size_t pos = header.find(':');
								// Если позиция разделителя найдена
								if(pos != string::npos){
									// Получаем ключ заголовка
									string key = header.substr(0, pos);
									// Получаем значение заголовка
									string val = header.substr(pos + 1);
									// Добавляем заголовок в список
									this->_headers.emplace(
										this->_fmk->transform(key, fmk_t::transform_t::LOWER),
										this->_fmk->transform(val, fmk_t::transform_t::TRIM)
									);
									// Если функция обратного вызова на вывод полученного заголовка с сервера установлена
									if(this->_callback.is("header"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const uint64_t,const string &, const string &)> ("header", this->_id, key, val);
									// Выполняем поиск ключа заголовка в списке трейлеров
									auto i = this->_trailers.find(key);
									// Если трейлер найден в списке
									if(i != this->_trailers.end())
										// Выполняем удаление полученного трейлера
										this->_trailers.erase(i);
									// Если трейлер не соответствует
									else {
										// Устанавливаем код внутренней ошибки сервера
										// this->_response.code = 500;
										// Стираем сообщение ответа сервера
										this->_response.message = this->_fmk->format("Trailer \"%s\" does not exist", key.c_str());
										// Выполняем очистку списка трейлеров
										this->_trailers.clear();
										// Выводим сообщение об ошибке, что трейлер не существует
										this->_log->print("Trailer \"%s\" does not exist", log_t::flag_t::WARNING, key.c_str());
										// Если функция обратного вызова на на вывод ошибок установлена
										if(this->_callback.is("error"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_response.message.c_str());
										// Выполняем переход к ошибке
										goto Stop;
									}
								}
								// Выполняем сброс тела данных
								this->_chunk.data.clear();
							// Выполняем сборку трейлера, выполняем сборку размера чанка
							} else this->_chunk.data.push_back(buffer[i]);
						} break;
						// Если мы ожидаем получения размера тела чанка
						case static_cast <uint8_t> (process_t::SIZE): {
							// Если мы получили возврат каретки
							if(buffer[i] == '\r'){
								// Меняем стейт чанка
								this->_chunk.state = process_t::END_SIZE;
								// Получаем размер чанка
								this->_chunk.size = this->_fmk->atoi <size_t> (string(
									this->_chunk.data.begin(),
									this->_chunk.data.end()
								), 16);
								// Устанавливаем смещение
								offset = (i + 1);
								// Запоминаем количество обработанных байт
								result = offset;
								// Выполняем сброс тела данных
								this->_chunk.data.clear();
							// Выполняем сборку 16-го размера чанка
							} else {
								// Запоминаем количество обработанных байт
								result = (i + 1);
								// Выполняем сборку размера чанка
								this->_chunk.data.push_back(buffer[i]);
							}
						} break;
						// Если мы ожидаем получение окончания сбора размера тела чанка
						case static_cast <uint8_t> (process_t::END_SIZE): {
							// Увеличиваем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили перевод строки
							if(buffer[i] == '\n'){
								// Если размер получен 0-й значит мы завершили сбор данных
								if(this->_chunk.size == 0){
									// Если список трейлеров собран
									if(!this->_trailers.empty()){
										// Если мы работаем с клиентом
										if(this->_hid == hid_t::CLIENT){
											// Выполняем сброс тела данных
											this->_chunk.data.clear();
											// Меняем стейт чанка на получение трейлеров
											this->_chunk.state = process_t::TRAILERS;
										// Если мы работаем с сервером
										} else {
											// Выводим сообщение об ошибке
											this->_log->print("Client cannot transfer trailers", log_t::flag_t::WARNING);
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Client cannot transfer trailers");
										}
									// Меняем стейт чанка на завершение сбора данных
									} else this->_chunk.state = process_t::STOP_BODY;
								// Если данные собраны не полностью
								} else {
									// Если количества байт достаточно для сбора тела чанка
									if((size - offset) >= this->_chunk.size){
										// Меняем стейт чанка
										this->_chunk.state = process_t::STOP_BODY;
										// Определяем конец буфера
										size_t end = (offset + this->_chunk.size);
										// Собираем тело чанка
										this->_chunk.data.insert(this->_chunk.data.end(), buffer + offset, buffer + end);
										// Выполняем смещение итератора
										i = (end - 1);
										// Увеличиваем смещение
										offset = end;
										// Запоминаем количество обработанных байт
										result = offset;
									// Если количества байт не достаточно для сбора тела
									} else {
										// Меняем стейт чанка
										this->_chunk.state = process_t::BODY;
										// Собираем тело чанка
										this->_chunk.data.insert(this->_chunk.data.end(), buffer + offset, buffer + size);
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
						case static_cast <uint8_t> (process_t::BODY): {
							// Определяем количество необходимых байт
							size_t rem = (this->_chunk.size - this->_chunk.data.size());
							// Если количества байт достаточно для сбора тела чанка
							if(size >= rem){
								// Меняем стейт чанка
								this->_chunk.state = process_t::STOP_BODY;
								// Собираем тело чанка
								this->_chunk.data.insert(this->_chunk.data.end(), buffer, buffer + rem);
								// Выполняем смещение итератора
								i = (rem - 1);
								// Увеличиваем смещение
								offset = rem;
								// Запоминаем количество обработанных байт
								result = offset;
							// Если количества байт не достаточно для сбора тела
							} else {
								// Собираем тело чанка
								this->_chunk.data.insert(this->_chunk.data.end(), buffer, buffer + size);
								// Запоминаем количество обработанных байт
								result = size;
								// Выходим из функции
								return result;
							}
						} break;
						// Если мы ожидаем перевод строки после сбора данных тела чанка
						case static_cast <uint8_t> (process_t::STOP_BODY): {
							// Увеличиваем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили возврат каретки
							if(buffer[i] == '\r')
								// Меняем стейт чанка
								this->_chunk.state = process_t::END_BODY;
							// Если символ отличается, значит ошибка
							else {
								// Устанавливаем символ ошибки
								error = 'r';
								// Выполняем переход к ошибке
								goto Stop;
							}
						} break;
						// Если мы ожидаем получение окончания сбора данных тела чанка
						case static_cast <uint8_t> (process_t::END_BODY): {
							// Увеличиваем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили перевод строки
							if(buffer[i] == '\n'){
								// Если размер получен 0-й значит мы завершили сбор данных
								if(this->_chunk.size == 0)
									// Выполняем переход к ошибке
									goto Stop;
								// Если функция обратного вызова на перехват входящих чанков установлена
								else if(this->_callback.is("binary"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const vector <char> &, const web_t *)> ("binary", this->_id, this->_chunk.data, this);
								// Выполняем очистку чанка
								this->_chunk.clear();
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
				this->_chunk.clear();
				// Определяем тип HTTP-модуля
				switch(static_cast <uint8_t> (this->_hid)){
					// Если мы работаем с клиентом
					case static_cast <uint8_t> (hid_t::CLIENT): {
						// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
						if(this->_callback.is("entityClient"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entityClient", this->_id, this->_response.code, this->_response.message, this->_body);
					} break;
					// Если мы работаем с сервером
					case static_cast <uint8_t> (hid_t::SERVER): {
						// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
						if(this->_callback.is("entityServer"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &)> ("entityServer", this->_id, this->_request.method, this->_request.url, this->_body);
					} break;
				}
				// Тело в запросе не передано
				this->_state = state_t::END;
				// Если мы получили ошибку обработки данных
				if(error != '\0'){
					// Сообщаем, что переданное тело содержит ошибки
					this->_log->print("Body chunk contains errors, [\\%c] is expected", log_t::flag_t::WARNING, error);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Body chunk contains errors, [\\%c] is expected", error));
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * readHeaders Метод извлечения заголовков
 * @param buffer буфер данных для чтения
 * @param size   размер буфера данных для чтения
 * @return       размер обработанных данных
 */
size_t awh::Web::readHeaders(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы
	if((buffer != nullptr) && (size > 0) && (this->_state != state_t::END)){
		// Если мы собираем заголовки или стартовый запрос
		if((this->_state == state_t::HEADERS) || (this->_state == state_t::QUERY)){
			// Определяем статус режима работы
			switch(static_cast <uint8_t> (this->_state)){
				// Если передан режим ожидания получения запроса
				case static_cast <uint8_t> (state_t::QUERY):
					// Устанавливаем разделитель
					this->_separator = ' ';
				break;
				// Если передан режим получения заголовков
				case static_cast <uint8_t> (state_t::HEADERS):
					// Устанавливаем разделитель
					this->_separator = ':';
				break;
			}
			/**
			 * Выполняем парсинг заголовков запроса
			 * @param buffer буфер бинарных данных
			 * @param size   размер бинарных данных
			 * @param bytes  общий размер обработанных данных
			 * @param stop   флаг завершения обработки данных
			 */
			this->prepare(buffer, size, [&result, this](const char * buffer, const size_t size, const size_t bytes, const bool stop) noexcept {
				// Запоминаем количество обработанных байт
				result = bytes;
				// Если все данные получены
				if(stop){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Определяем тип HTTP-модуля
						switch(static_cast <uint8_t> (this->_hid)){
							// Если мы работаем с клиентом
							case static_cast <uint8_t> (hid_t::CLIENT): {
								// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
								if(this->_callback.is("headersResponse"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headersResponse", this->_id, this->_response.code, this->_response.message, this->_headers);
							} break;
							// Если мы работаем с сервером
							case static_cast <uint8_t> (hid_t::SERVER): {
								// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
								if(this->_callback.is("headersRequest"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headersRequest", this->_id, this->_request.method, this->_request.url, this->_headers);
							} break;
						}
						// Получаем размер тела
						auto i = this->_headers.find("content-length");
						// Если размер запроса передан
						if(i != this->_headers.end()){
							// Запоминаем размер тела сообщения
							this->_bodySize = static_cast <size_t> (::stoull(i->second));
							// Если размер тела не получен
							if(this->_bodySize == 0){
								// Запрашиваем заголовок подключения
								const string & header = this->header("connection");
								// Если заголовок подключения найден
								if(header.empty() || !this->_fmk->exists("close", header)){
									// Тело в запросе не передано
									this->_state = state_t::END;
									// Выходим из функции
									return;
								}
							}
							// Устанавливаем стейт поиска тела запроса
							this->_state = state_t::BODY;
							// Продолжаем работу
							goto end;
						// Если тело приходит
						} else {
							// Выполняем извлечение списка параметров передачи данных
							const auto & range = this->_headers.equal_range("transfer-encoding");
							// Выполняем перебор всего списка указанных заголовков
							for(auto i = range.first; i != range.second; ++i){
								// Если нужно получать размер тела чанками
								if(this->_fmk->exists("chunked", i->second)){
									// Устанавливаем стейт поиска тела запроса
									this->_state = state_t::BODY;
									// Продолжаем работу
									goto end;
								}
							}
						}
						// Тело в запросе не передано
						this->_state = state_t::END;
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
				} else if((size > 0) && (this->_pos[0] > -1)) {
					// Определяем статус режима работы
					switch(static_cast <uint8_t> (this->_state)){
						// Если передан режим ожидания получения запроса
						case static_cast <uint8_t> (state_t::QUERY): {
							// Определяем тип HTTP-модуля
							switch(static_cast <uint8_t> (this->_hid)){
								// Если мы работаем с клиентом
								case static_cast <uint8_t> (hid_t::CLIENT): {
									/**
									 * Выполняем отлов ошибок
									 */
									try {
										// Создаём буфер для проверки
										char temp[5];
										// Копируем полученную строку
										::strncpy(temp, buffer, 4);
										// Устанавливаем конец строки
										temp[4] = '\0';
										// Если мы получили ответ от сервера
										if(::strcmp(temp, "HTTP") == 0){
											// Выполняем очистку всех ранее полученных данных
											this->clear();
											// Выполняем сброс размера тела
											this->_bodySize = -1;
											// Устанавливаем разделитель
											this->_separator = ':';
											// Устанавливаем стейт ожидания получения заголовков
											this->_state = state_t::HEADERS;
											// Получаем версию протокол запроса
											this->_response.version = ::stod(string(buffer + 5, this->_pos[0] - 5));
											// Получаем сообщение ответа
											this->_response.message.assign(buffer + (this->_pos[1] + 1), size - (this->_pos[1] + 1));
											// Получаем код ответа
											this->_response.code = static_cast <uint32_t> (::stoi(string(buffer + (this->_pos[0] + 1), this->_pos[1] - (this->_pos[0] + 1))));
											// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
											if(this->_callback.is("response"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint64_t, const uint32_t, const string &)> ("response", this->_id, this->_response.code, this->_response.message);
										// Если данные пришли неправильные
										} else {
											// Выполняем очистку всех ранее полученных данных
											this->clear();
											// Сообщаем, что переданное тело содержит ошибки
											this->_log->print("Broken response server", log_t::flag_t::WARNING);
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken response server");
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
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
										// Создаём буфер для проверки
										char temp[5];
										// Копируем полученную строку
										::strncpy(temp, buffer + (this->_pos[1] + 1), 4);
										// Устанавливаем конец строки
										temp[4] = '\0';
										// Если мы получили ответ от сервера
										if(::strcmp(temp, "HTTP") == 0){
											// Выполняем очистку всех ранее полученных данных
											this->clear();
											// Выполняем сброс размера тела
											this->_bodySize = -1;
											// Устанавливаем разделитель
											this->_separator = ':';
											// Выполняем смену стейта
											this->_state = state_t::HEADERS;
											// Получаем метод запроса
											const string method(buffer, this->_pos[0]);
											// Получаем параметры URI-запроса
											const string uri(buffer + (this->_pos[0] + 1), this->_pos[1] - (this->_pos[0] + 1));
											// Получаем версию протокол запроса
											this->_request.version = ::stod(string(buffer + (this->_pos[1] + 6), size - (this->_pos[1] + 6)));
											// Выполняем установку URI-параметров запроса
											this->_request.url = this->_uri.parse(uri);
											// Если метод определён как GET
											if(this->_fmk->compare(method, "get"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::GET;
											// Если метод определён как PUT
											else if(this->_fmk->compare(method, "put"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::PUT;
											// Если метод определён как POST
											else if(this->_fmk->compare(method, "post"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::POST;
											// Если метод определён как HEAD
											else if(this->_fmk->compare(method, "head"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::HEAD;
											// Если метод определён как DELETE
											else if(this->_fmk->compare(method, "delete"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::DEL;
											// Если метод определён как PATCH
											else if(this->_fmk->compare(method, "patch"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::PATCH;
											// Если метод определён как TRACE
											else if(this->_fmk->compare(method, "trace"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::TRACE;
											// Если метод определён как OPTIONS
											else if(this->_fmk->compare(method, "options"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::OPTIONS;
											// Если метод определён как CONNECT
											else if(this->_fmk->compare(method, "connect"))
												// Выполняем установку метода запроса
												this->_request.method = method_t::CONNECT;
											// Если функция обратного вызова на вывод запроса клиента на выполненный запрос к серверу установлена
											if(this->_callback.is("request"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint64_t, const method_t, const uri_t::url_t &)> ("request", this->_id, this->_request.method, this->_request.url);
										// Если данные пришли неправильные
										} else {
											// Выполняем очистку всех ранее полученных данных
											this->clear();
											// Сообщаем, что переданное тело содержит ошибки
											this->_log->print("Broken request client", log_t::flag_t::WARNING);
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken request client");
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
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
								// Получаем ключ заголовка
								string key(buffer, this->_pos[0]);
								// Получаем значение заголовка
								string val(buffer + (this->_pos[0] + 1), size - (this->_pos[0] + 1));
								// Добавляем заголовок в список заголовков
								if(!key.empty() && !val.empty()){
									// Если название заголовка соответствует HOST
									if(this->_fmk->compare(key, "host")){
										// Создаём объект работы с IP-адресами
										net_t net(this->_log);
										// Выполняем установку порта по умолчанию
										this->_request.url.port = 80;
										// Выполняем установку схемы запроса
										this->_request.url.schema = "http";
										// Выполняем установку хоста
										this->_request.url.host = this->_fmk->transform(val, fmk_t::transform_t::TRIM);
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
										// Определяем тип домена
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
									} else if(this->_fmk->compare(key, "upgrade"))
										// Выполняем установку название протокола для переключению
										this->_upgrade = val;
									// Если название заголовка соответствует трейлеру
									else if(this->_fmk->compare(key, "trailer")) {
										// Выполняем сбор трейлеров
										this->_trailers.emplace(this->_fmk->transform(this->_fmk->transform(val, fmk_t::transform_t::TRIM), fmk_t::transform_t::LOWER));
										// Выводим результат
										return;
									}
									// Добавляем заголовок в список
									this->_headers.emplace(
										this->_fmk->transform(key, fmk_t::transform_t::LOWER),
										this->_fmk->transform(val, fmk_t::transform_t::TRIM)
									);
									// Если функция обратного вызова на вывод полученного заголовка с сервера установлена
									if(this->_callback.is("header"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const uint64_t, const string &, const string &)> ("header", this->_id, key, val);
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
			});
		}
	}
	// Выводим результат
	return result;
}
/**
 * prepare Метод препарирования HTTP заголовков
 * @param buffer   буфер данных для парсинга
 * @param size     размер буфера данных для парсинга
 * @param callback функция обратного вызова
 */
void awh::Web::prepare(const char * buffer, const size_t size, function <void (const char *, const size_t, const size_t, const bool)> callback) noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (size > 0) && (callback != nullptr)){
		// Флаг завершения работы сборки
		bool stop = false;
		// Значение текущей и предыдущей буквы
		char letter = 0, old = 0;
		// Смещение в буфере и длина полученной строки
		size_t offset = 0, length = 0, count = 0;
		// Если позиция ещё не сброшена
		if((this->_pos[0] > -1) && (this->_pos[1] > -1))
			// Выполняем сброс массива сепараторов
			::memset(this->_pos, -1, sizeof(this->_pos));
		// Переходим по всему буферу
		for(size_t i = 0; i < size; i++){
			// Получаем значение текущей буквы
			letter = buffer[i];
			// Если текущий символ перенос строки и это конец, выходим
			if(stop && (letter == '\n')){
				// Выполняем функцию обратного вызова
				callback(nullptr, 0, i + 1, stop);
				// Выходим из цикла
				break;
			}
			// Если предыдущий символ был переносом строки а текущий возврат каретки
			if((old == '\n') && (letter == '\r'))
				// Устанавливаем флаг конца
				stop = true;
			// Если сепаратор найден, добавляем его в массив
			if((this->_separator != '\0') && (letter == this->_separator) && (count < 2)){
				// Устанавливаем позицию найденного разделителя
				this->_pos[count] = (i - offset);
				// Увеличиваем количество найденных разделителей
				count++;
			}
			// Если текущая буква является переносом строк
			// if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
			if((i > 0) && (letter == '\n')){
				// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
				length = ((old == '\r' ? i - 1 : i) - offset);
				/*
				// Если символ является последним и он не является переносом строки
				if((i == (size - 1)) && (letter != '\n'))
					// Увеличиваем общий размер обработанных байт
					length++;
				*/
				// Если данные не получены но мы дошли до конца
				if((length == 0) && (old == '\r') && (letter == '\n')){
					// Устанавливаем флаг конца
					stop = ((this->_state == state_t::HEADERS) || (this->_state == state_t::BODY));
					// Выполняем функцию обратного вызова
					callback(nullptr, 0, i + 1, stop);
				// Если длина слова получена, выводим полученную строку
				} else callback(buffer + offset, length, i + 1, stop);
				// Если массив сепараторов получен
				if(this->_separator != '\0'){
					// Выполняем сброс количество найденных сепараторов
					count = 0;
					// Выполняем сброс массива сепараторов
					::memset(this->_pos, -1, sizeof(this->_pos));
				}
				// Выполняем смещение
				offset = (i + 1);
			}
			// Запоминаем предыдущую букву
			old = letter;
		}
	}
}
/**
 * dump Метод получения бинарного дампа
 * @return бинарный дамп данных
 */
vector <char> awh::Web::dump() const noexcept {
	// Результат работы функции
	vector <char> result;
	{
		// Длина строки, количество элементов
		size_t length = 0, count = 0;
		// Устанавливаем идентификатор HTTP-модуля
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_id), reinterpret_cast <const char *> (&this->_id) + sizeof(this->_id));
		// Устанавливаем тип используемого HTTP-модуля
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_hid), reinterpret_cast <const char *> (&this->_hid) + sizeof(this->_hid));
		// Устанавливаем массив позиций в буфере сепаратора
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_pos), reinterpret_cast <const char *> (&this->_pos) + sizeof(this->_pos));
		// Устанавливаем стейт текущего запроса
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_state), reinterpret_cast <const char *> (&this->_state) + sizeof(this->_state));
		// Устанавливаем размер тела сообщения
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_bodySize), reinterpret_cast <const char *> (&this->_bodySize) + sizeof(this->_bodySize));
		// Устанавливаем сепаратор для детекции в буфере
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_separator), reinterpret_cast <const char *> (&this->_separator) + sizeof(this->_separator));
		// Устанавливаем код ответа на HTTP ответа
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_response.code), reinterpret_cast <const char *> (&this->_response.code) + sizeof(this->_response.code));
		// Устанавливаем версию протокола HTTP ответа
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_response.version), reinterpret_cast <const char *> (&this->_response.version) + sizeof(this->_response.version));
		// Устанавливаем метод HTTP-запроса
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_request.method), reinterpret_cast <const char *> (&this->_request.method) + sizeof(this->_request.method));
		// Устанавливаем версию протокола HTTP-запроса
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_request.version), reinterpret_cast <const char *> (&this->_request.version) + sizeof(this->_request.version));
		// Если URL-адрес запроса установлен
		if(!this->_request.url.empty()){
			// Получаем адрес URL-запроса
			const string & url = this->_uri.url(this->_request.url);
			// Получаем размер записи параметров HTTP-запроса
			length = url.size();
			// Устанавливаем размер записи параметров HTTP-запроса
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем параметры HTTP-запроса
			result.insert(result.end(), url.begin(), url.end());
		// Если URL-адрес запроса не установлен
		} else {
			// Получаем размер записи параметров HTTP-запроса
			length = 0;
			// Устанавливаем размер записи параметров HTTP-запроса
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		}
		// Если текст ответа установлен
		if(!this->_response.message.empty()){
			// Получаем размер сообщения HTTP ответа
			length = this->_response.message.size();
			// Устанавливаем размер сообщения HTTP ответа
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем данные сообщения HTTP ответа
			result.insert(result.end(), this->_response.message.begin(), this->_response.message.end());
		// Если текст ответа не установлен
		} else {
			// Получаем размер записи параметров HTTP-запроса
			length = 0;
			// Устанавливаем размер записи параметров HTTP-запроса
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		}
		// Получаем размер тела сообщения
		length = this->_body.size();
		// Устанавливаем размер тела сообщения
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		// Устанавливаем данные тела сообщения
		result.insert(result.end(), this->_body.begin(), this->_body.end());
		// Получаем количество HTTP заголовков
		count = this->_headers.size();
		// Устанавливаем количество HTTP заголовков
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Выполняем перебор всех HTTP заголовков
		for(auto & header : this->_headers){
			// Получаем размер названия HTTP заголовка
			length = header.first.size();
			// Устанавливаем размер названия HTTP заголовка
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем данные названия HTTP заголовка
			result.insert(result.end(), header.first.begin(), header.first.end());
			// Получаем размер значения HTTP заголовка
			length = header.second.size();
			// Устанавливаем размер значения HTTP заголовка
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем данные значения HTTP заголовка
			result.insert(result.end(), header.second.begin(), header.second.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * dump Метод установки бинарного дампа
 * @param data бинарный дамп данных
 */
void awh::Web::dump(const vector <char> & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty()){
		// Длина строки, количество элементов и смещение в буфере
		size_t length = 0, count = 0, offset = 0;
		// Выполняем получение идентификатора HTTP-модуля
		::memcpy(reinterpret_cast <void *> (&this->_id), data.data() + offset, sizeof(this->_id));
		// Выполняем смещение в буфере
		offset += sizeof(this->_id);
		// Выполняем получение типа используемого HTTP-модуля
		::memcpy(reinterpret_cast <void *> (&this->_hid), data.data() + offset, sizeof(this->_hid));
		// Выполняем смещение в буфере
		offset += sizeof(this->_hid);
		// Выполняем получение массива позиций в буфере сепаратора
		::memcpy(reinterpret_cast <void *> (&this->_pos), data.data() + offset, sizeof(this->_pos));
		// Выполняем смещение в буфере
		offset += sizeof(this->_pos);
		// Выполняем получение стейта текущего запроса
		::memcpy(reinterpret_cast <void *> (&this->_state), data.data() + offset, sizeof(this->_state));
		// Выполняем смещение в буфере
		offset += sizeof(this->_state);
		// Выполняем получение размера тела сообщения
		::memcpy(reinterpret_cast <void *> (&this->_bodySize), data.data() + offset, sizeof(this->_bodySize));
		// Выполняем смещение в буфере
		offset += sizeof(this->_bodySize);
		// Выполняем получение сепаратора для детекции в буфере
		::memcpy(reinterpret_cast <void *> (&this->_separator), data.data() + offset, sizeof(this->_separator));
		// Выполняем смещение в буфере
		offset += sizeof(this->_separator);
		// Выполняем получение кода ответа на HTTP-запрос
		::memcpy(reinterpret_cast <void *> (&this->_response.code), data.data() + offset, sizeof(this->_response.code));
		// Выполняем смещение в буфере
		offset += sizeof(this->_response.code);
		// Выполняем получение версии протокола HTTP ответа
		::memcpy(reinterpret_cast <void *> (&this->_response.version), data.data() + offset, sizeof(this->_response.version));
		// Выполняем смещение в буфере
		offset += sizeof(this->_response.version);
		// Выполняем получение метода HTTP-запроса
		::memcpy(reinterpret_cast <void *> (&this->_request.method), data.data() + offset, sizeof(this->_request.method));
		// Выполняем смещение в буфере
		offset += sizeof(this->_request.method);
		// Выполняем получение версии протокола HTTP-запроса
		::memcpy(reinterpret_cast <void *> (&this->_request.version), data.data() + offset, sizeof(this->_request.version));
		// Выполняем смещение в буфере
		offset += sizeof(this->_request.version);
		// Выполняем получение размера записи параметров HTTP-запроса
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если URL-адрес запроса установлен
		if(length > 0){
			// Создаём URL-адрес запроса
			string url(length, 0);
			// Выполняем получение параметров HTTP-запроса
			::memcpy(reinterpret_cast <void *> (url.data()), data.data() + offset, length);
			// Устанавливаем URL-адрес запроса
			this->_request.url = this->_uri.parse(url);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение размера сообщения HTTP ответа
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если сообщение ответа установлено
		if(length > 0){
			// Выделяем память для сообщения HTTP ответа
			this->_response.message.resize(length, 0);
			// Выполняем получение сообщения HTTP ответа
			::memcpy(reinterpret_cast <void *> (this->_response.message.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение размера тела сообщения
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если сообщение ответа установлено
		if(length > 0){
			// Выделяем память для данных тела сообщения
			this->_body.resize(length, 0);
			// Выполняем получение данных тела сообщения
			::memcpy(reinterpret_cast <void *> (this->_body.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение количества HTTP заголовков
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс заголовков
		this->_headers.clear();
		// Если количество заголовков больше чем ничего
		if(count > 0){
			// Выполняем последовательную загрузку всех заголовков
			for(size_t i = 0; i < count; i++){
				// Выполняем получение размера названия HTTP заголовка
				::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
				// Выполняем смещение в буфере
				offset += sizeof(length);
				// Если размер получен
				if(length > 0){
					// Выпделяем память для ключа заголовка
					string key(length, 0);
					// Выполняем получение ключа заголовка
					::memcpy(reinterpret_cast <void *> (key.data()), data.data() + offset, length);
					// Выполняем смещение в буфере
					offset += length;
					// Выполняем получение размера значения HTTP заголовка
					::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выпделяем память для значения заголовка
						string value(length, 0);
						// Выполняем получение значения заголовка
						::memcpy(reinterpret_cast <void *> (value.data()), data.data() + offset, length);
						// Выполняем смещение в буфере
						offset += length;
						// Если и ключ и значение заголовка получены
						if(!key.empty() && !value.empty())
							// Добавляем заголовок в список заголовков
							this->_headers.emplace(::move(key), ::move(value));
					}
				}
			}
		}
	}
}
/**
 * parse Метод выполнения парсинга HTTP буфера данных
 * @param buffer буфер данных для парсинга
 * @param size   размер буфера данных для парсинга
 * @return       размер обработанных данных
 */
size_t awh::Web::parse(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы или обработка полностью выполнена
	if((buffer != nullptr) && (size > 0) && (this->_state != state_t::END)){
		// Определяем текущий стейт
		switch(static_cast <uint8_t> (this->_state)){
			// Если установлен стейт чтения параметров запроса/ответа
			case static_cast <uint8_t> (state_t::QUERY):
			// Если установлен стейт чтения заголовков
			case static_cast <uint8_t> (state_t::HEADERS): {
				// Выполняем чтение заголовков
				result = this->readHeaders(buffer, size);
				// Если требуется продолжить извлечение данных тела сообщения
				if((result < size) && (this->_state == state_t::BODY))
					// Выполняем извлечение данных тела сообщения
					result += this->readPayload(buffer + result, size - result);
			} break;
			// Если установлен стейт чтения полезной нагрузки
			case static_cast <uint8_t> (state_t::BODY):
				// Выполняем извлечение данных тела сообщения
				result = this->readPayload(buffer, size);
			break;
		}
	}
	// Выводим реузльтат
	return result;
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Web::clear() noexcept {
	// Выполняем сброс параметров запроса
	this->_request = req_t();
	// Выполняем сброс параметров ответа
	this->_response = res_t();
	// Выполняем очистку тела HTTP-запроса
	this->_body.clear();
	// Выполняем сброс параметров чанка
	this->_chunk.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->_headers.clear();
	// Выполняем сброс списка трейлеров
	this->_trailers.clear();
	// Выполняем удаление памяти тела
	vector <char> ().swap(this->_body);
	// Выполняем удаление памяти списка трейлеров
	unordered_set <string> ().swap(this->_trailers);
	// Выполняем удаление памяти полученных HTTP заголовков
	unordered_multimap <string, string> ().swap(this->_headers);
}
/**
 * reset Метод сброса стейтов парсера
 */
void awh::Web::reset() noexcept {
	// Выполняем сброс размера тела
	this->_bodySize = -1;
	// Устанавливаем разделитель
	this->_separator = '\0';
	// Выполняем сброс стейта текущего запроса
	this->_state = state_t::QUERY;
	// Выполняем сброс массива сепараторов
	::memset(this->_pos, -1, sizeof(this->_pos));
}
/**
 * request Метод получения объекта запроса на сервер
 * @return объект запроса на сервер
 */
const awh::Web::req_t & awh::Web::request() const noexcept {
	// Выводим объект запроса на сервер
	return this->_request;
}
/**
 * request Метод установки объекта запроса на сервер
 * @param request объект запроса на сервер
 */
void awh::Web::request(req_t && request) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_request = ::move(request);
}
/**
 * request Метод установки объекта запроса на сервер
 * @param request объект запроса на сервер
 */
void awh::Web::request(const req_t & request) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_request = request;
}
/**
 * response Метод получения объекта ответа сервера
 * @return объект ответа сервера
 */
const awh::Web::res_t & awh::Web::response() const noexcept {
	// Выводим объект ответа сервера
	return this->_response;
}
/**
 * response Метод установки объекта ответа сервера
 * @param response объект ответа сервера
 */
void awh::Web::response(res_t && response) noexcept {
	// Устанавливаем объект ответа сервера
	this->_response = ::move(response);
}
/**
 * response Метод установки объекта ответа сервера
 * @param response объект ответа сервера
 */
void awh::Web::response(const res_t & response) noexcept {
	// Устанавливаем объект ответа сервера
	this->_response = response;
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Web::isEnd() const noexcept {
	// Выводрим результат проверки
	return (this->_state == state_t::END);
}
/**
 * isHeader Метод проверки существования заголовка
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Web::isHeader(const string & key) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если ключ передан
	if(!key.empty()){
		// Выполняем перебор всех заголовков
		for(auto & header : this->_headers){
			// Выполняем проверку существования заголовка
			result = this->_fmk->compare(header.first, key);
			// Выходим из цилка если заголовок найден
			if(result) break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * isStandard Проверка заголовка является ли он стандартным
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Web::isStandard(const string & key) const noexcept {
	// Если ключ передан
	if(!key.empty())
		// Выполняем проверку заголовка
		return (this->_standardHeaders.count(this->_fmk->transform(key, fmk_t::transform_t::LOWER)) > 0);
	// Выводим результат
	return false;
}
/**
 * clearBody Метод очистки данных тела
 */
void awh::Web::clearBody() noexcept {
	// Выполняем очистку данных тела
	this->_body.clear();
}
/**
 * clearHeaders Метод очистки списка заголовков
 */
void awh::Web::clearHeaders() noexcept {
	// Выполняем очистку заголовков
	this->_headers.clear();
}
/**
 * body Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Web::body() const noexcept {
	// Выводим данные тела
	return this->_body;
}
/**
 * body Метод добавления данных тела
 * @param body буфер тела для добавления
 */
void awh::Web::body(const vector <char> & body) noexcept {
	// Если тело данных передано
	if(!body.empty())
		// Выполняем установку данных тела
		this->_body.insert(this->_body.end(), body.begin(), body.end());
}
/**
 * body Метод добавления данных тела
 * @param buffer буфер тела для добавления
 * @param size   размер буфера теля для добавления
 */
void awh::Web::body(const char * buffer, const size_t size) noexcept {
	// Если тело данных передано
	if((buffer != nullptr) && (size > 0))
		// Выполняем установку данных тела
		this->_body.insert(this->_body.end(), buffer, buffer + size);
}
/**
 * upgrade Метод получение названия протокола для переключения
 * @return название протокола для переключения
 */
const string & awh::Web::upgrade() const noexcept {
	// Выполняем вывод название протокола для переключения
	return this->_upgrade;
}
/**
 * upgrade Метод установки название протокола для переключения
 * @param upgrade название протокола для переключения
 */
void awh::Web::upgrade(const string & upgrade) noexcept {
	// Выполняем установку названия протокола для переключения
	this->_upgrade = upgrade;
}
/**
 * proto Метод извлечения список протоколов к которому принадлежит заголовок
 * @param key ключ заголовка
 * @return    список протоколов
 */
set <awh::Web::proto_t> awh::Web::proto(const string & key) const noexcept {
	// Если ключ передан
	if(!key.empty()){
		// Выполняем поиск заголовка
		auto i = this->_standardHeaders.find(this->_fmk->transform(key, fmk_t::transform_t::LOWER));
		// Если заголовок найден выводим результат
		if(i != this->_standardHeaders.end())
			// Выводим результат
			return i->second;
	}
	// Выводим результат
	return set <awh::Web::proto_t> ();
}
/**
 * delHeader Метод удаления заголовка
 * @param key ключ заголовка
 */
void awh::Web::delHeader(const string & key) noexcept {
	// Если ключ заголовка передан
	if(!key.empty()){
		// Выполняем перебор всех заголовков
		for(auto i = this->_headers.begin(); i != this->_headers.end();){
			// Выполняем проверку существования заголовка
			if(this->_fmk->compare(i->first, key))
				// Выполняем удаление указанного заголовка
				i = this->_headers.erase(i);
			// Иначе ищем заголовок дальше
			else i++;
		}
	}
}
/**
 * header Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
string awh::Web::header(const string & key) const noexcept {
	// Если ключ заголовка передан
	if(!key.empty()){
		// Выполняем перебор всех заголовков
		for(auto & header : this->_headers){
			// Выполняем проверку существования заголовка
			if(this->_fmk->compare(header.first, key))
				// Выводим найденный заголовок
				return header.second;
		}
	}
	// Выводим результат
	return "";
}
/**
 * header Метод добавления заголовка
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::Web::header(const string & key, const string & val) noexcept {
	// Если даныне заголовка переданы
	if(!key.empty() && !val.empty())
		// Выполняем добавление передаваемого заголовка
		this->_headers.emplace(key, val);
}
/**
 * headers Метод получения списка заголовков
 * @return список существующих заголовков
 */
const unordered_multimap <string, string> & awh::Web::headers() const noexcept {
	// Выводим список доступных заголовков
	return this->_headers;
}
/**
 * headers Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::Web::headers(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков
	this->_headers = headers;
}
/**
 * id Метод получения идентификатора объекта
 * @return идентификатор объекта
 */
uint64_t awh::Web::id() const noexcept {
	// Выводим идентификатор объекта
	return this->_id;
}
/**
 * id Метод установки идентификатора объекта
 * @param id идентификатор объекта
 */
void awh::Web::id(const uint64_t id) noexcept {
	// Выполняем установку идентификатора объекта
	this->_id = id;
}
/**
 * hid Метод вывода идентификатора модуля
 * @return тип используемого HTTP-модуля
 */
const awh::Web::hid_t awh::Web::hid() const noexcept {
	// Выводим тип используемого HTTP-модуля
	return this->_hid;
}
/**
 * hid Метод установки идентификатора модуля
 * @param hid тип используемого HTTP-модуля
 */
void awh::Web::hid(const hid_t hid) noexcept {
	// Устанавливаем тип используемого HTTP-модуля
	this->_hid = hid;
}
/** 
 * state Метод установки стейта ожидания данных
 * @param state стейт ожидания данных для установки
 */
void awh::Web::state(const state_t state) noexcept {
	// Выполняем установку стейта
	this->_state = state;
}
/**
 * callback Метод установки функций обратного вызова
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
 * Web Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Web::Web(const fmk_t * fmk, const log_t * log) noexcept :
 _separator('\0'), _pos{-1, -1}, _bodySize(-1), _uri(fmk, log), _callback(log),
 _hid(hid_t::NONE), _state(state_t::QUERY), _upgrade{""}, _fmk(fmk), _log(log) {
	// Выполняем заполнение списка стандартных заголовков
	this->_standardHeaders.insert({
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
