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
 * Максимальная длина строки размера чанка вместе с расширениями
 */
static constexpr size_t CHUNK_SIZE_LINE_MAX = 0x1000;

/**
 * @brief Функция строгого разбора размера чанка
 *
 * Допускаются только шестнадцатеричные цифры (не более 16-ти), затем необязательные
 * пробелы и расширения чанка после ';'. Любой другой символ является ошибкой: разбор
 * через atoi принимал мусор как ноль и позволял переполнить проверку размера тела
 *
 * @param buffer буфер строки размера чанка
 * @param result полученный размер чанка
 * @return       результат разбора
 */
static bool chunkSize(const vector <char> & buffer, uint64_t & result) noexcept {
	// Количество полученных цифр
	size_t digits = 0, i = 0;
	// Выполняем сброс результата
	result = 0;
	// Выполняем перебор всех цифр
	for(; i < buffer.size(); i++){
		// Получаем текущий символ
		const char c = buffer[i];
		// Если символ является цифрой
		if((c >= '0') && (c <= '9'))
			// Добавляем цифру
			result = ((result << 4) | static_cast <uint64_t> (c - '0'));
		// Если символ является буквой в нижнем регистре
		else if((c >= 'a') && (c <= 'f'))
			// Добавляем цифру
			result = ((result << 4) | static_cast <uint64_t> (c - 'a' + 10));
		// Если символ является буквой в верхнем регистре
		else if((c >= 'A') && (c <= 'F'))
			// Добавляем цифру
			result = ((result << 4) | static_cast <uint64_t> (c - 'A' + 10));
		// Если цифры закончились, выходим
		else break;
		// Если цифр слишком много
		if(++digits > 16)
			// Выводим результат
			return false;
	}
	// Если цифры не получены
	if(digits == 0)
		// Выводим результат
		return false;
	// Пропускаем необязательные пробелы
	while((i < buffer.size()) && ((buffer[i] == ' ') || (buffer[i] == '\t')))
		// Переходим к следующему символу
		i++;
	// Строка завершена или далее следуют расширения чанка
	return ((i == buffer.size()) || (buffer[i] == ';'));
}
/**
 * @brief Функция строгого разбора размера тела Content-Length
 *
 * @param value  значение заголовка
 * @param result полученный размер тела
 * @return       результат разбора
 */
static bool contentLength(const string & value, uint64_t & result) noexcept {
	// Выполняем сброс результата
	result = 0;
	// Если значение пустое или слишком длинное
	if(value.empty() || (value.size() > 18))
		// Выводим результат
		return false;
	// Выполняем перебор всех символов
	for(auto & c : value){
		// Если символ не является цифрой
		if((c < '0') || (c > '9'))
			// Выводим результат
			return false;
		// Добавляем цифру
		result = ((result * 10) + static_cast <uint64_t> (c - '0'));
	}
	// Выводим результат
	return true;
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
	this->state = process_t::SIZE;
}
/**
 * @brief Конструктор
 *
 */
awh::Web::Chunk::Chunk() noexcept : size(0), state(process_t::SIZE) {}
/**
 * @brief Деструктор
 *
 */
