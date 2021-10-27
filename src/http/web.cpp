/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
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
	if((buffer != nullptr) && (size > 0) && (this->state != state_t::END)){
		// Если мы собираем тело полезной нагрузки
		if(this->state == state_t::BODY){
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
						this->state = state_t::END;
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
					// Запоминаем количество обработанных байт
					result++;
					// Определяем стейт чанка
					switch((uint8_t) this->chunk.state){
						// Если мы ожидаем получения размера тела чанка
						case (uint8_t) cstate_t::SIZE: {
							// Если мы получили возврат каретки
							if(buffer[i] == '\r'){
								// Меняем стейт чанка
								this->chunk.state = cstate_t::ENDSIZE;
								// Получаем размер чанка
								this->chunk.size = this->fmk->hexToDec(string(
									this->chunk.data.begin(),
									this->chunk.data.end()
								));
								// Устанавливаем смещение
								offset = (i + 1);
								// Выполняем сброс тела данных
								this->chunk.data.clear();
							// Выполняем сборку 16-го размера чанка
							} else this->chunk.data.push_back(buffer[i]);
						} break;
						// Если мы ожидаем получение окончания сбора размера тела чанка
						case (uint8_t) cstate_t::ENDSIZE: {
							// Увеличиваем смещение
							offset = (i + 1);
							// Если мы получили перевод строки
							if(buffer[i] == '\n'){
								// Если размер получен 0-й значит мы завершили сбор данных
								if(this->chunk.size == 0)
									// Меняем стейт чанка
									this->chunk.state = cstate_t::STOPBODY;
								// Если данные собраны не полностью
								else {
									// Если количества байт достаточно для сбора тела чанка
									if((size - offset) >= this->chunk.size){
										// Меняем стейт чанка
										this->chunk.state = cstate_t::STOPBODY;
										// Определяем конец буфера
										const size_t end = (offset + this->chunk.size);
										// Собираем тело чанка
										this->chunk.data.insert(this->chunk.data.end(), buffer + offset, buffer + end);
										// Выполняем смещение итератора
										i = (end - 1);
										// Увеличиваем смещение
										offset = end;
									// Если количества байт не достаточно для сбора тела
									} else {
										// Меняем стейт чанка
										this->chunk.state = cstate_t::BODY;
										// Собираем тело чанка
										this->chunk.data.insert(this->chunk.data.end(), buffer + offset, buffer + size);
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
						case (uint8_t) cstate_t::BODY: {
							// Определяем количество необходимых байт
							const size_t rem = (this->chunk.size - this->chunk.data.size());
							// Если количества байт достаточно для сбора тела чанка
							if(size >= rem){
								// Меняем стейт чанка
								this->chunk.state = cstate_t::STOPBODY;
								// Собираем тело чанка
								this->chunk.data.insert(this->chunk.data.end(), buffer, buffer + rem);
								// Выполняем смещение итератора
								i = (rem - 1);
								// Увеличиваем смещение
								offset = rem;
							// Если количества байт не достаточно для сбора тела
							} else {
								// Собираем тело чанка
								this->chunk.data.insert(this->chunk.data.end(), buffer, buffer + size);
								// Выходим из функции
								return result;
							}
						} break;
						// Если мы ожидаем перевод строки после сбора данных тела чанка
						case (uint8_t) cstate_t::STOPBODY: {
							// Увеличиваем смещение
							offset = (i + 1);
							// Если мы получили возврат каретки
							if(buffer[i] == '\r')
								// Меняем стейт чанка
								this->chunk.state = cstate_t::ENDBODY;
							// Если символ отличается, значит ошибка
							else {
								// Устанавливаем символ ошибки
								error = 'r';
								// Выходим
								goto Stop;
							}
						} break;
						// Если мы ожидаем получение окончания сбора данных тела чанка
						case (uint8_t) cstate_t::ENDBODY: {
							// Увеличиваем смещение
							offset = (i + 1);
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
				this->chunk.clear();
				// Тело в запросе не передано
				this->state = state_t::END;
				// Сообщаем, что переданное тело содержит ошибки
				if(error != '\0') this->log->print("body chunk contains errors, [\\%c] is expected", log_t::flag_t::WARNING, error);
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
	if((buffer != nullptr) && (size > 0) && (this->state != state_t::END)){
		// Если мы собираем заголовки или стартовый запрос
		if((this->state == state_t::HEADERS) || (this->state == state_t::QUERY)){
			// Определяем статус режима работы
			switch((uint8_t) this->state){
				// Если - это режим ожидания получения запроса
				case (uint8_t) state_t::QUERY: this->sep = ' '; break;
				// Если - это режим получения заголовков
				case (uint8_t) state_t::HEADERS: this->sep = ':'; break;
			}
			/**
			 * Выполняем парсинг заголовков запроса
			 */
			this->prepare(buffer, size, [&result, this](const char * buffer, const size_t size, const size_t bytes, const bool stop) noexcept {
				// Если все данные получены
				if(stop){
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
					this->state = state_t::END;
					// Устанавливаем метку завершения работы
					end:
					// Устанавливаем смещение в буфере
					result = bytes;
					// Выходим из функции
					return;
				// Если необходимо  получить оставшиеся данные
				} else {
					// Определяем статус режима работы
					switch((uint8_t) this->state){
						// Если - это режим ожидания получения запроса
						case (uint8_t) state_t::QUERY: {
							// Определяем тип HTTP модуля
							switch((uint8_t) this->httpType){
								// Если мы работаем с клиентом
								case (uint8_t) hid_t::CLIENT: {
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
										// Устанавливаем разделитель
										this->sep = ':';
										// Выполняем сброс размера тела
										this->bodySize = -1;
										// Устанавливаем стейт ожидания получения заголовков
										this->state = state_t::HEADERS;
										// Получаем версию протокол запроса
										this->query.ver = stof(string(buffer + 5, this->pos[0] - 5));
										// Получаем сообщение ответа
										this->query.message.assign(buffer + (this->pos[1] + 1), size - (this->pos[1] + 1));
										// Получаем код ответа
										this->query.code = stoi(string(buffer + (this->pos[0] + 1), this->pos[1] - (this->pos[0] + 1)));
									// Если данные пришли неправильные
									} else {
										// Выполняем очистку всех ранее полученных данных
										this->clear();
										// Сообщаем, что переданное тело содержит ошибки
										this->log->print("%s", log_t::flag_t::WARNING, "broken response server");
									}
								} break;
								// Если мы работаем с сервером
								case (uint8_t) hid_t::SERVER: {
									// Создаём буфер для проверки
									char temp[5];
									// Копируем полученную строку
									strncpy(temp, buffer + (this->pos[1] + 1), 4);
									// Устанавливаем конец строки
									temp[4] = '\0';
									// Если мы получили ответ от сервера
									if(strcmp(temp, "HTTP") == 0){
										// Выполняем очистку всех ранее полученных данных
										this->clear();
										// Устанавливаем разделитель
										this->sep = ':';
										// Выполняем сброс размера тела
										this->bodySize = -1;
										// Выполняем смену стейта
										this->state = state_t::HEADERS;
										// Получаем метод запроса
										const string & method = this->fmk->toLower(string(buffer, this->pos[0]));
										// Получаем версию протокол запроса
										this->query.ver = stof(string(buffer + (this->pos[1] + 6), size - (this->pos[1] + 6)));
										// Получаем URI запроса
										this->query.uri.assign(buffer + (this->pos[0] + 1), this->pos[1] - (this->pos[0] + 1));
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
									// Если данные пришли неправильные
									} else {
										// Выполняем очистку всех ранее полученных данных
										this->clear();
										// Сообщаем, что переданное тело содержит ошибки
										this->log->print("%s", log_t::flag_t::WARNING, "broken request client");
									}
								} break;
							}
						} break;
						// Если - это режим получения заголовков
						case (uint8_t) state_t::HEADERS: {
							// Получаем ключ заголовка
							const string & key = string(buffer, this->pos[0]);
							// Получаем значение заголовка
							const string & val = string(buffer + (this->pos[0] + 1), size - (this->pos[0] + 1));
							// Добавляем заголовок в список заголовков
							if(!key.empty() && !val.empty())
								// Добавляем заголовок в список
								this->headers.emplace(this->fmk->toLower(key), this->fmk->trim(val));
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
		memset(this->pos, -1, sizeof(this->pos));
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
			if((this->sep != '\0') && (letter == this->sep) && (count < 2)){
				// Устанавливаем позицию найденного разделителя
				this->pos[count] = (i - offset);
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
				if(this->sep != '\0'){
					// Выполняем сброс количество найденных сепараторов
					count = 0;
					// Выполняем сброс массива сепараторов
					memset(this->pos, -1, sizeof(this->pos));
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
 * parse Метод выполнения парсинга HTTP буфера данных
 * @param buffer буфер данных для парсинга
 * @param size   размер буфера данных для парсинга
 */
void awh::Web::parse(const char * buffer, const size_t size) noexcept {
	// Если данные переданы или обработка полностью выполнена
	if((buffer != nullptr) && (size > 0) && (this->state != state_t::END)){
		// Определяем текущий стейт
		switch((uint8_t) this->state){
			// Если установлен стейт чтения параметров запроса/ответа
			case (uint8_t) state_t::QUERY:
			// Если установлен стейт чтения заголовков
			case (uint8_t) state_t::HEADERS: {
				// Выполняем чтение заголовков
				const size_t bytes = this->readHeaders(buffer, size);
				// Если требуется продолжить извлечение данных тела сообщения
				if((bytes < size) && (this->state == state_t::BODY))
					// Выполняем извлечение данных тела сообщения
					this->readPayload(buffer + bytes, size - bytes);
			} break;
			// Если установлен стейт чтения полезной нагрузки
			case (uint8_t) state_t::BODY: this->readPayload(buffer, size); break;
		}
	}
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Web::clear() noexcept {
	// Выполняем очистку тела HTTP запроса
	this->body.clear();
	// Выполняем сброс параметров чанка
	this->chunk.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->headers.clear();
	// Выполняем сброс параметров запроса
	this->query = query_t();
}
/**
 * reset Метод сброса стейтов парсера
 */
void awh::Web::reset() noexcept {
	// Устанавливаем разделитель
	this->sep = '\0';
	// Выполняем сброс размера тела
	this->bodySize = -1;
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::QUERY;
	// Выполняем сброс массива сепараторов
	memset(this->pos, -1, sizeof(this->pos));
}
/**
 * getQuery Метод получения объекта запроса сервера
 * @return объект запроса сервера
 */
const awh::Web::query_t & awh::Web::getQuery() const noexcept {
	// Выводим объект запроса сервера
	return this->query;
}
/**
 * setQuery Метод добавления объекта запроса клиента
 * @param query объект запроса клиента
 */
void awh::Web::setQuery(const query_t & query) noexcept {
	// Устанавливаем объект запроса клиента
	this->query = query;
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Web::isEnd() const noexcept {
	// Выводрим результат проверки
	return (this->state == state_t::END);
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
		// Выполняем поиск ключа заголовка
		auto it = this->headers.find(this->fmk->toLower(key));
		// Выполняем проверку существования заголовка
		result = (it != this->headers.end());
	}
	// Выводим результат
	return result;
}
/**
 * clearBody Метод очистки данных тела
 */
void awh::Web::clearBody() noexcept {
	// Выполняем очистку данных тела
	this->body.clear();
}
/**
 * clearHeaders Метод очистки списка заголовков
 */
void awh::Web::clearHeaders() noexcept {
	// Выполняем очистку заголовков
	this->headers.clear();
}
/**
 * setBody Метод установки данных тела
 * @param buffer буфер тела для установки
 */
void awh::Web::setBody(const vector <char> & buffer) noexcept {
	// Выполняем установку данных тела
	this->body.assign(buffer.begin(), buffer.end());
}
/**
 * addBody Метод добавления буфера тела данных запроса
 * @param buffer буфер данных тела запроса
 * @param size   размер буфера данных
 */
void awh::Web::addBody(const char * buffer, const size_t size) noexcept {
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
void awh::Web::addHeader(const string & key, const string & val) noexcept {
	// Если даныне заголовка переданы
	if(!key.empty() && !val.empty())
		// Выполняем добавление передаваемого заголовка
		this->headers.emplace(key, val);
}
/**
 * getBody Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Web::getBody() const noexcept {
	// Выводим данные тела
	return this->body;
}
/**
 * getHeader Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
const string & awh::Web::getHeader(const string & key) const noexcept {
	// Если ключ заголовка передан
	if(!key.empty()){
		// Выполняем поиск ключа заголовка
		auto it = this->headers.find(key);
		// Если заголовок найден
		if(it != this->headers.end()) return it->second;
	}
	// Выводим результат
	return this->header;
}
/**
 * getHeaders Метод получения списка заголовков
 * @return список существующих заголовков
 */
const unordered_multimap <string, string> & awh::Web::getHeaders() const noexcept {
	// Выводим список доступных заголовков
	return this->headers;
}
/**
 * init Метод инициализации модуля
 * @param hid тип используемого HTTP модуля
 */
void awh::Web::init(const hid_t hid) noexcept {
	// Устанавливаем тип используемого HTTP мдуля
	this->httpType = hid;
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::Web::setChunkingFn(void * ctx, function <void (const vector <char> &, const Web *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию обратного вызова
	this->chunkingFn = callback;
}
