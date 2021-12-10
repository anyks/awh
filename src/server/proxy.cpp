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
void awh::server::Proxy::runCallback(const bool mode, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Выполняем биндинг базы событий для клиента
		if(mode) proxy->core.server.bind(reinterpret_cast <awh::core_t *> (&proxy->core.client));
		// Выполняем анбиндинг базы событий клиента
		else proxy->core.server.unbind(reinterpret_cast <awh::core_t *> (&proxy->core.client));
	}
}
/**
 * chunkingCallback Функция обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 * @param ctx   передаваемый контекст модуля
 */
void awh::server::Proxy::chunkingCallback(const vector <char> & chunk, const http_t * http, void * ctx) noexcept {
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
void awh::server::Proxy::openServerCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, proxy->port, proxy->host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(wid);
	}
}
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::Proxy::persistServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((adj->method != web_t::method_t::CONNECT) || adj->close) && ((!adj->alive && !proxy->alive) || adj->close)){
			// Если клиент давно должен был быть отключён, отключаем его
			if(adj->close || !adj->srv.isAlive()) proxy->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = proxy->fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= proxy->keepAlive)
					// Завершаем работу
					proxy->close(aid);
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
void awh::server::Proxy::connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(it->second));
			// Если подключение не выполнено
			if(!adj->connect){
				// Разрешаем обработки данных
				adj->locked = false;
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Выполняем сброс состояния HTTP парсера
				adj->srv.clear();
				// Выполняем сброс состояния HTTP парсера
				adj->srv.reset();
				// Если метод подключения CONNECT
				if(adj->method == web_t::method_t::CONNECT){
					// Формируем ответ клиенту
					const auto & response = adj->srv.response((u_int) 200);
					// Если ответ получен
					if(!response.empty()){
						// Тело полезной нагрузки
						vector <char> payload;
						// Отправляем ответ клиенту
						reinterpret_cast <awh::core_t *> (&proxy->core.server)->write(response.data(), response.size(), it->second);
						// Получаем данные тела запроса
						while(!(payload = adj->srv.payload()).empty()){
							// Отправляем тело на сервер
							reinterpret_cast <awh::core_t *> (&proxy->core.server)->write(payload.data(), payload.size(), it->second);
						}
					// Выполняем отключение клиента
					} else proxy->close(it->second);
				// Отправляем сообщение на сервер, так-как оно пришло от клиента
				} else proxy->prepare(it->second, proxy->worker.wid, reinterpret_cast <awh::core_t *> (&proxy->core.server));
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
void awh::server::Proxy::connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Создаём адъютанта
		proxy->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Устанавливаем размер чанка
		adj->cli.setChunkSize(proxy->chunkSize);
		adj->srv.setChunkSize(proxy->chunkSize);
		// Устанавливаем данные сервиса
		adj->cli.setServ(proxy->sid, proxy->name, proxy->version);
		adj->srv.setServ(proxy->sid, proxy->name, proxy->version);
		// Если функция обратного вызова для обработки чанков установлена
		if(proxy->chunkingFn != nullptr)
			// Устанавливаем внешнюю функцию обработки вызова для получения чанков
			adj->cli.setChunkingFn(proxy->ctx.at(5), proxy->chunkingFn);
		// Устанавливаем функцию обработки вызова для получения чанков
		else adj->cli.setChunkingFn(proxy, &chunkingCallback);
		// Устанавливаем функцию обработки вызова для получения чанков
		adj->srv.setChunkingFn(proxy, &chunkingCallback);
		// Если данные будем передавать в зашифрованном виде
		if(proxy->crypt){
			// Устанавливаем параметры шифрования
			adj->cli.setCrypt(proxy->pass, proxy->salt, proxy->aes);
			adj->srv.setCrypt(proxy->pass, proxy->salt, proxy->aes);
		}
		// Если сервер требует авторизацию
		if(proxy->authType != auth_t::type_t::NONE){
			// Определяем тип авторизации
			switch((uint8_t) proxy->authType){
				// Если тип авторизации Basic
				case (uint8_t) auth_t::type_t::BASIC: {
					// Устанавливаем параметры авторизации
					adj->srv.setAuthType(proxy->authType);
					// Устанавливаем функцию проверки авторизации
					adj->srv.setAuthCallback(proxy->ctx.at(4), proxy->checkAuthFn);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->srv.setRealm(proxy->realm);
					// Устанавливаем временный ключ сессии сервера
					adj->srv.setOpaque(proxy->opaque);
					// Устанавливаем параметры авторизации
					adj->srv.setAuthType(proxy->authType, proxy->authHash);
					// Устанавливаем функцию извлечения пароля
					adj->srv.setExtractPassCallback(proxy->ctx.at(3), proxy->extractPassFn);
				} break;
			}
		}
		// Устанавливаем контекст сообщения
		adj->worker.ctx = proxy;
		// Устанавливаем флаг ожидания входящих сообщений
		adj->worker.wait = proxy->worker.wait;
		// Устанавливаем количество секунд на чтение
		adj->worker.timeRead = proxy->worker.timeRead;
		// Устанавливаем количество секунд на запись
		adj->worker.timeWrite = proxy->worker.timeWrite;
		// Устанавливаем функцию чтения данных
		adj->worker.readFn = readClientCallback;
		// Устанавливаем событие подключения
		adj->worker.connectFn = connectClientCallback;
		// Устанавливаем событие отключения
		adj->worker.disconnectFn = disconnectClientCallback;
		// Добавляем воркер в биндер TCP/IP
		proxy->core.client.add(&adj->worker);
		// Создаём пару клиента и сервера
		proxy->worker.pairs.emplace(adj->worker.wid, aid);
		// Если функция обратного вызова установлена, выполняем
		if(proxy->openStopFn != nullptr) proxy->openStopFn(aid, mode_t::CONNECT, proxy, proxy->ctx.at(0));
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::Proxy::disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			proxy->worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
			// Если подключение не выполнено, отправляем ответ клиенту
			if(!adj->connect)
				// Выполняем реджект
				proxy->reject(aid, 404);
			// Устанавливаем флаг отключения клиента
			else adj->close = true;
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
void awh::server::Proxy::disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Выполняем отключение клиента от стороннего сервера
		if(adj != nullptr) adj->close = true;
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
bool awh::server::Proxy::acceptServerCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(proxy->acceptFn != nullptr) result = proxy->acceptFn(ip, mac, proxy, proxy->ctx.at(6));
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
void awh::server::Proxy::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if(adj->connect){
				// Если указан метод не CONNECT и функция обработки сообщения установлена
				if((proxy->messageFn != nullptr) && (adj->method != web_t::method_t::CONNECT)){
					// Добавляем полученные данные в буфер
					adj->client.insert(adj->client.end(), buffer, buffer + size);
					// Выполняем обработку полученных данных
					while(!adj->close && !adj->client.empty()){
						// Выполняем парсинг полученных данных
						size_t bytes = adj->cli.parse(adj->client.data(), adj->client.size());
						// Если все данные получены
						if(adj->cli.isEnd()){
							// Получаем параметры запроса
							const auto & query = adj->cli.getQuery();
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								// Получаем данные ответа
								const auto & response = adj->cli.response(true);
								// Если параметры ответа получены
								if(!response.empty()){
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
									// Если тело ответа существует
									if(!adj->cli.getBody().empty())
										// Выводим сообщение о выводе чанка тела
										cout << proxy->fmk->format("<body %u>", adj->cli.getBody().size())  << endl;
								}
							#endif
							// Выводим сообщение
							if(proxy->messageFn(it->second, event_t::RESPONSE, &adj->cli, proxy, proxy->ctx.at(1))){
								// Получаем данные ответа
								const auto & response = adj->cli.response();
								// Если данные ответа получены
								if(!response.empty()){
									// Тело REST сообщения
									vector <char> entity;
									// Отправляем ответ клиенту
									reinterpret_cast <awh::core_t *> (&proxy->core.server)->write(response.data(), response.size(), it->second);
									// Получаем данные тела ответа
									while(!(entity = adj->cli.payload()).empty()){
										// Отправляем тело клиенту
										reinterpret_cast <awh::core_t *> (&proxy->core.server)->write(entity.data(), entity.size(), it->second);
									}
								}
							}
						}
						// Устанавливаем метку продолжения обработки пайплайна
						Next:
						// Если парсер обработал какое-то количество байт
						if(bytes > 0){
							// Удаляем количество обработанных байт
							adj->client.erase(adj->client.begin(), adj->client.begin() + bytes);
							// Если данных для обработки не осталось, выходим
							if(adj->client.empty()) break;
						// Если данных для обработки недостаточно, выходим
						} else break;
					}
				// Передаём данные так-как они есть
				} else {
					// Если функция обратного вызова установлена, выполняем
					if(proxy->binaryFn != nullptr){
						// Выводим сообщение
						if(proxy->binaryFn(it->second, event_t::RESPONSE, buffer, size, proxy, proxy->ctx.at(2)))
							// Отправляем ответ клиенту
							reinterpret_cast <awh::core_t *> (&proxy->core.server)->write(buffer, size, it->second);
					// Отправляем ответ клиенту
					} else reinterpret_cast <awh::core_t *> (&proxy->core.server)->write(buffer, size, it->second);
				}
			}
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
void awh::server::Proxy::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если указан метод CONNECT
			if(adj->connect && (adj->method == web_t::method_t::CONNECT)){
				// Получаем идентификатор адъютанта
				const size_t aid = adj->worker.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(proxy->binaryFn != nullptr){
						// Выводим сообщение
						if(proxy->binaryFn(aid, event_t::REQUEST, buffer, size, proxy, proxy->ctx.at(2)))
							// Отправляем запрос на внешний сервер
							reinterpret_cast <awh::core_t *> (&proxy->core.client)->write(buffer, size, aid);
					// Отправляем запрос на внешний сервер
					} else reinterpret_cast <awh::core_t *> (&proxy->core.client)->write(buffer, size, aid);
				}
			// Если подключение ещё не выполнено
			} else {
				// Добавляем полученные данные в буфер
				adj->server.insert(adj->server.end(), buffer, buffer + size);
				// Выполняем обработку полученных данных
				proxy->prepare(aid, wid, core);
			}
		}
	}
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
void awh::server::Proxy::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->worker.getAdj(aid));
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
 * prepare Метод обработки входящих данных
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::prepare(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && !adj->locked){
			// Выполняем обработку полученных данных
			while(!adj->close && !adj->server.empty()){
				// Выполняем парсинг полученных данных
				size_t bytes = adj->srv.parse(adj->server.data(), adj->server.size());
				// Если все данные получены
				if(adj->srv.isEnd()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = adj->srv.request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->srv.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->fmk->format("<body %u>", adj->srv.getBody().size())  << endl;
						}
					#endif
					// Если подключение не установлено как постоянное
					if(!this->alive && !adj->alive){
						// Увеличиваем количество выполненных запросов
						adj->requests++;
						// Если количество выполненных запросов превышает максимальный
						if(adj->requests >= this->maxRequests)
							// Устанавливаем флаг закрытия подключения
							adj->close = true;
						// Получаем текущий штамп времени
						else adj->checkPoint = this->fmk->unixTimestamp();
					// Выполняем сброс количества выполненных запросов
					} else adj->requests = 0;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->srv.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Получаем флаг шифрованных данных
							adj->crypt = adj->srv.isCrypt();
							// Получаем поддерживаемый метод компрессии
							adj->compress = adj->srv.getCompress();
							// Если подключение не выполнено
							if(!adj->connect){
								// Получаем данные запроса
								const auto & query = adj->srv.getQuery();
								// Выполняем проверку разрешено ли нам выполнять подключение
								const bool allow = (!this->noConnect || (query.method != web_t::method_t::CONNECT));
								// Получаем хост сервера
								const auto & host = adj->srv.getHeader("host");
								// Сообщение запрета подключения
								const string message = (allow ? "" : "Connect method prohibited");
								// Если хост сервера получен
								if(allow && (adj->locked = !host.empty())){
									// Запоминаем метод подключения
									adj->method = query.method;
									// Формируем адрес подключения
									adj->worker.url = this->worker.uri.parseUrl(this->fmk->format("http://%s", host.c_str()));
									// Выполняем запрос на сервер
									this->core.client.open(adj->worker.wid);
									// Выходим из функции
									return;
								// Если хост не получен
								} else {
									// Выполняем сброс состояния HTTP парсера
									adj->srv.clear();
									// Выполняем сброс состояния HTTP парсера
									adj->srv.reset();
									// Выполняем очистку буфера полученных данных
									adj->server.clear();
									// Формируем запрос реджекта
									const auto & response = adj->srv.reject(403, message);
									// Если ответ получен
									if(!response.empty()){
										// Тело полезной нагрузки
										vector <char> payload;
										// Отправляем ответ клиенту
										core->write(response.data(), response.size(), aid);
										// Получаем данные тела запроса
										while(!(payload = adj->srv.payload()).empty()){
											// Отправляем тело на сервер
											core->write(payload.data(), payload.size(), aid);
										}
									// Выполняем отключение клиента
									} else this->close(aid);
									// Выходим из функции
									return;
								}
							// Если подключение выполнено
							} else {
								// Выполняем удаление заголовка авторизации на прокси-сервере
								adj->srv.rmHeader("proxy-authorization");
								{
									// Получаем данные запроса
									const auto & query = adj->srv.getQuery();
									// Получаем данные заголовка Via
									string via = adj->srv.getHeader("via");
									// Если заголовок получен
									if(!via.empty())
										// Устанавливаем Via заголовок
										via = this->fmk->format("%s, %.1f %s:%u", via.c_str(), query.ver, this->host.c_str(), this->port);
									// Иначе просто формируем заголовок Via
									else via = this->fmk->format("%.1f %s:%u", query.ver, this->host.c_str(), this->port);
									// Устанавливаем заголовок Via
									adj->srv.addHeader("Via", via);
								}{
									// Название операционной системы
									const char * os = nullptr;
									// Определяем название операционной системы
									switch((uint8_t) this->fmk->os()){
										// Если операционной системой является Unix
										case (uint8_t) fmk_t::os_t::UNIX: os = "Unix"; break;
										// Если операционной системой является Linux
										case (uint8_t) fmk_t::os_t::LINUX: os = "Linux"; break;
										// Если операционной системой является неизвестной
										case (uint8_t) fmk_t::os_t::NONE: os = "Unknown"; break;
										// Если операционной системой является Windows
										case (uint8_t) fmk_t::os_t::WIND32:
										case (uint8_t) fmk_t::os_t::WIND64: os = "Windows"; break;
										// Если операционной системой является MacOS X
										case (uint8_t) fmk_t::os_t::MACOSX: os = "MacOS X"; break;
										// Если операционной системой является FreeBSD
										case (uint8_t) fmk_t::os_t::FREEBSD: os = "FreeBSD"; break;
									}
									// Устанавливаем наименование агента
									adj->srv.addHeader("X-Proxy-Agent", this->fmk->format("(%s; %s) %s/%s", os, this->name.c_str(), this->sid.c_str(), this->version.c_str()));
								}
								// Если функция обратного вызова установлена, выполняем
								if(this->messageFn != nullptr){
									// Выводим сообщение
									if(this->messageFn(aid, event_t::REQUEST, &adj->srv, this, this->ctx.at(1))){
										// Получаем данные запроса
										const auto & request = adj->srv.request();
										// Если данные запроса получены
										if(!request.empty()){
											// Получаем идентификатор адъютанта
											const size_t aid = adj->worker.getAid();
											// Отправляем запрос на внешний сервер
											if(aid > 0){
												// Тело REST сообщения
												vector <char> entity;
												// Отправляем серверу сообщение
												reinterpret_cast <awh::core_t *> (&this->core.client)->write(request.data(), request.size(), aid);
												// Получаем данные тела запроса
												while(!(entity = adj->srv.payload()).empty()){
													// Отправляем тело на сервер
													reinterpret_cast <awh::core_t *> (&this->core.client)->write(entity.data(), entity.size(), aid);
												}
											}
										}
									}
								// Если функция обратного вызова не установлена
								} else {
									// Получаем идентификатор адъютанта
									const size_t aid = adj->worker.getAid();
									// Отправляем запрос на внешний сервер
									if(aid > 0){
										// Получаем данные запроса
										const auto & request = adj->srv.request();
										// Если данные запроса получены
										if(!request.empty()){
											// Тело REST сообщения
											vector <char> entity;
											// Если функция обратного вызова установлена, выполняем
											if(this->binaryFn != nullptr){
												// Выводим сообщение
												if(this->binaryFn(aid, event_t::REQUEST, request.data(), request.size(), this, this->ctx.at(2)))
													// Отправляем запрос на внешний сервер
													reinterpret_cast <awh::core_t *> (&this->core.client)->write(request.data(), request.size(), aid);
											// Отправляем запрос на внешний сервер
											} else reinterpret_cast <awh::core_t *> (&this->core.client)->write(request.data(), request.size(), aid);
											// Получаем данные тела запроса
											while(!(entity = adj->srv.payload()).empty()){
												// Отправляем тело на сервер
												reinterpret_cast <awh::core_t *> (&this->core.client)->write(entity.data(), entity.size(), aid);
											}
										}
									}
								}
							}
							// Выполняем сброс состояния HTTP парсера
							adj->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->srv.reset();
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->srv.reset();
							// Выполняем очистку буфера полученных данных
							adj->server.clear();
							// Формируем запрос авторизации
							const auto & response = adj->srv.reject(407);
							// Если ответ получен
							if(!response.empty()){
								// Тело полезной нагрузки
								vector <char> payload;
								// Устанавливаем размер стопбайт
								if(!adj->srv.isAlive()) adj->stopBytes = response.size();
								// Отправляем ответ клиенту
								core->write(response.data(), response.size(), aid);
								// Получаем данные тела запроса
								while(!(payload = adj->srv.payload()).empty()){
									// Устанавливаем размер стопбайт
									if(!adj->srv.isAlive()) adj->stopBytes += payload.size();
									// Отправляем тело на сервер
									core->write(payload.data(), payload.size(), aid);
								}
							// Выполняем отключение клиента
							} else this->close(aid);
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
					adj->server.erase(adj->server.begin(), adj->server.begin() + bytes);
					// Если данных для обработки не осталось, выходим
					if(adj->server.empty()) break;
				// Если данных для обработки недостаточно, выходим
				} else break;
			}
		}
	}
}
/**
 * init Метод инициализации WebSocket клиента
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Proxy::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
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
void awh::server::Proxy::on(void * ctx, function <void (const size_t, const mode_t, Proxy *, void *)> callback) noexcept {
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
void awh::server::Proxy::on(void * ctx, function <bool (const size_t, const event_t, http_t *, Proxy *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(void * ctx, function <bool (const size_t, const event_t, const char *, const size_t, Proxy *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(2) = ctx;
	// Устанавливаем функцию получения сообщений в бинарном виде с сервера
	this->binaryFn = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Proxy::on(void * ctx, function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(3) = ctx;
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->extractPassFn = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Proxy::on(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(4) = ctx;
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->checkAuthFn = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(void * ctx, function <void (const vector <char> &, const http_t *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(5) = ctx;
	// Устанавливаем функцию обратного вызова для получения чанков
	this->chunkingFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(void * ctx, function <bool (const string &, const string &, Proxy *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(6) = ctx;
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
void awh::server::Proxy::reject(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->core.server.working()){
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->srv.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->srv.setHeaders(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->alive && !adj->alive && adj->srv.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->srv.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", this->keepAlive / 1000, this->maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->srv.reject(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем размер стопбайт
			if(!adj->srv.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <server::core_t *> (&this->core.server))->write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Устанавливаем размер стопбайт
				if(!adj->srv.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((awh::core_t *) const_cast <server::core_t *> (&this->core.server))->write(payload.data(), payload.size(), aid);
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
void awh::server::Proxy::response(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->core.server.working()){
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->srv.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->srv.setHeaders(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->alive && !adj->alive && adj->srv.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->srv.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", this->keepAlive / 1000, this->maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->srv.response(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем размер стопбайт
			if(!adj->srv.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <server::core_t *> (&this->core.server))->write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Устанавливаем размер стопбайт
				if(!adj->srv.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((awh::core_t *) const_cast <server::core_t *> (&this->core.server))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Proxy::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Proxy::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.mac(aid);
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::setAlive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->alive = mode;
}
/**
 * setAlive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::setAlive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
}
/**
 * start Метод запуска клиента
 */
