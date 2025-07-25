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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <server/web/web.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * openEvents Метод обратного вызова при запуске работы
 * @param sid идентификатор схемы сети
 */
void awh::server::Web::openEvents(const uint16_t sid) noexcept {
	// Если данные существуют
	if(sid > 0){
		// Устанавливаем хост сервера
		const_cast <server::core_t *> (this->_core)->init(sid, this->_service.port, this->_service.host);
		// Выполняем запуск сервера
		const_cast <server::core_t *> (this->_core)->launch(sid);
	}
}
/**
 * statusEvents Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::server::Web::statusEvents(const awh::core_t::status_t status) noexcept {
	// Если режим работы кластера не активирован
	if(this->_core->cluster() == awh::scheme_t::mode_t::DISABLED){
		// Определяем статус активности сетевого ядра
		switch(static_cast <uint8_t> (status)){
			// Если система запущена
			case static_cast <uint8_t> (awh::core_t::status_t::START): {
				// Выполняем биндинг ядра локального таймера
				const_cast <server::core_t *> (this->_core)->bind(&this->_timer);
				// Если разрешено выполнять пинги
				if(this->_pinging){
					// Устанавливаем интервал времени на выполнения пинга клиента
					const uint16_t tid = this->_timer.interval(this->_pingInterval);
					// Выполняем добавление функции обратного вызова
					this->_timer.on(tid, &web_t::pinging, this, tid);
				}
				// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 3 секунды
				const uint16_t tid = this->_timer.interval(3000);
				// Выполняем добавление функции обратного вызова
				this->_timer.on(tid, &web_t::disconected, this, tid);
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
				// Останавливаем все установленные таймеры
				this->_timer.clear();
				// Выполняем анбиндинг ядра локального таймера
				const_cast <server::core_t *> (this->_core)->unbind(&this->_timer);
			} break;
		}
	}
	// Если функция получения событий запуска и остановки сетевого ядра установлена
	if(this->_callback.is("status"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const awh::core_t::status_t)> ("status", status);
}
/**
 * launchedEvents Метод получения события запуска сервера
 * @param host хост запущенного сервера
 * @param port порт запущенного сервера
 */
void awh::server::Web::launchedEvents(const string & host, const uint32_t port) noexcept {
	// Если параметра активации сервера переданы
	if(!host.empty()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("launched"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const string &, const uint32_t)> ("launched", host, port);
	}
}
/**
 * acceptEvents Метод обратного вызова при проверке подключения брокера
 * @param ip   адрес интернет подключения брокера
 * @param mac  мак-адрес подключившегося брокера
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @return     результат разрешения к подключению брокера
 */
