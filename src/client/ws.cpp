/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <client/ws.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param ctx   контекст объекта http
 */
void awh::WebSocketClient::chunking(const vector <char> & chunk, const http_t * ctx) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (ctx)->addBody(chunk.data(), chunk.size());
}
/**
 * runCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketClient::runCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Выполняем подключение
	core->open(wid);
}
/**
 * openCallback Функция обратного вызова при подключении к серверу
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketClient::openCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wcli_t * ws = reinterpret_cast <wcli_t *> (ctx);
		// Выполняем сброс состояния HTTP парсера
		ws->http->clear();
		// Получаем бинарные данные REST запроса
		const auto & rest = ws->http->request(ws->worker.url);
		// Если бинарные данные запроса получены, отправляем на сервер
		if(!rest.empty()) core->write(rest.data(), rest.size(), wid);
	}
}
/**
 * closeCallback Функция обратного вызова при отключении от сервера
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketClient::closeCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wcli_t * ws = reinterpret_cast <wcli_t *> (ctx);
		// Если прокси-сервер активирован но уже переключён на работу с сервером
		if((ws->worker.proxy.type != proxy_t::type_t::NONE) && !ws->worker.isProxy())
			// Выполняем переключение обратно на прокси-сервер
			reinterpret_cast <ccli_t *> (core)->switchProxy(ws->worker.wid);
		// Очищаем буфер фрагментированного сообщения
		ws->fragmes.clear();
		// Останавливаем таймер пинга сервера
		ws->timerPing.stop();
		// Останавливаем таймер подключения
		ws->timerConnect.stop();
		// Если нужно произвести запрос заново
		if((ws->code == 301) || (ws->code == 308) ||
		   (ws->code == 401) || (ws->code == 407)){
			// Выполняем запрос заново
			core->open(ws->worker.wid);
			// Выходим из функции
			return;
		}
		// Очищаем код ответа
		ws->code = 0;
		// Завершаем работу
		if(ws->unbind) core->stop();
	}
}
/**
 * openProxyCallback Функция обратного вызова при подключении к прокси-серверу
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::WebSocketClient::openProxyCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		wcli_t * ws = reinterpret_cast <wcli_t *> (ctx);
		// Выполняем сброс состояния HTTP парсера
		ws->worker.proxy.http->clear();
		// Получаем бинарные данные REST запроса
		const auto & rest = ws->worker.proxy.http->proxy(ws->worker.url);
		// Если бинарные данные запроса получены, отправляем на прокси-сервер
		if(!rest.empty()) core->write(rest.data(), rest.size(), wid);
		// Выполняем сброс состояния HTTP парсера
		ws->worker.proxy.http->clear();
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::WebSocketClient::readCallback(const char * buffer, const size_t size, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0)){
		// Получаем контекст модуля
		wcli_t * ws = reinterpret_cast <wcli_t *> (ctx);
		// Если рукопожатие не выполнено
		if(!reinterpret_cast <http_t *> (ws->http)->isHandshake()){
			// Выполняем парсинг полученных данных
			ws->http->parse(buffer, size);
			// Если все данные получены
			if(ws->http->isEnd()){
				// Выполняем проверку авторизации
				switch((u_short) ws->http->getAuth()){
					// Если нужно попытаться ещё раз
					case (u_short) http_t::stath_t::RETRY: {
						// Если попытка повторить авторизацию ещё не проводилась
						if(!ws->failAuth){
							// Получаем новый адрес запроса
							ws->worker.url = ws->http->getUrl();
							// Если адрес запроса получен
							if(!ws->worker.url.empty()){
								// Запоминаем, что попытка выполнена
								ws->failAuth = true;
								// Если соединение является постоянным
								if(ws->http->isAlive())
									// Выполняем повторно отправку сообщения на сервер
									openCallback(wid, core, ctx);
								// Завершаем работу
								else core->close(ws->worker.wid);
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем код ответа
						ws->code = 403;
						// Создаём сообщение
						mess_t mess(ws->code, ws->http->getMessage(ws->code));
						// Выводим сообщение
						ws->error(mess);
					} break;
					// Если запрос выполнен удачно
					case (u_short) http_t::stath_t::GOOD: {
						// Если рукопожатие выполнено
						if(ws->http->isHandshake()){
							// Выполняем сброс количество попыток
							ws->failAuth = false;
							// Получаем флаг шифрованных данных
							ws->crypt = ws->http->isCrypt();
							// Получаем поддерживаемый метод компрессии
							ws->compress = ws->http->getCompress();
							// Выводим в лог сообщение
							ws->log->print("%s", log_t::flag_t::INFO, "authorization on the WebSocket server was successful");
							// Если функция обратного вызова установлена, выполняем
							if(ws->openStopFn != nullptr) ws->openStopFn(true, ws);
							// Устанавливаем таймер на контроль подключения
							ws->timerPing.setInterval([ws]{
								// Выполняем пинг сервера
								ws->ping(to_string(time(nullptr)));
							}, PING_INTERVAL);
							// Завершаем работу
							return;
						// Сообщаем, что рукопожатие не выполнено
						} else {
							// Устанавливаем код ответа
							ws->code = 404;
							// Создаём сообщение
							mess_t mess(ws->code, ws->http->getMessage(ws->code));
							// Выводим сообщение
							ws->error(mess);
						}
					} break;
					// Если запрос неудачный
					case (u_short) http_t::stath_t::FAULT: {
						// Получаем параметры запроса
						const auto & query = ws->http->getQuery();
						// Устанавливаем код ответа
						ws->code = query.code;
						// Создаём сообщение
						mess_t mess(ws->code, query.message);
						// Выводим сообщение
						ws->error(mess);
					} break;
				}
				// Выполняем сброс количество попыток
				ws->failAuth = false;
				// Завершаем работу
				core->close(ws->worker.wid);
			}
			// Завершаем работу
			return;
		// Если рукопожатие выполнено
		} else {
			// Смещение в буфере данных
			size_t offset = 0;
			// Создаём объект шапки фрейма
			frame_t::head_t head;
			// Если флаг ожидания входящих сообщений, активирован
			if(ws->worker.wait){
				// Останавливаем таймер
				ws->timerConnect.stop();
				// Устанавливаем таймер на контроль подключения
				ws->timerConnect.setTimeout([core, ws]{
					// Если нужно выполнить автоматическое переподключение
					if(ws->worker.alive)
						// Завершаем работу
						core->close(ws->worker.wid);
					// Если выполнять автоматическое подключение не требуется, просто выходим
					else if(ws->unbind) core->stop();
				}, CONNECT_TIMEOUT);
			}
			// Выполняем перебор полученных данных
			while((size - offset) > 0){
				// Выполняем чтение фрейма WebSocket
				const auto & data = ws->frame->get(head, buffer + offset, size - offset);
				// Если буфер данных получен
				if(!data.empty()){
					// Проверяем состояние флагов RSV2 и RSV3
					if(head.rsv[1] || head.rsv[2]){
						// Создаём сообщение
						mess_t mess(1002, "RSV2 and RSV3 must be clear");
						// Выводим сообщение
						ws->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					}
					// Если флаг компресси включён а данные пришли не сжатые
					if(head.rsv[0] && ((ws->compress == http_t::compress_t::NONE) ||
					(head.optcode == frame_t::opcode_t::CONTINUATION) ||
					(((u_short) head.optcode > 0x07) && ((u_short) head.optcode < 0x0b)))){
						// Создаём сообщение
						mess_t mess(1002, "RSV1 must be clear");
						// Выводим сообщение
						ws->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					}
					// Если опкоды требуют финального фрейма
					if(!head.fin && ((u_short) head.optcode > 0x07) && ((u_short) head.optcode < 0x0b)){
						// Создаём сообщение
						mess_t mess(1002, "FIN must be set");
						// Выводим сообщение
						ws->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					}
					// Определяем тип ответа
					switch((u_short) head.optcode){
						// Если ответом является PING
						case (u_short) frame_t::opcode_t::PING:
							// Отправляем ответ серверу
							ws->pong(string(data.begin(), data.end()));
						break;
						// Если ответом является PONG
						case (u_short) frame_t::opcode_t::PONG:
							// Если функция обратного вызова обработки PONG существует
							if(ws->pongFn != nullptr) ws->pongFn(string(data.begin(), data.end()), ws);
						break;
						// Если ответом является TEXT
						case (u_short) frame_t::opcode_t::TEXT:
						// Если ответом является BINARY
						case (u_short) frame_t::opcode_t::BINARY: {
							// Запоминаем полученный опкод
							ws->opcode = head.optcode;
							// Запоминаем, что данные пришли сжатыми
							ws->compressed = (head.rsv[0] && (ws->compress != http_t::compress_t::NONE));
							// Если список фрагментированных сообщений существует
							if(!ws->fragmes.empty()){
								// Создаём сообщение
								mess_t mess(1002, "opcode for subsequent fragmented messages should not be set");
								// Выводим сообщение
								ws->error(mess);
								// Выполняем реконнект
								goto Reconnect;
							// Если сообщение является не последнем
							} else if(!head.fin)
								// Заполняем фрагментированное сообщение
								ws->fragmes.insert(ws->fragmes.end(), data.begin(), data.end());
							// Если сообщение является последним
							else ws->extraction(data, (ws->opcode == frame_t::opcode_t::TEXT));
						} break;
						// Если ответом является CONTINUATION
						case (u_short) frame_t::opcode_t::CONTINUATION: {
							// Заполняем фрагментированное сообщение
							ws->fragmes.insert(ws->fragmes.end(), data.begin(), data.end());
							// Если сообщение является последним
							if(head.fin){
								// Выполняем извлечение данных
								ws->extraction(ws->fragmes, (ws->opcode == frame_t::opcode_t::TEXT));
								// Очищаем список фрагментированных сообщений
								ws->fragmes.clear();
							}
						} break;
						// Если ответом является CLOSE
						case (u_short) frame_t::opcode_t::CLOSE: {
							// Извлекаем сообщение
							const auto & mess = ws->frame->message(data);
							// Выводим сообщение
							ws->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						} break;
					}
					// Увеличиваем смещение в буфере
					offset += (head.payload + head.size);
				// Выходим из цикла, данных в буфере не достаточно
				} else break;
			}
			// Выходим из функции
			return;
		}
		// Устанавливаем метку реконнекта
		Reconnect:
		// Завершаем работу
		core->close(ws->worker.wid);
	}
}
/**
 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::WebSocketClient::readProxyCallback(const char * buffer, const size_t size, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0)){
		// Получаем контекст модуля
		wcli_t * ws = reinterpret_cast <wcli_t *> (ctx);
		// Выполняем парсинг полученных данных
		ws->worker.proxy.http->parse(buffer, size);
		// Если все данные получены
		if(ws->worker.proxy.http->isEnd()){
			// Получаем параметры запроса
			const auto & query = ws->worker.proxy.http->getQuery();
			// Устанавливаем код ответа
			ws->code = query.code;
			// Создаём сообщение
			mess_t mess(ws->code, query.message);
			// Выполняем проверку авторизации
			switch((u_short) ws->worker.proxy.http->getAuth()){
				// Если нужно попытаться ещё раз
				case (u_short) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(!ws->failAuth){
						// Получаем новый адрес запроса
						ws->worker.proxy.url = ws->worker.proxy.http->getUrl();
						// Если адрес запроса получен
						if(!ws->worker.proxy.url.empty()){
							// Запоминаем, что попытка выполнена
							ws->failAuth = true;
							// Если соединение является постоянным
							if(ws->http->isAlive())
								// Выполняем повторно отправку сообщения на сервер
								openProxyCallback(wid, core, ctx);
							// Завершаем работу
							else core->close(ws->worker.wid);
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем код ответа
					ws->code = 403;
					// Присваиваем сообщению, новое значение кода
					mess = ws->code;
					// Устанавливаем сообщение ответа
					mess = ws->worker.proxy.http->getMessage(ws->code);
				} break;
				// Если запрос выполнен удачно
				case (u_short) http_t::stath_t::GOOD: {
					// Выполняем сброс количество попыток
					ws->failAuth = false;
					// Выполняем переключение на работу с сервером
					reinterpret_cast <ccli_t *> (core)->switchProxy(ws->worker.wid);
					// Завершаем работу
					return;
				} break;
			}
			// Выводим сообщение
			ws->error(mess);
			// Выполняем сброс количество попыток
			ws->failAuth = false;
			// Завершаем работу
			core->close(ws->worker.wid);
		}
	}
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::WebSocketClient::error(const mess_t & message) const noexcept {
	// Если сообщение об ошибке пришло
	if(!message.text.empty()){
		// Если тип сообщения получен
		if(!message.type.empty())
			// Выводим в лог сообщение
			this->log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
		// Иначе выводим сообщение в упрощёном виде
		else this->log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
		// Если функция обратного вызова установлена, выводим полученное сообщение
		if(this->errorFn != nullptr) this->errorFn(message.code, message.text, const_cast <WebSocketClient *> (this));
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::WebSocketClient::extraction(const vector <char> & buffer, const bool utf8) const noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->freeze && (this->messageFn != nullptr)){
		// Если данные пришли в сжатом виде
		if(this->compressed && (this->compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch((u_short) this->compress){
				// Если метод компрессии выбран Deflate
				case (u_short) http_t::compress_t::DEFLATE: {
					// Устанавливаем размер скользящего окна
					this->hash->setWbit(this->http->getWbitServer());
					// Добавляем хвост в полученные данные
					this->hash->setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->hash->decompress(buffer.data(), buffer.size());
				} break;
				// Если метод компрессии выбран GZip
				case (u_short) http_t::compress_t::GZIP:
					// Выполняем декомпрессию полученных данных
					data = this->hash->decompressGzip(buffer.data(), buffer.size());
				break;
				// Если метод компрессии выбран Brotli
				case (u_short) http_t::compress_t::BROTLI:
					// Выполняем декомпрессию полученных данных
					data = this->hash->decompressBrotli(buffer.data(), buffer.size());
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash->decrypt(data.data(), data.size());
					// Отправляем полученный результат
					if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocketClient *> (this));
					// Иначе выводим сообщение так - как оно пришло
					else this->messageFn(data, utf8, const_cast <WebSocketClient *> (this));
				// Отправляем полученный результат
				} else this->messageFn(data, utf8, const_cast <WebSocketClient *> (this));
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Выводим сообщение
				this->error(mess);
				// Иначе выводим сообщение так - как оно пришло
				this->messageFn(buffer, utf8, const_cast <WebSocketClient *> (this));
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash->decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocketClient *> (this));
				// Иначе выводим сообщение так - как оно пришло
				else this->messageFn(buffer, utf8, const_cast <WebSocketClient *> (this));
			// Отправляем полученный результат
			} else this->messageFn(buffer, utf8, const_cast <WebSocketClient *> (this));
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::WebSocketClient::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->core->isStart() && !this->locker){
		// Если рукопожатие выполнено
		if(this->http->isHandshake()){
			// Создаём буфер для отправки
			const auto & buffer = this->frame->pong(message);
			// Отправляем серверу сообщение
			((core_t *) const_cast <ccli_t *> (this->core))->write(buffer.data(), buffer.size(), this->worker.wid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::WebSocketClient::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->core->isStart() && !this->locker){
		// Если рукопожатие выполнено
		if(this->http->isHandshake()){
			// Создаём буфер для отправки
			const auto & buffer = this->frame->ping(message);
			// Отправляем серверу сообщение
			((core_t *) const_cast <ccli_t *> (this->core))->write(buffer.data(), buffer.size(), this->worker.wid);
		}
	}
}
/**
 * init Метод инициализации WebSocket клиента
 * @param url      адрес WebSocket сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::WebSocketClient::init(const string & url, const http_t::compress_t compress) noexcept {
	// Если адрес сервера передан
	if(!url.empty()){
		// Выполняем очистку воркера
		this->worker.clear();
		// Устанавливаем флаг активации сжатия данных
		this->compress = compress;
		// Устанавливаем URL адрес запроса
		this->worker.url = this->uri->parseUrl(url);
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::WebSocketClient::on(function <void (const bool, WebSocketClient *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->openStopFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения PONG
 * @param callback функция обратного вызова
 */
