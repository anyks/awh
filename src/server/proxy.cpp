/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/proxy.hpp>

/**
 * runCallback Функция обратного вызова при активации ядра сервера
 * @param mode флаг запуска/остановки
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::runCallback(const bool mode, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Выполняем биндинг базы событий для клиента
		if(mode) proxy->coreSrv.bind(reinterpret_cast <core_t *> (&proxy->coreCli));
		// Выполняем анбиндинг базы событий клиента
		else proxy->coreSrv.unbind(reinterpret_cast <core_t *> (&proxy->coreCli));
	}
}
/**
 * chunkingCallback Функция обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 * @param ctx   передаваемый контекст модуля
 */
void awh::ProxyServer::chunkingCallback(const vector <char> & chunk, const http_t * http, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) ctx;
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (http)->addBody(chunk.data(), chunk.size());
}
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::openServerCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <coreSrv_t *> (core)->init(wid, proxy->port, proxy->host);
		// Выполняем запуск сервера
		core->run(wid);
	}
}
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::persistServerCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj->method != web_t::method_t::CONNECT) && (adj != nullptr) && ((!adj->alive && !proxy->alive) || adj->close)){
			// Если клиент давно должен был быть отключён, отключаем его
			if(adj->close || !adj->http.isAlive()) core->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = proxy->fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= proxy->keepAlive)
					// Завершаем работу
					core->close(aid);
			}
		}
	}
}
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::connectClientCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(it->second));
			// Если подключение не выполнено
			if(!adj->connect){
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Если метод подключения CONNECT
				if(adj->method == web_t::method_t::CONNECT){
					// Формируем ответ клиенту
					const auto & response = adj->http.response(200);
					// Если ответ получен
					if(!response.empty()){
						// Тело полезной нагрузки
						vector <char> payload;
						// Отправляем ответ клиенту
						reinterpret_cast <core_t *> (&proxy->coreSrv)->write(response.data(), response.size(), it->second);
						// Получаем данные тела запроса
						while(!(payload = adj->http.payload()).empty()){
							// Отправляем тело на сервер
							reinterpret_cast <core_t *> (&proxy->coreSrv)->write(payload.data(), payload.size(), it->second);
						}
					// Выполняем отключение клиента
					} else reinterpret_cast <core_t *> (&proxy->coreSrv)->close(it->second);
				// Отправляем сообщение на сервер, так-как оно пришло от клиента
				} else {
					// Отправляем ответ клиенту
					reinterpret_cast <core_t *> (&proxy->coreCli)->write(adj->buffer.data(), adj->buffer.size(), aid);
					// Очищаем буфер данных полученный от клиента
					adj->buffer.clear();
				}
			}
		}
	}
}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::connectServerCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Создаём адъютанта
		proxy->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Устанавливаем размер чанка
		adj->http.setChunkSize(proxy->chunkSize);
		// Устанавливаем данные сервиса
		adj->http.setServ(proxy->sid, proxy->name, proxy->version);
		// Если функция обратного вызова для обработки чанков установлена
		if(proxy->chunkingFn != nullptr)
			// Устанавливаем внешнюю функцию обработки вызова для получения чанков
			adj->http.setChunkingFn(proxy->ctx.at(4), proxy->chunkingFn);
		// Устанавливаем функцию обработки вызова для получения чанков
		else adj->http.setChunkingFn(proxy, &chunkingCallback);
		// Устанавливаем параметры шифрования
		if(proxy->crypt) adj->http.setCrypt(proxy->pass, proxy->salt, proxy->aes);
		// Если сервер требует авторизацию
		if(proxy->authType != auth_t::type_t::NONE){
			// Определяем тип авторизации
			switch((uint8_t) proxy->authType){
				// Если тип авторизации Basic
				case (uint8_t) auth_t::type_t::BASIC: {
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(proxy->authType);
					// Устанавливаем функцию проверки авторизации
					adj->http.setAuthCallback(proxy->ctx.at(3), proxy->checkAuthFn);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->http.setRealm(proxy->realm);
					// Устанавливаем временный ключ сессии сервера
					adj->http.setOpaque(proxy->opaque);
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(proxy->authType, proxy->authHash);
					// Устанавливаем функцию извлечения пароля
					adj->http.setExtractPassCallback(proxy->ctx.at(2), proxy->extractPassFn);
				} break;
			}
		}
		// Устанавливаем контекст сообщения
		adj->worker.ctx = proxy;
		// Устанавливаем функцию чтения данных
		adj->worker.readFn = readClientCallback;
		// Устанавливаем событие подключения
		adj->worker.connectFn = connectClientCallback;
		// Устанавливаем событие отключения
		adj->worker.disconnectFn = disconnectClientCallback;
		// Добавляем воркер в биндер TCP/IP
		proxy->coreCli.add(&adj->worker);
		// Создаём пару клиента и сервера
		proxy->worker.pairs.emplace(adj->worker.wid, aid);
		// Если функция обратного вызова установлена, выполняем
		if(proxy->openStopFn != nullptr) proxy->openStopFn(aid, true, proxy, proxy->ctx.at(0));
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::disconnectClientCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			proxy->worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
			// Устанавливаем флаг отключения клиента
			adj->close = true;
			// Выполняем отключение клиента
			reinterpret_cast <core_t *> (&proxy->coreSrv)->close(aid);
			// Выполняем удаление параметров адъютанта
			proxy->worker.removeAdj(aid);
		}
	}
}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::ProxyServer::disconnectServerCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Выполняем отключение клиента от стороннего сервера
		if(adj != nullptr){
			// Устанавливаем флаг отключения клиента
			adj->close = true;
			// Выполняем отключение всех дочерних клиентов
			reinterpret_cast <core_t *> (&proxy->coreCli)->close(adj->worker.getAid());
		}
		// Если функция обратного вызова установлена, выполняем
		if(proxy->openStopFn != nullptr) proxy->openStopFn(aid, false, proxy, proxy->ctx.at(0));
	}
}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 * @return     результат разрешения к подключению клиента
 */
