/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <http.hpp>

/**
 * date Метод получения текущей даты для HTTP запроса
 * @return текущая дата
 */
const string awh::Http::date() const noexcept {
	// Создаём буфер данных
	char buffer[1000];
	// Получаем текущее время
	time_t now = time(nullptr);
	// Извлекаем текущее время
	struct tm tm = * gmtime(&now);
	// Зануляем буфер
	memset(buffer, 0, sizeof(buffer));
	// Получаем формат времени
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", &tm);
	// Выводим результат
	return buffer;
}
/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::Http::checkAuth() noexcept {
	// Результат работы функции
	stath_t result = stath_t::FAULT;
	// Если активирован режим обработки ответа сервера
	if(this->mode == mode_t::RESPONSE){
		// Проверяем код ответа
		switch(this->query.code){
			// Если требуется авторизация
			case 401:
			case 407: {
				// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
				if(!this->failAuth && (this->auth->getType() == auth_t::type_t::DIGEST)){
					// Получаем параметры авторизации
					auto it = this->headers.find("www-authenticate");
					// Если параметры авторизации найдены
					if((this->failAuth = (it != this->headers.end()))){
						// Устанавливаем заголовок HTTP в параметры авторизации
						this->auth->setHeader(it->second);
						// Просим повторить авторизацию ещё раз
						result = stath_t::RETRY;
					}
				}
			} break;
			// Если нужно произвести редирект
			case 301:
			case 308: {
				// Получаем параметры авторизации
				auto it = this->headers.find("location");
				// Если адрес перенаправления найден
				if(it != this->headers.end()){
					// Выполняем парсинг URL
					uri_t::url_t tmp = this->uri->parseUrl(it->second);
					// Если параметры URL существуют
					if(!this->url.params.empty())
						// Переходим по всему списку параметров
						for(auto & param : this->url.params) tmp.params.emplace(param);
					// Меняем IP адрес сервера
					const_cast <uri_t::url_t *> (&this->url)->ip = move(tmp.ip);
					// Меняем порт сервера
					const_cast <uri_t::url_t *> (&this->url)->port = move(tmp.port);
					// Меняем на путь сервере
					const_cast <uri_t::url_t *> (&this->url)->path = move(tmp.path);
					// Меняем доменное имя сервера
					const_cast <uri_t::url_t *> (&this->url)->domain = move(tmp.domain);
					// Меняем протокол запроса сервера
					const_cast <uri_t::url_t *> (&this->url)->schema = move(tmp.schema);
					// Устанавливаем новый список параметров
					const_cast <uri_t::url_t *> (&this->url)->params = move(tmp.params);
					// Просим повторить авторизацию ещё раз
					result = stath_t::RETRY;
				}
			} break;
			// Сообщаем, что авторизация прошла успешно
			case 100:
			case 101:
			case 200:
			case 201:
			case 202:
			case 203:
			case 204:
			case 205:
			case 206: result = stath_t::GOOD; break;
		}
	// Если активирован режим обработки запроса клиента
	} else if(this->mode == mode_t::REQUEST) {
		// Если авторизация требуется
		if(this->auth->getType() != auth_t::type_t::NONE){
			// Получаем параметры авторизации
			auto it = this->headers.find("authorization");
			// Если параметры авторизации найдены
			if(it != this->headers.end()){
				// Создаём объект авторизации для клиента
				auth_t auth(this->fmk, this->log, true);
				// Получаем тип алгоритма Дайджест
				const auto & digest = this->auth->getDigest();
				// Устанавливаем тип авторизации
				auth.setType(this->auth->getType(), digest.algorithm);
				// Устанавливаем заголовок HTTP в параметры авторизации
				auth.setHeader(it->second);
				// Выполняем проверку авторизации
				if(this->auth->check(auth))
					// Запоминаем, что авторизация пройдена
					result = http_t::stath_t::GOOD;
			}
		// Сообщаем, что авторизация прошла успешно
		} else result = http_t::stath_t::GOOD;
	}
	// Выводим результат
	return result;
}
/**
 * update Метод обновления входящих данных
 */
