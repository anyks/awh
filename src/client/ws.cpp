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
#include <client/ws.hpp>

/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::WebSocket::openCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Выполняем подключение
	reinterpret_cast <client::core_t *> (core)->open(wid);
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::WebSocket::persistCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Получаем текущий штамп времени
		const time_t stamp = ws->fmk->unixTimestamp();
		// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
		if(ws->close || ((stamp - ws->checkPoint) >= (PERSIST_INTERVAL * 5)))
			// Завершаем работу
			reinterpret_cast <client::core_t *> (core)->close(aid);
		// Отправляем запрос адъютанту
		else ws->ping(to_string(aid));
	}
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::WebSocket::connectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Запоминаем объект адъютанта
		ws->aid = aid;
		// Выполняем сброс параметров запроса
		ws->flush();
		// Выполняем сброс состояния HTTP парсера
		ws->http.reset();
		// Выполняем очистку параметров HTTP запроса
		ws->http.clear();
		// Устанавливаем метод сжатия
		ws->http.setCompress(ws->compress);
		// Разрешаем перехватывать контекст для клиента
		ws->http.setClientTakeover(ws->takeOverCli);
		// Разрешаем перехватывать контекст для сервера
		ws->http.setServerTakeover(ws->takeOverSrv);
		// Разрешаем перехватывать контекст компрессии
		ws->hash.setTakeoverCompress(ws->takeOverCli);
		// Разрешаем перехватывать контекст декомпрессии
		ws->hash.setTakeoverDecompress(ws->takeOverSrv);
		// Получаем бинарные данные REST запроса
		const auto & request = ws->http.request(ws->worker.url);
		// Если бинарные данные запроса получены
		if(!request.empty()){
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок запроса
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры запроса
				cout << string(request.begin(), request.end()) << endl << endl;
			#endif
			// Отправляем серверу сообщение
			core->write(request.data(), request.size(), aid);
		}
	}
}
/**
 * closeCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::WebSocket::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Выполняем сброс параметров запроса
		ws->flush();
		// Если нужно произвести запрос заново
		if((ws->code == 301) || (ws->code == 308) ||
		   (ws->code == 401) || (ws->code == 407)){
			// Выполняем запрос заново
			reinterpret_cast <client::core_t *> (core)->open(ws->worker.wid);
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
 * connectProxyCallback Функция обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::WebSocket::connectProxyCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Определяем тип прокси-сервера
		switch((uint8_t) ws->worker.proxy.type){
			// Если прокси-сервер является Socks5
			case (uint8_t) proxy_t::type_t::SOCKS5: {
				// Выполняем сброс состояния Socks5 парсера
				ws->worker.proxy.socks5.reset();
				// Устанавливаем URL адрес запроса
				ws->worker.proxy.socks5.setUrl(ws->worker.url);
				// Выполняем создание буфера запроса
				ws->worker.proxy.socks5.parse();
				// Получаем данные запроса
				const auto & socks5 = ws->worker.proxy.socks5.get();
				// Если данные получены
				if(!socks5.empty()) core->write(socks5.data(), socks5.size(), aid);
			} break;
			// Если прокси-сервер является HTTP
			case (uint8_t) proxy_t::type_t::HTTP: {
				// Выполняем сброс состояния HTTP парсера
				ws->worker.proxy.http.reset();
				// Выполняем очистку параметров HTTP запроса
				ws->worker.proxy.http.clear();
				// Получаем бинарные данные REST запроса
				const auto & proxy = ws->worker.proxy.http.proxy(ws->worker.url);
				// Если бинарные данные запроса получены
				if(!proxy.empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим заголовок запроса
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры запроса
						cout << string(proxy.begin(), proxy.end()) << endl;
					#endif
					// Отправляем на прокси-сервер
					core->write(proxy.data(), proxy.size(), aid);
				}
			} break;
			// Иначе завершаем работу
			default: reinterpret_cast <client::core_t *> (core)->close(aid);
		}
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::client::WebSocket::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {	
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Объект сообщения
		mess_t mess;
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Если подключение закрыто
		if(ws->close){
			// Принудительно выполняем отключение лкиента
			reinterpret_cast <client::core_t *> (core)->close(aid);
			// Выходим из функции
			return;
		}
		// Если рукопожатие не выполнено
		if(!reinterpret_cast <http_t *> (&ws->http)->isHandshake()){
			// Добавляем полученные данные в буфер
			ws->buffer.insert(ws->buffer.end(), buffer, buffer + size);
			// Выполняем парсинг полученных данных
			size_t bytes = ws->http.parse(ws->buffer.data(), ws->buffer.size());
			// Если все данные получены
			if(ws->http.isEnd()){
				// Выполняем сброс данных буфера
				ws->buffer.clear();
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Получаем данные ответа
					const auto & response = reinterpret_cast <http_t *> (&ws->http)->response(true);
					// Если параметры ответа получены
					if(!response.empty()){
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(response.begin(), response.end()) << endl;
						// Если тело ответа существует
						if(!ws->http.getBody().empty())
							// Выводим сообщение о выводе чанка тела
							cout << ws->fmk->format("<body %u>", ws->http.getBody().size()) << endl << endl;
						// Иначе устанавливаем перенос строки
						else cout << endl;
					}
				#endif
				// Выполняем проверку авторизации
				switch((uint8_t) ws->http.getAuth()){
					// Если нужно попытаться ещё раз
					case (uint8_t) http_t::stath_t::RETRY: {
						// Если попытка повторить авторизацию ещё не проводилась
						if(!ws->failAuth){
							// Получаем новый адрес запроса
							ws->worker.url = ws->http.getUrl();
							// Если адрес запроса получен
							if(!ws->worker.url.empty()){
								// Запоминаем, что попытка выполнена
								ws->failAuth = true;
								// Если соединение является постоянным
								if(ws->http.isAlive())
									// Выполняем повторно отправку сообщения на сервер
									connectCallback(aid, wid, core, ctx);
								// Завершаем работу
								else reinterpret_cast <client::core_t *> (core)->close(aid);
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем код ответа
						ws->code = 403;
						// Создаём сообщение
						mess = mess_t(ws->code, ws->http.getMessage(ws->code));
						// Выводим сообщение
						ws->error(mess);
					} break;
					// Если запрос выполнен удачно
					case (uint8_t) http_t::stath_t::GOOD: {
						// Если рукопожатие выполнено
						if(ws->http.isHandshake()){
							// Очищаем буфер собранных данных
							ws->buffer.clear();
							// Очищаем список фрагментированных сообщений
							ws->fragmes.clear();
							// Выполняем сброс количество попыток
							ws->failAuth = false;
							// Получаем флаг шифрованных данных
							ws->crypt = ws->http.isCrypt();
							// Получаем поддерживаемый метод компрессии
							ws->compress = ws->http.getCompress();
							// Обновляем контрольную точку
							ws->checkPoint = ws->fmk->unixTimestamp();
							// Устанавливаем размер скользящего окна
							ws->hash.setWbit(ws->http.getWbitServer());
							// Если разрешено выполнять перехват контекста компрессии для клиента
							if(ws->http.getClientTakeover())
								// Разрешаем перехватывать контекст компрессии для клиента
								ws->hash.setTakeoverCompress(true);
							// Если разрешено выполнять перехват контекста компрессии для сервера
							if(ws->http.getServerTakeover())
								// Разрешаем перехватывать контекст компрессии для сервера
								ws->hash.setTakeoverDecompress(true);
							// Выводим в лог сообщение
							if(!ws->noinfo) ws->log->print("%s", log_t::flag_t::INFO, "authorization on the WebSocket server was successful");
							// Если функция обратного вызова установлена, выполняем
							if(ws->activeFn != nullptr) ws->activeFn(mode_t::CONNECT, ws, ws->ctx.at(0));
							// Есла данных передано больше чем обработано
							if(size > bytes)
								// Выполняем обработку дальше
								readCallback(buffer + bytes, size - bytes, aid, wid, core, ctx);
							// Завершаем работу
							return;
						// Сообщаем, что рукопожатие не выполнено
						} else {
							// Устанавливаем код ответа
							ws->code = 404;
							// Создаём сообщение
							mess = mess_t(ws->code, ws->http.getMessage(ws->code));
							// Выводим сообщение
							ws->error(mess);
						}
					} break;
					// Если запрос неудачный
					case (uint8_t) http_t::stath_t::FAULT: {
						// Получаем параметры запроса
						const auto & query = ws->http.getQuery();
						// Устанавливаем код ответа
						ws->code = query.code;
						// Создаём сообщение
						mess = mess_t(ws->code, query.message);
						// Запрещаем бесконечный редирект при запросе авторизации
						if((ws->code == 401) || (ws->code == 407)){
							// Устанавливаем код ответа
							ws->code = 403;
							// Присваиваем сообщению, новое значение кода
							mess = ws->code;
						}
						// Выводим сообщение
						ws->error(mess);
					} break;
				}
				// Выполняем сброс количество попыток
				ws->failAuth = false;
				// Завершаем работу
				reinterpret_cast <client::core_t *> (core)->close(aid);
			}
			// Завершаем работу
			return;
		// Если рукопожатие выполнено
		} else {
			// Основные флаг остановки перебора бинарного буфера
			bool stop = false;
			// Создаём объект шапки фрейма
			frame_t::head_t head;
			// Создаём буфер сообщения
			vector <char> message;
			// Добавляем полученные данные в буфер
			ws->buffer.insert(ws->buffer.end(), buffer, buffer + size);
			// Выполняем обработку полученных данных
			while(!ws->close){
				// Выполняем чтение фрейма WebSocket
				const auto & data = ws->frame.get(head, ws->buffer.data(), ws->buffer.size());
				// Если буфер данных получен
				if(!data.empty()){
					// Очищаем буфер полученного сообщения
					message.clear();
					// Проверяем состояние флагов RSV2 и RSV3
					if(head.rsv[1] || head.rsv[2]){
						// Создаём сообщение
						mess = mess_t(1002, "RSV2 and RSV3 must be clear");
						// Выводим сообщение
						ws->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					}
					// Если флаг компресси включён а данные пришли не сжатые
					if(head.rsv[0] && ((ws->compress == http_t::compress_t::NONE) ||
					(head.optcode == frame_t::opcode_t::CONTINUATION) ||
					(((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)))){
						// Создаём сообщение
						mess = mess_t(1002, "RSV1 must be clear");
						// Выводим сообщение
						ws->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					}
					// Если опкоды требуют финального фрейма
					if(!head.fin && ((uint8_t) head.optcode > 0x07) && ((uint8_t) head.optcode < 0x0b)){
						// Создаём сообщение
						mess = mess_t(1002, "FIN must be set");
						// Выводим сообщение
						ws->error(mess);
						// Выполняем реконнект
						goto Reconnect;
					}
					// Определяем тип ответа
					switch((uint8_t) head.optcode){
						// Если ответом является PING
						case (uint8_t) frame_t::opcode_t::PING:
							// Отправляем ответ серверу
							ws->pong(string(data.begin(), data.end()));
						break;
						// Если ответом является PONG
						case (uint8_t) frame_t::opcode_t::PONG:
							// Если идентификатор адъютанта совпадает
							if(memcmp(to_string(aid).c_str(), data.data(), data.size()) == 0)
								// Обновляем контрольную точку
								ws->checkPoint = ws->fmk->unixTimestamp();
						break;
						// Если ответом является TEXT
						case (uint8_t) frame_t::opcode_t::TEXT:
						// Если ответом является BINARY
						case (uint8_t) frame_t::opcode_t::BINARY: {
							// Запоминаем полученный опкод
							ws->opcode = head.optcode;
							// Запоминаем, что данные пришли сжатыми
							ws->compressed = (head.rsv[0] && (ws->compress != http_t::compress_t::NONE));
							// Если сообщение замаскированно
							if(head.mask){
								// Создаём сообщение
								mess = mess_t(1002, "masked frame from server");
								// Выводим сообщение
								ws->error(mess);
								// Выполняем реконнект
								goto Reconnect;
							// Если список фрагментированных сообщений существует
							} else if(!ws->fragmes.empty()) {
								// Очищаем список фрагментированных сообщений
								ws->fragmes.clear();
								// Создаём сообщение
								mess = mess_t(1002, "opcode for subsequent fragmented messages should not be set");
								// Выводим сообщение
								ws->error(mess);
								// Выполняем реконнект
								goto Reconnect;
							// Если сообщение является не последнем
							} else if(!head.fin)
								// Заполняем фрагментированное сообщение
								ws->fragmes.insert(ws->fragmes.end(), data.begin(), data.end());
							// Если сообщение является последним
							else message = move(* const_cast <vector <char> *> (&data));
						} break;
						// Если ответом является CONTINUATION
						case (uint8_t) frame_t::opcode_t::CONTINUATION: {
							// Заполняем фрагментированное сообщение
							ws->fragmes.insert(ws->fragmes.end(), data.begin(), data.end());
							// Если сообщение является последним
							if(head.fin){
								// Выполняем копирование всех собранных сегментов
								message = move(* const_cast <vector <char> *> (&ws->fragmes));
								// Очищаем список фрагментированных сообщений
								ws->fragmes.clear();
							}
						} break;
						// Если ответом является CLOSE
						case (uint8_t) frame_t::opcode_t::CLOSE: {
							// Извлекаем сообщение
							mess = ws->frame.message(data);
							// Выводим сообщение
							ws->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						} break;
					}
				}
				// Если парсер обработал какое-то количество байт
				if((head.frame > 0) && !ws->buffer.empty()){
					// Если размер буфера больше количества удаляемых байт
					if(ws->buffer.size() >= head.frame)
						// Удаляем количество обработанных байт
						vector <decltype(ws->buffer)::value_type> (ws->buffer.begin() + head.frame, ws->buffer.end()).swap(ws->buffer);
					// Если байт в буфере меньше, просто очищаем буфер
					else ws->buffer.clear();
					// Если данных для обработки не осталось, выходим
					stop = ws->buffer.empty();
				// Если данных для обработки недостаточно, выходим
				} else stop = true;
				// Если сообщения получены
				if(!message.empty())
					// Выполняем извлечение полученных сообщений
					ws->extraction(message, (ws->opcode == frame_t::opcode_t::TEXT));
				// Если установлен флаг выхода, выходим
				if(stop) break;
			}
			// Выходим из функции
			return;
		}
		// Устанавливаем метку реконнекта
		Reconnect:
		// Выполняем отправку сообщения об ошибке
		ws->sendError(mess);
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
void awh::client::WebSocket::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Если стоп-байты установлены
		if(!ws->close && (ws->stopBytes > 0)){
			// Запоминаем количество прочитанных байт
			ws->readBytes += size;
			// Если размер полученных байт соответствует
			ws->close = (ws->stopBytes >= ws->readBytes);
		}
	}
}
/**
 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::client::WebSocket::readProxyCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем контекст модуля
		ws_t * ws = reinterpret_cast <ws_t *> (ctx);
		// Определяем тип прокси-сервера
		switch((uint8_t) ws->worker.proxy.type){
			// Если прокси-сервер является Socks5
			case (uint8_t) proxy_t::type_t::SOCKS5: {
				// Если данные не получены
				if(!ws->worker.proxy.socks5.isEnd()){
					// Выполняем парсинг входящих данных
					ws->worker.proxy.socks5.parse(buffer, size);
					// Получаем данные запроса
					const auto & socks5 = ws->worker.proxy.socks5.get();
					// Если данные получены
					if(!socks5.empty()) core->write(socks5.data(), socks5.size(), aid);
					// Если данные все получены
					else if(ws->worker.proxy.socks5.isEnd()) {
						// Если рукопожатие выполнено
						if(ws->worker.proxy.socks5.isHandshake()){
							// Выполняем переключение на работу с сервером
							reinterpret_cast <client::core_t *> (core)->switchProxy(aid);
							// Завершаем работу
							return;
						// Если рукопожатие не выполнено
						} else {
							// Устанавливаем код ответа
							ws->code = ws->worker.proxy.socks5.getCode();
							// Создаём сообщение
							mess_t mess(ws->code);
							// Устанавливаем сообщение ответа
							mess = ws->worker.proxy.socks5.getMessage(ws->code);
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								// Если заголовки получены
								if(!mess.text.empty()){
									// Данные REST ответа
									const string & response = ws->fmk->format("SOCKS5 %u %s\r\n", ws->code, mess.text.c_str());
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Выводим сообщение
							ws->error(mess);
							// Завершаем работу
							reinterpret_cast <client::core_t *> (core)->close(aid);
						}
					}
				}
			} break;
			// Если прокси-сервер является HTTP
			case (uint8_t) proxy_t::type_t::HTTP: {
				// Выполняем парсинг полученных данных
				ws->worker.proxy.http.parse(buffer, size);
				// Если все данные получены
				if(ws->worker.proxy.http.isEnd()){
					// Получаем параметры запроса
					const auto & query = ws->worker.proxy.http.getQuery();
					// Устанавливаем код ответа
					ws->code = query.code;
					// Создаём сообщение
					mess_t mess(ws->code, query.message);
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные ответа
						const auto & response = ws->worker.proxy.http.response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!ws->worker.proxy.http.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << ws->fmk->format("<body %u>", ws->worker.proxy.http.getBody().size())  << endl;
						}
					#endif
					// Выполняем проверку авторизации
					switch((uint8_t) ws->worker.proxy.http.getAuth()){
						// Если нужно попытаться ещё раз
						case (uint8_t) http_t::stath_t::RETRY: {
							// Если попытка повторить авторизацию ещё не проводилась
							if(!ws->failAuth){
								// Получаем новый адрес запроса
								ws->worker.proxy.url = ws->worker.proxy.http.getUrl();
								// Если адрес запроса получен
								if(!ws->worker.proxy.url.empty()){
									// Запоминаем, что попытка выполнена
									ws->failAuth = true;
									// Если соединение является постоянным
									if(ws->worker.proxy.http.isAlive())
										// Выполняем повторно отправку сообщения на сервер
										connectProxyCallback(aid, wid, core, ctx);
									// Завершаем работу
									else reinterpret_cast <client::core_t *> (core)->close(aid);
									// Завершаем работу
									return;
								}
							}
							// Устанавливаем код ответа
							ws->code = 403;
							// Присваиваем сообщению, новое значение кода
							mess = ws->code;
						} break;
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Выполняем сброс количество попыток
							ws->failAuth = false;
							// Выполняем переключение на работу с сервером
							reinterpret_cast <client::core_t *> (core)->switchProxy(aid);
							// Завершаем работу
							return;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Запрещаем бесконечный редирект при запросе авторизации
							if((ws->code == 401) || (ws->code == 407)){
								// Устанавливаем код ответа
								ws->code = 403;
								// Присваиваем сообщению, новое значение кода
								mess = ws->code;
							}
						} break;
					}
					// Выводим сообщение
					ws->error(mess);
					// Выполняем сброс количество попыток
					ws->failAuth = false;
					// Завершаем работу
					reinterpret_cast <client::core_t *> (core)->close(aid);
				}
			} break;
			// Иначе завершаем работу
			default: reinterpret_cast <client::core_t *> (core)->close(aid);
		}
	}
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WebSocket::error(const mess_t & message) const noexcept {
	// Очищаем список буффер бинарных данных
	const_cast <ws_t *> (this)->buffer.clear();
	// Очищаем список фрагментированных сообщений
	const_cast <ws_t *> (this)->fragmes.clear();
	// Если код ошибки указан
	if(message.code > 0){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова установлена, выводим полученное сообщение
			if(this->errorFn != nullptr) this->errorFn(message.code, message.text, const_cast <WebSocket *> (this), this->ctx.at(1));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::client::WebSocket::extraction(const vector <char> & buffer, const bool utf8) noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->freeze && (this->messageFn != nullptr)){
		// Если данные пришли в сжатом виде
		if(this->compressed && (this->compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch((uint8_t) this->compress){
				// Если метод компрессии выбран Deflate
				case (uint8_t) http_t::compress_t::DEFLATE: {
					// Добавляем хвост в полученные данные
					this->hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->hash.decompress(buffer.data(), buffer.size());
				} break;
				// Если метод компрессии выбран GZip
				case (uint8_t) http_t::compress_t::GZIP:
					// Выполняем декомпрессию полученных данных
					data = this->hash.decompressGzip(buffer.data(), buffer.size());
				break;
				// Если метод компрессии выбран Brotli
				case (uint8_t) http_t::compress_t::BROTLI:
					// Выполняем декомпрессию полученных данных
					data = this->hash.decompressBrotli(buffer.data(), buffer.size());
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash.decrypt(data.data(), data.size());
					// Отправляем полученный результат
					if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
					// Иначе выводим сообщение так - как оно пришло
					else this->messageFn(data, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
				// Отправляем полученный результат
				} else this->messageFn(data, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Выводим сообщение
				this->error(mess);
				// Иначе выводим сообщение так - как оно пришло
				this->messageFn(buffer, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
				// Выполняем отправку сообщения об ошибке
				this->sendError(mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash.decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				if(!res.empty()) this->messageFn(res, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
				// Иначе выводим сообщение так - как оно пришло
				else this->messageFn(buffer, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
			// Отправляем полученный результат
			} else this->messageFn(buffer, utf8, const_cast <WebSocket *> (this), this->ctx.at(2));
		}
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WebSocket::flush() noexcept {
	// Снимаем флаг отключения
	this->close = false;
	// Выполняем сброс количество стоп-байт
	this->stopBytes = 0;
	// Выполняем сброс количества прочитанных байт
	this->readBytes = 0;
	// Очищаем буфер собранных данных
	this->buffer.clear();
	// Очищаем буфер фрагментированного сообщения
	this->fragmes.clear();
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->core->working() && !this->locker){
		// Если рукопожатие выполнено
		if(this->http.isHandshake() && (this->aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->frame.pong(message, true);
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <client::core_t *> (this->core))->write(buffer.data(), buffer.size(), this->aid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->core->working() && !this->locker){
		// Если рукопожатие выполнено
		if(this->http.isHandshake() && (this->aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->frame.ping(message, true);
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <client::core_t *> (this->core))->write(buffer.data(), buffer.size(), this->aid);
		}
	}
}
/**
 * init Метод инициализации WebSocket клиента
 * @param url      адрес WebSocket сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::WebSocket::init(const string & url, const http_t::compress_t compress) noexcept {
	// Если адрес сервера передан
	if(!url.empty()){
		// Выполняем очистку воркера
		this->worker.clear();
		// Устанавливаем метод компрессии сообщений
		this->compress = compress;
		// Устанавливаем URL адрес запроса
		this->worker.url = this->uri.parseUrl(url);
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(void * ctx, function <void (const mode_t, WebSocket *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию запуска и остановки
	this->activeFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket::on(void * ctx, function <void (const u_int, const string &, WebSocket *, void *)> callback) noexcept {
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
void awh::client::WebSocket::on(void * ctx, function <void (const vector <char> &, const bool, WebSocket *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(2) = ctx;
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::WebSocket::sendTimeout() noexcept {
	// Если подключение выполнено
	if(this->core->working() && !this->locker)
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->core)->sendTimeout(this->aid);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WebSocket::sendError(const mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->core->working() && !this->locker && (this->aid > 0)){
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
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
				// Запоминаем рамер данных для остановки
				this->stopBytes = buffer.size();
				// Отправляем серверу сообщение
				((awh::core_t *) const_cast <client::core_t *> (this->core))->write(buffer.data(), buffer.size(), this->aid);
				// Выходим из функции
				return;
			}
		}
		// Завершаем работу
		const_cast <client::core_t *> (this->core)->close(this->aid);
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::WebSocket::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if(this->core->working() && !this->locker){
		// Выполняем блокировку отправки сообщения
		this->locker = !this->locker;
		// Если рукопожатие выполнено
		if((message != nullptr) && (size > 0) && this->http.isHandshake() && (this->aid > 0)){
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
			head.mask = true;
			// Указываем, что сообщение передаётся в сжатом виде
			head.rsv[0] = (this->compress != http_t::compress_t::NONE);
			// Устанавливаем опкод сообщения
			head.optcode = (utf8 ? frame_t::opcode_t::TEXT : frame_t::opcode_t::BINARY);
			// Если нужно производить шифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				buffer = this->hash.encrypt(message, size);
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
			auto sendFn = [this](const frame_t::head_t & head, const char * message, const size_t size) noexcept {
				// Если все данные переданы
				if((message != nullptr) && (size > 0)){
					// Если необходимо сжимать сообщение перед отправкой
					if(this->compress != http_t::compress_t::NONE){
						// Компрессионные данные
						vector <char> data;
						// Определяем метод компрессии
						switch((uint8_t) this->compress){
							// Если метод компрессии выбран Deflate
							case (uint8_t) http_t::compress_t::DEFLATE: {
								// Выполняем компрессию полученных данных
								data = this->hash.compress(message, size);
								// Удаляем хвост в полученных данных
								this->hash.rmTail(data);
							} break;
							// Если метод компрессии выбран GZip
							case (uint8_t) http_t::compress_t::GZIP:
								// Выполняем компрессию полученных данных
								data = this->hash.compressGzip(message, size);
							break;
							// Если метод компрессии выбран Brotli
							case (uint8_t) http_t::compress_t::BROTLI:
								// Выполняем компрессию полученных данных
								data = this->hash.compressBrotli(message, size);
							break;
						}
						// Если сжатие данных прошло удачно
						if(!data.empty()){
							// Создаём буфер для отправки
							const auto & buffer = this->frame.set(head, data.data(), data.size());
							// Отправляем серверу сообщение
							((awh::core_t *) const_cast <client::core_t *> (this->core))->write(buffer.data(), buffer.size(), this->aid);
						// Если сжать данные не получилось
						} else {
							// Снимаем флаг сжатых данных
							const_cast <frame_t::head_t *> (&head)->rsv[0] = false;
							// Создаём буфер для отправки
							const auto & buffer = this->frame.set(head, message, size);
							// Отправляем серверу сообщение
							((awh::core_t *) const_cast <client::core_t *> (this->core))->write(buffer.data(), buffer.size(), this->aid);
						}
					// Если сообщение перед отправкой сжимать не нужно
					} else {
						// Создаём буфер для отправки
						const auto & buffer = this->frame.set(head, message, size);
						// Отправляем серверу сообщение
						((awh::core_t *) const_cast <client::core_t *> (this->core))->write(buffer.data(), buffer.size(), this->aid);
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
void awh::client::WebSocket::stop() noexcept {
	// Очищаем код ответа
	this->code = 0;
	// Если подключение выполнено
	if(this->core->working()){
		// Завершаем работу, если разрешено остановить
		if(this->unbind) const_cast <client::core_t *> (this->core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else {
			/**
			 * Если установлено постоянное подключение
			 * нам нужно заблокировать автоматический реконнект.
			 */
			// Считываем значение флага
			const bool alive = this->worker.alive;
			// Выполняем отключение флага постоянного подключения
			this->worker.alive = false;
			// Выполняем отключение клиента
			const_cast <client::core_t *> (this->core)->close(this->aid);
			// Восстанавливаем предыдущее значение флага
			this->worker.alive = alive;
		}
		// Если функция обратного вызова установлена, выполняем
		if(this->activeFn != nullptr) this->activeFn(mode_t::DISCONNECT, this, this->ctx.at(0));
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket::pause() noexcept {
	// Ставим работу клиента на паузу
	this->freeze = true;
}
/**
 * start Метод запуска клиента
 */