bool awh::ProxyServer::acceptServerCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(proxy->acceptFn != nullptr) result = proxy->acceptFn(ip, mac, proxy, proxy->ctx.at(5));
	}
	// Разрешаем подключение клиенту
	return result;
}
/**
 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::ProxyServer::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if(adj->connect) reinterpret_cast <core_t *> (&proxy->coreSrv)->write(buffer, size, it->second);
		}
	}
}
/**
 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::ProxyServer::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если указан метод CONNECT
			if(adj->connect && (adj->method == web_t::method_t::CONNECT)){
				// Получаем идентификатор адъютанта
				const size_t aid = adj->worker.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0) reinterpret_cast <core_t *> (&proxy->coreCli)->write(buffer, size, aid);
			// Если подключение ещё не выполнено
			} else {
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
							// Получаем заголовки запроса
							const auto & headers = adj->http.getHeaders();
							// Если заголовки получены
							if(!headers.empty()){
								// Данные REST запроса
								string request = "";
								// Получаем данные запроса
								const auto & query = adj->http.getQuery();
								// Определяем метод запроса
								switch((uint8_t) query.method){
									// Если метод запроса указан как GET
									case (uint8_t) web_t::method_t::GET:
										// Формируем GET запрос
										request = proxy->fmk->format("GET %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как PUT
									case (uint8_t) web_t::method_t::PUT:
										// Формируем PUT запрос
										request = proxy->fmk->format("PUT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как POST
									case (uint8_t) web_t::method_t::POST:
										// Формируем POST запрос
										request = proxy->fmk->format("POST %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как HEAD
									case (uint8_t) web_t::method_t::HEAD:
										// Формируем HEAD запрос
										request = proxy->fmk->format("HEAD %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как PATCH
									case (uint8_t) web_t::method_t::PATCH:
										// Формируем PATCH запрос
										request = proxy->fmk->format("PATCH %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как TRACE
									case (uint8_t) web_t::method_t::TRACE:
										// Формируем TRACE запрос
										request = proxy->fmk->format("TRACE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как DELETE
									case (uint8_t) web_t::method_t::DEL:
										// Формируем DELETE запрос
										request = proxy->fmk->format("DELETE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как OPTIONS
									case (uint8_t) web_t::method_t::OPTIONS:
										// Формируем OPTIONS запрос
										request = proxy->fmk->format("OPTIONS %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
									// Если метод запроса указан как CONNECT
									case (uint8_t) web_t::method_t::CONNECT:
										// Формируем CONNECT запрос
										request = proxy->fmk->format("CONNECT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
									break;
								}
								// Переходим по всему списку заголовков
								for(auto & header : headers){
									// Формируем заголовок запроса
									request.append(proxy->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
								}
								// Добавляем разделитель
								request.append("\r\n");
								// Выводим заголовок запроса
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры запроса
								cout << string(request.begin(), request.end()) << endl;
								// Если тело запроса существует
								if(!adj->http.getBody().empty())
									// Выводим сообщение о выводе чанка тела
									cout << proxy->fmk->format("<body %u>", adj->http.getBody().size())  << endl;
							}
						#endif
						// Если подключение не установлено как постоянное
						if(!proxy->alive && !adj->alive){
							// Увеличиваем количество выполненных запросов
							adj->requests++;
							// Если количество выполненных запросов превышает максимальный
							if(adj->requests >= proxy->maxRequests)
								// Устанавливаем флаг закрытия подключения
								adj->close = true;
							// Получаем текущий штамп времени
							else adj->checkPoint = proxy->fmk->unixTimestamp();
						// Выполняем сброс количества выполненных запросов
						} else adj->requests = 0;
						// Выполняем проверку авторизации
						switch((uint8_t) adj->http.getAuth()){
							// Если запрос выполнен удачно
							case (uint8_t) http_t::stath_t::GOOD: {
								// Получаем флаг шифрованных данных
								adj->crypt = adj->http.isCrypt();
								// Получаем поддерживаемый метод компрессии
								adj->compress = adj->http.getCompress();
								// Если подключение не выполнено
								if(!adj->connect){
									// Получаем хост сервера
									const auto & host = adj->http.getHeader("host");
									// Если хост сервера получен
									if(!host.empty()){
										// Получаем данные запроса
										const auto & query = adj->http.getQuery();
										// Запоминаем метод подключения
										adj->method = query.method;
										// Формируем адрес подключения
										adj->worker.url = proxy->worker.uri.parseUrl(proxy->fmk->format("http://%s", host.c_str()));
										// Выполняем запрос на сервер
										proxy->coreCli.open(adj->worker.wid);
										// Выполняем сброс состояния HTTP парсера
										adj->http.clear();
										// Выполняем сброс состояния HTTP парсера
										adj->http.reset();
										// Выходим из функции
										return;
									}
								// Если подключение выполнено
								} else {
									// Получаем идентификатор адъютанта
									const size_t aid = adj->worker.getAid();
									// Отправляем запрос на внешний сервер
									if(aid > 0) reinterpret_cast <core_t *> (&proxy->coreCli)->write(buffer, size, aid);
								}
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->http.reset();


								/*
								// Устанавливаем метод запроса
								request.method = query.method;
								// Устанавливаем IP адрес
								request.ip = proxy->worker.ip(aid);
								// Устанавливаем MAC адрес
								request.mac = proxy->worker.mac(aid);
								// Получаем тело запроса
								request.entity = adj->http.getBody();
								// Получаем заголовки запроса
								request.headers = adj->http.getHeaders();
								// Выполняем парсинг URL адреса
								const auto & split = proxy->worker.uri.split(query.uri);
								// Если список параметров получен
								if(!split.empty()){
									// Определяем сколько элементов мы получили
									switch(split.size()){
										// Если количество элементов равно 1
										case 1: {
											// Проверяем путь запроса
											const string & value = split.front();
											// Если первый символ является путём запроса
											if(value.front() == '/') request.path = proxy->worker.uri.splitPath(value);
											// Если же первый символ является параметром запросов
											else if(value.front() == '?') request.params = proxy->worker.uri.splitParams(value);
										} break;
										// Если количество элементов равно 2
										case 2: {
											// Устанавливаем путь запроса
											request.path = proxy->worker.uri.splitPath(split.front());
											// Устанавливаем параметры запроса
											request.params = proxy->worker.uri.splitParams(split.back());
										} break;
									}
								}
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->http.reset();
								// Если функция обратного вызова установлена, выполняем
								if(proxy->messageFn != nullptr) proxy->messageFn(aid, request, proxy, proxy->ctx.at(1));
								*/
								// Завершаем обработку
								goto Next;
							} break;
							// Если запрос неудачный
							case (uint8_t) http_t::stath_t::FAULT: {
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->http.reset();
								// Выполняем очистку буфера полученных данных
								adj->buffer.clear();
								// Формируем запрос авторизации
								const auto & response = adj->http.reject(407);
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
								} else core->close(aid);
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

	/*
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restSrv_t * web = reinterpret_cast <restSrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (web->worker.getAdj(aid));
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
						// Получаем заголовки запроса
						const auto & headers = adj->http.getHeaders();
						// Если заголовки получены
						if(!headers.empty()){
							// Данные REST запроса
							string request = "";
							// Получаем данные запроса
							const auto & query = adj->http.getQuery();
							// Определяем метод запроса
							switch((uint8_t) query.method){
								// Если метод запроса указан как GET
								case (uint8_t) web_t::method_t::GET:
									// Формируем GET запрос
									request = web->fmk->format("GET %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как PUT
								case (uint8_t) web_t::method_t::PUT:
									// Формируем PUT запрос
									request = web->fmk->format("PUT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как POST
								case (uint8_t) web_t::method_t::POST:
									// Формируем POST запрос
									request = web->fmk->format("POST %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как HEAD
								case (uint8_t) web_t::method_t::HEAD:
									// Формируем HEAD запрос
									request = web->fmk->format("HEAD %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как PATCH
								case (uint8_t) web_t::method_t::PATCH:
									// Формируем PATCH запрос
									request = web->fmk->format("PATCH %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как TRACE
								case (uint8_t) web_t::method_t::TRACE:
									// Формируем TRACE запрос
									request = web->fmk->format("TRACE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как DELETE
								case (uint8_t) web_t::method_t::DEL:
									// Формируем DELETE запрос
									request = web->fmk->format("DELETE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как OPTIONS
								case (uint8_t) web_t::method_t::OPTIONS:
									// Формируем OPTIONS запрос
									request = web->fmk->format("OPTIONS %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
								// Если метод запроса указан как CONNECT
								case (uint8_t) web_t::method_t::CONNECT:
									// Формируем CONNECT запрос
									request = web->fmk->format("CONNECT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
								break;
							}
							// Переходим по всему списку заголовков
							for(auto & header : headers){
								// Формируем заголовок запроса
								request.append(web->fmk->format("%s: %s\r\n", header.first.c_str(), header.second.c_str()));
							}
							// Добавляем разделитель
							request.append("\r\n");
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->http.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << web->fmk->format("<body %u>", adj->http.getBody().size())  << endl;
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
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
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
							} else core->close(aid);
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
	*/
}
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::ProxyServer::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxySrv_t * proxy = reinterpret_cast <proxySrv_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
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
void awh::ProxyServer::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
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
void awh::ProxyServer::on(void * ctx, function <void (const size_t, const bool, ProxyServer *, void *)> callback) noexcept {
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
void awh::ProxyServer::on(void * ctx, function <void (const size_t, const req_t &, ProxyServer *, void *)> callback) noexcept {
	/*
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
	*/
}
/**
 * on Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::ProxyServer::on(void * ctx, function <string (const string &, void *)> callback) noexcept {
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
void awh::ProxyServer::on(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
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
void awh::ProxyServer::on(void * ctx, function <void (const vector <char> &, const http_t *, void *)> callback) noexcept {
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
void awh::ProxyServer::on(void * ctx, function <bool (const string &, const string &, ProxyServer *, void *)> callback) noexcept {
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
void awh::ProxyServer::reject(const size_t aid, const u_short code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->coreSrv.working()){
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (this->worker.getAdj(aid));
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
			((core_t *) const_cast <coreSrv_t *> (&this->coreSrv))->write(response.data(), response.size(), aid);
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
				((core_t *) const_cast <coreSrv_t *> (&this->coreSrv))->write(payload.data(), payload.size(), aid);
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
void awh::ProxyServer::response(const size_t aid, const u_short code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->coreSrv.working()){
		// Получаем параметры подключения адъютанта
		workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (this->worker.getAdj(aid));
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
			((core_t *) const_cast <coreSrv_t *> (&this->coreSrv))->write(response.data(), response.size(), aid);
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
				((core_t *) const_cast <coreSrv_t *> (&this->coreSrv))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::ProxyServer::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::ProxyServer::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.mac(aid);
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::ProxyServer::setAlive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->alive = mode;
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::ProxyServer::setAlive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
}
/**
 * start Метод запуска клиента
 */
void awh::ProxyServer::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->coreSrv.working())
		// Выполняем запуск биндинга
		this->coreSrv.start();
}
/**
 * stop Метод остановки клиента
 */