bool awh::server::Web::acceptEvents(const string & ip, const string & mac, const uint32_t port, const uint16_t sid) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0)){
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выполняем функцию обратного вызова
			return this->_callback.call <bool (const string &, const string &, const uint32_t)> ("accept", ip, mac, port);
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
 * callbackEvents Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param fid   идентификатор функции обратного вызова
 * @param fn    функция обратного вызова в чистом виде
 */
void awh::server::Web::callbackEvents([[maybe_unused]] const callback_t::event_t event, [[maybe_unused]] const uint64_t fid, [[maybe_unused]] const callback_t::fn_t & fn) noexcept {}
/**
 * clusterEvents Метод вывода статуса кластера
 * @param family флаг семейства кластера
 * @param sid    идентификатор схемы сети
 * @param pid    идентификатор процесса
 * @param event  идентификатор события
 */
void awh::server::Web::clusterEvents(const cluster_t::family_t family, const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept {
	// Определяем полученное событие
	switch(static_cast <uint8_t> (event)){
		// Если событие запуска сервиса
		case static_cast <uint8_t> (cluster_t::event_t::START): {
			// Если событие прислал дочерний процесс
			if(family == cluster_t::family_t::CHILDREN){
				// Выполняем биндинг ядра локального таймера
				const_cast <server::core_t *> (this->_core)->bind(&this->_timer);
				// Если разрешено выполнять пинги
				if(this->_pinging){
					// Устанавливаем интервал времени на выполнения пинга клиента
					const uint16_t tid = this->_timer.interval(this->_pingInterval);
					// Выполняем добавление функции обратного вызова
					this->_timer.on(tid, &web_t::pinging, this, tid);
				}
				// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 3 секунды
				const uint16_t tid = this->_timer.interval(3000);
				// Выполняем добавление функции обратного вызова
				this->_timer.on(tid, &web_t::disconected, this, tid);
			}
		} break;
		// Если событие остановки сервиса
		case static_cast <uint8_t> (cluster_t::event_t::STOP): {
			// Если событие прислал дочерний процесс
			if(family == cluster_t::family_t::CHILDREN){
				// Останавливаем все установленные таймеры
				this->_timer.clear();
				// Выполняем анбиндинг ядра локального таймера
				const_cast <server::core_t *> (this->_core)->unbind(&this->_timer);
			}
		} break;
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("cluster"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("cluster", family, sid, pid, event);
}
/**
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::Web::erase(const uint64_t bid) noexcept {
	// Если список отключившихся клиентов не пустой
	if(!this->_disconected.empty()){
		// Получаем текущее значение времени
		const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
		// Если идентификатор брокера передан
		if(bid > 0){
			// Выполняем поиск указанного брокера
			auto i = this->_disconected.find(bid);
			// Если данные отключившегося брокера найдены
			if((i != this->_disconected.end()) && ((date - i->second) >= 10000))
				// Выполняем удаление брокера
				this->_disconected.erase(i);
		// Если идентификатор брокера не передан
		} else {
			// Выполняем переход по всему списку отключившихся брокеров
			for(auto i = this->_disconected.begin(); i != this->_disconected.end();){
				// Если брокер уже давно отключился
				if((date - i->second) >= 10000)
					// Выполняем удаление объекта брокеров из списка отключившихся
					i = this->_disconected.erase(i);
				// Выполняем пропуск брокера
				else ++i;
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
	this->_disconected.emplace(bid, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS));
}
/**
 * disconected Метод удаления отключившихся брокеров
 * @param tid идентификатор таймера
 */
void awh::server::Web::disconected(const uint16_t tid) noexcept {
	// Выполняем удаление отключившихся брокеров
	this->erase();
}
/**
 * init Метод инициализации WEB брокера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Web::init([[maybe_unused]] const string & socket, [[maybe_unused]] const vector <http_t::compressor_t> & compressors) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если объект сетевого ядра создан
		if(this->_core != nullptr)
			// Выполняем установку unix-сокет
			const_cast <server::core_t *> (this->_core)->sockname(socket);
	#endif
}
/**
 * init Метод инициализации WEB брокера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Web::init(const uint32_t port, const string & host, [[maybe_unused]] const vector <http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем порт сервера
	this->_service.port = port;
	// Устанавливаем хост сервера
	this->_service.host = host;
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::server::Web::callback(const callback_t & callback) noexcept {
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
	this->_callback.set("status", callback);
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
	// Выполняем установку функции завершения выполнения запроса
	this->_callback.set("complete", callback);
	// Выполняем установку функции обратного вызова для выполнения события запуска сервера
	this->_callback.set("launched", callback);
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
 * proto Метод извлечения поддерживаемого протокола подключения
 * @param bid идентификатор брокера
 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::server::Web::proto(const uint64_t bid) const noexcept {
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Выводим идентификатор активного HTTP-протокола
		return this->_core->proto(bid);
	// Выводим протокол по умолчанию
	return engine_t::proto_t::NONE;
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
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Web::core(const server::core_t * core) noexcept {
	// Выполняем установку объекта сетевого ядра
	this->_core = core;
	// Если объект сетевого ядра передан
	if(this->_core != nullptr){
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on <void (const awh::core_t::status_t)> ("status", &web_t::statusEvents, this, _1);
		// Устанавливаем функцию обратного вызова на перехват событий кластера
		const_cast <server::core_t *> (this->_core)->on <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("cluster", &web_t::clusterEvents, this, _1, _2, _3, _4);
	}
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Web::stop() noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Если завершить работу разрешено
		if(this->_complete && (this->_core != nullptr))
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
	this->_chunkSize = (size > 0 ? size : AWH_CHUNK_SIZE);
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
 _pid(::getpid()),
 _uri(fmk, log), _callback(log), _timer(fmk, log),
 _pinging(true), _complete(true),
 _chunkSize(AWH_CHUNK_SIZE), _pingInterval(PING_INTERVAL),
 _fmk(fmk), _log(log), _core(nullptr) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.verbose(false);
	// Выполняем активацию ловушки событий контейнера функций обратного вызова
	this->_callback.on(std::bind(&web_t::callbackEvents, this, _1, _2, _3));
}
/**
 * Web Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Web::Web(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()),
 _uri(fmk, log), _callback(log), _timer(fmk, log),
 _pinging(true), _complete(true),
 _chunkSize(AWH_CHUNK_SIZE), _pingInterval(PING_INTERVAL),
 _fmk(fmk), _log(log), _core(core) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.verbose(false);
	// Выполняем активацию ловушки событий контейнера функций обратного вызова
	this->_callback.on(std::bind(&web_t::callbackEvents, this, _1, _2, _3));
	// Устанавливаем функцию активации ядра сервера
	const_cast <server::core_t *> (this->_core)->on <void (const awh::core_t::status_t)> ("status", &web_t::statusEvents, this, _1);
	// Устанавливаем функцию получения события запуска сервера
	const_cast <server::core_t *> (this->_core)->on <void (const string &, const uint32_t)> ("launched", &web_t::launchedEvents, this, _1, _2);
	// Устанавливаем функцию обратного вызова на перехват событий кластера
	const_cast <server::core_t *> (this->_core)->on <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("cluster", &web_t::clusterEvents, this, _1, _2, _3, _4);
}
