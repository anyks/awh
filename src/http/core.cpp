/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <http/core.hpp>

/**
 * chunkingCallback Функция вывода полученных чанков полезной нагрузки
 * @param buffer буфер данных чанка полезной нагрузки
 * @param web    объект HTTP парсера
 * @param ctx    передаваемый контекст модуля
 */
void awh::Http::chunkingCallback(const vector <char> & buffer, const web_t * web, void * ctx) noexcept {
	// Если передаваемый контекст передан
	if(ctx != nullptr){
		// Получаем объект модуля для работы с REST
		http_t * http = reinterpret_cast <http_t *> (ctx);
		// Если функция обратного вызова передана
		if(http->chunkingFn != nullptr) http->chunkingFn(buffer, http, http->ctx.at(0));
	}
}
/**
 * update Метод обновления входящих данных
 */
void awh::Http::update() noexcept {
	// Получаем данные тела
	const auto & body = this->web.getBody();
	// Если тело сообщения получено
	if(!body.empty()){
		// Отключаем сжатие ответа с сервера
		this->compress = compress_t::NONE;
		// Получаем заголовок шифрования
		const string & encrypt = this->web.getHeader("x-awh-encryption");
		// Если заголовок найден
		if((this->crypt = !encrypt.empty())){
			// Определяем размер шифрования
			switch(stoi(encrypt)){
				// Если шифрование произведено 128 битным ключём
				case 128: this->hash.setAES(hash_t::aes_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->hash.setAES(hash_t::aes_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->hash.setAES(hash_t::aes_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & res = this->hash.decrypt(body.data(), body.size());
			// Если данные расшифрованны, заменяем тело данных
			if(!res.empty()) this->web.setBody(res);
		}
		// Проверяем пришли ли сжатые данные
		const string & encoding = this->web.getHeader("content-encoding");
		// Если данные пришли сжатые
		if(!encoding.empty()){
			// Если данные пришли сжатые методом Brotli
			if(encoding.compare("br") == 0){
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::BROTLI;
				// Выполняем декомпрессию данных
				const auto & res = this->hash.decompressBrotli(body.data(), body.size());
				// Заменяем полученное тело
				if(!res.empty()) this->web.setBody(res);
			// Если данные пришли сжатые методом GZip
			} else if(encoding.compare("gzip") == 0) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::GZIP;
				// Выполняем декомпрессию данных
				const auto & res = this->hash.decompressGzip(body.data(), body.size());
				// Заменяем полученное тело
				if(!res.empty()) this->web.setBody(res);
			// Если данные пришли сжатые методом Deflate
			} else if(encoding.compare("deflate") == 0) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->compress = compress_t::DEFLATE;
				// Получаем данные тела в бинарном виде
				vector <char> buffer(body.begin(), body.end());
				// Добавляем хвост в полученные данные
				this->hash.setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & res = this->hash.decompress(buffer.data(), buffer.size());
				// Заменяем полученное тело
				if(!res.empty()) this->web.setBody(res);
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
	// Выполняем очистку данных парсера
	this->web.clear();
	// Выполняем сброс чёрного списка HTTP заголовков
	this->black.clear();
}
/**
 * reset Метод сброса параметров запроса
 */
void awh::Http::reset() noexcept {
	// Выполняем сброс данных парсера
	this->web.reset();
	// Выполняем сброс параметров запроса
	this->url.clear();
	// Обнуляем флаг проверки авторизации
	this->failAuth = false;
	// Выполняем сброс стейта авторизации
	this->stath = stath_t::NONE;
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::NONE;
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
		// Выполняем парсинг сырых данных
		this->web.parse(buffer, size);
		// Если парсинг выполнен
		if(this->web.isEnd()){
			// Выполняем проверку авторизации
			this->stath = this->checkAuth();
			// Если ключ соответствует
			if(this->stath == stath_t::GOOD){
				// Выполняем обновление входящих параметров
				this->update();
				// Устанавливаем стейт рукопожатия
				this->state = state_t::GOOD;
			// Поменяем данные как бракованные
			} else this->state = state_t::BROKEN;
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
		this->web.addBody(buffer, size);
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
		this->web.addHeader(key, val);
}
/**
 * payload Метод чтения чанка тела запроса
 * @return текущий чанк запроса
 */
const vector <char> awh::Http::payload() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Получаем собранные данные тела
	vector <char> * body = const_cast <vector <char> *> (&this->web.getBody());
	// Если данные тела ещё существуют
	if(!body->empty()){
		// Если нужно тело выводить в виде чанков
		if(this->chunking){
			// Тело чанка запроса
			string chunk = "";
			// Если тело сообщения больше размера чанка
			if(body->size() >= this->chunkSize){
				// Получаем размер чанка
				chunk = this->fmk->decToHex(this->chunkSize);
				// Добавляем разделитель
				chunk.append("\r\n");
				// Формируем тело чанка
				chunk.insert(chunk.end(), body->begin(), body->begin() + this->chunkSize);
				// Добавляем конец запроса
				chunk.append("\r\n");
				// Удаляем полученные данные в теле сообщения
				body->erase(body->begin(), body->begin() + this->chunkSize);
			// Если тело сообщения полностью убирается в размер чанка
			} else {
				// Получаем размер чанка
				chunk = this->fmk->decToHex(body->size());
				// Добавляем разделитель
				chunk.append("\r\n");
				// Формируем тело чанка
				chunk.insert(chunk.end(), body->begin(), body->end());
				// Добавляем конец запроса
				chunk.append("\r\n0\r\n\r\n");
				// Очищаем данные тела
				body->clear();
			}
			// Формируем результат
			result.assign(chunk.begin(), chunk.end());
			// Освобождаем память
			string().swap(chunk);
		// Выводим данные тела как есть
		} else {
			// Если тело сообщения больше размера чанка
			if(body->size() >= this->chunkSize){
				// Получаем нужный нам размер данных
				result.assign(body->begin(), body->begin() + this->chunkSize);
				// Удаляем полученные данные в теле сообщения
				body->erase(body->begin(), body->begin() + this->chunkSize);
			// Если тело сообщения полностью убирается в размер чанка
			} else {
				// Получаем нужный нам размер данных
				result.assign(body->begin(), body->end());
				// Очищаем данные тела
				body->clear();
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * getBody Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Http::getBody() const noexcept {
	// Выводим данные тела
	return this->web.getBody();
}
/**
 * getHeader Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
const string & awh::Http::getHeader(const string & key) const noexcept {
	// Выводим запрашиваемый заголовок
	return this->web.getHeader(key);
}
/**
 * getHeaders Метод получения списка заголовков
 * @return список существующих заголовков
 */
const unordered_multimap <string, string> & awh::Http::getHeaders() const noexcept {
	// Выводим список доступных заголовков
	return this->web.getHeaders();
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
	// Запрашиваем заголовок подключения
	const string & header = this->web.getHeader("connection");
	// Если заголовок получен
	if(!header.empty())
		// Выполняем проверку является ли соединение закрытым
		result = (header.compare("close") != 0);
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
	// Выводим результат проверки
	return this->web.isHeader(key);
}
/**
 * getQuery Метод получения объекта запроса сервера
 * @return объект запроса сервера
 */
const awh::web_t::query_t & awh::Http::getQuery() const noexcept {
	// Выводим объект запроса сервера
	return this->web.getQuery();
}
/**
 * setQuery Метод добавления объекта запроса клиента
 * @param query объект запроса клиента
 */
void awh::Http::setQuery(const web_t::query_t & query) noexcept {
	// Устанавливаем объект запроса клиента
	this->web.setQuery(query);
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
			// Объект параметров запроса
			web_t::query_t query;
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
			query.uri = this->fmk->format("%s:%u", host.c_str(), url.port);
			// Устанавливаем парарметр запроса
			this->web.setQuery(query);
			// Выполняем создание запроса
			return this->request(url, web_t::method_t::CONNECT);
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
	// Объект параметров запроса
	web_t::query_t query;
	// Получаем текст сообщения
	query.message = (!mess.empty() ? mess : this->getMessage(code));
	// Если сообщение получено
	if(!query.message.empty()){
		// Если требуется ввод авторизационных данных
		if((code == 401) || (code == 407))
			// Добавляем заголовок закрытия подключения
			this->web.addHeader("Connection", "keep-alive");
		// Добавляем заголовок закрытия подключения
		else this->web.addHeader("Connection", "close");
		// Добавляем заголовок тип контента
		this->web.addHeader("Content-type", "text/html; charset=utf-8");
		// Если запрос должен содержать тело сообщения
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308)){
			// Получаем данные тела
			const auto & body = this->web.getBody();
			// Если тело ответа не установлено, устанавливаем своё
			if(body.empty()){
				// Формируем тело ответа
				const string & body = this->fmk->format(
					"<html><head><title>%u %s</title></head>\n<body><h2>%u %s</h2></body></html>",
					code, query.message.c_str(), code, query.message.c_str()
				);
				// Добавляем тело сообщения
				this->web.addBody(body.data(), body.size());
			}
			// Добавляем заголовок тела сообщения
			this->web.addHeader("Content-Length", to_string(body.size()));
		}
		// Устанавливаем парарметр запроса
		this->web.setQuery(query);
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
	// Получаем объект параметров запроса
	web_t::query_t query = this->web.getQuery();
	// Получаем текст сообщения
	query.message = (!mess.empty() ? mess : this->getMessage(code));
	// Если сообщение получено
	if(!query.message.empty()){
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
		query.code = code;
		// Данные REST ответа
		string response = this->fmk->format("HTTP/%.1f %u %s\r\n", query.ver, code, query.message.c_str());
		// Если заголовок не запрещён
		if(!this->isBlack("Date"))
			// Добавляем заголовок даты в ответ
			response.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
		// Переходим по всему списку заголовков
		for(auto & header : this->web.getHeaders()){
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
		// Получаем данные тела
		const auto & body = this->web.getBody();
		// Если запрос должен содержать тело и тело ответа существует
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308) && !body.empty()){
			// Проверяем нужно ли передать тело разбив на чанки
			this->chunking = (!available[2] || ((length > 0) && (length != body.size())));
			// Если нужно производить шифрование
			if(this->crypt && !this->isBlack("X-AWH-Encryption")){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash.encrypt(body.data(), body.size());
				// Если данные зашифрованы, заменяем тело данных
				if(!res.empty()){
					// Заменяем тело данных
					this->web.setBody(res);
					// Заменяем размер тела данных
					if(!this->chunking) length = body.size();
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
						const auto & brotli = this->hash.compressBrotli(body.data(), body.size());
						// Если данные сжаты, заменяем тело данных
						if(!brotli.empty()){
							// Заменяем тело данных
							this->web.setBody(brotli);
							// Заменяем размер тела данных
							if(!this->chunking) length = body.size();
							// Устанавливаем Content-Encoding если не передан
							if(!available[3]) response.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
						}
					} break;
					// Если нужно сжать тело методом GZIP
					case (uint8_t) compress_t::GZIP: {
						// Выполняем сжатие тела сообщения
						const auto & gzip = this->hash.compressGzip(body.data(), body.size());
						// Если данные сжаты, заменяем тело данных
						if(!gzip.empty()){
							// Заменяем тело данных
							this->web.setBody(gzip);
							// Заменяем размер тела данных
							if(!this->chunking) length = body.size();
							// Устанавливаем Content-Encoding если не передан
							if(!available[3]) response.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
						}
					} break;
					// Если нужно сжать тело методом DEFLATE
					case (uint8_t) compress_t::DEFLATE: {
						// Выполняем сжатие тела сообщения
						auto deflate = this->hash.compress(body.data(), body.size());
						// Удаляем хвост в полученных данных
						this->hash.rmTail(deflate);
						// Если данные сжаты, заменяем тело данных
						if(!deflate.empty()){
							// Заменяем тело данных
							this->web.setBody(deflate);
							// Заменяем размер тела данных
							if(!this->chunking) length = body.size();
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
		} else this->web.clearBody();
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
vector <char> awh::Http::request(const uri_t::url_t & url, const web_t::method_t method) const noexcept {
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
			// Получаем объект параметров запроса
			web_t::query_t query = this->web.getQuery();
			// Устанавливаем параметры REST запроса
			this->authCli.setUri(this->uri->createUrl(url));
			// Если метод не CONNECT или URI не установлен
			if((method != web_t::method_t::CONNECT) || query.uri.empty())
				// Формируем HTTP запрос
				query.uri = this->uri->createQuery(url);
			// Определяем метод запроса
			switch((uint8_t) method){
				// Если метод запроса указан как GET
				case (uint8_t) web_t::method_t::GET:
					// Формируем GET запрос
					request = this->fmk->format("GET %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как PUT
				case (uint8_t) web_t::method_t::PUT:
					// Формируем PUT запрос
					request = this->fmk->format("PUT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как POST
				case (uint8_t) web_t::method_t::POST:
					// Формируем POST запрос
					request = this->fmk->format("POST %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как HEAD
				case (uint8_t) web_t::method_t::HEAD:
					// Формируем HEAD запрос
					request = this->fmk->format("HEAD %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как PATCH
				case (uint8_t) web_t::method_t::PATCH:
					// Формируем PATCH запрос
					request = this->fmk->format("PATCH %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как TRACE
				case (uint8_t) web_t::method_t::TRACE:
					// Формируем TRACE запрос
					request = this->fmk->format("TRACE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как DELETE
				case (uint8_t) web_t::method_t::DEL:
					// Формируем DELETE запрос
					request = this->fmk->format("DELETE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как OPTIONS
				case (uint8_t) web_t::method_t::OPTIONS:
					// Формируем OPTIONS запрос
					request = this->fmk->format("OPTIONS %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как CONNECT
				case (uint8_t) web_t::method_t::CONNECT:
					// Формируем CONNECT запрос
					request = this->fmk->format("CONNECT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
			}
			// Запоминаем метод запроса
			query.method = method;
			// Если заголовок не запрещён
			if(!this->isBlack("Date"))
				// Добавляем заголовок даты в запрос
				request.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
			// Переходим по всему списку заголовков
			for(auto & header : this->web.getHeaders()){
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
			if(!available[1] && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept"))
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
			if(!available[6] && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Language"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
			// Если нужно произвести сжатие контента
			if((this->compress != compress_t::NONE) && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Encoding"))
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
			// Получаем данные тела
			const auto & body = this->web.getBody();
			// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
			if((method != web_t::method_t::GET) && (method != web_t::method_t::HEAD) && (method != web_t::method_t::TRACE) && !body.empty()){
				// Проверяем нужно ли передать тело разбив на чанки
				this->chunking = (!available[5] || ((length > 0) && (length != body.size())));
				// Если нужно производить шифрование
				if(this->crypt && !this->isBlack("X-AWH-Encryption")){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash.encrypt(body.data(), body.size());
					// Если данные зашифрованы, заменяем тело данных
					if(!res.empty()){
						// Заменяем тело данных
						this->web.setBody(res);
						// Заменяем размер тела данных
						if(!this->chunking) length = body.size();
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
							const auto & brotli = this->hash.compressBrotli(body.data(), body.size());
							// Если данные сжаты, заменяем тело данных
							if(!brotli.empty()){
								// Заменяем тело данных
								this->web.setBody(brotli);
								// Заменяем размер тела данных
								if(!this->chunking) length = body.size();
								// Устанавливаем Content-Encoding если не передан
								if(!available[7]) request.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
							}
						} break;
						// Если нужно сжать тело методом GZIP
						case (uint8_t) compress_t::GZIP: {
							// Выполняем сжатие тела сообщения
							const auto & gzip = this->hash.compressGzip(body.data(), body.size());
							// Если данные сжаты, заменяем тело данных
							if(!gzip.empty()){
								// Заменяем тело данных
								this->web.setBody(gzip);
								// Заменяем размер тела данных
								if(!this->chunking) length = body.size();
								// Устанавливаем Content-Encoding если не передан
								if(!available[7]) request.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
							}
						} break;
						// Если нужно сжать тело методом DEFLATE
						case (uint8_t) compress_t::DEFLATE: {
							// Выполняем сжатие тела сообщения
							auto deflate = this->hash.compress(body.data(), body.size());
							// Удаляем хвост в полученных данных
							this->hash.rmTail(deflate);
							// Если данные сжаты, заменяем тело данных
							if(!deflate.empty()){
								// Заменяем тело данных
								this->web.setBody(deflate);
								// Заменяем размер тела данных
								if(!this->chunking) length = body.size();
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
			} else this->web.clearBody();
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