void awh::ProxyServer::stop() noexcept {
	// Если подключение выполнено
	if(this->coreSrv.working())
		// Завершаем работу, если разрешено остановить
		this->coreSrv.stop();
}
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
void awh::ProxyServer::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	workSrvProxy_t::adjp_t * adj = const_cast <workSrvProxy_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr) adj->close = true;
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::ProxyServer::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::ProxyServer::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::ProxyServer::setRealm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->realm = realm;
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::ProxyServer::setOpaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->opaque = opaque;
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::ProxyServer::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->authHash = hash;
	// Устанавливаем тип авторизации
	this->authType = type;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::ProxyServer::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	this->coreCli.setDefer(flag & (uint8_t) flag_t::DEFER);
	this->coreSrv.setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->coreCli.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	this->coreSrv.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::ProxyServer::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * setKeepAlive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::ProxyServer::setKeepAlive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->keepAlive = time;
}
/**
 * setMaxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::ProxyServer::setMaxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->maxRequests = max;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::ProxyServer::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->worker.compress = compress;
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::ProxyServer::setServ(const string & id, const string & name, const string & ver) noexcept {
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
void awh::ProxyServer::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
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
 * ProxyServer Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::ProxyServer::ProxyServer(const fmk_t * fmk, const log_t * log) noexcept : coreCli(fmk, log), coreSrv(fmk, log), worker(fmk, log), hash(fmk, log), fmk(fmk), log(log) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openServerCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readServerCallback;
	// Устанавливаем функцию записи данных
	this->worker.writeFn = writeServerCallback;
	// Добавляем событие аццепта клиента
	this->worker.acceptFn = acceptServerCallback;
	// Устанавливаем функцию персистентного вызова
	this->worker.persistFn = persistServerCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectServerCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectServerCallback;
	// Активируем персистентный запуск для работы пингов
	this->coreSrv.setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	this->coreSrv.add(&this->worker);
	// Устанавливаем функцию активации ядра сервера
	this->coreSrv.setCallback(this, &runCallback);
	// Устанавливаем интервал персистентного таймера для работы пингов
	this->coreSrv.setPersistInterval(KEEPALIVE_TIMEOUT / 2);
}
