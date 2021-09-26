/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <bind/rest.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param ctx   контекст объекта http
 */
void awh::Rest::chunking(const vector <char> & chunk, const http_t * ctx) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (ctx)->addBody(chunk.data(), chunk.size());
}
/**
 * GET Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём воркер запроса
		worker_t worker(this->fmk, this->log);
		// Устанавливаем количество попыток реконнекта
		worker.attempts.second = 2;
		// Устанавливаем минимальное количество байт на чтение
		worker.byteRead = 10;
		// Устанавливаем минимальное количество байт на запись
		worker.byteWrite = 10;
		// Устанавливаем таймер на чтение
		worker.timeRead = READ_TIMEOUT;
		// Устанавливаем таймер на запись
		worker.timeWrite = WRITE_TIMEOUT;
		// Устанавливаем флаг ожидания входящих сообщений
		worker.wait = this->wait;
		// Устанавливаем флаг автоматического поддержания подключения
		worker.alive = this->alive;
		// Устанавливаем прокси-сервер
		worker.proxy = this->proxy;
		// Устанавливаем URL адрес запроса
		worker.url = url;
		// Устанавливаем событие на запуск системы
		worker.startFn = [](const size_t wid, bind_t * bind) noexcept {
			cout << " ----------------START " << wid << endl;
			// Выполняем подключение
			bind->open(wid);
		};
		// Устанавливаем событие подключения
		worker.openFn = [&url, &headers, this](const size_t wid, bind_t * bind) noexcept {
			cout << " ----------------OPEN " << wid << endl;

			// Очищаем объект запроса
			this->http->clear();
			// Если список заголовков получен
			if(!headers.empty()){
				// Переходим по всему списку заголовков
				for(auto & header : headers){
					// Устанавливаем заголовок
					this->http->addHeader(header.first, header.second);
				}
			}
			// Получаем бинарные данные REST запроса
			const auto & rest = this->http->request(url, http_t::method_t::GET);
			// Если бинарные данные запроса получены
			if(!rest.empty())
				// Отправляем серверу сообщение
				bind->write(rest.data(), rest.size(), wid);
			// Выполняем сброс всех отправленных данных
			this->http->clear();
		};
		// Устанавливаем событие отключения
		worker.closeFn = [](const size_t wid, bind_t * bind) noexcept {
			cout << " ----------------CLOSE " << wid << endl;

			// Завершаем работу
			bind->stop();
		};
		// Устанавливаем функцию чтения данных
		worker.readFn = [](const char * buffer, const size_t size, const size_t wid, bind_t * bind) noexcept {
			cout << " ----------------READ " << wid << " == " << string(buffer, size) << endl;
		};
		// Устанавливаем функцию записи данных
		worker.writeFn = [](const char * buffer, const size_t size, const size_t wid, bind_t * bind) noexcept {
			cout << " ----------------WRITE " << wid << " == " << string(buffer, size) << endl;
		};
		// Добавляем воркер в биндер TCP/IP
		const_cast <bind_t *> (this->bind)->add(&worker);
		// Запускаем биндинг
		const_cast <bind_t *> (this->bind)->start();
	}
	// Выводим результат
	return result;
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Rest::setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http->setChunkingFn(callback);
}
/**
 * setKeepAlive Метод установки флага автоматического поддержания подключения
 * @param mode флаг автоматического поддержания подключения
 */
void awh::Rest::setKeepAlive(const bool mode) noexcept {
	// Устанавливаем флаг поддержания автоматического подключения
	this->alive = mode;
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Rest::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http->setChunkSize(size);
}
/**
 * setWaitMessage Метод установки флага ожидания входящих сообщений
 * @param mode флаг состояния разрешения проверки
 */
void awh::Rest::setWaitMessage(const bool mode) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->wait = mode;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Rest::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->http->setUserAgent(userAgent);
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::Rest::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод сжатия
	this->http->setCompress(compress);
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Rest::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->http->setUser(login, password);
}
/**
 * setProxyServer Метод установки прокси-сервера
 * @param uri  параметры прокси-сервера
 * @param type тип прокси-сервера
 */
void awh::Rest::setProxyServer(const string & uri, const proxy_t::type_t type) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем тип прокси-сервера
		this->proxy.type = type;
		// Устанавливаем параметры прокси-сервера
		this->proxy.url = this->uri->parseUrl(uri);
	}
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Rest::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http->setServ(id, name, ver);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем параметры шифрования
	this->http->setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	this->http->setAuthType(type, algorithm);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthTypeProxy(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Устанавливаем тип авторизации прокси-сервера
	this->proxy.auth = type;
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->proxy.algorithm = algorithm;
}
/**
 * Rest Конструктор
 * @param bind объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Rest::Rest(const bind_t * bind, const fmk_t * fmk, const log_t * log) noexcept : bind(bind), fmk(fmk), log(log) {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект URI
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с HTTP
		this->http = new http_t(this->fmk, this->log, this->uri);
		// Устанавливаем функцию обработки вызова для получения чанков
		this->http->setChunkingFn(&chunking);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Rest Деструктор
 */
awh::Rest::~Rest() noexcept {
	// Удаляем объект для работы с URI
	if(this->uri != nullptr) delete this->uri;
	// Удаляем объект работы с HTTP
	if(this->http != nullptr) delete this->http;
}
