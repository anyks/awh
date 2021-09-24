/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <http/http.hpp>

/**
 * key Метод генерации ключа для WebSocket
 * @return сгенерированный ключ для WebSocket
 */
const string awh::Http::key() const noexcept {
	// Результат работы функции
	string result = "";
	// Выполняем перехват ошибки
	try {
		// Создаём контейнер
		string nonce = "";
		// Адаптер для работы с случайным распределением
		random_device rd;
		// Резервируем память
		nonce.reserve(16);
		// Формируем равномерное распределение целых чисел в выходном инклюзивно-эксклюзивном диапазоне
		uniform_int_distribution <u_short> dist(0, 255);
		// Формируем бинарный ключ из случайных значений
		for(size_t c = 0; c < 16; c++) nonce += static_cast <char> (dist(rd));
		// Выполняем создание ключа
		result = base64_t().encode(nonce);
	// Выполняем прехват ошибки
	} catch(const exception & error) {
		// Выводим в лог сообщение
		this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		// Выполняем повторно генерацию ключа
		result = this->key();
	}
	// Выводим результат
	return result;
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
 * generateHash Метод генерации хэша ключа
 * @return сгенерированный хэш ключа клиента
 */
const string awh::Http::generateHash() const noexcept {
	// Результат работы функции
	string result = "";
	// Если ключ клиента передан
	if(!this->keyWebSocket.empty()){
		// Создаем контекст
		SHA_CTX ctx;
		// Выполняем инициализацию контекста
		SHA1_Init(&ctx);
		// Массив полученных значений
		u_char digest[20];
		// Формируем магический ключ
		const string text = (this->keyWebSocket + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
		// Выполняем расчет суммы
		SHA1_Update(&ctx, text.c_str(), text.length());
		// Копируем полученные данные
		SHA1_Final(digest, &ctx);
		// Формируем ключ для клиента
		result = base64_t().encode(string((const char *) digest, 20));
	}
	// Выводим результат
	return result;
}
/**
 * checkUpgrade Метод получения флага переключения протокола
 * @return флага переключения протокола
 */
bool awh::Http::checkUpgrade() const noexcept {
	// Результат работы функции
	bool result = false;
	// Если список заголовков получен
	if(!this->headers.empty()){
		// Выполняем поиск заголовка смены протокола
		auto it = this->headers.find("upgrade");
		// Выполняем поиск заголовка с параметрами подключения
		auto jt = this->headers.find("connection");
		// Если заголовки расширений найдены
		if((it != this->headers.end()) && (jt != this->headers.end())){
			// Получаем значение заголовка Upgrade
			const string & upgrade = this->fmk->toLower(it->second);
			// Получаем значение заголовка Connection
			const string & connection = this->fmk->toLower(jt->second);
			// Если заголовки соответствуют
			result = ((upgrade.compare("websocket") == 0) && (connection.compare("upgrade") == 0));
		}
	}
	// Выводим результат
	return result;
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Http::clear() noexcept {
	// Выполняем сброс поддерживаемого сабпротокола
	this->sub.clear();
	// Выполняем сброс поддерживаемых сабпротоколов
	this->subs.clear();
	// Выполняем очистку тела HTTP запроса
	this->body.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->headers.clear();
	// Выполняем сброс ключа клиента
	this->keyWebSocket.clear();
	// Выполняем сброс параметров запроса
	this->query = query_t();
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::QUERY;
	// Выполняем сброс стейта авторизации
	this->stath = stath_t::EMPTY;
	// Выполняем сброс размера скользящего окна для клиента
	this->wbitClient = GZIP_MAX_WBITS;
	// Выполняем сброс размера скользящего окна для сервера
	this->wbitServer = GZIP_MAX_WBITS;
	// Выполняем сброс метода сжатия сообщений
	this->compress = compress_t::DEFLATE;
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
										this->query.method = data.substr(0, offset);
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
					this->stath = this->checkAuthenticate();
					// Если ключ соответствует
					if(this->stath == stath_t::GOOD){
					// if((this->stath == stath_t::GOOD) && this->checkKeyWebSocket() && this->checkUpgrade() && this->checkVerWebSocket()){
						// Выполняем обновление списка расширений
						this->updateExtensions();
						// Выполняем обновление списка сабпротоколов
						this->updateSubProtocol();
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
				// Если это конец файла, корректируем размер последнего байта
				if(length == 0) length = 1;
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
 * getWbitClient Метод получения размер скользящего окна для клиента
 * @return размер скользящего окна
 */
short awh::Http::getWbitClient() const noexcept {
	// Выводим размер скользящего окна
	return this->wbitClient;
}
/**
 * getWbitServer Метод получения размер скользящего окна для сервера
 * @return размер скользящего окна
 */
short awh::Http::getWbitServer() const noexcept {
	// Выводим размер скользящего окна
	return this->wbitServer;
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
 * getSub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::Http::getSub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->sub;
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
 * getReject Метод получения буфера ответа HTML реджекта
 * @return собранный HTML буфер
 */
vector <char> awh::Http::restReject() const noexcept {
	// Строка HTTP запроса
	const string & result = this->fmk->format(
		"HTTP/%.1f 400 Bad Request\r\n"
		"Date: %s\r\n"
		"Server: %s\r\n"
		"Connection: close\r\n"
		"Content-Length: %zu\r\n"
		"Sec-WebSocket-Version: %u\r\n"
		"Content-type: text/html; charset=utf-8\r\n"
		"X-Powered-By: %s/%s\r\n\r\n"
		"<html><head><title>400 Bad Request</title></head>\r\n<body><h2>400 Bad Request</h2></body></html>",
		this->query.ver, this->date().c_str(), AWH_NAME,
		95, WS_VERSION, AWH_SHORT_NAME, AWH_VERSION
	);
	// Выводим результат
	return vector <char> (result.begin(), result.end());
}
/**
 * getResponse Метод получения буфера HTML ответа
 * @return собранный HTML буфер
 */
vector <char> awh::Http::restResponse() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем генерацию хеша ключа
	const string & hash = this->generateHash();
	// Если хэш ключа сгенерирован
	if(!hash.empty()){
		// Расширения WebSocket и подпротоколы
		string extensions = "", sub = "";
		// Если необходимо активировать сжатие
		if(this->compress != compress_t::NONE){
			// Если метод компрессии выбран Deflate
			if(this->compress == compress_t::DEFLATE){
				// Формируем заголовок расширений
				extensions = "Sec-WebSocket-Extensions: permessage-deflate; server_no_context_takeover; client_max_window_bits";
				// Если требуется указать количество байт
				if(this->wbitServer > 0) extensions.append(this->fmk->format("; server_max_window_bits=%u", this->wbitServer));
			// Если метод компрессии выбран GZip
			} else if(this->compress == compress_t::GZIP)
				// Формируем заголовок расширений
				extensions = "Sec-WebSocket-Extensions: permessage-gzip; server_no_context_takeover";
			// Если данные должны быть зашифрованны
			if(this->crypt) extensions.append("; permessage-encrypt");
		// Если данные должны быть зашифрованны
		} else if(this->crypt) extensions = "Sec-WebSocket-Extensions: permessage-encrypt; server_no_context_takeover";
		// Ищем адрес сайта с которого выполняется запрос
		string origin = (this->headers.count("origin") > 0 ? this->headers.find("origin")->second : "");
		// Если Origin передан, формируем заголовок
		if(!origin.empty()){
			// Формируем заголовок
			origin.insert(0, "Origin: ");
			// Формируем сепаратор
			origin.append("\r\n");
		}
		// Если подпротокол выбран
		if(!this->sub.empty())
			// Формируем HTTP заголовок подпротокола
			sub = this->fmk->format("Sec-WebSocket-Protocol: %s\r\n", this->sub.c_str());
		// Строка HTTP запроса
		const string & response = this->fmk->format(
			"HTTP/%.1f 101 Switching Protocols\r\n"
			"Date: %s\r\n"
			"Server: %s\r\n"
			"Upgrade: websocket\r\n"
			"Connection: upgrade\r\n"
			"X-Powered-By: %s/%s\r\n"
			"%sSec-WebSocket-Accept: %s\r\n"
			"%s%s\r\n\r\n",
			this->query.ver, this->date().c_str(),
			AWH_NAME, AWH_SHORT_NAME, AWH_VERSION,
			origin.c_str(), hash.c_str(),
			sub.c_str(), extensions.c_str()
		);
		// Формируем результат
		result.assign(response.begin(), response.end());
	}
	// Выводим результат
	return result;
}
/**
 * getUnauthorized Метод получения буфера запроса HTML авторизации
 * @return собранный HTML буфер
 */
vector <char> awh::Http::restUnauthorized() const noexcept {
	// Строка HTTP запроса
	const string & result = this->fmk->format(
		"HTTP/%.1f 401 Unauthorized\r\n"
		"Date: %s\r\n"
		"Server: %s\r\n"
		"Connection: close\r\n"
		"Content-Length: %zu\r\n"
		"Content-type: text/html; charset=utf-8\r\n"
		"X-Powered-By: %s/%s\r\n%s\r\n\r\n"
		"<html><head><title>401 Authentication Required</title></head>\r\n"
		"<body><h2>401 Authentication Required</h2>\r\n"
		"<h3>Access to requested resource disallowed by administrator or you need valid username/password to use this resource</h3>\r\n"
		"</body></html>",
		this->query.ver, this->date().c_str(),
		AWH_NAME, 245, AWH_SHORT_NAME,
		AWH_VERSION, this->auth->header().c_str()
	);
	// Выводим результат
	return vector <char> (result.begin(), result.end());
}
/**
 * restRequest Метод получения буфера HTML запроса
 * @param compress метод сжатия сообщений
 * @param crypt    флаг зашифрованных данных
 * @return         собранный HTML буфер
 */
vector <char> awh::Http::restRequest(const compress_t compress, const bool crypt) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если URL объект передан
	if(this->url != nullptr){
		// Получаем путь HTTP запроса
		const string & path = this->uri->joinPath(this->url->path);
		// Получаем параметры запроса
		const string & params = this->uri->joinParams(this->url->params);
		// Получаем хост запроса
		const string & host = (!this->url->domain.empty() ? this->url->domain : this->url->ip);
		// Если хост получен
		if(!host.empty() && !path.empty()){
			// Список желаемых подпротоколов и расширения протокола
			string subs = "", extensions = "";
			// Получаем параметры авторизации
			const string & auth = this->auth->header();
			// Формируем HTTP запрос
			const string & query = this->fmk->format("%s%s", path.c_str(), (!params.empty() ? params.c_str() : ""));
			// Если подпротоколы существуют
			if(!this->subs.empty()){
				// Если количество подпротоколов больше 5-ти
				if(this->subs.size() > 5){
					// Переходим по всему списку подпротоколов
					for(auto & sub : this->subs){
						// Если подпротокол уже не пустой, добавляем разделитель
						if(!subs.empty()) subs.append(", ");
						// Добавляем в список желаемый подпротокол
						subs.append(sub);
					}
					// Формируем заголовок
					subs.insert(0, "Sec-WebSocket-Protocol: ");
					// Формируем сепаратор
					subs.append("\r\n");
				// Если подпротоколов слишком много
				} else {
					// Переходим по всему списку подпротоколов
					for(auto & sub : this->subs){
						// Формируем список подпротоколов
						subs.append(this->fmk->format("Sec-WebSocket-Protocol: %s\r\n", sub.c_str()));
					}
				}
			}
			// Генерируем ключ клиента
			this->keyWebSocket = this->key();
			// Получаем User-Agent
			const string userAgent = USER_AGENT;
			// Ищем адрес сайта с которого выполняется запрос
			string origin = (this->headers.count("origin") > 0 ? this->headers.find("origin")->second : this->uri->createOrigin(* this->url));
			// Если Origin передан, формируем заголовок
			if(!origin.empty()){
				// Формируем заголовок
				origin.insert(0, "Origin: ");
				// Формируем сепаратор
				origin.append("\r\n");
			}
			// Если метод компрессии указан
			if(compress != compress_t::NONE){
				// Если метод компрессии выбран Deflate
				if(compress == compress_t::DEFLATE)
					// Устанавливаем тип компрессии Deflate
					extensions = "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits";
				// Если метод компрессии выбран GZip
				else if(compress == compress_t::GZIP)
					// Устанавливаем тип компрессии GZip
					extensions = "Sec-WebSocket-Extensions: permessage-gzip";
				// Если шифровать данные не нужно
				if(!crypt) extensions.append("\r\n");
				// Если данные должны быть зашифрованны
				else extensions.append("; permessage-encrypt\r\n");
			// Если метод компрессии не указан но указан режим шифрования
			} else if(crypt) extensions = "Sec-WebSocket-Extensions: permessage-encrypt\r\n";
			// Строка HTTP запроса
			const string & request = this->fmk->format(
				"GET %s HTTP/%.1f\r\n"
				"Host: %s\r\n"
				"Date: %s\r\n"
				"%sUser-Agent: %s\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: websocket\r\n"
				"Sec-WebSocket-Version: %u\r\n"
				"Sec-WebSocket-Key: %s\r\n"
				"%s%s%s\r\n",
				query.c_str(), this->query.ver,
				host.c_str(), this->date().c_str(),
				origin.c_str(), userAgent.c_str(),
				WS_VERSION, this->keyWebSocket.c_str(),
				(!extensions.empty() ? extensions.c_str() : ""),
				(!subs.empty() ? subs.c_str() : ""),
				(!auth.empty() ? auth.c_str() : "")
			);
			// Формируем результат
			result.assign(request.begin(), request.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * response Метод создания ответа
 * @return буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::response() const noexcept {

}
/**
 * reject Метод создания отрицательного ответа
 * @param code код ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::reject(const u_short code) const noexcept {

}
/**
 * websocket Метод создания запроса для WebSocket
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::websocket(const uri_t::url_t & url) const noexcept {

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
	// Получаем путь HTTP запроса
	const string & path = this->uri->joinPath(url.path);
	// Получаем параметры запроса
	const string & params = this->uri->joinParams(url.params);
	// Получаем хост запроса
	const string & host = (!url.domain.empty() ? url.domain : url.ip);
	// Если хост получен
	if(!host.empty() && !path.empty()){
		// Данные REST запроса
		string request = "";
		// Размер тела сообщения
		size_t contentLength = 0;
		// Список существующих заголовков
		set <header_t> available;
		// Устанавливаем параметры REST запроса
		const_cast <auth_t *> (this->auth)->setUri(this->uri->createUrl(url));
		// Формируем HTTP запрос
		const string & query = this->fmk->format("%s%s", path.c_str(), (!params.empty() ? params.c_str() : ""));
		// Определяем метод запроса
		switch((u_short) method){
			// Если метод запроса указан как GET
			case (u_short) method_t::GET:
				// Формируем GET запрос
				request = this->fmk->format("GET %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как DEL
			case (u_short) method_t::DEL:
				// Формируем DEL запрос
				request = this->fmk->format("DEL %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как PUT
			case (u_short) method_t::PUT:
				// Формируем PUT запрос
				request = this->fmk->format("PUT %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как PUT
			case (u_short) method_t::POST:
				// Формируем POST запрос
				request = this->fmk->format("POST %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как HEAD
			case (u_short) method_t::HEAD:
				// Формируем HEAD запрос
				request = this->fmk->format("HEAD %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как PATCH
			case (u_short) method_t::PATCH:
				// Формируем PATCH запрос
				request = this->fmk->format("PATCH %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как TRACE
			case (u_short) method_t::TRACE:
				// Формируем TRACE запрос
				request = this->fmk->format("TRACE %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
			// Если метод запроса указан как OPTIONS
			case (u_short) method_t::OPTIONS:
				// Формируем OPTIONS запрос
				request = this->fmk->format("OPTIONS %s HTTP/%.1f\r\n", query.c_str(), this->query.ver);
			break;
		}
		// Добавляем заголовок даты в запрос
		request.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
		// Переходим по всему списку заголовков
		for(auto & header : this->headers){
			// Получаем анализируемый заголовок
			const string & head = this->fmk->toLower(header.first);
			// Если заголовок Host передан, запоминаем , что мы его нашли
			if(head.compare("host") == 0) available.emplace(header_t::HOST);
			// Если заголовок Accept передан, запоминаем , что мы его нашли
			if(head.compare("accept") == 0) available.emplace(header_t::ACCEPT);
			// Если заголовок Origin перадан, запоминаем, что мы его нашли
			else if(head.compare("origin") == 0) available.emplace(header_t::ORIGIN);
			// Если заголовок User-Agent передан, запоминаем, что мы его нашли
			else if(head.compare("user-agent") == 0) available.emplace(header_t::USERAGENT);
			// Если заголовок Connection перадан, запоминаем, что мы его нашли
			else if(head.compare("connection") == 0) available.emplace(header_t::CONNECTION);
			// Если заголовок Accept-Language передан, запоминаем, что мы его нашли
			else if(head.compare("accept-language") == 0) available.emplace(header_t::ACCEPTLANGUAGE);
			// Если заголовок Content-Length перадан, запоминаем, что мы его нашли
			else if(head.compare("content-length") == 0){
				// Устанавливаем размер тела сообщения
				contentLength = stoull(header.second);
				// Запоминаем, что мы нашли заголовок
				available.emplace(header_t::CONTENTLENGTH);
			}
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
		}
		// Устанавливаем Host если не передан
		if(available.count(header_t::HOST) < 1)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("Host: %s\r\n", (!url.domain.empty() ? url.domain : url.ip).c_str()));
		// Устанавливаем Origin если не передан
		if(available.count(header_t::ORIGIN) < 1)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("Origin: %s\r\n", this->uri->createOrigin(url).c_str()));
		// Устанавливаем User-Agent если не передан
		if(available.count(header_t::USERAGENT) < 1)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("User-Agent: %s\r\n", this->userAgent.c_str()));
		// Устанавливаем Connection если не передан
		if(available.count(header_t::CONNECTION) < 1)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
		// Устанавливаем Accept-Language если не передан
		if(available.count(header_t::ACCEPTLANGUAGE) < 1)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
		// Устанавливаем Accept если не передан
		if(available.count(header_t::ACCEPT) < 1)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
		// Если нужно произвести сжатие контента
		if(http->zip != zip_t::NONE)
			// Добавляем заголовок в запрос
			request.append(this->fmk->format("Accept-Encoding: %s\r\n", HTTP_HEADER_ACCEPTENCODING));
		// Получаем параметры авторизации
		const string & auth = this->auth->header();
		// Если данные авторизации получены
		if(!auth.empty()) request.append(auth);
		// Если запрос не является HEAD и тело запроса существует
		if((method != method_t::HEAD) && !this->body.empty()){
			// Проверяем нужно ли передать тело разбив на чанки
			this->chunking = ((available.count(header_t::CONTENTLENGTH) < 1) || (contentLength != this->body.size()));
			// Устанавливаем Content-Length если не передан
			if(this->chunking) request.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
		}
		// Устанавливаем завершающий разделитель
		request.append("\r\n");
		// Формируем результат запроса
		result.assign(result.begin(), result.end());
	}
	// Выводим результат
	return result;
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::Http::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->subs.emplace(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::Http::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()){
		// Переходим по всем подпротоколам
		for(auto & sub : subs)
			// Устанавливаем подпротокол
			this->setSub(sub);
	}
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
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 * @param url объект URL адреса сервера
 */
awh::Http::Http(const fmk_t * fmk, const log_t * log, const uri_t * uri, const uri_t::url_t * url) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		this->uri = uri;
		this->url = url;
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->log);
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
	// Удаляем объект авторизации
	if(this->auth != nullptr) delete this->auth;
}
