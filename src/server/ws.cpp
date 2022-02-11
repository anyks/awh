/**
 * @file: ws.cpp
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
#include <server/ws.hpp>

/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::WebSocket::openCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, ws->port, ws->host);
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
void awh::server::WebSocket::persistCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Получаем текущий штамп времени
			const time_t stamp = ws->fmk->unixTimestamp();
			// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
			if(adj->close || ((stamp - adj->checkPoint) >= (PERSIST_INTERVAL * 5)))
				// Завершаем работу
				reinterpret_cast <server::core_t *> (core)->close(aid);
			// Отправляем запрос адъютанту
			else ws->ping(aid, core, to_string(aid));
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
void awh::server::WebSocket::connectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Создаём адъютанта
		ws->worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если данные необходимо зашифровать
		if(ws->crypt){
			// Устанавливаем размер шифрования
			adj->hash.setAES(ws->aes);
			// Устанавливаем соль шифрования
			adj->hash.setSalt(ws->salt);
			// Устанавливаем пароль шифрования
			adj->hash.setPassword(ws->pass);
		}
		// Разрешаем перехватывать контекст для клиента
		adj->http.setClientTakeover(ws->takeOverCli);
		// Разрешаем перехватывать контекст для сервера
		adj->http.setServerTakeover(ws->takeOverSrv);
		// Разрешаем перехватывать контекст компрессии
		adj->hash.setTakeoverCompress(ws->takeOverSrv);
		// Разрешаем перехватывать контекст декомпрессии
		adj->hash.setTakeoverDecompress(ws->takeOverCli);
		// Устанавливаем данные сервиса
		adj->http.setServ(ws->sid, ws->name, ws->version);
		// Устанавливаем поддерживаемые сабпротоколы
		if(!ws->subs.empty()) adj->http.setSubs(ws->subs);
		// Устанавливаем метод компрессии поддерживаемый сервером
		adj->http.setCompress(ws->worker.compress);
		// Устанавливаем параметры шифрования
		if(ws->crypt) adj->http.setCrypt(ws->pass, ws->salt, ws->aes);
		// Если сервер требует авторизацию
		if(ws->authType != auth_t::type_t::NONE){
			// Определяем тип авторизации
			switch((uint8_t) ws->authType){
				// Если тип авторизации Basic
				case (uint8_t) auth_t::type_t::BASIC: {
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(ws->authType);
					// Устанавливаем функцию проверки авторизации
					adj->http.setAuthCallback(ws->ctx.at(4), ws->checkAuthFn);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->http.setRealm(ws->realm);
					// Устанавливаем временный ключ сессии сервера
					adj->http.setOpaque(ws->opaque);
					// Устанавливаем параметры авторизации
					adj->http.setAuthType(ws->authType, ws->authHash);
					// Устанавливаем функцию извлечения пароля
					adj->http.setExtractPassCallback(ws->ctx.at(3), ws->extractPassFn);
				} break;
			}
		}
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::server::WebSocket::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Выполняем удаление параметров адъютанта
		ws->worker.removeAdj(aid);
		// Если функция обратного вызова установлена, выполняем
		if(ws->openStopFn != nullptr) ws->openStopFn(aid, mode_t::DISCONNECT, ws, ws->ctx.at(0));
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
bool awh::server::WebSocket::acceptCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(ws->acceptFn != nullptr) result = ws->acceptFn(ip, mac, ws, ws->ctx.at(5));
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
void awh::server::WebSocket::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Объект сообщения
			mess_t mess;
			// Если рукопожатие не выполнено
			if(!reinterpret_cast <http_t *> (&adj->http)->isHandshake()){
				// Добавляем полученные данные в буфер
				adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
				// Выполняем парсинг полученных данных
				adj->http.parse(adj->buffer.data(), adj->buffer.size());
				// Если все данные получены
				if(adj->http.isEnd()){
					// Выполняем сброс данных буфера
					adj->buffer.clear();
					// Метод компрессии данных
					http_t::compress_t compress = http_t::compress_t::NONE;
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = reinterpret_cast <http_t *> (&adj->http)->request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->http.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << ws->fmk->format("<body %u>", adj->http.getBody().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Бинарный буфер ответа сервера
					vector <char> response;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->http.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Если рукопожатие выполнено
							if(adj->http.isHandshake()){
								// Получаем метод компрессии HTML данных
								compress = adj->http.extractCompression();
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
								// Устанавливаем размер скользящего окна
								adj->hash.setWbit(adj->http.getWbitServer());
								// Если разрешено выполнять перехват контекста компрессии для сервера
								if(adj->http.getServerTakeover())
									// Разрешаем перехватывать контекст компрессии для клиента
									adj->hash.setTakeoverCompress(true);
								// Если разрешено выполнять перехват контекста компрессии для клиента
								if(adj->http.getClientTakeover())
									// Разрешаем перехватывать контекст компрессии для сервера
									adj->hash.setTakeoverDecompress(true);
								// Получаем бинарные данные REST запроса
								response = adj->http.response();
								// Если бинарные данные ответа получены
								if(!response.empty()){
									// Если включён режим отладки
									#if defined(DEBUG_MODE)
										// Выводим заголовок ответа
										cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl << endl;
									#endif
									// Отправляем сообщение клиенту
									core->write(response.data(), response.size(), aid);
									// Если функция обратного вызова установлена, выполняем
									if(ws->openStopFn != nullptr) ws->openStopFn(aid, mode_t::CONNECT, ws, ws->ctx.at(0));
									// Завершаем работу
									return;
								// Выполняем реджект
								} else {
									// Выполняем сброс состояния HTTP парсера
									adj->http.clear();
									// Выполняем сброс состояния HTTP парсера
									adj->http.reset();
									// Формируем ответ, что страница не доступна
									response = adj->http.reject(500);
								}
							// Сообщаем, что рукопожатие не выполнено
							} else {
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->http.reset();
								// Формируем ответ, что страница не доступна
								response = adj->http.reject(403);
							}
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
						// Если включён режим отладки
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl << endl;
						#endif
						// Устанавливаем метод компрессии данных ответа
						adj->http.setCompress(compress);
						// Устанавливаем размер стопбайт
						if(!adj->http.isAlive()) adj->stopBytes = response.size();
						// Отправляем ответ клиенту
						core->write(response.data(), response.size(), aid);
						// Получаем данные тела запроса
						while(!(payload = adj->http.payload()).empty()){
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								// Выводим сообщение о выводе чанка полезной нагрузки
								cout << ws->fmk->format("<chunk %u>", payload.size()) << endl;
							#endif
							// Устанавливаем размер стопбайт
							if(!adj->http.isAlive()) adj->stopBytes += payload.size();
							// Отправляем тело на сервер
							core->write(payload.data(), payload.size(), aid);
						}
					// Завершаем работу клиента
					} else reinterpret_cast <server::core_t *> (core)->close(aid);
				}
				// Завершаем работу
				return;
			// Если рукопожатие выполнено
			} else {
				// Создаём объект шапки фрейма
				frame_t::head_t head;
				// Добавляем полученные данные в буфер
				adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
				// Выполняем обработку полученных данных
				while(!adj->close){
					// Выполняем чтение фрейма WebSocket
					const auto & data = ws->frame.get(head, adj->buffer.data(), adj->buffer.size());
					// Если буфер данных получен
					if(!data.empty()){
						// Проверяем состояние флагов RSV2 и RSV3
						if(head.rsv[1] || head.rsv[2]){
							// Создаём сообщение
							mess = mess_t(1002, "RSV2 and RSV3 must be clear");
							// Выполняем отключение клиента
							goto Stop;
						}
						// Если флаг компресси включён а данные пришли не сжатые
						if(head.rsv[0] && ((adj->compress == http_t::compress_t::NONE) ||
						(head.optcode == frame_t::opcode_t::CONTINUATION) ||
						(((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)))){
							// Создаём сообщение
							mess = mess_t(1002, "RSV1 must be clear");
							// Выполняем отключение клиента
							goto Stop;
						}
						// Если опкоды требуют финального фрейма
						if(!head.fin && ((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)){
							// Создаём сообщение
							mess = mess_t(1002, "FIN must be set");
							// Выполняем отключение клиента
							goto Stop;
						}
						// Определяем тип ответа
						switch((uint8_t) head.optcode){
							// Если ответом является PING
							case (uint8_t) frame_t::opcode_t::PING:
								// Отправляем ответ клиенту
								ws->pong(aid, core, string(data.begin(), data.end()));
							break;
							// Если ответом является PONG
							case (uint8_t) frame_t::opcode_t::PONG: {
								// Если идентификатор адъютанта совпадает
								if(memcmp(to_string(aid).c_str(), data.data(), data.size()) == 0)
									// Обновляем контрольную точку
									adj->checkPoint = ws->fmk->unixTimestamp();
							} break;
							// Если ответом является TEXT
							case (uint8_t) frame_t::opcode_t::TEXT:
							// Если ответом является BINARY
							case (uint8_t) frame_t::opcode_t::BINARY: {
								// Запоминаем полученный опкод
								adj->opcode = head.optcode;
								// Запоминаем, что данные пришли сжатыми
								adj->compressed = (head.rsv[0] && (adj->compress != http_t::compress_t::NONE));
								// Если сообщение не замаскированно
								if(!head.mask){
									// Создаём сообщение
									mess = mess_t(1002, "not masked frame from client");
									// Выполняем отключение клиента
									goto Stop;
								// Если список фрагментированных сообщений существует
								} else if(!adj->fragmes.empty()) {
									// Очищаем список фрагментированных сообщений
									adj->fragmes.clear();
									// Создаём сообщение
									mess = mess_t(1002, "opcode for subsequent fragmented messages should not be set");
									// Выполняем отключение клиента
									goto Stop;
								// Если сообщение является не последнем
								} else if(!head.fin)
									// Заполняем фрагментированное сообщение
									adj->fragmes.insert(adj->fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								else ws->extraction(adj, aid, core, data, (adj->opcode == frame_t::opcode_t::TEXT));
							} break;
							// Если ответом является CONTINUATION
							case (uint8_t) frame_t::opcode_t::CONTINUATION: {
								// Заполняем фрагментированное сообщение
								adj->fragmes.insert(adj->fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								if(head.fin){
									// Выполняем извлечение данных
									ws->extraction(adj, aid, core, adj->fragmes, (adj->opcode == frame_t::opcode_t::TEXT));
									// Очищаем список фрагментированных сообщений
									adj->fragmes.clear();
								}
							} break;
							// Если ответом является CLOSE
							case (uint8_t) frame_t::opcode_t::CLOSE: {
								// Создаём сообщение
								mess = ws->frame.message(data);
								// Выводим сообщение об ошибке
								ws->error(aid, mess);
								// Завершаем работу
								reinterpret_cast <server::core_t *> (core)->close(aid);
								// Выходим из функции
								return;
							} break;
						}
					}
					// Если парсер обработал какое-то количество байт
					if((head.frame > 0) && !adj->buffer.empty()){
						// Если размер буфера больше количества удаляемых байт
						if(adj->buffer.size() >= head.frame)
							// Удаляем количество обработанных байт
							vector <decltype(adj->buffer)::value_type> (adj->buffer.begin() + head.frame, adj->buffer.end()).swap(adj->buffer);
						// Если байт в буфере меньше, просто очищаем буфер
						else adj->buffer.clear();
						// Если данных для обработки не осталось, выходим
						if(adj->buffer.empty()) break;
					// Если данных для обработки недостаточно, выходим
					} else break;
				}
				// Выходим из функции
				return;
			}
			// Устанавливаем метку остановки клиента
			Stop:
			// Отправляем серверу сообщение
			ws->sendError(aid, mess);
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
void awh::server::WebSocket::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (ws->worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && !adj->close && (adj->stopBytes > 0)){
			// Запоминаем количество прочитанных байт
			adj->readBytes += size;
			// Если размер полученных байт соответствует
			adj->close = (adj->stopBytes >= adj->readBytes);
		}
	}
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param aid     идентификатор адъютанта
 * @param message сообщение с описанием ошибки
 */