awh::Web::Chunk::~Chunk() noexcept {}
/**
 * @brief Метод извлечения полезной нагрузки
 *
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
					// Заполняем собранные данные в промежуточный буфер
					this->_chunk.buffer.assign(buffer, buffer + result);
					// Если функция обратного вызова на перехват входящих чанков установлена
					if(this->_callback.is("binary"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const vector <char> &, const web_t *)> ("binary", this->_id, this->_chunk.buffer, this);
				// Если размер установлен конкретный
				} else {
					// Получаем актуальный размер тела
					result = (this->_bodySize - this->_body.size());
					// Фиксируем актуальный размер тела
					result = (size > result ? result : size);
					// Увеличиваем общий размер полученных данных
					this->_chunk.size += result;
					// Заполняем собранные данные в промежуточный буфер
					this->_chunk.buffer.assign(buffer, buffer + result);
					// Если функция обратного вызова на перехват входящих чанков установлена
					if(this->_callback.is("binary"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const vector <char> &, const web_t *)> ("binary", this->_id, this->_chunk.buffer, this);
					// Если тело сообщения полностью собранно
					if(this->_bodySize == this->_chunk.size){
						// Очищаем собранные данные
						this->_chunk.clear();
						/**
						 * Определяем тип HTTP-модуля
						 */
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
					/**
					 * Определяем стейт чанка
					 */
					switch(static_cast <uint8_t> (this->_chunk.state)){
						// Если мы собираем трейделы переданные сервером
						case static_cast <uint8_t> (process_t::TRAILERS): {
							// Устанавливаем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы работаем с сервером
							if(this->_hid == hid_t::SERVER){
								// Трейлеры учитываются в ограничении размера секции заголовков
								this->_headerBytes++;
								// Если секция трейлеров превышает допустимый размер
								if(this->_headerBytes > AWH_MAX_HEADERS_SIZE){
									// Выводим сообщение об ошибке
									this->_log->print("Request trailer fields are too large", log_t::flag_t::WARNING);
									// Запрос отклоняется целиком
									this->_fault = 431;
									// Выполняем переход к ошибке
									goto Stop;
								}
							// Если строка трейлера ответа слишком длинная
							} else if(this->_chunk.buffer.size() >= AWH_MAX_HEADERS_SIZE) {
								// Выводим сообщение об ошибке
								this->_log->print("Response trailer fields are too large", log_t::flag_t::WARNING);
								// Выполняем переход к ошибке
								goto Stop;
							}
							// Если мы получили перевод строки, строка трейлера уже обработана по возврату каретки
							if(buffer[i] == '\n')
								// Продолжаем обработку
								break;
							// Если мы получили возврат каретки
							else if(buffer[i] == '\r') {
								// Если строка пустая, секция трейлеров завершена
								if(this->_chunk.buffer.empty()){
									// Ожидаем завершающий перевод строки
									this->_chunk.state = process_t::END_BODY;
									// Продолжаем обработку
									break;
								}
								// Получаем заголовок переданного трейлера
								const string header(this->_chunk.buffer.begin(), this->_chunk.buffer.end());
								// Выполняем сброс тела данных
								this->_chunk.buffer.clear();
								// Выполняем поиск разделителя заголовка
								const size_t pos = header.find(':');
								// Получаем ключ заголовка
								string key = ((pos != string::npos) ? header.substr(0, pos) : "");
								/**
								 * Строка трейлера без двоеточия или с пустым либо содержащим пробелы именем
								 * является ошибкой разбора (RFC 9112 §5.1)
								 */
								if(key.empty() || (key.find_first_of(" \t") != string::npos)){
									// Выводим сообщение об ошибке
									this->_log->print("Broken trailer field", log_t::flag_t::WARNING);
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken trailer field");
									// Если мы работаем с сервером
									if(this->_hid == hid_t::SERVER)
										// Запрос отклоняется целиком
										this->_fault = 400;
									// Выполняем переход к ошибке
									goto Stop;
								}
								// Если мы работаем с сервером и трейлеров слишком много
								if((this->_hid == hid_t::SERVER) && (++this->_headerCount > AWH_MAX_HEADERS_COUNT)){
									// Выводим сообщение об ошибке
									this->_log->print("Request trailer fields are too many", log_t::flag_t::WARNING);
									// Запрос отклоняется целиком
									this->_fault = 431;
									// Выполняем переход к ошибке
									goto Stop;
								}
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
								// Если трейлер был объявлен, снимаем его из списка ожидаемых
								this->_trailers.erase(key);
							// Выполняем сборку строки трейлера
							} else this->_chunk.buffer.push_back(buffer[i]);
						} break;
						// Если мы ожидаем получения размера тела чанка
						case static_cast <uint8_t> (process_t::SIZE): {
							// Если мы получили возврат каретки
							if(buffer[i] == '\r'){
								// Размер чанка
								uint64_t length = 0;
								// Меняем стейт чанка
								this->_chunk.state = process_t::END_SIZE;
								// Устанавливаем смещение
								offset = (i + 1);
								// Запоминаем количество обработанных байт
								result = offset;
								// Если размер чанка передан неверно
								if(!::chunkSize(this->_chunk.buffer, length)){
									// Выводим сообщение об ошибке
									this->_log->print("Body chunk size is invalid", log_t::flag_t::WARNING);
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Body chunk size is invalid");
									// Если мы работаем с сервером
									if(this->_hid == hid_t::SERVER)
										// Запрос отклоняется целиком
										this->_fault = 400;
									// Выполняем переход к ошибке
									goto Stop;
								}
								// Выполняем сброс тела данных
								this->_chunk.buffer.clear();
								/**
								 * Размер проверяется до сложения с телом: сложение с размером
								 * близким к максимуму 64 бит переполнялось и снимало ограничение
								 */
								if((length > AWH_MAX_BODY_SIZE) || (this->_body.size() > AWH_MAX_BODY_SIZE) || (length > (AWH_MAX_BODY_SIZE - this->_body.size()))){
									// Если мы работаем с сервером
									if(this->_hid == hid_t::SERVER)
										// Тело запроса слишком большое
										this->_fault = 413;
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug(
											"HTTP-body is %s and is too large, the HTTP-body cannot exceed %s",
											__PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL,
											this->_fmk->bytes(static_cast <double> (length) + static_cast <double> (this->_body.size())).c_str(),
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
											this->_fmk->bytes(static_cast <double> (length) + static_cast <double> (this->_body.size())).c_str(),
											this->_fmk->bytes(static_cast <double> (AWH_MAX_BODY_SIZE)).c_str()
										);
									#endif
									// Выполняем переход к ошибке
									goto Stop;
								}
								// Запоминаем размер чанка
								this->_chunk.size = static_cast <size_t> (length);
							// Выполняем сборку 16-го размера чанка
							} else {
								// Запоминаем количество обработанных байт
								result = (i + 1);
								// Если строка размера чанка слишком длинная
								if(this->_chunk.buffer.size() >= CHUNK_SIZE_LINE_MAX){
									// Выводим сообщение об ошибке
									this->_log->print("Body chunk size line is too long", log_t::flag_t::WARNING);
									// Если мы работаем с сервером
									if(this->_hid == hid_t::SERVER)
										// Запрос отклоняется целиком
										this->_fault = 400;
									// Выполняем переход к ошибке
									goto Stop;
								}
								// Выполняем сборку размера чанка
								this->_chunk.buffer.push_back(buffer[i]);
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
									// Выполняем сброс тела данных
									this->_chunk.buffer.clear();
									/**
									 * После последнего чанка всегда разбирается секция трейлеров до пустой строки
									 * (RFC 9112 §7.1.2): поля трейлеров допустимы и без объявления в заголовке Trailer
									 */
									this->_chunk.state = process_t::TRAILERS;
								// Если данные собраны не полностью
								} else {
									// Если количества байт достаточно для сбора тела чанка
									if((size - offset) >= this->_chunk.size){
										// Меняем стейт чанка
										this->_chunk.state = process_t::STOP_BODY;
										// Определяем конец буфера
										size_t end = (offset + this->_chunk.size);
										// Собираем тело чанка
										this->_chunk.buffer.insert(this->_chunk.buffer.end(), buffer + offset, buffer + end);
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
										this->_chunk.buffer.insert(this->_chunk.buffer.end(), buffer + offset, buffer + size);
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
							size_t rem = (this->_chunk.size - this->_chunk.buffer.size());
							// Если количества байт достаточно для сбора тела чанка
							if(size >= rem){
								// Меняем стейт чанка
								this->_chunk.state = process_t::STOP_BODY;
								// Собираем тело чанка
								this->_chunk.buffer.insert(this->_chunk.buffer.end(), buffer, buffer + rem);
								// Выполняем смещение итератора
								i = (rem - 1);
								// Увеличиваем смещение
								offset = rem;
								// Запоминаем количество обработанных байт
								result = offset;
							// Если количества байт не достаточно для сбора тела
							} else {
								// Собираем тело чанка
								this->_chunk.buffer.insert(this->_chunk.buffer.end(), buffer, buffer + size);
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
									this->_callback.call <void (const uint64_t, const vector <char> &, const web_t *)> ("binary", this->_id, this->_chunk.buffer, this);
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
				// Если мы работаем с сервером и в чанках допущена ошибка
				if((error != '\0') && (this->_hid == hid_t::SERVER))
					// Запрос отклоняется целиком
					this->_fault = 400;
				/**
				 * Определяем тип HTTP-модуля
				 */
				switch((this->_fault == 0) ? static_cast <uint8_t> (this->_hid) : static_cast <uint8_t> (hid_t::NONE)){
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
 * @brief Метод извлечения заголовков
 *
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
			/**
			 * Определяем статус режима работы
			 */
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
				// Если запрос уже отклонён, дальнейшие строки не обрабатываем
				if(this->_fault > 0)
					// Выходим из функции
					return;
				// Если мы работаем с сервером
				if(this->_hid == hid_t::SERVER){
					// Если получена очередная строка заголовка
					if(!stop && (size > 0) && (this->_state == state_t::HEADERS))
						// Увеличиваем количество полученных заголовков
						this->_headerCount++;
					/**
					 * Размер секции заголовков и число заголовков ограничены,
					 * иначе клиент заставляет сервер копить заголовки без предела
					 */
					if(((this->_headerBytes + bytes) > AWH_MAX_HEADERS_SIZE) || (this->_headerCount > AWH_MAX_HEADERS_COUNT)){
						// Выводим сообщение об ошибке
						this->_log->print("Request header fields are too large", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Request header fields are too large");
						// Запрос отклоняется целиком
						this->_fault = 431;
						// Прекращаем обработку запроса
						this->_state = state_t::END;
						// Выходим из функции
						return;
					}
				}
				// Если все данные получены
				if(stop){
					/**
					 * Пустые строки перед стартовой строкой запроса пропускаются (RFC 9112 §2.2),
					 * иначе сервер передавал приложению пустой запрос без метода
					 */
					if((this->_hid == hid_t::SERVER) && (this->_state == state_t::QUERY))
						// Выходим из функции
						return;
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
									this->_callback.call <void (const uint64_t, const uint32_t, const string &, const std::unordered_multimap <string, string> &)> ("headersResponse", this->_id, this->_response.code, this->_response.message, this->_headers);
							} break;
							// Если мы работаем с сервером
							case static_cast <uint8_t> (hid_t::SERVER): {
								// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
								if(this->_callback.is("headersRequest"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const method_t, const uri_t::url_t &, const std::unordered_multimap <string, string> &)> ("headersRequest", this->_id, this->_request.method, this->_request.url, this->_headers);
							} break;
						}
						/**
						 * @brief Функция отклонения запроса с ошибкой разбора
						 *
						 * @param code    код ответа сервера
						 * @param message сообщение об ошибке
						 */
						auto faultFn = [this](const uint32_t code, const char * message) noexcept -> void {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, message);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, message);
							// Если мы работаем с сервером
							if(this->_hid == hid_t::SERVER)
								// Запрос отклоняется целиком
								this->_fault = code;
							// Тело в запросе не передано
							this->_state = state_t::END;
						};
						// Если секция заголовков слишком большая или заголовков слишком много
						if(this->_fault > 0){
							// Тело в запросе не передано
							this->_state = state_t::END;
							// Выходим из функции
							return;
						}
						// Флаг передачи тела чанками
						bool chunked = false;
						// Выполняем извлечение списка параметров передачи данных
						const auto & encodings = this->_headers.equal_range("transfer-encoding");
						// Выполняем извлечение списка размеров тела
						const auto & lengths = this->_headers.equal_range("content-length");
						// Если способ передачи данных указан
						if(encodings.first != encodings.second){
							// Последний способ передачи данных
							string last = "";
							// Выполняем перебор всего списка указанных заголовков
							for(auto i = encodings.first; i != encodings.second; ++i){
								// Список способов передачи данных
								vector <string> tokens;
								// Выполняем разделение способов передачи данных
								this->_fmk->split(i->second, ",", tokens);
								// Выполняем перебор всех способов передачи данных
								for(auto & token : tokens){
									// Если способ передачи данных указан
									if(!token.empty())
										// Запоминаем последний способ передачи данных
										last = token;
								}
							}
							/**
							 * Чанки признаются только если chunked является последним способом передачи
							 * (RFC 9112 §6.3), поиск подстроки принимал "xchunked" и "chunked, gzip"
							 */
							chunked = this->_fmk->compare(last, "chunked");
						}
						// Если мы работаем с сервером и способ передачи данных указан
						if((this->_hid == hid_t::SERVER) && (encodings.first != encodings.second)){
							/**
							 * Запрос одновременно с Content-Length и Transfer-Encoding отклоняется
							 * (RFC 9112 §6.1), иначе фронт и сервер по-разному определяют границу тела
							 */
							if(lengths.first != lengths.second)
								// Выполняем отклонение запроса
								faultFn(400, "Request contains both Content-Length and Transfer-Encoding");
							// Если последним способом передачи являются не чанки
							else if(!chunked)
								// Выполняем отклонение запроса
								faultFn(400, "Request Transfer-Encoding must end with chunked");
							// Если тело передаётся чанками
							else this->_state = state_t::BODY;
							// Выходим из функции
							return;
						}
						// Если клиент получил тело чанками
						if(chunked){
							// Устанавливаем стейт поиска тела запроса
							this->_state = state_t::BODY;
							// Выходим из функции
							return;
						}
						// Если размер запроса передан
						if(lengths.first != lengths.second){
							// Размер тела сообщения
							uint64_t length = 0;
							// Флаг корректности размера тела
							bool valid = true, first = true;
							// Выполняем перебор всех переданных размеров
							for(auto i = lengths.first; (i != lengths.second) && valid; ++i){
								// Список значений размера
								vector <string> values;
								// Выполняем разделение значений размера
								this->_fmk->split(i->second, ",", values);
								// Если значения не получены
								if(values.empty())
									// Размер передан неверно
									valid = false;
								// Выполняем перебор всех значений размера
								for(auto & value : values){
									// Текущее значение размера
									uint64_t current = 0;
									/**
									 * Размер должен состоять только из цифр, а повторы обязаны совпадать:
									 * иначе "+5", " 5", "5abc", "-1" и разные дубли давали разное понимание тела
									 */
									if(!::contentLength(value, current) || (!first && (current != length))){
										// Размер передан неверно
										valid = false;
										// Выходим из цикла
										break;
									}
									// Запоминаем размер тела
									length = current;
									// Снимаем флаг первого значения
									first = false;
								}
							}
							// Если размер передан неверно
							if(!valid){
								// Выполняем отклонение запроса
								faultFn(400, "Content-Length is invalid");
								// Выходим из функции
								return;
							}
							// Если размер тела не получен
							if(length == 0){
								// Запоминаем размер тела сообщения
								this->_bodySize = 0;
								// Запрашиваем заголовок подключения
								const string & header = this->header("connection");
								/**
								 * Нулевой размер с закрытием подключения читается до закрытия только клиентом:
								 * так вещают интернет-радиостанции (наследие HTTP/1.0). Сервер такие тела не читает,
								 * иначе один клиент удерживает обработку бесконечно, поэтому тело запроса пустое
								 */
								if((this->_hid != hid_t::CLIENT) || header.empty() || !this->_fmk->exists("close", header)){
									// Тело в запросе не передано
									this->_state = state_t::END;
									// Выходим из функции
									return;
								}
							// Если размер тела слишком большой
							} else if(length > AWH_MAX_BODY_SIZE) {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(
										"HTTP-body is %s and is too large, the HTTP-body cannot exceed %s",
										__PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL,
										this->_fmk->bytes(static_cast <double> (length)).c_str(),
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
										this->_fmk->bytes(static_cast <double> (length)).c_str(),
										this->_fmk->bytes(static_cast <double> (AWH_MAX_BODY_SIZE)).c_str()
									);
								#endif
								// Если мы работаем с сервером
								if(this->_hid == hid_t::SERVER)
									// Тело запроса слишком большое
									this->_fault = 413;
								// Тело в запросе не передано
								this->_state = state_t::END;
								// Выходим из функции
								return;
							// Запоминаем размер тела сообщения
							} else this->_bodySize = static_cast <int64_t> (length);
							// Устанавливаем стейт поиска тела запроса
							this->_state = state_t::BODY;
							// Выходим из функции
							return;
						}
						// Тело в запросе не передано
						this->_state = state_t::END;
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						// Если мы работаем с сервером
						if(this->_hid == hid_t::SERVER)
							// Запрос отклоняется целиком
							this->_fault = 400;
						// Тело в запросе не передано
						this->_state = state_t::END;
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
					// Выходим из функции
					return;
				/**
				 * Строка запроса без разделителя (стартовая строка без пробела, заголовок без двоеточия)
				 * или с одиночным возвратом каретки либо нулевым байтом на сервере является ошибкой разбора
				 * (RFC 9112 §2.2, §3, §5, RFC 9110 §5.5):
				 * раньше такая строка молча пропускалась, и сервер ждал запроса до истечения времени ожидания
				 */
				} else if((size > 0) && (this->_hid == hid_t::SERVER) && ((this->_pos[0] < 0) || (::memchr(buffer, '\r', size) != nullptr) || (::memchr(buffer, '\0', size) != nullptr))) {
					// Получаем текст сообщения об ошибке
					const char * message = ((this->_state == state_t::QUERY) ? "Broken request client" : "Broken request header");
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, message);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, message);
					// Запрос отклоняется целиком
					this->_fault = 400;
					// Прекращаем обработку запроса
					this->_state = state_t::END;
				// Если необходимо  получить оставшиеся данные
				} else if((size > 0) && (this->_pos[0] > -1)) {
					/**
					 * Определяем статус режима работы
					 */
					switch(static_cast <uint8_t> (this->_state)){
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
										/**
										 * Стартовая строка обязана иметь вид «метод SP цель SP HTTP/цифра.цифра» (RFC 9112 §3):
										 * метод является токеном, цель непуста и без управляющих символов. Прежняя проверка
										 * читала четыре байта за концом короткой строки и пропускала мусор вместо метода
										 */
										bool valid = ((this->_pos[0] > 0) && (this->_pos[1] > (this->_pos[0] + 1)) && (size == static_cast <size_t> (this->_pos[1] + 9)));
										// Если длина частей строки верная
										if(valid){
											// Получаем версию протокола
											const char * version = (buffer + (this->_pos[1] + 1));
											// Версия протокола должна соответствовать HTTP/цифра.цифра
											valid = ((::memcmp(version, "HTTP/", 5) == 0) && (::isdigit(static_cast <uint8_t> (version[5])) != 0) && (version[6] == '.') && (::isdigit(static_cast <uint8_t> (version[7])) != 0));
										}
										// Выполняем проверку символов метода запроса
										for(int32_t j = 0; valid && (j < this->_pos[0]); j++)
											// Метод запроса должен состоять только из символов токена
											valid = ((::isalnum(static_cast <uint8_t> (buffer[j])) != 0) || ((buffer[j] != '\0') && (::strchr("!#$%&'*+-.^_`|~", buffer[j]) != nullptr)));
										// Выполняем проверку символов цели запроса
										for(int32_t j = (this->_pos[0] + 1); valid && (j < this->_pos[1]); j++)
											// Цель запроса не может содержать управляющие символы
											valid = ((static_cast <uint8_t> (buffer[j]) > 0x20) && (static_cast <uint8_t> (buffer[j]) != 0x7F));
										// Если стартовая строка запроса корректна и старшая версия протокола равна 1
										if(valid && (buffer[this->_pos[1] + 6] == '1')){
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
											/**
											 * Младшая версия выше поддерживаемой обрабатывается как HTTP/1.1 (RFC 9110 §2.5):
											 * HTTP/1.2 и далее совместимы с HTTP/1.1, поэтому отклонять их нельзя
											 */
											this->_request.version = ((buffer[this->_pos[1] + 8] == '0') ? 1.0 : 1.1);
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
										/**
										 * Старшая версия протокола отличная от 1 (например HTTP/2.0 или HTTP/9.9) не поддерживается
										 * разборщиком HTTP/1: отвечаем 505 и закрываем подключение (RFC 9110 §15.6.6), а не 200
										 */
										} else if(valid) {
											// Выполняем очистку всех ранее полученных данных
											this->clear();
											// Запрос с неподдерживаемой версией протокола отклоняется целиком
											this->_fault = 505;
											// Прекращаем обработку запроса
											this->_state = state_t::END;
											// Сообщаем, что версия протокола не поддерживается
											this->_log->print("HTTP version not supported", log_t::flag_t::WARNING);
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "HTTP version not supported");
										// Если данные пришли неправильные
										} else {
											// Выполняем очистку всех ранее полученных данных
											this->clear();
											// Запрос с неверной стартовой строкой отклоняется целиком
											this->_fault = 400;
											// Прекращаем обработку запроса
											this->_state = state_t::END;
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
										// Запрос с неверной стартовой строкой отклоняется целиком
										this->_fault = 400;
										// Прекращаем обработку запроса
										this->_state = state_t::END;
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
								// Получаем ключ заголовка
								string key(buffer, this->_pos[0]);
								// Получаем значение заголовка
								string val(buffer + (this->_pos[0] + 1), size - (this->_pos[0] + 1));
								/**
								 * Имя заголовка запроса не может быть пустым или содержать пробелы (RFC 9112 §5.1):
								 * "Content-Length : 5" и продолжения строк иначе понимаются разными узлами по-разному
								 */
								if((this->_hid == hid_t::SERVER) && (key.empty() || (key.find_first_of(" \t") != string::npos))){
									// Выводим сообщение об ошибке
									this->_log->print("Broken request header", log_t::flag_t::WARNING);
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken request header");
									// Запрос отклоняется целиком
									this->_fault = 400;
									// Прекращаем обработку запроса
									this->_state = state_t::END;
									// Выходим из функции
									return;
								}
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
			});
			// Если мы работаем с сервером и запрос ещё не отклонён
			if((this->_hid == hid_t::SERVER) && (this->_fault == 0)){
				// Увеличиваем размер полученной секции заголовков
				this->_headerBytes += result;
				// Если заголовки ещё не получены, а недочитанная строка превышает допустимый размер
				if(((this->_state == state_t::QUERY) || (this->_state == state_t::HEADERS)) && ((this->_headerBytes + (size - result)) > AWH_MAX_HEADERS_SIZE)){
					// Выводим сообщение об ошибке
					this->_log->print("Request header fields are too large", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Request header fields are too large");
					// Запрос отклоняется целиком
					this->_fault = 431;
					// Прекращаем обработку запроса
					this->_state = state_t::END;
				// Если заголовки ещё не получены
				} else if((this->_state == state_t::QUERY) || (this->_state == state_t::HEADERS)) {
					/**
					 * Недочитанная строка с одиночным возвратом каретки (RFC 9112 §2.2) отклоняется сразу:
					 * при окончаниях строк CR перевод строки не придёт никогда, и сервер молчал бы до таймаута
					 */
					for(size_t i = result; i < size; i++){
						// Если строка завершена, её разберёт следующий вызов
						if(buffer[i] == '\n')
							// Выходим из цикла
							break;
						// Если за возвратом каретки получен не перевод строки
						else if((buffer[i] == '\r') && ((i + 1) < size) && (buffer[i + 1] != '\n')) {
							// Выводим сообщение об ошибке
							this->_log->print("Broken request line ending", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_id, log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Broken request line ending");
							// Запрос отклоняется целиком
							this->_fault = 400;
							// Прекращаем обработку запроса
							this->_state = state_t::END;
							// Выходим из цикла
							break;
						}
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод препарирования HTTP заголовков
 *
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
			/**
			 * Возврат каретки без следующего за ним перевода строки концом секции не является (RFC 9112 §2.2):
			 * иначе остаток строки после одиночного возврата каретки молча проглатывался как пустая строка
			 */
			else if(stop && (old == '\r'))
				// Снимаем флаг конца, строка разбирается обычным образом
				stop = false;
			// Если сепаратор найден, добавляем его в массив
			if((this->_separator != '\0') && (letter == this->_separator) && (count < 2)){
				// Устанавливаем позицию найденного разделителя
				this->_pos[count] = (i - offset);
				// Увеличиваем количество найденных разделителей
				count++;
			}
			// Если текущая буква является переносом строк
			/**
			 * Перевод строки в самом начале буфера тоже завершает строку: при одиночных переводах строки
			 * (RFC 9112 §2.2 разрешает их принимать) пустая строка конца заголовков может прийти отдельным пакетом
			 */
			if(letter == '\n'){
				// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
				length = ((old == '\r' ? i - 1 : i) - offset);
				/*
				// Если символ является последним и он не является переносом строки
				if((i == (size - 1)) && (letter != '\n'))
					// Увеличиваем общий размер обработанных байт
					length++;
				*/
				/**
				 * Пустая строка завершает секцию заголовков и при одиночном переводе строки (RFC 9112 §2.2),
				 * иначе запрос с окончаниями строк LF ожидал продолжения до истечения времени ожидания
				 */
				if(length == 0){
					// Устанавливаем флаг конца
					stop = ((this->_state == state_t::HEADERS) || (this->_state == state_t::BODY));
					// Выполняем функцию обратного вызова
					callback(nullptr, 0, i + 1, stop);
					/**
					 * После конца секции заголовков разбор строк прекращается: следующие байты
					 * принадлежат телу или следующему запросу и не должны учитываться как заголовки
					 */
					if(stop)
						// Выходим из цикла
						break;
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
 * @brief Метод получения бинарного дампа
 *
 * @return бинарный дамп данных
 */
awh::buffer_t awh::Web::dump() const noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	{
		// Длина строки, количество элементов
		size_t length = 0, count = 0;
		// Устанавливаем идентификатор HTTP-модуля
		result.push(&this->_id, sizeof(this->_id));
		// Устанавливаем тип используемого HTTP-модуля
		result.push(&this->_hid, sizeof(this->_hid));
		// Устанавливаем массив позиций в буфере сепаратора
		result.push(&this->_pos, sizeof(this->_pos));
		// Устанавливаем стейт текущего запроса
		result.push(&this->_state, sizeof(this->_state));
		// Устанавливаем размер тела сообщения
		result.push(&this->_bodySize, sizeof(this->_bodySize));
		// Устанавливаем сепаратор для детекции в буфере
		result.push(&this->_separator, sizeof(this->_separator));
		// Устанавливаем код ответа на HTTP ответа
		result.push(&this->_response.code, sizeof(this->_response.code));
		// Устанавливаем версию протокола HTTP ответа
		result.push(&this->_response.version, sizeof(this->_response.version));
		// Устанавливаем метод HTTP-запроса
		result.push(&this->_request.method, sizeof(this->_request.method));
		// Устанавливаем версию протокола HTTP-запроса
		result.push(&this->_request.version, sizeof(this->_request.version));
		// Если URL-адрес запроса установлен
		if(!this->_request.url.empty()){
			// Получаем адрес URL-запроса
			const string & url = this->_uri.url(this->_request.url);
			// Получаем размер записи параметров HTTP-запроса
			length = url.size();
			// Устанавливаем размер записи параметров HTTP-запроса
			result.push(&length, sizeof(length));
			// Устанавливаем параметры HTTP-запроса
			result.push(url.data(), url.size());
		// Если URL-адрес запроса не установлен
		} else {
			// Получаем размер записи параметров HTTP-запроса
			length = 0;
			// Устанавливаем размер записи параметров HTTP-запроса
			result.push(&length, sizeof(length));
		}
		// Если текст ответа установлен
		if(!this->_response.message.empty()){
			// Получаем размер сообщения HTTP ответа
			length = this->_response.message.size();
			// Устанавливаем размер сообщения HTTP ответа
			result.push(&length, sizeof(length));
			// Устанавливаем данные сообщения HTTP ответа
			result.push(this->_response.message.data(), this->_response.message.size());
		// Если текст ответа не установлен
		} else {
			// Получаем размер записи параметров HTTP-запроса
			length = 0;
			// Устанавливаем размер записи параметров HTTP-запроса
			result.push(&length, sizeof(length));
		}
		// Получаем размер тела сообщения
		length = this->_body.size();
		// Устанавливаем размер тела сообщения
		result.push(&length, sizeof(length));
		// Устанавливаем данные тела сообщения
		result.push(static_cast <const char *> (this->_body), static_cast <size_t> (this->_body));
		// Получаем количество HTTP заголовков
		count = this->_headers.size();
		// Устанавливаем количество HTTP заголовков
		result.push(&count, sizeof(count));
		// Выполняем перебор всех HTTP заголовков
		for(auto & header : this->_headers){
			// Получаем размер названия HTTP заголовка
			length = header.first.size();
			// Устанавливаем размер названия HTTP заголовка
			result.push(&length, sizeof(length));
			// Устанавливаем данные названия HTTP заголовка
			result.push(header.first.data(), header.first.size());
			// Получаем размер значения HTTP заголовка
			length = header.second.size();
			// Устанавливаем размер значения HTTP заголовка
			result.push(&length, sizeof(length));
			// Устанавливаем данные значения HTTP заголовка
			result.push(header.second.data(), header.second.size());
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
void awh::Web::dump(const awh::buffer_t & data) noexcept {
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
			::memcpy(reinterpret_cast <void *> (&this->_id), buffer + offset, sizeof(this->_id));
			// Выполняем смещение в буфере
			offset += sizeof(this->_id);
			// Выполняем получение типа используемого HTTP-модуля
			::memcpy(reinterpret_cast <void *> (&this->_hid), buffer + offset, sizeof(this->_hid));
			// Выполняем смещение в буфере
			offset += sizeof(this->_hid);
			// Выполняем получение массива позиций в буфере сепаратора
			::memcpy(reinterpret_cast <void *> (&this->_pos), buffer + offset, sizeof(this->_pos));
			// Выполняем смещение в буфере
			offset += sizeof(this->_pos);
			// Выполняем получение стейта текущего запроса
			::memcpy(reinterpret_cast <void *> (&this->_state), buffer + offset, sizeof(this->_state));
			// Выполняем смещение в буфере
			offset += sizeof(this->_state);
			// Выполняем получение размера тела сообщения
			::memcpy(reinterpret_cast <void *> (&this->_bodySize), buffer + offset, sizeof(this->_bodySize));
			// Выполняем смещение в буфере
			offset += sizeof(this->_bodySize);
			// Выполняем получение сепаратора для детекции в буфере
			::memcpy(reinterpret_cast <void *> (&this->_separator), buffer + offset, sizeof(this->_separator));
			// Выполняем смещение в буфере
			offset += sizeof(this->_separator);
			// Выполняем получение кода ответа на HTTP-запрос
			::memcpy(reinterpret_cast <void *> (&this->_response.code), buffer + offset, sizeof(this->_response.code));
			// Выполняем смещение в буфере
			offset += sizeof(this->_response.code);
			// Выполняем получение версии протокола HTTP ответа
			::memcpy(reinterpret_cast <void *> (&this->_response.version), buffer + offset, sizeof(this->_response.version));
			// Выполняем смещение в буфере
			offset += sizeof(this->_response.version);
			// Выполняем получение метода HTTP-запроса
			::memcpy(reinterpret_cast <void *> (&this->_request.method), buffer + offset, sizeof(this->_request.method));
			// Выполняем смещение в буфере
			offset += sizeof(this->_request.method);
			// Выполняем получение версии протокола HTTP-запроса
			::memcpy(reinterpret_cast <void *> (&this->_request.version), buffer + offset, sizeof(this->_request.version));
			// Выполняем смещение в буфере
			offset += sizeof(this->_request.version);
			// Выполняем получение размера записи параметров HTTP-запроса
			::memcpy(reinterpret_cast <void *> (&length), buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если URL-адрес запроса установлен
			if(length > 0){
				// Создаём URL-адрес запроса
				string url(length, 0);
				// Выполняем получение параметров HTTP-запроса
				::memcpy(reinterpret_cast <void *> (url.data()), buffer + offset, length);
				// Устанавливаем URL-адрес запроса
				this->_request.url = this->_uri.parse(url);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера сообщения HTTP ответа
			::memcpy(reinterpret_cast <void *> (&length), buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если сообщение ответа установлено
			if(length > 0){
				// Выделяем память для сообщения HTTP ответа
				this->_response.message.resize(length, 0);
				// Выполняем получение сообщения HTTP ответа
				::memcpy(reinterpret_cast <void *> (this->_response.message.data()), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера тела сообщения
			::memcpy(reinterpret_cast <void *> (&length), buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если сообщение ответа установлено
			if(length > 0){
				// Выполняем получение данных тела сообщения
				this->_body.push(buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение количества HTTP заголовков
			::memcpy(reinterpret_cast <void *> (&count), buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем сброс заголовков
			this->_headers.clear();
			// Если количество заголовков больше чем ничего
			if(count > 0){
				// Выполняем последовательную загрузку всех заголовков
				for(size_t i = 0; i < count; i++){
					// Выполняем получение размера названия HTTP заголовка
					::memcpy(reinterpret_cast <void *> (&length), buffer + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выпделяем память для ключа заголовка
						string key(length, 0);
						// Выполняем получение ключа заголовка
						::memcpy(reinterpret_cast <void *> (key.data()), buffer + offset, length);
						// Выполняем смещение в буфере
						offset += length;
						// Выполняем получение размера значения HTTP заголовка
						::memcpy(reinterpret_cast <void *> (&length), buffer + offset, sizeof(length));
						// Выполняем смещение в буфере
						offset += sizeof(length);
						// Если размер получен
						if(length > 0){
							// Выпделяем память для значения заголовка
							string value(length, 0);
							// Выполняем получение значения заголовка
							::memcpy(reinterpret_cast <void *> (value.data()), buffer + offset, length);
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
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
	if((buffer != nullptr) && (size > 0) && (this->_state != state_t::END)){
		/**
		 * Определяем текущий стейт
		 */
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
 * @brief Метод очистки собранных данных
 *
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
	// Выполняем удаление памяти списка трейлеров
	std::unordered_set <decltype(this->_trailers)::value_type> ().swap(this->_trailers);
	// Выполняем удаление памяти полученных HTTP заголовков
	std::unordered_multimap <decltype(this->_headers)::key_type, decltype(this->_headers)::mapped_type> ().swap(this->_headers);
}
/**
 * @brief Метод сброса стейтов парсера
 *
 */
void awh::Web::reset() noexcept {
	// Выполняем сброс кода ошибки разбора запроса
	this->_fault = 0;
	// Выполняем сброс количества полученных заголовков
	this->_headerCount = 0;
	// Выполняем сброс размера полученной секции заголовков
	this->_headerBytes = 0;
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
 * @brief Метод проверки завершения обработки
 *
 * @return результат проверки
 */
bool awh::Web::isEnd() const noexcept {
	// Выводрим результат проверки
	return (this->_state == state_t::END);
}
/**
 * @brief Метод проверки существования заголовка
 *
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
 * @brief Проверка заголовка является ли он стандартным
 *
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Web::isStandard(const string & key) const noexcept {
	// Если ключ передан
	if(!key.empty())
		// Выполняем проверку заголовка (по копии: константный transform меняет строку на месте)
		return (this->_standardHeaders.count(this->_fmk->transform(string(key), fmk_t::transform_t::LOWER)) > 0);
	// Выводим результат
	return false;
}
/**
 * @brief Метод очистки данных тела
 *
 */
void awh::Web::clearBody() noexcept {
	// Выполняем очистку данных тела
	this->_body.clear();
}
/**
 * @brief Метод очистки списка заголовков
 *
 */
void awh::Web::clearHeaders() noexcept {
	// Выполняем очистку заголовков
	this->_headers.clear();
}
/**
 * @brief Метод получения данных тела запроса
 *
 * @return буфер данных тела запроса
 */
const awh::buffer_t & awh::Web::body() const noexcept {
	// Выводим данные тела
	return this->_body;
}
/**
 * @brief Метод добавления данных тела
 *
 * @param body буфер тела для добавления
 */
void awh::Web::body(const vector <char> & body) noexcept {
	// Если тело данных передано
	if(!body.empty())
		// Выполняем установку данных тела
		this->_body.push(body.data(), body.size());
}
/**
 * @brief Метод добавления данных тела
 *
 * @param buffer буфер тела для добавления
 * @param size   размер буфера теля для добавления
 */
void awh::Web::body(const char * buffer, const size_t size) noexcept {
	// Если тело данных передано
	if((buffer != nullptr) && (size > 0))
		// Выполняем установку данных тела
		this->_body.push(buffer, size);
}
/**
 * @brief Метод получение названия протокола для переключения
 *
 * @return название протокола для переключения
 */
const string & awh::Web::upgrade() const noexcept {
	// Выполняем вывод название протокола для переключения
	return this->_upgrade;
}
/**
 * @brief Метод установки название протокола для переключения
 *
 * @param upgrade название протокола для переключения
 */
void awh::Web::upgrade(const string & upgrade) noexcept {
	// Выполняем установку названия протокола для переключения
	this->_upgrade = upgrade;
}
/**
 * @brief Метод извлечения список протоколов к которому принадлежит заголовок
 *
 * @param key ключ заголовка
 * @return    список протоколов
 */
std::set <awh::Web::proto_t> awh::Web::proto(const string & key) const noexcept {
	// Если ключ передан
	if(!key.empty()){
		// Выполняем поиск заголовка (по копии: константный transform меняет строку на месте)
		auto i = this->_standardHeaders.find(this->_fmk->transform(string(key), fmk_t::transform_t::LOWER));
		// Если заголовок найден выводим результат
		if(i != this->_standardHeaders.end())
			// Выводим результат
			return i->second;
	}
	// Выводим результат
	return std::set <awh::Web::proto_t> ();
}
/**
 * @brief Метод удаления заголовка
 *
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
 * @brief Метод получения данных заголовка
 *
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
 * @brief Метод добавления заголовка
 *
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
 * @brief Метод получения списка заголовков
 *
 * @return список существующих заголовков
 */
const std::unordered_multimap <string, string> & awh::Web::headers() const noexcept {
	// Выводим список доступных заголовков
	return this->_headers;
}
/**
 * @brief Метод установки списка заголовков
 *
 * @param headers список заголовков для установки
 */
void awh::Web::headers(const std::unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков
	this->_headers = headers;
}
/**
 * @brief Метод получения идентификатора объекта
 *
 * @return идентификатор объекта
 */
uint64_t awh::Web::id() const noexcept {
	// Выводим идентификатор объекта
	return this->_id;
}
/**
 * @brief Метод установки идентификатора объекта
 *
 * @param id идентификатор объекта
 */
void awh::Web::id(const uint64_t id) noexcept {
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
 * @brief Метод установки стейта ожидания данных
 *
 * @param state стейт ожидания данных для установки
 */
void awh::Web::state(const state_t state) noexcept {
	// Выполняем установку стейта
	this->_state = state;
}
/**
 * @brief Метод получения кода ошибки разбора запроса
 *
 * @return код HTTP-ответа для ошибки разбора (0 если ошибки нет)
 */
uint32_t awh::Web::fault() const noexcept {
	// Выводим код ошибки разбора запроса
	return this->_fault;
}
/**
 * @brief Метод установки кода ошибки обработки запроса
 *
 * @param code код HTTP-ответа для ошибки (устанавливается только если ошибки ещё нет)
 */
void awh::Web::fault(const uint32_t code) noexcept {
	// Если ошибка ещё не установлена, первая причина отклонения сохраняется
	if(this->_fault == 0)
		// Устанавливаем код ошибки обработки запроса
		this->_fault = code;
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
 _separator('\0'), _pos{-1, -1},
 _bodySize(-1), _uri(fmk, log), _callback(log),
 _hid(hid_t::NONE), _state(state_t::QUERY),
 _fault(0), _headerCount(0), _headerBytes(0),
 _body(fmk, log), _upgrade{""}, _fmk(fmk), _log(log) {
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