void awh::client::WebSocket::start() noexcept {
	// Если адрес URL запроса передан
	if(!this->freeze && !this->worker.url.empty()){
		// Если биндинг не запущен, выполняем запуск биндинга
		if(!this->core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->core)->start();
		// Выполняем запрос на сервер
		else const_cast <client::core_t *> (this->core)->open(this->worker.wid);
	}
	// Снимаем с паузы клиент
	this->freeze = false;
}
/**
 * getSub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::client::WebSocket::getSub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->http.getSub();
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::client::WebSocket::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->http.setSub(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::client::WebSocket::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->http.setSubs(subs);
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::client::WebSocket::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::client::WebSocket::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::WebSocket::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->noinfo = (flag & (uint8_t) flag_t::NOINFO);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->takeOverCli = (flag & (uint8_t) flag_t::TAKEOVERCLI);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->takeOverSrv = (flag & (uint8_t) flag_t::TAKEOVERSRV);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <client::core_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->core)->setVerifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri параметры прокси-сервера
 */
void awh::client::WebSocket::setProxy(const string & uri) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем параметры прокси-сервера
		this->worker.proxy.url = this->uri.parseUrl(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->worker.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->worker.proxy.url.schema.compare("socks5") == 0){
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->worker.proxy.socks5.setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if((this->worker.proxy.url.schema.compare("http") == 0) || (this->worker.proxy.url.schema.compare("https") == 0)) {
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->worker.proxy.http.setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
			}
		}
	}
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http.setChunkSize(size);
}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket::setFrameSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->frameSize = size;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->http.setUserAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->worker.proxy.http.setUserAgent(userAgent);
	}
}
/**
 * setCompress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::WebSocket::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->compress = compress;
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->http.setUser(login, password);
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http.setServ(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->worker.proxy.http.setServ(id, name, ver);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::client::WebSocket::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash.setAES(aes);
	// Устанавливаем соль шифрования
	this->hash.setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash.setPassword(pass);
	// Устанавливаем параметры шифрования
	if(this->crypt) this->http.setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WebSocket::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->http.setAuthType(type, hash);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WebSocket::setAuthTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->worker.proxy.http.setAuthType(type, hash);
}
/**
 * WebSocket Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket::WebSocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), mode(mode_t::DISCONNECT), frame(fmk, log), hash(fmk, log), http(fmk, log, &uri), worker(fmk, log), fmk(fmk), log(log), core(core) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readCallback;
	// Устанавливаем функцию записи данных
	this->worker.writeFn = writeCallback;
	// Устанавливаем функцию персистентного вызова
	this->worker.persistFn = persistCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectCallback;
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->worker.readProxyFn = readProxyCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectCallback;
	// Устанавливаем событие на подключение к прокси-серверу
	this->worker.connectProxyFn = connectProxyCallback;
	// Активируем персистентный запуск для работы пингов
	const_cast <client::core_t *> (this->core)->setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	const_cast <client::core_t *> (this->core)->add(&this->worker);
}