void awh::server::WebSocket::error(const size_t aid, const mess_t & message) const noexcept {
	// Если идентификатор адъютанта передан и код ошибки указан
	if((aid > 0) && (message.code > 0)){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова установлена, выводим полученное сообщение
			if(this->errorFn != nullptr) this->errorFn(aid, message.code, message.text, const_cast <WebSocket *> (this), this->ctx.at(1));
		// Если функция обратного вызова установлена, выводим только код ошибки
		} else if(this->errorFn != nullptr) this->errorFn(aid, message.code, "", const_cast <WebSocket *> (this), this->ctx.at(1));
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param adj    параметры подключения адъютанта
 * @param aid    идентификатор адъютанта
 * @param core   объект биндинга TCP/IP
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::server::WebSocket::extraction(workerWS_t::adjp_t * adj, const size_t aid, awh::core_t * core, const vector <char> & buffer, const bool utf8) const noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && (this->messageFn != nullptr)){
		// Если данные пришли в сжатом виде
		if(adj->compressed && (adj->compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch((uint8_t) adj->compress){
				// Если метод компрессии выбран Deflate
				case (uint8_t) http_t::compress_t::DEFLATE: {
					// Добавляем хвост в полученные данные
					adj->hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = adj->hash.decompress(buffer.data(), buffer.size());
				} break;
				// Если метод компрессии выбран GZip
				case (uint8_t) http_t::compress_t::GZIP:
					// Выполняем декомпрессию полученных данных
					data = adj->hash.decompressGzip(buffer.data(), buffer.size());
				break;
				// Если метод компрессии выбран Brotli
				case (uint8_t) http_t::compress_t::BROTLI:
					// Выполняем декомпрессию полученных данных
					data = adj->hash.decompressBrotli(buffer.data(), buffer.size());
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(adj->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = adj->hash.decrypt(data.data(), data.size());
					// Отправляем полученный результат
					if(!res.empty()) this->messageFn(aid, res, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
					// Иначе выводим сообщение так - как оно пришло
					else this->messageFn(aid, data, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
				// Отправляем полученный результат
				} else this->messageFn(aid, data, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Получаем буфер сообщения
				const auto & buffer = this->frame.message(mess);
				// Если данные сообщения получены
				if(!buffer.empty()){
					// Устанавливаем размер стопбайт
					adj->stopBytes = buffer.size();
					// Отправляем серверу сообщение
					core->write(buffer.data(), buffer.size(), aid);
				// Завершаем работу
				} else reinterpret_cast <server::core_t *> (core)->close(aid);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(adj->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = adj->hash.decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				if(!res.empty()) this->messageFn(aid, res, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
				// Иначе выводим сообщение так - как оно пришло
				else this->messageFn(aid, buffer, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
			// Отправляем полученный результат
			} else this->messageFn(aid, buffer, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект биндинга TCP/IP
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket::pong(const size_t aid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && !adj->locker){
			// Создаём буфер для отправки
			const auto & buffer = this->frame.pong(message);
			// Отправляем серверу сообщение
			core->write(buffer.data(), buffer.size(), aid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект биндинга TCP/IP
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket::ping(const size_t aid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr) && core->working()){
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && !adj->locker){
			// Создаём буфер для отправки
			const auto & buffer = this->frame.ping(message);
			// Отправляем серверу сообщение
			core->write(buffer.data(), buffer.size(), aid);
		}
	}
}
/**
 * init Метод инициализации WebSocket клиента
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
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
void awh::server::WebSocket::on(void * ctx, function <void (const size_t, const mode_t, WebSocket *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->openStopFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(void * ctx, function <void (const size_t, const u_int, const string &, WebSocket *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию получения ошибок
	this->errorFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(void * ctx, function <void (const size_t, const vector <char> &, const bool, WebSocket *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(2) = ctx;
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::WebSocket::on(void * ctx, function <string (const string &, void *)> callback) noexcept {
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
void awh::server::WebSocket::on(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(4) = ctx;
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->checkAuthFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket::on(void * ctx, function <bool (const string &, const string &, WebSocket *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(5) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->acceptFn = callback;
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param aid  идентификатор адъютанта
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::WebSocket::sendError(const size_t aid, const mess_t & mess) const noexcept {
	// Если подключение выполнено
	if(this->core->working()){
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && !adj->locker){
			// Получаем буфер сообщения
			const auto & buffer = this->frame.message(mess);
			// Если данные сообщения получены
			if(!buffer.empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим отправляемое сообщение
					cout << this->fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
				#endif
				// Устанавливаем размер стопбайт
				adj->stopBytes = buffer.size();
				// Отправляем серверу сообщение
				((awh::core_t *) const_cast <server::core_t *> (this->core))->write(buffer.data(), buffer.size(), aid);
				// Выходим из функции
				return;
			}
		}
		// Завершаем работу
		const_cast <server::core_t *> (this->core)->close(aid);
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param aid     идентификатор адъютанта
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::server::WebSocket::send(const size_t aid, const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if(this->core->working() && (aid > 0) && (size > 0) && (message != nullptr)){
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && !adj->locker){
			// Выполняем блокировку отправки сообщения
			adj->locker = !adj->locker;
			// Если рукопожатие выполнено
			if(adj->http.isHandshake()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Если отправляемое сообщение является текстом
					if(utf8)
						// Выводим параметры ответа
						cout << string(message, size) << endl << endl;
					// Выводим сообщение о выводе чанка полезной нагрузки
					else cout << this->fmk->format("<bytes %u>", size) << endl << endl;
				#endif
				// Буфер сжатых данных
				vector <char> buffer;
				// Создаём объект заголовка для отправки
				frame_t::head_t head;
				// Передаём сообщение одним запросом
				head.fin = true;
				// Выполняем маскировку сообщения
				head.mask = false;
				// Указываем, что сообщение передаётся в сжатом виде
				head.rsv[0] = (adj->compress != http_t::compress_t::NONE);
				// Устанавливаем опкод сообщения
				head.optcode = (utf8 ? frame_t::opcode_t::TEXT : frame_t::opcode_t::BINARY);
				// Если нужно производить шифрование
				if(this->crypt){
					// Выполняем шифрование переданных данных
					buffer = adj->hash.encrypt(message, size);
					// Если данные зашифрованны
					if(!buffer.empty()){
						// Заменяем сообщение для передачи
						message = buffer.data();
						// Заменяем размер сообщения
						(* const_cast <size_t *> (&size)) = buffer.size();
					}
				}
				/**
				 * sendFn Функция отправки сообщения на сервер
				 * @param head    объект заголовков фрейма WebSocket
				 * @param message буфер сообщения в бинарном виде
				 * @param size    размер сообщения в байтах
				 */
				auto sendFn = [aid, adj, this](const frame_t::head_t & head, const char * message, const size_t size) noexcept {
					// Если все данные переданы
					if((message != nullptr) && (size > 0)){
						// Если необходимо сжимать сообщение перед отправкой
						if(adj->compress != http_t::compress_t::NONE){
							// Компрессионные данные
							vector <char> data;
							// Определяем метод компрессии
							switch((uint8_t) adj->compress){
								// Если метод компрессии выбран Deflate
								case (uint8_t) http_t::compress_t::DEFLATE: {
									// Выполняем компрессию полученных данных
									data = adj->hash.compress(message, size);
									// Удаляем хвост в полученных данных
									adj->hash.rmTail(data);
								} break;
								// Если метод компрессии выбран GZip
								case (uint8_t) http_t::compress_t::GZIP:
									// Выполняем компрессию полученных данных
									data = adj->hash.compressGzip(message, size);
								break;
								// Если метод компрессии выбран Brotli
								case (uint8_t) http_t::compress_t::BROTLI:
									// Выполняем компрессию полученных данных
									data = adj->hash.compressBrotli(message, size);
								break;
							}
							// Если сжатие данных прошло удачно
							if(!data.empty()){
								// Создаём буфер для отправки
								const auto & buffer = this->frame.set(head, data.data(), data.size());
								// Отправляем серверу сообщение
								((awh::core_t *) const_cast <server::core_t *> (this->core))->write(buffer.data(), buffer.size(), aid);
							// Если сжать данные не получилось
							} else {
								// Снимаем флаг сжатых данных
								const_cast <frame_t::head_t *> (&head)->rsv[0] = false;
								// Создаём буфер для отправки
								const auto & buffer = this->frame.set(head, message, size);
								// Отправляем серверу сообщение
								((awh::core_t *) const_cast <server::core_t *> (this->core))->write(buffer.data(), buffer.size(), aid);
							}
						// Если сообщение перед отправкой сжимать не нужно
						} else {
							// Создаём буфер для отправки
							const auto & buffer = this->frame.set(head, message, size);
							// Отправляем серверу сообщение
							((awh::core_t *) const_cast <server::core_t *> (this->core))->write(buffer.data(), buffer.size(), aid);
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
			adj->locker = !adj->locker;
		}
	}
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::WebSocket::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.ip(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::WebSocket::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->worker.mac(aid);
}
/**
 * stop Метод остановки клиента
 */
void awh::server::WebSocket::stop() noexcept {
	// Если подключение выполнено
	if(this->core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <server::core_t *> (this->core)->stop();
}
/**
 * start Метод запуска клиента
 */
void awh::server::WebSocket::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->core)->start();
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::server::WebSocket::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->subs.push_back(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::server::WebSocket::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->subs = subs;
}
/**
 * getSub Метод получения выбранного сабпротокола
 * @param aid идентификатор адъютанта
 * @return    название поддерживаемого сабпротокола
 */
const string awh::server::WebSocket::getSub(const size_t aid) const noexcept {
	// Результат работы функции
	string result = "";
	// Если идентификатор адъютанта передан
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		workerWS_t::adjp_t * adj = const_cast <workerWS_t::adjp_t *> (this->worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr) result = adj->http.getSub();
	}
	// Выводим результат
	return result;
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::WebSocket::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::WebSocket::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WebSocket::setRealm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->realm = realm;
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WebSocket::setOpaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->opaque = opaque;
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WebSocket::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->authHash = hash;
	// Устанавливаем тип авторизации
	this->authType = type;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::WebSocket::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->takeOverCli = (flag & (uint8_t) flag_t::TAKEOVERCLI);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->takeOverSrv = (flag & (uint8_t) flag_t::TAKEOVERSRV);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <server::core_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <server::core_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::WebSocket::setTotal(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->core)->setTotal(this->worker.wid, total);
}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::WebSocket::setFrameSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->frameSize = size;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::WebSocket::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->worker.compress = compress;
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::WebSocket::setServ(const string & id, const string & name, const string & ver) noexcept {
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
void awh::server::WebSocket::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
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
 * WebSocket Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::WebSocket::WebSocket(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : frame(fmk, log), core(core), fmk(fmk), log(log), worker(fmk, log) {
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
}
