/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core/http.hpp>

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
				case 128: this->hash.setAES(hash_t::aes_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->hash.setAES(hash_t::aes_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->hash.setAES(hash_t::aes_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & res = this->hash.decrypt(this->body.data(), this->body.size());
			// Если данные расшифрованны, заменяем тело данных
			if(!res.empty()) this->body.assign(res.begin(), res.end());
		}
		// Проверяем пришли ли сжатые данные
		it = this->headers.find("content-encoding");
		// Если данные пришли сжатые
		if(it != this->headers.end()){
			// Если данные пришли сжатые методом Brotli
			if(it->second.compare("br") == 0){
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::BROTLI;
				// Выполняем декомпрессию данных
				const auto & body = this->hash.decompressBrotli(this->body.data(), this->body.size());
				// Заменяем полученное тело
				if(!body.empty()) this->body.assign(body.begin(), body.end());
			// Если данные пришли сжатые методом GZip
			} else if(it->second.compare("gzip") == 0) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::GZIP;
				// Выполняем декомпрессию данных
				const auto & body = this->hash.decompressGzip(this->body.data(), this->body.size());
				// Заменяем полученное тело
				if(!body.empty()) this->body.assign(body.begin(), body.end());
			// Если данные пришли сжатые методом Deflate
			} else if(it->second.compare("deflate") == 0) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::DEFLATE;
				// Получаем данные тела в бинарном виде
				vector <char> buffer(this->body.begin(), this->body.end());
				// Добавляем хвост в полученные данные
				this->hash.setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & body = this->hash.decompress(buffer.data(), buffer.size());
				// Заменяем полученное тело
				if(!body.empty()) this->body.assign(body.begin(), body.end());
			}
		}
	}
}
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
 * clear Метод очистки собранных данных
 */
void awh::Http::clear() noexcept {
	// Выполняем очистку тела HTTP запроса
	this->body.clear();
	// Выполняем сброс чёрного списка HTTP заголовков
	this->black.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->headers.clear();
	// Выполняем сброс параметров чанка
	this->chunk = chunk_t();
	// Выполняем сброс параметров запроса
	this->query = query_t();
}
/**
 * reset Метод сброса параметров запроса
 */
void awh::Http::reset() noexcept {
	// Выполняем сброс параметров запроса
	this->url.clear();
	// Выполняем сброс размера тела
	this->bodySize = -1;
	// Обнуляем флаг проверки авторизации
	this->failAuth = false;
	// Выполняем сброс стейта авторизации
	this->stath = stath_t::EMPTY;
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::QUERY;
}
/**
 * addBlack Метод добавления заголовка в чёрный список
 * @param key ключ заголовка
 */