void awh::server::Proxy::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->core.server.working())
		// Выполняем запуск биндинга
		this->core.server.start();
}
/**
 * stop Метод остановки клиента
 */
void awh::server::Proxy::stop() noexcept {
	// Если подключение выполнено
	if(this->core.server.working())
		// Завершаем работу, если разрешено остановить
		this->core.server.stop();
}
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
void awh::server::Proxy::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем отключение всех дочерних клиентов
		reinterpret_cast <awh::core_t *> (&this->core.client)->close(adj->worker.getAid());
		// Выполняем удаление параметров адъютанта
		this->worker.removeAdj(aid);
	}
	// Отключаем клиента от сервера
	reinterpret_cast <awh::core_t *> (&this->core.server)->close(aid);
	// Если функция обратного вызова установлена, выполняем
	if(this->openStopFn != nullptr) this->openStopFn(aid, mode_t::DISCONNECT, this, this->ctx.at(0));
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Proxy::setRealm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->realm = realm;
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Proxy::setOpaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->opaque = opaque;
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Proxy::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->authHash = hash;
	// Устанавливаем тип авторизации
	this->authType = type;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Proxy::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий метод CONNECT
	this->noConnect = (flag & (uint8_t) flag_t::NOCONNECT);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	this->core.client.setDefer(flag & (uint8_t) flag_t::DEFER);
	this->core.server.setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->core.client.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	this->core.server.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Proxy::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * setKeepAlive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Proxy::setKeepAlive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->keepAlive = time;
}
/**
 * setMaxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Proxy::setMaxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->maxRequests = max;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Proxy::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->worker.compress = compress;
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Proxy::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->worker.timeWrite = write;
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Proxy::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->sid = id;
	// Устанавливаем название сервера
	this->name = name;
	// Устанавливаем версию сервера
	this->version = ver;
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Proxy::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::server::Proxy::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
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
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept : core(fmk, log), worker(fmk, log), fmk(fmk), log(log) {
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
	this->core.server.setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	this->core.server.add(&this->worker);
	// Устанавливаем функцию активации ядра сервера
	this->core.server.setCallback(this, &runCallback);
	// Устанавливаем интервал персистентного таймера для работы пингов
	this->core.server.setPersistInterval(KEEPALIVE_TIMEOUT / 2);
}
