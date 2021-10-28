/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/rest.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 * @param ctx   передаваемый контекст модуля
 */
void awh::RestServer::chunking(const vector <char> & chunk, const http_t * http, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) ctx;
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (http)->addBody(chunk.data(), chunk.size());
}
/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::RestServer::openCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <coreSrv_t *> (core)->init(wid, web->port, web->host);
		// Выполняем запуск сервера
		core->run(wid);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::RestServer::persistCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((!adj->alive && !web->alive) || adj->close)){
			// Если клиент давно должен был быть отключён, отключаем его
			if(adj->close || !adj->http.isAlive()) core->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = web->fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= MAX_TIME_CONNECT)
					// Завершаем работу
					core->close(aid);
			}
		}
	}
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::RestServer::connectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Создаём адъютанта
		web->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Устанавливаем размер чанка
		adj->http.setChunkSize(web->chunkSize);
		// Устанавливаем данные сервиса
		adj->http.setServ(web->sid, web->name, web->version);
		// Если функция обратного вызова для обработки чанков установлена
		if(web->chunkingFn != nullptr)
			// Устанавливаем внешнюю функцию обработки вызова для получения чанков
			adj->http.setChunkingFn(web->ctx.at(5), web->chunkingFn);
		// Устанавливаем функцию обработки вызова для получения чанков
		else adj->http.setChunkingFn(web, &chunking);
		// Устанавливаем параметры шифрования
		if(web->crypt) adj->http.setCrypt(web->pass, web->salt, web->aes);
		// Если сервер требует авторизацию
		if(web->authType != auth_t::type_t::NONE){
			// Определяем тип авторизации
			switch((uint8_t) web->authType){
				// Если тип авторизации Basic
				case (uint8_t) auth_t::type_t::BASIC: {
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(web->authType);
					// Устанавливаем функцию проверки авторизации
					adj->http.setAuthCallback(web->ctx.at(4), web->checkAuthFn);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->http.setRealm(web->realm);
					// Устанавливаем временный ключ сессии сервера
					adj->http.setOpaque(web->opaque);
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(web->authType, web->authAlg);
					// Устанавливаем функцию извлечения пароля
					adj->http.setExtractPassCallback(web->ctx.at(3), web->extractPassFn);
				} break;
			}
		}
		// Если функция обратного вызова установлена, выполняем
		if(web->openStopFn != nullptr) web->openStopFn(aid, true, web, web->ctx.at(0));
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::RestServer::disconnectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Выполняем удаление параметров адъютанта
		web->worker.removeAdj(aid);
		// Если функция обратного вызова установлена, выполняем
		if(web->openStopFn != nullptr) web->openStopFn(aid, false, web, web->ctx.at(0));
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
bool awh::RestServer::acceptCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(web->acceptFn != nullptr) result = web->acceptFn(ip, mac, web, web->ctx.at(2));
	}
	// Разрешаем подключение клиенту
	return result;
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::RestServer::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Увеличиваем количество выполненных запросов
			adj->requests++;
			// Если количество выполненных запросов превышает максимальный
			if(adj->requests >= web->maxRequests)
				// Устанавливаем флаг закрытия подключения
				adj->close = true;
			// Получаем текущий штамп времени
			else adj->checkPoint = web->fmk->unixTimestamp();
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
						// Объект запроса клиента
						req_t request;
						// Получаем флаг шифрованных данных
						adj->crypt = adj->http.isCrypt();
						// Получаем поддерживаемый метод компрессии
						adj->compress = adj->http.getCompress();
						// Получаем данные запроса
						const auto & query = adj->http.getQuery();
						// Устанавливаем метод запроса
						request.method = query.method;
						// Устанавливаем IP адрес
						request.ip = web->worker.ip(aid);
						// Устанавливаем MAC адрес
						request.mac = web->worker.mac(aid);
						// Получаем тело запроса
						request.entity = adj->http.getBody();
						// Получаем заголовки запроса
						request.headers = adj->http.getHeaders();
						// Выполняем парсинг URL адреса
						const auto & split = web->worker.uri.split(query.uri);
						// Если список параметров получен
						if(!split.empty()){
							// Определяем сколько элементов мы получили
							switch(split.size()){
								// Если количество элементов равно 1
								case 1: {
									// Проверяем путь запроса
									const string & value = split.front();
									// Если первый символ является путём запроса
									if(value.front() == '/') request.path = web->worker.uri.splitPath(value);
									// Если же первый символ является параметром запросов
									else if(value.front() == '?') request.params = web->worker.uri.splitParams(value);
								} break;
								// Если количество элементов равно 2
								case 2: {
									// Устанавливаем путь запроса
									request.path = web->worker.uri.splitPath(split.front());
									// Устанавливаем параметры запроса
									request.params = web->worker.uri.splitParams(split.back());
								} break;
							}
						}
						// Выполняем сброс состояния HTTP парсера
						adj->http.clear();
						// Выполняем сброс состояния HTTP парсера
						adj->http.reset();
						// Если функция обратного вызова установлена, выполняем
						if(web->messageFn != nullptr) web->messageFn(aid, request, web, web->ctx.at(1));
						// Завершаем работу
						return;
					} break;
					// Если запрос неудачный
					case (uint8_t) http_t::stath_t::FAULT: {
						// Выполняем сброс состояния HTTP парсера
						adj->http.clear();
						// Выполняем сброс состояния HTTP парсера
						adj->http.reset();
						// Формируем запрос авторизации
						response = adj->http.reject(401);
					} break;
				}
				// Если бинарные данные запроса получены, отправляем на сервер
				if(!response.empty()){
					// Тело полезной нагрузки
					vector <char> payload;
					// Устанавливаем размер стопбайт
					if(!adj->http.isAlive()) adj->stopBytes = response.size();
					// Отправляем ответ клиенту
					core->write(response.data(), response.size(), aid);
					// Получаем данные тела запроса
					while(!(payload = adj->http.payload()).empty()){
						// Устанавливаем размер стопбайт
						if(!adj->http.isAlive()) adj->stopBytes += payload.size();
						// Отправляем тело на сервер
						core->write(payload.data(), payload.size(), aid);
					}
				// Завершаем работу
				} else core->close(aid);
			}
		}
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::RestServer::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && (adj->stopBytes > 0)){
			// Запоминаем количество прочитанных байт
			adj->readBytes += size;
			// Если размер полученных байт соответствует
			adj->close = (adj->stopBytes >= adj->readBytes);
		}
	}
}
/**
 * init Метод инициализации WebSocket клиента
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::RestServer::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем порт сервера
	this->port = port;
	// Устанавливаем хост сервера
	this->host = host;
	// Устанавливаем тип компрессии
	this->worker.compress = compress;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::RestServer::on(void * ctx, function <void (const size_t, const bool, RestServer *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->openStopFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::RestServer::on(void * ctx, function <void (const size_t, const req_t &, RestServer *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::RestServer::on(void * ctx, function <bool (const string &, const string &, RestServer *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(2) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->acceptFn = callback;
}
/**
 * reject Метод отправки сообщения об ошибке
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для клиента
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::RestServer::reject(const size_t aid, const u_short code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Получаем параметры подключения адъютанта
		workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->http.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->http.setHeaders(headers);
			// Если подключение постоянное
			if(adj->http.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->http.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", MAX_TIME_CONNECT / 1000, this->maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->http.reject(code, mess);
			// Устанавливаем размер стопбайт
			if(!adj->http.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((core_t *) const_cast <coreSrv_t *> (this->core))->write(response.data(), response.size(), aid);
			// Получаем данные тела запроса
			while(!(payload = adj->http.payload()).empty()){
				// Устанавливаем размер стопбайт
				if(!adj->http.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((core_t *) const_cast <coreSrv_t *> (this->core))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * response Метод отправки сообщения клиенту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для клиента
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::RestServer::response(const size_t aid, const u_short code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Получаем параметры подключения адъютанта
		workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->http.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->http.setHeaders(headers);
			// Если подключение постоянное
			if(adj->http.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->http.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", MAX_TIME_CONNECT / 1000, this->maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->http.response(code, mess);
			// Устанавливаем размер стопбайт
			if(!adj->http.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((core_t *) const_cast <coreSrv_t *> (this->core))->write(response.data(), response.size(), aid);
			// Получаем данные тела запроса
			while(!(payload = adj->http.payload()).empty()){
				// Устанавливаем размер стопбайт
				if(!adj->http.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((core_t *) const_cast <coreSrv_t *> (this->core))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::RestServer::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::RestServer::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.mac(aid);
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::RestServer::setAlive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->alive = mode;
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::RestServer::setAlive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	workSrvRest_t::adjp_t * adj = const_cast <workSrvRest_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
}
/**
 * stop Метод остановки клиента
 */