void awh::Http::update() noexcept {
	// Если тело сообщения получено
	if(!this->body.empty()){
		// Отключаем сжатие ответа с сервера
		this->compress = compress_t::NONE;
		// Выполняем поиск расширений
		auto it = this->headers.find("x-awh-encryption");
		// Если заголовок найден
		if((this->crypt = (it != this->headers.end()))){
			// Определяем размер шифрования
			switch(stoi(it->second)){
				// Если шифрование произведено 128 битным ключём
				case 128: this->hash->setAES(hash_t::aes_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->hash->setAES(hash_t::aes_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->hash->setAES(hash_t::aes_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & res = this->hash->decrypt(this->body.data(), this->body.size());
			// Если данные расшифрованны, заменяем тело данных
			if(!res.empty()) this->body.assign(res.begin(), res.end());
		}
		// Проверяем пришли ли сжатые данные
		it = this->headers.find("content-encoding");
		// Если данные пришли сжатые
		if(it != this->headers.end()){
			// Если данные пришли сжатые методом GZip
			if(it->second.compare("gzip") == 0){
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::GZIP;
				// Выполняем декомпрессию данных
				const auto & body = this->hash->decompressGzip(this->body.data(), this->body.size());
				// Заменяем полученное тело
				if(!body.empty()) this->body.assign(body.begin(), body.end());
			// Если данные пришли сжатые методом Deflate
			} else if(it->second.compare("deflate") == 0) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::DEFLATE;
				// Получаем данные тела в бинарном виде
				vector <char> buffer(this->body.begin(), this->body.end());
				// Добавляем хвост в полученные данные
				this->hash->setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & body = this->hash->decompress(buffer.data(), buffer.size());
				// Заменяем полученное тело
				if(!body.empty()) this->body.assign(body.begin(), body.end());
			}
		}
	}
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Http::clear() noexcept {
	// Выполняем очистку тела HTTP запроса
	this->body.clear();
	// Выполняем сброс параметров запроса
	this->url.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->headers.clear();
	// Выполняем сброс параметров чанка
	this->chunk = chunk_t();
	// Выполняем сброс параметров запроса
	this->query = query_t();
	// Обнуляем флаг проверки авторизации
	this->failAuth = false;
	// Выполняем сброс режим работы модуля
	this->mode = mode_t::NONE;
	// Выполняем сброс стейта авторизации
	this->stath = stath_t::EMPTY;
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::QUERY;
}
/**
 * parse Метод парсинга сырых данных
 * @param buffer буфер данных для обработки
 * @param size   размер буфера данных
 */
void awh::Http::parse(const char * buffer, const size_t size) noexcept {
	// Если рукопожатие не выполнено
	if((this->state != state_t::HANDSHAKE) && (this->state != state_t::BROKEN)){
		// Если мы собираем заголовки или стартовый запрос
		if((this->state == state_t::HEADERS) || (this->state == state_t::QUERY)){
			// Позиция найденного разделителя
			size_t pos = string::npos;
			// Получаем данные запроса
			const string http(buffer, size);
			// Если все заголовки получены
			if((pos = http.find("\r\n\r\n")) != string::npos){
				// Выполняем сброс всех предыдущих данных
				this->clear();
				// Выполняем чтение полученного буфера
				readHeader(http.data(), pos, [this](string data) noexcept {
					// Определяем статус режима работы
					switch((u_short) this->state){
						// Если - это режим ожидания получения запроса
						case (u_short) state_t::QUERY: {
							// Выполняем поиск протокола
							size_t offset = data.find("HTTP/");
							// Если протокол найден
							if(offset != string::npos){
								// Если протокол находится в начае запроса
								if(offset == 0){
									// Выполняем поиск разделителя
									offset = data.find(" ", 5);
									// Если разделитель найден
									if(offset != string::npos){
										// Получаем версию протокол запроса
										this->query.ver = stod(data.substr(5, offset - 5));
										// Удаляем лишние символы
										data.erase(data.begin(), data.begin() + (offset + 1));
										// Выполняем поиск второго разделителя
										offset = data.find(" ");
										// Если пробел получен
										if(offset != string::npos){
											// Устанавливаем режим работы (получение ответа от сервера)
											this->mode = mode_t::RESPONSE;
											// Выполняем смену стейта
											this->state = state_t::HEADERS;
											// Получаем сообщение сервера
											this->query.message = data.substr(offset + 1);
											// Получаем код ответа
											this->query.code = stoi(data.substr(0, offset));
											// Выходим из условия
											break;
										}
									}
								// Если протокол находится в конце запроса
								} else {
									// Получаем версию протокол запроса
									this->query.ver = stod(data.substr(offset + 5));
									// Удаляем лишние символы
									data.erase(data.begin() + (offset - 1), data.end());
									// Выполняем поиск разделителя
									offset = data.find(" ");
									// Если пробел получен
									if(offset != string::npos){
										// Устанавливаем режим работы (получение запроса от клиента)
										this->mode = mode_t::REQUEST;
										// Выполняем смену стейта
										this->state = state_t::HEADERS;
										// Получаем URI запроса
										this->query.uri = data.substr(offset + 1);
										// Получаем метод запроса
										const string & method = this->fmk->toLower(data.substr(0, offset));
										// Если метод определён как GET
										if(method.compare("get") == 0) this->query.method = method_t::GET;
										// Если метод определён как PUT
										else if(method.compare("put") == 0) this->query.method = method_t::PUT;
										// Если метод определён как POST
										else if(method.compare("post") == 0) this->query.method = method_t::POST;
										// Если метод определён как HEAD
										else if(method.compare("head") == 0) this->query.method = method_t::HEAD;
										// Если метод определён как PATCH
										else if(method.compare("patch") == 0) this->query.method = method_t::PATCH;
										// Если метод определён как TRACE
										else if(method.compare("trace") == 0) this->query.method = method_t::TRACE;
										// Если метод определён как DELETE
										else if(method.compare("delete") == 0) this->query.method = method_t::DELETE;
										// Если метод определён как OPTIONS
										else if(method.compare("options") == 0) this->query.method = method_t::OPTIONS;
										// Если метод определён как CONNECT
										else if(method.compare("connect") == 0) this->query.method = method_t::CONNECT;
										// Выходим из условия
										break;
									}
								}
							}
							// Сообщаем, что произошла ошибка
							this->stath = stath_t::FAULT;
						} break;
						// Если - это режим получения заголовков
						case (u_short) state_t::HEADERS: {
							// Выполняем поиск пробела
							size_t pos = data.find(":");
							// Если разделитель найден
							if(pos != string::npos){
								// Получаем ключ заголовка
								const string & key = this->fmk->trim(data.substr(0, pos));
								// Получаем значение заголовка
								const string & val = this->fmk->trim(data.substr(pos + 1));
								// Добавляем заголовок в список заголовков
								if(!key.empty() && !val.empty())
									// Добавляем заголовок в список
									this->headers.emplace(this->fmk->toLower(key), val);
							}
						} break;
					}
				});
				// Получаем размер тела
				auto it = this->headers.find("content-length");
				// Если размер запроса передан
				if(it != this->headers.end()){
					// Устанавливаем стейт поиска тела запроса
					this->state = state_t::BODY;
					// Продолжаем работу
					goto end;
				// Если тело приходит
				} else {
					// Получаем размер тела
					it = this->headers.find("transfer-encoding");
					// Если размер запроса передан
					if(it != this->headers.end()){
						// Если нужно получать размер тела чанками
						if(it->second.find("chunked") != string::npos){
							// Устанавливаем стейт поиска тела запроса
							this->state = state_t::BODY;
							// Продолжаем работу
							goto end;
						}
					}
				}
				// Тело в запросе не передано
				this->state = state_t::GOOD;
				// Устанавливаем метку завершения работы
				end:
				// Продолжаем работу
				this->parse(http.data() + (pos + 4), http.size() - (pos + 4));
			}
		// Если мы собираем тело запроса или завершаем работу
		} else {
			// Определяем статус режима работы
			switch((u_short) this->state){
				// Если - все данные запроса собраны
				case (u_short) state_t::GOOD: {
					// Выполняем проверку авторизации
					this->stath = this->checkAuth();
					// Если ключ соответствует
					if(this->stath == stath_t::GOOD){
					// if((this->stath == stath_t::GOOD) && this->checkKeyWebSocket() && this->checkUpgrade() && this->checkVerWebSocket()){
						// Выполняем обновление входящих параметров
						this->update();
						// Устанавливаем стейт рукопожатия
						this->state = state_t::HANDSHAKE;
					// Поменяем данные как бракованные
					} else this->state = state_t::BROKEN;
				} break;
				// Если - это режим получения тела
				case (u_short) state_t::BODY: {
					// Если размер данных получен
					if(size > 0){
						// Выполняем поиск размера тела сообщения
						auto it = this->headers.find("content-length");
						// Если размер тела сообщения получен
						if(it != this->headers.end()){
							// Получаем максимальный размер тела сообщения
							const size_t max = stoull(it->second);
							// Если размер тела не получен
							if(max == 0){
								// Заполняем собранные данные тела
								this->chunk.data.assign(buffer, buffer + size);
								// Если функция обратного вызова установлена
								if(this->chunkingFn != nullptr)
									// Выводим функцию обратного вызова
									this->chunkingFn(this->chunk.data, this);
							// Если размер установлен конкретный
							} else {
								// Получаем актуальный размер тела
								size_t actual = (max - this->body.size());
								// Фиксируем актуальный размер тела
								actual = (size > actual ? actual : size);
								// Увеличиваем общий размер полученных данных
								this->chunk.size += actual;
								// Заполняем собранные данные тела
								this->chunk.data.assign(buffer, buffer + actual);
								// Если функция обратного вызова установлена
								if(this->chunkingFn != nullptr)
									// Выводим функцию обратного вызова
									this->chunkingFn(this->chunk.data, this);
								// Если тело сообщения полностью собранно
								if(max == this->chunk.size){
									// Очищаем собранные данные
									this->chunk.clear();
									// Тело в запросе не передано
									this->state = state_t::GOOD;
									// Продолжаем работу
									this->parse(buffer, size);
									// Выходим из функции
									break;
								}
							}
						// Если получение данных ведётся чанками
						} else {
							// Формируем данные тела
							string body(buffer, size);
							// Если размер чанка не получен
							if(this->chunk.size == 0){
								// Ищем размер чанка
								const size_t pos = body.find("\r\n");
								// Если размер чанка найден
								if(pos != string::npos){
									// Получаем размер чанка
									this->chunk.size = this->fmk->hexToDec(body.substr(0, pos));
									// Если это последний чанк
									if(this->chunk.size == 0){
										// Очищаем собранные данные
										this->chunk.clear();
										// Тело в запросе не передано
										this->state = state_t::GOOD;
										// Продолжаем работу
										this->parse(buffer, size);
										// Выходим из функции
										break;
									}
									// Удаляем полученные данные
									body.erase(body.begin(), body.begin() + (pos + 2));
								}
							}
							// Если данные ещё есть, продолжаем собирать
							if(!body.empty()){
								// Получаем актуальный размер тела
								size_t actual = (this->chunk.size - this->chunk.data.size());
								// Фиксируем актуальный размер тела
								actual = (body.size() > actual ? actual : body.size());
								// Собираем тело запроса
								this->chunk.data.insert(this->chunk.data.end(), body.begin(), body.begin() + actual);
								// Если весь чанк собран
								if(this->chunk.size == this->chunk.data.size()){
									// Если функция обратного вызова установлена
									if(this->chunkingFn != nullptr)
										// Выводим функцию обратного вызова
										this->chunkingFn(this->chunk.data, this);
									// Очищаем собранные данные
									this->chunk.clear();
									// Удаляем полученные данные
									body.erase(body.begin(), body.begin() + actual);
									// Если тело запроса ещё не всё прочитано
									if(!body.empty()) this->parse(body.data(), body.size());
								}
							}
						}
					}
				} break;
			}
		}
	}
}
/**
 * addBody Метод добавления буфера тела данных запроса
 * @param buffer буфер данных тела запроса
 * @param size   размер буфера данных
 */
void awh::Http::addBody(const char * buffer, const size_t size) noexcept {
	// Если даныне переданы
	if((buffer != nullptr) && (size > 0))
		// Добавляем данные буфера в буфер тела
		this->body.insert(this->body.end(), buffer, buffer + size);
}
/**
 * addHeader Метод добавления заголовка
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::Http::addHeader(const string & key, const string & val) noexcept {
	// Если даныне заголовка переданы
	if(!key.empty() && !val.empty())
		// Выполняем добавление передаваемого заголовка
		this->headers.emplace(key, val);
}
/**
 * getBody Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Http::getBody() const noexcept {
	// Выводим данные тела
	return this->body;
}
/**
 * chunkBody Метод чтения чанка тела запроса
 * @return текущий чанк запроса
 */
const vector <char> awh::Http::chunkBody() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные тела ещё существуют
	if(!this->body.empty()){
		// Если нужно тело выводить в виде чанков
		if(this->chunking){
			// Тело чанка запроса
			string chunk = "";
			// Если тело сообщения больше размера чанка
			if(this->body.size() >= this->chunkSize){
				// Получаем размер чанка
				chunk = this->fmk->decToHex(this->chunkSize);
				// Добавляем разделитель
				chunk.append("\r\n");
				// Формируем тело чанка
				chunk.insert(chunk.end(), this->body.begin(), this->body.begin() + this->chunkSize);
				// Добавляем конец запроса
				chunk.append("\r\n");
				// Удаляем полученные данные в теле сообщения
				this->body.erase(this->body.begin(), this->body.begin() + this->chunkSize);
			// Если тело сообщения полностью убирается в размер чанка
			} else {
				// Получаем размер чанка
				chunk = this->fmk->decToHex(this->body.size());
				// Добавляем разделитель
				chunk.append("\r\n");
				// Формируем тело чанка
				chunk.insert(chunk.end(), this->body.begin(), this->body.end());
				// Добавляем конец запроса
				chunk.append("\r\n0\r\n\r\n");
				// Очищаем данные тела
				this->body.clear();
			}
			// Формируем результат
			result.assign(chunk.begin(), chunk.end());
			// Освобождаем память
			string().swap(chunk);
		// Выводим данные тела как есть
		} else {
			// Если тело сообщения больше размера чанка
			if(this->body.size() >= this->chunkSize){
				// Получаем нужный нам размер данных
				result.assign(this->body.begin(), this->body.begin() + this->chunkSize);
				// Удаляем полученные данные в теле сообщения
				this->body.erase(this->body.begin(), this->body.begin() + this->chunkSize);
			// Если тело сообщения полностью убирается в размер чанка
			} else {
				// Получаем нужный нам размер данных
				result.assign(this->body.begin(), this->body.end());
				// Очищаем данные тела
				this->body.clear();
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * getHeader Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
const string & awh::Http::getHeader(const string & key) const noexcept {
	// Результат работы функции
	static string result = "";
	// Если ключ заголовка передан
	if(!key.empty()){
		// Выполняем поиск ключа заголовка
		auto it = this->headers.find(key);
		// Если заголовок найден
		if(it != this->headers.end()) return it->second;
	}
	// Выводим результат
	return result;
}
/**
 * getHeaders Метод получения списка заголовков
 * @return список существующих заголовков
 */
const unordered_multimap <string, string> & awh::Http::getHeaders() const noexcept {
	// Выводим список доступных заголовков
	return this->headers;
}
/**
 * readHeader Функция чтения заголовков из буфера данных
 * @param buffer   буфер данных для чтения
 * @param size     размер буфера данных для чтения
 * @param callback функция обратного вызова
 */
void awh::Http::readHeader(const char * buffer, const size_t size, function <void (string)> callback) noexcept {
	// Если файл прочитан удачно
	if((buffer != nullptr) && (size > 0)){
		// Значение текущей и предыдущей буквы
		char letter = 0, old = 0;
		// Смещение в буфере и длина полученной строки
		size_t offset = 0, length = 0;
		// Переходим по всему буферу
		for(size_t i = 0; i < size; i++){
			// Получаем значение текущей буквы
			letter = buffer[i];
			// Если текущая буква является переносом строк
			if((i > 0) && ((letter == '\n') || (i == (size - 1)))){
				// Если предыдущая буква была возвратом каретки, уменьшаем длину строки
				length = ((old == '\r' ? i - 1 : i) - offset);
				// Если символ является последним и он не является переносом строки
				if((i == (size - 1)) && (letter != '\n')) length++;
				// Если длина слова получена, выводим полученную строку
				callback(string(buffer + offset, length));
				// Выполняем смещение
				offset = (i + 1);
			}
			// Запоминаем предыдущую букву
			old = letter;
		}
	}
}
/**
 * getAuth Метод проверки статуса авторизации
 * @return результат проверки
 */
awh::Http::stath_t awh::Http::getAuth() const noexcept {
	// Выводим результат проверки
	return this->stath;
}
/**
 * getCompress Метод получения метода сжатия
 * @return метод сжатия сообщений
 */
awh::Http::compress_t awh::Http::getCompress() const noexcept {
	// Выводим метод сжатия сообщений
	return this->compress;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::Http::setCompress(const compress_t compress) noexcept {
	// Устанавливаем метод сжатия сообщений
	this->compress = compress;
}
/**
 * getUrl Метод извлечения параметров запроса
 * @return установленные параметры запроса
 */
const awh::uri_t::url_t & awh::Http::getUrl() const noexcept {
	// Выводим параметры запроса
	return this->url;
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Http::isEnd() const noexcept {
	// Выводрим результат проверки
	return ((this->state == state_t::HANDSHAKE) || (this->state == state_t::BROKEN));
}
/**
 * isCrypt Метод проверки на зашифрованные данные
 * @return флаг проверки на зашифрованные данные
 */
bool awh::Http::isCrypt() const noexcept {
	// Выводим результат проверки
	return this->crypt;
}
/**
 * isAlive Метод проверки на постоянное подключение
 * @return результат проверки
 */
bool awh::Http::isAlive() const noexcept {
	// Результат работы функции
	bool result = true;
	// Выполняем поиск заголовка подключения
	auto it = this->headers.find("connection");
	// Если заголовок найден
	if(it != this->headers.end())
		// Выполняем проверку является ли соединение закрытым
		result = (it->second.compare("close") != 0);
	// Выводим результат
	return result;
}
/**
 * isHandshake Метод получения флага рукопожатия
 * @return флаг получения рукопожатия
 */
bool awh::Http::isHandshake() const noexcept {
	// Выводрим результат проверки рукопожатия
	return (this->state == state_t::HANDSHAKE);
}
/**
 * isHeader Метод проверки существования заголовка
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Http::isHeader(const string & key) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если ключ передан
	if(!key.empty()){
		// Выполняем поиск ключа заголовка
		auto it = this->headers.find(key);
		// Выполняем проверку существования заголовка
		result = (it != this->headers.end());
	}
	// Выводим результат
	return result;
}
/**
 * getQuery Метод получения объекта запроса сервера
 * @return объект запроса сервера
 */
const awh::Http::query_t & awh::Http::getQuery() const noexcept {
	// Выводим объект запроса сервера
	return this->query;
}
/**
 * setQuery Метод добавления объекта запроса клиента
 * @param query объект запроса клиента
 */
void awh::Http::setQuery(const query_t & query) noexcept {
	// Устанавливаем объект запроса клиента
	this->query = query;
}
/**
 * getMessage Метод получения HTTP сообщения
 * @param code код сообщения для получение
 * @return     соответствующее коду HTTP сообщение
 */
const string & awh::Http::getMessage(const u_short code) const noexcept {
	/**
	 * Подробнее: https://developer.mozilla.org/ru/docs/Web/HTTP/Status
	 */
	// Результат работы функции
	static string result = "";
	// Выполняем поиск кода сообщения
	auto it = this->messages.find(code);
	// Если код сообщения найден
	if(it != this->messages.end()) return it->second;
	// Выводим результат
	return result;
}
/**
 * reject Метод создания отрицательного ответа
 * @param code код ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::reject(const u_short code) const noexcept {
	// Получаем текст сообщения
	this->query.message = this->getMessage(code);
	// Если сообщение получено
	if(!this->query.message.empty()){
		// Если требуется ввод авторизационных данных
		if((code == 401) || (code == 407))
			// Добавляем заголовок закрытия подключения
			this->headers.emplace("Connection", "keep-alive");
		// Добавляем заголовок закрытия подключения
		else this->headers.emplace("Connection", "close");
		// Добавляем заголовок тип контента
		this->headers.emplace("Content-type", "text/html; charset=utf-8");
		// Если запрос должен содержать тело сообщения
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308)){
			// Если тело ответа не установлено, устанавливаем своё
			if(this->body.empty()){
				// Формируем тело ответа
				const string & body = this->fmk->format(
					"<html><head><title>%u %s</title></head>\n<body><h2>%u %s</h2></body></html>",
					code, this->query.message.c_str(), code, this->query.message.c_str()
				);
				// Добавляем тело сообщения
				this->body.assign(body.begin(), body.end());
			}
			// Добавляем заголовок тела сообщения
			this->headers.emplace("Content-Length", to_string(this->body.size()));
		}
		// Выводим результат
		return this->response(code);
	}
	// Выводим результат
	return vector <char> ();
}
/**
 * response Метод создания ответа
 * @param code код ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::response(const u_short code) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Получаем текст сообщения
	this->query.message = this->getMessage(code);
	// Если сообщение получено
	if(!this->query.message.empty()){
		/**
		 * Типы основных заголовков
		 */
		bool available[7] = {
			false, // Connection
			false, // Content-Type
			false, // Content-Length
			false, // Content-Encoding
			false, // Transfer-Encoding
			false, // X-AWH-Encryption
			false  // WWW-Authenticate
		};
		// Размер тела сообщения
		size_t length = 0;
		// Устанавливаем код ответа
		this->query.code = code;
		// Данные REST ответа
		string response = this->fmk->format("HTTP/%.1f %u %s\r\n", this->query.ver, code, this->query.message.c_str());
		// Добавляем заголовок даты в ответ
		response.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
		// Переходим по всему списку заголовков
		for(auto & header : this->headers){
			// Получаем анализируемый заголовок
			const string & head = this->fmk->toLower(header.first);
			// Выполняем перебор всех обязательных заголовков
			for(u_short i = 0; i < 7; i++){
				// Если заголовок уже найден пропускаем его
				if(available[i]) continue;
				// Выполняем првоерку заголовка
				switch(i){
					case 0: available[i] = (head.compare("connection") == 0);        break;
					case 1: available[i] = (head.compare("content-type") == 0);      break;
					case 3: available[i] = (head.compare("content-encoding") == 0);  break;
					case 4: available[i] = (head.compare("transfer-encoding") == 0); break;
					case 5: available[i] = (head.compare("x-awh-encryption") == 0);  break;
					case 6: available[i] = (head.compare("www-authenticate") == 0);  break;
					case 2: {
						// Запоминаем, что мы нашли заголовок размера тела
						available[i] = (head.compare("content-length") == 0);
						// Устанавливаем размер тела сообщения
						if(available[i]) length = stoull(header.second);
					} break;
				}
			}
			// Если заголовок не является запрещённым
			if(!available[2] && !available[4] && !available[5])
				// Добавляем заголовок в ответ
				response.append(this->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
		}
		// Устанавливаем Connection если не передан
		if(!available[0])
			// Добавляем заголовок в ответ
			response.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
		// Устанавливаем Content-Type если не передан
		if(!available[1])
			// Добавляем заголовок в ответ
			response.append(this->fmk->format("Content-Type: %s\r\n", HTTP_HEADER_CONTENTTYPE));
		// Добавляем название сервера в ответ
		response.append(this->fmk->format("Server: %s\r\n", this->servName.c_str()));
		// Добавляем название рабочей системы в ответ
		response.append(this->fmk->format("X-Powered-By: %s/%s\r\n", this->servId.c_str(), this->servVer.c_str()));
		// Если заголовок авторизации не передан
		if(!available[6]){
			// Получаем параметры авторизации
			const string & auth = this->auth->header();
			// Если данные авторизации получены
			if(!auth.empty()) response.append(auth);
		}
		// Если запрос должен содержать тело и тело ответа существует
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308) && !this->body.empty()){
			// Проверяем нужно ли передать тело разбив на чанки
			this->chunking = (!available[2] || ((length > 0) && (length != this->body.size())));
			// Если нужно производить шифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash->encrypt(this->body.data(), this->body.size());
				// Если данные зашифрованы, заменяем тело данных
				if(!res.empty()){
					// Заменяем тело данных
					this->body.assign(res.begin(), res.end());
					// Устанавливаем X-AWH-Encryption
					response.append(this->fmk->format("X-AWH-Encryption: %u\r\n", (u_short) this->hash->getAES()));
				}
			}
			// Определяем метод сжатия тела сообщения
			switch((u_short) this->compress){
				// Если нужно сжать тело методом GZIP
				case (u_short) compress_t::GZIP: {
					// Выполняем сжатие тела сообщения
					const auto & gzip = this->hash->compressGzip(this->body.data(), this->body.size());
					// Если данные сжаты, заменяем тело данных
					if(!gzip.empty()){
						// Заменяем тело данных
						this->body.assign(gzip.begin(), gzip.end());
						// Заменяем размер тела данных
						if(!this->chunking) length = this->body.size();
						// Устанавливаем Content-Encoding если не передан
						if(!available[3]) response.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
					}
				} break;
				// Если нужно сжать тело методом DEFLATE
				case (u_short) compress_t::DEFLATE: {
					// Выполняем сжатие тела сообщения
					auto deflate = this->hash->compress(this->body.data(), this->body.size());
					// Удаляем хвост в полученных данных
					this->hash->rmTail(deflate);
					// Если данные сжаты, заменяем тело данных
					if(!deflate.empty()){
						// Заменяем тело данных
						this->body.assign(deflate.begin(), deflate.end());
						// Заменяем размер тела данных
						if(!this->chunking) length = this->body.size();
						// Устанавливаем Content-Encoding если не передан
						if(!available[3]) response.append(this->fmk->format("Content-Encoding: %s\r\n", "deflate"));
					}
				} break;
			}
			// Если данные необходимо разбивать на чанки
			if(this->chunking)
				// Устанавливаем заголовок Transfer-Encoding
				response.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
			// Устанавливаем размер передаваемого тела Content-Length
			else response.append(this->fmk->format("Content-Length: %zu\r\n", length));
		// Очищаем тела сообщения
		} else this->body.clear();
		// Устанавливаем завершающий разделитель
		response.append("\r\n");
		// Формируем результат ответа
		result.assign(response.begin(), response.end());
	}
	// Выводим результат
	return result;
}
/**
 * proxy Метод создания запроса для авторизации на прокси-сервере
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::proxy(const uri_t::url_t & url) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем хост сервера
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост сервера получен
		if(!host.empty() && (url.port > 0)){
			// Добавляем поддержку постоянного подключения
			this->headers.emplace("Connection", "keep-alive");
			// Добавляем поддержку постоянного подключения для прокси-сервера
			this->headers.emplace("Proxy-Connection", "keep-alive");
			// Получаем параметры авторизации
			const string & auth = this->auth->header(true);
			// Если данные авторизации получены
			if(!auth.empty()) this->headers.emplace("Proxy-Authorization", auth);
			// Формируем URI запроса
			this->query.uri = this->fmk->format("%s:%u", host.c_str(), url.port);
			// Выполняем создание запроса
			return this->request(url, method_t::CONNECT);
		}
	}
	// Выводим результат
	return result;
}
/**
 * request Метод создания запроса
 * @param url    объект параметров REST запроса
 * @param method метод REST запроса
 * @return       буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::request(const uri_t::url_t & url, const method_t method) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем путь HTTP запроса
		const string & path = this->uri->joinPath(url.path);
		// Получаем параметры запроса
		const string & params = this->uri->joinParams(url.params);
		// Получаем хост запроса
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост получен
		if(!host.empty() && !path.empty()){
			/**
			 * Типы основных заголовков
			 */
			bool available[11] = {
				false, // Host
				false, // Accept
				false, // Origin
				false, // User-Agent
				false, // Connection
				false, // Content-Length
				false, // Accept-Language
				false, // Content-Encoding
				false, // Transfer-Encoding
				false, // X-AWH-Encryption
				false  // Proxy-Authorization
			};
			// Запоминаем параметры запроса
			this->url = url;
			// Размер тела сообщения
			size_t length = 0;
			// Данные REST запроса
			string request = "";
			// Устанавливаем параметры REST запроса
			const_cast <auth_t *> (this->auth)->setUri(this->uri->createUrl(url));
			// Если метод не CONNECT или URI не установлен
			if((method != method_t::CONNECT) || this->query.uri.empty())
				// Формируем HTTP запрос
				this->query.uri = this->fmk->format("%s%s", path.c_str(), (!params.empty() ? params.c_str() : ""));
			// Определяем метод запроса
			switch((u_short) method){
				// Если метод запроса указан как GET
				case (u_short) method_t::GET:
					// Формируем GET запрос
					request = this->fmk->format("GET %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как PUT
				case (u_short) method_t::PUT:
					// Формируем PUT запрос
					request = this->fmk->format("PUT %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как POST
				case (u_short) method_t::POST:
					// Формируем POST запрос
					request = this->fmk->format("POST %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как HEAD
				case (u_short) method_t::HEAD:
					// Формируем HEAD запрос
					request = this->fmk->format("HEAD %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как PATCH
				case (u_short) method_t::PATCH:
					// Формируем PATCH запрос
					request = this->fmk->format("PATCH %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как TRACE
				case (u_short) method_t::TRACE:
					// Формируем TRACE запрос
					request = this->fmk->format("TRACE %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как DELETE
				case (u_short) method_t::DELETE:
					// Формируем DELETE запрос
					request = this->fmk->format("DELETE %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как OPTIONS
				case (u_short) method_t::OPTIONS:
					// Формируем OPTIONS запрос
					request = this->fmk->format("OPTIONS %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как CONNECT
				case (u_short) method_t::CONNECT:
					// Формируем CONNECT запрос
					request = this->fmk->format("CONNECT %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
			}
			// Запоминаем метод запроса
			this->query.method = method;
			// Добавляем заголовок даты в запрос
			request.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
			// Переходим по всему списку заголовков
			for(auto & header : this->headers){
				// Получаем анализируемый заголовок
				const string & head = this->fmk->toLower(header.first);
				// Выполняем перебор всех обязательных заголовков
				for(u_short i = 0; i < 11; i++){
					// Если заголовок уже найден пропускаем его
					if(available[i]) continue;
					// Выполняем првоерку заголовка
					switch(i){
						case 0:  available[i] = (head.compare("host") == 0);                  break;
						case 1:  available[i] = (head.compare("accept") == 0);                break;
						case 2:  available[i] = (head.compare("origin") == 0);                break;
						case 3:  available[i] = (head.compare("user-agent") == 0);            break;
						case 4:  available[i] = (head.compare("connection") == 0);            break;
						case 6:  available[i] = (head.compare("accept-language") == 0);       break;
						case 7:  available[i] = (head.compare("content-encoding") == 0);      break;
						case 8:  available[i] = (head.compare("transfer-encoding") == 0);     break;
						case 9:  available[i] = (head.compare("x-awh-encryption") == 0);      break;
						case 10: available[i] = ((head.compare("authorization") == 0) ||
							                     (head.compare("proxy-authorization") == 0)); break;
						case 5: {
							// Запоминаем, что мы нашли заголовок размера тела
							available[i] = (head.compare("content-length") == 0);
							// Устанавливаем размер тела сообщения
							if(available[i]) length = stoull(header.second);
						} break;
					}
				}
				// Если заголовок не является запрещённым
				if(!available[5] && !available[8] && !available[9])
					// Добавляем заголовок в запрос
					request.append(this->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
			}
			// Устанавливаем Host если не передан
			if(!available[0])
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Host: %s\r\n", (!url.domain.empty() ? url.domain : url.ip).c_str()));
			// Устанавливаем Accept если не передан
			if(!available[1] && (method != method_t::CONNECT))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
			// Устанавливаем Origin если не передан
			if(!available[2])
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Origin: %s\r\n", this->uri->createOrigin(url).c_str()));
			// Устанавливаем Connection если не передан
			if(!available[4])
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
			// Устанавливаем Accept-Language если не передан
			if(!available[6] && (method != method_t::CONNECT))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
			// Если нужно произвести сжатие контента
			if((this->compress != compress_t::NONE) && (method != method_t::CONNECT))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept-Encoding: %s\r\n", HTTP_HEADER_ACCEPTENCODING));
			// Устанавливаем User-Agent если не передан
			if(!available[3]){
				// Если User-Agent установлен стандартный
				if(this->userAgent.compare(HTTP_HEADER_AGENT) == 0){
					// Название операционной системы
					const char * nameOs = nullptr;
					// Определяем название операционной системы
					switch((u_short) this->fmk->os()){
						// Если операционной системой является Unix
						case (u_short) fmk_t::os_t::UNIX: nameOs = "Unix"; break;
						// Если операционной системой является Linux
						case (u_short) fmk_t::os_t::LINUX: nameOs = "Linux"; break;
						// Если операционной системой является неизвестной
						case (u_short) fmk_t::os_t::NONE: nameOs = "Unknown"; break;
						// Если операционной системой является Windows
						case (u_short) fmk_t::os_t::WIND32:
						case (u_short) fmk_t::os_t::WIND64: nameOs = "Windows"; break;
						// Если операционной системой является MacOS X
						case (u_short) fmk_t::os_t::MACOSX: nameOs = "MacOS X"; break;
						// Если операционной системой является FreeBSD
						case (u_short) fmk_t::os_t::FREEBSD: nameOs = "FreeBSD"; break;
					}
					// Выполняем генерацию Юзер-агента клиента выполняющего HTTP запрос
					this->userAgent = this->fmk->format("Mozilla/5.0 (%s; %s) %s/%s", nameOs, this->servName.c_str(), this->servId.c_str(), this->servVer.c_str());
				}
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("User-Agent: %s\r\n", this->userAgent.c_str()));
			}
			// Если заголовок авторизации не передан
			if(!available[10]){
				// Получаем параметры авторизации
				const string & auth = this->auth->header();
				// Если данные авторизации получены
				if(!auth.empty()) request.append(auth);
			}
			// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
			if((method != method_t::GET) && (method != method_t::HEAD) && (method != method_t::TRACE) && !this->body.empty()){
				// Проверяем нужно ли передать тело разбив на чанки
				this->chunking = (!available[5] || ((length > 0) && (length != this->body.size())));
				// Если нужно производить шифрование
				if(this->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash->encrypt(this->body.data(), this->body.size());
					// Если данные зашифрованы, заменяем тело данных
					if(!res.empty()){
						// Заменяем тело данных
						this->body.assign(res.begin(), res.end());
						// Устанавливаем X-AWH-Encryption
						request.append(this->fmk->format("X-AWH-Encryption: %u\r\n", (u_short) this->hash->getAES()));
					}
				}
				// Определяем метод сжатия тела сообщения
				switch((u_short) this->compress){
					// Если нужно сжать тело методом GZIP
					case (u_short) compress_t::GZIP: {
						// Выполняем сжатие тела сообщения
						const auto & gzip = this->hash->compressGzip(this->body.data(), this->body.size());
						// Если данные сжаты, заменяем тело данных
						if(!gzip.empty()){
							// Заменяем тело данных
							this->body.assign(gzip.begin(), gzip.end());
							// Заменяем размер тела данных
							if(!this->chunking) length = this->body.size();
							// Устанавливаем Content-Encoding если не передан
							if(!available[7]) request.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
						}
					} break;
					// Если нужно сжать тело методом DEFLATE
					case (u_short) compress_t::DEFLATE: {
						// Выполняем сжатие тела сообщения
						auto deflate = this->hash->compress(this->body.data(), this->body.size());
						// Удаляем хвост в полученных данных
						this->hash->rmTail(deflate);
						// Если данные сжаты, заменяем тело данных
						if(!deflate.empty()){
							// Заменяем тело данных
							this->body.assign(deflate.begin(), deflate.end());
							// Заменяем размер тела данных
							if(!this->chunking) length = this->body.size();
							// Устанавливаем Content-Encoding если не передан
							if(!available[7]) request.append(this->fmk->format("Content-Encoding: %s\r\n", "deflate"));
						}
					} break;
				}
				// Если данные необходимо разбивать на чанки
				if(this->chunking)
					// Устанавливаем заголовок Transfer-Encoding
					request.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
				// Устанавливаем размер передаваемого тела Content-Length
				else request.append(this->fmk->format("Content-Length: %zu\r\n", length));
			// Очищаем тела сообщения
			} else this->body.clear();
			// Устанавливаем завершающий разделитель
			request.append("\r\n");
			// Формируем результат запроса
			result.assign(request.begin(), request.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Http::setChunkingFn(function <void (const vector <char> &, const Http *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->chunkingFn = callback;
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Http::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	if(size >= 100) this->chunkSize = size;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Http::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->userAgent = userAgent;
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Http::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Если идентификатор сервиса передан, устанавливаем
	if(!id.empty()) this->servId = id;
	// Если версия сервиса передана
	if(!ver.empty()) this->servVer = ver;
	// Если название сервиса передано, устанавливаем
	if(!name.empty()) this->servName = name;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::Http::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->auth->setRealm(realm);
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::Http::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->auth->setOpaque(opaque);
}
/**
 * clearUsers Метод очистки списка пользователей
 */
void awh::Http::clearUsers() noexcept {
	// Выполняем очистку списка пользователей
	this->auth->clearUsers();
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Http::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty()){
		// Устанавливаем логин пользователя
		this->auth->setLogin(login);
		// Устанавливаем пароль пользователя
		this->auth->setPassword(password);
	}
}
/**
 * setUsers Метод добавления списка пользователей
 * @param users список пользователей для добавления
 */
void awh::Http::setUsers(const unordered_map <string, string> & users) noexcept {
	// Если данные пользователей переданы
	if(!users.empty()) this->auth->setUsers(users);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Http::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash->setAES(aes);
	// Устанавливаем соль шифрования
	this->hash->setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash->setPassword(pass);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Http::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	if(this->auth != nullptr) this->auth->setType(type, algorithm);
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::Http::Http(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		this->uri = uri;
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->log);
		// Создаём объект для работы с сжатым контентом
		this->hash = new hash_t(this->fmk, this->log);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Http Деструктор
 */
awh::Http::~Http() noexcept {
	// Удаляем объект работы с сжатым контентом
	if(this->hash != nullptr) delete this->hash;
	// Удаляем объект авторизации
	if(this->auth != nullptr) delete this->auth;
}
