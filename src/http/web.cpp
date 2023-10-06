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
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <http/web.hpp>

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
					if(this->_callback.is("chunking"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const vector <char> &, const web_t *> ("chunking", this->_id, this->_chunk.data, this);
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
					if(this->_callback.is("chunking"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const vector <char> &, const web_t *> ("chunking", this->_id, this->_chunk.data, this);
					// Если тело сообщения полностью собранно
					if(this->_bodySize == this->_chunk.size){
						// Очищаем собранные данные
						this->_chunk.clear();
						// Определяем тип HTTP модуля
						switch(static_cast <uint8_t> (this->_hid)){
							// Если мы работаем с клиентом
							case static_cast <uint8_t> (hid_t::CLIENT): {
								// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
								if(this->_callback.is("entity"))
									// Выводим функцию обратного вызова
									this->_callback.call <const uint64_t, const u_int, const string &, const vector <char> &> ("entity", this->_id, this->_res.code, this->_res.message, this->_body);
							} break;
							// Если мы работаем с сервером
							case static_cast <uint8_t> (hid_t::SERVER): {
								// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
								if(this->_callback.is("entity"))
									// Выводим функцию обратного вызова
									this->_callback.call <const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &> ("entity", this->_id, this->_req.method, this->_req.url, this->_body);
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
						// Если мы ожидаем получения размера тела чанка
						case static_cast <uint8_t> (cstate_t::SIZE): {
							// Если мы получили возврат каретки
							if(buffer[i] == '\r'){
								// Меняем стейт чанка
								this->_chunk.state = cstate_t::ENDSIZE;
								// Получаем размер чанка
								this->_chunk.size = this->_fmk->atoi(string(
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
						case static_cast <uint8_t> (cstate_t::ENDSIZE): {
							// Увеличиваем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили перевод строки
							if(buffer[i] == '\n'){
								// Если размер получен 0-й значит мы завершили сбор данных
								if(this->_chunk.size == 0)
									// Меняем стейт чанка
									this->_chunk.state = cstate_t::STOPBODY;
								// Если данные собраны не полностью
								else {
									// Если количества байт достаточно для сбора тела чанка
									if((size - offset) >= this->_chunk.size){
										// Меняем стейт чанка
										this->_chunk.state = cstate_t::STOPBODY;
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
										this->_chunk.state = cstate_t::BODY;
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
								// Выходим
								goto Stop;
							}
						} break;
						// Если мы ожидаем сбора тела чанка
						case static_cast <uint8_t> (cstate_t::BODY): {
							// Определяем количество необходимых байт
							size_t rem = (this->_chunk.size - this->_chunk.data.size());
							// Если количества байт достаточно для сбора тела чанка
							if(size >= rem){
								// Меняем стейт чанка
								this->_chunk.state = cstate_t::STOPBODY;
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
						case static_cast <uint8_t> (cstate_t::STOPBODY): {
							// Увеличиваем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили возврат каретки
							if(buffer[i] == '\r')
								// Меняем стейт чанка
								this->_chunk.state = cstate_t::ENDBODY;
							// Если символ отличается, значит ошибка
							else {
								// Устанавливаем символ ошибки
								error = 'r';
								// Выходим
								goto Stop;
							}
						} break;
						// Если мы ожидаем получение окончания сбора данных тела чанка
						case static_cast <uint8_t> (cstate_t::ENDBODY): {
							// Увеличиваем смещение
							offset = (i + 1);
							// Запоминаем количество обработанных байт
							result = offset;
							// Если мы получили перевод строки
							if(buffer[i] == '\n'){
								// Если размер получен 0-й значит мы завершили сбор данных
								if(this->_chunk.size == 0) goto Stop;								
								// Если функция обратного вызова на перехват входящих чанков установлена
								else if(this->_callback.is("chunking"))
									// Выводим функцию обратного вызова
									this->_callback.call <const uint64_t, const vector <char> &, const web_t *> ("chunking", this->_id, this->_chunk.data, this);
								// Выполняем очистку чанка
								this->_chunk.clear();
							// Если символ отличается, значит ошибка
							} else {
								// Устанавливаем символ ошибки
								error = 'n';
								// Выходим
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
				// Определяем тип HTTP модуля
				switch(static_cast <uint8_t> (this->_hid)){
					// Если мы работаем с клиентом
					case static_cast <uint8_t> (hid_t::CLIENT): {
						// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
						if(this->_callback.is("entity"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const u_int, const string &, const vector <char> &> ("entity", this->_id, this->_res.code, this->_res.message, this->_body);
					} break;
					// Если мы работаем с сервером
					case static_cast <uint8_t> (hid_t::SERVER): {
						// Если функция обратного вызова на вывод полученного тела данных с сервера установлена
						if(this->_callback.is("entity"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &> ("entity", this->_id, this->_req.method, this->_req.url, this->_body);
					} break;
				}
				// Тело в запросе не передано
				this->_state = state_t::END;
				// Если мы получили ошибку обработки данных
				if(error != '\0')
					// Сообщаем, что переданное тело содержит ошибки
					this->_log->print("Body chunk contains errors, [\\%c] is expected", log_t::flag_t::WARNING, error);
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
				// Если - это режим ожидания получения запроса
				case static_cast <uint8_t> (state_t::QUERY): this->_separator = ' '; break;
				// Если - это режим получения заголовков
				case static_cast <uint8_t> (state_t::HEADERS): this->_separator = ':'; break;
			}
			/**
			 * Выполняем парсинг заголовков запроса
			 */
			this->prepare(buffer, size, [&result, this](const char * buffer, const size_t size, const size_t bytes, const bool stop) noexcept {
				// Запоминаем количество обработанных байт
				result = bytes;
				// Если все данные получены
				if(stop){
					// Определяем тип HTTP модуля
					switch(static_cast <uint8_t> (this->_hid)){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (hid_t::CLIENT): {
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callback.is("headers"))
								// Выводим функцию обратного вызова
								this->_callback.call <const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", this->_id, this->_res.code, this->_res.message, this->_headers);
						} break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (hid_t::SERVER): {
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callback.is("headers"))
								// Выводим функцию обратного вызова
								this->_callback.call <const uint64_t, const method_t, const uri_t::url_t &, const unordered_multimap <string, string> &> ("headers", this->_id, this->_req.method, this->_req.url, this->_headers);
						} break;
					}
					// Получаем размер тела
					auto it = this->_headers.find("content-length");
					// Если размер запроса передан
					if(it != this->_headers.end()){
						// Запоминаем размер тела сообщения
						this->_bodySize = static_cast <size_t> (::stoull(it->second));
						// Если размер тела не получен
						if(this->_bodySize == 0){
							// Запрашиваем заголовок подключения
							const string & header = this->header("connection");
							// Если заголовок подключения найден
							if(header.empty() || !this->_fmk->compare(header, "close")){
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
						// Получаем размер тела
						it = this->_headers.find("transfer-encoding");
						// Если размер запроса передан
						if(it != this->_headers.end()){
							// Если нужно получать размер тела чанками
							if(this->_fmk->exists("chunked", it->second)){
								// Устанавливаем стейт поиска тела запроса
								this->_state = state_t::BODY;
								// Продолжаем работу
								goto end;
							}
						}
					}
					// Тело в запросе не передано
					this->_state = state_t::END;
					// Устанавливаем метку завершения работы
					end:
					// Выходим из функции
					return;
				// Если необходимо  получить оставшиеся данные
				} else {
					// Определяем статус режима работы
					switch(static_cast <uint8_t> (this->_state)){
						// Если - это режим ожидания получения запроса
						case static_cast <uint8_t> (state_t::QUERY): {
							// Определяем тип HTTP модуля
							switch(static_cast <uint8_t> (this->_hid)){
								// Если мы работаем с клиентом
								case static_cast <uint8_t> (hid_t::CLIENT): {
									// Создаём буфер для проверки
									char temp[5];
									// Копируем полученную строку
									strncpy(temp, buffer, 4);
									// Устанавливаем конец строки
									temp[4] = '\0';
									// Если мы получили ответ от сервера
									if(strcmp(temp, "HTTP") == 0){
										// Выполняем очистку всех ранее полученных данных
										this->clear();
										// Выполняем сброс размера тела
										this->_bodySize = -1;
										// Устанавливаем разделитель
										this->_separator = ':';
										// Устанавливаем стейт ожидания получения заголовков
										this->_state = state_t::HEADERS;
										// Получаем версию протокол запроса
										this->_res.version = ::stof(string(buffer + 5, this->_pos[0] - 5));
										// Получаем сообщение ответа
										this->_res.message.assign(buffer + (this->_pos[1] + 1), size - (this->_pos[1] + 1));
										// Получаем код ответа
										this->_res.code = static_cast <u_int> (::stoi(string(buffer + (this->_pos[0] + 1), this->_pos[1] - (this->_pos[0] + 1))));
										// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
										if(this->_callback.is("response"))
											// Выводим функцию обратного вызова
											this->_callback.call <const uint64_t, const u_int, const string &> ("response", this->_id, this->_res.code, this->_res.message);
									// Если данные пришли неправильные
									} else {
										// Выполняем очистку всех ранее полученных данных
										this->clear();
										// Сообщаем, что переданное тело содержит ошибки
										this->_log->print("Broken response server", log_t::flag_t::WARNING);
									}
								} break;
								// Если мы работаем с сервером
								case static_cast <uint8_t> (hid_t::SERVER): {
									// Создаём буфер для проверки
									char temp[5];
									// Копируем полученную строку
									strncpy(temp, buffer + (this->_pos[1] + 1), 4);
									// Устанавливаем конец строки
									temp[4] = '\0';
									// Если мы получили ответ от сервера
									if(strcmp(temp, "HTTP") == 0){
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
										this->_req.version = ::stof(string(buffer + (this->_pos[1] + 6), size - (this->_pos[1] + 6)));
										// Выполняем установку URI-параметров запроса
										this->_req.url = this->_uri.parse(uri);
										// Если метод определён как GET
										if(this->_fmk->compare(method, "get"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::GET;
										// Если метод определён как PUT
										else if(this->_fmk->compare(method, "put"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::PUT;
										// Если метод определён как POST
										else if(this->_fmk->compare(method, "post"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::POST;
										// Если метод определён как HEAD
										else if(this->_fmk->compare(method, "head"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::HEAD;
										// Если метод определён как DELETE
										else if(this->_fmk->compare(method, "delete"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::DEL;
										// Если метод определён как PATCH
										else if(this->_fmk->compare(method, "patch"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::PATCH;
										// Если метод определён как TRACE
										else if(this->_fmk->compare(method, "trace"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::TRACE;
										// Если метод определён как OPTIONS
										else if(this->_fmk->compare(method, "options"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::OPTIONS;
										// Если метод определён как CONNECT
										else if(this->_fmk->compare(method, "connect"))
											// Выполняем установку метода запроса
											this->_req.method = method_t::CONNECT;
										// Если функция обратного вызова на вывод запроса клиента на выполненный запрос к серверу установлена
										if(this->_callback.is("request"))
											// Выводим функцию обратного вызова
											this->_callback.call <const uint64_t, const method_t, const uri_t::url_t &> ("request", this->_id, this->_req.method, this->_req.url);
									// Если данные пришли неправильные
									} else {
										// Выполняем очистку всех ранее полученных данных
										this->clear();
										// Сообщаем, что переданное тело содержит ошибки
										this->_log->print("Broken request client", log_t::flag_t::WARNING);
									}
								} break;
							}
						} break;
						// Если - это режим получения заголовков
						case static_cast <uint8_t> (state_t::HEADERS): {
							// Получаем ключ заголовка
							string key(buffer, this->_pos[0]);
							// Получаем значение заголовка
							string val(buffer + (this->_pos[0] + 1), size - (this->_pos[0] + 1));
							// Добавляем заголовок в список заголовков
							if(!key.empty() && !val.empty()){
								// Если название заголовка соответствует HOST
								if(this->_fmk->compare(key, "host")){
									// Создаём объект работы с IP-адресами
									net_t net;
									// Выполняем установку схемы запроса
									this->_req.url.schema = "http";
									// Выполняем установку хоста
									this->_req.url.host = this->_fmk->transform(val, fmk_t::transform_t::TRIM);
									
									cout << " ---------------------- " << this->_req.url.host << endl;
									
									// Выполняем поиск разделителя
									const size_t pos = this->_req.url.host.rfind(':');
									// Если разделитель найден
									if(pos != string::npos){
										// Получаем порт сервера
										const string & port = this->_req.url.host.substr(pos + 1);
										// Если данные порта являются числом
										if(this->_fmk->is(port, fmk_t::check_t::NUMBER)){
											// Выполняем установку порта сервера
											this->_req.url.port = static_cast <u_int> (::stoi(port));
											// Выполняем получение хоста сервера
											this->_req.url.host = this->_req.url.host.substr(0, pos);
										}
									}
									// Определяем тип домена
									switch(static_cast <uint8_t> (net.host(this->_req.url.host))){
										// Если - это IP-адрес сети IPv4
										case static_cast <uint8_t> (net_t::type_t::IPV4): {
											// Выполняем установку семейства IP-адресов
											this->_req.url.family = AF_INET;
											// Выполняем установку IPv4 адреса
											this->_req.url.ip = this->_req.url.host;
										} break;
										// Если - это IP-адрес сети IPv6
										case static_cast <uint8_t> (net_t::type_t::IPV6): {
											// Выполняем установку семейства IP-адресов
											this->_req.url.family = AF_INET6;
											// Выполняем установку IPv6 адреса
											this->_req.url.ip = net = this->_req.url.host;
										} break;
										// Если - это доменное имя
										case static_cast <uint8_t> (net_t::type_t::DOMN):
											// Выполняем установку IPv6 адреса
											this->_req.url.domain = this->_fmk->transform(this->_req.url.host, fmk_t::transform_t::LOWER);
										break;
									}
								}
								// Добавляем заголовок в список
								this->_headers.emplace(
									this->_fmk->transform(key, fmk_t::transform_t::LOWER),
									this->_fmk->transform(val, fmk_t::transform_t::TRIM)
								);
								// Если функция обратного вызова на вывод полученного заголовка с сервера установлена
								if(this->_callback.is("header"))
									// Выводим функцию обратного вызова
									this->_callback.call <const uint64_t,const string &, const string &> ("header", this->_id, std::move(key), std::move(val));
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
		// Выполняем сброс массива сепараторов
		memset(this->_pos, -1, sizeof(this->_pos));
		// Переходим по всему буферу
		for(size_t i = 0; i < size; i++){
			// Получаем значение текущей буквы
			letter = buffer[i];
			// Если текущий символ перенос строки и это конец, выходим
			if(stop && (letter == '\n')){
				// Выводим функцию обратного вызова
				callback(nullptr, 0, i + 1, stop);
				// Выходим из цикла
				break;
			}
			// Если предыдущий символ был переносом строки а текущий возврат каретки
			if((old == '\n') && (letter == '\r')) stop = true;
			// Если сепаратор найден, добавляем его в массив
			if((this->_separator != '\0') && (letter == this->_separator) && (count < 2)){
				// Устанавливаем позицию найденного разделителя
				this->_pos[count] = (i - offset);
				// Увеличиваем количество найденных разделителей
				count++;
			}
			// Если текущая буква является переносом строк
			if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
				// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
				length = ((old == '\r' ? i - 1 : i) - offset);
				// Если символ является последним и он не является переносом строки
				if((i == (size - 1)) && (letter != '\n')) length++;
				// Если длина слова получена, выводим полученную строку
				callback(buffer + offset, length, i + 1, stop);
				// Если массив сепараторов получен
				if(this->_separator != '\0'){
					// Выполняем сброс количество найденных сепараторов
					count = 0;
					// Выполняем сброс массива сепараторов
					memset(this->_pos, -1, sizeof(this->_pos));
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
		// Устанавливаем идентификатор HTTP модуля
		result.insert(result.end(), (const char *) &this->_id, (const char *) &this->_id + sizeof(this->_id));
		// Устанавливаем тип используемого HTTP модуля
		result.insert(result.end(), (const char *) &this->_hid, (const char *) &this->_hid + sizeof(this->_hid));
		// Устанавливаем стейт текущего запроса
		result.insert(result.end(), (const char *) &this->_state, (const char *) &this->_state + sizeof(this->_state));
		// Устанавливаем массив позиций в буфере сепаратора
		result.insert(result.end(), (const char *) &this->_pos, (const char *) &this->_pos + sizeof(this->_pos));
		// Устанавливаем размер тела сообщения
		result.insert(result.end(), (const char *) &this->_bodySize, (const char *) &this->_bodySize + sizeof(this->_bodySize));
		// Устанавливаем сепаратор для детекции в буфере
		result.insert(result.end(), (const char *) &this->_separator, (const char *) &this->_separator + sizeof(this->_separator));
		// Устанавливаем версию протокола HTTP запроса
		result.insert(result.end(), (const char *) &this->_req.version, (const char *) &this->_req.version + sizeof(this->_req.version));
		// Устанавливаем версию протокола HTTP ответа
		result.insert(result.end(), (const char *) &this->_res.version, (const char *) &this->_res.version + sizeof(this->_res.version));
		// Устанавливаем код ответа на HTTP ответа
		result.insert(result.end(), (const char *) &this->_res.code, (const char *) &this->_res.code + sizeof(this->_res.code));
		// Устанавливаем метод HTTP запроса
		result.insert(result.end(), (const char *) &this->_req.method, (const char *) &this->_req.method + sizeof(this->_req.method));
		// Если URL-адрес запроса установлен
		if(!this->_req.url.empty()){
			// Получаем адрес URL-запроса
			const string & url = this->_uri.url(this->_req.url);
			// Получаем размер записи параметров HTTP запроса
			length = url.size();
			// Устанавливаем размер записи параметров HTTP запроса
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем параметры HTTP запроса
			result.insert(result.end(), url.begin(), url.end());
		// Если URL-адрес запроса не установлен
		} else {
			// Получаем размер записи параметров HTTP запроса
			length = 0;
			// Устанавливаем размер записи параметров HTTP запроса
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		}
		// Если текст ответа установлен
		if(!this->_res.message.empty()){
			// Получаем размер сообщения HTTP ответа
			length = this->_res.message.size();
			// Устанавливаем размер сообщения HTTP ответа
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные сообщения HTTP ответа
			result.insert(result.end(), this->_res.message.begin(), this->_res.message.end());
		// Если текст ответа не установлен
		} else {
			// Получаем размер записи параметров HTTP запроса
			length = 0;
			// Устанавливаем размер записи параметров HTTP запроса
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		}
		// Получаем размер тела сообщения
		length = this->_body.size();
		// Устанавливаем размер тела сообщения
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные тела сообщения
		result.insert(result.end(), this->_body.begin(), this->_body.end());
		// Получаем количество HTTP заголовков
		count = this->_headers.size();
		// Устанавливаем количество HTTP заголовков
		result.insert(result.end(), (const char *) &count, (const char *) &count + sizeof(count));
		// Выполняем перебор всех HTTP заголовков
		for(auto & header : this->_headers){
			// Получаем размер названия HTTP заголовка
			length = header.first.size();
			// Устанавливаем размер названия HTTP заголовка
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные названия HTTP заголовка
			result.insert(result.end(), header.first.begin(), header.first.end());
			// Получаем размер значения HTTP заголовка
			length = header.second.size();
			// Устанавливаем размер значения HTTP заголовка
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
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
		// Выполняем получение идентификатора HTTP модуля
		::memcpy((void *) &this->_id, data.data() + offset, sizeof(this->_id));
		// Выполняем смещение в буфере
		offset += sizeof(this->_id);
		// Выполняем получение типа используемого HTTP модуля
		::memcpy((void *) &this->_hid, data.data() + offset, sizeof(this->_hid));
		// Выполняем смещение в буфере
		offset += sizeof(this->_hid);
		// Выполняем получение стейта текущего запроса
		::memcpy((void *) &this->_state, data.data() + offset, sizeof(this->_state));
		// Выполняем смещение в буфере
		offset += sizeof(this->_state);
		// Выполняем получение массива позиций в буфере сепаратора
		::memcpy((void *) &this->_pos, data.data() + offset, sizeof(this->_pos));
		// Выполняем смещение в буфере
		offset += sizeof(this->_pos);
		// Выполняем получение размера тела сообщения
		::memcpy((void *) &this->_bodySize, data.data() + offset, sizeof(this->_bodySize));
		// Выполняем смещение в буфере
		offset += sizeof(this->_bodySize);
		// Выполняем получение сепаратора для детекции в буфере
		::memcpy((void *) &this->_separator, data.data() + offset, sizeof(this->_separator));
		// Выполняем смещение в буфере
		offset += sizeof(this->_separator);
		// Выполняем получение версии протокола HTTP запроса
		::memcpy((void *) &this->_req.version, data.data() + offset, sizeof(this->_req.version));
		// Выполняем смещение в буфере
		offset += sizeof(this->_req.version);
		// Выполняем получение версии протокола HTTP ответа
		::memcpy((void *) &this->_res.version, data.data() + offset, sizeof(this->_res.version));
		// Выполняем смещение в буфере
		offset += sizeof(this->_res.version);
		// Выполняем получение кода ответа на HTTP запрос
		::memcpy((void *) &this->_res.code, data.data() + offset, sizeof(this->_res.code));
		// Выполняем смещение в буфере
		offset += sizeof(this->_res.code);
		// Выполняем получение метода HTTP запроса
		::memcpy((void *) &this->_req.method, data.data() + offset, sizeof(this->_req.method));
		// Выполняем смещение в буфере
		offset += sizeof(this->_req.method);
		// Выполняем получение размера записи параметров HTTP запроса
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если URL-адрес запроса установлен
		if(length > 0){
			// Создаём URL-адрес запроса
			string url(length, 0);
			// Выполняем получение параметров HTTP запроса
			::memcpy((void *) url.data(), data.data() + offset, length);
			// Устанавливаем URL-адрес запроса
			this->_req.url = this->_uri.parse(std::move(url));
		}
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера сообщения HTTP ответа
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если сообщение ответа установлено
		if(length > 0){
			// Выделяем память для сообщения HTTP ответа
			this->_res.message.resize(length, 0);
			// Выполняем получение сообщения HTTP ответа
			::memcpy((void *) this->_res.message.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение размера HTTP заголовка
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выполняем получение размера тела сообщения
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных тела сообщения
		this->_body.resize(length, 0);
		// Выполняем получение данных тела сообщения
		::memcpy((void *) this->_body.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение количества HTTP заголовков
		::memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс заголовков
		this->_headers.clear();
		// Выполняем последовательную загрузку всех заголовков
		for(size_t i = 0; i < count; i++){
			// Выполняем получение размера названия HTTP заголовка
			::memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выпделяем память для ключа заголовка
			string key(length, 0);
			// Выполняем получение ключа заголовка
			::memcpy((void *) key.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Выполняем получение размера значения HTTP заголовка
			::memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выпделяем память для значения заголовка
			string value(length, 0);
			// Выполняем получение значения заголовка
			::memcpy((void *) value.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если и ключ и значение заголовка получены
			if(!key.empty() && !value.empty())
				// Добавляем заголовок в список заголовков
				this->_headers.emplace(std::move(key), std::move(value));
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
			case static_cast <uint8_t> (state_t::BODY): result = this->readPayload(buffer, size); break;
		}
	}
	// Выводим реузльтат
	return result;
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Web::clear() noexcept {
	// Выполняем очистку тела HTTP запроса
	this->_body.clear();
	// Выполняем сброс параметров чанка
	this->_chunk.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->_headers.clear();
	// Выполняем сброс параметров запроса
	this->_req = req_t();
	// Выполняем сброс параметров ответа
	this->_res = res_t();
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
	return this->_req;
}
/**
 * request Метод добавления объекта запроса на сервер
 * @param req объект запроса на сервер
 */
void awh::Web::request(const req_t & req) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_req = req;
}
/**
 * response Метод получения объекта ответа сервера
 * @return объект ответа сервера
 */
const awh::Web::res_t & awh::Web::response() const noexcept {
	// Выводим объект ответа сервера
	return this->_res;
}
/**
 * response Метод добавления объекта ответа сервера
 * @param res объект ответа сервера
 */
void awh::Web::response(const res_t & res) noexcept {
	// Устанавливаем объект ответа сервера
	this->_res = res;
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
 * body Метод установки данных тела
 * @param body буфер тела для установки
 */
void awh::Web::body(const vector <char> & body) noexcept {
	// Если тело данных передано
	if(!body.empty())
		// Выполняем установку данных тела
		this->_body.insert(this->_body.end(), body.begin(), body.end());
}
/**
 * rmHeader Метод удаления заголовка
 * @param key ключ заголовка
 */
void awh::Web::rmHeader(const string & key) noexcept {
	// Если ключ заголовка передан
	if(!key.empty()){
		// Выполняем перебор всех заголовков
		for(auto it = this->_headers.begin(); it != this->_headers.end();){
			// Выполняем проверку существования заголовка
			if(this->_fmk->compare(it->first, key))
				// Выполняем удаление указанного заголовка
				it = this->_headers.erase(it);
			// Иначе ищем заголовок дальше
			else it++;
		}
	}
}
/**
 * header Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
const string awh::Web::header(const string & key) const noexcept {
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
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const u_int, const string &)> ("response", callback);
}
/** 
 * on Метод установки функции вывода запроса клиента на выполненный запрос к серверу
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const method_t, const uri_t::url_t &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const method_t, const uri_t::url_t &)> ("request", callback);
}
/** 
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const string &, const string &)> ("header", callback);
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const vector <char> &, const Web *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const vector <char> &, const web_t *)> ("chunking", callback);
}
/** 
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const u_int, const string &, const vector <char> &)> ("entity", callback);
}
/** 
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &)> ("entity", callback);
}
/** 
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", callback);
}
/** 
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::Web::on(function <void (const uint64_t, const method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", callback);
}