void awh::RestServer::stop() noexcept {
	// Если подключение выполнено
	if(this->core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <coreSrv_t *> (this->core)->stop();
}
/**
 * start Метод запуска клиента
 */
void awh::RestServer::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->core->working())
		// Выполняем запуск биндинга
		const_cast <coreSrv_t *> (this->core)->start();
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::RestServer::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->worker.timeWrite = write;
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::RestServer::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::RestServer::setRealm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->realm = realm;
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::RestServer::setOpaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->opaque = opaque;
}
/**
 * setExtractPassCallback Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::RestServer::setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(3) = ctx;
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->extractPassFn = callback;
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::RestServer::setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(4) = ctx;
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->checkAuthFn = callback;
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::RestServer::setChunkingFn(void * ctx, function <void (const vector <char> &, const http_t *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(5) = ctx;
	// Устанавливаем функцию обратного вызова для получения чанков
	this->chunkingFn = callback;
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::RestServer::setAuthType(const auth_t::type_t type, const auth_t::alg_t alg) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->authAlg = alg;
	// Устанавливаем тип авторизации
	this->authType = type;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::RestServer::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <coreSrv_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <coreSrv_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::RestServer::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * setMaxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::RestServer::setMaxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->maxRequests = max;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::RestServer::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->worker.compress = compress;
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::RestServer::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->sid = id;
	// Устанавливаем название сервера
	this->name = name;
	// Устанавливаем версию сервера
	this->version = ver;
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::RestServer::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем размер шифрования
	this->hash.setAES(aes);
	// Устанавливаем соль шифрования
	this->hash.setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash.setPassword(pass);
	// Устанавливаем флаг шифрования
	if((this->crypt = !pass.empty())){
		// Размер шифрования передаваемых данных
		this->aes = aes;
		// Пароль шифрования передаваемых данных
		this->pass = pass;
		// Соль шифрования передаваемых данных
		this->salt = salt;
	}
}
/**
 * RestServer Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::RestServer::RestServer(const coreSrv_t * core, const fmk_t * fmk, const log_t * log) noexcept : hash(fmk, log), core(core), fmk(fmk), log(log), worker(fmk, log) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readCallback;
	// Устанавливаем функцию записи данных
	this->worker.writeFn = writeCallback;
	// Добавляем событие аццепта клиента
	this->worker.acceptFn = acceptCallback;
	// Устанавливаем функцию персистентного вызова
	this->worker.persistFn = persistCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectCallback;
	// Активируем персистентный запуск для работы пингов
	const_cast <coreSrv_t *> (this->core)->setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	const_cast <coreSrv_t *> (this->core)->add(&this->worker);
	// Устанавливаем интервал персистентного таймера для работы пингов
	const_cast <coreSrv_t *> (this->core)->setPersistInterval(MAX_TIME_CONNECT / 2);
}
