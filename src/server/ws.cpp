/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/ws.hpp>

/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::openCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Устанавливаем хост сервера
	reinterpret_cast <coreSrv_t *> (core)->init(wid, 2222, "127.0.0.1");
	// Выполняем запуск сервера
	core->run(wid);
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::connectCallback(const size_t aid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Создаём адъютанта
		ws->worker.createAdj(aid);
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketServer::disconnectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Выполняем удаление параметров адъютанта
		ws->worker.removeAdj(aid);
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::WebSocketServer::readCallback(const char * buffer, const size_t size, const size_t aid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0)){
		// Получаем контекст модуля
		wsSrv_t * ws = reinterpret_cast <wsSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvWss_t::adjp_t * adj = const_cast <workSrvWss_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если рукопожатие не выполнено
			if(!reinterpret_cast <http_t *> (&adj->http)->isHandshake()){
				// Выполняем парсинг полученных данных
				adj->http.parse(buffer, size);
				// Если все данные получены
				if(adj->http.isEnd()){
					// Бинарный буфер ответа сервера
					vector <char> response;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->http.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Если рукопожатие выполнено
							if(adj->http.isHandshake()){
								// Проверяем версию протокола
								if(!adj->http.checkVer()){
									// Выполняем сброс состояния HTTP парсера
									adj->http.clear();
									// Получаем бинарные данные REST запроса
									response = adj->http.reject(400, "Unsupported protocol version");
									// Завершаем работу
									break;
								}
								// Проверяем ключ клиента
								if(!adj->http.checkKey()){
									// Выполняем сброс состояния HTTP парсера
									adj->http.clear();
									// Получаем бинарные данные REST запроса
									response = adj->http.reject(400, "Wrong client key");
									// Завершаем работу
									break;
								}
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Получаем флаг шифрованных данных
								adj->crypt = adj->http.isCrypt();
								// Получаем поддерживаемый метод компрессии
								adj->compress = adj->http.getCompress();
								// Получаем бинарные данные REST запроса
								response = adj->http.response();
								// Если бинарные данные запроса получены, отправляем на сервер
								if(!response.empty()) core->write(response.data(), response.size(), aid);
								// Завершаем работу
								return;
							// Сообщаем, что рукопожатие не выполнено
							} else {
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Формируем ответ, что страница не найдена
								response = adj->http.reject(404);
							}
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Формируем запрос авторизации
							response = adj->http.reject(401);
						} break;
					}
					// Если бинарные данные запроса получены, отправляем на сервер
					if(!response.empty()) core->write(response.data(), response.size(), aid);
					// Завершаем работу
					core->close(aid);
				}
				// Завершаем работу
				return;
			// Если рукопожатие выполнено
			} else {
				cout << " +++++++++++++++++2 " << string(buffer, size) << endl;
			}
		}
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 * @return     результат разрешения к подключению клиента
 */
bool awh::WebSocketServer::acceptCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept {
	// Разрешаем подключение клиенту
	return true;
}
/**
 * stop Метод остановки клиента
 */
void awh::WebSocketServer::stop() noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Завершаем работу, если разрешено остановить
		const_cast <coreSrv_t *> (this->core)->stop();
		// Если функция обратного вызова установлена, выполняем
		// if(this->openStopFn != nullptr) this->openStopFn(false, this, this->ctx.at(0));
	}
}
/**
 * start Метод запуска клиента
 */
void awh::WebSocketServer::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->core->working())
		// Выполняем запуск биндинга
		const_cast <coreSrv_t *> (this->core)->start();
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::WebSocketServer::setSub(const string & sub) noexcept {

}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::WebSocketServer::setSubs(const vector <string> & subs) noexcept {

}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::WebSocketServer::setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept {

}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::WebSocketServer::setWaitTimeDetect(const time_t read, const time_t write) noexcept {

}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::WebSocketServer::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {

}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::WebSocketServer::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->noinfo = (flag & (uint8_t) flag_t::NOINFO);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <coreSrv_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <coreSrv_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::WebSocketServer::setChunkSize(const size_t size) noexcept {

}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::WebSocketServer::setFrameSize(const size_t size) noexcept {

}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::WebSocketServer::setCompress(const http_t::compress_t compress) noexcept {

}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::WebSocketServer::setServ(const string & id, const string & name, const string & ver) noexcept {

}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::WebSocketServer::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {

}
/**
 * WebSocketServer Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::WebSocketServer::WebSocketServer(const coreSrv_t * core, const fmk_t * fmk, const log_t * log) noexcept : hash(fmk, log), frame(fmk, log), core(core), fmk(fmk), log(log), worker(fmk, log) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readCallback;
	// Добавляем событие аццепта клиента
	this->worker.acceptFn = acceptCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectCallback;
	// Добавляем воркер в биндер TCP/IP
	const_cast <coreSrv_t *> (this->core)->add(&this->worker);
}