void awh::WebSocketClient::on(function <void (const string &, WebSocketClient *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений PONG
	this->pongFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::WebSocketClient::on(function <void (const u_short, const string &, WebSocketClient *)> callback) noexcept {
	// Устанавливаем функцию получения ошибок
	this->errorFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::WebSocketClient::on(function <void (const vector <char> &, const bool, WebSocketClient *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::WebSocketClient::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if(this->core->isStart() && !this->locker){
		// Выполняем блокировку отправки сообщения
		this->locker = !this->locker;
		// Если рукопожатие выполнено
		if((message != nullptr) && (size > 0) && this->http->isHandshake()){
			// Создаём объект заголовка для отправки
			frame_t::head_t head;
			// Передаём сообщение одним запросом
			head.fin = true;
			// Выполняем маскировку сообщения
			head.mask = true;
			// Указываем, что сообщение передаётся в сжатом виде
			head.rsv[0] = (this->compress != http_t::compress_t::NONE);
			// Устанавливаем опкод сообщения
			head.optcode = (utf8 ? frame_t::opcode_t::TEXT : frame_t::opcode_t::BINARY);
			// Если нужно производить шифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash->encrypt(message, size);
				// Если данные зашифрованны
				if(!res.empty()){
					// Заменяем сообщение для передачи
					message = res.data();
					// Заменяем размер сообщения
					(* const_cast <size_t *> (&size)) = res.size();
				}
			}
			/**
			 * sendFn Функция отправки сообщения на сервер
			 * @param head    объект заголовков фрейма WebSocket
			 * @param message буфер сообщения в бинарном виде
			 * @param size    размер сообщения в байтах
			 */
			auto sendFn = [this](const frame_t::head_t & head, const char * message, const size_t size) noexcept {
				// Если все данные переданы
				if((message != nullptr) && (size > 0)){
					// Если необходимо сжимать сообщение перед отправкой
					if(this->compress != http_t::compress_t::NONE){
						// Компрессионные данные
						vector <char> data;
						// Определяем метод компрессии
						switch((u_short) this->compress){
							// Если метод компрессии выбран Deflate
							case (u_short) http_t::compress_t::DEFLATE: {
								// Устанавливаем размер скользящего окна
								this->hash->setWbit(this->http->getWbitClient());
								// Выполняем компрессию полученных данных
								data = this->hash->compress(message, size);
								// Удаляем хвост в полученных данных
								this->hash->rmTail(data);
							} break;
							// Если метод компрессии выбран GZip
							case (u_short) http_t::compress_t::GZIP:
								// Выполняем компрессию полученных данных
								data = this->hash->compressGzip(message, size);
							break;
							// Если метод компрессии выбран Brotli
							case (u_short) http_t::compress_t::BROTLI:
								// Выполняем компрессию полученных данных
								data = this->hash->compressBrotli(message, size);
							break;
						}
						// Создаём буфер для отправки
						const auto & buffer = this->frame->set(head, data.data(), data.size());
						// Отправляем серверу сообщение
						((core_t *) const_cast <ccli_t *> (this->core))->write(buffer.data(), buffer.size(), this->worker.wid);
					// Если сообщение перед отправкой сжимать не нужно
					} else {
						// Создаём буфер для отправки
						const auto & buffer = this->frame->set(head, message, size);
						// Отправляем серверу сообщение
						((core_t *) const_cast <ccli_t *> (this->core))->write(buffer.data(), buffer.size(), this->worker.wid);
					}
				}
			};
			// Если требуется фрагментация сообщения
			if(size > this->frameSize){
				// Бинарный буфер чанка данных
				vector <char> chunk(this->frameSize);
				// Смещение в бинарном буфере
				size_t start = 0, stop = this->frameSize;
				// Выполняем разбивку полезной нагрузки на сегменты
				while(stop < size){
					// Увеличиваем длину чанка
					stop += this->frameSize;
					// Если длина чанка слишком большая, компенсируем
					stop = (stop > size ? size : stop);
					// Устанавливаем флаг финального сообщения
					head.fin = (stop == size);
					// Формируем чанк бинарных данных
					chunk.assign(message + start, message + stop);
					// Выполняем отправку чанка на сервер
					sendFn(head, chunk.data(), chunk.size());
					// Выполняем сброс RSV1
					head.rsv[0] = false;
					// Устанавливаем опкод сообщения
					head.optcode = frame_t::opcode_t::CONTINUATION;
					// Увеличиваем смещение в буфере
					start = stop;
				}
			// Если фрагментация сообщения не требуется
			} else sendFn(head, message, size);
		}
		// Выполняем разблокировку отправки сообщения
		this->locker = !this->locker;
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::WebSocketClient::stop() noexcept {
	// Если подключение выполнено
	if(this->core->isStart()){
		// Завершаем работу, если разрешено остановить
		if(this->unbind) const_cast <ccli_t *> (this->core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else ((core_t *) const_cast <ccli_t *> (this->core))->close(this->worker.wid);
		// Если функция обратного вызова установлена, выполняем
		if(this->openStopFn != nullptr) this->openStopFn(false, this);
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::WebSocketClient::pause() noexcept {
	// Ставим работу клиента на паузу
	this->freeze = true;
}
/**
 * start Метод запуска клиента
 */
void awh::WebSocketClient::start() noexcept {
	// Снимаем с паузы клиент
	this->freeze = false;
	// Если адрес URL запроса передан
	if(!this->worker.url.empty()){
		// Если биндинг не запущен, выполняем запуск биндинга
		if(!this->core->isStart())
			// Выполняем запуск биндинга
			const_cast <ccli_t *> (this->core)->start();
		// Выполняем запрос на сервер
		else const_cast <ccli_t *> (this->core)->open(this->worker.wid);
	}
}
/**
 * getSub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::WebSocketClient::getSub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->http->getSub();
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::WebSocketClient::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->subs.push_back(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::WebSocketClient::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->subs = subs;
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::WebSocketClient::setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http->setChunkingFn(callback);
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::WebSocketClient::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::WebSocketClient::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::WebSocketClient::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга
	this->unbind = !(flag & (u_short) core_t::flag_t::NOTSTOP);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (u_short) core_t::flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (u_short) core_t::flag_t::KEEPALIVE);
	// Выполняем установку флага проверки домена
	const_cast <ccli_t *> (this->core)->setVerifySSL(flag & (u_short) core_t::flag_t::VERIFYSSL);
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri параметры прокси-сервера
 */
void awh::WebSocketClient::setProxy(const string & uri) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем параметры прокси-сервера
		this->worker.proxy.url = this->uri->parseUrl(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->worker.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->worker.proxy.url.schema.compare("socks5") == 0)
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::SOCKS5;
			// Если протокол подключения HTTP
			else if((this->worker.proxy.url.schema.compare("http") == 0) ||
			(this->worker.proxy.url.schema.compare("https") == 0))
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::HTTP;
			// Если требуется авторизация на прокси-сервере
			if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
				// Устанавливаем данные пользователя
				this->worker.proxy.http->setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
		}
	}
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::WebSocketClient::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http->setChunkSize(size);
}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::WebSocketClient::setFrameSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->frameSize = size;
}
/**
 * setAttempts Метод установки количества попыток переподключения
 * @param count количество попыток переподключения
 */
void awh::WebSocketClient::setAttempts(const u_short count) noexcept {
	// Устанавливаем количество попыток переподключения
	this->worker.attempts.second = count;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::WebSocketClient::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->http->setUserAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->worker.proxy.http->setUserAgent(userAgent);
	}
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::WebSocketClient::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод сжатия
	this->http->setCompress(compress);
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::WebSocketClient::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->http->setUser(login, password);
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::WebSocketClient::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http->setServ(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->worker.proxy.http->setServ(id, name, ver);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::WebSocketClient::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash->setAES(aes);
	// Устанавливаем соль шифрования
	this->hash->setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash->setPassword(pass);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::WebSocketClient::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	this->http->setAuthType(type, algorithm);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::WebSocketClient::setAuthTypeProxy(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	this->worker.proxy.http->setAuthType(type, algorithm);
}
/**
 * WebSocketClient Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::WebSocketClient::WebSocketClient(const ccli_t * core, const fmk_t * fmk, const log_t * log) noexcept : core(core), fmk(fmk), log(log), worker(fmk, log) {
	try {
		// Устанавливаем контекст сообщения
		this->worker.ctx = this;
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект для работы с URI ссылками
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с компрессией/декомпрессией
		this->hash = new hash_t(this->fmk, this->log);
		// Создаём объект для работы с фреймом WebSocket
		this->frame = new frame_t(this->fmk, this->log);
		// Создаём объект для работы с HTTP протоколом
		this->http = new wsc_t(this->fmk, this->log, this->uri);
		// Устанавливаем функцию обработки вызова для получения чанков
		this->http->setChunkingFn(&chunking);
		// Устанавливаем событие на запуск системы
		this->worker.runFn = runCallback;
		// Устанавливаем событие подключения
		this->worker.openFn = openCallback;
		// Устанавливаем функцию чтения данных
		this->worker.readFn = readCallback;
		// Устанавливаем событие отключения
		this->worker.closeFn = closeCallback;
		// Устанавливаем событие на подключение к прокси-серверу
		this->worker.openProxyFn = openProxyCallback;
		// Устанавливаем событие на чтение данных с прокси-сервера
		this->worker.readProxyFn = readProxyCallback;
		// Добавляем воркер в биндер TCP/IP
		const_cast <ccli_t *> (this->core)->add(&this->worker);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~WebSocketClient Деструктор
 */
awh::WebSocketClient::~WebSocketClient() noexcept {
	// Удаляем объект для работы с сетью
	if(this->nwk != nullptr) delete this->nwk;
	// Удаляем объект для работы с URI ссылками
	if(this->uri != nullptr) delete this->uri;
	// Удаляем объект работы с HTTP протоколом
	if(this->http != nullptr) delete this->http;
	// Если объект для компрессии/декомпрессии создан
	if(this->hash != nullptr) delete this->hash;
	// Если объект для работы с фреймом WebSocket создан
	if(this->frame != nullptr) delete this->frame;
}
