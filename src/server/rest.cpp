/**
 * @file: rest.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <server/rest.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 * @param ctx   передаваемый контекст модуля
 */
void awh::server::Rest::chunking(const vector <char> & chunk, const awh::http_t * http, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) ctx;
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->addBody(chunk.data(), chunk.size());
}
/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::Rest::openCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, web->port, web->host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(wid);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::Rest::persistCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((!adj->alive && !web->alive) || adj->close)){
			// Если клиент давно должен был быть отключён, отключаем его
			if(adj->close || !adj->http.isAlive()) reinterpret_cast <server::core_t *> (core)->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = web->fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= web->keepAlive)
					// Завершаем работу
					reinterpret_cast <server::core_t *> (core)->close(aid);
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
void awh::server::Rest::connectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Создаём адъютанта
		web->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Устанавливаем размер чанка
		adj->http.setChunkSize(web->chunkSize);
		// Устанавливаем данные сервиса
		adj->http.setServ(web->sid, web->name, web->version);
		// Если функция обратного вызова для обработки чанков установлена
		if(web->chunkingFn != nullptr)
			// Устанавливаем внешнюю функцию обработки вызова для получения чанков
			adj->http.setChunkingFn(web->ctx.at(4), web->chunkingFn);
		// Устанавливаем функцию обработки вызова для получения чанков
		else adj->http.setChunkingFn(web, &chunking);
		// Устанавливаем метод компрессии поддерживаемый сервером
		adj->http.setCompress(web->worker.compress);
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
					adj->http.setAuthCallback(web->ctx.at(3), web->checkAuthFn);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->http.setRealm(web->realm);
					// Устанавливаем временный ключ сессии сервера
					adj->http.setOpaque(web->opaque);
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(web->authType, web->authHash);
					// Устанавливаем функцию извлечения пароля
					adj->http.setExtractPassCallback(web->ctx.at(2), web->extractPassFn);
				} break;
			}
		}
		// Если функция обратного вызова установлена, выполняем
		if(web->openStopFn != nullptr) web->openStopFn(aid, mode_t::CONNECT, web, web->ctx.at(0));
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::Rest::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		adj->close = (adj != nullptr);
		// Выполняем удаление параметров адъютанта
		web->worker.removeAdj(aid);
		// Если функция обратного вызова установлена, выполняем
		if(web->openStopFn != nullptr) web->openStopFn(aid, mode_t::DISCONNECT, web, web->ctx.at(0));
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
bool awh::server::Rest::acceptCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(web->acceptFn != nullptr) result = web->acceptFn(ip, mac, web, web->ctx.at(5));
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
void awh::server::Rest::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (web->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Добавляем полученные данные в буфер
			adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
			// Выполняем обработку полученных данных
			while(!adj->close){
				// Выполняем парсинг полученных данных
				size_t bytes = adj->http.parse(adj->buffer.data(), adj->buffer.size());
				// Если все данные получены
				if(adj->http.isEnd()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = adj->http.request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->http.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << web->fmk->format("<body %u>", adj->http.getBody().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Если подключение не установлено как постоянное
					if(!web->alive && !adj->alive){
						// Увеличиваем количество выполненных запросов
						adj->requests++;
						// Если количество выполненных запросов превышает максимальный
						if(adj->requests >= web->maxRequests)
							// Устанавливаем флаг закрытия подключения
							adj->close = true;
						// Получаем текущий штамп времени
						else adj->checkPoint = web->fmk->unixTimestamp();
					// Выполняем сброс количества выполненных запросов
					} else adj->requests = 0;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->http.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) awh::http_t::stath_t::GOOD: {
							// Получаем флаг шифрованных данных
							adj->crypt = adj->http.isCrypt();
							// Получаем поддерживаемый метод компрессии
							adj->compress = adj->http.getCompress();
							// Если функция обратного вызова установлена, выполняем
							if(web->messageFn != nullptr) web->messageFn(aid, &adj->http, web, web->ctx.at(1));
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->http.reset();
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case (uint8_t) awh::http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->http.reset();
							// Выполняем очистку буфера полученных данных
							adj->buffer.clear();
							// Формируем запрос авторизации
							const auto & response = adj->http.reject(401);
							// Если ответ получен
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
							// Выполняем отключение клиента
							} else reinterpret_cast <server::core_t *> (core)->close(aid);
							// Выходим из функции
							return;
						}
					}
				}
				// Устанавливаем метку продолжения обработки пайплайна
				Next:
				// Если парсер обработал какое-то количество байт
				if(bytes > 0){
					// Удаляем количество обработанных байт
					adj->buffer.erase(adj->buffer.begin(), adj->buffer.begin() + bytes);
					// Если данных для обработки не осталось, выходим
					if(adj->buffer.empty()) break;
				// Если данных для обработки недостаточно, выходим
				} else break;
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
void awh::server::Rest::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (web->worker.getAdj(aid));
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
void awh::server::Rest::init(const u_int port, const string & host, const awh::http_t::compress_t compress) noexcept {
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
void awh::server::Rest::on(void * ctx, function <void (const size_t, const mode_t, Rest *, void *)> callback) noexcept {
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
void awh::server::Rest::on(void * ctx, function <void (const size_t, const awh::http_t *, Rest *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Rest::on(void * ctx, function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(2) = ctx;
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->extractPassFn = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Rest::on(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(3) = ctx;
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->checkAuthFn = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::Rest::on(void * ctx, function <void (const vector <char> &, const awh::http_t *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(4) = ctx;
	// Устанавливаем функцию обратного вызова для получения чанков
	this->chunkingFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::Rest::on(void * ctx, function <bool (const string &, const string &, Rest *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(5) = ctx;
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
void awh::server::Rest::reject(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->http.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->http.setHeaders(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->alive && !adj->alive && adj->http.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->http.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", this->keepAlive / 1000, this->maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->http.reject(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем размер стопбайт
			if(!adj->http.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <server::core_t *> (this->core))->write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->http.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Устанавливаем размер стопбайт
				if(!adj->http.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((awh::core_t *) const_cast <server::core_t *> (this->core))->write(payload.data(), payload.size(), aid);
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
void awh::server::Rest::response(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Получаем параметры подключения адъютанта
		workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->http.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->http.setHeaders(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->alive && !adj->alive && adj->http.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->http.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", this->keepAlive / 1000, this->maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->http.response(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем размер стопбайт
			if(!adj->http.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <server::core_t *> (this->core))->write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->http.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Устанавливаем размер стопбайт
				if(!adj->http.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((awh::core_t *) const_cast <server::core_t *> (this->core))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Rest::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Rest::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.mac(aid);
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Rest::setAlive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->alive = mode;
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Rest::setAlive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
}
/**
 * start Метод запуска клиента
 */
void awh::server::Rest::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->core)->start();
}
/**
 * stop Метод остановки клиента
 */
void awh::server::Rest::stop() noexcept {
	// Если подключение выполнено
	if(this->core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <server::core_t *> (this->core)->stop();
}
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
void awh::server::Rest::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	workerRest_t::adjp_t * adj = const_cast <workerRest_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr) adj->close = true;
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Rest::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::Rest::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Rest::setRealm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->realm = realm;
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Rest::setOpaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->opaque = opaque;
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Rest::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->authHash = hash;
	// Устанавливаем тип авторизации
	this->authType = type;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Rest::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <server::core_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <server::core_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Rest::setTotal(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->core)->setTotal(this->worker.wid, total);
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Rest::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * setKeepAlive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Rest::setKeepAlive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->keepAlive = time;
}
/**
 * setMaxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Rest::setMaxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->maxRequests = max;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Rest::setCompress(const awh::http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->worker.compress = compress;
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Rest::setServ(const string & id, const string & name, const string & ver) noexcept {
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
void awh::server::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
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
 * Rest Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Rest::Rest(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : core(core), fmk(fmk), log(log), worker(fmk, log) {
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
	const_cast <server::core_t *> (this->core)->setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	const_cast <server::core_t *> (this->core)->add(&this->worker);
	// Устанавливаем интервал персистентного таймера для работы пингов
	const_cast <server::core_t *> (this->core)->setPersistInterval(KEEPALIVE_TIMEOUT / 2);
}
