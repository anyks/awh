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
				// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 5 секунд
				this->_timer.setInterval(5000, std::bind(&web_t::disconected, this, _1, _2));
				// Устанавливаем интервал времени на выполнения пинга удалённого сервера
				this->_timer.setInterval(PING_INTERVAL, std::bind(&web_t::pinging, this, _1, _2));
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
				// Останавливаем все установленные таймеры
				this->_timer.clearTimers();
				// Выполняем анбиндинг ядра локального таймера
				core->unbind(&this->_timer);
			} break;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const awh::core_t::status_t, awh::core_t *)> ("events", status, core);
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения брокера
 * @param ip   адрес интернет подключения брокера
 * @param mac  мак-адрес подключившегося брокера
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению брокера
 */
bool awh::server::Web::acceptCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выполняем функцию обратного вызова
			return this->_callback.call <bool (const string &, const string &, const u_int)> ("accept", ip, mac, port);
	}
	// Разрешаем подключение брокеру
	return result;
}
/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::server::Web::chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()){
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
		// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", 1, bid, chunk);
	}
}
/**
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::Web::erase(const uint64_t bid) noexcept {
	// Если список отключившихся клиентов не пустой
	if(!this->_disconected.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Если идентификатор брокера передан
		if(bid > 0){
			// Выполняем поиск указанного брокера
			auto it = this->_disconected.find(bid);
			// Если данные отключившегося брокера найдены
			if((it != this->_disconected.end()) && ((date - it->second) >= 10000))
				// Выполняем удаление брокера
				this->_disconected.erase(it);
		// Если идентификатор брокера не передан
		} else {
			// Выполняем переход по всему списку отключившихся брокеров
			for(auto it = this->_disconected.begin(); it != this->_disconected.end();){
				// Если брокер уже давно отключился
				if((date - it->second) >= 10000)
					// Выполняем удаление объекта брокеров из списка отключившихся
					it = this->_disconected.erase(it);
				// Выполняем пропуск брокера
				else ++it;
			}
		}
	}
}
/**
 * disconnect Метод отключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Web::disconnect(const uint64_t bid) noexcept {
	// Добавляем в очередь список отключившихся клиентов
	this->_disconected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
}
/**
 * disconected Метод удаления отключившихся брокеров
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Web::disconected(const uint16_t tid, awh::core_t * core) noexcept {
	// Выполняем удаление отключившихся брокеров
	this->erase();
}
/**
 * init Метод инициализации WEB брокера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Web::init(const string & socket, const vector <http_t::compress_t> & compressors) noexcept {
	// Отключаем неиспользуемую переменную
	(void) compressors;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если объект сетевого ядра создан
		if(this->_core != nullptr)
			// Выполняем установку unix-сокет
			const_cast <server::core_t *> (this->_core)->unixSocket(socket);
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Отключаем неиспользуемую переменную
		(void) socket;
	#endif
}
/**
 * init Метод инициализации WEB брокера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Web::init(const u_int port, const string & host, const vector <http_t::compress_t> & compressors) noexcept {
	// Отключаем неиспользуемую переменную
	(void) compressors;
	// Устанавливаем порт сервера
	this->_service.port = port;
	// Устанавливаем хост сервера
	this->_service.host = host;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если объект сетевого ядра создан
		if(this->_core != nullptr)
			// Удаляем unix-сокет ранее установленный
			const_cast <server::core_t *> (this->_core)->removeUnixSocket();
	#endif
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::server::Web::callback(const fn_t & callback) noexcept {
	// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с клиента
	this->_callback.set("raw", callback);
	// Выполняем установку функции обратного вызова при завершении запроса
	this->_callback.set("end", callback);
	// Выполняем установку функции обратного вызова при удаление клиента из стека сервера
	this->_callback.set("erase", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова для вывода полученного заголовка с клиента
	this->_callback.set("header", callback);
	// Выполняем установку функции обратного вызова для полученного тела данных с клиента
	this->_callback.set("entity", callback);
	// Выполняем установку функции обратного вызова для вывода полученного чанка бинарных данных с клиента
	this->_callback.set("chunks", callback);
	// Выполняем установку функции обратного вызова на событие запуска или остановки подключения
	this->_callback.set("active", callback);
	// Выполняем установку функции обратного вызова получения событий запуска и остановки сетевого ядра
	this->_callback.set("events", callback);
	// Выполняем установку функции обратного вызова на событие активации брокера на сервере
	this->_callback.set("accept", callback);
	// Выполняем установку функции обратного вызова активности потока
	this->_callback.set("stream", callback);
	// Выполняем установку функции обратного вызова для вывода запроса клиента к серверу
	this->_callback.set("request", callback);
	// Выполняем установку функции обратного вызова для вывода полученных заголовков с клиента
	this->_callback.set("headers", callback);
	// Выполняем установку функции обратного вызова для перехвата полученных чанков
	this->_callback.set("chunking", callback);
	// Выполняем установку функции обратного вызова при выполнении рукопожатия
	this->_callback.set("handshake", callback);
	// Выполняем установку функции обратного вызова для обработки авторизации
	this->_callback.set("checkPassword", callback);
	// Выполняем установку функции обратного вызова для извлечения пароля
	this->_callback.set("extractPassword", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок Websocket
	this->_callback.set("errorWebsocket", callback);
	// Выполняем установку функции обратного вызова на событие получения сообщений Websocket
	this->_callback.set("messageWebsocket", callback);
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
		{
			// Создаём локальный контейнер функций обратного вызова
			fn_t callback(this->_log);
			// Устанавливаем функцию активации ядра сервера
			callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("events", std::bind(&web_t::eventsCallback, this, _1, _2));
			// Выполняем установку функций обратного вызова для сервера
			const_cast <server::core_t *> (this->_core)->callback(std::move(callback));
		}
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr)
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Web::stop() noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
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
	if((this->_core != nullptr) && !this->_core->working())
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
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Web::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->_ident.id = id;
	// Устанавливаем версию сервера
	this->_ident.ver = ver;
	// Устанавливаем название сервера
	this->_ident.name = name;
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Web::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->_service.hash = hash;
	// Устанавливаем тип авторизации
	this->_service.type = type;
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::Web::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования
	this->_encryption.mode = mode;
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Web::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Пароль шифрования передаваемых данных
	this->_encryption.pass = pass;
	// Соль шифрования передаваемых данных
	this->_encryption.salt = salt;
	// Размер шифрования передаваемых данных
	this->_encryption.cipher = cipher;
}
/**
 * Web Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Web::Web(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(getpid()), _uri(fmk), _callback(log), _timer(fmk, log), _unbind(true), _timeAlive(KEEPALIVE_TIMEOUT),
 _chunkSize(BUFFER_CHUNK), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log), _core(nullptr) {
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
 _pid(getpid()), _uri(fmk), _callback(log), _timer(fmk, log), _unbind(true), _timeAlive(KEEPALIVE_TIMEOUT),
 _chunkSize(BUFFER_CHUNK), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log), _core(core) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.noInfo(true);
	{
		// Создаём локальный контейнер функций обратного вызова
		fn_t callback(this->_log);
		// Устанавливаем функцию активации ядра сервера
		callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("status", std::bind(&web_t::eventsCallback, this, _1, _2));
		// Выполняем установку функций обратного вызова для сервера
		const_cast <server::core_t *> (this->_core)->callback(std::move(callback));
	}
}
