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
		this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, error.what());
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
	if(!this->clientKey.empty()){
		// Создаем контекст
		SHA_CTX ctx;
		// Выполняем инициализацию контекста
		SHA1_Init(&ctx);
		// Массив полученных значений
		u_char digest[20];
		// Формируем магический ключ
		const string text = (this->clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
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
	// Выполняем сброс код ответа сервера
	this->code = 0;
	// Выполняем сброс поддерживаемого сабпротокола
	this->sub = "";
	// Выполняем сброс сообщения сервера
	this->message = "";
	// Выполняем сброс ключа клиента
	this->clientKey = "";
	// Выполняем сброс флага разрешающего сжатие данных
	this->gzip = false;
	// Выполняем сброс поддерживаемых сабпротоколов
	this->subs.clear();
	// Выполняем сброс полученных HTTP заголовков
	this->headers.clear();
	// Выполняем сброс размера окна сжатия данных
	this->wbit = MAX_WBITS;
	// Выполняем сброс версии протокола
	this->version = HTTP_VERSION;
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::QUERY;
}
/**
 * add Метод добавления данных заголовков
 * @param buffer буфер данных с заголовками
 * @param size   размер буфера данных с заголовками
 * @return       результат завершения сбора данных
 */
bool awh::Http::add(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	bool result = (this->state == state_t::HANDSHAKE);
	// Если рукопожатие не выполнено
	if(!result){
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			// Получаем строку ответа
			const string data(buffer, size);
			// Определяем статус режима работы
			switch((u_short) this->state){
				// Если - это режим ожидания получения запроса
				case (u_short) state_t::QUERY: {
					// Выполняем поиск первого пробела
					size_t first = data.find(" ");
					// Если пробел получен
					if(first == 8){
						// Получаем версию протокол запроса
						this->version = stod(data.substr(5, first));
						// Выполняем поиск второго пробела
						const size_t second = data.find(" ", first + 1);
						// Если пробел получен
						if(second != string::npos){
							// Выполняем смену стейта
							this->state = state_t::HEADERS;
							// Получаем сообщение сервера
							this->message = data.substr(second + 1);
							// Получаем код ответа
							this->code = stoi(data.substr(first + 1, second));
						}
					}
				} break;
				// Если - это режим получения заголовков
				case (u_short) state_t::HEADERS: {
					// Выполняем поиск пробела
					const size_t pos = data.find(": ");
					// Если разделитель найден
					if(pos != string::npos){
						// Получаем значение заголовка
						const string & val = data.substr(pos + 2);
						// Получаем ключ заголовка
						const string & key = this->fmk->toLower(data.substr(0, pos));
						// Добавляем заголовок в список заголовков
						if(!key.empty() && !val.empty())
							// Добавляем заголовок в список
							this->headers.emplace(key, val);
					}

				} break;
			}
		// Если буфер данных не передан
		} else if(!this->headers.empty()){
			// Если ключ соответствует
			if((result = (this->checkKey() && this->checkUpgrade() && this->checkVersion()))){
				// Выполняем обновление списка расширений
				this->updateExtensions();
				// Выполняем обновление списка сабпротоколов
				this->updateSubProtocol();
				// Устанавливаем стейт рукопожатия
				this->state = state_t::HANDSHAKE;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * isGzip Метод получения флага сжатого контента в GZIP
 * @return значение флага сжатого контента в GZIP
 */
bool awh::Http::isGzip() const noexcept {
	// Выводим результат проверки
	return this->gzip;
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
 * getWbit Метод получения размер окна для сжатия в GZIP
 * @return размер окна для сжатия в GZIP
 */
u_int awh::Http::getWbit() const noexcept {
	// Выводим размер окна для сжатия в GZIP
	return this->wbit;
}
/**
 * getCode Метод получения кода ответа сервера
 * @return код ответа сервера
 */
u_short awh::Http::getCode() const noexcept {
	// Выводим код ответа сервера
	return this->code;
}
/**
 * getVersion Метод получения версии HTTP протокола
 * @return версия HTTP протокола
 */
double awh::Http::getVersion() const noexcept {
	// Выводим версию HTTP протокола
	return this->version;
}
/**
 * getMessage Метод получения HTTP сообщения
 * @param code код сообщения для получение
 * @return     соответствующее коду HTTP сообщение
 */
const string & awh::Http::getMessage(const u_short code) const noexcept {
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
		this->version, this->date().c_str(), AWH_NAME,
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
		if(this->isGzip()){
			// Получаем размер окна для сжатия
			const u_short wbit = this->getWbit();
			// Формируем заголовок расширений
			extensions = "Sec-WebSocket-Extensions: permessage-deflate; server_no_context_takeover";
			// Если требуется указать количество байт
			if(wbit > 0) extensions.append(this->fmk->format("; client_max_window_bits=%u", wbit));
		}
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
			"%sSec-WebSocket-Accept: %s\r\n%s"
			"%s\r\n\r\n",
			this->version, this->date().c_str(),
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
		this->version, this->date().c_str(),
		AWH_NAME, 245, AWH_SHORT_NAME,
		AWH_VERSION, this->auth->header().c_str()
	);
	// Выводим результат
	return vector <char> (result.begin(), result.end());
}
/**
 * getRequest Метод получения буфера HTML запроса
 * @param gzip флаг просьбы предоставить контент в сжатом виде
 * @return     собранный HTML буфер
 */
vector <char> awh::Http::restRequest(const bool gzip) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если URL объект передан
	if(this->url != nullptr){
		// Получаем путь HTTP запроса
		const string & path = this->uri->joinPath(this->url->path);
		// Генерируем URL адрес запроса
		const string & origin = this->uri->createOrigin(* this->url);
		// Получаем параметры запроса
		const string & params = this->uri->joinParams(this->url->params);
		// Получаем хост запроса
		const string & host = (!this->url->domain.empty() ? this->url->domain : this->url->ip);
		// Если хост получен
		if(!host.empty() && !path.empty()){
			// Список желаемых подпротоколов и желаемая компрессия
			string subs = "", compress = "";
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
			this->clientKey = this->key();
			// Устанавливаем параметры желаемой компрессии
			if(gzip) compress = "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n";
			// Строка HTTP запроса
			const string & request = this->fmk->format(
				"GET %s HTTP/%.1f\r\n"
				"Host: %s\r\n"
				"Date: %s\r\n"
				"Origin: %s\r\n"
				"User-Agent: %s\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: websocket\r\n"
				"Sec-WebSocket-Version: %u\r\n"
				"Sec-WebSocket-Key: %s\r\n"
				"%s%s%s\r\n",
				query.c_str(), this->version,
				host.c_str(), this->date().c_str(),
				origin.c_str(), this->userAgent.c_str(),
				WS_VERSION, this->clientKey.c_str(),
				(!compress.empty() ? compress.c_str() : ""),
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
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Http::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->userAgent = userAgent;
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
 * @param fmk     объект фреймворка
 * @param uri     объект работы с URI
 * @param logfile адрес файла для сохранения логов
 */
awh::Http::Http(const fmk_t * fmk, const uri_t * uri, const char * logfile) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk     = fmk;
		this->uri     = uri;
		this->logfile = logfile;
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->logfile);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		fmk->log("%s", fmk_t::log_t::CRITICAL, logfile, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * Http Конструктор
 * @param fmk     объект фреймворка
 * @param uri     объект работы с URI
 * @param url     объект URL адреса сервера
 * @param logfile адрес файла для сохранения логов
 */
awh::Http::Http(const fmk_t * fmk, const uri_t * uri, const uri_t::url_t * url, const char * logfile) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk     = fmk;
		this->uri     = uri;
		this->url     = url;
		this->logfile = logfile;
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->logfile);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		fmk->log("%s", fmk_t::log_t::CRITICAL, logfile, "memory could not be allocated");
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
