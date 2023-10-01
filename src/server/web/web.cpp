/**
 * @file: web.cpp
 * @date: 2022-10-01
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <server/web/web.hpp>

/**
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Web::openCallback(const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		dynamic_cast <server::core_t *> (core)->init(sid, this->_service.port, this->_service.host);
		// Выполняем запуск сервера
		dynamic_cast <server::core_t *> (core)->run(sid);
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::server::Web::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Определяем статус активности сетевого ядра
		switch(static_cast <uint8_t> (status)){
			// Если система запущена
			case static_cast <uint8_t> (awh::core_t::status_t::START): {
				// Выполняем биндинг ядра локального таймера
				core->bind(&this->_timer);
				// Устанавливаем таймаут времени на удаление мусорных адъютантов раз в 10 секунд
				this->_timer.setTimeout(10000, (function <void (const u_short, awh::core_t *)>) std::bind(&web_t::garbage, this, _1, _2));
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP):
				// Выполняем анбиндинг ядра локального таймера
				core->unbind(&this->_timer);
			break;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
 * @param ip   адрес интернет подключения адъютанта
 * @param mac  мак-адрес подключившегося адъютанта
 * @param port порт подключившегося адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению адъютанта
 */
bool awh::server::Web::acceptCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Выполняем блокировку неиспользуемых переменных
		(void) sid;
		(void) core;
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выводим функцию обратного вызова
			return this->_callback.apply <bool, const string &, const string &, const u_int> ("accept", ip, mac, port);
	}
	// Разрешаем подключение адъютанту
	return result;
}
/**
 * chunking Метод обработки получения чанков
 * @param aid   идентификатор адъютанта
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::server::Web::chunking(const uint64_t aid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()){
		// Выполняем блокировку неиспользуемую переменную
		(void) aid;
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
		// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выводим функцию обратного вызова
			this->_callback.call <const int32_t, const vector <char> &> ("chunks", 1, chunk);
	}
}
/**
 * garbage Метод удаления мусорных адъютантов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Web::garbage(const u_short tid, awh::core_t * core) noexcept {
	// Если список мусорных адъютантов не пустой
	if(!this->_garbage.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку мусорных адъютантов
		for(auto it = this->_garbage.begin(); it != this->_garbage.end();){
			// Если адъютант уже давно удалился
			if((date - it->second) >= 10000)
				// Выполняем удаление объекта адъютантов из списка мусора
				it = this->_garbage.erase(it);
			// Выполняем пропуск адъютанта
			else ++it;
		}
	}
	// Устанавливаем таймаут времени на удаление мусорных адъютантов раз в 10 секунд
	this->_timer.setTimeout(10000, (function <void (const u_short, awh::core_t *)>) std::bind(&web_t::garbage, this, _1, _2));
}
/**
 * init Метод инициализации WEB адъютанта
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Web::init(const string & socket, const http_t::compress_t compress) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку unix-сокет
		const_cast <server::core_t *> (this->_core)->unixSocket(socket);
	#endif
}
/**
 * init Метод инициализации WEB адъютанта
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Web::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем порт сервера
	this->_service.port = port;
	// Устанавливаем хост сервера
	this->_service.host = host;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Удаляем unix-сокет ранее установленный
		const_cast <server::core_t *> (this->_core)->removeUnixSocket();
	#endif
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const mode_t)> ("active", callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <string (const string &)> ("extractPassword", callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const string &, const string &)> ("checkPassword", callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const vector <char> &, const awh::http_t *)> ("chunking", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("events", callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const string &, const string &, const u_int)> ("accept", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const log_t::flag_t, const http::error_t, const string &)> ("error", callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const uint64_t, const mode_t)> ("stream", callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const uint64_t, const string &, const string &)> ("header", callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set<void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Web::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", callback);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Web::alive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->_service.alive = mode;
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Web::alive(const time_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_timeAlive = time;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Web::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Активируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(true);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on(std::bind(&web_t::eventsCallback, this, _1, _2));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Деактивируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(false);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Web::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если завершить работу разрешено
		if(this->_unbind)
			// Завершаем работу
			const_cast <server::core_t *> (this->_core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else const_cast <server::core_t *> (this->_core)->close();
	}
}
/**
 * start Метод запуска сервера
 */
void awh::server::Web::start() noexcept {	
	// Если биндинг не запущен
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->_core)->start();
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Web::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->_service.realm = realm;
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Web::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->_service.opaque = opaque;
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Web::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Web::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->_maxRequests = max;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Web::serv(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->_serv.id = id;
	// Устанавливаем версию сервера
	this->_serv.ver = ver;
	// Устанавливаем название сервера
	this->_serv.name = name;
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Web::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->_authHash = hash;
	// Устанавливаем тип авторизации
	this->_authType = type;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Web::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	if((this->_crypto.mode = !pass.empty())){
		// Пароль шифрования передаваемых данных
		this->_crypto.pass = pass;
		// Соль шифрования передаваемых данных
		this->_crypto.salt = salt;
		// Размер шифрования передаваемых данных
		this->_crypto.cipher = cipher;
	}
}
/**
 * Web Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Web::Web(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(getpid()), _uri(fmk), _callback(log), _timer(fmk, log), _authHash(auth_t::hash_t::MD5), _authType(auth_t::type_t::NONE),
 _unbind(false), _timeAlive(KEEPALIVE_TIMEOUT), _chunkSize(BUFFER_CHUNK), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log), _core(nullptr) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.noInfo(true);
}
/**
 * Web Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Web::Web(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(getpid()), _uri(fmk), _callback(log), _timer(fmk, log), _authHash(auth_t::hash_t::MD5), _authType(auth_t::type_t::NONE),
 _unbind(false), _timeAlive(KEEPALIVE_TIMEOUT), _chunkSize(BUFFER_CHUNK), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log), _core(core) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.noInfo(true);
	// Активируем персистентный запуск для работы пингов
	const_cast <server::core_t *> (this->_core)->persistEnable(true);
	// Устанавливаем функцию активации ядра сервера
	const_cast <server::core_t *> (this->_core)->on(std::bind(&web_t::eventsCallback, this, _1, _2));
}