void awh::Http::addBlack(const string & key) noexcept {
	// Если ключ заголовка передан, добавляем в список
	if(!key.empty()) this->black.emplace(this->fmk->toLower(key));
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
				// Выполняем сброс размера тела
				this->bodySize = -1;
				// Выполняем чтение полученного буфера
				readHeader(http.data(), pos, [this](string data) noexcept {
					// Определяем статус режима работы
					switch((uint8_t) this->state){
						// Если - это режим ожидания получения запроса
						case (uint8_t) state_t::QUERY: {
							// Выполняем поиск протокола
							size_t offset = data.find("HTTP/");
							// Если протокол найден
							if(offset != string::npos){
								// Выполняем очистку всех ранее полученных данных
								this->clear();
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
										// Если метод определён как DELETE
										else if(method.compare("delete") == 0) this->query.method = method_t::DEL;
										// Если метод определён как PATCH
										else if(method.compare("patch") == 0) this->query.method = method_t::PATCH;
										// Если метод определён как TRACE
										else if(method.compare("trace") == 0) this->query.method = method_t::TRACE;
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
						case (uint8_t) state_t::HEADERS: {
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
					// Запоминаем размер тела сообщения
					this->bodySize = stoull(it->second);
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
			switch((uint8_t) this->state){
				// Если - все данные запроса собраны
				case (uint8_t) state_t::GOOD: {
					// Выполняем проверку авторизации
					this->stath = this->checkAuth();
					// Если ключ соответствует
					if(this->stath == stath_t::GOOD)
						// Выполняем обновление входящих параметров
						this->update();
					// Поменяем данные как бракованные
					else this->state = state_t::BROKEN;
				} break;
				// Если - это режим получения тела
				case (uint8_t) state_t::BODY: {
					// Если размер данных получен
					if(size > 0){
						// Если размер тела сообщения получен
						if(this->bodySize > -1){
							// Если размер тела не получен
							if(this->bodySize == 0){
								// Заполняем собранные данные тела
								this->chunk.data.assign(buffer, buffer + size);
								// Если функция обратного вызова установлена
								if(this->chunkingFn != nullptr)
									// Выводим функцию обратного вызова
									this->chunkingFn(this->chunk.data, this, this->ctx.at(0));
							// Если размер установлен конкретный
							} else {
								// Получаем актуальный размер тела
								size_t actual = (this->bodySize - this->body.size());
								// Фиксируем актуальный размер тела
								actual = (size > actual ? actual : size);
								// Увеличиваем общий размер полученных данных
								this->chunk.size += actual;
								// Заполняем собранные данные тела
								this->chunk.data.assign(buffer, buffer + actual);
								// Если функция обратного вызова установлена
								if(this->chunkingFn != nullptr)
									// Выводим функцию обратного вызова
									this->chunkingFn(this->chunk.data, this, this->ctx.at(0));
								// Если тело сообщения полностью собранно
								if(this->bodySize == this->chunk.size){
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
							// Символ буфера в котором допущена ошибка
							char error = '\0';
							// Переходим по всему буферу данных
							for(size_t i = 0; i < size; i ++){
								// Определяем стейт чанка
								switch((uint8_t) this->chunk.state){
									// Если мы ожидаем получения размера тела чанка
									case (uint8_t) cstate_t::SIZE: {
										// Если мы получили возврат каретки
										if(buffer[i] == '\r'){
											// Меняем стейт чанка
											this->chunk.state = cstate_t::ENDSIZE;
											// Получаем размер чанка
											this->chunk.size = this->fmk->hexToDec(this->chunk.hexSize);
										// Собираем размер тела чанка
										} else this->chunk.hexSize.append(1, buffer[i]);
									} break;
									// Если мы ожидаем получение окончания сбора размера тела чанка
									case (uint8_t) cstate_t::ENDSIZE: {
										// Если мы получили перевод строки
										if(buffer[i] == '\n'){
											// Очищаем размер чанка в 16-м виде
											this->chunk.hexSize.clear();
											// Если размер получен 0-й значит мы завершили сбор данных
											if(this->chunk.size == 0)
												// Меняем стейт чанка
												this->chunk.state = cstate_t::STOPBODY;
											// Меняем стейт чанка
											else this->chunk.state = cstate_t::BODY;
										// Если символ отличается, значит ошибка
										} else {
											// Устанавливаем символ ошибки
											error = '\n';
											// Выходим
											goto Stop;
										}
									} break;
									// Если мы ожидаем сбора тела чанка
									case (uint8_t) cstate_t::BODY: {
										// Собираем тело чанка
										this->chunk.data.push_back(buffer[i]);
										// Если все данные чанка собраны
										if(this->chunk.data.size() == this->chunk.size)
											// Меняем стейт чанка
											this->chunk.state = cstate_t::STOPBODY;
									} break;
									// Если мы ожидаем перевод строки после сбора данных тела чанка
									case (uint8_t) cstate_t::STOPBODY: {
										// Если мы получили возврат каретки
										if(buffer[i] == '\r')
											// Меняем стейт чанка
											this->chunk.state = cstate_t::ENDBODY;
										// Если символ отличается, значит ошибка
										else {
											// Устанавливаем символ ошибки
											error = '\r';
											// Выходим
											goto Stop;
										}
									} break;
									// Если мы ожидаем получение окончания сбора данных тела чанка
									case (uint8_t) cstate_t::ENDBODY: {
										// Если мы получили перевод строки
										if(buffer[i] == '\n'){
											// Если размер получен 0-й значит мы завершили сбор данных
											if(this->chunk.size == 0) goto Stop;
											// Если функция обратного вызова установлена
											else if(this->chunkingFn != nullptr)
												// Выводим функцию обратного вызова
												this->chunkingFn(this->chunk.data, this, this->ctx.at(0));
											// Выполняем очистку чанка
											this->chunk.clear();
										// Если символ отличается, значит ошибка
										} else {
											// Устанавливаем символ ошибки
											error = '\n';
											// Выходим
											goto Stop;
										}
									} break;
								}
							}
							// Выходим из функции
							return;
							// Устанавливаем метку выхода
							Stop:
							// Выполняем очистку чанка
							this->chunk.clear();
							// Тело в запросе не передано
							this->state = state_t::GOOD;
							// Сообщаем, что переданное тело содержит ошибки
							if(error != '\0') this->log->print("body chunk contains errors, [\\%с] is expected", log_t::flag_t::WARNING, error);
							// Продолжаем работу
							this->parse(buffer, size);
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
	return (
		(this->state == state_t::GOOD) ||
		(this->state == state_t::BROKEN) ||
		(this->state == state_t::HANDSHAKE)
	);
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
 * isHandshake Метод проверки рукопожатия
 * @return проверка рукопожатия
 */
bool awh::Http::isHandshake() noexcept {
	// Выполняем проверку на удачное рукопожатие
	return (this->state == state_t::HANDSHAKE);
}
/**
 * isBlack Метод проверки существования заголовка в чёрный списоке
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Http::isBlack(const string & key) const noexcept {
	// Выводим результат проверки
	return (this->black.count(this->fmk->toLower(key)) > 0);
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
		auto it = this->headers.find(this->fmk->toLower(key));
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
 * proxy Метод создания запроса для авторизации на прокси-сервере
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::proxy(const uri_t::url_t & url) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем хост сервера
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост сервера получен
		if(!host.empty() && (url.port > 0)){
			// Добавляем в чёрный список заголовок Accept
			this->addBlack("Accept");
			// Добавляем в чёрный список заголовок Accept-Language
			this->addBlack("Accept-Language");
			// Добавляем в чёрный список заголовок Accept-Encoding
			this->addBlack("Accept-Encoding");
			// Добавляем поддержку постоянного подключения
			this->addHeader("Connection", "keep-alive");
			// Добавляем поддержку постоянного подключения для прокси-сервера
			this->addHeader("Proxy-Connection", "keep-alive");
			// Получаем параметры авторизации
			const string & auth = this->authCli.getHeader(true);
			// Если данные авторизации получены
			if(!auth.empty()) this->addHeader("Proxy-Authorization", auth);
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
 * reject Метод создания отрицательного ответа
 * @param code код ответа
 * @param mess сообщение ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::reject(const u_short code, const string & mess) const noexcept {
	// Получаем текст сообщения
	this->query.message = (!mess.empty() ? mess : this->getMessage(code));
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
		return this->response(code, mess);
	}
	// Выводим результат
	return vector <char> ();
}
/**
 * response Метод создания ответа
 * @param code код ответа
 * @param mess сообщение ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::response(const u_short code, const string & mess) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Получаем текст сообщения
	this->query.message = (!mess.empty() ? mess : this->getMessage(code));
	// Если сообщение получено
	if(!this->query.message.empty()){
		/**
		 * Типы основных заголовков
		 */
		bool available[8] = {
			false, // Connection
			false, // Content-Type
			false, // Content-Length
			false, // Content-Encoding
			false, // Transfer-Encoding
			false, // X-AWH-Encryption
			false, // WWW-Authenticate
			false  // Proxy-Authenticate
		};
		// Размер тела сообщения
		size_t length = 0;
		// Устанавливаем код ответа
		this->query.code = code;
		// Данные REST ответа
		string response = this->fmk->format("HTTP/%.1f %u %s\r\n", this->query.ver, code, this->query.message.c_str());
		// Если заголовок не запрещён
		if(!this->isBlack("Date"))
			// Добавляем заголовок даты в ответ
			response.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
		// Переходим по всему списку заголовков
		for(auto & header : this->headers){
			// Получаем анализируемый заголовок
			const string & head = this->fmk->toLower(header.first);
			// Флаг разрешающий вывода заголовка
			bool allow = !this->isBlack(head);
			// Выполняем перебор всех обязательных заголовков
			for(u_short i = 0; i < 8; i++){
				// Если заголовок уже найден пропускаем его
				if(available[i]) continue;
				// Выполняем првоерку заголовка
				switch(i){
					case 0: available[i] = (head.compare("connection") == 0);         break;
					case 1: available[i] = (head.compare("content-type") == 0);       break;
					case 3: available[i] = (head.compare("content-encoding") == 0);   break;
					case 4: available[i] = (head.compare("transfer-encoding") == 0);  break;
					case 5: available[i] = (head.compare("x-awh-encryption") == 0);   break;
					case 6: available[i] = (head.compare("www-authenticate") == 0);   break;
					case 7: available[i] = (head.compare("proxy-authenticate") == 0); break;
					case 2: {
						// Запоминаем, что мы нашли заголовок размера тела
						available[i] = (head.compare("content-length") == 0);
						// Устанавливаем размер тела сообщения
						if(available[i]) length = stoull(header.second);
					} break;
				}
				// Если заголовок разрешён для вывода
				if(allow){
					// Выполняем првоерку заголовка
					switch(i){
						case 2:
						case 4:
						case 5: allow = !available[i]; break;
					}
				}
			}
			// Если заголовок не является запрещённым, добавляем заголовок в ответ
			if(allow) response.append(this->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
		}
		// Устанавливаем Connection если не передан
		if(!available[0] && !this->isBlack("Connection"))
			// Добавляем заголовок в ответ
			response.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
		// Устанавливаем Content-Type если не передан
		if(!available[1] && !this->isBlack("Content-Type"))
			// Добавляем заголовок в ответ
			response.append(this->fmk->format("Content-Type: %s\r\n", HTTP_HEADER_CONTENTTYPE));
		// Если заголовок не запрещён
		if(!this->isBlack("Server"))
			// Добавляем название сервера в ответ
			response.append(this->fmk->format("Server: %s\r\n", this->servName.c_str()));
		// Если заголовок не запрещён
		if(!this->isBlack("X-Powered-By"))
			// Добавляем название рабочей системы в ответ
			response.append(this->fmk->format("X-Powered-By: %s/%s\r\n", this->servId.c_str(), this->servVer.c_str()));
		// Если заголовок авторизации не передан
		if(((code == 401) && !available[6]) || ((code == 407) && !available[7])){
			// Получаем параметры авторизации
			const string & auth = this->authSrv.getHeader(true);
			// Если параметры авторизации получены
			if(!auth.empty()){
				// Определяем код авторизации
				switch(code){
					// Если авторизация производится для Web-Сервера
					case 401: {
						// Если заголовок не запрещён
						if(!this->isBlack("WWW-Authenticate"))
							// Добавляем параметры авторизации
							response.append(this->fmk->format("WWW-Authenticate: %s\r\n", auth.c_str()));
					} break;
					// Если авторизация производится для Прокси-Сервера
					case 407: {
						// Если заголовок не запрещён
						if(!this->isBlack("Proxy-Authenticate"))
							// Добавляем параметры авторизации
							response.append(this->fmk->format("Proxy-Authenticate: %s\r\n", auth.c_str()));
					} break;
				}
			}
		}
		// Если запрос должен содержать тело и тело ответа существует
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308) && !this->body.empty()){
			// Проверяем нужно ли передать тело разбив на чанки
			this->chunking = (!available[2] || ((length > 0) && (length != this->body.size())));
			// Если нужно производить шифрование
			if(this->crypt && !this->isBlack("X-AWH-Encryption")){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash.encrypt(this->body.data(), this->body.size());
				// Если данные зашифрованы, заменяем тело данных
				if(!res.empty()){
					// Заменяем тело данных
					this->body.assign(res.begin(), res.end());
					// Устанавливаем X-AWH-Encryption
					response.append(this->fmk->format("X-AWH-Encryption: %u\r\n", (u_short) this->hash.getAES()));
				}
			}
			// Если заголовок не запрещён
			if(!this->isBlack("Content-Encoding")){
				// Определяем метод сжатия тела сообщения
				switch((uint8_t) this->compress){
					// Если нужно сжать тело методом BROTLI
					case (uint8_t) compress_t::BROTLI: {
						// Выполняем сжатие тела сообщения
						const auto & brotli = this->hash.compressBrotli(this->body.data(), this->body.size());
						// Если данные сжаты, заменяем тело данных
						if(!brotli.empty()){
							// Заменяем тело данных
							this->body.assign(brotli.begin(), brotli.end());
							// Заменяем размер тела данных
							if(!this->chunking) length = this->body.size();
							// Устанавливаем Content-Encoding если не передан
							if(!available[3]) response.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
						}
					} break;
					// Если нужно сжать тело методом GZIP
					case (uint8_t) compress_t::GZIP: {
						// Выполняем сжатие тела сообщения
						const auto & gzip = this->hash.compressGzip(this->body.data(), this->body.size());
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
					case (uint8_t) compress_t::DEFLATE: {
						// Выполняем сжатие тела сообщения
						auto deflate = this->hash.compress(this->body.data(), this->body.size());
						// Удаляем хвост в полученных данных
						this->hash.rmTail(deflate);
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
			}
			// Если данные необходимо разбивать на чанки
			if(this->chunking && !this->isBlack("Transfer-Encoding"))
				// Устанавливаем заголовок Transfer-Encoding
				response.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
			// Если заголовок размера передаваемого тела, не запрещён
			else if(!this->isBlack("Content-Length"))
				// Устанавливаем размер передаваемого тела Content-Length
				response.append(this->fmk->format("Content-Length: %zu\r\n", length));
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
		// Получаем хост запроса
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост получен
		if(!host.empty()){
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
			this->authCli.setUri(this->uri->createUrl(url));
			// Если метод не CONNECT или URI не установлен
			if((method != method_t::CONNECT) || this->query.uri.empty())
				// Формируем HTTP запрос
				this->query.uri = this->uri->createQuery(url);
			// Определяем метод запроса
			switch((uint8_t) method){
				// Если метод запроса указан как GET
				case (uint8_t) method_t::GET:
					// Формируем GET запрос
					request = this->fmk->format("GET %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как PUT
				case (uint8_t) method_t::PUT:
					// Формируем PUT запрос
					request = this->fmk->format("PUT %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как POST
				case (uint8_t) method_t::POST:
					// Формируем POST запрос
					request = this->fmk->format("POST %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как HEAD
				case (uint8_t) method_t::HEAD:
					// Формируем HEAD запрос
					request = this->fmk->format("HEAD %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как PATCH
				case (uint8_t) method_t::PATCH:
					// Формируем PATCH запрос
					request = this->fmk->format("PATCH %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как TRACE
				case (uint8_t) method_t::TRACE:
					// Формируем TRACE запрос
					request = this->fmk->format("TRACE %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как DELETE
				case (uint8_t) method_t::DEL:
					// Формируем DELETE запрос
					request = this->fmk->format("DELETE %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как OPTIONS
				case (uint8_t) method_t::OPTIONS:
					// Формируем OPTIONS запрос
					request = this->fmk->format("OPTIONS %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
				// Если метод запроса указан как CONNECT
				case (uint8_t) method_t::CONNECT:
					// Формируем CONNECT запрос
					request = this->fmk->format("CONNECT %s HTTP/%.1f\r\n", this->query.uri.c_str(), this->query.ver);
				break;
			}
			// Запоминаем метод запроса
			this->query.method = method;
			// Если заголовок не запрещён
			if(!this->isBlack("Date"))
				// Добавляем заголовок даты в запрос
				request.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
			// Переходим по всему списку заголовков
			for(auto & header : this->headers){
				// Получаем анализируемый заголовок
				const string & head = this->fmk->toLower(header.first);
				// Флаг разрешающий вывода заголовка
				bool allow = !this->isBlack(head);
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
					// Если заголовок разрешён для вывода
					if(allow){
						// Выполняем првоерку заголовка
						switch(i){
							case 5:
							case 8:
							case 9: allow = !available[i]; break;
						}
					}
				}
				// Если заголовок не является запрещённым, добавляем заголовок в запрос
				if(allow) request.append(this->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
			}
			// Устанавливаем Host если не передан
			if(!available[0] && !this->isBlack("Host"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Host: %s\r\n", host.c_str()));
			// Устанавливаем Accept если не передан
			if(!available[1] && (method != method_t::CONNECT) && !this->isBlack("Accept"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
			// Устанавливаем Origin если не передан
			if(!available[2] && !this->isBlack("Origin"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Origin: %s\r\n", this->uri->createOrigin(url).c_str()));
			// Устанавливаем Connection если не передан
			if(!available[4] && !this->isBlack("Connection"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
			// Устанавливаем Accept-Language если не передан
			if(!available[6] && (method != method_t::CONNECT) && !this->isBlack("Accept-Language"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
			// Если нужно произвести сжатие контента
			if((this->compress != compress_t::NONE) && (method != method_t::CONNECT) && !this->isBlack("Accept-Encoding"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept-Encoding: %s\r\n", HTTP_HEADER_ACCEPTENCODING));
			// Устанавливаем User-Agent если не передан
			if(!available[3] && !this->isBlack("User-Agent")){
				// Если User-Agent установлен стандартный
				if(this->userAgent.compare(HTTP_HEADER_AGENT) == 0){
					// Название операционной системы
					const char * nameOs = nullptr;
					// Определяем название операционной системы
					switch((uint8_t) this->fmk->os()){
						// Если операционной системой является Unix
						case (uint8_t) fmk_t::os_t::UNIX: nameOs = "Unix"; break;
						// Если операционной системой является Linux
						case (uint8_t) fmk_t::os_t::LINUX: nameOs = "Linux"; break;
						// Если операционной системой является неизвестной
						case (uint8_t) fmk_t::os_t::NONE: nameOs = "Unknown"; break;
						// Если операционной системой является Windows
						case (uint8_t) fmk_t::os_t::WIND32:
						case (uint8_t) fmk_t::os_t::WIND64: nameOs = "Windows"; break;
						// Если операционной системой является MacOS X
						case (uint8_t) fmk_t::os_t::MACOSX: nameOs = "MacOS X"; break;
						// Если операционной системой является FreeBSD
						case (uint8_t) fmk_t::os_t::FREEBSD: nameOs = "FreeBSD"; break;
					}
					// Выполняем генерацию Юзер-агента клиента выполняющего HTTP запрос
					this->userAgent = this->fmk->format("Mozilla/5.0 (%s; %s) %s/%s", nameOs, this->servName.c_str(), this->servId.c_str(), this->servVer.c_str());
				}
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("User-Agent: %s\r\n", this->userAgent.c_str()));
			}
			// Если заголовок авторизации не передан
			if(!available[10] && !this->isBlack("Authorization")){
				// Получаем параметры авторизации
				const string & auth = this->authCli.getHeader();
				// Если данные авторизации получены
				if(!auth.empty()) request.append(auth);
			}
			// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
			if((method != method_t::GET) && (method != method_t::HEAD) && (method != method_t::TRACE) && !this->body.empty()){
				// Проверяем нужно ли передать тело разбив на чанки
				this->chunking = (!available[5] || ((length > 0) && (length != this->body.size())));
				// Если нужно производить шифрование
				if(this->crypt && !this->isBlack("X-AWH-Encryption")){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash.encrypt(this->body.data(), this->body.size());
					// Если данные зашифрованы, заменяем тело данных
					if(!res.empty()){
						// Заменяем тело данных
						this->body.assign(res.begin(), res.end());
						// Устанавливаем X-AWH-Encryption
						request.append(this->fmk->format("X-AWH-Encryption: %u\r\n", (u_short) this->hash.getAES()));
					}
				}
				// Если заголовок не запрещён
				if(!this->isBlack("Content-Encoding")){
					// Определяем метод сжатия тела сообщения
					switch((uint8_t) this->compress){
						// Если нужно сжать тело методом BROTLI
						case (uint8_t) compress_t::BROTLI: {
							// Выполняем сжатие тела сообщения
							const auto & brotli = this->hash.compressBrotli(this->body.data(), this->body.size());
							// Если данные сжаты, заменяем тело данных
							if(!brotli.empty()){
								// Заменяем тело данных
								this->body.assign(brotli.begin(), brotli.end());
								// Заменяем размер тела данных
								if(!this->chunking) length = this->body.size();
								// Устанавливаем Content-Encoding если не передан
								if(!available[7]) request.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
							}
						} break;
						// Если нужно сжать тело методом GZIP
						case (uint8_t) compress_t::GZIP: {
							// Выполняем сжатие тела сообщения
							const auto & gzip = this->hash.compressGzip(this->body.data(), this->body.size());
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
						case (uint8_t) compress_t::DEFLATE: {
							// Выполняем сжатие тела сообщения
							auto deflate = this->hash.compress(this->body.data(), this->body.size());
							// Удаляем хвост в полученных данных
							this->hash.rmTail(deflate);
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
				}
				// Если данные необходимо разбивать на чанки
				if(this->chunking && !this->isBlack("Transfer-Encoding"))
					// Устанавливаем заголовок Transfer-Encoding
					request.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
				// Если заголовок размера передаваемого тела, не запрещён
				else if(!this->isBlack("Content-Length"))
					// Устанавливаем размер передаваемого тела Content-Length
					request.append(this->fmk->format("Content-Length: %zu\r\n", length));
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
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::Http::setChunkingFn(void * ctx, function <void (const vector <char> &, const Http *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
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
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Http::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash.setAES(aes);
	// Устанавливаем соль шифрования
	this->hash.setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash.setPassword(pass);
}
